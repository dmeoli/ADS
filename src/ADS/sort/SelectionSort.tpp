/*--------------------------------------------------------------------------*/
/*------------------------- File SelectionSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of SelectionSort.h [see SelectionSort.h].
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
void selectionSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 for( std::size_t i = 0 ; i + 1 < A.size() ; ++i ) {
  const std::size_t pos = argmin( A , i , A.size() - 1 , less );
  std::swap( A[ i ] , A[ pos ] );            // extends the sorted prefix
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SelectionSort.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
