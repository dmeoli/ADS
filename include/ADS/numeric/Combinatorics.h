/*--------------------------------------------------------------------------*/
/*-------------------------- File Combinatorics.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The examples of the introduction and of section 13.4 of chapter 13:
 *
 * - `binomial()`, \f$ \binom{n}{k} \f$ by its recursive definition
 *   \f$ \binom{n}{k} = \binom{n-1}{k-1} + \binom{n-1}{k} \f$, whose calls are
 *   as many as \f$ 2 \binom{n}{k} - 1 \f$, and `tartaglia()`, the triangle of
 *   Tartaglia of all the \f$ \binom{m}{k} \f$, \f$ m \le n \f$, in
 *   \f$ \Theta( n^2 ) \f$ time, each then read in \f$ O( 1 ) \f$;
 * - `fibonacci()`, \f$ F_0 = F_1 = 1 \f$ and \f$ F_n = F_{n-1} + F_{n-2} \f$,
 *   with a vector of \f$ n \f$ places and with three variables,
 *   \f$ O( n ) \f$ time and \f$ O( 1 ) \f$ space, while the recursion makes
 *   about \f$ 1.6^n \f$ calls; `FibonacciCache` keeps the numbers already
 *   computed between the calls, in a mutable member, so that a number
 *   already asked for costs \f$ O( 1 ) \f$ and a new one only the steps from
 *   the largest one known;
 * - beyond the book, `factorial()` and `factorialRec()`, the iterative and
 *   the recursive realization of the same specification (the functional
 *   abstraction), and `binomialByFactorials()`,
 *   \f$ n! / ( k! ( n - k )! ) \f$, which overflows as soon as \f$ n! \f$
 *   does, i.e., for \f$ n > 20 \f$, while the recursion and the triangle
 *   reach \f$ n = 67 \f$.
 *
 * The contracts [see `Contract.h`]: the results fit in 64 bits, i.e.,
 * \f$ 0 \le k \le n \le 67 \f$ for `binomial()` and the triangle,
 * \f$ 0 \le n \le 92 \f$ for the numbers of Fibonacci, \f$ 0 \le n \le 20 \f$
 * for the factorial and for `binomialByFactorials()`. The functions are
 * defined in `Combinatorics.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, the introduction of
 *      chapter 13 and section 13.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

/// the binomial coefficient \f$ \binom{n}{k} \f$ by the recursion; the calls
/// added to `*calls`, if given
///
/// In pseudocode (the introduction of chapter 13), \f$ 2 \binom{n}{k} - 1 \f$
/// calls, since each
/// leaf returns 1:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binomial}( n , k ) \cr
/// 2 & \quad \textbf{if}\ k = 0\ \textbf{or}\ k = n\ \textbf{then return}\ 1
///     \cr
/// 3 & \quad \textbf{return}\ \mathrm{binomial}( n - 1 , k - 1 ) +
///     \mathrm{binomial}( n - 1 , k )
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le k \le n \le 67 \f$
inline std::uint64_t binomial( int n , int k , long * calls = nullptr );

/// the triangle of Tartaglia, the table of the \f$ \binom{m}{k} \f$,
/// \f$ 0 \le k \le m \le n \f$
///
/// In pseudocode (the introduction of chapter 13), each row from the one
/// before:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tartaglia}( n ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n\ \textbf{do} \cr
/// 3 & \qquad C[ i ][ 0 ] \gets C[ i ][ i ] \gets 1 \cr
/// 4 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ i - 1\ \textbf{do}\ C[
///     i ][ j ] \gets C[ i - 1 ][ j - 1 ] + C[ i - 1 ][ j ] \cr
/// 5 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le n \le 67 \f$
inline std::vector< std::vector< std::uint64_t > > tartaglia( int n );

/// the number \f$ F_n \f$ of Fibonacci, \f$ F_0 = F_1 = 1 \f$, with a vector
///
/// In pseudocode (section 13.4):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fibonacciTable}( n ) \cr
/// 2 & \quad F[ 0 ] \gets F[ 1 ] \gets 1 \cr
/// 3 & \quad \textbf{for}\ i \gets 2\ \textbf{to}\ n\ \textbf{do}\ F[ i ]
///     \gets F[ i - 1 ] + F[ i - 2 ] \cr
/// 4 & \quad \textbf{return}\ F[ n ]
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le n \le 92 \f$
inline std::uint64_t fibonacciTable( int n );

/// the number \f$ F_n \f$ of Fibonacci, \f$ F_0 = F_1 = 1 \f$, in
/// \f$ O( 1 ) \f$ space
///
/// In pseudocode (section 13.4), the last three numbers only:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fibonacci}( n ) \cr
/// 2 & \quad F_0 \gets F_1 \gets F_2 \gets 1 \cr
/// 3 & \quad \textbf{for}\ i \gets 2\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad F_0 \gets F_1;\ F_1 \gets F_2;\ F_2 \gets F_0 + F_1 \cr
/// 5 & \quad \textbf{return}\ F_2
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le n \le 92 \f$
inline std::uint64_t fibonacci( int n );

/// the numbers of Fibonacci, those computed kept between the calls
class FibonacciCache {
 public:
 /// the number \f$ F_n \f$, the ones not kept yet computed from the last
 /// two kept
 ///
 /// In pseudocode, \f$ F \f$ the vector kept, \f$ ( 1 , 1 ) \f$ at first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{value}( n ) \cr
 /// 2 & \quad \textbf{while}\ |F| \le n\ \textbf{do}\ \text{append }
 ///     F[ |F| - 1 ] + F[ |F| - 2 ] \text{ to } F \cr
 /// 3 & \quad \textbf{return}\ F[ n ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le n \le 92 \f$
 /// \post the numbers up to \f$ F_n \f$ are kept
 [[nodiscard]] std::uint64_t value( int n ) const;

 /// how many numbers are kept, \f$ F_0 , \ldots , F_{\mathrm{size}() - 1} \f$
 [[nodiscard]] int size( void ) const;

 private:
 mutable std::vector< std::uint64_t > f_F = { 1 , 1 };   ///< the numbers kept
 };

/// the factorial \f$ n! \f$ by a loop
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{factorial}( n ) \cr
/// 2 & \quad f \gets 1 \cr
/// 3 & \quad \textbf{for}\ i \gets 2\ \textbf{to}\ n\ \textbf{do}\ f \gets f
///     \cdot i \cr
/// 4 & \quad \textbf{return}\ f
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le n \le 20 \f$
inline std::uint64_t factorial( int n );

/// the factorial \f$ n! \f$ by the recursion \f$ n! = n ( n - 1 )! \f$,
/// \f$ 0! = 1 \f$
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{factorialRec}( n ) \cr
/// 2 & \quad \textbf{if}\ n \le 1\ \textbf{then return}\ 1 \cr
/// 3 & \quad \textbf{return}\ n \cdot \mathrm{factorialRec}( n - 1 )
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le n \le 20 \f$
inline std::uint64_t factorialRec( int n );

/// the binomial coefficient \f$ \binom{n}{k} \f$ as
/// \f$ n! / ( k! ( n - k )! ) \f$
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binomialByFactorials}( n , k ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{factorial}( n ) / ( \mathrm{factorial}(
///     k ) \cdot \mathrm{factorial}( n - k ) )
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le k \le n \le 20 \f$
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
