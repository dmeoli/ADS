/*--------------------------------------------------------------------------*/
/*------------------------ File RandomGenerators.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered random numbers, beyond the book: generators of 64 bits that
 * cost a few instructions and pass the statistical batteries (BigCrush of
 * TestU01, PractRand), and the uses of a word that need no division.
 *
 * - `Xoshiro256` is xoshiro256++ of Blackman and Vigna: the state
 *   \f$ s \in \mathbb{F}_2^{256} \f$ goes by a linear map of xor, shifts and
 *   rotations of full period \f$ 2^{256} - 1 \f$ (all the states but 0), and
 *   the output \f$ \mathrm{rotl}( s_0 + s_3 , 23 ) + s_0 \f$ scrambles it;
 *   `jump()` advances it by \f$ 2^{128} \f$ steps (the product of the state
 *   by a fixed polynomial of the map, 256 steps), so that each of
 *   \f$ 2^{128} \f$ streams of \f$ 2^{128} \f$ numbers serves a thread.
 *   `Xoshiro256StarStar` is xoshiro256** on the same state, the output
 *   \f$ \mathrm{rotl}( 5 s_1 , 7 ) \cdot 9 \f$. The seed fills the state by
 *   SplitMix64 of Steele, Lea and Flood [see `RandomHashing.h`].
 * - `WyRand` is the generator of Wang Yi: the Weyl sequence
 *   \f$ s_{i+1} = s_i + c \bmod 2^{64} \f$, \f$ c \f$ odd, of period
 *   \f$ 2^{64} \f$, hashed by the folding of a product of 128 bits,
 *   \f$ h \oplus l \f$ for \f$ h R + l = s ( s \oplus c' ) \f$.
 * - `Philox4x32` and `Philox4x64` are the counter-based generators
 *   Philox4xW-10 of Salmon, Moraes, Dror and Shaw: the output of the counter
 *   \f$ c \f$ (4 words of \f$ W \f$ bits) under the key \f$ k \f$ (2 words)
 *   is 10 rounds of the bijection
 *   \f$ ( c_0 , c_1 , c_2 , c_3 ) \mapsto ( h( M_1 c_2 ) \oplus c_1 \oplus
 *   k_0 , l( M_1 c_2 ) , h( M_0 c_0 ) \oplus c_3 \oplus k_1 , l( M_0 c_0 )
 *   ) \f$, \f$ h \f$ and \f$ l \f$ the high and the low word of a product of
 *   \f$ 2 W \f$ bits, the key bumped by the Weyl constants between the
 *   rounds; the counter only increases, so that `discard( z )` costs
 *   \f$ O( 1 ) \f$ and any block of the stream is computed on its own. The
 *   sequence is that of `std::philox4x32` and `std::philox4x64` of C++26
 *   (the key \f$ ( \mathit{seed} , 0 ) \f$, the counter from 0, the words of
 *   a block in order); two blocks are computed at once, their rounds
 *   interleaved, so that the products of one fill the latency of those of
 *   the other.
 * - `normalZiggurat()` and `exponentialZiggurat()` are the ziggurats of
 *   Marsaglia and Tsang with 256 layers of equal area \f$ v \f$ under the
 *   density [see `ZigguratTables.tpp`]: one word gives the layer \f$ i \f$
 *   (its low 8 bits) and a uniform \f$ u \f$ of \f$ [ -1 , 1 ) \f$ or
 *   \f$ [ 0 , 1 ) \f$ (its top 53 bits, apart from the layer, as Leong and
 *   others ask), and \f$ u x_i \f$ is the result when
 *   \f$ | u | < x_{i+1} / x_i \f$, about 98.8% (normal) and 98.9%
 *   (exponential) of the draws, at the cost of a word, a product and a
 *   comparison; otherwise a uniform height decides the wedge against the
 *   density, and in the base strip the tail beyond \f$ r \f$ is drawn by the
 *   exponentials of Marsaglia (normal) or from \f$ r \f$ again (exponential,
 *   with no memory). The tables are constants, and the functions are
 *   inlined, so that the state of the generator stays in the registers of
 *   the caller.
 * - `uniformBelow( g , s )` is the integer uniform in \f$ [ 0 , s ) \f$ of
 *   Lemire: for the word \f$ x \f$, \f$ x s = h 2^{64} + l \f$ and the result
 *   is \f$ h \f$, which is uniform once the \f$ l \f$ below
 *   \f$ t = 2^{64} \bmod s \f$ are rejected; since \f$ t < s \f$, the
 *   comparison \f$ l < s \f$ decides without the division in all but a
 *   fraction \f$ s / 2^{64} \f$ of the draws, a draw is rejected with
 *   probability \f$ t / 2^{64} < s / 2^{64} \f$, and the expected words per
 *   result are \f$ 2^{64} / ( 2^{64} - t ) \f$; `uniformReal()` is
 *   \f$ \lfloor x / 2^{11} \rfloor 2^{-53} \f$, uniform on the
 *   \f$ 2^{53} \f$ doubles \f$ k 2^{-53} \f$ of \f$ [ 0 , 1 ) \f$.
 * - `shuffleFast()` is the shuffle of Fisher and Yates [see `shuffle()` of
 *   `RandomNumbers.h`] with the indices drawn in batches, after
 *   Brackett-Rozinsky and Lemire: one word \f$ x \f$ gives the indices of
 *   \f$ k \f$ consecutive steps with bounds \f$ b_1 , \dots , b_k \f$ of
 *   product \f$ p \le 2^{64} \f$, the \f$ j \f$-th the high word of
 *   \f$ l_{j-1} b_j \f$ with \f$ l_0 = x \f$ and \f$ l_j \f$ its low word,
 *   which is uniform on the \f$ \prod b_j \f$ tuples once
 *   \f$ l_k < 2^{64} \bmod p \f$ is rejected, with probability below
 *   \f$ p / 2^{64} \f$; 6 steps a word while the bounds are below
 *   \f$ 2^{10} \f$ (\f$ p < 2^{60} \f$), 2 below \f$ 2^{32} \f$, so that a
 *   shuffle of \f$ n \f$ items draws about \f$ n / 2 \f$ words instead of
 *   \f$ n \f$.
 * - `AliasTable` is the alias method of Walker in the construction of Vose:
 *   with \f$ p_i = n w_i / \sum_j w_j \f$, the items with \f$ p_i < 1 \f$ and
 *   those with \f$ p_i \ge 1 \f$ in two lists, each step gives to a small one
 *   its alias, a large one that gives up \f$ 1 - p_i \f$ of its mass, so that
 *   the cell \f$ i \f$ keeps \f$ i \f$ with probability \f$ p_i \f$ and gives
 *   its alias otherwise: \f$ O( n ) \f$ time and space to build, the
 *   probabilities exact up to the rounding of \f$ p_i \f$. A draw is one word
 *   \f$ x \f$: \f$ x n = h 2^{64} + l \f$, the cell \f$ h \f$ and the coin
 *   \f$ l < \lfloor p_h 2^{64} \rfloor \f$ from the same product,
 *   \f$ O( 1 ) \f$ time and two loads, against the \f$ O( \log n ) \f$ of the
 *   search in the cumulative sums; the cells and the coins are off the exact
 *   ones by at most \f$ n / 2^{64} \f$ each, as for the products of Lemire
 *   with no rejection.
 *
 * The generators meet `std::uniform_random_bit_generator`, with
 * \f$ \mathrm{min}() = 0 \f$ and \f$ \mathrm{max}() = 2^{64} - 1 \f$, so that
 * they serve the distributions of `std`; the functions above need such a
 * generator of 64 full bits.
 *
 * The contracts [see `Contract.h`]: \f$ s \ge 1 \f$ for `uniformBelow()`,
 * and its result is below \f$ s \f$. The functions are defined in
 * `RandomGenerators.tpp`.
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
 *         Università di Pisa \n
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

/// a `std::uniform_random_bit_generator` of 64 full bits, from 0 to
/// \f$ 2^{64} - 1 \f$
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

/// xoshiro256++ (or xoshiro256** if `StarStar`), a
/// `std::uniform_random_bit_generator` of 64 bits
template< bool StarStar >
class Xoshiro256T {
 public:
 using result_type = std::uint64_t;   ///< the type of the numbers

 /// the state from the seed by SplitMix64, never all zero
 explicit constexpr Xoshiro256T( std::uint64_t seed = 1 );

 /// the next number
 ///
 /// In pseudocode (Blackman and Vigna, beyond the book), the output from
 /// the state before the step:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{next}() \cr
 /// 2 & \quad r \gets \mathrm{rotl}( s_0 + s_3 , 23 ) + s_0\quad
 ///     \triangleright\ \mathrm{rotl}( 5 s_1 , 7 ) \cdot 9 \text{ for **} \cr
 /// 3 & \quad t \gets s_1 \ll 17 \cr
 /// 4 & \quad s_2 \gets s_2 \oplus s_0;\ s_3 \gets s_3 \oplus s_1;\ s_1
 ///     \gets s_1 \oplus s_2;\ s_0 \gets s_0 \oplus s_3 \cr
 /// 5 & \quad s_2 \gets s_2 \oplus t;\ s_3 \gets \mathrm{rotl}( s_3 , 45 )
 ///     \cr
 /// 6 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 constexpr std::uint64_t operator()( void );

 /// advances by \f$ 2^{128} \f$ steps, a stream of \f$ 2^{128} \f$ numbers
 /// apart
 ///
 /// In pseudocode (Blackman and Vigna, beyond the book), \f$ J \f$ the 256
 /// bits of the polynomial of the jump:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{jump}() \cr
 /// 2 & \quad a \gets 0 \cr
 /// 3 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ 255\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ J_b = 1\ \textbf{then}\ a \gets a \oplus s \cr
 /// 5 & \qquad \mathrm{next}() \cr
 /// 6 & \quad s \gets a
 /// \end{array}}
 /// \f]
 constexpr void jump( void );

 /// the least number, 0
 static constexpr std::uint64_t min( void ) { return( 0 ); }
 /// the largest number, \f$ 2^{64} - 1 \f$
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 /// true if the two states are equal
 bool operator==( const Xoshiro256T & ) const = default;

 private:
 std::uint64_t f_s[ 4 ];   ///< the state \f$ s_0 , \ldots , s_3 \f$
 };  // end( class Xoshiro256T )

using Xoshiro256 = Xoshiro256T< false >;          ///< xoshiro256++
using Xoshiro256StarStar = Xoshiro256T< true >;   ///< xoshiro256**

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS WyRand -------------------------------*/
/*--------------------------------------------------------------------------*/

