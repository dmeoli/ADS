/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryInteger.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic on integers of n bits, vectors of bits from the least
 * significant one, of section 2.6:
 *
 * - binaryAdd(), the sum with the propagation of the carry, \f$ \Theta( n )
 *   \f$, optimal since every bit must be read;
 * - multiplyBinarySchool(), the product of the school, a partial product
 *   per bit, \f$ \Theta( n^2 ) \f$;
 * - multiplyBinaryFour() and multiplyBinaryKaratsuba(), X = a 2^{n/2} + b and
 *   Y = c 2^{n/2} + d, with four products or with the three of Karatsuba,
 *   \f$ \Theta( n^2 ) \f$ and \f$ \Theta( n^{\log_2 3} ) \f$: the products
 *   of the polynomials of the bits [see Polynomial.h] followed by the
 *   carries;
 * - ahmes(), the multiplication of the papyrus of Ahmes (1650 b.C.), which
 *   adds b to p when a is odd, halves a and doubles b, until a = 0: from
 *   \f$ a b = ( a / 2 ) ( 2 b ) \f$ for a even and
 *   \f$ a b = ( ( a - 1 ) / 2 ) ( 2 b ) + b \f$ for a odd, \f$ O( \log a ) \f$
 *   steps, the shift and add of the arithmetic units.
 *
 * The contracts [see Contract.h]: a vector of bits holds only 0 and 1, an
 * audit on the inputs of the sums and of the products, and fromBits() needs at
 * most 64 of them; toBits() gives at least n bits whose value is x, and
 * ahmes() gives the product modulo 2^64. The functions are defined in
 * BinaryInteger.tpp.
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

/// the bits of x, at least n of them
/// \post at least n bits, of value x
inline Bits toBits( std::uint64_t x , std::size_t n = 0 );

/// the value of the bits, which must be at most 64 significant ones
/// \pre at most 64 bits, each 0 or 1
inline std::uint64_t fromBits( const Bits & b );

/// the coefficients, possibly larger than 1 or negative, made bits by the
/// carries
inline Bits carry( const std::vector< long > & c );

/// X + Y, \f$ \Theta( n ) \f$
/// \pre the items of X and Y are bits
inline Bits binaryAdd( const Bits & X , const Bits & Y );

inline std::vector< long > widen( const Bits & X );

/// X Y by the method of the school, \f$ \Theta( n^2 ) \f$
/// \pre the items of X and Y are bits
inline Bits multiplyBinarySchool( const Bits & X , const Bits & Y ,
                            long * mults = nullptr );

/// X Y by four products of half size, \f$ \Theta( n^2 ) \f$
/// \pre the items of X and Y are bits
inline Bits multiplyBinaryFour( const Bits & X , const Bits & Y ,
                          long * mults = nullptr );

/// X Y by the three products of Karatsuba, \f$ O( n^{1.59} ) \f$
/// \pre the items of X and Y are bits
inline Bits multiplyBinaryKaratsuba( const Bits & X , const Bits & Y ,
                               long * mults = nullptr );

/// the multiplication of Ahmes, in *steps the halvings, if given
/// \post the result is a b modulo 2^64
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
