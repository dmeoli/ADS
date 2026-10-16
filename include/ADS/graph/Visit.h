/*--------------------------------------------------------------------------*/
/*------------------------------ File Visit.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The visits of a GRAPH of section 9.5 and their applications, each in
 * Theta( n + m ) time on lists or vectors of adjacency:
 *
 * - bfs(), the visit in breadth with a QUEUE, which examines the nodes by
 *   increasing distance from the root r; distance() computes the distance
 *   (the minimum number of edges, the Erdos number of the example) and the
 *   tree of the shortest paths as a vector of parents, and path() reads a
 *   path from it, as printPath() does;
 * - dfs(), the visit in depth, recursive, with a previsit and a postvisit;
 * - cc(), the connected components of an unoriented graph, starting the
 *   visits from the nodes in the order in which they leave a STACK;
 * - DfsSchema, the dfs-schema() of the book, which numbers the discovery
 *   time dt[ u ] and the finish time ft[ u ] of each node and says of each
 *   edge ( u , v ) whether it is in the tree T (dt[ v ] = 0), back
 *   (dt[ u ] > dt[ v ] and ft[ v ] = 0), forward (dt[ u ] < dt[ v ] and
 *   ft[ v ] != 0) or cross (the others);
 * - hasCycle(), true if and only if a visit finds a back edge;
 * - topsort(), the nodes by decreasing finish time, a topological order of
 *   an acyclic graph: if ( u , v ) is an edge then ft[ v ] < ft[ u ];
 *   topsortKahn() builds one by removing the nodes with no entering edge;
 * - transpose() and scc(), the strongly connected components of an oriented
 *   graph by the algorithm of Kosaraju: a visit gives the nodes by
 *   decreasing finish time, and cc() on the transposed graph starts its
 *   visits in that order. By the Teorema 9.1, if there is an edge from the
 *   component C to C' then ft( C ) > ft( C' ), so the second visit meets C
 *   first and, the edge being reversed, cannot reach C' from it.
 *
 * Nil is -1 and the infinite distance is INF.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see E. F. Moore, The shortest path through a maze, Proceedings of the
 *      International Symposium on the Theory of Switching, 285-292, 1959
 * \see R. E. Tarjan, Depth-first search and linear graph algorithms, SIAM
 *      Journal on Computing 1(2), 146-160, 1972, doi:10.1137/0201010
 * \see A. B. Kahn, Topological sorting of large networks, Communications of
 *      the ACM 5(11), 558-562, 1962, doi:10.1145/368996.369025
 * \see M. Sharir, A strong-connectivity algorithm and its applications in
 *      data flow analysis, Computers & Mathematics with Applications 7(1),
 *      67-72, 1981, doi:10.1016/0898-1221(81)90008-0
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

#ifndef __ADS_Visit
 #define __ADS_Visit
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/queue/ArrayQueue.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the infinite distance
inline constexpr int INF = std::numeric_limits< int >::max();

/// no node
inline constexpr int NIL = -1;

/*--------------------------------------------------------------------------*/
/*--------------------------------- BFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// bfs(): node( u ) when u is examined, edge( u , v ) when ( u , v ) is
template< Graph G , class FN , class FE >
void bfs( const G & g , int r , FN && node , FE && edge )
{
 std::vector< bool > visited( g.bound() , false );
 ArrayQueue< int > S( std::max( g.n() , 1 ) );
 S.enqueue( r );
 visited[ r ] = true;
 while( ! S.isEmpty() ) {
  const int u = S.dequeue();
  node( u );
  g.forEachAdj( u , [ & ]( int v ) {
   edge( u , v );
   if( ! visited[ v ] ) {
    visited[ v ] = true;
    S.enqueue( v );
    }
   } );
  }
 }

/// distance(): the distances from r, INF if not reachable, and the parents
/// in the tree of the shortest paths, NIL for r and the unreachable nodes
template< Graph G >
std::pair< std::vector< int > , std::vector< int > > distance( const G & g ,
                                                                int r )
{
 std::vector< int > d( g.bound() , INF ) , p( g.bound() , NIL );
 ArrayQueue< int > S( std::max( g.n() , 1 ) );
 S.enqueue( r );
 d[ r ] = 0;
 while( ! S.isEmpty() ) {
  const int u = S.dequeue();
  g.forEachAdj( u , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ u ] + 1;
    p[ v ] = u;
    S.enqueue( v );
    }
   } );
  }
 return( std::pair( d , p ) );
 }

/// the path from r to s in the tree of the parents p, empty if there is
/// none (the printPath() of the book)
inline std::vector< int > path( int r , int s , const std::vector< int > & p )
{
 if( r == s )
  return( std::vector< int >{ s } );
 if( p[ s ] == NIL )
  return( std::vector< int >() );
 auto out = path( r , p[ s ] , p );
 if( ! out.empty() )
  out.push_back( s );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- DFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// dfs(): pre( u ) in the previsit, edge( u , v ) on each edge, post( u ) in
/// the postvisit
template< Graph G , class Pre , class Edge , class Post >
void dfs( const G & g , int u , std::vector< bool > & visited , Pre && pre ,
          Edge && edge , Post && post )
{
 visited[ u ] = true;
 pre( u );                                            // (1)
 g.forEachAdj( u , [ & ]( int v ) {
  edge( u , v );
  if( ! visited[ v ] )
   dfs( g , v , visited , pre , edge , post );
  } );
 post( u );                                           // (2)
 }

/// cc(): the component of each node, numbered from 1, the visits starting
/// from the nodes as they leave the stack S (its last item first)
template< Graph G >
std::vector< int > cc( const G & g , std::vector< int > S )
{
 std::vector< int > id( g.bound() , 0 );
 int count = 0;
 auto rec = [ & ]( auto & self , int u ) -> void {
  id[ u ] = count;
  g.forEachAdj( u , [ & ]( int v ) { if( id[ v ] == 0 ) self( self , v ); } );
  };
 while( ! S.empty() ) {
  const int u = S.back();
  S.pop_back();
  if( id[ u ] == 0 ) {
   ++count;
   rec( rec , u );
   }
  }
 return( id );
 }

/// cc() starting the visits from the nodes in increasing order
template< Graph G >
std::vector< int > cc( const G & g )
{
 auto S = g.V();
 std::reverse( S.begin() , S.end() );
 return( cc( g , S ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DfsSchema ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the kind of an edge with respect to a depth-first visit
enum class EdgeKind { TREE , BACK , FORWARD , CROSS };

/// dfs-schema(): the discovery and finish times and the kind of each edge
template< Graph G >
class DfsSchema {

/*--------------------------------------------------------------------------*/
 public:

 explicit DfsSchema( const G & g ) : f_g( g ) , f_dt( g.bound() , 0 ) ,
                                     f_ft( g.bound() , 0 ) {}

 /// the visit from u, which must not be discovered yet; edge( u , v , k )
 /// on each edge examined, of kind k
 template< class Edge >
 void visit( int u , Edge && edge ) {
  f_dt[ u ] = ++f_time;
  f_g.forEachAdj( u , [ & ]( int v ) {
   if( f_dt[ v ] == 0 ) {
    edge( u , v , EdgeKind::TREE );
    visit( v , edge );
    }
   else if( ( f_dt[ u ] > f_dt[ v ] ) && ( f_ft[ v ] == 0 ) )
    edge( u , v , EdgeKind::BACK );
   else if( ( f_dt[ u ] < f_dt[ v ] ) && ( f_ft[ v ] != 0 ) )
    edge( u , v , EdgeKind::FORWARD );
   else
    edge( u , v , EdgeKind::CROSS );
   } );
  f_ft[ u ] = ++f_time;
  }

 /// the visits from all the nodes not discovered yet, in increasing order
 template< class Edge >
 void visitAll( Edge && edge ) {
  for( int u : f_g.V() )
   if( f_dt[ u ] == 0 )
    visit( u , edge );
  }

 [[nodiscard]] int dt( int u ) const { return( f_dt[ u ] ); }

 [[nodiscard]] int ft( int u ) const { return( f_ft[ u ] ); }

