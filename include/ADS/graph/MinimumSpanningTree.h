/*--------------------------------------------------------------------------*/
/*----------------------- File MinimumSpanningTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum spanning tree of an unoriented connected graph with weights on
 * the edges (section 14.3): a tree on all the nodes, of \f$ n - 1 \f$ edges,
 * of least total weight. Both algorithms are greedy, and correct by the
 * Theorem 14.1: if A is part of a minimum spanning tree and no edge of A
 * crosses the cut ( S , V - S ), then the lightest edge across it can be added
 * to A.
 *
 * - `kruskal()` scans the edges by increasing weight and keeps one if its ends
 *   are in two components of the forest, an MFSET with the rank and the
 *   compression of the paths: \f$ O( m \log n ) \f$ for the sort and almost
 *   \f$ O( m ) \f$ for the rest; it stops after n - 1 edges. With integer
 *   weights in [ 0 , k ) the edges are sorted by Counting Sort,
 *   \f$ O( m + n + k ) \f$ (exercise 14.2).
 * - `prim()` grows the tree from a root r, adding the lightest edge that
 *   leaves it, the nodes in a priority queue by the weight of their
 *   lightest edge to the tree: \f$ O( m \log n ) \f$ with a binary heap,
 *   \f$ O( n^2 ) \f$ with an unsorted list, better on dense graphs, and
 *   \f$ O( m + n \log n ) \f$ with a Fibonacci heap (exercise 14.4).
 *
 * A graph that is not connected gives a minimum spanning forest by
 * `kruskal()`, and the tree of the component of r by `prim()`. Both take
 * their structure as well: `kruskal()` any MFSET with `merge()` [see
 * Mergeable], `prim()` any priority queue of section 10.1, whose priority
 * may be the pair ( weight , node ) so that every queue gives the same
 * tree with the same counts [see `FrontierCounts`, `MergeCounts`].
 *
 * The contracts [see `Contract.h`]: the graph is unoriented; the edges given
 * to `kruskal()` join nodes in [ 0 , n ), with weights in [ 0 , k ) when k >
 * 0; `prim()` starts from a node, with a priority queue as frontier. At the
 * level of the audits the result is a minimum spanning forest, which
 * `isMinimumSpanningForest()` checks by the property of the cycles: a
 * spanning forest is minimum if and only if no edge out of it weighs less
 * than an edge of the path of the forest between its ends. The functions
 * are defined in `MinimumSpanningTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see J. B. Kruskal, On the shortest spanning subtree of a graph and the
 *      traveling salesman problem, Proceedings of the American Mathematical
 *      Society 7(1), 48-50, 1956, doi:10.1090/S0002-9939-1956-0078686-7
 * \see R. C. Prim, Shortest connection networks and some generalizations,
 *      Bell System Technical Journal 36(6), 1389-1401, 1957,
 *      doi:10.1002/j.1538-7305.1957.tb01515.x
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

#ifndef __ADS_MinimumSpanningTree
 #define __ADS_MinimumSpanningTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// an edge \f$ [ u , v ] \f$ of weight \f$ w \f$
template< class W >
struct WeightedEdge {
 int u ,   ///< one end
     v;    ///< the other end
 W w;      ///< the weight
 /// whether the two edges have the same ends, in order, and weight
 bool operator==( const WeightedEdge & ) const = default;
 };

/// the edges of an unoriented graph, each once, from its smaller end
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{edges}( G ) \cr
/// 2 & \quad A \gets () \cr
/// 3 & \quad \textbf{foreach}\ u \in V\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}\ u <
///     v\ \textbf{do}\  \text{append } [ u , v ] \text{ of weight } w( u , v )
///     \text{ to } A \cr
/// 5 & \quad \textbf{return}\ A
/// \end{array}}
/// \f]
/// \pre the graph is unoriented
template< Graph G >
std::vector< WeightedEdge< typename G::Weight > > edges( const G & g );

/// the edges of a minimum spanning forest of the nodes
/// \f$ [ 0 , n ) \f$; with \f$ k > 0 \f$ the weights are integers in
/// \f$ [ 0 , k ) \f$, sorted by counting
///
/// In pseudocode (section 14.3): by Theorem 14.1 the lightest edge between
/// two components of the forest belongs to a minimum spanning forest.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kruskal}( n , A ) \cr
/// 2 & \quad \text{sort } A \text{ by increasing weight}\quad \triangleright
///     \ \text{by Counting Sort if } k > 0 \cr
/// 3 & \quad M \gets \text{an MFSET of } \{ 0 \} , \ldots , \{ n - 1 \};\ T
///     \gets \emptyset \cr
/// 4 & \quad \textbf{foreach}\ [ u , v ] \in A \text{ in this order, while }
///     |T| < n - 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{find}( M , u ) \ne \mathrm{find}( M , v )
///     \ \textbf{then} \cr
/// 6 & \qquad\quad \mathrm{merge}( M , u , v );\ T \gets T \cup \{ [ u , v ]
///     \} \cr
/// 7 & \quad \textbf{return}\ T
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the ends of the edges in \f$ [ 0 , n ) \f$, the
///      weights in \f$ [ 0 , k ) \f$ if \f$ k > 0 \f$
/// \post a minimum spanning forest (audit)
template< class W >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          long k = 0 );

/// a union of disjoint sets as `kruskal()` needs it: merge( u , v ) joins
/// the components of u and v and tells whether they were apart
template< class M >
concept Mergeable = requires( M m , int u , int v ) {
 { m.merge( u , v ) } -> std::convertible_to< bool >;
 };

/// what a run of `kruskal()` did: the merges tried and those that joined
/// two components
struct MergeCounts {
 long long merges = 0 ,   ///< the merges tried
           unions = 0;   ///< the merges that joined two components
 };

/// the minimum spanning forest of `kruskal()` above, with the MFSET `sets`
/// given, whose merge( u , v ) joins the components of u and v and tells
/// whether they were apart [see `MFSet.h`]; its work in `*c`, if given
///
/// It is kruskal( n , A ) above, with merge( u , v ) of sets in place of the
/// test on `find()` and the merge.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kruskal}( n , A , \mathit{sets} ) \cr
/// 2 & \quad \text{as } \mathrm{kruskal}( n , A ) \text{ , with } M =
///     \mathit{sets}
/// \end{array}}
/// \f]
/// \pre as `kruskal()`, `sets` has the components
///      \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$
/// \post a minimum spanning forest (audit)
template< class W , Mergeable M >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          M & sets , long k = 0 ,
                                          MergeCounts * c = nullptr );

/// the minimum spanning forest of `kruskal()` above, on the edges of an
/// unoriented graph
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kruskal}( G ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{kruskal}( n , \mathrm{edges}( G ) )
/// \end{array}}
/// \f]
template< Graph G >
auto kruskal( const G & g , long k = 0 );

/// the parents of the tree from \f$ r \f$, `NIL` for \f$ r \f$ and the
/// nodes out of its component; how is `LIST`, `HEAP` or `FIBONACCI`
///
/// It is prim( G , r , q ) below, with q the priority queue that how names.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{prim}( G , r , \mathit{how} ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{prim}( G , r , \text{an empty priority }
///     \text{queue of kind } \mathit{how} )
/// \end{array}}
/// \f]
/// \pre the graph is unoriented, \f$ r \f$ is a node, `how` is a priority
///      queue
/// \post a minimum spanning tree of the component of \f$ r \f$ (audit)
template< Graph G >
std::vector< int > prim( const G & g , int r , Frontier how = Frontier::HEAP );

/// the tree of `prim()` above, on the priority queue \f$ q \f$ given [see
/// `PriorityQueue.h`], whose priority is the weight or the pair
/// ( weight , node ), which breaks the ties by the names of the nodes; its
/// work in `*c`, if given
///
/// In pseudocode (section 14.3): the key of a node out of the tree is the
/// weight of its lightest edge to the tree, and the node of least key joins
/// it.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{prim}( G , r , Q ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ \mathit{key}[ v ] \gets
///     \infty;\ p[ v ] \gets \textbf{nil};\ \mathit{in}[ v ] \gets
///     \textbf{false} \cr
/// 3 & \quad \mathit{key}[ r ] \gets 0 \cr
/// 4 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ \mathrm{insert}( Q , v ,
///     \mathit{key}[ v ] ) \cr
/// 5 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad u \gets \mathrm{deleteMin}( Q );\ \mathit{in}[ u ] \gets
///     \textbf{true} \cr
/// 7 & \qquad \textbf{if}\ \mathit{key}[ u ] = \infty\ \textbf{then
///     continue}\quad \triangleright\ \text{another component} \cr
/// 8 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with not}
///     \ \mathit{in}[ v ]\ \textbf{and}\ w( u , v ) < \mathit{key}[ v ]
///     \ \textbf{do} \cr
/// 9 & \qquad\quad \mathit{key}[ v ] \gets w( u , v );\ p[ v ] \gets u;
///     \ \mathrm{decrease}( Q , v , \mathit{key}[ v ] ) \cr
/// 10 & \quad \textbf{return}\ p
/// \end{array}}
/// \f]
/// \pre the graph is unoriented, \f$ r \f$ is a node, \f$ q \f$ is empty
/// \post a minimum spanning tree of the component of \f$ r \f$ (audit)
template< Graph G , PriorityQueue Q >
std::vector< int > prim( const G & g , int r , Q & q ,
                         FrontierCounts * c = nullptr );

/// whether \f$ T \f$ is a minimum spanning forest of the edges \f$ A \f$ on
/// the nodes \f$ [ 0 , n ) \f$: its edges are in A, it has no cycle, as many
/// edges as n minus the components of A, and no edge of A weighs less than the
/// heaviest edge of the path of T between its ends, \f$ O( m n ) \f$
///
/// In pseudocode (the property of the cycles):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isMinimumSpanningForest}( n , A , T ) \cr
/// 2 & \quad \textbf{if}\ T \not\subseteq A\ \textbf{or}\ T \text{ has a }
///     \text{cycle }\ \textbf{then return false} \cr
/// 3 & \quad \textbf{if}\ |T| \ne n - \text{ the components of } A
///     \ \textbf{then return false} \cr
/// 4 & \quad \textbf{foreach}\ [ u , v ] \in A\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ w( u , v ) < \text{ the heaviest edge of the path }
///     \text{of } T \text{ from } u \text{ to } v\ \textbf{then} \cr
/// 6 & \qquad\quad \textbf{return false} \cr
/// 7 & \quad \textbf{return true}
/// \end{array}}
/// \f]
template< class W >
bool isMinimumSpanningForest( int n ,
                              const std::vector< WeightedEdge< W > > & A ,
                              const std::vector< WeightedEdge< W > > & T );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/MinimumSpanningTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MinimumSpanningTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File MinimumSpanningTree.h ---------------------*/
/*--------------------------------------------------------------------------*/
