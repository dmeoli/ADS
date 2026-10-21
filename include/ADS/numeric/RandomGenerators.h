/*--------------------------------------------------------------------------*/
/*------------------------ File RandomGenerators.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered random numbers, beyond the book: generators of 64 bits
 * that cost a few instructions and pass the statistical batteries
 * (BigCrush of TestU01, PractRand), and the uses of a word that need no
 * division.
 *
 * - Xoshiro256 is xoshiro256++ of Blackman and Vigna: the state \f$ s
 *   \in \mathbb{F}_2^{256} \f$ goes by a linear map of xor, shifts and
 *   rotations of full period \f$ 2^{256} - 1 \f$ (all the states but 0),
 *   and the output \f$ \mathrm{rotl}( s_0 + s_3 , 23 ) + s_0 \f$ scrambles
 *   it; jump() advances it by \f$ 2^{128} \f$ steps (the product of the
 *   state by a fixed polynomial of the map, 256 steps), so that each of \f$
 *   2^{128} \f$ streams of \f$ 2^{128} \f$ numbers serves a thread.
 *   Xoshiro256StarStar is xoshiro256** on the same state, the output
 *   \f$ \mathrm{rotl}( 5 s_1 , 7 ) \cdot 9 \f$. The seed fills the state
 *   by SplitMix64 of Steele, Lea and Flood [see RandomHashing.h].
 * - WyRand is the generator of Wang Yi: the Weyl sequence \f$ s_{i+1} = s_i
 *   + c \bmod 2^{64} \f$, c odd, of period \f$ 2^{64} \f$, hashed by the
 *   folding of a product of 128 bits, \f$ h \oplus l \f$ for \f$ h R + l =
 *   s ( s \oplus c' ) \f$.
 * - Philox4x32 and Philox4x64 are the counter-based generators Philox4xW-10
 *   of Salmon, Moraes, Dror and Shaw: the output of the counter c (4 words
 *   of W bits) under the key k (2 words) is 10 rounds of the bijection
 *   \f$ ( c_0 , c_1 , c_2 , c_3 ) \mapsto ( h( M_1 c_2 ) \oplus c_1 \oplus
 *   k_0 , l( M_1 c_2 ) , h( M_0 c_0 ) \oplus c_3 \oplus k_1 , l( M_0 c_0 )
 *   ) \f$, h and l the high and the low word of a product of 2W bits, the
 *   key bumped by the Weyl constants between the rounds; the counter only
 *   increases, so that discard( z ) costs O( 1 ) and any block of the
 *   stream is computed on its own. The sequence is that of std::philox4x32
 *   and std::philox4x64 of C++26 (the key ( seed , 0 ), the counter from
 *   0, the words of a block in order); two blocks are computed at once,
 *   their rounds interleaved, so that the products of one fill the
 *   latency of those of the other.
 * - normalZiggurat() and exponentialZiggurat() are the ziggurats of
 *   Marsaglia and Tsang with 256 layers of equal area v under the density
 *   [see ZigguratTables.tpp]: one word gives the layer i (its low 8 bits)
 *   and a uniform u of [ -1 , 1 ) or [ 0 , 1 ) (its top 53 bits, apart
 *   from the layer, as Leong et al. ask), and \f$ u x_i \f$ is the result
 *   when \f$ | u | < x_{i+1} / x_i \f$, about 98.8% (normal) and 98.9%
 *   (exponential) of the draws, at the cost of a word, a product and a
 *   comparison; otherwise a uniform height decides the wedge against the
 *   density, and in the base strip the tail beyond r is drawn by the
 *   exponentials of Marsaglia (normal) or from r again (exponential, with
 *   no memory). The tables are constants, and the functions are inlined,
 *   so that the state of the generator stays in the registers of the
 *   caller.
 * - uniformBelow( g , s ) is the integer uniform in [ 0 , s ) of Lemire:
 *   for the word x, \f$ x s = h 2^{64} + l \f$ and the result is h, which
 *   is uniform once the l below \f$ t = 2^{64} \bmod s \f$ are rejected;
 *   since \f$ t < s \f$, the comparison \f$ l < s \f$ decides without the
 *   division in all but a fraction \f$ s / 2^{64} \f$ of the draws, a
 *   draw is rejected with probability \f$ t / 2^{64} < s / 2^{64} \f$, and
 *   the expected words per result are \f$ 2^{64} / ( 2^{64} - t ) \f$;
 *   uniformReal() is \f$ \lfloor x / 2^{11} \rfloor 2^{-53} \f$, uniform on
 *   the \f$ 2^{53} \f$ doubles \f$ k 2^{-53} \f$ of [ 0 , 1 ).
 * - shuffleFast() is the shuffle of Fisher and Yates [see shuffle() of
 *   RandomNumbers.h] with the indices drawn in batches, after Brackett-
 *   Rozinsky and Lemire: one word x gives the indices of k consecutive
 *   steps with bounds \f$ b_1 , \dots , b_k \f$ of product \f$ p \le
 *   2^{64} \f$, the j-th the high word of \f$ l_{j-1} b_j \f$ with \f$ l_0
 *   = x \f$ and \f$ l_j \f$ its low word, which is uniform on the
 *   \f$ \prod b_j \f$ tuples once \f$ l_k < 2^{64} \bmod p \f$ is
 *   rejected, with probability below \f$ p / 2^{64} \f$; 6 steps a word
 *   while the bounds are below \f$ 2^{10} \f$ (\f$ p < 2^{60} \f$), 2
 *   below \f$ 2^{32} \f$, so that a shuffle of n items draws about
 *   \f$ n / 2 \f$ words instead of n.
 *
 * - AliasTable is the alias method of Walker in the construction of Vose:
 *   with \f$ p_i = n w_i / \sum_j w_j \f$, the items with \f$ p_i < 1 \f$
 *   and those with \f$ p_i \ge 1 \f$ in two lists, each step gives to a
 *   small one its alias, a large one that gives up \f$ 1 - p_i \f$ of its
 *   mass, so that the cell i keeps i with probability \f$ p_i \f$ and
 *   gives its alias otherwise: O( n ) time and space to build, the
 *   probabilities exact up to the rounding of \f$ p_i \f$. A draw is one
 *   word x: \f$ x n = h 2^{64} + l \f$, the cell h and the coin \f$ l <
 *   \lfloor p_h 2^{64} \rfloor \f$ from the same product, O( 1 ) time and
 *   two loads, against the \f$ O( \log n ) \f$ of the search in the
 *   cumulative sums; the cells and the coins are off the exact ones by at
 *   most \f$ n / 2^{64} \f$ each, as for the products of Lemire with no
 *   rejection.
 *
 * The generators meet UniformRandomBitGenerator, with min() = 0 and max()
 * = \f$ 2^{64} - 1 \f$, so that they serve the distributions of std; the
 * functions above need such a generator of 64 full bits.
 *
 * The contracts [see Contract.h]: s >= 1 for uniformBelow(), and its
 * result is below s. The functions are defined in RandomGenerators.tpp.
 *
 * \see D. Blackman, S. Vigna, Scrambled linear pseudorandom number
 *      generators, ACM Transactions on Mathematical Software 47(4), 36:1-32,
 *      2021, doi:10.1145/3460772
 * \see G. L. Steele, D. Lea, C. H. Flood, Fast splittable pseudorandom
 *      number generators, Proceedings of the 2014 ACM International
 *      Conference on Object Oriented Programming Systems Languages and
 *      Applications (OOPSLA), 453-472, 2014, doi:10.1145/2660193.2660195
 * \see W. Yi, wyhash and wyrand, https://github.com/wangyi-fudan/wyhash
 * \see D. Lemire, Fast random integer generation in an interval, ACM
 *      Transactions on Modeling and Computer Simulation 29(1), 3:1-12,
 *      2019, doi:10.1145/3230636
 * \see R. A. Fisher, F. Yates, Statistical Tables for Biological,
 *      Agricultural and Medical Research, Oliver and Boyd, 1938, and R.
 *      Durstenfeld, Algorithm 235: random permutation, Communications of
 *      the ACM 7(7), 420, 1964, doi:10.1145/364520.364540
 * \see N. Brackett-Rozinsky, D. Lemire, Batched ranged random integer
 *      generation, Software: Practice and Experience 55(1), 155-169, 2025,
 *      doi:10.1002/spe.3369
 * \see J. K. Salmon, M. A. Moraes, R. O. Dror, D. E. Shaw, Parallel random
 *      numbers: as easy as 1, 2, 3, International Conference for High
 *      Performance Computing, Networking, Storage and Analysis (SC), 1-12,
 *      2011, doi:10.1145/2063384.2063405
 * \see G. Marsaglia, W. W. Tsang, The ziggurat method for generating random
 *      variables, Journal of Statistical Software 5(8), 2000,
 *      doi:10.18637/jss.v005.i08
 * \see P. H. W. Leong, G. Zhang, D.-U. Lee, W. Luk, J. D. Villasenor, A
 *      comment on the implementation of the ziggurat method, Journal of
 *      Statistical Software 12(7), 2005, doi:10.18637/jss.v012.i07
 * \see A. J. Walker, An efficient method for generating discrete random
 *      variables with general distributions, ACM Transactions on
 *      Mathematical Software 3(3), 253-256, 1977, doi:10.1145/355744.355749
 * \see M. D. Vose, A linear algorithm for generating random numbers with a
 *      given distribution, IEEE Transactions on Software Engineering
 *      17(9), 972-975, 1991, doi:10.1109/32.92917
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

#ifndef __ADS_RandomGenerators
 #define __ADS_RandomGenerators
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/numeric/NumberTheory.h"

#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// a function inlined where it is used, so that the state of the
/// generator stays in the registers of the caller's loop
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_RNG_INLINE inline __attribute__(( always_inline ))
#elif defined( _MSC_VER )
 #define ADS_RNG_INLINE __forceinline
#else
 #define ADS_RNG_INLINE inline
#endif

namespace ADS {

/// a UniformRandomBitGenerator of 64 full bits
template< class G >
concept FullWordGenerator =
 std::uniform_random_bit_generator< std::remove_reference_t< G > > &&
 std::same_as< typename std::remove_reference_t< G >::result_type ,
               std::uint64_t > &&
 ( std::remove_reference_t< G >::min() == 0 ) &&
 ( std::remove_reference_t< G >::max() ==
   std::numeric_limits< std::uint64_t >::max() );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Xoshiro256 -----------------------------*/
