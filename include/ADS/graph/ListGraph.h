/*--------------------------------------------------------------------------*/
/*---------------------------- File ListGraph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH with lists of adjacency of section 9.4 [see `Graph.h`]: for each
 * node the LIST of its adjacent nodes and of the weights of the edges,
 * \f$ \Theta( n + m ) \f$ space, adj( u ) in
 * \f$ \Theta( |\mathrm{adj}( u )| ) \f$, hasEdge( u , v ) in
 * \f$ O( |\mathrm{adj}( u )| ) \f$, all the edges in \f$ \Theta( n + m ) \f$,
 * `deleteNode()` in \f$ O( n + m ) \f$.
 *
 * The contracts [see `Contract.h`]: `insertEdge()` and `deleteEdge()` need two
 * distinct nodes, `deleteNode()`, `adj()`, `forEachAdj()` and `forEachEdge()`
 * a node; the invariant is that the lists hold only nodes of the graph,
 * without loops nor repetitions, `m()` edges in all, and that an unoriented
 * edge is in the lists of both its ends with the same weight. The insertions
 * and the deletions of edges check it on the lists of the two ends, in
 * \f$ O( d \log d ) \f$ for d adjacent nodes, `deleteNode()` on the whole
 * graph. The operations are defined in `ListGraph.tpp`.
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

#ifndef __ADS_ListGraph
 #define __ADS_ListGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/graph/Graph.h"
#include "ADS/sequence/List.h"

#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ListGraph -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph with lists of adjacency
template< class W = std::monostate , class T = std::monostate >
class ListGraph : public GraphNodes< T > {

 using B = GraphNodes< T >;

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights
 using Label = T;    ///< the type of the labels

 /// the graph of the nodes \f$ 0 , \ldots , n - 1 \f$, with no edge
 /// \pre \f$ n \ge 0 \f$
 explicit ListGraph( int n = 0 , bool directed = true );

 /// a new node, with no edge, and its name
 int insertNode( const T & t = T() );

 /// the edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$, of weight \f$ w \f$;
 /// only the weight changes if it is there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 /// \post `hasEdge( u , v )`
 void insertEdge( int u , int v , const W & w = W() );

 /// removes the edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$, if it is there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 /// \post not `hasEdge( u , v )`
 void deleteEdge( int u , int v );

 /// removes the node \f$ u \f$ and its edges, \f$ O( n + m ) \f$ time
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{deleteNode}( u ) \cr
 /// 2 & \quad \textbf{foreach}\ x \in V\ \textbf{do}\  \text{remove } u
 ///     \text{ from the list of } x \cr
 /// 3 & \quad \text{empty the list of } u;\  \text{mark the name } u \text{ }
 ///     \text{unused}
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ is a node
 /// \post not `hasNode( u )`
 void deleteNode( int u );

 /// whether \f$ ( u , v ) \f$ is an edge
 [[nodiscard]] bool hasEdge( int u , int v ) const;

 /// the weight of \f$ ( u , v ) \f$, nothing if there is no such edge
 [[nodiscard]] std::optional< W > weight( int u , int v ) const;

 /// the set of the nodes adjacent to \f$ u \f$, in the order of insertion
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \f$ f( v ) \f$ on the nodes adjacent to \f$ u \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \f$ f( v , w ) \f$ on the nodes adjacent to \f$ u \f$ and the weights of
 /// the edges
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the nodes adjacent to \f$ u \f$, in the order of its list, bidirectional
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] auto neighbors( int u ) const {
  using It = KeyIterator< typename List< std::pair< int , W > >::
                          const_iterator >;
  const auto & L = f_adj[ this->check( u ) ];
  return( std::ranges::subrange( It( L.begin() ) , It( L.end() ) ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 bool find( int u , int v ) const;

 bool set( int u , int v , const W & w );

 bool erase( int u , int v );

 /// the lists hold `m()` edges between distinct nodes, once each, and both
 /// ends of an unoriented one, \f$ O( n + m ) \f$ for an oriented graph
 [[nodiscard]] bool consistent( void ) const;

 /// the list of \f$ u \f$ holds distinct nodes other than \f$ u \f$, and, if
 /// unoriented, the edge \f$ [ u , v ] \f$ is in both lists with the same
 /// weight or in none, \f$ O( d \log d ) \f$
 [[nodiscard]] bool consistentAt( int u , int v ) const;

 std::vector< List< std::pair< int , W > > > f_adj;   ///< the lists

 };  // end( class ListGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/ListGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListGraph.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ListGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
