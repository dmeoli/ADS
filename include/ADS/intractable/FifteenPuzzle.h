/*--------------------------------------------------------------------------*/
/*-------------------------- File FifteenPuzzle.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The FIFTEEN PUZZLE of section 19.5 (the GIOCO DEL QUINDICI of the book,
 * Samuel Loyd, 1878): \f$ n^2 - 1 \f$ tiles numbered 1 to \f$ n^2 - 1 \f$ on
 * an \f$ n \times n \f$ grid with a hole, a move slides a tile next to the
 * hole into it, and the goal has the tiles in increasing order and the hole
 * last. `solvePuzzle()` is the branch-&-bound of the text: a node is a
 * configuration, its children those one move away, its cost the number of
 * moves from the root, its lower bound the sum of the distances of the tiles
 * from their places [see `manhattan()`], and the next node visited the one
 * of least cost plus bound, by a priority queue (the algorithm A* of Hart,
 * Nilsson and Raphael with a consistent heuristic, so that the first goal
 * extracted is optimal). A configuration is reached from the goal if and
 * only if the parity of its permutation, plus that of the row of the hole
 * when \f$ n \f$ is even, matches the goal [see `solvable()`]. The map of
 * the least level of each configuration is a parameter of `solvePuzzle()`,
 * `std::map` by default, so that the same search runs on the maps of std,
 * Boost or ADS with the same nodes.
 *
 * Beyond the text, two searches that work with any goal, e.g., the spiral
 * one of `puzzleSpiralGoal()` (1 2 3 / 8 0 4 / 7 6 5 for \f$ n = 3 \f$):
 * `puzzleBFS()` visits the configurations in breadth, so that the first goal
 * reached is one of the fewest moves, with no heuristic and therefore many
 * more nodes than A*; `puzzleDFS()` is the backtracking in depth that goes
 * back when a configuration is already on the current path or the moves
 * exceed a bound, and so finds a solution of at most that many moves, if
 * any, not necessarily the shortest one.
 *
 * The contracts [see `Contract.h`]: a configuration is a permutation of
 * \f$ [ 0 , n^2 ) \f$, and a solution returned goes from it to the goal by
 * moves. The operations are defined in `FifteenPuzzle.tpp`.
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
 *         Università di Pisa \n
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

/// the goal on \f$ n \times n \f$ squares, the tiles
/// \f$ 1 , \ldots , n^2 - 1 \f$ by rows and the hole last
/// \pre \f$ n \ge 1 \f$
/// \post a configuration
inline Board puzzleGoal( int n );

/// the lower bound of the text: the sum over the tiles of the rows and
/// columns between the tile and its place,
/// \f$ \sum_{B[ p ] \ne 0} | \lfloor p / n \rfloor - \lfloor q / n \rfloor |
/// + | p \bmod n - q \bmod n | \f$ with \f$ q = B[ p ] - 1 \f$
/// \pre \f$ B \f$ is a configuration of \f$ n \times n \f$ squares
inline int manhattan( const Board & B , int n );

/// the configurations one move away, the hole swapped with the tile above,
/// below, on the left and on the right of it, in this order
/// \pre \f$ B \f$ is a configuration of \f$ n \times n \f$ squares
/// \post two to four configurations if \f$ n \ge 2 \f$
inline std::vector< Board > puzzleMoves( const Board & B , int n );

/// the spiral goal on \f$ n \times n \f$ squares: the tiles clockwise from
/// the top left corner, the hole where the spiral ends
/// \pre \f$ n \ge 1 \f$
/// \post a configuration
inline Board puzzleSpiralGoal( int n );

/// the configurations from \f$ B \f$ to `goal` of a shortest solution by the
/// visit in breadth, or nothing if `goal` cannot be reached; the
/// configurations examined go to `*visited`, if given
///
/// In pseudocode (the visit in breadth of section 9.5 on the graph of the
/// configurations, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{puzzleBFS}( B , \mathit{goal} ) \cr
/// 2 & \quad p[ B ] \gets B;\ Q \gets ( B ) \cr
/// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
/// 5 & \qquad \textbf{if}\ u = \mathit{goal}\ \textbf{then return}
///     \ \text{the path from } B \text{ to } u \text{ by } p \cr
/// 6 & \qquad \textbf{foreach}\ v \in \mathrm{puzzleMoves}( u )
///     \text{ with no } p[ v ]\ \textbf{do}\ p[ v ] \gets u;
///     \ \mathrm{enqueue}( Q , v ) \cr
/// 7 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ B \f$ and `goal` are configurations of \f$ n \times n \f$
///      squares
/// \post a solution returned goes from \f$ B \f$ to `goal` one move at a
///       time
inline std::optional< std::vector< Board > > puzzleBFS(
                        const Board & B , const Board & goal , int n ,
                        long * visited = nullptr );

/// the configurations from \f$ B \f$ to `goal` of a solution of at most
/// \f$ \mathit{nmax} \f$ moves by the backtracking in depth, or nothing if
/// there is none
///
/// In pseudocode (the backtracking of chapter 16, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( S )\quad \triangleright
///     \ \text{started with } S = ( B ) \cr
/// 2 & \quad \textbf{if}\ \text{the last of } S \text{ is } \mathit{goal}
///     \ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ |S| > \mathit{nmax}
///     \ \textbf{then return false}\quad \triangleright\ \mathit{nmax}
///     \text{ moves made} \cr
/// 4 & \quad \textbf{foreach}\ v \in \mathrm{puzzleMoves}(
///     \text{the last of } S ) \text{ with } v \notin S\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{extend}( S \cdot ( v ) )
///     \ \textbf{then return true} \cr
/// 6 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ B \f$ and `goal` are configurations of \f$ n \times n \f$
///      squares, \f$ \mathit{nmax} \ge 0 \f$
/// \post a solution returned goes from \f$ B \f$ to `goal` one move at a
///       time
inline std::optional< std::vector< Board > > puzzleDFS(
                        const Board & B , const Board & goal , int n ,
                        int nmax );

/// whether the goal can be reached from \f$ B \f$
///
/// In pseudocode (section 19.5, after Johnson and Story), a move changes the
/// parity of the inversions when \f$ n \f$ is even and the hole changes row,
/// and never otherwise:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{solvable}( B , n ) \cr
/// 2 & \quad \mathit{inv} \gets |\{ p < q : B[ q ] \ne 0 \text{ and } B[ q ]
///     < B[ p ] \}| \cr
/// 3 & \quad \textbf{if}\ n \text{ is odd}\ \textbf{then return}
///     \ \mathit{inv} \text{ is even} \cr
/// 4 & \quad \textbf{return}\ \mathit{inv} + n - 1 - r \text{ is even}\quad
///     \triangleright\ r \text{ the row of the hole}
/// \end{array}}
/// \f]
/// \pre \f$ B \f$ is a configuration of \f$ n \times n \f$ squares
inline bool solvable( const Board & B , int n );

/// the configurations from \f$ B \f$ to the goal of a shortest solution, or
/// nothing; the number of nodes extracted goes to `*visited`, if given; the
/// least level of each configuration is kept in a `Map`, `std::map` by
/// default, or another map of std or Boost, or a hash table of ADS
/// [see `FlatHashTable.h`], with the same nodes visited
///
/// In pseudocode (section 19.5), \f$ g( x ) \f$ the moves from \f$ B \f$ to
/// the node \f$ x \f$ and \f$ \mathit{best} \f$ the least level met of each
/// configuration, an entry of the queue with a larger one being stale:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{solvePuzzle}( B , n ) \cr
/// 2 & \quad \textbf{if not}\ \mathrm{solvable}( B , n )
///     \ \textbf{then return nil} \cr
/// 3 & \quad \mathit{best}[ B ] \gets 0;\ \mathrm{insert}( Q , B ,
///     \mathrm{manhattan}( B ) ) \cr
/// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad x \gets \mathrm{deleteMin}( Q ) \cr
/// 6 & \qquad \textbf{if}\ g( x ) > \mathit{best}[ x ]
///     \ \textbf{then continue}\quad \triangleright\ \text{a stale entry} \cr
/// 7 & \qquad \textbf{if}\ x \text{ is the goal}\ \textbf{then return}
///     \ \text{the path from } B \text{ to } x \cr
/// 8 & \qquad \textbf{foreach}\ y \in \mathrm{puzzleMoves}( x )\ \textbf{do}
///     \cr
/// 9 & \qquad\quad \textbf{if}\ \mathit{best}[ y ] \le g( x ) + 1
///     \ \textbf{then continue} \cr
/// 10 & \qquad\quad \mathit{best}[ y ] \gets g( x ) + 1;\ \mathrm{insert}( Q ,
///     y , g( x ) + 1 + \mathrm{manhattan}( y ) ) \cr
/// 11 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ B \f$ is a configuration of \f$ n \times n \f$ squares
/// \post a solution returned goes from \f$ B \f$ to the goal one move at a
///       time
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
