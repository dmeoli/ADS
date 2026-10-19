/*--------------------------------------------------------------------------*/
/*------------------------- File InsertionSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of InsertionSort.h [see InsertionSort.h].
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
void insertionSort( std::vector< T > & A , std::size_t lo , std::size_t hi ,
                    C less )
{
 ADS_PRE( ( lo <= hi ) && ( hi <= A.size() ) );
 const auto before = detail::auditCopy( A , lo , hi );
 T * const a = A.data();                     // not reloaded at each move
 for( std::size_t i = lo + 1 ; i < hi ; ++i ) {
  T temp = std::move( a[ i ] );               // the item to insert
  std::size_t j = i;
  while( ( j > lo ) && less( temp , a[ j - 1 ] ) ) {
   a[ j ] = std::move( a[ j - 1 ] );          // a larger item to the right
   --j;
   }
  a[ j ] = std::move( temp );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less , lo , hi ) );
 }

template< class T , class C >
void insertionSort( std::vector< T > & A , C less )
{
 insertionSort( A , 0 , A.size() , less );
 }

template< class T , class C >
void insertionSortSentinel( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 std::size_t m = 0;                         // the sentinel
 for( std::size_t i = 1 ; i < n ; ++i )
  if( less( A[ i ] , A[ m ] ) ) m = i;
 std::swap( A[ 0 ] , A[ m ] );
 for( std::size_t i = 2 ; i < n ; ++i ) {
  T x = std::move( A[ i ] );
  std::size_t j = i;
  while( less( x , A[ j - 1 ] ) ) {         // no test on j
   A[ j ] = std::move( A[ j - 1 ] );
   --j;
   }
  A[ j ] = std::move( x );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File InsertionSort.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
