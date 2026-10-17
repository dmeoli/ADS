/*--------------------------------------------------------------------------*/
/*-------------------------- File ExchangeSorts.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts by exchanges of items, beyond the book, all in place:
 *
 * - bubbleSort(), passes from left to right that swap the adjacent items
 *   out of order, each ending at the last swap of the previous one, since
 *   the items after it are already in their places: \f$ O( n^2 ) \f$
 *   comparisons, n - 1 on a sorted vector, stable;
 * - cocktailShakerSort(), the bubble sort with passes alternately to the
 *   right and to the left, so that a small item near the end (a turtle)
 *   moves to the front in one pass instead of n, stable;
 * - oddEvenSort(), the odd-even transposition sort: the pairs ( 2 i ,
 *   2 i + 1 ) and then the pairs ( 2 i + 1 , 2 i + 2 ) compared and swapped
 *   in turns, n rounds at most, each of independent comparisons, which is
 *   why it is a sort for n processors on a line, stable;
 * - combSort(), the bubble sort with a gap that shrinks by 1.3 at each
 *   pass, which moves the turtles far, ending as a bubble sort with gap 1:
 *   not stable, near \f$ O( n \log n ) \f$ in practice, though
 *   \f$ O( n^2 ) \f$ in the worst case;
 * - gnomeSort(), one position that moves right while the two items behind
 *   it are in order and swaps them and moves left otherwise: the insertion
 *   sort with swaps and no nested loop, \f$ O( n^2 ) \f$, stable.
 *
 * The contracts [see Contract.h]: each sort ends with the same items
 * sorted, an audit. The functions are defined in ExchangeSorts.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.2.2 (sorting by exchanging)
 * \see A. N. Habermann, Parallel neighbor-sort (or the glory of the
 *      induction principle), Technical Report AD-759 248, Carnegie Mellon
 *      University, 1972
 * \see W. Dobosiewicz, An efficient variation of bubble sort, Information
 *      Processing Letters 11(1), 5-6, 1980,
 *      doi:10.1016/0020-0190(80)90022-8
 * \see H. Sarbazi-Azad, Stupid sort: a new sorting algorithm, Newsletter
 *      599, Department of Computing Science, University of Glasgow, 2000
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

#ifndef __ADS_ExchangeSorts
 #define __ADS_ExchangeSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
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

/// the bubble sort, each pass ending at the last swap of the previous one
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void bubbleSort( std::vector< T > & A , C less = C() );

/// the bubble sort with passes in both directions
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void cocktailShakerSort( std::vector< T > & A , C less = C() );

/// the odd-even transposition sort; the rounds go to *rounds, if given
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void oddEvenSort( std::vector< T > & A , C less = C() ,
                  std::size_t * rounds = nullptr );

/// the comb sort, the gap shrinking by 1.3
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void combSort( std::vector< T > & A , C less = C() );

/// the gnome sort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void gnomeSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/ExchangeSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ExchangeSorts.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ExchangeSorts.h ------------------------*/
/*--------------------------------------------------------------------------*/
