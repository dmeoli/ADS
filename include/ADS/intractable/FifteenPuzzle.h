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
 * matches the goal [see solvable()]. The map of the least level of each
 * configuration is a parameter of solvePuzzle(), std::map by default, so
 * that the same search runs on the maps of std, Boost or ADS with the same
 * nodes.
 *
 * Beyond the text, two searches that work with any goal, e.g., the spiral
 * one of puzzleSpiralGoal() (1 2 3 / 8 0 4 / 7 6 5 for n = 3): puzzleBFS()
 * visits the configurations in breadth, so that the first goal reached is
 * one of the fewest moves, with no heuristic and therefore many more nodes
 * than A*; puzzleDFS() is the backtracking in depth that goes back when a
 * configuration is already on the current path or the moves exceed a
 * bound, and so finds a solution of at most that many moves, if any, not
 * necessarily the shortest one.
 *
 * The contracts [see Contract.h]: a configuration is a permutation of [ 0 ,
 * n^2 ), and a solution returned goes from it to the goal by moves. The
 * operations are defined in FifteenPuzzle.tpp.
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

#include "ADS/Contract.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"

#include <cstdlib>
#include <map>
#include <type_traits>
#include <optional>
#include <queue>
#include <set>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a configuration: the tile of each square by rows, 0 the hole
using Board = std::vector< int >;

/// the goal on n x n squares
/// \pre n >= 1
/// \post a configuration
inline Board puzzleGoal( int n );

/// the lower bound of the text: the sum over the tiles of the rows and
/// columns between the tile and its place
/// \pre B is a configuration of n x n squares
inline int manhattan( const Board & B , int n );

/// the configurations one move away
/// \pre B is a configuration of n x n squares
/// \post two to four configurations if n >= 2
inline std::vector< Board > puzzleMoves( const Board & B , int n );

/// the spiral goal on n x n squares: the tiles clockwise from the top left
/// corner, the hole where the spiral ends
/// \pre n >= 1
/// \post a configuration
inline Board puzzleSpiralGoal( int n );

/// the configurations from B to goal of a shortest solution by the visit
/// in breadth, or nothing if goal cannot be reached; the configurations
/// examined go to *visited, if given
/// \pre B and goal are configurations of n x n squares
/// \post a solution returned goes from B to goal one move at a time
inline std::optional< std::vector< Board > > puzzleBFS(
                        const Board & B , const Board & goal , int n ,
                        long * visited = nullptr );

/// the configurations from B to goal of a solution of at most nmax moves
/// by the backtracking in depth, or nothing if there is none
/// \pre B and goal are configurations of n x n squares, nmax >= 0
/// \post a solution returned goes from B to goal one move at a time
inline std::optional< std::vector< Board > > puzzleDFS(
                        const Board & B , const Board & goal , int n ,
                        int nmax );

/// whether the goal can be reached from B
/// \pre B is a configuration of n x n squares
inline bool solvable( const Board & B , int n );

/// the configurations from B to the goal of a shortest solution, or
/// nothing; the number of nodes extracted goes to *visited, if given; the
/// least level of each configuration is kept in a Map, std::map by
/// default, or another map of std or Boost, or a hash table of ADS
/// [see FlatHashTable.h], with the same nodes visited
/// \pre B is a configuration of n x n squares
/// \post a solution returned goes from B to the goal one move at a time
template< class Map = std::map< Board , int > >
std::optional< std::vector< Board > > solvePuzzle( const Board & B , int n ,
                                                   long * visited = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/impl/FifteenPuzzle.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FifteenPuzzle.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FifteenPuzzle.h ------------------------*/
/*--------------------------------------------------------------------------*/
