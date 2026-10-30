/*--------------------------------------------------------------------------*/
/*---------------------------- File RunSorts.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The natural merge sort that adapts to the runs of the input, beyond the
 * book [see `naturalMergeSort()` of `SequenceAlgorithms.h` for the plain
 * one], stable:
 *
 * - a run is a maximal non-decreasing stretch, or a strictly decreasing one
 *   that is reversed (strictly, so that the reversal keeps the sort stable); a
 *   run shorter than \f$ \mathit{minrun} = 24 \f$ items is extended to
 *   \f$ \mathit{minrun} \f$ items by a binary insertion sort, so that the
 *   merges start from runs long enough;
 * - `powerSort()` of Munro and Wild (the sort of Python from 3.11) gives each
 *   boundary between two runs a power, the first bit where the midpoints of
 *   the two runs, as fractions of \f$ n \f$, differ, and merges the runs on
 *   the stack while their power is larger than that of the new boundary: the
 *   merges then follow a nearly optimal tree of merges, at most
 *   \f$ n H + 2 n \f$ comparisons for \f$ H \f$ the entropy of the lengths of
 *   the runs, which is \f$ O( n ) \f$ on few runs and \f$ O( n \log n ) \f$
 *   always.
 *
 * A merge first skips the items of the left run already before the first item
 * of the right one, and those of the right run already after the last item of
 * the left one, both by galloping (an exponential search and then a binary
 * one), and rotates the two runs if the right one comes all before the left
 * one; it then merges with a buffer of the shorter run, from the front if it
 * is the left one and from the back otherwise, so that the buffer, made once,
 * holds half of the items, switching to galloping when one run wins 7 times in
 * a row, as McIlroy proposed, so that two interleaved runs merge in
 * \f$ O( n ) \f$ and two runs one after the other in \f$ O( \log n ) \f$
 * comparisons; as in Timsort, the 7 drops by one at each gallop that pays and
 * rises by two at each that does not. On the arithmetic types, two runs that
 * both come from runs extended to \f$ \mathit{minrun} \f$, i.e., from an input
 * in disorder whose merge no branch predictor guesses, merge with no branch on
 * the order (a conditional move and two increments); the runs of an input with
 * long runs keep the branches, which follow its patterns, and with the natural
 * order they do not gallop, whose steps cost more than the comparisons that it
 * saves.
 *
 * The contracts [see `Contract.h`]: the sort ends with the same items sorted,
 * an audit. The function is defined in `RunSorts.tpp`.
 *
 * \see T. Peters, listsort.txt, the documentation of the sort of CPython,
 *      2002
 * \see J. I. Munro, S. Wild, Nearly-optimal mergesorts: fast, practical
 *      sorting methods that optimally adapt to existing runs, Proceedings
 *      of ESA 2018, LIPIcs 112, article 63, 2018,
 *      doi:10.4230/LIPIcs.ESA.2018.63
 * \see P. M. McIlroy, Optimistic sorting and information theoretic
 *      complexity, Proceedings of the 4th ACM-SIAM Symposium on Discrete
 *      Algorithms (SODA), 467-474, 1993
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
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// Powersort; the number of runs found goes to `*runs`, if given
///
/// In pseudocode (Munro and Wild, beyond the book), the power of the boundary
/// between two adjacent runs \f$ [ s_1 , s_2 ) \f$ and \f$ [ s_2 , e_2 ) \f$
/// being the least \f$ p \f$ such that \f$ ( s_1 + s_2 ) / 2 n \f$ and
/// \f$ ( s_2 + e_2 ) / 2 n \f$ differ in their \f$ p \f$-th bit after the
/// point:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{powerSort}( A , n ) \cr
/// 2 & \quad S \gets \text{an empty stack};\ [ s , e ) \gets \text{the first }
///     \text{run} \cr
/// 3 & \quad \textbf{while}\ e < n\ \textbf{do} \cr
/// 4 & \qquad [ e , e_2 ) \gets \text{the next run};\ p \gets \text{the }
///     \text{power of the boundary } e \cr
/// 5 & \qquad \textbf{while}\ S \ne \emptyset\ \textbf{and}\ \text{the power }
///     \text{of } \mathrm{top}( S ) > p\ \textbf{do} \cr
/// 6 & \qquad\quad [ s' , s ) \gets \mathrm{pop}( S );\ \text{merge } [ s' , s
///     ) \text{ and } [ s , e );\ s \gets s' \cr
/// 7 & \qquad \mathrm{push}( S , ( [ s , e ) , p ) );\ [ s , e ) \gets [ e ,
///     e_2 ) \cr
/// 8 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do}\ \text{merge }
///     \mathrm{pop}( S ) \text{ with } [ s , e )
/// \end{array}}
/// \f]
/// where a run is ascending, or strictly descending and reversed, and is
/// extended to \f$ \mathit{minrun} \f$ items by binary insertion if shorter.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void powerSort( std::vector< T > & A , C less = C() ,
                std::size_t * runs = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/RunSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RunSorts.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RunSorts.h ---------------------------*/
/*--------------------------------------------------------------------------*/
