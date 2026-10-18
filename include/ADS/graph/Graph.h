/*--------------------------------------------------------------------------*/
/*------------------------------ File Graph.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH of chapter 9 and its realizations. A graph G = ( V , E ) has n
 * nodes, named by integers, and m edges, ordered pairs ( u , v ) if it is
 * oriented and unordered pairs [ u , v ] otherwise; an unoriented graph is
 * the oriented one with both ( u , v ) and ( v , u ) (section 9.2), and so
 * adj( u ) gives v for each edge [ u , v ]. The edges may have a weight of
 * type W and the nodes a label of type T, both std::monostate if absent;
 * there are no loops ( u , u ), so \f$ 0 \leq m \leq n ( n - 1 ) \f$.
 *
 * The nodes of a graph are the integers in [ 0 , bound() ): insertNode()
 * gives the next one and deleteNode() leaves its name unused, so that the
 * vectors indexed by the nodes of the visits have bound() places, and n()
 * = |V()| is the G.n of the book. The concept Graph asks for V(), adj( u ),
 * which gives the adjacent nodes in a vector, as the SET of the book, and
 * forEachAdj( u , f ), which calls f on them without building it.
 *
 * incidence() gives the n x m matrix B of incidence nodes-edges (section
 * 9.3): \f$ b_{uk} = -1 \f$ if the edge k leaves u and +1 if it enters it,
 * for an oriented graph, and 1 if the edge k touches u, for an unoriented
 * one.
 *
 * adjFromIncidence() reads the adjacent nodes of u back from the matrix: in
 * the row u, each column k of an edge leaving u (or touching it, if the
 * graph is unoriented) leads to the other row with a nonzero entry.
 *
 * The contracts [see Contract.h]: a graph has n >= 0 nodes, the operations
 * on a node need one of the graph, and an edge needs two distinct nodes of
 * the graph (no loops); the invariant of the nodes is that n() counts the
 * names in use. The realizations relax some preconditions of the
 * specification, which asks for an edge not there in insertEdge(), for one
 * there in deleteEdge() and for an isolated node in deleteNode(): instead,
 * insertEdge() of an edge already there replaces its weight, deleteEdge() of
 * one not there does nothing, and deleteNode() also deletes the edges of the
 * node (PointerGraph [see PointerGraph.h] keeps the strict deleteNode()).
 * The operations are defined in Graph.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 9.1-9.4,
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

#ifndef __ADS_Graph
 #define __ADS_Graph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// GRAPH (section 9.2), as far as the visits need it: the nodes, the
/// adjacent nodes of a node, the range [ 0 , bound() ) of the names
template< class G >
concept Graph = requires( const G g , int u ) {
 { g.n() } -> std::convertible_to< int >;
 { g.bound() } -> std::convertible_to< int >;
 { g.directed() } -> std::same_as< bool >;
 { g.hasNode( u ) } -> std::same_as< bool >;
 { g.V() } -> std::same_as< std::vector< int > >;
 { g.adj( u ) } -> std::same_as< std::vector< int > >;
 g.forEachAdj( u , []( int ) {} );
 };

/// a GRAPH with the insertions and the deletions of its specification
template< class G >
concept DynamicGraph = Graph< G > && requires( G g , int u , int v ) {
 { g.insertNode() } -> std::same_as< int >;
 g.insertEdge( u , v );
 g.deleteNode( u );
 g.deleteEdge( u , v );
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS GraphNodes ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the nodes, their labels and the counts shared by the realizations
template< class T >
class GraphNodes {

/*--------------------------------------------------------------------------*/
 public:

 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes, the G.n of the book
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// the names of the nodes are in [ 0 , bound() )
 [[nodiscard]] int bound( void ) const { return( int( f_alive.size() ) ); }

 [[nodiscard]] bool hasNode( int u ) const;

 /// the set of the nodes, in increasing order
 [[nodiscard]] std::vector< int > V( void ) const;

 /// the label of the node u
 /// \pre u is a node
 [[nodiscard]] const T & label( int u ) const;

 /// \pre u is a node
 void setLabel( int u , const T & t );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the nodes and (in the derived graphs) the adjacent nodes as ranges,
 // which visit them where they are, instead of the vectors that V() and
 // adj() build

 /// the names in use, for SlotIterator
 struct Alive {
  static std::size_t size( const auto & o ) { return( o.f_alive.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_alive[ i ] );
   }
  static int read( const auto & , std::size_t i ) { return( int( i ) ); }
  };

 /// the nodes in increasing order, bidirectional
 [[nodiscard]] auto nodes( void ) const {
  using It = SlotIterator< GraphNodes , Alive >;
  return( std::ranges::subrange( It( this , 0 ) ,
                                 It( this , f_alive.size() ) ) );
  }

/*--------------------------------------------------------------------------*/
 protected:

 /// \pre n >= 0
 GraphNodes( int n , bool directed );

 int addNode( const T & t );

 /// u, which must be a node
 int check( int u ) const;

 /// u and v must be two distinct nodes
 void edge( int u , int v ) const;

 /// n() counts the names in use, and each has a label
 [[nodiscard]] bool nodesCounted( void ) const;

 bool f_directed;              ///< oriented or not
 std::vector< bool > f_alive;  ///< whether a name is a node
 std::vector< T > f_label;     ///< the labels
 int f_n = 0;                  ///< the number of nodes
 int f_m = 0;                  ///< the number of edges

 };  // end( class GraphNodes )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the matrix of incidence nodes-edges, the edges in the order of V() and
/// of adj(), an unoriented one when met from its smaller end
template< Graph G >
std::vector< std::vector< int > > incidence( const G & g );

/// the adjacent nodes of u from the matrix of incidence B of a graph,
/// oriented or not, in the order of the columns
/// \pre u is a row of B
inline std::vector< int > adjFromIncidence(
                          const std::vector< std::vector< int > > & B ,
                          int u , bool directed );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Graph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Graph.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Graph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
