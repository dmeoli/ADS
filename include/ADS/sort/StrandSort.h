/*--------------------------------------------------------------------------*/
/*--------------------------- File StrandSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The strand sort, beyond the book, on a list with pointers [see List.h]: a
 * strand, i.e., the first item and every later one not smaller than the last
 * taken, is pulled out of the list and merged with the result, until the list
 * is empty. Pulling a strand and merging it cost \f$ O( n ) \f$, and there are
 * as many strands as the decreasing runs suggest: \f$ O( n ) \f$ on a sorted
 * list and \f$ O( n^2 ) \f$ on a decreasing one. It is stable, and moves cells
 * instead of items, as the natural merge sort of which it is a variant.
 *
 * The contracts [see Contract.h]: the list ends sorted, an audit. The
 * function is defined in StrandSort.tpp.
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

/// the strand sort of a list; the number of strands goes to *strands, if
/// given
/// \post L is sorted (an audit)
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
