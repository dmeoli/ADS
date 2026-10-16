/*--------------------------------------------------------------------------*/
/*------------------------- File PigeonholeSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Pigeonhole Sort (exercise 4.11) of the pairs ( key , value ) with keys
 * in [ 0 , k ): the value of each pair goes at the end of the LIST of its
 * key, then the lists are read in the order of the keys; O( n + k ) time,
 * and stable, unlike Counting Sort, which only counts the keys. A key out
 * of [ 0 , k ) throws std::out_of_range.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_PigeonholeSort
 #define __ADS_PigeonholeSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.h"

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the pairs sorted by key, the values of a key in their first order
template< class V >
std::vector< std::pair< int , V > > pigeonholeSort(
                    const std::vector< std::pair< int , V > > & A , int k )
{
 std::vector< List< V > > B( k );
 for( const auto & [ key , v ] : A ) {
  if( ( key < 0 ) || ( key >= k ) )
   throw( std::out_of_range( "ADS::pigeonholeSort: a key out of [0, k)" ) );
  B[ key ].insert( B[ key ].next( B[ key ].tail() ) , v );
  }
 std::vector< std::pair< int , V > > out;
 out.reserve( A.size() );
 for( int i = 0 ; i < k ; ++i )
  for( auto p = B[ i ].head() ; ! B[ i ].finished( p ) ; p = B[ i ].next( p ) )
   out.emplace_back( i , B[ i ].read( p ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* PigeonholeSort.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File PigeonholeSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
