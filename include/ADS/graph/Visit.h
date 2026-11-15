/*--------------------------------------------------------------------------*/
/*------------------------------ File Visit.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The visits of a GRAPH of section 9.5 and their applications, each in
 * \f$ \Theta( n + m ) \f$ time on lists or vectors of adjacency:
 *
 * - `genericVisit()`, the general scheme of which the others are cases: the
 *   reached nodes R and the open ones Q start as { r }, the tree B as
 *   empty; while Q is not empty a node j is chosen from Q, and if some k
 *   adjacent to j is not in R then k joins R and Q and ( j , k ) joins B,
 *   otherwise j leaves Q. Choosing the newest node of Q gives the visit in
 *   depth, the oldest the visit in breadth; B is a tree spanning the nodes
 *   reachable from r. A cursor in the adjacency of each node makes each
 *   edge examined once;
 * - `bfs()`, the visit in breadth with a QUEUE, which examines the nodes by
 *   increasing distance from the root r; `distance()` computes the distance
 *   (the minimum number of edges, the Erdos number of the example) and the
 *   tree of the shortest paths as a vector of parents, and `path()` reads a
 *   path from it, as printPath of the book does; `distance()` runs on an
 *   `ArrayQueue` or on any queue given, with `enqueue()` and `dequeue()` [see
 *   `VisitCounts`];
 * - `dfs()`, the visit in depth, recursive, with a previsit and a postvisit;
 * - `cc()`, the connected components of an unoriented graph, starting the
 *   visits from the nodes in the order in which they leave a STACK;
 * - `DfsSchema`, the procedure dfs-schema of the book, which numbers the
 *   discovery time dt[ u ] and the finish time ft[ u ] of each node and says
 *   of each edge ( u , v ) whether it is in the tree T (dt[ v ] = 0), back
 *   (dt[ u ] > dt[ v ] and ft[ v ] = 0), forward (dt[ u ] < dt[ v ] and
 *   ft[ v ] != 0) or cross (the others);
 * - `hasCycle()`, true if and only if a visit finds a back edge;
 * - `topsort()`, the nodes by decreasing finish time, a topological order of
 *   an acyclic graph: if ( u , v ) is an edge then ft[ v ] < ft[ u ];
 *   `topsortKahn()` builds one by removing the nodes with no entering edge,
 *   and `isTopologicalOrder()` checks one;
 * - `unilaterallyConnected()`, true if for every two nodes u and v there is
 *   a path from u to v or one from v to u: so it is exactly when the graph
 *   of the strongly connected components, which is acyclic, has a path
 *   through all of them, i.e., when its topological order is unique, which
 *   the algorithm of Kahn finds out by never having two sources at once;
 * - `transpose()` and `scc()`, the strongly connected components of an
 *   oriented graph by the algorithm of Kosaraju: a visit gives the nodes by
 *   decreasing finish time, and `cc()` on the transposed graph starts its
 *   visits in that order. By Theorem 9.1, if there is an edge from the
 *   component C to C' then ft( C ) > ft( C' ), so the second visit meets C
 *   first and, the edge being reversed, cannot reach C' from it;
 * - `sccTarjan()` and `sccPearce()`, the same components by one visit in
 *   depth and no transposed graph (beyond the book): the algorithm of
 *   Tarjan keeps the order of discovery and the lowest one reachable
 *   through the tree and one more edge of each node, and pops a component
 *   off a stack when its first node finishes with the two equal; the
 *   variant of Pearce keeps one integer per node instead of three, the two
 *   numbers sharing a place and the closed components taking numbers that
 *   count down from n - 1 above those in use, and one bit per node. Both
 *   number the components as `scc()` does, in a topological order of the
 *   graph of the components.
 *
 * Nil is -1 and the infinite distance is `INF`. The visits in depth and
 * `path()` keep their recursion on an explicit stack, in the order of the
 * recursive ones, so that a path of 10^5 nodes does not overflow the
 * stack of the calls.
 *
 * The properties on which the visits rest, and the arguments behind their
 * bounds, are in the theory page of the area (see \ref theory_graph_visits).
 *
 * The contracts [see `Contract.h`]: a visit starts from a node of the graph
 * (and `dfs()` from one not visited yet, with a vector of a place per name);
 * `distance()` ensures, at the level of the audits, that its distances are
 * those of a breadth first tree, `topsort()` and `topsortKahn()` that their
 * order is topological when the graph is acyclic, `transpose()` that it
 * reverses each edge. The functions are defined in `Visit.tpp`.
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
 * \see D. J. Pearce, A space-efficient algorithm for finding strongly
 *      connected components, Information Processing Letters 116(1), 47-52,
 *      2016, doi:10.1016/j.ipl.2015.08.010
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
#include <cstddef>
#include <deque>
#include <limits>
#include <set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the infinite distance
inline constexpr int INF = std::numeric_limits< int >::max();

/// the node of \f$ Q \f$ that `genericVisit()` chooses
enum class Choice {
 NEWEST ,  ///< the last one added: the visit in depth
 OLDEST   ///< the first one added: the visit in breadth
 };

/// no node
inline constexpr int NIL = -1;

/*--------------------------------------------------------------------------*/
/*--------------------------------- BFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the general visit from \f$ r \f$: the nodes in the order in which they join
/// \f$ R \f$, and the edges of the tree \f$ B \f$ in the order in which they
/// join it
///
/// In pseudocode (section 9.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{genericVisit}( G , r )\quad
///     \triangleright\ R \text{ reached, } Q \text{ open, } B
///     \text{ the tree} \cr
/// 2 & \quad R \gets \{ r \};\ Q \gets \{ r \};\ B \gets \emptyset \cr
/// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad j \gets \text{a node of } Q\quad \triangleright
///     \ \text{the last added for depth, the first for breadth} \cr
/// 5 & \qquad \textbf{if}\ G.\mathrm{adj}( j ) \subseteq R\ \textbf{then}\ Q
///     \gets Q \setminus \{ j \}\quad \triangleright\ j \text{ is closed} \cr
/// 6 & \qquad \textbf{else}\ k \gets \text{a node of } G.\mathrm{adj}( j )
///     \setminus R;\ R \gets R \cup \{ k \};\ Q \gets Q \cup \{ k \};\ B
///     \gets B \cup \{ ( j , k ) \} \cr
/// 7 & \quad \textbf{return}\ ( R , B )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node
/// \post \f$ |B| = |R| - 1 \f$, each edge of \f$ B \f$ from a node already in
///       \f$ R \f$ (audit)
template< Graph G >
std::pair< std::vector< int > , std::vector< std::pair< int , int > > >
genericVisit( const G & g , int r , Choice c );

/// the visit in breadth from \f$ r \f$, calling `node( u )` when \f$ u \f$ is
/// examined and `edge( u , v )` when \f$ ( u , v ) \f$ is examined
///
/// In pseudocode (section 9.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bfs}( G , r )\quad \triangleright
///     \ \text{a node is marked when it enters } Q \cr
/// 2 & \quad \mathit{visited}[ r ] \gets \textbf{true};\ Q \gets
///     \text{a queue holding } r \cr
/// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathrm{dequeue}( Q );\ \mathrm{node}( u ) \cr
/// 5 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 6 & \qquad\quad \mathrm{edge}( u , v ) \cr
/// 7 & \qquad\quad \textbf{if}\ \textbf{not}\ \mathit{visited}[ v ]
///     \ \textbf{then}\ \mathit{visited}[ v ] \gets \textbf{true};
///     \ \mathrm{enqueue}( Q , v )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node
template< Graph G , class FN , class FE >
void bfs( const G & g , int r , FN && node , FE && edge );

/// the distances from \f$ r \f$, `INF` if not reachable, and the
/// parents in the tree of the shortest paths, `NIL` for \f$ r \f$ and the
/// unreachable nodes
///
/// In pseudocode (section 9.5), each node entering the queue once:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{distance}( G , r )\quad \triangleright
///     \ \text{the visit in breadth, } d \text{ in place of the marks} \cr
/// 2 & \quad d[ v ] \gets \infty;\ p[ v ] \gets \textbf{nil}
///     \text{ for every node } v \cr
/// 3 & \quad d[ r ] \gets 0;\ Q \gets \text{a queue holding } r \cr
/// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
/// 6 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u ) \text{ with } d[ v
///     ] = \infty\ \textbf{do}\ ( d[ v ] , p[ v ] ) \gets ( d[ u ] + 1 , u
///     );\ \mathrm{enqueue}( Q , v ) \cr
/// 7 & \quad \textbf{return}\ ( d , p )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node
/// \post each edge \f$ ( u , v ) \f$ from a reached \f$ u \f$ has
///       \f$ d[ v ] \le d[ u ] + 1 \f$, and each reached \f$ v \ne r \f$ has
///       \f$ d[ v ] = d[ p[ v ] ] + 1 \f$ (audit)
template< Graph G >
std::pair< std::vector< int > , std::vector< int > > distance( const G & g ,
							      int r );

/// what a visit did: the nodes taken out of its structure and the edges
/// examined
struct VisitCounts
{
 long long nodes = 0 , edges = 0;
 };

/// the distances of `distance()` above, with the queue \f$ S \f$ given, empty,
/// whose `enqueue()` and `dequeue()` keep the order of arrival [see
/// `ArrayQueue.h`]; its work in `*c`, if given
/// \pre \f$ r \f$ is a node, \f$ S \f$ is empty
/// \post as `distance()`
template< Graph G , class Q >
 requires requires( Q q , int v )
{
  q.enqueue( v );
  { q.dequeue() } -> std::convertible_to< int >;
  }
std::pair< std::vector< int > , std::vector< int > >
distance( const G & g , int r , Q & S , VisitCounts * c = nullptr );

/// the path from \f$ r \f$ to \f$ s \f$ in the tree of the parents \f$ p \f$,
/// empty if there is none (the procedure printPath of the book)
///
/// In pseudocode (section 9.5), the path read backwards from s:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{path}( r , s , p )\quad \triangleright
///     \ \text{from } s \text{ back to } r \text{ along } p \cr
/// 2 & \quad P \gets ( s ) \cr
/// 3 & \quad \textbf{while}\ \text{the head } x \text{ of } P
///     \text{ is not } r\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ p[ x ] = \textbf{nil}\ \textbf{then return}
///     \ ()\quad \triangleright\ s \text{ is not reached from } r \cr
/// 5 & \qquad \text{put } p[ x ] \text{ before the head of } P \cr
/// 6 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ and \f$ s \f$ are places of \f$ p \f$
/// \post the path is empty or goes from \f$ r \f$ to \f$ s \f$
inline std::vector< int > path( int r , int s , const std::vector< int > & p );

/*--------------------------------------------------------------------------*/
/*--------------------------------- DFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the visit in depth from \f$ u \f$, calling `pre( u )` in the previsit,
/// `edge( u , v )` on each edge and `post( u )` in the postvisit
///
/// In pseudocode (section 9.5), recursive as in the book, while the code keeps
/// the recursion on a stack of its own:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dfs}( G , u , \mathit{visited} ) \cr
/// 2 & \quad \mathit{visited}[ u ] \gets \textbf{true};\ \mathrm{pre}( u
///     )\quad \triangleright\ \text{on the way down} \cr
/// 3 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do}
///     \ \mathrm{edge}( u , v );\ \textbf{if}\ \textbf{not}
///     \ \mathit{visited}[ v ]\ \textbf{then}\ \mathrm{dfs}( G , v ,
///     \mathit{visited} ) \cr
/// 4 & \quad \mathrm{post}( u )\quad \triangleright\ \text{on the way up}
/// \end{array}}
/// \f]
/// and on the stack \f$ S \f$ of the code, a frame \f$ ( x , A , i ) \f$
/// holding a node, its adjacent nodes and the place of the next one:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dfs}( G , u , \mathit{visited} ) \cr
/// 2 & \quad \mathit{visited}[ u ] \gets \textbf{true};\ \mathrm{pre}( u );
///     \ S \gets ( ( u , G.\mathrm{adj}( u ) , 0 ) ) \cr
/// 3 & \quad \textbf{while}\ S \ne ()\ \textbf{do} \cr
/// 4 & \qquad ( x , A , i ) \gets \text{the top of } S \cr
/// 5 & \qquad \textbf{if}\ i = \lvert A \rvert\ \textbf{then}
///     \ \text{pop } S;\ \mathrm{post}( x );\ \textbf{continue}
///     \quad \triangleright\ \text{the return} \cr
/// 6 & \qquad v \gets A[ i ];\ i \gets i + 1 \text{ in the top frame};
///     \ \mathrm{edge}( x , v ) \cr
/// 7 & \qquad \textbf{if not}\ \mathit{visited}[ v ]\ \textbf{then}
///     \quad \triangleright\ \text{the call} \cr
/// 8 & \qquad\quad \mathit{visited}[ v ] \gets \textbf{true};
///     \ \mathrm{pre}( v );\ \text{push } ( v , G.\mathrm{adj}( v ) , 0 )
///     \text{ on } S
/// \end{array}}
/// \f]
/// \pre \f$ u \f$ is a node not visited, and `visited` has a place per name
template< Graph G , class Pre , class Edge , class Post >
void dfs( const G & g , int u , std::vector< bool > & visited , Pre && pre ,
	  Edge && edge , Post && post );

/// the component of each node, numbered from 1, the visits starting
/// from the nodes as they leave the stack \f$ S \f$ (its last item first)
///
/// In pseudocode (section 9.5), each visit numbering the nodes it reaches:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cc}( G , S )\quad \triangleright\ S
///     \text{ the order of the roots, } 0 \text{ an unlabeled node} \cr
/// 2 & \quad \mathit{id}[ v ] \gets 0 \text{ for every node } v;\ c \gets 0
///     \cr
/// 3 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathrm{pop}( S );\ \textbf{if}\ \mathit{id}[ u ] \ne
///     0\ \textbf{then continue} \cr
/// 5 & \qquad c \gets c + 1;\ \mathit{id}[ u ] \gets c;\ T \gets
///     \text{a stack holding } u\quad \triangleright\ \text{a new component}
///     \cr
/// 6 & \qquad \textbf{while}\ T \ne \emptyset\ \textbf{do} \cr
/// 7 & \qquad\quad x \gets \mathrm{pop}( T ) \cr
/// 8 & \qquad\quad \textbf{foreach}\ v \in G.\mathrm{adj}( x ) \text{ with }
///     \mathit{id}[ v ] = 0\ \textbf{do}\ \mathit{id}[ v ] \gets c;
///     \ \mathrm{push}( T , v ) \cr
/// 9 & \quad \textbf{return}\ \mathit{id}
/// \end{array}}
/// \f]
/// \pre the items of \f$ S \f$ are nodes (audit)
template< Graph G >
std::vector< int > cc( const G & g , std::vector< int > S );

/// the components of `cc()` above, the visits starting from the nodes in
/// increasing order
///
/// It is cc( G , S ) with S the nodes in decreasing order, so that the
/// smallest one leaves S first.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cc}( G ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{cc}( G , \text{the nodes of } G \text{ }
///     \text{in decreasing order} )
/// \end{array}}
/// \f]
/// \post each node has a component (audit)
template< Graph G >
std::vector< int > cc( const G & g );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DfsSchema ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the kind of an edge with respect to a depth-first visit
enum class EdgeKind {
 TREE ,     ///< an edge of the tree of the visit, to a node not discovered
 BACK ,     ///< an edge to an ancestor, not finished yet
 FORWARD ,  ///< an edge to a descendant, already finished
 CROSS     ///< any other edge, between two subtrees or two trees
 };

/// the discovery and finish times and the kind of each edge, the
/// procedure dfs-schema of the book
template< Graph G >
class DfsSchema
{

/*--------------------------------------------------------------------------*/
 public:
 explicit DfsSchema( const G & g );

 /// the visit from \f$ u \f$; `edge( u , v , k )` on each edge examined, of
 /// kind \f$ k \f$
 ///
 /// In pseudocode (the procedure dfs-schema of section 9.5), with the clock t:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{visit}( u )\quad \triangleright
 ///     \ \mathit{dt} , \mathit{ft}
 ///     \text{ the discovery and the finish times, } 0 \text{ unset} \cr
 /// 2 & \quad t \gets t + 1;\ \mathit{dt}[ u ] \gets t \cr
 /// 3 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \mathit{dt}[ v ] = 0\ \textbf{then}
 ///     \ \mathrm{edge}( u , v , \text{tree} );\ \mathrm{visit}( v ) \cr
 /// 5 & \qquad \textbf{else if}\ \mathit{ft}[ v ] = 0\ \textbf{then}
 ///     \ \mathrm{edge}( u , v , \text{back} )\quad \triangleright\ v
 ///     \text{ is an open ancestor} \cr
 /// 6 & \qquad \textbf{else if}\ \mathit{dt}[ v ] > \mathit{dt}[ u ]
 ///     \ \textbf{then}\ \mathrm{edge}( u , v , \text{forward} )\quad
 ///     \triangleright\ \text{a closed descendant} \cr
 /// 7 & \qquad \textbf{else}\ \mathrm{edge}( u , v , \text{cross} ) \cr
 /// 8 & \quad t \gets t + 1;\ \mathit{ft}[ u ] \gets t
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ is a node not discovered yet
 /// \post \f$ u \f$ is finished after it is discovered
 template< class Edge >
 void visit( int u , Edge && edge );

 /// the visits from all the nodes not discovered yet, in increasing order
 template< class Edge >
 void visitAll( Edge && edge );

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] int dt( int u ) const;

 /// \pre \f$ u \f$ is a node
 [[nodiscard]] int ft( int u ) const;

