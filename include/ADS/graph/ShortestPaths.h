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
 * parent of v are written in both branches of that test. The structure S
 * is a type [see PrototypeFrontier]: shortestPaths() with a Frontier runs
 * the prototype on the six structures of the book, and the prototype runs
 * as well on any priority queue of section 10.1 [see PriorityFrontier],
 * queue, stack or deque of another library; with the priorities
 * \f$ ( d_v , v ) \f$ the ties are broken by the names of the nodes, and
 * every priority queue extracts the same nodes in the same order, so that
 * the counts of a run [see FrontierCounts] measure the same work on any
 * of them. Without negative
 * cycles every frontier gives the shortest paths; with negative costs the
 * priority queues are still correct but may extract a node more than
 * once. dagShortestPaths() scans an acyclic graph in topological order,
 * \f$ O( n + m ) \f$ whatever the signs of the costs. The distances are
 * sums in D, not checked for overflow: the cost of every path from r must
 * stay strictly between -infinity< D >() and infinity< D >(), and a node
 * at distance infinity< D >() counts as unreachable.
 *
 * The contracts [see Contract.h]: r is a node; with a frontier other than
 * QUEUE no negative cycle is reachable from r, and dagShortestPaths() needs
 * an acyclic graph, both checked at the level of the audits (the first by
 * Bellman-Ford, \f$ O( n m ) \f$); the tree found satisfies the conditions of
 * Bellman unless a negative cycle was found, which isShortestPathTree()
 * checks. The functions are defined in ShortestPaths.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 * \see R. Bellman, On a routing problem, Quarterly of Applied Mathematics
 *      16(1), 87-90, 1958, doi:10.1090/qam/102435
 * \see E. W. Dijkstra, A note on two problems in connexion with graphs,
 *      Numerische Mathematik 1, 269-271, 1959, doi:10.1007/BF01386390
 * \see M. L. Fredman, R. E. Tarjan, Fibonacci heaps and their uses in
 *      improved network optimization algorithms, Journal of the ACM 34(3),
 *      596-615, 1987, doi:10.1145/28869.28874
 * \see L. R. Ford Jr., Network flow theory, RAND Corporation paper P-923,
 *      1956
 * \see E. F. Moore, The shortest path through a maze, Proceedings of an
 *      International Symposium on the Theory of Switching, Part II,
 *      Harvard University Press, 285-292, 1959
 * \see D. B. Johnson, Efficient algorithms for shortest paths in sparse
 *      networks, Journal of the ACM 24(1), 1-13, 1977,
 *      doi:10.1145/321992.321993
 * \see U. Pape, Implementation and efficiency of Moore-algorithms for the
 *      shortest route problem, Mathematical Programming 7, 212-222, 1974,
 *      doi:10.1007/BF01585517
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

#ifndef __ADS_ShortestPaths
 #define __ADS_ShortestPaths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Visit.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/stack/ArrayStack.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
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

/// what a run of the prototype did: the nodes added to S (the lines (1)
/// and (3)), the extractions (2), the decreases of a priority (4) and the
/// edges examined; the same on two frontiers that extract the nodes in
/// the same order
struct FrontierCounts {
 long long adds = 0 , extracts = 0 , decreases = 0 , edges = 0;
 };

/// the structure S of the prototype as a type: add( v , d , first ) puts
/// v into S at the distance d, first if v was never reached; extract()
/// takes a node out of S; update( v , d ) tells S that the distance of v,
/// already in S, fell to d
template< class S , class D >
concept PrototypeFrontier = requires( S s , int v , const D & d , bool b ) {
 s.add( v , d , b );
 { s.extract() } -> std::convertible_to< int >;
 s.update( v , d );
 { s.empty() } -> std::same_as< bool >;
 };

/// a priority queue of the nodes [see PriorityQueue.h] as S: the priority
/// of a node its distance or, if the priority is a pair, the pair
/// ( distance , node ), which breaks the ties by the names of the nodes,
/// so that any priority queue extracts the same nodes in the same order
template< PriorityQueue Q >
class PriorityFrontier {

 public:

 /// S on the queue q, empty, for the nodes [ 0 , n )
 /// \pre q is empty, n >= 0
 PriorityFrontier( Q & q , int n ) : f_q( q ) ,
                                     f_h( std::size_t( n < 0 ? 0 : n ) ) {}

 template< class D >
 void add( int v , const D & d , bool ) {
  f_h[ v ] = f_q.insert( v , key( v , d ) );
  }

 int extract( void ) { return( f_q.deleteMin() ); }

 template< class D >
 void update( int v , const D & d ) {
  f_q.decrease( f_h[ v ] , key( v , d ) );
  }

 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 /// the priority of v at the distance d
 template< class D >
 static typename Q::Priority key( int v , const D & d ) {
  using P = typename Q::Priority;
  if constexpr( requires { typename P::first_type; } )
   return( P( d , v ) );
  else
   return( P( d ) );
  }

