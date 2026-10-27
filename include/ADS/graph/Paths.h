/*--------------------------------------------------------------------------*/
/*------------------------------ File Paths.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Problems on the paths of a GRAPH solved by the visits of section 9.5 and by
 * the topological order, each in \f$ O( n + m ) \f$ time on lists or vectors
 * of adjacency unless said otherwise:
 *
 * - `countShortestPaths()`, the number of distinct shortest paths, in edges,
 *   from s to each node: a visit in breadth in which a node discovered from
 *   u inherits the count of u and adds that of every other u' with an edge
 *   ( u' , v ) and d[ u' ] + 1 = d[ v ], right since all the nodes at
 *   distance d - 1 leave the queue before those at distance d. The counts
 *   grow as \f$ 2^{n/3} \f$ on a chain of diamonds, and are given modulo
 *   \f$ 2^{64} \f$. They are the first pass of the algorithm of Brandes for
 *   the betweenness centrality, which adds a second pass in the reverse
 *   order of the visit;
 * - `multiSourceDistance()`, the distance from the nearest of a set S of
 *   sources, all of them at distance 0 in the queue, and the tree of the
 *   shortest paths as a forest rooted at the sources; it is the visit from
 *   a new node joined to the sources, without the node. `setDistance()` is
 *   the distance between two sets of nodes, the visit from the first one
 *   stopping at the first node of the second, 0 if they meet;
 * - `longestPathLayering()`, for each node of an acyclic graph the edges of
 *   a longest path ending at it, by the algorithm of Kahn: a node gets its
 *   layer when its last entering edge is removed, one more than the
 *   largest layer of its predecessors. The layers are antichains, and by
 *   the theorem of Mirsky (the dual of that of Dilworth) their number, one
 *   more than the longest path, is the least number of antichains that
 *   cover the nodes, e.g., the fewest rounds for jobs with precedences, or
 *   the levels of the drawing of Sugiyama, Tagawa and Toda. A cycle leaves
 *   some node with an entering edge forever, and breaks the precondition;
 * - `dagLongestPaths()`, for each node the largest total length of a path
 *   leaving it, along the edges kept by a predicate (all of them by
 *   default), with the weights of the graph as lengths (1 each if it has
 *   none): L[ u ] is the largest of 0 and of w( u , v ) + L[ v ] over the
 *   kept edges ( u , v ), by a memoized visit in depth, iterative, of the
 *   graph of the kept edges, which must be acyclic. With the predicate an
 *   unoriented graph gives an acyclic one, e.g., keeping the edges that go
 *   downhill on a map of altitudes. It is the critical path method of
 *   Kelley and Walker; `dagShortestPaths()` of `ShortestPaths.h` with the
 *   lengths negated gives the same lengths from one source, while the
 *   longest path of a graph with cycles is NP-hard (section 18.4);
 * - `simpleCycles()`, the simple oriented cycles through s, by the algorithm
 *   of Johnson: a visit in depth that blocks the nodes on its stack and keeps
 *   them blocked, with the lists B of the nodes waiting on them, while no path
 *   from them to s avoids the stack, so that no branch is explored without a
 *   cycle at its end. It takes \f$ O( ( n + m ) ( c + 1 ) ) \f$ time for c
 *   cycles, against the backtracking of section 16, which tries every simple
 *   path from s and may take exponential time between two cycles; the bound of
 *   Tarjan's algorithm of 1973 is \f$ O( n m ( c + 1 ) ) \f$. The complete
 *   digraph of n nodes has \f$ \lfloor e ( n - 1 )! \rfloor - 1 \f$ cycles
 *   through a node;
 * - `refuelingStops()`, the fewest stops of a trip from 0 to L along a line
 *   with stations at D, where a full tank lasts R: the greedy of chapter 14
 *   that, when the next station is out of reach, stops at the last one in
 *   reach. It is a shortest path in the graph whose edges join the points at
 *   distance at most R, which the greedy finds in \f$ O( n ) \f$ by the
 *   exchange argument: a trip with fewer stops would get past the farthest
 *   point of the greedy at some stop, and so would the greedy.
 *
 * Nil is `NIL` and the infinite distance `INF` [see `Visit.h`].
 *
 * The contracts [see `Contract.h`]: the sources and the starts are nodes, the
 * sets are characteristic vectors with a place per name, the graph of
 * `longestPathLayering()` and the edges kept by `dagLongestPaths()` make no
 * cycle (checked as they go), `simpleCycles()` needs an oriented graph, and
 * `refuelingStops()` a sorted D in [ 0 , L ] with L , R >= 0. At the level of
 * the audits `countShortestPaths()` ensures that its counts satisfy their
 * recurrence on the levels of `distance()`, `multiSourceDistance()` that its
 * distances are those of a breadth first forest, `longestPathLayering()` and
 * `dagLongestPaths()` that each edge respects their lengths and that each
 * length is reached by an edge, and `refuelingStops()` that its stops make
 * the trip. The functions are defined in `Paths.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 9.5 and
 *      14, https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see A. B. Kahn, Topological sorting of large networks, Communications of
 *      the ACM 5(11), 558-562, 1962, doi:10.1145/368996.369025
 * \see U. Brandes, A faster algorithm for betweenness centrality, Journal
 *      of Mathematical Sociology 25(2), 163-177, 2001,
 *      doi:10.1080/0022250X.2001.9990249
 * \see L. Mirsky, A dual of Dilworth's decomposition theorem, The American
 *      Mathematical Monthly 78(8), 876-877, 1971, doi:10.2307/2316481
 * \see K. Sugiyama, S. Tagawa, M. Toda, Methods for visual understanding of
 *      hierarchical system structures, IEEE Transactions on Systems, Man,
 *      and Cybernetics 11(2), 109-125, 1981,
 *      doi:10.1109/TSMC.1981.4308636
 * \see J. E. Kelley, M. R. Walker, Critical-path planning and scheduling,
 *      Proceedings of the Eastern Joint Computer Conference, 160-173, 1959,
 *      doi:10.1145/1460299.1460318
 * \see D. B. Johnson, Finding all the elementary circuits of a directed
 *      graph, SIAM Journal on Computing 4(1), 77-84, 1975,
 *      doi:10.1137/0204007
 * \see R. E. Tarjan, Enumeration of the elementary circuits of a directed
 *      graph, SIAM Journal on Computing 2(3), 211-216, 1973,
 *      doi:10.1137/0202017
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

#ifndef __ADS_Paths
 #define __ADS_Paths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Visit.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the length of an edge of \f$ G \f$: its weight, or 1 if it has none
template< class G >
struct EdgeLength { using type = int; };

template< class G >
 requires requires { typename G::Weight; } &&
          ( ! std::is_same_v< typename G::Weight , std::monostate > )
struct EdgeLength< G > { using type = typename G::Weight; };

}  // end( namespace detail )

/// the type of the lengths of the paths of \f$ G \f$
template< class G >
using PathLength = typename detail::EdgeLength< G >::type;

/*--------------------------------------------------------------------------*/
/*----------------------------- SHORTEST PATHS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the number of shortest paths, in edges, from
/// \f$ s \f$ to each node, modulo \f$ 2^{64} \f$: 1 for \f$ s \f$, 0 for the
/// nodes not reachable
///
/// In pseudocode: all the nodes at distance d - 1 leave the queue before those
/// at distance d, so the count of v is complete when v leaves it.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countShortestPaths}( G , s ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty;
///     \ \sigma[ v ] \gets 0 \cr
/// 3 & \quad d[ s ] \gets 0;\ \sigma[ s ] \gets 1;\ Q \gets ( s ) \cr
/// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
/// 6 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 7 & \qquad\quad \textbf{if}\ d[ v ] = \infty\ \textbf{then}\ d[ v ] \gets
///     d[ u ] + 1;\ \mathrm{enqueue}( Q , v ) \cr
/// 8 & \qquad\quad \textbf{if}\ d[ v ] = d[ u ] + 1\ \textbf{then}\ \sigma[ v
///     ] \gets ( \sigma[ v ] + \sigma[ u ] ) \bmod 2^{64} \cr
/// 9 & \quad \textbf{return}\ \sigma
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is a node
/// \post the counts satisfy their recurrence on the levels of `distance()`
///       (audit)
template< Graph G >
std::vector< std::uint64_t > countShortestPaths( const G & g , int s );

/// the distance from the nearest node of \f$ S \f$,
/// `INF` if none reaches it, and the parents in the forest of the shortest
/// paths, `NIL` for the nodes of \f$ S \f$ and those not reached
///
/// In pseudocode, the visit in breadth from all the sources at once:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiSourceDistance}( G , S ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty;\ p[
///     v ] \gets \textbf{nil} \cr
/// 3 & \quad Q \gets () \cr
/// 4 & \quad \textbf{foreach}\ s \in S\ \textbf{with}\ d[ s ] = \infty
///     \ \textbf{do}\ d[ s ] \gets 0;\ \mathrm{enqueue}( Q , s ) \cr
/// 5 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}\ d[ v
///     ] = \infty\ \textbf{do} \cr
/// 8 & \qquad\quad d[ v ] \gets d[ u ] + 1;\ p[ v ] \gets u;
///     \ \mathrm{enqueue}( Q , v ) \cr
/// 9 & \quad \textbf{return}\ ( d , p )
/// \end{array}}
/// \f]
/// \pre the items of \f$ S \f$ are nodes
/// \post 0 on \f$ S \f$, and each node reached but those of \f$ S \f$ one edge
///       farther than its parent, with no edge from a reached node farther
///       (audit)
template< Graph G >
std::pair< std::vector< int > , std::vector< int > >
multiSourceDistance( const G & g , const std::vector< int > & S );

/// the fewest edges of a path from a node of \f$ A \f$ to one
/// of \f$ B \f$, 0 if they meet, `INF` if there is none
///
/// In pseudocode, the visit from A stopping at the first node of B:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setDistance}( G , A , B ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty \cr
/// 3 & \quad Q \gets () \cr
/// 4 & \quad \textbf{foreach}\ a \in A\ \textbf{do}\ d[ a ] \gets 0;
///     \ \mathrm{enqueue}( Q , a ) \cr
/// 5 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad \textbf{if}\ u \in B\ \textbf{then return}\ d[ u ] \cr
/// 8 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}\ d[ v
///     ] = \infty\ \textbf{do} \cr
/// 9 & \qquad\quad d[ v ] \gets d[ u ] + 1;\ \mathrm{enqueue}( Q , v ) \cr
/// 10 & \quad \textbf{return}\ \infty
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ have a place per name
template< Graph G >
int setDistance( const G & g , const std::vector< bool > & A ,
                 const std::vector< bool > & B );

/*--------------------------------------------------------------------------*/
/*----------------------------- LONGEST PATHS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the edges of a longest path ending at each node
/// of the acyclic graph, 0 on the sources (and on the names that are not
/// nodes)
///
/// In pseudocode (the algorithm of Kahn): a node gets its layer when its last
/// entering edge is removed.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestPathLayering}( G ) \cr
/// 2 & \quad \textbf{foreach}\ ( u , v ) \in E\ \textbf{do}\ \mathit{in}[ v ]
///     \gets \mathit{in}[ v ] + 1 \cr
/// 3 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ \mathit{layer}[ v ] \gets
///     0 \cr
/// 4 & \quad S \gets \text{the nodes with } \mathit{in} = 0 \cr
/// 5 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad u \gets \mathrm{pop}( S ) \cr
/// 7 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 8 & \qquad\quad \mathit{layer}[ v ] \gets \max( \mathit{layer}[ v ] ,
///     \mathit{layer}[ u ] + 1 );\ \mathit{in}[ v ] \gets \mathit{in}[ v ] - 1
///     \cr
/// 9 & \qquad\quad \textbf{if}\ \mathit{in}[ v ] = 0\ \textbf{then}
///     \ \mathrm{push}( S , v ) \cr
/// 10 & \quad \textbf{return}\ \mathit{layer}
/// \end{array}}
/// \f]
/// \pre the graph is acyclic
/// \post each edge \f$ ( u , v ) \f$ has
///       \f$ \mathit{layer}[ v ] > \mathit{layer}[ u ] \f$, and each node of a
///       positive layer a predecessor in the layer before (audit)
template< Graph G >
std::vector< int > longestPathLayering( const G & g );

/// the largest total length of a path leaving each node
/// along the edges \f$ ( u , v ) \f$ with \f$ \mathrm{keep}( u , v ) \f$, the
/// empty path included, 0 on the names that are not nodes
///
/// In pseudocode (the critical path method), recursive with memoization, while
/// the code keeps the recursion on a stack of its own:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ L( u ) \cr
/// 2 & \quad \textbf{if}\ L[ u ] \text{ is known }\ \textbf{then return}\ L[ u
///     ] \cr
/// 3 & \quad L[ u ] \gets 0 \cr
/// 4 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{with}
///     \ \mathrm{keep}( u , v )\ \textbf{do} \cr
/// 5 & \qquad L[ u ] \gets \max( L[ u ] , w( u , v ) + L( v ) ) \cr
/// 6 & \quad \textbf{return}\ L[ u ]
/// \end{array}}
/// \f]
/// \pre the edges kept make no cycle
/// \post \f$ L[ u ] \ge w( u , v ) + L[ v ] \f$ on each edge kept, and each
///       \f$ L[ u ] \f$ is 0 or reached by one of them (audit)
template< Graph G , class Keep >
std::vector< PathLength< G > > dagLongestPaths( const G & g , Keep && keep );

/// the longest paths of `dagLongestPaths()` above, along all the edges
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dagLongestPaths}( G ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{dagLongestPaths}( G , \text{a }
///     \text{predicate true on every edge} )
/// \end{array}}
/// \f]
/// \pre the graph is acyclic
template< Graph G >
std::vector< PathLength< G > > dagLongestPaths( const G & g );

/*--------------------------------------------------------------------------*/
/*--------------------------------- CYCLES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// calls \f$ f( C ) \f$ on each simple cycle through \f$ s \f$,
/// \f$ C \f$ its nodes from \f$ s \f$ back to \f$ s \f$
///
/// In pseudocode (Johnson's algorithm, beyond the book): a node stays blocked,
/// with the nodes waiting on it in its list B, while no path from it to s
/// avoids the stack.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{circuit}( v ) \cr
/// 2 & \quad \mathit{found} \gets \textbf{false};\ \mathrm{push}(
///     \mathit{stack} , v );\ \mathit{blocked}[ v ] \gets \textbf{true} \cr
/// 3 & \quad \textbf{foreach}\ x \in G.\mathrm{adj}( v )\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x = s\ \textbf{then}\ f( \mathit{stack} , s );
///     \ \mathit{found} \gets \textbf{true} \cr
/// 5 & \qquad \textbf{else if not}\ \mathit{blocked}[ x ]\ \textbf{and}
///     \ \mathrm{circuit}( x )\ \textbf{then}\ \mathit{found} \gets
///     \textbf{true} \cr
/// 6 & \quad \textbf{if}\ \mathit{found}\ \textbf{then}\ \mathrm{unblock}( v )
///     \cr
/// 7 & \quad \textbf{else foreach}\ x \in G.\mathrm{adj}( v )\ \textbf{do}\ B[
///     x ] \gets B[ x ] \cup \{ v \} \cr
/// 8 & \quad \mathrm{pop}( \mathit{stack} );\ \textbf{return}\ \mathit{found}
///     \cr
/// 9 & \textbf{procedure}\ \mathrm{unblock}( u ) \cr
/// 10 & \quad \mathit{blocked}[ u ] \gets \textbf{false} \cr
/// 11 & \quad \textbf{foreach}\ x \in B[ u ]\ \textbf{do}\ B[ u ] \gets B[ u ]
///     \setminus \{ x \};\ \textbf{if}\ \mathit{blocked}[ x ]\ \textbf{then}
///     \ \mathrm{unblock}( x )
/// \end{array}}
/// \f]
/// \pre the graph is oriented, \f$ s \f$ is a node
template< Graph G , class F >
void simpleCycles( const G & g , int s , F && f );

/*--------------------------------------------------------------------------*/
/*--------------------------------- A LINE ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the places in \f$ D \f$ of the fewest stops of a trip
/// from 0 to \f$ L \f$, a full tank lasting \f$ R \f$ and the stations at
/// \f$ D \f$, nothing if the trip cannot be made
///
/// In pseudocode (a greedy of chapter 14): when the next station is out of
/// reach, the last station in reach is the stop.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{refuelingStops}( D , L , R ) \cr
/// 2 & \quad S \gets ();\ \mathit{reach} \gets R;\ i \gets 1 \cr
/// 3 & \quad \textbf{while}\ \mathit{reach} < L\ \textbf{do} \cr
/// 4 & \qquad b \gets \textbf{nil} \cr
/// 5 & \qquad \textbf{while}\ i \le |D|\ \textbf{and}\ D[ i ] \le
///     \mathit{reach}\ \textbf{do}\ b \gets i;\ i \gets i + 1 \cr
/// 6 & \qquad \textbf{if}\ b = \textbf{nil}\ \textbf{then return nil} \cr
/// 7 & \qquad \text{append } b \text{ to } S;\ \mathit{reach} \gets D[ b ] + R
///     \cr
/// 8 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ sorted in \f$ [ 0 , L ] \f$, \f$ L \ge 0 \f$,
///      \f$ R \ge 0 \f$
/// \post the stops increase, and no leg is longer than \f$ R \f$ (audit)
template< class T >
std::optional< std::vector< std::size_t > > refuelingStops(
                                      const std::vector< T > & D , T L , T R );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Paths.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Paths.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Paths.h ----------------------------*/
/*--------------------------------------------------------------------------*/
