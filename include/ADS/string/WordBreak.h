/*--------------------------------------------------------------------------*/
/*---------------------------- File WordBreak.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Is it Italian? (exercise 13.13): a string s[ 0 .. n - 1 ] with no spaces
 * is a sequence of words of a dictionary if one of its prefixes s[ 0 .. j )
 * is and s[ j .. i ) is a word: DP[ i ] = true if and only if there is j < i
 * with DP[ j ] and s[ j .. i ) in the dictionary, DP[ 0 ] = true; O( n^2 )
 * lookups, or O( n w ) if no word is longer than w. The words are read back
 * from the table.
 *
 * The contracts [see Contract.h]: the words found, joined, give back the
 * string. The operations are defined in WordBreak.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 13.13,
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

/// the words of s, if it is a sequence of words, by the lookup( word ) of
/// a dictionary; the lookups made go to *lookups, if given
/// \post the words joined give s
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

#include "ADS/string/WordBreak.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WordBreak.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File WordBreak.h --------------------------*/
/*--------------------------------------------------------------------------*/
