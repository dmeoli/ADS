/*--------------------------------------------------------------------------*/
/*-------------------------- File CountingSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Counting Sort (section 2.7.5) of integers in [ 0 , k ): B[ v ] counts
 * the occurrences of v, then each v is written B[ v ] times in order;
 * \f$ \Theta( n + k ) \f$ time and \f$ \Theta( k ) \f$ space, linear when
 * k = O( n ). The lower bound \f$ \Omega( n \log n ) \f$ of the sorts by
 * comparisons does not apply, since it compares no item: it uses that the
 * keys are small integers. A key out of [ 0 , k ) throws
 * std::out_of_range.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.5,
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

#ifndef __ADS_CountingSort
 #define __ADS_CountingSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// countingSort(): A, of integers in [ 0 , k ), in increasing order
template< class T >
void countingSort( std::vector< T > & A , std::size_t k )
{
 std::vector< std::size_t > B( k , 0 );          // the occurrences
 for( const T & a : A ) {
  if( ( a < 0 ) || ( std::size_t( a ) >= k ) )
   throw( std::out_of_range( "ADS::countingSort: a key out of [0, k)" ) );
  ++B[ a ];
  }
 std::size_t j = 0;
 for( std::size_t i = 0 ; i < k ; ++i )
  for( std::size_t c = B[ i ] ; c > 0 ; --c )
   A[ j++ ] = T( i );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* CountingSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File CountingSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
