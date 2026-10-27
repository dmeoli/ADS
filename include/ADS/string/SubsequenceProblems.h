/*--------------------------------------------------------------------------*/
/*----------------------- File SubsequenceProblems.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Four classic problems on the subsequences of sequences \f$ X \f$,
 * \f$ Y \f$, \f$ Z \f$ of lengths \f$ n \f$, \f$ m \f$, \f$ l \f$, relatives
 * of the longest common subsequence [see `LongestCommonSubsequence.h`],
 * each by dynamic programming on the prefixes.
 *
 * - `isInterleaving()`: whether \f$ Z \f$ is a shuffle (an interleaving) of
 *   \f$ X \f$ and \f$ Y \f$, the items of each in their order:
 *   \f$ D[ i ][ j ] \f$, whether \f$ Z[ 0 .. i + j ) \f$ interleaves
 *   \f$ X[ 0 .. i ) \f$ and \f$ Y[ 0 .. j ) \f$, is \f$ D[ i - 1 ][ j ] \f$
 *   with \f$ x_{i-1} = z_{i+j-1} \f$ or \f$ D[ i ][ j - 1 ] \f$ with
 *   \f$ y_{j-1} = z_{i+j-1} \f$; \f$ \Theta( n m ) \f$ time and
 *   \f$ \Theta( m ) \f$ space, one row of the table. Mansfield showed the
 *   problem NP-complete for the shuffle of an unbounded number of strings,
 *   and Warmuth and Haussler for the iterated shuffle of one string.
 * - `shortestCommonSupersequence()`: the shortest sequence that has
 *   \f$ X \f$ and \f$ Y \f$ as subsequences, of length
 *   \f$ n + m - | \mathrm{LCS}( X , Y ) | \f$, read back from the table of
 *   the LCS: the common items once, the others in their order;
 *   \f$ \Theta( n m ) \f$ time and space, while `scsLength()` keeps one
 *   row, \f$ \Theta( m ) \f$ space. For an unbounded number of strings the
 *   problem is NP-hard (Maier).
 * - `subsequenceOccurrences()`: the number of occurrences of \f$ P \f$ as a
 *   subsequence of \f$ T \f$, the tuples of places
 *   \f$ j_1 < \cdots < j_m \f$ with \f$ t_{j_k} = p_k \f$:
 *   \f$ C[ i ][ j ] = C[ i - 1 ][ j ] + [ t_{i-1} = p_{j-1} ]
 *   C[ i - 1 ][ j - 1 ] \f$, \f$ \Theta( n m ) \f$ time and one row updated
 *   from the right, \f$ \Theta( m ) \f$ space; the count grows as fast as
 *   the binomial coefficient \f$ \binom{n}{m} \f$, and it is given modulo
 *   \f$ 2^{64} \f$.
 * - `lcs3()`: a longest common subsequence of three sequences, the table of
 *   `lcs()` in three dimensions, \f$ \Theta( n m l ) \f$ time and space.
 *   The LCS of two of them and then of the third is not a solution: the
 *   longest pairwise one may leave out the items that the third shares.
 *   For \f$ k \f$ sequences the table has \f$ \prod_i n_i \f$ entries, and
 *   the problem is NP-hard for an unbounded \f$ k \f$ (Maier).
 *
 * Each function takes any sequence `S` with `size()`, `operator[]`,
 * `push_back()` and the comparison of its items; the overloads on
 * `std::string` also take string literals.
 *
 * The contracts [see `Contract.h`]: the supersequence has \f$ X \f$ and
 * \f$ Y \f$ as subsequences and the length of `scsLength()`, and `lcs3()`
 * gives a subsequence of the three (audits). The functions are defined in
 * `SubsequenceProblems.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see D. Maier, The complexity of some problems on subsequences and
 *      supersequences, Journal of the ACM 25(2), 322-336, 1978,
 *      doi:10.1145/322063.322075
 * \see A. Mansfield, On the computational complexity of a merge recognition
 *      problem, Discrete Applied Mathematics 5(1), 119-122, 1983,
 *      doi:10.1016/0166-218X(83)90021-5
 * \see M. K. Warmuth, D. Haussler, On the complexity of iterated shuffle,
 *      Journal of Computer and System Sciences 28(3), 345-358, 1984,
 *      doi:10.1016/0022-0000(84)90018-7
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

#ifndef __ADS_SubsequenceProblems
 #define __ADS_SubsequenceProblems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/LongestCommonSubsequence.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether \f$ Z \f$ interleaves \f$ X \f$ and \f$ Y \f$ keeping the order
/// of the items of each
///
/// In pseudocode (chapter 13, beyond the book), the row \f$ i \f$ of the
/// table in \f$ D[ 0 .. m ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isInterleaving}( X , Y , Z ) \cr
/// 2 & \quad \textbf{if}\ |Z| \ne n + m\ \textbf{then return false} \cr
/// 3 & \quad D[ 0 ] \gets \textbf{true};\ \textbf{for}\ j \gets 1
///     \ \textbf{to}\ m\ \textbf{do}\ D[ j ] \gets D[ j - 1 ]\ \textbf{and}
///     \ Y[ j - 1 ] = Z[ j - 1 ] \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad D[ 0 ] \gets D[ 0 ]\ \textbf{and}\ X[ i - 1 ] = Z[ i - 1 ] \cr
/// 6 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 7 & \qquad\quad D[ j ] \gets ( D[ j ]\ \textbf{and}\ X[ i - 1 ] = Z[ i + j
///     - 1 ] )\ \textbf{or}\ ( D[ j - 1 ]\ \textbf{and}\ Y[ j - 1 ] = Z[ i +
///     j - 1 ] ) \cr
/// 8 & \quad \textbf{return}\ D[ m ]
/// \end{array}}
/// \f]
template< class S > requires std::is_class_v< S >
[[nodiscard]] bool isInterleaving( const S & X , const S & Y , const S & Z );

/// `isInterleaving()` above on `std::string` (or literals)
[[nodiscard]] inline bool isInterleaving( const std::string & X ,
                                          const std::string & Y ,
                                          const std::string & Z );

/// the length of a shortest common supersequence of \f$ X \f$ and \f$ Y \f$
///
/// In pseudocode (chapter 13, beyond the book), one row \f$ L \f$ of the
/// table of the LCS:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{scsLength}( X , Y ) \cr
/// 2 & \quad L[ 0 .. m ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \mathit{diag} \gets 0 \cr
/// 5 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 6 & \qquad\quad \mathit{up} \gets L[ j ] \cr
/// 7 & \qquad\quad L[ j ] \gets \mathit{diag} + 1 \text{ if } X[ i - 1 ] =
///     Y[ j - 1 ] \text{, else } \max\{ \mathit{up} , L[ j - 1 ] \} \cr
/// 8 & \qquad\quad \mathit{diag} \gets \mathit{up} \cr
/// 9 & \quad \textbf{return}\ n + m - L[ m ]
/// \end{array}}
/// \f]
/// \post \f$ n + m - | \mathrm{lcs}( X , Y ) | \f$ (an audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::size_t scsLength( const S & X , const S & Y );

/// `scsLength()` above on `std::string` (or literals)
[[nodiscard]] inline std::size_t scsLength( const std::string & X ,
                                            const std::string & Y );

/// a shortest common supersequence of \f$ X \f$ and \f$ Y \f$
///
/// In pseudocode (chapter 13, beyond the book), the table of the LCS read
/// back as `lcs()` does, keeping also the items left out:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{shortestCommonSupersequence}( X , Y ) \cr
/// 2 & \quad L \gets \mathrm{lcsTable}( X , Y );\ i \gets n;\ j \gets m;\ W
///     \gets (\,) \cr
/// 3 & \quad \textbf{while}\ i > 0\ \textbf{and}\ j > 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ X[ i - 1 ] = Y[ j - 1 ]\ \textbf{then}
///     \ \text{prepend } X[ i - 1 ] \text{ to } W;\ i \gets i - 1;\ j \gets j
///     - 1 \cr
/// 5 & \qquad \textbf{else if}\ L[ i - 1 ][ j ] \ge L[ i ][ j - 1 ]
///     \ \textbf{then}\ \text{prepend } X[ i - 1 ] \text{ to } W;\ i \gets i -
///     1 \cr
/// 6 & \qquad \textbf{else}\ \text{prepend } Y[ j - 1 ] \text{ to } W;\ j
///     \gets j - 1 \cr
/// 7 & \quad \textbf{return}\ X[ 0 .. i - 1 ] \cdot Y[ 0 .. j - 1 ] \cdot W
/// \end{array}}
/// \f]
/// \post \f$ X \f$ and \f$ Y \f$ subsequences of it (an audit), of length
///       `scsLength()`
template< class S > requires std::is_class_v< S >
[[nodiscard]] S shortestCommonSupersequence( const S & X , const S & Y );

/// `shortestCommonSupersequence()` above on `std::string` (or literals)
[[nodiscard]] inline std::string shortestCommonSupersequence(
                               const std::string & X , const std::string & Y );

/// the number of occurrences of \f$ P \f$ as a subsequence of \f$ T \f$,
/// modulo \f$ 2^{64} \f$: 1 for the empty \f$ P \f$, 0 if \f$ P \f$ is
/// longer than \f$ T \f$
///
/// In pseudocode (chapter 13, beyond the book), \f$ C[ j ] \f$ the
/// occurrences of \f$ P[ 0 .. j - 1 ] \f$ in the prefix of \f$ T \f$ read so
/// far, updated from the right:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{subsequenceOccurrences}( P , T ) \cr
/// 2 & \quad C[ 0 ] \gets 1;\ C[ 1 .. m ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |T| - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ j \gets \min\{ m , i + 1 \}\ \textbf{downto}\ 1
///     \ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ T[ i ] = P[ j - 1 ]\ \textbf{then}\ C[ j ]
///     \gets ( C[ j ] + C[ j - 1 ] ) \bmod 2^{64} \cr
/// 6 & \quad \textbf{return}\ C[ m ]
/// \end{array}}
/// \f]
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::uint64_t subsequenceOccurrences( const S & P ,
                                                    const S & T );

/// `subsequenceOccurrences()` above on `std::string` (or literals)
[[nodiscard]] inline std::uint64_t subsequenceOccurrences(
                               const std::string & P , const std::string & T );

/// a longest common subsequence of \f$ X \f$, \f$ Y \f$ and \f$ Z \f$
///
/// In pseudocode (section 13.7 in three dimensions, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcs3}( X , Y , Z ) \cr
/// 2 & \quad D[ 0 .. n ][ 0 .. m ][ 0 .. l ] \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ ( i , j , k ) \in [ 1 , n ] \times [ 1 , m ]
///     \times [ 1 , l ] \text{ in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ X[ i - 1 ] = Y[ j - 1 ] = Z[ k - 1 ]
///     \ \textbf{then}\ D[ i ][ j ][ k ] \gets D[ i - 1 ][ j - 1 ][ k - 1 ] +
///     1 \cr
/// 5 & \qquad \textbf{else}\ D[ i ][ j ][ k ] \gets \max\{ D[ i - 1 ][ j ][ k
///     ] , D[ i ][ j - 1 ][ k ] , D[ i ][ j ][ k - 1 ] \} \cr
/// 6 & \quad \textbf{return}\ \text{the subsequence read back from }
///     D[ n ][ m ][ l ] \text{, as } \mathrm{lcs}() \text{ does}
/// \end{array}}
/// \f]
/// \post a subsequence of the three (an audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] S lcs3( const S & X , const S & Y , const S & Z );

/// `lcs3()` above on `std::string` (or literals)
[[nodiscard]] inline std::string lcs3( const std::string & X ,
                                       const std::string & Y ,
                                       const std::string & Z );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SubsequenceProblems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SubsequenceProblems.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SubsequenceProblems.h ---------------------*/
/*--------------------------------------------------------------------------*/
