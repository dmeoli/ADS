/*--------------------------------------------------------------------------*/
/*--------------------------- File MatrixGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH with an adjacency matrix of section 9.3 [see `Graph.h`]: the
 * weight of \f$ ( u , v ) \f$, or nothing, the infinity of the book, in place
 * \f$ ( u , v ) \f$; \f$ \Theta( n^2 ) \f$ space, `hasEdge()` in
 * \f$ O( 1 ) \f$, adj( u ) in \f$ \Theta( n ) \f$, `insertNode()` and
 * `deleteNode()` in \f$ \Theta( n ) \f$.
 *
 * The contracts [see `Contract.h`]: `insertEdge()` and `deleteEdge()` need two
 * distinct nodes, `deleteNode()`, `adj()`, `forEachAdj()` and `forEachEdge()`
 * a node; the invariant is that the matrix is n x n with no entry on the
 * diagonal nor in the rows and columns of the names not in use, `m()` entries
 * in all, and symmetric for an unoriented graph. The operations check it on
 * the rows and the columns they touch, in \f$ O( n ) \f$. The operations are
 * defined in `MatrixGraph.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.3,
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

#ifndef __ADS_MatrixGraph
 #define __ADS_MatrixGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/graph/Graph.h"

#include <cstddef>
#include <optional>
#include <ranges>
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

 /// the graph of the nodes \f$ 0 , \ldots , n - 1 \f$, with no edge
 /// \pre \f$ n \ge 0 \f$
 explicit MatrixGraph( int n = 0 , bool directed = true );

 /// a new node, a new row and a new column, \f$ O( n ) \f$ time
 int insertNode( const T & t = T() );

 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 /// \post `hasEdge( u , v )`
 void insertEdge( int u , int v , const W & w = W() );

 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 /// \post not `hasEdge( u , v )`
 void deleteEdge( int u , int v );

 /// removes the node \f$ u \f$ and its edges, \f$ \Theta( n ) \f$ time
 /// \pre \f$ u \f$ is a node
 /// \post not `hasNode( u )`
 void deleteNode( int u );

 /// \f$ O( 1 ) \f$ time
 [[nodiscard]] bool hasEdge( int u , int v ) const;

 /// the weight of \f$ ( u , v ) \f$, nothing (the infinity of the book) if
 /// there is no such edge
 [[nodiscard]] std::optional< W > weight( int u , int v ) const;

 /// the set of the nodes adjacent to \f$ u \f$, in increasing order,
 /// \f$ \Theta( n ) \f$ time
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the columns of the edges in a row, for `SlotIterator`
 struct Present {
  /// the number of the columns
  static std::size_t size( const auto & r ) { return( r.size() ); }
  /// whether the column \f$ i \f$ holds an edge
  static bool occupied( const auto & r , std::size_t i ) {
   return( r[ i ].has_value() );
   }
  /// the node of the column \f$ i \f$
  static int read( const auto & , std::size_t i ) { return( int( i ) ); }
  };

 /// the nodes adjacent to \f$ u \f$ in increasing order, bidirectional
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] auto neighbors( int u ) const {
  using It = SlotIterator< std::vector< std::optional< W > > , Present >;
  const auto & r = f_M[ this->check( u ) ];
  return( std::ranges::subrange( It( &r , 0 ) , It( &r , r.size() ) ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the matrix is \f$ \mathrm{bound}() \times \mathrm{bound}() \f$, with `m()`
 /// entries off the diagonal between nodes, symmetric if unoriented,
 /// \f$ \Theta( n^2 ) \f$
 [[nodiscard]] bool consistent( void ) const;

 /// the row and the column of \f$ u \f$ have no entry on the diagonal nor out
 /// of the nodes, and they are equal if unoriented, \f$ O( n ) \f$
 [[nodiscard]] bool consistentAt( int u ) const;

 std::vector< std::vector< std::optional< W > > > f_M;   ///< the matrix

 };  // end( class MatrixGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/MatrixGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MatrixGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MatrixGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
