/*--------------------------------------------------------------------------*/
/*-------------------------- File FifteenPuzzle.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GIOCO DEL QUINDICI of section 19.5 (Samuel Loyd, 1878): n^2 - 1
 * tiles numbered 1 to n^2 - 1 on an n x n grid with a hole, a move slides
 * a tile next to the hole into it, and the goal has the tiles in
 * increasing order and the hole last. solvePuzzle() is the branch-&-bound
 * of the text: a node is a configuration, its children those one move
 * away, its cost the number of moves from the root, its lower bound the
 * sum of the distances of the tiles from their places [see manhattan()],
 * and the next node visited the one of least cost plus bound, by a
 * priority queue (the algorithm A* of Hart, Nilsson and Raphael with a
 * consistent heuristic, so that the first goal extracted is optimal). A
 * configuration is reached from the goal if and only if the parity of
 * its permutation, plus that of the row of the hole when n is even,
 * matches the goal [see solvable()].
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see P. E. Hart, N. J. Nilsson, B. Raphael, A formal basis for the
 *      heuristic determination of minimum cost paths, IEEE Transactions on
 *      Systems Science and Cybernetics 4(2), 100-107, 1968,
 *      doi:10.1109/TSSC.1968.300136
 * \see W. W. Johnson, W. E. Story, Notes on the "15" puzzle, American
 *      Journal of Mathematics 2(4), 397-404, 1879, doi:10.2307/2369492
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

#ifndef __ADS_FifteenPuzzle
 #define __ADS_FifteenPuzzle
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/FibonacciHeapPriorityQueue.h"

#include <cstdlib>
#include <map>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a configuration: the tile of each square by rows, 0 the hole
using Board = std::vector< int >;

/// the goal on n x n squares
inline Board puzzleGoal( int n )
{
 Board B( n * n );
 for( int i = 0 ; i + 1 < n * n ; ++i ) B[ i ] = i + 1;
 B[ n * n - 1 ] = 0;
 return( B );
 }

/// the lower bound of the text: the sum over the tiles of the rows and
/// columns between the tile and its place
inline int manhattan( const Board & B , int n )
{
 int d = 0;
 for( int p = 0 ; p < n * n ; ++p )
  if( B[ p ] ) {
   const int q = B[ p ] - 1;
   d += std::abs( p / n - q / n ) + std::abs( p % n - q % n );
   }
 return( d );
 }

/// the configurations one move away
inline std::vector< Board > puzzleMoves( const Board & B , int n )
{
 int h = 0;
 while( B[ h ] ) ++h;
 std::vector< Board > C;
 const int r = h / n , c = h % n;
 for( auto [ dr , dc ] : { std::pair( -1 , 0 ) , std::pair( 1 , 0 ) ,
                           std::pair( 0 , -1 ) , std::pair( 0 , 1 ) } ) {
  const int rr = r + dr , cc = c + dc;
  if( ( rr < 0 ) || ( rr >= n ) || ( cc < 0 ) || ( cc >= n ) ) continue;
  Board X = B;
  std::swap( X[ h ] , X[ rr * n + cc ] );
  C.push_back( X );
  }
 return( C );
 }

/// whether the goal can be reached from B
inline bool solvable( const Board & B , int n )
{
 int inv = 0 , hole = 0;
 for( int p = 0 ; p < n * n ; ++p ) {
  if( ! B[ p ] ) {
   hole = p / n;
   continue;
   }
  for( int q = p + 1 ; q < n * n ; ++q ) inv += B[ q ] && ( B[ q ] < B[ p ] );
  }
 if( n % 2 ) return( inv % 2 == 0 );
 return( ( inv + ( n - 1 - hole ) ) % 2 == 0 );
 }

/// the configurations from B to the goal of a shortest solution, or
/// nothing; the number of nodes extracted goes to *visited, if given
inline std::optional< std::vector< Board > > solvePuzzle(
                        const Board & B , int n , long * visited = nullptr )
{
 if( visited ) *visited = 0;
 if( ! solvable( B , n ) ) return( std::nullopt );
 const Board goal = puzzleGoal( n );
 std::vector< Board > node( 1 , B );
 std::vector< int > parent( 1 , -1 ) , level( 1 , 0 );
 std::map< Board , int > best;           // the least level of each one
 best[ B ] = 0;
 FibonacciHeapPriorityQueue< int , int > Q;
 Q.insert( 0 , manhattan( B , n ) );
 while( ! Q.isEmpty() ) {
  const int k = Q.deleteMin();
  if( level[ k ] > best[ node[ k ] ] ) continue;     // a stale entry
  if( visited ) ++*visited;
  if( node[ k ] == goal ) {
   std::vector< Board > S;
   for( int x = k ; x >= 0 ; x = parent[ x ] )
    S.insert( S.begin() , node[ x ] );
   return( S );
   }
  for( Board & C : puzzleMoves( node[ k ] , n ) ) {
   const int g = level[ k ] + 1;
   auto it = best.find( C );
   if( ( it != best.end() ) && ( it->second <= g ) ) continue;
   best[ C ] = g;
   const int lb = manhattan( C , n );
   node.push_back( std::move( C ) );
   parent.push_back( k );
   level.push_back( g );
   Q.insert( int( node.size() ) - 1 , g + lb );
   }
  }
 return( std::nullopt );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* FifteenPuzzle.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FifteenPuzzle.h ------------------------*/
/*--------------------------------------------------------------------------*/
