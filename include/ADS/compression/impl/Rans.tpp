/*--------------------------------------------------------------------------*/
/*------------------------------ File Rans.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Rans.h, with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/Unaligned.h"

#include <memory>

namespace ADS {

namespace detail {

constexpr int kRansScale = 12;                        ///< M = 2^12
constexpr std::uint32_t kRansLow = 1u << 16;          ///< the least state
constexpr std::size_t kRansWays = 4;                  ///< the states
constexpr std::size_t kRansHead = 8 + 512 + 4 * kRansWays;   ///< the header

}  // end( namespace detail )

inline std::array< std::uint32_t , 256 > scaleFrequencies(
                       const std::array< std::uint64_t , 256 > & f , int b )
{
 std::uint64_t total = 0;
 std::size_t used = 0;
 for( const auto x : f ) {
  total += x;
  used += x > 0;
  }
 const std::uint64_t M = std::uint64_t( 1 ) << b;
 ADS_PRE( ( total > 0 ) && ( used <= M ) );
 std::array< std::uint32_t , 256 > g{};
 std::int64_t sum = 0;
 for( int s = 0 ; s < 256 ; ++s )
  if( f[ s ] > 0 ) {
   g[ s ] = std::uint32_t( std::max< std::uint64_t >(
             1 , ( f[ s ] * M + total / 2 ) / total ) );
   sum += g[ s ];
   }
 // the difference to M, taken from or given to the largest ones
 while( sum != std::int64_t( M ) ) {
  const int s = int( std::max_element( g.begin() , g.end() ) - g.begin() );
  if( sum < std::int64_t( M ) ) {
   g[ s ] += std::uint32_t( std::int64_t( M ) - sum );
   sum = std::int64_t( M );
   }
  else {
   const auto d = std::min< std::int64_t >( sum - std::int64_t( M ) ,
                                            std::int64_t( g[ s ] ) / 2 );
   g[ s ] -= std::uint32_t( std::max< std::int64_t >( d , 1 ) );
   sum -= std::max< std::int64_t >( d , 1 );
   }
  }
 return( g );
 }

inline std::vector< std::uint8_t > ransCompress(
                                          std::span< const std::uint8_t > in )
{
 using namespace detail;
 const std::size_t n = in.size();
 std::vector< std::uint8_t > out( kRansHead , 0 );
 storeLE< std::uint64_t >( out.data() , n );
 if( n == 0 ) {
  out.resize( out.size() + 8 , 0 );
  return( out );
  }
 const auto g = scaleFrequencies( byteHistogram( in ) , kRansScale );
 std::array< std::uint32_t , 256 > start{};
 for( int s = 1 ; s < 256 ; ++s ) start[ s ] = start[ s - 1 ] + g[ s - 1 ];
 for( int s = 0 ; s < 256 ; ++s )
  storeLE< std::uint16_t >( out.data() + 8 + 2 * s , std::uint16_t( g[ s ] ) );
 // the division of the state by a frequency f as a multiplication by a
 // reciprocal: q = floor( x / f ) = ( x r ) >> ( 32 + t ), with
 // t = ceil( log2 f ) - 1 and r = ceil( 2^( 32 + t ) / f ) (exact for the
 // states below 2^32 and f < 2^12), and C( s , x ) = x + c + q ( M - f );
 // f = 1 has q = x, by r = 2^32 - 1, t = 0 and c + M - 1 in place of c
 struct Sym { std::uint32_t max , rcp , shift , bias , cmpl; };
 std::array< Sym , 256 > Y{};
 for( int s = 0 ; s < 256 ; ++s )
  if( const std::uint32_t fs = g[ s ] ; fs > 0 ) {
   Sym & y = Y[ s ];
   y.max = ( ( kRansLow >> kRansScale ) << 16 ) * fs - 1;
   y.cmpl = ( 1u << kRansScale ) - fs;
   if( fs == 1 ) {
    y.rcp = ~0u;
    y.shift = 0;
    y.bias = start[ s ] + ( 1u << kRansScale ) - 1;
    }
   else {
    std::uint32_t t = 0;
    while( fs > ( 1u << t ) ) ++t;
    y.rcp = std::uint32_t( ( ( std::uint64_t( 1 ) << ( t + 31 ) ) + fs - 1 ) /
                           fs );
    y.shift = t - 1;
    y.bias = start[ s ];
    }
   }
 // from the last symbol to the first, the words in the reverse order
 // a word a symbol at most
 const auto words = std::make_unique_for_overwrite< std::uint16_t[] >( n + 1 );
 std::size_t nw = 0;
 std::array< std::uint32_t , kRansWays > x;
 x.fill( kRansLow );
 auto put = [ & ]( std::uint32_t & y , const Sym & z ) {
  const bool emit = y > z.max;              // with no branch
  words[ nw ] = std::uint16_t( y );
  nw += emit;
  y = emit ? y >> 16 : y;
  const std::uint32_t q = std::uint32_t( ( std::uint64_t( y ) * z.rcp ) >>
                                         32 ) >> z.shift;
  y += z.bias + q * z.cmpl;
  };
 std::size_t i = n;
 for( ; i % kRansWays ; --i )
  put( x[ ( i - 1 ) % kRansWays ] , Y[ in[ i - 1 ] ] );
 for( ; i > 0 ; i -= kRansWays )            // the states in registers
  for( std::size_t k = kRansWays ; k-- > 0 ; )
   put( x[ k ] , Y[ in[ i - kRansWays + k ] ] );
 for( std::size_t k = 0 ; k < kRansWays ; ++k )
  storeLE< std::uint32_t >( out.data() + 8 + 512 + 4 * k , x[ k ] );
 const std::size_t at = out.size();
 out.resize( at + 2 * nw + 8 , 0 );
 for( std::size_t j = 0 ; j < nw ; ++j )
  storeLE< std::uint16_t >( out.data() + at + 2 * j , words[ nw - 1 - j ] );
 ADS_AUDIT( std::ranges::equal( ransDecompress( out ) , in ) );
 return( out );
 }

inline std::vector< std::uint8_t > ransDecompress(
                                          std::span< const std::uint8_t > in )
{
 using namespace detail;
 ADS_PRE( in.size() >= kRansHead + 8 );
 const std::size_t n = std::size_t( loadLE< std::uint64_t >( in.data() ) );
 std::vector< std::uint8_t > out( n );
 if( n == 0 ) return( out );
 // the table of the slots: the frequency of the symbol in the low 16 bits
 // and the slot less its cumulative frequency in the high ones, and the
 // symbol apart
 std::array< std::uint32_t , 1 << kRansScale > T;
 std::array< std::uint8_t , 1 << kRansScale > Z;
 std::uint32_t c = 0;
 for( std::uint32_t s = 0 ; s < 256 ; ++s ) {
  const std::uint32_t g = loadLE< std::uint16_t >( in.data() + 8 + 2 * s );
  ADS_PRE( c + g <= ( 1u << kRansScale ) );
  for( std::uint32_t j = 0 ; j < g ; ++j ) {
   T[ c + j ] = g | ( j << 16 );
   Z[ c + j ] = std::uint8_t( s );
   }
  c += g;
  }
 ADS_PRE( c == ( 1u << kRansScale ) );
 std::array< std::uint32_t , kRansWays > x;
 for( std::size_t k = 0 ; k < kRansWays ; ++k ) {
  x[ k ] = loadLE< std::uint32_t >( in.data() + 8 + 512 + 4 * k );
  ADS_PRE( x[ k ] >= kRansLow );
  }
 const std::uint8_t * p = in.data() + kRansHead;
 const std::uint8_t * const end = in.data() + in.size() - 8;
 constexpr std::uint32_t mask = ( 1u << kRansScale ) - 1;
 auto step = [ & ]( std::uint32_t & y ) {   // a symbol, and the 16 bits
  const std::uint32_t slot = y & mask , e = T[ slot ];
  y = ( e & 0xFFFF ) * ( y >> kRansScale ) + ( e >> 16 );
  const bool r = y < kRansLow;
  const std::uint32_t v = loadLE< std::uint16_t >( p );
  y = r ? ( y << 16 ) | v : y;
  p += 2 * std::size_t( r );
  return( Z[ slot ] );
  };
 std::size_t i = 0;
 for( ; i + kRansWays <= n ; i += kRansWays )
  for( std::size_t k = 0 ; k < kRansWays ; ++k ) out[ i + k ] = step( x[ k ] );
 for( ; i < n ; ++i ) out[ i ] = step( x[ i % kRansWays ] );
 ADS_PRE( p <= end );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Rans.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
