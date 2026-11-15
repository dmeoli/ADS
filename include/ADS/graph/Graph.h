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
 * The nodes of a graph are the integers in [ 0 , `bound()` ): `insertNode()`
 * gives the next one and `deleteNode()` leaves its name unused, so that the
 * vectors indexed by the nodes of the visits have `bound()` places, and `n()`
 * = |V()| is the G.n of the book. The concept `Graph` asks for `V()`,
 * adj( u ), which gives the adjacent nodes in a vector, as the SET of the
 * book, and forEachAdj( u , f ), which calls f on them without building it.
 *
 * `incidence()` gives the n x m matrix B of incidence nodes-edges (section
 * 9.3): \f$ b_{uk} = -1 \f$ if the edge k leaves u and +1 if it enters it,
 * for an oriented graph, and 1 if the edge k touches u, for an unoriented
 * one.
 *
 * `adjFromIncidence()` reads the adjacent nodes of u back from the matrix: in
 * the row u, each column k of an edge leaving u (or touching it, if the
 * graph is unoriented) leads to the other row with a nonzero entry.
 *
 * The contracts [see `Contract.h`]: a graph has n >= 0 nodes, the operations
 * on a node need one of the graph, and an edge needs two distinct nodes of the
 * graph (no loops); the invariant of the nodes is that `n()` counts the names
 * in use. The realizations relax some preconditions of the specification,
 * which asks for an edge not there in `insertEdge()`, for one there in
 * `deleteEdge()` and for an isolated node in `deleteNode()`: instead,
 * `insertEdge()` of an edge already there replaces its weight, `deleteEdge()`
 * of one not there does nothing, and `deleteNode()` also deletes the edges of
 * the node (`PointerGraph` [see `PointerGraph.h`] keeps the strict
 * `deleteNode()`). The operations are defined in `Graph.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 9.1-9.4,
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
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// GRAPH (section 9.2), as far as the visits need it: the nodes, the adjacent
/// nodes of a node, the range \f$ [ 0 , \mathrm{bound}() ) \f$ of the names
template< class G >
concept Graph = requires( const G g , int u )
{
 { g.n() } -> std::convertible_to< int >;
 { g.bound() } -> std::convertible_to< int >;
 { g.directed() } -> std::same_as< bool >;
 { g.hasNode( u ) } -> std::same_as< bool >;
 { g.V() } -> std::same_as< std::vector< int > >;
 { g.adj( u ) } -> std::same_as< std::vector< int > >;
 g.forEachAdj( u , []( int ) { } );
 };

/// a GRAPH with the insertions and the deletions of its specification
template< class G >
concept DynamicGraph = Graph< G > && requires( G g , int u , int v )
{
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
class GraphNodes
{

/*--------------------------------------------------------------------------*/
 public:
 /// whether the graph is oriented
 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes, the G.n of the book
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// the names of the nodes are in \f$ [ 0 , \mathrm{bound}() ) \f$
 [[nodiscard]] int bound( void ) const { return( int( f_alive.size() ) ); }

 /// whether \f$ u \f$ is a node
 [[nodiscard]] bool hasNode( int u ) const;

 /// the set of the nodes, in increasing order
 [[nodiscard]] std::vector< int > V( void ) const;

 /// the label of the node \f$ u \f$
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] const T & label( int u ) const;

 /// the label of the node \f$ u \f$ becomes \f$ t \f$
 /// \pre \f$ u \f$ is a node
 void setLabel( int u , const T & t );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the nodes and (in the derived graphs) the adjacent nodes as ranges,
 // which visit them where they are, instead of the vectors that V() and
 // adj() build

 /// the names in use, for `SlotIterator`
 struct Alive
 {
  /// the number of the names
  static std::size_t size( const auto & o ) { return( o.f_alive.size() ); }
  /// whether the name \f$ i \f$ is in use
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_alive[ i ] );
   }
  /// the node of the name \f$ i \f$
  static int read( const auto & , std::size_t i ) { return( int( i ) ); }
  };

 /// the nodes in increasing order, bidirectional
 [[nodiscard]] auto nodes( void ) const {
  using It = SlotIterator< GraphNodes , Alive >;
  return(
    std::ranges::subrange( It( this , 0 ) , It( this , f_alive.size() ) ) );
  }

/*--------------------------------------------------------------------------*/
 protected:
 /// \pre \f$ n \ge 0 \f$
 GraphNodes( int n , bool directed );

 /// a new node of label \f$ t \f$, and its name
 int addNode( const T & t );

 /// \f$ u \f$, which must be a node
 int check( int u ) const;

 /// \f$ u \f$ and \f$ v \f$ must be two distinct nodes
 void edge( int u , int v ) const;

 /// `n()` counts the names in use, and each has a label
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

/// the matrix of incidence nodes-edges, the edges in the order of `V()` and
/// of `adj()`, an unoriented one when met from its smaller end
///
/// In pseudocode (section 9.3):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{incidence}( G )\quad \triangleright
///     \ \text{a column for each edge} \cr
/// 2 & \quad B \gets \text{an } n \times m \text{ matrix of zeros};\ k \gets
///     0 \cr
/// 3 & \quad \textbf{foreach}\ \text{edge } ( u , v )
///     \text{, each unoriented one once with } u < v\ \textbf{do} \cr
/// 4 & \qquad ( B[ u ][ k ] , B[ v ][ k ] ) \gets ( -1 , +1 ) \text{ if } G
///     \text{ is oriented, else } ( 1 , 1 );\ k \gets k + 1 \cr
/// 5 & \quad \textbf{return}\ B
/// \end{array}}
/// \f]
template< Graph G >
std::vector< std::vector< int > > incidence( const G & g );

/// the adjacent nodes of \f$ u \f$ from the matrix of incidence \f$ B \f$ of a
/// graph, oriented or not, in the order of the columns
///
/// In pseudocode (section 9.3):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{adjFromIncidence}( B , u ) \cr
/// 2 & \quad A \gets ()\quad \triangleright\ \text{the edges that leave } u
///     \text{ have } -1 \text{ in its row, } 1 \text{ if unoriented} \cr
/// 3 & \quad \textbf{foreach}\ \text{column } k \text{ of such an edge}
///     \ \textbf{do}\ \text{append to } A \text{ the row } v \ne u
///     \text{ with } B[ v ][ k ] \ne 0 \cr
/// 4 & \quad \textbf{return}\ A
/// \end{array}}
/// \f]
/// \pre \f$ u \f$ is a row of \f$ B \f$
inline std::vector< int >
adjFromIncidence( const std::vector< std::vector< int > > & B , int u ,
		  bool directed );

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Rows --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

/// the type of the arcs of the rows of a graph read where they are
template< class G , bool Direct >
struct RowsArc
{
 using type = int;  ///< none: the copy has the heads only
 };

/// the type of the arcs of `arcData()`
template< class G >
struct RowsArc< G , true >
{
 /// an arc of the graph, its head or a pair of head and weight
 using type =
   std::remove_cvref_t< decltype( *std::declval< const G & >().arcData() ) >;
 };

/// the arcs of a graph in compressed rows, for the algorithms that walk the
/// adjacencies with a cursor per node: those of a graph that keeps them so
/// (`offsets()` and `arcData()`, as `StaticGraph`) where they are, those of
/// any other model copied once, in the order of `forEachAdj()`, with none
/// for a name that is not a node
template< Graph G >
class Rows
{

 public:
 /// whether the rows of the graph are read where they are
 static constexpr bool direct = requires( const G & h ) {
  { h.offsets() } -> std::convertible_to< const int * >;
  h.arcData();
  };

 /// the rows of \f$ g \f$, which must outlive them if `direct`
 explicit Rows( const G & g ) {
  if constexpr( direct ) {
   f_o = g.offsets();
   f_a = g.arcData();
   }
  else {
   const int n = g.bound();
   f_off.assign( std::size_t( n ) + 1 , 0 );
   for( int u = 0 ; u < n ; ++u ) {
    f_off[ u ] = f_head.size();
    if( g.hasNode( u ) )
     g.forEachAdj( u , [ & ]( int v ) { f_head.push_back( v ); } );
    }
   f_off[ n ] = f_head.size();
   }
  }

 /// the place of the first arc leaving \f$ u \f$
 [[nodiscard]] std::size_t first( int u ) const {
  if constexpr( direct )
   return( std::size_t( f_o[ u ] ) );
  else
   return( f_off[ u ] );
  }

 /// the place after the last arc leaving \f$ u \f$
 [[nodiscard]] std::size_t last( int u ) const { return( first( u + 1 ) ); }

 /// the head of the arc in the place \f$ i \f$
 [[nodiscard]] int head( std::size_t i ) const {
  if constexpr( ! direct )
   return( f_head[ i ] );
  else if constexpr( std::is_convertible_v< decltype( f_a[ i ] ) , int > )
   return( int( f_a[ i ] ) );
  else
   return( f_a[ i ].to );
  }

 private:
 using Arc = typename RowsArc< G , direct >::type;  ///< an arc of g

 const int * f_o = nullptr;         ///< the offsets of the graph
 const Arc * f_a = nullptr;         ///< its arcs
 std::vector< std::size_t > f_off;  ///< the offsets of the copy
 std::vector< int > f_head;         ///< the heads of the copy

 };  // end( class Rows )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Graph.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Graph.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Graph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
