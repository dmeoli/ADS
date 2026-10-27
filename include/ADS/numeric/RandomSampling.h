/*--------------------------------------------------------------------------*/
/*------------------------- File RandomSampling.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The uniform sampling of \f$ n \f$ of \f$ N \f$ items, as chapter 3 (Random
 * Sampling) of Ferragina's Pearls of Algorithm Engineering treats it, in the
 * three settings there: the items on the disk with \f$ N \f$ known, a stream
 * with \f$ N \f$ known, a stream with \f$ N \f$ unknown. The samplings of
 * positions return them sorted, so that the items are then read by one pass,
 * at most \f$ \min\{ n , \lceil N / B \rceil \} \f$ I/Os [see
 * `DiskReader::at()` of `TwoLevelMemory.h`].
 *
 * - `sampleBySwaps()` is the first algorithm of the chapter, the first
 *   \f$ n \f$ steps of the shuffle of Fisher and Yates [see `shuffle()` of
 *   `RandomNumbers.h`] on an array of the \f$ N \f$ positions: \f$ n \f$
 *   random numbers, but \f$ \Theta( N ) \f$ space and up to \f$ n \f$ random
 *   I/Os.
 * - `sampleByDictionary()` keeps the random positions in a hash
 *   table, the repeated ones drawn again; with \f$ n \leq N / 2 \f$ a draw
 *   repeats with probability below \f$ 1 / 2 \f$, hence \f$ O( n ) \f$
 *   expected time and space, and for \f$ n > N / 2 \f$ the
 *   \f$ N - n \f$ positions left out are drawn instead.
 * - `sampleBySorting()` sorts the positions drawn by a bucket sort:
 *   \f$ n \f$ random positions sorted by \f$ n \f$ buckets of width
 *   \f$ \lceil N / n \rceil \f$, \f$ O( 1 ) \f$ expected items each, sorted
 *   by insertion [see `InsertionSort.h`], and the repetitions removed; the
 *   positions missing are drawn again and merged, instead of all \f$ n \f$ as
 *   in the chapter, which is the same distribution (the process is symmetric
 *   in the \f$ N \f$ positions) but needs no luck when \f$ n \f$ exceeds
 *   \f$ \sqrt{N} \f$ and the birthday paradox makes a repetition likely,
 *   \f$ 1 - e^{-n ( n - 1 ) / 2 N} \f$; for \f$ n > N / 2 \f$ the positions
 *   left out are drawn, as above.
 * - The one pass that selects the position \f$ j \f$ with
 *   probability \f$ ( n - s ) / ( N - j ) \f$, \f$ s \f$ the positions
 *   already selected, is `selectionSampling()` of `RandomNumbers.h`
 *   (Algorithm S of Knuth), \f$ N \f$ random numbers. `sampleBySkips()` is
 *   the method of Vitter that draws the gaps instead,
 *   \f$ P( G = g ) = \binom{V - g - 1}{v - 1} / \binom{V}{v} \f$ for
 *   \f$ v \f$ positions still to select among \f$ V \f$: Algorithm D, by
 *   rejection with a continuous envelope, about \f$ n \f$ random numbers and
 *   \f$ O( n ) \f$ expected time, falling back to Algorithm A (the gap by the
 *   sequential search of its distribution, \f$ O( N ) \f$ time but \f$ n \f$
 *   random numbers) once \f$ n \f$ exceeds \f$ N / 13 \f$, where A is
 *   faster.
 * - `heapSampling()` is the sampling for a stream of unknown length: a
 *   random key per item, the \f$ n \f$ largest keys kept in a heap [see
 *   `DaryHeap.h`], \f$ O( N \log n ) \f$ time; `reservoirSampling()` of
 *   `RandomNumbers.h` is the reservoir (Algorithm R of Knuth),
 *   \f$ O( N ) \f$ time and exactly \f$ n \f$ items of space, optimal in this
 *   model.
 * - `reservoirSampleL()` is Algorithm L of Li, beyond the chapter, the
 *   reservoir for a stream of unknown length that draws the number of items
 *   to skip instead of a number per item: with a uniform key per item, the
 *   reservoir holds the \f$ n \f$ items of the smallest keys, and \f$ W \f$,
 *   the largest of them, is distributed as a product of \f$ n \f$-th roots
 *   of uniforms, \f$ W \leftarrow W U^{1/n} \f$ at each entry; the next item
 *   whose key is below \f$ W \f$ comes after a geometric number of items,
 *   \f$ S = \lfloor \ln U' / \ln( 1 - W ) \rfloor \f$,
 *   \f$ P( S = s ) = ( 1 - W )^s W \f$, and replaces a uniform item of the
 *   reservoir. The item \f$ t > n \f$ enters with probability \f$ n / t \f$,
 *   so that \f$ n ( H_N - H_n ) \approx n \ln( N / n ) \f$ items enter, each
 *   for 3 random numbers: \f$ O( n ( 1 + \log( N / n ) ) ) \f$ of them in all
 *   and, on a random access range, as much time, the skipped items never
 *   touched.
 *
 * Compared with the literature: Vitter's Algorithm D is the method of choice
 * when \f$ N \f$ is known, Algorithm Z of the same author does the same for
 * the reservoir as Algorithm L, with a rejection that is more complex and no
 * faster, and the random keys of `heapSampling()` are the start of the
 * weighted sampling of Efraimidis and Spirakis, where the key of an item of
 * weight \f$ w \f$ is \f$ U^{1 / w} \f$.
 *
 * The contracts [see `Contract.h`]: \f$ n \le N \f$; the positions are
 * distinct, below \f$ N \f$ and sorted, an audit. The functions are defined
 * in `RandomSampling.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 3 (Random Sampling),
 *      doi:10.1017/9781009128933
 * \see J. S. Vitter, Faster methods for random sampling, Communications of
 *      the ACM 27(7), 703-718, 1984, doi:10.1145/358105.893
 * \see J. S. Vitter, An efficient algorithm for sequential random
 *      sampling, ACM Transactions on Mathematical Software 13(1), 58-67,
 *      1987, doi:10.1145/23002.23003
 * \see J. S. Vitter, Random sampling with a reservoir, ACM Transactions on
 *      Mathematical Software 11(1), 37-57, 1985, doi:10.1145/3147.3165
 * \see K.-H. Li, Reservoir-sampling algorithms of time complexity
 *      \f$ O( n ( 1 + \log( N / n ) ) ) \f$, ACM Transactions on Mathematical
 *      Software 20(4), 481-493, 1994, doi:10.1145/198429.198435
 * \see P. S. Efraimidis, P. G. Spirakis, Weighted random sampling with a
 *      reservoir, Information Processing Letters 97(5), 181-185, 2006,
 *      doi:10.1016/j.ipl.2005.11.003
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

#ifndef __ADS_RandomSampling
 #define __ADS_RandomSampling
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/sort/InsertionSort.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// \f$ n \f$ of the positions \f$ 0 , \ldots , N - 1 \f$, sorted, by
/// \f$ n \f$ swaps in an array of the \f$ N \f$ positions
///
/// In pseudocode (the sampling by swaps of Ferragina, chapter 3, beyond
/// the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sampleBySwaps}( n , N ) \cr
/// 2 & \quad P \gets ( 0 , 1 , \ldots , N - 1 ) \cr
/// 3 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \mathrm{swap}( P[ \text{a uniform of } [ 0 , N - s ) ] , P[ N
///     - 1 - s ] ) \cr
/// 5 & \quad \textbf{return}\ P[ N - n .. N - 1 ] \text{ sorted}
/// \end{array}}
/// \f]
/// \pre \f$ n \le N \f$
/// \post \f$ n \f$ distinct positions below \f$ N \f$, sorted (audit)
template< class R >
std::vector< std::size_t > sampleBySwaps( std::size_t n , std::size_t N ,
                                          R & g );

/// \f$ n \f$ of the positions \f$ 0 , \ldots , N - 1 \f$, sorted, by a hash
/// table of those drawn
///
/// In pseudocode (the dictionary of Ferragina, chapter 3, beyond the book),
/// the
/// complement drawn when it is smaller:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sampleByDictionary}( n , N ) \cr
/// 2 & \quad m \gets \min( n , N - n );\ D \gets \text{an empty hash table}
///     \cr
/// 3 & \quad \textbf{while}\ |D| < m\ \textbf{do}\ D \gets D \cup \{
///     \text{a uniform of } [ 0 , N ) \} \cr
/// 4 & \quad \textbf{if}\ m = n\ \textbf{then return}\ D \text{ sorted} \cr
/// 5 & \quad \textbf{return}\ \text{the positions not in } D \text{, in }
///     \text{order}
/// \end{array}}
/// \f]
/// \pre \f$ n \le N \f$
/// \post \f$ n \f$ distinct positions below \f$ N \f$, sorted (audit)
template< class R >
std::vector< std::size_t > sampleByDictionary( std::size_t n ,
                                               std::size_t N , R & g );

/// \f$ n \f$ of the positions \f$ 0 , \ldots , N - 1 \f$, sorted, by a
/// bucket sort of random positions; the rounds of draws go to `*rounds`, if
/// given
///
/// In pseudocode (the sampling by sorting of Ferragina, chapter 3, beyond
/// the book), \f$ D \f$
/// sorted and with no repetition:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sampleBySorting}( n , N ) \cr
/// 2 & \quad m \gets \min( n , N - n );\ D \gets \emptyset \cr
/// 3 & \quad \textbf{while}\ |D| < m\ \textbf{do} \cr
/// 4 & \qquad k \gets m - |D|;\ X \gets k \text{ uniforms of } [ 0 , N ) \cr
/// 5 & \qquad \text{sort } X \text{ into } k \text{ buckets of width }
///     \lceil N / k \rceil \text{, each by insertion} \cr
/// 6 & \qquad D \gets \text{the merge of } D \text{ and } X \text{, the }
///     \text{repetitions removed} \cr
/// 7 & \quad \textbf{if}\ m = n\ \textbf{then return}\ D \cr
/// 8 & \quad \textbf{return}\ \text{the positions not in } D \text{, in }
///     \text{order}
/// \end{array}}
/// \f]
/// \pre \f$ n \le N \f$
/// \post \f$ n \f$ distinct positions below \f$ N \f$, sorted (audit)
template< class R >
std::vector< std::size_t > sampleBySorting( std::size_t n , std::size_t N ,
                                            R & g ,
                                            std::size_t * rounds = nullptr );

/// \f$ n \f$ of the positions \f$ 0 , \ldots , N - 1 \f$, sorted, by the
/// gaps of Vitter's Algorithm D (A when \f$ 13 n \ge N \f$); the random
/// numbers drawn go to `*randoms`, if given
///
/// In pseudocode (Algorithms A and D of Vitter, beyond the book), \f$ v \f$
/// positions still to select among the \f$ V \f$ still to see:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sampleBySkips}( n , N ) \cr
/// 2 & \quad v \gets n;\ V \gets N;\ \mathit{cur} \gets 0 \cr
/// 3 & \quad \textbf{while}\ v > 1\ \textbf{and}\ 13 v < V\ \textbf{do} \cr
/// 4 & \qquad \textbf{repeat}\ X \gets V ( 1 - U^{1/v} );\ S \gets \lfloor X
///     \rfloor\quad \triangleright\ \text{the envelope} \cr
/// 5 & \qquad \textbf{until}\ S \text{ is accepted against }
///     P( G = S ) \text{ (the tests D3 and D4)} \cr
/// 6 & \qquad \text{select } \mathit{cur} + S;\ \mathit{cur} \gets
///     \mathit{cur} + S + 1;\ V \gets V - S - 1;\ v \gets v - 1 \cr
/// 7 & \quad \textbf{while}\ v > 0\ \textbf{do}\quad \triangleright
///     \ \text{Algorithm A} \cr
/// 8 & \qquad S \gets \text{the least } s \text{ with }
///     P( G > s ) \le U \text{, by a sequential search} \cr
/// 9 & \qquad \text{select } \mathit{cur} + S;\ \mathit{cur} \gets
///     \mathit{cur} + S + 1;\ V \gets V - S - 1;\ v \gets v - 1
/// \end{array}}
/// \f]
/// \pre \f$ n \le N \f$
/// \post \f$ n \f$ distinct positions below \f$ N \f$, sorted (audit)
template< class R >
std::vector< std::size_t > sampleBySkips( std::size_t n , std::size_t N ,
                                          R & g ,
                                          std::size_t * randoms = nullptr );

/// \f$ n \f$ items of the stream \f$ S \f$, in the order of \f$ S \f$, by
/// the \f$ n \f$ largest of random keys, as if the length of \f$ S \f$ were
/// unknown
///
/// In pseudocode (the heap of random keys of Ferragina, chapter 3, beyond
/// the book), \f$ H \f$ a
/// heap with the least key on top:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{heapSampling}( S , n ) \cr
/// 2 & \quad \textbf{foreach}\ j \gets 0 , 1 , \ldots , |S| - 1\ \textbf{do}
///     \cr
/// 3 & \qquad r \gets \text{a uniform of } ( 0 , 1 ) \cr
/// 4 & \qquad \textbf{if}\ |H| < n\ \textbf{then}\ \mathrm{insert}( H , ( r
///     , j ) ) \cr
/// 5 & \qquad \textbf{else if}\ r > \text{the least key of } H\ \textbf{then}
///     \ \mathrm{deleteMin}( H );\ \mathrm{insert}( H , ( r , j ) ) \cr
/// 6 & \quad \textbf{return}\ \text{the items } S[ j ] \text{ of } H
///     \text{, by } j
/// \end{array}}
/// \f]
/// \pre \f$ n \le |S| \f$
template< class T , class R >
std::vector< T > heapSampling( const std::vector< T > & S , std::size_t n ,
                               R & g );

/// \f$ n \f$ items of \f$ [ \mathit{first} , \mathit{last} ) \f$, a stream
/// of unknown length, by the skips of Algorithm L, in no particular order;
/// all of them, in their order, when the stream has at most \f$ n \f$ items
///
/// In pseudocode (Algorithm L of Li, beyond the book), \f$ U \f$ a fresh
/// uniform of \f$ ( 0 , 1 ) \f$ at each use:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reservoirSampleL}( \mathit{stream} , n ) \cr
/// 2 & \quad R \gets \text{the first } n \text{ items};\ W \gets U^{1/n} \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad s \gets \lfloor \ln U / \ln( 1 - W ) \rfloor \cr
/// 5 & \qquad \text{skip } s \text{ items};\ \textbf{if}\ \text{the stream }
///     \text{ended}\ \textbf{then return}\ R \cr
/// 6 & \qquad R[ \text{a uniform of } [ 0 , n ) ] \gets \text{the next item}
///     \cr
/// 7 & \qquad W \gets W U^{1/n}
/// \end{array}}
/// \f]
template< std::input_iterator I , std::sentinel_for< I > S , class R >
std::vector< std::iter_value_t< I > > reservoirSampleL( I first , S last ,
                                                        std::size_t n ,
                                                        R & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/RandomSampling.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RandomSampling.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File RandomSampling.h ------------------------*/
/*--------------------------------------------------------------------------*/