/// wyrand, a `std::uniform_random_bit_generator` of 64 bits
class WyRand {
 public:
 using result_type = std::uint64_t;   ///< the type of the numbers

 /// the generator whose Weyl sequence starts at the seed
 explicit constexpr WyRand( std::uint64_t seed = 1 ) : f_s( seed ) {}

 /// the next number
 ///
 /// In pseudocode (Wang Yi, beyond the book), \f$ c \f$ and \f$ c' \f$ two
 /// constants of 64 bits, \f$ c \f$ odd:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{next}() \cr
 /// 2 & \quad s \gets s + c \bmod 2^{64} \cr
 /// 3 & \quad ( h , l ) \gets \text{the high and the low word of } s ( s
 ///     \oplus c' ) \cr
 /// 4 & \quad \textbf{return}\ h \oplus l
 /// \end{array}}
 /// \f]
 constexpr std::uint64_t operator()( void );

 /// the least number, 0
 static constexpr std::uint64_t min( void ) { return( 0 ); }
 /// the largest number, \f$ 2^{64} - 1 \f$
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 /// true if the two states are equal
 bool operator==( const WyRand & ) const = default;

 private:
 std::uint64_t f_s;   ///< the state, the Weyl sequence
 };  // end( class WyRand )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS PhiloxT ------------------------------*/