 private:

 Q & f_q;
 std::vector< typename Q::Handle > f_h;   ///< the handle of each node

 };  // end( class PriorityFrontier )

/// a queue as S, enqueue() and dequeue(): first in, first out, so that a
/// node leaves S at most n times unless a negative cycle is reachable
template< class Q >
class QueueFrontier {

 public:

 static constexpr bool fifo = true;   ///< the bound of the extractions

 explicit QueueFrontier( Q & q ) : f_q( q ) {}

 template< class D >
 void add( int v , const D & , bool ) { f_q.enqueue( v ); }

 int extract( void ) { return( f_q.dequeue() ); }

 template< class D >
 void update( int , const D & ) {}

 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 private:

 Q & f_q;

 };  // end( class QueueFrontier )

/// a stack as S, push() and pop()
template< class Q >
class StackFrontier {

 public:

 explicit StackFrontier( Q & q ) : f_q( q ) {}

 template< class D >
 void add( int v , const D & , bool ) { f_q.push( v ); }

 int extract( void ) { return( f_q.pop() ); }

 template< class D >
 void update( int , const D & ) {}

 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 private:

 Q & f_q;

 };  // end( class StackFrontier )

/// a deque as S, the rule of Pape and D'Esopo: a node at the tail the
/// first time, at the head afterwards, extracted from the head
template< class Q >
class DequeFrontier {

 public:

 explicit DequeFrontier( Q & q ) : f_q( q ) {}

 template< class D >
 void add( int v , const D & , bool first ) {
  if( first ) f_q.insertTail( v ); else f_q.insertHead( v );
  }

 int extract( void ) { return( f_q.removeHead() ); }

 template< class D >
 void update( int , const D & ) {}

 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 private:

 Q & f_q;

 };  // end( class DequeFrontier )

/*--------------------------------------------------------------------------*/
/*------------------------------ PROTOTYPE ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// camminiMinimi(): the shortest paths from r with the structure how
/// \pre r is a node; no negative cycle reachable from r unless how is
///      QUEUE (audit)
/// \post the conditions of Bellman, unless a negative cycle was found
///       (audit)
template< Graph G >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                                      Frontier how );

/// camminiMinimi() with the structure S given, its work in *c if given
/// \pre r is a node, S is empty; no negative cycle reachable from r unless
///      S is first in, first out (audit)
/// \post the conditions of Bellman, unless a negative cycle was found
///       (audit)
template< Graph G , class S >
requires PrototypeFrontier< S , typename G::Weight >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                     S & frontier ,
                                     FrontierCounts * c = nullptr );

/// Dijkstra: a priority queue on an unsorted list, \f$ O( n^2 ) \f$
template< Graph G >
auto dijkstra( const G & g , int r );

/// Johnson: a priority queue on a binary heap, \f$ O( m \log n ) \f$
template< Graph G >
auto johnson( const G & g , int r );

/// Fredman-Tarjan: a priority queue on a Fibonacci heap,
/// \f$ O( m + n \log n ) \f$
template< Graph G >
auto fredmanTarjan( const G & g , int r );

/// Bellman-Ford-Moore: a queue, \f$ O( n m ) \f$, negative cycles detected
template< Graph G >
auto bellmanFordMoore( const G & g , int r );

/// Pape-D'Esopo: a dequeue
template< Graph G >
auto papeDEsopo( const G & g , int r );

/// the shortest paths of an acyclic graph, scanning its nodes in
/// topological order, \f$ O( n + m ) \f$
/// \pre r is a node, and the graph is acyclic (audit)
/// \post the conditions of Bellman (audit)
template< Graph G >
ShortestPathTree< typename G::Weight > dagShortestPaths( const G & g ,
                                                         int r );

/// whether R is a tree of shortest paths from r: d[ r ] = 0, the
/// unreachable nodes at infinity() with no parent, d[ v ] = d[ T[ v ] ] +
/// w( T[ v ] , v ) on the others, and d[ u ] + w( u , v ) >= d[ v ] on each
/// edge from a reached node (the Teorema 11.1), \f$ O( n + m ) \f$ on lists
template< Graph G >
bool isShortestPathTree( const G & g , int r ,
                         const ShortestPathTree< typename G::Weight > & R );

/// whether a negative cycle is reachable from r, by Bellman-Ford,
/// \f$ O( n m ) \f$
template< Graph G >
bool reachesNegativeCycle( const G & g , int r );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/ShortestPaths.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ShortestPaths.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ShortestPaths.h ------------------------*/
/*--------------------------------------------------------------------------*/
