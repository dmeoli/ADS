/*--------------------------------------------------------------------------*/
/*---------------------------- File WordBreak.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Is it Italian? (exercise 13.13): a string \f$ s[ 0 .. n - 1 ] \f$ with no
 * spaces is a sequence of words of a dictionary if one of its prefixes
 * \f$ s[ 0 .. j ) \f$ is and \f$ s[ j .. i ) \f$ is a word, i.e.,
 * \f$ \mathit{DP}[ i ] \f$ is true if and only if there is \f$ j < i \f$
 * with \f$ \mathit{DP}[ j ] \f$ true and \f$ s[ j .. i ) \f$ in the
 * dictionary, and \f$ \mathit{DP}[ 0 ] \f$ is true; \f$ O( n^2 ) \f$
 * lookups, or \f$ O( n w ) \f$ if no word is longer than \f$ w \f$. The
 * words are read back from the table.
 *
 * The contracts [see `Contract.h`]: the words found, joined, give back the
 * string. The operations are defined in `WordBreak.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 13.13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#ifndef __ADS_WordBreak
 #define __ADS_WordBreak
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include "ADS/Contract.h"

#include <numeric>
#include <optional>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the words of \f$ s \f$, if it is a sequence of words, by the
/// `lookup( word )` of a dictionary; the lookups made go to `*lookups`, if
/// given
///
/// In pseudocode (exercise 13.13), \f$ \mathit{from}[ i ] \f$ the start of
/// the last word of \f$ s[ 0 .. i ) \f$, \f$ -1 \f$ if it is no sequence of
/// words, and \f$ w \f$ the length of the longest word:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{wordBreak}( s , \mathrm{lookup} , w ) \cr
/// 2 & \quad \mathit{from}[ 0 ] \gets 0;\ \mathit{from}[ i ] \gets -1
///     \text{ for } i > 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ j = i - 1 , \ldots , \max\{ 0 , i - w \}
///     \ \textbf{with}\ \mathit{from}[ j ] \ge 0\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ \mathrm{lookup}( s[ j .. i ) )
///     \ \textbf{then}\ \mathit{from}[ i ] \gets j;\ \textbf{break} \cr
/// 6 & \quad \textbf{if}\ \mathit{from}[ n ] < 0\ \textbf{then return}
///     \ \textbf{nil} \cr
/// 7 & \quad \textbf{return}\ \text{the words read back from } n \text{ on }
///     \mathit{from}
/// \end{array}}
/// \f]
/// \post the words joined give \f$ s \f$
template< class F >
std::optional< std::vector< std::string > > wordBreak(
                         const std::string & s , F && lookup ,
                         std::size_t longest = std::string::npos ,
                         long * lookups = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/WordBreak.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WordBreak.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File WordBreak.h --------------------------*/
/*--------------------------------------------------------------------------*/