/*--------------------------------------------------------------------------*/
 private:
 const G & f_g;
 std::vector< int > f_dt;  ///< the discovery times, 0 if not yet
 std::vector< int > f_ft;  ///< the finish times, 0 if not yet
 int f_time = 0;           ///< the clock

 };  // end( class DfsSchema )

/*--------------------------------------------------------------------------*/
/*--------------------- CYCLES, ORDERS AND COMPONENTS ----------------------*/
/*--------------------------------------------------------------------------*/

/// true if the oriented graph has a cycle, i.e., if a visit
/// finds a back edge; on an unoriented graph each edge, met again from its
/// other end, is a back edge, so it is true if and only if there is an edge
///
/// In pseudocode (section 9.5), on the visits of `DfsSchema`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hasCycle}( G )\quad \triangleright
///     \ \text{a cycle exists iff a visit in depth meets a back edge} \cr
/// 2 & \quad \textbf{foreach}\ u \text{ with } \mathit{dt}[ u ] = 0
///     \ \textbf{do}\ \mathrm{visit}( u )
///     \text{ of DfsSchema, stopping with } \textbf{true}
///     \text{ on a back edge} \cr
/// 3 & \quad \textbf{return false}
/// \end{array}}
/// \f]
template< Graph G >
bool hasCycle( const G & g );

