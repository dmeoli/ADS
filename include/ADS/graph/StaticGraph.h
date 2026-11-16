/*--------------------------------------------------------------------------*/
/*--------------------------- File StaticGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A static GRAPH for the engineered algorithms, beyond the book [see
 * `Graph.h`]: the compressed sparse rows of `ArrayGraph` [see `ArrayGraph.h`]
 * (the format CSR of the sparse matrices), with the arcs of each node sorted
 * by head, which makes the reads of the visits that follow them go forward
 * in memory. The graph is three plain arrays, as a structure of arrays: the
 * offsets of the rows, the heads of the arcs and, if the graph is weighted,
 * their weights in the same places, so that a visit that needs only the
 * heads reads none of the weights, and the offsets and the heads are the
 * arrays `xadj` and `adjncy` of METIS and the arrays `Ap` and `Aj` of the
 * compressed rows of GraphBLAS, with the weights as `adjwgt` and `Ax`, which
 * those libraries take without a copy. With \f$ a = m \f$ arcs if oriented
 * and \f$ a = 2 m \f$ if not, the arcs of u are the places
 * \f[
 *   \mathit{off}( u ) \leq k < \mathit{off}( u + 1 ) , \qquad
 *   \mathit{off}( 0 ) = 0 , \quad \mathit{off}( n ) = a ,
 * \f]
 * so that the graph takes \f$ 4 ( n + 1 ) + ( 4 + s ) a \f$ bytes, s the
 * size of a weight (0 if unweighted), with no padding between a head and a
 * weight wider than it. The graph is built from a list of
 * edges in \f$ \Theta( n + a ) \f$ time by two passes of counting sort, by
 * head and then, stably, by tail, and an unoriented edge [ u , v ] gives
 * the two arcs ( u , v ) and ( v , u ). There may be parallel arcs.
 *
 * An arc is still read as a whole, a head with its weight: `out( u )` is the
 * row of u as a range of random access of `WeightedArc` values, made from
 * the two arrays as it is read, and `arcData()` is the cursor at the first
 * arc, which takes the indexing and the arithmetic of a pointer; on an
 * unweighted graph they are a `std::span` and a pointer of the heads.
 *
 * The layout of a weighted graph is the parameter \f$ L \f$: the two
 * arrays above (`ArcLayout::Separate`, the default), or one array of the
 * pairs `WeightedArc` (`ArcLayout::Interleaved`), in which a head and its
 * weight share a line of the cache, at the price of the padding of a
 * weight wider than a head, so that \f$ 4 ( n + 1 ) + s' a \f$ bytes are
 * taken, \f$ s' \f$ the size of a pair. The algorithms that read each head
 * with its weight (Dijkstra, the stepping algorithms) miss the cache less
 * often on the pairs when the weights are integers, those that read only
 * the heads (the visits) on the two arrays. Both layouts give the same
 * interface: `out( u )` is a range of random access of `WeightedArc`, a
 * `std::span` of the pairs if interleaved, `arcData()` its cursor, and
 * `arcHeads()` and `arcWeights()` read the heads and the weights in place
 * by the indexing of an array; only the separate layout has `heads()` and
 * `weightData()`, the plain arrays that the concept `CsrArrays` requires.
 *
 * `transpose()` gives the graph with the arcs reversed, which the visits
 * from the bottom up and the strongly connected components of a reversed
 * graph need, and `relabel()` gives the same graph with the nodes renamed,
 * e.g., by `bfsOrder()`, the order of a visit in breadth from a node and then
 * from each node not reached (the ordering of Cuthill and McKee without its
 * sort by degree), which puts close together the nodes that the algorithms
 * visit close in time; both take \f$ \Theta( n + a ) \f$. `StaticGraph`
 * models the concept `Graph`, so that the algorithms of the book also apply
 * to it.
 *
 * The contracts [see `Contract.h`]: the graph has n >= 0 nodes and fewer
 * than \f$ 2^{31} \f$ arcs, its edges join two distinct nodes; the
 * operations on a node need a node, `relabel()` a permutation of the nodes;
 * the invariant is that the offsets do not decrease and end at a, and that
 * the heads are nodes, in increasing order for each tail. The operations
 * are defined in `StaticGraph.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see Y. Saad, Iterative Methods for Sparse Linear Systems, 2nd ed.,
 *      SIAM, 2003, section 3.4, doi:10.1137/1.9780898718003
 * \see E. Cuthill, J. McKee, Reducing the bandwidth of sparse symmetric
 *      matrices, Proceedings of the 24th ACM National Conference, 157-172,
 *      1969, doi:10.1145/800195.805928
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

#ifndef __ADS_StaticGraph
 #define __ADS_StaticGraph
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/external/MappedFile.h"
#include "ADS/graph/Graph.h"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
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
/*---------------------------- CLASS StaticGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an arc of a weighted `StaticGraph`, its head with its weight
template< class W >
struct WeightedArc
{
 int to;  ///< the head
 W w;     ///< the weight
 };

/// the layout of the arcs of a weighted `StaticGraph`
enum class ArcLayout {
 Separate ,    ///< the heads and the weights in two arrays of the same places
 Interleaved  ///< the pairs `WeightedArc` in one array
 };

namespace detail
{

/// a member of the pairs of an interleaved `StaticGraph` read in place as
/// an array: the indexing and the arithmetic of a pointer to it
template< class W , class T , T WeightedArc< W >::* M >
class ArcMember
{
 public:
 ArcMember( void ) = default;

 /// the member of the pairs from \f$ a \f$ on
 explicit ArcMember( const WeightedArc< W > * a ) : f_a( a ) { }

 /// the member of the pair \f$ k \f$ places further
 const T & operator[]( std::ptrdiff_t k ) const { return( f_a[ k ].*M ); }
 /// the member of the pair here
 const T & operator*( void ) const { return( ( *f_a ).*M ); }
 /// the member of the pairs \f$ k \f$ places further on
 friend ArcMember operator+( ArcMember p , std::ptrdiff_t k ) {
  p.f_a += k;
  return( p );
  }
 friend bool operator==( const ArcMember & , const ArcMember & ) = default;

 private:
 const WeightedArc< W > * f_a = nullptr;  ///< the pair here
 };  // end( class ArcMember )

/// the arcs of a weighted `StaticGraph` from a place on, read from the array
/// of the heads and from that of the weights: a random access iterator whose
/// reference is the `WeightedArc` value, which also takes the indexing and
/// the arithmetic of a pointer to the arcs
template< class W >
class WeightedArcCursor
{
 public:
 using iterator_concept = std::random_access_iterator_tag;
 using iterator_category = std::input_iterator_tag;
 using value_type = WeightedArc< W >;
 using difference_type = std::ptrdiff_t;
 using reference = WeightedArc< W >;

 /// the arc read, for the arrow
 struct Pointer
 {
  WeightedArc< W > a;  ///< the arc
  const WeightedArc< W > * operator->( void ) const { return( &a ); }
  };
 using pointer = Pointer;

 WeightedArcCursor( void ) = default;

 /// at the place of the head \f$ h \f$ and of the weight \f$ w \f$
 WeightedArcCursor( const int * h , const W * w ) : f_h( h ) , f_w( w ) { }

 /// the arc here
 reference operator*( void ) const { return( reference{ *f_h , *f_w } ); }
 /// the arc here, for the arrow
 pointer operator->( void ) const { return( Pointer{ **this } ); }
 /// the arc \f$ k \f$ places further
 reference operator[]( difference_type k ) const {
  return( reference{ f_h[ k ] , f_w[ k ] } );
  }

 WeightedArcCursor & operator++( void ) {
  ++f_h;
  ++f_w;
  return( *this );
  }
 WeightedArcCursor operator++( int ) {
  WeightedArcCursor c = *this;
  ++*this;
  return( c );
  }
 WeightedArcCursor & operator--( void ) {
  --f_h;
  --f_w;
  return( *this );
  }
 WeightedArcCursor operator--( int ) {
  WeightedArcCursor c = *this;
  --*this;
  return( c );
  }
 WeightedArcCursor & operator+=( difference_type k ) {
  f_h += k;
  f_w += k;
  return( *this );
  }
 WeightedArcCursor & operator-=( difference_type k ) {
  f_h -= k;
  f_w -= k;
  return( *this );
  }
 friend WeightedArcCursor operator+( WeightedArcCursor c ,
				     difference_type k ) {
  return( c += k );
  }
 friend WeightedArcCursor operator+( difference_type k ,
				     WeightedArcCursor c ) {
  return( c += k );
  }
 friend WeightedArcCursor operator-( WeightedArcCursor c ,
				     difference_type k ) {
  return( c -= k );
  }
 friend difference_type operator-( const WeightedArcCursor & a ,
				   const WeightedArcCursor & b ) {
  return( a.f_h - b.f_h );
  }
 friend bool operator==( const WeightedArcCursor & a ,
			 const WeightedArcCursor & b ) {
  return( a.f_h == b.f_h );
  }
 friend auto operator<=>( const WeightedArcCursor & a ,
			  const WeightedArcCursor & b ) {
  return( a.f_h <=> b.f_h );
  }

 /// the head here
 [[nodiscard]] const int * heads( void ) const { return( f_h ); }
 /// the weight here
 [[nodiscard]] const W * weights( void ) const { return( f_w ); }

 private:
 const int * f_h = nullptr;  ///< the head of the place
 const W * f_w = nullptr;    ///< its weight
 };  // end( class WeightedArcCursor )

}  // end( namespace detail )

/// a static graph in compressed sparse rows, the arcs of each node sorted
/// by head, their heads and their weights in two arrays or, with
/// `ArcLayout::Interleaved`, in one array of pairs
template< class W = std::monostate , ArcLayout L = ArcLayout::Separate >
class StaticGraph
{

/*--------------------------------------------------------------------------*/
 public:
 using Weight = W;  ///< the type of the weights

 /// whether the arcs have a weight
 static constexpr bool weighted = ! std::is_same_v< W , std::monostate >;

 /// the layout of the arcs
 static constexpr ArcLayout layout = L;

 /// whether the arcs are the pairs of one array (never if unweighted)
 static constexpr bool interleaved =
   weighted && ( L == ArcLayout::Interleaved );

 /// an arc: its head, and its weight if the graph is weighted
 using Arc = std::conditional_t< weighted , WeightedArc< W > , int >;

 /// the cursor of the arcs: a pointer to the heads if the graph is
 /// unweighted, to the pairs if interleaved
 using ArcCursor = std::conditional_t<
   ! weighted , const int * ,
   std::conditional_t< interleaved , const WeightedArc< W > * ,
		       detail::WeightedArcCursor< W > > >;

 /// the arcs of a row, random access
 using ArcRange = std::conditional_t<
   ! weighted , std::span< const int > ,
   std::conditional_t< interleaved , std::span< const WeightedArc< W > > ,
		       std::ranges::subrange< ArcCursor > > >;

 /// the heads of the arcs read in place by their places: a pointer, or
 /// the heads of the pairs if interleaved
 using HeadArray =
   std::conditional_t< interleaved ,
		       detail::ArcMember< W , int , &WeightedArc< W >::to > ,
		       const int * >;

 /// the weights of the arcs read in place by their places
 using WeightArray = std::conditional_t<
   interleaved , detail::ArcMember< W , W , &WeightedArc< W >::w > ,
     const W * >;

 /// the graph with no node
 StaticGraph( void ) : f_off( 1 , 0 ) { }

 /// the graph of the nodes \f$ 0 , \ldots , n - 1 \f$ and of the given edges;
 /// an unoriented edge \f$ [ u , v ] \f$ is given once
 ///
 /// In pseudocode (two passes of counting sort, the second stable, so that the
 /// heads of each row come sorted):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( n , E ) \cr
 /// 2 & \quad A \gets \text{the arcs of } E \text{ , both } ( u , v ) \text{ }
 ///     \text{and } ( v , u ) \text{ if unoriented} \cr
 /// 3 & \quad A \gets \text{the arcs of } A \text{ sorted by head, by }
 ///     \text{counting} \cr
 /// 4 & \quad \textbf{foreach}\ ( u , v ) \in A\ \textbf{do}\ \mathit{off}[ u
 ///     + 1 ] \gets \mathit{off}[ u + 1 ] + 1 \cr
 /// 5 & \quad \textbf{for}\ u \gets 1\ \textbf{to}\ n\ \textbf{do}
 ///     \ \mathit{off}[ u ] \gets \mathit{off}[ u ] + \mathit{off}[ u - 1 ]
 ///     \cr
 /// 6 & \quad \textbf{foreach}\ ( u , v , w ) \in A \text{ in order }
 ///     \ \textbf{do}\ p \gets \mathit{next}[ u ];\ \mathit{next}[ u ]
 ///     \gets p + 1\quad \triangleright\ \mathit{next}[ u ] \text{ starts }
 ///     \text{at } \mathit{off}[ u ] \cr
 /// 7 & \qquad \mathit{head}[ p ] \gets v;\ \mathit{weight}[ p ] \gets w
 ///     \cr
 /// 8 & \quad \textbf{return}\ ( \mathit{off} , \mathit{head} ,
 ///     \mathit{weight} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 0 \f$, each edge joins two distinct nodes, fewer than
 ///      \f$ 2^{31} \f$ arcs
 StaticGraph( int n , const std::vector< std::tuple< int , int , W > > & E ,
	      bool directed = true );

 /// the unweighted graph of the nodes \f$ 0 , \ldots , n - 1 \f$ and of the
 /// edges \f$ E \f$
 /// \pre \f$ n \ge 0 \f$, each edge joins two distinct nodes, fewer than
 ///      \f$ 2^{31} \f$ arcs
 StaticGraph( int n , const std::vector< std::pair< int , int > > & E ,
	      bool directed = true )
  requires( ! weighted );

 /// the copy of any graph, the names that are not nodes of \f$ g \f$ becoming
 /// isolated nodes
 template< Graph G >
 explicit StaticGraph( const G & g );

 /// whether the graph is oriented
 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( int( f_off.size() ) - 1 ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// the number of arcs, 2 `m()` if the graph is unoriented
 [[nodiscard]] int arcs( void ) const {
  return( int( interleaved ? f_arc.size() : f_head.size() ) );
  }

 [[nodiscard]] int bound( void ) const { return( n() ); }

 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < n() ) );
  }

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] int degree( int u ) const;

 [[nodiscard]] std::vector< int > V( void ) const;

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const;

 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const;

 /// \f$ f( v , w ) \f$ on each arc \f$ ( u , v ) \f$ of weight \f$ w \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const
  requires( weighted );

 /// the weight of the first arc \f$ ( u , v ) \f$, nothing if there is
 /// none, by a binary search of the row of \f$ u \f$, in
 /// \f$ O( \log |\mathrm{adj}( u )| ) \f$
 [[nodiscard]] std::optional< W > weight( int u , int v ) const
  requires( weighted );

 /// the weight of the first arc \f$ ( u , v ) \f$ set to \f$ w \f$, and
 /// that of the first arc \f$ ( v , u ) \f$ if the graph is unoriented,
 /// in \f$ O( \log |\mathrm{adj}( u )| + \log |\mathrm{adj}( v )| ) \f$;
 /// false if there is no such arc
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes
 bool setWeight( int u , int v , const W & w )
  requires( weighted );

 /// the arcs out of \f$ u \f$, contiguous, random access: on a weighted
 /// graph the `WeightedArc` values made from `heads()` and `weightData()`,
 /// or the pairs if interleaved
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] ArcRange out( int u ) const;

 /// the offsets of the rows: the arcs of u are those in the places
 /// [ `offsets()`[ u ] , `offsets()`[ u + 1 ] ) of `arcData()`, of
 /// `heads()` and of `weightData()`, \f$ n + 1 \f$ of them
 [[nodiscard]] const int * offsets( void ) const { return( f_off.data() ); }

 /// the heads of all the arcs, row by row, `arcs()` of them, an array of
 /// the separate layout
 [[nodiscard]] const int * heads( void ) const
  requires( ! interleaved )
 {
  return( f_head.data() );
  }

 /// the weights of all the arcs, in the places of `heads()`, an array of
 /// the separate layout
 [[nodiscard]] const W * weightData( void ) const
  requires( weighted && ( ! interleaved ) )
 {
  return( f_w.data() );
  }

 /// the heads of all the arcs read in place, in either layout: `heads()`,
 /// or the heads of the pairs
 [[nodiscard]] HeadArray arcHeads( void ) const {
  if constexpr( interleaved )
   return( HeadArray( f_arc.data() ) );
  else
   return( f_head.data() );
  }

 /// the weights of all the arcs read in place, in either layout
 [[nodiscard]] WeightArray arcWeights( void ) const
  requires( weighted )
 {
  if constexpr( interleaved )
   return( WeightArray( f_arc.data() ) );
  else
   return( f_w.data() );
  }

 /// all the arcs, row by row: the cursor at the first, whose place
 /// \f$ k \f$ is the arc \f$ k \f$ (the pointer `heads()` if the graph is
 /// unweighted, the pointer to the pairs if interleaved)
 [[nodiscard]] ArcCursor arcData( void ) const {
  if constexpr( interleaved )
   return( f_arc.data() );
  else if constexpr( weighted )
   return( ArcCursor( f_head.data() , f_w.data() ) );
  else
   return( f_head.data() );
  }

 /// the head of an arc
 static int head( const Arc & a ) {
  if constexpr( weighted )
   return( a.to );
  else
   return( a );
  }

 /// the graph with each arc reversed (the same graph if unoriented)
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{transpose}() \cr
 /// 2 & \quad \textbf{return}\ \mathrm{build}( n , \text{the arcs } ( v , u )
 ///     \text{ for each arc } ( u , v ) )
 /// \end{array}}
 /// \f]
 [[nodiscard]] StaticGraph transpose( void ) const;

 /// the same graph with the node \f$ u \f$ renamed \f$ p[ u ] \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{relabel}( p ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{build}( n , \text{the arcs } ( p[ u ] ,
 ///     p[ v ] ) \text{ for each arc } ( u , v ) )
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ is a permutation of the nodes
 [[nodiscard]] StaticGraph relabel( const std::vector< int > & p ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the nodes, \f$ 0 , \ldots , n - 1 \f$, random access
 [[nodiscard]] auto nodes( void ) const {
  return( std::views::iota( 0 , n() ) );
  }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "StaticGraph";

 /// the type of the weights, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  if constexpr( weighted )
   return( detail::persistMix(
     { sizeof( W ) , std::is_floating_point_v< W > ,
       std::is_signed_v< W > } ) );
  else
   return( 0 );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

 private:
 /// the rows of the arcs \f$ ( u , \mathit{head} , \mathit{weight} ) \f$ of
 /// \f$ E \f$, both ways if unoriented, by counting sort on the head and then
 /// on the tail
 template< class Edges , class Get >
 void build( int n , const Edges & E , Get && get );

 /// the place of the first arc \f$ ( u , v ) \f$, \f$ -1 \f$ if there is
 /// none
 [[nodiscard]] int find( int u , int v ) const;

 /// the weight of the arc of the place \f$ k \f$, to be written
 [[nodiscard]] W & weightAt( int k )
  requires( weighted )
 {
  if constexpr( interleaved )
   return( f_arc[ std::size_t( k ) ].w );
  else
   return( f_w[ std::size_t( k ) ] );
  }

 /// the offsets do not decrease and end at the number of arcs, whose heads
 /// are nodes, increasing in each row, with a weight each if weighted
 [[nodiscard]] bool consistent( void ) const;

 bool f_directed = true;  ///< oriented or not
 /// the arcs of u from f_off[ u ]
 std::vector< int , MappableAllocator< int > > f_off;
 /// the heads of the arcs, row by row, empty if interleaved
 std::vector< int , MappableAllocator< int > > f_head;
 /// their weights, empty if unweighted or interleaved
 std::vector< W , MappableAllocator< W > > f_w;
 /// the pairs of the arcs, row by row, empty unless interleaved
 std::vector< WeightedArc< W > > f_arc;
 int f_m = 0;  ///< the number of edges

 };  // end( class StaticGraph )