/*--------------------------------------------------------------------------*/

/// Philox4xW-10, the counter-based `std::uniform_random_bit_generator` of
/// words of \f$ W \f$ bits (32 or 64) of Salmon, Moraes, Dror and Shaw, with
/// the sequence of `std::philox4x32` and `std::philox4x64` of C++26
template< class W >
class PhiloxT {
 static_assert( std::same_as< W , std::uint32_t > ||
                std::same_as< W , std::uint64_t > ,
                "words of 32 or 64 bits" );
 public:
 using result_type = W;               ///< the type of the numbers
 using Block = std::array< W , 4 >;   ///< a counter, or its 4 outputs
 using Key = std::array< W , 2 >;     ///< a key

 /// the seed of the default constructor, as in C++26
 static constexpr W default_seed = 20111115u;

 /// the key \f$ ( \mathit{seed} , 0 ) \f$ and the counter 0
 explicit constexpr PhiloxT( W seed = default_seed );

 /// the next number, the next word of the buffer of two blocks, which is
 /// filled again when it is empty
 constexpr W operator()( void );

 /// advances by \f$ z \f$ outputs, in \f$ O( 1 ) \f$ blocks
 ///
 /// In pseudocode (beyond the book), the counter moved instead of the
 /// blocks computed:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{discard}( z ) \cr
 /// 2 & \quad \textbf{if}\ z \le \text{the outputs left in the buffer}
 ///     \ \textbf{then}\ \text{skip them};\ \textbf{return} \cr
 /// 3 & \quad z \gets z - \text{those left};\ \mathit{ctr} \gets
 ///     \mathit{ctr} + \lfloor z / 4 \rfloor\quad \triangleright\ \text{on }
 ///     4 W \text{ bits} \cr
 /// 4 & \quad \text{empty the buffer, and draw } z \bmod 4 \text{ outputs}
 /// \end{array}}
 /// \f]
 constexpr void discard( unsigned long long z );

