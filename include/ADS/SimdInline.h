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
 * Without `ADS_WITH_HIGHWAY`, or with `ADS_SIMD_SCALAR` [see Simd.h], this
 * header declares nothing, and the structures run their broadword code.
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

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

// ADS_SIMD, and the dispatch before hwy/highway.h, which it includes
#include "ADS/Simd.h"

#if ADS_SIMD

 #include <hwy/highway.h>

 #include <array>
 #include <cstddef>
 #include <cstdint>
 #include <cstring>
 #include <limits>
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

/// the 1 bits of the 8 words of \f$ w \f$ below the bit \f$ p \le 512 \f$
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

/// the 8 words of 64 bits from the bit \f$ o < 64 \f$ of the 9 words at
/// \f$ w \f$, into \f$ x \f$: a shift of the same count on every lane
HWY_INLINE void shiftedWords8( const std::uint64_t * w , unsigned o ,
			       std::uint64_t * x )
{
 const hn::CappedTag< std::uint64_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 for( std::size_t j = 0 ; j < 8 ; j += L ) {
  const auto lo = hn::ShiftRightSame( hn::LoadU( d , w + j ) , int( o ) );
  const auto hi = hn::ShiftLeftSame(
    hn::ShiftLeft< 1 >( hn::LoadU( d , w + j + 1 ) ) , int( 63 - o ) );
  hn::StoreU( hn::Or( lo , hi ) , d , x + j );
  }
 }

/// whether `selectInWord512()` runs here: a target whose compress of bytes
/// is one instruction (AVX-512 VBMI2, the targets from AVX3_DL on), with 64
/// lanes of a byte; where the compress of bytes is emulated, the kernel is
/// slower than the broadword select [see `selectInWord()`]
inline constexpr bool kSelectInWord512 =
  ( HWY_TARGET <= HWY_AVX3_DL ) &&
  ( hn::MaxLanes( hn::CappedTag< std::uint8_t , 64 >() ) >= 64 );

/// the place of the \f$ k \f$-th 1 of the word \f$ x \f$, from 0, on 64
/// lanes of a byte: the indices of the lanes, compressed by the mask of
/// the bits of \f$ x \f$, hold the places of its 1s in order, and the lane
/// \f$ k \f$ goes to the lane 0 by a permutation of the bytes; it needs
/// `kSelectInWord512` and \f$ k < \f$ the 1s of \f$ x \f$
HWY_INLINE int selectInWord512( std::uint64_t x , int k )
{
 const hn::CappedTag< std::uint8_t , 64 > d;
 std::uint8_t b[ 8 ];  // the bit j in the bit j % 8 of the byte j / 8
 for( int j = 0 ; j < 8 ; ++j )
  b[ j ] = std::uint8_t( x >> ( 8 * j ) );
 const auto v = hn::Compress( hn::Iota( d , 0 ) , hn::LoadMaskBits( d , b ) );
 return( int( hn::GetLane( hn::TableLookupLanes(
   v , hn::IndicesFromVec( d , hn::Set( d , std::uint8_t( k ) ) ) ) ) ) );
 }

