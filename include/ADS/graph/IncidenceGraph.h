/*--------------------------------------------------------------------------*/
/*-------------------------- File IncidenceGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The oriented graph on its matrix of incidence, exercise 12.2 of Morin's
 * book, beyond the book: the \f$ n \times m \f$ matrix \f$ A \f$ with
 * \f$ A_{i,j} = -1 \f$ if the node \f$ i \f$ is the source of the edge
 * \f$ j \f$, \f$ +1 \f$ if it is its target, and 0 otherwise, a GRAPH [see
 * `Graph.h`] whose nodes are \f$ 0 , \ldots , n - 1 \f$, on which the
 * visits of `Visit.h` run as on the others.
 *
 * The matrix is kept by columns, one column of \f$ n \f$ bytes per edge, so
 * that the space is \f$ n m \f$ bytes, \f$ \Theta( n m ) \f$, more than the
 * \f$ \Theta( n^2 ) \f$ of the adjacency matrix as soon as \f$ m > n \f$,
 * and the costs of the operations of Morin's interface are
 *
 * - `insertEdge( i , j )`, the `addEdge()` of the book: a new column at the
 *   end, \f$ O( n ) \f$ amortized;
 * - `hasEdge( i , j )`: the row \f$ i \f$, one entry per column, and the
 *   entry \f$ j \f$ of each column whose entry \f$ i \f$ is \f$ -1 \f$,
 *   \f$ O( m ) \f$;
 * - `deleteEdge( i , j )`, the `removeEdge()` of the book: the column found
 *   as by `hasEdge()`, and the last column copied in its place,
 *   \f$ O( m + n ) \f$;
 * - `adj( i )`, the `outEdges()` of the book: the row \f$ i \f$, and for
 *   each \f$ -1 \f$ the column searched for its \f$ +1 \f$,
 *   \f$ O( m + n \deg^+( i ) ) \f$, and `inAdj( i )`, the `inEdges()`,
 *   \f$ O( m + n \deg^-( i ) ) \f$;
 *
 * so that no operation is faster than on the adjacency matrix or on the
 * lists, and the matrix of incidence is a representation for the algebra
 * of the graph (its rank, its cycles and cuts, the flows as the solutions
 * of \f$ A x = b \f$ [see `incidence()` in `Graph.h`]), not for its
 * visits. A loop has no column (its \f$ -1 \f$ and \f$ +1 \f$ would be in
 * the same place), and two parallel edges have two equal columns.
 *
 * The contracts [see `Contract.h`]: the nodes are in
 * \f$ [ 0 , n ) \f$, an edge joins two distinct nodes; the invariant (one
 * \f$ -1 \f$ and one \f$ +1 \f$ per column) is checked on the columns that
 * an operation writes. The operations are defined in `IncidenceGraph.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 12.1 and exercise
 *      12.2
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

#ifndef __ADS_IncidenceGraph
 #define __ADS_IncidenceGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS IncidenceGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an oriented graph on its matrix of incidence, kept by columns
class IncidenceGraph {

/*--------------------------------------------------------------------------*/
 public:

 /// the graph of \f$ n \f$ nodes and no edge
 /// \pre \f$ n \ge 0 \f$
 explicit IncidenceGraph( int n = 0 );

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the names of the nodes are in \f$ [ 0 , n ) \f$
 [[nodiscard]] int bound( void ) const { return( f_n ); }

 /// the number of edges, the columns
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// always oriented
 [[nodiscard]] bool directed( void ) const { return( true ); }

 /// whether \f$ u \f$ is a node
 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < f_n ) );
  }

 /// the nodes, \f$ 0 , \ldots , n - 1 \f$
 [[nodiscard]] std::vector< int > V( void ) const;

 /// adds the edge \f$ ( i , j ) \f$ as a new column, \f$ O( n ) \f$
 /// amortized time
 /// \pre \f$ i \f$ and \f$ j \f$ are distinct nodes
 void insertEdge( int i , int j );

 /// removes the edge \f$ ( i , j ) \f$, if it is there, the last column
 /// copied in its place, \f$ O( m + n ) \f$ time
 /// \pre \f$ i \f$ and \f$ j \f$ are distinct nodes
 void deleteEdge( int i , int j );

 /// whether \f$ ( i , j ) \f$ is an edge, \f$ O( m ) \f$ time
 ///
 /// In pseudocode (exercise 12.2 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{hasEdge}( i , j ) \cr
 /// 2 & \quad \textbf{for}\ e \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ A_{i,e} = -1\ \textbf{and}\ A_{j,e} = +1
 ///     \ \textbf{then return true} \cr
 /// 4 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ and \f$ j \f$ are nodes
 [[nodiscard]] bool hasEdge( int i , int j ) const;

 /// the targets of the edges out of \f$ i \f$, in the order of the
 /// columns, \f$ O( m + n \deg^+( i ) ) \f$ time
 ///
 /// In pseudocode (exercise 12.2 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{adj}( i ) \cr
 /// 2 & \quad L \gets () \cr
 /// 3 & \quad \textbf{for}\ e \gets 0\ \textbf{to}\ m - 1
 ///     \ \textbf{with}\ A_{i,e} = -1\ \textbf{do} \cr
 /// 4 & \qquad L \gets L \cdot ( \text{the } j \text{ with } A_{j,e} = +1 )
 ///     \cr
 /// 5 & \quad \textbf{return}\ L
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] std::vector< int > adj( int i ) const;

 /// the sources of the edges into \f$ i \f$, in the order of the columns,
 /// \f$ O( m + n \deg^-( i ) ) \f$ time
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] std::vector< int > inAdj( int i ) const;

 /// \f$ f( j ) \f$ on the targets of the edges out of \f$ i \f$
 /// \pre \f$ i \f$ is a node
 template< class F >
 void forEachAdj( int i , F && f ) const;

 /// the entry \f$ A_{i,e} \f$
 /// \pre \f$ i \f$ is a node, \f$ 0 \le e < m \f$
 [[nodiscard]] int entry( int i , int e ) const;

 /// the bytes of the matrix, \f$ n m \f$
 [[nodiscard]] std::size_t bytes( void ) const { return( f_A.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the column of \f$ ( i , j ) \f$, or \f$ m \f$
 [[nodiscard]] int column( int i , int j ) const;

 /// the node of the entry \f$ v \f$ of the column \f$ e \f$
 [[nodiscard]] int find( int e , std::int8_t v ) const;

 /// one \f$ -1 \f$ and one \f$ +1 \f$ in the column \f$ e \f$
 [[nodiscard]] bool columnOk( int e ) const;

 int f_n;                         ///< the nodes
 int f_m = 0;                     ///< the edges
 std::vector< std::int8_t > f_A;  ///< the matrix, by columns

 };  // end( class IncidenceGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/IncidenceGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IncidenceGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File IncidenceGraph.h -----------------------*/
/*--------------------------------------------------------------------------*/
