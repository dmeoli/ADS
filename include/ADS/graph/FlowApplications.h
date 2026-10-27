/*--------------------------------------------------------------------------*/
/*------------------------ File FlowApplications.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two uses of the maximum flow of section 15.2 [see `MaxFlow.h`]:
 *
 * - `edgeDisjointPaths()`, at most k paths from s to t with no edge in
 *   common, as many as there can be: by the theorem of Menger, their
 *   largest number is the maximum flow from s to t with capacity 1 on each
 *   edge, which equals the fewest edges whose removal separates t from s.
 *   A new source joined to s by capacity k stops the flow at k. The flow
 *   is integral, and it is decomposed into paths by walking from s along
 *   the edges of flow 1 not used yet, a cycle met on the way being cut out
 *   of the walk, so that the paths are simple. On an unoriented graph each
 *   edge gives two opposite edges of capacity 1, and their flows, if both
 *   are 1, cancel out before the walks. With `DINIC` the flow takes
 *   \f$ O( m \min( \sqrt{m} , n^{2/3} ) ) \f$ on these networks of unit
 *   capacities (Even and Tarjan), and \f$ O( k ( n + m ) ) \f$ by k augmenting
 *   paths; the walks take \f$ O( n + m ) \f$ more;
 * - `decreaseCapacity()`, the maximum flow f, kept on matrices of the
 *   capacities c and of the flows (f( y , x ) = - f( x , y )), after the
 *   capacity of ( u , v ) decreases by 1, without starting over. If the
 *   edge is not saturated f is still a maximum flow (`SLACK`). Otherwise one
 *   unit less goes on ( u , v ), which leaves an excess at u and a deficit
 *   at v; by the decomposition of f into paths and cycles, that unit lies
 *   on a cycle of flow, closed by a path of flow from v to u, or on a path
 *   of flow from s to t, through a path from s to u and one from v to t.
 *   One unit less on the cycle keeps the value (`CYCLE`); one unit less on
 *   the two paths lowers it by 1, and since a capacity lowered by 1 lowers
 *   the maximum by at most 1, one search of an augmenting path makes the
 *   flow maximum again (`PATH`). At most three visits, \f$ O( n^2 ) \f$ on
 *   the matrices, against \f$ O( n^2 m ) \f$ or more for a new maximum
 *   flow; an increase of a capacity by 1 needs one augmenting path.
 *
 * The contracts [see `Contract.h`]: s and t are distinct nodes, k >= 0; the
 * matrices of `decreaseCapacity()` are square of the same size and c( u , v
 * ) > 0. At the level of the audits `edgeDisjointPaths()` ensures that its
 * paths are simple, from s to t along the edges of the graph, and use each
 * edge once, and `decreaseCapacity()` that f is a maximum flow within c
 * before and after. The functions are defined in `FlowApplications.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see K. Menger, Zur allgemeinen Kurventheorie, Fundamenta Mathematicae
 *      10, 96-115, 1927, doi:10.4064/fm-10-1-96-115
 * \see L. R. Ford, D. R. Fulkerson, Maximal flow through a network,
 *      Canadian Journal of Mathematics 8, 399-404, 1956,
 *      doi:10.4153/CJM-1956-045-5
 * \see S. Even, R. E. Tarjan, Network flow and testing graph connectivity,
 *      SIAM Journal on Computing 4(4), 507-518, 1975, doi:10.1137/0204043
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

#ifndef __ADS_FlowApplications
 #define __ADS_FlowApplications
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/MaxFlow.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// what `decreaseCapacity()` did
enum class CapacityDecrease {
 SLACK ,   ///< the edge was not saturated, nothing changed
 CYCLE ,   ///< one unit of a cycle of flow cancelled, the value kept
 PATH      ///< one unit of a path cancelled, then one augmenting path
 };

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// at most \f$ k \f$ paths from \f$ s \f$ to \f$ t \f$
/// with no edge in common, as many as there can be, each as its nodes from
/// \f$ s \f$ to \f$ t \f$
///
/// In pseudocode (the theorem of Menger):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{edgeDisjointPaths}( G , s , t , k ) \cr
/// 2 & \quad N \gets \text{a network with capacity 1 on each edge of } G
///     \text{ , both ways if unoriented} \cr
/// 3 & \quad \text{add a new source } s' \text{ and the edge } ( s' , s )
///     \text{ of capacity } k \cr
/// 4 & \quad N.\mathrm{maxFlow}( s' , t )\quad \triangleright
///     \text{integral: each edge carries 0 or 1} \cr
/// 5 & \quad \text{cancel the opposite flows of the two edges of an }
///     \text{unoriented edge} \cr
/// 6 & \quad P \gets () \cr
/// 7 & \quad \textbf{while}\  \text{an edge of flow 1 leaves } s\ \textbf{do}
///     \cr
/// 8 & \qquad W \gets ( s );\  \text{walk from } s \text{ along the edges of }
///     \text{flow 1 not used yet, marking them, until } t \cr
/// 9 & \qquad \text{cut out of } W \text{ every cycle met on the way};
///     \ \text{append } W \text{ to } P \cr
/// 10 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ and \f$ t \f$ are distinct nodes, \f$ k \ge 0 \f$
/// \post at most \f$ k \f$ simple paths from \f$ s \f$ to \f$ t \f$ along the
///       edges of the graph, no edge used twice (audit)
template< Graph G >
std::vector< std::vector< int > > edgeDisjointPaths(
                  const G & g , int s , int t ,
                  long long k = std::numeric_limits< long long >::max() );

/// \f$ c( u , v ) \f$ decreases by 1, and the maximum
/// flow \f$ f \f$ from \f$ s \f$ to \f$ t \f$ within \f$ c \f$ becomes a
/// maximum flow within the new \f$ c \f$
///
/// In pseudocode, f( y , x ) = -f( x , y ) on the matrices:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decreaseCapacity}( c , f , s , t , u , v )
///     \cr
/// 2 & \quad c( u , v ) \gets c( u , v ) - 1 \cr
/// 3 & \quad \textbf{if}\ f( u , v ) \le c( u , v )\ \textbf{then return}
///     \ \text{SLACK} \cr
/// 4 & \quad f( u , v ) \gets f( u , v ) - 1;\ f( v , u ) \gets f( v , u ) +
///     1\quad \triangleright\ \text{an excess at } u \text{ , a deficit at } v
///     \cr
/// 5 & \quad \textbf{if}\  \text{a path } Q \text{ of positive flow goes }
///     \text{from } v \text{ to } u\ \textbf{then} \cr
/// 6 & \qquad \text{send one unit less along } Q;\ \textbf{return}
///     \ \text{CYCLE} \cr
/// 7 & \quad \text{send one unit less along a path of positive flow from } s
///     \text{ to } u \text{ and one from } v \text{ to } t \cr
/// 8 & \quad \text{augment by one path from } s \text{ to } t \text{ if }
///     \text{there is one};\ \textbf{return}\  \text{PATH}
/// \end{array}}
/// \f]
/// \pre \f$ c \f$ and \f$ f \f$ are square of the same size, \f$ s \f$ and
///      \f$ t \f$ distinct nodes, \f$ u \f$ and \f$ v \f$ distinct nodes,
///      \f$ c( u , v ) > 0 \f$, \f$ f \f$ a maximum flow within \f$ c \f$
///      (audit)
/// \post \f$ f \f$ a maximum flow within the new \f$ c \f$ (audit)
template< class T >
CapacityDecrease decreaseCapacity( std::vector< std::vector< T > > & c ,
                                   std::vector< std::vector< T > > & f ,
                                   int s , int t , int u , int v );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/FlowApplications.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FlowApplications.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File FlowApplications.h -----------------------*/
/*--------------------------------------------------------------------------*/
