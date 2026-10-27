/*--------------------------------------------------------------------------*/
/*--------------------------- File StrandSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The strand sort, beyond the book, on a list with pointers [see `List.h`]: a
 * strand, i.e., the first item and every later one not smaller than the last
 * taken, is pulled out of the list and merged with the result, until the list
 * is empty. Pulling a strand and merging it cost \f$ O( n ) \f$ each, and
 * there are as many strands as the items of a longest strictly
 * decreasing subsequence: \f$ O( n ) \f$ on a sorted list and \f$ O( n^2 ) \f$
 * on a decreasing one. It is stable, as the natural merge sort of which it is
 * a variant.
 *
 * The contracts [see `Contract.h`]: the list ends sorted, an audit. The
 * function is defined in `StrandSort.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.2.4 (natural merging)
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

#ifndef __ADS_StrandSort
 #define __ADS_StrandSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the strand sort of a list; the number of strands goes to `*strands`, if
/// given
///
/// In pseudocode (the natural merging of Knuth, section 5.2.4, beyond the
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{strandSort}( L ) \cr
/// 2 & \quad R \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ L \text{ is not empty}\ \textbf{do} \cr
/// 4 & \qquad S \gets ( \text{the head of } L \text{, removed from } L ) \cr
/// 5 & \qquad \textbf{foreach}\ x \in L \text{ from its head}\ \textbf{do} \cr
/// 6 & \qquad\quad \textbf{if}\ x \ge \text{the last item of } S
///     \ \textbf{then}\ \text{move } x \text{ to the end of } S \cr
/// 7 & \qquad R \gets \text{the merge of } R \text{ and } S \text{, } R
///     \text{ first on a tie} \cr
/// 8 & \quad L \gets R
/// \end{array}}
/// \f]
/// \post \f$ L \f$ is sorted (audit)
template< class T , class C = std::less<> >
void strandSort( List< T > & L , C less = C() ,
                 std::size_t * strands = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/StrandSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StrandSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File StrandSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
