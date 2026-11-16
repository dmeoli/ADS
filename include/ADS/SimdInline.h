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
 * translation units built with different flags get distinct functions;
 * the classes that call them out of that inline namespace (the B-trees,
 * the searches in a node) must be compiled with the same flags in all the
 * units of a program, as any inline code.
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
 #include <type_traits>

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

/// true if the vectors of Highway hold the items of T: the integers and
/// the floating types of 32 and 64 bits
template< class T >
inline constexpr bool kLanesOf =
  ( ( std::is_integral_v< T > && ! std::is_same_v< T , bool > ) ||
    std::is_same_v< T , float > || std::is_same_v< T , double > ) &&
  ( ( sizeof( T ) == 4 ) || ( sizeof( T ) == 8 ) );

/// the items of \f$ a[ 0 .. n ) \f$ smaller than \f$ x \f$ (not larger,
/// if `Up`), \f$ T \f$ of `kLanesOf`, a vector at a time
template< std::size_t n , bool Up , class T >
HWY_INLINE std::size_t countLess( const T * a , T x )
{
 using U = std::conditional_t<
   std::is_floating_point_v< T > , T ,
   std::conditional_t<
     std::is_signed_v< T > ,
     std::conditional_t< sizeof( T ) == 4 , std::int32_t , std::int64_t > ,
     std::conditional_t< sizeof( T ) == 4 , std::uint32_t ,
			 std::uint64_t > > >;
 const hn::ScalableTag< U > d;
 const std::size_t L = hn::Lanes( d );
 const U * b = reinterpret_cast< const U * >( a );
 const auto vx = hn::Set( d , U( x ) );
 std::size_t r = 0;
 for( std::size_t k = 0 ; k < n / L ; ++k ) {
  const auto v = hn::LoadU( d , b + k * L );
  if constexpr( Up )  // not x < a[ i ], as the scalar code on a NaN
   r += hn::CountTrue( d , hn::Not( hn::Lt( vx , v ) ) );
  else
   r += hn::CountTrue( d , hn::Lt( v , vx ) );
  }
 for( std::size_t i = n / L * L ; i < n ; ++i )
  if constexpr( Up )
   r += ! ( x < a[ i ] );
  else
   r += a[ i ] < x;
 return( r );
 }

/// the bits of the items of \f$ a[ 0 .. n ) \f$ larger than \f$ x \f$,
/// \f$ n \le 64 \f$, the bit \f$ q \f$ for \f$ a[ q ] \f$
template< std::size_t n , class T >
HWY_INLINE std::uint64_t greaterBits( const T * a , T x )
{
 using U = std::conditional_t<
   std::is_floating_point_v< T > , T ,
   std::conditional_t<
     std::is_signed_v< T > ,
     std::conditional_t< sizeof( T ) == 4 , std::int32_t , std::int64_t > ,
     std::conditional_t< sizeof( T ) == 4 , std::uint32_t ,
			 std::uint64_t > > >;
 const hn::CappedTag< U , 64 > d;
 const std::size_t L = hn::Lanes( d );
 const U * b = reinterpret_cast< const U * >( a );
 const auto vx = hn::Set( d , U( x ) );
 std::uint64_t r = 0;
 for( std::size_t k = 0 ; k < n / L ; ++k ) {
  const std::size_t i = k * L;
  std::uint8_t c[ 8 ] = {};
  hn::StoreMaskBits( d , hn::Lt( vx , hn::LoadU( d , b + i ) ) , c );
  std::uint64_t m = 0;  // the bit j in the bit j % 8 of the byte j / 8
  for( int j = 7 ; j >= 0 ; --j )
   m = ( m << 8 ) | c[ j ];
  r |= m << i;
  }
 for( std::size_t i = n / L * L ; i < n ; ++i )
  r |= std::uint64_t( x < a[ i ] ) << i;
 return( r );
 }

/// the bits \f$ j \f$, \f$ 0 \le j < W \le 64 \f$, of the bytes
/// \f$ p[ j ] \f$ equal to \f$ b \f$, \f$ p \f$ of any alignment
template< std::size_t W >
HWY_INLINE std::uint64_t bytesEqual( const std::uint8_t * p , std::uint8_t b )
{
 static_assert( W <= 64 , "the bits of a word" );
 const hn::CappedTag< std::uint8_t , W > d;
 const std::size_t L = hn::Lanes( d );
 const auto vb = hn::Set( d , b );
 std::uint64_t r = 0;
 for( std::size_t i = 0 ; i < W ; i += L ) {
  std::uint8_t c[ 8 ] = {};
  hn::StoreMaskBits( d , hn::Eq( hn::LoadU( d , p + i ) , vb ) , c );
  std::uint64_t m = 0;  // the bit j in the bit j % 8 of the byte j / 8
  for( int j = 7 ; j >= 0 ; --j )
   m = ( m << 8 ) | c[ j ];
  r |= m << i;
  }
 return( r );
 }

/// the bits \f$ j \f$, \f$ 0 \le j < W \le 64 \f$, of the bytes
/// \f$ p[ j ] < b \f$, unsigned
template< std::size_t W >
HWY_INLINE std::uint64_t bytesBelow( const std::uint8_t * p , std::uint8_t b )
{
 static_assert( W <= 64 , "the bits of a word" );
 const hn::CappedTag< std::uint8_t , W > d;
 const std::size_t L = hn::Lanes( d );
 const auto vb = hn::Set( d , b );
 std::uint64_t r = 0;
 for( std::size_t i = 0 ; i < W ; i += L ) {
  std::uint8_t c[ 8 ] = {};
  hn::StoreMaskBits( d , hn::Lt( hn::LoadU( d , p + i ) , vb ) , c );
  std::uint64_t m = 0;  // the bit j in the bit j % 8 of the byte j / 8
  for( int j = 7 ; j >= 0 ; --j )
   m = ( m << 8 ) | c[ j ];
  r |= m << i;
  }
 return( r );
 }

