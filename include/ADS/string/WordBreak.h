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
#include <optional>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the words of s, if it is a sequence of words, by the lookup( word ) of
/// a dictionary; the lookups made go to *lookups, if given
template< class F >
std::optional< std::vector< std::string > > wordBreak(
                         const std::string & s , F && lookup ,
                         std::size_t longest = std::string::npos ,
                         long * lookups = nullptr )
{
 const std::size_t n = s.size();
 std::vector< long > from( n + 1 , -1 );           // the start of the word
 from[ 0 ] = 0;
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( std::size_t j = i ; j-- > 0 ; ) {
   if( i - j > longest )
    break;
   if( from[ j ] < 0 )
    continue;
   if( lookups ) ++*lookups;
   if( lookup( s.substr( j , i - j ) ) ) {
    from[ i ] = long( j );
    break;
    }
   }
 if( from[ n ] < 0 )
  return( std::nullopt );
 std::vector< std::string > words;
 for( std::size_t i = n ; i > 0 ; i = std::size_t( from[ i ] ) )
  words.insert( words.begin() , s.substr( from[ i ] , i - from[ i ] ) );
 return( words );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* WordBreak.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File WordBreak.h --------------------------*/
/*--------------------------------------------------------------------------*/
