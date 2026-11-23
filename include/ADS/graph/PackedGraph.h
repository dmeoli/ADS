/*--------------------------------------------------------------------------*/
/*--------------------------- File PackedGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The dynamic graph in one compressed packed memory array, beyond the book:
 * the F-Graph of Wheatman, Burns, Buluç and Xu, a GRAPH [see `Graph.h`]
 * whose arcs are the keys of a `CompressedPackedMemoryArray` of 64 bits
 * [see `PackedMemoryArray.h`], the arc \f$ ( u , v ) \f$ being the key
 * \f$ 2^{32} u + v \f$, with no array of the nodes.
 *
 * The keys sorted are the arcs by tail and then by head, as the arrays of
 * compressed rows (CSR) hold them, so that the adjacent nodes of \f$ u \f$
 * are the keys in \f$ [ 2^{32} u , 2^{32} ( u + 1 ) ) \f$, found by one
 * search of the packed memory array and then read in order; the offsets of
 * the rows of CSR are implicit in the search, and the delta encoding of the
 * leaves stores the tail of an arc only in the first arc of a node in a
 * leaf, the others taking the bytes of the difference of their heads. An
 * unoriented edge is the two arcs \f$ ( u , v ) \f$ and \f$ ( v , u ) \f$.
 * Hence, for \f$ m \f$ arcs in \f$ N = \Theta( m ) \f$ bytes:
 *
 * - `hasEdge()` and the start of `forEachAdj()` cost the search of the
 *   packed memory array, \f$ O( \log m ) \f$, and `forEachAdj( u )` then
 *   reads \f$ O( 1 + \deg( u ) / B ) \f$ lines of cache;
 * - `insertEdge()` and `deleteEdge()` are an insertion or a removal of the
 *   packed memory array, \f$ O( \log^2 m / B + \log m ) \f$ amortized
 *   transfers, and `insertEdges()` and `deleteEdges()` its batches on an
 *   executor;
 * - `deleteNode()` removes the arcs of the node by a batch, after a visit
 *   of all of them when the graph is oriented, \f$ O( m ) \f$, as in
 *   `ListGraph`.
 *
 * Terrace of Pandey, Wheatman, Xu and Buluç keeps the adjacent nodes of a
 * node of small degree in the node, those of medium degree in a packed
 * memory array shared by them, and those of large degree in a B-tree of
 * their own; Aspen of Dhulipala, Blelloch and Shun a functional tree of the
 * nodes with a compressed tree of their arcs each. F-Graph, and this
 * header, keep the layout flat for the visits that read the arcs in order,
 * at the price of a search for each node visited in an arbitrary order.
 * The arcs carry no weight, since the keys of the packed memory array carry
 * no value (F-Graph has the same limit).
 *
 * The contracts [see `Contract.h`]: as in `ListGraph`, an edge needs two
 * distinct nodes, `insertEdge()` of an edge there and `deleteEdge()` of one
 * not there do nothing, and `deleteNode()` deletes the edges of the node;
 * the nodes are fewer than \f$ 2^{31} \f$. The invariant is that the array
 * holds only arcs between nodes, both arcs of an unoriented edge, and
 * `m()` edges. The operations are defined in `PackedGraph.tpp`.
 *
 * \see B. Wheatman, R. Burns, A. Buluç, H. Xu, CPMA: an efficient
 *      batch-parallel compressed set without pointers, Proceedings of
 *      PPoPP 2024, 348-363, 2024, doi:10.1145/3627535.3638492 (F-Graph)
 * \see B. Wheatman, H. Xu, Packed compressed sparse row: a dynamic graph
 *      representation, Proceedings of HPEC 2018, 1-7, 2018,
 *      doi:10.1109/HPEC.2018.8547566
 * \see P. Pandey, B. Wheatman, H. Xu, A. Buluç, Terrace: a hierarchical
 *      graph container for skewed dynamic graphs, Proceedings of SIGMOD
 *      2021, 1372-1385, 2021, doi:10.1145/3448016.3457313
 * \see L. Dhulipala, G. E. Blelloch, J. Shun, Low-latency graph streaming
 *      using compressed purely-functional trees, Proceedings of PLDI 2019,
 *      918-934, 2019, doi:10.1145/3314221.3314598
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

#ifndef __ADS_PackedGraph
 #define __ADS_PackedGraph
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/parallel/Executor.h"
#include "ADS/search/PackedMemoryArray.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PackedGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph whose arcs are the keys of one compressed packed memory array,
/// the labels of its nodes of type `T`
template< class T = std::monostate >
class PackedGraph : public GraphNodes< T >
{

 using B = GraphNodes< T >;

/*--------------------------------------------------------------------------*/
 public:
 using Label = T;  ///< the type of the labels

 /// the graph of the nodes \f$ 0 , \ldots , n - 1 \f$, with no edge
 /// \pre \f$ 0 \le n < 2^{31} \f$
 explicit PackedGraph( int n = 0 , bool directed = true );

 /// a new node, with no edge, and its name
 int insertNode( const T & t = T() );

 /// the edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$, if it is not there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 /// \post `hasEdge( u , v )`
 void insertEdge( int u , int v );

 /// removes the edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$, if it is there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 /// \post not `hasEdge( u , v )`
 void deleteEdge( int u , int v );

 /// removes the node \f$ u \f$ and its edges
 /// \pre \f$ u \f$ is a node
 /// \post not `hasNode( u )`
 void deleteNode( int u );

 /// whether \f$ ( u , v ) \f$ is an edge
 [[nodiscard]] bool hasEdge( int u , int v ) const;

 /// the set of the nodes adjacent to \f$ u \f$, in increasing order
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \f$ f( v ) \f$ on the nodes adjacent to \f$ u \f$, in increasing order
 ///
 /// In pseudocode (F-Graph of Wheatman, Burns, Buluç and Xu, beyond the
 /// book), \f$ A \f$ the packed memory array of the arcs:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{forEachAdj}( u , f ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{key } a \text{ of } A \text{ with }
 ///     2^{32} u \le a < 2^{32} ( u + 1 )\text{, in order}\ \textbf{do}
 ///     \cr
 /// 3 & \qquad f( a \bmod 2^{32} )\quad \triangleright\ \text{one search,}
 ///     \text{ then a scan}
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// inserts the edges \f$ A \f$ that are not there, by a batch of the
 /// packed memory array on the executor \f$ \mathit{ex} \f$; how many
 ///
 /// In pseudocode (F-Graph of Wheatman, Burns, Buluç and Xu, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertEdges}( \mathit{ex} , A ) \cr
 /// 2 & \quad K \gets \{ 2^{32} u + v : ( u , v ) \in A \} \cup
 ///     \{ 2^{32} v + u : ( u , v ) \in A \text{, if unoriented} \} \cr
 /// 3 & \quad a \gets \mathrm{insertBatch}( \mathit{ex} , K )\quad
 ///     \triangleright\ \text{the arcs new} \cr
 /// 4 & \quad m \gets m + a \text{ if oriented, else } m + a / 2;
 ///     \ \textbf{return}\ a \text{ or } a / 2
 /// \end{array}}
 /// \f]
 /// \pre each pair is of two distinct nodes
 template< Executor E >
 std::size_t insertEdges( E & ex ,
			  const std::vector< std::pair< int , int > > & A );

 /// `insertEdges()` on the calling thread
 std::size_t insertEdges( const std::vector< std::pair< int , int > > & A );

 /// removes the edges \f$ A \f$ that are there, by a batch of the packed
 /// memory array on the executor \f$ \mathit{ex} \f$; how many
 /// \pre each pair is of two distinct nodes
 template< Executor E >
 std::size_t deleteEdges( E & ex ,
			  const std::vector< std::pair< int , int > > & A );

 /// `deleteEdges()` on the calling thread
 std::size_t deleteEdges( const std::vector< std::pair< int , int > > & A );

 /// the packed memory array of the arcs
 [[nodiscard]] const CompressedPackedMemoryArray< std::uint64_t > &
 arcs( void ) const {
  return( f_arcs );
  }

 /// the bytes of the graph, the nodes and the arcs
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// the key of the arc \f$ ( u , v ) \f$, \f$ 2^{32} u + v \f$
 [[nodiscard]] static std::uint64_t key( int u , int v ) {
  return( ( std::uint64_t( u ) << 32 ) |
	   std::uint64_t( std::uint32_t( v ) ) );
  }

 /// the keys of the arcs of the edges \f$ A \f$, both of an unoriented one
 [[nodiscard]] std::vector< std::uint64_t >
 keys( const std::vector< std::pair< int , int > > & A ) const;

 /// the array holds arcs between nodes, both of an unoriented edge, and
 /// `m()` edges, \f$ O( m \log m ) \f$
 [[nodiscard]] bool consistent( void ) const;

 CompressedPackedMemoryArray< std::uint64_t > f_arcs;  ///< the arcs

 };  // end( class PackedGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/PackedGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif /* PackedGraph.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File PackedGraph.h ------------------------*/
/*--------------------------------------------------------------------------*/
