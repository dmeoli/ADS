/*--------------------------------------------------------------------------*/
/*----------------------------- File Entropy.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The empirical entropies against which the compressors of this folder are
 * measured. The entropy of order 0 of a distribution \f$ P \f$ on an
 * alphabet \f$ \Sigma \f$ is
 * \f$ H_0 = \sum_{\sigma \in \Sigma} P[ \sigma ] \log_2( 1 / P[ \sigma ] ) \f$
 * bits per symbol, and that of a string \f$ s \f$ takes
 * \f$ P[ \sigma ] = n_\sigma / |s| \f$, with \f$ n_\sigma \f$ the number of
 * occurrences of \f$ \sigma \f$ in \f$ s \f$; it bounds from below the
 * average length of a prefix-free code (Shannon), and the code of Huffman
 * stays within one bit of it, \f$ H_0 \le \bar\ell < H_0 + 1 \f$ [see
 * `CanonicalHuffman.h`], while the arithmetic code of \f$ s \f$ takes at most
 * \f$ 2 + |s| H_0 \f$ bits [see `ArithmeticCoding.h`]. The entropy of order
 * \f$ k \f$ of \f$ s \f$ is
 * \f$ H_k( s ) = \frac{1}{|s|} \sum_{w \in \Sigma^k} |w_s| H_0( w_s ) \f$,
 * where \f$ w_s \f$ is the string of the symbols that follow the
 * occurrences of \f$ w \f$ in \f$ s \f$, in their order; one has
 * \f$ H_k \le H_0 \f$, and \f$ H_k \f$ is the measure reached by the models
 * of context [see `PPM.h`] and by the compressors built on the
 * Burrows-Wheeler transform [see `BurrowsWheeler.h`]. Each function takes
 * \f$ O( |s| \log |s| ) \f$ time, the counts being kept in ordered maps.
 *
 * The contracts [see `Contract.h`]: the frequencies are not negative and
 * not all 0, and \f$ H_k( s ) \le H_0( s ) \f$ is checked on the result. The
 * functions are defined in `Entropy.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 12, Statistical Coding, and 14, Block-Sorting
 *      Compression, doi:10.1017/9781009128933
 * \see C. E. Shannon, A mathematical theory of communication, Bell System
 *      Technical Journal 27(3), 379-423, 1948,
 *      doi:10.1002/j.1538-7305.1948.tb01338.x
 * \see G. Manzini, An analysis of the Burrows-Wheeler transform, Journal
 *      of the ACM 48(3), 407-430, 2001, doi:10.1145/382780.382782
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

#ifndef __ADS_Entropy
 #define __ADS_Entropy
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the entropy of order 0, in bits per symbol, of the distribution
/// proportional to the frequencies \f$ f \f$ of `freq`
///
/// In pseudocode (Shannon's entropy, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{entropy}( f ) \cr
/// 2 & \quad N \gets \sum_\sigma f[ \sigma ];\ h \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ \sigma\ \textbf{with}\ f[ \sigma ] > 0
///     \ \textbf{do} \cr
/// 4 & \qquad p \gets f[ \sigma ] / N;\ h \gets h - p \log_2 p \cr
/// 5 & \quad \textbf{return}\ h
/// \end{array}}
/// \f]
/// \pre no frequency is negative, and not all are 0
template< class C , class F >
[[nodiscard]] double entropy( const std::map< C , F > & freq );

/// the empirical entropy of order 0 of \f$ s \f$, in bits per symbol; 0 if
/// \f$ s \f$ is empty
///
/// It is the entropy above of the counts of the characters of \f$ s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{entropy}( s ) \cr
/// 2 & \quad \textbf{foreach}\ i \gets 0\ \textbf{to}\ |s| - 1\ \textbf{do}
///     \ f[ s[ i ] ] \gets f[ s[ i ] ] + 1 \cr
/// 3 & \quad \textbf{return}\ \mathrm{entropy}( f )
/// \end{array}}
/// \f]
[[nodiscard]] inline double entropy( std::string_view s );

/// the empirical entropy of order \f$ k \f$ of \f$ s \f$, in bits per symbol;
/// 0 if \f$ s \f$ is empty
///
/// In pseudocode (Manzini's definition, beyond the book), \f$ f_w \f$ the
/// counts of the characters that follow the context \f$ w \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{entropy}( s , k ) \cr
/// 2 & \quad \textbf{foreach}\ i \gets k\ \textbf{to}\ |s| - 1\ \textbf{do}
///     \cr
/// 3 & \qquad w \gets s[ i - k .. i - 1 ];\ f_w[ s[ i ] ] \gets f_w[ s[ i ] ]
///     + 1 \cr
/// 4 & \quad h \gets 0 \cr
/// 5 & \quad \textbf{foreach}\ \text{context } w \text{ met}\ \textbf{do}
///     \ h \gets h + |f_w| \cdot \mathrm{entropy}( f_w )
///     \quad \triangleright\ |f_w| = \sum_c f_w[ c ] \cr
/// 6 & \quad \textbf{return}\ h / |s|
/// \end{array}}
/// \f]
/// \post the result is at most `entropy( s )`, up to the rounding
[[nodiscard]] inline double entropy( std::string_view s , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/Entropy.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Entropy.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Entropy.h ---------------------------*/
/*--------------------------------------------------------------------------*/
