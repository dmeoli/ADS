/*--------------------------------------------------------------------------*/
/*--------------------------- File MergeSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of MergeSort.h [see MergeSort.h].
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

template< class T , class C >
void mergeParts( std::vector< T > & A , std::vector< T > & B ,
                 std::size_t start , std::size_t mid , std::size_t end ,
                 C less , long long * inv )
{
 ADS_PRE( ( start <= mid ) && ( mid < end ) && ( end < A.size() ) &&
          ( B.size() >= A.size() ) );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::is_sorted( A.begin() + start , A.begin() + mid + 1 , less ) &&
          std::is_sorted( A.begin() + mid + 1 , A.begin() + end + 1 ,
                          less ) ); } ) );
 std::size_t i = start , j = mid + 1 , k = start;
 while( ( i <= mid ) && ( j <= end ) )
  if( ! less( A[ j ] , A[ i ] ) )              // A[ i ] <= A[ j ]
   B[ k++ ] = std::move( A[ i++ ] );
  else {
   if( inv )
    *inv += static_cast< long long >( mid - i + 1 );
   B[ k++ ] = std::move( A[ j++ ] );
   }
 while( i <= mid )                              // the rest of the left part
  B[ k++ ] = std::move( A[ i++ ] );
 for( std::size_t t = start ; t < k ; ++t )
  A[ t ] = std::move( B[ t ] );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::is_sorted( A.begin() + start , A.begin() + end + 1 ,
                          less ) ); } ) );
 }

template< class T , class C >
void mergeSortRec( std::vector< T > & A , std::vector< T > & B ,
                   std::size_t start , std::size_t end , std::size_t cutoff ,
                   C less , long long * inv )
{
 ADS_PRE( ( start <= end ) && ( end < A.size() ) &&
          ( B.size() >= A.size() ) && ( cutoff >= 1 ) );
 if( end - start + 1 <= cutoff ) {
  insertionSort( A , start , end + 1 , less );
  return;
  }
 if( start < end ) {
  const std::size_t mid = start + ( end - start ) / 2;
  mergeSortRec( A , B , start , mid , cutoff , less , inv );
  mergeSortRec( A , B , mid + 1 , end , cutoff , less , inv );
  if( ( cutoff > 1 ) && ! less( A[ mid + 1 ] , A[ mid ] ) )
   return;                                  // already in order
  mergeParts( A , B , start , mid , end , less , inv );
  }
 }

template< class T , class C >
void mergeSort( std::vector< T > & A , std::size_t cutoff ,
                C less )
{
 if( A.size() < 2 )
  return;
 const auto before = detail::auditCopy( A );
 std::vector< T > B( A.size() );
 mergeSortRec( A , B , 0 , A.size() - 1 ,
               std::max< std::size_t >( cutoff , 1 ) , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void mergeSortBottomUp( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 std::vector< T > B( n );
 for( std::size_t w = 1 ; w < n ; w *= 2 )
  for( std::size_t start = 0 ; start + w < n ; start += 2 * w )
   mergeParts( A , B , start , start + w - 1 ,
               std::min( start + 2 * w , n ) - 1 , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
long long countInversions( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 long long inv = 0;
 if( A.size() > 1 ) {
  std::vector< T > B( A.size() );
  mergeSortRec( A , B , 0 , A.size() - 1 , 1 , less , &inv );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 return( inv );
 }

template< class T , class C >
void mergeSort( List< T > & L , C less )
{
 // split: the second half moves to R
 std::size_t n = 0;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  ++n;
 if( n < 2 )
  return;
 List< T > R;
 auto p = L.head();
 for( std::size_t i = 0 ; i < n / 2 ; ++i )
  p = L.next( p );
 while( ! L.finished( p ) ) {
  R.insert( R.next( R.tail() ) , L.read( p ) );
  p = L.remove( p );
  }
 mergeSort( L , less );
 mergeSort( R , less );
 // merge: each item of R goes before the first larger item of L
 auto q = L.head();
 for( auto r = R.head() ; ! R.finished( r ) ; r = R.next( r ) ) {
  while( ! L.finished( q ) && ! less( R.read( r ) , L.read( q ) ) )
   q = L.next( q );
  L.insert( q , R.read( r ) );
  }
 ADS_AUDIT( detail::sequenceSorted( L , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MergeSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