/// whether \f$ S \f$ has each node once and each edge \f$ ( u , v ) \f$ has
/// \f$ u \f$ before \f$ v \f$
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isTopologicalOrder}( G , S ) \cr
/// 2 & \quad \textbf{if}\ |S| \ne n\ \textbf{then return false} \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ S[ i ] \text{ is not a node, or is repeated }
///     \ \textbf{then return false} \cr
/// 5 & \qquad \mathit{at}[ S[ i ] ] \gets i \cr
/// 6 & \quad \textbf{foreach}\ ( u , v ) \in E\ \textbf{do} \cr
/// 7 & \qquad \textbf{if}\ \mathit{at}[ u ] > \mathit{at}[ v ]\ \textbf{then
///     return false} \cr
/// 8 & \quad \textbf{return true}
/// \end{array}}
/// \f]
template< Graph G >
bool isTopologicalOrder( const G & g , const std::vector< int > & S );

/// the nodes by decreasing finish time, which for an acyclic
/// graph is a topological order; the first is the top of the stack of the
/// book
///
/// In pseudocode (section 9.5): since an edge ( u , v ) of an acyclic graph
/// has ft[ v ] < ft[ u ], the nodes by decreasing finish time come in a
/// topological order.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{topsort}( G ) \cr
/// 2 & \quad S \gets \text{an empty stack} \cr
/// 3 & \quad \textbf{foreach}\ u \text{ not yet visited}\ \textbf{do}
///     \ \mathrm{dfs}( G , u , \mathit{visited} ) \text{ with }
///     \mathrm{post}( v ) = \mathrm{push}( S , v ) \cr
/// 4 & \quad \textbf{return}\ \text{the nodes of } S
///     \text{ from the top}\quad \triangleright
///     \ \text{by decreasing finish time}
/// \end{array}}
/// \f]
/// \post a topological order if the graph is acyclic (audit)
template< Graph G >
std::vector< int > topsort( const G & g );

