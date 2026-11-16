/*--------------------------------------------------------------------------*/
/*----------------------------- File Sets.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The kernels of the sets on Highway: the intersection of two sorted arrays
 * of integers of 32 bits by blocks [see VectorIntersection.h], which
 * compares a block of \f$ L \le 8 \f$ items of each array with all the
 * rotations of the other, packs the common items of the first by a
 * compression of the lanes and moves on in the array whose block ends
 * first, as Lemire, Boytsov and Kurz do with the shuffles of SSE; and the
 * search of an item in windows of 64 items of the longer array, all the
 * comparisons of a window at once; and the intersection, the difference
 * and the union of the sorted arrays of 16 bits of Roaring by blocks of
 * \f$ L \le 8 \f$ items [see RoaringBitmap.h], the first two all against
 * all by the rotations, as Schlegel, Willhalm and Lehner do with the
 * comparison of strings of SSE4.2, the union by the merge network of
 * Inoue and Taura.
 *
 * \see B. Schlegel, T. Willhalm, W. Lehner, Fast sorted-set intersection
 *      using SIMD instructions, Proceedings of the 2nd International
 *      Workshop on Accelerating Data Management Systems Using Modern
 *      Processor and Storage Architectures (ADMS), 1-8, 2011
 * \see H. Inoue, K. Taura, SIMD- and cache-friendly algorithm for sorting
 *      an array of structures, Proceedings of the VLDB Endowment 8(11),
 *      1274-1285, 2015, doi:10.14778/2809974.2809988
 * \see D. Lemire, L. Boytsov, N. Kurz, SIMD compression and the
 *      intersection of sorted integers, Software: Practice and Experience
 *      46(6), 723-749, 2016, doi:10.1002/spe.2326
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

#if defined( __ADS_simd_Sets ) == defined( HWY_TARGET_TOGGLE )
 #ifdef __ADS_simd_Sets
  #undef __ADS_simd_Sets
 #else
  #define __ADS_simd_Sets
 #endif

 #include <hwy/highway.h>

 #include <cstddef>
 #include <cstdint>
 #include <cstring>

HWY_BEFORE_NAMESPACE();

namespace ADS::detail::simd::HWY_NAMESPACE
{

namespace hn = hwy::HWY_NAMESPACE;

/// the items common to a[ 0 .. n ) and b[ 0 .. m ), sorted by the order of
/// T, appended to out by blocks of L <= 8 items while both have a whole
/// block; the places where the blocks stopped in *i and *j, the items
/// written returned
template< class T >
HWY_INLINE std::size_t blocksCommon( const T * HWY_RESTRICT a , std::size_t n ,
				     const T * HWY_RESTRICT b , std::size_t m ,
				     T * HWY_RESTRICT out , std::size_t * i ,
				     std::size_t * j )
{
 const hn::CappedTag< std::uint32_t , 8 > d;
 const std::size_t L = hn::Lanes( d );
 std::size_t p = 0 , q = 0 , k = 0;
 if( ( n >= L ) && ( m >= L ) ) {
  const auto io = hn::Iota( d , 0 );
  const auto top = hn::Set( d , std::uint32_t( L - 1 ) );
  const auto * A = reinterpret_cast< const std::uint32_t * >( a );
  const auto * B = reinterpret_cast< const std::uint32_t * >( b );
  auto * O = reinterpret_cast< std::uint32_t * >( out );
  for( ; ; ) {
   const auto va = hn::LoadU( d , A + p ) , vb = hn::LoadU( d , B + q );
   auto c = hn::Eq( va , vb );
   for( std::size_t r = 1 ; r < L ; ++r ) {  // the rotations of vb
    const auto ix =
      hn::And( hn::Add( io , hn::Set( d , std::uint32_t( r ) ) ) , top );
    c = hn::Or( c , hn::Eq( va , hn::TableLookupLanes(
				 vb , hn::IndicesFromVec( d , ix ) ) ) );
    }
   k += hn::CompressBlendedStore( va , c , d , O + k );
   const T x = a[ p + L - 1 ] , y = b[ q + L - 1 ];
   p += ! ( y < x ) ? L : 0;  // the block that ends first
   q += ! ( x < y ) ? L : 0;
   if( ( p + L > n ) || ( q + L > m ) )
    break;
   }
  }
 *i = p;
 *j = q;
 return( k );
 }

/// the items of s[ 0 .. m ) also in l[ 0 .. n ), appended to out, while a
/// window of 64 items of l is left: the window moves by 1024 and then by
/// 64 items while its last item is smaller than the item looked for, and
/// the item is compared with the 64 at once; the items of s and of l
/// passed in *q and *p, the items written returned
template< class T >
HWY_INLINE std::size_t windowsCommon( const T * HWY_RESTRICT s ,
				      std::size_t m ,
				      const T * HWY_RESTRICT l ,
				      std::size_t n ,
				      T * HWY_RESTRICT out , std::size_t * q ,
				      std::size_t * p )
{
 constexpr std::size_t W = 64;
 const hn::CappedTag< std::uint32_t , 16 > d;
 const std::size_t L = hn::Lanes( d );
 const auto * P = reinterpret_cast< const std::uint32_t * >( l );
 std::size_t k = 0 , at = 0 , t = 0;
 for( ; ( t < m ) && ( at + W <= n ) ; ++t ) {
  const T x = s[ t ];
  while( ( at + 16 * W <= n ) && ( l[ at + 16 * W - 1 ] < x ) )
   at += 16 * W;
  while( ( at + W <= n ) && ( l[ at + W - 1 ] < x ) ) {
   ADS::detail::prefetchLine( P + at + 8 * W ,  // the strides ahead
			      ADS::Locality( 3 ) );
   at += W;
   }
  if( at + W > n )
   break;
  std::uint32_t u;
  std::memcpy( &u , &x , sizeof( u ) );
  const auto v = hn::Set( d , u );
  auto e = hn::Eq( v , hn::LoadU( d , P + at ) );
  for( std::size_t c = L ; c < W ; c += L )
   e = hn::Or( e , hn::Eq( v , hn::LoadU( d , P + at + c ) ) );
  out[ k ] = x;
  k += hn::AllFalse( d , e ) ? 0 : 1;
  }
 *q = t;
 *p = at;
 return( k );
 }

/// blocksCommon() on signed integers of 32 bits
inline std::size_t blocksCommonI32( const std::int32_t * a , std::size_t n ,
				    const std::int32_t * b , std::size_t m ,
				    std::int32_t * out , std::size_t * i ,
				    std::size_t * j )
{
 return( blocksCommon( a , n , b , m , out , i , j ) );
 }

/// blocksCommon() on unsigned integers of 32 bits
inline std::size_t blocksCommonU32( const std::uint32_t * a , std::size_t n ,
				    const std::uint32_t * b , std::size_t m ,
				    std::uint32_t * out , std::size_t * i ,
				    std::size_t * j )
{
 return( blocksCommon( a , n , b , m , out , i , j ) );
 }

/// windowsCommon() on signed integers of 32 bits
inline std::size_t windowsCommonI32( const std::int32_t * s , std::size_t m ,
				     const std::int32_t * l , std::size_t n ,
				     std::int32_t * out , std::size_t * q ,
				     std::size_t * p )
{
 return( windowsCommon( s , m , l , n , out , q , p ) );
 }

/// windowsCommon() on unsigned integers of 32 bits
inline std::size_t windowsCommonU32( const std::uint32_t * s , std::size_t m ,
				     const std::uint32_t * l , std::size_t n ,
				     std::uint32_t * out , std::size_t * q ,
				     std::size_t * p )
{
 return( windowsCommon( s , m , l , n , out , q , p ) );
 }

/// the lanes of v of the tag d rotated down by one place: the lane i takes
/// the lane ( i + 1 ) mod L, L a power of 2
template< class D , class V >
HWY_INLINE V rotateDown( D d , V v )
{
 const auto ix =
   hn::And( hn::Add( hn::Iota( d , 1 ) , hn::Zero( d ) ) ,
	    hn::Set( d , hn::TFromD< D >( hn::Lanes( d ) - 1 ) ) );
 return( hn::TableLookupLanes( v , hn::IndicesFromVec( d , ix ) ) );
 }

/// the mask of the lanes of va whose item is in vb, by the comparisons of
/// va with the L rotations of vb
template< class D , class V >
HWY_INLINE auto foundIn( D d , V va , V vb )
{
 auto c = hn::Eq( va , vb );
 for( std::size_t r = 1 ; r < hn::Lanes( d ) ; ++r ) {
  vb = rotateDown( d , vb );
  c = hn::Or( c , hn::Eq( va , vb ) );
  }
 return( c );
 }

/// the tag of the blocks of the arrays of 16 bits of Roaring
using Shorts8 = hn::CappedTag< std::uint16_t , 8 >;

/// the items common to the sorted a[ 0 .. na ) and b[ 0 .. nb ) of 16 bits
/// appended to out by blocks of L <= 8, all against all, the block that
/// ends first moving on, while both have a whole block from *i and *j; the
/// places where they stopped in *i and *j, the items written returned
inline std::size_t intersectBlocks16( const std::uint16_t * HWY_RESTRICT a ,
				      std::size_t na ,
				      const std::uint16_t * HWY_RESTRICT b ,
				      std::size_t nb ,
				      std::uint16_t * HWY_RESTRICT out ,
				      std::size_t * i , std::size_t * j )
{
 const Shorts8 d;
 const std::size_t L = hn::Lanes( d );
 std::size_t p = *i , q = *j , k = 0;
 if( ( p + L <= na ) && ( q + L <= nb ) ) {
  auto va = hn::LoadU( d , a + p ) , vb = hn::LoadU( d , b + q );
  for( ; ; ) {  // a block moves on, or both
   k += hn::CompressBlendedStore( va , foundIn( d , va , vb ) , d , out + k );
   const std::uint16_t x = a[ p + L - 1 ] , y = b[ q + L - 1 ];
   if( x <= y ) {
    p += L;
    if( p + L > na )
     break;
    va = hn::LoadU( d , a + p );
    }
   if( y <= x ) {
    q += L;
    if( q + L > nb )
     break;
    vb = hn::LoadU( d , b + q );
    }
   }
  }
 *i = p;
 *j = q;
 return( k );
 }

/// the items of the sorted a[ 0 .. na ) of 16 bits not in the sorted
/// b[ 0 .. nb ) appended to out by blocks of L <= 8 as in
/// intersectBlocks16(), the items of the block of a found in some block of
/// b gathered in a mask and the others stored when the block of a moves
/// on; the places where the blocks stopped in *i and *j, the bits of the
/// items of the block at *i found so far in *found, the items written
/// returned
inline std::size_t
differenceBlocks16( const std::uint16_t * HWY_RESTRICT a , std::size_t na ,
		    const std::uint16_t * HWY_RESTRICT b , std::size_t nb ,
		    std::uint16_t * HWY_RESTRICT out , std::size_t * i ,
		    std::size_t * j , unsigned * found )
{
 const Shorts8 d;
 const std::size_t L = hn::Lanes( d );
 std::size_t p = *i , q = *j , k = 0;
 auto m = hn::FirstN( d , 0 );
 if( ( p + L <= na ) && ( q + L <= nb ) ) {
  auto va = hn::LoadU( d , a + p ) , vb = hn::LoadU( d , b + q );
  for( ; ; ) {
   m = hn::Or( m , foundIn( d , va , vb ) );
   const std::uint16_t x = a[ p + L - 1 ] , y = b[ q + L - 1 ];
   if( x <= y ) {  // the block of a is done
    k += hn::CompressBlendedStore( va , hn::Not( m ) , d , out + k );
    m = hn::FirstN( d , 0 );
    p += L;
    if( p + L > na )
     break;
    va = hn::LoadU( d , a + p );
    }
   if( y <= x ) {
    q += L;
    if( q + L > nb )
     break;
    vb = hn::LoadU( d , b + q );
    }
   }
  }
 std::uint8_t bits[ 8 ] = {};
 hn::StoreMaskBits( d , m , bits );
 *found = bits[ 0 ];
 *i = p;
 *j = q;
 return( k );
 }

/// the items of the sorted a[ 0 .. na ) or b[ 0 .. nb ) of 16 bits, each
/// once, written to out (room for na + nb + 8), na , nb >= L: merged by
/// blocks of L <= 8 by the network of Inoue and Taura (the minima and the
/// maxima of two sorted blocks, the minima rotated by one place L - 1
/// times), the next block taken from the list with the smaller head, the
/// items equal to the one before them dropped, and the tails merged one
/// item at a time; the items written
inline std::size_t unionBlocks16( const std::uint16_t * HWY_RESTRICT a ,
				  std::size_t na ,
				  const std::uint16_t * HWY_RESTRICT b ,
				  std::size_t nb ,
				  std::uint16_t * HWY_RESTRICT out )
{
 const Shorts8 d;
 const std::size_t L = hn::Lanes( d );
 std::size_t i = L , j = L , k = 0;
 auto lo = hn::LoadU( d , a ) , hi = hn::LoadU( d , b );
 int last = -1;  // the last item written
 alignas( 16 ) std::uint16_t t[ 8 ];
 for( ; ; ) {
  auto mn = hn::Min( lo , hi ) , mx = hn::Max( lo , hi );
  for( std::size_t r = 1 ; r < L ; ++r ) {
   mn = rotateDown( d , mn );
   const auto m = hn::Min( mn , mx );
   mx = hn::Max( mn , mx );
   mn = m;
   }
  lo = rotateDown( d , mn );
  hi = mx;
  hn::StoreU( lo , d , t );
  for( std::size_t r = 0 ; r < L ; ++r ) {  // each item once
   out[ k ] = t[ r ];
   k += int( t[ r ] ) != last;
   last = t[ r ];
   }
  // the next block, from the list with the smaller head, if a whole
  // block; a smaller head in a tail ends the blocks
  const bool fa = i + L <= na , fb = j + L <= nb;
  const int ha = i < na ? a[ i ] : 1 << 16 , hb = j < nb ? b[ j ] : 1 << 16;
  if( fa && ( ha <= hb ) ) {
   lo = hn::LoadU( d , a + i );
   i += L;
   }
  else if( fb && ( hb < ha ) ) {
   lo = hn::LoadU( d , b + j );
   j += L;
   }
  else
   break;
  }
 hn::StoreU( hi , d , t );  // hi and the two tails
 std::size_t p = 0;
 while( ( p < L ) || ( i < na ) || ( j < nb ) ) {
  int v = 1 << 17;
  int src = 0;
  if( ( p < L ) && ( t[ p ] < v ) ) {
   v = t[ p ];
   src = 0;
   }
  if( ( i < na ) && ( a[ i ] < v ) ) {
   v = a[ i ];
   src = 1;
   }
  if( ( j < nb ) && ( b[ j ] < v ) ) {
   v = b[ j ];
   src = 2;
   }
  if( src == 0 )
   ++p;
  else if( src == 1 )
   ++i;
  else
   ++j;
  if( v != last ) {
   out[ k++ ] = std::uint16_t( v );
   last = v;
   }
  }
 return( k );
 }

}  // end( namespace ADS::detail::simd::HWY_NAMESPACE )

HWY_AFTER_NAMESPACE();

#endif

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Sets.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