/// a step of the \f$ K \f$ lanes of a strip of the edit distance: the lane
/// \f$ q \f$ moves its block of 64 cells (the vertical differences +1 in
/// \f$ pv[ q ] \f$ and -1 in \f$ mv[ q ] \f$, the matches in
/// \f$ eq[ q ] \f$, the horizontal difference at its top in
/// \f$ cp[ q ] \f$ and \f$ cn[ q ] \f$, replaced by the one at its bottom)
/// by one column, as the step of Myers; if `Masked`, only the lanes whose
/// \f$ act[ q ] \f$ is all ones move, and the others keep their state
template< std::size_t K , bool Masked >
HWY_INLINE void myersLanes( std::uint64_t * pv , std::uint64_t * mv ,
			    const std::uint64_t * eq , std::uint64_t * cp ,
			    std::uint64_t * cn , const std::uint64_t * act )
{
 const hn::CappedTag< std::uint64_t , K > d;
 const std::size_t L = hn::Lanes( d );
 for( std::size_t i = 0 ; i < K ; i += L ) {
  const auto Pv = hn::LoadU( d , pv + i ) , Mv = hn::LoadU( d , mv + i ) ,
	     Cp = hn::LoadU( d , cp + i ) , Cn = hn::LoadU( d , cn + i ) ,
	     E = hn::LoadU( d , eq + i );
  const auto Xv = hn::Or( E , Mv ) , Eq = hn::Or( E , Cn );
  const auto Xh =
    hn::Or( hn::Xor( hn::Add( hn::And( Eq , Pv ) , Pv ) , Pv ) , Eq );
  const auto ph = hn::Or( Mv , hn::Not( hn::Or( Xh , Pv ) ) ) ,
	     mh = hn::And( Pv , Xh );
  const auto Ph = hn::Or( hn::ShiftLeft< 1 >( ph ) , Cp ) ,
	     Mh = hn::Or( hn::ShiftLeft< 1 >( mh ) , Cn );
  auto P = hn::Or( Mh , hn::Not( hn::Or( Xv , Ph ) ) ) ,
    M = hn::And( Ph , Xv ) ,
       op = hn::ShiftRight< 63 >( ph ) , on = hn::ShiftRight< 63 >( mh );
  if constexpr( Masked ) {
   const auto a = hn::LoadU( d , act + i );
   P = hn::IfVecThenElse( a , P , Pv );
   M = hn::IfVecThenElse( a , M , Mv );
   op = hn::IfVecThenElse( a , op , Cp );
   on = hn::IfVecThenElse( a , on , Cn );
   }
  hn::StoreU( P , d , pv + i );
  hn::StoreU( M , d , mv + i );
  hn::StoreU( op , d , cp + i );
  hn::StoreU( on , d , cn + i );
  }
 }

/// the integer of the lanes of Highway of the size and the sign of the
/// integer \f$ T \f$ (e.g., `std::int64_t` for `long long`)
template< class T >
using LaneInt = std::conditional_t<
  std::is_signed_v< T > ,
  std::conditional_t<
    sizeof( T ) == 1 , std::int8_t ,
    std::conditional_t<
      sizeof( T ) == 2 , std::int16_t ,
      std::conditional_t< sizeof( T ) == 4 , std::int32_t ,
			  std::int64_t > > > ,
  std::conditional_t<
    sizeof( T ) == 1 , std::uint8_t ,
    std::conditional_t< sizeof( T ) == 2 , std::uint16_t ,
			std::conditional_t< sizeof( T ) == 4 , std::uint32_t ,
					    std::uint64_t > > > >;

/// the word of the bits \f$ j \f$, \f$ 0 \le j < 64 \f$, of the lanes of
/// the masks \f$ m( i ) \f$, the vector at \f$ i \f$ of the tag \f$ d \f$
/// of \f$ L \le 64 \f$ lanes, \f$ i = 0 , L , 2 L , \ldots \f$
template< class D , class F >
HWY_INLINE std::uint64_t maskWord64( D d , F && m )
{
 const std::size_t L = hn::Lanes( d );
 std::uint64_t r = 0;
 for( std::size_t i = 0 ; i < 64 ; i += L ) {
  std::uint8_t c[ 8 ] = {};
  hn::StoreMaskBits( d , m( i ) , c );
  std::uint64_t w = 0;  // the bit j in the bit j % 8 of the byte j / 8
  for( int j = 7 ; j >= 0 ; --j )
   w = ( w << 8 ) | c[ j ];
  r |= w << i;
  }
 return( r );
 }

/// the bits \f$ j \f$, \f$ 0 \le j < 64 \f$, of the places of
/// \f$ a[ j ] < a[ j + 1 ] \f$ in \f$ g \f$ and of
/// \f$ a[ j ] = a[ j + 1 ] \f$ in \f$ e \f$, \f$ T \f$ an integer of 1 or
/// 4 bytes
template< class T >
HWY_INLINE void neighbors64( const T * x , std::uint64_t * g ,
			     std::uint64_t * e )
{
 static_assert( std::is_integral_v< T > &&
		  ( ( sizeof( T ) == 1 ) || ( sizeof( T ) == 4 ) ) ,
		"the integers of 1 or 4 bytes" );
 using U = LaneInt< T >;
 const U * a = reinterpret_cast< const U * >( x );
 const hn::CappedTag< U , 64 > d;
 *g = maskWord64( d , [ & ]( std::size_t i ) {
  return( hn::Lt( hn::LoadU( d , a + i ) , hn::LoadU( d , a + i + 1 ) ) );
  } );
 *e = maskWord64( d , [ & ]( std::size_t i ) {
  return( hn::Eq( hn::LoadU( d , a + i ) , hn::LoadU( d , a + i + 1 ) ) );
  } );
 }

