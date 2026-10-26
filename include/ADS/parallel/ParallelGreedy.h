/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelGreedy.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The greedy algorithms on graphs in parallel, beyond the book, by the
 * deterministic reservations of speculativeFor() [see SpeculativeFor.h]
 * on the threads of an executor [see Executor.h]: each gives exactly the
 * result of the sequential greedy algorithm on the same order of the
 * edges or of the nodes, on every executor and whatever the number of its
 * threads.
 *
 * - parallelKruskal( ex , n , E ) is the algorithm of Kruskal (section
 *   14.3 of the book) on the order \f$ ( w( e ) , e ) \f$ of the edges, the
 *   ties of the weights broken by the place in E: the edges are sorted by
 *   the parallel sort [see ParallelSort.h], and then the edge i reserves
 *   the roots of its ends in a union-find, if they differ (else it is
 *   left out), and in its commit hangs the root it holds under the other.
 *   The union-find links the roots with no rank, and its find() halves the
 *   paths (a write of a node to its grandparent, which a concurrent find()
 *   reads either way, since no link happens while the finds run).
 * - filterKruskal( ex , n , E ) is the filter-Kruskal of Osipov, Sanders
 *   and Singler: if E is small it is the one above, else a pivot p (the
 *   median of a sample, in the order above) splits E into the light edges
 *   \f$ e < p \f$ and the heavy ones; the light ones are solved first,
 *   recursively, and then the heavy ones whose ends the forest already
 *   joins are filtered out, in parallel, before their recursion. A heavy
 *   edge left out would be left out by Kruskal too, and the light ones come
 *   first in the order, so that the forest is the same; the edges filtered
 *   out are never sorted, which on dense graphs saves most of the sort,
 *   for \f$ O( m + n \log n \log( m / n ) ) \f$ expected time on random
 *   weights.
 * - spanningForest( ex , n , E ) is the same on the order of E, with no
 *   sort: the forest that the union-find of section 10.2 gives scanning E.
 * - maximalIndependentSet( ex , g , \f$ \pi \f$ ) is the greedy maximal
 *   independent set in the order \f$ \pi \f$ of the nodes: the node of
 *   place i enters the set if no neighbour of an earlier place is in it.
 *   Its reserve() looks at the neighbours of earlier places: one in the
 *   set leaves it out, one still undecided makes it wait for the next
 *   round, and if all are out it enters; its commit() writes the decision
 *   (a node never waits for a later one, so that no reservation is
 *   needed).
 * - maximalMatching( ex , n , E , \f$ \pi \f$ ) is the greedy maximal
 *   matching in the order \f$ \pi \f$ of the edges: the edge i reserves its
 *   two ends if both are free, and in its commit takes them if it holds
 *   both.
 * With \f$ \pi \f$ a random permutation (randomOrder()), the iterations
 * that wait for each other form chains of length \f$ O( \log n ) \f$ with
 * high probability (Blelloch, Fineman and Shun; Fischer and Noever), and
 * the rounds of speculativeFor() few: the work is \f$ O( n + m ) \f$ for
 * the set and the matching, \f$ O( m \log m ) \f$ for Kruskal (the sort).
 * <pre>
 *   reserve( i ) of parallelKruskal, the edge \f$ e_i \f$ = [ u , v ]
 *  1  \f$ r_u \f$ ← find( u ); \f$ r_v \f$ ← find( v )
 *  2  if \f$ r_u = r_v \f$ then return false (left out)
 *  3  R[ \f$ r_u \f$ ].reserve( i ); R[ \f$ r_v \f$ ].reserve( i );
 *     return true
 *   commit( i )
 *  1  if R[ \f$ r_u \f$ ].check( i ) then free R[ \f$ r_u \f$ ] and R[
 *     \f$ r_v \f$ ] if i holds it; parent( \f$ r_u \f$ ) ← \f$ r_v \f$;
 *     take \f$ e_i \f$; return true
 *  2  if R[ \f$ r_v \f$ ].check( i ) then free it; parent( \f$ r_v \f$ ) ←
 *     \f$ r_u \f$; take \f$ e_i \f$; return true
 *  3  return false (tried again)
 *   filterKruskal( I ), I the edges still to scan
 *  1  if \f$ |I| \leq \max( 4096 , 2 n ) \f$ then parallelKruskal( I );
 *     return
 *  2  p ← the median of 1023 edges of I, evenly spaced, in the order
 *     \f$ ( w( e ) , e ) \f$
 *  3  \f$ I_< \f$ ← { e ∈ I : e < p }; \f$ I_\geq \f$ ← I -
 *     \f$ I_< \f$ (two parallel filters)
 *  4  filterKruskal( \f$ I_< \f$ )
 *  5  \f$ I_\geq \f$ ← { [ u , v ] ∈ \f$ I_\geq \f$ : find( u ) ≠
 *     find( v ) } (a parallel filter)
 *  6  filterKruskal( \f$ I_\geq \f$ )
 *   reserve( i ) of maximalMatching, the edge [ u , v ] of place i
 *  1  if u = v or u is matched or v is matched then return false
 *  2  R[ u ].reserve( i ); R[ v ].reserve( i ); return true
 *   commit( i )
 *  1  if R[ v ].check( i ) then free R[ v ]; if R[ u ].check( i ) then
 *     free R[ u ]; match u and v; take [ u , v ]; return true
 *  2  else free R[ u ] if i holds it
 *  3  return false (tried again)
 *   reserve( i ) of maximalIndependentSet, the node v of place i
 *  1  s ← IN
 *  2  for each neighbour u of v with \f$ \pi^{-1}( u ) < i \f$
 *  3   if state( u ) = IN then s ← OUT; break
 *  4   if state( u ) = UNDECIDED then s ← UNDECIDED
 *  5  next( v ) ← s; return true
 *   commit( i )
 *  1  state( v ) ← next( v ); return state( v ) ≠ UNDECIDED
 * </pre>
 *
 * The contracts [see Contract.h]: n >= 0, the ends of the edges in
 * [ 0 , n ) (a loop [ u , u ] joins nothing and matches nothing; parallel
 * edges are allowed); g is unoriented; an order is a permutation. At the
 * level of the audits each result is checked against the sequential
 * greedy algorithm.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 10.2 and
 *      14.3, https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see G. E. Blelloch, J. T. Fineman, P. B. Gibbons, J. Shun, Internally
 *      deterministic parallel algorithms can be fast, Proceedings of
 *      PPoPP, 181-192, 2012, doi:10.1145/2145816.2145840
 * \see G. E. Blelloch, J. T. Fineman, J. Shun, Greedy sequential maximal
 *      independent set and matching are parallel on average, Proceedings
 *      of SPAA, 308-317, 2012, doi:10.1145/2312005.2312058
 * \see M. Fischer, A. Noever, Tight analysis of parallel randomized greedy
 *      MIS, Proceedings of SODA, 2152-2160, 2018,
 *      doi:10.1137/1.9781611975031.140
 * \see V. Osipov, P. Sanders, J. Singler, The filter-Kruskal minimum
 *      spanning tree algorithm, Proceedings of ALENEX, 52-61, 2009,
 *      doi:10.1137/1.9781611972894.5
 * \see J. Shun, Y. Gu, G. E. Blelloch, J. T. Fineman, P. B. Gibbons,
 *      Sequential random permutation, list contraction and tree contraction
 *      are highly parallel, Proceedings of SODA, 431-448, 2015,
 *      doi:10.1137/1.9781611973730.30
 * \see L. Dhulipala, G. E. Blelloch, J. Shun, Theoretically efficient
 *      parallel graph algorithms can be fast and scalable, ACM
 *      Transactions on Parallel Computing 8(1), 1-70, 2021,
 *      doi:10.1145/3434393
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

