/*--------------------------------------------------------------------------*/
/*------------------------ File ConsensusRecSplit.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function of ADS by recursive splitting with the
 * combined search and encoding of the seeds, the Consensus-RecSplit of
 * Lehmann, Sanders, Walzer and Ziegler, beyond the book [see `RecSplit.h`
 * for the seeds of RecSplit, each the first one that works and written by
 * a code of Golomb and Rice]: a function that maps the \f$ n \f$ distinct
 * keys of a static set \f$ S \f$ onto \f$ \{ 0 , \ldots , n - 1 \} \f$ with
 * no collision in a number of bits per key that approaches the lower bound
 * \f$ \log_2 e \approx 1.4427 \f$ as the overhead goes to 0, at a build
 * cost that grows as its inverse rather than exponentially.
 *
 * A key with the hash \f$ h \f$ of 64 bits first goes to one of
 * \f$ \lfloor n / K \rfloor \f$ buckets of exactly \f$ K \f$ keys, or to
 * the last \f$ n \bmod K \f$ values, by a minimal \f$ K \f$-perfect hash
 * function by thresholds: in the layer 0 the keys go to the bucket
 * \f$ \lfloor ( h \bmod 2^{32} ) L_0 / 2^{32} \rfloor \f$ of
 * \f$ L_0 = \lceil \omega \lfloor n / K \rfloor \rceil \f$ buckets
 * (\f$ \omega \f$ 0.9, 0.95 or 0.97 as \f$ K \le 256 \f$,
 * \f$ \le 16384 \f$ or larger, so that the buckets are overloaded), and a
 * bucket keeps the \f$ K \f$ keys of the least thresholds
 * \f$ \lfloor h / 2^{32} \rfloor \f$, by storing its largest threshold in
 * \f$ \log_2 K - 1 \f$ bits, on a scale that spends them around the
 * expected one; the keys bumped go to the layer 1, rehashed, on the other
 * \f$ \lfloor n / K \rfloor - L_0 \f$ buckets, and those bumped again to a
 * `FingerprintPerfectHash` and to the free places of the buckets, by an
 * Elias-Fano sequence of the bucket of each free place.
 *
 * Each bucket is then split by a balanced binary tree of
 * \f$ d = \log_2 K \f$ levels, whose node of \f$ s \f$ keys needs a seed
 * \f$ \sigma \f$ that sends exactly \f$ s / 2 \f$ of them to the left,
 * those with the top bit of `splitmix64()` of \f$ h + \sigma \f$ set,
 * which happens with probability
 * \f$ p( s ) = \binom{ s }{ s / 2 } 2^{-s} = \Theta( 1 / \sqrt{ s } ) \f$.
 * The nodes of a level of all the buckets, in their order, are the tasks
 * of one instance of Consensus: the task \f$ i \f$ owns the fragment of
 * \f$ \ell_i \f$ bits at the place \f$ 64 + E_{i-1} \f$ of a string of bits
 * \f$ M \f$, where
 * \f[
 *   E_i = \Bigl\lfloor ( i + 1 ) \Bigl( \log_2 \frac{ 1 }{ p( s ) } +
 *   \frac{ \varepsilon }{ 3.4 } s^{3/4} \Bigr) \Bigr\rfloor ,
 * \f]
 * the bits rounded up to multiples of \f$ 2^{-20} \f$, so that
 * \f$ \ell_i = E_i - E_{i-1} \ge 1 \f$, and its seed is the word
 * \f$ \sigma_i \f$ of the 64 bits of \f$ M \f$ that end with its fragment,
 * i.e., the fragment after the bits of the tasks before it and of the
 * 64 bits of the root. The fragments are searched by a depth-first search:
 * a task that finds no successful fragment goes back to the task before
 * it, which tries its next fragment, and the first task, when all of its
 * fragments fail, increments the root; a fragment of
 * \f$ \ell_i \approx \log_2 ( 1 / p ) + \varepsilon_i \f$ bits has about
 * \f$ 2^{\varepsilon_i} \f$ successful values in expectation, and the
 * backtracking keeps the sequence going when it has none. The seeds are
 * thus stored in \f$ \sum_i \ell_i + 64 \f$ bits a level, close to the
 * \f$ \sum_i \log_2 ( 1 / p_i ) \f$ of the entropy, against the
 * \f$ \log_2 ( 1 / p ) + 1.563 \f$ of a code of Golomb and Rice [see
 * `RecSplit.h`]; the overhead \f$ \varepsilon \f$ of the formula, larger
 * for the larger nodes (as in the code of the authors), spends the slack
 * where a trial costs most. The tasks of a level are cut into chunks of
 * \f$ 2^{22} / K \f$ buckets (\f$ 2^{22} \f$ keys), each one an instance
 * of its own, with its root and, if there are two chunks or more, its bits
 * on whole words: the chunks are then searched apart, in parallel by the
 * constructor that takes an executor, at the price of fewer than 128 bits
 * per chunk and level, and the seeds do not depend on the threads.
 *
 * An evaluation computes the hash, the \f$ K \f$-perfect function (one or
 * two thresholds, and for the few keys bumped twice the fallback and a
 * select), and descends the \f$ d \f$ levels, each a read of 64 bits at a
 * place computed from the index of the node, with no select: the node
 * \f$ t \f$ of a level reads the word that ends at \f$ E_t \f$ and goes to
 * the node \f$ 2 t \f$ or \f$ 2 t + 1 \f$, and the last level gives the
 * value \f$ K b + \text{the leaf} \f$ of the bucket \f$ b \f$. For a key
 * out of \f$ S \f$ the result is some value below \f$ n \f$.
 *
 * The keys are hashed by `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise;
 * the hash of the build is `splitmix64()` of that hash XOR the seed, a
 * bijection, so that the build never draws another seed: it throws
 * `std::runtime_error` at once if two keys have the same hash, and
 * otherwise succeeds.
 *
 * The contracts [see `Contract.h`]: the keys distinct (an audit), fewer
 * than \f$ 2^{32} \f$, \f$ 0 < \varepsilon \le 1 \f$; \f$ K \f$ a power of 2
 * with \f$ 4 \le K \le 2^{16} \f$; after the build the keys are mapped onto
 * \f$ \{ 0 , \ldots , n - 1 \} \f$, an audit. The operations are defined in
 * `ConsensusRecSplit.tpp`.
 *
 * \see H.-P. Lehmann, P. Sanders, S. Walzer, J. Ziegler, Combined search
 *      and encoding for seeds, with an application to minimal perfect
 *      hashing, Proceedings of the 33rd European Symposium on Algorithms
 *      (ESA 2025), LIPIcs 351, 109:1-109:18, 2025,
 *      doi:10.4230/LIPIcs.ESA.2025.109
 * \see E. Esposito, T. Mueller Graf, S. Vigna, RecSplit: minimal perfect
 *      hashing via recursive splitting, Proceedings of the 22nd Workshop on
 *      Algorithm Engineering and Experiments (ALENEX 2020), 175-185, 2020,
 *      doi:10.1137/1.9781611976007.14
 * \see S. Hermann, S. Kirmayer, H.-P. Lehmann, P. Sanders, S. Walzer,
 *      Engineering minimal k-perfect hash functions, Proceedings of the
 *      33rd European Symposium on Algorithms (ESA 2025), LIPIcs 351,
 *      99:1-99:18, 2025, doi:10.4230/LIPIcs.ESA.2025.99 (the
 *      \f$ K \f$-perfect function by thresholds)
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

#ifndef __ADS_ConsensusRecSplit
 #define __ADS_ConsensusRecSplit
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FingerprintPerfectHash.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/parallel/Executor.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ConsensusRecSplit -------------------------*/
/*--------------------------------------------------------------------------*/

