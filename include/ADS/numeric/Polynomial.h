/*--------------------------------------------------------------------------*/
/*--------------------------- File Polynomial.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The product of two polynomials of n coefficients each, the vectors
 * p[ 0 .. n - 1 ] and q[ 0 .. n - 1 ] from the constant one, into the
 * 2 n - 1 coefficients of r = p q (section 12.3):
 *
 * - multiplyNaive(), \f$ r_{h+k} \mathrel{+}= p_h q_k \f$ for all h and k,
 *   \f$ \Theta( n^2 ) \f$;
 * - multiplyFour(), p = x^{n/2} p_S + p_D and q = x^{n/2} q_S + q_D, the
 *   four products \f$ p_S q_S \f$, \f$ p_S q_D \f$, \f$ p_D q_S \f$,
 *   \f$ p_D q_D \f$ recursively, \f$ T( n ) = 4 T( n / 2 ) + \Theta( n ) =
 *   \Theta( n^2 ) \f$ (the Prodotto4() of section 2.6);
 * - multiplyKaratsuba(), with the three products \f$ r_S = p_S q_S \f$,
 *   \f$ r_D = p_D q_D \f$ and \f$ r_M = ( p_S + p_D ) ( q_S + q_D ) \f$,
 * \f[
 *   r = x^n r_S + x^{n/2} ( r_M - r_S - r_D ) + r_D ,
 * \f]
 *   \f$ T( n ) = 3 T( n / 2 ) + \Theta( n ) = \Theta( n^{\log_2 3} ) =
 *   O( n^{1.59} ) \f$.
 *
 * The recursive ones split at h = \f$ \lfloor n / 2 \rfloor \f$, so that n
 * need not be a power of two, and count in *mults, if given, the products
 * of two coefficients.
 *
 * The contracts [see Contract.h]: the recursion needs two polynomials of the
 * same degree, and the products of the four and of the three halves equal the
 * product of multiplyNaive(), an audit. The functions are defined in
 * Polynomial.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see A. Karatsuba, Yu. Ofman, Multiplication of multidigit numbers on
 *      automata, Soviet Physics Doklady 7, 595-596, 1963
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// moltiplicaPolinomi(): Theta( n^2 )
template< class T >
std::vector< T > multiplyNaive( const std::vector< T > & p ,
                                const std::vector< T > & q ,
                                long * mults = nullptr );

namespace detail {

/// r += x^s a
template< class T >
void addShifted( std::vector< T > & r , const std::vector< T > & a ,
                 std::size_t s , T sign = T( 1 ) );

/// the product of two polynomials of n coefficients, by three or four
/// recursive products
template< class T >
std::vector< T > multiplyRec( const std::vector< T > & p ,
                              const std::vector< T > & q , bool three ,
                              long * mults );

/// p and q padded with zeros to the same length
template< class T >
std::vector< T > multiplySame( std::vector< T > p , std::vector< T > q ,
                               bool three , long * mults );

}  // end( namespace detail )

/// Prodotto4() on polynomials: four recursive products, Theta( n^2 )
/// \post the result is p q
template< class T >
std::vector< T > multiplyFour( const std::vector< T > & p ,
                               const std::vector< T > & q ,
                               long * mults = nullptr );

/// moltiplicaPolinomiD&I(): three recursive products, O( n^1.59 )
/// \post the result is p q
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
