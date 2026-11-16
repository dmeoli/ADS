/*--------------------------------------------------------------------------*/
/*------------------------------ File Simd.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector kernels of ADS on Google Highway, with `ADS_WITH_HIGHWAY`: the
 * loops over arrays that a structure of ADS hands over whole (the 1 bits
 * of an array of words, the intersection of two sorted arrays, the hash of
 * a long key), written once on the portable operations of Highway and
 * compiled for every set of vector instructions that Highway knows on the
 * machine of the compiler (SSE2 to AVX-512 on x86-64, NEON and SVE on
 * ARM, and the others), of which Highway calls, when the program runs, the
 * best that the processor has. The code of ADS then tests no processor,
 * compiler or system, and a program built without any flag of the machine
 * runs the instructions of the processor it finds. A kernel is reached by
 * one call through a table of pointers, which costs as much as a call that
 * is not inlined; the kernels that run on a few bytes in the middle of a
 * search (a group of control bytes of a hash table, a node of a tree) are
 * instead inlined at the instructions given to the compiler [see
 * SimdInline.h], where a call would cost more than the kernel.
 *
 * Without `ADS_WITH_HIGHWAY` this header declares nothing, and every
 * structure runs its scalar code, which gives the same results.
 *
 * Highway dispatches by re-including, for every set of instructions, the
 * file of the kernels ADS/impl/Simd.tpp, once per translation unit
 * (hwy/foreach_target.h may run once per unit, and before any include of
 * hwy/highway.h). Every header of ADS that includes Highway includes this
 * one first, so that the dispatch runs at the first of them; a unit that
 * includes a header of ADS with Highway may therefore neither include
 * hwy/highway.h before it nor run a dispatch of its own, and a program
 * that has kernels of its own on Highway keeps them in units that do not
 * include the structures of ADS.
 *
 * \see J. Wassenberg, J. Alakuijala, Highway: performance-portable, length-
 *      agnostic SIMD with runtime dispatch,
 *      https://github.com/google/highway
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Simd
 #define __ADS_Simd
/* self-identification: #endif constant */

#ifdef ADS_WITH_HIGHWAY

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

 #include <cstddef>
 #include <cstdint>
 #include <cstring>
 #include <type_traits>

