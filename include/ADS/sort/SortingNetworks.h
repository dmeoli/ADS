/*--------------------------------------------------------------------------*/
/*------------------------- File SortingNetworks.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Sorting networks, beyond the book: sequences of comparators
 * \f$ ( i , j ) \f$, \f$ i \ne j \f$, each putting the smaller of
 * \f$ A[ i ] \f$ and \f$ A[ j ] \f$ in \f$ A[ i ] \f$, fixed in advance
 * whatever the input, so that the comparators of a round touch disjoint places
 * and run in parallel (in hardware, on SIMD lanes, on a GPU):
 *
 * - `mergeExchangeNetwork()` is the merge exchange of Batcher (Algorithm M of
 *   Knuth) for any \f$ n \f$, \f$ O( n \log^2 n ) \f$ comparators in
 *   \f$ O( \log^2 n ) \f$ rounds;
 * - `bitonicNetwork()` is the bitonic sorter of Batcher for \f$ n \f$ a power
 *   of 2: two halves sorted in opposite directions form a bitonic sequence,
 *   which a half-cleaner splits into two bitonic halves, every item of the
 *   first not larger than every item of the second; also
 *   \f$ O( n \log^2 n ) \f$ comparators;
 * - `applyNetwork()` runs a network on a vector, and the 0-1 principle (a
 *   network sorts every input if it sorts every input of 0 and 1) is what the
 *   tests check.
 *
 * The contracts [see `Contract.h`]: the comparators of a network join two
 * distinct places within \f$ n \f$, a bitonic network needs \f$ n \f$ a power
 * of 2, and `applyNetwork()` a vector of at least \f$ n \f$ items. The
 * functions are defined in `SortingNetworks.tpp`.
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

/// a comparator \f$ ( i , j ) \f$ of a network: the smaller of \f$ A[ i ] \f$
/// and \f$ A[ j ] \f$ goes to \f$ A[ i ] \f$
using Comparator = std::pair< std::size_t , std::size_t >;

/// the merge exchange of Batcher for \f$ n \f$ items
///
/// In pseudocode (Algorithm M of Knuth, section 5.2.2, beyond the book),
/// \f$ t = \lceil \log_2 n \rceil \f$ and \f$ \& \f$ the and of the bits:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mergeExchangeNetwork}( n ) \cr
/// 2 & \quad N \gets \text{an empty sequence};\ \textbf{if}\ n < 2
///     \ \textbf{then return}\ N \cr
/// 3 & \quad \textbf{for}\ p \gets 2^{t-1} , 2^{t-2} , \ldots , 1\ \textbf{do}
///     \cr
/// 4 & \qquad q \gets 2^{t-1};\ r \gets 0;\ d \gets p \cr
/// 5 & \qquad \textbf{loop} \cr
/// 6 & \qquad\quad \textbf{foreach}\ i \text{ with } i + d < n \text{ and } i
///     \mathbin{\&} p = r\ \textbf{do}\ \text{append } ( i , i + d ) \text{ }
///     \text{to } N \cr
/// 7 & \qquad\quad \textbf{if}\ q = p\ \textbf{then}\ \textbf{break} \cr
/// 8 & \qquad\quad d \gets q - p;\ q \gets q / 2;\ r \gets p \cr
/// 9 & \quad \textbf{return}\ N
/// \end{array}}
/// \f]
/// \post each comparator \f$ ( i , j ) \f$ has \f$ i < j < n \f$
inline std::vector< Comparator > mergeExchangeNetwork( std::size_t n );

/// the bitonic sorter for \f$ n \f$ items
///
/// In pseudocode (Batcher, beyond the book), \f$ \oplus \f$ the exclusive or
/// of the bits and \f$ \& \f$ their and, the runs of \f$ k \f$ items made
/// sorted in alternate directions by the half-cleaners of distances
/// \f$ k / 2 , k / 4 , \ldots , 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bitonicNetwork}( n ) \cr
/// 2 & \quad N \gets \text{an empty sequence} \cr
/// 3 & \quad \textbf{for}\ k \gets 2 , 4 , \ldots , n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ j \gets k / 2 , k / 4 , \ldots , 1\ \textbf{do}
///     \cr
/// 5 & \qquad\quad \textbf{foreach}\ i < n \text{ with } l = i \oplus j > i
///     \ \textbf{do} \cr
/// 6 & \qquad\qquad \textbf{if}\ i \mathbin{\&} k = 0\ \textbf{then}
///     \ \text{append } ( i , l )\ \textbf{else}\ \text{append } ( l , i ) \cr
/// 7 & \quad \textbf{return}\ N
/// \end{array}}
/// \f]
/// \pre \f$ n \f$ is a power of 2, or 0
/// \post each comparator joins two distinct places below \f$ n \f$
inline std::vector< Comparator > bitonicNetwork( std::size_t n );

/// the comparators of \f$ N \f$ applied to \f$ A \f$ in their order
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{applyNetwork}( A , N ) \cr
/// 2 & \quad \textbf{foreach}\ ( i , j ) \in N \text{ in order}\ \textbf{do}
///     \cr
/// 3 & \qquad \textbf{if}\ A[ j ] < A[ i ]\ \textbf{then}\ A[ i ]
///     \leftrightarrow A[ j ]
/// \end{array}}
/// \f]
/// \pre each comparator is within \f$ A \f$
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
