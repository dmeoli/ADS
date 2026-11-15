/*--------------------------------------------------------------------------*/
/*---------------------------- File TableSize.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sizes of the hash tables that restructure themselves (section 7.4):
 * a size that is a power of two doubles or halves, any other one goes to
 * the first prime not smaller than its double or its half, so that the
 * inspections of an `OpenHashTable` that need a prime size, or a size
 * coprime with their step, keep working. By the theorem of Chebyshev there
 * is a prime between \f$ x \f$ and \f$ 2 x \f$, so that the next prime is
 * less than twice the size asked, and it is found by trial divisions in
 * \f$ O( \sqrt{m} ) \f$ each.
 *
 * The contracts [see `Contract.h`]: `resized()` starts from a size not
 * below the initial size \f$ m_0 > 0 \f$, never goes below it, and keeps a
 * power of two a power of two. The functions are defined in
 * `TableSize.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
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

#ifndef __ADS_TableSize
 #define __ADS_TableSize
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// true if \f$ n \f$ is prime, by trial division up to \f$ \sqrt{n} \f$
bool is_prime( std::size_t n );

/// the smallest prime not smaller than \f$ n \f$, by trial divisions
std::size_t next_prime( std::size_t n );

/// true if \f$ n \f$ is a power of two, i.e., \f$ n \ne 0 \f$ and
/// `n & ( n - 1 )` is 0
bool is_pow2( std::size_t n );

/// the size after a doubling, or after a halving down to \f$ m_0 \f$
///
/// In pseudocode (section 7.4, with the sizes beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{resized}( m , \mathit{grow} , m_0 ) \cr
/// 2 & \quad \textbf{if}\ m \text{ is a power of two }\ \textbf{then} \cr
/// 3 & \qquad \textbf{if}\ \mathit{grow}\ \textbf{then return}\ 2 m
///     \ \textbf{else return}\ \max\{ \lfloor m / 2 \rfloor , m_0 \} \cr
/// 4 & \quad \textbf{if}\ \mathit{grow}\ \textbf{then return}
///     \ \mathrm{nextPrime}( 2 m ) \cr
/// 5 & \quad \textbf{return}\ \max\{ \mathrm{nextPrime}( \lfloor m / 2
///     \rfloor ) , m_0 \}
/// \end{array}}
/// \f]
/// \pre \f$ 0 < m_0 \le m \f$
/// \post not below \f$ m_0 \f$, and a power of two (or \f$ m_0 \f$) if
///       \f$ m \f$ is one
std::size_t resized( std::size_t m , bool grow , std::size_t m0 );

/// a hash \f$ H \f$ that maps a key straight to a place of a table of
/// \f$ m = 2^d \f$ places by `h.place( k , m )`, as the multiplicative
/// hashing of section 5.1.1 of Morin's book and the tabulation hashing of
/// its section 5.2.3 do with the \f$ d \f$ most significant bits of a
/// product or of a XOR [see `MultiplicativeHash` in `UniversalHashing.h`
/// and `TabulationHash` in `RandomHashing.h`]; the tables use it in place
/// of \f$ h( k ) \bmod m \f$, whose low bits are the worst ones of a
/// product
template< class H , class K >
concept PlaceHash = requires( const H & h , const K & k , std::size_t m )
{
 { h.place( k , m ) } -> std::convertible_to< std::size_t >;
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/TableSize.tpp"

/*--------------------------------------------------------------------------*/

#endif /* TableSize.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File TableSize.h --------------------------*/
/*--------------------------------------------------------------------------*/
