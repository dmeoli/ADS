/*--------------------------------------------------------------------------*/
/*------------------------- File StreamVByte.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of StreamVByte.h, with their contracts.
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

#include <memory>

namespace ADS
{

namespace detail
{

/// for each control byte, the sum of its 4 lengths, the mask of the
/// shuffle that moves the bytes of its 4 integers to their places, and the
/// mask of the one that moves them back together
struct VByteTables
{
 std::array< std::uint8_t , 256 > len;
 std::array< std::array< std::uint8_t , 16 > , 256 > shuffle , pack;
 };

constexpr VByteTables vbyteTables( void )
{
 VByteTables t{};
 for( int c = 0 ; c < 256 ; ++c ) {
  int at = 0;
  for( int k = 0 ; k < 16 ; ++k )
   t.pack[ c ][ k ] = 0x80;
  for( int j = 0 ; j < 4 ; ++j ) {
   const int l = ( ( c >> ( 2 * j ) ) & 3 ) + 1;
   for( int k = 0 ; k < 4 ; ++k ) {
    t.shuffle[ c ][ 4 * j + k ] = std::uint8_t( k < l ? at + k : 0x80 );
    if( k < l )
     t.pack[ c ][ at + k ] = std::uint8_t( 4 * j + k );
    }
   at += l;
   }
  t.len[ c ] = std::uint8_t( at );
  }
 return( t );
 }

inline constexpr VByteTables kVByte = vbyteTables();

}  // end( namespace detail )

inline std::vector< std::uint8_t >
streamVByteEncode( std::span< const std::uint32_t > x )
{
 using namespace detail;
 const std::size_t n = x.size() , nc = ( n + 3 ) / 4;
 // room for the stores, not cleared, of which the output keeps the
 // bytes written
 const std::size_t room = nc + 4 * n + 32;
 const auto buf = std::make_unique_for_overwrite< std::uint8_t[] >( room );
 std::uint8_t * const base = buf.get();
 std::uint8_t * d = base + nc;
 // each integer stored in 4 bytes, the place moved by its length: the
 // bytes less 1, 0 for v < 2^8, ..., 3 for v >= 2^24
 auto put = [ & ]( std::uint32_t v ) {
  const std::uint32_t l = std::uint32_t( std::bit_width( v | 1 ) - 1 ) >> 3;
  storeLE< std::uint32_t >( d , v );
  d += l + 1;
  return( l );
  };
 std::size_t i = 0;
#ifdef ADS_WITH_HIGHWAY
 // 4 integers a step by the kernel of Highway [see Simd.h], on the
 // machines of little-endian order
 if constexpr( std::endian::native == std::endian::little )
  i = simd::vbyteEncode( x.data() , n , base , &d , kVByte.pack[ 0 ].data() ,
			 kVByte.len.data() );
#endif
 for( ; i + 4 <= n ; i += 4 ) {
  const std::uint32_t l0 = put( x[ i ] ) , l1 = put( x[ i + 1 ] ) ,
		      l2 = put( x[ i + 2 ] ) , l3 = put( x[ i + 3 ] );
  base[ i / 4 ] = std::uint8_t( l0 | ( l1 << 2 ) | ( l2 << 4 ) | ( l3 << 6 ) );
  }
 if( i < n ) {
  std::uint32_t c = 0;
  for( std::size_t j = 0 ; i + j < n ; ++j )
   c |= put( x[ i + j ] ) << ( 2 * j );
  base[ i / 4 ] = std::uint8_t( c );
  }
 return( std::vector< std::uint8_t >( base , d ) );
 }

inline std::vector< std::uint32_t >
streamVByteDecode( std::span< const std::uint8_t > code , std::size_t n )
{
 using namespace detail;
 const std::size_t nc = ( n + 3 ) / 4;
 ADS_PRE( code.size() >= nc + n );
 std::vector< std::uint32_t > x( n );
 const std::uint8_t * d = code.data() + nc;
 const std::uint8_t * const end = code.data() + code.size();
 std::size_t i = 0;
#ifdef ADS_WITH_HIGHWAY
 // 4 integers a step by the shuffle of their control byte, the kernel of
 // Highway [see Simd.h], on the machines of little-endian order
 if constexpr( std::endian::native == std::endian::little )
  i = simd::vbyteDecode( code.data() , &d , end , n , x.data() ,
			 kVByte.shuffle[ 0 ].data() , kVByte.len.data() );
#endif
 for( ; i < n ; ++i ) {  // an integer at a time
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
