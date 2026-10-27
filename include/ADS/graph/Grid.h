/*--------------------------------------------------------------------------*/
/*------------------------------ File Grid.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The visits of a GRID, the implicit graph whose nodes are the cells
 * \f$ ( r , c ) \f$ of a \f$ \mathit{rows} \times \mathit{cols} \f$ matrix and
 * whose edges are the moves from a cell to the cells at the given differences
 * \f$ ( \mathit{dr} , \mathit{dc} ) \f$ that are inside the matrix and free:
 * FOUR_MOVES (the four sides of a cell), EIGHT_MOVES (the sides and the
 * corners, the king) and KNIGHT_MOVES (the knight). The graph is never built:
 * the cells are its nodes and a vector of moves its adjacency, so that each
 * visit takes \f$ \Theta( R C ) \f$ time for k moves per cell treated as a
 * constant, against the \f$ \Theta( R C k ) \f$ space of the lists of
 * adjacency that it saves.
 *
 * - `gridDistances()`, the visit in breadth of section 9.5 from several
 *   sources at once, all of them at distance 0 in the queue: each cell gets
 *   its distance in moves from the nearest source, `INF` if none reaches it.
 *   It is the algorithm of Lee for the routing on a grid, and with the
 *   sources a whole set it gives the distance transform of an image; with
 *   no obstacles the two raster passes of Rosenfeld and Pfaltz compute the
 *   same distances for FOUR_MOVES and EIGHT_MOVES in the same time without
 *   a queue;
 * - `gridDistance()`, the distance from one cell to another, the visit
 *   stopping when the target leaves the queue; the A* of Hart, Nilsson and
 *   Raphael, with the distance of Manhattan as its admissible heuristic,
 *   examines fewer cells when the target is far from the obstacles, with
 *   the same bound in the worst case;
 * - `floodFill()`, the paint bucket: the cell ( r , c ) and every cell joined
 *   to it by moves through cells of its colour get the new colour; the
 *   visit paints a cell when it enters the queue, so that the colour itself
 *   marks the cells reached and no other matrix is needed. The fill by
 *   spans of Smith pushes a run of a row at a time instead of a cell, with
 *   fewer operations on the queue for the same bound;
 * - `knightDistance()`, the fewest moves of a knight between two squares of an
 *   n x n board, `INF` if there is no way (only on the boards of 1, 2 and 3
 *   squares a side): a visit in breadth of the graph of the knight moves,
 *   \f$ \Theta( n^2 ) \f$; on the infinite board the distance has a closed
 *   form, computed in \f$ O( 1 ) \f$, which differs from that of a finite
 *   board near its corners.
 *
 * The functions take the grid as its size and a predicate of the free
 * cells, and the moves as any range of differences, the knight's too;
 * `GridGraph` [see `GridGraph.h`] is instead a graph of the library, the
 * map stored as one vector of costs and the moves those of four or eight
 * directions, on which the visits of `Visit.h` and the shortest paths of
 * `ShortestPaths.h` run, and `gridDistances()` has an overload on it. There
 * is none of `floodFill()`: the paint bucket reads and writes the colours
 * of the matrix itself, and the region that it paints is the set of the
 * cells reached by a visit of the grid whose free cells are those of the
 * colour of the start, which `distance()` of `Visit.h` gives on a
 * `GridGraph` of that map.
 *
 * The contracts [see `Contract.h`]: the grid has rows, cols >= 0, the
 * matrices are rectangular, each source or start is a cell of the grid (and
 * free, for the visits); at the level of the audits `gridDistances()` ensures
 * that its distances are those of a breadth first visit (0 on the sources,
 * and each reached cell but a source one move farther than a neighbour).
 * The functions are defined in `Grid.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see E. F. Moore, The shortest path through a maze, Proceedings of the
 *      International Symposium on the Theory of Switching, 285-292, 1959
 * \see C. Y. Lee, An algorithm for path connections and its applications,
 *      IRE Transactions on Electronic Computers EC-10(3), 346-365, 1961,
 *      doi:10.1109/TEC.1961.5219222
 * \see A. Rosenfeld, J. L. Pfaltz, Sequential operations in digital picture
 *      processing, Journal of the ACM 13(4), 471-494, 1966,
 *      doi:10.1145/321356.321357
 * \see P. E. Hart, N. J. Nilsson, B. Raphael, A formal basis for the
 *      heuristic determination of minimum cost paths, IEEE Transactions on
 *      Systems Science and Cybernetics 4(2), 100-107, 1968,
 *      doi:10.1109/TSSC.1968.300136
 * \see A. R. Smith, Tint fill, Proceedings of SIGGRAPH 1979, 276-283, 1979,
 *      doi:10.1145/800249.807456
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

#ifndef __ADS_Grid
 #define __ADS_Grid
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Visit.h"

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a cell \f$ ( r , c ) \f$ of a grid, or a move as the difference of two
/// cells
using GridCell = std::pair< int , int >;

/// the moves to the four sides of a cell: up, left, down, right
inline constexpr std::array< GridCell , 4 > FOUR_MOVES = {
 GridCell( -1 , 0 ) , GridCell( 0 , -1 ) , GridCell( 1 , 0 ) ,
 GridCell( 0 , 1 ) };

/// the moves to the sides and to the corners of a cell, those of the king
inline constexpr std::array< GridCell , 8 > EIGHT_MOVES = {
 GridCell( -1 , -1 ) , GridCell( -1 , 0 ) , GridCell( -1 , 1 ) ,
 GridCell( 0 , -1 ) , GridCell( 0 , 1 ) , GridCell( 1 , -1 ) ,
 GridCell( 1 , 0 ) , GridCell( 1 , 1 ) };

/// the moves of the knight
inline constexpr std::array< GridCell , 8 > KNIGHT_MOVES = {
 GridCell( -2 , -1 ) , GridCell( -2 , 1 ) , GridCell( -1 , -2 ) ,
 GridCell( -1 , 2 ) , GridCell( 1 , -2 ) , GridCell( 1 , 2 ) ,
 GridCell( 2 , -1 ) , GridCell( 2 , 1 ) };

/// the type of FOUR_MOVES, the default range of moves
using FourMoves = std::remove_cv_t< decltype( FOUR_MOVES ) >;

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether \f$ ( r , c ) \f$ is a cell of a
/// \f$ \mathit{rows} \times \mathit{cols} \f$ grid
inline bool insideGrid( int rows , int cols , GridCell x );

/// the distance in moves of each cell of the
/// \f$ \mathit{rows} \times \mathit{cols} \f$ grid from the nearest source,
/// `INF` if none reaches it, entering only the cells \f$ x \f$ with
/// \f$ \mathrm{free}( x ) \f$; \f$ D[ r ][ c ] \f$ for the cell
/// \f$ ( r , c ) \f$
///
/// In pseudocode (the visit in breadth of section 9.5, from all the sources at
/// once):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gridDistances}( R , C , S , \mathit{free} ,
///     M ) \cr
/// 2 & \quad \textbf{foreach}\  \text{cell } x\ \textbf{do}\ D[ x ] \gets
///     \infty \cr
/// 3 & \quad Q \gets () \cr
/// 4 & \quad \textbf{foreach}\ s \in S\ \textbf{do}\ D[ s ] \gets 0;
///     \ \mathrm{enqueue}( Q , s ) \cr
/// 5 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad x \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad \textbf{foreach}\ \delta \in M\ \textbf{do} \cr
/// 8 & \qquad\quad y \gets x + \delta \cr
/// 9 & \qquad\quad \textbf{if}\ y \text{ is in the grid }\ \textbf{and}
///     \ \mathit{free}( y )\ \textbf{and}\ D[ y ] = \infty\ \textbf{then} \cr
/// 10 & \qquad\qquad D[ y ] \gets D[ x ] + 1;\ \mathrm{enqueue}( Q , y ) \cr
/// 11 & \quad \textbf{return}\ D
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{rows} , \mathit{cols} \ge 0 \f$, the sources are free
///      cells of the grid
/// \post 0 on the sources, and each other reached cell one move farther
///       than a neighbour from which it can be reached (audit)
template< class Free , class Moves = FourMoves >
std::vector< std::vector< int > > gridDistances( int rows , int cols ,
                                    const std::vector< GridCell > & sources ,
                                    Free && free ,
                                    const Moves & moves = FOUR_MOVES );

/// the distance in moves from the cell \f$ s \f$ to the cell
/// \f$ t \f$ of the \f$ \mathit{rows} \times \mathit{cols} \f$ grid through
/// the cells \f$ x \f$ with \f$ \mathrm{free}( x ) \f$, `INF` if \f$ t \f$
/// cannot be reached; the visit stops when \f$ t \f$ leaves the queue
///
/// In pseudocode, the visit of `gridDistances()` from s alone, stopped when t
/// leaves the queue:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gridDistance}( R , C , s , t , \mathit{free}
///     , M ) \cr
/// 2 & \quad \textbf{foreach}\  \text{cell } x\ \textbf{do}\ D[ x ] \gets
///     \infty \cr
/// 3 & \quad D[ s ] \gets 0;\ Q \gets ( s ) \cr
/// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad x \gets \mathrm{dequeue}( Q ) \cr
/// 6 & \qquad \textbf{if}\ x = t\ \textbf{then return}\ D[ t ] \cr
/// 7 & \qquad \textbf{foreach}\ \delta \in M\ \textbf{with}\ x + \delta
///     \text{ in the grid, free and } D[ x + \delta ] = \infty\ \textbf{do}
///     \cr
/// 8 & \qquad\quad D[ x + \delta ] \gets D[ x ] + 1;\ \mathrm{enqueue}( Q , x
///     + \delta ) \cr
/// 9 & \quad \textbf{return}\ \infty
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{rows} , \mathit{cols} \ge 0 \f$, \f$ s \f$ and \f$ t \f$
///      are cells of the grid, \f$ s \f$ is free
template< class Free , class Moves = FourMoves >
int gridDistance( int rows , int cols , GridCell s , GridCell t ,
                  Free && free , const Moves & moves = FOUR_MOVES );

/// the cell \f$ ( r , c ) \f$ of \f$ M \f$ and all the cells
/// joined to it by moves through cells of its colour get the colour
/// \f$ \mathit{color} \f$; the number of cells painted, 0 if
/// \f$ \mathit{color} \f$ is already the colour of \f$ ( r , c ) \f$
///
/// In pseudocode, the colour itself marking the cells reached:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{floodFill}( M , r , c , \mathit{color} ) \cr
/// 2 & \quad a \gets M[ r ][ c ] \cr
/// 3 & \quad \textbf{if}\ a = \mathit{color}\ \textbf{then return}\ 0 \cr
/// 4 & \quad M[ r ][ c ] \gets \mathit{color};\ Q \gets ( ( r , c ) );\ k
///     \gets 1 \cr
/// 5 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad x \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad \textbf{foreach}\ \delta \in M\ \textbf{do} \cr
/// 8 & \qquad\quad \textbf{if}\ x + \delta \text{ is in } M\ \textbf{and}\ M[
///     x + \delta ] = a\ \textbf{then} \cr
/// 9 & \qquad\qquad M[ x + \delta ] \gets \mathit{color};\ \mathrm{enqueue}( Q
///     , x + \delta );\ k \gets k + 1 \cr
/// 10 & \quad \textbf{return}\ k
/// \end{array}}
/// \f]
/// \pre \f$ M \f$ is rectangular, \f$ ( r , c ) \f$ is a cell of it
/// \post \f$ M[ r ][ c ] = \mathit{color} \f$
template< class T , class Moves = FourMoves >
std::size_t floodFill( std::vector< std::vector< T > > & M , int r , int c ,
                       const std::type_identity_t< T > & color ,
                       const Moves & moves = FOUR_MOVES );

/// the fewest moves of a knight from \f$ s \f$ to
/// \f$ t \f$ on an \f$ n \times n \f$ board, `INF` if it cannot get there
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{knightDistance}( n , s , t ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{gridDistance}( n , n , s , t ,
///     \text{every cell free} , \text{the moves of the knight} )
/// \end{array}}
/// \f]
/// \pre \f$ n > 0 \f$, \f$ s \f$ and \f$ t \f$ are squares of the board
inline int knightDistance( int n , GridCell s , GridCell t );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Grid.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Grid.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Grid.h -----------------------------*/
/*--------------------------------------------------------------------------*/
