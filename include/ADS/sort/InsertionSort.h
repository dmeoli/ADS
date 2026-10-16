/*--------------------------------------------------------------------------*/
/*-------------------------- File InsertionSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Insertion Sort (section 2.7.2), as one sorts a hand of cards: A[ i ] is
 * inserted in the sorted prefix A[ 0 .. i - 1 ], the larger items moving
 * one place to the right. \f$ \Theta( n ) \f$ time on a sorted vector,
 * \f$ \Theta( n^2 ) \f$ on a reversed one and on average (n ( n - 1 ) / 4
 * moves), O( 1 ) space, stable; with its simple inner loop it is the
 * fastest on few items, hence the base case of the hybrid sorts.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_InsertionSort
 #define __ADS_InsertionSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// insertionSort() of A[ lo .. hi - 1 ]
template< class T , class C = std::less<> >
void insertionSort( std::vector< T > & A , std::size_t lo , std::size_t hi ,
                    C less = C() )
{
 for( std::size_t i = lo + 1 ; i < hi ; ++i ) {
  T temp = std::move( A[ i ] );               // the item to insert
  std::size_t j = i;
  while( ( j > lo ) && less( temp , A[ j - 1 ] ) ) {
   A[ j ] = std::move( A[ j - 1 ] );          // a larger item to the right
   --j;
   }
  A[ j ] = std::move( temp );
  }
 }

/// insertionSort(): A in increasing order by less
template< class T , class C = std::less<> >
void insertionSort( std::vector< T > & A , C less = C() )
{
 insertionSort( A , 0 , A.size() , less );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* InsertionSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File InsertionSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
