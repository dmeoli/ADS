/*--------------------------------------------------------------------------*/
/*----------------------- File ApproximateMatching.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The approximate string matching of section 13.1: an occurrence of the
 * pattern \f$ P = p_0 \cdots p_{m-1} \f$ in the text
 * \f$ T = t_0 \cdots t_{n-1} \f$ with \f$ k \f$ errors, each a
 * substitution, an insertion (a character of \f$ P \f$ not in \f$ T \f$) or
 * a deletion (a character of \f$ T \f$ not in \f$ P \f$). The value
 * \f$ \mathit{DP}[ i ][ j ] \f$, the least \f$ k \f$ of an occurrence of
 * the prefix \f$ P( i ) \f$ of \f$ i \f$ characters ending at the end of
 * \f$ T( j ) \f$, is
 * \f[
 *   \mathit{DP}[ i ][ j ] = \left\{ \begin{array}{ll}
 *     0 & i = 0 \\ i & j = 0 \cr
 *     \min\{ \mathit{DP}[ i - 1 ][ j - 1 ] + \delta ,
 *            \mathit{DP}[ i - 1 ][ j ] + 1 ,
 *            \mathit{DP}[ i ][ j - 1 ] + 1 \} & \mbox{otherwise}
 *   \end{array} \right.
 * \f]
 * with \f$ \delta = 0 \f$ if \f$ p_{i-1} = t_{j-1} \f$ and
 * \f$ \delta = 1 \f$ otherwise, in \f$ \Theta( m n ) \f$ time; the best
 * occurrences end where the last row is smallest. `occurrence()` walks back
 * from \f$ \mathit{DP}[ m ][ j ] \f$ to where the occurrence starts
 * (exercise 13.1); `stringMatchingSingleRow()` keeps two rows only,
 * \f$ O( n ) \f$ space (section 13.4). `editDistance()`, the distance of
 * Levenshtein, has \f$ \mathit{DP}[ 0 ][ j ] = j \f$, since \f$ T( j ) \f$
 * has \f$ j \f$ characters more than the empty prefix of \f$ P \f$, and its
 * result in \f$ \mathit{DP}[ m ][ n ] \f$.
 *
 * The contracts [see `Contract.h`]: `occurrence()` needs the table of
 * \f$ P \f$ and \f$ T \f$ and a column of it; the best cost of
 * `stringMatching()` is between 0 and \f$ m \f$, the edit distance between
 * \f$ | m - n | \f$ and \f$ \max( m , n ) \f$. The operations are defined in
 * `ApproximateMatching.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 13.1 and
 *      13.4, https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see V. I. Levenshtein, Binary codes capable of correcting deletions,
 *      insertions, and reversals, Soviet Physics Doklady 10(8), 707-710,
 *      1966
 * \see P. H. Sellers, The theory and computation of evolutionary
 *      distances: pattern recognition, Journal of Algorithms 1(4),
 *      359-373, 1980, doi:10.1016/0196-6774(80)90016-4
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

#ifndef __ADS_ApproximateMatching
 #define __ADS_ApproximateMatching
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include "ADS/Contract.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the table \f$ \mathit{DP}[ i ][ j ] \f$ of the costs
using Table = std::vector< std::vector< int > >;

/// the table \f$ \mathit{DP} \f$ of \f$ P \f$ and \f$ T \f$, of
/// \f$ m + 1 \f$ rows and \f$ n + 1 \f$ columns; with `edit` the first row
/// is \f$ \mathit{DP}[ 0 ][ j ] = j \f$, that of the edit distance
///
/// In pseudocode (section 13.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{matchingTable}( P , T , \mathit{edit} ) \cr
/// 2 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ m\ \textbf{do}
///     \ \mathit{DP}[ i ][ 0 ] \gets i \cr
/// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n\ \textbf{do}
///     \ \mathit{DP}[ 0 ][ j ] \gets j \text{ if } \mathit{edit}
///     \text{, else }
///     0 \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 5 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 6 & \qquad\quad \delta \gets 0 \text{ if } p_{i-1} = t_{j-1} \text{, else }
///     1 \cr
/// 7 & \qquad\quad \mathit{DP}[ i ][ j ] \gets \min\{ \mathit{DP}[ i - 1 ][ j
///     - 1 ] + \delta , \mathit{DP}[ i - 1 ][ j ] + 1 , \mathit{DP}[ i ][ j
///     - 1 ] + 1 \} \cr
/// 8 & \quad \textbf{return}\ \mathit{DP}
/// \end{array}}
/// \f]
template< class S >
Table matchingTable( const S & P , const S & T , bool edit = false );

/// the end of an occurrence of \f$ P \f$ in \f$ T \f$ with the fewest
/// errors, the first one, and their number; \f$ ( -1 , m ) \f$ if \f$ T \f$
/// is empty
///
/// In pseudocode (section 13.1), the least value of the last row, the
/// column 0 left out:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{stringMatching}( P , T ) \cr
/// 2 & \quad \mathit{DP} \gets \mathrm{matchingTable}( P , T ,
///     \textbf{false} );\ \mathit{pos} \gets 1 \cr
/// 3 & \quad \textbf{for}\ j \gets 2\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{DP}[ m ][ j ] < \mathit{DP}[ m ][
///     \mathit{pos} ]\ \textbf{then}\ \mathit{pos} \gets j \cr
/// 5 & \quad \textbf{return}\ ( \mathit{pos} - 1 , \mathit{DP}[ m ][
///     \mathit{pos} ] )
/// \end{array}}
/// \f]
/// \post the cost is in \f$ [ 0 , m ] \f$
template< class S >
std::pair< long , int > stringMatching( const S & P , const S & T );

/// the place where the occurrence of \f$ P \f$ starts in \f$ T \f$ that
/// ends at \f$ T[ j - 1 ] \f$ with \f$ \mathit{DP}[ m ][ j ] \f$ errors,
/// walking back on the table
///
/// In pseudocode (exercise 13.1), each step undoing the term of the minimum
/// that gave the value of the cell:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{occurrence}( \mathit{DP} , P , T , j ) \cr
/// 2 & \quad i \gets m \cr
/// 3 & \quad \textbf{while}\ i > 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ j > 0\ \textbf{and}\ \mathit{DP}[ i ][ j ] =
///     \mathit{DP}[ i - 1 ][ j - 1 ] + \delta\ \textbf{then}\ i \gets i -
///     1;\ j \gets j - 1 \cr
/// 5 & \qquad \textbf{else if}\ \mathit{DP}[ i ][ j ] = \mathit{DP}[ i - 1
///     ][ j ] + 1\ \textbf{then}\ i \gets i - 1 \cr
/// 6 & \qquad \textbf{else}\ j \gets j - 1 \cr
/// 7 & \quad \textbf{return}\ j
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{DP} \f$ is `matchingTable( P , T )` and
///      \f$ j \le |T| \f$
template< class S >
std::size_t occurrence( const Table & DP , const S & P , const S & T ,
                        std::size_t j );

/// the least number of errors of an occurrence of \f$ P \f$ in \f$ T \f$,
/// with two rows of the table only
///
/// In pseudocode (section 13.4), \f$ \mathit{prev} \f$ the row \f$ i - 1 \f$
/// and \f$ D \f$ the row \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{stringMatchingSingleRow}( P , T ) \cr
/// 2 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n\ \textbf{do}\ D[ j ]
///     \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 4 & \qquad \mathit{prev} \gets D;\ D[ 0 ] \gets i \cr
/// 5 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 6 & \qquad\quad D[ j ] \gets \min\{ \mathit{prev}[ j - 1 ] + \delta ,
///     \mathit{prev}[ j ] + 1 , D[ j - 1 ] + 1 \} \cr
/// 7 & \quad \textbf{return}\ \min\{ D[ 1 ] , \ldots , D[ n ] \} \text{ (}
///     m \text{ if } n = 0 \text{)}
/// \end{array}}
/// \f]
template< class S >
int stringMatchingSingleRow( const S & P , const S & T );

/// the edit distance of \f$ P \f$ and \f$ T \f$, in
/// \f$ O( \min( m , n ) ) \f$ space
///
/// In pseudocode (section 13.4), the rows on the shorter string:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{editDistance}( P , T ) \cr
/// 2 & \quad \textbf{if}\ n > m\ \textbf{then return}\ \mathrm{editDistance}(
///     T , P ) \cr
/// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n\ \textbf{do}\ D[ j ]
///     \gets j \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 5 & \qquad \mathit{prev} \gets D;\ D[ 0 ] \gets i \cr
/// 6 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 7 & \qquad\quad D[ j ] \gets \min\{ \mathit{prev}[ j - 1 ] + \delta ,
///     \mathit{prev}[ j ] + 1 , D[ j - 1 ] + 1 \} \cr
/// 8 & \quad \textbf{return}\ D[ n ]
/// \end{array}}
/// \f]
/// \post the distance is in \f$ [ | m - n | , \max( m , n ) ] \f$
template< class S >
int editDistance( const S & P , const S & T );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/ApproximateMatching.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ApproximateMatching.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File ApproximateMatching.h ---------------------*/
/*--------------------------------------------------------------------------*/
