/*--------------------------------------------------------------------------*/
/*-------------------------- File ExternalSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of a file larger than the memory, beyond the book, as Knuth
 * describes it for tapes, here on sequences in memory that play the files:
 *
 * - replacementSelection() (5.4.1) makes the initial runs with a heap of M
 *   items: the least item of the current run goes out and the next input
 *   item takes its place, in the current run if it is not smaller than the
 *   one just written, in the next run otherwise; on random input the runs
 *   are about 2 M long, twice the M of sorting each memoryful, and a sorted
 *   input gives a single run;
 * - polyphaseMerge() (Algorithm 5.4.2D) merges the runs on T tapes, T - 1
 *   of input and one of output: the runs are dealt so that the counts on
 *   the tapes are a perfect Fibonacci distribution of order T - 1, padded
 *   with empty (dummy) runs, and each phase merges runs from all the input
 *   tapes onto the output one until one input tape is empty, which becomes
 *   the next output; each item takes part in about
 *   \f$ \log_\phi r \f$ merges for r runs with three tapes, fewer
 *   with more tapes, and no tape is ever only copied;
 * - externalSort() is the two together.
 *
 * The contracts [see Contract.h]: M >= 1 and T >= 3; the runs are sorted
 * and hold the items of the input, and the merge gives the items of the runs
 * sorted, two audits. The functions are defined in ExternalSort.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.4.1 and 5.4.2
 * \see R. L. Gilstad, Polyphase merge sorting: an advanced technique,
 *      Proceedings of the Eastern Joint Computer Conference 18, 143-148,
 *      1960
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

/// the runs of the input made by replacement selection with M items in
/// memory
/// \pre M >= 1
/// \post each run sorted, all together the items of the input (an audit)
template< class T , class C = std::less<> >
std::vector< std::vector< T > > replacementSelection(
                       const std::vector< T > & input , std::size_t M ,
                       C less = C() );

/// the polyphase merge of the runs on T tapes; the phases go to *phases,
/// if given
/// \pre T >= 3, each run sorted
/// \post the items of the runs, sorted (an audit)
template< class T , class C = std::less<> >
std::vector< T > polyphaseMerge( std::vector< std::vector< T > > runs ,
                                 std::size_t tapes , C less = C() ,
                                 std::size_t * phases = nullptr );

/// replacementSelection() and then polyphaseMerge()
/// \pre M >= 1 and T >= 3
/// \post the items of the input, sorted (an audit)
template< class T , class C = std::less<> >
std::vector< T > externalSort( const std::vector< T > & input ,
                               std::size_t M , std::size_t tapes ,
                               C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/ExternalSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ExternalSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ExternalSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
