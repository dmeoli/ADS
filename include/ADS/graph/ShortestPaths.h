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
 * \f$ O( n + m ) \f$ whatever the signs of the costs. The distances are
 * sums in D, not checked for overflow: the cost of every path from r must
 * stay strictly between -infinity< D >() and infinity< D >(), and a node
 * at distance infinity< D >() counts as unreachable.
 *
 * The contracts [see Contract.h]: r is a node; with a frontier other than
 * QUEUE no negative cycle is reachable from r, and dagShortestPaths() needs
 * an acyclic graph, both checked at the level of the audits (the first by
 * Bellman-Ford, O( n m )); the tree found satisfies the conditions of
 * Bellman unless a negative cycle was found, which isShortestPathTree()
 * checks. The functions are defined in ShortestPaths.tpp.
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

#include "ADS/Contract.h"
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
/// \pre r is a node; no negative cycle reachable from r unless how is
///      QUEUE (audit)
/// \post the conditions of Bellman, unless a negative cycle was found
///       (audit)
template< Graph G >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                                      Frontier how );

/// Dijkstra: a priority queue on an unsorted list, O( n^2 )
template< Graph G >
auto dijkstra( const G & g , int r );

/// Johnson: a priority queue on a binary heap, O( m log n )
template< Graph G >
auto johnson( const G & g , int r );

/// Fredman-Tarjan: a priority queue on a Fibonacci heap, O( m + n log n )
template< Graph G >
auto fredmanTarjan( const G & g , int r );

/// Bellman-Ford-Moore: a queue, O( n m ), negative cycles detected
template< Graph G >
auto bellmanFordMoore( const G & g , int r );

/// Pape-D'Esopo: a dequeue
template< Graph G >
auto papeDEsopo( const G & g , int r );

/// the shortest paths of an acyclic graph, scanning its nodes in
/// topological order, O( n + m )
/// \pre r is a node, and the graph is acyclic (audit)
/// \post the conditions of Bellman (audit)
template< Graph G >
ShortestPathTree< typename G::Weight > dagShortestPaths( const G & g ,
                                                         int r );

/// whether R is a tree of shortest paths from r: d[ r ] = 0, the
/// unreachable nodes at infinity() with no parent, d[ v ] = d[ T[ v ] ] +
/// w( T[ v ] , v ) on the others, and d[ u ] + w( u , v ) >= d[ v ] on each
/// edge from a reached node (the Teorema 11.1), O( n + m ) on lists
template< Graph G >
bool isShortestPathTree( const G & g , int r ,
                         const ShortestPathTree< typename G::Weight > & R );

/// whether a negative cycle is reachable from r, by Bellman-Ford, O( n m )
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
