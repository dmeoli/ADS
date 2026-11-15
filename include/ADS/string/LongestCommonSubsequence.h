/*--------------------------------------------------------------------------*/
/*-------------------- File LongestCommonSubsequence.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The longest common subsequence (LCS) of \f$ P \f$ and \f$ T \f$, the
 * reality check of section 13.7 of the book: \f$ \mathit{DP}[ i ][ j ] \f$,
 * its length for the prefixes \f$ P( i ) \f$ and \f$ T( j ) \f$, is
 * \f[
 *   \mathit{DP}[ i ][ j ] = \left\{ \begin{array}{ll}
 *     0 & i = 0 \mbox{ or } j = 0 , \cr
 *     \mathit{DP}[ i - 1 ][ j - 1 ] + 1 & p_{i-1} = t_{j-1} , \cr
 *     \max\{ \mathit{DP}[ i - 1 ][ j ] , \mathit{DP}[ i ][ j - 1 ] \} &
 *     \mbox{otherwise,}
 *   \end{array} \right.
 * \f]
 * \f$ \Theta( m n ) \f$ time. `lcsMemo()` is the memoized version of the
 * book, which solves only the subproblems that it reaches. `diff()` reads
 * the differences from the table in \f$ O( m + n ) \f$, as the Unix command
 * does on the lines of two files (exercise 13.10): the items of \f$ P \f$
 * only marked `'-'`, the items of \f$ T \f$ only `'+'`, the common ones
 * `' '`. The longest common substring, contiguous, has
 * \f$ \mathit{DP}[ i ][ j ] = \mathit{DP}[ i - 1 ][ j - 1 ] + 1 \f$ if
 * \f$ p_{i-1} = t_{j-1} \f$ and 0 otherwise, its length the largest entry
 * (exercise 13.2).
 *
 * The contracts [see `Contract.h`]: `lcs()` gives a subsequence of both
 * strings as long as the table says (checked at level 3), and
 * `longestCommonSubstring()` a substring of both. The operations are
 * defined in `LongestCommonSubsequence.tpp`.
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

namespace ADS
{

/// the table of the lengths of the LCS of the prefixes of \f$ P \f$ and
/// \f$ T \f$
///
/// In pseudocode (section 13.7):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcsTable}( P , T )\quad \triangleright
///     \ \mathit{DP}[ i ][ j ] \text{ for the prefixes of } i \text{ and } j
///     \text{ letters} \cr
/// 2 & \quad \mathit{DP}[ 0 .. m ][ 0 .. n ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ P[ i - 1 ] \ne T[ j - 1 ]\ \textbf{then}
///     \ \mathit{DP}[ i ][ j ] \gets \max\{ \mathit{DP}[ i ][ j - 1 ] ,
///     \mathit{DP}[ i - 1 ][ j ] \} \cr
/// 6 & \qquad\quad \textbf{else}\ \mathit{DP}[ i ][ j ] \gets 1 +
///     \mathit{DP}[ i - 1 ][ j - 1 ]\quad \triangleright
///     \ \text{the common last letter} \cr
/// 7 & \quad \textbf{return}\ \mathit{DP}
/// \end{array}}
/// \f]
template< class S >
std::vector< std::vector< int > > lcsTable( const S & P , const S & T );

/// the length of the LCS by the memoization of the book, \f$ -1 \f$
/// marking the subproblems not solved yet; the number of the ones solved
/// goes to `*solved`, if given
///
/// In pseudocode (section 13.7), \f$ \mathit{DP} \f$ all \f$ -1 \f$ at
/// first and the call \f$ \mathrm{rec}( m , n ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rec}( i , j )\quad \triangleright
///     \ \mathit{DP} \text{ starts at } -1 \text{, not yet computed} \cr
/// 2 & \quad \textbf{if}\ i = 0\ \textbf{or}\ j = 0\ \textbf{then return}\ 0
///     \cr
/// 3 & \quad \textbf{if}\ \mathit{DP}[ i ][ j ] \ge 0\ \textbf{then return}
///     \ \mathit{DP}[ i ][ j ] \cr
/// 4 & \quad \mathit{DP}[ i ][ j ] \gets 1 + \mathrm{rec}( i - 1 , j - 1 )
///     \text{ if } P[ i - 1 ] = T[ j - 1 ] \text{, else } \max\{
///     \mathrm{rec}( i , j - 1 ) , \mathrm{rec}( i - 1 , j ) \} \cr
/// 5 & \quad \textbf{return}\ \mathit{DP}[ i ][ j ]
/// \end{array}}
/// \f]
template< class S >
int lcsMemo( const S & P , const S & T , long * solved = nullptr );

/// a longest common subsequence, read back from the table
///
/// In pseudocode (section 13.7):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcs}( P , T ) \cr
/// 2 & \quad \mathit{DP} \gets \mathrm{lcsTable}( P , T );\ ( i , j ) \gets
///     ( m , n );\ X \gets () \cr
/// 3 & \quad \textbf{while}\ i > 0\ \textbf{and}\ j > 0\ \textbf{do}\quad
///     \triangleright\ X \text{ built from its end} \cr
/// 4 & \qquad \textbf{if}\ P[ i - 1 ] = T[ j - 1 ]\ \textbf{then}
///     \ \text{put } P[ i - 1 ] \text{ at the front of } X;\ ( i , j ) \gets
///     ( i - 1 , j - 1 ) \cr
/// 5 & \qquad \textbf{else if}\ \mathit{DP}[ i ][ j - 1 ] > \mathit{DP}[ i -
///     1 ][ j ]\ \textbf{then}\ j \gets j - 1\ \textbf{else}\ i \gets i - 1
///     \cr
/// 6 & \quad \textbf{return}\ X
/// \end{array}}
/// \f]
/// \post a subsequence of \f$ P \f$ and \f$ T \f$ of the length of
///       `lcsTable()`
template< class S >
S lcs( const S & P , const S & T );

/// the differences from \f$ P \f$ to \f$ T \f$: `( '-' , p )` for an item
/// of \f$ P \f$ only, `( '+' , t )` for an item of \f$ T \f$ only,
/// `( ' ' , x )` for a common one
///
/// In pseudocode (exercise 13.10), the table read back as `lcs()` does,
/// the items of \f$ T \f$ preferred on a tie:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{diff}( P , T )\quad \triangleright
///     \ \text{the lines kept, added (+) and removed (-)} \cr
/// 2 & \quad \mathit{DP} \gets \mathrm{lcsTable}( P , T );\ ( i , j ) \gets
///     ( m , n );\ D \gets () \cr
/// 3 & \quad \textbf{while}\ ( i , j ) \ne ( 0 , 0 )\ \textbf{do}\quad
///     \triangleright\ D \text{ built from its end} \cr
/// 4 & \qquad \textbf{if}\ i , j > 0 \text{ and } P[ i - 1 ] = T[ j - 1 ]
///     \ \textbf{then}\ \text{put } ( \text{' '} , P[ i - 1 ] )
///     \text{ at the front of } D;\ ( i , j ) \gets ( i - 1 , j - 1 ) \cr
/// 5 & \qquad \textbf{else if}\ i > 0 \text{ and } ( j = 0 \text{ or }
///     \mathit{DP}[ i - 1 ][ j ] > \mathit{DP}[ i ][ j - 1 ] )
///     \ \textbf{then} \cr
/// 6 & \qquad\quad \text{put } ( \text{'-'} , P[ i - 1 ] )
///     \text{ at the front of } D;\ i \gets i - 1 \cr
/// 7 & \qquad \textbf{else}\ \text{put } ( \text{'+'} , T[ j - 1 ] )
///     \text{ at the front of } D;\ j \gets j - 1 \cr
/// 8 & \quad \textbf{return}\ D
/// \end{array}}
/// \f]
template< class S >
std::vector< std::pair< char , typename S::value_type > > diff( const S & P ,
							       const S & T );

/// the longest common substring: its length and where it ends in
/// \f$ P \f$ and \f$ T \f$, the first one found by rows of \f$ P \f$;
/// \f$ ( 0 , 0 , 0 ) \f$ if they share nothing
///
/// In pseudocode (exercise 13.2), two rows of the table:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestCommonSubstring}( P , T )\quad
///     \triangleright\ \mathit{DP}[ i ][ j ]
///     \text{ the longest common suffix} \cr
/// 2 & \quad ( \mathit{best} , e_i , e_j ) \gets ( 0 , 0 , 0 ) \cr
/// 3 & \quad \textbf{foreach}\ i \in [ 1 , m ] , j \in [ 1 , n ]
///     \text{, by rows}\ \textbf{do} \cr
/// 4 & \qquad \mathit{DP}[ i ][ j ] \gets 0 \text{ if } P[ i - 1 ] \ne T[ j
///     - 1 ] \text{, else } 1 + \mathit{DP}[ i - 1 ][ j - 1 ] \cr
/// 5 & \qquad \textbf{if}\ \mathit{best} < \mathit{DP}[ i ][ j ]
///     \ \textbf{then}\ ( \mathit{best} , e_i , e_j ) \gets ( \mathit{DP}[ i
///     ][ j ] , i , j )\quad \triangleright\ \text{the first longest} \cr
/// 6 & \quad \textbf{return}\ ( \mathit{best} , e_i , e_j )
/// \end{array}}
/// \f]
/// \post \f$ P \f$ and \f$ T \f$ share the substring of length
///       \f$ \mathit{best} \f$ ending at \f$ e_i \f$ and \f$ e_j \f$
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

#endif /* LongestCommonSubsequence.h included */

/*--------------------------------------------------------------------------*/
/*------------------ End File LongestCommonSubsequence.h -------------------*/
/*--------------------------------------------------------------------------*/
