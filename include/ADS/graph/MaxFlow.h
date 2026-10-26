/*--------------------------------------------------------------------------*/
/*----------------------------- File MaxFlow.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The maximum flow of section 15.2 in a network
 * \f$ G = ( V , E , s , t , c ) \f$, with integer capacities, and the minimum
 * cut. The network keeps for each edge its residual twin, so that the residual
 * capacity \f$ c_f( u , v ) = c( u , v ) - f( u , v ) \f$ of both directions
 * is at hand. By Theorem 15.1 (maximum flow / minimum cut) f is maximum if and
 * only if there is no augmenting path, a path from s to t in the residual
 * network, and then the nodes S reached from s give a cut ( S , V - S ) of
 * capacity |f|. The local search augments the flow along augmenting paths:
 *
 * - `FORD_FULKERSON`, any path, by a depth first visit:
 *   \f$ O( ( n + m ) |f^*| ) \f$;
 * - `EDMONDS_KARP`, a shortest path, by a breadth first visit:
 *   \f$ O( n m^2 ) \f$;
 * - `DINIC`, the blocking flows of the levels of a breadth first visit:
 *   \f$ O( n^2 m ) \f$, and \f$ O( m \sqrt{n} ) \f$ with unit capacities,
 *   e.g., the bipartite matching (the bound that the book gives for the
 *   algorithm of the three Indians, Karzanov, Malhotra, Kumar and
 *   Maheshwari, of which this is the forerunner).
 *
 * The best bound that the book attributes to Karmarkar (1984),
 * \f$ O( n m \log( n^2 / m ) ) \f$, is the one of the push-relabel of
 * Goldberg and Tarjan (1988); Karmarkar's is the interior point method for
 * linear programming. `feasibleCirculation()` decides whether a circulation
 * with lower and upper bounds on the edges exists, by a maximum flow from a
 * new source that gives each node the lower bounds entering it.
 *
 * The contracts [see `Contract.h`]: a network has n >= 0 nodes, an edge
 * joins two of them with a capacity c >= 0, `maxFlow()` needs two distinct
 * nodes; the invariant is the skew symmetry of the flow on each edge and
 * its twin, within the capacity. At the level of the audits `maxFlow()`
 * ensures that no augmenting path is left, and `feasibleCirculation()` that
 * the flows it gives are within the bounds and conserved at every node.
 * The operations are defined in `MaxFlow.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see L. R. Ford, D. R. Fulkerson, Maximal flow through a network,
 *      Canadian Journal of Mathematics 8, 399-404, 1956,
 *      doi:10.4153/CJM-1956-045-5
 * \see J. Edmonds, R. M. Karp, Theoretical improvements in algorithmic
 *      efficiency for network flow problems, Journal of the ACM 19(2),
 *      248-264, 1972, doi:10.1145/321694.321699
 * \see E. A. Dinic, Algorithm for solution of a problem of maximum flow in
 *      a network with power estimation, Soviet Mathematics Doklady 11,
 *      1277-1280, 1970
 * \see A. V. Karzanov, Determining the maximal flow in a network by the
 *      method of preflows, Soviet Mathematics Doklady 15, 434-437, 1974
 * \see V. M. Malhotra, M. P. Kumar, S. N. Maheshwari, An \f$ O( |V|^3 ) \f$
 *      algorithm for finding maximum flows in networks, Information
 *      Processing Letters 7(6), 277-278, 1978,
 *      doi:10.1016/0020-0190(78)90016-9
 * \see N. Karmarkar, A new polynomial-time algorithm for linear
 *      programming, Combinatorica 4(4), 373-395, 1984,
 *      doi:10.1007/BF02579150
 * \see A. V. Goldberg, R. E. Tarjan, A new approach to the maximum-flow
 *      problem, Journal of the ACM 35(4), 921-940, 1988,
 *      doi:10.1145/48014.61051
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

#ifndef __ADS_MaxFlow
 #define __ADS_MaxFlow
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the way of finding the augmenting paths
enum class Augment {
 FORD_FULKERSON ,   ///< any augmenting path, by a visit in depth
 EDMONDS_KARP ,     ///< a shortest augmenting path, by a visit in breadth
 DINIC              ///< a blocking flow on the levels of a visit in breadth
 };

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS FlowNetwork ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a network of flow with integer capacities
class FlowNetwork {

/*--------------------------------------------------------------------------*/
 public:

 /// the network of the nodes \f$ 0 , \ldots , n - 1 \f$, with no edge
 /// \pre \f$ n \ge 0 \f$
 explicit FlowNetwork( int n );

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( int( f_adj.size() ) ); }

 /// the edge \f$ ( u , v ) \f$ of capacity \f$ c \f$, and its number
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes, \f$ c \ge 0 \f$
 int addEdge( int u , int v , long long c );

 /// a new node, with no edge, and its number
 int addNode( void );

 /// the capacity of the edge number \f$ e \f$ set to \f$ c \f$, its flow
 /// kept
 /// \pre \f$ e \f$ is the number of an edge, \f$ c \ge 0 \f$ and
 ///      \f$ c \ge \f$ `flow( e )`
 void setCapacity( int e , long long c );

 /// the flow on the edge number \f$ e \f$
 /// \pre \f$ e \f$ is the number of an edge
 [[nodiscard]] long long flow( int e ) const;

 /// the capacity of the edge number \f$ e \f$
 /// \pre \f$ e \f$ is the number of an edge
 [[nodiscard]] long long capacity( int e ) const;

 /// the maximum flow from \f$ s \f$ to \f$ t \f$, starting from the flow there
 /// is; the augmenting paths found go to `*paths`, if given
 ///
 /// In pseudocode (section 15.2), c_f( u , v ) = c( u , v ) - f( u , v ) the
 /// residual capacity, each edge with its twin of opposite flow:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maxFlow}( s , t , \mathit{how} ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad \textbf{if}\ \mathit{how} = \text{FORD_FULKERSON }
 ///     \ \textbf{then}\ P \gets \text{a path from } s \text{ to } t
 ///     \text{ of }
 ///     c_f > 0 \text{ by a visit in depth} \cr
 /// 4 & \qquad \textbf{else if}\ \mathit{how} = \text{EDMONDS_KARP }
 ///     \ \textbf{then}\ P \gets \text{a shortest such path, by a visit in }
 ///     \text{breadth} \cr
 /// 5 & \qquad \textbf{else}\quad \triangleright\ \text{DINIC} \cr
 /// 6 & \qquad\quad \ell \gets \text{the levels of a visit in breadth from } s
 ///     \text{ in the residual network} \cr
 /// 7 & \qquad\quad \textbf{if}\ t \text{ is not reached }\ \textbf{then }
 ///     \text{return}\ |f| \cr
 /// 8 & \qquad\quad \text{push a blocking flow along the edges } ( u , v )
 ///     \text{ with } \ell( v ) = \ell( u ) + 1;\ \textbf{continue} \cr
 /// 9 & \qquad \textbf{if}\ P = \textbf{nil}\ \textbf{then return}\ |f| \cr
 /// 10 & \qquad \delta \gets \min_{( u , v ) \in P} c_f( u , v ) \cr
 /// 11 & \qquad \textbf{foreach}\ ( u , v ) \in P\ \textbf{do}\ f( u , v )
 ///     \gets f( u , v ) + \delta;\ f( v , u ) \gets f( v , u ) - \delta
 /// \end{array}}
 /// \f]
 /// \pre \f$ s \f$ and \f$ t \f$ are distinct nodes
 /// \post no augmenting path from \f$ s \f$ to \f$ t \f$ (audit)
 long long maxFlow( int s , int t , Augment how = Augment::DINIC ,
                    long long * paths = nullptr );

 /// the side \f$ S \f$ of a minimum cut, the nodes reached from \f$ s \f$ in
 /// the residual network, after `maxFlow()`
 ///
 /// In pseudocode (Theorem 15.1): with f maximum, the nodes reached from s by
 /// edges of positive residual capacity are one side of a cut of capacity |f|.
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{minCut}( s ) \cr
 /// 2 & \quad \textbf{return}\  \text{the nodes reached from } s \text{ by a }
 ///     \text{visit of the edges with } c_f( u , v ) > 0
 /// \end{array}}
 /// \f]
 /// \pre \f$ s \f$ is a node
 [[nodiscard]] std::vector< bool > minCut( int s ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// an edge of the network
 struct Edge {
  int to;           ///< its head
  long long cap ,   ///< its capacity
            flow;   ///< the flow on it
  };

 static constexpr long long INF = std::numeric_limits< long long >::max();

 long long residual( int e ) const;

 void push( int e , long long d );

 /// one augmenting path by a breadth first visit
 long long bfsAugment( int s , int t );

 /// one augmenting path by a depth first visit, iterative
 long long dfsAugment( int s , int t );

 /// the levels of the breadth first visit from \f$ s \f$; true if \f$ t \f$ is
 /// reached
 bool levels( int s , int t );

 /// a path of the levels from \f$ u \f$ to \f$ t \f$, of at most \f$ d \f$
 /// units
 long long blocking( int u , int t , long long d );

 /// each edge and its twin carry opposite flows within their capacities
 [[nodiscard]] bool skewSymmetric( void ) const;

 std::vector< std::vector< int > > f_adj;   ///< the edges out of each node
 std::vector< Edge > f_e;                   ///< the edges, each and its twin
 std::vector< int > f_level;                ///< the levels of Dinic
 std::vector< std::size_t > f_next;         ///< the next edge of Dinic

 };  // end( class FlowNetwork )

