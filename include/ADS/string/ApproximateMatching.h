/*--------------------------------------------------------------------------*/
/*----------------------- File ApproximateMatching.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The approximate string matching of section 13.1: an occurrence of the
 * pattern P = p_0 ... p_{m-1} in the text T = t_0 ... t_{n-1} with k
 * errors, a substitution, an insertion (a character of P not in T) or a
 * deletion (a character of T not in P) each. DP[ i ][ j ], the least k of
 * an occurrence of P( i ) ending at T( j ), is
 * \f[
 *   DP[ i ][ j ] = \left\{ \begin{array}{ll}
 *     0 & i = 0 \\ i & j = 0 \\
 *     \min\{ DP[ i - 1 ][ j - 1 ] + \delta , DP[ i - 1 ][ j ] + 1 ,
 *            DP[ i ][ j - 1 ] + 1 \} & \mbox{otherwise}
 *   \end{array} \right.
 * \f]
 * with \f$ \delta = 0 \f$ if p_{i-1} = t_{j-1} and 1 otherwise,
 * \f$ \Theta( m n ) \f$ time; the best occurrences end where the last row is
 * smallest. occurrence() walks back from DP[ m ][ j ] to where the occurrence
 * starts (exercise 13.1); stringMatchingSingleRow() keeps two rows only,
 * \f$ O( n ) \f$ space (section 13.4). editDistance(), the distance of
 * Levenshtein, has DP[ 0 ][ j ] = j, since T( j ) has j characters more than
 * the empty P, and its result in DP[ m ][ n ].
 *
 * The contracts [see Contract.h]: occurrence() needs the table of P and T and
 * a column of it; the best cost of stringMatching() is between 0 and m, the
 * edit distance between | m - n | and max( m , n ). The operations are defined
 * in ApproximateMatching.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 13.1 and 13.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see V. I. Levenshtein, Binary codes capable of correcting deletions,
 *      insertions, and reversals, Soviet Physics Doklady 10(8), 707-710,
 *      1966
 * \see P. H. Sellers, The theory and computation of evolutionary
 *      distances: pattern recognition, Journal of Algorithms 1(4),
 *      359-373, 1980, doi:10.1016/0196-6774(80)90016-4
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

#ifndef __ADS_ApproximateMatching
 #define __ADS_ApproximateMatching
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include "ADS/Contract.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Table = std::vector< std::vector< int > >;   ///< DP[ i ][ j ]

/// the table DP of P and T, of m + 1 rows and n + 1 columns; with edit the
/// first row is DP[ 0 ][ j ] = j, the edit distance
template< class S >
Table matchingTable( const S & P , const S & T , bool edit = false );

/// stringMatching(): the end of an occurrence of P in T with the fewest
/// errors, the first one, and their number; ( -1 , m ) if T is empty
/// \post the cost is in [ 0 , m ]
template< class S >
std::pair< long , int > stringMatching( const S & P , const S & T );

/// the occurrence of P that ends at T[ j - 1 ] with DP[ m ][ j ] errors,
/// walking back on the table: the place where it starts in T
/// \pre DP is matchingTable( P , T ) and j <= T.size()
template< class S >
std::size_t occurrence( const Table & DP , const S & P , const S & T ,
                        std::size_t j );

/// the least number of errors, with two rows of the table only
template< class S >
int stringMatchingSingleRow( const S & P , const S & T );

/// the edit distance of P and T, \f$ O( \min( m , n ) ) \f$ space
/// \post the distance is in [ | m - n | , max( m , n ) ]
template< class S >
int editDistance( const S & P , const S & T );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/ApproximateMatching.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ApproximateMatching.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File ApproximateMatching.h ---------------------*/
/*--------------------------------------------------------------------------*/