/*--------------------------------------------------------------------------*/
 private:

 const G & f_g;
 std::vector< int > f_dt;   ///< the discovery times, 0 if not yet
 std::vector< int > f_ft;   ///< the finish times, 0 if not yet
 int f_time = 0;            ///< the clock

 };  // end( class DfsSchema )

/// hasCycle(): true if the oriented graph has a cycle, i.e., if a visit
/// finds a back edge
template< Graph G >
bool hasCycle( const G & g )
{
 std::vector< int > dt( g.bound() , 0 ) , ft( g.bound() , 0 );
 int time = 0;
 auto rec = [ & ]( auto & self , int u ) -> bool {
  dt[ u ] = ++time;
  bool found = false;
  g.forEachAdj( u , [ & ]( int v ) {
   if( found )
    return;
   if( dt[ v ] == 0 )
    found = self( self , v );
   else if( ( dt[ u ] > dt[ v ] ) && ( ft[ v ] == 0 ) )
    found = true;
   } );
  ft[ u ] = ++time;
  return( found );
  };
 for( int u : g.V() )
  if( ( dt[ u ] == 0 ) && rec( rec , u ) )
   return( true );
 return( false );
 }

/// topsort(): the nodes by decreasing finish time, which for an acyclic
/// graph is a topological order; the first is the top of the stack of the
/// book
template< Graph G >
std::vector< int > topsort( const G & g )
{
 std::vector< bool > visited( g.bound() , false );
 std::vector< int > S;
 auto none = []( auto... ) {};
 for( int u : g.V() )
  if( ! visited[ u ] )
   dfs( g , u , visited , none , none ,
        [ & ]( int v ) { S.push_back( v ); } );
 std::reverse( S.begin() , S.end() );
 return( S );
 }

/// the topological order by removing the nodes with no entering edge, in
/// the order of V() and then of discovery; fewer than n nodes if there is
/// a cycle
template< Graph G >
std::vector< int > topsortKahn( const G & g )
{
 std::vector< int > in( g.bound() , 0 ) , out;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ++in[ v ]; } );
 for( int u : g.V() )
  if( in[ u ] == 0 )
   out.push_back( u );
 for( std::size_t i = 0 ; i < out.size() ; ++i )
  g.forEachAdj( out[ i ] , [ & ]( int v ) {
   if( --in[ v ] == 0 )
    out.push_back( v );
   } );
 return( out );
 }

/// the transposed graph, with the edges reversed
template< Graph G >
ListGraph<> transpose( const G & g )
{
 ListGraph<> t( g.bound() , g.directed() );
 for( int u = 0 ; u < g.bound() ; ++u )
  if( ! g.hasNode( u ) )
   t.deleteNode( u );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { t.insertEdge( v , u ); } );
 return( t );
 }

/// scc(): the strongly connected component of each node, numbered from 1
template< Graph G >
std::vector< int > scc( const G & g )
{
 auto S = topsort( g );                           // first visit
 std::reverse( S.begin() , S.end() );             // its top at the back
 return( cc( transpose( g ) , S ) );              // second visit
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Visit.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Visit.h ----------------------------*/
/*--------------------------------------------------------------------------*/
