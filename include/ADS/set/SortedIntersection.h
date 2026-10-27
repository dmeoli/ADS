/*--------------------------------------------------------------------------*/
/*----------------------- File SortedIntersection.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The intersection of two sorted sets, the core of the resolution of a
 * query of several words by an inverted index (Ferragina, chapter 6):
 * \f$ A \f$ of \f$ n \f$ items and \f$ B \f$ of \f$ m \f$ items, strictly
 * increasing, \f$ m \leq n \f$ (the roles are swapped otherwise).
 *
 * - `intersectMerge()` is the scan of the two lists, at most
 *   \f$ n + m \f$ comparisons, \f$ O( ( n + m ) / B ) \f$ I/Os whatever
 *   \f$ B \f$, optimal when \f$ m \f$ and \f$ n \f$ are close; it is the
 *   procedure of section 8.3 of the book that `setIntersection()` of
 *   `SortedListSet.h` applies to its lists.
 * - `intersectBinarySearch()` searches each item of \f$ B \f$ in \f$ A \f$,
 *   \f$ O( m \log n ) \f$ comparisons, better when
 *   \f$ m = o( n / \log n ) \f$.
 * - `intersectMutualPartition()` is the algorithm of Baeza-Yates: the
 *   median of the shorter list searched in the longer one splits
 *   both, and the two halves recur, the roles swapped when the part of the
 *   longer one is the shorter, \f$ O( m ( 1 + \log( n / m ) ) ) \f$
 *   comparisons, which is optimal, since \f$ \log_2 \binom{n}{m} \f$
 *   comparisons are needed to tell the subsets of \f$ A \f$ of \f$ m \f$
 *   items apart.
 * - `intersectDoubling()` searches each item of \f$ B \f$ from the place of
 *   the previous one by galloping, the places \f$ 1 , 2 , 4 , \ldots \f$
 *   ahead and then a binary search within the last gap (the
 *   exponential search of Bentley and Yao; `gallopSearch()` of
 *   `BinarySearch.h` is the same search from the start of a vector, and
 *   the merges of `RunSorts.h` gallop in the same way),
 *   \f$ \sum_j ( 1 + \log \Delta_j ) = O( m ( 1 + \log( n / m ) ) ) \f$
 *   comparisons with no recursion.
 * - `intersectTwoLevel()` is the two-level storage: the first
 *   item of each block of \f$ L \f$ items of \f$ A \f$ in an array
 *   \f$ A' \f$ of \f$ n / L \f$ items, merged with \f$ B \f$ to find the
 *   block where each item of \f$ B \f$ may be, and each block met merged
 *   with its items of \f$ B \f$, \f$ O( n / L + m L ) \f$ time; on the disk
 *   [see `TwoLevelMemory.h`] `intersectTwoLevelExternal()` takes
 *   \f$ O( n / L B + m L / B + m ) \f$ I/Os, each block of \f$ A \f$ met
 *   costing at most \f$ \lceil L / B \rceil + 1 \f$ reads, against the
 *   \f$ \lceil n / B \rceil + \lceil m / B \rceil \f$ of
 *   `intersectMergeExternal()`.
 * - `PermutedBuckets` is the random permutation and splitting of the
 *   lists (the lookup of Sanders and Transier): the values of the universe
 *   \f$ [ 0 , 2^w ) \f$ permuted by \f$ \pi( x ) = ( a x + c ) \bmod 2^w \f$,
 *   \f$ a \f$ odd so that \f$ \pi \f$ is invertible, and a list of \f$ n \f$
 *   items split by the top \f$ \ell = \lceil \log_2( n / L ) \rceil \f$ bits
 *   of the images into buckets of \f$ L \f$ items on average, the images
 *   sorted and addressed by a table of offsets; each image of the shorter
 *   list is looked for in the bucket of the longer one named by its own top
 *   bits, so that a bucket of \f$ A \f$ is never looked at unless \f$ B \f$
 *   has items in it: \f$ O( m + \min\{ n , m L \} ) \f$ expected time, and
 *   the lists need not be sorted beforehand, only their images are.
 *
 * Compared with the literature: the experiments of Sanders and Transier,
 * and of Barbay et al., find the lookup best up to lists of similar length,
 * the two-level skipping better than the mutual partition for all but the
 * most unequal lists, whose recursion costs more than its bound says; the
 * adaptive algorithms of Demaine, López-Ortiz and Munro and of Barbay and
 * Kenyon extend the galloping to \f$ k \f$ lists, and Ding and König
 * replace the buckets by word-sized bitmaps.
 *
 * The contracts [see `Contract.h`]: \f$ A \f$ and \f$ B \f$ strictly
 * increasing, audits; \f$ L \ge 1 \f$; the result is the intersection,
 * strictly increasing, a postcondition checked as an audit against
 * `intersectMerge()`; \f$ 1 \le w \le 63 \f$, \f$ a \f$ odd, the values below
 * \f$ 2^w \f$, and two `PermutedBuckets` intersected with the same
 * permutation; the files of the external versions share a memory of at
 * least \f$ 4 B \f$. The functions are defined in `SortedIntersection.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 6, Set Intersection,
 *      doi:10.1017/9781009128933
 * \see R. Baeza-Yates, A fast set intersection algorithm for sorted
 *      sequences, Proceedings of the 15th Symposium on Combinatorial
 *      Pattern Matching (CPM), Lecture Notes in Computer Science 3109,
 *      400-408, 2004, doi:10.1007/978-3-540-27801-6_30
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
 * \see P. Sanders, F. Transier, Intersection in integer inverted indices,
 *      Proceedings of the 9th Workshop on Algorithm Engineering and
 *      Experiments (ALENEX), 71-83, 2007, doi:10.1137/1.9781611972870.7
 * \see J. Barbay, A. López-Ortiz, T. Lu, A. Salinger, An experimental
 *      investigation of set intersection algorithms for text searching,
 *      ACM Journal of Experimental Algorithmics 14, 3.7, 2009,
 *      doi:10.1145/1498698.1564507
 * \see E. D. Demaine, A. López-Ortiz, J. I. Munro, Adaptive set
 *      intersections, unions, and differences, Proceedings of the 11th
 *      ACM-SIAM Symposium on Discrete Algorithms (SODA), 743-752, 2000
 * \see B. Ding, A. C. König, Fast set intersection in memory, Proceedings
 *      of the VLDB Endowment 4(4), 255-266, 2011,
 *      doi:10.14778/1938545.1938550
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

#ifndef __ADS_SortedIntersection
 #define __ADS_SortedIntersection
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/TwoLevelMemory.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items common to \f$ A \f$ and \f$ B \f$, by merging
///
/// In pseudocode (Ferragina, chapter 6, and the procedure of section
/// 8.3 of the book), each comparison passing at least one item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intersectMerge}( A , B ) \cr
/// 2 & \quad i \gets 0;\ j \gets 0;\ C \gets ( ) \cr
/// 3 & \quad \textbf{while}\ i < n\ \textbf{and}\ j < m\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ i ] < B[ j ]\ \textbf{then}\ i \gets i + 1 \cr
/// 5 & \qquad \textbf{else if}\ B[ j ] < A[ i ]\ \textbf{then}\ j \gets j +
///     1 \cr
/// 6 & \qquad \textbf{else}\ \text{append } A[ i ] \text{ to } C;\ i \gets
///     i + 1;\ j \gets j + 1 \cr
/// 7 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ strictly increasing (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectMerge( const std::vector< T > & A ,
                                 const std::vector< T > & B , C less = C() );

/// the items common to \f$ A \f$ and \f$ B \f$, by a binary search in
/// the longer list of each item of the shorter one
///
/// In pseudocode (Ferragina, chapter 6), \f$ X \f$ the longer list and
/// \f$ Y \f$ the shorter:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intersectBinarySearch}( A , B ) \cr
/// 2 & \quad C \gets ( ) \cr
/// 3 & \quad \textbf{foreach}\ y \in Y\ \textbf{do} \cr
/// 4 & \qquad j \gets \text{the first place of } X \text{ not smaller than }
///     y \text{, by a binary search} \cr
/// 5 & \qquad \textbf{if}\ X[ j ] = y\ \textbf{then}\ \text{append } y
///     \text{ to } C \cr
/// 6 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ strictly increasing (an audit)
/// \post the result of `intersectMerge()` (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectBinarySearch( const std::vector< T > & A ,
                                        const std::vector< T > & B ,
                                        C less = C() );

/// the items common to \f$ A \f$ and \f$ B \f$, by mutual partitions
///
/// In pseudocode (Baeza-Yates, and Ferragina, chapter 6), on the parts
/// \f$ X[ a .. b ) \f$ and \f$ Y[ c .. d ) \f$, the result appended to
/// \f$ C \f$ in increasing order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{partition}( X , a , b , Y , c , d ) \cr
/// 2 & \quad \textbf{if}\ a \ge b\ \textbf{or}\ c \ge d\ \textbf{then
///     return} \cr
/// 3 & \quad \textbf{if}\ d - c > b - a\ \textbf{then}
///     \ \mathrm{partition}( Y , c , d , X , a , b );\ \textbf{return} \cr
/// 4 & \quad k \gets \lfloor ( c + d ) / 2 \rfloor;\ j \gets \text{the first }
///     \text{place of } X[ a .. b ) \text{ not smaller than } Y[ k ] \cr
/// 5 & \quad \mathrm{partition}( X , a , j , Y , c , k ) \cr
/// 6 & \quad \textbf{if}\ j < b\ \textbf{and}\ X[ j ] = Y[ k ]
///     \ \textbf{then}\ \text{append } Y[ k ] \text{ to } C;\ j \gets j + 1
///     \cr
/// 7 & \quad \mathrm{partition}( X , j , b , Y , k + 1 , d ) \cr
/// 8 & \textbf{function}\ \mathrm{intersectMutualPartition}( A , B ) \cr
/// 9 & \quad C \gets ( );\ \mathrm{partition}( A , 0 , n , B , 0 , m );
///     \ \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ strictly increasing (an audit)
/// \post the result of `intersectMerge()` (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectMutualPartition( const std::vector< T > & A ,
                                           const std::vector< T > & B ,
                                           C less = C() );

/// the items common to \f$ A \f$ and \f$ B \f$, by doubling searches
///
/// In pseudocode (Bentley and Yao, and Ferragina, chapter 6), \f$ X \f$
/// the longer list, of \f$ n \f$ items, and \f$ Y \f$ the shorter:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intersectDoubling}( A , B ) \cr
/// 2 & \quad i \gets 0;\ C \gets ( ) \cr
/// 3 & \quad \textbf{foreach}\ y \in Y\ \textbf{do} \cr
/// 4 & \qquad \ell \gets i;\ d \gets 1 \cr
/// 5 & \qquad \textbf{while}\ i + d - 1 < n\ \textbf{and}\ X[ i + d - 1 ] <
///     y\ \textbf{do}\ \ell \gets i + d;\ d \gets 2 d \cr
/// 6 & \qquad j \gets \text{the first place of } X[ \ell .. \min( i + d , n
///     ) ) \text{ not smaller than } y \cr
/// 7 & \qquad \textbf{if}\ j < n\ \textbf{and}\ X[ j ] = y\ \textbf{then}
///     \ \text{append } y \text{ to } C;\ j \gets j + 1 \cr
/// 8 & \qquad i \gets j \cr
/// 9 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ strictly increasing (an audit)
/// \post the result of `intersectMerge()` (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectDoubling( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    C less = C() );

/// the items common to \f$ A \f$ and \f$ B \f$, by the first items of
/// the blocks of \f$ L \f$ items of \f$ A \f$
///
/// In pseudocode (Ferragina, chapter 6), \f$ A' \f$ the first items of the
/// blocks, \f$ h \f$ the block reached and \f$ p \f$ the place reached in
/// \f$ A \f$, both only going forward:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intersectTwoLevel}( A , B , L ) \cr
/// 2 & \quad A' \gets ( A[ 0 ] , A[ L ] , A[ 2 L ] , \ldots );\ h \gets 0;
///     \ p \gets 0;\ C \gets ( ) \cr
/// 3 & \quad \textbf{foreach}\ b \in B\ \textbf{with}\ b \ge A'[ 0 ]
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{while}\ h + 1 < | A' |\ \textbf{and}\ A'[ h + 1 ] \le
///     b\ \textbf{do}\ h \gets h + 1 \cr
/// 5 & \qquad p \gets \max( p , h L );\ e \gets \min( ( h + 1 ) L , n ) \cr
/// 6 & \qquad \textbf{while}\ p < e\ \textbf{and}\ A[ p ] < b
///     \ \textbf{do}\ p \gets p + 1 \cr
/// 7 & \qquad \textbf{if}\ p < e\ \textbf{and}\ A[ p ] = b\ \textbf{then}
///     \ \text{append } b \text{ to } C \cr
/// 8 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ L \ge 1 \f$, \f$ A \f$ and \f$ B \f$ strictly increasing
///      (an audit)
/// \post the result of `intersectMerge()` (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectTwoLevel( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    std::size_t L , C less = C() );

/// the first item of each block of \f$ L \f$ items of \f$ A \f$, into
/// \f$ H \f$, by one scan
/// \pre \f$ L \ge 1 \f$, \f$ H \f$ empty, the files on the same memory
template< class T >
void twoLevelHeaders( const DiskFile< T > & A , std::size_t L ,
                      DiskFile< T > & H );

/// the items common to the sorted files \f$ A \f$ and \f$ B \f$ into
/// `out`, by merging, \f$ \lceil n / B \rceil + \lceil m / B \rceil \f$
/// reads
///
/// In pseudocode (Ferragina, chapter 6), the merge of `intersectMerge()`
/// on two readers and a writer of the disk:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{intersectMergeExternal}( A , B ,
///     \mathit{out} ) \cr
/// 2 & \quad \textbf{while}\ \text{neither reader is at its end }
///     \ \textbf{do} \cr
/// 3 & \qquad \text{pass the smaller of the two items, or write the item }
///     \text{to } \mathit{out} \text{ and pass both if they are equal}
/// \end{array}}
/// \f]
/// \pre \f$ M \ge 3 B \f$, `out` empty, the files on the same memory,
///      \f$ A \f$ and \f$ B \f$ strictly increasing (an audit)
template< class T , class C = std::less<> >
void intersectMergeExternal( const DiskFile< T > & A ,
                             const DiskFile< T > & B , DiskFile< T > & out ,
                             C less = C() );

/// the items common to the sorted files \f$ A \f$ and \f$ B \f$ into
/// `out`, by the headers \f$ H \f$ of the blocks of \f$ L \f$ items of
/// \f$ A \f$ [see `twoLevelHeaders()`]
///
/// In pseudocode (Ferragina, chapter 6), `intersectTwoLevel()` with
/// \f$ H \f$ read in order and \f$ A \f$ read only in the blocks met:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{intersectTwoLevelExternal}( A , H , L ,
///     B , \mathit{out} ) \cr
/// 2 & \quad \textbf{foreach}\ b \in B\ \textbf{with}\ b \ge H[ 0 ]
///     \ \textbf{do} \cr
/// 3 & \qquad h \gets \text{the last block with } H[ h ] \le b \text{, }
///     \text{reading } H \text{ forward} \cr
/// 4 & \qquad \text{scan the block } h \text{ of } A \text{ forward from the }
///     \text{place reached, up to } b \cr
/// 5 & \qquad \textbf{if}\ \text{it holds } b\ \textbf{then}\ \text{write }
///     b \text{ to } \mathit{out}
/// \end{array}}
/// \f]
/// \pre \f$ L \ge 1 \f$, \f$ M \ge 4 B \f$, `out` empty, the files on the
///      same memory, \f$ A \f$ and \f$ B \f$ strictly increasing (an
///      audit)
/// \post the result of `intersectMerge()` (an audit)
template< class T , class C = std::less<> >
void intersectTwoLevelExternal( const DiskFile< T > & A ,
                                const DiskFile< T > & H , std::size_t L ,
                                const DiskFile< T > & B ,
                                DiskFile< T > & out , C less = C() );

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS PermutedBuckets --------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of values below \f$ 2^w \f$, permuted by
/// \f$ x \mapsto ( a x + c ) \bmod 2^w \f$ and split by the top bits of the
/// images into buckets of \f$ L \f$ items on average
class PermutedBuckets {

/*--------------------------------------------------------------------------*/
 public:

 /// the values of \f$ A \f$, distinct, in any order
 ///
 /// In pseudocode (Sanders and Transier, and Ferragina, chapter 6):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( A , w , L , a , c ) \cr
 /// 2 & \quad \ell \gets \text{the least } \ell \le w \text{ with } 2^\ell L
 ///     \ge | A | \cr
 /// 3 & \quad I \gets ( ( a x + c ) \bmod 2^w : x \in A ) \text{, sorted} \cr
 /// 4 & \quad \mathit{off}[ g ] \gets \text{the first place of } I
 ///     \text{ whose top } \ell \text{ bits are } g \text{, by counting}
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le w \le 63 \f$, \f$ a \f$ odd, \f$ L \ge 1 \f$, the
 ///      values of \f$ A \f$ distinct and below \f$ 2^w \f$
 PermutedBuckets( const std::vector< std::uint64_t > & A , unsigned w ,
                  std::size_t L , std::uint64_t a , std::uint64_t c );

 /// the items
 [[nodiscard]] std::size_t size( void ) const;

 /// the buckets, \f$ 2^\ell \f$
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the values common to the two sets, sorted; the items of the two sets
 /// looked at go to `*scanned`, if given
 ///
 /// In pseudocode (Sanders and Transier, and Ferragina, chapter 6),
 /// \f$ S \f$
 /// the smaller set and \f$ G \f$ the larger, \f$ j \f$ only going
 /// forward in the images of \f$ G \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{intersect}( S , G ) \cr
 /// 2 & \quad j \gets 0;\ C \gets ( ) \cr
 /// 3 & \quad \textbf{foreach}\ \text{image } y \text{ of } S \text{, in }
 ///     \text{increasing order}\ \textbf{do} \cr
 /// 4 & \qquad g \gets \text{the top } \ell \text{ bits of } y \text{ in } G;
 ///     \ j \gets \max( j , G.\mathit{off}[ g ] ) \cr
 /// 5 & \qquad \textbf{while}\ j < G.\mathit{off}[ g + 1 ]\ \textbf{and}
 ///     \ G.I[ j ] < y\ \textbf{do}\ j \gets j + 1 \cr
 /// 6 & \qquad \textbf{if}\ j < G.\mathit{off}[ g + 1 ]\ \textbf{and}
 ///     \ G.I[ j ] = y\ \textbf{then}\ \text{append } \pi^{-1}( y )
 ///     \text{ to }
 ///     C;\ j \gets j + 1 \cr
 /// 7 & \quad \textbf{return}\ C \text{, sorted}
 /// \end{array}}
 /// \f]
 /// \pre the same \f$ w \f$, \f$ a \f$ and \f$ c \f$
 /// \post the intersection of the two sets (an audit)
 [[nodiscard]] std::vector< std::uint64_t > intersect(
                                     const PermutedBuckets & other ,
                                     std::size_t * scanned = nullptr ) const;

 /// the values of the set, sorted, by inverting the permutation
 [[nodiscard]] std::vector< std::uint64_t > values( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// \f$ 2^w - 1 \f$
 [[nodiscard]] std::uint64_t mask( void ) const;

 /// \f$ \pi( x ) = ( a x + c ) \bmod 2^w \f$
 [[nodiscard]] std::uint64_t permute( std::uint64_t x ) const;

 /// \f$ \pi^{-1}( y ) = a^{-1} ( y - c ) \bmod 2^w \f$
 [[nodiscard]] std::uint64_t invert( std::uint64_t y ) const;

 unsigned f_w;                     ///< the bits of the universe
 unsigned f_ell;                   ///< the bits of a bucket number
 std::uint64_t f_a , f_c;          ///< the permutation
 std::uint64_t f_ainv;             ///< the inverse of a modulo 2^w
 std::vector< std::uint64_t > f_img;   ///< the images, sorted
 std::vector< std::size_t > f_off;     ///< the start of each bucket

 };  // end( class PermutedBuckets )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/SortedIntersection.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SortedIntersection.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File SortedIntersection.h ---------------------*/
/*--------------------------------------------------------------------------*/
