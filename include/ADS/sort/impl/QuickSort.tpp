/*--------------------------------------------------------------------------*/
/*--------------------------- File QuickSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of QuickSort.h [see QuickSort.h].
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

template< class T , class C >
std::size_t partition( std::vector< T > & A , std::size_t start ,
                       std::size_t end , C less )
{
 ADS_PRE( ( start <= end ) && ( end < A.size() ) );
 const T x = A[ start ];
 std::size_t j = start;
 for( std::size_t i = start + 1 ; i <= end ; ++i )
  if( less( A[ i ] , x ) )
   std::swap( A[ i ] , A[ ++j ] );
 A[ start ] = A[ j ];
 A[ j ] = x;
 const auto below = [ & ]( const T & a ) { return( less( a , x ) ); };
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::all_of( A.begin() + start , A.begin() + j , below ) &&
          std::none_of( A.begin() + j + 1 , A.begin() + end + 1 ,
                        below ) ); } ) );
 return( j );
 }

template< class T , class C >
void quickSort( std::vector< T > & A , std::size_t start , std::size_t end ,
                C less )
{
 ADS_PRE( ( start > end ) || ( end < A.size() ) );
 if( start < end ) {
  const std::size_t j = partition( A , start , end , less );
  if( j > start )
   quickSort( A , start , j - 1 , less );
  quickSort( A , j + 1 , end , less );
  }
 }

template< class T , class C >
void quickSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 if( A.size() > 1 )
  quickSort( A , 0 , A.size() - 1 , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void quickSortTuned( std::vector< T > & A , std::size_t cutoff ,
                     C less , std::size_t * depth )
{
 if( A.size() < 2 )
  return;
 const auto before = detail::auditCopy( A );
 const auto sorted = [ & ]( void ) {
  return( detail::sortedPermutation( A , before , less ) ); };
 cutoff = cutoff < 3 ? 3 : cutoff;
 std::vector< std::pair< std::size_t , std::size_t > > S;   // [ lo , hi ]
 std::size_t lo = 0 , hi = A.size() - 1;
 for( ; ; ) {
  if( hi - lo + 1 <= cutoff ) {
   insertionSort( A , lo , hi + 1 , less );
   if( S.empty() ) {
    ADS_AUDIT( sorted() );
    return;
    }
   std::tie( lo , hi ) = S.back();
   S.pop_back();
   continue;
   }
  // the three samples sorted, the median, the pivot, in A[ hi - 1 ]: A[ lo ]
  // and A[ hi - 1 ] are then the sentinels of the two scans
  const std::size_t mid = lo + ( hi - lo ) / 2;
  if( less( A[ mid ] , A[ lo ] ) ) std::swap( A[ mid ] , A[ lo ] );
  if( less( A[ hi ] , A[ lo ] ) ) std::swap( A[ hi ] , A[ lo ] );
  if( less( A[ hi ] , A[ mid ] ) ) std::swap( A[ hi ] , A[ mid ] );
  std::swap( A[ mid ] , A[ hi - 1 ] );
  const T x = A[ hi - 1 ];
  std::size_t i = lo , j = hi - 1;
  for( ; ; ) {                                // the partition of Hoare
   while( less( A[ ++i ] , x ) ) {}
   while( less( x , A[ --j ] ) ) {}
   if( i >= j )
    break;
   std::swap( A[ i ] , A[ j ] );
   }
  std::swap( A[ i ] , A[ hi - 1 ] );          // the pivot in its place
  j = i;
  // the larger part on the stack, the smaller one now
  std::pair< std::size_t , std::size_t > left( lo , j ? j - 1 : 0 ) ,
                                         right( j + 1 , hi );
  const bool hasLeft = j > lo , hasRight = j < hi;
  if( hasLeft && hasRight ) {
   if( left.second - left.first < right.second - right.first ) {
    S.push_back( right );
    std::tie( lo , hi ) = left;
    }
   else {
    S.push_back( left );
    std::tie( lo , hi ) = right;
    }
   }
  else if( hasLeft )
   std::tie( lo , hi ) = left;
  else if( hasRight )
   std::tie( lo , hi ) = right;
  else if( S.empty() ) {
   ADS_AUDIT( sorted() );
   return;
   }
  else {
   std::tie( lo , hi ) = S.back();
   S.pop_back();
   }
  if( depth && ( S.size() > *depth ) )
   *depth = S.size();
  }
 }

template< class T , class C >
void quickSort( List< T > & L , C less )
{
 if( L.empty() || L.finished( L.next( L.head() ) ) )
  return;
 const T x = L.read( L.head() );
 List< T > S , E , G;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.remove( p ) ) {
  const T & y = L.read( p );
  List< T > & D = less( y , x ) ? S : less( x , y ) ? G : E;
  D.insert( D.next( D.tail() ) , y );
  }
 quickSort( S , less );
 quickSort( G , less );
 for( auto * D : { &S , &E , &G } )
  for( auto p = D->head() ; ! D->finished( p ) ; p = D->next( p ) )
   L.insert( L.next( L.tail() ) , D->read( p ) );
 ADS_AUDIT( detail::sequenceSorted( L , less ) );
 }

template< class T , class C >
std::size_t partitionByValue( std::vector< T > & A , std::size_t start ,
                              std::size_t end , const T & x , C less )
{
 ADS_PRE( ( start <= end ) && ( end < A.size() ) );
 const auto before = detail::auditCopy( A , start , end + 1 );
 std::size_t i = start , j = end + 1;       // A[ start .. i ) <= x ,
 for( ; ; ) {                               // A[ j .. end ] > x
  while( ( i < j ) && ! less( x , A[ i ] ) ) ++i;
  while( ( i < j ) && less( x , A[ j - 1 ] ) ) --j;
  if( i + 1 >= j )
   break;
  std::swap( A[ i++ ] , A[ --j ] );
  }
 const std::size_t r = i - start;
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  for( std::size_t k = start ; k <= end ; ++k )
   if( ( k < start + r ) == less( x , A[ k ] ) )
    return( false );
  return( true );
  } ) && detail::samePermutation( A , before , less , start , end + 1 ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File QuickSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