/// the topological order by removing the nodes with no entering edge, in the
/// order of `V()` and then of discovery; fewer than \f$ n \f$ nodes if there
/// is a cycle
///
/// In pseudocode (Kahn's algorithm, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{topsortKahn}( G ) \cr
/// 2 & \quad \textbf{foreach}\ ( u , v ) \in E\ \textbf{do}\ \mathit{in}[ v ]
///     \gets \mathit{in}[ v ] + 1 \cr
/// 3 & \quad L \gets \text{the nodes } u \text{ with } \mathit{in}[ u ] = 0
///     \text{ in increasing order};\ i \gets 1 \cr
/// 4 & \quad \textbf{while}\ i \le |L|\ \textbf{do} \cr
/// 5 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( L[ i ] )\ \textbf{do}
///     \cr
/// 6 & \qquad\quad \mathit{in}[ v ] \gets \mathit{in}[ v ] - 1 \cr
/// 7 & \qquad\quad \textbf{if}\ \mathit{in}[ v ] = 0\ \textbf{then}
///     \ \text{append } v \text{ to } L \cr
/// 8 & \qquad i \gets i + 1 \cr
/// 9 & \quad \textbf{return}\ L\quad \triangleright\ \text{fewer than } n
///     \text{ nodes if there is a cycle}
/// \end{array}}
/// \f]
/// \post a topological order if it has all the nodes (audit)
template< Graph G >
std::vector< int > topsortKahn( const G & g );

