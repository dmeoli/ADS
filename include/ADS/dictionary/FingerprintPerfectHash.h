/*--------------------------------------------------------------------------*/
/*---------------------- File FingerprintPerfectHash.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function by fingerprinting, beyond the book: the
 * method of Müller, Sanders, Schulze and Zhou, engineered as BBHash by
 * Limasset et al. and as FMPH by Beling, a function that maps the n
 * distinct keys of a static set S onto \f$ \{ 0 , \ldots , n - 1 \} \f$ in
 * about \f$ \gamma e^{1 / \gamma} \f$ bits per key, built in a few passes
 * over the keys [see MinimalPerfectHash.h for the function by pilots,
 * fewer bits and faster queries but a slower build, and RecSplit.h for the
 * fewest bits].
 *
 * The keys go through levels \f$ 0 , 1 , \ldots \f$: the level i is a
 * vector of \f$ m_i = \lceil \gamma n_i \rceil \f$ bits for the \f$ n_i \f$
 * keys that reach it (\f$ n_0 = n \f$, the sizes rounded up to blocks of
 * 512 bits), each key hashed to a place \f$ h_i( k ) \f$ of it; the bit of
 * a place is 1 if exactly one key falls there, and the keys that share
 * their place with others go to the next level. A key k is then mapped to
 * the number of 1s before its bit in the concatenation of the levels, the
 * rank of the first level i whose bit \f$ h_i( k ) \f$ is 1, which is a
 * bijection of S onto \f$ \{ 0 , \ldots , n - 1 \} \f$ since every key has
 * one bit of its own and every 1 belongs to one key. A key is alone in its
 * place with probability \f$ ( 1 - 1 / m_i )^{n_i - 1} \approx e^{-1 /
 * \gamma} \f$, so that
 * \f[
 *   E[ n_{i+1} ] \approx n_i ( 1 - e^{-1/\gamma} ) , \qquad
 *   \sum_i m_i \approx \gamma n \sum_i ( 1 - e^{-1/\gamma} )^i =
 *   \gamma e^{1 / \gamma} n ,
 * \f]
 * 3.30 bits per key for \f$ \gamma = 2 \f$, 2.72 (e) for \f$ \gamma = 1
 * \f$, the least, and the expected number of levels a key of S goes
 * through is \f$ e^{1 / \gamma} \f$, 1.65 for \f$ \gamma = 2 \f$: a
 * smaller \f$ \gamma \f$ saves bits and costs levels, in the build and in
 * the queries. The number of levels is \f$ O( \log n ) \f$ with high
 * probability.
 *
 * The build hashes each key once, to a word x of 64 bits, and each level
 * hashes x again by a folded product with a constant of its own,
 * \f$ h_i( k ) = \lfloor m_i \, g( x \oplus s_i , c_i ) / 2^{64} \rfloor
 * \f$; a level is two passes over its keys: the first sets in a vector A
 * the bit of each place and in a vector C the bits of the places met
 * twice, \f$ A \leftarrow A \wedge \neg C \f$ a word at a time, and the
 * second keeps the keys whose place is in C. The vectors of a level take
 * \f$ 2 \gamma n_i \f$ bits, about 0.5 MB for \f$ 10^6 \f$ keys, and stay
 * in the cache of level 2, so that a key costs a few nanoseconds per
 * level. A query computes \f$ h_0( k ) \f$, tests its bit and, if it is 1,
 * gives its rank: the number of 1s before each block of 512 bits is kept
 * in 32 bits (1 / 16 of the bits more), so that a rank costs at most 8
 * counts of the bits of a word. For a key out of S the result is some
 * value below n.
 *
 * The keys are hashed by h( k ), taken as it is if H declares
 * is_avalanching [see FastHash.h] and mixed by hashWord() otherwise (an
 * integer key under FastHash is taken as it is, the levels mixing it).
 * Two keys with the same hash of 64 bits meet at every level and cannot
 * be separated: the build throws std::runtime_error.
 *
 * The contracts [see Contract.h]: the keys distinct (an audit), fewer than
 * \f$ 2^{32} \f$, \f$ \gamma \ge 1 \f$; after the build the keys are mapped
 * onto { 0 , ... , n - 1 }, an audit. The operations are defined in
 * FingerprintPerfectHash.tpp.
 *
 * \see I. Müller, P. Sanders, R. Schulze, W. Zhou, Retrieval and perfect
 *      hashing using fingerprinting, Proceedings of SEA 2014, Lecture
 *      Notes in Computer Science 8504, 138-149, 2014,
 *      doi:10.1007/978-3-319-07959-2_12
 * \see A. Limasset, G. Rizk, R. Chikhi, P. Peterlongo, Fast and scalable
 *      minimal perfect hashing for massive key sets, Proceedings of SEA
 *      2017, LIPIcs 75, 25:1-25:16, 2017,
 *      doi:10.4230/LIPIcs.SEA.2017.25
 * \see P. Beling, Fingerprinting-based minimal perfect hashing revisited,
 *      ACM Journal of Experimental Algorithmics 28, 1-16, 2023,
 *      doi:10.1145/3596453
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

#ifndef __ADS_FingerprintPerfectHash
 #define __ADS_FingerprintPerfectHash
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS FingerprintPerfectHash ----------------------*/
/*--------------------------------------------------------------------------*/

/// a minimal perfect hash function of a static set of keys by levels of
/// fingerprints
template< class T , class H = DefaultHash< T > >
class FingerprintPerfectHash {

/*--------------------------------------------------------------------------*/
 public:

 using Key = T;   ///< the type of the keys

 /// the function of the distinct keys, with gamma bits per key in each
 /// level
 /// \pre the keys are distinct (an audit) and fewer than 2^32, gamma >= 1
 /// \post the keys are mapped onto { 0 , ... , n - 1 } (an audit)
 explicit FingerprintPerfectHash( const std::vector< T > & keys ,
                                  double gamma = 2 , std::uint64_t seed = 1 ,
                                  H h = H() );

 /// the value of k, in [ 0 , n ); some value in it for a key out of S
 /// \pre size() >= 1
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the number n of keys
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bits of the function: the levels, their ranks and their sizes
 [[nodiscard]] std::size_t bits( void ) const;

 /// the number of levels
 [[nodiscard]] std::size_t levels( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxLevels = 64;
 static constexpr std::size_t kBlock = 512;      ///< the bits of a block

 /// the hash of k under the seed, mixed unless H avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the place of the word x in the level i of m bits
 [[nodiscard]] static std::size_t place( std::uint64_t x , std::size_t i ,
                                         std::size_t m );

 /// the number of 1s before the bit g of the levels
 [[nodiscard]] std::size_t rank( std::size_t g ) const;

 std::uint64_t f_seed;
 std::size_t f_n = 0;
 std::vector< std::uint64_t > f_bits;   ///< the levels, one after another
 std::vector< std::uint32_t > f_rank;   ///< the 1s before each block
 std::vector< std::size_t > f_at;       ///< the first bit of each level
 std::vector< std::size_t > f_m;        ///< the bits of each level
 H f_h;

 };  // end( class FingerprintPerfectHash )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/FingerprintPerfectHash.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FingerprintPerfectHash.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File FingerprintPerfectHash.h ------------------*/
/*--------------------------------------------------------------------------*/
