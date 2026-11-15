/*--------------------------------------------------------------------------*/
/*---------------------------- File Anagrams.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The anagrams (exercise 16.8): the strings made of all the characters of a
 * given one, in any order, that split into words of a dictionary, written with
 * spaces between the words. The backtrack chooses the characters one at a
 * time, as the permutations do, each distinct character once at a level so
 * that no anagram comes twice, and closes a word whenever the characters since
 * the last space make one: a permutation is pruned as soon as its last part
 * cannot be completed into a word, if the dictionary also tells the prefixes
 * of its words.
 *
 * The contracts [see `Contract.h`]: each sentence found has the letters of the
 * string (checked at level 3). The operations are defined in `Anagrams.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#ifndef __ADS_Anagrams
 #define __ADS_Anagrams
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the anagrams of \f$ s \f$ made of words for which `word( w )` is true,
/// `prefix( w )` being true if \f$ w \f$ begins some word (a word included);
/// the words of a sentence are separated by one blank, and the empty
/// \f$ s \f$ has the one anagram `""`
///
/// In pseudocode (exercise 16.8), \f$ L \f$ the multiset of the characters of
/// \f$ s \f$ not placed yet, \f$ w \f$ the word being built and \f$ T \f$ the
/// words closed so far:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{anagrams}( L , w , T )\quad
///     \triangleright\ L \text{ the letters left, } w
///     \text{ the word open, } T \text{ the words closed} \cr
/// 2 & \quad \textbf{if}\ L = \emptyset\ \textbf{then}\ \text{record } T
///     \text{ if } w = \varepsilon;\ \textbf{return}\quad \triangleright
///     \ \text{a half word is no anagram} \cr
/// 3 & \quad \textbf{foreach}\ \text{letter } c \text{ of } L
///     \text{, each value once, in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \text{move } c \text{ from } L \text{ to the end of } w \cr
/// 5 & \qquad \textbf{if}\ \mathrm{prefix}( w )\ \textbf{then}\quad
///     \triangleright\ \text{else no word starts with } w \cr
/// 6 & \qquad\quad \mathrm{anagrams}( L , w , T )\quad \triangleright\ w
///     \text{ grows} \cr
/// 7 & \qquad\quad \textbf{if}\ \mathrm{word}( w )\ \textbf{then}
///     \ \mathrm{anagrams}( L , \varepsilon , T \cdot w )\quad
///     \triangleright\ w \text{ is closed} \cr
/// 8 & \qquad \text{move the last letter of } w \text{ back to } L
/// \end{array}}
/// \f]
/// \post each sentence has the letters of \f$ s \f$ (audit)
template< class W , class P >
std::set< std::string > anagrams( const std::string & s , W && word ,
				  P && prefix );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/Anagrams.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Anagrams.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Anagrams.h ---------------------------*/
/*--------------------------------------------------------------------------*/
