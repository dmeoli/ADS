/*--------------------------------------------------------------------------*/
/*------------------------- File StreamVByte.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of StreamVByte.h, with their contracts.
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

/// for each control byte, the sum of its 4 lengths, the mask of the
/// shuffle that moves the bytes of its 4 integers to their places, and the
/// mask of the one that moves them back together
struct VByteTables {
 std::array< std::uint8_t , 256 > len;
 std::array< std::array< std::uint8_t , 16 > , 256 > shuffle , pack;
 };

constexpr VByteTables vbyteTables( void )
{
 VByteTables t{};
 for( int c = 0 ; c < 256 ; ++c ) {
  int at = 0;
  for( int k = 0 ; k < 16 ; ++k ) t.pack[ c ][ k ] = 0x80;
  for( int j = 0 ; j < 4 ; ++j ) {
   const int l = ( ( c >> ( 2 * j ) ) & 3 ) + 1;
   for( int k = 0 ; k < 4 ; ++k ) {
    t.shuffle[ c ][ 4 * j + k ] = std::uint8_t( k < l ? at + k : 0x80 );
    if( k < l ) t.pack[ c ][ at + k ] = std::uint8_t( 4 * j + k );
    }
   at += l;
   }
  t.len[ c ] = std::uint8_t( at );
  }
 return( t );
 }

inline constexpr VByteTables kVByte = vbyteTables();

}  // end( namespace detail )

inline std::vector< std::uint8_t > streamVByteEncode(
                                          std::span< const std::uint32_t > x )
{
 using namespace detail;
 const std::size_t n = x.size() , nc = ( n + 3 ) / 4;
 std::vector< std::uint8_t > out( nc + 4 * n + 16 );   // room for a store
 std::uint8_t * d = out.data() + nc;
 // each integer stored in 4 bytes, the place moved by its length: the
 // bytes less 1, 0 for v < 2^8, ..., 3 for v >= 2^24
 auto put = [ & ]( std::uint32_t v ) {
  const std::uint32_t l = std::uint32_t( std::bit_width( v | 1 ) - 1 ) >> 3;
  storeLE< std::uint32_t >( d , v );
  d += l + 1;
  return( l );
  };
 std::size_t i = 0;
#if defined( __SSE4_1__ )
 // 4 integers a step: v <= 2^( 8k ) - 1 for k = 1 , 2 , 3 by the unsigned
 // minimum, the lengths less 1 as the number of the failed tests, packed in
 // the control byte by shifts, and the bytes moved together by a shuffle
 const __m128i b1 = _mm_set1_epi32( 0xFF ) , b2 = _mm_set1_epi32( 0xFFFF ) ,
               b3 = _mm_set1_epi32( 0xFFFFFF ) , three = _mm_set1_epi32( 3 );
 for( ; i + 4 <= n ; i += 4 ) {
  const __m128i v = _mm_loadu_si128(
                     reinterpret_cast< const __m128i * >( x.data() + i ) );
  const __m128i le = _mm_add_epi32(
                      _mm_add_epi32( _mm_cmpeq_epi32( _mm_min_epu32( v , b1 ) ,
                                                      v ) ,
                                     _mm_cmpeq_epi32( _mm_min_epu32( v , b2 ) ,
                                                      v ) ) ,
                      _mm_cmpeq_epi32( _mm_min_epu32( v , b3 ) , v ) );
  // the lengths less 1, one per byte of the low word
  const __m128i l = _mm_add_epi32( three , le );
  const std::uint32_t w = std::uint32_t( _mm_cvtsi128_si32(
                           _mm_packus_epi16( _mm_packus_epi32( l , l ) ,
                                             _mm_setzero_si128() ) ) );
  const std::uint32_t c = ( w & 3 ) | ( ( w >> 6 ) & 0xC ) |
                          ( ( w >> 12 ) & 0x30 ) | ( ( w >> 18 ) & 0xC0 );
  out[ i / 4 ] = std::uint8_t( c );
  _mm_storeu_si128( reinterpret_cast< __m128i * >( d ) ,
                    _mm_shuffle_epi8( v , _mm_loadu_si128(
                     reinterpret_cast< const __m128i * >(
                      kVByte.pack[ c ].data() ) ) ) );
  d += kVByte.len[ c ];
  }
#endif
 for( ; i + 4 <= n ; i += 4 ) {
  const std::uint32_t l0 = put( x[ i ] ) , l1 = put( x[ i + 1 ] ) ,
                      l2 = put( x[ i + 2 ] ) , l3 = put( x[ i + 3 ] );
  out[ i / 4 ] = std::uint8_t( l0 | ( l1 << 2 ) | ( l2 << 4 ) | ( l3 << 6 ) );
  }
 if( i < n ) {
  std::uint32_t c = 0;
  for( std::size_t j = 0 ; i + j < n ; ++j )
   c |= put( x[ i + j ] ) << ( 2 * j );
  out[ i / 4 ] = std::uint8_t( c );
  }
 out.resize( std::size_t( d - out.data() ) );
 return( out );
 }

inline std::vector< std::uint32_t > streamVByteDecode(
                         std::span< const std::uint8_t > code , std::size_t n )
{
 using namespace detail;
 const std::size_t nc = ( n + 3 ) / 4;
 ADS_PRE( code.size() >= nc + n );
 std::vector< std::uint32_t > x( n );
 const std::uint8_t * d = code.data() + nc;
 const std::uint8_t * const end = code.data() + code.size();
 std::size_t i = 0;
#if defined( __SSSE3__ )
 // 4 integers a step, as long as 16 bytes can be loaded, and 8 of them
 // while two steps fit for sure (16 bytes a step at most)
 auto step = [ & ]( std::size_t j ) {
  const std::uint8_t c = code[ j / 4 ];
  const __m128i v = _mm_loadu_si128(
                     reinterpret_cast< const __m128i * >( d ) );
  const __m128i m = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                      kVByte.shuffle[ c ].data() ) );
  _mm_storeu_si128( reinterpret_cast< __m128i * >( x.data() + j ) ,
                    _mm_shuffle_epi8( v , m ) );
  d += kVByte.len[ c ];
  };
 for( ; ( i + 8 <= n ) && ( end - d >= 32 ) ; i += 8 ) {
  step( i );
  step( i + 4 );
  }
 for( ; ( i + 4 <= n ) && ( end - d >= 16 ) ; i += 4 ) step( i );
#endif
 for( ; i < n ; ++i ) {                     // an integer at a time
  const std::uint32_t l = ( ( code[ i / 4 ] >> ( 2 * ( i % 4 ) ) ) & 3 ) + 1;
  ADS_PRE( d + l <= end );
  std::uint32_t v = 0;
  for( std::uint32_t k = 0 ; k < l ; ++k )
   v |= std::uint32_t( d[ k ] ) << ( 8 * k );
  x[ i ] = v;
  d += l;
  }
 ADS_PRE( d == end );
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File StreamVByte.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
