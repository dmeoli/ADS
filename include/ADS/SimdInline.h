/*--------------------------------------------------------------------------*/
/*--------------------------- File SimdInline.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector kernels of ADS that are inlined in the middle of a search, on
 * Google Highway, with `ADS_WITH_HIGHWAY`: a comparison of the 16 control
 * bytes of a group of a hash table with a fingerprint, the place of a byte
 * among the 16 keys of a node of a tree, a count of the keys below a key
 * in a block, where a call through the table of the kernels of Simd.h
 * would cost more than the kernel. They are compiled for the set of vector
 * instructions that the flags of the compiler give (the static target of
 * Highway, SSE2 on x86-64 and NEON on AArch64 by default), in the
 * namespace `ADS::detail::simd::HWY_NAMESPACE` of that set, which enters
 * the name of the configuration of the layouts [see Abi.h], so that two
 * translation units built with different flags get distinct functions.
 * Each kernel works on any number of lanes, one included, so that it
 * compiles for every target of Highway, the scalar ones too.
 *
 * Without `ADS_WITH_HIGHWAY` this header declares nothing, and the
 * structures run their broadword code.
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

#ifndef __ADS_SimdInline
 #define __ADS_SimdInline
/* self-identification: #endif constant */

#ifdef ADS_WITH_HIGHWAY

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

 #include "ADS/Simd.h"  // before hwy/highway.h, which it includes

 #include <hwy/highway.h>

 #include <cstddef>
 #include <cstdint>
 #include <cstring>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

HWY_BEFORE_NAMESPACE();

namespace ADS::detail::simd::HWY_NAMESPACE
{

namespace hn = hwy::HWY_NAMESPACE;

/// the tag of the vectors of at most 16 bytes of the target
using Bytes16 = hn::CappedTag< std::uint8_t , 16 >;

/// the bits \f$ i \f$, \f$ 0 \le i < L \f$, of the lanes \f$ i \f$ of the
/// mask \f$ m \f$ of the \f$ L \le 16 \f$ lanes of the tag \f$ d \f$
template< class D , class M >
HWY_INLINE std::uint32_t maskBits( D d , M m )
{
 std::uint8_t b[ 8 ] = {};
 hn::StoreMaskBits( d , m , b );
 return( std::uint32_t( b[ 0 ] ) | ( std::uint32_t( b[ 1 ] ) << 8 ) );
 }

/// the bits \f$ j \f$, \f$ 0 \le j < 16 \f$, of the bytes \f$ c[ j ] \f$
/// equal to \f$ t \f$, \f$ c \f$ aligned to 16
HWY_INLINE std::uint32_t bytesEqual16( const std::uint8_t * c ,
				       std::uint8_t t )
{
 const Bytes16 d;
 const std::size_t L = hn::Lanes( d );
 std::uint32_t m = 0;
 for( std::size_t i = 0 ; i < 16 ; i += L )
  m |= maskBits( d , hn::Eq( hn::Load( d , c + i ) , hn::Set( d , t ) ) ) << i;
 return( m );
 }

/// the 16 bytes at \f$ c \f$, aligned to 16, with the byte \f$ j \f$ set to
/// 0, by one store of the whole group, so that a load of the group that
/// follows is forwarded from it
HWY_INLINE void clearByte16( std::uint8_t * c , std::size_t j )
{
 const Bytes16 d;
 const std::size_t L = hn::Lanes( d );
 for( std::size_t i = 0 ; i < 16 ; i += L ) {
  const auto at = hn::Eq( hn::Iota( d , std::uint8_t( i ) ) ,
			  hn::Set( d , std::uint8_t( j ) ) );
  hn::Store( hn::IfThenZeroElse( at , hn::Load( d , c + i ) ) , d , c + i );
  }
 }

}  // end( namespace ADS::detail::simd::HWY_NAMESPACE )

HWY_AFTER_NAMESPACE();

namespace ADS::detail::simd
{

/// the kernels inlined at the set of the flags of the compiler
namespace here = HWY_NAMESPACE;

}  // end( namespace ADS::detail::simd )

#endif

/*--------------------------------------------------------------------------*/

#endif /* SimdInline.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SimdInline.h --------------------------*/
/*--------------------------------------------------------------------------*/
