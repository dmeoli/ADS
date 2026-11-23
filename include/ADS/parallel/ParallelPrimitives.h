/*--------------------------------------------------------------------------*/
/*------------------------ File ParallelPrimitives.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel primitives on sequences, beyond the book: the operations of
 * the toolkit ParlayLib of Blelloch, Anderson and Dhulipala that the
 * patterns of `Executor.h` (the loops, the reductions and the scans by an
 * associative operation, i.e., by any monoid) and the sorts of
 * `ParallelSort.h` do not already give, each a function of an executor
 * [see `Executor.h`] with the same result on every executor:
 *
 * - `tabulate( ex , n , f )`, the vector of \f$ f( 0 ) , \ldots ,
 *   f( n - 1 ) \f$;
 * - `pack( ex , A , flags )`, `packIndex( ex , n , pred )` and
 *   `filter( ex , A , pred )`, the items (or the indices) that a flag or a
 *   predicate keeps, in their order: the kept items of each block counted,
 *   the counts summed in the order of the blocks, and each block copying its
 *   kept items from its offset, so that a predicate of the index
 *   (`packIndex()`) is never stored in a sequence (the block-delayed
 *   evaluation of ParlayLib);
 * - `flatten( ex , L )`, the concatenation of a vector of vectors, the
 *   offsets by a scan of the sizes;
 * - `histogram( ex , n , key , m )`, the counts of the keys in
 *   \f$ [ 0 , m ) \f$, by the counts of each block summed bucket by bucket
 *   when \f$ m \f$ is small w.r.t. \f$ n \f$, by atomic increments
 *   otherwise;
 * - `countingSort( ex , A , key , m )`, the stable distribution of the items
 *   into the buckets of their keys in \f$ [ 0 , m ) \f$, and
 *   `integerSort( ex , A , key , b )`, the stable sort by keys of \f$ b \f$
 *   bits, a radix sort from the most significant digit of 8 bits: each level
 *   is a counting sort into a buffer, the buckets are sorted recursively
 *   between the buffer and the vector, the large ones on all the threads and
 *   the small ones by `radixSortBy()` [see `RadixSort.h`] on one thread
 *   each;
 * - `semisort( ex , A , key )`, which makes the items of equal keys
 *   adjacent in some order of the keys (the semisort of Gu, Shun, Sun and
 *   Blelloch, here by the sort of the hashes of the keys, cf. below), and on
 *   it `groupByKey()`, `histogramByKey()` and `removeDuplicates()`;
 * - `randomShuffle( ex , A , seed )` and `randomPermutation( ex , n , seed )`,
 *   a random permutation: each item goes to a random bucket of about 4096
 *   items, by a counting sort, and each bucket is shuffled by the algorithm of
 *   Fisher and Yates, which gives every permutation with the same probability
 *   when the random numbers are uniform and independent;
 * - `tokens( ex , s , isSpace )` and `split( ex , s , isSep )`, the maximal
 *   runs of symbols that are not spaces, and the fields between the
 *   separators, as views of \f$ s \f$;
 * - `reduceOf< G >( ex , A )` and `scanOf< G >( ex , A )`, the reduction and
 *   the exclusive scan of a vector by a monoid \f$ G \f$ of `Algebra.h`.
 *
 * With \f$ B \f$ the blocks of a loop [see `Executor.h`] and \f$ p \f$ the
 * threads, the work \f$ W \f$ and the depth \f$ D \f$ are those of the
 * patterns they are made of: \f$ W = O( n ) \f$ and
 * \f$ D = O( n / B + B ) \f$ for `tabulate()`, the packs, `tokens()` and
 * `split()`, \f$ W = O( n + m ) \f$ and \f$ D = O( n / B + m ) \f$ for
 * `histogram()` (by blocks; \f$ D = O( n / B ) \f$ by atomics, against the
 * \f$ O( \log n ) \f$ of the PRAM bounds of ParlayLib, which assume
 * \f$ B = n / \log n \f$ blocks), \f$ W = O( n + m B ) \f$ and
 * \f$ D = O( n / B + m B / p ) \f$ for `countingSort()`,
 * \f$ W = O( n \lceil b / 8 \rceil ) \f$ for `integerSort()`, whose depth is
 * that of \f$ \lceil b / 8 \rceil \f$ counting sorts plus
 * \f$ O( \max\{ 2^{14} , n / p \} ) \f$ for the buckets sorted on one
 * thread, and \f$ W = O( n ) \f$ expected for `semisort()` and
 * `randomShuffle()`, with the depth of `integerSort()` on the hashes and
 * \f$ O( n / B + 4096 ) \f$ respectively. A vector of \f$ n \f$
 * items takes a buffer of \f$ n \f$ items in the sorts, the semisort and the
 * shuffle, and \f$ n \f$ bytes of flags in `filter()` and `packIndex()`.
 *
 * The semisort sorts the pairs \f$ ( h( k ) , i ) \f$ of the hash of the key
 * of each item and of its place by `integerSort()` on the top
 * \f$ t = \min\{ 64 , \lceil \log_2 ( n + 1 ) \rceil + 8 \} \f$ bits of the
 * hash, so that equal keys fall into one run of equal top bits; two distinct
 * keys share their top bits with probability \f$ 2^{-t} \f$, and the runs
 * that hold distinct keys, about \f$ n / 512 \f$ on average and found by a
 * pack of the places where a key differs from the one before in the same
 * run, are regrouped by a sequential scan; the items are moved once, into
 * the order of the pairs. The algorithm of Gu et al. samples the heavy keys
 * and sends each to a bucket of its own; here a heavy key makes one run of
 * equal hashes, which the radix sort moves once per level and never
 * compares, so that the samples are not needed.
 *
 * The contracts [see `Contract.h`]: the keys of `histogram()` and
 * `countingSort()` are less than \f$ m \f$ (a precondition, checked on each
 * key), those of `integerSort()` less than \f$ 2^b \f$, with
 * \f$ 0 \le b \le 64 \f$; the items are default constructible and movable;
 * `key` and the predicates may run on any thread, at once, and equal keys
 * of the semisort have equal hashes. The functions are defined in
 * `ParallelPrimitives.tpp`.
 *
 * \see G. E. Blelloch, D. Anderson, L. Dhulipala, ParlayLib - a toolkit for
 *      parallel algorithms on shared-memory multicore machines, Proceedings
 *      of the 32nd ACM Symposium on Parallelism in Algorithms and
 *      Architectures (SPAA), 507-509, 2020, doi:10.1145/3350755.3400254
 * \see J. Shun, G. E. Blelloch, J. T. Fineman, P. B. Gibbons, A. Kyrola,
 *      H. V. Simhadri, K. Tangwongsan, Brief announcement: the Problem Based
 *      Benchmark Suite, Proceedings of the 24th ACM Symposium on Parallelism
 *      in Algorithms and Architectures (SPAA), 68-70, 2012,
 *      doi:10.1145/2312005.2312018
 * \see Y. Gu, J. Shun, Y. Sun, G. E. Blelloch, A top-down parallel
 *      semisort, Proceedings of the 27th ACM Symposium on Parallelism in
 *      Algorithms and Architectures (SPAA), 24-34, 2015,
 *      doi:10.1145/2755573.2755597
 * \see J. Shun, Y. Gu, G. E. Blelloch, J. T. Fineman, P. B. Gibbons,
 *      Sequential random permutation, list contraction and tree contraction
 *      are highly parallel, Proceedings of the 26th ACM-SIAM Symposium on
 *      Discrete Algorithms (SODA), 431-448, 2015,
 *      doi:10.1137/1.9781611973730.30
 * \see G. E. Blelloch, Prefix sums and their applications, Technical
 *      Report CMU-CS-90-190, Carnegie Mellon University, 1990
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

#ifndef __ADS_ParallelPrimitives
 #define __ADS_ParallelPrimitives
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Algebra.h"
#include "ADS/Contract.h"
#include "ADS/Wide.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/parallel/Executor.h"
#include "ADS/sort/RadixSort.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the vector of \f$ f( 0 ) , \ldots , f( n - 1 ) \f$, computed in parallel
template< Executor E , class F >
[[nodiscard]] auto tabulate( E & ex , std::size_t n , F && f )
  -> std::vector< std::decay_t< std::invoke_result_t< F & , std::size_t > > >;

/// the items \f$ A[ i ] \f$ whose flag \f$ \mathit{flags}[ i ] \f$ is set,
/// in their order
///
/// In pseudocode (the pack by blocks, beyond the book), \f$ \mathit{keep}(
/// i ) \f$ the flag of \f$ i \f$ and \f$ \mathit{lo}_b , \mathit{hi}_b \f$
/// the ends of the block \f$ b \f$ of `blocksOf()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pack}( \mathit{ex} , A , \mathit{keep} )
///     \cr
/// 2 & \quad ( B , s ) \gets \mathrm{blocksOf}( 0 , n , 2048 ) \cr
/// 3 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ B - 1\ \textbf{do in }
///     \textbf{parallel}\ c[ b ] \gets | \{ i \in [ \mathit{lo}_b ,
///     \mathit{hi}_b ) : \mathit{keep}( i ) \} | \cr
/// 4 & \quad o[ 0 ] \gets 0;\ \textbf{for}\ b \gets 1\ \textbf{to}\ B
///     \ \textbf{do}\ o[ b ] \gets o[ b - 1 ] + c[ b - 1 ] \cr
/// 5 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ B - 1\ \textbf{do in }
///     \textbf{parallel} \cr
/// 6 & \qquad j \gets o[ b ] \cr
/// 7 & \qquad \textbf{for}\ i \gets \mathit{lo}_b\ \textbf{to}\ \mathit{hi}_b
///     - 1\ \textbf{do if}\ \mathit{keep}( i )\ \textbf{then}\ R[ j ] \gets
///     A[ i ];\ j \gets j + 1 \cr
/// 8 & \quad \textbf{return}\ R[ 0 .. o[ B ] - 1 ]
/// \end{array}}
/// \f]
/// \pre \f$ |\mathit{flags}| = |A| \f$
template< Executor E , class T , class F >
[[nodiscard]] std::vector< T > pack( E & ex , const std::vector< T > & A ,
				     const std::vector< F > & flags );

/// the indices \f$ i \in [ 0 , n ) \f$ with \f$ \mathit{pred}( i ) \f$, in
/// increasing order: the pack above of the indices, the predicate evaluated
/// once per index into \f$ n \f$ bytes of flags
template< Executor E , class P >
[[nodiscard]] std::vector< std::size_t > packIndex( E & ex , std::size_t n ,
						    P && pred );

/// the items \f$ x \f$ of \f$ A \f$ with \f$ \mathit{pred}( x ) \f$, in
/// their order: the pack above, the predicate evaluated once per item
template< Executor E , class T , class P >
[[nodiscard]] std::vector< T > filter( E & ex , const std::vector< T > & A ,
				       P && pred );

/// the concatenation of the vectors of \f$ L \f$ after the items of
/// \f$ \mathit{head} \f$, in their order
///
/// In pseudocode (beyond the book), \f$ m = |L| \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{flatten}( \mathit{ex} , L ,
///     \mathit{head} ) \cr
/// 2 & \quad o \gets \text{the exclusive scan of } |L[ 0 ]| , \ldots ,
///     |L[ m - 1 ]| \text{ from } |\mathit{head}| \cr
/// 3 & \quad R \gets \mathit{head} \text{ followed by }
///     o[ m ] - |\mathit{head}| \text{ places} \cr
/// 4 & \quad \textbf{if}\ o[ m ] - |\mathit{head}| < 2^{16}
///     \ \textbf{then}\ \text{copy each } L[ k ] \text{ to } R[ o[ k ] ..
///     ]\text{, one after the other} \cr
/// 5 & \quad \textbf{else}\ \text{the same copies in parallel, those of }
///     \text{more than } 2^{16} \text{ items by blocks} \cr
/// 6 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
template< Executor E , class T >
[[nodiscard]] std::vector< T >
flatten( E & ex , const std::vector< std::vector< T > > & L ,
	 std::vector< T > head = {} );

/// the counts \f$ H[ k ] = | \{ i < n : \mathit{key}( i ) = k \} | \f$ of
/// the keys in \f$ [ 0 , m ) \f$
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{histogram}( \mathit{ex} , n ,
///     \mathit{key} , m ) \cr
/// 2 & \quad \textbf{if}\ 4 m > n\ \textbf{then} \cr
/// 3 & \qquad H \gets 0;\ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1
///     \ \textbf{do in parallel}\ \mathrm{fetchAndAdd}( H[ \mathit{key}( i )
///     ] , 1 ) \cr
/// 4 & \quad \textbf{else} \cr
/// 5 & \qquad ( B , s ) \gets \mathrm{blocksOf}( 0 , n , \max\{ 1024 , m \}
///     )\quad \triangleright\ B m \le n + m \cr
/// 6 & \qquad \textbf{for}\ b \gets 0\ \textbf{to}\ B - 1\ \textbf{do in }
///     \textbf{parallel}\ C[ b ][ k ] \gets | \{ i \in [ \mathit{lo}_b ,
///     \mathit{hi}_b ) : \mathit{key}( i ) = k \} | \cr
/// 7 & \qquad \textbf{for}\ k \gets 0\ \textbf{to}\ m - 1\ \textbf{do in }
///     \textbf{parallel}\ H[ k ] \gets \sum_b C[ b ][ k ] \cr
/// 8 & \quad \textbf{return}\ H
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{key}( i ) < m \f$ for every \f$ i < n \f$
template< Executor E , class K >
[[nodiscard]] std::vector< std::size_t > histogram( E & ex , std::size_t n ,
						    K && key , std::size_t m );

/// the counts of the unsigned integers of \f$ A \f$, less than \f$ m \f$:
/// `histogram()` above with \f$ \mathit{key}( i ) = A[ i ] \f$
template< Executor E , std::unsigned_integral U >
[[nodiscard]] std::vector< std::size_t >
histogram( E & ex , const std::vector< U > & A , std::size_t m );

/// the items of \f$ A \f$ distributed stably into the buckets of their keys
/// in \f$ [ 0 , m ) \f$, and the \f$ m + 1 \f$ starts of the buckets
///
/// In pseudocode (the counting sort by blocks, beyond the book), \f$ X \f$
/// the buffer of \f$ n \f$ items:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countingSort}( \mathit{ex} , A ,
///     \mathit{key} , m ) \cr
/// 2 & \quad ( B , s ) \gets \mathrm{blocksOf}( 0 , n , \max\{ 1024 , 2 m \}
///     ) \cr
/// 3 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ B - 1\ \textbf{do in }
///     \textbf{parallel}\ C[ k B + b ] \gets | \{ i \in [ \mathit{lo}_b ,
///     \mathit{hi}_b ) : \mathit{key}( A[ i ] ) = k \} | \cr
/// 4 & \quad O \gets \text{the exclusive scan of } C\quad \triangleright
///     \ \text{in the order (bucket, block)} \cr
/// 5 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ B - 1\ \textbf{do in }
///     \textbf{parallel} \cr
/// 6 & \qquad \textbf{for}\ i \gets \mathit{lo}_b\ \textbf{to}\ \mathit{hi}_b
///     - 1\ \textbf{do}\ k \gets \mathit{key}( A[ i ] );\ X[ O[ k B + b ] ]
///     \gets A[ i ];\ O[ k B + b ] \gets O[ k B + b ] + 1 \cr
/// 7 & \quad A \gets X \cr
/// 8 & \quad \textbf{return}\ \text{the starts } O[ k B ] \text{ before }
///     \text{line 5, for } k = 0 , \ldots , m - 1 \text{, and } n
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{key}( x ) < m \f$ for every item \f$ x \f$
/// \post \f$ A \f$ holds the same items, in the order of their keys and,
///       among equal keys, in their former order
template< Executor E , class T , class K >
std::vector< std::size_t > countingSort( E & ex , std::vector< T > & A ,
					 K && key , std::size_t m );

/// the stable sort of \f$ A \f$ by the unsigned keys \f$ \mathit{key}( x )
/// < 2^b \f$
///
/// In pseudocode (the parallel radix sort from the most significant digit,
/// beyond the book), \f$ X \f$ the vector being sorted, \f$ Y \f$ a buffer
/// of as many items, and \f$ \mathit{here} \f$ whether the result is to end
/// in \f$ X \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{integerSort}( \mathit{ex} , X , Y , b ,
///     \mathit{here} ) \cr
/// 2 & \quad \textbf{if}\ b = 0\ \textbf{or}\ |X| < 2^{14}\ \textbf{or}
///     \ p = 1\ \textbf{then} \cr
/// 3 & \qquad \text{sort } X \text{ by the low } b \text{ bits of the keys}
///     \text{ (by } \mathrm{radixSortBy} \text{)};\ \textbf{if}\ \textbf{not}
///     \ \mathit{here}\ \textbf{then}\ Y \gets X \cr
/// 4 & \qquad \textbf{return} \cr
/// 5 & \quad r \gets \min\{ b , 8 \} \cr
/// 6 & \quad \text{distribute } X \text{ into } Y \text{ stably by the bits }
///     b - r .. b - 1 \text{ of the keys}\quad \triangleright\ \text{lines 2}
///     \text{ to 6 of } \mathrm{countingSort} \cr
/// 7 & \quad \textbf{for each}\ \text{bucket } [ l , h ) \text{ of fewer}
///     \text{ than } \max\{ 2^{14} , n / p \} \text{ items}\ \textbf{do in }
///     \textbf{parallel} \cr
/// 8 & \qquad \mathrm{integerSort}( \mathit{seq} , Y[ l .. h ) , X[ l .. h ) ,
///     b - r , \textbf{not}\ \mathit{here} ) \cr
/// 9 & \quad \textbf{for each}\ \text{other bucket}\ [ l , h )\ \textbf{do}
///     \ \mathrm{integerSort}( \mathit{ex} , Y[ l .. h ) , X[ l .. h ) , b -
///     r , \textbf{not}\ \mathit{here} )
/// \end{array}}
/// \f]
/// called as \f$ \mathrm{integerSort}( \mathit{ex} , A , Y , b ,
/// \textbf{true} ) \f$.
/// \pre \f$ 0 \le b \le 64 \f$ and \f$ \mathit{key}( x ) < 2^b \f$ for every
///      item \f$ x \f$
/// \post \f$ A \f$ is sorted by the keys, equal keys in their former order
///       (an audit)
template< Executor E , class T , class K >
void integerSort( E & ex , std::span< T > A , K && key , int b );

/// `integerSort()` above on a vector
template< Executor E , class T , class K >
void integerSort( E & ex , std::vector< T > & A , K && key , int b );

/// the items of \f$ A \f$ reordered so that those of equal keys are
/// adjacent, the keys hashed by \f$ h \f$
///
/// In pseudocode (beyond the book), \f$ \mathit{top}( j ) \f$ the top
/// \f$ t \f$ bits of \f$ P[ j ].h \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{semisort}( \mathit{ex} , A ,
///     \mathit{key} , h ) \cr
/// 2 & \quad t \gets \min\{ 64 , \lceil \log_2 ( n + 1 ) \rceil + 8 \} \cr
/// 3 & \quad P \gets \mathrm{tabulate}( n , i \mapsto ( h( \mathit{key}( A[
///     i ] ) ) , i ) ) \cr
/// 4 & \quad \mathrm{integerSort}( \mathit{ex} , P , \mathit{top} , t ) \cr
/// 5 & \quad D \gets \mathrm{packIndex}( n , j \mapsto j > 0\ \textbf{and}
///     \ \mathit{top}( j ) = \mathit{top}( j - 1 )\ \textbf{and}
///     \ \mathit{key}( A[ P[ j ].i ] ) \ne \mathit{key}( A[ P[ j - 1 ].i ] )
///     ) \cr
/// 6 & \quad \textbf{for each}\ \text{run of equal } \mathit{top}
///     \text{ that holds a place of } D\ \textbf{do in parallel} \cr
/// 7 & \qquad \text{regroup the run by the keys, stably, on one thread} \cr
/// 8 & \quad A \gets \mathrm{tabulate}( n , j \mapsto A[ P[ j ].i ] )
/// \end{array}}
/// \f]
/// \pre equal keys have equal hashes
/// \post \f$ A \f$ holds the same items, those of equal keys adjacent (an
///       audit)
template< Executor E , class T , class K ,
	  class H = DefaultHash<
	    std::decay_t< std::invoke_result_t< K & , const T & > > > >
void semisort( E & ex , std::vector< T > & A , K && key , H hash = H() );

/// the pairs \f$ ( k , V_k ) \f$ of the distinct keys \f$ k \f$ of the pairs
/// of \f$ A \f$ and of the vectors \f$ V_k \f$ of their values, in the order
/// of \f$ A \f$, by `semisort()` and a pack of the starts of the groups; the
/// keys in the order of the semisort
template< Executor E , class K , class V , class H = DefaultHash< K > >
[[nodiscard]] std::vector< std::pair< K , std::vector< V > > >
groupByKey( E & ex , const std::vector< std::pair< K , V > > & A ,
	    H hash = H() );

/// the pairs \f$ ( k , c_k ) \f$ of the distinct keys of \f$ A \f$ and of
/// their counts, the keys in the order of the semisort
template< Executor E , class K , class H = DefaultHash< K > >
[[nodiscard]] std::vector< std::pair< K , std::size_t > >
histogramByKey( E & ex , const std::vector< K > & A , H hash = H() );

/// the distinct keys of \f$ A \f$, each once, in the order of the semisort
template< Executor E , class K , class H = DefaultHash< K > >
[[nodiscard]] std::vector< K >
removeDuplicates( E & ex , const std::vector< K > & A , H hash = H() );

/// a random permutation of the items of \f$ A \f$, the same on every
/// executor for the same seed
///
/// In pseudocode (beyond the book), \f$ \rho( x , s ) \f$ the random word
/// of 64 bits that `hashWord()` [see `FastHash.h`] gives for \f$ x \f$ and
/// the seed \f$ s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{randomShuffle}( \mathit{ex} , A , s ) \cr
/// 2 & \quad m \gets \max\{ 1 , \lfloor n / 4096 \rfloor \} \cr
/// 3 & \quad \text{distribute } A \text{ stably into } m \text{ buckets, }
///     A[ i ] \text{ into } \lfloor m \rho( i , s ) / 2^{64} \rfloor
///     \quad \triangleright\ \text{as } \mathrm{countingSort} \cr
/// 4 & \quad \textbf{for}\ c \gets 0\ \textbf{to}\ m - 1\ \textbf{do in }
///     \textbf{parallel} \cr
/// 5 & \qquad \text{shuffle the bucket } c \text{ by Fisher and Yates, on }
///     \text{the random numbers of the seed } \rho( c , s )
/// \end{array}}
/// \f]
template< Executor E , class T >
void randomShuffle( E & ex , std::vector< T > & A , std::uint64_t seed );

/// a random permutation of \f$ 0 , \ldots , n - 1 \f$, by `randomShuffle()`
template< Executor E >
[[nodiscard]] std::vector< std::size_t >
randomPermutation( E & ex , std::size_t n , std::uint64_t seed );

/// the maximal runs of the symbols of \f$ s \f$ for which `isSpace` is
/// false, in their order
///
/// In pseudocode (beyond the book), \f$ \sigma( i ) \f$ true when
/// \f$ i < 0 \f$, \f$ i \ge n \f$ or \f$ s[ i ] \f$ is a space:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tokens}( \mathit{ex} , s ) \cr
/// 2 & \quad L \gets \mathrm{packIndex}( n , i \mapsto \sigma( i - 1 )
///     \ \textbf{and not}\ \sigma( i ) ) \cr
/// 3 & \quad R \gets \mathrm{packIndex}( n , i \mapsto \sigma( i + 1 )
///     \ \textbf{and not}\ \sigma( i ) ) \cr
/// 4 & \quad \textbf{return}\ \mathrm{tabulate}( |L| , j \mapsto s[ L[ j ]
///     .. R[ j ] ] )
/// \end{array}}
/// \f]
template< Executor E , class P >
[[nodiscard]] std::vector< std::string_view >
tokens( E & ex , std::string_view s , P && isSpace );

/// the fields of \f$ s \f$ between the separators for which `isSep` is
/// true, empty ones included: one more than the separators
template< Executor E , class P >
[[nodiscard]] std::vector< std::string_view >
split( E & ex , std::string_view s , P && isSep );

/// the reduction \f$ \mathit{id} \oplus A[ 0 ] \oplus \cdots \oplus
/// A[ n - 1 ] \f$ by the monoid \f$ G \f$ [see `Algebra.h`], by
/// `parallelReduce()`
template< Monoid G , Executor E >
[[nodiscard]] typename G::Value
reduceOf( E & ex , const std::vector< typename G::Value > & A );

/// the exclusive prefixes of \f$ A \f$ by the monoid \f$ G \f$ and the
/// reduction of all of \f$ A \f$, by `parallelExclusiveScan()`
template< Monoid G , Executor E >
[[nodiscard]] std::pair< std::vector< typename G::Value > , typename G::Value >
scanOf( E & ex , const std::vector< typename G::Value > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelPrimitives.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ParallelPrimitives.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ParallelPrimitives.h ---------------------*/
/*--------------------------------------------------------------------------*/
