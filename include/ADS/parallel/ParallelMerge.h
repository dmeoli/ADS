/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelMerge.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel merge of k sorted ranges, beyond the book: multiwayMerge(
 * ex , R , out , less ) writes into out what multiwayMerge( R , out , less
 * ) [see MultiwayMerge.h] writes, the same items in the same order (on
 * equal items the range of lower index first), on the threads of the
 * executor ex [see Executor.h].
 *
 * The output of N items is cut into B parts of about N / B items, B = 4 p
 * for p threads (at least \f$ 2^{14} \f$ items each), and the part that
 * starts at rank r takes from each range i the items of ranks
 * \f$ [ s_i( r ) , s_i( r' ) ) \f$, where \f$ s( r ) \f$ is the exact
 * multisequence selection: the split with \f$ \sum_i s_i( r ) = r \f$
 * that leaves on its left the r least items in the total order of the
 * triples ( item , range , position ), which is the order of the stable
 * merge. Each part is then merged by the sequential multiwayMerge() into
 * its place of the output, in parallel, so that no part waits for
 * another. A split is found by narrowing a window \f$ [ lo_i , hi_i ) \f$
 * on each range: the pivot is the middle item x of the widest window, of
 * range j and position m, and a binary search in each window counts the
 * items before ( x , j , m ), those \f$ \leq x \f$ in the ranges i < j and
 * those < x in the ranges i > j; if they are fewer than r, x and all of
 * them go to the left (each \f$ lo_i \f$ rises to its count, \f$ lo_j \f$
 * past x), otherwise to the right. The widest window halves at each step
 * and is at least the k-th of the total, so that a split takes
 * \f$ O( k \log N ) \f$ steps of k binary searches, \f$ O( k^2 \log^2 N )
 * \f$ comparisons; the splits are found in parallel, one per part, and
 * cost far less than the merge, \f$ O( N \log k ) \f$, when
 * \f$ k \ll N \f$ (the selection of Varman et al. does it in
 * \f$ O( k \log k \log N ) \f$, which matters only for large k).
 *
 * The contracts [see Contract.h]: the iterators are random access, each
 * range sorted by less (an audit, as in the sequential merge), out does
 * not overlap the ranges; the output sorted, with the items of the ranges
 * (an audit).
 * \see P. J. Varman, S. D. Scheufler, B. R. Iyer, G. R. Ricard, Merging
 *      multiple lists on hierarchical-memory multiprocessors, Journal of
 *      Parallel and Distributed Computing 12(2), 171-177, 1991,
 *      doi:10.1016/0743-7315(91)90122-H
 * \see J. Singler, P. Sanders, F. Putze, MCSTL: the multi-core standard
 *      template library, Euro-Par 2007 Parallel Processing, LNCS 4641,
 *      682-694, 2007, doi:10.1007/978-3-540-74466-5_72
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ParallelMerge
 #define __ADS_ParallelMerge
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/priority/MultiwayMerge.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the split of the sorted ranges R at rank r of the stable merge: the
/// number of items that each range gives to the r least
/// \pre each range sorted by less, r at most the items of R
template< std::random_access_iterator I , class C = std::less<> >
[[nodiscard]] std::vector< std::size_t > multisequenceSplit(
                         const std::vector< std::pair< I , I > > & R ,
                         std::size_t r , C less = C() );

/// the items of the sorted ranges R merged into out on the threads of ex,
/// the range of lower index first on equal items; the end of the output
/// \pre each range sorted by less (an audit)
/// \post the output sorted, with the items of the ranges (an audit)
template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class C = std::less<> >
O multiwayMerge( E & ex , const std::vector< std::pair< I , I > > & R ,
                 O out , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelMerge.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelMerge.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelMerge.h ------------------------*/
/*--------------------------------------------------------------------------*/
