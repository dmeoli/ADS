/*--------------------------------------------------------------------------*/
/*------------------------- File SortingNetworks.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Sorting networks, beyond the book: sequences of comparators ( i , j ),
 * i != j, each putting the smaller of A[ i ] and A[ j ] in A[ i ], fixed in
 * advance whatever the input, so that the comparators of a round touch
 * disjoint places and run in parallel (in hardware, on SIMD lanes, on a
 * GPU):
 *
 * - mergeExchangeNetwork() is the merge exchange of Batcher (Algorithm M of
 *   Knuth) for any n, \f$ O( n \log^2 n ) \f$ comparators in
 *   \f$ O( \log^2 n ) \f$ rounds;
 * - bitonicNetwork() is the bitonic sorter of Batcher for n a power of 2:
 *   two halves sorted in opposite directions form a bitonic sequence, which
 *   a half-cleaner splits into two bitonic halves, every item of the first
 *   not larger than every item of the second; also
 *   \f$ O( n \log^2 n ) \f$ comparators;
 * - applyNetwork() runs a network on a vector, and the 0-1 principle (a
 *   network sorts every input if it sorts every input of 0 and 1) is what
 *   the tests check.
 *
 * The contracts [see Contract.h]: the comparators of a network join two
 * distinct places within n,
 * a bitonic network needs n a power of 2, and applyNetwork() a vector of at
 * least n items. The functions are defined in SortingNetworks.tpp.
 *
 * \see K. E. Batcher, Sorting networks and their applications, Proceedings
 *      of the AFIPS Spring Joint Computer Conference 32, 307-314, 1968,
 *      doi:10.1145/1468075.1468121
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.2.2 (Algorithm M) and 5.3.4
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

#ifndef __ADS_SortingNetworks
 #define __ADS_SortingNetworks
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a comparator ( i , j ) of a network: the smaller of A[ i ] and A[ j ]
/// goes to A[ i ]
using Comparator = std::pair< std::size_t , std::size_t >;

/// the merge exchange of Batcher for n items
/// \post each comparator ( i , j ) has i < j < n
inline std::vector< Comparator > mergeExchangeNetwork( std::size_t n );

/// the bitonic sorter for n items
/// \pre n is a power of 2, or 0
/// \post each comparator joins two distinct places below n
inline std::vector< Comparator > bitonicNetwork( std::size_t n );

/// the comparators of N applied to A in their order
/// \pre each comparator is within A
template< class T , class C = std::less<> >
void applyNetwork( std::vector< T > & A , const std::vector< Comparator > & N ,
                   C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/SortingNetworks.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SortingNetworks.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SortingNetworks.h -----------------------*/
/*--------------------------------------------------------------------------*/
