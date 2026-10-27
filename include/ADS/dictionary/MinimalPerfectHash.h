/*--------------------------------------------------------------------------*/
/*------------------------ File MinimalPerfectHash.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function of ADS by pilots, the method of PTHash
 * of Pibiri and Trani, beyond the book [see `PerfectHashing.h` for the
 * schemes of Fredman, Komlós and Szemerédi and of Czech, Havas and
 * Majewski]: a function that maps the \f$ n \f$ distinct keys of a static
 * set \f$ S \f$ onto \f$ \{ 0 , \ldots , n - 1 \} \f$ with no collision, in a
 * few bits per key (about 3 for the default parameters) and with two
 * accesses to memory per evaluation.
 *
 * A key with the hashes \f$ ( h , h' ) \f$ goes to the bucket
 * \f$ b( h ) \f$ of about \f$ n / \lambda \f$ buckets, 60 % of the keys into
 * 30 % of the buckets (the skewed buckets of PTHash, so that the large
 * buckets are placed while the table is still empty), and to the place
 * \f[
 *   p = \lfloor m \, g( h' \oplus \pi_{b( h )} ) / 2^{64} \rfloor
 * \f]
 * of a table of \f$ m = \lceil n / \alpha \rceil \f$ places, where \f$ g \f$
 * is a folded product and \f$ \pi_b \f$ is the pilot of the bucket
 * \f$ b \f$, a product of the integer \f$ k_b \f$ (the one stored) by an odd
 * constant. The build places the buckets from the largest to the smallest,
 * trying for each one the integers \f$ k_b = 0 , 1 , 2 , \ldots \f$ until
 * all its keys fall in free places, distinct among them; the expected
 * number of trials of a bucket grows as the table fills,
 * \f$ 1 / ( 1 - \text{load} )^s \f$ for a bucket of \f$ s \f$ keys, so that
 * the larger buckets go first. The integers \f$ k_b \f$ are stored in
 * \f$ \lceil \log_2 ( \max_b k_b + 1 ) \rceil \f$ bits each (the compact
 * encoding of PTHash), and a place \f$ p \ge n \f$ of a key is sent to one
 * of the places below \f$ n \f$ left free, by a table of the \f$ m - n \f$
 * places beyond \f$ n \f$ (\f$ \alpha < 1 \f$ makes the search of the last
 * buckets much faster, at the cost of this table).
 *
 * The search of the last buckets, placed when few places are left, is the
 * bottleneck of the build: a large \f$ \lambda \f$ leaves no bucket of one
 * key for the end, and with \f$ \alpha \f$ near 1 some bucket may find no
 * pilot within its limit, so that \f$ \lambda \f$ should stay below about 7
 * for \f$ \alpha = 0.99 \f$, as in the experiments of PTHash
 * (\f$ \lambda \f$ from 3 to 7).
 *
 * An evaluation computes the two hashes, the bucket, reads the \f$ k_b \f$
 * of the bucket (one access), computes the place and, for a place beyond
 * \f$ n \f$, reads the table of the free places (a second access, for about
 * \f$ 1 - \alpha \f$ of the keys). For a key out of \f$ S \f$ the result is
 * some place below \f$ n \f$.
 *
 * The keys are hashed by `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise;
 * the two hashes of the build are two folded products of that hash XOR the
 * seed. Two keys with the same hash of 64 bits and the same seed cannot be
 * separated, and a build that meets them, or a bucket that tries
 * \f$ 2^{24} \f$ pilots, draws a new seed.
 *
 * The contracts [see `Contract.h`]: the keys distinct (an audit), fewer
 * than \f$ 2^{32} \f$, \f$ \lambda \ge 1 \f$ and
 * \f$ 0.5 \le \alpha \le 1 \f$; after the build the keys are mapped onto
 * \f$ \{ 0 , \ldots , n - 1 \} \f$, an audit. A build that draws 100 seeds
 * with no success throws `std::runtime_error`. The operations are defined
 * in `MinimalPerfectHash.tpp`.
 *
 * \see G. E. Pibiri, R. Trani, PTHash: revisiting FCH minimal perfect
 *      hashing, Proceedings of SIGIR 2021, 1339-1348, 2021,
 *      doi:10.1145/3404835.3462849
 * \see G. E. Pibiri, R. Trani, Parallel and external-memory construction of
 *      minimal perfect hash functions with PTHash, IEEE Transactions on
 *      Knowledge and Data Engineering 36(3), 1249-1259, 2024,
 *      doi:10.1109/TKDE.2023.3303341
 * \see S. Hermann, H.-P. Lehmann, G. E. Pibiri, P. Sanders, S. Walzer,
 *      PHOBIC: perfect hashing with optimized bucket sizes and interleaved
 *      coding, Proceedings of ESA 2024, LIPIcs 308, 69:1-69:17, 2024,
 *      doi:10.4230/LIPIcs.ESA.2024.69
 * \see E. A. Fox, Q. F. Chen, L. S. Heath, A faster algorithm for
 *      constructing minimal perfect hash functions, Proceedings of SIGIR
 *      1992, 266-273, 1992, doi:10.1145/133160.133209 (the buckets and
 *      their displacements)
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

 /// the function of the distinct keys, with \f$ \lambda \f$ keys per bucket
 /// on average and the load \f$ \alpha \f$ of the table
 ///
 /// In pseudocode (PTHash of Pibiri and Trani, beyond the book), \f$ B_b \f$
 /// the second hashes of the keys of the bucket \f$ b \f$ and
 /// \f$ p( h' , k ) \f$ the place of \f$ h' \f$ with the pilot \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( S , \lambda , \alpha ) \cr
 /// 2 & \quad \text{group the keys by bucket, by a counting sort} \cr
 /// 3 & \quad \textbf{foreach}\ b \text{ by decreasing } |B_b|\ \textbf{do}
 ///     \cr
 /// 4 & \qquad k \gets 0 \cr
 /// 5 & \qquad \textbf{while}\ \text{the places } p( h' , k ) \text{ of }
 ///     B_b \text{ are not free and distinct}\ \textbf{do}\ k \gets k + 1 \cr
 /// 6 & \qquad k_b \gets k;\ \text{take the places } p( h' , k ) \cr
 /// 7 & \quad \text{map each place } p \ge n \text{ taken to a free place }
 ///     \text{below } n
 /// \end{array}}
 /// \f]
 /// A seed is drawn again if two keys of a bucket have the same hashes or a
 /// bucket tries \f$ 2^{24} \f$ pilots.
 /// \pre the keys are distinct (an audit) and fewer than \f$ 2^{32} \f$,
 ///      \f$ \lambda \ge 1 \f$ and \f$ 0.5 \le \alpha \le 1 \f$
 /// \post the keys are mapped onto \f$ \{ 0 , \ldots , n - 1 \} \f$ (an
 ///       audit)
 explicit MinimalPerfectHash( const std::vector< T > & keys ,
                              double lambda = 5 , double alpha = 0.99 ,
                              std::uint64_t seed = 1 , H h = H() );

 /// the value of \f$ k \f$, in \f$ [ 0 , n ) \f$; some value in it for a key
 /// out of \f$ S \f$
 ///
 /// In pseudocode (PTHash, beyond the book), \f$ F \f$ the table of the free
 /// places below \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{value}( k ) \cr
 /// 2 & \quad ( h , h' ) \gets \text{the two hashes of } k \cr
 /// 3 & \quad p \gets p( h' , k_{b( h )} )\quad \triangleright\ \text{one }
 ///     \text{access, the pilot} \cr
 /// 4 & \quad \textbf{if}\ p < n\ \textbf{then return}\ p\ \textbf{else
 ///     return}\ F[ p - n ]
 /// \end{array}}
 /// \f]
 /// \pre `size()` \f$ \ge 1 \f$
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
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

 /// the hash of \f$ k \f$ before the seed, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the bucket of the first hash \f$ h \f$
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the place of the second hash \f$ h' \f$ with the pilot \f$ k \f$
 [[nodiscard]] std::size_t place( std::uint64_t h2 , std::uint64_t k ) const;

 /// the pilot of the bucket \f$ b \f$
 [[nodiscard]] std::uint64_t pilot( std::size_t b ) const;

 /// the pilots and the free places of the base hashes \f$ B \f$; false if
 /// two hashes cannot be separated
 bool build( const std::vector< std::uint64_t > & B );

 std::uint64_t f_seed;
 std::size_t f_n = 0;
 std::size_t f_m = 0;              ///< the places, \f$ n / \alpha \f$
 std::size_t f_dense = 0;          ///< the buckets of 60 % of the keys
 std::size_t f_sparse = 0;         ///< the other buckets
 int f_w = 0;                      ///< the bits of a pilot
 std::vector< std::uint64_t > f_P; ///< the pilots, `f_w` bits each
 std::vector< std::uint32_t > f_free;   ///< the place of \f$ p \ge n \f$
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
