/*--------------------------------------------------------------------------*/
/*---------------------------- File Digraphs.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graphs of ADS as digraphs of LEMON and the digraphs of LEMON as
 * graphs of ADS, beyond the book (see \ref theory_graph_lemon):
 *
 * - `StaticDigraphView< W , L >`, a model of `lemon::concepts::Digraph`
 *   over a `StaticGraph< W , L >` of ADS [see `StaticGraph.h`] of either
 *   layout, with no copy of its rows: the base of
 *   `lemon::DigraphExtender`, which supplies the iterators, the maps and
 *   the notifiers, on the places of the compressed rows, the arc \f$ k \f$
 *   being the place \f$ k \f$ of `arcHeads()`, so that `OutArcIt` walks
 *   the row in memory, and `weights()` a `ReadMap` of the weights in the
 *   same places of `arcWeights()`. `source()` and
 *   `InArcIt` need what the rows lack, a tail per arc and the lists of the
 *   arcs into each node, built once in \f$ \Theta( n + a ) \f$ time and
 *   \f$ 8 a + 4 n \f$ bytes (the `StaticDigraph` of LEMON keeps
 *   16 a + 8 n). An unoriented `StaticGraph` is seen with its two arcs per
 *   edge. The algorithms of LEMON (`Dijkstra`, `Bfs`, `Dfs`, `Preflow`,
 *   `NetworkSimplex`, ...) then run on the graph of ADS by replacing the
 *   graph and the length map, and any other graph of ADS reaches them
 *   through the copy `StaticGraph( g )`. A static graph never changes, so
 *   that the notifiers tell only the `build()` of the construction, as
 *   those of the `StaticDigraph` of LEMON;
 * - `GraphView< GR , LM >`, a GRAPH of ADS [see `Graph.h`] over a digraph
 *   of LEMON and a length map, with no copy: the names of the nodes are the
 *   ids of LEMON, `bound()` is `maxNodeId() + 1`, `hasNode()` asks `valid()`
 *   where the graph has it, and `forEachEdge()` walks `OutArcIt` and reads
 *   the map, so that the algorithms of the book [see `ShortestPaths.h`,
 *   `Visit.h`] run on the graph of LEMON; the layout is that of LEMON;
 * - `toStatic( g , l )`, the copy of a digraph of LEMON into a
 *   `StaticGraph` of ADS, once, in \f$ \Theta( n + m ) \f$ time and
 *   \f$ 4 n + ( 4 + s ) m \f$ bytes for lengths of \f$ s \f$ bytes, the
 *   nodes by their ids and the loops left out, on which the parallel
 *   algorithms of ADS [see `ParallelShortestPaths.h`, `ParallelGraph.h`]
 *   take their path on the compressed rows; their results, by the ids, go
 *   back into a map of LEMON by `nodeFromId()`.
 *
 * Reading a graph of LEMON from several threads is safe while nobody
 * changes it, since its iterators and its maps only read arrays.
 *
 * The contracts [see `Contract.h`]: the graph of ADS outlives its view; the
 * weights of `weights()` exist only on a weighted `StaticGraph`. The
 * classes are defined in this file.
 *
 * \see B. Dezső, A. Jüttner, P. Kovács, LEMON - an open source C++ graph
 *      template library, Electronic Notes in Theoretical Computer Science
 *      264(5), 23-45, 2011, doi:10.1016/j.entcs.2011.06.003
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

#ifndef __ADS_interop_lemon_Digraphs
 #define __ADS_interop_lemon_Digraphs
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <lemon/core.h> )
 #error "the interoperation with LEMON needs the headers of LEMON"
#endif

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/StaticGraph.h"

#include <lemon/bits/graph_extender.h>
#include <lemon/core.h>

#include <cstddef>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::lemon_interop
{

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS StaticDigraphBase --------------------------*/
/*--------------------------------------------------------------------------*/

/// the base of `lemon::DigraphExtender` on a `StaticGraph< W , L >`: the
/// arcs are the places of its compressed rows
template< class W , ArcLayout L = ArcLayout::Separate >
class StaticDigraphBase
{

/*--------------------------------------------------------------------------*/
 public:
 /// a node, by its name in the graph of ADS
 struct Node
 {
  int id = -1;  ///< the name, -1 if invalid
  /// an invalid node
  Node( void ) = default;
  /// the node \f$ i \f$
  explicit Node( int i ) : id( i ) { }
  /// the invalid node of LEMON
  Node( ::lemon::Invalid ) { }
  /// whether the two nodes are the same
  bool operator==( Node o ) const { return( id == o.id ); }
  /// whether the two nodes differ
  bool operator!=( Node o ) const { return( id != o.id ); }
  /// the order of the ids
  bool operator<( Node o ) const { return( id < o.id ); }
  };

