/*--------------------------------------------------------------------------*/
/*----------------------------- File Sorted.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The checks of the contracts of the sorts [see Contract.h]: a copy of the
 * input kept only when the audits are on (ADS_CONTRACTS >= 3), and the
 * check that the output is sorted and holds the same items as the input,
 * which sorts a copy of the input with std::sort and compares the two item
 * by item through the equivalence of the order, in \f$ O( n \log n ) \f$
 * time instead of the \f$ O( n^2 ) \f$ of std::is_permutation with a
 * predicate; and the check that a sequence is sorted. These checks call the
 * comparator of the caller, so that checkingContract() tells a comparator
 * that counts its calls, as those of the tests of the book, which calls
 * come from a contract. The functions are defined in Sorted.tpp.
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

#ifndef __ADS_Sorted
 #define __ADS_Sorted
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

/// true while a contract of ADS is evaluated: a comparator that counts its
/// calls skips those made by the contracts
[[nodiscard]] bool checkingContract( void );

namespace detail {

/// the condition f() evaluated with checkingContract() true
template< class F >
bool contract( F && f );

/// a copy of A[ lo .. hi ) if the audits are on, an empty vector otherwise
template< class T >
std::vector< T > auditCopy( const std::vector< T > & A , std::size_t lo = 0 ,
                            std::size_t hi = std::size_t( -1 ) );

/// A[ lo .. hi ) is sorted by less and holds the items of before, the copy
/// of the same range made by auditCopy() before the sort
template< class T , class C >
bool sortedPermutation( const std::vector< T > & A ,
                        std::vector< T > before , C less ,
                        std::size_t lo = 0 ,
                        std::size_t hi = std::size_t( -1 ) );

/// A[ lo .. hi ) holds the items of before, sorted or not
template< class T , class C >
bool samePermutation( const std::vector< T > & A ,
                      std::vector< T > before , C less ,
                      std::size_t lo = 0 ,
                      std::size_t hi = std::size_t( -1 ) );

/// the items of the SEQUENCE L [see ADT.h] are sorted by less
template< class S , class C >
bool sequenceSorted( const S & L , C less );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Sorted.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Sorted.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sorted.h ----------------------------*/
/*--------------------------------------------------------------------------*/
