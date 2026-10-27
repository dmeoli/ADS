/*--------------------------------------------------------------------------*/
/*-------------------------- File ExternalSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of a file larger than the memory, beyond the book, as Knuth
 * describes it for tapes, here on sequences in memory that play the files:
 *
 * - `replacementSelection()` (section 5.4.1 of Knuth) makes the initial runs
 *   with a heap of \f$ M \f$ items: the least item of the current run goes out
 *   and the next input item takes its place, in the current run if it is not
 *   smaller than the one just written, in the next run otherwise; on random
 *   input the runs are about \f$ 2 M \f$ long, twice the \f$ M \f$ of sorting
 *   one memory load at a time, and a sorted input gives a single run;
 * - `polyphaseMerge()` (Algorithm 5.4.2D) merges the runs on \f$ T \f$ tapes,
 *   \f$ T - 1 \f$ of input and one of output: the runs are dealt so that the
 *   counts on the tapes are a perfect Fibonacci distribution of order
 *   \f$ T - 1 \f$, padded with empty (dummy) runs, and each phase merges runs
 *   from all the input tapes onto the output one until one input tape is
 *   empty, which becomes the next output; each item takes part in about
 *   \f$ \log_\phi r \f$ merges for \f$ r \f$ runs with three tapes, fewer with
 *   more tapes, and no tape is ever only copied;
 * - `externalSort()` is the two together.
 *
 * The contracts [see `Contract.h`]: \f$ M \ge 1 \f$ and \f$ T \ge 3 \f$; the
 * runs are sorted and hold the items of the input, and the merge gives the
 * items of the runs sorted, two audits. The functions are defined in
 * `ExternalSort.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.4.1 and 5.4.2
 * \see R. L. Gilstad, Polyphase merge sorting: an advanced technique,
 *      Proceedings of the Eastern Joint Computer Conference 18, 143-148,
 *      1960
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

#ifndef __ADS_ExternalSort
 #define __ADS_ExternalSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the runs of the input made by replacement selection with \f$ M \f$ items in
/// memory
///
/// In pseudocode (section 5.4.1 of Knuth, beyond the book), a heap of pairs
/// \f$ ( \mathit{run} , x ) \f$ ordered by the run and then by the item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{replacementSelection}( I , M ) \cr
/// 2 & \quad H \gets \text{a heap of } ( 0 , x ) \text{ for the first } M
///     \text{ items } x \text{ of } I \cr
/// 3 & \quad R \gets \text{an empty sequence of runs} \cr
/// 4 & \quad \textbf{while}\ H \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad ( r , x ) \gets \mathrm{deleteMin}( H ) \cr
/// 6 & \qquad \textbf{if}\ r = |R|\ \textbf{then}\ \text{append an empty run }
///     \text{to } R \cr
/// 7 & \qquad \textbf{if}\ I \text{ has an item } y \text{ left}
///     \ \textbf{then} \cr
/// 8 & \qquad\quad \textbf{if}\ y < x\ \textbf{then}\ \mathrm{insert}( H , ( r
///     + 1 , y ) )\ \textbf{else}\ \mathrm{insert}( H , ( r , y ) ) \cr
/// 9 & \qquad \text{append } x \text{ to the last run of } R \cr
/// 10 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \pre \f$ M \ge 1 \f$
/// \post each run sorted, all together the items of the input (audit)
template< class T , class C = std::less<> >
std::vector< std::vector< T > > replacementSelection(
                       const std::vector< T > & input , std::size_t M ,
                       C less = C() );

/// the polyphase merge of the runs on \f$ T \f$ tapes; the phases go to
/// `*phases`, if given
///
/// In pseudocode (Algorithm 5.4.2D of Knuth, beyond the book), the perfect
/// distribution of a level, \f$ a_1 \ge \cdots \ge a_{T-1} \f$, giving that of
/// the next level as \f$ a'_i = a_1 + a_{i+1} \f$, with \f$ a_T = 0 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{polyphaseMerge}( R , T ) \cr
/// 2 & \quad a \gets ( 1 , \ldots , 1 ) \cr
/// 3 & \quad \textbf{while}\ \sum_i a_i < |R|\ \textbf{do}\ a \gets
///     \text{the next level of } a \cr
/// 4 & \quad \text{deal } a_i \text{ runs to the tape } i \text{, the runs }
///     \text{of } R \text{ first, then empty ones} \cr
/// 5 & \quad o \gets T \quad \triangleright\ \text{the output tape} \cr
/// 6 & \quad \textbf{while}\ \text{more than one run is left}\ \textbf{do} \cr
/// 7 & \qquad k \gets \text{the fewest runs on an input tape} \cr
/// 8 & \qquad \textbf{repeat}\ k \text{ times}\ \text{merge the first run of }
///     \text{each input tape onto } o \cr
/// 9 & \qquad o \gets \text{an input tape left empty} \cr
/// 10 & \quad \textbf{return}\ \text{the run left}
/// \end{array}}
/// \f]
/// \pre \f$ T \ge 3 \f$, each run sorted
/// \post the items of the runs, sorted (audit)
template< class T , class C = std::less<> >
std::vector< T > polyphaseMerge( std::vector< std::vector< T > > runs ,
                                 std::size_t tapes , C less = C() ,
                                 std::size_t * phases = nullptr );

/// the sort of `input` by `replacementSelection()` and then `polyphaseMerge()`
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{externalSort}( I , M , T ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{polyphaseMerge}(
///     \mathrm{replacementSelection}( I , M ) , T )
/// \end{array}}
/// \f]
/// \pre \f$ M \ge 1 \f$ and \f$ T \ge 3 \f$
/// \post the items of the input, sorted (audit)
template< class T , class C = std::less<> >
std::vector< T > externalSort( const std::vector< T > & input ,
                               std::size_t M , std::size_t tapes ,
                               C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/ExternalSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ExternalSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ExternalSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
