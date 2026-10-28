/*--------------------------------------------------------------------------*/
/*----------------------------- File MaxFlow.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The maximum flow of section 15.2 in a network
 * \f$ G = ( V , E , s , t , c ) \f$, with integer capacities, and the minimum
 * cut. The network keeps for each edge its residual twin, so that the residual
 * capacity \f$ c_f( u , v ) = c( u , v ) - f( u , v ) \f$ of both directions
 * is at hand. By Theorem 15.1 (maximum flow / minimum cut) \f$ f \f$ is
 * maximum if and only if there is no augmenting path, a path from \f$ s \f$
 * to \f$ t \f$ in the residual network, and then the nodes \f$ S \f$
 * reached from \f$ s \f$ give a cut \f$ ( S , V \setminus S ) \f$ of
 * capacity \f$ | f | \f$. The local search augments the flow along
 * augmenting paths:
 *
 * - `FORD_FULKERSON`, any path, by a depth first visit:
 *   \f$ O( ( n + m ) |f^*| ) \f$;
 * - `EDMONDS_KARP`, a shortest path, by a breadth first visit:
 *   \f$ O( n m^2 ) \f$;
 * - `DINIC`, the blocking flows of the levels of a breadth first visit:
 *   \f$ O( n^2 m ) \f$, and \f$ O( m \sqrt{n} ) \f$ with unit capacities,
 *   e.g., the bipartite matching (the bound that the book gives for the
 *   algorithm of the three Indians, Karzanov, Malhotra, Kumar and
 *   Maheshwari, of which this is the forerunner);
 * - `PUSH_RELABEL`, beyond the book, the push-relabel of Goldberg and
 *   Tarjan, which keeps a preflow (more flow may enter a node than leave
 *   it) and labels that bound the distances from \f$ t \f$, with the node
 *   of the highest label discharged first, \f$ O( n^2 \sqrt{m} ) \f$
 *   (Cheriyan and Maheshwari), and the global relabelings and the gaps of
 *   Cherkassky and Goldberg, on which its speed depends in practice (see
 *   \ref theory_graph_flow); from the flow there is it first looks for a
 *   residual path from \f$ s \f$ to \f$ t \f$ in the side of \f$ s \f$,
 *   and stops if there is none, and it takes the excesses that
 *   `setCapacity()` left where they are, so that a change of the
 *   capacities costs less than a computation from scratch; the visits of
 *   `DINIC`, which explore only the nodes reached from \f$ s \f$, remain
 *   the cheaper repair when the network is large and the change small.
 *
 * The best bound that the book attributes to Karmarkar (1984),
 * \f$ O( n m \log( n^2 / m ) ) \f$, is the one of the push-relabel of
 * Goldberg and Tarjan (1988) with the dynamic trees; Karmarkar's is the
 * interior point method for linear programming. Every way starts from the
 * flow there is, and `setCapacity()` may lower a capacity below the flow
 * of its edge, which leaves an excess and a deficit for the next
 * `maxFlow()` to route first (the reparametrization of the dynamic cuts of
 * Kohli and Torr), so that a flow is reoptimized after any change of the
 * capacities [see `MaxFlowObserver` of `Reoptimization.h`].
 * `feasibleCirculation()` decides whether a circulation with lower and
 * upper bounds on the edges exists, by a maximum flow from a new source
 * that gives each node the lower bounds entering it.
 *
 * The contracts [see `Contract.h`]: a network has \f$ n \ge 0 \f$ nodes,
 * an edge joins two of them with a capacity \f$ c \ge 0 \f$, `maxFlow()`
 * needs two distinct nodes; the invariant is the skew symmetry of the flow
 * on each edge and its twin, within the capacity. At the level of the
 * audits `maxFlow()` ensures that no augmenting path is left, and
 * `feasibleCirculation()` that the flows it gives are within the bounds
 * and conserved at every node.
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
 * \see J. Cheriyan, S. N. Maheshwari, Analysis of preflow push algorithms
 *      for maximum network flow, SIAM Journal on Computing 18(6),
 *      1057-1086, 1989, doi:10.1137/0218072
 * \see B. V. Cherkassky, A. V. Goldberg, On implementing the push-relabel
 *      method for the maximum flow problem, Algorithmica 19(4), 390-410,
 *      1997, doi:10.1007/PL00009180
 * \see P. Kohli, P. H. S. Torr, Dynamic graph cuts for efficient inference
 *      in Markov random fields, IEEE Transactions on Pattern Analysis and
 *      Machine Intelligence 29(12), 2079-2088, 2007,
 *      doi:10.1109/TPAMI.2007.1128
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

/// the way of increasing the flow
enum class Augment {
 FORD_FULKERSON ,   ///< any augmenting path, by a visit in depth
 EDMONDS_KARP ,     ///< a shortest augmenting path, by a visit in breadth
 DINIC ,            ///< a blocking flow on the levels of a visit in breadth
 PUSH_RELABEL       ///< the pushes and the relabelings of a preflow
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
 /// kept if at most \f$ c \f$ and cut to \f$ c \f$ otherwise, the
 /// difference left as an excess at the tail and a deficit at the head,
 /// which the next `maxFlow()` routes before it increases the flow
 /// \pre \f$ e \f$ is the number of an edge, \f$ c \ge 0 \f$
 void setCapacity( int e , long long c );

 /// the flow on the edge number \f$ e \f$
 /// \pre \f$ e \f$ is the number of an edge
 [[nodiscard]] long long flow( int e ) const;

 /// the capacity of the edge number \f$ e \f$
 /// \pre \f$ e \f$ is the number of an edge
 [[nodiscard]] long long capacity( int e ) const;

 /// the maximum flow from \f$ s \f$ to \f$ t \f$, starting from the flow
 /// there is, after the excesses and the deficits left by `setCapacity()`
 /// are routed: the change of its value \f$ | f | \f$, the net flow into
 /// \f$ t \f$; the augmenting paths found go to `*paths`, if given (none
 /// for `PUSH_RELABEL`)
 ///
 /// In pseudocode (section 15.2), \f$ c_f( u , v ) = c( u , v ) -
 /// f( u , v ) \f$ the residual capacity, each edge with its twin of
 /// opposite flow:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maxFlow}( s , t , \mathit{how} ) \cr
 /// 2 & \quad \mathrm{rebalance}( s , t ) \cr
 /// 3 & \quad \textbf{if}\ \mathit{how} = \text{PUSH_RELABEL}
 ///     \ \textbf{then}\ \mathrm{pushRelabel}( s , t );\ \textbf{return}
 ///     \ | f | \cr
 /// 4 & \quad \textbf{loop} \cr
 /// 5 & \qquad \textbf{if}\ \mathit{how} = \text{FORD_FULKERSON }
 ///     \ \textbf{then}\ P \gets \text{a path from } s \text{ to } t
 ///     \text{ of } c_f > 0 \text{ by a visit in depth} \cr
 /// 6 & \qquad \textbf{else if}\ \mathit{how} = \text{EDMONDS_KARP }
 ///     \ \textbf{then}\ P \gets \text{a shortest such path, by a visit}
 ///     \text{ in breadth} \cr
 /// 7 & \qquad \textbf{else}\quad \triangleright\ \text{DINIC} \cr
 /// 8 & \qquad\quad \ell \gets \text{the levels of a visit in breadth from }
 ///     s \text{ in the residual network} \cr
 /// 9 & \qquad\quad \textbf{if}\ t \text{ is not reached}\ \textbf{then}
 ///     \ \textbf{return}\ | f | \cr
 /// 10 & \qquad\quad \text{push a blocking flow along the edges } ( u , v )
 ///     \text{ with } \ell( v ) = \ell( u ) + 1;\ \textbf{continue} \cr
 /// 11 & \qquad \textbf{if}\ P = \textbf{nil}\ \textbf{then return}\ | f |
 ///     \cr
 /// 12 & \qquad \delta \gets \min_{( u , v ) \in P} c_f( u , v ) \cr
 /// 13 & \qquad \textbf{foreach}\ ( u , v ) \in P\ \textbf{do}\ f( u , v )
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
 /// In pseudocode (Theorem 15.1): with \f$ f \f$ maximum, the nodes reached
 /// from \f$ s \f$ by edges of positive residual capacity are one side of a
 /// cut of capacity \f$ | f | \f$.
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


 /// no bound
 static constexpr long long INF = std::numeric_limits< long long >::max();

 /// the residual capacity of the edge number \f$ e \f$
 long long residual( int e ) const;

 /// \f$ d \f$ units more on the edge number \f$ e \f$, and \f$ d \f$ less
 /// on its twin
 void push( int e , long long d );

 /// the net flow into \f$ t \f$
 [[nodiscard]] long long inflow( int t ) const;

 /// the excesses (if `excesses`) and the deficits of `setCapacity()`
 /// routed, a flow again if `excesses`, and a preflow otherwise, whose
 /// excesses the push-relabel takes as they are
 ///
 /// In pseudocode (beyond the book; the reparametrization of the dynamic
 /// cuts of Kohli and Torr, without their terminal edges), with
 /// \f$ e( x ) \f$ the excess of \f$ x \f$, negative for a deficit;
 /// \f$ s \f$ and \f$ t \f$ give and take any amount, and a path always
 /// exists, by the decomposition of the flow before the cut into paths and
 /// cycles:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rebalance}( s , t ,
 ///     \mathit{excesses} ) \quad
 ///     \triangleright\ \text{lines 2 to 5 only if } \mathit{excesses}
 ///     \cr
 /// 2 & \quad \textbf{foreach}\ x \notin \{ s , t \}\ \textbf{with}
 ///     \ e( x ) > 0\ \textbf{do} \cr
 /// 3 & \qquad \textbf{while}\ e( x ) > 0\ \textbf{do} \cr
 /// 4 & \qquad\quad P \gets \text{a path of } c_f > 0 \text{ from } x
 ///     \text{ to } s \text{, to } t \text{ or to a } y \text{ with }
 ///     e( y ) < 0 \text{, by a visit in breadth} \cr
 /// 5 & \qquad\quad \text{push along } P \text{ the least of } e( x ) ,
 ///     \ \min_P c_f \text{ and } - e( y ) \cr
 /// 6 & \quad \textbf{foreach}\ x \notin \{ s , t \}\ \textbf{with}
 ///     \ e( x ) < 0\ \textbf{do} \cr
 /// 7 & \qquad \textbf{while}\ e( x ) < 0\ \textbf{do}\ \text{push the}
 ///     \text{ least of } - e( x ) \text{ and } \min_P c_f \text{ along a}
 ///     \text{ path } P \text{ of } c_f > 0 \text{ from } s \text{, } t
 ///     \text{ or a } y \text{ with } e( y ) > 0 \text{ to } x
 /// \end{array}}
 /// \f]
 void rebalance( int s , int t , bool excesses = true );

 /// the maximum flow from \f$ s \f$ to \f$ t \f$ by the push-relabel of
 /// Goldberg and Tarjan, from the flow there is and the excesses left by
 /// `setCapacity()`
 ///
 /// In pseudocode (beyond the book; the highest label first, with the
 /// global relabelings and the gaps of Cherkassky and Goldberg, the
 /// labelings that stop at the last node with an excess of their HIPR,
 /// and the two phases and the scan of the edges of a node that pushes
 /// and finds its new label at once of the Preflow of LEMON), with
 /// \f$ e( x ) \f$
 /// the excess of \f$ x \f$ and \f$ d( x ) \f$ its label, a lower bound of
 /// its distance from the target in the residual network, \f$ D \f$ the
 /// label of the nodes out of the game:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{pushRelabel}( s , t ) \cr
 /// 2 & \quad e \gets \text{the excesses left by } \mathrm{setCapacity}()
 ///     \cr
 /// 3 & \quad \textbf{if}\ e = 0 \textbf{ and } | f | > 0 \textbf{ and }
 ///     t \text{ is not reached from } s \text{ in the residual network}
 ///     \ \textbf{then}\ \textbf{return} \cr
 /// 4 & \quad \textbf{foreach}\ ( s , v )\ \textbf{with}\ c_f( s , v )
 ///     > 0\ \textbf{do}\ \text{push } c_f( s , v ) \text{ on it} \cr
 /// 5 & \quad d \gets \mathrm{labels}( t ) \cr
 /// 6 & \quad \mathrm{discharge}( t , n ) \quad \triangleright\ \text{a}
 ///     \text{ maximum preflow} \cr
 /// 7 & \quad \textbf{if}\ \text{some } u \text{ of label } n \text{ has }
 ///     e( u ) > 0\ \textbf{then} \cr
 /// 8 & \qquad d \gets \mathrm{labels}( s ) \text{ among the nodes of}
 ///     \text{ label } n;\ \mathrm{discharge}( s , 2 n ) \cr
 /// 9 & \textbf{procedure}\ \mathrm{discharge}( z , D ) \cr
 /// 10 & \quad \textbf{while}\ \text{some } u \notin \{ s , t \}
 ///     \text{ has } e( u ) > 0 \text{ and } d( u ) < D\ \textbf{do} \cr
 /// 11 & \qquad u \gets \text{such a node of the highest label};
 ///     \ \lambda \gets D \cr
 /// 12 & \qquad \textbf{foreach}\ ( u , v )\ \textbf{with}\ c_f( u , v )
 ///     > 0\ \textbf{while}\ e( u ) > 0\ \textbf{do} \cr
 /// 13 & \qquad\quad \textbf{if}\ d( v ) < d( u )\ \textbf{then}
 ///     \ \text{push } \min( e( u ) , c_f( u , v ) ) \text{ on } ( u , v )
 ///     \ \textbf{else}\ \lambda \gets \min( \lambda , d( v ) ) \cr
 /// 14 & \qquad \textbf{if}\ e( u ) > 0\ \textbf{then} \cr
 /// 15 & \qquad\quad \textbf{if}\ u \text{ is the only node of label }
 ///     d( u )\ \textbf{then}\ d( x ) \gets D \text{ for } u \text{ and}
 ///     \text{ each } x \text{ with } d( x ) > d( u ) \cr
 /// 16 & \qquad\quad \textbf{else}\ d( u ) \gets \min( D , \lambda + 1 )
 ///     \cr
 /// 17 & \qquad \textbf{if}\ \text{the relabelings cost more than }
 ///     6 n + m \text{ since the last labels}\ \textbf{then}\ d \gets
 ///     \mathrm{labels}( z ) \cr
 /// 18 & \textbf{function}\ \mathrm{labels}( z ) \cr
 /// 19 & \quad \text{visit in breadth backwards from } z \text{, level by}
 ///     \text{ level, } d( u ) \text{ the level of } u \cr
 /// 20 & \quad \text{stop after the level } k \text{ that labels the last}
 ///     \text{ node with an excess} \cr
 /// 21 & \quad \textbf{return}\ d \text{, and } k + 2 \text{ for the}
 ///     \text{ nodes not reached} \quad \triangleright\ \text{a lower}
 ///     \text{ bound}
 /// \end{array}}
 /// \f]
 void pushRelabel( int s , int t );

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

 /// an edge out of a node: its number and its head, kept together so
 /// that a visit reads the edge only when its head is of use
 struct Incidence {
  int e;            ///< the number of the edge
  int to;           ///< its head
  };

 std::vector< std::vector< Incidence > > f_adj;   ///< the edges out of
                                                  ///< each node
 std::vector< int > f_to;                   ///< the head of each edge,
                                            ///< each and its twin
 std::vector< long long > f_cap;            ///< the capacities
 std::vector< long long > f_res;            ///< the residual capacities,
                                            ///< apart, so that a visit
                                            ///< reads 8 bytes per edge
 std::vector< int > f_level;                ///< the levels of Dinic
 std::vector< std::size_t > f_next;         ///< the next edge of Dinic
 std::vector< long long > f_imb;            ///< the excesses left by
                                            ///< `setCapacity()`
 bool f_unbalanced = false;                 ///< whether there are some
 std::vector< int > f_imbNodes;             ///< the nodes that have them

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