 /// the counter of the next output, which will be the first word of its
 /// block
 constexpr void setCounter( const Block & c );

 /// the block of the counter \f$ c \f$ under the key \f$ k \f$: 10 rounds
 /// of the products of \f$ 2 W \f$ bits by the multipliers, with the key
 /// bumped by the Weyl constants between the rounds
 ///
 /// In pseudocode (Salmon and others, beyond the book), \f$ M_0 , M_1 \f$
 /// the multipliers and \f$ w_0 , w_1 \f$ the Weyl constants:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{block}( c , k ) \cr
 /// 2 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ 9\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ r > 0\ \textbf{then}\ k_0 \gets k_0 + w_0;\ k_1
 ///     \gets k_1 + w_1 \cr
 /// 4 & \qquad ( h_0 , l_0 ) \gets M_0 c_0;\ ( h_1 , l_1 ) \gets M_1 c_2 \cr
 /// 5 & \qquad c \gets ( h_1 \oplus c_1 \oplus k_0 , l_1 , h_0 \oplus c_3
 ///     \oplus k_1 , l_0 ) \cr
 /// 6 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 static constexpr Block block( Block c , Key k );

 /// the least number, 0
 static constexpr W min( void ) { return( 0 ); }
 /// the largest number, \f$ 2^W - 1 \f$
 static constexpr W max( void ) { return( std::numeric_limits< W >::max() ); }

 /// true if the two states are equal
 bool operator==( const PhiloxT & ) const = default;

 private:
 /// the blocks computed at once, their rounds interleaved (more run out
 /// of registers, as measured on a Xeon Gold 5120)
 static constexpr unsigned lanes = 2;

