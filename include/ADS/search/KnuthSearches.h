/*--------------------------------------------------------------------------*/
/*-------------------------- File KnuthSearches.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two searches of a sorted vector from Knuth, beyond the book [see
 * BinarySearch.h for the binary search of section 1.2]:
 *
 * - fibonacciSearch() (Algorithm 6.2.1F) splits the part still to examine
 *   at a Fibonacci number instead of the middle, so that the next part is
 *   found by subtractions alone, with no division: the part has length
 *   \f$ F_{k+1} - 1 \f$, the item compared is the \f$ F_k \f$-th, and the
 *   two parts left have lengths \f$ F_k - 1 \f$ and \f$ F_{k-1} - 1 \f$;
 *   at most about \f$ 1.44 \log_2 n \f$ comparisons, a little more than
 *   the binary search, for an average cost per step that was lower on the
 *   machines of the time (and may still be, where division is dear);
 * - uniformBinarySearch() (Algorithm 6.2.1U, with the table of Algorithm
 *   C) keeps the place i of the last item compared and the width
 *   \f$ \delta_j \f$ of the step j, \f$ \delta_j = \lfloor ( n +
 *   2^{j-1} ) / 2^j \rfloor \f$, precomputed: a step adds or subtracts the
 *   next width, so that the search moves on a fixed tree of differences
 *   with no endpoint to update.
 *
 * Both return the place of v, or -1; when n is not a Fibonacci number, a
 * probe of the Fibonacci search past the end of the vector is moved to its
 * last item, which is the same as imagining items larger than all the
 * others up to the next Fibonacci number; the uniform search may step to
 * the places 0 and n + 1, which stand for the keys \f$ - \infty \f$ and
 * \f$ + \infty \f$ of the book and are compared without reading the
 * vector. The contracts [see Contract.h]: the
 * vector is sorted (an audit), and the result is -1 if and only if v is not
 * there. The functions are defined in KnuthSearches.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.1
 * \see D. E. Ferguson, Fibonaccian searching, Communications of the ACM
 *      3(12), 648, 1960, doi:10.1145/367487.367496
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

#ifndef __ADS_KnuthSearches
 #define __ADS_KnuthSearches
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the Fibonacci search of v in the sorted A: its place, or -1; the
/// comparisons go to *cmp, if given
/// \pre A is sorted (an audit)
/// \post the result is -1 if and only if v is not in A
template< class T >
long fibonacciSearch( const std::vector< T > & A , const T & v ,
                      long * cmp = nullptr );

/// the uniform binary search of v in the sorted A: its place, or -1; the
/// comparisons go to *cmp, if given
/// \pre A is sorted (an audit)
/// \post the result is -1 if and only if v is not in A
template< class T >
long uniformBinarySearch( const std::vector< T > & A , const T & v ,
                          long * cmp = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/KnuthSearches.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnuthSearches.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File KnuthSearches.h ------------------------*/
/*--------------------------------------------------------------------------*/
