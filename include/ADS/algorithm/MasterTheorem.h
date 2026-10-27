/*--------------------------------------------------------------------------*/
/*-------------------------- File MasterTheorem.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The solutions of the recurrences of section 2.9, as orders of growth
 * \f$ \Theta( c^n n^p \log^q n ( \log \log n )^r ) \f$ [see `Growth`]:
 *
 * - `master( a , b , beta , k )` solves
 *   \f$ T( n ) = a T( n / b ) + \Theta( n^\beta \log^k n ) \f$,
 *   \f$ a \ge 1 \f$, \f$ b > 1 \f$, by Theorem 2.4: with
 *   \f$ \alpha = \log_b a \f$, it is \f$ \Theta( n^\alpha ) \f$ if
 *   \f$ \beta < \alpha \f$, \f$ \Theta( n^\beta \log^k n ) \f$ if
 *   \f$ \beta > \alpha \f$, and for \f$ \beta = \alpha \f$
 *   \f$ \Theta( n^\alpha \log^{k+1} n ) \f$ if \f$ k > -1 \f$,
 *   \f$ \Theta( n^\alpha \log \log n ) \f$ if \f$ k = -1 \f$,
 *   \f$ \Theta( n^\alpha ) \f$ if \f$ k < -1 \f$; with \f$ k = 0 \f$ it is
 *   Theorem 2.2;
 * - `linear( a , beta )` solves
 *   \f$ T( n ) = \sum_{i=1}^h a_i T( n - i ) + \Theta( n^\beta ) \f$ with
 *   \f$ a = \sum_i a_i \f$ by Theorem 2.5: \f$ \Theta( n^{\beta+1} ) \f$
 *   if \f$ a = 1 \f$, and the upper bound \f$ O( a^n n^\beta ) \f$ if
 *   \f$ a \ge 2 \f$, which is not always tight (Fibonacci is
 *   \f$ \Theta( \phi^n ) \f$); the theorem has integer \f$ a_i \f$, and for
 *   real \f$ a_i \f$ with \f$ 1 < a < 2 \f$ (e.g.,
 *   \f$ T( n ) = 1.5 T( n - 1 ) + 1 \f$, \f$ \Theta( 1.5^n ) \f$) the
 *   recurrence is exponential as well, and `linear()` gives the same upper
 *   bound.
 *
 * Two orders of growth compare as their functions do for \f$ n \f$ large:
 * first by the base \f$ c \f$ of the exponential, then by the power \f$ p \f$
 * of \f$ n \f$, then by the powers \f$ q \f$ and \f$ r \f$ of the logarithms,
 * which gives the hierarchy
 * \f$ O( 1 ) \subset O( \log n ) \subset O( n ) \subset O( n \log n )
 * \subset O( n^2 ) \subset O( n^3 ) \subset O( 2^n ) \f$; note that
 * \f$ 2^{\log_2 n} = n \f$ is a polynomial, not an exponential, and that
 * \f$ n^{\log n} \f$ grows faster than every polynomial and slower than
 * every \f$ c^n \f$ with \f$ c > 1 \f$, so that it has no place in this
 * family.
 *
 * The contracts [see `Contract.h`]: \f$ a \ge 1 \f$ and \f$ b > 1 \f$ for
 * `master()`, \f$ a \ge 1 \f$ for `linear()`. The functions are defined in
 * `MasterTheorem.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#ifndef __ADS_MasterTheorem
 #define __ADS_MasterTheorem
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <compare>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the order \f$ c^n n^p \log^q n ( \log \log n )^r \f$, and whether it
/// is only an upper bound
struct Growth {
 double c = 1;          ///< the base of the exponential
 double p = 0;          ///< the power of \f$ n \f$
 double q = 0;          ///< the power of \f$ \log n \f$
 double r = 0;          ///< the power of \f$ \log \log n \f$
 bool upper = false;    ///< \f$ O \f$ rather than \f$ \Theta \f$

 /// the value of the order at \f$ n \f$, for \f$ n > e \f$
 [[nodiscard]] double at( double n ) const {
  return( std::pow( c , n ) * std::pow( n , p ) * std::pow( std::log( n ) , q )
          * std::pow( std::log( std::log( n ) ) , r ) );
  }

 /// the order of the two functions for \f$ n \f$ large: lexicographic on
 /// \f$ c \f$, \f$ p \f$, \f$ q \f$ and \f$ r \f$ (`upper` is not compared)
 friend std::partial_ordering operator<=>( const Growth & a ,
                                           const Growth & b ) {
  for( auto [ x , y ] : { std::pair( a.c , b.c ) , std::pair( a.p , b.p ) ,
                          std::pair( a.q , b.q ) , std::pair( a.r , b.r ) } )
   if( x != y )
    return( x <=> y );
  return( std::partial_ordering::equivalent );
  }

 /// true if the two orders are the same, i.e., if `operator<=>` finds them
 /// equivalent
 friend bool operator==( const Growth & a , const Growth & b ) {
  return( ( a <=> b ) == 0 );
  }
 };

/// the order of \f$ T( n ) = a T( n / b ) + \Theta( n^\beta \log^k n ) \f$;
/// \f$ \log_b a \f$ within \f$ 10^{-12} \f$ of an integer is taken as that
/// integer (\f$ \log_{10} 1000 \f$ is 3, not 2.9999999999999996), and with
/// \f$ \beta = \log_b a \f$ the power is \f$ \beta \f$
///
/// In pseudocode (Theorem 2.4 of section 2.9):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{master}( a , b , \beta , k ) \cr
/// 2 & \quad \alpha \gets \log_b a \cr
/// 3 & \quad \textbf{if}\ \beta < \alpha\ \textbf{then return}\ \Theta(
///     n^\alpha ) \cr
/// 4 & \quad \textbf{if}\ \beta > \alpha\ \textbf{then return}\ \Theta(
///     n^\beta \log^k n ) \cr
/// 5 & \quad \textbf{if}\ k > -1\ \textbf{then return}\ \Theta( n^\alpha
///     \log^{k+1} n ) \cr
/// 6 & \quad \textbf{if}\ k = -1\ \textbf{then return}\ \Theta( n^\alpha
///     \log \log n ) \cr
/// 7 & \quad \textbf{return}\ \Theta( n^\alpha )
/// \end{array}}
/// \f]
/// \pre \f$ a \ge 1 \f$ and \f$ b > 1 \f$
inline Growth master( double a , double b , double beta , double k = 0 );

/// the order of \f$ T( n ) = \sum_i a_i T( n - i ) + \Theta( n^\beta ) \f$,
/// \f$ a = \sum_i a_i \f$
///
/// In pseudocode (Theorem 2.5 of section 2.9):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{linear}( a , \beta ) \cr
/// 2 & \quad \textbf{if}\ a = 1\ \textbf{then return}\ \Theta( n^{\beta + 1}
///     ) \cr
/// 3 & \quad \textbf{return}\ O( a^n n^\beta )\quad \triangleright
///     \ \text{an upper bound only}
/// \end{array}}
/// \f]
/// \pre \f$ a \ge 1 \f$
inline Growth linear( double a , double beta );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/MasterTheorem.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MasterTheorem.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MasterTheorem.h ------------------------*/
/*--------------------------------------------------------------------------*/
