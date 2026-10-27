/*--------------------------------------------------------------------------*/
/*--------------------------- File Smoothsort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Smoothsort of Dijkstra, beyond the book: a heapsort on a forest of
 * Leonardo heaps, whose sizes are Leonardo numbers \f$ L_0 = L_1 = 1 ,
 * L_{k+2} = L_{k+1} + L_k + 1 \f$, laid side by side in the vector with
 * the roots increasing from left to right; the forest grows by one item at
 * a time, merging the two rightmost heaps when their orders are consecutive,
 * and shrinks by removing the root of the rightmost heap, which is the
 * maximum. It sorts in place in \f$ O( n \log n ) \f$ time like heapsort,
 * but in O( n ) on a vector that is already sorted, the transition between
 * the two being smooth, whence the name; it is not stable. The forest is
 * kept as a word of bits p, the orders of its heaps relative to the order
 * pshift of the rightmost one, as in the classical implementations.
 *
 * The contracts [see Contract.h]: the vector ends with the same items
 * sorted, an audit. The function is defined in Smoothsort.tpp.
 *
 * \see E. W. Dijkstra, Smoothsort, an alternative for sorting in situ,
 *      Science of Computer Programming 1(3), 223-233, 1982,
 *      doi:10.1016/0167-6423(82)90016-8
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

#ifndef __ADS_Smoothsort
 #define __ADS_Smoothsort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// smoothsort
/// \pre \f$ n < L_{64} = 34335360355129 \f$, as the forest is a word of 64
///      bits [see Smoothsort.tpp]
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void smoothsort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Smoothsort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Smoothsort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Smoothsort.h --------------------------*/
/*--------------------------------------------------------------------------*/
