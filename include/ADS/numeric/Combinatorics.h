/*--------------------------------------------------------------------------*/
/*-------------------------- File Combinatorics.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The examples of the introduction and of section 13.4 of chapter 13:
 *
 * - binomial( n , k ) by its recursive definition
 *   \f$ C( n , k ) = C( n - 1 , k - 1 ) + C( n - 1 , k ) \f$, whose calls
 *   are as many as \f$ 2 C( n , k ) - 1 \f$, and tartaglia(), the triangle
 *   of Tartaglia of all the C( m , k ), m <= n, in Theta( n^2 ) time, each
 *   then read in O( 1 );
 * - fibonacci(), F_0 = F_1 = 1 and F_n = F_{n-1} + F_{n-2}, with a vector
 *   of n places and with three variables, O( n ) time and O( 1 ) space,
 *   while the recursion makes about \f$ 1.6^n \f$ calls.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
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

#ifndef __ADS_Combinatorics
 #define __ADS_Combinatorics
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// C( n , k ) by the recursion, the calls added to *calls, if given
inline std::uint64_t binomial( int n , int k , long * calls = nullptr )
{
 if( calls ) ++*calls;
 if( ( k == 0 ) || ( k == n ) )
  return( 1 );
 return( binomial( n - 1 , k - 1 , calls ) + binomial( n - 1 , k , calls ) );
 }

/// tartaglia(): the table of C( m , k ), 0 <= k <= m <= n
inline std::vector< std::vector< std::uint64_t > > tartaglia( int n )
{
 std::vector< std::vector< std::uint64_t > > C( n + 1 );
 for( int i = 0 ; i <= n ; ++i ) {
  C[ i ].assign( i + 1 , 1 );
  for( int j = 1 ; j < i ; ++j )
   C[ i ][ j ] = C[ i - 1 ][ j - 1 ] + C[ i - 1 ][ j ];
  }
 return( C );
 }

/// the n-th number of Fibonacci, F_0 = F_1 = 1, with a vector
inline std::uint64_t fibonacciTable( int n )
{
 std::vector< std::uint64_t > F( n < 1 ? 2 : n + 1 );
 F[ 0 ] = F[ 1 ] = 1;
 for( int i = 2 ; i <= n ; ++i )
  F[ i ] = F[ i - 1 ] + F[ i - 2 ];
 return( F[ n ] );
 }

/// the n-th number of Fibonacci, F_0 = F_1 = 1, in O( 1 ) space
inline std::uint64_t fibonacci( int n )
{
 std::uint64_t F0 = 1 , F1 = 1 , F2 = 1;
 for( int i = 2 ; i <= n ; ++i ) {
  F0 = F1;
  F1 = F2;
  F2 = F0 + F1;
  }
 return( F2 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Combinatorics.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Combinatorics.h ------------------------*/
/*--------------------------------------------------------------------------*/
