/*--------------------------------------------------------------------------*/
/*-------------------------- File Biconnectivity.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The biconnected components (the blocks) of an unoriented graph, its cut
 * nodes (the articulation points), its bridges, its 2-edge-connected
 * components and its block-cut tree, by one visit in depth, beyond the
 * book:
 *
 * - `biconnectedComponents( g )` is the algorithm of Hopcroft and Tarjan
 *   with no stack of edges: the visit keeps the order of discovery
 *   \f$ \mathit{dt} \f$ and the lowpoint \f$ \mathit{low} \f$ of each node,
 *   i.e., the least \f$ \mathit{dt} \f$ that its subtree reaches by one
 *   edge out of the tree, the edge to the parent excluded once (so that a
 *   parallel copy of it counts), and then a pass over the nodes in the
 *   order of discovery gives the edge from each node \f$ u \f$ to its
 *   parent \f$ p \f$ a new block if \f$ \mathit{low}[ u ] \ge \mathit{dt}[
 *   p ] \f$ and the block of the edge above \f$ p \f$ otherwise. Each other
 *   edge joins a node to one of its ancestors and is in the block of the
 *   edge above its deeper end, so that the result is \f$ 3 n \f$ integers
 *   and no label per edge, which `block( u , v )` reads in
 *   \f$ O( 1 ) \f$ and `arcBlocks()` writes out for all the arcs. Boost.Graph
 *   (`biconnected_components()`) pushes each edge on a stack and pops it
 *   into its block, i.e., \f$ m \f$ pushes and pops and a property per edge
 *   that this visit does not need;
 * - `twoEdgeConnectedComponents( g )`, the classes of the nodes joined by
 *   two paths with no edge in common, i.e., the connected components left
 *   when the bridges are removed: the same visit, a node starting a new
 *   class when the edge to its parent is a bridge
 *   (\f$ \mathit{low}[ u ] > \mathit{dt}[ p ] \f$);
 * - `blockCutTree( b )`, the forest whose nodes are the blocks and the cut
 *   nodes, with an edge between a block and each cut node in it.
 *
 * A block is numbered by the least of its nodes other than its head, the
 * node of the block nearest to the least node of its connected component
 * (which every path from the block to that node goes through), so that the
 * numbers do not depend on the visit, and `BiconnectedComponents` holds the
 * same decomposition whoever computes it [see `ParallelBiconnectivity.h`].
 * A node is a cut node if and only if it is in two blocks or more; a block
 * of one edge is a bridge; an isolated node is in no block, and a loop
 * \f$ [ u , u ] \f$ in none (its block is \f$ -1 \f$), since it is in no
 * cycle with another edge. The visit keeps its recursion on an explicit
 * stack, as those of `Visit.h`, so that a path of \f$ 10^8 \f$ nodes does
 * not overflow the stack of the calls; it reads the adjacencies through the
 * compressed rows of `Graph.h`, where they are for a `StaticGraph` and an
 * `ArrayGraph`, copied once for any other model of `Graph`.
 *
 * The visit takes \f$ \Theta( n + m ) \f$ time, and besides the result
 * \f$ 4 n \f$ integers and a stack of at most one frame per node of the
 * current path. The chain decomposition of Schmidt gives the same
 * tests of 2-connectivity from the same visit, by a second pass over the
 * edges out of the tree, and so it would only add a pass here.
 *
 * The contracts [see `Contract.h`]: the graph is unoriented, `block( u , v
 * )` needs an edge \f$ [ u , v ] \f$; at the level of the audits the parts
 * of a decomposition are checked to agree (each block has its head and its
 * least node, and a node is a cut node exactly when it heads a block and is
 * in another one) and the block-cut tree to be a forest. The
 * functions are defined in `Biconnectivity.tpp`.
 *
 * \see J. Hopcroft, R. Tarjan, Algorithm 447: efficient algorithms for
 *      graph manipulation, Communications of the ACM 16(6), 372-378, 1973,
 *      doi:10.1145/362248.362272
 * \see R. E. Tarjan, Depth-first search and linear graph algorithms, SIAM
 *      Journal on Computing 1(2), 146-160, 1972, doi:10.1137/0201010
 * \see J. M. Schmidt, A simple test on 2-vertex- and 2-edge-connectivity,
 *      Information Processing Letters 113(7), 241-244, 2013,
 *      doi:10.1016/j.ipl.2013.01.016
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

#ifndef __ADS_Biconnectivity
 #define __ADS_Biconnectivity
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/StaticGraph.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BiconnectedComponents ----------------------*/
/*--------------------------------------------------------------------------*/

