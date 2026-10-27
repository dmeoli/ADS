/*--------------------------------------------------------------------------*/
/*--------------------------- File StaticGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A static GRAPH for the engineered algorithms, beyond the book [see
 * `Graph.h`]: the compressed sparse rows of `ArrayGraph` [see `ArrayGraph.h`]
 * (the format CSR of the sparse matrices), with each arc of a weighted
 * graph stored next to its weight, so that a scan of the arcs of u reads
 * one contiguous run of ( head , weight ) pairs, and the arcs of each node
 * sorted by head, which makes the reads of the visits that follow them go
 * forward in memory. With \f$ a = m \f$ arcs if oriented and
 * \f$ a = 2 m \f$ if not, the arcs of u are the places
 * \f[
 *   \mathit{off}( u ) \leq k < \mathit{off}( u + 1 ) , \qquad
 *   \mathit{off}( 0 ) = 0 , \quad \mathit{off}( n ) = a ,
 * \f]
 * so that the graph takes \f$ 4 ( n + 1 ) + ( 4 + s ) a \f$ bytes, s the
 * size of a weight (0 if unweighted). The graph is built from a list of
 * edges in \f$ \Theta( n + a ) \f$ time by two passes of counting sort, by
 * head and then, stably, by tail, and an unoriented edge [ u , v ] gives
 * the two arcs ( u , v ) and ( v , u ). There may be parallel arcs.
 *
 * `transpose()` gives the graph with the arcs reversed, which the visits
 * from the bottom up and the strongly connected components of a reversed
 * graph need, and `relabel()` gives the same graph with the nodes renamed,
 * e.g., by `bfsOrder()`, the order of a visit in breadth from a node and then
 * from each node not reached (the ordering of Cuthill and McKee without its
 * sort by degree), which puts close together the nodes that the algorithms
 * visit close in time; both take \f$ \Theta( n + a ) \f$. `StaticGraph`
 * models the concept `Graph`, so that the algorithms of the book also apply
 * to it.
 *
 * The contracts [see `Contract.h`]: the graph has n >= 0 nodes and fewer
 * than \f$ 2^{31} \f$ arcs, its edges join two distinct nodes; the
 * operations on a node need a node, `relabel()` a permutation of the nodes;
 * the invariant is that the offsets do not decrease and end at a, and that
 * the heads are nodes, in increasing order for each tail. The operations
 * are defined in `StaticGraph.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see Y. Saad, Iterative Methods for Sparse Linear Systems, 2nd ed.,
 *      SIAM, 2003, section 3.4, doi:10.1137/1.9780898718003
 * \see E. Cuthill, J. McKee, Reducing the bandwidth of sparse symmetric
 *      matrices, Proceedings of the 24th ACM National Conference, 157-172,
 *      1969, doi:10.1145/800195.805928
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

#ifndef __ADS_StaticGraph
 #define __ADS_StaticGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <cstddef>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/// a hint to bring the line of \f$ p \f$ into the cache, or nothing
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_GRAPH_PREFETCH( p ) __builtin_prefetch( ( const void * )( p ) )
#else
 #define ADS_GRAPH_PREFETCH( p ) ( ( void ) 0 )
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS StaticGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an arc of a weighted `StaticGraph`, its head next to its weight
template< class W >
struct WeightedArc {
 int to;   ///< the head
 W w;      ///< the weight
 };

/// a static graph in compressed sparse rows, the arcs of each node sorted
/// by head, each next to its weight
template< class W = std::monostate >
class StaticGraph {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights

 /// whether the arcs have a weight
 static constexpr bool weighted = ! std::is_same_v< W , std::monostate >;

 /// an arc: its head, and its weight if the graph is weighted
 using Arc = std::conditional_t< weighted , WeightedArc< W > , int >;

 /// the graph with no node
 StaticGraph( void ) : f_off( 1 , 0 ) {}

 /// the graph of the nodes \f$ 0 , \ldots , n - 1 \f$ and of the given edges;
 /// an unoriented edge \f$ [ u , v ] \f$ is given once
 ///
 /// In pseudocode (two passes of counting sort, the second stable, so that the
 /// heads of each row come sorted):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( n , E ) \cr
 /// 2 & \quad A \gets \text{the arcs of } E \text{ , both } ( u , v ) \text{ }
 ///     \text{and } ( v , u ) \text{ if unoriented} \cr
 /// 3 & \quad A \gets \text{the arcs of } A \text{ sorted by head, by }
 ///     \text{counting} \cr
 /// 4 & \quad \textbf{foreach}\ ( u , v ) \in A\ \textbf{do}\ \mathit{off}[ u
 ///     + 1 ] \gets \mathit{off}[ u + 1 ] + 1 \cr
 /// 5 & \quad \textbf{for}\ u \gets 1\ \textbf{to}\ n\ \textbf{do}
 ///     \ \mathit{off}[ u ] \gets \mathit{off}[ u ] + \mathit{off}[ u - 1 ]
 ///     \cr
 /// 6 & \quad \textbf{foreach}\ ( u , v ) \in A \text{ in order }
 ///     \ \textbf{do}\ \mathit{arc}[ \mathit{next}[ u ] ] \gets v;
 ///     \ \mathit{next}[ u ] \gets \mathit{next}[ u ] + 1\quad \triangleright
 ///     \ \mathit{next}[ u ] \text{ starts at } \mathit{off}[ u ] \cr
 /// 7 & \quad \textbf{return}\ ( \mathit{off} , \mathit{arc} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 0 \f$, each edge joins two distinct nodes, fewer than
 ///      \f$ 2^{31} \f$ arcs
 StaticGraph( int n , const std::vector< std::tuple< int , int , W > > & E ,
              bool directed = true );

 /// the unweighted graph of the nodes \f$ 0 , \ldots , n - 1 \f$ and of the
 /// edges \f$ E \f$
 /// \pre \f$ n \ge 0 \f$, each edge joins two distinct nodes, fewer than
 ///      \f$ 2^{31} \f$ arcs
 StaticGraph( int n , const std::vector< std::pair< int , int > > & E ,
              bool directed = true ) requires( ! weighted );

 /// the copy of any graph, the names that are not nodes of \f$ g \f$ becoming
 /// isolated nodes
 template< Graph G >
 explicit StaticGraph( const G & g );

 /// whether the graph is oriented
 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( int( f_off.size() ) - 1 ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// the number of arcs, 2 `m()` if the graph is unoriented
 [[nodiscard]] int arcs( void ) const { return( int( f_arc.size() ) ); }

 [[nodiscard]] int bound( void ) const { return( n() ); }

 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < n() ) );
  }

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] int degree( int u ) const;

 [[nodiscard]] std::vector< int > V( void ) const;

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \f$ f( v , w ) \f$ on each arc \f$ ( u , v ) \f$ of weight \f$ w \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const requires( weighted );

 /// the arcs out of \f$ u \f$, contiguous
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::span< const Arc > out( int u ) const;

 /// the offsets of the rows: the arcs of u are those in the places
 /// [ `offsets()`[ u ] , `offsets()`[ u + 1 ] ) of `arcData()`
 [[nodiscard]] const int * offsets( void ) const { return( f_off.data() ); }

 /// all the arcs, row by row
 [[nodiscard]] const Arc * arcData( void ) const { return( f_arc.data() ); }

 /// the head of an arc
 static int head( const Arc & a ) {
  if constexpr( weighted ) return( a.to ); else return( a );
  }

 /// the graph with each arc reversed (the same graph if unoriented)
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{transpose}() \cr
 /// 2 & \quad \textbf{return}\ \mathrm{build}( n , \text{the arcs } ( v , u )
 ///     \text{ for each arc } ( u , v ) )
 /// \end{array}}
 /// \f]
 [[nodiscard]] StaticGraph transpose( void ) const;

 /// the same graph with the node \f$ u \f$ renamed \f$ p[ u ] \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{relabel}( p ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{build}( n , \text{the arcs } ( p[ u ] ,
 ///     p[ v ] ) \text{ for each arc } ( u , v ) )
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ is a permutation of the nodes
 [[nodiscard]] StaticGraph relabel( const std::vector< int > & p ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the nodes, \f$ 0 , \ldots , n - 1 \f$, random access
 [[nodiscard]] auto nodes( void ) const {
  return( std::views::iota( 0 , n() ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the rows of the arcs \f$ ( u , \mathit{head} , \mathit{weight} ) \f$ of
 /// \f$ E \f$, both ways if unoriented, by counting sort on the head and then
 /// on the tail
 template< class Edges , class Get >
 void build( int n , const Edges & E , Get && get );

 /// the offsets do not decrease and end at the number of arcs, whose heads
 /// are nodes, increasing in each row
 [[nodiscard]] bool consistent( void ) const;

 bool f_directed = true;      ///< oriented or not
 std::vector< int > f_off;    ///< the arcs of u from f_off[ u ]
 std::vector< Arc > f_arc;    ///< the arcs, row by row
 int f_m = 0;                 ///< the number of edges

 };  // end( class StaticGraph )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the order of a visit in breadth from \f$ r \f$, then from the least node
/// not reached, and so on: the name p[ u ] of each node u in the relabelled
/// graph, a permutation of the nodes
///
/// In pseudocode (the ordering of Cuthill and McKee without its sort by
/// degree):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bfsOrder}( G , r ) \cr
/// 2 & \quad k \gets 0;\ p[ v ] \gets \textbf{nil}\  \text{for each node} \cr
/// 3 & \quad \textbf{foreach}\ s \in ( r , 0 , 1 , \ldots , n - 1 )
///     \ \textbf{with}\ p[ s ] = \textbf{nil}\ \textbf{do} \cr
/// 4 & \qquad p[ s ] \gets k;\ k \gets k + 1;\ Q \gets ( s ) \cr
/// 5 & \qquad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad\quad u \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad\quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}
///     \ p[ v ] = \textbf{nil}\ \textbf{do}\ p[ v ] \gets k;\ k \gets k + 1;
///     \ \mathrm{enqueue}( Q , v ) \cr
/// 8 & \quad \textbf{return}\ p
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node, or the graph has none
template< class W >
std::vector< int > bfsOrder( const StaticGraph< W > & g , int r = 0 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/StaticGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StaticGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File StaticGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
