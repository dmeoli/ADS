/*--------------------------------------------------------------------------*/
/*---------------------------- File Anagrams.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The anagrams (exercise 16.8): the strings made of all the characters of
 * a given one, in any order, that split into words of a dictionary,
 * written with spaces between the words. The backtrack chooses the
 * characters one at a time, as the permutations do, each distinct
 * character once at a level so that no anagram comes twice, and closes a
 * word whenever the characters since the last space make one: a
 * permutation is pruned as soon as its last part cannot be completed into
 * a word, if the dictionary also tells the prefixes of its words.
 *
 * The contracts [see Contract.h]: each sentence found has the letters of the
 * string (checked at level 3). The operations are defined in Anagrams.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

namespace ADS {

/// the anagrams of s made of words for which word( w ) is true; prefix( w )
/// is true if w begins some word; the words of a sentence are separated by
/// one blank, and the empty s has the one anagram ""
/// \post each sentence has the letters of s
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

#endif  /* Anagrams.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Anagrams.h ---------------------------*/
/*--------------------------------------------------------------------------*/
