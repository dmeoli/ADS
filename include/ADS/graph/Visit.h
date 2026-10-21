/*--------------------------------------------------------------------------*/
/*------------------------------ File Visit.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The visits of a GRAPH of section 9.5 and their applications, each in
 * Theta( n + m ) time on lists or vectors of adjacency:
 *
 * - genericVisit(), the general scheme of which the others are cases: the
 *   reached nodes R and the open ones Q start as { r }, the tree B as
 *   empty; while Q is not empty a node j is chosen from Q, and if some k
 *   adjacent to j is not in R then k joins R and Q and ( j , k ) joins B,
 *   otherwise j leaves Q. Choosing the newest node of Q gives the visit in
 *   depth, the oldest the visit in breadth; B is a tree spanning the nodes
 *   reachable from r. A cursor in the adjacency of each node makes each
 *   edge examined once;
 * - bfs(), the visit in breadth with a QUEUE, which examines the nodes by
 *   increasing distance from the root r; distance() computes the distance
 *   (the minimum number of edges, the Erdos number of the example) and the
 *   tree of the shortest paths as a vector of parents, and path() reads a
 *   path from it, as printPath() does; distance() runs on an ArrayQueue
 *   or on any queue given, with enqueue() and dequeue() [see
 *   VisitCounts];
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
 *   topsortKahn() builds one by removing the nodes with no entering edge,
 *   and isTopologicalOrder() checks one;
 * - unilaterallyConnected(), true if for every two nodes u and v there is
 *   a path from u to v or one from v to u: so it is exactly when the graph
 *   of the strongly connected components, which is acyclic, has a path
 *   through all of them, i.e., when its topological order is unique, which
 *   the algorithm of Kahn finds out by never having two sources at once;
 * - transpose() and scc(), the strongly connected components of an oriented
 *   graph by the algorithm of Kosaraju: a visit gives the nodes by
 *   decreasing finish time, and cc() on the transposed graph starts its
 *   visits in that order. By the Teorema 9.1, if there is an edge from the
 *   component C to C' then ft( C ) > ft( C' ), so the second visit meets C
 *   first and, the edge being reversed, cannot reach C' from it.
 *
 * Nil is -1 and the infinite distance is INF. The visits in depth and
 * path() keep their recursion on an explicit stack, in the order of the
 * recursive ones, so that a path of 10^5 nodes does not overflow the
 * stack of the calls.
 *
 * The contracts [see Contract.h]: a visit starts from a node of the graph
 * (and dfs() from one not visited yet, with a vector of a place per name);
 * distance() ensures, at the level of the audits, that its distances are
 * those of a breadth first tree, topsort() and topsortKahn() that their
 * order is topological when the graph is acyclic, transpose() that it
 * reverses each edge. The functions are defined in Visit.tpp.
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

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/queue/ArrayQueue.h"

#include <algorithm>
#include <concepts>
#include <deque>
#include <limits>
#include <set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the infinite distance
inline constexpr int INF = std::numeric_limits< int >::max();

/// the node of Q that genericVisit() chooses
enum class Choice {
 NEWEST ,   ///< the last one added: the visit in depth
 OLDEST     ///< the first one added: the visit in breadth
 };

/// no node
inline constexpr int NIL = -1;

/*--------------------------------------------------------------------------*/
/*--------------------------------- BFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the general visit from r: the nodes in the order in which they join R,
/// and the edges of the tree B in the order in which they join it
/// \pre r is a node
/// \post B has one edge less than R, each from a node already in R (audit)
template< Graph G >
std::pair< std::vector< int > , std::vector< std::pair< int , int > > >
genericVisit( const G & g , int r , Choice c );

/// bfs(): node( u ) when u is examined, edge( u , v ) when ( u , v ) is
/// \pre r is a node
template< Graph G , class FN , class FE >
void bfs( const G & g , int r , FN && node , FE && edge );

/// distance(): the distances from r, INF if not reachable, and the parents
/// in the tree of the shortest paths, NIL for r and the unreachable nodes
/// \pre r is a node
/// \post each edge ( u , v ) from a reached u has d[ v ] <= d[ u ] + 1, and
///       each reached v but r has d[ v ] = d[ p[ v ] ] + 1 (audit)
template< Graph G >
std::pair< std::vector< int > , std::vector< int > > distance( const G & g ,
                                                                int r );

/// what a visit did: the nodes taken out of its structure and the edges
/// examined
struct VisitCounts {
 long long nodes = 0 , edges = 0;
 };

/// distance() with the queue S given, empty, whose enqueue() and dequeue()
/// keep the order of arrival [see ArrayQueue.h]; its work in *c, if given
/// \pre r is a node, S is empty
/// \post as distance()
template< Graph G , class Q >
requires requires( Q q , int v ) { q.enqueue( v ); { q.dequeue() } ->
                                   std::convertible_to< int >; }
std::pair< std::vector< int > , std::vector< int > > distance( const G & g ,
                                 int r , Q & S , VisitCounts * c = nullptr );

/// the path from r to s in the tree of the parents p, empty if there is
/// none (the printPath() of the book)
/// \pre r and s are places of p
/// \post the path is empty or goes from r to s
inline std::vector< int > path( int r , int s , const std::vector< int > & p );

/*--------------------------------------------------------------------------*/
/*--------------------------------- DFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// dfs(): pre( u ) in the previsit, edge( u , v ) on each edge, post( u ) in
/// the postvisit
/// \pre u is a node not visited, and visited has a place per name
template< Graph G , class Pre , class Edge , class Post >
void dfs( const G & g , int u , std::vector< bool > & visited , Pre && pre ,
          Edge && edge , Post && post );

/// cc(): the component of each node, numbered from 1, the visits starting
/// from the nodes as they leave the stack S (its last item first)
/// \pre the items of S are nodes (audit)
template< Graph G >
std::vector< int > cc( const G & g , std::vector< int > S );

/// cc() starting the visits from the nodes in increasing order
/// \post each node has a component (audit)
template< Graph G >
std::vector< int > cc( const G & g );

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

 explicit DfsSchema( const G & g );

 /// the visit from u; edge( u , v , k ) on each edge examined, of kind k
 /// \pre u is a node not discovered yet
 /// \post u is finished after it is discovered
 template< class Edge >
 void visit( int u , Edge && edge );

 /// the visits from all the nodes not discovered yet, in increasing order
 template< class Edge >
 void visitAll( Edge && edge );

 /// \pre u is a node
 [[nodiscard]] int dt( int u ) const;

 /// \pre u is a node
 [[nodiscard]] int ft( int u ) const;

/*--------------------------------------------------------------------------*/
 private:

 const G & f_g;
 std::vector< int > f_dt;   ///< the discovery times, 0 if not yet
 std::vector< int > f_ft;   ///< the finish times, 0 if not yet
 int f_time = 0;            ///< the clock

 };  // end( class DfsSchema )

