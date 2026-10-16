/*--------------------------------------------------------------------------*/
/*---------------------------- File Knapsack.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The 0-1 knapsack of section 13.6: n objects of profit p_i and volume
 * v_i, positive integers, and a knapsack of capacity C; a set S of volume
 * at most C and largest profit. DP[ i ][ c ], the best profit of the first
 * i objects in capacity c, is \f$ -\infty \f$ if c < 0, 0 if i = 0 or
 * c = 0, and \f$ \max\{ DP[ i - 1 ][ c ] , DP[ i - 1 ][ c - v_{i-1} ] +
 * p_{i-1} \} \f$ otherwise.
 *
 * knapsack() fills the table bottom up in Theta( n C ) time and reads the
 * set back; zaino() is the memoization of the book, top down, which solves
 * only the subproblems it reaches, in a hash table rather than in a table
 * of n C places, whose initialization alone would cost Theta( n C ).
 * Either is pseudopolynomial: C takes log C bits of the input.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.6,
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

#ifndef __ADS_Knapsack
 #define __ADS_Knapsack
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/ChainedHashTable.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the best profit and a set of objects, by places, of the 0-1 knapsack
inline std::pair< long , std::vector< std::size_t > > knapsack(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C )
{
 const std::size_t n = p.size();
 std::vector< std::vector< long > > DP( n + 1 ,
                                       std::vector< long >( C + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( long c = 1 ; c <= C ; ++c ) {
   DP[ i ][ c ] = DP[ i - 1 ][ c ];
   if( v[ i - 1 ] <= c )
    DP[ i ][ c ] = std::max( DP[ i ][ c ] ,
                             DP[ i - 1 ][ c - v[ i - 1 ] ] + p[ i - 1 ] );
   }
 std::vector< std::size_t > S;
 for( std::size_t i = n , c = std::size_t( C ) ; i > 0 ; --i )
  if( DP[ i ][ c ] != DP[ i - 1 ][ c ] ) {
   S.push_back( i - 1 );
   c -= std::size_t( v[ i - 1 ] );
   }
 std::reverse( S.begin() , S.end() );
 return( std::pair( DP[ n ][ C ] , S ) );
 }

/// zaino(): the best profit by memoization in a hash table; the number of
/// subproblems solved goes to *solved, if given
inline long zaino( const std::vector< long > & p ,
                   const std::vector< long > & v , long C ,
                   long * solved = nullptr )
{
 const long NEG = std::numeric_limits< long >::min() / 2;
 ChainedHashTable< long , long > DP( 64 , true );   // key i ( C + 1 ) + c
 auto rec = [ & ]( auto & self , long i , long c ) -> long {
  if( c < 0 )
   return( NEG );
  if( ( i == 0 ) || ( c == 0 ) )
   return( 0 );
  const long key = i * ( C + 1 ) + c;
  if( auto r = DP.lookup( key ) )
   return( *r );
  if( solved ) ++*solved;
  const long r = std::max( self( self , i - 1 , c ) ,
                           self( self , i - 1 , c - v[ i - 1 ] ) +
                           p[ i - 1 ] );
  DP.insert( key , r );
  return( r );
  };
 return( rec( rec , long( p.size() ) , C ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Knapsack.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Knapsack.h ---------------------------*/
/*--------------------------------------------------------------------------*/