/*--------------------------------------------------------------------------*/

/// xoshiro256++ (or xoshiro256** if StarStar), a UniformRandomBitGenerator
/// of 64 bits
template< bool StarStar >
class Xoshiro256T {
 public:
 using result_type = std::uint64_t;

 /// the state from the seed by SplitMix64, never all zero
 explicit constexpr Xoshiro256T( std::uint64_t seed = 1 );

 constexpr std::uint64_t operator()( void );

 /// advances by 2^128 steps, a stream of 2^128 numbers apart
 constexpr void jump( void );

 static constexpr std::uint64_t min( void ) { return( 0 ); }
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 bool operator==( const Xoshiro256T & ) const = default;

 private:
 std::uint64_t f_s[ 4 ];
 };  // end( class Xoshiro256T )

using Xoshiro256 = Xoshiro256T< false >;          ///< xoshiro256++
using Xoshiro256StarStar = Xoshiro256T< true >;   ///< xoshiro256**

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS WyRand -------------------------------*/
/*--------------------------------------------------------------------------*/

/// wyrand, a UniformRandomBitGenerator of 64 bits
class WyRand {
 public:
 using result_type = std::uint64_t;

 explicit constexpr WyRand( std::uint64_t seed = 1 ) : f_s( seed ) {}

 constexpr std::uint64_t operator()( void );

