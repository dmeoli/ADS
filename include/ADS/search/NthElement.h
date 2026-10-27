/*--------------------------------------------------------------------------*/
/*-------------------------- File NthElement.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered selection, beyond the book [see `Selection.h` for the
 * selection of Hoare of section 17.1 and the median of the medians]:
 * `nthElement()` puts in the place `nth` of the range
 * \f$ [ \mathit{first} , \mathit{last} ) \f$ the item that would be there if
 * the range were sorted, the items before it not larger and those after it
 * not smaller, as `std::nth_element`, which it is meant to replace.
 *
 * The pivot is chosen by the sampling of Floyd and Rivest: on a range of
 * \f$ n > 600 \f$ items and a rank \f$ k \f$ (from 0), a sample of the places
 * from
 * \f[
 *   \ell = \max\Big( 0 , k - \frac{k s}{n} + \delta \Big) \quad
 *   \mbox{to} \quad
 *   r = \min\Big( n - 1 , k + \frac{( n - k ) s}{n} + \delta \Big) ,
 *   \qquad s = \frac{1}{2} n^{2/3} , \quad
 *   \delta = \pm \frac{1}{2} \sqrt{ \frac{ s ( n - s ) \ln n }{ n } }
 * \f]
 * (the sign of \f$ \delta \f$ towards the median) is selected recursively
 * for the rank \f$ k \f$, so that the item that lands in the place \f$ k \f$
 * falls just on the side of \f$ k \f$ that leaves the smaller part to recur
 * on; the expected number of comparisons is
 * \f$ n + \min( k , n - k ) + O( n^{2/3} ( \ln n )^{1/3} ) \f$ (Kiwiel),
 * against the
 * \f$ 2 n + 2 k \ln( n / k ) + 2 ( n - k ) \ln( n / ( n - k ) ) + o( n ) \f$
 * of a pivot at random (up to \f$ 2 ( 1 + \ln 2 ) n \approx 3.39 n \f$ for
 * the median). Below 600 items the pivot is the median of three.
 *
 * The partition suits the order of the range, which 64 items taken at a
 * stride tell: on a random order (more than 16 turns, a rise followed by a
 * fall or the opposite, among them; about \f$ 2 \cdot 62 / 3 \approx 41 \f$
 * on a random permutation) it is the scheme of Lomuto with no branch, an
 * item moved to the end of the smaller ones and the item there to its
 * place, the end advancing by the outcome of the comparison (an addition
 * rather than a jump, so that no comparison is mispredicted, where the
 * partition of Hoare mispredicts half of them on random items, some 10
 * cycles each), unrolled by 4 so that the loop is not at the mercy of the
 * alignment of one jump; on a range in some order (an organ pipe, a few
 * runs) it is the scheme of Hoare, whose branches are then predicted and
 * which writes only the items on the wrong side, and the sample of Floyd
 * and Rivest is first gathered from the whole range at a stride, since a
 * contiguous one would not represent it. A range whose 64 items rise all
 * (or fall all) is checked to be sorted, by blocks of 64 pairs with no exit
 * inside a block, and is then left as it is (or reversed in the same pass
 * of the check), \f$ n \f$ comparisons in all. When the pivot is the
 * smallest item of the range, the items equal to it go to the left side, so
 * that many equal items end in one step; the items of a range cut on its
 * left by a pivot are not smaller than it, and a later pivot equal to it is
 * known to be the smallest with no pass.
 *
 * Each round that does not cut the range by at least a quarter counts
 * against a budget of \f$ 2 \lceil \log_2 ( n + 1 ) \rceil \f$ rounds; past
 * it the pivot is the median of the medians of the groups of 5, which has at
 * least \f$ 3 n / 10 \f$ items on each side, so that
 * \f$ T( n ) \le T( n / 5 ) + T( 7 n / 10 ) + O( n ) = O( n ) \f$ in the
 * worst case. A range of at most 24 items is sorted by insertion.
 *
 * The contracts [see `Contract.h`]: `nth` in
 * \f$ [ \mathit{first} , \mathit{last} ) \f$; the items before `nth` are not
 * larger than it and those after it not smaller, an audit. The function is
 * defined in `NthElement.tpp`.
 *
 * \see R. W. Floyd, R. L. Rivest, Expected time bounds for selection,
 *      Communications of the ACM 18(3), 165-172, 1975,
 *      doi:10.1145/360680.360691
 * \see R. W. Floyd, R. L. Rivest, Algorithm 489: the algorithm SELECT, for
 *      finding the ith smallest of n elements, Communications of the ACM
 *      18(3), 173, 1975, doi:10.1145/360680.360694
 * \see K. C. Kiwiel, On Floyd and Rivest's SELECT algorithm, Theoretical
 *      Computer Science 347(1-2), 214-238, 2005,
 *      doi:10.1016/j.tcs.2005.06.032
 * \see M. Blum, R. W. Floyd, V. Pratt, R. L. Rivest, R. E. Tarjan, Time
 *      bounds for selection, Journal of Computer and System Sciences 7(4),
 *      448-461, 1973, doi:10.1016/S0022-0000(73)80033-9
 * \see S. Edelkamp, A. Weiß, BlockQuicksort: avoiding branch mispredictions
 *      in quicksort, Proceedings of ESA 2016, LIPIcs 57, 38:1-38:16, 2016,
 *      doi:10.4230/LIPIcs.ESA.2016.38
 * \see C. A. R. Hoare, Algorithm 65: Find, Communications of the ACM 4(7),
 *      321-322, 1961, doi:10.1145/366622.366647
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
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the item of its rank put in `nth`, in
/// \f$ [ \mathit{first} , \mathit{last} ) \f$ by `less()`, the items before
/// it not larger and those after it not smaller
///
/// In pseudocode (the selection of Hoare with the sample of Floyd and
/// Rivest, beyond the book), \f$ A[ 0 .. n - 1 ] \f$ the range and \f$ k \f$
/// the place `nth`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{nthElement}( A , k ) \cr
/// 2 & \quad \textbf{if}\ n > 600 \text{ and the 64 items at a stride rise}
///     \text{ (fall)}\ \textbf{then} \cr
/// 3 & \qquad \textbf{if}\ A \text{ is sorted (reversed, and then }
///     \text{reversed back)}\ \textbf{then return} \cr
/// 4 & \quad \mathit{budget} \gets 2 \lceil \log_2 ( n + 1 ) \rceil \cr
/// 5 & \quad \textbf{while}\ \text{the part } A[ i .. j ] \text{ has more }
///     \text{than 24 items}\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ \mathit{budget} \le 0\ \textbf{then}\ p \gets
///     \text{the median of the medians of the groups of 5} \cr
/// 7 & \qquad \textbf{else if}\ j - i + 1 > 600\ \textbf{then} \cr
/// 8 & \qquad\quad \mathrm{nthElement}( A[ \ell .. r ] , k )\quad
///     \triangleright\ \text{the sample of Floyd and Rivest};\ p \gets k \cr
/// 9 & \qquad \textbf{else}\ p \gets \text{the median of } A[ i ] , A[ ( i
///     + j ) / 2 ] , A[ j ] \cr
/// 10 & \qquad m \gets \text{the place of } A[ p ] \text{ after a partition}
///     \text{ of } A[ i .. j ] \text{ around it} \cr
/// 11 & \qquad \textbf{if}\ m = k\ \textbf{then return} \cr
/// 12 & \qquad \textbf{else if}\ k < m\ \textbf{then}\ j \gets m - 1
///     \ \textbf{else}\ i \gets m + 1 \cr
/// 13 & \qquad \textbf{if}\ \text{the part kept is more than } 3 / 4
///     \text{ of the last one}\ \textbf{then}\ \mathit{budget} \gets
///     \mathit{budget} - 1 \cr
/// 14 & \quad \text{sort } A[ i .. j ] \text{ by insertion}
/// \end{array}}
/// \f]
/// where the partition is that of Lomuto with no branch on a part with
/// many turns, that of Hoare on a part in some order, and puts the items
/// equal to the pivot on its left when the pivot is the smallest item of
/// the part.
/// \pre \f$ \mathit{first} \le \mathit{nth} < \mathit{last} \f$, or
///      \f$ \mathit{first} = \mathit{last} \f$
/// \post the items before `nth` are not larger than it, those after it not
///       smaller (audit)
template< std::random_access_iterator It , class C = std::less<> >
void nthElement( It first , It nth , It last , C less = C() );

/// the item of rank \f$ k \f$ (from 0) of \f$ A \f$, which is rearranged by
/// `nthElement()` above
/// \pre \f$ k < n \f$
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
