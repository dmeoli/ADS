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
 * - the middle square, the p bits in the middle of
 *   \f$ i^2 \f$ [see hash_midsquare()].
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
 *         Universita' di Pisa \n
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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- HASH FUNCTIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the bit of place j of bin( k ), counted from the left; 0 past the end
inline unsigned bin_bit( std::string_view k , std::size_t j )
{
 if( j >= 8 * k.size() )
  return( 0 );
 return( ( static_cast< unsigned char >( k[ j / 8 ] ) >> ( 7 - j % 8 ) ) &
         1u );
 }

/// int( bin( k ) ) of a key of at most 8 characters
inline std::uint64_t int_bin( std::string_view k )
{
 if( k.size() > 8 )
  throw( std::invalid_argument( "ADS::int_bin: more than 64 bits" ) );
 std::uint64_t i = 0;
 for( unsigned char c : k )
  i = ( i << 8 ) | c;
 return( i );
 }

/// the extraction: the p <= 63 bits of bin( k ) from the place s on,
/// counted from the left, as an integer in [ 0 , 2^p )
inline std::uint64_t hash_extraction( std::string_view k , int p ,
                                      std::size_t s )
{
 if( ( p <= 0 ) || ( p > 63 ) )
  throw( std::invalid_argument( "ADS::hash_extraction: p not in [1, 63]" ) );
 std::uint64_t b = 0;
 for( int j = 0 ; j < p ; ++j )
  b = ( b << 1 ) | bin_bit( k , s + j );
 return( b );
 }

/// the XOR of the blocks of p <= 63 bits of bin( k ), the last one padded
/// with zeros on the right
inline std::uint64_t hash_xor( std::string_view k , int p )
{
 if( ( p <= 0 ) || ( p > 63 ) )
  throw( std::invalid_argument( "ADS::hash_xor: p not in [1, 63]" ) );
 std::uint64_t b = 0;
 for( std::size_t s = 0 ; s < 8 * k.size() ; s += p )
  b ^= hash_extraction( k , p , s );
 return( b );
 }

/// the division int( bin( k ) ) mod m by the rule of Horner in the given
/// base, each character being a digit, from the leading digit seed (the
/// h = 5 , h = 3 h + c is base 3 and seed 5);
/// ( m - 1 ) base + base - 1 must fit in 64 bits
inline std::uint64_t hash_division( std::string_view k , std::uint64_t m ,
                                    std::uint64_t base = 256 ,
                                    std::uint64_t seed = 0 )
{
 if( ( m == 0 ) || ( base == 0 ) ||
     ( m > std::numeric_limits< std::uint64_t >::max() / base ) )
  throw( std::invalid_argument( "ADS::hash_division: m too large" ) );
 std::uint64_t b = seed % m;
 for( unsigned char c : k )
  b = ( b * base + c ) % m;
 return( b );
 }

/// the multiplication in floating point, the book's formula: exact only
/// while C i keeps enough digits after the point, i.e., for i well below
/// the 2^64 of the mantissa of a long double
inline std::uint64_t hash_multiplication( std::uint64_t i , std::uint64_t m ,
                                          long double C =
                                           ( std::sqrt( 5.0L ) - 1 ) / 2 )
{
 long double ip;
 const long double f = std::modf( C * static_cast< long double >( i ) , &ip );
 return( static_cast< std::uint64_t >( m * f ) );
 }

/// the multiplication with integers for m = 2^p, w = 64: the p most
/// significant bits of the 64 least significant ones of i s, with
/// s = floor( C 2^64 ); it is floor( m frac( i s / 2^64 ) ), which differs
/// from the exact floor( m frac( C i ) ) by about m i / 2^64
inline std::uint64_t hash_fibonacci( std::uint64_t i , int p )
{
 if( ( p <= 0 ) || ( p > 63 ) )
  throw( std::invalid_argument( "ADS::hash_fibonacci: p not in [1, 63]" ) );
 constexpr std::uint64_t s = 0x9E3779B97F4A7C15ull;   // floor( C 2^64 )
 return( ( i * s ) >> ( 64 - p ) );
 }

/// the middle square: the p <= 32 bits in the middle of the 64 least
/// significant ones of i^2
inline std::uint64_t hash_midsquare( std::uint64_t i , int p )
{
 if( ( p <= 0 ) || ( p > 32 ) )
  throw( std::invalid_argument( "ADS::hash_midsquare: p not in [1, 32]" ) );
 return( ( ( i * i ) >> ( 32 - p / 2 ) ) & ( ( 1ull << p ) - 1 ) );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* HashFunctions.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashFunctions.h ------------------------*/
/*--------------------------------------------------------------------------*/