/// the blocks of an unoriented graph, its cut nodes and its bridges, by
/// two integers and a byte per node: the block in which the node is not the
/// head, the rank that tells which end of an edge names its block, and
/// whether the node is a cut node (and, per block, its head, its least node
/// and whether it is a bridge)
class BiconnectedComponents
{

/*--------------------------------------------------------------------------*/
 public:
 /// no node and no block
 BiconnectedComponents( void ) = default;

 /// the decomposition of its parts, as the algorithms build it: the block
 /// \f$ B[ u ] \f$ in which each name \f$ u \f$ is a node but not the head
 /// (\f$ -1 \f$ if none), a rank \f$ r[ u ] \f$ of each name such that the
 /// end of larger rank of each edge has the block of the edge, the head
 /// \f$ h[ k ] \f$ and the least node \f$ \ell[ k ] \f$ with
 /// \f$ B[ \ell[ k ] ] = k \f$ of each block \f$ k \f$, whether it is a
 /// bridge, and whether each name is a cut node
 /// \pre the vectors per name and those per block have the same sizes, the
 ///      blocks are numbered by increasing \f$ \ell \f$
 BiconnectedComponents( std::vector< int > B , std::vector< int > r ,
			std::vector< int > h , std::vector< int > least ,
			std::vector< std::uint8_t > bridge ,
			std::vector< std::uint8_t > cut );

 /// the number of the blocks
 [[nodiscard]] int blocks( void ) const { return( int( f_head.size() ) ); }

 /// the number of the names, the `bound()` of the graph
 [[nodiscard]] int bound( void ) const { return( int( f_B.size() ) ); }

 /// the block of the edge \f$ [ u , v ] \f$, \f$ -1 \f$ for a loop
 /// \pre \f$ [ u , v ] \f$ is an edge of the graph
 [[nodiscard]] int block( int u , int v ) const;

 /// the block in which \f$ u \f$ is a node and not the head, \f$ -1 \f$ if
 /// none (the least node of a connected component, an isolated node)
 /// \pre \f$ 0 \le u < \f$ `bound()`
 [[nodiscard]] int blockOf( int u ) const;

 /// the head of the block \f$ k \f$, its node nearest to the least node of
 /// its connected component
 /// \pre \f$ 0 \le k < \f$ `blocks()`
 [[nodiscard]] int head( int k ) const;

 /// the least node of the block \f$ k \f$ other than its head
 /// \pre \f$ 0 \le k < \f$ `blocks()`
 [[nodiscard]] int least( int k ) const;

 /// whether the block \f$ k \f$ is one edge, a bridge
 /// \pre \f$ 0 \le k < \f$ `blocks()`
 [[nodiscard]] bool isBridge( int k ) const;

 /// whether \f$ u \f$ is a cut node, in two blocks or more
 /// \pre \f$ 0 \le u < \f$ `bound()`
 [[nodiscard]] bool isCut( int u ) const;

 /// the cut nodes, increasing
 [[nodiscard]] std::vector< int > cutNodes( void ) const;

 /// the bridges as pairs \f$ ( \min , \max ) \f$ of their ends, by
 /// increasing block
 [[nodiscard]] std::vector< std::pair< int , int > > bridges( void ) const;

 /// the block of each arc of \f$ g \f$, in the places of its compressed rows
 /// (the order of `forEachAdj()`), \f$ -1 \f$ for a loop
 /// \pre \f$ g \f$ is the graph of the decomposition
 template< Graph G >
 [[nodiscard]] std::vector< int > arcBlocks( const G & g ) const;

 /// the rank \f$ r[ u ] \f$ of the end that names the block of an edge
 /// \pre \f$ 0 \le u < \f$ `bound()`
 [[nodiscard]] int rank( int u ) const;

