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
/// is true if w begins some word
template< class W , class P >
std::set< std::string > anagrams( const std::string & s , W && word ,
                                  P && prefix )
{
 std::map< char , int > left;
 for( char c : s ) ++left[ c ];
 std::set< std::string > out;
 std::string sentence , current;
 auto rec = [ & ]( auto & self , std::size_t placed ) -> void {
  if( placed == s.size() ) {
   if( current.empty() )
    out.insert( sentence.substr( 0 , sentence.size() - 1 ) );
   return;
   }
  for( auto & [ c , k ] : left ) {
   if( k == 0 ) continue;
   --k;
   current += c;
   if( prefix( current ) ) {
    self( self , placed + 1 );                   // the word goes on
    if( word( current ) ) {                      // or it ends here
     const std::string w = current , before = sentence;
     sentence += w + ' ';
     current.clear();
     self( self , placed + 1 );
     current = w;
     sentence = before;
     }
    }
   current.pop_back();
   ++k;
   }
  };
 rec( rec , 0 );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Anagrams.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Anagrams.h ---------------------------*/
/*--------------------------------------------------------------------------*/
