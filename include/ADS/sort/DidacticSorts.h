/*--------------------------------------------------------------------------*/
/*-------------------------- File DidacticSorts.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Three sorts that are slow on purpose, beyond the book, as examples of the
 * analysis of recurrences and of the expected time:
 *
 * - stoogeSort() sorts the first two thirds, the last two thirds and the
 *   first two thirds again, \f$ T( n ) = 3 T( 2 n / 3 ) + O( 1 ) \f$,
 *   i.e., \f$ \Theta( n^{\log_{1.5} 3} ) = O( n^{2.71} ) \f$ by the
 *   master theorem [see MasterTheorem.h];
 * - slowSort() of Broder and Stolfi (multiply and surrender): the maximum of
 *   the two halves, sorted recursively, goes to the end and the rest is
 *   sorted again, \f$ T( n ) = 2 T( n / 2 ) + T( n - 1 ) + 1 \f$, which
 *   is not even polynomial;
 * - bogoSort() shuffles the vector until it is sorted, n! / ( number of
 *   sorted orders ) shuffles expected, each O( n ).
 *
 * The contracts [see Contract.h]: bogoSort() needs n <= 10, so that it ends
 * in a reasonable time; each sort ends with the same items sorted, an
 * audit. The functions are defined in DidacticSorts.tpp.
 *
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, problem 7-3 (stooge sort)
 * \see A. Broder, J. Stolfi, Pessimal algorithms and simplexity analysis,
 *      ACM SIGACT News 16(3), 49-53, 1984, doi:10.1145/990534.990536
 * \see H. Gruber, M. Holzer, O. Ruepp, Sorting the slow way: an analysis
 *      of perversely awful randomized sorting algorithms, Proceedings of FUN
 *      2007, LNCS 4475, 183-197, 2007
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

#ifndef __ADS_DidacticSorts
 #define __ADS_DidacticSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <random>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the stooge sort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void stoogeSort( std::vector< T > & A , C less = C() );

/// the slow sort of Broder and Stolfi
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void slowSort( std::vector< T > & A , C less = C() );

/// the bogo sort with the generator g; the number of shuffles
/// \pre n <= 10
/// \post A is sorted, with the same items (an audit)
template< class T , class R , class C = std::less<> >
long bogoSort( std::vector< T > & A , R & g , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/DidacticSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DidacticSorts.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DidacticSorts.h ------------------------*/
/*--------------------------------------------------------------------------*/
