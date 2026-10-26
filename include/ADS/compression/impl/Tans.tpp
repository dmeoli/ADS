/*--------------------------------------------------------------------------*/
/*------------------------------ File Tans.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Tans.h, with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/Unaligned.h"

namespace ADS {

namespace detail {

inline constexpr int kTansLog = 11;                   ///< R
inline constexpr std::uint32_t kTansStates =
 1u << kTansLog;                                      ///< L
inline constexpr std::size_t kTansWays = 4;           ///< the states
inline constexpr std::size_t kTansStreams = 2;        ///< the streams
inline constexpr std::size_t kTansHead =
 8 + 512 + 8 * kTansStreams;                          ///< header

/// the symbol of each state, dealt by the step L / 2 + L / 8 + 3
inline std::array< std::uint8_t , kTansStates > tansSpread(
                               const std::array< std::uint32_t , 256 > & f )
{
 std::array< std::uint8_t , kTansStates > sym{};
 constexpr std::uint32_t step = ( kTansStates >> 1 ) +
                                ( kTansStates >> 3 ) + 3;
 std::uint32_t pos = 0;
 for( std::uint32_t s = 0 ; s < 256 ; ++s )
  for( std::uint32_t i = 0 ; i < f[ s ] ; ++i ) {
   sym[ pos ] = std::uint8_t( s );
   pos = ( pos + step ) & ( kTansStates - 1 );
   }
 return( sym );
 }

}  // end( namespace detail )

inline std::vector< std::uint8_t > tansCompress(
                                          std::span< const std::uint8_t > in )
{
 using namespace detail;
 constexpr std::uint32_t L = kTansStates;
 const std::size_t n = in.size();
 std::vector< std::uint8_t > out( kTansHead , 0 );
 storeLE< std::uint64_t >( out.data() , n );
 if( n == 0 ) {
  out.resize( out.size() + 8 , 0 );
  return( out );
  }
 const auto f = scaleFrequencies( byteHistogram( in ) , kTansLog );
 for( int s = 0 ; s < 256 ; ++s )
  storeLE< std::uint16_t >( out.data() + 8 + 2 * s , std::uint16_t( f[ s ] ) );
 // the next state of each symbol and place: L plus the state of its i-th
 // place, the places of s from cum_s on; and the bits of a symbol in a
 // state X, ( X + delta_s ) >> 16, which bring X >> k into [ f_s , 2 f_s )
 const auto sym = tansSpread( f );
 std::array< std::uint32_t , 256 > cum{} , fill{};
 for( int s = 1 ; s < 256 ; ++s ) cum[ s ] = cum[ s - 1 ] + f[ s - 1 ];
 std::array< std::uint16_t , L > next;
 for( std::uint32_t x = 0 ; x < L ; ++x )
  next[ cum[ sym[ x ] ] + fill[ sym[ x ] ]++ ] = std::uint16_t( L + x );
 struct Sym { std::uint32_t delta; std::int32_t find; };
 std::array< Sym , 256 > Y{};
 for( int s = 0 ; s < 256 ; ++s ) {
  if( f[ s ] == 0 ) continue;
  if( f[ s ] == 1 )
   Y[ s ].delta = ( std::uint32_t( kTansLog ) << 16 ) - L;
  else {
   const std::uint32_t mb = std::uint32_t( kTansLog ) -
                            std::uint32_t( std::bit_width( f[ s ] - 1 ) - 1 );
   Y[ s ].delta = ( mb << 16 ) - ( f[ s ] << mb );
   }
  Y[ s ].find = std::int32_t( cum[ s ] ) - std::int32_t( f[ s ] );
  }
 // the string in 2 halves (the first one a multiple of 4), each in a
 // stream of its own; in a stream, from the last symbol to the first, the
 // bits of each from the low bit of a word, written 8 bytes at a time, and
 // the states at the end
 const std::size_t h = ( n / 2 ) & ~std::size_t( 3 );
 out.resize( out.size() + ( n * kTansLog ) / 8 + 64 * kTansStreams );
 std::uint8_t * p = out.data() + kTansHead;
 for( std::size_t g = 0 ; g < kTansStreams ; ++g ) {
  const std::size_t a = g == 0 ? 0 : h , b = g == 0 ? h : n;
  std::uint64_t acc = 0;
  unsigned bits = 0;
  std::size_t total = 0;
  auto flush = [ & ]( void ) {
   storeLE< std::uint64_t >( p , acc );
   p += bits / 8;
   total += 8 * ( bits / 8 );
   acc = bits >= 64 ? 0 : acc >> ( 8 * ( bits / 8 ) );
   bits %= 8;
   };
  std::array< std::uint32_t , kTansWays > X;
  X.fill( L );
  auto put = [ & ]( std::uint32_t & x , std::uint8_t s ) {
   const Sym & y = Y[ s ];
   const unsigned k = ( x + y.delta ) >> 16;
   acc |= std::uint64_t( x & ( ( 1u << k ) - 1 ) ) << bits;
   bits += k;
   x = next[ std::size_t( std::int32_t( x >> k ) + y.find ) ];
   };
  std::size_t i = b - a;
  for( ; i % kTansWays ; --i ) {
   put( X[ ( i - 1 ) % kTansWays ] , in[ a + i - 1 ] );
   flush();
   }
  for( ; i > 0 ; i -= kTansWays ) {         // 4 symbols of 11 bits at most
   for( std::size_t k = kTansWays ; k-- > 0 ; )
    put( X[ k ] , in[ a + i - kTansWays + k ] );
   flush();
   }
  for( std::size_t k = 0 ; k < kTansWays ; ++k ) {
   acc |= std::uint64_t( X[ k ] - L ) << bits;
   bits += kTansLog;
   flush();
   }
  const std::size_t all = total + bits;    // the last byte, if partial
  bits += 7;
  flush();
  storeLE< std::uint64_t >( out.data() + 8 + 512 + 8 * g , all );
  }
 out.resize( std::size_t( p - out.data() ) + 8 );
 std::fill( out.end() - 8 , out.end() , 0 );
 ADS_AUDIT( std::ranges::equal( tansDecompress( out ) , in ) );
 return( out );
 }

inline std::vector< std::uint8_t > tansDecompress(
                                          std::span< const std::uint8_t > in )
{
 using namespace detail;
 constexpr std::uint32_t L = kTansStates;
 ADS_PRE( in.size() >= kTansHead + 8 );
 const std::size_t n = std::size_t( loadLE< std::uint64_t >( in.data() ) );
 std::vector< std::uint8_t > out( n );
 if( n == 0 ) return( out );
 std::array< std::uint32_t , 256 > f;
 std::uint32_t sum = 0;
 for( int s = 0 ; s < 256 ; ++s ) {
  f[ s ] = loadLE< std::uint16_t >( in.data() + 8 + 2 * s );
  sum += f[ s ];
  }
 ADS_PRE( sum == L );
 std::array< std::size_t , kTansStreams > total;
 std::size_t bytes = 0;
 for( std::size_t g = 0 ; g < kTansStreams ; ++g ) {
  total[ g ] = std::size_t( loadLE< std::uint64_t >( in.data() + 8 + 512 +
                                                     8 * g ) );
  bytes += ( total[ g ] + 7 ) / 8;
  ADS_PRE( total[ g ] >= kTansWays * kTansLog );
  }
 ADS_PRE( kTansHead + bytes + 8 <= in.size() );
 // the table of the states: the symbol, the bits to read and the base of
 // the next state, in 8 , 8 and 16 bits
 const auto sym = tansSpread( f );
 std::array< std::uint32_t , L > D;
 std::array< std::uint32_t , 256 > cnt = f;
 for( std::uint32_t x = 0 ; x < L ; ++x ) {
  const std::uint32_t s = sym[ x ] , nx = cnt[ s ]++;
  const std::uint32_t nb = std::uint32_t( kTansLog ) -
                           std::uint32_t( std::bit_width( nx ) - 1 );
  D[ x ] = s | ( nb << 8 ) | ( ( ( nx << nb ) - L ) << 16 );
  }
 // each stream read backward from its last bit: a word holds the 57 bits
 // or more below the place, refilled every 4 symbols (the bytes before the
 // stream pad the loads near its start); the two halves decoded together,
 // a round of 4 symbols of each in turn, so that their chains overlap
 struct Reader {
  const std::uint8_t * s0;
  std::ptrdiff_t left;                      // the place less 8 base
  std::uint64_t w = 0;
  std::ptrdiff_t base = 0;                  // the byte of the bit 0 of w
  void refill( void ) {
   const std::ptrdiff_t b = base + ( ( left - 57 ) >> 3 );
   left -= 8 * ( b - base );
   base = b;
   w = loadLE< std::uint64_t >( s0 + base );
   }
  std::uint32_t read( unsigned k ) {        // the k bits below the place
   left -= k;
   // left = 64 only for k = 0, whose mask is 0
   const std::uint64_t v = w >> ( unsigned( left ) & 63 );
#ifdef __BMI2__
   return( std::uint32_t( _bzhi_u64( v , k ) ) );
#else
   return( std::uint32_t( v & ( ( std::uint64_t( 1 ) << k ) - 1 ) ) );
#endif
   }
  [[nodiscard]] std::ptrdiff_t place( void ) const {
   return( 8 * base + left );
   }
  };
 const std::size_t h = ( n / 2 ) & ~std::size_t( 3 );
 std::array< Reader , kTansStreams > R{
  Reader{ in.data() + kTansHead , std::ptrdiff_t( total[ 0 ] ) } ,
  Reader{ in.data() + kTansHead + ( total[ 0 ] + 7 ) / 8 ,
          std::ptrdiff_t( total[ 1 ] ) } };
 std::array< std::array< std::uint32_t , kTansWays > , kTansStreams > x;
 for( std::size_t g = 0 ; g < kTansStreams ; ++g ) {
  R[ g ].refill();
  for( std::size_t k = kTansWays ; k-- > 0 ; )
   x[ g ][ k ] = R[ g ].read( kTansLog );
  R[ g ].refill();
  }
 auto step = [ & ]( Reader & r , std::uint32_t & y ) {   // a symbol
  const std::uint32_t e = D[ y ];
  y = ( e >> 16 ) + r.read( ( e >> 8 ) & 0xFF );
  return( std::uint8_t( e ) );
  };
 const std::size_t len[ 2 ] = { h , n - h };
 const std::size_t common = std::min( len[ 0 ] , len[ 1 ] ) &
                            ~std::size_t( 3 );
 std::size_t i = 0;
 for( ; i < common ; i += kTansWays ) {
  for( std::size_t g = 0 ; g < kTansStreams ; ++g ) {
   std::uint8_t * o = out.data() + ( g == 0 ? 0 : h ) + i;
   for( std::size_t k = 0 ; k < kTansWays ; ++k ) o[ k ] = step( R[ g ] ,
                                                                 x[ g ][ k ] );
   R[ g ].refill();
   }
  }
 for( std::size_t g = 0 ; g < kTansStreams ; ++g )   // the rest of each
  for( std::size_t j = i ; j < len[ g ] ; ++j ) {
   out[ ( g == 0 ? 0 : h ) + j ] = step( R[ g ] , x[ g ][ j % kTansWays ] );
   R[ g ].refill();
   }
 for( std::size_t g = 0 ; g < kTansStreams ; ++g )
  ADS_PRE( R[ g ].place() == 0 );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Tans.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
