/*--------------------------------------------------------------------------*/
/*---------------------------- File Matching.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Matchings, the sets of edges of a graph no two of which share an end
 * beyond the capacities of the ends:
 *
 * - bipartiteBMatching(), the b-matching of largest size of a bipartite
 *   graph, each node u on the left in at most capL[ u ] pairs and each v on
 *   the right in at most capR[ v ], each edge used once: a maximum flow of
 *   section 15.2 from a source joined to each u by capL[ u ], through the
 *   edges of capacity 1, to a sink joined from each v by capR[ v ]. The
 *   flow is integral, and its edges of flow 1 between the two sides are
 *   the pairs; with DINIC of MaxFlow.h it takes \f$ O( N^2 M ) \f$ for the
 *   N = n + 2 nodes and the M = m + n edges of the network, and less on
 *   such layered networks. With all the capacities 1 it is the
 *   bipartiteMatching() of EasyCases.h, for which the algorithm of Hopcroft
 *   and Karp takes \f$ O( m \sqrt{n} ) \f$; copying each node as many
 *   times as its capacity reduces the general case to it, and Gabow shows
 *   how to do so without the blow up of the copies. It decides, e.g.,
 *   whether the tasks can all be given to workers who can do them, each
 *   worker taking a bounded number of them;
 * - maxWeightMatching(), a matching of largest total weight of a general
 *   graph of a few nodes, by a branch and bound: the first node not yet
 *   decided is left unmatched or matched to each free neighbour, and a
 *   branch whose weight plus half the best weight at each node left cannot
 *   beat the best matching found is cut. It takes time proportional to the
 *   number of matchings of the graph in the worst case, the telephone
 *   numbers \f$ \sum_k \binom{n}{2k} ( 2k - 1 )!! \f$ on the complete
 *   graph, i.e., it suits graphs of some tens of nodes; the dynamic
 *   programming on the subsets of the nodes takes \f$ \Theta( 2^n n ) \f$,
 *   and the blossoms of Edmonds solve it in polynomial time, in
 *   \f$ O( n m + n^2 \log n ) \f$ with the structures of Gabow.
 *
 * The contracts [see Contract.h]: the nodes on the right of the edges are
 * in [ 0 , nr ), the capacities are not negative, an edge joins two
 * distinct nodes of the graph; at the level of the audits the matchings
 * given respect the capacities of the ends. The functions are defined in
 * Matching.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see J. E. Hopcroft, R. M. Karp, An n^{5/2} algorithm for maximum
 *      matchings in bipartite graphs, SIAM Journal on Computing 2(4),
 *      225-231, 1973, doi:10.1137/0202019
 * \see H. N. Gabow, An efficient reduction technique for degree-constrained
 *      subgraph and bidirected network flow problems, Proceedings of the
 *      15th ACM Symposium on Theory of Computing, 448-456, 1983,
 *      doi:10.1145/800061.808776
 * \see J. Edmonds, Maximum matching and a polyhedron with 0,1-vertices,
 *      Journal of Research of the National Bureau of Standards 69B,
 *      125-130, 1965, doi:10.6028/jres.069B.013
 * \see H. N. Gabow, Data structures for weighted matching and nearest
 *      common ancestors with linking, Proceedings of the 1st ACM-SIAM
 *      Symposium on Discrete Algorithms, 434-443, 1990
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

#ifndef __ADS_Matching
 #define __ADS_Matching
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/MaxFlow.h"

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// bipartiteBMatching(): the pairs ( u , v ) of a b-matching of largest
/// size, A[ u ] the nodes on the right adjacent to the node u on the left,
/// u in at most capL[ u ] pairs and v in at most capR[ v ]
/// \pre the items of A in [ 0 , nr ), capL has a place per node on the
///      left and capR nr, all of them >= 0
/// \post the pairs are edges, within the capacities of their ends (audit)
inline std::vector< std::pair< int , int > > bipartiteBMatching(
                      const std::vector< std::vector< int > > & A , int nr ,
                      const std::vector< long > & capL ,
                      const std::vector< long > & capR );

/// maxWeightMatching(): the largest total weight of a matching of the
/// graph of n nodes and of the edges E = ( u , v , w ), and the places in
/// E of its edges, in increasing order
/// \pre n >= 0, each edge joins two distinct nodes in [ 0 , n )
/// \post no two edges given have an end in common (audit)
template< class W >
std::pair< W , std::vector< std::size_t > > maxWeightMatching( int n ,
                     const std::vector< std::tuple< int , int , W > > & E );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Matching.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Matching.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Matching.h ---------------------------*/
/*--------------------------------------------------------------------------*/