/// the transposed graph, with the edges reversed
///
/// In pseudocode (section 9.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{transpose}( G ) \cr
/// 2 & \quad \textbf{return}\ \text{the graph on the nodes of } G
///     \text{ with an edge } ( v , u ) \text{ for each edge } ( u , v )
///     \text{ of } G
/// \end{array}}
/// \f]
/// \post \f$ ( v , u ) \f$ is an edge of it for each edge \f$ ( u , v ) \f$
///       (audit)
template< Graph G >
ListGraph<> transpose( const G & g );

/// the strongly connected component of each node, numbered from 1
///
/// In pseudocode (Kosaraju's algorithm, section 9.5): if there is an edge from
/// the component C to C' then ft( C ) > ft( C' ), so the visits of the
/// transposed graph, started by decreasing finish time, meet C first and
/// cannot leave it.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{scc}( G ) \cr
/// 2 & \quad S \gets \mathrm{topsort}( G )\quad \triangleright
///     \ \text{the first node of } S \text{ finished last} \cr
/// 3 & \quad \textbf{return}\ \mathrm{cc}( \mathrm{transpose}( G ) , S
///     )\quad \triangleright\ \text{the roots taken from the top of } S
/// \end{array}}
/// \f]
/// \post each node has a component, and an edge from the component
///       \f$ C \f$ to \f$ C' \ne C \f$ gives \f$ C < C' \f$ (audit)
template< Graph G >
std::vector< int > scc( const G & g );

