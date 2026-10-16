/*--------------------------------------------------------------------------*/
/*--------------------------- File MatrixChain.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multiplication of a chain of matrices of section 13.3: A_i of size
 * c[ i ] x c[ i + 1 ], 0 <= i < n, and the order of the products with the
 * fewest scalar multiplications. The number of parenthesizations is the
 * Catalan number \f$ F( n ) = \Omega( 4^n / n^{3/2} ) \f$, but
 * \f[
 *   DP[ i ][ j ] = \min_{i \leq k < j} \{ DP[ i ][ k ] + DP[ k + 1 ][ j ] +
 *   c[ i ] c[ k + 1 ] c[ j + 1 ] \} , \qquad DP[ i ][ i ] = 0 ,
 * \f]
 * filled by increasing length h = j - i + 1 in O( n^3 ) time, with S[ i ][ j ]
 * the best k, from which stampaPar() writes the parenthesization in O( n ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see T. C. Hu, M. T. Shing, Computation of matrix chain products, part I,
 *      SIAM Journal on Computing 11(2), 362-373, 1982,
 *      doi:10.1137/0211028
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

#ifndef __ADS_MatrixChain
 #define __ADS_MatrixChain
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// parentesizzazione(): the least cost DP[ 0 ][ n - 1 ] and the table S of
/// the splits, for the n matrices of sizes c[ 0 .. n ]
inline std::pair< long , std::vector< std::vector< std::size_t > > >
matrixChain( const std::vector< long > & c )
{
 const std::size_t n = c.size() - 1;
 std::vector< std::vector< long > > DP( n , std::vector< long >( n , 0 ) );
 std::vector< std::vector< std::size_t > > S( n ,
                                             std::vector< std::size_t >( n ) );
 for( std::size_t h = 2 ; h <= n ; ++h )
  for( std::size_t i = 0 ; i + h <= n ; ++i ) {
   const std::size_t j = i + h - 1;
   DP[ i ][ j ] = std::numeric_limits< long >::max();
   for( std::size_t k = i ; k < j ; ++k ) {
    const long t = DP[ i ][ k ] + DP[ k + 1 ][ j ] +
                   c[ i ] * c[ k + 1 ] * c[ j + 1 ];
    if( t < DP[ i ][ j ] ) {
     DP[ i ][ j ] = t;
     S[ i ][ j ] = k;
     }
    }
   }
 return( std::pair( n ? DP[ 0 ][ n - 1 ] : 0 , S ) );
 }

/// stampaPar(): the parenthesization of A_i ... A_j, as "((A0·A1)·A2)"
inline std::string parenthesization(
                         const std::vector< std::vector< std::size_t > > & S ,
                         std::size_t i , std::size_t j )
{
 if( i == j )
  return( "A" + std::to_string( i ) );
 return( "(" + parenthesization( S , i , S[ i ][ j ] ) + "·" +
         parenthesization( S , S[ i ][ j ] + 1 , j ) + ")" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MatrixChain.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MatrixChain.h -------------------------*/
/*--------------------------------------------------------------------------*/