// the prefetch of the kernels, defined in Prefetch.h, which may not be
// included before the dispatch, since nothing may include hwy/highway.h
// before hwy/foreach_target.h
namespace ADS
{
enum class Locality : int;
namespace detail
{
inline void prefetchLine( const void * p , Locality l ) noexcept;
}  // end( namespace detail )
}  // end( namespace ADS )

 #undef HWY_TARGET_INCLUDE
 #define HWY_TARGET_INCLUDE "ADS/impl/Simd.tpp"
 #include <hwy/foreach_target.h>  // IWYU pragma: keep

 #include <hwy/highway.h>

 #include "ADS/impl/Simd.tpp"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail::simd
{

HWY_EXPORT( popcountWords );
HWY_EXPORT( popcountAndWords );
HWY_EXPORT( combineWords );
HWY_EXPORT( blocksCommonI32 );
HWY_EXPORT( blocksCommonU32 );
HWY_EXPORT( windowsCommonI32 );
HWY_EXPORT( windowsCommonU32 );
HWY_EXPORT( intersectBlocks16 );
HWY_EXPORT( differenceBlocks16 );
HWY_EXPORT( unionBlocks16 );
HWY_EXPORT( hashStripes );
HWY_EXPORT( vbyteEncode );
HWY_EXPORT( vbyteDecode );
HWY_EXPORT( bpPack );
HWY_EXPORT( bpUnpack );

/// the 1 bits of the words \f$ w[ 0 .. n - 1 ] \f$
inline std::uint64_t popcount( const std::uint64_t * w , std::size_t n )
{
 return( HWY_DYNAMIC_DISPATCH( popcountWords )( w , n ) );
 }

/// the 1 bits of the words \f$ a[ i ] \wedge b[ i ] \f$, \f$ 0 \le i < n \f$
inline std::uint64_t popcountAnd( const std::uint64_t * a ,
				  const std::uint64_t * b , std::size_t n )
{
 return( HWY_DYNAMIC_DISPATCH( popcountAndWords )( a , b , n ) );
 }

/// \f$ c[ i ] = a[ i ] \circ b[ i ] \f$, \f$ 0 \le i < n \f$, for
/// \f$ \circ \f$ the operation \f$ o \f$: 0 or, 1 and, 2 and not, 3 xor;
/// the 1 bits of \f$ c \f$
inline std::uint64_t combine( const std::uint64_t * a ,
			      const std::uint64_t * b ,
			      std::uint64_t * c , std::size_t n , int o )
{
 return( HWY_DYNAMIC_DISPATCH( combineWords )( a , b , c , n , o ) );
 }

/// the common items of the sorted arrays \f$ a[ 0 .. n ) \f$ and
/// \f$ b[ 0 .. m ) \f$ of integers of 32 bits appended to \f$ out \f$ by
/// blocks while both have one, the places where the blocks stopped in
/// \f$ *i \f$ and \f$ *j \f$; the items written [see Sets.tpp]
template< class T >
inline std::size_t blocksCommon( const T * a , std::size_t n , const T * b ,
				 std::size_t m , T * out , std::size_t * i ,
				 std::size_t * j )
{
 if constexpr( std::is_signed_v< T > )
  return( HWY_DYNAMIC_DISPATCH( blocksCommonI32 )(
    reinterpret_cast< const std::int32_t * >( a ) , n ,
    reinterpret_cast< const std::int32_t * >( b ) , m ,
    reinterpret_cast< std::int32_t * >( out ) , i , j ) );
 else
  return( HWY_DYNAMIC_DISPATCH( blocksCommonU32 )(
    reinterpret_cast< const std::uint32_t * >( a ) , n ,
    reinterpret_cast< const std::uint32_t * >( b ) , m ,
    reinterpret_cast< std::uint32_t * >( out ) , i , j ) );
 }

/// the items of the sorted \f$ s[ 0 .. m ) \f$ also in the sorted
/// \f$ l[ 0 .. n ) \f$, integers of 32 bits, appended to \f$ out \f$ by
/// windows of 64 items of \f$ l \f$ while one is left, the items of
/// \f$ s \f$ and of \f$ l \f$ passed in \f$ *q \f$ and \f$ *p \f$; the
/// items written [see Sets.tpp]
template< class T >
inline std::size_t windowsCommon( const T * s , std::size_t m , const T * l ,
				  std::size_t n , T * out , std::size_t * q ,
				  std::size_t * p )
{
 if constexpr( std::is_signed_v< T > )
  return( HWY_DYNAMIC_DISPATCH( windowsCommonI32 )(
    reinterpret_cast< const std::int32_t * >( s ) , m ,
    reinterpret_cast< const std::int32_t * >( l ) , n ,
    reinterpret_cast< std::int32_t * >( out ) , q , p ) );
 else
  return( HWY_DYNAMIC_DISPATCH( windowsCommonU32 )(
    reinterpret_cast< const std::uint32_t * >( s ) , m ,
    reinterpret_cast< const std::uint32_t * >( l ) , n ,
    reinterpret_cast< std::uint32_t * >( out ) , q , p ) );
 }

/// the items common to the sorted \f$ a[ 0 .. na ) \f$ and
/// \f$ b[ 0 .. nb ) \f$ of 16 bits appended to \f$ out \f$ by blocks from
/// \f$ *i \f$ and \f$ *j \f$, where the blocks stopped left in them; the
/// items written [see Sets.tpp]
inline std::size_t intersectBlocks16( const std::uint16_t * a ,
				      std::size_t na ,
				      const std::uint16_t * b ,
				      std::size_t nb ,
				      std::uint16_t * out , std::size_t * i ,
				      std::size_t * j )
{
 return(
   HWY_DYNAMIC_DISPATCH( intersectBlocks16 )( a , na , b , nb , out , i ,
					      j ) );
 }

/// the items of the sorted \f$ a[ 0 .. na ) \f$ of 16 bits not in the
/// sorted \f$ b[ 0 .. nb ) \f$ appended to \f$ out \f$ by blocks, as
/// intersectBlocks16(), the bits of the items of the block at \f$ *i \f$
/// found so far in \f$ *found \f$; the items written [see Sets.tpp]
inline std::size_t differenceBlocks16( const std::uint16_t * a ,
				       std::size_t na ,
				       const std::uint16_t * b ,
				       std::size_t nb ,
				       std::uint16_t * out , std::size_t * i ,
				       std::size_t * j , unsigned * found )
{
 return( HWY_DYNAMIC_DISPATCH( differenceBlocks16 )( a , na , b , nb , out ,
						     i , j ,
						      found ) );
 }

/// the items of the sorted \f$ a[ 0 .. na ) \f$ or \f$ b[ 0 .. nb ) \f$
/// of 16 bits, each once, written to \f$ out \f$ (room for
/// \f$ na + nb + 8 \f$), \f$ na , nb \ge 8 \f$; their number [see
/// Sets.tpp]
inline std::size_t unionBlocks16( const std::uint16_t * a , std::size_t na ,
				  const std::uint16_t * b , std::size_t nb ,
				  std::uint16_t * out )
{
 return( HWY_DYNAMIC_DISPATCH( unionBlocks16 )( a , na , b , nb , out ) );
 }

/// the 8 lanes of the stripes of 64 bytes of \f$ s[ 0 .. n ) \f$,
/// \f$ n > 64 \f$, with the seed and the salts \f$ k \f$ of the hash of
/// FastHash.tpp, into \f$ out \f$ [see Hash.tpp]
inline void hashStripes( const unsigned char * s , std::size_t n ,
			 std::uint64_t seed , const std::uint64_t * k ,
			 std::uint64_t * out )
{
 HWY_DYNAMIC_DISPATCH( hashStripes )( s , n , seed , k , out );
 }

/// the integers \f$ x[ 0 .. i ) \f$ of Stream VByte encoded 4 at a time,
/// \f$ i \f$ the largest multiple of 4 not above \f$ n \f$, their control
/// bytes in \f$ keys \f$ and their bytes from \f$ *d \f$, by the tables
/// of StreamVByte.tpp; \f$ i \f$ [see Compression.tpp]
inline std::size_t vbyteEncode( const std::uint32_t * x , std::size_t n ,
				std::uint8_t * keys , std::uint8_t ** d ,
				const std::uint8_t * pack ,
				const std::uint8_t * len )
{
 return( HWY_DYNAMIC_DISPATCH( vbyteEncode )( x , n , keys , d , pack ,
					      len ) );
 }

/// the integers \f$ x[ 0 .. i ) \f$ of Stream VByte decoded from their
/// control bytes \f$ keys \f$ and their bytes from \f$ *d \f$, before
/// \f$ end \f$, by the tables of StreamVByte.tpp; \f$ i \f$ [see
/// Compression.tpp]
inline std::size_t
vbyteDecode( const std::uint8_t * keys , const std::uint8_t ** d ,
	     const std::uint8_t * end , std::size_t n , std::uint32_t * x ,
	     const std::uint8_t * shuffle , const std::uint8_t * len )
{
 return(
   HWY_DYNAMIC_DISPATCH( vbyteDecode )( keys , d , end , n , x , shuffle ,
					len ) );
 }

/// the 128 integers of \f$ in \f$, below \f$ 2^b \f$, packed into
/// \f$ 16 b \f$ bytes at \f$ out \f$ in the layout of 4 lanes of
/// BinaryPacking.h [see Compression.tpp]
inline void bpPack( const std::uint32_t * in , std::uint8_t * out , int b )
{
 HWY_DYNAMIC_DISPATCH( bpPack )( in , out , b );
 }

/// the 128 integers of \f$ b \f$ bits at \f$ in \f$ unpacked to
/// \f$ out \f$, each one plus the one 4 places before it if sorted,
/// \f$ prev \f$ the last 4 before the block, which become the last 4 of
/// it [see Compression.tpp]
inline void bpUnpack( const std::uint8_t * in , std::uint32_t * out , int b ,
		      bool sorted , std::uint32_t * prev )
{
 HWY_DYNAMIC_DISPATCH( bpUnpack )( in , out , b , sorted , prev );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::detail::simd )

 #include "ADS/memory/Prefetch.h"  // the prefetch of the kernels

#endif

/*--------------------------------------------------------------------------*/

#endif /* Simd.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Simd.h -----------------------------*/
/*--------------------------------------------------------------------------*/