/// a minimal perfect hash function of a static set of keys by the recursive
/// splitting of buckets of `K` keys, its seeds searched and encoded together
template< class T , std::size_t K = 4096 , class H = DefaultHash< T > >
class ConsensusRecSplit
{

 static_assert( std::has_single_bit( K ) && ( K >= 4 ) && ( K <= 65536 ) ,
		"ConsensusRecSplit: K a power of 2, 4 <= K <= 2^16" );

/*--------------------------------------------------------------------------*/
 public:
 /// the function of no key
 ConsensusRecSplit( void ) : ConsensusRecSplit( std::vector< T >() ) { }

 using Key = T;  ///< the type of the keys

 /// the function of the distinct keys, with the overhead \f$ \varepsilon \f$
 /// of the seeds
 ///
 /// In pseudocode (Consensus of Lehmann, Sanders, Walzer and Ziegler, beyond
 /// the book), the search of the seeds of the \f$ N \f$ tasks of a chunk
 /// of a level,
 /// \f$ \sigma_i \f$ the 64 bits of \f$ M \f$ that end at \f$ 64 + E_i \f$
 /// and \f$ f_i \f$ their last \f$ \ell_i \f$ bits, the fragment of the task
 /// \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{consensus}( N ) \cr
 /// 2 & \quad M \gets 0;\ i \gets 0 \cr
 /// 3 & \quad \textbf{while}\ i < N\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \sigma_i \text{ splits the keys of the task } i
 ///     \text{ in halves}\ \textbf{then}\ i \gets i + 1;\ f_i \gets 0 \cr
 /// 5 & \qquad \textbf{else} \cr
 /// 6 & \qquad\quad \textbf{while}\ f_i = 2^{\ell_i} - 1 \textbf{ and } i >
 ///     0\ \textbf{do}\ i \gets i - 1\quad \triangleright\ \text{backtrack}
 ///     \cr
 /// 7 & \qquad\quad \textbf{if}\ f_i < 2^{\ell_i} - 1\ \textbf{then}\ f_i
 ///     \gets f_i + 1 \cr
 /// 8 & \qquad\quad \textbf{else}\ f_0 \gets 0;\ \text{the root, the first }
 ///     \text{64 bits of } M \text{, grows by } 1
 /// \end{array}}
 /// \f]
 /// The levels are searched from the root down, the keys of each task
 /// grouped into its two halves before the next level.
 /// \pre the keys are distinct (an audit) and fewer than \f$ 2^{32} \f$,
 ///      \f$ 0 < \varepsilon \le 1 \f$
 /// \post the keys are mapped onto \f$ \{ 0 , \ldots , n - 1 \} \f$ (an
 ///       audit)
 explicit ConsensusRecSplit( const std::vector< T > & keys ,
			     double epsilon = 0.01 , std::uint64_t seed = 1 ,
			     H h = H() );

