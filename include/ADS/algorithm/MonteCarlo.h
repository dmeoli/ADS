/*--------------------------------------------------------------------------*/
/*--------------------------- File MonteCarlo.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Monte Carlo algorithms of chapter 17, quick and wrong with a bounded
 * probability, which K independent runs lower to its K-th power:
 *
 * - zeroPolynomial(), whether a polynomial of total degree d, given as a
 *   function of its n variables, is identically 0: by the lemma of Schwartz
 *   and Zippel a random point of { 1 , ... , 2 d }^n is a root of a
 *   nonzero one with probability at most 1/2; "zero" may be wrong, "not
 *   zero" never;
 * - freivalds(), whether A B = C for n x n matrices: A ( B x ) against C x
 *   for a random x of +1 and -1, O( n^2 ) per run instead of the product,
 *   wrong with probability at most 1/2 when A B != C (exercise 17.4);
 * - halfSatisfiable(), whether a boolean formula of n variables is true on
 *   more than half of the assignments, from the fraction of the true ones
 *   among K random assignments (exercise 17.5): by the bound of Hoeffding
 *   it errs with probability at most \f$ e^{-2 K \epsilon^2} \f$ when the
 *   true fraction is 1/2 + epsilon or 1/2 - epsilon.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 17 and
 *      exercises 17.4 and 17.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see J. T. Schwartz, Fast probabilistic algorithms for verification of
 *      polynomial identities, Journal of the ACM 27(4), 701-717, 1980,
 *      doi:10.1145/322217.322225
 * \see R. Freivalds, Probabilistic machines can use less running time,
 *      IFIP Congress 1977, 839-842
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

#ifndef __ADS_MonteCarlo
 #define __ADS_MonteCarlo
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <random>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether p, of n variables and total degree d, is identically 0, by K
/// random points
template< class F , class R >
bool zeroPolynomial( F && p , std::size_t n , long d , int K , R & rng )
{
 std::uniform_int_distribution< long > v( 1 , 2 * ( d < 1 ? 1 : d ) );
 std::vector< long > x( n );
 for( int k = 0 ; k < K ; ++k ) {
  for( auto & xi : x ) xi = v( rng );
  if( p( x ) != 0 )
   return( false );                        // surely not zero
  }
 return( true );
 }

/// whether A B = C, by K runs of O( n^2 )
template< class T , class R >
bool freivalds( const std::vector< std::vector< T > > & A ,
                const std::vector< std::vector< T > > & B ,
                const std::vector< std::vector< T > > & C , int K , R & rng )
{
 const std::size_t n = A.size();
 std::vector< T > x( n ) , y( n ) , z( n ) , w( n );
 auto mul = [ n ]( const auto & M , const std::vector< T > & v ,
                   std::vector< T > & r ) {
  for( std::size_t i = 0 ; i < n ; ++i ) {
   r[ i ] = T( 0 );
   for( std::size_t j = 0 ; j < n ; ++j ) r[ i ] += M[ i ][ j ] * v[ j ];
   }
  };
 for( int k = 0 ; k < K ; ++k ) {
  for( auto & xi : x ) xi = ( rng() & 1 ) ? T( 1 ) : T( -1 );
  mul( B , x , y );
  mul( A , y , z );
  mul( C , x , w );
  if( z != w )
   return( false );                        // surely A B != C
  }
 return( true );
 }

/// whether the formula f of n variables is true on more than half of the
/// assignments, from K random ones
template< class F , class R >
bool halfSatisfiable( F && f , std::size_t n , int K , R & rng )
{
 std::vector< bool > x( n );
 int yes = 0;
 for( int k = 0 ; k < K ; ++k ) {
  for( std::size_t i = 0 ; i < n ; ++i ) x[ i ] = rng() & 1;
  yes += f( x ) ? 1 : 0;
  }
 return( 2 * yes > K );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MonteCarlo.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MonteCarlo.h --------------------------*/
/*--------------------------------------------------------------------------*/
