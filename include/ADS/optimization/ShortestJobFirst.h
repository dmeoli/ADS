/*--------------------------------------------------------------------------*/
/*------------------------ File ShortestJobFirst.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Shortest Job First (exercise 14.5): n programs of durations t_i, run one
 * after the other, in the order that minimizes the average time of
 * completion: by increasing duration, since moving the shortest program to
 * the front of any order does not increase any time of completion;
 * O( n log n ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 14.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
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

#ifndef __ADS_ShortestJobFirst
 #define __ADS_ShortestJobFirst
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the order of the programs, by increasing duration
template< class T >
std::vector< std::size_t > shortestJobFirst( const std::vector< T > & t )
{
 std::vector< std::size_t > A( t.size() );
 std::iota( A.begin() , A.end() , 0 );
 std::stable_sort( A.begin() , A.end() , [ & ]( std::size_t a ,
                   std::size_t b ) { return( t[ a ] < t[ b ] ); } );
 return( A );
 }

/// the sum of the times of completion of the programs in the order A
template< class T >
T totalCompletion( const std::vector< T > & t ,
                   const std::vector< std::size_t > & A )
{
 T now = T( 0 ) , sum = T( 0 );
 for( std::size_t i : A )
  sum += ( now += t[ i ] );
 return( sum );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ShortestJobFirst.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ShortestJobFirst.h -----------------------*/
/*--------------------------------------------------------------------------*/
