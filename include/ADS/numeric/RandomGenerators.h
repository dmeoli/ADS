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
 * - WyRand is the generator of Wang Yi: the Weyl sequence \f$ s_{i+1} = s_i
 *   + c \bmod 2^{64} \f$, c odd, of period \f$ 2^{64} \f$, hashed by the
 *   folding of a product of 128 bits, \f$ h \oplus l \f$ for \f$ h R + l =
 *   s ( s \oplus c' ) \f$.
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

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

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

/// xoshiro256++, a UniformRandomBitGenerator of 64 bits
class Xoshiro256 {
 public:
 using result_type = std::uint64_t;

 /// the state from the seed by SplitMix64, never all zero
 explicit constexpr Xoshiro256( std::uint64_t seed = 1 );

 constexpr std::uint64_t operator()( void );

 /// advances by 2^128 steps, a stream of 2^128 numbers apart
 constexpr void jump( void );

 static constexpr std::uint64_t min( void ) { return( 0 ); }
 static constexpr std::uint64_t max( void ) {
  return( std::numeric_limits< std::uint64_t >::max() );
  }

 bool operator==( const Xoshiro256 & ) const = default;

 private:
 std::uint64_t f_s[ 4 ];
 };  // end( class Xoshiro256 )

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
