/*--------------------------------------------------------------------------*/
/*-------------------------- File Segmentations.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The number of ways of writing a sequence \f$ X \f$ of \f$ n \f$ items as
 * a concatenation of words, the counting version of the word break problem
 * [see `WordBreak.h`, which finds one of them].
 *
 * - `countSegmentations()`: \f$ D[ i ] \f$, the number of segmentations of
 *   \f$ X[ i .. n ) \f$, is the sum of \f$ D[ i + |w| ] \f$ over the words
 *   \f$ w \f$ of the list that occur in \f$ X \f$ at \f$ i \f$, and
 *   \f$ D[ n ] = 1 \f$, filled from the right in
 *   \f$ O( n \sum_w |w| ) \f$ time and \f$ O( n ) \f$ space. With the
 *   automaton of Aho and Corasick on the words, the occurrences ending at
 *   each place come in one scan, and the time drops to
 *   \f$ O( n + \sum_w |w| + \mathit{occ} ) \f$ for \f$ \mathit{occ} \f$
 *   occurrences.
 * - `countDecodings()`: the readings of a string of digits as letters,
 *   \f$ A = 1 , \ldots , Z = 26 \f$, i.e., the segmentations into the words
 *   \f$ 1 , \ldots , 26 \f$; \f$ D[ i ] = 0 \f$ if \f$ d_i = 0 \f$, else
 *   \f$ D[ i + 1 ] \f$ plus \f$ D[ i + 2 ] \f$ if \f$ d_i d_{i+1} \f$ is in
 *   \f$ 10 .. 26 \f$, in \f$ \Theta( n ) \f$ time and \f$ O( 1 ) \f$ space,
 *   the two last entries of \f$ D \f$. On \f$ n \f$ digits 1 the count is
 *   the Fibonacci number \f$ F_{n+1} \f$.
 *
 * The counts grow exponentially with \f$ n \f$, and they are given modulo
 * \f$ 2^{64} \f$. `countSegmentations()` takes any sequence `S` with
 * `size()`, `operator[]` and the comparison of its items; the overload on
 * `std::string` also takes string literals.
 *
 * The contracts [see `Contract.h`]: no word is empty, and the digits are in
 * \f$ 0 .. 9 \f$. The functions are defined in `Segmentations.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see A. V. Aho, M. J. Corasick, Efficient string matching: an aid to
 *      bibliographic search, Communications of the ACM 18(6), 333-340,
 *      1975, doi:10.1145/360825.360855
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

#ifndef __ADS_Segmentations
 #define __ADS_Segmentations
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the number of ways of writing \f$ X \f$ as a sequence of the words,
/// modulo \f$ 2^{64} \f$; a word repeated in the list counts once for each
/// copy; 1 for the empty \f$ X \f$
///
/// In pseudocode (chapter 13), the table filled from the right:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countSegmentations}( X , W )\quad
///     \triangleright\ D[ i ] \text{ the segmentations of } X[ i .. n ) \cr
/// 2 & \quad D[ n ] \gets 1 \cr
/// 3 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 0\ \textbf{do}
///     \ D[ i ] \gets \textstyle\sum \{ D[ i + |w| ] : w \in W \text{, } w
///     \text{ a prefix of } X[ i .. n ) \} \cr
/// 4 & \quad \textbf{return}\ D[ 0 ]
/// \end{array}}
/// \f]
/// \pre no word is empty
template< class S >
 requires std::is_class_v< S >
[[nodiscard]] std::uint64_t
countSegmentations( const S & X , const std::vector< S > & words );

/// the count of `countSegmentations()` above, on `std::string` (or
/// literals)
[[nodiscard]] inline std::uint64_t
countSegmentations( const std::string & X ,
		    const std::vector< std::string > & words );

/// the number of readings of the digits as letters
/// \f$ A = 1 , \ldots , Z = 26 \f$, modulo \f$ 2^{64} \f$; 1 for no digits
///
/// In pseudocode (chapter 13), \f$ \mathit{next} = D[ i + 1 ] \f$ and
/// \f$ \mathit{after} = D[ i + 2 ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countDecodings}( d )\quad \triangleright
///     \ \mathit{next} , \mathit{after}
///     \text{ the counts of the suffixes from } i + 1 , i + 2 \cr
/// 2 & \quad \mathit{next} \gets 1;\ \mathit{after} \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
/// 4 & \qquad h \gets 0\quad \triangleright\ \text{a } 0
///     \text{ starts no letter} \cr
/// 5 & \qquad \textbf{if}\ d_i \ne 0\ \textbf{then}\ h \gets \mathit{next} +
///     ( \mathit{after} \text{ if } i + 1 < n \text{ and } 10 d_i + d_{i+1}
///     \le 26 \text{, else } 0 ) \cr
/// 6 & \qquad ( \mathit{after} , \mathit{next} ) \gets ( \mathit{next} , h )
///     \cr
/// 7 & \quad \textbf{return}\ \mathit{next}
/// \end{array}}
/// \f]
/// \pre every digit is in \f$ 0 .. 9 \f$
[[nodiscard]] inline std::uint64_t
countDecodings( const std::vector< int > & digits );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Segmentations.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Segmentations.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Segmentations.h ------------------------*/
/*--------------------------------------------------------------------------*/