/*--------------------------------------------------------------------------*/
/*------------------------------- CONCEPTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph whose compressed rows are plain arrays, `offsets()`, `heads()`
/// and, if weighted, `weightData()`, which a library that takes the
/// compressed rows (METIS, KaHIP, GraphBLAS) reads without a copy: a
/// `StaticGraph` of the separate layout, or an `ArrayGraph`
template< class G >
concept CsrArrays = requires( const G & g )
{
 { g.offsets() } -> std::same_as< const int * >;
 { g.heads() } -> std::same_as< const int * >;
 } && ( ( ! requires { requires G::weighted; } ) || requires( const G & g )
{
		     {
		      g.weightData()
		      } -> std::same_as< const typename G::Weight * >;
		     } );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the order of a visit in breadth from \f$ r \f$, then from the least node
/// not reached, and so on: the name p[ u ] of each node u in the relabelled
/// graph, a permutation of the nodes
///
/// In pseudocode (the ordering of Cuthill and McKee without its sort by
/// degree):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bfsOrder}( G , r ) \cr
/// 2 & \quad k \gets 0;\ p[ v ] \gets \textbf{nil}\  \text{for each node} \cr
/// 3 & \quad \textbf{foreach}\ s \in ( r , 0 , 1 , \ldots , n - 1 )
///     \ \textbf{with}\ p[ s ] = \textbf{nil}\ \textbf{do} \cr
/// 4 & \qquad p[ s ] \gets k;\ k \gets k + 1;\ Q \gets ( s ) \cr
/// 5 & \qquad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad\quad u \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad\quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}
///     \ p[ v ] = \textbf{nil}\ \textbf{do}\ p[ v ] \gets k;\ k \gets k + 1;
///     \ \mathrm{enqueue}( Q , v ) \cr
/// 8 & \quad \textbf{return}\ p
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node, or the graph has none
template< class W , ArcLayout L >
std::vector< int > bfsOrder( const StaticGraph< W , L > & g , int r = 0 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/StaticGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif /* StaticGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File StaticGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
