/*--------------------------------------------------------------------------*/
/*----------------------------- File PtrHash.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function of ADS by pilots of 8 bits, the
 * PtrHash of Groot Koerkamp, beyond the book [see `MinimalPerfectHash.h`
 * for PTHash, whose pilots are not bounded and are written in as many bits
 * as the largest one needs]: a function that maps the \f$ n \f$ distinct
 * keys of a static set \f$ S \f$ onto \f$ \{ 0 , \ldots , n - 1 \} \f$ with
 * no collision in about \f$ 8 / \lambda \f$ bits per key plus those of the
 * remap, whose query reads one byte of an array and, for about
 * \f$ 1 - \alpha \f$ of the keys, one item of the remap.
 *
 * A key with the hash \f$ h \f$ of 64 bits goes to the part
 * \f$ \pi( h ) = \lfloor P h / 2^{64} \rfloor \f$ of \f$ P \f$ parts, each of
 * the same \f$ S \f$ slots and \f$ B \f$ buckets, to the bucket
 * \f$ \beta( h ) = \pi( h ) B + \lfloor B \gamma( r ) / 2^{64} \rfloor \f$
 * of its part, where \f$ r = P h \bmod 2^{64} \f$ is what of \f$ h \f$ the
 * part leaves, and to the slot
 * \f[
 *   s( h ) = \pi( h ) S + \bigl( ( h \oplus c ( p_{\beta( h )} \oplus x ) )
 *   \bmod_{\!*} S \bigr) ,
 * \f]
 * where \f$ p_b \in [ 0 , 256 ) \f$ is the pilot of the bucket \f$ b \f$,
 * \f$ c \f$ an odd constant, \f$ x \f$ the seed and \f$ y \bmod_{\!*} S =
 * \lfloor S ( m y \bmod 2^{64} ) / 2^{64} \rfloor \f$ with
 * \f$ m = \lfloor ( 2^{64} - 1 ) / S \rfloor + 1 \f$ the fast modulo of
 * Lemire, Kaser and Kurz (exactly \f$ y \bmod S \f$ for \f$ y < 2^{32} \f$,
 * a mixing of all the bits of \f$ y \f$ otherwise). The bucket function
 * \f$ \gamma \f$ is the identity (`PtrHashBucket::Linear`) or the cubic
 * \f$ \gamma_3( r ) = \frac{255}{256} \frac{ r^2 ( 1 + r ) }{ 2 } +
 * \frac{ r }{ 256 } \f$ of a fraction \f$ r \f$ of \f$ 2^{64} \f$
 * (`PtrHashBucket::Cubic`), the Taylor approximation of the optimal
 * function \f$ r + ( 1 - r ) \ln( 1 - r ) \f$ of PHOBIC with a slope of at
 * least \f$ 1 / 256 \f$, so that the first buckets are large and the last
 * ones small. The parts are \f$ P = \max\{ 1 , \lfloor n / \kappa
 * \rfloor \} \f$ with \f$ \kappa = ( 4 / \epsilon^2 ) \max\{ 1 , \ln( n
 * \epsilon^2 / 2 ) \} \f$ and \f$ \epsilon = ( 1 - \alpha ) / 2 \f$, half
 * those of the bound of Vigna for the \f$ \epsilon \f$-cost sharding, so
 * that a part is rarely fuller than its slots (\f$ \kappa \ge 160000 \f$
 * keys for \f$ \alpha = 0.99 \f$, and a single part for
 * \f$ \alpha = 1 \f$); \f$ S \f$ is \f$ \lceil \lfloor n / P \rfloor /
 * \alpha \rceil \f$ rounded up to a number neither even nor a multiple of
 * 3, so that the modulo depends on all the bits of \f$ y \f$, and
 * \f$ B \f$ is \f$ \lceil \lfloor n / P \rfloor / \lambda \rceil + 8 \f$
 * rounded up to an odd number.
 *
 * The build takes the buckets of a part from the largest to the smallest,
 * and gives each one the first pilot of \f$ [ 0 , 256 ) \f$ that sends its
 * keys to free slots, distinct among them. When no pilot does, the bucket
 * takes the pilot whose slots meet the buckets of the least sum of the
 * squares of their sizes, skipping the pilots that meet one of the
 * \f$ \min\{ 16 , \lfloor B / 4 \rfloor \} \f$ buckets placed last (the
 * cycles of evictions) or send two of its keys to
 * the same slot, from a random pilot onwards; it evicts those buckets, as
 * cuckoo hashing evicts its keys, and they are placed again, the largest
 * first, before the next bucket of the order. A bucket of the order whose
 * chain of evictions exceeds \f$ 10 S \f$, or a part of more than \f$ S \f$
 * keys, makes the build draw a new seed. The
 * \f$ P S - n \f$ slots beyond \f$ n \f$ are then remapped: the
 * \f$ i \f$-th of them taken goes to the \f$ i \f$-th slot below \f$ n \f$
 * left free, and the remap, non-decreasing, is an array of 32 bits
 * (`PtrHashRemap::Array`) or an Elias-Fano sequence
 * (`PtrHashRemap::EliasFano`, about \f$ 2 + \log_2 ( n / ( P S - n ) )
 * \f$ bits per item and its directory, against the 32 of the array and the
 * 11.6 of the CacheLineEF of PtrHash).
 *
 * An evaluation computes the hash, the bucket and the slot, reads the
 * pilot (one access to memory, a byte of \f$ P B \approx n / \lambda \f$)
 * and, for a slot \f$ \ge n \f$, the remap. `stream()` evaluates a range of
 * keys with the pilot of the key 32 places ahead prefetched [see
 * `Prefetch.h`], so that up to 32 accesses are in flight, and `prefetch()`
 * prefetches the pilot of one key. For a key out of \f$ S \f$ the result is
 * some place below \f$ n \f$.
 *
 * The keys are hashed by `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise;
 * the hash of the build is `splitmix64()` of that hash XOR the seed, a
 * bijection, so that two keys have the same \f$ h \f$ for no seed if they
 * have the same hash and for every seed otherwise: the build throws
 * `std::runtime_error` at once if two keys have the same hash.
 *
 * The contracts [see `Contract.h`]: the keys distinct (an audit), fewer
 * than \f$ 2^{32} \f$, \f$ \lambda \ge 1 \f$ and
 * \f$ 0.5 \le \alpha \le 1 \f$; after the build the keys are mapped onto
 * \f$ \{ 0 , \ldots , n - 1 \} \f$, an audit. A build that draws 100 seeds
 * with no success throws `std::runtime_error`. The operations are defined
 * in `PtrHash.tpp`.
 *
 * \see R. Groot Koerkamp, PtrHash: minimal perfect hashing at RAM
 *      throughput, Proceedings of the 23rd International Symposium on
 *      Experimental Algorithms (SEA 2025), LIPIcs 338, 21:1-21:21, 2025,
 *      doi:10.4230/LIPIcs.SEA.2025.21
 * \see S. Hermann, H.-P. Lehmann, G. E. Pibiri, P. Sanders, S. Walzer,
 *      PHOBIC: perfect hashing with optimized bucket sizes and interleaved
 *      coding, Proceedings of ESA 2024, LIPIcs 308, 69:1-69:17, 2024,
 *      doi:10.4230/LIPIcs.ESA.2024.69 (the optimal bucket function)
 * \see S. Vigna, epsilon-cost sharding: scaling hypergraph-based data
 *      structures to trillions of keys, 2025,
 *      doi:10.48550/arXiv.2503.18397 (the size of the parts)
 * \see D. Lemire, O. Kaser, N. Kurz, Faster remainder by direct
 *      computation: applications to compilers and software libraries,
 *      Software: Practice and Experience 49(6), 953-970, 2019,
 *      doi:10.1002/spe.2689
 * \see R. Pagh, F. F. Rodler, Cuckoo hashing, Journal of Algorithms 51(2),
 *      122-144, 2004, doi:10.1016/j.jalgor.2003.12.002 (the evictions)
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

#ifndef __ADS_PtrHash
 #define __ADS_PtrHash
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/external/MappedFile.h"
#include "ADS/memory/Prefetch.h"
#include "ADS/parallel/Executor.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <span>
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

/// the bucket function of a PtrHash: the identity, or the cubic one that
/// makes the first buckets large and the last ones small
enum class PtrHashBucket { Linear , Cubic };

/// the remap of the slots beyond \f$ n \f$ of a PtrHash: an array of 32
/// bits, or an Elias-Fano sequence
enum class PtrHashRemap { Array , EliasFano };

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS PtrHash ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a minimal perfect hash function of a static set of keys by pilots of 8
/// bits, placed by evictions
template< class T , PtrHashBucket F = PtrHashBucket::Linear ,
	  PtrHashRemap R = PtrHashRemap::Array , class H = DefaultHash< T > >
class PtrHash
{

/*--------------------------------------------------------------------------*/
 public:
 /// the function of no key
 PtrHash( void ) : PtrHash( std::vector< T >() ) { }

 using Key = T;  ///< the type of the keys

 /// the function of the distinct keys, with \f$ \lambda \f$ keys per bucket
 /// on average and the load \f$ \alpha \f$ of the slots
 ///
 /// In pseudocode (PtrHash of Groot Koerkamp, beyond the book), for a part
 /// with the buckets \f$ B_b \f$, \f$ s( h , p ) \f$ the slot of \f$ h \f$
 /// with the pilot \f$ p \f$, \f$ o( t ) \f$ the bucket in the slot
 /// \f$ t \f$ (none if it is free), \f$ R \f$ the
 /// \f$ \min\{ 16 , \lfloor B / 4 \rfloor \} \f$ buckets placed last and
 /// \f$ e \f$ the evictions of the chain:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( \text{a part} ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{bucket } b \text{, by decreasing
 ///     size,}\ \textbf{do} \cr
 /// 3 & \qquad Q \gets \{ b \};\ R \gets \{ b \};\ e \gets 0 \cr
 /// 4 & \qquad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 5 & \qquad\quad c \gets \text{the largest bucket of } Q \text{, out of }
 ///     Q \cr
 /// 6 & \qquad\quad \textbf{if}\ \text{some } p < 256 \text{ sends } B_c
 ///     \text{ to free slots, distinct}\ \textbf{then}\ p_c \gets \text{the
 ///     first one} \cr
 /// 7 & \qquad\quad \textbf{else} \cr
 /// 8 & \qquad\qquad p_c \gets \text{the } p \text{ of least }
 ///     \sum_{h \in B_c ,\, o( s( h , p ) ) \ne \bot} | B_{o( s( h , p ) )}
 ///     |^2 \text{, its slots distinct and out of } R \cr
 /// 9 & \qquad\qquad \textbf{foreach}\ d = o( s( h , p_c ) ) \ne \bot
 ///     \text{, } h \in B_c\ \textbf{do}\ \text{free the slots of } d;\ Q
 ///     \gets Q \cup \{ d \};\ e \gets e + 1 \cr
 /// 10 & \qquad\qquad R \gets R \cup \{ c \}\quad \triangleright\ \text{the
 ///     oldest one out} \cr
 /// 11 & \qquad\qquad \textbf{if}\ e > 10 S\ \textbf{then}\ \text{draw a
 ///     new seed} \cr
 /// 12 & \qquad\quad o( s( h , p_c ) ) \gets c \text{ for each } h \in B_c
 /// \end{array}}
 /// \f]
 /// Then the \f$ i \f$-th slot \f$ \ge n \f$ taken is remapped to the
 /// \f$ i \f$-th slot \f$ < n \f$ left free.
 /// \pre the keys are distinct (an audit) and fewer than \f$ 2^{32} \f$,
 ///      \f$ \lambda \ge 1 \f$ and \f$ 0.5 \le \alpha \le 1 \f$
 /// \post the keys are mapped onto \f$ \{ 0 , \ldots , n - 1 \} \f$ (an
 ///       audit)
 explicit PtrHash( const std::vector< T > & keys , double lambda = 3 ,
		   double alpha = 0.99 , std::uint64_t seed = 1 , H h = H() );

 /// the same function, its parts built in parallel on the threads of
 /// `ex` [see `Executor.h`]: the pilots do not depend on the threads
 /// \pre as above, and `ex` may run the parts on any thread
 template< Executor E >
 PtrHash( E & ex , const std::vector< T > & keys , double lambda = 3 ,
	  double alpha = 0.99 , std::uint64_t seed = 1 , H h = H() );

 /// the value of \f$ k \f$, in \f$ [ 0 , n ) \f$; some value in it for a key
 /// out of \f$ S \f$
 ///
 /// In pseudocode (PtrHash, beyond the book), \f$ F \f$ the remap:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{value}( k ) \cr
 /// 2 & \quad h \gets \text{the hash of } k;\ b \gets \beta( h ) \cr
 /// 3 & \quad t \gets s( h , p_b )\quad \triangleright\ \text{one access,
 ///     the byte of the pilot} \cr
 /// 4 & \quad \textbf{if}\ t < n\ \textbf{then return}\ t\ \textbf{else
 ///     return}\ F[ t - n ]
 /// \end{array}}
 /// \f]
 /// \pre `size()` \f$ \ge 1 \f$
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the values of the keys \f$ K \f$ in \f$ V \f$, \f$ V[ i ] \f$ that of
 /// \f$ K[ i ] \f$, with the pilot of the key 32 places ahead prefetched
 ///
 /// In pseudocode (the streaming queries of PtrHash, beyond the book),
 /// \f$ a = 32 \f$ and \f$ ( h_j , b_j ) \f$ a ring of \f$ a \f$ hashes and
 /// buckets:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{stream}( K , V ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ \min\{ a , |K| \} - 1
 ///     \ \textbf{do}\ h_i , b_i \gets \text{those of } K[ i ];\ \text{
 ///     prefetch } p_{b_i} \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |K| - 1\ \textbf{do} \cr
 /// 4 & \qquad j \gets i \bmod a;\ V[ i ] \gets \text{line 3 and 4 of }
 ///     \mathrm{value} \text{ on } h_j , b_j \cr
 /// 5 & \qquad \textbf{if}\ i + a < |K|\ \textbf{then}\ h_j , b_j \gets
 ///     \text{those of } K[ i + a ];\ \text{prefetch } p_{b_j}
 /// \end{array}}
 /// \f]
 /// \pre \f$ |V| \ge |K| \f$, and `size()` \f$ \ge 1 \f$ if \f$ K \f$ is not
 ///      empty
 void stream( std::span< const T > K , std::span< std::size_t > V ) const;

 /// a hint that \f$ k \f$ is evaluated next: the line of its pilot is
 /// prefetched [see `Prefetch.h`]
 /// \pre `size()` \f$ \ge 1 \f$
 void prefetch( const T & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the bits of the function: the pilots, the remap and the object
 [[nodiscard]] std::size_t bits( void ) const;

 /// the number \f$ P \f$ of parts
 [[nodiscard]] std::size_t parts( void ) const;

 /// the number \f$ P B \f$ of buckets
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the number \f$ P S \f$ of slots
 [[nodiscard]] std::size_t slots( void ) const;

 /// the seeds drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

 /// the evictions of the last seed drawn by the build
 [[nodiscard]] std::size_t evictions( void ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "PtrHash";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix(
    { sizeof( T ) , std::uint64_t( F ) , std::uint64_t( R ) } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 static constexpr std::size_t kMaxTrials = 100;
 static constexpr std::size_t kPilots = 256;  ///< the pilots, 8 bits
 static constexpr std::size_t kRecent = 16;   ///< the buckets not evicted
 static constexpr std::size_t kAhead = 32;    ///< the prefetch of stream()
 /// the odd constant of the hash of the pilots (that of FxHash)
 static constexpr std::uint64_t kC = 0x517CC1B727220A95ULL;
 static constexpr std::uint32_t kNone = ~std::uint32_t( 0 );

 /// the type of the remap
 using Remap = std::conditional_t< R == PtrHashRemap::EliasFano , EliasFano ,
				   std::vector< std::uint32_t > >;

 /// the hash of \f$ k \f$ before the seed, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the hash \f$ h \f$ of \f$ k \f$, a bijection of `base( k )`
 [[nodiscard]] std::uint64_t hash( const T & k ) const;

 /// the bucket \f$ \beta( h ) \f$, among the \f$ P B \f$ of all the parts
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the slot of \f$ h \f$ in its part with the hash \f$ c ( p \oplus x ) \f$
 /// of its pilot
 [[nodiscard]] std::size_t inPart( std::uint64_t h , std::uint64_t hp ) const;

 /// the hash of the pilot \f$ p \f$
 [[nodiscard]] std::uint64_t pilotHash( std::uint64_t p ) const;

 /// the value of the hash \f$ h \f$ whose bucket is \f$ b \f$
 [[nodiscard]] std::size_t value( std::uint64_t h , std::size_t b ) const;

 /// the pilots of the part \f$ q \f$, whose sorted hashes are \f$ K \f$,
 /// with its taken slots set in the words \f$ W \f$; the evictions, or
 /// \f$ -1 \f$ if a chain evicts too many buckets
 [[nodiscard]] std::ptrdiff_t
 part( std::size_t q , std::span< const std::uint64_t > K ,
       std::uint64_t * W );

 /// the parts, the trials of the seeds and the audit of the constructors
 template< Executor E >
 void make( E & ex , const std::vector< T > & keys , double lambda ,
	    double alpha );

 /// the pilots and the remap of the keys; false if a part is too full or
 /// evicts too many
 template< Executor E >
 bool build( E & ex , const std::vector< T > & keys );

 std::uint64_t f_seed;    ///< the seed of the hashes
 std::size_t f_n = 0;     ///< the number of keys
 std::size_t f_P = 1;     ///< the parts
 std::size_t f_S = 0;     ///< the slots of a part
 std::size_t f_B = 0;     ///< the buckets of a part
 std::uint64_t f_mS = 0;  ///< the multiplier of the fast modulo by S
 /// the pilots, a byte each
 std::vector< std::uint8_t , MappableAllocator< std::uint8_t > > f_pilot;
 Remap f_remap;                ///< the slots \f$ \ge n \f$ remapped
 std::size_t f_trials = 0;     ///< the seeds drawn
 std::size_t f_evictions = 0;  ///< the evictions of the last seed
 H f_h;                        ///< the hash of the keys

 };  // end( class PtrHash )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PtrHash.tpp"

/*--------------------------------------------------------------------------*/

#endif /* PtrHash.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File PtrHash.h ---------------------------*/
/*--------------------------------------------------------------------------*/
