/*--------------------------------------------------------------------------*/
/*--------------------------- File KnuthSorts.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The internal sorts of Knuth that the book leaves out, beyond the book:
 *
 * - binaryInsertionSort() (5.2.1) finds the place of each item by a binary
 *   search in the sorted prefix, \f$ \sum_k \lceil \log_2 k \rceil \f$
 *   comparisons, near the least possible, but still
 *   \f$ \Theta( n^2 ) \f$ moves; stable;
 * - listInsertionSort() (Algorithm 5.2.1L) inserts each item in a list of
 *   links, so that no item moves until the end, when the list is followed
 *   to rearrange the vector: \f$ \Theta( n^2 ) \f$ comparisons and only
 *   O( n ) moves of items; stable;
 * - addressCalculationSort() (5.2.1, of Isaac and Singleton) inserts each
 *   number in one of M lists by its value, list i taking the numbers in
 *   the i-th of M equal ranges between the least and the largest, as one
 *   would file cards under the letters: O( n ) expected time on numbers
 *   spread evenly, with M about n / 4; stable;
 * - radixExchangeSort() (Algorithm 5.2.2R) is the quicksort on the bits of
 *   the keys from the most significant: the part is split into the keys
 *   with the bit 0 and those with the bit 1, by exchanges as in a
 *   partition, and each half is sorted on the next bit; O( n w ) for keys of
 *   w bits, not stable;
 * - tournamentSort() (5.2.3, tree selection) plays a knockout tournament on
 *   a complete binary tree of the items, the winner (the least) goes out
 *   and only the matches on its path are played again, \f$ \lceil \log_2
 *   n \rceil \f$ comparisons each: \f$ O( n \log n ) \f$ with n extra
 *   places, the idea from which heapsort grew; stable when equal winners
 *   are broken by their places;
 * - mergeInsertionSort() of Ford and Johnson (5.3.1): the items are paired
 *   and compared, the larger ones sorted recursively, and the smaller ones
 *   inserted by binary search into the chain in the order of the Jacobsthal
 *   numbers 1 , 3 , 5 , 11 , 21 , ..., so that each insertion searches a
 *   prefix of \f$ 2^k - 1 \f$ items; it makes \f$ \sum_{k=1}^n \lceil
 *   \log_2 ( 3 k / 4 ) \rceil \f$ comparisons in the worst case, the
 *   fewest of any known method for most n (and the least possible for
 *   n <= 11 and n = 20 , 21), at the price of a bookkeeping that makes it
 *   slow in time; not stable.
 *
 * The contracts [see Contract.h]: each sort ends with the same items sorted,
 * an audit. The functions are defined in KnuthSorts.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.2.1, 5.2.2, 5.2.3 and 5.3.1
 * \see E. J. Isaac, R. C. Singleton, Sorting by address calculation,
 *      Journal of the ACM 3(3), 169-174, 1956, doi:10.1145/320831.320834
 * \see P. Hildebrandt, H. Isbitz, Radix exchange: an internal sorting
 *      method for digital computers, Journal of the ACM 6(2), 156-163,
 *      1959, doi:10.1145/320964.320972
 * \see L. R. Ford, S. M. Johnson, A tournament problem, American
 *      Mathematical Monthly 66(5), 387-389, 1959, doi:10.2307/2308750
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

#ifndef __ADS_KnuthSorts
 #define __ADS_KnuthSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the binary insertion sort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void binaryInsertionSort( std::vector< T > & A , C less = C() );

/// the list insertion sort; the moves of the items go to *moves, if given
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void listInsertionSort( std::vector< T > & A , C less = C() ,
                        std::size_t * moves = nullptr );

/// the address calculation sort of numbers into M lists, M = n / 4 + 1 if
/// M is 0
/// \post A is sorted, with the same items (an audit)
template< class T >
void addressCalculationSort( std::vector< T > & A , std::size_t M = 0 );

/// the radix exchange sort
/// \post A is sorted, with the same items (an audit)
template< RadixKey T >
void radixExchangeSort( std::vector< T > & A );

/// the tournament sort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void tournamentSort( std::vector< T > & A , C less = C() );

/// the merge insertion sort of Ford and Johnson
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void mergeInsertionSort( std::vector< T > & A , C less = C() );

/// the worst case of the comparisons of mergeInsertionSort() on n items,
/// sum of ceil( log2( 3 k / 4 ) ) for k = 1 , ... , n
inline long fordJohnsonBound( std::size_t n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/KnuthSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnuthSorts.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File KnuthSorts.h --------------------------*/
/*--------------------------------------------------------------------------*/
