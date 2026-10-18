/*--------------------------------------------------------------------------*/
/*------------------------- File RandomSampling.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The uniform sampling of n of N items, chapter 3 of the notes, in the
 * three settings there: the items on the disk with N known, a stream with
 * N known, a stream with N unknown. The samplings of positions return
 * them sorted, so that the items are then read by one pass, at most
 * \f$ \min\{ n , \lceil N / B \rceil \} \f$ I/Os [see DiskReader::at() of
 * TwoLevelMemory.h].
 *
 * - sampleBySwaps() is Algorithm 3.1, the first n steps of the shuffle of
 *   Fisher and Yates [see shuffle() of RandomNumbers.h] on an array of the
 *   N positions: n random numbers, but \f$ \Theta( N ) \f$ space and up to
 *   n random I/Os.
 * - sampleByDictionary() is Algorithm 3.2: random positions kept in a hash
 *   table, the repeated ones drawn again; with \f$ n \leq N / 2 \f$ a draw
 *   repeats with probability below 1/2, hence O( n ) expected time and
 *   space (Fact 3.1), and for n > N / 2 the N - n positions left out are
 *   drawn instead.
 * - sampleBySorting() is Algorithm 3.3 with the bucket sort of Fact 3.3:
 *   n random positions sorted by n buckets of width \f$ \lceil N / n
 *   \rceil \f$, O( 1 ) expected items each, sorted by insertion [see
 *   InsertionSort.h], and the repetitions removed; the positions missing
 *   are drawn again and merged, instead of all n as in the notes, which is
 *   the same distribution (the process is symmetric in the N positions)
 *   but needs no luck when n exceeds \f$ \sqrt{N} \f$ and the birthday
 *   paradox makes a repetition likely, \f$ 1 - e^{-n ( n - 1 ) / 2 N} \f$;
 *   for n > N / 2 the positions left out are drawn, as above.
 * - Algorithm 3.4, one pass that selects position j with probability
 *   \f$ ( n - s ) / ( N - j ) \f$, s the positions already selected, is
 *   selectionSampling() of RandomNumbers.h (Algorithm S of Knuth), N
 *   random numbers. sampleBySkips() is the method of Vitter that draws the
 *   gaps instead, \f$ P( G = g ) = \binom{V - g - 1}{v - 1} / \binom{V}{v}
 *   \f$ for v positions still to select among V: Algorithm D, by rejection
 *   with a continuous envelope, about n random numbers and O( n ) expected
 *   time, falling back to Algorithm A (the gap by the sequential search of
 *   its distribution, O( N ) time but n random numbers) once n exceeds
 *   N / 13, where A is faster.
 * - heapSampling() is Algorithm 3.5 for a stream of unknown length: a
 *   random key per item, the n largest keys kept in a heap [see
 *   DaryHeap.h], \f$ O( N \log n ) \f$ time; reservoirSampling() of
 *   RandomNumbers.h is Algorithm 3.6 (Algorithm R of Knuth), O( N ) time
 *   and exactly n items of space, optimal in this model.
 *
 * Compared with the literature: Vitter's Algorithm D is the method of
 * choice when N is known, Algorithm Z of the same author does the same for
 * the reservoir, drawing the number of items to skip, and the random keys
 * of heapSampling() are the start of the weighted sampling of Efraimidis
 * and Spirakis, where the key of an item of weight w is \f$ U^{1 / w} \f$.
 *
 * The contracts [see Contract.h]: n <= N; the positions are distinct,
 * below N and sorted, an audit. The functions are defined in
 * RandomSampling.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 3
 * \see J. S. Vitter, Faster methods for random sampling, Communications of
 *      the ACM 27(7), 703-718, 1984, doi:10.1145/358105.893
 * \see J. S. Vitter, An efficient algorithm for sequential random
 *      sampling, ACM Transactions on Mathematical Software 13(1), 58-67,
 *      1987, doi:10.1145/23002.23003
 * \see J. S. Vitter, Random sampling with a reservoir, ACM Transactions on
 *      Mathematical Software 11(1), 37-57, 1985, doi:10.1145/3147.3165
 * \see P. S. Efraimidis, P. G. Spirakis, Weighted random sampling with a
 *      reservoir, Information Processing Letters 97(5), 181-185, 2006,
 *      doi:10.1016/j.ipl.2005.11.003
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

/// n of the positions 0 , ... , N - 1, sorted, by n swaps in an array of
/// the N positions
/// \pre n <= N
/// \post n distinct positions below N, sorted (an audit)
template< class R >
std::vector< std::size_t > sampleBySwaps( std::size_t n , std::size_t N ,
                                          R & g );

/// n of the positions 0 , ... , N - 1, sorted, by a hash table of those
/// drawn
/// \pre n <= N
/// \post n distinct positions below N, sorted (an audit)
template< class R >
std::vector< std::size_t > sampleByDictionary( std::size_t n ,
                                               std::size_t N , R & g );

/// n of the positions 0 , ... , N - 1, sorted, by a bucket sort of random
/// positions; the rounds of draws go to *rounds, if given
/// \pre n <= N
/// \post n distinct positions below N, sorted (an audit)
template< class R >
std::vector< std::size_t > sampleBySorting( std::size_t n , std::size_t N ,
                                            R & g ,
                                            std::size_t * rounds = nullptr );

/// n of the positions 0 , ... , N - 1, sorted, by the gaps of Vitter's
/// Algorithm D (A when n > N / 13); the random numbers drawn go to
/// *randoms, if given
/// \pre n <= N
/// \post n distinct positions below N, sorted (an audit)
template< class R >
std::vector< std::size_t > sampleBySkips( std::size_t n , std::size_t N ,
                                          R & g ,
                                          std::size_t * randoms = nullptr );

/// n items of the stream S, in the order of S, by the n largest of random
/// keys, as if the length of S were unknown
/// \pre n <= the size of S
template< class T , class R >
std::vector< T > heapSampling( const std::vector< T > & S , std::size_t n ,
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
