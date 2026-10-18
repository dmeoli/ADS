/*--------------------------------------------------------------------------*/
/*----------------------- File SortedIntersection.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The intersection of two sorted sets, the core of the resolution of a
 * query of several words by an inverted index (chapter 6 of the notes):
 * A of n items and B of m items, strictly increasing, \f$ m \leq n \f$
 * (the roles are swapped otherwise).
 *
 * - intersectMerge() is the scan of the two lists of Fact 6.1, at most
 *   n + m comparisons, \f$ O( ( n + m ) / B ) \f$ I/Os whatever B, optimal
 *   when m and n are close; it is the procedure of section 8.3 of the book
 *   that setIntersection() of SortedListSet.h applies to its lists.
 * - intersectBinarySearch() searches each item of B in A, \f$ O( m \log n
 *   ) \f$ comparisons (Fact 6.2), better when \f$ m = o( n / \log n ) \f$.
 * - intersectMutualPartition() is the algorithm of Baeza-Yates (Algorithm
 *   6.1): the median of the shorter list searched in the longer one splits
 *   both, and the two halves recur, the roles swapped when the part of the
 *   longer one is the shorter, \f$ O( m ( 1 + \log( n / m ) ) ) \f$
 *   (Fact 6.3), which is optimal, since \f$ \log_2 \binom{n}{m} \f$
 *   comparisons are needed to tell the subsets of A of m items apart.
 * - intersectDoubling() searches each item of B from the place of the
 *   previous one by galloping, the places 1 , 2 , 4 , ... ahead and then a
 *   binary search within the last gap (Algorithm 6.2, the exponential
 *   search of Bentley and Yao; gallopSearch() of BinarySearch.h is the
 *   same search from the start of a vector, and the merges of RunSorts.h
 *   gallop in the same way), \f$ \sum_j ( 1 + \log \Delta_j ) = O( m ( 1 +
 *   \log( n / m ) ) ) \f$ comparisons with no recursion (Fact 6.4).
 * - intersectTwoLevel() is the two-level storage of Fact 6.5: the first
 *   item of each block of L items of A in an array A' of n / L items,
 *   merged with B to find the block where each item of B may be, and each
 *   block met merged with its items of B, \f$ O( n / L + m L ) \f$ time;
 *   on the disk [see TwoLevelMemory.h] intersectTwoLevelExternal() takes
 *   \f$ O( n / L B + m L / B + m ) \f$ I/Os, each block of A met costing
 *   at most \f$ \lceil L / B \rceil + 1 \f$ reads, against the
 *   \f$ \lceil n / B \rceil + \lceil m / B \rceil \f$ of
 *   intersectMergeExternal().
 * - PermutedBuckets is the random permutation and splitting of Fact 6.6
 *   (the lookup of Sanders and Transier): the values of the universe
 *   \f$ [ 0 , 2^w ) \f$ permuted by \f$ \pi( x ) = ( a x + c ) \bmod 2^w
 *   \f$, a odd so that \f$ \pi \f$ is invertible, and a list of n items
 *   split by the top \f$ \ell = \lceil \log_2( n / L ) \rceil \f$ bits of
 *   the images into buckets of L items on average, the images sorted and
 *   addressed by a table of offsets; each image of the shorter list is
 *   looked for in the bucket of the longer one named by its own top bits,
 *   so that a bucket of A is never looked at unless B has items in it:
 *   \f$ O( m + \min\{ n , m L \} ) \f$ expected time, and the lists need
 *   not be sorted beforehand, only their images are.
 *
 * Compared with the literature: the experiments of Sanders and Transier,
 * and of Barbay et al., find the lookup best up to lists of similar
 * length, the two-level skipping better than the mutual partition for all
 * but the most unequal lists, whose recursion costs more than its bound
 * says; the adaptive algorithms of Demaine, López-Ortiz and Munro and of
 * Barbay and Kenyon extend the galloping to k lists, and Ding and König
 * replace the buckets by word-sized bitmaps.
 *
 * The contracts [see Contract.h]: A and B strictly increasing, audits; L >=
 * 1; the result is the intersection, strictly increasing, a postcondition
 * checked as an audit against intersectMerge(); 1 <= w <= 63, a odd, the
 * values below 2^w, and two PermutedBuckets intersected with the same
 * permutation; the files of the external versions share a memory of at
 * least 4 B. The functions are defined in SortedIntersection.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 6
 * \see R. Baeza-Yates, A fast set intersection algorithm for sorted
 *      sequences, Proceedings of the 15th Symposium on Combinatorial
 *      Pattern Matching (CPM), Lecture Notes in Computer Science 3109,
 *      400-408, 2004, doi:10.1007/978-3-540-27801-6_30
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
 * \see P. Sanders, F. Transier, Intersection in integer inverted indices,
 *      Proceedings of the 9th Workshop on Algorithm Engineering and
 *      Experiments (ALENEX), 71-83, 2007
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
 *         Universita' di Pisa \n
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

/// the items common to A and B, by merging
/// \pre A and B strictly increasing (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectMerge( const std::vector< T > & A ,
                                 const std::vector< T > & B , C less = C() );

/// the items common to A and B, by a binary search in the longer list of
/// each item of the shorter one
/// \pre A and B strictly increasing (an audit)
/// \post the result of intersectMerge() (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectBinarySearch( const std::vector< T > & A ,
                                        const std::vector< T > & B ,
                                        C less = C() );

/// the items common to A and B, by mutual partitions
/// \pre A and B strictly increasing (an audit)
/// \post the result of intersectMerge() (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectMutualPartition( const std::vector< T > & A ,
                                           const std::vector< T > & B ,
                                           C less = C() );

/// the items common to A and B, by doubling searches
/// \pre A and B strictly increasing (an audit)
/// \post the result of intersectMerge() (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectDoubling( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    C less = C() );

/// the items common to A and B, by the first items of the blocks of L
/// items of A
/// \pre L >= 1, A and B strictly increasing (an audit)
/// \post the result of intersectMerge() (an audit)
template< class T , class C = std::less<> >
std::vector< T > intersectTwoLevel( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    std::size_t L , C less = C() );

/// the first item of each block of L items of A, into H, by one scan
/// \pre L >= 1, H empty, the files on the same memory
template< class T >
void twoLevelHeaders( const DiskFile< T > & A , std::size_t L ,
                      DiskFile< T > & H );

/// the items common to the sorted files A and B into out, by merging
/// \pre M >= 3 B, out empty, the files on the same memory, A and B
///      strictly increasing (an audit)
template< class T , class C = std::less<> >
void intersectMergeExternal( const DiskFile< T > & A ,
                             const DiskFile< T > & B , DiskFile< T > & out ,
                             C less = C() );

/// the items common to the sorted files A and B into out, by the headers H
/// of the blocks of L items of A [see twoLevelHeaders()]
/// \pre L >= 1, M >= 4 B, out empty, the files on the same memory, A and B
///      strictly increasing (an audit)
/// \post the result of intersectMerge() (an audit)
template< class T , class C = std::less<> >
void intersectTwoLevelExternal( const DiskFile< T > & A ,
                                const DiskFile< T > & H , std::size_t L ,
                                const DiskFile< T > & B ,
                                DiskFile< T > & out , C less = C() );

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS PermutedBuckets --------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of values below 2^w, permuted by x -> a x + c mod 2^w and split
/// by the top bits of the images into buckets of L items on average
class PermutedBuckets {

/*--------------------------------------------------------------------------*/
 public:

 /// the values of A, distinct, in any order
 /// \pre 1 <= w <= 63, a odd, L >= 1, the values of A distinct and below
 ///      2^w
 PermutedBuckets( const std::vector< std::uint64_t > & A , unsigned w ,
                  std::size_t L , std::uint64_t a , std::uint64_t c );

 /// the items
 [[nodiscard]] std::size_t size( void ) const;

 /// the buckets, 2^ell
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the values common to the two sets, sorted; the items of the two sets
 /// looked at go to *scanned, if given
 /// \pre the same w, a and c
 /// \post the intersection of the two sets (an audit)
 [[nodiscard]] std::vector< std::uint64_t > intersect(
                                     const PermutedBuckets & other ,
                                     std::size_t * scanned = nullptr ) const;

 /// the values of the set, sorted, by inverting the permutation
 [[nodiscard]] std::vector< std::uint64_t > values( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 [[nodiscard]] std::uint64_t mask( void ) const;

 [[nodiscard]] std::uint64_t permute( std::uint64_t x ) const;

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
