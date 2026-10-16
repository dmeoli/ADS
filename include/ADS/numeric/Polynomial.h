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
                                long * mults = nullptr )
{
 if( p.empty() || q.empty() )
  return( std::vector< T >() );
 std::vector< T > r( p.size() + q.size() - 1 , T( 0 ) );
 for( std::size_t h = 0 ; h < p.size() ; ++h )
  for( std::size_t k = 0 ; k < q.size() ; ++k )
   r[ h + k ] += p[ h ] * q[ k ];
 if( mults )
  *mults += long( p.size() * q.size() );
 return( r );
 }

namespace detail {

/// r += x^s a
template< class T >
void addShifted( std::vector< T > & r , const std::vector< T > & a ,
                 std::size_t s , T sign = T( 1 ) )
{
 if( r.size() < a.size() + s )
  r.resize( a.size() + s , T( 0 ) );
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  r[ i + s ] += sign * a[ i ];
 }

/// the product of two polynomials of n coefficients, by three or four
/// recursive products
template< class T >
std::vector< T > multiplyRec( const std::vector< T > & p ,
                              const std::vector< T > & q , bool three ,
                              long * mults )
{
 const std::size_t n = p.size();
 if( n == 1 ) {
  if( mults ) ++*mults;
  return( std::vector< T >{ p[ 0 ] * q[ 0 ] } );
  }
 const std::size_t h = n / 2;                    // x^h splits them
 const std::vector< T > pD( p.begin() , p.begin() + h ) ,
                        pS( p.begin() + h , p.end() ) ,
                        qD( q.begin() , q.begin() + h ) ,
                        qS( q.begin() + h , q.end() );
 auto pad = []( std::vector< T > a , std::size_t m ) {
  a.resize( m , T( 0 ) );
  return( a );
  };
 const std::size_t m = n - h;                    // pS and qS, m >= h
 std::vector< T > r( 2 * n - 1 , T( 0 ) );
 const auto rS = multiplyRec( pS , qS , three , mults );
 const auto rD = multiplyRec( pad( pD , m ) , pad( qD , m ) , three , mults );
 addShifted( r , rS , 2 * h );
 addShifted( r , rD , 0 );
 if( three ) {
  std::vector< T > sp = pS , sq = qS;
  for( std::size_t i = 0 ; i < h ; ++i ) {
   sp[ i ] += pD[ i ];
   sq[ i ] += qD[ i ];
   }
  auto rM = multiplyRec( sp , sq , three , mults );
  addShifted( rM , rS , 0 , T( -1 ) );
  addShifted( rM , rD , 0 , T( -1 ) );
  addShifted( r , rM , h );
  }
 else {
  addShifted( r , multiplyRec( pad( pD , m ) , qS , three , mults ) , h );
  addShifted( r , multiplyRec( pS , pad( qD , m ) , three , mults ) , h );
  }
 r.resize( 2 * n - 1 );
 return( r );
 }

/// p and q padded with zeros to the same length
template< class T >
std::vector< T > multiplySame( std::vector< T > p , std::vector< T > q ,
                               bool three , long * mults )
{
 if( p.empty() || q.empty() )
  return( std::vector< T >() );
 const std::size_t np = p.size() , nq = q.size();
 const std::size_t n = std::max( np , nq );
 p.resize( n , T( 0 ) );
 q.resize( n , T( 0 ) );
 auto r = multiplyRec( p , q , three , mults );
 r.resize( np + nq - 1 );
 return( r );
 }

}  // end( namespace detail )

/// Prodotto4() on polynomials: four recursive products, Theta( n^2 )
template< class T >
std::vector< T > multiplyFour( const std::vector< T > & p ,
                               const std::vector< T > & q ,
                               long * mults = nullptr )
{
 return( detail::multiplySame( p , q , false , mults ) );
 }

/// moltiplicaPolinomiD&I(): three recursive products, O( n^1.59 )
template< class T >
std::vector< T > multiplyKaratsuba( const std::vector< T > & p ,
                                    const std::vector< T > & q ,
                                    long * mults = nullptr )
{
 return( detail::multiplySame( p , q , true , mults ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Polynomial.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Polynomial.h --------------------------*/
/*--------------------------------------------------------------------------*/
