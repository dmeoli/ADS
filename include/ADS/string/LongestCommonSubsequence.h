/*--------------------------------------------------------------------------*/
/*-------------------- File LongestCommonSubsequence.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The longest common subsequence (LCS) of P and T, the reality check of
 * section 13.7: DP[ i ][ j ], its length for the prefixes P( i ) and
 * T( j ), is 0 if i = 0 or j = 0, DP[ i - 1 ][ j - 1 ] + 1 if
 * p_{i-1} = t_{j-1}, and \f$ \max\{ DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] \}
 * \f$ otherwise, Theta( m n ). lcsMemo() is the memoized version of the
 * book, which solves only the subproblems that it reaches. diff() reads the
 * differences from the table in O( m + n ), as the Unix command does on
 * the lines of two files (exercise 13.10): the items of P only marked
 * '-', the items of T only '+', the common ones ' '. The longest common
 * substring, contiguous, has DP[ i ][ j ] = DP[ i - 1 ][ j - 1 ] + 1 if
 * p_{i-1} = t_{j-1} and 0 otherwise, its length the largest entry
 * (exercise 13.2).
 *
 * The contracts [see Contract.h]: lcs() gives a subsequence of both strings as
 * long as the table says (checked at level 3), and longestCommonSubstring() a
 * substring of both. The operations are defined in
 * LongestCommonSubsequence.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see J. W. Hunt, M. D. McIlroy, An algorithm for differential file
 *      comparison, Computing Science Technical Report 41, Bell
 *      Laboratories, 1976
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

#ifndef __ADS_LongestCommonSubsequence
 #define __ADS_LongestCommonSubsequence
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the table of the lengths of the LCS of the prefixes
template< class S >
std::vector< std::vector< int > > lcsTable( const S & P , const S & T );

/// lcs() of the book, by memoization: -1 marks the subproblems not solved
/// yet; the number of the ones solved goes to *solved, if given
template< class S >
int lcsMemo( const S & P , const S & T , long * solved = nullptr );

/// a longest common subsequence, read back from the table
/// \post a subsequence of P and T of the length of lcsTable()
template< class S >
S lcs( const S & P , const S & T );

/// the differences from P to T: ( '-' , p ) for an item of P only,
/// ( '+' , t ) for an item of T only, ( ' ' , x ) for a common one
template< class S >
std::vector< std::pair< char , typename S::value_type > > diff(
                                                 const S & P , const S & T );

/// the longest common substring: its length and where it ends in P and T,
/// the first one found by rows of P; ( 0 , 0 , 0 ) if they share nothing
/// \post P and T share the substring of length best ending at ei and ej
template< class S >
std::tuple< std::size_t , std::size_t , std::size_t >
longestCommonSubstring( const S & P , const S & T );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/LongestCommonSubsequence.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LongestCommonSubsequence.h included */

/*--------------------------------------------------------------------------*/
/*------------------ End File LongestCommonSubsequence.h -------------------*/
/*--------------------------------------------------------------------------*/
