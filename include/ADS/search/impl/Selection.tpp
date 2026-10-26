/*--------------------------------------------------------------------------*/
/*--------------------------- File Selection.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Selection.h [see Selection.h], with
 * their contracts.
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

namespace ADS {

namespace detail {

/// fewer than k items of A are smaller than x, and at least k are not
/// larger
template< class T >
bool hasRank( const std::vector< T > & A , const T & x , std::size_t k )
{
 std::size_t less = 0 , notLarger = 0;
 for( const T & a : A ) {
  less += a < x;
  notLarger += !( x < a );
  }
 return( ( less < k ) && ( notLarger >= k ) );
 }

}  // end( namespace detail )

template< class T , class R >
T selection( std::vector< T > & A , std::size_t k , R * rng )
{
 ADS_PRE( ( k >= 1 ) && ( k <= A.size() ) );
 const std::size_t rank = k;
 auto ranked = [ & ]( const T & x ) {
  return( detail::hasRank( A , x , rank ) ); };
 std::size_t start = 0 , end = A.size() - 1;
 for( ; ; ) {                                   // the tail recursion
  if( start == end ) {
   ADS_AUDIT( ranked( A[ start ] ) );
   return( A[ start ] );
   }
  if( rng ) {
   std::uniform_int_distribution< std::size_t > pick( start , end );
   std::swap( A[ pick( *rng ) ] , A[ start ] );
   }
  const std::size_t j = partition( A , start , end );
  const std::size_t q = j - start + 1;
  if( k == q ) {
   ADS_AUDIT( ranked( A[ j ] ) );
   return( A[ j ] );
   }
  if( k < q )
   end = j - 1;
  else {
   start = j + 1;
   k -= q;
   }
  }
 }

template< class T >
T medianOfMedians( std::vector< T > & A , std::size_t k )
{
 ADS_PRE( ( k >= 1 ) && ( k <= A.size() ) );
 auto rec = [ & ]( auto & self , std::size_t lo , std::size_t hi ,
                   std::size_t r ) -> T {     // rank r in A[ lo .. hi ]
  for( ; ; ) {
   const std::size_t n = hi - lo + 1;
   if( n <= 10 ) {
    insertionSort( A , lo , hi + 1 );
    return( A[ lo + r - 1 ] );
    }
   // the medians of the groups of 5 to the front
   std::size_t m = lo;
   for( std::size_t g = lo ; g + 4 <= hi ; g += 5 ) {
    insertionSort( A , g , g + 5 );
    std::swap( A[ m++ ] , A[ g + 2 ] );
    }
   const T pivot = self( self , lo , m - 1 , ( m - lo + 1 ) / 2 );
   // three ways: smaller, equal, larger
   std::size_t lt = lo , i = lo , gt = hi;
   while( i <= gt )
    if( A[ i ] < pivot ) std::swap( A[ lt++ ] , A[ i++ ] );
    else if( pivot < A[ i ] ) std::swap( A[ i ] , A[ gt-- ] );
    else ++i;
   if( r <= lt - lo )
    hi = lt - 1;
   else if( r <= i - lo )
    return( pivot );
   else {
    r -= i - lo;
    lo = i;
    }
   }
  };
 const T x = rec( rec , 0 , A.size() - 1 , k );
 ADS_AUDIT( detail::hasRank( A , x , k ) );
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Selection.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
