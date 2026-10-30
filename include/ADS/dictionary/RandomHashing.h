/*--------------------------------------------------------------------------*/
/*-------------------------- File RandomHashing.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Hashing with random tables, beyond the book:
 *
 * - `splitmix64()`, the finalizer of the generator SplitMix64 of Steele,
 *   Lea and Flood, a bijection of the words of 64 bits that mixes every
 *   input bit into every output bit, and `SplitMix64`, the generator
 *   itself, which meets `UniformRandomBitGenerator` and seeds the tables
 *   below;
 * - `fibonacciPlace()`, the top \f$ b \f$ bits of
 *   \f$ h \lfloor 2^{64} / \phi \rfloor \f$, the multiplicative hashing of
 *   Knuth with the golden ratio, which spreads over a table of
 *   \f$ 2^b \f$ places also the hashes whose low bits are regular;
 * - `ZobristTable`, a random word \f$ R[ f ][ v ] \f$ for each feature
 *   \f$ f \f$ and value \f$ v \f$, the hash of a state being the XOR of the
 *   words of its pairs: a move that changes the value of a feature updates
 *   the hash in \f$ O( 1 ) \f$ by two XOR, which makes it the hash of the
 *   board games and of the states of a search;
 * - `TabulationHash`, the simple tabulation of Carter and Wegman: a key of
 *   64 bits is cut into 8 bytes and the hash is the XOR of a random word
 *   of a table per byte; it is 3-independent, and Pătrașcu and Thorup
 *   showed that it gives linear probing, cuckoo hashing and the minwise
 *   estimates the bounds of fully random hashing.
 *
 * The contracts [see `Contract.h`]: \f$ 1 \le b \le 64 \f$ for
 * `fibonacciPlace()`, at least a feature and a value for `ZobristTable`,
 * and the features and the values within it. The functions are defined in
 * `RandomHashing.tpp`.
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
 * \see M. Pătrașcu, M. Thorup, The power of simple tabulation hashing,
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
#include <ranges>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the mix of SplitMix64: a bijection of the words of 64 bits
///
/// In pseudocode (Steele, Lea and Flood, beyond the book), the arithmetic
/// modulo \f$ 2^{64} \f$, \f$ \gg \f$ the shift to the right; each step is
/// a bijection, a sum, a product by an odd constant or
/// \f$ x \oplus ( x \gg s ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{splitmix64}( x ) \cr
/// 2 & \quad x \gets x + \lfloor 2^{64} / \phi \rfloor \cr
/// 3 & \quad x \gets ( x \oplus ( x \gg 30 ) ) \cdot
///     \mathrm{BF58476D1CE4E5B9}_{16} \cr
/// 4 & \quad x \gets ( x \oplus ( x \gg 27 ) ) \cdot
///     \mathrm{94D049BB133111EB}_{16} \cr
/// 5 & \quad \textbf{return}\ x \oplus ( x \gg 31 )
/// \end{array}}
/// \f]
[[nodiscard]] constexpr std::uint64_t splitmix64( std::uint64_t x );

/// the place of the hash \f$ h \f$ in a table of \f$ 2^b \f$ places, its top
/// \f$ b \f$ bits after the product by \f$ \lfloor 2^{64} / \phi \rfloor \f$
///
/// In pseudocode (section 6.4 of TAOCP, the multiplicative hashing):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fibonacciPlace}( h , b ) \cr
/// 2 & \quad \textbf{return}\ \lfloor ( h \lfloor 2^{64} / \phi \rfloor
///     \bmod 2^{64} ) / 2^{64 - b} \rfloor
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le b \le 64 \f$
[[nodiscard]] constexpr std::uint64_t fibonacciPlace( std::uint64_t h ,
                                                      int b );

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SplitMix64 ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the generator SplitMix64, a `UniformRandomBitGenerator`
class SplitMix64 {
 public:
 using result_type = std::uint64_t;   ///< the words generated

 /// the generator from the state `seed`
 explicit constexpr SplitMix64( std::uint64_t seed = 0 ) : f_s( seed ) {}

 /// the next word, `splitmix64()` of the state, which then grows by
 /// \f$ \lfloor 2^{64} / \phi \rfloor \f$
 constexpr std::uint64_t operator()( void ) {
  const std::uint64_t r = splitmix64( f_s );
  f_s += 0x9e3779b97f4a7c15ULL;
  return( r );
  }

 /// the least word, 0
 static constexpr std::uint64_t min( void ) { return( 0 ); }

 /// the largest word, \f$ 2^{64} - 1 \f$
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 private:
 std::uint64_t f_s;   ///< the state
 };

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ZobristTable ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a random word for each pair \f$ ( f , v ) \f$ of a feature and a value
class ZobristTable {
 public:
 /// the words of \f$ n \f$ features with \f$ m \f$ values each, from the seed
 /// \pre \f$ n \ge 1 \f$ and \f$ m \ge 1 \f$
 ZobristTable( std::size_t n , std::size_t m , std::uint64_t seed = 1 );

 /// the word \f$ R[ f ][ v ] \f$ of the feature \f$ f \f$ with the value
 /// \f$ v \f$
 /// \pre \f$ f < n \f$ and \f$ v < m \f$
 [[nodiscard]] std::uint64_t operator()( std::size_t f ,
                                         std::size_t v ) const;

 /// the hash of a state, the value of each feature by its place in
 /// \f$ S \f$, any range with a size and access by index (a vector, an
 /// array, a span of the state)
 ///
 /// In pseudocode (Zobrist, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{hash}( S ) \cr
 /// 2 & \quad h \gets 0 \cr
 /// 3 & \quad \textbf{for}\ f \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ h
 ///     \gets h \oplus R[ f ][ S[ f ] ] \cr
 /// 4 & \quad \textbf{return}\ h
 /// \end{array}}
 /// \f]
 /// \pre \f$ S \f$ has \f$ n \f$ values, each below \f$ m \f$
 template< std::ranges::random_access_range S >
  requires std::ranges::sized_range< S >
 [[nodiscard]] std::uint64_t hash( const S & s ) const;

 /// the hash \f$ h \f$ of a state after the feature \f$ f \f$ changes from
 /// \f$ a \f$ to \f$ b \f$, \f$ h \oplus R[ f ][ a ] \oplus R[ f ][ b ] \f$,
 /// since \f$ x \oplus x = 0 \f$ takes the old word away
 /// \pre \f$ f < n \f$, \f$ a < m \f$ and \f$ b < m \f$
 [[nodiscard]] std::uint64_t update( std::uint64_t h , std::size_t f ,
                                     std::size_t a , std::size_t b ) const;

 private:
 std::size_t f_n , f_m;             ///< the features and the values
 std::vector< std::uint64_t > f_R;  ///< \f$ R[ f m + v ] \f$
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS TabulationHash ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the simple tabulation hashing of the keys of 64 bits
class TabulationHash {
 public:
 /// the 8 tables of 256 random words, from the seed
 explicit TabulationHash( std::uint64_t seed = 1 );

 /// the XOR of the words of the 8 bytes of \f$ x \f$
 ///
 /// In pseudocode (the simple tabulation of Carter and Wegman, beyond the
 /// book), \f$ x_0 , \ldots , x_7 \f$ the bytes of \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tabulation}( x ) \cr
 /// 2 & \quad \textbf{return}\ T_0[ x_0 ] \oplus T_1[ x_1 ] \oplus \cdots
 ///     \oplus T_7[ x_7 ]
 /// \end{array}}
 /// \f]
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
