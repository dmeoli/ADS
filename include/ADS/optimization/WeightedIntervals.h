/*--------------------------------------------------------------------------*/
/*------------------------ File WeightedIntervals.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The independent set of weighted intervals of section 13.2: among n
 * intervals [ a_i , b_i ), open on the right, of weights w_i, a set of
 * disjoint ones of largest weight. With the intervals by increasing right
 * end and pred[ i ] the largest j < i with b_{j-1} <= a_{i-1} (0 if none),
 * \f[
 *   DP[ i ] = \max\{ DP[ i - 1 ] , w_{i-1} + DP[ pred[ i ] ] \} ,
 * \f]
 * DP[ 0 ] = 0, and the set is read back from DP[ n ]; the predecessors come
 * from one scan of the 2 n ends sorted, so that maxinterval() takes
 * O( n log n ) time for the sort and O( n ) for the rest.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#ifndef __ADS_WeightedIntervals
 #define __ADS_WeightedIntervals
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

/// an interval [ a , b ) of weight w
template< class W >
struct WeightedInterval {
 long a , b;
 W w;
 };

/// maxinterval(): the places of the intervals of a largest independent
/// set, in increasing order, and its weight
template< class W >
std::pair< std::vector< std::size_t > , W > maxinterval(
                              const std::vector< WeightedInterval< W > > & I )
{
 const std::size_t n = I.size();
 std::vector< std::size_t > by( n );                // by increasing b
 std::iota( by.begin() , by.end() , 0 );
 std::stable_sort( by.begin() , by.end() , [ & ]( std::size_t x ,
                   std::size_t y ) { return( I[ x ].b < I[ y ].b ); } );
 // the ends sorted, a right end before a left one equal to it
 std::vector< std::pair< long , long > > E;         // ( end , +-( i + 1 ) )
 for( std::size_t i = 0 ; i < n ; ++i ) {
  E.emplace_back( I[ by[ i ] ].a , long( i + 1 ) );
  E.emplace_back( I[ by[ i ] ].b , -long( i + 1 ) );
  }
 std::sort( E.begin() , E.end() );
 std::vector< std::size_t > pred( n + 1 , 0 );
 std::size_t h = 0;
 for( auto [ x , e ] : E )
  if( e > 0 )
   pred[ e ] = h;
  else
   h = std::max( h , std::size_t( -e ) );
 std::vector< W > DP( n + 1 , W( 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  DP[ i ] = std::max( DP[ i - 1 ] , I[ by[ i - 1 ] ].w + DP[ pred[ i ] ] );
 std::vector< std::size_t > S;
 for( std::size_t i = n ; i > 0 ; )
  if( DP[ i - 1 ] >= I[ by[ i - 1 ] ].w + DP[ pred[ i ] ] )
   --i;
  else {
   S.push_back( by[ i - 1 ] );
   i = pred[ i ];
   }
 std::sort( S.begin() , S.end() );
 return( std::pair( S , DP[ n ] ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* WeightedIntervals.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File WeightedIntervals.h ----------------------*/
/*--------------------------------------------------------------------------*/
