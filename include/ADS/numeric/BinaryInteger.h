/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryInteger.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic on integers of \f$ n \f$ bits, vectors of bits from the
 * least significant one, of section 2.6:
 *
 * - `binaryAdd()`, the sum with the propagation of the carry,
 *   \f$ \Theta( n ) \f$, optimal since every bit must be read;
 * - `multiplyBinarySchool()`, the product of the school, a partial product
 *   per bit, \f$ \Theta( n^2 ) \f$;
 * - `multiplyBinaryFour()` and `multiplyBinaryKaratsuba()`, with
 *   \f$ X = a 2^{n/2} + b \f$ and \f$ Y = c 2^{n/2} + d \f$, by four products
 *   of half size or by the three of Karatsuba, \f$ \Theta( n^2 ) \f$ and
 *   \f$ \Theta( n^{\log_2 3} ) \f$: the products of the polynomials of the
 *   bits [see `Polynomial.h`] followed by the carries;
 * - `ahmes()`, the multiplication of the papyrus of Ahmes (1650 BC), which
 *   adds \f$ b \f$ to \f$ p \f$ when \f$ a \f$ is odd, halves \f$ a \f$ and
 *   doubles \f$ b \f$, until \f$ a = 0 \f$: from
 *   \f$ a b = ( a / 2 ) ( 2 b ) \f$ for \f$ a \f$ even and
 *   \f$ a b = ( ( a - 1 ) / 2 ) ( 2 b ) + b \f$ for \f$ a \f$ odd,
 *   \f$ O( \log a ) \f$ steps, the shift and add of the arithmetic units.
 *
 * The contracts [see `Contract.h`]: a vector of bits holds only 0 and 1, an
 * audit on the inputs of the sums and of the products, and `fromBits()` needs
 * at most 64 of them; `toBits()` gives at least \f$ n \f$ bits whose value is
 * \f$ x \f$, and `ahmes()` gives the product modulo \f$ 2^{64} \f$. The
 * functions are defined in `BinaryInteger.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.4 and 2.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see A. Karatsuba, Yu. Ofman, Multiplication of multidigit numbers on
 *      automata, Soviet Physics Doklady 7, 595-596, 1963
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

#ifndef __ADS_BinaryInteger
 #define __ADS_BinaryInteger
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/Polynomial.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Bits = std::vector< int >;   ///< the bits, the least significant first

/// the bits of \f$ x \f$, at least \f$ n \f$ of them
///
/// In pseudocode, the remainders of the halvings:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{toBits}( x , n ) \cr
/// 2 & \quad b \gets \text{the empty vector} \cr
/// 3 & \quad \textbf{while}\ x > 0\ \textbf{or}\ |b| < n\ \textbf{do} \cr
/// 4 & \qquad \text{append } x \bmod 2 \text{ to } b;\ x \gets \lfloor x / 2
///     \rfloor \cr
/// 5 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
/// \post at least \f$ n \f$ bits, of value \f$ x \f$
inline Bits toBits( std::uint64_t x , std::size_t n = 0 );

/// the value \f$ \sum_i b_i 2^i \f$ of the bits, which must be at most 64
///
/// In pseudocode, by the rule of Horner from the most significant bit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fromBits}( b ) \cr
/// 2 & \quad x \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets |b| - 1\ \textbf{downto}\ 0\ \textbf{do}
///     \ x \gets 2 x + b_i \cr
/// 4 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre at most 64 bits, each 0 or 1
inline std::uint64_t fromBits( const Bits & b );

/// the coefficients \f$ c_i \f$, possibly larger than 1 or negative, made
/// bits of the same value \f$ \sum_i c_i 2^i \f$ by the carries
///
/// In pseudocode, the carry \f$ r \f$ an integer of any sign:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{carry}( c ) \cr
/// 2 & \quad r \gets 0;\ i \gets 0;\ b \gets \text{the empty vector} \cr
/// 3 & \quad \textbf{while}\ i < |c|\ \textbf{or}\ r \ne 0\ \textbf{do} \cr
/// 4 & \qquad v \gets r + c_i\quad \triangleright\ c_i = 0 \text{ for }
///     i \ge |c| \cr
/// 5 & \qquad \text{append } v \bmod 2 \in \{ 0 , 1 \} \text{ to } b;
///     \ r \gets ( v - v \bmod 2 ) / 2;\ i \gets i + 1 \cr
/// 6 & \quad \text{drop the most significant zeros of } b \text{, but one} \cr
/// 7 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
inline Bits carry( const std::vector< long > & c );

/// the sum \f$ X + Y \f$, \f$ \Theta( n ) \f$
///
/// In pseudocode (section 2.6), \f$ X_i = Y_i = 0 \f$ beyond their bits:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binaryAdd}( X , Y ) \cr
/// 2 & \quad r \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ \max( |X| , |Y| ) - 1
///     \ \textbf{do} \cr
/// 4 & \qquad s \gets X_i + Y_i + r;\ Z_i \gets s \bmod 2;\ r \gets \lfloor
///     s / 2 \rfloor \cr
/// 5 & \quad \textbf{if}\ r = 1\ \textbf{then}\ \text{append } r \text{ to }
///     Z \cr
/// 6 & \quad \textbf{return}\ Z
/// \end{array}}
/// \f]
/// \pre the items of \f$ X \f$ and \f$ Y \f$ are bits (audit)
inline Bits binaryAdd( const Bits & X , const Bits & Y );

/// the bits of \f$ X \f$ as the coefficients of a polynomial in
/// \f$ x = 2 \f$
inline std::vector< long > widen( const Bits & X );

/// the product \f$ X Y \f$ by the method of the school,
/// \f$ \Theta( n^2 ) \f$; the products of two bits added to `*mults`, if
/// given
///
/// In pseudocode (section 2.6), the bits as the coefficients of polynomials
/// in \f$ x = 2 \f$ [see `multiplyNaive()`]:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyBinarySchool}( X , Y ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{carry}( \mathrm{multiplyNaive}( X , Y
///     ) )
/// \end{array}}
/// \f]
/// \pre the items of \f$ X \f$ and \f$ Y \f$ are bits (audit)
inline Bits multiplyBinarySchool( const Bits & X , const Bits & Y ,
                            long * mults = nullptr );

/// the product \f$ X Y \f$ by four products of half size,
/// \f$ \Theta( n^2 ) \f$; the products of two bits added to `*mults`, if
/// given
///
/// In pseudocode (section 2.6) [see `multiplyFour()`]:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyBinaryFour}( X , Y ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{carry}( \mathrm{multiplyFour}( X , Y )
///     )
/// \end{array}}
/// \f]
/// \pre the items of \f$ X \f$ and \f$ Y \f$ are bits (audit)
inline Bits multiplyBinaryFour( const Bits & X , const Bits & Y ,
                          long * mults = nullptr );

/// the product \f$ X Y \f$ by the three products of Karatsuba,
/// \f$ O( n^{1.59} ) \f$; the products of two bits added to `*mults`, if
/// given
///
/// In pseudocode (section 2.6) [see `multiplyKaratsuba()`]:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyBinaryKaratsuba}( X , Y ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{carry}( \mathrm{multiplyKaratsuba}( X ,
///     Y ) )
/// \end{array}}
/// \f]
/// \pre the items of \f$ X \f$ and \f$ Y \f$ are bits (audit)
inline Bits multiplyBinaryKaratsuba( const Bits & X , const Bits & Y ,
                               long * mults = nullptr );

/// the product \f$ a b \f$ by the multiplication of Ahmes; the halvings
/// added to `*steps`, if given
///
/// In pseudocode (section 1.4), \f$ p + a b \f$ the same at each step:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{ahmes}( a , b ) \cr
/// 2 & \quad p \gets 0 \cr
/// 3 & \quad \textbf{while}\ a > 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ a \text{ is odd}\ \textbf{then}\ p \gets p + b \cr
/// 5 & \qquad a \gets \lfloor a / 2 \rfloor;\ b \gets 2 b \cr
/// 6 & \quad \textbf{return}\ p
/// \end{array}}
/// \f]
/// \post the result is \f$ a b \bmod 2^{64} \f$
inline std::uint64_t ahmes( std::uint64_t a , std::uint64_t b ,
                            long * steps = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/BinaryInteger.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BinaryInteger.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinaryInteger.h ------------------------*/
/*--------------------------------------------------------------------------*/
