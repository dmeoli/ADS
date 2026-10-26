/*--------------------------------------------------------------------------*/
/*-------------------------- File PointerGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An oriented graph with pointers, beyond the book: each node is an
 * object with its label, a flag for the visits, the number of edges that
 * touch it and the list of its edges, each a pointer to the other end and
 * a weight. A node is reached by its address, which stays valid until it
 * is deleted, and found by label in O( n ). deleteNode() deletes only an
 * isolated node, forceDeleteNode() first deletes the edges that enter or
 * leave it, in O( n + m ); dfs() and bfs() mark the nodes with the flags,
 * cleared at the start of each visit. The graphs of chapter 9 number their
 * nodes from 0 [see ListGraph.h]; this one has neither numbers nor a bound.
 *
 * The contracts [see Contract.h]: the operations need nodes of this graph,
 * not nil (whether a node belongs to the graph costs O( n ), and is checked
 * at the level of the audits); the invariant is that the degree of each
 * node counts the edges that leave it and those that enter it, and that
 * the edges lead to nodes of the graph. The operations are defined in
 * PointerGraph.tpp.
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
  Node * to;
  W weight;
  };

 /// a node
 struct Node {
  T label;
  int degree = 0;              ///< the edges that enter or leave it
  bool seen = false;
  std::list< Edge > out;
  };

 PointerGraph( void ) = default;
 PointerGraph( const PointerGraph & ) = delete;
 PointerGraph & operator=( const PointerGraph & ) = delete;

 [[nodiscard]] int n( void ) const { return( int( f_V.size() ) ); }

 /// a new isolated node of label t
 Node * insertNode( const T & t );

 /// the first node of label t, nullptr if none
 [[nodiscard]] Node * find( const T & t );

 /// \pre u and v are nodes of the graph
 [[nodiscard]] bool hasEdge( const Node * u , const Node * v ) const;

 /// the edge ( u , v ) of weight w, if not there
 /// \pre u and v are nodes of the graph
 /// \post hasEdge( u , v )
 void insertEdge( Node * u , Node * v , const W & w = W() );

 /// \pre u and v are nodes of the graph
 /// \post ! hasEdge( u , v )
 void deleteEdge( Node * u , Node * v );

 /// deletes u if isolated; false otherwise
 /// \pre u is a node of the graph
 bool deleteNode( Node * u );

 /// deletes the edges of u and then u
 /// \pre u is a node of the graph
 void forceDeleteNode( Node * u );

 /// the nodes adjacent to u
 /// \pre u is a node of the graph
 [[nodiscard]] std::vector< Node * > adjacent( const Node * u ) const;

 /// the depth first visit from r, f on each node reached
 /// \pre r is a node of the graph
 template< class F >
 void dfs( Node * r , F && f );

 /// the breadth first visit from r
 /// \pre r is a node of the graph
 template< class F >
 void bfs( Node * r , F && f );

/*--------------------------------------------------------------------------*/
 private:

 void clear( void );

 /// u is one of the nodes, O( n )
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
