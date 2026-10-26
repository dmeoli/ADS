/*--------------------------------------------------------------------------*/
/*-------------------------- File HashFunctions.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash functions of section 7.1.
 *
 * A hash function \f$ H : U \rightarrow \{ 0 , \ldots , m - 1 \} \f$ maps a
 * key to a place of a vector of size m; it has the simple uniformity if
 * \f[
 *   Q( i ) = \sum_{k : H( k ) = i} P( k ) = \frac{1}{m}
 *   \qquad \forall i \in \{ 0 , \ldots , m - 1 \}
 * \f]
 * where P( k ) is the probability that the key k is in the dictionary. A key
 * is read as the string bin( k ) of its bits, the bytes of a string one
 * after the other from the first, the most significant bit of each first,
 * and int( b ) is the integer of the bit string b. The functions are
 *
 * - the extraction, \f$ H( k ) = int( b ) \f$ with b a block of p bits of
 *   bin( k ) [see hash_extraction()];
 * - the XOR, \f$ H( k ) = int( \bigoplus_i b_i ) \f$ of the blocks of p bits
 *   of bin( k ), the last one padded with zeros [see hash_xor()];
 * - the division, \f$ H( k ) = int( bin( k ) ) \bmod m \f$, computed with
 *   the rule of Horner and a mod at each character so that nothing
 *   overflows [see hash_division()];
 * - the multiplication, \f$ H( k ) = \lfloor m ( C i - \lfloor C i \rfloor )
 *   \rfloor \f$ with \f$ i = int( bin( k ) ) \f$ and \f$ C = ( \sqrt{5} -
 *   1 ) / 2 \f$, the conjugate of the golden ratio [see
 *   hash_multiplication()], and its version with integers for
 *   \f$ m = 2^p \f$: with \f$ s = \lfloor C 2^w \rfloor \f$ and
 *   \f$ i s = r_1 2^w + r_0 \f$, H( k ) is made of the p most significant
 *   bits of \f$ r_0 \f$ [see hash_fibonacci()];
 * - the middle square, the p bits in the middle of \f$ i^2 \f$ [see
 *   hash_midsquare()].
 *
 * The contracts [see Contract.h]: the number p of bits in its range, a key
 * of at most 8 characters for int_bin(), and for hash_division() a size m
 * > 0 and a base > 0 such that the rule of Horner does not overflow; each
 * result is below the size it is meant for. The functions are defined in
 * HashFunctions.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 7.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 6.4
 * \see R. Morris, Scatter storage techniques, Communications of the ACM
 *      11(1), 38-44, 1968, doi:10.1145/362851.362882
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

#ifndef __ADS_HashFunctions
 #define __ADS_HashFunctions
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- HASH FUNCTIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the bit of place j of bin( k ), counted from the left; 0 past the end
unsigned bin_bit( std::string_view k , std::size_t j );

/// int( bin( k ) ) of a key of at most 8 characters
/// \pre k has at most 8 characters
std::uint64_t int_bin( std::string_view k );

/// the extraction: the p bits of bin( k ) from the place s on, counted
/// from the left, as an integer in [ 0 , 2^p )
/// \pre 1 <= p <= 63
/// \post the result is below 2^p
std::uint64_t hash_extraction( std::string_view k , int p , std::size_t s );

/// the XOR of the blocks of p bits of bin( k ), the last one padded with
/// zeros on the right
/// \pre 1 <= p <= 63
/// \post the result is below 2^p
std::uint64_t hash_xor( std::string_view k , int p );

/// the division int( bin( k ) ) mod m by the rule of Horner in the given
/// base, each character being a digit, from the leading digit seed (the
/// h = 5 , h = 3 h + c is base 3 and seed 5); a character is any byte, up
/// to 255 also in a base below 256
/// \pre m > 0, base > 0, and ( m - 1 ) base + 255 fits in 64 bits
/// \post the result is below m
std::uint64_t hash_division( std::string_view k , std::uint64_t m ,
                             std::uint64_t base = 256 ,
                             std::uint64_t seed = 0 );

/// the multiplication in floating point, the book's formula: exact only
/// while C i keeps enough digits after the point, i.e., for i well below
/// the 2^64 of the mantissa of a long double
/// \pre 0 <= C < 1
/// \post the result is below m, if m > 0
std::uint64_t hash_multiplication( std::uint64_t i , std::uint64_t m ,
                                   long double C =
                                    ( std::sqrt( 5.0L ) - 1 ) / 2 );

/// the multiplication with integers for m = 2^p, w = 64: the p most
/// significant bits of the 64 least significant ones of i s, with
/// s = floor( C 2^64 ); it is floor( m frac( i s / 2^64 ) ), which differs
/// from the exact floor( m frac( C i ) ) by about m i / 2^64
/// \pre 1 <= p <= 63
/// \post the result is below 2^p
std::uint64_t hash_fibonacci( std::uint64_t i , int p );

/// the middle square: the p bits in the middle of the 64 least significant
/// ones of i^2
/// \pre 1 <= p <= 32
/// \post the result is below 2^p
std::uint64_t hash_midsquare( std::uint64_t i , int p );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/HashFunctions.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HashFunctions.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashFunctions.h ------------------------*/
/*--------------------------------------------------------------------------*/
