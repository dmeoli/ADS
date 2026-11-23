/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayGraph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The static GRAPH with vectors of adjacency of section 9.4 [see `Graph.h`]:
 * the adjacent nodes of all the nodes in one vector, the ones of \f$ u \f$ in
 * the places \f$ [ \mathrm{off}( u ) , \mathrm{off}( u + 1 ) ) \f$; no
 * insertion nor deletion, \f$ \Theta( n + m ) \f$ space and no pointer.
 *
 * The contracts [see `Contract.h`]: the graph has n >= 0 nodes and its edges
 * join two distinct nodes; `adj()`, `forEachAdj()` and `forEachEdge()` need a
 * node; the invariant is that the offsets do not decrease and end at the
 * size of the vector, whose entries are nodes. The operations are defined
 * in `ArrayGraph.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
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

#ifndef __ADS_ArrayGraph
 #define __ADS_ArrayGraph
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ArrayGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a static graph with vectors of adjacency, all in one vector
template< class W = std::monostate >
class ArrayGraph
{

/*--------------------------------------------------------------------------*/
 public:
 using Weight = W;  ///< the type of the weights

 /// whether the arcs have a weight, as in `StaticGraph`
 static constexpr bool weighted = ! std::is_same_v< W , std::monostate >;

 /// the graph of the nodes \f$ 0 , \ldots , n - 1 \f$ and of the given edges;
 /// an unoriented edge \f$ [ u , v ] \f$ is given once
 ///
 /// In pseudocode (section 9.4), the offsets by counting:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( n , E )\quad \triangleright
 ///     \ \text{the adjacent nodes of } u \text{ in } [ \mathit{off}[ u ] ,
 ///     \mathit{off}[ u + 1 ] ) \cr
 /// 2 & \quad \mathit{off}[ u + 1 ] \gets \text{the degree of } u
 ///     \text{ in } E \text{ for each node } u\quad \triangleright
 ///     \ \text{both ends if unoriented} \cr
 /// 3 & \quad \text{make } \mathit{off} \text{ its own prefix sums};
 ///     \ \mathit{off}[ 0 ] = 0 \cr
 /// 4 & \quad \textbf{foreach}\ ( u , v ) \in E\ \textbf{do}\ \text{write }
 ///     v \text{ in the first free place of the range of } u \cr
 ///   & \qquad \text{(and } u \text{ in that of } v \text{ if unoriented)}
 ///     \cr
 /// 5 & \quad \textbf{return}\ ( \mathit{off} ,
 ///     \text{the vector of the adjacent nodes} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 0 \f$, and each edge joins two distinct nodes
 ArrayGraph( int n , const std::vector< std::tuple< int , int , W > > & E ,
	     bool directed = true );

 /// the copy of any graph, with the nodes 0 , ... , g.bound() - 1: the
 /// names that are not nodes of g, deleted ones, become isolated nodes, so
 /// that each node keeps its name
 template< Graph G >
 explicit ArrayGraph( const G & g );

 /// whether the graph is oriented
 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( int( f_off.size() ) - 1 ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// the number of arcs, 2 `m()` if the graph is unoriented, as in
 /// `StaticGraph`
 [[nodiscard]] int arcs( void ) const { return( int( f_to.size() ) ); }

 /// the names of the nodes, \f$ [ 0 , \mathrm{bound}() ) = [ 0 , n ) \f$
 [[nodiscard]] int bound( void ) const { return( n() ); }

 /// whether \f$ u \f$ is a node
 [[nodiscard]] bool hasNode( int u ) const;

 /// the set of the nodes, in increasing order
 [[nodiscard]] std::vector< int > V( void ) const;

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const;

 /// the weight of \f$ ( u , v ) \f$, nothing if there is no such edge,
 /// in \f$ O( |\mathrm{adj}( u )| ) \f$
 [[nodiscard]] std::optional< W > weight( int u , int v ) const;

 /// the weight of \f$ ( u , v ) \f$ set to \f$ w \f$ (of both its arcs if
 /// unoriented), in \f$ O( |\mathrm{adj}( u )| + |\mathrm{adj}( v )| )
 /// \f$; false if there is no such edge
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes
 bool setWeight( int u , int v , const W & w );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the nodes, \f$ 0 , \ldots , n - 1 \f$, random access
 [[nodiscard]] auto nodes( void ) const {
  return( std::views::iota( 0 , n() ) );
  }

 /// the nodes adjacent to \f$ u \f$, contiguous in the array of the edges
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::span< const int > neighbors( int u ) const;

 /// the offsets of the rows, \f$ n + 1 \f$ of them: the adjacent nodes of
 /// \f$ u \f$ are the places from `offsets()[ u ]` to `offsets()[ u + 1 ]`
 /// of `heads()`, and their weights the same places of `weightData()`
 /// (the arrays that the adapters to other libraries read in place)
 [[nodiscard]] const int * offsets( void ) const { return( f_off.data() ); }

 /// the adjacent nodes, row by row
 [[nodiscard]] const int * heads( void ) const { return( f_to.data() ); }

 /// the weights, in the places of `heads()`
 [[nodiscard]] const W * weightData( void ) const { return( f_w.data() ); }

 /// the heads of all the arcs read in place, `heads()`, as in `StaticGraph`
 [[nodiscard]] const int * arcHeads( void ) const { return( heads() ); }

 /// the weights of all the arcs read in place, `weightData()`, as in
 /// `StaticGraph`
 [[nodiscard]] const W * arcWeights( void ) const
  requires( weighted )
 {
  return( weightData() );
  }

/*--------------------------------------------------------------------------*/
 private:
 template< class G >
 static std::vector< std::tuple< int , int , W > > edges( const G & g );

 /// the offsets do not decrease, the last is the size of the vector, and
 /// its entries are nodes
 [[nodiscard]] bool consistent( void ) const;

 bool f_directed;           ///< oriented or not
 std::vector< int > f_off;  ///< the adjacent nodes of u from f_off[ u ]
 std::vector< int > f_to;   ///< the adjacent nodes
 std::vector< W > f_w;      ///< the weights
 int f_m = 0;               ///< the number of edges

 };  // end( class ArrayGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/ArrayGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ArrayGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
