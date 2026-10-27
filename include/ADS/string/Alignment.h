/*--------------------------------------------------------------------------*/
/*---------------------------- File Alignment.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The alignments of two strings \f$ P \f$ and \f$ T \f$ of exercises 13.7
 * to 13.9 of the book: spaces are inserted in \f$ P \f$ or \f$ T \f$, never
 * one in front of the other, and each column scores \f$ +1 \f$ if the
 * characters are equal, \f$ -1 \f$ if they differ and \f$ -2 \f$ if one is
 * a space. The best score \f$ \mathit{DP}[ i ][ j ] \f$ of the prefixes
 * \f$ P( i ) \f$ and \f$ T( j ) \f$ is
 * \f[
 *   \max\{ \mathit{DP}[ i ][ j - 1 ] - 2 , \mathit{DP}[ i - 1 ][ j ] - 2 ,
 *          \mathit{DP}[ i - 1 ][ j - 1 ] + \delta_{ij} \}
 * \f]
 * with \f$ \delta_{ij} = \pm 1 \f$, \f$ \Theta( m n ) \f$ time:
 *
 * - global, \f$ \mathit{DP}[ i ][ 0 ] = -2 i \f$ and
 *   \f$ \mathit{DP}[ 0 ][ j ] = -2 j \f$, the score in
 *   \f$ \mathit{DP}[ m ][ n ] \f$;
 * - semiglobal, the spaces before the start and after the end of either
 *   string free: the first row and column are 0, the score the largest of
 *   the last row and column;
 * - local, the best alignment of two substrings: 0 is a fourth term of the
 *   maximum, and the score the largest entry.
 *
 * The contracts [see `Contract.h`]: the two strings of a global alignment
 * have the same length, at least that of the longer string. The operations
 * are defined in `Alignment.tpp`.
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
enum class Alignment {
 GLOBAL ,       ///< the whole strings, as Needleman and Wunsch
 SEMIGLOBAL ,   ///< the spaces at the two ends of either string free
 LOCAL          ///< two substrings, as Smith and Waterman
 };

/// the best score of an alignment of \f$ P \f$ and \f$ T \f$
///
/// In pseudocode (exercises 13.7 to 13.9), \f$ \delta_{ij} = +1 \f$ if
/// \f$ P[ i - 1 ] = T[ j - 1 ] \f$ and \f$ -1 \f$ otherwise:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{alignmentScore}( P , T , \mathit{kind} )
///     \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m\ \textbf{do}
///     \ \mathit{DP}[ i ][ 0 ] \gets -2 i \text{ if } \mathit{kind} =
///     \text{GLOBAL, else 0} \cr
/// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n\ \textbf{do}
///     \ \mathit{DP}[ 0 ][ j ] \gets -2 j \text{ if } \mathit{kind} =
///     \text{GLOBAL, else 0} \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 5 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 6 & \qquad\quad \mathit{DP}[ i ][ j ] \gets \max\{ \mathit{DP}[ i ][ j -
///     1 ] - 2 , \mathit{DP}[ i - 1 ][ j ] - 2 , \mathit{DP}[ i - 1 ][ j - 1
///     ] + \delta_{ij} \} \cr
/// 7 & \qquad\quad \textbf{if}\ \mathit{kind} = \text{LOCAL}\ \textbf{then}
///     \ \mathit{DP}[ i ][ j ] \gets \max\{ \mathit{DP}[ i ][ j ] , 0 \} \cr
/// 8 & \quad \textbf{if}\ \mathit{kind} = \text{GLOBAL}\ \textbf{then return}
///     \ \mathit{DP}[ m ][ n ] \cr
/// 9 & \quad \textbf{if}\ \mathit{kind} = \text{LOCAL}\ \textbf{then return}
///     \ \text{the largest entry of } \mathit{DP} \cr
/// 10 & \quad \textbf{return}\ \text{the largest entry of the last row and }
///     \text{of the last column}
/// \end{array}}
/// \f]
template< class S >
int alignmentScore( const S & P , const S & T , Alignment kind );

/// a best global alignment, the two strings with the spaces as `gap`
///
/// In pseudocode (exercises 13.7 to 13.9), the table of the global
/// alignment of `alignmentScore()` read back from \f$ ( m , n ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{globalAlignment}( P , T ) \cr
/// 2 & \quad \mathit{DP} \gets \text{the table of the global alignment};\ i
///     \gets m;\ j \gets n;\ A \gets (\,);\ B \gets (\,) \cr
/// 3 & \quad \textbf{while}\ i > 0\ \textbf{or}\ j > 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ i > 0\ \textbf{and}\ j > 0\ \textbf{and}
///     \ \mathit{DP}[ i ][ j ] = \mathit{DP}[ i - 1 ][ j - 1 ] + \delta_{ij}
///     \ \textbf{then} \cr
/// 5 & \qquad\quad \text{prepend } P[ i - 1 ] \text{ to } A \text{ and }
///     T[ j - 1 ] \text{ to } B;\ i \gets i - 1;\ j \gets j - 1 \cr
/// 6 & \qquad \textbf{else if}\ i > 0\ \textbf{and}\ \mathit{DP}[ i ][ j ] =
///     \mathit{DP}[ i - 1 ][ j ] - 2\ \textbf{then} \cr
/// 7 & \qquad\quad \text{prepend } P[ i - 1 ] \text{ to } A \text{ and a }
///     \text{space to } B;\ i \gets i - 1 \cr
/// 8 & \qquad \textbf{else}\ \text{prepend a space to } A \text{ and }
///     T[ j - 1 ] \text{ to } B;\ j \gets j - 1 \cr
/// 9 & \quad \textbf{return}\ ( A , B )
/// \end{array}}
/// \f]
/// \post the two strings have the same length, at least
///       \f$ \max\{ m , n \} \f$
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
