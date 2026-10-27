/*--------------------------------------------------------------------------*/
/*---------------------- File FingerprintPerfectHash.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function by fingerprinting, beyond the book: the
 * method of Müller, Sanders, Schulze and Zhou, engineered as BBHash by
 * Limasset et al. and as FMPH by Beling, a function that maps the
 * \f$ n \f$ distinct keys of a static set \f$ S \f$ onto
 * \f$ \{ 0 , \ldots , n - 1 \} \f$ in about \f$ \gamma e^{1 / \gamma} \f$
 * bits per key, built in a few passes over the keys [see
 * `MinimalPerfectHash.h` for the function by pilots, fewer bits and faster
 * queries but a slower build, and `RecSplit.h` for the fewest bits].
 *
 * The keys go through levels \f$ 0 , 1 , \ldots \f$: the level \f$ i \f$ is
 * a vector of \f$ m_i = \lceil \gamma n_i \rceil \f$ bits for the
 * \f$ n_i \f$ keys that reach it (\f$ n_0 = n \f$, the sizes rounded up to
 * blocks of 512 bits), each key hashed to a place \f$ h_i( k ) \f$ of it;
 * the bit of a place is 1 if exactly one key falls there, and the keys that
 * share their place with others go to the next level. A key \f$ k \f$ is
 * then mapped to the number of 1s before its bit in the concatenation of
 * the levels, the rank of the first level \f$ i \f$ whose bit
 * \f$ h_i( k ) \f$ is 1, which is a bijection of \f$ S \f$ onto
 * \f$ \{ 0 , \ldots , n - 1 \} \f$ since every key has one bit of its own
 * and every 1 belongs to one key. A key is alone in its place with
 * probability
 * \f$ ( 1 - 1 / m_i )^{n_i - 1} \approx e^{-1 / \gamma} \f$, so that
 * \f[
 *   E[ n_{i+1} ] \approx n_i ( 1 - e^{-1/\gamma} ) , \qquad
 *   \sum_i m_i \approx \gamma n \sum_i ( 1 - e^{-1/\gamma} )^i =
 *   \gamma e^{1 / \gamma} n ,
 * \f]
 * 3.30 bits per key for \f$ \gamma = 2 \f$, 2.72 (\f$ e \f$) for
 * \f$ \gamma = 1 \f$, the least, and the expected number of levels a key of
 * \f$ S \f$ goes through is \f$ e^{1 / \gamma} \f$, 1.65 for
 * \f$ \gamma = 2 \f$: a smaller \f$ \gamma \f$ saves bits and costs levels,
 * in the build and in the queries. The number of levels is
 * \f$ O( \log n ) \f$ with high probability.
 *
 * The build hashes each key once, to a word \f$ x \f$ of 64 bits, and each
 * level hashes \f$ x \f$ again by a folded product with a constant of its
 * own, \f$ h_i( k ) = \lfloor m_i \, g( x \oplus s_i , c_i ) / 2^{64}
 * \rfloor \f$; a level is two passes over its keys: the first sets in a
 * vector \f$ A \f$ the bit of each place and in a vector \f$ C \f$ the bits
 * of the places met twice, \f$ A \leftarrow A \wedge \neg C \f$ a word at a
 * time, and the second keeps the keys whose place is in \f$ C \f$. The
 * vectors of a level take \f$ 2 \gamma n_i \f$ bits, about 0.5 MB for
 * \f$ 10^6 \f$ keys, and stay in the cache of level 2, so that a key costs a
 * few nanoseconds per level. A query computes \f$ h_0( k ) \f$, tests its
 * bit and, if it is 1, gives its rank: the number of 1s before each block of
 * 512 bits is kept in 32 bits (1 / 16 of the bits more), so that a rank
 * costs at most 8 counts of the bits of a word. For a key out of \f$ S \f$
 * the result is some value below \f$ n \f$.
 *
 * The keys are hashed by `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise
 * (an integer key under `FastHash` is taken as it is, the levels mixing
 * it). Two keys with the same hash of 64 bits meet at every level and
 * cannot be separated: the build throws `std::runtime_error`.
 *
 * The contracts [see `Contract.h`]: the keys distinct (an audit), fewer
 * than \f$ 2^{32} \f$, \f$ \gamma \ge 1 \f$; after the build the keys are
 * mapped onto \f$ \{ 0 , \ldots , n - 1 \} \f$, an audit. The operations are
 * defined in `FingerprintPerfectHash.tpp`.
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
 *         Università di Pisa \n
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

 /// the function of the distinct keys, with \f$ \gamma \f$ bits per key in
 /// each level
 ///
 /// In pseudocode (Müller, Sanders, Schulze and Zhou, beyond the book),
 /// \f$ X \f$ the hashes of the keys still without a bit, \f$ A_i \f$ and
 /// \f$ C_i \f$ the bits of the level \f$ i \f$ and of its places met twice:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( S , \gamma ) \cr
 /// 2 & \quad X \gets \{ \mathrm{base}( k ) : k \in S \};\ i \gets 0 \cr
 /// 3 & \quad \textbf{while}\ X \ne \emptyset\ \textbf{do} \cr
 /// 4 & \qquad m_i \gets \lceil \gamma |X| \rceil \text{, rounded up to }
 ///     \text{512};\ A_i , C_i \gets m_i \text{ bits } 0 \cr
 /// 5 & \qquad \textbf{foreach}\ x \in X\ \textbf{do} \cr
 /// 6 & \qquad\quad p \gets h_i( x );\ C_i[ p ] \gets C_i[ p ] \lor A_i[ p ];
 ///     \ A_i[ p ] \gets 1 \cr
 /// 7 & \qquad A_i \gets A_i \land \lnot C_i\quad \triangleright\ \text{a }
 ///     \text{word at a time} \cr
 /// 8 & \qquad X \gets \{ x \in X : C_i[ h_i( x ) ] = 1 \};\ i \gets i + 1 \cr
 /// 9 & \quad \text{the count of the 1s before each block of 512 bits}
 /// \end{array}}
 /// \f]
 /// \pre the keys are distinct (an audit) and fewer than \f$ 2^{32} \f$,
 ///      \f$ \gamma \ge 1 \f$
 /// \post the keys are mapped onto \f$ \{ 0 , \ldots , n - 1 \} \f$ (an
 ///       audit)
 explicit FingerprintPerfectHash( const std::vector< T > & keys ,
                                  double gamma = 2 , std::uint64_t seed = 1 ,
                                  H h = H() );

 /// the value of \f$ k \f$, in \f$ [ 0 , n ) \f$; some value in it for a key
 /// out of \f$ S \f$
 ///
 /// In pseudocode (beyond the book), \f$ L \f$ the levels one after another,
 /// \f$ a_i \f$ the first bit of the level \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{value}( k ) \cr
 /// 2 & \quad x \gets \mathrm{base}( k ) \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ \text{the last level}
 ///     \ \textbf{do} \cr
 /// 4 & \qquad g \gets a_i + h_i( x ) \cr
 /// 5 & \qquad \textbf{if}\ L[ g ] = 1\ \textbf{then return}
 ///     \ \mathrm{rank}( g )\quad \triangleright\ \text{the 1s before } g \cr
 /// 6 & \quad \textbf{return}\ \text{a value in } [ 0 , n ) \text{ from }
 ///     x\quad \triangleright\ k \notin S
 /// \end{array}}
 /// \f]
 /// \pre `size()` \f$ \ge 1 \f$
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the bits of the function: the levels, their ranks and their sizes
 [[nodiscard]] std::size_t bits( void ) const;

 /// the number of levels
 [[nodiscard]] std::size_t levels( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxLevels = 64;
 static constexpr std::size_t kBlock = 512;      ///< the bits of a block

 /// the hash of \f$ k \f$ under the seed, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the place of the word \f$ x \f$ in the level \f$ i \f$ of \f$ m \f$ bits
 [[nodiscard]] static std::size_t place( std::uint64_t x , std::size_t i ,
                                         std::size_t m );

 /// the number of 1s before the bit \f$ g \f$ of the levels
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
