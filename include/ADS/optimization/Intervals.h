/*--------------------------------------------------------------------------*/
/*---------------------------- File Intervals.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Greedy algorithms on intervals [ a_i , b_i ) of the line:
 *
 * - independentSet() (section 14.2), a largest set of disjoint intervals:
 *   by increasing right end, an interval that does not meet the last one
 *   taken is taken; optimal, since an optimal set can always swap its
 *   first interval for the one that ends first; O( n log n );
 * - intervalColoring() (exercise 14.6), the fewest colors such that the
 *   intervals that meet have different colors: by scanning the 2 n ends,
 *   a left end takes a free color from a stack and a right one gives it
 *   back; the colors are as many as the largest number of intervals over
 *   a point (exercise 9.8), which is a lower bound, O( n log n );
 * - towers() (exercise 14.7), the fewest points such that each of the
 *   given ones is within d of one of them: a tower at k_0 + d serves all
 *   the houses up to k_0 + 2 d, and so on from the first one left out;
 *   O( n log n ) for the sort.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.2 and
 *      exercises 14.6 and 14.7,
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

#ifndef __ADS_Intervals
 #define __ADS_Intervals
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// independentSet(): the places of a largest set of disjoint intervals
/// [ a , b ), in increasing order
inline std::vector< std::size_t > independentSet(
                           const std::vector< std::pair< long , long > > & I )
{
 std::vector< std::size_t > by( I.size() );
 std::iota( by.begin() , by.end() , 0 );
 std::stable_sort( by.begin() , by.end() ,
                   [ & ]( std::size_t x , std::size_t y ) {
                    return( I[ x ].second < I[ y ].second ); } );
 std::vector< std::size_t > S;
 for( std::size_t i : by )
  if( S.empty() || ( I[ i ].first >= I[ S.back() ].second ) )
   S.push_back( i );
 std::sort( S.begin() , S.end() );
 return( S );
 }

/// the colors, from 0, of the intervals [ a , b ), the fewest possible
inline std::vector< int > intervalColoring(
                           const std::vector< std::pair< long , long > > & I )
{
 std::vector< std::pair< long , long > > E;      // ( end , -i-1 or +i+1 )
 for( std::size_t i = 0 ; i < I.size() ; ++i ) {
  E.emplace_back( I[ i ].first , long( i ) + 1 );
  E.emplace_back( I[ i ].second , -long( i ) - 1 );
  }
 std::sort( E.begin() , E.end() );              // a right end first
 std::vector< int > color( I.size() , -1 ) , free;
 int used = 0;
 for( auto [ x , e ] : E )
  if( e > 0 ) {
   if( free.empty() )
    free.push_back( used++ );
   color[ e - 1 ] = free.back();
   free.pop_back();
   }
  else
   free.push_back( color[ -e - 1 ] );
 return( color );
 }

/// torri(): the fewest towers such that each house is within d of one
inline std::vector< double > towers( std::vector< double > k , double d )
{
 std::sort( k.begin() , k.end() );
 std::vector< double > S;
 for( double x : k )
  if( S.empty() || ( x > S.back() + d ) )
   S.push_back( x + d );
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Intervals.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Intervals.h --------------------------*/
/*--------------------------------------------------------------------------*/
