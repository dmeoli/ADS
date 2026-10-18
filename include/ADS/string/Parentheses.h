/*--------------------------------------------------------------------------*/
/*--------------------------- File Parentheses.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The longest balanced strings of parentheses inside a text of n
 * characters, where open and close are the two parentheses and every other
 * character is neither.
 *
 * - longestBalancedSubsequence(): the longest subsequence that is
 *   balanced. A scan matches each close with the nearest open still
 *   unmatched before it, which never loses a pair (an exchange argument),
 *   so the length is twice the number of pairs matched: \f$ \Theta( n ) \f$
 *   time and O( 1 ) space. The interval recursion of the textbook, the
 *   pair of the two ends or the best split, gives the same in
 *   \f$ \Theta( n^3 ) \f$ time.
 * - longestBalancedSubstring(): the longest contiguous balanced substring.
 *   A stack keeps the places of the opens not matched yet; when a close
 *   matches the top, the substring from the next open still unmatched (or
 *   from the last place where the balance broke) to the close is
 *   balanced, and it is the longest one ending there: \f$ \Theta( n ) \f$
 *   time and O( n ) space. The two scans of the counts of opens and closes,
 *   left to right and right to left, need only O( 1 ) space.
 *
 * The contracts [see Contract.h]: the substring found is balanced. The
 * functions are defined in Parentheses.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

/// the length of a longest subsequence of s that is a balanced string of
/// the parentheses open and close
/// \pre open != close
[[nodiscard]] inline std::size_t longestBalancedSubsequence(
                     std::string_view s , char open = '(' , char close = ')' );

/// where the leftmost of the longest balanced substrings of s starts and
/// its length, ( 0 , 0 ) if there is none
/// \pre open != close
/// \post s[ start .. start + length ) is balanced
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
