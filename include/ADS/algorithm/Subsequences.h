/*--------------------------------------------------------------------------*/
/*-------------------------- File Subsequences.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems on a sequence solved by the programming of chapter 13:
 *
 * - the longest increasing subsequence (exercise 13.6): L[ i ], the length
 *   of the longest one that ends in A[ i ], is 1 plus the largest L[ j ],
 *   j < i, with A[ j ] < A[ i ], O( n^2 ); lisFast() keeps, for each
 *   length l, the least item that ends an increasing subsequence of
 *   length l, an increasing vector, and puts each item in place by a
 *   binary search, O( n log n );
 * - the subvector of largest sum (exercises 12.2 and 13.12): t_i, the best
 *   sum of a subvector ending in A[ i ], is \f$ \max\{ t_{i-1} + A[ i ] , 0
 *   \} \f$, the empty subvector of sum 0 allowed, O( n ) (the scan of
 *   Kadane).
 *
 * The contracts [see Contract.h]: lis() gives a strictly increasing
 * subsequence of A, as long as lisLength() says, an audit; maxSubarraySum() is
 * never negative, the empty subvector counting 0. The functions are defined in
 * Subsequences.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercises 13.6 and
 *      13.12, https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see M. L. Fredman, On computing the length of longest increasing
 *      subsequences, Discrete Mathematics 11(1), 29-35, 1975,
 *      doi:10.1016/0012-365X(75)90103-X
 * \see J. Bentley, Programming pearls: algorithm design techniques,
 *      Communications of the ACM 27(9), 865-873, 1984,
 *      doi:10.1145/358234.381162
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

#ifndef __ADS_Subsequences
 #define __ADS_Subsequences
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

/// a longest increasing subsequence, O( n^2 )
/// \post a strictly increasing subsequence of A of the largest length
template< class T >
std::vector< T > lis( const std::vector< T > & A );

/// the length of a longest increasing subsequence, O( n log n )
template< class T >
std::size_t lisLength( const std::vector< T > & A );

/// the largest sum of a subvector, the empty one included, O( n )
/// \post the result is not negative
template< class T >
T maxSubarraySum( const std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/Subsequences.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Subsequences.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Subsequences.h -------------------------*/
/*--------------------------------------------------------------------------*/
