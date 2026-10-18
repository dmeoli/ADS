/*--------------------------------------------------------------------------*/
/*--------------------------- File CycleSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of CycleSort.h, with their contracts.
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
std::size_t cycleSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 std::size_t writes = 0;
 for( std::size_t start = 0 ; start + 1 < n ; ++start ) {
  T item = A[ start ];
  auto place = [ & ]( const T & x ) {       // start + the smaller items
   std::size_t p = start;
   for( std::size_t i = start + 1 ; i < n ; ++i )
    if( less( A[ i ] , x ) ) ++p;
   return( p );
   };
  std::size_t p = place( item );
  if( p == start )                          // already in its place
   continue;
  while( ! less( item , A[ p ] ) && ! less( A[ p ] , item ) )
   ++p;                                     // after its equals
  std::swap( item , A[ p ] );
  ++writes;
  while( p != start ) {                     // the rest of the cycle
   p = place( item );
   while( ! less( item , A[ p ] ) && ! less( A[ p ] , item ) &&
          ( p != start ) )
    ++p;
   std::swap( item , A[ p ] );
   ++writes;
   }
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 ADS_POST( writes <= n );
 return( writes );
 }

template< class T , class C >
std::size_t pancakeSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 std::size_t flips = 0;
 for( std::size_t k = A.size() ; k > 1 ; --k ) {   // A[ 0 .. k ) unsorted
  std::size_t m = 0;
  for( std::size_t i = 1 ; i < k ; ++i )
   if( less( A[ m ] , A[ i ] ) ) m = i;
  if( m == k - 1 )
   continue;                                // already at the end
  if( m > 0 ) {
   std::reverse( A.begin() , A.begin() + m + 1 );   // to the front
   ++flips;
   }
  std::reverse( A.begin() , A.begin() + k );        // to the end
  ++flips;
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 ADS_POST( flips <= ( A.size() < 2 ? 0 : 2 * A.size() - 3 ) );
 return( flips );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File CycleSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