/*--------------------------------------------------------------------------*/
/*--------------------- CYCLES, ORDERS AND COMPONENTS ----------------------*/
/*--------------------------------------------------------------------------*/

/// hasCycle(): true if the oriented graph has a cycle, i.e., if a visit
/// finds a back edge; on an unoriented graph each edge, met again from its
/// other end, is a back edge, so it is true if and only if there is an edge
template< Graph G >
bool hasCycle( const G & g );

/// whether S has each node once and each edge ( u , v ) has u before v
template< Graph G >
bool isTopologicalOrder( const G & g , const std::vector< int > & S );

/// topsort(): the nodes by decreasing finish time, which for an acyclic
/// graph is a topological order; the first is the top of the stack of the
/// book
/// \post a topological order if the graph is acyclic (audit)
template< Graph G >
std::vector< int > topsort( const G & g );

/// the topological order by removing the nodes with no entering edge, in
/// the order of V() and then of discovery; fewer than n nodes if there is
/// a cycle
/// \post a topological order if it has all the nodes (audit)
template< Graph G >
std::vector< int > topsortKahn( const G & g );

/// the transposed graph, with the edges reversed
/// \post ( v , u ) is an edge of it for each edge ( u , v ) (audit)
template< Graph G >
ListGraph<> transpose( const G & g );

/// scc(): the strongly connected component of each node, numbered from 1
/// \post each node has a component
template< Graph G >
std::vector< int > scc( const G & g );

/// true if every two nodes have a path between them in one direction
template< Graph G >
bool unilaterallyConnected( const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Visit.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Visit.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Visit.h ----------------------------*/
/*--------------------------------------------------------------------------*/
