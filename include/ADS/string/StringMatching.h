/*--------------------------------------------------------------------------*/
/*------------------------- File StringMatching.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact string matching, beyond the book: the occurrences of a
 * pattern \f$ P \f$ of \f$ m \f$ characters in a text \f$ T \f$ of
 * \f$ n \f$ characters, i.e., the places \f$ s \in [ 0 , n - m ] \f$ with
 * \f$ T[ s .. s + m - 1 ] = P \f$. These are the algorithms of the
 * textbooks, which count their comparisons of characters where asked; the
 * engineered search of ADS is `StringSearcher` [see `StringSearch.h`].
 *
 * - `naiveMatching()` tries each shift of \f$ P \f$ along \f$ T \f$,
 *   \f$ O( n m ) \f$;
 * - `kmp()`, the algorithm of Knuth, Morris and Pratt, first computes the
 *   failure function of \f$ P \f$, \f$ F[ q ] \f$ the length of the longest
 *   proper border of \f$ P[ 0 .. q - 1 ] \f$ (a <em>border</em> is both a
 *   prefix and a suffix), and then scans \f$ T \f$ once, shifting \f$ P \f$
 *   by its failure on a mismatch, so that no character of \f$ T \f$ is
 *   compared again after a match: at most \f$ 2 n \f$ comparisons,
 *   \f$ \Theta( n + m ) \f$ time. `strongFailure()` is the failure function
 *   improved by Knuth, \f$ G[ q ] = G[ F[ q ] ] \f$ if
 *   \f$ P[ F[ q ] ] = P[ q ] \f$ and \f$ G[ q ] = F[ q ] \f$ otherwise,
 *   since a mismatch on \f$ P[ q ] \f$ would repeat on the character after
 *   the border; `kmpStrong()` is `kmp()` with it, which never compares a
 *   character of \f$ T \f$ twice against the same character of \f$ P \f$;
 * - `boyerMoore()` compares \f$ P \f$ from right to left and shifts by the
 *   larger of the bad character rule (the last place of the character of
 *   \f$ T \f$ in \f$ P \f$) and the strong good suffix rule (the next place
 *   of the suffix matched, preceded by another character, or the longest
 *   prefix of \f$ P \f$ that is its suffix), both prepared in
 *   \f$ O( m + \sigma ) \f$; \f$ O( n / m ) \f$ comparisons at best,
 *   \f$ O( n m ) \f$ in the worst case for all the occurrences
 *   (\f$ O( n ) \f$ for the first one);
 * - `horspool()` shifts by the bad character of the last character of the
 *   window alone,
 *   \f$ \mathit{shift}[ c ] = m - 1 - \max\{ i < m - 1 : P[ i ] = c \} \f$
 *   (\f$ m \f$ if none), the simplest and often the fastest of the family
 *   on large alphabets, \f$ O( n m ) \f$ in the worst case;
 * - `shiftOr()` (\f$ m \le 64 \f$) keeps in a word \f$ D \f$ the prefixes of
 *   \f$ P \f$ that end at the current character, a clear bit \f$ i \f$ when
 *   \f$ P[ 0 .. i ] \f$ does, and updates it as
 *   \f$ D \gets ( D \ll 1 ) \vee B[ T[ j ] ] \f$, with the bit \f$ i \f$ of
 *   \f$ B[ c ] \f$ clear if \f$ P[ i ] = c \f$: \f$ \Theta( n ) \f$
 *   operations on words;
 * - `zFunction()` gives \f$ Z[ i ] = \mathrm{lcp}( S , S[ i .. ] ) \f$ for
 *   every \f$ i \f$ in \f$ \Theta( n ) \f$, from the Z-box
 *   \f$ [ l , r ) \f$ that reaches farthest: inside it
 *   \f$ Z[ i ] \ge \min( Z[ i - l ] , r - i ) \f$, and only the characters
 *   past \f$ r \f$ are compared; `zMatching()` extends the boxes of
 *   \f$ T \f$ against \f$ P \f$ in the same way, \f$ \Theta( n + m ) \f$;
 * - `twoWay()` is the Two-Way algorithm of Crochemore and Perrin [see
 *   `StringSearch.h`]: the critical factorization \f$ P = u v \f$ by two
 *   maximal suffixes, \f$ v \f$ compared from left to right and \f$ u \f$
 *   from right to left, at most \f$ 2 n \f$ comparisons in \f$ O( 1 ) \f$
 *   extra space.
 *
 * The empty pattern occurs at every place \f$ 0 , \ldots , n \f$.
 *
 * The contracts [see `Contract.h`]: the failure function gives proper
 * borders, the Z-function values that fit the string, and every search the
 * occurrences of `naiveMatching()` (checked at the level of the audits).
 * The operations are defined in `StringMatching.tpp`.
 *
 * \see D. E. Knuth, J. H. Morris, V. R. Pratt, Fast pattern matching in
 *      strings, SIAM Journal on Computing 6(2), 323-350, 1977,
 *      doi:10.1137/0206024
 * \see R. S. Boyer, J. S. Moore, A fast string searching algorithm,
 *      Communications of the ACM 20(10), 762-772, 1977,
 *      doi:10.1145/359842.359859
 * \see R. N. Horspool, Practical fast searching in strings, Software:
 *      Practice and Experience 10(6), 501-506, 1980,
 *      doi:10.1002/spe.4380100608
 * \see R. A. Baeza-Yates, G. H. Gonnet, A new approach to text searching,
 *      Communications of the ACM 35(10), 74-82, 1992,
 *      doi:10.1145/135239.135243
 * \see D. Gusfield, Algorithms on Strings, Trees and Sequences, Cambridge
 *      University Press, 1997, section 1.4, doi:10.1017/CBO9780511574931
 * \see M. Crochemore, D. Perrin, Two-way string-matching, Journal of the
 *      ACM 38(3), 650-674, 1991, doi:10.1145/116825.116845
 * \see C. Charras, T. Lecroq, Handbook of Exact String Matching
 *      Algorithms, King's College Publications, 2004, ISBN 0-9543006-4-5
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

#ifndef __ADS_StringMatching
 #define __ADS_StringMatching
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <cstdint>
#include "ADS/Contract.h"

#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by trying
/// every shift
///
/// In pseudocode (the naive algorithm, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{naiveMatching}( T , P ) \cr
/// 2 & \quad R \gets \emptyset \cr
/// 3 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ n - m\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ T[ s .. s + m - 1 ] = P\ \textbf{then}\ R \gets R
///     \cup \{ s \} \cr
/// 5 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
inline std::vector< std::size_t > naiveMatching( std::string_view T ,
                                                 std::string_view P );

/// the failure function: \f$ F[ q ] \f$ is the length of the longest proper
/// border of \f$ P[ 0 .. q - 1 ] \f$, and \f$ F[ 0 ] = -1 \f$
///
/// In pseudocode (Knuth, Morris and Pratt, beyond the book), \f$ k \f$ the
/// border of the prefix read so far, shortened by the failures until it can
/// be extended:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{failure}( P ) \cr
/// 2 & \quad F[ 0 ] \gets -1;\ k \gets -1 \cr
/// 3 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{while}\ k \ge 0\ \textbf{and}\ P[ k ] \ne P[ q ]
///     \ \textbf{do}\ k \gets F[ k ] \cr
/// 5 & \qquad k \gets k + 1;\ F[ q + 1 ] \gets k \cr
/// 6 & \quad \textbf{return}\ F
/// \end{array}}
/// \f]
/// \post \f$ F[ q ] < q \f$ for \f$ q > 0 \f$
inline std::vector< long > failure( std::string_view P );

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by
/// Knuth-Morris-Pratt; the comparisons of characters go to `*comparisons`,
/// if given
///
/// In pseudocode (Knuth, Morris and Pratt, beyond the book), \f$ q \f$ the
/// characters of \f$ P \f$ matched, which never decrease below the border
/// of what was matched; \f$ i \f$ never goes back, which gives at most
/// \f$ 2 n \f$ comparisons:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kmp}( T , P ) \cr
/// 2 & \quad F \gets \mathrm{failure}( P );\ q \gets 0;\ R \gets \emptyset \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{while}\ q \ge 0\ \textbf{and}\ P[ q ] \ne T[ i ]
///     \ \textbf{do}\ q \gets F[ q ] \cr
/// 5 & \qquad q \gets q + 1 \cr
/// 6 & \qquad \textbf{if}\ q = m\ \textbf{then}\ R \gets R \cup \{ i + 1 - m
///     \};\ q \gets F[ m ] \cr
/// 7 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post the occurrences of `naiveMatching( T , P )`
inline std::vector< std::size_t > kmp( std::string_view T ,
                                       std::string_view P ,
                                       long * comparisons = nullptr );

/// the strong failure function: \f$ G[ q ] = F[ q ] \f$ unless
/// \f$ P[ F[ q ] ] = P[ q ] \f$, in which case \f$ G[ q ] = G[ F[ q ] ] \f$;
/// \f$ G[ m ] = F[ m ] \f$
///
/// In pseudocode (Knuth, Morris and Pratt, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{strongFailure}( P ) \cr
/// 2 & \quad F \gets \mathrm{failure}( P );\ G[ 0 ] \gets -1 \cr
/// 3 & \quad \textbf{for}\ q \gets 1\ \textbf{to}\ m\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ q < m\ \textbf{and}\ F[ q ] \ge 0\ \textbf{and}
///     \ P[ F[ q ] ] = P[ q ]\ \textbf{then}\ G[ q ] \gets G[ F[ q ] ] \cr
/// 5 & \qquad \textbf{else}\ G[ q ] \gets F[ q ] \cr
/// 6 & \quad \textbf{return}\ G
/// \end{array}}
/// \f]
/// \post \f$ G[ q ] \le F[ q ] \f$
inline std::vector< long > strongFailure( std::string_view P );

/// the occurrences of `kmp()` above, with the strong failure function on a
/// mismatch
///
/// In pseudocode (Knuth, Morris and Pratt, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kmpStrong}( T , P ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{kmp}( T , P ) \text{ with }
///     \mathrm{strongFailure}( P ) \text{ in place of } F \text{ in its }
///     \text{line 4}
/// \end{array}}
/// \f]
/// \post the occurrences of `naiveMatching( T , P )`
inline std::vector< std::size_t > kmpStrong( std::string_view T ,
                                             std::string_view P ,
                                             long * comparisons = nullptr );

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by
/// Boyer-Moore with the bad character and the strong good suffix rules
///
/// In pseudocode (Boyer and Moore, beyond the book), \f$ \mathit{bc} \f$ the
/// table of `horspool()` and \f$ \mathit{gs}[ i ] \f$ the shift of the good
/// suffix after a mismatch at \f$ i \f$, both computed from \f$ P \f$
/// alone; each rule shifts by no more than the next place where the
/// window can match:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{boyerMoore}( T , P ) \cr
/// 2 & \quad \mathit{bc} \gets \text{the bad characters of } P;
///     \ \mathit{gs} \gets \text{the good suffixes of } P \cr
/// 3 & \quad j \gets 0;\ R \gets \emptyset \cr
/// 4 & \quad \textbf{while}\ j \le n - m\ \textbf{do} \cr
/// 5 & \qquad i \gets m - 1 \cr
/// 6 & \qquad \textbf{while}\ i \ge 0\ \textbf{and}\ P[ i ] = T[ j + i ]
///     \ \textbf{do}\ i \gets i - 1 \cr
/// 7 & \qquad \textbf{if}\ i < 0\ \textbf{then}\ R \gets R \cup \{ j \};
///     \ j \gets j + \mathit{gs}[ 0 ] \cr
/// 8 & \qquad \textbf{else}\ j \gets j + \max\{ \mathit{gs}[ i ] ,
///     \mathit{bc}[ T[ j + i ] ] - ( m - 1 - i ) \} \cr
/// 9 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post the occurrences of `naiveMatching( T , P )` (audit)
inline std::vector< std::size_t > boyerMoore( std::string_view T ,
                                              std::string_view P ,
                                              long * comparisons = nullptr );

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by Horspool
///
/// In pseudocode (Horspool's algorithm, beyond the book), the window
/// compared from its last character and shifted by the bad character of
/// that character:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{horspool}( T , P ) \cr
/// 2 & \quad \textbf{foreach}\ c \in \Sigma\ \textbf{do}\ \mathit{bc}[ c ]
///     \gets m \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 2\ \textbf{do}
///     \ \mathit{bc}[ P[ i ] ] \gets m - 1 - i \cr
/// 4 & \quad j \gets 0;\ R \gets \emptyset \cr
/// 5 & \quad \textbf{while}\ j \le n - m\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ T[ j .. j + m - 1 ] = P \text{ (from the right)}
///     \ \textbf{then}\ R \gets R \cup \{ j \} \cr
/// 7 & \qquad j \gets j + \mathit{bc}[ T[ j + m - 1 ] ] \cr
/// 8 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post the occurrences of `naiveMatching( T , P )` (audit)
inline std::vector< std::size_t > horspool( std::string_view T ,
                                            std::string_view P ,
                                            long * comparisons = nullptr );

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by Shift-Or
///
/// In pseudocode (Baeza-Yates and Gonnet, beyond the book), the bit
/// \f$ i \f$ of \f$ D \f$ clear if and only if \f$ P[ 0 .. i ] \f$ ends at
/// \f$ T[ j ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{shiftOr}( T , P ) \cr
/// 2 & \quad \textbf{foreach}\ c \in \Sigma\ \textbf{do}\ B[ c ] \gets
///     1^{64} \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 1\ \textbf{do}
///     \ \text{clear the bit } i \text{ of } B[ P[ i ] ] \cr
/// 4 & \quad D \gets 1^{64};\ R \gets \emptyset \cr
/// 5 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 6 & \qquad D \gets ( D \ll 1 ) \vee B[ T[ j ] ] \cr
/// 7 & \qquad \textbf{if}\ \text{the bit } m - 1 \text{ of } D \text{ is }
///     0\ \textbf{then}\ R \gets R \cup \{ j + 1 - m \} \cr
/// 8 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \pre \f$ m \le 64 \f$
/// \post the occurrences of `naiveMatching( T , P )` (audit)
inline std::vector< std::size_t > shiftOr( std::string_view T ,
                                           std::string_view P );

/// the Z-function: \f$ Z[ i ] \f$ is the length of the longest common
/// prefix of \f$ S \f$ and \f$ S[ i .. ] \f$, and \f$ Z[ 0 ] = |S| \f$
///
/// In pseudocode (Gusfield, section 1.4, beyond the book), \f$ [ l , r ) \f$
/// the Z-box that reaches farthest, i.e., the one with
/// \f$ S[ l .. r - 1 ] = S[ 0 .. r - l - 1 ] \f$; each comparison that
/// succeeds moves \f$ r \f$ forward, hence \f$ \Theta( n ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{zFunction}( S ) \cr
/// 2 & \quad Z[ 0 ] \gets n;\ l \gets 0;\ r \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ i < r\ \textbf{then}\ k \gets \min\{ Z[ i - l ] ,
///     r - i \}\ \textbf{else}\ k \gets 0 \cr
/// 5 & \qquad \textbf{while}\ i + k < n\ \textbf{and}\ S[ k ] = S[ i + k ]
///     \ \textbf{do}\ k \gets k + 1 \cr
/// 6 & \qquad Z[ i ] \gets k \cr
/// 7 & \qquad \textbf{if}\ i + k > r\ \textbf{then}\ l \gets i;\ r \gets i +
///     k \cr
/// 8 & \quad \textbf{return}\ Z
/// \end{array}}
/// \f]
/// \post \f$ Z[ i ] \le |S| - i \f$
inline std::vector< std::size_t > zFunction( std::string_view S );

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by the
/// Z-function of \f$ P \f$ and the Z-boxes of \f$ T \f$
///
/// In pseudocode (Gusfield, section 1.4, beyond the book), the box
/// \f$ [ l , r ) \f$ of \f$ T \f$ with
/// \f$ T[ l .. r - 1 ] = P[ 0 .. r - l - 1 ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{zMatching}( T , P ) \cr
/// 2 & \quad Z \gets \mathrm{zFunction}( P );\ l \gets 0;\ r \gets 0;\ R
///     \gets \emptyset \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ i < r\ \textbf{then}\ k \gets \min\{ Z[ i - l ] ,
///     r - i \}\ \textbf{else}\ k \gets 0 \cr
/// 5 & \qquad \textbf{while}\ i + k < n\ \textbf{and}\ k < m\ \textbf{and}
///     \ T[ i + k ] = P[ k ]\ \textbf{do}\ k \gets k + 1 \cr
/// 6 & \qquad \textbf{if}\ i + k > r\ \textbf{then}\ l \gets i;\ r \gets i +
///     k \cr
/// 7 & \qquad \textbf{if}\ k = m\ \textbf{then}\ R \gets R \cup \{ i \} \cr
/// 8 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post the occurrences of `naiveMatching( T , P )` (audit)
inline std::vector< std::size_t > zMatching( std::string_view T ,
                                             std::string_view P );

/// the places of all the occurrences of \f$ P \f$ in \f$ T \f$, by the
/// Two-Way algorithm of Crochemore and Perrin
///
/// In pseudocode (Crochemore and Perrin, beyond the book), the critical
/// factorization \f$ P = P[ 0 .. \ell ] \, P[ \ell + 1 .. m - 1 ] \f$ with
/// the period \f$ p \f$, the right part compared first, and \f$ \mu \f$ the
/// prefix of \f$ P \f$ known to match after a shift by the period of a
/// periodic pattern (\f$ -1 \f$ otherwise):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{twoWay}( T , P ) \cr
/// 2 & \quad ( \ell , p ) \gets \text{the later of the maximal suffixes of }
///     P \text{ for an order and its reverse, with its period} \cr
/// 3 & \quad \mathit{periodic} \gets P[ 0 .. \ell ] = P[ p .. p + \ell ] \cr
/// 4 & \quad \textbf{if not}\ \mathit{periodic}\ \textbf{then}\ p \gets \max\{
///     \ell + 1 , m - \ell - 1 \} + 1 \cr
/// 5 & \quad j \gets 0;\ \mu \gets -1;\ R \gets \emptyset \cr
/// 6 & \quad \textbf{while}\ j \le n - m\ \textbf{do} \cr
/// 7 & \qquad i \gets \max\{ \ell , \mu \} + 1 \cr
/// 8 & \qquad \textbf{while}\ i < m\ \textbf{and}\ P[ i ] = T[ i + j ]
///     \ \textbf{do}\ i \gets i + 1 \cr
/// 9 & \qquad \textbf{if}\ i < m\ \textbf{then}\ j \gets j + i - \ell;\ \mu
///     \gets -1 \cr
/// 10 & \qquad \textbf{else} \cr
/// 11 & \qquad\quad i \gets \ell \cr
/// 12 & \qquad\quad \textbf{while}\ i > \mu\ \textbf{and}\ P[ i ] = T[ i + j
///     ]\ \textbf{do}\ i \gets i - 1 \cr
/// 13 & \qquad\quad \textbf{if}\ i \le \mu\ \textbf{then}\ R \gets R \cup \{
///     j \} \cr
/// 14 & \qquad\quad j \gets j + p \cr
/// 15 & \qquad\quad \textbf{if}\ \mathit{periodic}\ \textbf{then}\ \mu \gets
///     m - p - 1 \cr
/// 16 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post the occurrences of `naiveMatching( T , P )` (audit)
inline std::vector< std::size_t > twoWay( std::string_view T ,
                                          std::string_view P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/StringMatching.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StringMatching.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StringMatching.h ------------------------*/
/*--------------------------------------------------------------------------*/
