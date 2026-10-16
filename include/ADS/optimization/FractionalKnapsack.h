/*--------------------------------------------------------------------------*/
/*----------------------- File FractionalKnapsack.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The real (fractional) knapsack of section 14.1: a fraction x_i in
 * [ 0 , 1 ] of each object, of largest profit \f$ \sum_i p_i x_i \f$ and
 * volume \f$ \sum_i v_i x_i \leq C \f$. The greedy choice by decreasing
 * specific profit \f$ p_i / v_i \f$ is optimal: the most profitable objects
 * per unit of volume go in whole, the next one in part, the others not;
 * O( n log n ) time for the sort. By decreasing profit or by increasing
 * volume it is not, and neither is any of them for the 0-1 knapsack
 * (exercise 14.1).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see G. B. Dantzig, Discrete-variable extremum problems, Operations
 *      Research 5(2), 266-288, 1957, doi:10.1287/opre.5.2.266
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

#ifndef __ADS_FractionalKnapsack
 #define __ADS_FractionalKnapsack
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

/// zaino(): the fractions x of the objects, by decreasing p / v
inline std::vector< double > fractionalKnapsack(
                                            const std::vector< double > & p ,
                                            const std::vector< double > & v ,
                                            double C )
{
 const std::size_t n = p.size();
 std::vector< std::size_t > by( n );
 std::iota( by.begin() , by.end() , 0 );
 std::stable_sort( by.begin() , by.end() ,
                   [ & ]( std::size_t a , std::size_t b ) {
                    return( p[ a ] * v[ b ] > p[ b ] * v[ a ] ); } );
 std::vector< double > x( n , 0 );
 for( std::size_t i : by ) {
  x[ i ] = std::min( C / v[ i ] , 1.0 );
  C -= x[ i ] * v[ i ];
  }
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* FractionalKnapsack.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File FractionalKnapsack.h ----------------------*/
/*--------------------------------------------------------------------------*/