/// \f$ o[ j ] = 4 x[ j ] + 2 t_j + b_j \f$, \f$ 0 \le j < 64 \f$, with
/// \f$ t_j \f$ and \f$ b_j \f$ the bits \f$ j \f$ of \f$ t \f$ and
/// \f$ b \f$
HWY_INLINE void kinds64( const std::uint8_t * x , std::uint64_t t ,
			 std::uint64_t b , std::uint16_t * o )
{
 const hn::CappedTag< std::uint16_t , 64 > d;
 const hn::Rebind< std::uint8_t , decltype( d ) > d8;
 const std::size_t L = hn::Lanes( d );
 const auto two = hn::Set( d , std::uint16_t( 2 ) ) ,
	    one = hn::Set( d , std::uint16_t( 1 ) );
 // the bits from i of a word, as the bytes of a mask of L lanes
 auto bits = [ & ]( std::uint64_t w , std::size_t i ) {
  std::uint8_t c[ 8 ];
  for( int j = 0 ; j < 8 ; ++j )
   c[ j ] = std::uint8_t( ( w >> i ) >> ( 8 * j ) );
  return( hn::LoadMaskBits( d , c ) );
  };
 for( std::size_t i = 0 ; i < 64 ; i += L ) {
  const auto k = hn::Or( hn::IfThenElseZero( bits( t , i ) , two ) ,
			 hn::IfThenElseZero( bits( b , i ) , one ) );
  const auto v = hn::PromoteTo( d , hn::LoadU( d8 , x + i ) );
  hn::StoreU( hn::Or( hn::ShiftLeft< 2 >( v ) , k ) , d , o + i );
  }
 }

/// the items but 0 of \f$ a[ i .. ) \f$, less one, moved to
/// \f$ a[ j .. ) \f$ in their order, a vector at a time while one is left
/// before \f$ to \f$ (each store at \f$ j \le i \f$ passes no item unread);
/// \f$ i \f$ and \f$ j \f$ advanced
template< class T >
HWY_INLINE void compactLess( T * x , std::size_t & i , std::size_t to ,
			     std::size_t & j )
{
 using U = LaneInt< T >;
 U * a = reinterpret_cast< U * >( x );
 const hn::ScalableTag< U > d;
 const std::size_t L = hn::Lanes( d );
 const auto one = hn::Set( d , U( 1 ) );
 for( ; i + L <= to ; i += L ) {
  const auto v = hn::LoadU( d , a + i );
  j += hn::CompressStore( hn::Sub( v , one ) , hn::Ne( v , hn::Zero( d ) ) ,
			  d ,
			  a + j );
  }
 }

/// the bits \f$ j \f$, \f$ 0 \le j < 64 \f$, of the negative items of
/// \f$ a[ 0 .. 64 ) \f$, whose signs are cleared, \f$ T \f$ signed
template< class T >
HWY_INLINE std::uint64_t takeSigns64( T * x )
{
 using U = LaneInt< T >;
 static_assert( std::is_signed_v< U > , "the signed integers" );
 U * a = reinterpret_cast< U * >( x );
 const hn::CappedTag< U , 64 > d;
 const auto keep = hn::Set( d , std::numeric_limits< U >::max() );
 return( maskWord64( d , [ & ]( std::size_t i ) {
  const auto v = hn::LoadU( d , a + i );
  hn::StoreU( hn::And( v , keep ) , d , a + i );
  return( hn::Lt( v , hn::Zero( d ) ) );
  } ) );
 }

