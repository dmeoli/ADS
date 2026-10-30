/*--------------------------------------------------------------------------*/
/*---------------------------- File RadixSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts by the digits of the keys, beyond the book (which has Counting
 * Sort and Pigeonhole Sort, the one-digit cases, in `CountingSort.h` and
 * `PigeonholeSort.h`):
 *
 * - `radixSort()` sorts integers and floating point numbers by their bytes,
 *   from the least significant (LSD), each pass a stable Counting Sort on one
 *   byte between the vector and a buffer: \f$ O( w ( n + 256 ) ) \f$ for keys
 *   of \f$ w \f$ bytes, no comparison. The key of a number is its bits made
 *   unsigned so that their order is the order of the numbers: the sign bit
 *   flipped for a signed integer, and for a floating point number all the bits
 *   flipped if it is negative and the sign bit otherwise (the order of IEEE
 *   754, \f$ -0 \f$ before \f$ +0 \f$, the NaN at the ends). A first sweep
 *   finds the bytes in which some keys differ, and only those are counted (all
 *   the histograms in a second sweep) and make a pass, so that small keys in
 *   wide types cost only the bytes they use; when they differ in one byte
 *   alone, the key tells the number, and a counting sort writes the numbers
 *   back from one of each value, with no buffer. A vector larger than the
 *   cache (1 MiB), whose keys differ in 3 bytes or more, first goes by its top
 *   byte into the buffer, the one pass that scatters over the whole memory,
 *   and each part is sorted in the same way and then by the other bytes while
 *   it fits, so that their passes stay in the cache (an MSD pass before the
 *   LSD ones);
 * - `radixSortBy()` is the same on any items, by an unsigned key that a
 *   function gives, stable, e.g., the records by one field;
 * - `americanFlagSort()` is the radix sort from the most significant byte
 *   (MSD), in place: two scans that stop at the first pair of keys out of
 *   their order find a vector already sorted or in reverse order
 *   (reversed), and a sweep the bytes in which some keys differ (one alone:
 *   the counting sort of `radixSort()`); then the items are permuted
 *   into the 256 buckets of the first byte that differs, as the flag of the
 *   name divides into stripes, and each bucket is sorted in the same way by
 *   the next one. The permutation does not follow its cycles, whose every
 *   step waits for the load of the step before: each item of the part of a
 *   bucket not yet in place is swapped to the next free place of its own
 *   bucket, and the buckets are taken in turn until all are full, so that
 *   the loads of a round are independent (Skarupke). A bucket of fewer than
 *   `kFlagSmall` items is left to `vectorQuickSort()` [see
 *   `VectorQuickSort.h`], whose order on the floating point numbers is the
 *   one of the keys, or, without the vector registers, to `pdqSort()` [see
 *   `EngineeredQuickSorts.h`]: \f$ O( w n ) \f$ for keys of \f$ w \f$
 *   bytes, \f$ O( 1 ) \f$ extra space per level of the recursion and no
 *   buffer of the items, not stable;
 * - `msdRadixSort()` sorts strings by their characters from the first one, the
 *   end of a string before every character: the first pass permutes them into
 *   their buckets in place, as the American flag sort does, and then each
 *   bucket has a buffer, made only if some bucket is not all of equal strings,
 *   between which and the vector the strings go back and forth, one move per
 *   pass, the character of each string read once per pass and kept aside, and
 *   the small groups sorted by insertion, so that it reads only the
 *   distinguishing prefixes of the strings: \f$ O( D + n \log 256 ) \f$ for
 *   \f$ D \f$ the total length of those prefixes; the prefix that all the
 *   strings of a group share is first skipped by comparing them with one of
 *   them (one `memcmp()` for a string that keeps the whole prefix), so that a
 *   group of equal strings, or of strings with a long common prefix, costs one
 *   comparison instead of one pass per character;
 * - `bucketSort()` sorts numbers in \f$ [ 0 , 1 ) \f$ into \f$ n \f$ buckets
 *   by their value, each sorted by insertion: \f$ O( n ) \f$ expected time on
 *   numbers drawn uniformly. The buckets are no lists: the items are counted
 *   by their bucket and scattered into a buffer in the order of the buckets,
 *   and one insertion sort over the whole vector sorts them, since no item
 *   has to leave its bucket; a vector larger than the cache is first
 *   scattered by groups of 1024 consecutive buckets, so that the counts and
 *   the scatter of each group stay in the cache, and a bucket of more than 32
 *   items goes to `pdqSort()`, so that the worst case is
 *   \f$ O( n \log n ) \f$ instead of the \f$ \Theta( n^2 ) \f$ of the
 *   classical one.
 *
 * The contracts [see `Contract.h`]: each sort ends with the same items sorted,
 * an audit; `bucketSort()` needs numbers in \f$ [ 0 , 1 ) \f$. The functions
 * are defined in `RadixSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.5
 *      (Counting Sort) and exercise 4.11 (Pigeonhole Sort),
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see P. M. McIlroy, K. Bostic, M. D. McIlroy, Engineering radix sort,
 *      Computing Systems 6(1), 5-27, 1993
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, sections 8.3 and 8.4
 * \see M. Herf, Radix tricks, 2001, http://stereopsis.com/radix.html (the
 *      key of a floating point number, the histograms in one sweep)
 * \see M. Skarupke, I wrote a faster sorting algorithm, 2016,
 *      (the permutation by rounds of independent swaps),
 *   https://probablydance.com/2016/12/27/i-wrote-a-faster-sorting-algorithm/
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

#ifndef __ADS_RadixSort
 #define __ADS_RadixSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/Sorted.h"
#include "ADS/sort/VectorQuickSort.h"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// an integer or a floating point number of at most 8 bytes
template< class T >
concept RadixKey = ( std::is_integral_v< T > ||
                     std::is_floating_point_v< T > ) && ( sizeof( T ) <= 8 );

/// the unsigned key of \f$ x \f$, whose order is the order of the numbers: the
/// bits of \f$ x \f$ with the sign bit flipped for a signed integer, and for a
/// floating point number all of them flipped if it is negative and the sign
/// bit otherwise
template< RadixKey T >
[[nodiscard]] auto radixKey( T x );

/// the LSD radix sort of numbers
///
/// In pseudocode (the LSD radix sort of Cormen, Leiserson, Rivest and Stein,
/// section 8.3, on bytes, beyond the book), \f$ b_j( x ) \f$ the byte
/// \f$ j \f$ of the key of \f$ x \f$ from the least significant and \f$ B \f$
/// a buffer of \f$ n \f$ places:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{radixSort}( A , n ) \cr
/// 2 & \quad D \gets \text{the bytes } j \text{ in which some two keys differ}
///     \cr
/// 3 & \quad \textbf{if}\ D = \emptyset\ \textbf{then return} \cr
/// 4 & \quad \textbf{if}\ |D| = 1\ \textbf{then}\ \text{count the keys and }
///     \text{write them back in order};\ \textbf{return} \cr
/// 5 & \quad \text{count the bytes } b_j( x ) \text{ of all the items, for }
///     \text{all } j \in D \text{, in one sweep} \cr
/// 6 & \quad \textbf{foreach}\ j \in D \text{ in increasing order}
///     \ \textbf{do} \cr
/// 7 & \qquad \text{scatter } A \text{ into } B \text{ by } b_j \text{, }
///     \text{stably, from the prefix sums of the counts} \cr
/// 8 & \qquad A \leftrightarrow B \cr
/// 9 & \quad \textbf{if}\ \text{the items ended in the buffer}\ \textbf{then}
///     \ \text{copy them to } A
/// \end{array}}
/// \f]
/// where a vector larger than the cache, whose keys differ in at least 3
/// bytes, is first scattered by its top byte of \f$ D \f$, and each part is
/// sorted in the same way.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< RadixKey T >
void radixSort( std::vector< T > & A );

/// the LSD radix sort of any items by the unsigned `key( x )`, stable
///
/// It is the pseudocode of `radixSort()` with \f$ b_j( x ) \f$ the byte
/// \f$ j \f$ of `key( x )`, and without the step 4, since equal keys need not
/// be equal items.
/// \post \f$ A \f$ is sorted by the key, with the same items (audit)
template< class T , class K >
void radixSortBy( std::vector< T > & A , K key );

/// the groups that americanFlagSort() leaves to a sort by comparisons are
/// those of fewer items than this: many with the vector registers, whose
/// quicksort beats the radix passes that it replaces, few otherwise
inline constexpr std::size_t kFlagSmall = kVectorSort ? 8192 : 256;

/// the MSD radix sort of numbers in place
///
/// In pseudocode (McIlroy, Bostic and McIlroy, and Skarupke, beyond the book),
/// \f$ b_d( x ) \f$ the byte \f$ d \f$ of the key of \f$ x \f$, \f$ D \f$ the
/// bytes in which some keys differ, the most significant first, and
/// \f$ \mathrm{small} \f$ the sort of `vectorQuickSort()` or `pdqSort()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{flag}( A , \mathit{lo} , \mathit{hi} , D )
///     \cr
/// 2 & \quad d \gets \text{the first of } D;\ c[ b ] \gets \text{the items }
///     \text{of } A[ \mathit{lo} .. \mathit{hi} - 1 ] \text{ with } b_d = b
///     \cr
/// 3 & \quad \textbf{if}\ \text{one } c[ b ] \text{ is all of them}
///     \ \textbf{then}\ \mathrm{flag}( A , \mathit{lo} , \mathit{hi} , D
///     \setminus d );\ \textbf{return} \cr
/// 4 & \quad \mathit{next}[ b ] \gets \mathit{lo} + \textstyle\sum_{b' < b}
///     c[ b' ];\ \mathit{end}[ b ] \gets \mathit{next}[ b ] + c[ b ] \cr
/// 5 & \quad \textbf{while}\ \text{two buckets have } \mathit{next}[ b ] <
///     \mathit{end}[ b ]\ \textbf{do} \cr
/// 6 & \qquad \textbf{foreach}\ b \text{ with } \mathit{next}[ b ] <
///     \mathit{end}[ b ]\ \textbf{do} \cr
/// 7 & \qquad\quad \textbf{for}\ i \gets \mathit{next}[ b ]\ \textbf{to}
///     \ \mathit{end}[ b ] - 1\ \textbf{do} \cr
/// 8 & \qquad\qquad e \gets b_d( A[ i ] );\ A[ i ] \leftrightarrow A[
///     \mathit{next}[ e ] ];\ \mathit{next}[ e ] \gets \mathit{next}[ e ] + 1
///     \cr
/// 9 & \quad \textbf{if}\ D \ne \{ d \}\ \textbf{then foreach}\ b
///     \text{ with } c[ b ] > 1\ \textbf{do} \cr
/// 10 & \qquad \textbf{if}\ c[ b ] \ge \mathtt{kFlagSmall}\ \textbf{then}
///     \ \mathrm{flag}( \text{the bucket } b , D \setminus d )
///     \ \textbf{else}\ \mathrm{small}( \text{the bucket } b ) \cr
/// 11 & \textbf{procedure}\ \mathrm{americanFlagSort}( A , n ) \cr
/// 12 & \quad \textbf{if}\ A \text{ is sorted}\ \textbf{then return} \cr
/// 13 & \quad \textbf{if}\ A \text{ is non-increasing}\ \textbf{then}
///     \ \text{reverse } A;\ \textbf{return} \cr
/// 14 & \quad \textbf{if}\ | D | = 1\ \textbf{then}\ \text{count the keys}
///     \text{ and write them back};\ \textbf{return} \cr
/// 15 & \quad \textbf{if}\ n < \mathtt{kFlagSmall}\ \textbf{then}
///     \ \mathrm{small}( A )\ \textbf{else}\ \mathrm{flag}( A , 0 , n , D )
/// \end{array}}
/// \f]
/// where the loop of the step 7 runs over the part of the bucket that was
/// not in place when the round started, and the order of the keys is the one
/// of `radixKey()`: \f$ -0 \f$ before \f$ +0 \f$, the NaN at the ends.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< RadixKey T >
void americanFlagSort( std::vector< T > & A );

/// the MSD radix sort of strings
///
/// In pseudocode (McIlroy, Bostic and McIlroy, beyond the book),
/// \f$ c_d( s ) \f$ the character \f$ d \f$ of \f$ s \f$ plus 1, 0 past its
/// end, and the strings of a group sharing their first \f$ d \f$ characters:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{msd}( S , d ) \cr
/// 2 & \quad \textbf{if}\ |S| \le 16\ \textbf{then}\ \text{sort } S \text{ }
///     \text{by insertion on the suffixes from } d;\ \textbf{return} \cr
/// 3 & \quad d \gets d + \text{the length of the prefix that the strings of }
///     S \text{ share from } d \cr
/// 4 & \quad \textbf{if}\ \text{the strings of } S \text{ are all equal}
///     \ \textbf{then return} \cr
/// 5 & \quad \text{distribute } S \text{ into the buckets } S_0 , \ldots ,
///     S_{256} \text{ by } c_d \cr
/// 6 & \quad \textbf{for}\ r \gets 1\ \textbf{to}\ 256\ \textbf{do}
///     \ \textbf{if}\ |S_r| \ge 2\ \textbf{then}\ \mathrm{msd}( S_r , d + 1 )
///     \cr
/// 7 & \textbf{procedure}\ \mathrm{msdRadixSort}( A ) \cr
/// 8 & \quad \textbf{if}\ n > 1\ \textbf{then}\ \mathrm{msd}( A , 0 )
/// \end{array}}
/// \f]
/// where the bucket \f$ S_0 \f$ holds the strings that end at \f$ d \f$, all
/// equal, the first distribution permutes the strings in place as
/// `americanFlagSort()` does, and the later ones move them between the vector
/// and a buffer.
/// \post \f$ A \f$ is sorted, with the same items (audit)
inline void msdRadixSort( std::vector< std::string > & A );

/// the bucket sort of numbers in \f$ [ 0 , 1 ) \f$
///
/// In pseudocode (Cormen, Leiserson, Rivest and Stein, section 8.4, beyond the
/// book, with the buckets counted instead of listed), \f$ B \f$ a buffer of
/// \f$ n \f$ items and \f$ \beta( x ) = \min\{ \lfloor n x \rfloor , n - 1
/// \} \f$ the bucket of \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bucketSort}( A , n ) \cr
/// 2 & \quad c[ j ] \gets \text{the number of the items } x \text{ with }
///     \beta( x ) < j ,\ \text{for } j = 0 , \ldots , n - 1 \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{do}\ B[ c[ \beta( x ) ] ]
///     \gets x;\ c[ \beta( x ) ] \gets c[ \beta( x ) ] + 1 \cr
/// 4 & \quad \textbf{foreach}\ \text{bucket of more than 32 items}
///     \ \textbf{do}\ \mathrm{pdqSort}( \text{its part of } B ) \cr
/// 5 & \quad A \gets B;\ \mathrm{insertionSort}( A )
/// \end{array}}
/// \f]
/// where the insertion sort moves no item out of its bucket, since
/// \f$ \beta \f$ does not decrease; a vector larger than the cache runs the
/// steps 2 and 3 first on the groups of 1024 consecutive buckets, and then on
/// the buckets of each group, which fit in the cache.
/// \pre each number is in \f$ [ 0 , 1 ) \f$
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< std::floating_point T >
void bucketSort( std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/RadixSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RadixSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RadixSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
