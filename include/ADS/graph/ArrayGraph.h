/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayGraph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The static GRAPH with vectors of adjacency of section 9.4 [see Graph.h]:
 * the adjacent nodes of all the nodes in one vector, the ones of u in the
 * places [ off( u ) , off( u + 1 ) ); no insertion nor deletion,
 * Theta( n + m ) space and no pointer.
 *
 * The contracts [see Contract.h]: the graph has n >= 0 nodes and its edges
 * join two distinct nodes; adj(), forEachAdj() and forEachEdge() need a
 * node; the invariant is that the offsets do not decrease and end at the
 * size of the vector, whose entries are nodes. The operations are defined
 * in ArrayGraph.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.4,
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

#ifndef __ADS_ArrayGraph
 #define __ADS_ArrayGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <tuple>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ArrayGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a static graph with vectors of adjacency, all in one vector
template< class W = std::monostate >
class ArrayGraph {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights

 /// the graph of the nodes 0 , ... , n - 1 and of the given edges; an
 /// unoriented edge [ u , v ] is given once
 /// \pre n >= 0, and each edge joins two distinct nodes
 ArrayGraph( int n , const std::vector< std::tuple< int , int , W > > & E ,
             bool directed = true );

 /// the copy of any graph whose nodes are 0 , ... , n - 1
 template< Graph G >
 explicit ArrayGraph( const G & g );

 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 [[nodiscard]] int n( void ) const { return( int( f_off.size() ) - 1 ); }

 [[nodiscard]] int m( void ) const { return( f_m ); }

 [[nodiscard]] int bound( void ) const { return( n() ); }

 [[nodiscard]] bool hasNode( int u ) const;

 [[nodiscard]] std::vector< int > V( void ) const;

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

 template< class G >
 static std::vector< std::tuple< int , int , W > > edges( const G & g );

 /// the offsets do not decrease, the last is the size of the vector, and
 /// its entries are nodes
 [[nodiscard]] bool consistent( void ) const;

 bool f_directed;            ///< oriented or not
 std::vector< int > f_off;   ///< the adjacent nodes of u from f_off[ u ]
 std::vector< int > f_to;    ///< the adjacent nodes
 std::vector< W > f_w;       ///< the weights
 int f_m = 0;                ///< the number of edges

 };  // end( class ArrayGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/ArrayGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArrayGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
