/*--------------------------------------------------------------------------*/
/*---------------------------- File GridGraph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graph of a grid map, beyond the book [see `Graph.h` and `Grid.h`]: the
 * cells of a \f$ \mathit{rows} \times \mathit{cols} \f$ map, the cell
 * \f$ ( r , c ) \f$ the node \f$ u = r \cdot \mathit{cols} + c \f$, each cell
 * free or blocked, and the moves to the four sides of a cell or also to its
 * four corners. The map is one flat vector of the costs of the cells, or
 * none at all when every cell is free and costs 1; a cell is blocked when
 * its cost is not positive. The adjacency is computed, never stored: the
 * neighbours of \f$ u \f$ come from the row and the column of \f$ u \f$, one
 * division, and from at most 8 reads of the vector, so that the graph takes
 * \f$ s \, \mathit{rows} \, \mathit{cols} \f$ bytes (\f$ s \f$ the size of
 * a cost, and 0 bytes without the vector) against the
 * \f$ 4 ( n + 1 ) + ( 4 + s' ) a \f$ of a `StaticGraph` of the same grid
 * with \f$ a \le 8 n \f$ arcs (\f$ s' \f$ the size of a weight). This is
 * the representation of the programs of best-first search on grid maps (see
 * \ref theory_graph_grids): the maps of the benchmarks of Sturtevant are
 * read as such vectors [see `readGridMap()`], and the jump point search of
 * Harabor and Grastien needs exactly what the vector gives, i.e., whether a
 * cell is free in \f$ O( 1 ) \f$, with no list of adjacency.
 *
 * A move from the cell \f$ u \f$ into the cell \f$ v \f$ costs, as
 * `MoveCost` chooses, \f$ \mathit{cost}( v ) \f$ (the cell entered, the
 * default) or \f$ \mathit{cost}( u ) \f$ (the cell left) times the cost of
 * the move on a map of unit costs: \f$ \mathit{straight} \f$ for a side,
 * \f$ \mathit{diagonal} \f$ for a corner, by default 1 and \f$ \sqrt{2} \f$
 * (the octile costs, which the benchmarks use). A move to a corner may cut
 * it, i.e., pass by a blocked side: `Corners` says whether it may, as in the
 * cutting of corners allowed by the first jump point search, whether at
 * least one of the two sides must be free, or whether both must, as in the
 * benchmarks of Sturtevant since 2012. The graph is oriented when two free
 * cells have different costs, since under either model one of
 * \f$ ( u , v ) \f$ and \f$ ( v , u ) \f$ is weighed by the cost of \f$ u \f$
 * and the other by that of \f$ v \f$; with one cost for all the free cells
 * it is unoriented, the two moves of a pair costing the same, as the rules
 * on the corners are symmetric.
 *
 * The nodes are the free cells: `hasNode( u )` is whether \f$ u \f$ is a cell
 * and free, `n()` counts the free cells and `bound()` is
 * \f$ \mathit{rows} \cdot \mathit{cols} \f$, so that the visits of `Visit.h`
 * and the shortest paths of `ShortestPaths.h` run on the grid unchanged, with
 * vectors of `bound()` places. `forEachAdj()`, `forEachEdge()` and
 * `forEachMove()` (which also tells whether a move is to a corner) call
 * their function on the neighbours in increasing order of name, as the rows
 * of a `StaticGraph` are sorted, and allocate nothing; `adj()` and `V()` build
 * the vectors of the concept `Graph`. `gridDistances()` is the visit in
 * breadth of `Grid.h` from several sources on such a grid, the distances in a
 * vector by node, and `manhattan()`, `octile()` and `chebyshev()` are the
 * lower bounds on the cost between two cells that the A* of Hart, Nilsson and
 * Raphael takes as heuristics, `gridHeuristic()` the one of the moves and of
 * the costs of a grid.
 *
 * `setCost( u , c )` changes the cost of the cell \f$ u \f$, blocking it
 * when \f$ c \f$ is not positive and freeing it otherwise, and tells the
 * change, if the cost is not already \f$ c \f$, as the event
 * `Changed< Value , int , C >` of `Modification.h` (the type
 * `CellChanged`, with the cell and its two costs), through the subject
 * policy `Policy`: `NoObservers` by default, with which the change compiles
 * to the code of the grid alone, and `Static< O... >`, `Dynamic<>` or
 * `Both< ... >` otherwise, as for `ObservedGraph` [see `Observable.h`],
 * a `batch()` sending the changes of one cell coalesced into the first
 * cost and the last one. `forEachAffected( u , from , to , f )` calls
 * \f$ f \f$ on the heads of the moves that such a change may create,
 * remove or weigh differently, which depend on the model of the costs and
 * on whether the cell changes between free and blocked: the cell itself
 * with `ENTERED` and its free neighbours with `LEFT`, when it stays free,
 * and the cell with all its free neighbours (those of the four or the eight
 * moves, since the cell is a side of the corners that the moves between two
 * of its neighbours cut), when it is blocked or freed; nothing when it
 * stays blocked. An incremental search on the grid (LPA*, D* Lite) thus
 * recomputes the right-hand sides of at most nine cells per change, with no
 * rebuilding of the grid, and the union of the heads of the events of a
 * batch, read after it, holds every head whose arcs the batch changed (see
 * \ref theory_graph_grids). A grid with no vector of the costs takes one at
 * its first `setCost()` of a cost other than 1, of
 * \f$ \mathit{rows} \cdot \mathit{cols} \f$ places; `minCost()` and
 * `directed()` follow the changes by the number of the free cells of the
 * least cost, in \f$ O( 1 ) \f$ time, and in time linear in the cells when
 * the last cell of the least cost leaves it.
 *
 * The contracts [see `Contract.h`]: the grid has rows, cols >= 0 and at most
 * \f$ 2^{31} - 1 \f$ cells, so that the name of each cell is an int and no
 * sum in the moves overflows; the vector of the costs has a place per cell,
 * or none; the costs of the moves are positive, and a diagonal one is given
 * for an integral type of the weights; the operations on a node need a free
 * cell, and those on a cell a cell of the grid. The invariant is that `n()`
 * counts the free cells and the cells of the least cost. The operations are
 * defined in `GridGraph.tpp`.
 *
 * \see N. R. Sturtevant, Benchmarks for grid-based pathfinding, IEEE
 *      Transactions on Computational Intelligence and AI in Games 4(2),
 *      144-148, 2012, doi:10.1109/TCIAIG.2012.2197681
 * \see D. Harabor, A. Grastien, Online graph pruning for pathfinding on grid
 *      maps, Proceedings of the AAAI Conference on Artificial Intelligence
 *      25(1), 1114-1119, 2011, doi:10.1609/aaai.v25i1.7994
 * \see D. Harabor, A. Grastien, Improving jump point search, Proceedings of
 *      the International Conference on Automated Planning and Scheduling
 *      24, 128-135, 2014, doi:10.1609/icaps.v24i1.13633
 * \see P. Yap, Grid-based path-finding, Advances in Artificial Intelligence,
 *      Lecture Notes in Computer Science 2338, 44-55, 2002,
 *      doi:10.1007/3-540-47922-8_4
 * \see P. E. Hart, N. J. Nilsson, B. Raphael, A formal basis for the
 *      heuristic determination of minimum cost paths, IEEE Transactions on
 *      Systems Science and Cybernetics 4(2), 100-107, 1968,
 *      doi:10.1109/TSSC.1968.300136
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

#ifndef __ADS_GridGraph
 #define __ADS_GridGraph
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/Portable.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Grid.h"

#include <cstddef>
#include <istream>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the moves of a `GridGraph`
enum class GridMoves {
 FOUR ,  ///< to the four sides of a cell
 EIGHT  ///< to the sides and to the four corners
 };

/// when a move to a corner of a cell is allowed, the corner free
enum class Corners {
 CUT ,         ///< always, even between two blocked sides
 NO_SQUEEZE ,  ///< if at least one of the two sides is free
 NO_CUT       ///< if both sides are free
 };

/// which cell of a move of a `GridGraph` weighs it
enum class MoveCost {
 ENTERED ,  ///< the cell entered: \f$ ( u , v ) \f$ weighed by \f$ v \f$
 LEFT      ///< the cell left: \f$ ( u , v ) \f$ weighed by \f$ u \f$
 };

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS GridGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the implicit graph of a grid map, the cells of cost \f$ C \f$ and the
/// weights of the moves of type \f$ W \f$, whose changes of the costs are
/// told through the subject policy `Policy` [see Modification.h]
template< class W = int , class C = W , class Policy = NoObservers >
class GridGraph
{

/*--------------------------------------------------------------------------*/
 public:
 using Weight = W;  ///< the type of the weights
 using Cost = C;    ///< the type of the costs of the cells
 /// the event of a change of the cost of a cell, the cell its handle
 using CellChanged = mod::Changed< mod::Value , int , C >;
 /// the events
 using modifications = mod::list< CellChanged >;
 /// the subject
 using Subject = typename Policy::template bind< modifications >;
 /// the channel of a batch, open while it lives [see batch()]
 using Batch = ADS::Batch< GridGraph >;

 /// the grid of \f$ \mathit{rows} \times \mathit{cols} \f$ free cells of
 /// cost 1, with no vector; the diagonal cost \f$ \sqrt{2} \f$ times the
 /// straight one if not given
 /// \pre \f$ \mathit{rows} , \mathit{cols} \ge 0 \f$, at most
 ///      \f$ 2^{31} - 1 \f$ cells, positive costs of the moves, the diagonal
 ///      one given if \f$ W \f$ is integral and the moves are eight
 GridGraph( int rows , int cols , GridMoves moves = GridMoves::FOUR ,
	    Corners corners = Corners::NO_CUT , W straight = W( 1 ) ,
	    std::optional< W > diagonal = std::nullopt ,
	    MoveCost model = MoveCost::ENTERED );

 /// the grid of \f$ \mathit{rows} \times \mathit{cols} \f$ cells of the
 /// given costs, \f$ \mathit{cost}[ r \cdot \mathit{cols} + c ] \f$ that of
 /// the cell \f$ ( r , c ) \f$, a cell blocked if its cost is not positive,
 /// each move weighed by the cost of the cell that \f$ \mathit{model} \f$
 /// names (with no vector, as above, the two models coincide)
 /// \pre as above, and \f$ \mathit{cost} \f$ has
 ///      \f$ \mathit{rows} \cdot \mathit{cols} \f$ places
 GridGraph( int rows , int cols , std::vector< C > cost ,
	    GridMoves moves = GridMoves::FOUR ,
	    Corners corners = Corners::NO_CUT , W straight = W( 1 ) ,
	    std::optional< W > diagonal = std::nullopt ,
	    MoveCost model = MoveCost::ENTERED );

 /// the grid of the given costs as above, every parameter given, and the
 /// static observers `o...` of `Policy`, which must outlive it
 /// \pre as above
 template< class... O >
  requires( sizeof...( O ) > 0 )
 GridGraph( int rows , int cols , std::vector< C > cost , GridMoves moves ,
	    Corners corners , W straight , std::optional< W > diagonal ,
	    MoveCost model , O &... o );

 /// the number of rows
 [[nodiscard]] int rows( void ) const { return( f_rows ); }

 /// the number of columns
 [[nodiscard]] int cols( void ) const { return( f_cols ); }

 /// the moves
 [[nodiscard]] GridMoves moves( void ) const { return( f_moves ); }

 /// the rule on the corners
 [[nodiscard]] Corners corners( void ) const { return( f_corners ); }

 /// the cell whose cost weighs a move
 [[nodiscard]] MoveCost moveCost( void ) const { return( f_model ); }

 /// the cost of a move to a side on a map of unit costs
 [[nodiscard]] W straight( void ) const { return( f_straight ); }

 /// the cost of a move to a corner on a map of unit costs
 [[nodiscard]] W diagonal( void ) const { return( f_diagonal ); }

 /// whether the graph is oriented, i.e., two free cells cost differently
 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes, the free cells
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the names of the nodes are in \f$ [ 0 , \mathit{rows} \cdot \mathit{cols}
 /// ) \f$
 [[nodiscard]] int bound( void ) const { return( f_rows * f_cols ); }

 /// whether \f$ u \f$ is a free cell
 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < bound() ) && open( u ) );
  }

 /// the node of the cell \f$ ( r , c ) \f$
 /// \pre \f$ ( r , c ) \f$ is a cell of the grid
 [[nodiscard]] int node( int r , int c ) const;

 /// the cell \f$ ( r , c ) \f$ of the name \f$ u \f$
 /// \pre \f$ 0 \le u < \mathrm{bound}() \f$
 [[nodiscard]] GridCell cell( int u ) const;

 /// the cost of the cell \f$ u \f$, not positive if blocked
 /// \pre \f$ 0 \le u < \mathrm{bound}() \f$
 [[nodiscard]] C cost( int u ) const;

 /// the least cost of a free cell, 1 if there is no vector of the costs or
 /// no free cell
 [[nodiscard]] C minCost( void ) const { return( f_min ); }

 /// the cost of the cell \f$ u \f$ set to \f$ c \f$, the cell blocked if
 /// \f$ c \f$ is not positive and free otherwise, and `CellChanged` told
 /// with the cost before and \f$ c \f$; nothing if the cost is already
 /// \f$ c \f$
 ///
 /// In pseudocode (beyond the book), \f$ \gamma \f$ the vector of the
 /// costs, \f$ n \f$ the free cells, \f$ \gamma_{\mathrm{min}} \f$ their
 /// least cost and \f$ m \f$ the free cells of that cost:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{setCost}( u , c ) \cr
 /// 2 & \quad a \gets \gamma[ u ];\ \textbf{if}\ a = c\ \textbf{then}
 ///     \ \textbf{return} \cr
 /// 3 & \quad \gamma[ u ] \gets c \cr
 /// 4 & \quad \textbf{if}\ a > 0\ \textbf{then}\ n \gets n - 1;
 ///     \ \textbf{if}\ a = \gamma_{\mathrm{min}}\ \textbf{then}\ m
 ///     \gets m - 1 \cr
 /// 5 & \quad \textbf{if}\ c > 0\ \textbf{then}\ n \gets n + 1;
 ///     \ \textbf{if}\ n = 1\ \textbf{or}\ c < \gamma_{\mathrm{min}}
 ///     \ \textbf{then}\ ( \gamma_{\mathrm{min}} , m ) \gets ( c , 1 ) \cr
 /// 6 & \qquad \textbf{else if}\ c = \gamma_{\mathrm{min}}\ \textbf{then}
 ///     \ m \gets m + 1 \cr
 /// 7 & \quad \textbf{if}\ n > 0\ \textbf{and}\ m = 0\ \textbf{then}
 ///     \ \text{count again } \gamma_{\mathrm{min}} \text{ and } m
 ///     \text{ on the free cells} \cr
 /// 8 & \quad \mathit{directed} \gets n > 0\ \textbf{and}\ m \ne n \cr
 /// 9 & \quad \textbf{if}\ \text{someone wants the event}\ \textbf{then}
 ///     \ \text{tell } \mathrm{CellChanged}( u , a , c )
 /// \end{array}}
 /// \f]
 /// The test of line 9 is false at compile time with `NoObservers`, and
 /// the change is then the code of the grid alone.
 /// \pre \f$ 0 \le u < \mathrm{bound}() \f$
 void setCost( int u , const C & c );

 /// \f$ f( v ) \f$, in increasing order of \f$ v \f$, on each cell
 /// \f$ v \f$ whose moves in may have been created, removed or weighed
 /// differently by a change of the cost of the cell \f$ u \f$ from
 /// \f$ \mathit{from} \f$ to \f$ \mathit{to} \f$, i.e., on a superset of
 /// the heads of the arcs that the change touched: none if the cell is
 /// blocked before and after, \f$ u \f$ (under `ENTERED`) or its free
 /// neighbours (under `LEFT`) if it is free before and after, \f$ u \f$
 /// and its free neighbours of the four or the eight moves if it changes
 /// between free and blocked; at most nine calls, read on the grid after
 /// the change
 ///
 /// In pseudocode (beyond the book), \f$ N( u ) \f$ the cells at a move
 /// of \f$ u \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{forEachAffected}( u , a , c , f ) \cr
 /// 2 & \quad \textbf{if}\ a \le 0\ \textbf{and}\ c \le 0
 ///     \ \textbf{then}\ \textbf{return} \cr
 /// 3 & \quad \textbf{if}\ a > 0\ \textbf{and}\ c > 0\ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ \text{the cell entered pays}\ \textbf{then}
 ///     \ f( u )\ \textbf{else foreach}\ v \in \mathrm{adj}( u )
 ///     \ \textbf{do}\ f( v ) \cr
 /// 5 & \quad \textbf{else foreach}\ v \in \{ u \} \cup \{ x \in
 ///     N( u ) : x \text{ free} \}\ \text{in increasing order}
 ///     \ \textbf{do}\ f( v )
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le u < \mathrm{bound}() \f$
 template< class F >
 void forEachAffected( int u , const C & from , const C & to , F && f ) const;

 /// the set of the nodes, in increasing order
 [[nodiscard]] std::vector< int > V( void ) const;

 /// the neighbours of \f$ u \f$, in increasing order
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \f$ f( v ) \f$ on each neighbour \f$ v \f$ of \f$ u \f$, in increasing
 /// order, with no allocation
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \f$ f( v , w ) \f$ on each neighbour \f$ v \f$ of \f$ u \f$, \f$ w \f$ the
 /// cost of the move, in increasing order of \f$ v \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const;

 /// \f$ f( v , d ) \f$ on each neighbour \f$ v \f$ of \f$ u \f$, in
 /// increasing order, \f$ d \f$ whether the move is to a corner, with no
 /// allocation: the weight of the move is \f$ \mathit{cost}( x ) \f$ times
 /// `diagonal()` if \f$ d \f$ and `straight()` if not, \f$ x \f$ the cell
 /// that `moveCost()` names
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachMove( int u , F && f ) const;

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class D >
 int attach( D & o )
  requires( Subject::dynamic )
 {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre \f$ \mathit{id} \f$ was given by `attach()` and not yet
 ///      detached, no batch is open
 void detach( int id )
  requires( Subject::dynamic )
 {
  f_sub.detach( id );
  }

 /// opens a batch: the events are collected until it is closed, and sent
 /// coalesced
 [[nodiscard]] Batch batch( void ) { return( Batch( *this ) ); }

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the nodes in increasing order, a view on the names that skips the
 /// blocked cells
 [[nodiscard]] auto nodes( void ) const {
  return( std::views::iota( 0 , bound() ) |
	   std::views::filter( [ this ]( int u ) { return( open( u ) ); } ) );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the checks of the constructors and the counts of the free cells
 void init( std::optional< W > diagonal );

 /// whether the cell \f$ u \f$ is free
 [[nodiscard]] bool open( int u ) const {
  return( f_cost.empty() || ( f_cost[ u ] > C( 0 ) ) );
  }

 /// whether the rule on the corners lets a move pass by two sides, free
 /// or not as \f$ a \f$ and \f$ b \f$
 [[nodiscard]] bool corner( bool a , bool b ) const {
  return( ( f_corners == Corners::CUT ) ||
	   ( f_corners == Corners::NO_SQUEEZE ? ( a || b ) : ( a && b ) ) );
  }

 /// `forEachMove()` with no check of \f$ u \f$, for the other scans
 template< class G >
 void scan( int u , G && g ) const;

 /// `n()` counts the free cells, `f_atmin` those of the least cost
 [[nodiscard]] bool counted( void ) const;

 /// the least cost of a free cell and their number at it, counted again
 void recount( void );

 /// setCost() when someone listens, out of the hot code
 ADS_COLD_PATH void tellCold( const CellChanged & e ) {
  f_sub.emit( e );
  f_sub.audit( *this );
  }

 int f_rows;               ///< the number of rows
 int f_cols;               ///< the number of columns
 GridMoves f_moves;        ///< the moves
 Corners f_corners;        ///< the rule on the corners
 MoveCost f_model;         ///< the cell whose cost weighs a move
 W f_straight;             ///< the cost of a move to a side
 W f_diagonal;             ///< the cost of a move to a corner
 std::vector< C > f_cost;  ///< the costs of the cells, or none
 int f_n = 0;              ///< the number of free cells
 int f_atmin = 0;          ///< the number of free cells of cost f_min
 C f_min = C( 1 );         ///< the least cost of a free cell
 bool f_directed = false;  ///< whether two free cells cost differently
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class GridGraph )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the distance in moves of each cell of the grid \f$ g \f$ from the
/// nearest of the nodes \f$ S \f$, `INF` if none reaches it or if the cell
/// is blocked; \f$ D[ u ] \f$ for the cell \f$ u \f$
///
/// In pseudocode (the visit in breadth of section 9.5, from all the sources at
/// once, as `gridDistances()` of `Grid.h`):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gridDistances}( G , S ) \cr
/// 2 & \quad D[ u ] \gets \infty \text{ for every } u \in [ 0 ,
///     \mathrm{bound}() );\ Q \gets () \cr
/// 3 & \quad \textbf{foreach}\ s \in S \text{ with } D[ s ] = \infty
///     \ \textbf{do}\ D[ s ] \gets 0;\ \mathrm{enqueue}( Q , s )\quad
///     \triangleright\ \text{a source given twice counts once} \cr
/// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
/// 6 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u ) \text{ with } D[ v
///     ] = \infty\ \textbf{do}\ D[ v ] \gets D[ u ] + 1;\ \mathrm{enqueue}(
///     Q , v ) \cr
/// 7 & \quad \textbf{return}\ D
/// \end{array}}
/// \f]
/// \pre each node of \f$ S \f$ is a free cell
/// \post 0 on the sources, and each other reached cell one move farther
///       than a neighbour from which it is reached (audit)
template< class W , class C , class P >
std::vector< int > gridDistances( const GridGraph< W , C , P > & g ,
				  const std::vector< int > & S );

/// the distance of Manhattan between the cells \f$ a \f$ and \f$ b \f$,
/// \f$ \mathit{straight} \cdot ( | \Delta r | + | \Delta c | ) \f$, the
/// least cost of a path of moves to the sides of cost
/// \f$ \mathit{straight} \f$
/// \pre \f$ \mathit{straight} \ge 0 \f$
template< class W = int >
W manhattan( GridCell a , GridCell b , W straight = W( 1 ) );

/// the octile distance between the cells \f$ a \f$ and \f$ b \f$,
/// \f$ \mathit{straight} \cdot ( M - m ) + \mathit{diagonal} \cdot m \f$
/// with \f$ M \f$ and \f$ m \f$ the larger and the smaller of
/// \f$ | \Delta r | \f$ and \f$ | \Delta c | \f$, the least cost of a path of
/// moves to the sides and to the corners
/// \pre \f$ 0 \le \mathit{straight} \le \mathit{diagonal} \le 2 \,
///      \mathit{straight} \f$
template< class W >
W octile( GridCell a , GridCell b , W straight , W diagonal );

/// the distance of Chebyshev between the cells \f$ a \f$ and \f$ b \f$,
/// \f$ \mathit{straight} \cdot \max\{ | \Delta r | , | \Delta c | \} \f$, the
/// octile distance when a move to a corner costs as one to a side (the
/// king)
/// \pre \f$ \mathit{straight} \ge 0 \f$
template< class W = int >
W chebyshev( GridCell a , GridCell b , W straight = W( 1 ) );

/// a lower bound on the cost of a path from \f$ u \f$ to \f$ t \f$ in
/// \f$ g \f$, consistent, for the A* of Hart, Nilsson and Raphael: the least
/// cost \f$ \gamma_{\mathrm{min}} \f$ of a cell times `manhattan()` for four
/// moves and times `octile()` for eight, the latter with
/// \f$ \min\{ \mathit{diagonal} , 2 \, \mathit{straight} \} \f$ as the cost
/// of a corner; the same bound under both values of `MoveCost`, since a move
/// weighs at least \f$ \gamma_{\mathrm{min}} \f$ times its unit cost
/// whichever of its two cells pays (see \ref theory_graph_grids)
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gridHeuristic}( G , u , t ) \cr
/// 2 & \quad \textbf{if}\ G \text{ has four moves }\ \textbf{then return}
///     \ \gamma_{\mathrm{min}} \cdot \mathrm{manhattan}( u , t ,
///     \mathit{straight} ) \cr
/// 3 & \quad \textbf{return}\ \gamma_{\mathrm{min}} \cdot \mathrm{octile}( u ,
///     t , \mathit{straight} , \min\{ \mathit{diagonal} , 2 \,
///     \mathit{straight} \} )
/// \end{array}}
/// \f]
/// \pre \f$ u \f$ and \f$ t \f$ are cells of the grid; for eight moves,
///      \f$ \mathit{diagonal} \ge \mathit{straight} \f$
template< class W , class C , class P >
W gridHeuristic( const GridGraph< W , C , P > & g , int u , int t );

/// the grid of a map in the format of the benchmarks of Sturtevant: the
/// lines `type octile`, `height` \f$ h \f$, `width` \f$ w \f$ and `map`,
/// then \f$ h \f$ lines of \f$ w \f$ characters, of which the dot, `G` and
/// `S` are free cells of cost 1 and the others (`O`, `T`, `W` and the at
/// sign) are blocked
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{readGridMap}( \mathit{in} ) \cr
/// 2 & \quad \text{read the header: } h \text{ and } w \cr
/// 3 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ h - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ c \gets 0\ \textbf{to}\ w - 1\ \textbf{do}
///     \ \mathit{cost}[ r w + c ] \gets 1\ \textbf{if}\  \text{the }
///     \text{character is free, else } 0 \cr
/// 5 & \quad \textbf{return}\ \mathrm{GridGraph}( h , w , \mathit{cost} )
/// \end{array}}
/// \f]
/// \pre the stream holds a map of that format
template< class W = double , class C = unsigned char >
GridGraph< W , C > readGridMap( std::istream & in ,
			       GridMoves moves = GridMoves::EIGHT ,
			       Corners corners = Corners::NO_CUT );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/GridGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif /* GridGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File GridGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
