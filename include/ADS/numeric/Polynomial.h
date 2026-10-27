/*--------------------------------------------------------------------------*/
/*--------------------------- File Polynomial.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The product of two polynomials of \f$ n \f$ coefficients each, the vectors
 * \f$ p[ 0 .. n - 1 ] \f$ and \f$ q[ 0 .. n - 1 ] \f$ from the constant one,
 * into the \f$ 2 n - 1 \f$ coefficients of \f$ r = p q \f$ (section 12.3):
 *
 * - `multiplyNaive()`, \f$ r_{h+k} \mathrel{+}= p_h q_k \f$ for all
 *   \f$ h \f$ and \f$ k \f$, \f$ \Theta( n^2 ) \f$;
 * - `multiplyFour()`, \f$ p = x^{n/2} p_S + p_D \f$ and
 *   \f$ q = x^{n/2} q_S + q_D \f$, the four products \f$ p_S q_S \f$,
 *   \f$ p_S q_D \f$, \f$ p_D q_S \f$, \f$ p_D q_D \f$ recursively,
 *   \f$ T( n ) = 4 T( n / 2 ) + \Theta( n ) = \Theta( n^2 ) \f$ (the
 *   Prodotto4() of section 2.6);
 * - `multiplyKaratsuba()`, with the three products \f$ r_S = p_S q_S \f$,
 *   \f$ r_D = p_D q_D \f$ and \f$ r_M = ( p_S + p_D ) ( q_S + q_D ) \f$,
 *   since \f$ r = x^n r_S + x^{n/2} ( r_M - r_S - r_D ) + r_D \f$, and
 *   \f$ T( n ) = 3 T( n / 2 ) + \Theta( n ) \f$, i.e.,
 *   \f$ T( n ) = \Theta( n^{\log_2 3} ) = O( n^{1.59} ) \f$.
 *
 * The recursive ones split at \f$ h = \lfloor n / 2 \rfloor \f$, so that
 * \f$ n \f$ need not be a power of two, and count in `*mults`, if given, the
 * products of two coefficients.
 *
 * The contracts [see `Contract.h`]: the recursion needs two polynomials of
 * the same degree, and the products of the four and of the three halves equal
 * the product of `multiplyNaive()`, an audit. The functions are defined in
 * `Polynomial.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
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

#ifndef __ADS_Polynomial
 #define __ADS_Polynomial
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the product \f$ p q \f$ by all the products of two coefficients,
/// \f$ \Theta( n^2 ) \f$ (the moltiplicaPolinomi() of the book); their
/// number added to `*mults`, if given
///
/// In pseudocode (section 12.3):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyNaive}( p , q ) \cr
/// 2 & \quad r \gets \text{a vector of } |p| + |q| - 1 \text{ zeros} \cr
/// 3 & \quad \textbf{for}\ h \gets 0\ \textbf{to}\ |p| - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ k \gets 0\ \textbf{to}\ |q| - 1\ \textbf{do} \cr
/// 5 & \qquad\quad r_{h+k} \gets r_{h+k} + p_h q_k \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
template< class T >
std::vector< T > multiplyNaive( const std::vector< T > & p ,
                                const std::vector< T > & q ,
                                long * mults = nullptr );

namespace detail {

/// \f$ r \gets r + \mathit{sign} \cdot x^s a \f$
template< class T >
void addShifted( std::vector< T > & r , const std::vector< T > & a ,
                 std::size_t s , T sign = T( 1 ) );

/// the product of two polynomials of \f$ n \f$ coefficients, by three or
/// four recursive products
template< class T >
std::vector< T > multiplyRec( const std::vector< T > & p ,
                              const std::vector< T > & q , bool three ,
                              long * mults );

/// the product of \f$ p \f$ and \f$ q \f$ padded with zeros to the same
/// length
template< class T >
std::vector< T > multiplySame( std::vector< T > p , std::vector< T > q ,
                               bool three , long * mults );

}  // end( namespace detail )

/// the product \f$ p q \f$ by four recursive products of half size,
/// \f$ \Theta( n^2 ) \f$ (the Prodotto4() of the book on polynomials); the
/// products of two coefficients added to `*mults`, if given
///
/// In pseudocode (section 2.6), \f$ p \f$ and \f$ q \f$ padded with zeros to
/// the same length \f$ n \f$, \f$ h = \lfloor n / 2 \rfloor \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyFour}( p , q ) \cr
/// 2 & \quad \textbf{if}\ n = 1\ \textbf{then return}\ ( p_0 q_0 ) \cr
/// 3 & \quad p_D , q_D \gets \text{the } h \text{ lowest coefficients};
///     \ p_S , q_S \gets \text{the others} \cr
/// 4 & \quad r_S \gets \mathrm{multiplyFour}( p_S , q_S );\ r_D \gets
///     \mathrm{multiplyFour}( p_D , q_D ) \cr
/// 5 & \quad r_M \gets \mathrm{multiplyFour}( p_D , q_S ) +
///     \mathrm{multiplyFour}( p_S , q_D ) \cr
/// 6 & \quad \textbf{return}\ x^{2h} r_S + x^h r_M + r_D
/// \end{array}}
/// \f]
/// \post the result is \f$ p q \f$ (audit)
template< class T >
std::vector< T > multiplyFour( const std::vector< T > & p ,
                               const std::vector< T > & q ,
                               long * mults = nullptr );

/// the product \f$ p q \f$ by the three recursive products of Karatsuba,
/// \f$ O( n^{1.59} ) \f$ (the moltiplicaPolinomiD&I() of the book); the
/// products of two coefficients added to `*mults`, if given
///
/// In pseudocode (section 12.3), \f$ p \f$ and \f$ q \f$ padded with zeros
/// to the same length \f$ n \f$, \f$ h = \lfloor n / 2 \rfloor \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyKaratsuba}( p , q ) \cr
/// 2 & \quad \textbf{if}\ n = 1\ \textbf{then return}\ ( p_0 q_0 ) \cr
/// 3 & \quad p_D , q_D \gets \text{the } h \text{ lowest coefficients};
///     \ p_S , q_S \gets \text{the others} \cr
/// 4 & \quad r_S \gets \mathrm{multiplyKaratsuba}( p_S , q_S );\ r_D \gets
///     \mathrm{multiplyKaratsuba}( p_D , q_D ) \cr
/// 5 & \quad r_M \gets \mathrm{multiplyKaratsuba}( p_S + p_D , q_S + q_D ) \cr
/// 6 & \quad \textbf{return}\ x^{2h} r_S + x^h ( r_M - r_S - r_D ) + r_D
/// \end{array}}
/// \f]
/// \post the result is \f$ p q \f$ (audit)
template< class T >
std::vector< T > multiplyKaratsuba( const std::vector< T > & p ,
                                    const std::vector< T > & q ,
                                    long * mults = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/Polynomial.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Polynomial.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Polynomial.h --------------------------*/
/*--------------------------------------------------------------------------*/
