/*--------------------------------------------------------------------------*/
/*------------------------ File VectorIntersection.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered intersection of two sorted sets, beyond the book [see
 * SortedIntersection.h for the algorithms of the notes and SortedListSet.h
 * for the merge of section 8.3]: intersect() writes the items common to A
 * of n items and B of m items, both strictly increasing, into out, and
 * gives their number.
 *
 * - On lists of similar lengths (the longer at most 32 times the shorter)
 *   the two lists are merged a block of 8 items at a time on each side, in
 *   at most \f$ ( n + m ) / 8 \f$ steps, on the integers of 32 bits with
 *   AVX2: the block of A is compared with
 *   the 8 rotations of the block of B, 8 comparisons of vectors that find
 *   all the equal pairs, the items of A found are packed to the front of
 *   a vector by a permutation looked up from the mask of the comparison and
 *   stored, and the block whose last item is not larger moves on (both, on
 *   a tie), with no branch; the items left at the end are merged one by
 *   one. On the other types, and without AVX2, the merge goes one item at a
 *   time with no branch: an item is written whatever happens, the count of
 *   the output advances on equality, and each list advances when its item
 *   is not larger, three additions in place of the jumps of the merge,
 *   which mispredict on lists that interleave at random.
 * - On lists of very different lengths (the longer more than 32 times the
 *   shorter), each item of the shorter one is looked for in the longer one
 *   from the place of the previous one. On the integers of 32 bits with
 *   AVX2, the longer list is read through a window of 64 items, which moves
 *   by strides of 1024 and then of 64 items while its last item is smaller
 *   than the item looked for (the line 8 strides ahead prefetched); the
 *   item is then compared with the 64 of the window at once, 8 comparisons
 *   of vectors and no search inside the window, so that an item at a gap
 *   of g places from the previous one costs \f$ \lfloor g / 1024 \rfloor +
 *   \min( 16 , \lfloor g / 64 \rfloor ) + O( 1 ) \f$ steps that the
 *   processor predicts (a binary search on the windows would mispredict
 *   half of its steps). Elsewhere the item is found by galloping, the
 *   places 1, 2, 4, ... ahead, and a binary search with no branch [see
 *   branchlessLowerBound() in StaticSearch.h], \f$ 2 \lceil \log_2 ( g + 1
 *   ) \rceil + 1 \f$ comparisons, \f$ O( m \log( n / m ) ) \f$ for all [see
 *   intersectDoubling() in SortedIntersection.h].
 *
 * Compared with the literature: the blocks compared all against all are
 * the method of Schlegel, Willhalm and Lehner on SSE (with the instruction
 * of the comparison of strings) and of Lemire, Boytsov and Kurz, whose
 * library switches to galloping on a ratio of 50; the compressed bitmaps of
 * Roaring [see RoaringBitmap.h] answer by and-ing the containers of \f$
 * 2^{16} \f$ values instead, which is faster on dense sets and slower on
 * sparse ones.
 *
 * The contracts [see Contract.h]: A and B strictly increasing, audits; out
 * has room for min( n , m ) + 8 items (the vectors store 8 at a time); the
 * result is the intersection, strictly increasing, a postcondition checked
 * as an audit against std::set_intersection. The functions are defined in
 * VectorIntersection.tpp.
 *
 * \see B. Schlegel, T. Willhalm, W. Lehner, Fast sorted-set intersection
 *      using SIMD instructions, Proceedings of ADMS 2011, 1-8, 2011
 * \see D. Lemire, L. Boytsov, N. Kurz, SIMD compression and the
 *      intersection of sorted integers, Software: Practice and Experience
 *      46(6), 723-749, 2016, doi:10.1002/spe.2326
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
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

#ifndef __ADS_VectorIntersection
 #define __ADS_VectorIntersection
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items common to a[ 0 .. n ) and b[ 0 .. m ), written to out, and
/// their number
/// \pre a and b strictly increasing; out has room for min( n , m ) + 8
template< class T >
std::size_t intersect( const T * a , std::size_t n , const T * b ,
                       std::size_t m , T * out );

/// the items common to A and B
/// \pre A and B strictly increasing
template< class T >
std::vector< T > intersect( const std::vector< T > & A ,
                            const std::vector< T > & B );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/VectorIntersection.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* VectorIntersection.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File VectorIntersection.h ---------------------*/
/*--------------------------------------------------------------------------*/