 /// the same function, the seeds of its chunks searched in parallel on the
 /// threads of `ex` [see `Executor.h`]: the seeds do not depend on the
 /// threads
 /// \pre as above, and `ex` may run the chunks on any thread
 template< Executor E >
 ConsensusRecSplit( E & ex , const std::vector< T > & keys ,
		    double epsilon = 0.01 , std::uint64_t seed = 1 ,
		    H h = H() );

 /// the value of \f$ k \f$, in \f$ [ 0 , n ) \f$; some value in it for a key
 /// out of \f$ S \f$
 ///
 /// In pseudocode (Consensus-RecSplit, beyond the book), \f$ E^l \f$ the
 /// ends of the fragments of the level \f$ l \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{value}( k ) \cr
 /// 2 & \quad h \gets \text{the hash of } k;\ t \gets \text{the bucket of }
 ///     h \text{ by thresholds} \cr
 /// 3 & \quad \textbf{if}\ t \text{ is one of the last } n \bmod K \text{
 ///     values}\ \textbf{then return}\ t \cr
 /// 4 & \quad \textbf{for}\ l \gets 0\ \textbf{to}\ \log_2 K - 1\ \textbf{do}
 ///     \cr
 /// 5 & \qquad \sigma \gets \text{the 64 bits of the level } l \text{ that }
 ///     \text{end at } 64 + E^l_t\quad \triangleright\ \text{no select} \cr
 /// 6 & \qquad t \gets 2 t + ( 0 \text{ if the key goes left by } \sigma
 ///     \text{, else } 1 ) \cr
 /// 7 & \quad \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// \pre `size()` \f$ \ge 1 \f$
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the bits of the function: the seeds, the thresholds, the fallback,
 /// the free places and the object
 [[nodiscard]] std::size_t bits( void ) const;