/// the 8 words \f$ w_j \f$ shifted by 4, the bits \f$ j \f$ of \f$ b_1 \f$
/// to \f$ b_4 \f$ entering in this order, \f$ b_4 \f$ the lowest
HWY_INLINE void shiftIn4( std::uint64_t * w , std::uint32_t b1 ,
			  std::uint32_t b2 , std::uint32_t b3 ,
			  std::uint32_t b4 )
{
 const hn::CappedTag< std::uint64_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 const auto one = hn::Set( d , std::uint64_t( 1 ) );
 for( std::size_t j = 0 ; j < 8 ; j += L ) {
  const auto at = hn::Iota( d , std::uint64_t( j ) );
  auto bit = [ & ]( std::uint32_t b ) {
   return( hn::And( hn::Shr( hn::Set( d , std::uint64_t( b ) ) , at ) ,
		    one ) );
   };
  const auto in = hn::Or(
    hn::Or( hn::ShiftLeft< 3 >( bit( b1 ) ) ,
	    hn::ShiftLeft< 2 >( bit( b2 ) ) ) ,
    hn::Or( hn::ShiftLeft< 1 >( bit( b3 ) ) , bit( b4 ) ) );
  hn::StoreU( hn::Or( hn::ShiftLeft< 4 >( hn::LoadU( d , w + j ) ) , in ) , d ,
	      w + j );
  }
 }

/// the filter of the \f$ q \le 8 \f$ places of the patterns at the 32
/// places \f$ t[ p ] \f$, \f$ 0 \le p < 32 \f$:
/// \f$ C[ p ] = \bigwedge_{r < q} lo_r[ t[ p + r ] \bmod 16 ] \wedge
/// hi_r[ \lfloor t[ p + r ] / 16 \rfloor ] \f$ for the tables of 16
/// bytes \f$ lo_r \f$ and \f$ hi_r \f$, and the bits \f$ p \f$ of the
/// places with \f$ C[ p ] \ne 0 \f$; a vector of 16 bytes or more looks
/// up 16 places at once in each table, a smaller one a byte at a time
HWY_INLINE std::uint32_t
nibbleFilter32( const std::uint8_t * t ,
		const std::array< std::uint8_t , 16 > * lo ,
		const std::array< std::uint8_t , 16 > * hi , std::size_t q ,
		std::uint8_t * C )
{
 const hn::CappedTag< std::uint8_t , 32 > d;
 if constexpr( hn::MaxLanes( hn::CappedTag< std::uint8_t , 32 >() ) >= 16 ) {
  const std::size_t L = hn::Lanes( d );
  const auto low = hn::Set( d , std::uint8_t( 0x0f ) );
  std::uint32_t m = 0;
  for( std::size_t i = 0 ; i < 32 ; i += L ) {
   auto acc = hn::Set( d , std::uint8_t( 0xff ) );
   for( std::size_t r = 0 ; r < q ; ++r ) {
    const auto v = hn::LoadU( d , t + i + r );
    acc = hn::And(
      acc ,
      hn::And( hn::TableLookupBytes( hn::LoadDup128( d , lo[ r ].data() ) ,
					  hn::And( v , low ) ) ,
		    hn::TableLookupBytes( hn::LoadDup128( d ,
							  hi[ r ].data() ) ,
					  hn::ShiftRight< 4 >( v ) ) ) );
    }
   hn::StoreU( acc , d , C + i );
   std::uint8_t c[ 4 ] = {};
   hn::StoreMaskBits( d , hn::Ne( acc , hn::Zero( d ) ) , c );
   m |=
     ( std::uint32_t( c[ 0 ] ) | ( std::uint32_t( c[ 1 ] ) << 8 ) |
       ( std::uint32_t( c[ 2 ] ) << 16 ) | ( std::uint32_t( c[ 3 ] ) << 24 ) )
     << i;
   }
  return( m );
  }
 else {
  std::uint32_t m = 0;
  for( std::size_t p = 0 ; p < 32 ; ++p ) {
   unsigned c = 0xff;
   for( std::size_t r = 0 ; r < q ; ++r )
    c &= lo[ r ][ t[ p + r ] & 15 ] & hi[ r ][ t[ p + r ] >> 4 ];
   C[ p ] = std::uint8_t( c );
   m |= std::uint32_t( c != 0 ) << p;
   }
  return( m );
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