 /// whether two decompositions are the same, the ranks aside
 [[nodiscard]] bool sameBlocks( const BiconnectedComponents & o ) const {
  return( ( f_B == o.f_B ) && ( f_head == o.f_head ) &&
	   ( f_least == o.f_least ) && ( f_bridge == o.f_bridge ) &&
	   ( f_cut == o.f_cut ) );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the parts agree: each block has its least node and its head, a head
 /// is a cut node unless it heads one block alone and is in no other
 [[nodiscard]] bool consistent( void ) const;

 std::vector< int > f_B;                ///< the block of each name
 std::vector< int > f_rank;             ///< the rank of each name
 std::vector< int > f_head;             ///< the head of each block
 std::vector< int > f_least;            ///< the least node of each block
 std::vector< std::uint8_t > f_bridge;  ///< whether a block is a bridge
 std::vector< std::uint8_t > f_cut;     ///< whether a name is a cut node

 };  // end( class BiconnectedComponents )

/// the block-cut tree: the nodes \f$ 0 , \ldots , b - 1 \f$ are the blocks,
/// the node \f$ b + i \f$ the cut node `cuts[ i ]`
struct BlockCutTree
{
 StaticGraph<> tree;       ///< the forest, unoriented
 std::vector< int > cuts;  ///< the cut nodes, increasing
 };

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the blocks, the cut nodes and the bridges of an unoriented graph
///
/// In pseudocode (Hopcroft and Tarjan with no stack of edges, beyond the
/// book): \f$ \mathit{dt} \f$ the order of discovery, \f$ p \f$ the parent
/// in the tree of the visit, \f$ \mathit{low}[ u ] \f$ the least
/// \f$ \mathit{dt} \f$ that the subtree of \f$ u \f$ reaches by one edge
/// out of the tree; the recursion is kept on an explicit stack.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{biconnectedComponents}( G ) \cr
/// 2 & \quad t \gets 0;\ k \gets 0;\ \mathit{dt}[ u ] \gets -1\ \forall u \cr
/// 3 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ \mathit{dt}[ r ]
///     = -1\ \textbf{do}\ p[ r ] \gets \textbf{nil};\ \mathrm{visit}( r ) \cr
/// 4 & \quad \textbf{foreach}\ u \text{ by increasing } \mathit{dt}
///     \ \textbf{with}\ p[ u ] \ne \textbf{nil}\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathit{low}[ u ] \ge \mathit{dt}[ p[ u ] ]
///     \ \textbf{then}\ B[ u ] \gets k;\ h[ k ] \gets p[ u ];\ k \gets k + 1
///     \quad \triangleright\ u \text{ opens a block} \cr
/// 6 & \qquad \textbf{else}\ B[ u ] \gets B[ p[ u ] ] \cr
/// 7 & \quad \textbf{return}\ B , h \text{ with the blocks numbered by }
///     \text{their least node } u \text{ with } B[ u ] = k \cr
/// 8 & \textbf{procedure}\ \mathrm{visit}( u ) \cr
/// 9 & \quad \mathit{dt}[ u ] \gets t;\ \mathit{low}[ u ] \gets t;\ t \gets
///     t + 1;\ s \gets \textbf{false} \cr
/// 10 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 11 & \qquad \textbf{if}\ v = p[ u ]\ \textbf{and not}\ s\ \textbf{then}
///     \ s \gets \textbf{true}\quad \triangleright\ \text{the edge of the }
///     \text{tree, once} \cr
/// 12 & \qquad \textbf{else if}\ \mathit{dt}[ v ] = -1\ \textbf{then}
///     \ p[ v ] \gets u;\ \mathrm{visit}( v );\ \mathit{low}[ u ] \gets
///     \min( \mathit{low}[ u ] , \mathit{low}[ v ] ) \cr
/// 13 & \qquad \textbf{else}\ \mathit{low}[ u ] \gets \min( \mathit{low}[
///     u ] , \mathit{dt}[ v ] )
/// \end{array}}
/// \f]
/// The block of an edge \f$ [ u , v ] \f$ is \f$ B \f$ of its end of larger
/// \f$ \mathit{dt} \f$; the block \f$ k \f$ is a bridge if
/// \f$ \mathit{low}[ u ] > \mathit{dt}[ p[ u ] ] \f$ for the node \f$ u \f$
/// that opens it; a node is a cut node if it is the head of a block, of two
/// if \f$ p[ u ] = \textbf{nil} \f$.
/// \pre \f$ g \f$ is unoriented
/// \post the parts of the result agree (audit)
template< Graph G >
[[nodiscard]] BiconnectedComponents biconnectedComponents( const G & g );

/// the least node of the 2-edge-connected component of each name,
/// \f$ -1 \f$ for a name that is not a node
///
/// In pseudocode (beyond the book), the visit of `biconnectedComponents()`
/// and then:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{twoEdgeConnectedComponents}( G ) \cr
/// 2 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ \mathit{dt}[ r ]
///     = -1\ \textbf{do}\ p[ r ] \gets \textbf{nil};\ \mathrm{visit}( r ) \cr
/// 3 & \quad \textbf{foreach}\ u \text{ by increasing } \mathit{dt}
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ p[ u ] = \textbf{nil}\ \textbf{or}\ \mathit{low}[
///     u ] > \mathit{dt}[ p[ u ] ]\ \textbf{then}\ C[ u ] \gets u
///     \quad \triangleright\ [ p[ u ] , u ] \text{ is a bridge} \cr
/// 5 & \qquad \textbf{else}\ C[ u ] \gets C[ p[ u ] ] \cr
/// 6 & \quad \textbf{return}\ \text{each } C[ u ] \text{ replaced by the }
///     \text{least node } v \text{ with } C[ v ] = C[ u ]
/// \end{array}}
/// \f]
/// \pre \f$ g \f$ is unoriented
template< Graph G >
[[nodiscard]] std::vector< int > twoEdgeConnectedComponents( const G & g );

/// the block-cut tree of a decomposition
///
/// In pseudocode (beyond the book), \f$ b \f$ the number of the blocks and
/// \f$ c \f$ the cut nodes in increasing order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{blockCutTree}( B ) \cr
/// 2 & \quad T \gets \text{the graph of the nodes } 0 , \ldots , b + | c |
///     - 1 \text{ and no edge} \cr
/// 3 & \quad \textbf{foreach}\ \text{block } k\ \textbf{with}\ h[ k ]
///     \text{ a cut node}\ \textbf{do}\ \text{add } [ k , b + i ] \text{ to }
///     T \text{ for } c_i = h[ k ] \cr
/// 4 & \quad \textbf{foreach}\ c_i\ \textbf{with}\ B[ c_i ] \ne -1
///     \ \textbf{do}\ \text{add } [ B[ c_i ] , b + i ] \text{ to } T \cr
/// 5 & \quad \textbf{return}\ T
/// \end{array}}
/// \f]
/// \post the tree has as many edges as nodes less its connected components
///       (audit)
[[nodiscard]] inline BlockCutTree
blockCutTree( const BiconnectedComponents & b );

/*--------------------------------------------------------------------------*/

namespace detail
{

/// the visit of `biconnectedComponents()`: the order of discovery, the
/// lowpoint and the parent of each name, \f$ -1 \f$ if none, and the nodes
/// in the order of discovery
struct LowPoints
{
 std::vector< int > dt;     ///< the order of discovery
 std::vector< int > low;    ///< the lowpoint
 std::vector< int > p;      ///< the parent
 std::vector< int > order;  ///< the nodes by increasing dt
 };

/// the visit of an unoriented graph by its compressed rows
template< Graph G >
LowPoints lowPoints( const G & g , const Rows< G > & A );

/// the decomposition of the block \f$ B[ u ] \f$ of each name
/// (\f$ -1 \f$ if none), of the head of each block and of whether it is a
/// bridge, in any numbering of the blocks: the blocks numbered by their
/// least node, and the cut nodes
inline BiconnectedComponents
assembleBlocks( std::vector< int > B , std::vector< int > rank ,
		std::vector< int > h , std::vector< std::uint8_t > bridge );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Biconnectivity.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Biconnectivity.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Biconnectivity.h ------------------------*/
/*--------------------------------------------------------------------------*/