/// the strongly connected components of `scc()` by one visit in depth, with
/// no transposed graph, numbered as there
///
/// In pseudocode (Tarjan's algorithm, beyond the book):
/// \f$ \mathit{low}[ u ] \f$ is the least order of discovery of a node of
/// the stack \f$ S \f$ that the subtree of \f$ u \f$ reaches by one more
/// edge, and \f$ u \f$ is the first node of its component if and only if
/// \f$ \mathit{low}[ u ] = \mathit{dt}[ u ] \f$ when \f$ u \f$ finishes
/// (see \ref theory_graph_visits); the recursion is kept on an explicit
/// stack.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sccTarjan}( G ) \cr
/// 2 & \quad t \gets 0;\ k \gets 0;\ S \gets \text{an empty stack};
///     \ \mathit{dt}[ u ] \gets 0 , C[ u ] \gets 0\ \forall u \cr
/// 3 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ \mathit{dt}[ r ]
///     = 0\ \textbf{do}\ \mathrm{visit}( r ) \cr
/// 4 & \quad \textbf{return}\ C\ \text{with each } c \text{ replaced by }
///     k + 1 - c \cr
/// 5 & \textbf{procedure}\ \mathrm{visit}( u ) \cr
/// 6 & \quad t \gets t + 1;\ \mathit{dt}[ u ] \gets t;\ \mathit{low}[ u ]
///     \gets t;\ \mathrm{push}( S , u ) \cr
/// 7 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 8 & \qquad \textbf{if}\ \mathit{dt}[ v ] = 0\ \textbf{then}
///     \ \mathrm{visit}( v );\ \mathit{low}[ u ] \gets \min( \mathit{low}[ u ]
///     , \mathit{low}[ v ] ) \cr
/// 9 & \qquad \textbf{else if}\ C[ v ] = 0\ \textbf{then}\ \mathit{low}[ u ]
///     \gets \min( \mathit{low}[ u ] , \mathit{dt}[ v ] )\quad
///     \triangleright\ v \in S \cr
/// 10 & \quad \textbf{if}\ \mathit{low}[ u ] = \mathit{dt}[ u ]
///     \ \textbf{then} \cr
/// 11 & \qquad k \gets k + 1 \cr
/// 12 & \qquad \textbf{repeat}\ v \gets \mathrm{pop}( S );\ C[ v ] \gets k
///     \ \textbf{until}\ v = u
/// \end{array}}
/// \f]
/// \post each node has a component, and an edge from the component
///       \f$ C \f$ to \f$ C' \ne C \f$ gives \f$ C < C' \f$ (audit)
template< Graph G >
std::vector< int > sccTarjan( const G & g );