 static constexpr std::uint64_t min( void ) { return( 0 ); }
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 bool operator==( const WyRand & ) const = default;

 private:
 std::uint64_t f_s;
 };  // end( class WyRand )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS PhiloxT ------------------------------*/
/*--------------------------------------------------------------------------*/

/// Philox4xW-10, the counter-based UniformRandomBitGenerator of words of W
/// bits (32 or 64) of Salmon, Moraes, Dror and Shaw, with the sequence of
/// std::philox4x32 and std::philox4x64 of C++26
template< class W >
class PhiloxT {
 static_assert( std::same_as< W , std::uint32_t > ||
                std::same_as< W , std::uint64_t > ,
                "words of 32 or 64 bits" );
 public:
 using result_type = W;
 using Block = std::array< W , 4 >;   ///< a counter, or its 4 outputs
 using Key = std::array< W , 2 >;     ///< a key

 static constexpr W default_seed = 20111115u;

 /// the key ( seed , 0 ) and the counter 0
 explicit constexpr PhiloxT( W seed = default_seed );

 constexpr W operator()( void );

 /// advances by z outputs, in O( 1 ) blocks
 constexpr void discard( unsigned long long z );

 /// the counter of the next output, which will be the first word of its
 /// block
 constexpr void setCounter( const Block & c );

