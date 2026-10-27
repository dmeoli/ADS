/*--------------------------------------------------------------------------*/
/*--------------------------- File GraphBuilder.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The step from a graph built one edge at a time to the static GRAPHs in
 * compressed rows, beyond the book [see `StaticGraph.h` and `ArrayGraph.h`]:
 * a `GraphBuilder` collects the nodes and the weighted edges as they come,
 * with no row to keep, and freezes them into a `StaticGraph` or an
 * `ArrayGraph` in \f$ \Theta( n + a ) \f$ time, \f$ a \f$ the number of
 * arcs, by the counting sort by tail of their constructors. The two forms
 * keep a different order of the arcs of a node: `ArrayGraph` the order in
 * which the edges were added (its counting sort by tail is stable, and an
 * unoriented edge \f$ [ u , v ] \f$ puts \f$ v \f$ in the row of \f$ u \f$
 * and \f$ u \f$ in that of \f$ v \f$ at its place in the input), while
 * `StaticGraph` sorts each row by head, the invariant on which its visits and
 * its search of an arc rest. The builder adds nothing that the constructors
 * from a list of edges do not already do but the growing of the nodes and the
 * checks at each insertion, where the error is made: a graph whose number of
 * nodes is known and whose edges are already in a vector goes to the
 * constructors directly.
 *
 * The contracts [see `Contract.h`]: the builder has n >= 0 nodes, and an
 * edge joins two distinct nodes of it (no loop, as in every GRAPH of the
 * library); parallel edges are kept. The operations are defined in
 * `GraphBuilder.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see Y. Saad, Iterative Methods for Sparse Linear Systems, 2nd ed.,
 *      SIAM, 2003, section 3.4, doi:10.1137/1.9780898718003
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

#ifndef __ADS_GraphBuilder
 #define __ADS_GraphBuilder
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/StaticGraph.h"

#include <cstddef>
#include <tuple>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS GraphBuilder ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the nodes and the edges of a graph collected one at a time, then frozen
/// into a `StaticGraph` or an `ArrayGraph`
template< class W = std::monostate >
class GraphBuilder {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights

 /// an edge \f$ ( u , v ) \f$ of weight \f$ w \f$
 using Edge = std::tuple< int , int , W >;

 /// the builder of the nodes \f$ 0 , \ldots , n - 1 \f$ and of no edge
 /// \pre \f$ n \ge 0 \f$
 explicit GraphBuilder( int n = 0 , bool directed = true );

 /// whether the graph is oriented
 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( int( f_E.size() ) ); }

 /// the edges, in the order in which they were added
 [[nodiscard]] const std::vector< Edge > & edges( void ) const {
  return( f_E );
  }

 /// room for \f$ m \f$ edges, so that the next insertions do not move them
 void reserve( std::size_t m ) { f_E.reserve( m ); }

 /// a new node, the next name
 int addNode( void ) { return( f_n++ ); }

 /// the edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$ if unoriented, of weight
 /// \f$ w \f$, after those already there, even if parallel to one of them
 /// \pre \f$ u \f$ and \f$ v \f$ are two distinct nodes, fewer than
 ///      \f$ 2^{31} - 1 \f$ edges
 void addEdge( int u , int v , const W & w = W() );

 /// the graph in compressed rows, each row sorted by head
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{toStaticGraph}() \cr
 /// 2 & \quad \textbf{return}\ \mathrm{StaticGraph}( n , E )\quad
 ///     \triangleright\ \text{two passes of counting sort, by head and by }
 ///     \text{tail}
 /// \end{array}}
 /// \f]
 [[nodiscard]] StaticGraph< W > toStaticGraph( void ) const;

 /// the graph in compressed rows, each row in the order of insertion
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{toArrayGraph}() \cr
 /// 2 & \quad \textbf{return}\ \mathrm{ArrayGraph}( n , E )\quad
 ///     \triangleright\ \text{one stable pass of counting sort by tail}
 /// \end{array}}
 /// \f]
 [[nodiscard]] ArrayGraph< W > toArrayGraph( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 bool f_directed;             ///< oriented or not
 int f_n;                     ///< the number of nodes
 std::vector< Edge > f_E;     ///< the edges, in the order of insertion

 };  // end( class GraphBuilder )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/GraphBuilder.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* GraphBuilder.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File GraphBuilder.h -------------------------*/
/*--------------------------------------------------------------------------*/
