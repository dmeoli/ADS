/*--------------------------------------------------------------------------*/
/*-------------------------- File PointerGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An oriented graph with pointers, beyond the book: each node is an object
 * with its label, a flag for the visits, the number of edges that touch it and
 * the list of its edges, each a pointer to the other end and a weight. A node
 * is reached by its address, which stays valid until it is deleted, and found
 * by label in \f$ O( n ) \f$. `deleteNode()` deletes only an isolated node,
 * `forceDeleteNode()` first deletes the edges that enter or leave it, in
 * \f$ O( n + m ) \f$; `dfs()` and `bfs()` mark the nodes with the flags,
 * cleared at the start of each visit. The graphs of chapter 9 number their
 * nodes from 0 [see `ListGraph.h`]; this one has neither numbers nor a bound.
 *
 * The contracts [see `Contract.h`]: the operations need nodes of this graph,
 * not nil (whether a node belongs to the graph costs \f$ O( n ) \f$, and is
 * checked at the level of the audits); the invariant is that the degree of
 * each node counts the edges that leave it and those that enter it, and that
 * the edges lead to nodes of the graph. The operations are defined in
 * `PointerGraph.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.2,
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

#ifndef __ADS_PointerGraph
 #define __ADS_PointerGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <list>
#include <queue>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PointerGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an oriented graph of node objects linked by pointers
template< class T , class W = int >
class PointerGraph {

/*--------------------------------------------------------------------------*/
 public:

 struct Node;

 /// an edge, to its other end
 struct Edge {
  Node * to;   ///< the other end
  W weight;    ///< the weight
  };

 /// a node
 struct Node {
  T label;                     ///< the label
  int degree = 0;              ///< the edges that enter or leave it
  bool seen = false;           ///< the flag of the visits
  std::list< Edge > out;       ///< the edges that leave it
  };

 /// the empty graph
 PointerGraph( void ) = default;
 /// no copy, since the nodes are linked by pointers
 PointerGraph( const PointerGraph & ) = delete;
 /// no copy, since the nodes are linked by pointers
 PointerGraph & operator=( const PointerGraph & ) = delete;

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( int( f_V.size() ) ); }

 /// a new isolated node of label \f$ t \f$
 Node * insertNode( const T & t );

 /// the first node of label \f$ t \f$, nullptr if none
 [[nodiscard]] Node * find( const T & t );

 /// \pre \f$ u \f$ and \f$ v \f$ are nodes of the graph
 [[nodiscard]] bool hasEdge( const Node * u , const Node * v ) const;

 /// the edge \f$ ( u , v ) \f$ of weight \f$ w \f$, if not there
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes of the graph
 /// \post `hasEdge( u , v )`
 void insertEdge( Node * u , Node * v , const W & w = W() );

 /// \pre \f$ u \f$ and \f$ v \f$ are nodes of the graph
 /// \post not `hasEdge( u , v )`
 void deleteEdge( Node * u , Node * v );

 /// deletes \f$ u \f$ if isolated; false otherwise
 /// \pre \f$ u \f$ is a node of the graph
 bool deleteNode( Node * u );

 /// deletes the edges of \f$ u \f$ and then \f$ u \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{forceDeleteNode}( u ) \cr
 /// 2 & \quad \textbf{foreach}\ x \in V\ \textbf{do}\  \text{remove the edge }
 ///     \text{from } x \text{ to } u \text{ , if there is one} \cr
 /// 3 & \quad \text{remove the edges of } u;\ \mathrm{deleteNode}( u )
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ is a node of the graph
 void forceDeleteNode( Node * u );

 /// the nodes adjacent to \f$ u \f$
 /// \pre \f$ u \f$ is a node of the graph
 [[nodiscard]] std::vector< Node * > adjacent( const Node * u ) const;

 /// the depth first visit from \f$ r \f$, \f$ f \f$ on each node reached
 ///
 /// In pseudocode (section 9.5), the flags of the nodes cleared first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{dfs}( r , f ) \cr
 /// 2 & \quad r.\mathit{flag} \gets \textbf{true};\ f( r ) \cr
 /// 3 & \quad \textbf{foreach}\ v \in \mathrm{adjacent}( r )\ \textbf{with
 ///     not}\ v.\mathit{flag}\ \textbf{do}\ \mathrm{dfs}( v , f )
 /// \end{array}}
 /// \f]
 /// \pre \f$ r \f$ is a node of the graph
 template< class F >
 void dfs( Node * r , F && f );

 /// the breadth first visit from \f$ r \f$
 ///
 /// In pseudocode (section 9.5), the flags of the nodes cleared first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bfs}( r , f ) \cr
 /// 2 & \quad r.\mathit{flag} \gets \textbf{true};\ Q \gets ( r ) \cr
 /// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 4 & \qquad u \gets \mathrm{dequeue}( Q );\ f( u ) \cr
 /// 5 & \qquad \textbf{foreach}\ v \in \mathrm{adjacent}( u )\ \textbf{with
 ///     not}\ v.\mathit{flag}\ \textbf{do}\ v.\mathit{flag} \gets
 ///     \textbf{true};\ \mathrm{enqueue}( Q , v )
 /// \end{array}}
 /// \f]
 /// \pre \f$ r \f$ is a node of the graph
 template< class F >
 void bfs( Node * r , F && f );

/*--------------------------------------------------------------------------*/
 private:

 void clear( void );

 /// \f$ u \f$ is one of the nodes, \f$ O( n ) \f$
 [[nodiscard]] bool owns( const Node * u ) const;

 /// the degrees count the edges in and out, and the edges lead to nodes
 [[nodiscard]] bool consistent( void ) const;

 std::list< Node > f_V;   ///< the nodes, at fixed addresses

 };  // end( class PointerGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/PointerGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PointerGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PointerGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
