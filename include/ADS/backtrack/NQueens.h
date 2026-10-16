/*--------------------------------------------------------------------------*/
/*----------------------------- File NQueens.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The n queens (exercise 16.5): n queens on an n x n board, no two on a row, a
 * column or a diagonal. A row holds exactly one queen, and so the solution is
 * the vector Q of their columns, a permutation: n! candidates instead of
 * 2^{n^2} placements; the backtrack prunes the columns and the two diagonals
 * already taken, kept as masks of bits, so that a choice is checked in O( 1 ).
 * The numbers of solutions are 1, 0, 0, 2, 10, 4, 40, 92, 352, 724 for n = 1 ,
 * ... , 10.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see J. Bell, B. Stevens, A survey of known results and research areas
 *      for n-queens, Discrete Mathematics 309(1), 1-31, 2009,
 *      doi:10.1016/j.disc.2007.12.043
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

#ifndef __ADS_NQueens
 #define __ADS_NQueens
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the placements of n <= 32 queens, f( Q ) on each, Q[ r ] the column of
/// the queen of row r; f returns true to stop; the number found
template< class F >
long nQueens( int n , F && f )
{
 if( ( n < 1 ) || ( n > 32 ) )
  throw( std::invalid_argument( "ADS::nQueens: n not in [1, 32]" ) );
 std::vector< int > Q( n );
 long count = 0;
 bool stop = false;
 const std::uint64_t all = ( std::uint64_t( 1 ) << n ) - 1;
 auto rec = [ & ]( auto & self , int r , std::uint64_t cols ,
                   std::uint64_t d1 , std::uint64_t d2 ) -> void {
  if( stop ) return;
  if( r == n ) {
   ++count;
   stop = f( Q );
   return;
   }
  for( std::uint64_t free = all & ~( cols | d1 | d2 ) ; free && ! stop ;
       free &= free - 1 ) {
   const std::uint64_t bit = free & ( ~free + 1 );
   Q[ r ] = std::countr_zero( bit );
   self( self , r + 1 , cols | bit , ( ( d1 | bit ) << 1 ) & all ,
         ( d2 | bit ) >> 1 );
   }
  };
 rec( rec , 0 , 0 , 0 , 0 );
 return( count );
 }

/// admissible(): no two queens attack each other
inline bool admissible( const std::vector< int > & Q )
{
 for( std::size_t i = 0 ; i < Q.size() ; ++i )
  for( std::size_t j = i + 1 ; j < Q.size() ; ++j )
   if( ( Q[ i ] == Q[ j ] ) ||
       ( std::abs( Q[ i ] - Q[ j ] ) == int( j - i ) ) )
    return( false );
 return( true );
 }

/// the board, a line per row, "Q" a queen and "." an empty square
inline std::string board( const std::vector< int > & Q )
{
 std::string s;
 for( int c : Q ) {
  for( std::size_t j = 0 ; j < Q.size() ; ++j )
   s += int( j ) == c ? 'Q' : '.';
  s += '\n';
  }
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* NQueens.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File NQueens.h ---------------------------*/
/*--------------------------------------------------------------------------*/
