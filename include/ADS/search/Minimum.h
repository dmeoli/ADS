/*--------------------------------------------------------------------------*/
/*----------------------------- File Minimum.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum of a vector A[ 0 .. n - 1 ] (section 1.2): min() keeps the
 * minimum so far and compares it with each of the other items, n - 1
 * comparisons, which are needed, since each comparison can exclude at most
 * one of the two items from being the minimum (section 2.6). argmin()
 * gives the place of the minimum of A[ start .. end ], the first one if it
 * occurs more than once.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.2 and 2.7.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
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

#ifndef __ADS_Minimum
 #define __ADS_Minimum
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// argmin(): the place of the first minimum of A[ start .. end ]
template< class T , class C = std::less<> >
std::size_t argmin( const std::vector< T > & A , std::size_t start ,
                    std::size_t end , C less = C() )
{
 std::size_t pos = start;
 for( std::size_t j = start + 1 ; j <= end ; ++j )
  if( less( A[ j ] , A[ pos ] ) )
   pos = j;
 return( pos );
 }

/// min(): the minimum of a vector that is not empty, n - 1 comparisons
template< class T , class C = std::less<> >
const T & min( const std::vector< T > & A , C less = C() )
{
 if( A.empty() )
  throw( std::invalid_argument( "ADS::min: empty vector" ) );
 return( A[ argmin( A , 0 , A.size() - 1 , less ) ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Minimum.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Minimum.h ---------------------------*/
/*--------------------------------------------------------------------------*/
