/*--------------------------------------------------------------------------*/
/*----------------------- File MinimumSpanningTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum spanning tree of an unoriented connected graph with weights
 * on the edges (section 14.3): a tree on all the nodes, of n - 1 edges,
 * of least total weight. Both algorithms are greedy, and correct by the
 * Teorema 14.1: if A is part of a minimum spanning tree and no edge of A
 * crosses the cut ( S , V - S ), then the lightest edge across it can be
 * added to A.
 *
 * - kruskal() scans the edges by increasing weight and keeps one if its ends
 *   are in two components of the forest, an MFSET with the rank and the
 *   compression of the paths: \f$ O( m \log n ) \f$ for the sort and almost
 *   \f$ O( m ) \f$ for the rest; it stops after n - 1 edges. With integer
 *   weights in [ 0 , k ) the edges are sorted by Counting Sort,
 *   \f$ O( m + n + k ) \f$ (exercise 14.2).
 * - prim() grows the tree from a root r, adding the lightest edge that
 *   leaves it, the nodes in a priority queue by the weight of their
 *   lightest edge to the tree: \f$ O( m \log n ) \f$ with a binary heap,
 *   \f$ O( n^2 ) \f$ with an unsorted list, better on dense graphs, and
 *   \f$ O( m + n \log n ) \f$ with a Fibonacci heap (exercise 14.4).
 *
 * A graph that is not connected gives a minimum spanning forest by
 * kruskal(), and the tree of the component of r by prim(). Both take
 * their structure as well: kruskal() any MFSET with merge() [see
 * Mergeable], prim() any priority queue of section 10.1, whose priority
 * may be the pair ( weight , node ) so that every queue gives the same
 * tree with the same counts [see FrontierCounts, MergeCounts].
 *
 * The contracts [see Contract.h]: the graph is unoriented; the edges given
 * to kruskal() join nodes in [ 0 , n ), with weights in [ 0 , k ) when k >
 * 0; prim() starts from a node, with a priority queue as frontier. At the
 * level of the audits the result is a minimum spanning forest, which
 * isMinimumSpanningForest() checks by the property of the cycles: a
 * spanning forest is minimum if and only if no edge out of it weighs less
 * than an edge of the path of the forest between its ends. The functions
 * are defined in MinimumSpanningTree.tpp.
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

/// an edge [ u , v ] of weight w
template< class W >
struct WeightedEdge {
 int u , v;
 W w;
 bool operator==( const WeightedEdge & ) const = default;
 };

/// the edges of an unoriented graph, each once, from its smaller end
/// \pre the graph is unoriented
template< Graph G >
std::vector< WeightedEdge< typename G::Weight > > edges( const G & g );

/// kruskal(): the edges of a minimum spanning forest of the nodes
/// [ 0 , n ); with k > 0 the weights are integers in [ 0 , k ), sorted by
/// counting
/// \pre n >= 0, the ends of the edges in [ 0 , n ), the weights in
///      [ 0 , k ) if k > 0
/// \post a minimum spanning forest (audit)
template< class W >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          long k = 0 );

/// a union of disjoint sets as kruskal() needs it: merge( u , v ) joins
/// the components of u and v and tells whether they were apart
template< class M >
concept Mergeable = requires( M m , int u , int v ) {
 { m.merge( u , v ) } -> std::convertible_to< bool >;
 };

/// what a run of kruskal() did: the merges tried and those that joined
/// two components
struct MergeCounts {
 long long merges = 0 , unions = 0;
 };

/// kruskal() with the MFSET sets given, whose merge( u , v ) joins the
/// components of u and v and tells whether they were apart [see MFSet.h];
/// its work in *c, if given
/// \pre as kruskal(), sets has the components {0} , ... , {n - 1}
/// \post a minimum spanning forest (audit)
template< class W , Mergeable M >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          M & sets , long k = 0 ,
                                          MergeCounts * c = nullptr );

/// kruskal() on the edges of an unoriented graph
template< Graph G >
auto kruskal( const G & g , long k = 0 );

/// prim(): the parents of the tree from r, NIL for r and the nodes out of
/// its component; how is LIST, HEAP or FIBONACCI
/// \pre the graph is unoriented, r is a node, how is a priority queue
/// \post a minimum spanning tree of the component of r (audit)
template< Graph G >
std::vector< int > prim( const G & g , int r , Frontier how = Frontier::HEAP );

/// prim() on the priority queue q given [see PriorityQueue.h], whose
/// priority is the weight or the pair ( weight , node ), which breaks the
/// ties by the names of the nodes; its work in *c, if given
/// \pre the graph is unoriented, r is a node, q is empty
/// \post a minimum spanning tree of the component of r (audit)
template< Graph G , PriorityQueue Q >
std::vector< int > prim( const G & g , int r , Q & q ,
                         FrontierCounts * c = nullptr );

/// whether T is a minimum spanning forest of the edges A on the nodes
/// [ 0 , n ): its edges are in A, it has no cycle, as many edges as n
/// minus the components of A, and no edge of A weighs less than the
/// heaviest edge of the path of T between its ends, \f$ O( m n ) \f$
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
