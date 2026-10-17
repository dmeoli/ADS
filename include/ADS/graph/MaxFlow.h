/*--------------------------------------------------------------------------*/
/*----------------------------- File MaxFlow.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The maximum flow of section 15.2 in a network G = ( V , E , s , t , c ),
 * with integer capacities, and the minimum cut. The network keeps for
 * each edge its residual twin, so that the residual capacity
 * \f$ c_f( u , v ) = c( u , v ) - f( u , v ) \f$ of both directions is at
 * hand. By the Teorema 15.1 (maximum flow / minimum cut) f is maximum if
 * and only if there is no augmenting path, a path from s to t in the
 * residual network, and then the nodes S reached from s give a cut
 * ( S , V - S ) of capacity |f|. The local search augments the flow along
 * augmenting paths:
 *
 * - FORD_FULKERSON, any path, by a depth first visit: O( ( n + m ) |f*| );
 * - EDMONDS_KARP, a shortest path, by a breadth first visit: O( n m^2 );
 * - DINIC, the blocking flows of the levels of a breadth first visit:
 *   \f$ O( n^2 m ) \f$, and \f$ O( m \sqrt{n} ) \f$ with unit capacities,
 *   e.g., the bipartite matching (the bound that the book gives for the
 *   algorithm of the three Indians, Karzanov, Malhotra, Kumar and
 *   Maheshwari, of which this is the forerunner).
 *
 * The best bound that the book attributes to Karmarkar (1984),
 * \f$ O( n m \log( n^2 / m ) ) \f$, is the one of the push-relabel of
 * Goldberg and Tarjan (1988); Karmarkar's is the interior point method for
 * linear programming. feasibleCirculation() decides whether a circulation
 * with lower and upper bounds on the edges exists, by a maximum flow from a
 * new source that gives each node the lower bounds entering it.
 *
 * The contracts [see Contract.h]: a network has n >= 0 nodes, an edge
 * joins two of them with a capacity c >= 0, maxFlow() needs two distinct
 * nodes; the invariant is the skew symmetry of the flow on each edge and
 * its twin, within the capacity. At the level of the audits maxFlow()
 * ensures that no augmenting path is left, and feasibleCirculation() that
 * the flows it gives are within the bounds and conserved at every node.
 * The operations are defined in MaxFlow.tpp.
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
 * \see A. V. Goldberg, R. E. Tarjan, A new approach to the maximum-flow
 *      problem, Journal of the ACM 35(4), 921-940, 1988,
 *      doi:10.1145/48014.61051
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
enum class Augment { FORD_FULKERSON , EDMONDS_KARP , DINIC };

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS FlowNetwork ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a network of flow with integer capacities
class FlowNetwork {

/*--------------------------------------------------------------------------*/
 public:

 /// the network of the nodes 0 , ... , n - 1, with no edge
 /// \pre n >= 0
 explicit FlowNetwork( int n );

 [[nodiscard]] int n( void ) const { return( int( f_adj.size() ) ); }

 /// the edge ( u , v ) of capacity c, and its number
 /// \pre u and v are nodes, c >= 0
 int addEdge( int u , int v , long c );

 /// the flow on the edge number e
 /// \pre e is the number of an edge
 [[nodiscard]] long flow( int e ) const;

 /// the capacity of the edge number e
 /// \pre e is the number of an edge
 [[nodiscard]] long capacity( int e ) const;

 /// the maximum flow from s to t, starting from the flow there is; the
 /// augmenting paths found go to *paths, if given
 /// \pre s and t are distinct nodes
 /// \post no augmenting path from s to t (audit)
 long maxFlow( int s , int t , Augment how = Augment::DINIC ,
               long * paths = nullptr );

 /// the side S of a minimum cut, the nodes reached from s in the residual
 /// network, after maxFlow()
 /// \pre s is a node
 [[nodiscard]] std::vector< bool > minCut( int s ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Edge { int to; long cap , flow; };

 static constexpr long INF = std::numeric_limits< long >::max();

 long residual( int e ) const;

 void push( int e , long d );

 /// one augmenting path by a breadth first visit
 long bfsAugment( int s , int t );

 /// one augmenting path by a depth first visit, iterative
 long dfsAugment( int s , int t );

 /// the levels of the breadth first visit from s; true if t is reached
 bool levels( int s , int t );

 /// a path of the levels from u to t, of at most d units
 long blocking( int u , int t , long d );

 /// each edge and its twin carry opposite flows within their capacities
 [[nodiscard]] bool skewSymmetric( void ) const;

 std::vector< std::vector< int > > f_adj;   ///< the edges out of each node
 std::vector< Edge > f_e;                   ///< the edges, each and its twin
 std::vector< int > f_level;                ///< the levels of Dinic
 std::vector< std::size_t > f_next;         ///< the next edge of Dinic

 };  // end( class FlowNetwork )

/// an edge ( u , v ) with lower and upper bounds on its flow
struct BoundedEdge { int u , v; long low , high; };

/// whether the edges admit a circulation, a flow conserved at every node
/// with low <= f( e ) <= high; then the flows in *flows, if given
/// \pre n >= 0, the ends of the edges in [ 0 , n )
/// \post the flows given are within the bounds and conserved (audit)
inline bool feasibleCirculation( int n , const std::vector< BoundedEdge > & E ,
                                 std::vector< long > * flows = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/MaxFlow.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MaxFlow.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File MaxFlow.h ---------------------------*/
/*--------------------------------------------------------------------------*/
