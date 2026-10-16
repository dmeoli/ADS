/*--------------------------------------------------------------------------*/
/*-------------------------- File ShortestPaths.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths of chapter 11 from a node r of an oriented GRAPH whose
 * edges have a cost w( u , v ) [see Graph.h]. A feasible solution is a tree
 * T rooted in r, each node v at distance \f$ d_v \f$ from r in T; by the
 * theorem of Bellman (Teorema 11.1) T is optimal if and only if
 * \f[
 *   d_v = d_u + w( u , v ) \quad \forall ( u , v ) \in T , \qquad
 *   d_u + w( u , v ) \geq d_v \quad \forall ( u , v ) \in E .
 * \f]
 * The prototype (section 11.2) starts from d = 0 in r and \f$ + \infty \f$
 * elsewhere and, while an edge violates the condition, sets
 * \f$ d_v = d_u + w( u , v ) \f$ and makes u the parent of v; a structure S
 * holds the nodes whose edges must be checked. Its kind gives the
 * algorithm [see Frontier]:
 *
 * - LIST, a priority queue on an unsorted list: Dijkstra (1959), each node
 *   extracted once if the costs are not negative, \f$ O( n^2 ) \f$;
 * - HEAP, a priority queue on a binary heap: Johnson (1977),
 *   \f$ O( m \log n ) \f$;
 * - FIBONACCI, a priority queue on a Fibonacci heap: Fredman and Tarjan
 *   (1987), \f$ O( m + n \log n ) \f$;
 * - QUEUE: Bellman-Ford-Moore, a node extracted at most n times even with
 *   negative costs, \f$ O( n m ) \f$, and a negative cycle reachable from r
 *   is detected when a node is extracted more than n times (exercise
 *   11.2); with the other frontiers there must be no negative cycle;
 * - STACK: superpolynomial in the worst case (section 11.7);
 * - DEQUE: Pape-D'Esopo (section 11.8), a node at the tail of S the first
 *   time and at its head afterwards, superpolynomial in the worst case and
 *   the fastest in practice on road networks.
 *
 * With a priority queue a node already in S has its priority decreased
 * (the line (4) of the book); in the code of the book the new distance and
 * parent of v are written in both branches of that test. Without negative
 * cycles every frontier gives the shortest paths; with negative costs the
 * priority queues are still correct but may extract a node more than
 * once. dagShortestPaths() scans an acyclic graph in topological order,
 * \f$ O( n + m ) \f$ whatever the signs of the costs.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 * \see R. Bellman, On a routing problem, Quarterly of Applied Mathematics
 *      16(1), 87-90, 1958, doi:10.1090/qam/102435
 * \see E. W. Dijkstra, A note on two problems in connexion with graphs,
 *      Numerische Mathematik 1, 269-271, 1959, doi:10.1007/BF01386390
 * \see D. B. Johnson, Efficient algorithms for shortest paths in sparse
 *      networks, Journal of the ACM 24(1), 1-13, 1977,
 *      doi:10.1145/321992.321993
 * \see U. Pape, Implementation and efficiency of Moore-algorithms for the
 *      shortest route problem, Mathematical Programming 7, 212-222, 1974,
 *      doi:10.1007/BF01585517
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

#ifndef __ADS_ShortestPaths
 #define __ADS_ShortestPaths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"
#include "ADS/graph/Visit.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/stack/ArrayStack.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure S of the prototype
enum class Frontier { LIST , HEAP , FIBONACCI , QUEUE , STACK , DEQUE };

/// the infinite distance of the type D
template< class D >
constexpr D infinity( void )
{
 if constexpr( std::numeric_limits< D >::has_infinity )
  return( std::numeric_limits< D >::infinity() );
 else
  return( std::numeric_limits< D >::max() );
 }

/// the result: the distances, infinity() if not reachable, the tree of the
/// parents, NIL for r and the unreachable nodes, the nodes in the order of
/// extraction from S, and whether a negative cycle was found
template< class D >
struct ShortestPathTree {
 std::vector< D > d;
 std::vector< int > T;
 std::vector< int > order;
 bool negativeCycle = false;
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ PROTOTYPE ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// camminiMinimi(): the shortest paths from r with the structure how
template< Graph G >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                                      Frontier how )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 const int n = g.bound();
 ShortestPathTree< D > R;
 R.d.assign( n , INF );
 R.T.assign( n , NIL );
 std::vector< bool > b( n , false );
 std::vector< int > count( n , 0 );

 // the six structures, only one of which is used
 using LQ = ListPriorityQueue< int , D , false >;
 using HQ = HeapPriorityQueue< int , D >;
 using FQ = FibonacciHeapPriorityQueue< int , D >;
 std::unique_ptr< LQ > lq;
 std::unique_ptr< HQ > hq;
 std::unique_ptr< FQ > fq;
 std::vector< typename LQ::Handle > lh;
 std::vector< typename HQ::Handle > hh;
 std::vector< typename FQ::Handle > fh;
 ArrayQueue< int > Q( std::max( n , 1 ) );
 ArrayStack< int > St( std::max( n , 1 ) );
 Dequeue< int > Dq;
 switch( how ) {
  case( Frontier::LIST ): lq = std::make_unique< LQ >(); lh.resize( n );
                          break;
  case( Frontier::HEAP ): hq = std::make_unique< HQ >( n ); hh.resize( n );
                          break;
  case( Frontier::FIBONACCI ): fq = std::make_unique< FQ >(); fh.resize( n );
                               break;
  default: break;
  }

 auto add = [ & ]( int v , D key , bool first ) {          // (1) and (3)
  switch( how ) {
   case( Frontier::LIST ): lh[ v ] = lq->insert( v , key ); break;
   case( Frontier::HEAP ): hh[ v ] = hq->insert( v , key ); break;
   case( Frontier::FIBONACCI ): fh[ v ] = fq->insert( v , key ); break;
   case( Frontier::QUEUE ): Q.enqueue( v ); break;
   case( Frontier::STACK ): St.push( v ); break;
   case( Frontier::DEQUE ):
    if( first ) Dq.insertTail( v ); else Dq.insertHead( v );
   }
  };
 auto empty = [ & ]( void ) {
  switch( how ) {
   case( Frontier::LIST ): return( lq->isEmpty() );
   case( Frontier::HEAP ): return( hq->isEmpty() );
   case( Frontier::FIBONACCI ): return( fq->isEmpty() );
   case( Frontier::QUEUE ): return( Q.isEmpty() );
   case( Frontier::STACK ): return( St.isEmpty() );
   default: return( Dq.isEmpty() );
   }
  };
 auto extract = [ & ]( void ) {                             // (2)
  switch( how ) {
   case( Frontier::LIST ): return( lq->deleteMin() );
   case( Frontier::HEAP ): return( hq->deleteMin() );
   case( Frontier::FIBONACCI ): return( fq->deleteMin() );
   case( Frontier::QUEUE ): return( Q.dequeue() );
   case( Frontier::STACK ): return( St.pop() );
   default: return( Dq.removeHead() );
   }
  };
 auto update = [ & ]( int v , D key ) {                     // (4)
  switch( how ) {
   case( Frontier::LIST ): lq->decrease( lh[ v ] , key ); break;
   case( Frontier::HEAP ): hq->decrease( hh[ v ] , key ); break;
   case( Frontier::FIBONACCI ): fq->decrease( fh[ v ] , key ); break;
   default: break;
   }
  };

 R.d[ r ] = D( 0 );
 b[ r ] = true;
 add( r , D( 0 ) , true );
 while( ! empty() ) {
  const int u = extract();
  b[ u ] = false;
  R.order.push_back( u );
  if( ( how == Frontier::QUEUE ) && ( ++count[ u ] > n ) ) {
   // a node is extracted at most n times by a queue, unless there is a
   // negative cycle; the other frontiers have no such bound
   R.negativeCycle = true;
   return( R );
   }
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   const D nd = R.d[ u ] + w;
   if( nd < R.d[ v ] ) {
    const bool first = ( R.d[ v ] == INF );
    R.d[ v ] = nd;
    R.T[ v ] = u;
    if( ! b[ v ] ) {
     add( v , nd , first );
     b[ v ] = true;
     }
    else
     update( v , nd );
    }
   } );
  }
 return( R );
 }

/// Dijkstra: a priority queue on an unsorted list, O( n^2 )
template< Graph G >
auto dijkstra( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::LIST ) );
 }

/// Johnson: a priority queue on a binary heap, O( m log n )
template< Graph G >
auto johnson( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::HEAP ) );
 }

/// Fredman-Tarjan: a priority queue on a Fibonacci heap, O( m + n log n )
template< Graph G >
auto fredmanTarjan( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::FIBONACCI ) );
 }

/// Bellman-Ford-Moore: a queue, O( n m ), negative cycles detected
template< Graph G >
auto bellmanFordMoore( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::QUEUE ) );
 }

/// Pape-D'Esopo: a dequeue
template< Graph G >
auto papeDEsopo( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::DEQUE ) );
 }

/// the shortest paths of an acyclic graph, scanning its nodes in
/// topological order, O( n + m )
template< Graph G >
ShortestPathTree< typename G::Weight > dagShortestPaths( const G & g ,
                                                         int r )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 ShortestPathTree< D > R;
 R.d.assign( g.bound() , INF );
 R.T.assign( g.bound() , NIL );
 R.d[ r ] = D( 0 );
 for( int u : topsort( g ) ) {
  if( R.d[ u ] == INF )
   continue;
  R.order.push_back( u );
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   if( R.d[ u ] + w < R.d[ v ] ) {
    R.d[ v ] = R.d[ u ] + w;
    R.T[ v ] = u;
    }
   } );
  }
 return( R );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ShortestPaths.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ShortestPaths.h ------------------------*/
/*--------------------------------------------------------------------------*/
