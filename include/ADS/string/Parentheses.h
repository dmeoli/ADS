/*--------------------------------------------------------------------------*/
/*--------------------------- File Parentheses.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The longest balanced strings of parentheses inside a text of \f$ n \f$
 * characters, where `open` and `close` are the two parentheses and every
 * other character is neither.
 *
 * - `longestBalancedSubsequence()`: the longest subsequence that is
 *   balanced. A scan matches each close with the nearest open still
 *   unmatched before it, which never loses a pair (an exchange argument),
 *   so the length is twice the number of pairs matched: \f$ \Theta( n ) \f$
 *   time and \f$ O( 1 ) \f$ space. The interval recursion of the textbook,
 *   the pair of the two ends or the best split, gives the same in
 *   \f$ \Theta( n^3 ) \f$ time.
 * - `longestBalancedSubstring()`: the longest contiguous balanced
 *   substring. A stack keeps the places of the opens not matched yet; when
 *   a close matches the top, the substring from the next open still
 *   unmatched (or from the last place where the balance broke) to the close
 *   is balanced, and it is the longest one ending there:
 *   \f$ \Theta( n ) \f$ time and \f$ O( n ) \f$ space. The two scans of the
 *   counts of opens and closes, left to right and right to left, need only
 *   \f$ O( 1 ) \f$ space.
 *
 * The contracts [see `Contract.h`]: the substring found is balanced. The
 * functions are defined in `Parentheses.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
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

#ifndef __ADS_Parentheses
 #define __ADS_Parentheses
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the length of a longest subsequence of \f$ s \f$ that is a balanced
/// string of the parentheses `open` and `close`
///
/// In pseudocode (chapter 13, by a greedy scan instead of the recursion on
/// the intervals), each close matched with the nearest open still pending:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestBalancedSubsequence}( s ) \cr
/// 2 & \quad \mathit{pairs} \gets 0;\ \mathit{pending} \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ c \in s\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ c = \mathit{open}\ \textbf{then}\ \mathit{pending}
///     \gets \mathit{pending} + 1 \cr
/// 5 & \qquad \textbf{else if}\ c = \mathit{close}\ \textbf{and}
///     \ \mathit{pending} > 0\ \textbf{then} \cr
/// 6 & \qquad\quad \mathit{pending} \gets \mathit{pending} - 1;
///     \ \mathit{pairs} \gets \mathit{pairs} + 1 \cr
/// 7 & \quad \textbf{return}\ 2 \cdot \mathit{pairs}
/// \end{array}}
/// \f]
/// \pre `open` differs from `close`
[[nodiscard]] inline std::size_t longestBalancedSubsequence(
                     std::string_view s , char open = '(' , char close = ')' );

/// where the leftmost of the longest balanced substrings of \f$ s \f$
/// starts and its length, \f$ ( 0 , 0 ) \f$ if there is none
///
/// In pseudocode (chapter 13, by a stack), \f$ O \f$ the places of the
/// opens not matched yet and \f$ \mathit{base} \f$ the start of the current
/// balanced run:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestBalancedSubstring}( s ) \cr
/// 2 & \quad O \gets \text{an empty stack};\ \mathit{base} \gets 0;\ (
///     \mathit{start} , \ell ) \gets ( 0 , 0 ) \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ s[ i ] = \mathit{open}\ \textbf{then}
///     \ \mathrm{push}( O , i ) \cr
/// 5 & \qquad \textbf{else if}\ s[ i ] = \mathit{close}\ \textbf{and}\ O \ne
///     \emptyset\ \textbf{then} \cr
/// 6 & \qquad\quad \mathrm{pop}( O ) \cr
/// 7 & \qquad\quad a \gets \mathit{base} \text{ if } O = \emptyset
///     \text{, else } \mathrm{top}( O ) + 1 \cr
/// 8 & \qquad\quad \textbf{if}\ i + 1 - a > \ell\ \textbf{then}\ (
///     \mathit{start} , \ell ) \gets ( a , i + 1 - a ) \cr
/// 9 & \qquad \textbf{else}\ O \gets \emptyset;\ \mathit{base} \gets i + 1 \cr
/// 10 & \quad \textbf{return}\ ( \mathit{start} , \ell )
/// \end{array}}
/// \f]
/// \pre `open` differs from `close`
/// \post \f$ s[ \mathit{start} .. \mathit{start} + \mathit{length} ) \f$
///       is balanced
[[nodiscard]] inline std::pair< std::size_t , std::size_t >
longestBalancedSubstring( std::string_view s , char open = '(' ,
                          char close = ')' );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Parentheses.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Parentheses.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Parentheses.h -------------------------*/
/*--------------------------------------------------------------------------*/