/// the strongly connected components of `sccTarjan()` with one integer and
/// one bit per node in place of three integers, numbered as `scc()` does
///
/// In pseudocode (Pearce's algorithm, beyond the book):
/// \f$ \mathit{rindex}[ u ] \f$ is the order of discovery of \f$ u \f$
/// until a node of a lower one is reached, which it then takes; a closed
/// component takes the number \f$ c \f$, which counts down from
/// \f$ N - 1 \f$ (\f$ N \f$ the `bound()`) and is never below a number
/// in use, since \f$ i \f$ gives back the numbers of the nodes closed. A
/// test of \f$ \mathit{rindex} \f$ alone thus tells a node of the stack
/// from a closed one (see \ref theory_graph_visits).
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sccPearce}( G ) \cr
/// 2 & \quad i \gets 1;\ c \gets N - 1;\ S \gets \text{an empty stack};
///     \ \mathit{rindex}[ u ] \gets 0\ \forall u \cr
/// 3 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ \mathit{rindex}[ r ]
///     = 0\ \textbf{do}\ \mathrm{visit}( r ) \cr
/// 4 & \quad \textbf{return}\ \mathit{rindex}[ u ] - c\ \forall u \cr
/// 5 & \textbf{procedure}\ \mathrm{visit}( u ) \cr
/// 6 & \quad \mathit{root}[ u ] \gets \textbf{true};\ \mathit{rindex}[ u ]
///     \gets i;\ i \gets i + 1 \cr
/// 7 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 8 & \qquad \textbf{if}\ \mathit{rindex}[ v ] = 0\ \textbf{then}
///     \ \mathrm{visit}( v ) \cr
/// 9 & \qquad \textbf{if}\ \mathit{rindex}[ v ] < \mathit{rindex}[ u ]
///     \ \textbf{then}\ \mathit{rindex}[ u ] \gets \mathit{rindex}[ v ];
///     \ \mathit{root}[ u ] \gets \textbf{false} \cr
/// 10 & \quad \textbf{if not}\ \mathit{root}[ u ]\ \textbf{then}
///     \ \mathrm{push}( S , u );\ \textbf{return} \cr
/// 11 & \quad i \gets i - 1 \cr
/// 12 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{and}
///     \ \mathit{rindex}[ u ] \le \mathit{rindex}[ \mathrm{top}( S ) ]
///     \ \textbf{do} \cr
/// 13 & \qquad \mathit{rindex}[ \mathrm{pop}( S ) ] \gets c;\ i \gets i - 1
///     \cr
/// 14 & \quad \mathit{rindex}[ u ] \gets c;\ c \gets c - 1
/// \end{array}}
/// \f]
/// \post each node has a component, and an edge from the component
///       \f$ C \f$ to \f$ C' \ne C \f$ gives \f$ C < C' \f$ (audit)
template< Graph G >
std::vector< int > sccPearce( const G & g );

