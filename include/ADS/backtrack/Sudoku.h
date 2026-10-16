/*--------------------------------------------------------------------------*/
/*----------------------------- File Sudoku.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Sudoku (reality check of section 16.3 and exercise 16.7), of size
 * n^2 x n^2 with the numbers 1 , ... , n^2, n = 3 the classic one: the
 * squares are filled in order, each with the numbers that check() allows,
 * no equal number on its row, its column and its block of n x n, and the
 * search backtracks when a square has none. The cost is a constant for
 * n = 3, a huge one, and superpolynomial in n. 0 marks an empty square.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.3 and
 *      exercise 16.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#ifndef __ADS_Sudoku
 #define __ADS_Sudoku
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Grid = std::vector< std::vector< int > >;   ///< the squares, 0 empty

/// check(): c can be written in the square ( x , y )
inline bool check( const Grid & S , std::size_t x , std::size_t y , int c )
{
 const std::size_t N = S.size();
 std::size_t n = 1;
 while( n * n < N ) ++n;
 for( std::size_t j = 0 ; j < N ; ++j )
  if( ( S[ x ][ j ] == c ) || ( S[ j ][ y ] == c ) )   // the row, the column
   return( false );
 const std::size_t bx = x / n * n , by = y / n * n;
 for( std::size_t i = 0 ; i < n ; ++i )                 // the block
  for( std::size_t j = 0 ; j < n ; ++j )
   if( S[ bx + i ][ by + j ] == c )
    return( false );
 return( true );
 }

/// sudoku(): fills the empty squares of S, true if it can
inline bool sudoku( Grid & S , std::size_t i = 0 )
{
 const std::size_t N = S.size();
 if( i == N * N )
  return( true );
 const std::size_t x = i / N , y = i % N;
 if( S[ x ][ y ] != 0 )                                 // a given number
  return( sudoku( S , i + 1 ) );
 for( int c = 1 ; c <= int( N ) ; ++c )
  if( check( S , x , y , c ) ) {
   S[ x ][ y ] = c;
   if( sudoku( S , i + 1 ) )
    return( true );
   }
 S[ x ][ y ] = 0;
 return( false );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Sudoku.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sudoku.h ----------------------------*/
/*--------------------------------------------------------------------------*/
