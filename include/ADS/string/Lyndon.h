/*--------------------------------------------------------------------------*/
/*----------------------------- File Lyndon.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Lyndon factorization and the least rotation of a string, beyond the
 * book. A <em>Lyndon word</em> is a nonempty string strictly smaller than
 * all its proper rotations (equivalently, than all its proper suffixes),
 * and every string \f$ S \f$ has one factorization
 * \f[
 *   S = w_1 w_2 \cdots w_k , \qquad w_1 \ge w_2 \ge \cdots \ge w_k ,
 * \f]
 * into Lyndon words (Chen, Fox and Lyndon).
 *
 * - `lyndonFactorization()` is the algorithm of Duval: it keeps
 *   \f$ S = s_1 s_2 s_3 \f$ with \f$ s_1 \f$ factored,
 *   \f$ s_2 = w^h w' \f$ for a Lyndon word \f$ w \f$ and a proper prefix
 *   \f$ w' \f$ of \f$ w \f$, and compares the next character \f$ c \f$ of
 *   \f$ s_3 \f$ with the one \f$ |w| \f$ places before: if equal,
 *   \f$ w' \f$ grows; if larger, \f$ s_2 c \f$ is a Lyndon word and becomes
 *   \f$ w \f$; if smaller, the \f$ h \f$ copies of \f$ w \f$ are factors and
 *   the scan resumes after them. Each comparison either advances the scan
 *   or is followed by the output of a factor at least as long as the
 *   characters rescanned, so that the time is \f$ O( n ) \f$, in
 *   \f$ O( 1 ) \f$ space besides the factors.
 * - `leastRotation()` gives the start of the lexicographically least
 *   rotation of \f$ S \f$ (the first one if several), by the same scan on
 *   \f$ S S \f$: the least rotation starts at the start of the factor of
 *   \f$ S S \f$ that covers the place \f$ n \f$, in \f$ O( n ) \f$ time.
 *
 * The contracts [see `Contract.h`]: the factors are Lyndon words in
 * non-increasing order that give \f$ S \f$ back (an audit), and the
 * rotation is not larger than any other (an audit, \f$ O( n^2 ) \f$). The
 * functions are defined in `Lyndon.tpp`.
 *
 * \see K. T. Chen, R. H. Fox, R. C. Lyndon, Free differential calculus,
 *      IV. The quotient groups of the lower central series, Annals of
 *      Mathematics 68(1), 81-95, 1958, doi:10.2307/1970044
 * \see J.-P. Duval, Factorizing words over an ordered alphabet, Journal of
 *      Algorithms 4(4), 363-381, 1983, doi:10.1016/0196-6774(83)90017-2
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

#ifndef __ADS_Lyndon
 #define __ADS_Lyndon
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether \f$ S \f$ is a Lyndon word, i.e., nonempty and smaller than all
/// its proper suffixes, in \f$ O( n^2 ) \f$ by the definition
[[nodiscard]] inline bool isLyndon( std::string_view S );

/// the starts of the factors of the Lyndon factorization of \f$ S \f$, by
/// Duval
///
/// In pseudocode (Duval's algorithm, beyond the book), \f$ k \f$ the place
/// \f$ |w| \f$ before \f$ j \f$, back to \f$ i \f$ when a longer Lyndon word
/// starts at \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lyndonFactorization}( S ) \cr
/// 2 & \quad i \gets 0;\ F \gets \text{an empty sequence} \cr
/// 3 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 4 & \qquad j \gets i + 1;\ k \gets i \cr
/// 5 & \qquad \textbf{while}\ j < n\ \textbf{and}\ S[ k ] \le S[ j ]
///     \ \textbf{do} \cr
/// 6 & \qquad\quad \textbf{if}\ S[ k ] < S[ j ]\ \textbf{then}\ k \gets i
///     \ \textbf{else}\ k \gets k + 1 \cr
/// 7 & \qquad\quad j \gets j + 1 \cr
/// 8 & \qquad \textbf{while}\ i + ( j - k ) \le j\ \textbf{do}\ \text{append }
///     i \text{ to } F;\ i \gets i + ( j - k ) \cr
/// 9 & \quad \textbf{return}\ F
/// \end{array}}
/// \f]
/// \post Lyndon words, non-increasing, that give \f$ S \f$ back (audit)
[[nodiscard]] inline std::vector< std::size_t > lyndonFactorization(
                                                        std::string_view S );

/// the start of the least rotation of \f$ S \f$, 0 if \f$ S \f$ is empty
///
/// In pseudocode (Duval's algorithm, beyond the book), the scan of
/// `lyndonFactorization()` on \f$ S S \f$, stopped at the factor that
/// covers the place \f$ n \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{leastRotation}( S ) \cr
/// 2 & \quad D \gets S S;\ i \gets 0;\ r \gets 0 \cr
/// 3 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 4 & \qquad r \gets i \cr
/// 5 & \qquad ( w , j ) \gets \text{the length of } w \text{ and the end of }
///     \text{the scan of lines 4 to 7 above on } D \text{ from } i \cr
/// 6 & \qquad \textbf{while}\ i + w \le j\ \textbf{do}\ i \gets i + w \cr
/// 7 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \post no rotation is smaller (audit)
[[nodiscard]] inline std::size_t leastRotation( std::string_view S );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Lyndon.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Lyndon.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Lyndon.h ----------------------------*/
/*--------------------------------------------------------------------------*/