 /// an arc, by its place in the rows
 struct Arc
 {
  int id = -1;  ///< the place, -1 if invalid
  /// an invalid arc
  Arc( void ) = default;
  /// the arc \f$ i \f$
  explicit Arc( int i ) : id( i ) { }
  /// the invalid arc of LEMON
  Arc( ::lemon::Invalid ) { }
  /// whether the two arcs are the same
  bool operator==( Arc o ) const { return( id == o.id ); }
  /// whether the two arcs differ
  bool operator!=( Arc o ) const { return( id != o.id ); }
  /// the order of the ids
  bool operator<( Arc o ) const { return( id < o.id ); }
  };

 using NodeNumTag = ::lemon::True;  ///< `nodeNum()` in O( 1 )
 using ArcNumTag = ::lemon::True;   ///< `arcNum()` in O( 1 )

 /// the base of \f$ g \f$: the tails and the lists of the arcs into each
 /// node, in \f$ \Theta( n + a ) \f$ time
 void attach( const StaticGraph< W , L > & g );

 /// the graph of ADS
 [[nodiscard]] const StaticGraph< W , L > & graph( void ) const {
  return( *f_g );
  }

 /// the number of nodes
 [[nodiscard]] int nodeNum( void ) const { return( f_g->n() ); }
 /// the number of arcs
 [[nodiscard]] int arcNum( void ) const { return( f_g->arcs() ); }
 /// the largest id of a node
 [[nodiscard]] int maxNodeId( void ) const { return( f_g->n() - 1 ); }
 /// the largest id of an arc
 [[nodiscard]] int maxArcId( void ) const { return( f_g->arcs() - 1 ); }

 /// the id of a node
 static int id( Node u ) { return( u.id ); }
 /// the id of an arc
 static int id( Arc a ) { return( a.id ); }
 /// the node of id \f$ i \f$
 static Node nodeFromId( int i ) { return( Node( i ) ); }
 /// the arc of id \f$ i \f$
 static Arc arcFromId( int i ) { return( Arc( i ) ); }

 /// the tail of an arc
 [[nodiscard]] Node source( Arc a ) const {
  return( Node( f_tail[ std::size_t( a.id ) ] ) );
  }
 /// the head of an arc
 [[nodiscard]] Node target( Arc a ) const {
  return( Node( f_g->arcHeads()[ a.id ] ) );
  }

 /// the first node, the last name
 void first( Node & u ) const { u.id = f_g->n() - 1; }
 /// the next node
 static void next( Node & u ) { --u.id; }
 /// the first arc, the last place
 void first( Arc & a ) const { a.id = f_g->arcs() - 1; }
 /// the next arc
 static void next( Arc & a ) { --a.id; }

 /// the first arc out of \f$ u \f$, in the order of the row
 void firstOut( Arc & a , const Node & u ) const {
  const int k = f_g->offsets()[ u.id ];
  a.id = k < f_g->offsets()[ u.id + 1 ] ? k : -1;
  }
 /// the next arc out of the same node
 void nextOut( Arc & a ) const {
  const int k = a.id + 1;
  a.id = k < f_g->offsets()[ f_tail[ std::size_t( a.id ) ] + 1 ] ? k : -1;
  }
 /// the first arc into \f$ v \f$
 void firstIn( Arc & a , const Node & v ) const {
  a.id = f_firstIn[ std::size_t( v.id ) ];
  }
 /// the next arc into the same node
 void nextIn( Arc & a ) const { a.id = f_nextIn[ std::size_t( a.id ) ]; }

/*--------------------------------------------------------------------------*/
 private:
 const StaticGraph< W , L > * f_g = nullptr;  ///< the graph of ADS
 std::vector< int > f_tail;                  ///< the tail of each arc
 std::vector< int > f_nextIn;                ///< the next arc into its head
 std::vector< int > f_firstIn;               ///< the first arc into each node

 };  // end( class StaticDigraphBase )

