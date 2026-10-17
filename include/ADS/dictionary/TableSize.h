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
 * The contracts [see Contract.h]: resized() starts from a size not below
 * the initial size m0 > 0, never goes below it, and keeps a power of two a
 * power of two.
 * The functions are defined in TableSize.tpp.
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

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// true if n is prime
bool is_prime( std::size_t n );

/// the smallest prime not smaller than n
std::size_t next_prime( std::size_t n );

/// true if n is a power of two
bool is_pow2( std::size_t n );

/// the size after a doubling, or after a halving down to m0
/// \pre 0 < m0 <= m
/// \post not below m0, and a power of two if m is one, a prime otherwise
std::size_t resized( std::size_t m , bool grow , std::size_t m0 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/TableSize.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TableSize.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File TableSize.h --------------------------*/
/*--------------------------------------------------------------------------*/