/// an edge \f$ ( u , v ) \f$ with lower and upper bounds on its flow
struct BoundedEdge {
 int u ,           ///< the tail
     v;            ///< the head
 long long low ,   ///< the least flow on the edge
           high;   ///< the largest flow on the edge
 };

/// whether the edges admit a circulation, a flow conserved at every node with
/// \f$ \mathit{low} \le f( e ) \le \mathit{high} \f$; then the flows in
/// `*flows`, if given
///
/// In pseudocode (the reduction to a maximum flow): sending the lower bounds
/// first leaves each node an excess, positive or negative, which a flow from a
/// new source s to a new sink t must balance within the capacities high - low.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{feasibleCirculation}( n , E ) \cr
/// 2 & \quad N \gets \text{a network of the nodes } 0 , \ldots , n + 1;\ s
///     \gets n;\ t \gets n + 1 \cr
/// 3 & \quad \textbf{foreach}\ ( u , v , \mathit{low} , \mathit{high} ) \in E
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{low} > \mathit{high}\ \textbf{then return
///     false} \cr
/// 5 & \qquad \text{add } ( u , v ) \text{ to } N \text{ with capacity }
///     \mathit{high} - \mathit{low} \cr
/// 6 & \qquad e[ v ] \gets e[ v ] + \mathit{low};\ e[ u ] \gets e[ u ] -
///     \mathit{low} \cr
/// 7 & \quad \textbf{foreach}\ v\ \textbf{with}\ e[ v ] > 0\ \textbf{do}
///     \ \text{add } ( s , v ) \text{ with capacity } e[ v ] \cr
/// 8 & \quad \textbf{foreach}\ v\ \textbf{with}\ e[ v ] < 0\ \textbf{do}
///     \ \text{add } ( v , t ) \text{ with capacity } -e[ v ] \cr
/// 9 & \quad \textbf{return}\ N.\mathrm{maxFlow}( s , t ) = \sum_{e[ v ] > 0}
///     e[ v ]\quad \triangleright\ \text{the flow of each edge is then }
///     \mathit{low} + f
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the ends of the edges in \f$ [ 0 , n ) \f$
/// \post the flows given are within the bounds and conserved (audit)
inline bool feasibleCirculation( int n , const std::vector< BoundedEdge > & E ,
                                 std::vector< long long > * flows = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/MaxFlow.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MaxFlow.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File MaxFlow.h ---------------------------*/
/*--------------------------------------------------------------------------*/
