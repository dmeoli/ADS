/*--------------------------------------------------------------------------*/
/*---------------------------- File FastHash.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered hash functions of ADS, beyond the book [see
 * HashFunctions.h for those of section 7.1 and UniversalHashing.h for the
 * universal classes]: hashes of 64 bits for the tables, the filters and the
 * perfect hash functions, made of the folded product, the product of two
 * words on 128 bits whose two halves are XORed [see foldedProduct()]. Each
 * bit of the folded product depends on every bit of its factors that the
 * low half of the product does not lose, and a 64-bit processor computes
 * it by one multiplication, so that a word costs one product and a string
 * one product per 16 bytes.
 *
 * - hashWord() is the hash of a word, its folded product with the seed
 *   XORed with a constant;
 * - hashBytes() is the hash of a string of n bytes, by the length: up to
 *   8 bytes, the string is read as one word, by two loads of 4 bytes at
 *   its ends (overlapping below 8) or by its first, middle and last byte
 *   below 4, and its hash is the folded product of that word XOR the seed
 *   and of a constant XOR n; from 9 to 16 bytes, the product of the two
 *   words that end at its ends; from 17 to 32, the XOR of the products of
 *   its first and its last 16 bytes, two products at once; from 33 to 64,
 *   four products at once, of its first and its last 32 bytes, folded by a
 *   fifth one; these are inline, with no loop. Past 64 bytes, out of line,
 *   up to 256 the blocks of 16 bytes are folded into a state, each by the
 *   product of its two words, one XORed with the state, by 6 independent
 *   states over each 96 bytes, and the last 16 bytes go through a last
 *   product with the state and n; past 256, the string is cut into
 *   stripes of 64 bytes, i.e., 8 lanes of a word, each adding the product
 *   of the two halves of its word XOR a constant, and its word to the
 *   other lane of its pair, scrambled every 16 stripes (the stripes of
 *   XXH3), which vectors of AVX2 or AVX-512 do 4 or 8 lanes at a time with
 *   the same result as the scalar operations, and the 8 lanes are folded
 *   by 4 products and a last one;
 * - FastHash< T > is the hash of the integers, the enumerations, the
 *   pointers, the floating point numbers (+0 and -0 hashing the same) and
 *   the strings (std::string, std::string_view and the C strings), by
 *   hashWord() and hashBytes(); it declares is_avalanching (the
 *   convention of Boost), so that FlatHashTable [see FlatHashTable.h] and
 *   the tables of Boost take its hash as it is; DefaultHash< K > is
 *   FastHash< K > when it is defined and std::hash< K > otherwise, the
 *   default hash of the engineered tables and filters.
 *
 * Every bit of a hash depends on every bit of the key, the high half of a
 * product carrying the bits that its low half loses, which is what a hash
 * table needs; a single product does not give the full avalanche that the
 * statistical batteries ask (one bit of a key flips each bit of the hash
 * with probability near 1 / 2, not exactly 1 / 2), as for the hashes of the
 * short strings of Abseil. The functions are not cryptographic: a string
 * chosen to make a factor of a product 0 loses the bits of the other one,
 * and anyone who knows the seed can make collisions; a table that faces
 * adversarial keys must use a seed drawn at random. A word is read in the
 * byte order of the machine, so the hashes differ between little and big
 * endian machines.
 *
 * The contracts [see Contract.h]: a string of n bytes from a pointer that
 * is not nil when n > 0. The functions are defined in FastHash.tpp.
 *
 * \see M. Thorup, High speed hashing for integers and strings, 2015,
 *      arXiv:1504.06804 (the multiply-shift hashing and its analysis)
 * \see J. M. Lopez Munoz, Inside boost::unordered_flat_map, 2022 (the
 *      folded product as the mixing of a hash)
 * \see Wang Yi, wyhash, 2019, https://github.com/wangyi-fudan/wyhash (the
 *      folded product of the words of a string, the overlapping loads of
 *      its short tails)
 * \see Y. Collet, XXH3, 2019, https://github.com/Cyan4973/xxHash (the
 *      stripes of the long strings in vectors)
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

#ifndef __ADS_FastHash
 #define __ADS_FastHash
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- THE HASH FUNCTIONS ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the product a b on 128 bits, its low and its high half
struct Product128 {
 std::uint64_t lo , hi;
 };

/// the product a b on 128 bits
[[nodiscard]] constexpr Product128 product128( std::uint64_t a ,
                                               std::uint64_t b );

/// the folded product: the low half of a b on 128 bits XOR its high half
[[nodiscard]] constexpr std::uint64_t foldedProduct( std::uint64_t a ,
                                                     std::uint64_t b );

/// the hash of the word x under the seed
[[nodiscard]] constexpr std::uint64_t hashWord( std::uint64_t x ,
                                                std::uint64_t seed = 0 );

/// the hash of the n bytes from p under the seed
/// \pre p is not nil if n > 0
[[nodiscard]] std::uint64_t hashBytes( const void * p , std::size_t n ,
                                       std::uint64_t seed = 0 );

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS FastHash ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the hash of the values of T, for the integers, the enumerations, the
/// pointers, the floating point numbers and the strings
template< class T >
struct FastHash;

/// the hash of an integer, an enumeration, a pointer or a floating point
/// number, +0 and -0 the same
template< class T >
 requires( std::is_integral_v< T > || std::is_enum_v< T > ||
           std::is_pointer_v< T > ||
           ( std::is_floating_point_v< T > && ( sizeof( T ) <= 8 ) ) )
struct FastHash< T > {
 using is_avalanching = void;   ///< the hash needs no further mixing

 std::uint64_t seed = 0;

 [[nodiscard]] std::size_t operator()( T x ) const noexcept {
  if constexpr( std::is_pointer_v< T > )
   return( std::size_t( hashWord( std::uint64_t(
                         reinterpret_cast< std::uintptr_t >( x ) ) ,
                         seed ) ) );
  else if constexpr( std::is_enum_v< T > )
   return( std::size_t( hashWord( std::uint64_t(
                         std::underlying_type_t< T >( x ) ) , seed ) ) );
  else if constexpr( std::is_floating_point_v< T > ) {
   if( x == T( 0 ) )
    x = T( 0 );                     // -0 == +0
   std::uint64_t w = 0;
   std::memcpy( &w , &x , sizeof( T ) );
   return( std::size_t( hashWord( w , seed ) ) );
   }
  else
   return( std::size_t( hashWord( std::uint64_t( x ) , seed ) ) );
  }
 };

/// the hash of a string, by its bytes; any string_view, std::string and
/// C string with the same characters hash the same
template<>
struct FastHash< std::string_view > {
 using is_avalanching = void;
 using is_transparent = void;   ///< for the lookups by any string

 std::uint64_t seed = 0;

 [[nodiscard]] std::size_t operator()( std::string_view s ) const noexcept
 {
  return( std::size_t( hashBytes( s.data() , s.size() , seed ) ) );
  }
 };

template<>
struct FastHash< std::string > : FastHash< std::string_view > {};

template<>
struct FastHash< const char * > : FastHash< std::string_view > {};

namespace detail {

/// std::hash< K >, or FastHash< K > if it is defined
template< class K >
struct DefaultHashOf {
 using type = std::hash< K >;
 };

template< class K >
 requires requires { sizeof( FastHash< K > ); }
struct DefaultHashOf< K > {
 using type = FastHash< K >;
 };

}  // end( namespace detail )

/// the hash of the engineered tables and filters of ADS: FastHash< K > for
/// the integers, the enumerations, the pointers, the floating point
/// numbers and the strings, std::hash< K > for any other K
template< class K >
using DefaultHash = typename detail::DefaultHashOf< K >::type;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/FastHash.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FastHash.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File FastHash.h ---------------------------*/
/*--------------------------------------------------------------------------*/
