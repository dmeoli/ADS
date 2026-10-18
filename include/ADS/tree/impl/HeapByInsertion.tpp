/*--------------------------------------------------------------------------*/
/*------------------------ File HeapByInsertion.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of HeapByInsertion.h, with their contracts.
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
void maxHeapSiftUp( std::vector< T > & A , std::size_t i , C less )
{
 ADS_PRE( i < A.size() );
 ADS_AUDIT( isMaxHeap( A , i , less ) );
 const std::size_t dim = i + 1;
 while( i > 0 ) {
  const std::size_t p = ( i - 1 ) / 2;       // the parent
  if( ! less( A[ p ] , A[ i ] ) ) break;
  std::swap( A[ p ] , A[ i ] );
  i = p;
  }
 ADS_AUDIT( isMaxHeap( A , dim , less ) );
 }

template< class T , class C >
void heapBuildByInsertion( std::vector< T > & A , C less )
{
 for( std::size_t i = 1 ; i < A.size() ; ++i )
  maxHeapSiftUp( A , i , less );
 ADS_POST( isMaxHeap( A , A.size() , less ) );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HeapByInsertion.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
