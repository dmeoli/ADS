/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelGreedy.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The greedy algorithms on graphs in parallel, beyond the book, by the
 * deterministic reservations of `speculativeFor()` [see `SpeculativeFor.h`]
 * on the threads of an executor [see `Executor.h`]: each gives exactly the
 * result of the sequential greedy algorithm on the same order of the edges
 * or of the nodes, on every executor and whatever the number of its
 * threads (see \ref theory_parallel_reserve). They are the algorithm of
 * Kruskal (`parallelKruskal()`, section 14.3 of the book) and its
 * filter-Kruskal variant (`filterKruskal()`), the spanning forest in the
 * order of the edges (`spanningForest()`, by the union-find of section
 * 10.2), and the greedy maximal independent set and maximal matching in a
 * given order (`maximalIndependentSet()`, `maximalMatching()`); on a random
 * order (`randomOrder()`) the iterations that wait for each other form
 * short chains, and the rounds are few.
 *
 * The union-find of the forests links the roots with no rank, and its
 * `find()` halves the paths (a write of a node to its grandparent, which a
 * concurrent `find()` reads either way, since no link happens while the
 * finds run). On one thread, or on fewer than 4096 items, the sequential
 * greedy algorithm runs instead.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 0 \f$, the ends of the edges
 * in \f$ [ 0 , n ) \f$ (a loop \f$ [ u , u ] \f$ joins nothing and matches
 * nothing; parallel edges are allowed); the graph is unoriented; an order is
 * a permutation. At the level of the audits each result is checked against
 * the sequential greedy algorithm.
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

/// a random permutation of \f$ [ 0 , n ) \f$ from the seed, the same on
/// every machine (the shuffle of Fisher and Yates on a SplitMix64
/// generator)
/// \pre \f$ n \ge 0 \f$
[[nodiscard]] inline std::vector< int > randomOrder( int n ,
                                                     unsigned long long seed );

/// the places in \f$ A \f$ of the edges of the minimum spanning forest
/// that Kruskal gives on the order \f$ ( w , \mathit{place} ) \f$, in that
/// order
///
/// In pseudocode (section 14.3, on the deterministic reservations of
/// Blelloch, Fineman, Gibbons and Shun), the two steps of the edge
/// \f$ e_i = [ u , v ] \f$ of place \f$ i \f$ in the order, \f$ R \f$ the
/// reservations of the roots:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reserve}( i ) \cr
/// 2 & \quad r_u \gets \mathrm{find}( u );\ r_v \gets \mathrm{find}( v ) \cr
/// 3 & \quad \textbf{if}\ r_u = r_v\ \textbf{then}\ \textbf{return}
///     \ \textbf{false} \quad \triangleright\ \text{left out} \cr
/// 4 & \quad \mathrm{reserve}( R[ r_u ] , i );\ \mathrm{reserve}( R[ r_v ] ,
///     i );\ \textbf{return}\ \textbf{true} \cr
/// 5 & \textbf{function}\ \mathrm{commit}( i ) \cr
/// 6 & \quad \textbf{if}\ R[ r_u ] \text{ holds } i\ \textbf{then}
///     \ \text{free } R[ r_u ] \text{, and } R[ r_v ] \text{ if it holds }
///     i;\ p[ r_u ] \gets r_v \cr
/// & \qquad \text{take } e_i;\ \textbf{return}\ \textbf{true} \cr
/// 7 & \quad \textbf{if}\ R[ r_v ] \text{ holds } i\ \textbf{then}
///     \ \text{free it};\ p[ r_v ] \gets r_u;\ \text{take } e_i;
///     \ \textbf{return}\ \textbf{true} \cr
/// 8 & \quad \textbf{return}\ \textbf{false} \quad \triangleright
///     \ \text{tried again}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the ends of the edges in \f$ [ 0 , n ) \f$
/// \post the forest of the sequential greedy algorithm (audit)
template< Executor E , class W >
[[nodiscard]] std::vector< int > parallelKruskal( E & ex , int n ,
                                const std::vector< WeightedEdge< W > > & A );

/// the forest of `parallelKruskal()` above, by filter-Kruskal
///
/// In pseudocode (Osipov, Sanders and Singler, beyond the book), with
/// \f$ I \f$ the edges still to scan, all of them at first:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{filterKruskal}( I ) \cr
/// 2 & \quad \textbf{if}\ |I| \le \max( 4096 , 2 n )\ \textbf{then}
///     \ \mathrm{parallelKruskal}( I );\ \textbf{return} \cr
/// 3 & \quad p \gets \text{the median of 1023 edges of } I
///     \text{, evenly spaced, by } ( w( e ) , e ) \cr
/// 4 & \quad I_< \gets \{ e \in I : e < p \};\ I_\ge \gets I \setminus
///     I_< \quad \triangleright\ \text{two parallel filters} \cr
/// 5 & \quad \mathrm{filterKruskal}( I_< ) \cr
/// 6 & \quad I_\ge \gets \{ [ u , v ] \in I_\ge : \mathrm{find}( u ) \ne
///     \mathrm{find}( v ) \} \quad \triangleright\ \text{a parallel filter}
///     \cr
/// 7 & \quad \mathrm{filterKruskal}( I_\ge )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the ends of the edges in \f$ [ 0 , n ) \f$
/// \post the forest of the sequential greedy algorithm (audit)
template< Executor E , class W >
[[nodiscard]] std::vector< int > filterKruskal( E & ex , int n ,
                                const std::vector< WeightedEdge< W > > & A );

