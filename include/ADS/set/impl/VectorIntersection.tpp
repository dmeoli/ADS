/*--------------------------------------------------------------------------*/
/*----------------------- File VectorIntersection.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The intersections of VectorIntersection.h, with their contracts.
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

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iterator>
#include <type_traits>

#if defined( __AVX2__ )
 #include <immintrin.h>
#endif

namespace ADS {

namespace detail {

/// the merge of a[ i .. n ) and b[ j .. m ) one item at a time with no
/// branch, appended to out[ k .. ); the new k
template< class T >
std::size_t mergeCommon( const T * a , std::size_t n , const T * b ,
                         std::size_t m , T * out , std::size_t i ,
                         std::size_t j , std::size_t k )
{
 while( ( i < n ) && ( j < m ) ) {
  const T x = a[ i ] , y = b[ j ];
  out[ k ] = x;
  k += ( x == y );
  i += !( y < x );
  j += !( x < y );
  }
 return( k );
 }

/// the items of s[ 0 .. m ) also in l[ 0 .. n ), by galloping in l
template< class T >
std::size_t gallopCommon( const T * s , std::size_t m , const T * l ,
                          std::size_t n , T * out )
{
 std::size_t k = 0 , lo = 0;
 for( std::size_t q = 0 ; ( q < m ) && ( lo < n ) ; ++q ) {
  const T x = s[ q ];
  if( l[ lo ] < x ) {                       // past lo: gallop, then search
   std::size_t d = 1;
   while( ( lo + d < n ) && ( l[ lo + d ] < x ) )
    d *= 2;
   // the first item not smaller than x is in [ u , w ], or it is w + 1
   const std::size_t u = lo + d / 2 + 1 , w = std::min( lo + d , n - 1 );
   if( u > w )
    break;                                  // none left in l
   const T * base = l + u;
   std::size_t len = w - u + 1;
   while( len > 1 ) {                       // with no branch
    const std::size_t h = len / 2;
    base = base[ h ] < x ? base + h : base;
    len -= h;
    }
   base += *base < x ? 1 : 0;
   lo = std::size_t( base - l );
   }
  if( ( lo < n ) && !( x < l[ lo ] ) )
   out[ k++ ] = x;
  }
 return( k );
 }

#if defined( __AVX2__ )

/// the permutations that pack the items of a block of 8 chosen by a mask
/// to its front
struct PackTable {
 alignas( 32 ) std::uint32_t p[ 256 ][ 8 ];
 constexpr PackTable( void ) : p{} {
  for( int mask = 0 ; mask < 256 ; ++mask ) {
   int c = 0;
   for( int b = 0 ; b < 8 ; ++b )
    if( mask & ( 1 << b ) )
     p[ mask ][ c++ ] = std::uint32_t( b );
   }
  }
 };

inline constexpr PackTable packTable{};

/// the items common to a and b of 32 bits, a block of 8 at a time
template< class T >
std::size_t blockCommon( const T * a , std::size_t n , const T * b ,
                         std::size_t m , T * out )
{
 std::size_t i = 0 , j = 0 , k = 0;
 if( ( n >= 8 ) && ( m >= 8 ) ) {
  const __m256i r1 = _mm256_setr_epi32( 1 , 2 , 3 , 4 , 5 , 6 , 7 , 0 );
  const __m256i r2 = _mm256_setr_epi32( 2 , 3 , 4 , 5 , 6 , 7 , 0 , 1 );
  const __m256i r3 = _mm256_setr_epi32( 3 , 4 , 5 , 6 , 7 , 0 , 1 , 2 );
  const __m256i r4 = _mm256_setr_epi32( 4 , 5 , 6 , 7 , 0 , 1 , 2 , 3 );
  const __m256i r5 = _mm256_setr_epi32( 5 , 6 , 7 , 0 , 1 , 2 , 3 , 4 );
  const __m256i r6 = _mm256_setr_epi32( 6 , 7 , 0 , 1 , 2 , 3 , 4 , 5 );
  const __m256i r7 = _mm256_setr_epi32( 7 , 0 , 1 , 2 , 3 , 4 , 5 , 6 );
  for( ; ; ) {
   const __m256i va = _mm256_loadu_si256(
                       reinterpret_cast< const __m256i * >( a + i ) );
   const __m256i vb = _mm256_loadu_si256(
                       reinterpret_cast< const __m256i * >( b + j ) );
   // the rotations are independent of one another, not a chain
   const __m256i c01 = _mm256_or_si256(
    _mm256_cmpeq_epi32( va , vb ) ,
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r1 ) ) );
   const __m256i c23 = _mm256_or_si256(
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r2 ) ) ,
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r3 ) ) );
   const __m256i c45 = _mm256_or_si256(
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r4 ) ) ,
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r5 ) ) );
   const __m256i c67 = _mm256_or_si256(
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r6 ) ) ,
    _mm256_cmpeq_epi32( va , _mm256_permutevar8x32_epi32( vb , r7 ) ) );
   const __m256i c = _mm256_or_si256( _mm256_or_si256( c01 , c23 ) ,
                                      _mm256_or_si256( c45 , c67 ) );
   const int mask = _mm256_movemask_ps( _mm256_castsi256_ps( c ) );
   const __m256i * perm = reinterpret_cast< const __m256i * >(
                           packTable.p[ mask ] );
   const __m256i pack = _mm256_permutevar8x32_epi32(
                         va , _mm256_load_si256( perm ) );
   _mm256_storeu_si256( reinterpret_cast< __m256i * >( out + k ) , pack );
   k += std::size_t( std::popcount( unsigned( mask ) ) );
   const T x = a[ i + 7 ] , y = b[ j + 7 ];
   i += !( y < x ) ? 8 : 0;                 // the block that ends first
   j += !( x < y ) ? 8 : 0;
   if( ( i + 8 > n ) || ( j + 8 > m ) )
    break;
   }
  }
 return( mergeCommon( a , n , b , m , out , i , j , k ) );
 }

/// the items of s[ 0 .. m ) also in l[ 0 .. n ), of 32 bits, the longer
/// list l read through a window of 64 items: the window moves by strides
/// of 1024 and then of 64 items while its last item is smaller than the
/// item looked for, the line 8 strides ahead prefetched, and the item is
/// compared with the 64 of the window at once, 8 comparisons of vectors
/// and no search inside the window (a binary search on the windows would
/// mispredict half of its steps)
template< class T >
std::size_t blockGallop( const T * s , std::size_t m , const T * l ,
                         std::size_t n , T * out )
{
 constexpr std::size_t W = 64;
 std::size_t k = 0 , p = 0 , q = 0;
 for( ; ( q < m ) && ( p + W <= n ) ; ++q ) {
  const T x = s[ q ];
  while( ( p + 16 * W <= n ) && ( l[ p + 16 * W - 1 ] < x ) )
   p += 16 * W;
  while( ( p + W <= n ) && ( l[ p + W - 1 ] < x ) ) {
   __builtin_prefetch( l + p + 8 * W );     // the strides ahead
   p += W;
   }
  if( p + W > n )
   break;
  const __m256i v = _mm256_set1_epi32( std::int32_t( x ) );
  const __m256i * w = reinterpret_cast< const __m256i * >( l + p );
  __m256i e = _mm256_cmpeq_epi32( v , _mm256_loadu_si256( w ) );
  for( int c = 1 ; c < 8 ; ++c ) {
   const __m256i y = _mm256_loadu_si256( w + c );
   e = _mm256_or_si256( e , _mm256_cmpeq_epi32( v , y ) );
   }
  out[ k ] = x;
  k += _mm256_testz_si256( e , e ) ? 0 : 1;
  }
 // the items left against the tail of l, past the last whole window
 return( k + gallopCommon( s + q , m - q , l + p , n - p , out + k ) );
 }

#endif

}  // end( namespace detail )

template< class T >
std::size_t intersect( const T * a , std::size_t n , const T * b ,
                       std::size_t m , T * out )
{
 ADS_AUDIT( std::adjacent_find( a , a + n , std::greater_equal<>() ) ==
            a + n );
 ADS_AUDIT( std::adjacent_find( b , b + m , std::greater_equal<>() ) ==
            b + m );
 std::size_t k;
 if( n < m ) {                              // a the longer
  std::swap( a , b );
  std::swap( n , m );
  }
 if( m * 32 < n ) {
#if defined( __AVX2__ )
  if constexpr( std::is_integral_v< T > && ( sizeof( T ) == 4 ) )
   k = detail::blockGallop( b , m , a , n , out );
  else
   k = detail::gallopCommon( b , m , a , n , out );
#else
  k = detail::gallopCommon( b , m , a , n , out );
#endif
  }
 else {
#if defined( __AVX2__ )
  if constexpr( std::is_integral_v< T > && ( sizeof( T ) == 4 ) )
   k = detail::blockCommon( a , n , b , m , out );
  else
   k = detail::mergeCommon( a , n , b , m , out , 0 , 0 , 0 );
#else
  k = detail::mergeCommon( a , n , b , m , out , 0 , 0 , 0 );
#endif
  }
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > C;
  std::set_intersection( a , a + n , b , b + m , std::back_inserter( C ) );
  return( ( C.size() == k ) && std::equal( C.begin() , C.end() , out ) );
  }() );
 return( k );
 }

template< class T >
std::vector< T > intersect( const std::vector< T > & A ,
                            const std::vector< T > & B )
{
 std::vector< T > C( std::min( A.size() , B.size() ) + 8 );
 C.resize( intersect( A.data() , A.size() , B.data() , B.size() ,
                      C.data() ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File VectorIntersection.tpp -------------------*/
/*--------------------------------------------------------------------------*/