template< class W , ArcLayout L >
void StaticDigraphBase< W , L >::attach( const StaticGraph< W , L > & g )
{
 f_g = &g;
 const int n = g.n();
 const int a = g.arcs();
 f_tail.assign( std::size_t( a ) , 0 );
 f_nextIn.assign( std::size_t( a ) , -1 );
 f_firstIn.assign( std::size_t( n ) , -1 );
 for( int u = n - 1 ; u >= 0 ; --u )
  for( int k = g.offsets()[ u + 1 ] - 1 ; k >= g.offsets()[ u ] ; --k ) {
   f_tail[ std::size_t( k ) ] = u;
   const int v = g.arcHeads()[ k ];
   f_nextIn[ std::size_t( k ) ] = f_firstIn[ std::size_t( v ) ];
   f_firstIn[ std::size_t( v ) ] = k;
   }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS StaticDigraphView --------------------------*/
/*--------------------------------------------------------------------------*/

/// a `StaticGraph< W , L >` of ADS as a digraph of LEMON, with no copy of
/// its rows
template< class W , ArcLayout L = ArcLayout::Separate >
class StaticDigraphView
    : public ::lemon::DigraphExtender< StaticDigraphBase< W , L > >
{

 using P = ::lemon::DigraphExtender< StaticDigraphBase< W , L > >;

/*--------------------------------------------------------------------------*/
 public:
 /// the view of \f$ g \f$, which must outlive it
 explicit StaticDigraphView( const StaticGraph< W , L > & g ) {
  P::attach( g );
  P::notifier( typename P::Node() ).build();  // the maps get their room
  P::notifier( typename P::Arc() ).build();
  }

 /// not copied, as the graphs of LEMON
 StaticDigraphView( const StaticDigraphView & ) = delete;
 /// not assigned
 StaticDigraphView & operator=( const StaticDigraphView & ) = delete;

 /// the weights of the arcs, a `ReadMap` of LEMON
 struct WeightMap
 {
  using Key = typename P::Arc;  ///< the arcs
  using Value = W;              ///< the weights
  const StaticDigraphView * g;  ///< the view
  /// the weight of the arc \f$ a \f$
  W operator[]( Key a ) const { return( g->graph().arcWeights()[ a.id ] ); }
  };

 /// the weights of the arcs, read in the rows of the graph of ADS
 [[nodiscard]] WeightMap weights( void ) const
  requires( StaticGraph< W , L >::weighted )
 {
  return( WeightMap{ this } );
  }

 };  // end( class StaticDigraphView )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS GraphView -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a digraph of LEMON and a length map as a GRAPH of ADS, with no copy, the
/// names of the nodes their ids
template< class GR , class LM >
class GraphView
{

/*--------------------------------------------------------------------------*/
 public:
 using Weight = typename LM::Value;  ///< the type of the weights

 /// the view of \f$ g \f$ with the lengths \f$ l \f$, which must outlive it
 GraphView( const GR & g , const LM & l ) : f_g( g ) , f_l( l ) { }

 /// the number of nodes, counted in \f$ O( n ) \f$ unless the graph knows it
 [[nodiscard]] int n( void ) const { return( ::lemon::countNodes( f_g ) ); }

 /// the names are in \f$ [ 0 , \mathrm{bound}() ) \f$, `maxNodeId() + 1`
 [[nodiscard]] int bound( void ) const { return( f_g.maxNodeId() + 1 ); }

 /// oriented, as the digraphs of LEMON
 [[nodiscard]] bool directed( void ) const { return( true ); }

 /// whether \f$ u \f$ is the id of a node
 [[nodiscard]] bool hasNode( int u ) const {
  if( ( u < 0 ) || ( u >= bound() ) )
   return( false );
  if constexpr( requires { f_g.valid( f_g.nodeFromId( u ) ); } )
   return( f_g.valid( f_g.nodeFromId( u ) ) );
  else
   return( true );
  }

 /// the ids of the nodes
 [[nodiscard]] std::vector< int > V( void ) const {
  std::vector< int > v;
  for( typename GR::NodeIt x( f_g ) ; x != ::lemon::INVALID ; ++x )
   v.push_back( f_g.id( x ) );
  return( v );
  }

 /// \f$ f( v ) \f$ for the head \f$ v \f$ of each arc out of \f$ u \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const {
  for( typename GR::OutArcIt a( f_g , f_g.nodeFromId( u ) ) ;
       a != ::lemon::INVALID ; ++a )
   f( f_g.id( f_g.target( a ) ) );
  }

 /// \f$ f( v , w ) \f$ for each arc \f$ ( u , v ) \f$ of length \f$ w \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const {
  for( typename GR::OutArcIt a( f_g , f_g.nodeFromId( u ) ) ;
       a != ::lemon::INVALID ; ++a )
   f( f_g.id( f_g.target( a ) ) , Weight( f_l[ a ] ) );
  }

 /// the heads of the arcs out of \f$ u \f$
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const {
  std::vector< int > v;
  forEachAdj( u , [ & ]( int x ) { v.push_back( x ); } );
  return( v );
  }

 /// the digraph of LEMON
 [[nodiscard]] const GR & digraph( void ) const { return( f_g ); }

/*--------------------------------------------------------------------------*/
 private:
 const GR & f_g;  ///< the digraph
 const LM & f_l;  ///< the lengths

 };  // end( class GraphView )

/*--------------------------------------------------------------------------*/
/*------------------------------- toStatic ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the copy of the digraph \f$ g \f$ of LEMON with the lengths \f$ l \f$
/// into a `StaticGraph` of ADS, the nodes by their ids and the loops left
/// out, in \f$ \Theta( n + m ) \f$ time
template< class GR , class LM >
[[nodiscard]] StaticGraph< typename LM::Value > toStatic( const GR & g ,
							  const LM & l )
{
 using W = typename LM::Value;
 std::vector< std::tuple< int , int , W > > E;
 E.reserve( std::size_t( ::lemon::countArcs( g ) ) );
 for( typename GR::ArcIt a( g ) ; a != ::lemon::INVALID ; ++a ) {
  const int u = g.id( g.source( a ) );
  const int v = g.id( g.target( a ) );
  if( u != v )
   E.emplace_back( u , v , W( l[ a ] ) );
  }
 return( StaticGraph< W >( g.maxNodeId() + 1 , E , true ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::lemon_interop )

/*--------------------------------------------------------------------------*/

#endif /* Digraphs.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Digraphs.h ---------------------------*/
/*--------------------------------------------------------------------------*/