 /// the next `lanes` blocks into the buffer, their rounds interleaved
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

/// the alias method of Walker and Vose: draws \f$ i \f$ with probability
/// \f$ w_i / \sum_j w_j \f$, in \f$ O( 1 ) \f$ time from one random word
class AliasTable {
 public:
 /// the table of the weights \f$ w \f$, not all zero
 ///
 /// In pseudocode (the construction of Vose, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{AliasTable}( w ) \cr
 /// 2 & \quad p_i \gets n w_i / \textstyle\sum_j w_j \text{ for each } i \cr
 /// 3 & \quad S \gets \{ i : p_i < 1 \};\ L \gets \{ i : p_i \ge 1 \} \cr
 /// 4 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{and}\ L \ne
 ///     \emptyset\ \textbf{do} \cr
 /// 5 & \qquad s \gets \text{an item of } S;\ l \gets \text{an item of } L;
 ///     \ S \gets S \setminus \{ s \} \cr
 /// 6 & \qquad \mathit{keep}[ s ] \gets p_s;\ \mathit{alias}[ s ] \gets l;
 ///     \ p_l \gets p_l + p_s - 1 \cr
 /// 7 & \qquad \textbf{if}\ p_l < 1\ \textbf{then}\ \text{move } l
 ///     \text{ from } L \text{ to } S \cr
 /// 8 & \quad \textbf{foreach}\ i \in S \cup L\ \textbf{do}
 ///     \ \mathit{keep}[ i ] \gets 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ w \f$ not empty, its items finite and \f$ \ge 0 \f$ with a
 ///      positive sum, and at most \f$ 2^{32} - 1 \f$ of them
 explicit AliasTable( const std::vector< double > & w );

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const { return( f_T.size() ); }

 /// an index drawn with probability proportional to its weight
 ///
 /// In pseudocode (Walker, beyond the book), the cell and the coin from one
 /// product:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{draw}( g ) \cr
 /// 2 & \quad ( h , l ) \gets \text{the high and the low word of } g() \cdot
 ///     n \cr
 /// 3 & \quad \textbf{if}\ l < \lfloor \mathit{keep}[ h ] 2^{64} \rfloor
 ///     \ \textbf{then return}\ h \cr
 /// 4 & \quad \textbf{return}\ \mathit{alias}[ h ]
 /// \end{array}}
 /// \f]
 template< FullWordGenerator G >
 std::size_t operator()( G && g ) const;

 /// the probability of \f$ i \f$, as the table gives it
 /// \pre \f$ i < \mathrm{size}() \f$
 [[nodiscard]] double probability( std::size_t i ) const;

