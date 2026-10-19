/*--------------------------------------------------------------------------*/
/*-------------------------- File NthElement.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered selection, beyond the book [see Selection.h for the
 * selection of Hoare and the median of the medians of section 17.1]:
 * nthElement() puts in the place nth of [ first , last ) the item that
 * would be there if the range were sorted, the items before it not larger
 * and those after it not smaller, as std::nth_element, which it is meant
 * to replace.
 *
 * The pivot is chosen by the sampling of Floyd and Rivest: on a range of n
 * > 600 items and a rank k, a sample of \f$ s = n^{2/3} / 2 \f$ items
 * around the place k is selected recursively for the rank \f$ k s / n \f$
 * shifted by \f$ \sqrt{ s \ln n } / 2 \f$ standard deviations towards the
 * median, so that the pivot falls just on the side of k that leaves the
 * smaller part to recur on; the expected number of comparisons is \f$ n +
 * \min( k , n - k ) + o( n ) \f$, against the \f$ 2 n + 2 k \ln( n / k ) +
 * \dots \f$ of the pivot at random (up to 3.39 n for the median). Below
 * 600 items the pivot is the median of three.
 *
 * The partition is the scheme of Lomuto with no branch: an item is moved
 * to the end of the smaller ones and the item there to its place, and the
 * end advances by the outcome of the comparison, an addition rather than a
 * jump, so that a comparison is never mispredicted (the branches of the
 * partition of Hoare, of std::nth_element, are mispredicted half of the
 * times on random items, some 10 cycles each). When the pivot is the
 * smallest item of the range, the items equal to it go to the left side,
 * so that many equal items end in one step.
 *
 * Each round that does not cut the range by at least a quarter counts
 * against a budget of \f$ 2 \log_2 n \f$ rounds; past it the pivot is the
 * median of the medians of the groups of 5 [see medianOfMedians() in
 * Selection.h], so that the worst case stays linear. A range of at most 24
 * items is sorted by insertion.
 *
 * The contracts [see Contract.h]: nth in [ first , last ); the items
 * before nth are not larger than it and those after it not smaller, an
 * audit. The function is defined in NthElement.tpp.
 *
 * \see R. W. Floyd, R. L. Rivest, Expected time bounds for selection,
 *      Communications of the ACM 18(3), 165-172, 1975,
 *      doi:10.1145/360680.360691
 * \see K. C. Kiwiel, On Floyd and Rivest's SELECT algorithm, Theoretical
 *      Computer Science 347(1-2), 214-238, 2005,
 *      doi:10.1016/j.tcs.2005.06.032
 * \see A. Alexandrescu, Fast deterministic selection, Proceedings of SEA
 *      2017, LIPIcs 75, 24:1-24:19, 2017, doi:10.4230/LIPIcs.SEA.2017.24
 * \see S. Edelkamp, A. Weiß, BlockQuicksort: avoiding branch mispredictions
 *      in quicksort, ACM Journal of Experimental Algorithmics 24, 1.4:1-
 *      1.4:22, 2019, doi:10.1145/3274660
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

#ifndef __ADS_NthElement
 #define __ADS_NthElement
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// puts in nth the item of its rank in [ first , last ) by less(), the
/// items before it not larger and those after it not smaller
/// \pre first <= nth < last, or first == last
template< std::random_access_iterator It , class C = std::less<> >
void nthElement( It first , It nth , It last , C less = C() );

/// the item of rank k (from 0) of A, which is rearranged
/// \pre k < A.size()
template< class T , class C = std::less<> >
T nthElement( std::vector< T > & A , std::size_t k , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/NthElement.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* NthElement.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File NthElement.h --------------------------*/
/*--------------------------------------------------------------------------*/
