/*--------------------------------------------------------------------------*/
/*-------------------------- File RandomHashing.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Hashing with random tables, beyond the book:
 *
 * - splitmix64(), the finalizer of the generator SplitMix64 of Steele, Lea
 *   and Flood, a bijection of the 64-bit words that mixes every input bit
 *   into every output bit, and SplitMix64, the generator itself, which
 *   meets UniformRandomBitGenerator and seeds the tables below;
 * - fibonacciPlace(), the top b bits of \f$ h \lfloor 2^{64} / \phi
 *   \rfloor \f$, the multiplicative hashing of Knuth with the golden
 *   ratio, which spreads over a table of \f$ 2^b \f$ places also the
 *   hashes whose low bits are regular;
 * - ZobristTable, a random word R[ f ][ v ] for each feature f and value v,
 *   the hash of a state being the XOR of the words of its pairs: a move
 *   that changes the value of a feature updates the hash in O( 1 ) by two
 *   XOR, which makes it the hash of the board games and of the states of a
 *   search;
 * - TabulationHash, the simple tabulation of Carter and Wegman: a key of
 *   64 bits is cut into 8 bytes and the hash is the XOR of a random word
 *   of a table per byte; it is 3-independent, and Patrascu and Thorup
 *   showed that it gives linear probing, cuckoo hashing and the minwise
 *   estimates the bounds of fully random hashing.
 *
 * The contracts [see Contract.h]: 1 <= b <= 64 for fibonacciPlace(), at
 * least a feature and a value for ZobristTable, and the features and the
 * values within it. The functions are defined in RandomHashing.tpp.
 *
 * \see G. L. Steele, D. Lea, C. H. Flood, Fast splittable pseudorandom
 *      number generators, Proceedings of OOPSLA 2014, 453-472, 2014,
 *      doi:10.1145/2660193.2660195
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (the multiplicative hashing)
 * \see A. L. Zobrist, A new hashing method with application for game
 *      playing, Technical Report 88, University of Wisconsin, 1970
 * \see J. L. Carter, M. N. Wegman, Universal classes of hash functions,
 *      Journal of Computer and System Sciences 18(2), 143-154, 1979,
 *      doi:10.1016/0022-0000(79)90044-8
 * \see M. Patrascu, M. Thorup, The power of simple tabulation hashing,
 *      Journal of the ACM 59(3), article 14, 2012,
 *      doi:10.1145/2220357.2220361
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

#ifndef __ADS_RandomHashing
 #define __ADS_RandomHashing
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the mix of SplitMix64: a bijection of the words of 64 bits
[[nodiscard]] constexpr std::uint64_t splitmix64( std::uint64_t x );

/// the place of the hash h in a table of 2^b places, its top b bits after
/// the product by floor( 2^64 / phi )
/// \pre 1 <= b <= 64
[[nodiscard]] constexpr std::uint64_t fibonacciPlace( std::uint64_t h ,
                                                      int b );

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SplitMix64 ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the generator SplitMix64, a UniformRandomBitGenerator
class SplitMix64 {
 public:
 using result_type = std::uint64_t;

 explicit constexpr SplitMix64( std::uint64_t seed = 0 ) : f_s( seed ) {}

 /// the next word
 constexpr std::uint64_t operator()( void ) {
  const std::uint64_t r = splitmix64( f_s );
  f_s += 0x9e3779b97f4a7c15ULL;
  return( r );
  }

 static constexpr std::uint64_t min( void ) { return( 0 ); }
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 private:
 std::uint64_t f_s;   ///< the state
 };

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ZobristTable ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a random word for each pair ( feature , value )
class ZobristTable {
 public:
 /// the words of n features with m values each, from the seed
 /// \pre n >= 1 and m >= 1
 ZobristTable( std::size_t n , std::size_t m , std::uint64_t seed = 1 );

 /// the word of the feature f with the value v
 /// \pre f < n and v < m
 [[nodiscard]] std::uint64_t operator()( std::size_t f ,
                                         std::size_t v ) const;

 /// the hash of a state, the value of each feature by its place in S
 /// \pre S has n values, each below m
 template< class V >
 [[nodiscard]] std::uint64_t hash( const std::vector< V > & S ) const;

 /// the hash h of a state after the feature f changes from a to b
 /// \pre f < n , a < m and b < m
 [[nodiscard]] std::uint64_t update( std::uint64_t h , std::size_t f ,
                                     std::size_t a , std::size_t b ) const;

 private:
 std::size_t f_n , f_m;             ///< the features and the values
 std::vector< std::uint64_t > f_R;  ///< R[ f m + v ]
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS TabulationHash ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the simple tabulation hashing of the keys of 64 bits
class TabulationHash {
 public:
 explicit TabulationHash( std::uint64_t seed = 1 );

 /// the XOR of the words of the 8 bytes of x
 [[nodiscard]] std::uint64_t operator()( std::uint64_t x ) const;

 private:
 std::array< std::array< std::uint64_t , 256 > , 8 > f_T;   ///< the tables
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/RandomHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RandomHashing.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RandomHashing.h ------------------------*/
/*--------------------------------------------------------------------------*/