 /// the number \f$ \lfloor n / K \rfloor \f$ of buckets
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the number of keys bumped by both layers, those of the fallback
 [[nodiscard]] std::size_t bumped( void ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "ConsensusRecSplit";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix( { sizeof( T ) , K } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 /// the levels of the splitting trees
 static constexpr std::size_t kLevels = std::size_t( std::bit_width( K ) ) - 1;
 /// the bits of a threshold, and its largest value
 static constexpr unsigned kThresholdBits = unsigned( kLevels ) - 1;
 static constexpr std::uint64_t kRange = std::uint64_t( 1 ) << kThresholdBits;
 /// the load of the buckets of the layer 0
 static constexpr double kOverload =
   K <= 256 ? 0.9 : ( K <= 16384 ? 0.95 : 0.97 );
 /// the buckets of a chunk, \f$ 2^{22} \f$ keys, whose seeds are searched
 /// apart from those of the others, each after a root of its own
 static constexpr std::size_t kChunkLog =
   K >= ( std::size_t( 1 ) << 22 )
     ? 0
     : 22 - ( std::size_t( std::bit_width( K ) ) - 1 );
 /// the fraction of the expected threshold that the scale spans
 static constexpr std::uint64_t kTrim =
   K <= 8 ? 2 : ( K <= 256 ? 3 : ( K <= 512 ? 4 : 8 ) );

 /// the hash of \f$ k \f$ before the seed, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the hash \f$ h \f$ of \f$ k \f$, a bijection of `base( k )`
 [[nodiscard]] std::uint64_t hash( const T & k ) const;

 /// true if the key of hash \f$ h \f$ goes left by the seed \f$ \sigma \f$
 [[nodiscard]] static bool left( std::uint64_t h , std::uint64_t sigma );

 /// the end \f$ E_i \f$ of the fragment of the task \f$ i \f$ of the level
 /// \f$ l \f$, from the start of its bits
 [[nodiscard]] std::uint64_t end( std::size_t l , std::size_t i ) const;

 /// the threshold \f$ t \f$ of a key on the scale of the layer \f$ y \f$,
 /// in \f$ [ 1 , 2^{\log_2 K - 1} ) \f$
 [[nodiscard]] std::uint64_t scale( std::uint64_t t , std::size_t y ) const;

 /// the bucket of the hash \f$ h \f$, or \f$ \lfloor n / K \rfloor + j \f$
 /// for the \f$ j \f$-th of the last values
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the build of the constructors
 template< Executor E >
 void make( E & ex , const std::vector< T > & keys , double epsilon );

 /// the seeds of the level \f$ l \f$ of the keys \f$ X \f$, grouped by task,
 /// then each task split into its two halves, a chunk at a time
 template< std::size_t l , Executor E >
 void level( E & ex , std::vector< std::uint64_t > & X );

 /// the seeds of the level \f$ l \f$ of the \f$ N \f$ tasks of a chunk,
 /// whose keys \f$ X \f$ are grouped by task, from the bit \f$ o \f$
 template< std::size_t l >
 void search( std::uint64_t * X , std::size_t N , std::uint64_t o );

 std::uint64_t f_seed;      ///< the seed of the hashes
 std::size_t f_n = 0;       ///< the number of keys
 std::size_t f_nb = 0;      ///< the buckets of K keys
 std::size_t f_layers = 0;  ///< the layers of thresholds, at most 2
 /// the first bucket of each layer, and the end of the last one
 std::array< std::uint64_t , 3 > f_lbase{};
 std::array< std::uint64_t , 2 > f_expect{};  ///< the expected threshold
 BitVector f_thr;                            ///< the thresholds of the buckets
 /// the keys bumped by both layers, and the bucket of each free place
 FingerprintPerfectHash< std::uint64_t > f_fallback;
 EliasFano f_free;
 /// the bits of a split of each level, by \f$ 2^{20} \f$
 std::array< std::uint64_t , kLevels > f_micro{};
 /// the first bit of each level, and the end of the last one
 std::array< std::uint64_t , kLevels + 1 > f_off{};
 /// the bits of a chunk of each level, a multiple of 64 if there are two
 /// chunks or more
 std::array< std::uint64_t , kLevels > f_chunk{};
 BitVector f_seeds;  ///< the seeds of all the levels
 H f_h;              ///< the hash of the keys

 };  // end( class ConsensusRecSplit )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/ConsensusRecSplit.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ConsensusRecSplit.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ConsensusRecSplit.h ----------------------*/
/*--------------------------------------------------------------------------*/