/// the first place \f$ j < W \f$ where the bytes of \f$ p \f$ and
/// \f$ q \f$ differ, \f$ W \f$ if none
template< std::size_t W >
HWY_INLINE std::size_t bytesMismatch( const std::uint8_t * p ,
				      const std::uint8_t * q )
{
 const hn::CappedTag< std::uint8_t , W > d;
 const std::size_t L = hn::Lanes( d );
 for( std::size_t i = 0 ; i < W ; i += L ) {
  const std::intptr_t f = hn::FindFirstTrue(
    d , hn::Ne( hn::LoadU( d , p + i ) , hn::LoadU( d , q + i ) ) );
  if( f >= 0 )
   return( i + std::size_t( f ) );
  }
 return( W );
 }

/// the 8 words of 32 bits of the bits of the hash \f$ h \f$ in a block of
/// a filter, one bit per word: \f$ m[ i ] = 2^{( h s_i ) \gg 27} \f$
/// for the 8 odd salts \f$ s \f$
HWY_INLINE void blockBits8( std::uint32_t h , const std::uint32_t * s ,
			    std::uint32_t * m )
{
 const hn::CappedTag< std::uint32_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 for( std::size_t i = 0 ; i < 8 ; i += L )
  hn::StoreU( hn::Shl( hn::Set( d , 1u ) ,
		       hn::ShiftRight< 27 >(
			 hn::Mul( hn::Set( d , h ) ,
				  hn::LoadU( d , s + i ) ) ) ) ,
	      d , m + i );
 }

/// true if all the bits of the 8 words of \f$ m \f$ are in the 8 words of
/// \f$ w \f$
HWY_INLINE bool blockHas8( const std::uint32_t * w , const std::uint32_t * m )
{
 const hn::CappedTag< std::uint32_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 auto miss = hn::Zero( d );
 for( std::size_t i = 0 ; i < 8 ; i += L )
  miss =
    hn::Or( miss ,
	    hn::AndNot( hn::LoadU( d , w + i ) , hn::LoadU( d , m + i ) ) );
 return( hn::AllTrue( d , hn::Eq( miss , hn::Zero( d ) ) ) );
 }

/// the bits of the 8 words of \f$ m \f$ set in the 8 words of \f$ w \f$
HWY_INLINE void blockSet8( std::uint32_t * w , const std::uint32_t * m )
{
 const hn::CappedTag< std::uint32_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 for( std::size_t i = 0 ; i < 8 ; i += L )
  hn::StoreU( hn::Or( hn::LoadU( d , w + i ) , hn::LoadU( d , m + i ) ) , d ,
	      w + i );
 }

/// the parities of \f$ ( u_j \wedge lo ) \oplus ( v_j \wedge hi ) \f$,
/// \f$ 0 \le j < 8 \f$, the bit \f$ j \f$ for the word \f$ j \f$
HWY_INLINE std::uint32_t parities8( const std::uint64_t * u ,
				    const std::uint64_t * v ,
				    std::uint64_t lo ,
				    std::uint64_t hi )
{
 const hn::CappedTag< std::uint64_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 const auto vl = hn::Set( d , lo ) , vh = hn::Set( d , hi );
 const auto one = hn::Set( d , std::uint64_t( 1 ) );
 std::uint32_t y = 0;
 for( std::size_t j = 0 ; j < 8 ; j += L ) {
  const auto x = hn::Xor( hn::And( hn::LoadU( d , u + j ) , vl ) ,
			  hn::And( hn::LoadU( d , v + j ) , vh ) );
  std::uint8_t c[ 8 ] = {};
  hn::StoreMaskBits(
    d , hn::Eq( hn::And( hn::PopulationCount( x ) , one ) , one ) , c );
  y |= std::uint32_t( c[ 0 ] ) << j;
  }
 return( y );
 }

/// the 1 bits of the 8 words of \f$ w \f$ below the bit \f$ p < 512 \f$
HWY_INLINE std::size_t lineRank512( const std::uint64_t * w , unsigned p )
{
 const hn::CappedTag< std::uint64_t , 8 > d;
 const hn::RebindToSigned< decltype( d ) > di;
 const std::size_t L = hn::Lanes( d );
 const auto ones = hn::Set( d , ~std::uint64_t( 0 ) );
 const auto all = hn::Set( d , std::uint64_t( 64 ) );
 auto acc = hn::Zero( d );
 for( std::size_t j = 0 ; j < 8 ; j += L ) {
  // the bits of the word i below p: clamp( p - 64 i , 0 , 64 )
  const auto c = hn::BitCast(
    d , hn::Min( hn::Max( hn::Sub( hn::Set( di , std::int64_t( p ) ) ,
				  hn::ShiftLeft< 6 >(
				    hn::Iota( di , std::int64_t( j ) ) ) ) ,
			 hn::Zero( di ) ) ,
		hn::Set( di , std::int64_t( 64 ) ) ) );
  const auto low = hn::IfThenElse(
    hn::Eq( c , all ) , ones ,
    hn::Not(
      hn::Shl( ones , hn::And( c , hn::Set( d , std::uint64_t( 63 ) ) ) ) ) );
  acc = hn::Add(
    acc , hn::PopulationCount( hn::And( hn::LoadU( d , w + j ) , low ) ) );
  }
 return( std::size_t( hn::GetLane( hn::SumOfLanes( d , acc ) ) ) );
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
