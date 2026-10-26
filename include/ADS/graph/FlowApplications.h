/*--------------------------------------------------------------------------*/
/*------------------------ File FlowApplications.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two uses of the maximum flow of section 15.2 [see MaxFlow.h]:
 *
 * - edgeDisjointPaths(), at most k paths from s to t with no edge in
 *   common, as many as there can be: by the theorem of Menger, their
 *   largest number is the maximum flow from s to t with capacity 1 on each
 *   edge, which equals the fewest edges whose removal separates t from s.
 *   A new source joined to s by capacity k stops the flow at k. The flow
 *   is integral, and it is decomposed into paths by walking from s along
 *   the edges of flow 1 not used yet, a cycle met on the way being cut out
 *   of the walk, so that the paths are simple. On an unoriented graph each
 *   edge gives two opposite edges of capacity 1, and their flows, if both
 *   are 1, cancel out before the walks. With DINIC the flow takes
 *   \f$ O( m \min( \sqrt{m} , n^{2/3} ) ) \f$ on these networks of unit
 *   capacities (Even and Tarjan), and \f$ O( k ( n + m ) ) \f$ by k augmenting
 *   paths; the walks take \f$ O( n + m ) \f$ more;
 * - decreaseCapacity(), the maximum flow f, kept on matrices of the
 *   capacities c and of the flows (f( y , x ) = - f( x , y )), after the
 *   capacity of ( u , v ) decreases by 1, without starting over. If the
 *   edge is not saturated f is still a maximum flow (SLACK). Otherwise one
 *   unit less goes on ( u , v ), which leaves an excess at u and a deficit
 *   at v; by the decomposition of f into paths and cycles, that unit lies
 *   on a cycle of flow, closed by a path of flow from v to u, or on a path
 *   of flow from s to t, through a path from s to u and one from v to t.
 *   One unit less on the cycle keeps the value (CYCLE); one unit less on
 *   the two paths lowers it by 1, and since a capacity lowered by 1 lowers
 *   the maximum by at most 1, one search of an augmenting path makes the
 *   flow maximum again (PATH). At most three visits, \f$ O( n^2 ) \f$ on
 *   the matrices, against \f$ O( n^2 m ) \f$ or more for a new maximum
 *   flow; an increase of a capacity by 1 needs one augmenting path.
 *
 * The contracts [see Contract.h]: s and t are distinct nodes, k >= 0; the
 * matrices of decreaseCapacity() are square of the same size and c( u , v
 * ) > 0. At the level of the audits edgeDisjointPaths() ensures that its
 * paths are simple, from s to t along the edges of the graph, and use each
 * edge once, and decreaseCapacity() that f is a maximum flow within c
 * before and after. The functions are defined in FlowApplications.tpp.
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

/// what decreaseCapacity() did
enum class CapacityDecrease {
 SLACK ,   ///< the edge was not saturated, nothing changed
 CYCLE ,   ///< one unit of a cycle of flow cancelled, the value kept
 PATH      ///< one unit of a path cancelled, then one augmenting path
 };

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// edgeDisjointPaths(): at most k paths from s to t with no edge in common,
/// as many as there can be, each as its nodes from s to t
/// \pre s and t are distinct nodes, k >= 0
/// \post at most k simple paths from s to t along the edges of the graph,
///       no edge used twice (audit)
template< Graph G >
std::vector< std::vector< int > > edgeDisjointPaths(
                  const G & g , int s , int t ,
                  long long k = std::numeric_limits< long long >::max() );

/// decreaseCapacity(): c( u , v ) decreases by 1, and the maximum flow f
/// from s to t within c becomes a maximum flow within the new c
/// \pre c and f are square of the same size, s and t distinct nodes, u and
///      v distinct nodes, c( u , v ) > 0, f a maximum flow within c (audit)
/// \post f a maximum flow within the new c (audit)
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
