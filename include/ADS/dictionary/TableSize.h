/*--------------------------------------------------------------------------*/
/*---------------------------- File TableSize.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sizes of the hash tables that restructure themselves (section 7.4):
 * a size that is a power of two doubles or halves, any other one goes to
 * the first prime not smaller than its double or its half, so that the
 * inspections of an OpenHashTable that need a prime size, or a size
 * coprime with their step, keep working.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
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

#ifndef __ADS_TableSize
 #define __ADS_TableSize
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// true if n is prime
inline bool is_prime( std::size_t n )
{
 if( n < 2 )
  return( false );
 for( std::size_t d = 2 ; d * d <= n ; ++d )
  if( n % d == 0 )
   return( false );
 return( true );
 }

/// the smallest prime not smaller than n
inline std::size_t next_prime( std::size_t n )
{
 while( ! is_prime( n ) )
  ++n;
 return( n );
 }

/// true if n is a power of two
inline bool is_pow2( std::size_t n ) { return( n && ! ( n & ( n - 1 ) ) ); }

/// the size after a doubling, or after a halving down to m0
inline std::size_t resized( std::size_t m , bool grow , std::size_t m0 )
{
 if( is_pow2( m ) )
  return( grow ? 2 * m : std::max( m / 2 , m0 ) );
 return( grow ? next_prime( 2 * m ) : std::max( next_prime( m / 2 ) , m0 ) );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* TableSize.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File TableSize.h --------------------------*/
/*--------------------------------------------------------------------------*/