/// the places in \f$ A \f$ of the edges of the spanning forest of the
/// greedy algorithm in the order of \f$ A \f$, increasing
///
/// In pseudocode (section 10.2), the steps of `parallelKruskal()` above on
/// the edges in the order of \f$ A \f$, with no sort:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{spanningForest}( n , A ) \cr
/// 2 & \quad \textbf{return}\ \text{the edges that } \mathrm{reserve}
///     \text{ and } \mathrm{commit} \text{ of } \mathrm{parallelKruskal}
///     \text{ take on } A \text{ in its order}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the ends of the edges in \f$ [ 0 , n ) \f$
/// \post the forest of the sequential greedy algorithm (audit)
template< Executor E >
[[nodiscard]] std::vector< int > spanningForest( E & ex , int n ,
                          const std::vector< std::pair< int , int > > & A );

/// the nodes of the greedy maximal independent set of \f$ g \f$ in the
/// order \f$ \pi \f$ (the node \f$ \pi[ 0 ] \f$ first), increasing
///
/// In pseudocode (Blelloch, Fineman and Shun, beyond the book), the two
/// steps of the node \f$ v = \pi[ i ] \f$; a node never waits for a later
/// one, so that no reservation is needed:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reserve}( i ) \cr
/// 2 & \quad s \gets \mathrm{IN} \cr
/// 3 & \quad \textbf{foreach}\ u \in g.\mathrm{adj}( v )\ \textbf{with}
///     \ \pi^{-1}( u ) < i\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{state}[ u ] = \mathrm{IN}\ \textbf{then}
///     \ s \gets \mathrm{OUT};\ \textbf{break} \cr
/// 5 & \qquad \textbf{if}\ \mathit{state}[ u ] = \mathrm{UNDECIDED}
///     \ \textbf{then}\ s \gets \mathrm{UNDECIDED} \cr
/// 6 & \quad \mathit{next}[ v ] \gets s;\ \textbf{return}\ \textbf{true} \cr
/// 7 & \textbf{function}\ \mathrm{commit}( i ) \cr
/// 8 & \quad \mathit{state}[ v ] \gets \mathit{next}[ v ];\ \textbf{return}
///     \ \mathit{state}[ v ] \ne \mathrm{UNDECIDED}
/// \end{array}}
/// \f]
/// \pre \f$ g \f$ is unoriented, \f$ \pi \f$ is a permutation of its nodes
/// \post the set of the sequential greedy algorithm (audit)
template< Executor E , class W >
[[nodiscard]] std::vector< int > maximalIndependentSet( E & ex ,
                                 const StaticGraph< W > & g ,
                                 const std::vector< int > & pi );

/// the places in \f$ A \f$ of the edges of the greedy maximal matching in
/// the order \f$ \pi \f$ of the edges (the edge \f$ A[ \pi[ 0 ] ] \f$
/// first), increasing
///
/// In pseudocode (Blelloch, Fineman and Shun, beyond the book), the two
/// steps of the edge \f$ [ u , v ] = A[ \pi[ i ] ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reserve}( i ) \cr
/// 2 & \quad \textbf{if}\ u = v \textbf{ or } u \text{ is matched}
///     \textbf{ or } v \text{ is matched}\ \textbf{then}\ \textbf{return}
///     \ \textbf{false} \cr
/// 3 & \quad \mathrm{reserve}( R[ u ] , i );\ \mathrm{reserve}( R[ v ] , i );
///     \ \textbf{return}\ \textbf{true} \cr
/// 4 & \textbf{function}\ \mathrm{commit}( i ) \cr
/// 5 & \quad \textbf{if}\ R[ v ] \text{ holds } i\ \textbf{then}
///     \ \text{free } R[ v ] \cr
/// 6 & \qquad \textbf{if}\ R[ u ] \text{ holds } i\ \textbf{then}
///     \ \text{free } R[ u ];\ \text{match } u \text{ and } v;
///     \ \textbf{return}\ \textbf{true} \cr
/// 7 & \quad \textbf{else}\ \text{free } R[ u ] \text{ if it holds } i \cr
/// 8 & \quad \textbf{return}\ \textbf{false} \quad \triangleright
///     \ \text{tried again}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the ends of the edges in \f$ [ 0 , n ) \f$,
///      \f$ \pi \f$ is a permutation of \f$ [ 0 , |A| ) \f$
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
