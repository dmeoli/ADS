/*--------------------------------------------------------------------------*/
/*------------------------ File MinimalPerfectHash.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function of ADS by pilots, the method of PTHash
 * of Pibiri and Trani, beyond the book [see PerfectHashing.h for the
 * schemes of Fredman, Komlós and Szemerédi and of Czech, Havas and
 * Majewski]: a function that maps the n distinct keys of a static set S
 * onto \f$ \{ 0 , \ldots , n - 1 \} \f$ with no collision, in a few bits
 * per key (about 3 for the default parameters) and with two accesses to
 * memory per evaluation.
 *
 * A key with the hashes \f$ ( h , h' ) \f$ goes to the bucket b( h ) of
 * about \f$ n / \lambda \f$ buckets, 60 % of the keys into 30 % of the
 * buckets (the skewed buckets of PTHash, so that the large buckets are
 * placed while the table is still empty), and to the place
 * \f[
 *   p = \lfloor m \, g( h' \oplus \pi_{b( h )} ) / 2^{64} \rfloor
 * \f]
 * of a table of \f$ m = \lceil n / \alpha \rceil \f$ places, where g is a
 * folded product and \f$ \pi_b \f$ is the pilot of the bucket b, a
 * product of the integer \f$ k_b \f$ (the one stored) by an odd constant.
 * The build places the buckets from the largest to the smallest, trying for
 * each one the integers \f$ k_b = 0 , 1 , 2 , \ldots \f$ until all its keys
 * fall in free places, distinct among them; the expected number of trials
 * of a bucket grows as the table fills, \f$ 1 / ( 1 - \text{load} )^s \f$
 * for a bucket of s keys, so that the larger buckets go first. The
 * integers \f$ k_b \f$ are stored in \f$ \lceil \log_2 ( \max_b k_b + 1 )
 * \rceil \f$ bits each (the compact encoding of PTHash), and a place p >= n
 * of a key is sent to one of the places below n left free, by a table of
 * the m - n places beyond n (\f$ \alpha < 1 \f$ makes the search of the
 * last buckets much faster, at the cost of this table).
 *
 * The search of the last buckets, placed when few places are left, is the
 * bottleneck of the build: a large lambda leaves no bucket of one key for
 * the end, and with alpha near 1 some bucket may find no pilot within its
 * limit, so that lambda should stay below about 7 for alpha = 0.99, as in
 * the experiments of PTHash (lambda from 3 to 7).
 *
 * An evaluation computes the two hashes, the bucket, reads the k of the
 * bucket (one access), computes the place and, for a place beyond n, reads
 * the table of the free places (a second access, for about 1 - alpha of
 * the keys). For a key out of S the result is some place below n.
 *
 * The keys are hashed by h( k ), taken as it is if H declares
 * is_avalanching [see FastHash.h] and mixed by hashWord() otherwise; the
 * two hashes of the build are two folded products of that hash XOR the
 * seed. Two keys with the same hash of 64 bits and the same seed cannot be
 * separated, and a build that meets them, or a bucket that tries \f$ 2^{24}
 * \f$ pilots, draws a new seed.
 *
 * The contracts [see Contract.h]: the keys distinct (an audit), fewer than
 * \f$ 2^{32} \f$, \f$ \lambda \ge 1 \f$ and \f$ 0.5 \le \alpha \le 1 \f$;
 * after the build the keys are mapped onto { 0 , ... , n - 1 }, an audit.
 * A build that draws 100 seeds with no success throws std::runtime_error.
 * The operations are defined in MinimalPerfectHash.tpp.
 *
 * \see G. E. Pibiri, R. Trani, PTHash: revisiting FCH minimal perfect
 *      hashing, Proceedings of SIGIR 2021, 1339-1348, 2021,
 *      doi:10.1145/3404835.3462849
 * \see G. E. Pibiri, R. Trani, Parallel and external-memory construction of
 *      minimal perfect hash functions with PTHash, IEEE Transactions on
 *      Knowledge and Data Engineering 36(3), 1249-1259, 2024
 * \see S. Hermann, H.-P. Lehmann, G. E. Pibiri, P. Sanders, S. Walzer,
 *      PHOBIC: perfect hashing with optimized bucket sizes and interleaved
 *      coding, Proceedings of ESA 2024, LIPIcs 308, 69, 2024
 * \see E. A. Fox, Q. F. Chen, L. S. Heath, A faster algorithm for
 *      constructing minimal perfect hash functions, Proceedings of SIGIR
 *      1992, 266-273, 1992 (the buckets and their displacements)
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

#ifndef __ADS_MinimalPerfectHash
 #define __ADS_MinimalPerfectHash
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MinimalPerfectHash ------------------------*/
/*--------------------------------------------------------------------------*/

/// a minimal perfect hash function of a static set of keys by pilots
template< class T , class H = DefaultHash< T > >
class MinimalPerfectHash {

/*--------------------------------------------------------------------------*/
 public:

 using Key = T;   ///< the type of the keys

 /// the function of the distinct keys, with lambda keys per bucket on
 /// average and the load alpha of the table
 /// \pre the keys are distinct (an audit) and fewer than 2^32, lambda >= 1
 ///      and 0.5 <= alpha <= 1
 /// \post the keys are mapped onto { 0 , ... , n - 1 } (an audit)
 explicit MinimalPerfectHash( const std::vector< T > & keys ,
                              double lambda = 5 , double alpha = 0.99 ,
                              std::uint64_t seed = 1 , H h = H() );

 /// the value of k, in [ 0 , n ); some value in it for a key out of S
 /// \pre size() >= 1
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the number n of keys
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bits of the function: the pilots, the free places and the sizes
 [[nodiscard]] std::size_t bits( void ) const;

 /// the number of buckets
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the seeds drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxTrials = 100;
 static constexpr std::uint64_t kMaxPilot = std::uint64_t( 1 ) << 24;

 /// the hash of k before the seed, mixed unless H avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the bucket of the first hash h
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the place of the second hash h2 with the pilot k
 [[nodiscard]] std::size_t place( std::uint64_t h2 , std::uint64_t k ) const;

 /// the pilot of the bucket b
 [[nodiscard]] std::uint64_t pilot( std::size_t b ) const;

 /// the pilots and the free places of the base hashes B; false if two
 /// hashes cannot be separated
 bool build( const std::vector< std::uint64_t > & B );

 std::uint64_t f_seed;
 std::size_t f_n = 0;
 std::size_t f_m = 0;              ///< the places, n / alpha
 std::size_t f_dense = 0;          ///< the buckets of 60 % of the keys
 std::size_t f_sparse = 0;         ///< the other buckets
 int f_w = 0;                      ///< the bits of a pilot
 std::vector< std::uint64_t > f_P; ///< the pilots, f_w bits each
 std::vector< std::uint32_t > f_free;   ///< the place of p >= n
 std::size_t f_trials = 0;
 H f_h;

 };  // end( class MinimalPerfectHash )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/MinimalPerfectHash.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MinimalPerfectHash.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File MinimalPerfectHash.h ---------------------*/
/*--------------------------------------------------------------------------*/
