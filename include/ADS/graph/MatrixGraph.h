/*--------------------------------------------------------------------------*/
/*--------------------------- File MatrixGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH with an adjacency matrix of section 9.3 [see Graph.h]: the
 * weight of ( u , v ), or nothing, the infinity of the book, in place
 * ( u , v ); Theta( n^2 ) space, hasEdge() in O( 1 ), adj( u ) in
 * Theta( n ), insertNode() and deleteNode() in Theta( n ).
 *
 * The contracts [see Contract.h]: insertEdge() and deleteEdge() need two
 * distinct nodes, deleteNode(), adj(), forEachAdj() and forEachEdge() a
 * node; the invariant is that the matrix is n x n with no entry on the
 * diagonal nor in the rows and columns of the names not in use, m() entries
 * in all, and symmetric for an unoriented graph. The operations check it
 * on the rows and the columns they touch, in O( n ). The operations are
 * defined in MatrixGraph.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
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

#ifndef __ADS_MatrixGraph
 #define __ADS_MatrixGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <optional>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MatrixGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph with an adjacency matrix
template< class W = std::monostate , class T = std::monostate >
class MatrixGraph : public GraphNodes< T > {

 using B = GraphNodes< T >;

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights
 using Label = T;    ///< the type of the labels

 /// Graph(): the graph of the nodes 0 , ... , n - 1, with no edge
 /// \pre n >= 0
 explicit MatrixGraph( int n = 0 , bool directed = true );

 /// a new node, a new row and a new column, O( n ) time
 int insertNode( const T & t = T() );

 /// \pre u and v are distinct nodes
 /// \post hasEdge( u , v )
 void insertEdge( int u , int v , const W & w = W() );

 /// \pre u and v are distinct nodes
 /// \post ! hasEdge( u , v )
 void deleteEdge( int u , int v );

 /// removes the node u and its edges, Theta( n ) time
 /// \pre u is a node
 /// \post ! hasNode( u )
 void deleteNode( int u );

 /// O( 1 ) time
 [[nodiscard]] bool hasEdge( int u , int v ) const;

 /// the weight of ( u , v ), nothing (the infinity of the book) if there is
 /// no such edge
 [[nodiscard]] std::optional< W > weight( int u , int v ) const;

 /// the set of the nodes adjacent to u, in increasing order, Theta( n ) time
 /// \pre u is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \pre u is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \pre u is a node
 template< class F >
 void forEachEdge( int u , F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the matrix is bound() x bound(), with m() entries off the diagonal
 /// between nodes, symmetric if unoriented, Theta( n^2 )
 [[nodiscard]] bool consistent( void ) const;

 /// the row and the column of u have no entry on the diagonal nor out of
 /// the nodes, and they are equal if unoriented, O( n )
 [[nodiscard]] bool consistentAt( int u ) const;

 std::vector< std::vector< std::optional< W > > > f_M;   ///< the matrix

 };  // end( class MatrixGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/MatrixGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MatrixGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MatrixGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
