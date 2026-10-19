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
 std::vector< std::uint8_t > out( nc + 4 * n + 32 );   // room for stores
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
 // 8 integers a step, their lengths by saturations with no table: each
 // byte not 0 made 1 (unsigned minimum with 1), each half of 16 bits made
 // 0, 1 or 255 as its bytes are 0, its high byte is 0 or not (saturated
 // pack), the pair of an integer made 0x0101 if its high half is 1 and its
 // low one 255 (signed minimum), then 0x7F00 added with saturation, so that
 // the high bits of its 2 bytes are its length less 1 (0x7F00 , 0x7F01 ->
 // 0; 0x7FFF -> 1; 0x8000 , 0x8001 -> 2; 0xFFFF -> 3): a mask of the 16
 // bytes is the 2 control bytes; then the bytes moved together by the
 // shuffle of each control byte
 const __m128i one = _mm_set1_epi8( 1 ) , cap = _mm_set1_epi16( 0x0101 ) ,
               lift = _mm_set1_epi16( 0x7F00 );
 for( ; i + 8 <= n ; i += 8 ) {
  const __m128i v0 = _mm_loadu_si128(
                      reinterpret_cast< const __m128i * >( x.data() + i ) );
  const __m128i v1 = _mm_loadu_si128(
                      reinterpret_cast< const __m128i * >( x.data() + i ) + 1 );
  const __m128i t = _mm_min_epi16( _mm_packus_epi16(
                                    _mm_min_epu8( v0 , one ) ,
                                    _mm_min_epu8( v1 , one ) ) , cap );
  const unsigned key = unsigned( _mm_movemask_epi8(
                                  _mm_adds_epu16( t , lift ) ) );
  storeLE< std::uint16_t >( out.data() + i / 4 , std::uint16_t( key ) );
  auto store = [ & ]( __m128i v , unsigned c ) {
   _mm_storeu_si128( reinterpret_cast< __m128i * >( d ) ,
                     _mm_shuffle_epi8( v , _mm_loadu_si128(
                      reinterpret_cast< const __m128i * >(
                       kVByte.pack[ c ].data() ) ) ) );
   d += kVByte.len[ c ];
   };
  store( v0 , key & 0xFF );
  store( v1 , key >> 8 );
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
 // 4 integers a step by the shuffle of their control byte: 32 at a time
 // while 12 more follow, whose bytes leave room for the loads of 16 bytes,
 // with no test on the data, then 4 at a time while 16 bytes are left
 auto step = [ & ]( std::size_t j , unsigned c ) {
  const __m128i v = _mm_loadu_si128(
                     reinterpret_cast< const __m128i * >( d ) );
  const __m128i m = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                      kVByte.shuffle[ c ].data() ) );
  _mm_storeu_si128( reinterpret_cast< __m128i * >( x.data() + j ) ,
                    _mm_shuffle_epi8( v , m ) );
  d += kVByte.len[ c ];
  };
 for( ; i + 32 + 12 <= n ; i += 32 ) {
  const std::uint64_t keys = loadLE< std::uint64_t >( code.data() + i / 4 );
  for( unsigned g = 0 ; g < 8 ; ++g )
   step( i + 4 * g , unsigned( keys >> ( 8 * g ) ) & 0xFF );
  }
 for( ; ( i + 4 <= n ) && ( end - d >= 16 ) ; i += 4 )
  step( i , code[ i / 4 ] );
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
