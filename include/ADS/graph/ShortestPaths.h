/*--------------------------------------------------------------------------*/
/*-------------------------- File ShortestPaths.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths of chapter 11 from a node \f$ r \f$ of an oriented GRAPH
 * whose edges have a cost \f$ w( u , v ) \f$ [see `Graph.h`]. A feasible
 * solution is a tree T rooted in r, each node v at distance \f$ d_v \f$ from r
 * in T; by the theorem of Bellman (Theorem 11.1) T is optimal if and only if
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
 * - `LIST`, a priority queue on an unsorted list: Dijkstra (1959), each node
 *   extracted once if the costs are not negative, \f$ O( n^2 ) \f$;
 * - `HEAP`, a priority queue on a binary heap: Johnson (1977),
 *   \f$ O( m \log n ) \f$;
 * - `FIBONACCI`, a priority queue on a Fibonacci heap: Fredman and Tarjan
 *   (1987), \f$ O( m + n \log n ) \f$;
 * - `QUEUE`: Bellman-Ford-Moore, a node extracted at most n times even with
 *   negative costs, \f$ O( n m ) \f$, and a negative cycle reachable from r
 *   is detected when a node is extracted more than n times (exercise
 *   11.2); with the other frontiers there must be no negative cycle;
 * - `STACK`: superpolynomial in the worst case (section 11.7);
 * - `DEQUE`: Pape-D'Esopo (section 11.8), a node at the tail of S the first
 *   time and at its head afterwards, superpolynomial in the worst case and
 *   the fastest in practice on road networks.
 *
 * With a priority queue a node already in S has its priority decreased
 * (the line (4) of the book); in the code of the book the new distance and
 * parent of v are written in both branches of that test. The structure S
 * is a type [see `PrototypeFrontier`]: `shortestPaths()` with a Frontier runs
 * the prototype on the six structures of the book, and the prototype runs
 * as well on any priority queue of section 10.1 [see `PriorityFrontier`],
 * queue, stack or deque of another library; with the priorities
 * \f$ ( d_v , v ) \f$ the ties are broken by the names of the nodes, and
 * every priority queue extracts the same nodes in the same order, so that
 * the counts of a run [see `FrontierCounts`] measure the same work on any
 * of them. Without negative
 * cycles every frontier gives the shortest paths; with negative costs the
 * priority queues are still correct but may extract a node more than
 * once. `dagShortestPaths()` scans an acyclic graph in topological order,
 * \f$ O( n + m ) \f$ whatever the signs of the costs. The distances are
 * sums in D, not checked for overflow: the cost of every path from r must
 * stay strictly between -infinity< D >() and infinity< D >(), and a node
 * at distance infinity< D >() counts as unreachable.
 *
 * The contracts [see `Contract.h`]: r is a node; with a frontier other than
 * `QUEUE` no negative cycle is reachable from r, and `dagShortestPaths()`
 * needs an acyclic graph, both checked at the level of the audits (the first
 * by Bellman-Ford, \f$ O( n m ) \f$); the tree found satisfies the conditions
 * of Bellman unless a negative cycle was found, which `isShortestPathTree()`
 * checks. The functions are defined in `ShortestPaths.tpp`.
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

/// the structure \f$ S \f$ of the prototype
enum class Frontier {
 LIST ,        ///< a priority queue on an unsorted list: Dijkstra
 HEAP ,        ///< a priority queue on a binary heap: Johnson
 FIBONACCI ,   ///< a priority queue on a Fibonacci heap: Fredman and Tarjan
 QUEUE ,       ///< a queue: Bellman, Ford and Moore
 STACK ,       ///< a stack
 DEQUE         ///< a deque: Pape and D'Esopo
 };

/// the infinite distance of the type `D`
template< class D >
constexpr D infinity( void )
{
 if constexpr( std::numeric_limits< D >::has_infinity )
  return( std::numeric_limits< D >::infinity() );
 else
  return( std::numeric_limits< D >::max() );
 }

/// the result: the distances, `infinity()` if not reachable, the tree of the
/// parents, `NIL` for \f$ r \f$ and the unreachable nodes, the nodes in the
/// order of extraction from \f$ S \f$, and whether a negative cycle was found
template< class D >
struct ShortestPathTree {
 std::vector< D > d;            ///< the distances
 std::vector< int > T;          ///< the parent of each node
 std::vector< int > order;      ///< the nodes in the order of extraction
 bool negativeCycle = false;    ///< whether a negative cycle was found
 };

/// what a run of the prototype did: the nodes added to \f$ S \f$ (the lines
/// (1) and (3)), the extractions (2), the decreases of a priority (4) and the
/// edges examined; the same on two frontiers that extract the nodes in the
/// same order
struct FrontierCounts {
 long long adds = 0 ,        ///< the nodes added to \f$ S \f$
           extracts = 0 ,    ///< the extractions
           decreases = 0 ,   ///< the decreases of a priority
           edges = 0;        ///< the edges examined
 };

/// the structure \f$ S \f$ of the prototype as a type: add( v , d , first )
/// puts v into S at the distance d, first if v was never reached; `extract()`
/// takes a node out of S; update( v , d ) tells S that the distance of v,
/// already in S, fell to d
template< class S , class D >
concept PrototypeFrontier = requires( S s , int v , const D & d , bool b ) {
 s.add( v , d , b );
 { s.extract() } -> std::convertible_to< int >;
 s.update( v , d );
 { s.empty() } -> std::same_as< bool >;
 };

/// a priority queue of the nodes [see `PriorityQueue.h`] as \f$ S \f$: the
/// priority of a node its distance or, if the priority is a pair, the pair
/// ( distance , node ), which breaks the ties by the names of the nodes, so
/// that any priority queue extracts the same nodes in the same order
template< PriorityQueue Q >
class PriorityFrontier {

 public:

 /// \f$ S \f$ on the queue \f$ q \f$, empty, for the nodes \f$ [ 0 , n ) \f$
 /// \pre \f$ q \f$ is empty, \f$ n \ge 0 \f$
 PriorityFrontier( Q & q , int n ) : f_q( q ) {
  ADS_PRE( n >= 0 );
  f_h.resize( std::size_t( n ) );
  }

 /// \f$ v \f$ into \f$ S \f$ at the distance \f$ d \f$
 template< class D >
 void add( int v , const D & d , bool ) {
  f_h[ v ] = f_q.insert( v , key( v , d ) );
  }

 /// a node out of \f$ S \f$
 int extract( void ) { return( f_q.deleteMin() ); }

 /// the priority of \f$ v \f$, already in \f$ S \f$, falls to \f$ d \f$
 template< class D >
 void update( int v , const D & d ) {
  f_q.decrease( f_h[ v ] , key( v , d ) );
  }

 /// whether \f$ S \f$ is empty
 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 /// the priority of \f$ v \f$ at the distance \f$ d \f$
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

/// a queue as \f$ S \f$, `enqueue()` and `dequeue()`: first in, first out, so
/// that a node leaves S at most n times unless a negative cycle is reachable
template< class Q >
class QueueFrontier {

 public:

 static constexpr bool fifo = true;   ///< the bound of the extractions

 /// \f$ S \f$ on the empty structure \f$ q \f$
 explicit QueueFrontier( Q & q ) : f_q( q ) {}

 /// \f$ v \f$ into \f$ S \f$
 template< class D >
 void add( int v , const D & , bool ) { f_q.enqueue( v ); }

 /// a node out of \f$ S \f$
 int extract( void ) { return( f_q.dequeue() ); }

 /// nothing: the order of \f$ S \f$ does not depend on the distances
 template< class D >
 void update( int , const D & ) {}

 /// whether \f$ S \f$ is empty
 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 private:

 Q & f_q;

 };  // end( class QueueFrontier )

/// a stack as \f$ S \f$, `push()` and `pop()`
template< class Q >
class StackFrontier {

 public:

 /// \f$ S \f$ on the empty structure \f$ q \f$
 explicit StackFrontier( Q & q ) : f_q( q ) {}

 /// \f$ v \f$ into \f$ S \f$
 template< class D >
 void add( int v , const D & , bool ) { f_q.push( v ); }

 /// a node out of \f$ S \f$
 int extract( void ) { return( f_q.pop() ); }

 /// nothing: the order of \f$ S \f$ does not depend on the distances
 template< class D >
 void update( int , const D & ) {}

 /// whether \f$ S \f$ is empty
 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 private:

 Q & f_q;

 };  // end( class StackFrontier )

/// a deque as \f$ S \f$, the rule of Pape and D'Esopo: a node at the tail the
/// first time, at the head afterwards, extracted from the head
template< class Q >
class DequeFrontier {

 public:

 /// \f$ S \f$ on the empty structure \f$ q \f$
 explicit DequeFrontier( Q & q ) : f_q( q ) {}

 /// \f$ v \f$ at the tail of \f$ S \f$ if first, at its head if not
 template< class D >
 void add( int v , const D & , bool first ) {
  if( first ) f_q.insertTail( v ); else f_q.insertHead( v );
  }

 /// a node out of \f$ S \f$
 int extract( void ) { return( f_q.removeHead() ); }

 /// nothing: the order of \f$ S \f$ does not depend on the distances
 template< class D >
 void update( int , const D & ) {}

 /// whether \f$ S \f$ is empty
 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 private:

 Q & f_q;

 };  // end( class DequeFrontier )

/*--------------------------------------------------------------------------*/
/*------------------------------ PROTOTYPE ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the shortest paths from \f$ r \f$ with the structure
/// `how`
///
/// In pseudocode, the prototype `shortestPaths( G , r , S )` below, with
/// \f$ S \f$ the structure that `how` names:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{shortestPaths}( G , r , \mathit{how} ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{an empty }
///     \text{structure of kind } \mathit{how} )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node; no negative cycle reachable from \f$ r \f$ unless
///      `how` is `QUEUE` (audit)
/// \post the conditions of Bellman, unless a negative cycle was found
///       (audit)
template< Graph G >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                                      Frontier how );

/// the shortest paths of `shortestPaths()` above, with the structure \f$ S \f$
/// given, its work in `*c` if given
///
/// In pseudocode (the prototype of section 11.2, its lines (1) to (4) as in
/// the book), w( u , v ) the cost of the edge:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{shortestPaths}( G , r , S ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty;\ T[
///     v ] \gets \textbf{nil} \cr
/// 3 & \quad d[ r ] \gets 0;\ \mathrm{add}( S , r )\quad \triangleright
///     \ \text{(1)} \cr
/// 4 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad u \gets \mathrm{extract}( S )\quad \triangleright\ \text{(2)}
///     \cr
/// 6 & \qquad \textbf{if}\ S \text{ is first in, first out and } u
///     \text{left it more than } n \text{ times }\ \textbf{then} \cr
/// 7 & \qquad\quad \textbf{return}\  \text{a negative cycle} \cr
/// 8 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}\ d[ u
///     ] + w( u , v ) < d[ v ]\ \textbf{do} \cr
/// 9 & \qquad\quad d[ v ] \gets d[ u ] + w( u , v );\ T[ v ] \gets u \cr
/// 10 & \qquad\quad \textbf{if}\ v \notin S\ \textbf{then}\ \mathrm{add}( S ,
///     v )\quad \triangleright\ \text{(3)} \cr
/// 11 & \qquad\quad \textbf{else}\ \mathrm{update}( S , v )\quad
///     \triangleright\ \text{(4), the decrease of a priority} \cr
/// 12 & \quad \textbf{return}\ ( d , T )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node, \f$ S \f$ is empty; no negative cycle reachable
///      from \f$ r \f$ unless \f$ S \f$ is first in, first out (audit)
/// \post the conditions of Bellman, unless a negative cycle was found
///       (audit)
template< Graph G , class S >
requires PrototypeFrontier< S , typename G::Weight >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                     S & frontier ,
                                     FrontierCounts * c = nullptr );

/// Dijkstra: a priority queue on an unsorted list, \f$ O( n^2 ) \f$
///
/// The prototype with its structure S (section 11.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dijkstra}( G , r ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{a }
///     \text{priority queue on an unsorted list} )
/// \end{array}}
/// \f]
template< Graph G >
auto dijkstra( const G & g , int r );

/// Johnson: a priority queue on a binary heap, \f$ O( m \log n ) \f$
///
/// The prototype with its structure S (section 11.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{johnson}( G , r ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{a }
///     \text{priority queue on a binary heap} )
/// \end{array}}
/// \f]
template< Graph G >
auto johnson( const G & g , int r );

/// Fredman-Tarjan: a priority queue on a Fibonacci heap,
/// \f$ O( m + n \log n ) \f$
///
/// The prototype with its structure S (section 11.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fredmanTarjan}( G , r ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{a }
///     \text{priority queue on a Fibonacci heap} )
/// \end{array}}
/// \f]
template< Graph G >
auto fredmanTarjan( const G & g , int r );

/// Bellman-Ford-Moore: a queue, \f$ O( n m ) \f$, negative cycles detected
///
/// The prototype with its structure S (section 11.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bellmanFordMoore}( G , r ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{a queue} )
/// \end{array}}
/// \f]
template< Graph G >
auto bellmanFordMoore( const G & g , int r );

/// Pape-D'Esopo: a deque
///
/// The prototype with its structure S (section 11.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{papeDEsopo}( G , r ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{a deque, }
///     \text{a node at its tail the first time and at its head afterwards} )
/// \end{array}}
/// \f]
template< Graph G >
auto papeDEsopo( const G & g , int r );

/// the shortest paths of an acyclic graph, scanning its nodes in
/// topological order, \f$ O( n + m ) \f$
///
/// In pseudocode (section 11.2): in topological order the nodes before u have
/// their final distances when u is scanned, whatever the signs of the costs.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dagShortestPaths}( G , r ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty;\ T[
///     v ] \gets \textbf{nil} \cr
/// 3 & \quad d[ r ] \gets 0 \cr
/// 4 & \quad \textbf{foreach}\ u \in \mathrm{topsort}( G )\ \textbf{with}\ d[
///     u ] < \infty\ \textbf{do} \cr
/// 5 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}\ d[ u
///     ] + w( u , v ) < d[ v ]\ \textbf{do} \cr
/// 6 & \qquad\quad d[ v ] \gets d[ u ] + w( u , v );\ T[ v ] \gets u \cr
/// 7 & \quad \textbf{return}\ ( d , T )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node, and the graph is acyclic (audit)
/// \post the conditions of Bellman (audit)
template< Graph G >
ShortestPathTree< typename G::Weight > dagShortestPaths( const G & g ,
                                                         int r );

/// whether \f$ R \f$ is a tree of shortest paths from \f$ r \f$: d[ r ] = 0,
/// the unreachable nodes at `infinity()` with no parent, d[ v ] = d[ T[ v ] ]
/// + w( T[ v ] , v ) on the others, and d[ u ] + w( u , v ) >= d[ v ] on each
/// edge from a reached node (Theorem 11.1), \f$ O( n + m ) \f$ on lists
///
/// In pseudocode (the conditions of Bellman, Theorem 11.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isShortestPathTree}( G , r , d , T ) \cr
/// 2 & \quad \textbf{if}\ d[ r ] \ne 0\ \textbf{or}\ T[ r ] \ne \textbf{nil}
///     \ \textbf{then return false} \cr
/// 3 & \quad \textbf{foreach}\ v \in V \setminus \{ r \}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ d[ v ] = \infty\ \textbf{and}\ T[ v ] \ne
///     \textbf{nil}\ \textbf{then return false} \cr
/// 5 & \qquad \textbf{if}\ d[ v ] < \infty\ \textbf{and}\ d[ v ] \ne d[ T[ v ]
///     ] + w( T[ v ] , v )\ \textbf{then return false} \cr
/// 6 & \quad \textbf{foreach}\ ( u , v ) \in E\ \textbf{with}\ d[ u ] <
///     \infty\ \textbf{do} \cr
/// 7 & \qquad \textbf{if}\ d[ u ] + w( u , v ) < d[ v ]\ \textbf{then return
///     false} \cr
/// 8 & \quad \textbf{return true}
/// \end{array}}
/// \f]
template< Graph G >
bool isShortestPathTree( const G & g , int r ,
                         const ShortestPathTree< typename G::Weight > & R );

/// whether a negative cycle is reachable from \f$ r \f$, by Bellman-Ford,
/// \f$ O( n m ) \f$
///
/// In pseudocode (exercise 11.2): with a queue no node leaves S more than n
/// times unless a negative cycle is reachable.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reachesNegativeCycle}( G , r ) \cr
/// 2 & \quad \mathrm{shortestPaths}( G , r , \text{an empty queue} ) \cr
/// 3 & \quad \textbf{return}\  \text{whether it found a negative cycle}
/// \end{array}}
/// \f]
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
