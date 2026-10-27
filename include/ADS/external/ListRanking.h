/*--------------------------------------------------------------------------*/
/*-------------------------- File ListRanking.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ranking of a list, the distance of each item from the tail, in the
 * memory and in the two-level memory [see `TwoLevelMemory.h`], following
 * chapter 4, List Ranking, of Ferragina's book. A list of \f$ n \f$ items
 * is the vector \f$ \mathit{succ} \f$ of their successors, the items being
 * \f$ 0 , \ldots , n - 1 \f$ and the tail \f$ t \f$ the only one with
 * \f$ \mathit{succ}[ t ] = t \f$.
 *
 * - `listRank()` is the sequential algorithm: the predecessors, then a walk
 *   back from the tail, \f$ O( n ) \f$ time, but up to one I/O per item on
 *   the disk, since the links jump anywhere.
 * - `listRankPointerJumping()` is the parallel algorithm of Wyllie: every
 *   item at once adds the rank of its successor to its own and jumps to the
 *   successor of its successor, so that the rank of \f$ i \f$ is the
 *   distance between \f$ i \f$ and its current successor;
 *   \f$ \lceil \log_2 n \rceil \f$ rounds, \f$ O( n \log n ) \f$
 *   operations.
 * - `parallelStep()` simulates one parallel step
 *   \f$ A[ a_i ] \gets \mathrm{op}( A[ a_i ] , A[ b_i ] ) \f$ on the disk by
 *   two sorts and three scans: the
 *   triples \f$ \langle a_i , b_i , \cdot \rangle \f$ sorted by \f$ b_i \f$,
 *   filled in by a scan of \f$ A \f$, sorted back by \f$ a_i \f$ and applied
 *   by a second scan of \f$ A \f$; any parallel algorithm of \f$ T \f$ steps
 *   on \f$ n \f$ processors becomes \f$ \tilde O( ( n / B ) T ) \f$ I/Os
 *   (Chiang et al.). `externalPointerJumping()` is the
 *   algorithm of Wyllie so simulated, the rank and the successor carried in
 *   one record, \f$ \tilde O( ( n / B ) \log n ) \f$ I/Os.
 * - The divide and conquer of the same chapter removes an
 *   independent set \f$ I \f$ (no item of \f$ I \f$ is the successor of
 *   another one of \f$ I \f$), letting the predecessors of its items jump
 *   over them, ranks the shorter list and puts the items of \f$ I \f$ back,
 *   each one adding the rank of its successor. `randomIndependentSet()`
 *   tosses a coin per item and takes the heads whose successor shows tails,
 *   \f$ n / 4 \f$ items on average; `deterministicCoinTossing()` is the
 *   technique of Cole and Vishkin: the colors start as the distinct indices
 *   and each round replaces the color of \f$ i \f$ by \f$ 2 \pi + z \f$,
 *   \f$ \pi \f$ the lowest bit where it differs from the color of the
 *   successor and \f$ z \f$ that bit of \f$ i \f$ (the tail compares with
 *   its bit 0), so that adjacent colors stay different while \f$ b \f$ bits
 *   become \f$ \lceil \log_2 b \rceil + 1 \f$, down to six colors in
 *   \f$ \log^* n \f$ rounds, and three by recoloring 5, 4 and 3;
 *   `independentSetOfColors()` takes the local minima, at least about
 *   \f$ n / 4 \f$ items. `listRankIndependentSet()` is the recursion in the
 *   memory with either set, and `externalListRank()` is the same recursion
 *   on the disk with the random coins, each step a join of records by sorts
 *   and scans,
 *   \f$ T( n ) = \tilde O( n / B ) + T( 7 n / 8 ) = \tilde O( n / B ) \f$
 *   I/Os (the coins are tossed again until
 *   \f$ | I | \ge n / 8 \f$, which happens after \f$ O( 1 ) \f$ attempts on
 *   average), the list in memory once it fits.
 *
 * Compared with the literature: the optimal parallel rankings of Cole and
 * Vishkin and of Anderson and Miller reach \f$ O( n ) \f$ operations in
 * \f$ O( \log n ) \f$ time by the same independent sets; on the disk the
 * bound \f$ \Theta( \mathrm{sort}( n ) ) \f$ of Chiang et al. is optimal,
 * and is what `externalListRank()` attains, while the deterministic
 * recursion on the disk would repeat the joins of the coins
 * \f$ \log^* n \f$ times per level, and is here only in the memory. The
 * records of the disk are atomic items of the model.
 *
 * The contracts [see `Contract.h`]: \f$ \mathit{succ} \f$ is a list, one
 * tail and every item reached from the only head, an audit; the operations
 * of a parallel step have distinct destinations and the indices are in
 * \f$ A \f$, an audit; the colors of adjacent items differ and the
 * independent sets are independent, audits; the files share the memory,
 * which is at least \f$ 3 B \f$, and the outputs are empty. The functions
 * are defined in `ListRanking.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 4, List Ranking, doi:10.1017/9781009128933
 * \see J. C. Wyllie, The complexity of parallel computations, PhD thesis,
 *      Cornell University, 1979
 * \see R. Cole, U. Vishkin, Deterministic coin tossing with applications
 *      to optimal parallel list ranking, Information and Control 70(1),
 *      32-53, 1986, doi:10.1016/S0019-9958(86)80023-7
 * \see R. J. Anderson, G. L. Miller, Deterministic parallel list ranking,
 *      Algorithmica 6, 859-868, 1991, doi:10.1007/BF01759076
 * \see Y.-J. Chiang, M. T. Goodrich, E. F. Grove, R. Tamassia, D. E.
 *      Vengroff, J. S. Vitter, External-memory graph algorithms,
 *      Proceedings of the 6th ACM-SIAM Symposium on Discrete Algorithms
 *      (SODA), 139-149, 1995
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

#ifndef __ADS_ListRanking
 #define __ADS_ListRanking
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/ExternalSorting.h"
#include "ADS/external/TwoLevelMemory.h"
#include "ADS/search/BinarySearch.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether \f$ \mathit{succ} \f$ is a list: one tail \f$ t \f$ with
/// \f$ \mathit{succ}[ t ] = t \f$, and every item reached from the only item
/// that is no successor
[[nodiscard]] inline bool isList( const std::vector< std::size_t > & succ );

/// the distances from the tail, by the predecessors and a walk back
///
/// In pseudocode (the sequential algorithm of chapter 4, List Ranking, of
/// Ferragina's book, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{listRank}( \mathit{succ} ) \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{with}\ \mathit{succ}[ i ] \ne i
///     \ \textbf{do}\ \mathit{pred}[ \mathit{succ}[ i ] ] \gets i \cr
/// 3 & \quad x \gets \text{the tail};\ r[ x ] \gets 0 \cr
/// 4 & \quad \textbf{while}\ x \text{ has a predecessor}\ \textbf{do} \cr
/// 5 & \qquad r[ \mathit{pred}[ x ] ] \gets r[ x ] + 1;\ x \gets
///     \mathit{pred}[ x ] \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit)
[[nodiscard]] inline std::vector< std::size_t > listRank(
                                    const std::vector< std::size_t > & succ );

/// the distances from the tail, by pointer jumping; the rounds go to
/// `*rounds`, if given
///
/// In pseudocode (Wyllie's algorithm, beyond the book), each round
/// synchronous, its reads before its writes:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{listRankPointerJumping}( \mathit{succ} )
///     \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ s[ i ] \gets \mathit{succ}[
///     i ];\ r[ i ] \gets ( 0 \text{ if } i \text{ is the tail, else } 1 )
///     \cr
/// 3 & \quad \textbf{repeat} \cr
/// 4 & \qquad \textbf{foreach}\ i\ \textbf{with}\ s[ i ] \ne s[ s[ i ] ]
///     \ \textbf{do}\quad \triangleright\ \text{in parallel} \cr
/// 5 & \qquad\quad r[ i ] \gets r[ i ] + r[ s[ i ] ];\ s[ i ] \gets s[ s[ i
///     ] ] \cr
/// 6 & \quad \textbf{until}\ \text{no item jumped} \cr
/// 7 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit)
[[nodiscard]] inline std::vector< std::size_t > listRankPointerJumping(
                                    const std::vector< std::size_t > & succ ,
                                    std::size_t * rounds = nullptr );

/// the heads whose successor shows tails, a coin per item drawn by `g`
///
/// In pseudocode (chapter 4, List Ranking, of Ferragina's book, beyond the
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{randomIndependentSet}( \mathit{succ} ) \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ c[ i ] \gets \text{a random }
///     \text{bit} \cr
/// 3 & \quad \textbf{return}\ \{ i : \mathit{succ}[ i ] \ne i ,\ c[ i ] = 1
///     ,\ c[ \mathit{succ}[ i ] ] = 0 \}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit)
/// \post an independent set, without the tail (an audit)
template< class R >
std::vector< bool > randomIndependentSet(
                          const std::vector< std::size_t > & succ , R & g );

/// three colors in \f$ \{ 0 , 1 , 2 \} \f$, by deterministic coin tossing;
/// the rounds down to six colors go to `*rounds`, if given
///
/// In pseudocode (the deterministic coin tossing of Cole and Vishkin,
/// beyond the book), \f$ \oplus \f$ the exclusive or of the bits:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{deterministicCoinTossing}( \mathit{succ}
///     ) \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ c[ i ] \gets i \cr
/// 3 & \quad \textbf{while}\ \max_i c[ i ] \ge 6\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ i\ \textbf{do}\quad \triangleright
///     \ \text{in parallel} \cr
/// 5 & \qquad\quad d \gets ( 1 \text{ if } i \text{ is the tail, else }
///     c[ i ] \oplus c[ \mathit{succ}[ i ] ] ) \cr
/// 6 & \qquad\quad \pi \gets \text{the lowest bit set in } d;\ c[ i ]
///     \gets 2 \pi + \text{the bit } \pi \text{ of } c[ i ] \cr
/// 7 & \quad \textbf{foreach}\ v \in ( 5 , 4 , 3 )\ \textbf{do} \cr
/// 8 & \qquad \textbf{foreach}\ i\ \textbf{with}\ c[ i ] = v\ \textbf{do} \cr
/// 9 & \qquad\quad c[ i ] \gets \text{the least color in } \{ 0 , 1 , 2 \}
///     \text{ of neither neighbour of } i \cr
/// 10 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit)
/// \post adjacent items have different colors (an audit)
[[nodiscard]] inline std::vector< int > deterministicCoinTossing(
                                    const std::vector< std::size_t > & succ ,
                                    std::size_t * rounds = nullptr );

/// the items whose color is smaller than those of their neighbours, the
/// tail excluded and not counted as a neighbour, so that a list of two
/// items or more gives at least one
///
/// In pseudocode (chapter 4, List Ranking, of Ferragina's book, beyond the
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{independentSetOfColors}( \mathit{succ} ,
///     c ) \cr
/// 2 & \quad \textbf{return}\ \{ i : i \text{ is not the tail, }
///     c[ i ] < c[ \mathit{pred}[ i ] ] \text{ unless } i \text{ is the }
///     \text{head,} \cr
/// 3 & \qquad c[ i ] < c[ \mathit{succ}[ i ] ] \text{ unless }
///     \mathit{succ}[ i ] \text{ is the tail} \}
/// \end{array}}
/// \f]
/// \pre adjacent items have different colors
/// \post an independent set (an audit)
[[nodiscard]] inline std::vector< bool > independentSetOfColors(
                                    const std::vector< std::size_t > & succ ,
                                    const std::vector< int > & color );

/// the distances from the tail, by removing independent sets, random ones
/// drawn by `g` or, if `deterministic`, the local minima of the three
/// colors; the levels of the recursion go to `*levels`, if given
///
/// In pseudocode (chapter 4, List Ranking, of Ferragina's book, beyond the
/// book), the
/// recursion unrolled into a loop over the levels:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{listRankIndependentSet}( \mathit{succ} )
///     \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ s[ i ] \gets \mathit{succ}[
///     i ];\ r[ i ] \gets ( 0 \text{ if } i \text{ is the tail, else } 1 )
///     \cr
/// 3 & \quad L \gets \text{all the items};\ \ell \gets 0 \cr
/// 4 & \quad \textbf{while}\ |L| > 1\ \textbf{do} \cr
/// 5 & \qquad I_\ell \gets \text{an independent set of the list } L
///     \text{, random with } |I_\ell| \ge |L| / 8 \text{ or of the colors}
///     \cr
/// 6 & \qquad \textbf{foreach}\ x \in L \setminus I_\ell\ \textbf{with}
///     \ s[ x ] \in I_\ell\ \textbf{do} \cr
/// 7 & \qquad\quad y \gets s[ x ];\ r[ x ] \gets r[ x ] + r[ y ];\ s[ x ]
///     \gets s[ y ] \cr
/// 8 & \qquad L \gets L \setminus I_\ell;\ \ell \gets \ell + 1 \cr
/// 9 & \quad \textbf{foreach}\ j \in ( \ell - 1 , \ldots , 1 , 0 )
///     \ \textbf{do} \cr
/// 10 & \qquad \textbf{foreach}\ x \in I_j\ \textbf{do}\ r[ x ] \gets r[ x
///     ] + r[ s[ x ] ] \cr
/// 11 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit)
/// \post the ranks of `listRank()` (an audit)
template< class R >
std::vector< std::size_t > listRankIndependentSet(
                                    const std::vector< std::size_t > & succ ,
                                    R & g , bool deterministic = false ,
                                    std::size_t * levels = nullptr );

/// \f$ \mathit{out}[ i ] = \mathrm{op}( A[ i ] , A[ b ] ) \f$ for each pair
/// \f$ ( i , b ) \f$ of `ops`, \f$ \mathit{out}[ i ] = A[ i ] \f$ for the
/// \f$ i \f$ of no pair, the values of \f$ A \f$ read before any change; two
/// sorts and three scans
///
/// In pseudocode (chapter 4, List Ranking, of Ferragina's book, beyond the
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{parallelStep}( A , \mathit{ops} ,
///     \mathrm{op} , \mathit{out} ) \cr
/// 2 & \quad P \gets \text{the triples } \langle a , b , \cdot \rangle
///     \text{ of the pairs } ( a , b ) \text{ of } \mathit{ops} \cr
/// 3 & \quad \text{sort } P \text{ by } b \cr
/// 4 & \quad \text{fill the third component of each triple with } A[ b ]
///     \text{, by a coordinated scan of } A \cr
/// 5 & \quad \text{sort } P \text{ by } a \cr
/// 6 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |A| - 1\ \textbf{do}
///     \quad \triangleright\ \text{a coordinated scan of } A \text{ and } P
///     \cr
/// 7 & \qquad \textbf{if}\ P \text{ has the triple } \langle i , b , v
///     \rangle\ \textbf{then}\ \text{append } \mathrm{op}( A[ i ] , v )
///     \text{ to } \mathit{out} \cr
/// 8 & \qquad \textbf{else}\ \text{append } A[ i ] \text{ to }
///     \mathit{out}
/// \end{array}}
/// \f]
/// \pre the first components of `ops` distinct (an audit), all indices
///      below the size of \f$ A \f$, `out` empty, the files on the same
///      memory
template< class T , class Op >
void parallelStep( const DiskFile< T > & A ,
                   const DiskFile< std::pair< std::size_t , std::size_t > > &
                                                                         ops ,
                   Op op , DiskFile< T > & out );

/// the ranks of the list `succ` into `rank`, by pointer jumping, each round
/// a `parallelStep()`; the rounds go to `*rounds`, if given
///
/// In pseudocode (Wyllie's algorithm simulated on the disk, beyond the
/// book), each
/// item a record \f$ \langle s , r \rangle \f$ on the disk:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{externalPointerJumping}( \mathit{succ} ,
///     \mathit{rank} ) \cr
/// 2 & \quad N \gets \text{the records } \langle \mathit{succ}[ i ] , ( 0
///     \text{ if } i \text{ is the tail, else } 1 ) \rangle \text{, by a }
///     \text{scan} \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \mathit{ops} \gets \text{the pairs } ( i , N[ i ].s )
///     \text{ with } N[ i ].s \text{ not the tail, by a scan} \cr
/// 5 & \qquad \textbf{if}\ \mathit{ops} = \emptyset\ \textbf{then} \cr
/// 6 & \qquad\quad \mathit{rank} \gets \text{the components } r \text{ of }
///     N \text{, by a scan};\ \textbf{return} \cr
/// 7 & \qquad N \gets \mathrm{parallelStep}( N , \mathit{ops} , ( x , y )
///     \mapsto \langle y.s , x.r + y.r \rangle )
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit), \f$ M \ge 3 B \f$,
///      `rank` empty, the files on the same memory
/// \post the ranks of `listRank()` (an audit)
inline void externalPointerJumping( const DiskFile< std::size_t > & succ ,
                                    DiskFile< std::size_t > & rank ,
                                    std::size_t * rounds = nullptr );

/// the ranks of the list `succ` into `rank`, by removing random independent
/// sets drawn by `g`; the levels of the recursion go to `*levels`, if given
///
/// In pseudocode (chapter 4, List Ranking, of Ferragina's book, beyond the
/// book), each
/// item a record \f$ \langle \mathit{id} , s , r \rangle \f$ and the records
/// sorted by \f$ \mathit{id} \f$; a join pairs each record with the record of
/// its successor, by two sorts and two scans:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rankRecords}( L ) \cr
/// 2 & \quad \textbf{if}\ 4 |L| \le M - B\ \textbf{then return}\ \text{the }
///     \text{records of } L \text{ ranked in memory} \cr
/// 3 & \quad \textbf{repeat} \cr
/// 4 & \qquad \text{toss a coin per record};\ I \gets \text{the heads }
///     \text{whose successor shows tails, by a join} \cr
/// 5 & \quad \textbf{until}\ 8 |I| \ge |L| \cr
/// 6 & \quad L' \gets \text{the records not in } I \text{, those whose }
///     \text{successor } y \text{ is in } I \text{ with} \cr
/// 7 & \qquad r \gets r + y.r \text{ and } s \gets y.s \text{, by a join} \cr
/// 8 & \quad R' \gets \mathrm{rankRecords}( L' ) \cr
/// 9 & \quad \textbf{foreach}\ x \in I\ \textbf{do}\ x.r \gets x.r + R'[
///     x.s ].r\quad \triangleright\ \text{by a join} \cr
/// 10 & \quad \textbf{return}\ R' \text{ and } I \text{ merged by }
///     \mathit{id}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{succ} \f$ is a list (an audit), \f$ M \ge 3 B \f$,
///      `rank` empty, the files on the same memory
/// \post the ranks of `listRank()` (an audit)
template< class R >
void externalListRank( const DiskFile< std::size_t > & succ ,
                       DiskFile< std::size_t > & rank , R & g ,
                       std::size_t * levels = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/ListRanking.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListRanking.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ListRanking.h -------------------------*/
/*--------------------------------------------------------------------------*/
