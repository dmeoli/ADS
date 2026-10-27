/*--------------------------------------------------------------------------*/
/*-------------------------- File HashFunctions.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash functions of section 7.1.
 *
 * A hash function \f$ H : U \rightarrow \{ 0 , \ldots , m - 1 \} \f$ maps a
 * key to a place of a vector of size \f$ m \f$; it has the simple uniformity
 * if
 * \f[
 *   Q( i ) = \sum_{k : H( k ) = i} P( k ) = \frac{1}{m}
 *   \qquad \forall i \in \{ 0 , \ldots , m - 1 \}
 * \f]
 * where \f$ P( k ) \f$ is the probability that the key \f$ k \f$ is in the
 * dictionary. A key is read as the string \f$ \mathrm{bin}( k ) \f$ of its
 * bits, the bytes of a string one after the other from the first, the most
 * significant bit of each first, and \f$ \mathrm{int}( b ) \f$ is the
 * integer of the bit string \f$ b \f$. The functions are
 *
 * - the extraction, \f$ H( k ) = \mathrm{int}( b ) \f$ with \f$ b \f$ a
 *   block of \f$ p \f$ bits of \f$ \mathrm{bin}( k ) \f$ [see
 *   `hash_extraction()`];
 * - the XOR, \f$ H( k ) = \mathrm{int}( \bigoplus_i b_i ) \f$ of the blocks
 *   of \f$ p \f$ bits of \f$ \mathrm{bin}( k ) \f$, the last one padded with
 *   zeros [see `hash_xor()`];
 * - the division,
 *   \f$ H( k ) = \mathrm{int}( \mathrm{bin}( k ) ) \bmod m \f$, computed
 *   with the rule of Horner and a mod at each character so that nothing
 *   overflows [see `hash_division()`];
 * - the multiplication,
 *   \f$ H( k ) = \lfloor m ( C i - \lfloor C i \rfloor ) \rfloor \f$ with
 *   \f$ i = \mathrm{int}( \mathrm{bin}( k ) ) \f$ and
 *   \f$ C = ( \sqrt{5} - 1 ) / 2 \f$, the conjugate of the golden ratio
 *   [see `hash_multiplication()`], and its version with integers for
 *   \f$ m = 2^p \f$: with \f$ s = \lfloor C 2^w \rfloor \f$ and
 *   \f$ i s = r_1 2^w + r_0 \f$, \f$ H( k ) \f$ is made of the \f$ p \f$
 *   most significant bits of \f$ r_0 \f$ [see `hash_fibonacci()`];
 * - the middle square, the \f$ p \f$ bits in the middle of \f$ i^2 \f$ [see
 *   `hash_midsquare()`].
 *
 * The contracts [see `Contract.h`]: the number \f$ p \f$ of bits in its
 * range, a key of at most 8 characters for `int_bin()`, and for
 * `hash_division()` a size \f$ m > 0 \f$ and a base \f$ > 0 \f$ such that the
 * rule of Horner does not overflow; each result is below the size it is
 * meant for. The functions are defined in `HashFunctions.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 7.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 6.4
 * \see R. Morris, Scatter storage techniques, Communications of the ACM
 *      11(1), 38-44, 1968, doi:10.1145/362851.362882
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

#ifndef __ADS_HashFunctions
 #define __ADS_HashFunctions
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- HASH FUNCTIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the bit of place \f$ j \f$ of \f$ \mathrm{bin}( k ) \f$, counted from
/// the left; 0 past the end
unsigned bin_bit( std::string_view k , std::size_t j );

/// \f$ \mathrm{int}( \mathrm{bin}( k ) ) \f$ of a key of at most 8
/// characters
/// \pre \f$ k \f$ has at most 8 characters
std::uint64_t int_bin( std::string_view k );

/// the extraction: the \f$ p \f$ bits of \f$ \mathrm{bin}( k ) \f$ from the
/// place \f$ s \f$ on, counted from the left, as an integer in
/// \f$ [ 0 , 2^p ) \f$
///
/// In pseudocode (section 7.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extraction}( k , p , s ) \cr
/// 2 & \quad b \gets 0 \cr
/// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ p - 1\ \textbf{do}\ b
///     \gets 2 b + \mathrm{bin}( k )[ s + j ] \cr
/// 4 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le p \le 63 \f$
/// \post the result is below \f$ 2^p \f$
std::uint64_t hash_extraction( std::string_view k , int p , std::size_t s );

/// the XOR of the blocks of \f$ p \f$ bits of \f$ \mathrm{bin}( k ) \f$, the
/// last one padded with zeros on the right
///
/// In pseudocode (section 7.1), \f$ \ell \f$ the length of
/// \f$ \mathrm{bin}( k ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{xor}( k , p ) \cr
/// 2 & \quad b \gets 0;\ s \gets 0 \cr
/// 3 & \quad \textbf{while}\ s < \ell\ \textbf{do}\ b \gets b \oplus
///     \mathrm{extraction}( k , p , s );\ s \gets s + p \cr
/// 4 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le p \le 63 \f$
/// \post the result is below \f$ 2^p \f$
std::uint64_t hash_xor( std::string_view k , int p );

/// the division \f$ \mathrm{int}( \mathrm{bin}( k ) ) \bmod m \f$ by the rule
/// of Horner in the given base, each character being a digit, from the
/// leading digit `seed` (the \f$ h = 5 \f$, \f$ h = 3 h + c \f$ is base 3
/// and seed 5); a character is any byte, up to 255 also in a base below 256
///
/// In pseudocode (section 7.1), \f$ \beta \f$ the base and \f$ c_1 , \ldots
/// , c_\ell \f$ the characters of \f$ k \f$; taking the remainder at each
/// step gives the same result, since \f$ ( x \bmod m ) \beta + c \equiv x
/// \beta + c \pmod{m} \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{division}( k , m , \beta ,
///     \mathit{seed} ) \cr
/// 2 & \quad b \gets \mathit{seed} \bmod m \cr
/// 3 & \quad \textbf{for}\ j \gets 1\ \textbf{to}\ \ell\ \textbf{do}\ b
///     \gets ( b \beta + c_j ) \bmod m \cr
/// 4 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
/// \pre \f$ m > 0 \f$, \f$ \beta > 0 \f$, and \f$ ( m - 1 ) \beta + 255 \f$
///      fits in 64 bits
/// \post the result is below \f$ m \f$
std::uint64_t hash_division( std::string_view k , std::uint64_t m ,
                             std::uint64_t base = 256 ,
                             std::uint64_t seed = 0 );

/// the multiplication in floating point, the formula of the book,
/// \f$ \lfloor m ( C i - \lfloor C i \rfloor ) \rfloor \f$: exact only
/// while \f$ C i \f$ keeps enough digits after the point, i.e., for
/// \f$ i \f$ well below the \f$ 2^{64} \f$ of the mantissa of a
/// `long double`
/// \pre \f$ 0 \le C < 1 \f$
/// \post the result is below \f$ m \f$, if \f$ m > 0 \f$
std::uint64_t hash_multiplication( std::uint64_t i , std::uint64_t m ,
                                   long double C =
                                    ( std::sqrt( 5.0L ) - 1 ) / 2 );

/// the multiplication with integers for \f$ m = 2^p \f$, \f$ w = 64 \f$:
/// the \f$ p \f$ most significant bits of the 64 least significant ones of
/// \f$ i s \f$, with \f$ s = \lfloor C 2^{64} \rfloor \f$; it is
/// \f$ \lfloor m \, \mathrm{frac}( i s / 2^{64} ) \rfloor \f$, which differs
/// from the exact \f$ \lfloor m \, \mathrm{frac}( C i ) \rfloor \f$ by about
/// \f$ m i / 2^{64} \f$
///
/// In pseudocode (section 7.1, the version of Knuth, section 6.4 of TAOCP):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fibonacci}( i , p ) \cr
/// 2 & \quad s \gets \lfloor C 2^{64} \rfloor\quad \triangleright
///     \ \text{the conjugate of the golden ratio, scaled} \cr
/// 3 & \quad r_0 \gets i s \bmod 2^{64} \cr
/// 4 & \quad \textbf{return}\ \lfloor r_0 / 2^{64 - p} \rfloor
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le p \le 63 \f$
/// \post the result is below \f$ 2^p \f$
std::uint64_t hash_fibonacci( std::uint64_t i , int p );

/// the middle square: the \f$ p \f$ bits in the middle of the 64 least
/// significant ones of \f$ i^2 \f$
///
/// In pseudocode (section 7.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{midsquare}( i , p ) \cr
/// 2 & \quad q \gets i^2 \bmod 2^{64} \cr
/// 3 & \quad \textbf{return}\ \lfloor q / 2^{32 - \lfloor p / 2 \rfloor}
///     \rfloor \bmod 2^p
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le p \le 32 \f$
/// \post the result is below \f$ 2^p \f$
std::uint64_t hash_midsquare( std::uint64_t i , int p );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/HashFunctions.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HashFunctions.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashFunctions.h ------------------------*/
/*--------------------------------------------------------------------------*/
