/*--------------------------------------------------------------------------*/
/*---------------------------- File Alignment.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The alignments of two strings P and T of exercises 13.7-13.9: spaces are
 * inserted in P or T, never one in front of the other, and each column
 * scores +1 if the characters are equal, -1 if they differ and -2 if one
 * is a space. DP[ i ][ j ], the best score of P( i ) and T( j ), is
 * \f[
 *   \max\{ DP[ i ][ j - 1 ] - 2 , DP[ i - 1 ][ j ] - 2 ,
 *          DP[ i - 1 ][ j - 1 ] + \delta_{ij} \}
 * \f]
 * with \f$ \delta_{ij} = \pm 1 \f$, \f$ \Theta( m n ) \f$ time:
 *
 * - global, DP[ i ][ 0 ] = -2 i and DP[ 0 ][ j ] = -2 j, the score in
 *   DP[ m ][ n ];
 * - semiglobal, the spaces before the start and after the end of either
 *   string free: the first row and column are 0, the score the largest of
 *   the last row and column;
 * - local, the best alignment of two substrings: 0 is a fourth term of the
 *   maximum, and the score the largest entry.
 *
 * The contracts [see Contract.h]: the two strings of a global alignment have
 * the same length, at least that of the longer string. The operations are
 * defined in Alignment.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercises 13.7-13.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see S. B. Needleman, C. D. Wunsch, A general method applicable to the
 *      search for similarities in the amino acid sequence of two proteins,
 *      Journal of Molecular Biology 48(3), 443-453, 1970,
 *      doi:10.1016/0022-2836(70)90057-4
 * \see T. F. Smith, M. S. Waterman, Identification of common molecular
 *      subsequences, Journal of Molecular Biology 147(1), 195-197, 1981,
 *      doi:10.1016/0022-2836(81)90087-5
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

#ifndef __ADS_Alignment
 #define __ADS_Alignment
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

/// the kind of alignment
enum class Alignment { GLOBAL , SEMIGLOBAL , LOCAL };

/// the best score of an alignment of P and T
template< class S >
int alignmentScore( const S & P , const S & T , Alignment kind );

/// a best global alignment, the two strings with the spaces as gap
/// \post the two strings have the same length
template< class S >
std::pair< S , S > globalAlignment( const S & P , const S & T ,
                                    typename S::value_type gap = ' ' );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Alignment.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Alignment.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Alignment.h --------------------------*/
/*--------------------------------------------------------------------------*/
