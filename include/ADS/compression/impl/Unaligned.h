/*--------------------------------------------------------------------------*/
/*---------------------------- File Unaligned.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The loads and the stores of little-endian integers at any address, which
 * the fast coders of this folder use [see HuffmanCoder.h, Rans.h]: one move
 * of the processor on the little-endian machines, and a turn of the bytes
 * on the others; and the histogram of a string of bytes, on four tables of
 * counts, so that a run of equal bytes does not wait for the store of the
 * count before.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Unaligned
 #define __ADS_Unaligned
                      /* self-identification: #endif constant */

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace ADS::detail {

/// x with its bytes turned around on a big-endian machine
template< class T >
constexpr T littleEndian( T x )
{
 if constexpr( std::endian::native == std::endian::big ) {
  T r = 0;
  for( unsigned i = 0 ; i < sizeof( T ) ; ++i , x >>= 8 )
   r = T( ( r << 8 ) | ( x & 0xFF ) );
  return( r );
  }
 return( x );
 }

/// the little-endian integer of type T at p
template< class T >
inline T loadLE( const std::uint8_t * p )
{
 T x;
 std::memcpy( &x , p , sizeof( T ) );
 return( littleEndian( x ) );
 }

/// writes x at p, little-endian
template< class T >
inline void storeLE( std::uint8_t * p , T x )
{
 x = littleEndian( x );
 std::memcpy( p , &x , sizeof( T ) );
 }

/// the number of each byte in s
inline std::array< std::uint64_t , 256 > byteHistogram(
                                           std::span< const std::uint8_t > s )
{
 std::array< std::array< std::uint32_t , 256 > , 4 > c{};
 std::array< std::uint64_t , 256 > f{};
 // chunks of 2^30 bytes, so that no count of 32 bits overflows, 8 bytes
 // of a load at a time
 for( std::size_t a = 0 ; a < s.size() ; a += std::size_t( 1 ) << 30 ) {
  const std::size_t b = std::min( s.size() ,
                                  a + ( std::size_t( 1 ) << 30 ) );
  std::size_t i = a;
  for( ; i + 8 <= b ; i += 8 ) {
   const std::uint64_t x = loadLE< std::uint64_t >( s.data() + i );
   ++c[ 0 ][ x & 0xFF ];
   ++c[ 1 ][ ( x >> 8 ) & 0xFF ];
   ++c[ 2 ][ ( x >> 16 ) & 0xFF ];
   ++c[ 3 ][ ( x >> 24 ) & 0xFF ];
   ++c[ 0 ][ ( x >> 32 ) & 0xFF ];
   ++c[ 1 ][ ( x >> 40 ) & 0xFF ];
   ++c[ 2 ][ ( x >> 48 ) & 0xFF ];
   ++c[ 3 ][ x >> 56 ];
   }
  for( ; i < b ; ++i ) ++c[ 0 ][ s[ i ] ];
  for( auto & t : c )
   for( int v = 0 ; v < 256 ; ++v ) {
    f[ v ] += t[ v ];
    t[ v ] = 0;
    }
  }
 return( f );
 }

}  // end( namespace ADS::detail )

#endif  /* Unaligned.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Unaligned.h --------------------------*/
/*--------------------------------------------------------------------------*/