/// true if every two nodes have a path between them in one direction
///
/// In pseudocode: the graph of the strongly connected components is acyclic,
/// and it has a path through all of them if and only if its topological order
/// is unique, i.e., if the algorithm of Kahn never has two sources at once.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{unilaterallyConnected}( G ) \cr
/// 2 & \quad C \gets \mathrm{scc}( G ) \cr
/// 3 & \quad H \gets \text{the graph with an edge } ( C[ u ] , C[ v ] )
///     \text{ for each } ( u , v ) \in E \text{ with } C[ u ] \ne C[ v ] \cr
/// 4 & \quad S \gets \text{the nodes of } H \text{ with no entering edge} \cr
/// 5 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ |S| > 1\ \textbf{then return false} \cr
/// 7 & \qquad c \gets \mathrm{pop}( S ) \cr
/// 8 & \qquad \textbf{foreach}\ d \in H.\mathrm{adj}( c )\ \textbf{do} \cr
/// 9 & \qquad\quad \mathit{in}[ d ] \gets \mathit{in}[ d ] - 1 \cr
/// 10 & \qquad\quad \textbf{if}\ \mathit{in}[ d ] = 0\ \textbf{then}
///     \ \mathrm{push}( S , d ) \cr
/// 11 & \quad \textbf{return true}
/// \end{array}}
/// \f]
template< Graph G >
bool unilaterallyConnected( const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Visit.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Visit.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Visit.h ----------------------------*/
/*--------------------------------------------------------------------------*/
