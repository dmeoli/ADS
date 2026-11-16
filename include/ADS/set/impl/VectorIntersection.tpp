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

namespace ADS
{

namespace detail
{

/// the merge of a[ i .. n ) and b[ j .. m ) one item at a time with no
/// branch, appended to out[ k .. ); the new k
template< class T >
std::size_t mergeCommon( const T * a , std::size_t n , const T * b ,
			 std::size_t m , T * out , std::size_t i ,
			 std::size_t j ,
			 std::size_t k )
{
 while( ( i < n ) && ( j < m ) ) {
  const T x = a[ i ] , y = b[ j ];
  out[ k ] = x;
  k += ( x == y );
  i += ! ( y < x );
  j += ! ( x < y );
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
  if( l[ lo ] < x ) {  // past lo: gallop, then search
   std::size_t d = 1;
   while( ( lo + d < n ) && ( l[ lo + d ] < x ) )
    d *= 2;
   // the first item not smaller than x is in [ u , w ], or it is w + 1
   const std::size_t u = lo + d / 2 + 1 , w = std::min( lo + d , n - 1 );
   if( u > w )
    break;  // none left in l
   const T * base = l + u;
   std::size_t len = w - u + 1;
   while( len > 1 ) {  // with no branch
    const std::size_t h = len / 2;
    base = base[ h ] < x ? base + h : base;
    len -= h;
    }
   base += *base < x ? 1 : 0;
   lo = std::size_t( base - l );
   }
  if( ( lo < n ) && ! ( x < l[ lo ] ) )
   out[ k++ ] = x;
  }
 return( k );
 }

}  // end( namespace detail )

template< class T >
std::size_t intersect( const T * a , std::size_t n , const T * b ,
		       std::size_t m ,
		       T * out )
{
 ADS_AUDIT( std::adjacent_find( a , a + n ,
				std::greater_equal<>() ) == a + n );
 ADS_AUDIT( std::adjacent_find( b , b + m ,
				std::greater_equal<>() ) == b + m );
 std::size_t k;
 if( n < m ) {  // a the longer
  std::swap( a , b );
  std::swap( n , m );
  }
 // the integers of 32 bits by the kernels of Highway [see Simd.h], whose
 // ends are merged as any others
 [[maybe_unused]] constexpr bool hwy32 =
   std::is_integral_v< T > && ( sizeof( T ) == 4 );
 if( m * 32 < n ) {
  std::size_t q = 0 , p = 0;
  k = 0;
#ifdef ADS_WITH_HIGHWAY
  if constexpr( hwy32 )
   k = detail::simd::windowsCommon( b , m , a , n , out , &q , &p );
#endif
  k += detail::gallopCommon( b + q , m - q , a + p , n - p , out + k );
  }
 else {
  std::size_t i = 0 , j = 0;
  k = 0;
#ifdef ADS_WITH_HIGHWAY
  if constexpr( hwy32 )
   k = detail::simd::blocksCommon( a , n , b , m , out , &i , &j );
#endif
  k = detail::mergeCommon( a , n , b , m , out , i , j , k );
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
 C.resize( intersect( A.data() , A.size() , B.data() , B.size() , C.data() ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File VectorIntersection.tpp -------------------*/
/*--------------------------------------------------------------------------*/