#ifndef __ADS_ParallelGreedy
 #define __ADS_ParallelGreedy
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/SpeculativeFor.h"

#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a random permutation of [ 0 , n ) from the seed, the same on every
/// machine (the shuffle of Fisher and Yates on a SplitMix64 generator)
/// \pre n >= 0
[[nodiscard]] inline std::vector< int > randomOrder( int n ,
                                                     unsigned long long seed );

/// the places in E of the edges of the minimum spanning forest that
/// Kruskal gives on the order ( w , place ), in that order
/// \pre n >= 0, the ends of the edges in [ 0 , n )
/// \post the forest of the sequential greedy algorithm (audit)
template< Executor E , class W >
[[nodiscard]] std::vector< int > parallelKruskal( E & ex , int n ,
                                const std::vector< WeightedEdge< W > > & A );

/// the result of parallelKruskal() by filter-Kruskal
/// \pre n >= 0, the ends of the edges in [ 0 , n )
/// \post the forest of the sequential greedy algorithm (audit)
template< Executor E , class W >
[[nodiscard]] std::vector< int > filterKruskal( E & ex , int n ,
                                const std::vector< WeightedEdge< W > > & A );

/// the places in E of the edges of the spanning forest of the greedy
/// algorithm in the order of E, increasing
/// \pre n >= 0, the ends of the edges in [ 0 , n )
/// \post the forest of the sequential greedy algorithm (audit)
template< Executor E >
[[nodiscard]] std::vector< int > spanningForest( E & ex , int n ,
                          const std::vector< std::pair< int , int > > & A );

/// the nodes of the greedy maximal independent set of g in the order pi
/// (the node pi[ 0 ] first), increasing
/// \pre g is unoriented, pi is a permutation of its nodes
/// \post the set of the sequential greedy algorithm (audit)
template< Executor E , class W >
[[nodiscard]] std::vector< int > maximalIndependentSet( E & ex ,
                                 const StaticGraph< W > & g ,
                                 const std::vector< int > & pi );

/// the places in E of the edges of the greedy maximal matching in the
/// order pi of the edges (the edge E[ pi[ 0 ] ] first), increasing
/// \pre n >= 0, the ends of the edges in [ 0 , n ), pi is a permutation
///      of [ 0 , |E| )
/// \post the matching of the sequential greedy algorithm (audit)
template< Executor E >
[[nodiscard]] std::vector< int > maximalMatching( E & ex , int n ,
                          const std::vector< std::pair< int , int > > & A ,
                          const std::vector< int > & pi );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelGreedy.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelGreedy.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelGreedy.h -----------------------*/
/*--------------------------------------------------------------------------*/