 /// the block of the counter c under the key k: 10 rounds of the products
 /// of 2W bits by the multipliers, with the key bumped by the Weyl
 /// constants between the rounds
 static constexpr Block block( Block c , Key k );

 static constexpr W min( void ) { return( 0 ); }
 static constexpr W max( void ) { return( std::numeric_limits< W >::max() ); }

 bool operator==( const PhiloxT & ) const = default;

 private:
 /// the blocks computed at once, their rounds interleaved (more run out
 /// of registers, as measured on a Xeon Gold 5120)
 static constexpr unsigned lanes = 2;

 /// the next lanes blocks into the buffer
 constexpr void refill( void );

 Key f_key;                                ///< the key
 Block f_ctr{};                            ///< the counter after the buffer
 W f_buf[ 4 * lanes ] = {};                ///< the outputs not yet given
 unsigned f_at = 4 * lanes;                ///< the next one
 };  // end( class PhiloxT )

using Philox4x32 = PhiloxT< std::uint32_t >;   ///< Philox4x32-10
using Philox4x64 = PhiloxT< std::uint64_t >;   ///< Philox4x64-10

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS AliasTable -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the alias method of Walker and Vose: draws i with probability
/// w_i / sum w, in O( 1 ) time from one random word
class AliasTable {
 public:
 /// the table of the weights w, not all zero
 /// \pre w not empty, its items finite and >= 0 with a positive sum, and
 ///      fewer than 2^32 of them
 explicit AliasTable( const std::vector< double > & w );

 [[nodiscard]] std::size_t size( void ) const { return( f_T.size() ); }

 /// an index drawn with probability proportional to its weight
 template< FullWordGenerator G >
 std::size_t operator()( G && g ) const;

 /// the probability of i, as the table gives it
 /// \pre i < size()
 [[nodiscard]] double probability( std::size_t i ) const;

 private:
 struct Entry {
  std::uint64_t keep;      ///< the threshold of the low word to keep i
  std::uint32_t alias;     ///< the index otherwise
  };
 std::vector< Entry > f_T;
 };  // end( class AliasTable )

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer uniform in [ 0 , s ), by the method of Lemire
/// \pre s >= 1
/// \post the result is below s
template< FullWordGenerator G >
std::uint64_t uniformBelow( G && g , std::uint64_t s );

/// a double uniform in [ 0 , 1 ), the top 53 bits of a word
template< FullWordGenerator G >
double uniformReal( G && g );

/// a normal deviate of mean 0 and variance 1, by the ziggurat of Marsaglia
/// and Tsang with 256 layers
template< FullWordGenerator G >
ADS_RNG_INLINE double normalZiggurat( G && g );

/// an exponential deviate of mean 1, by the ziggurat of Marsaglia and
/// Tsang with 256 layers
template< FullWordGenerator G >
ADS_RNG_INLINE double exponentialZiggurat( G && g );

/// the items of [ first , last ) shuffled by g, every permutation equally
/// likely, the indices drawn in batches
template< std::random_access_iterator I , FullWordGenerator G >
void shuffleFast( I first , I last , G && g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/RandomGenerators.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RandomGenerators.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File RandomGenerators.h ----------------------*/
/*--------------------------------------------------------------------------*/
