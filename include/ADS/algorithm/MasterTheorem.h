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
 *   which is not always tight (Fibonacci is \f$ \Theta( \phi^n ) \f$).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#ifndef __ADS_MasterTheorem
 #define __ADS_MasterTheorem
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cmath>
#include <stdexcept>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the order c^n n^p log^q n ( log log n )^r, and whether it is only an
/// upper bound
struct Growth {
 double c = 1;          ///< the base of the exponential
 double p = 0;          ///< the power of n
 double q = 0;          ///< the power of log n
 double r = 0;          ///< the power of log log n
 bool upper = false;    ///< O() rather than Theta()

 /// the value at n, for n > e
 [[nodiscard]] double at( double n ) const {
  return( std::pow( c , n ) * std::pow( n , p ) * std::pow( std::log( n ) , q )
          * std::pow( std::log( std::log( n ) ) , r ) );
  }
 };

/// T( n ) = a T( n / b ) + Theta( n^beta log^k n )
inline Growth master( double a , double b , double beta , double k = 0 )
{
 if( ( a < 1 ) || ( b <= 1 ) )
  throw( std::invalid_argument( "ADS::master: a < 1 or b <= 1" ) );
 const double alpha = std::log( a ) / std::log( b );
 const double eps = 1e-12;
 Growth g;
 if( beta < alpha - eps )
  g.p = alpha;
 else if( beta > alpha + eps ) {
  g.p = beta;
  g.q = k;
  }
 else {
  g.p = alpha;
  if( k > -1 + eps )
   g.q = k + 1;
  else if( k > -1 - eps )
   g.r = 1;
  }
 return( g );
 }

/// T( n ) = sum_i a_i T( n - i ) + Theta( n^beta ), a = sum_i a_i
inline Growth linear( double a , double beta )
{
 if( a < 1 )
  throw( std::invalid_argument( "ADS::linear: a < 1" ) );
 Growth g;
 if( a < 2 )
  g.p = beta + 1;
 else {
  g.c = a;
  g.p = beta;
  g.upper = true;
  }
 return( g );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MasterTheorem.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MasterTheorem.h ------------------------*/
/*--------------------------------------------------------------------------*/
