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
 *   \f$ a b = ( ( a - 1 ) / 2 ) ( 2 b ) + b \f$ for a odd, O( log a )
 *   steps, the shift and add of the arithmetic units.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.4 and 2.6,
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

#ifndef __ADS_BinaryInteger
 #define __ADS_BinaryInteger
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/Polynomial.h"

#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Bits = std::vector< int >;   ///< the bits, the least significant first

/// the bits of x, at least n of them
inline Bits toBits( std::uint64_t x , std::size_t n = 0 )
{
 Bits b;
 for( ; x || ( b.size() < n ) ; x >>= 1 )
  b.push_back( int( x & 1 ) );
 return( b );
 }

/// the value of the bits, which must be at most 64 significant ones
inline std::uint64_t fromBits( const Bits & b )
{
 std::uint64_t x = 0;
 for( std::size_t i = b.size() ; i-- > 0 ; )
  x = ( x << 1 ) | std::uint64_t( b[ i ] );
 return( x );
 }

/// the coefficients, possibly larger than 1 or negative, made bits by the
/// carries
inline Bits carry( const std::vector< long > & c )
{
 Bits b;
 long r = 0;
 for( std::size_t i = 0 ; ( i < c.size() ) || r ; ++i ) {
  long v = r + ( i < c.size() ? c[ i ] : 0 );
  long bit = v % 2;
  if( bit < 0 ) bit += 2;
  b.push_back( int( bit ) );
  r = ( v - bit ) / 2;
  }
 while( ( b.size() > 1 ) && ( b.back() == 0 ) )
  b.pop_back();
 return( b );
 }

/// X + Y, Theta( n )
inline Bits binaryAdd( const Bits & X , const Bits & Y )
{
 Bits Z;
 int r = 0;
 for( std::size_t i = 0 ; ( i < X.size() ) || ( i < Y.size() ) ; ++i ) {
  const int s = ( i < X.size() ? X[ i ] : 0 ) +
                ( i < Y.size() ? Y[ i ] : 0 ) + r;
  Z.push_back( s & 1 );
  r = s >> 1;
  }
 if( r )
  Z.push_back( r );
 return( Z );
 }

inline std::vector< long > widen( const Bits & X )
{
 return( std::vector< long >( X.begin() , X.end() ) );
 }

/// X Y by the method of the school, Theta( n^2 )
inline Bits multiplyBinarySchool( const Bits & X , const Bits & Y ,
                            long * mults = nullptr )
{
 return( carry( multiplyNaive( widen( X ) , widen( Y ) , mults ) ) );
 }

/// X Y by four products of half size, Theta( n^2 )
inline Bits multiplyBinaryFour( const Bits & X , const Bits & Y ,
                          long * mults = nullptr )
{
 return( carry( multiplyFour( widen( X ) , widen( Y ) , mults ) ) );
 }

/// X Y by the three products of Karatsuba, O( n^1.59 )
inline Bits multiplyBinaryKaratsuba( const Bits & X , const Bits & Y ,
                               long * mults = nullptr )
{
 return( carry( multiplyKaratsuba( widen( X ) , widen( Y ) , mults ) ) );
 }

/// the multiplication of Ahmes, in *steps the halvings, if given
inline std::uint64_t ahmes( std::uint64_t a , std::uint64_t b ,
                            long * steps = nullptr )
{
 std::uint64_t p = 0;
 for( ; a ; a >>= 1 , b <<= 1 ) {
  if( a & 1 )
   p += b;
  if( steps )
   ++*steps;
  }
 return( p );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* BinaryInteger.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinaryInteger.h ------------------------*/
/*--------------------------------------------------------------------------*/
