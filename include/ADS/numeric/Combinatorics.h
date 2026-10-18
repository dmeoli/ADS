/*--------------------------------------------------------------------------*/
/*-------------------------- File Combinatorics.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The examples of the introduction and of section 13.4 of chapter 13:
 *
 * - binomial( n , k ) by its recursive definition
 *   \f$ C( n , k ) = C( n - 1 , k - 1 ) + C( n - 1 , k ) \f$, whose calls
 *   are as many as \f$ 2 C( n , k ) - 1 \f$, and tartaglia(), the triangle
 *   of Tartaglia of all the C( m , k ), m <= n, in Theta( n^2 ) time, each
 *   then read in O( 1 );
 * - fibonacci(), F_0 = F_1 = 1 and F_n = F_{n-1} + F_{n-2}, with a vector
 *   of n places and with three variables, O( n ) time and O( 1 ) space,
 *   while the recursion makes about \f$ 1.6^n \f$ calls; FibonacciCache
 *   keeps the numbers already computed between the calls, in a mutable
 *   member, so that a number already asked for costs O( 1 ) and a new one
 *   only the steps from the largest one known;
 * - beyond the book, factorial() and factorialRec(), the iterative and the
 *   recursive realization of the same specification (the functional
 *   abstraction), and binomialByFactorials(), \f$ n! / ( k! ( n - k )! )
 *   \f$, which overflows as soon as n! does, i.e., for n > 20, while the
 *   recursion and the triangle reach n = 67.
 *
 * The contracts [see Contract.h]: the results fit in 64 bits, i.e.,
 * 0 <= k <= n <= 67 for binomial() and the triangle, 0 <= n <= 92 for the
 * numbers of Fibonacci, 0 <= n <= 20 for the factorial and for
 * binomialByFactorials(). The functions are defined in Combinatorics.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#ifndef __ADS_Combinatorics
 #define __ADS_Combinatorics
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// C( n , k ) by the recursion, the calls added to *calls, if given
/// \pre 0 <= k <= n <= 67
inline std::uint64_t binomial( int n , int k , long * calls = nullptr );

/// tartaglia(): the table of C( m , k ), 0 <= k <= m <= n
/// \pre 0 <= n <= 67
inline std::vector< std::vector< std::uint64_t > > tartaglia( int n );

/// the n-th number of Fibonacci, F_0 = F_1 = 1, with a vector
/// \pre 0 <= n <= 92
inline std::uint64_t fibonacciTable( int n );

/// the n-th number of Fibonacci, F_0 = F_1 = 1, in O( 1 ) space
/// \pre 0 <= n <= 92
inline std::uint64_t fibonacci( int n );

/// the numbers of Fibonacci, those computed kept between the calls
class FibonacciCache {
 public:
 /// F_n
 /// \pre 0 <= n <= 92
 /// \post the numbers up to F_n are kept
 [[nodiscard]] std::uint64_t value( int n ) const;

 /// the numbers kept, F_0 , ... , F_{size() - 1}
 [[nodiscard]] int size( void ) const;

 private:
 mutable std::vector< std::uint64_t > f_F = { 1 , 1 };
 };

/// n! by a loop
/// \pre 0 <= n <= 20
inline std::uint64_t factorial( int n );

/// n! by the recursion n! = n ( n - 1 )!, 0! = 1
/// \pre 0 <= n <= 20
inline std::uint64_t factorialRec( int n );

/// C( n , k ) as n! / ( k! ( n - k )! )
/// \pre 0 <= k <= n <= 20
inline std::uint64_t binomialByFactorials( int n , int k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/Combinatorics.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Combinatorics.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Combinatorics.h ------------------------*/
/*--------------------------------------------------------------------------*/
