/*--------------------------------------------------------------------------*/
/*---------------------------- File RunSorts.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The natural merge sorts that adapt to the runs of the input, beyond the
 * book [see naturalMergeSort() of SequenceAlgorithms.h for the plain one],
 * both stable:
 *
 * - a run is a maximal non-decreasing stretch, or a strictly decreasing one
 *   that is reversed (strictly, so that the reversal keeps the sort
 *   stable); a run shorter than minrun is extended to minrun items by a
 *   binary insertion sort, so that the merges start from runs long enough;
 * - timSort() of Peters (the sort of Python until 3.10 and of the objects of
 *   Java) keeps the runs on a stack whose lengths, from the top, must grow
 *   faster than the Fibonacci numbers, merging when the rule fails, with the
 *   check on the fourth run from the top added after the proof by de Gouw
 *   et al. that the original rule could fail;
 * - powerSort() of Munro and Wild (the sort of Python from 3.11) gives each
 *   boundary between two runs a power, the first bit where the midpoints of
 *   the two runs, as fractions of n, differ, and merges the runs on the
 *   stack while their power is larger than that of the new boundary: the
 *   merges then follow a nearly optimal tree of merges, at most
 *   \f$ n H + 2 n \f$ comparisons for H the entropy of the lengths of the
 *   runs, which is O( n ) on few runs and \f$ O( n \log n ) \f$ always.
 *
 * A merge first skips the items of the left run already before the first
 * item of the right one, and those of the right run already after the last
 * item of the left one, both by galloping (an exponential search and then a
 * binary one); it then merges with a buffer of the left run, switching to
 * galloping when one run wins 7 times in a row, as McIlroy proposed, so
 * that two interleaved runs merge in O( n ) and two runs one after the
 * other in \f$ O( \log n ) \f$ comparisons.
 *
 * The contracts [see Contract.h]: each sort ends with the same items
 * sorted, an audit. The functions are defined in RunSorts.tpp.
 *
 * \see T. Peters, listsort.txt, the documentation of the sort of CPython,
 *      2002
 * \see S. de Gouw, J. Rot, F. S. de Boer, R. Bubel, R. Hähnle, OpenJDK's
 *      java.utils.Collection.sort() is broken: the good, the bad and the
 *      worst case, Proceedings of CAV 2015, LNCS 9206, 273-289, 2015
 * \see J. I. Munro, S. Wild, Nearly-optimal mergesorts: fast, practical
 *      sorting methods that optimally adapt to existing runs, Proceedings
 *      of ESA 2018, LIPIcs 112, article 63, 2018
 * \see P. M. McIlroy, Optimistic sorting and information theoretic
 *      complexity, Proceedings of the 4th ACM-SIAM Symposium on Discrete
 *      Algorithms (SODA), 467-474, 1993
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

#ifndef __ADS_RunSorts
 #define __ADS_RunSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

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

/// Timsort; the number of runs found goes to *runs, if given
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void timSort( std::vector< T > & A , C less = C() ,
              std::size_t * runs = nullptr );

/// Powersort; the number of runs found goes to *runs, if given
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void powerSort( std::vector< T > & A , C less = C() ,
                std::size_t * runs = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/RunSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RunSorts.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RunSorts.h ---------------------------*/
/*--------------------------------------------------------------------------*/
