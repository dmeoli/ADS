/*--------------------------------------------------------------------------*/
/*-------------------------- File MasterTheorem.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The solutions of the recurrences of section 2.9, as orders of growth
 * \f$ \Theta( c^n n^p \log^q n ( \log \log n )^r ) \f$ [see Growth]:
 *
 * - master( a , b , beta , k ) solves
 *   \f$ T( n ) = a T( n / b ) + \Theta( n^\beta \log^k n ) \f$, a >= 1,
 *   b > 1, by the Teorema 2.4: with \f$ \alpha = \log_b a \f$, it is
 *   \f$ \Theta( n^\alpha ) \f$ if \f$ \beta < \alpha \f$,
 *   \f$ \Theta( n^\beta \log^k n ) \f$ if \f$ \beta > \alpha \f$, and for
 *   \f$ \beta = \alpha \f$ \f$ \Theta( n^\alpha \log^{k+1} n ) \f$ if
 *   k > -1, \f$ \Theta( n^\alpha \log \log n ) \f$ if k = -1,
 *   \f$ \Theta( n^\alpha ) \f$ if k < -1; with k = 0 it is the Teorema 2.2;
 * - linear( a , beta ) solves
 *   \f$ T( n ) = \sum_{i=1}^h a_i T( n - i ) + \Theta( n^\beta ) \f$ with
 *   \f$ a = \sum_i a_i \f$ by the Teorema 2.5: \f$ \Theta( n^{\beta+1} ) \f$
 *   if a = 1, and the upper bound \f$ O( a^n n^\beta ) \f$ if a >= 2,
 *   which is not always tight (Fibonacci is \f$ \Theta( \phi^n ) \f$);
 *   the theorem has integer a_i, and for real a_i with 1 < a < 2 (e.g.,
 *   T( n ) = 1.5 T( n - 1 ) + 1, \f$ \Theta( 1.5^n ) \f$) the recurrence
 *   is exponential as well, and linear() gives the same upper bound.
 *
 * Two orders of growth compare as their functions do for n large: first by
 * the base c of the exponential, then by the power p of n, then by the
 * powers q and r of the logarithms, which gives the hierarchy
 * \f$ O( 1 ) \subset O( \log n ) \subset O( n ) \subset O( n \log n )
 * \subset O( n^2 ) \subset O( n^3 ) \subset O( 2^n ) \f$; note that
 * \f$ 2^{\log_2 n} = n \f$ is a polynomial, not an exponential, and that
 * \f$ n^{\log n} \f$ grows faster than every polynomial and slower than
 * every \f$ c^n \f$ with c > 1, so that it has no place in this family.
 *
 * The contracts [see Contract.h]: a >= 1 and b > 1 for master(), a >= 1 for
 * linear(). The functions are defined in MasterTheorem.tpp.
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

/// the order \f$ c^n n^p \log^q n ( \log \log n )^r \f$, and whether it is
/// only an upper bound
struct Growth {
 double c = 1;          ///< the base of the exponential
 double p = 0;          ///< the power of n
 double q = 0;          ///< the power of \f$ \log n \f$
 double r = 0;          ///< the power of \f$ \log \log n \f$
 bool upper = false;    ///< \f$ O \f$ rather than \f$ \Theta \f$

 /// the value at n, for n > e
 [[nodiscard]] double at( double n ) const {
  return( std::pow( c , n ) * std::pow( n , p ) * std::pow( std::log( n ) , q )
          * std::pow( std::log( std::log( n ) ) , r ) );
  }

 /// the order of the two functions for n large: lexicographic on c, p, q
 /// and r (upper is not compared)
 friend std::partial_ordering operator<=>( const Growth & a ,
                                           const Growth & b ) {
  for( auto [ x , y ] : { std::pair( a.c , b.c ) , std::pair( a.p , b.p ) ,
                          std::pair( a.q , b.q ) , std::pair( a.r , b.r ) } )
   if( x != y )
    return( x <=> y );
  return( std::partial_ordering::equivalent );
  }

 friend bool operator==( const Growth & a , const Growth & b ) {
  return( ( a <=> b ) == 0 );
  }
 };

/// \f$ T( n ) = a T( n / b ) + \Theta( n^\beta \log^k n ) \f$;
/// \f$ \log_b a \f$ within 1e-12 of an integer is taken as that integer
/// (\f$ \log_{10} 1000 \f$ is 3, not 2.9999999999999996), and with
/// \f$ \beta = \log_b a \f$ the power is \f$ \beta \f$
/// \pre a >= 1 and b > 1
inline Growth master( double a , double b , double beta , double k = 0 );

/// \f$ T( n ) = \sum_i a_i T( n - i ) + \Theta( n^\beta ) \f$,
/// \f$ a = \sum_i a_i \f$
/// \pre a >= 1
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