 private:
 /// a cell of the table
 struct Entry {
  std::uint64_t keep;      ///< the threshold of the low word to keep it
  std::uint32_t alias;     ///< the index otherwise
  };
 std::vector< Entry > f_T;   ///< the cells
 };  // end( class AliasTable )

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer uniform in \f$ [ 0 , s ) \f$, by the method of Lemire
///
/// In pseudocode (Lemire, beyond the book), the division only when a
/// rejection is possible:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{uniformBelow}( g , s ) \cr
/// 2 & \quad ( h , l ) \gets \text{the high and the low word of } g() \cdot
///     s \cr
/// 3 & \quad \textbf{if}\ l < s\ \textbf{then} \cr
/// 4 & \qquad t \gets 2^{64} \bmod s \cr
/// 5 & \qquad \textbf{while}\ l < t\ \textbf{do}\ ( h , l ) \gets g() \cdot
///     s \cr
/// 6 & \quad \textbf{return}\ h
/// \end{array}}
/// \f]
/// \pre \f$ s \ge 1 \f$
/// \post the result is below \f$ s \f$
template< FullWordGenerator G >
std::uint64_t uniformBelow( G && g , std::uint64_t s );

/// a double uniform in \f$ [ 0 , 1 ) \f$, the top 53 bits of a word times
/// \f$ 2^{-53} \f$
template< FullWordGenerator G >
double uniformReal( G && g );

/// a normal deviate of mean 0 and variance 1, by the ziggurat of Marsaglia
/// and Tsang with 256 layers
///
/// In pseudocode (Marsaglia and Tsang, beyond the book), \f$ x_i \f$ the
/// right ends of the layers, \f$ y_i \f$ the density there, and
/// \f$ r = x_1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{normalZiggurat}( g ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad w \gets g();\ i \gets w \bmod 256;\ u \gets \text{the top 53 }
///     \text{bits of } w \text{ in } [ -1 , 1 ) \cr
/// 4 & \qquad x \gets u x_i \cr
/// 5 & \qquad \textbf{if}\ |u| < x_{i+1} / x_i\ \textbf{then return}\ x \cr
/// 6 & \qquad \textbf{if}\ i = 0\ \textbf{then} \cr
/// 7 & \qquad\quad \textbf{repeat}\ a \gets -\ln( U_1 ) / r;\ b \gets -\ln(
///     U_2 )\ \textbf{until}\ 2 b \ge a^2 \cr
/// 8 & \qquad\quad \textbf{return}\ \pm ( r + a ) \text{, the sign of } u \cr
/// 9 & \qquad \textbf{if}\ y_{i+1} + ( y_i - y_{i+1} ) U < e^{-x^2/2}
///     \ \textbf{then return}\ x
/// \end{array}}
/// \f]
template< FullWordGenerator G >
ADS_RNG_INLINE double normalZiggurat( G && g );

/// an exponential deviate of mean 1, by the ziggurat of Marsaglia and
/// Tsang with 256 layers
///
/// In pseudocode (Marsaglia and Tsang, beyond the book), the tail drawn
/// again from \f$ r = x_1 \f$, since the density has no memory:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{exponentialZiggurat}( g ) \cr
/// 2 & \quad b \gets 0 \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad w \gets g();\ i \gets w \bmod 256;\ u \gets \text{the top 53 }
///     \text{bits of } w \text{ in } [ 0 , 1 );\ x \gets u x_i \cr
/// 5 & \qquad \textbf{if}\ u < x_{i+1} / x_i\ \textbf{then return}\ b + x \cr
/// 6 & \qquad \textbf{if}\ i = 0\ \textbf{then}\ b \gets b + r \cr
/// 7 & \qquad \textbf{else if}\ y_{i+1} + ( y_i - y_{i+1} ) U < e^{-x}
///     \ \textbf{then return}\ b + x
/// \end{array}}
/// \f]
template< FullWordGenerator G >
ADS_RNG_INLINE double exponentialZiggurat( G && g );

/// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$ shuffled by
/// \f$ g \f$, every permutation equally likely, the indices drawn in
/// batches
///
/// In pseudocode (Fisher and Yates, with the batches of Brackett-Rozinsky
/// and Lemire, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{shuffleFast}( A , g )\quad
///     \triangleright\ n = |A| \cr
/// 2 & \quad \textbf{while}\ n > 1\ \textbf{do} \cr
/// 3 & \qquad k \gets 1 \text{ if } n > 2^{32} \text{, } 2 \text{ if } n >
///     2^{10} \text{, } 6 \text{ if } n > 6 \text{, else } 1 \cr
/// 4 & \qquad p \gets n ( n - 1 ) \cdots ( n - k + 1 ) \cr
/// 5 & \qquad \textbf{repeat}\ l \gets g() \cr
/// 6 & \qquad\quad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
///     \ ( \mathit{idx}_j , l ) \gets l \cdot ( n - j ) \cr
/// 7 & \qquad \textbf{until}\ l \ge p\ \textbf{or}\ l \ge 2^{64} \bmod p \cr
/// 8 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
///     \ \mathrm{swap}( A[ n - 1 - j ] , A[ \mathit{idx}_j ] ) \cr
/// 9 & \qquad n \gets n - k
/// \end{array}}
/// \f]
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
