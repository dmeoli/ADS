/*--------------------------------------------------------------------------*/
/*--------------------------- File CoinChange.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The change with the fewest coins (introduction of chapter 14): the
 * greedy choice takes the largest coin that fits, and is optimal for
 * coins of 50, 10, 5 and 1 cents, but not for coins of 11, 5 and 1, where
 * it pays 15 with five coins instead of three of 5. optimalChange() is the
 * dynamic programming, the fewest coins N[ c ] of each amount c,
 * \f$ N[ c ] = 1 + \min_{d \leq c} N[ c - d ] \f$, O( n C ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 14,
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

#ifndef __ADS_CoinChange
 #define __ADS_CoinChange
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the coins taken by the greedy choice, the coins in any order; nothing
/// if it cannot pay
inline std::optional< std::vector< long > > greedyChange(
                                       std::vector< long > coins , long c )
{
 std::sort( coins.rbegin() , coins.rend() );
 std::vector< long > out;
 for( long d : coins )
  for( ; ( d > 0 ) && ( c >= d ) ; c -= d )
   out.push_back( d );
 if( c != 0 )
  return( std::nullopt );
 return( out );
 }

/// the fewest coins that pay c, nothing if none
inline std::optional< long > optimalChange( const std::vector< long > & coins ,
                                            long c )
{
 const long INF = std::numeric_limits< long >::max();
 std::vector< long > N( c + 1 , INF );
 N[ 0 ] = 0;
 for( long x = 1 ; x <= c ; ++x )
  for( long d : coins )
   if( ( d > 0 ) && ( d <= x ) && ( N[ x - d ] != INF ) )
    N[ x ] = std::min( N[ x ] , N[ x - d ] + 1 );
 if( N[ c ] == INF )
  return( std::nullopt );
 return( N[ c ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* CoinChange.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CoinChange.h --------------------------*/
/*--------------------------------------------------------------------------*/
