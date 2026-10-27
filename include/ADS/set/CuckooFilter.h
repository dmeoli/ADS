/*--------------------------------------------------------------------------*/
/*--------------------------- File CuckooFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cuckoo filter of Fan, Andersen, Kaminsky and Mitzenmacher, beyond
 * the book: a dynamic filter, which takes insertions and removals one at a
 * time, of a set \f$ S \f$ of keys, answering maybe for every key of
 * \f$ S \f$ and for a key out of \f$ S \f$ with a small probability. It is
 * a cuckoo table [see `CuckooHashing.h` and `BucketCuckooTable.h`] of
 * \f$ 2^\ell \f$ buckets of 4 places that keeps of each key only a
 * fingerprint of \f$ f \f$ bits (8 or 16, the type `F`), never 0, so that 0
 * marks a free place.
 *
 * A key with the hash \f$ h \f$ has the fingerprint \f$ t = t( h ) \f$, the
 * high \f$ f \f$ bits of \f$ h a \bmod 2^{64} \f$, \f$ a \f$ an odd
 * constant (1 if they are 0), which depend on every bit of \f$ h \f$, so
 * that \f$ t \f$ does not follow the bucket when \f$ h \f$ is a folded
 * product of structured keys, such as consecutive integers, at the cost of
 * one product of 64 bits, and the two buckets
 * \f[ b_1 = h_1( h ) \bmod 2^\ell \;, \qquad
 *     b_2 = b_1 \oplus g( t ) \bmod 2^\ell \;, \f]
 * \f$ h_1( h ) \f$ the high 32 bits of \f$ h \f$ and \f$ g( t ) \f$ the high
 * half of a product of \f$ t \f$ by an odd constant (partial-key cuckoo
 * hashing): the other bucket of a fingerprint is known from the fingerprint
 * and either of its buckets, \f$ b_1 = b_2 \oplus g( t ) \f$, so that a
 * fingerprint can be moved with no key at hand. An insertion puts \f$ t \f$
 * in a free place of \f$ b_1 \f$ or of \f$ b_2 \f$; if both are full it
 * evicts the fingerprint of a random place of one of them, moves it to its
 * other bucket, and so on, at most 500 times; if the last fingerprint
 * evicted finds no place it is kept aside, in a stash of one entry (the
 * victim) that the queries read too, and the next insertion fails: a filter
 * cannot be rehashed, since it keeps no key, so an insertion into a full
 * filter returns false and changes nothing. A removal clears one place
 * holding \f$ t \f$ in \f$ b_1 \f$ or \f$ b_2 \f$ and moves the victim back
 * into the table.
 *
 * The 4 fingerprints of a bucket are one word (32 bits for \f$ f = 8 \f$,
 * 64 for \f$ f = 16 \f$), and a query loads the two words before comparing:
 * the places equal to \f$ t \f$ are the lanes of zero bits of
 * \f$ u = w \oplus t \cdot ( 1 , \dots , 1 ) \f$, found for the 4 lanes at
 * once by \f$ \lnot( ( ( u \wedge m ) + m ) \vee u \vee m ) \f$, \f$ m \f$
 * having all the bits but the top one of every lane (exact, with no carry
 * from a lane to the next), which an insertion and a removal need to know
 * the place; a query only asks whether one lane of the two words is zero,
 * by \f$ ( ( u_1 - \mathbf{1} ) \wedge \lnot u_1 \vee ( u_2 - \mathbf{1} )
 * \wedge \lnot u_2 ) \wedge \lnot m \ne 0 \f$, \f$ \mathbf{1} \f$ having
 * the bit 0 of every lane (Knuth, Warren): a borrow may mark a lane above a
 * zero one, never a word with no zero lane, so that the test is exact. The
 * victim is read only when there is one, a branch that is taken only when
 * the filter is full.
 *
 * Analysis. A query of a key out of \f$ S \f$ compares \f$ t \f$ with the
 * at most \f$ 2 b = 8 \f$ fingerprints of its buckets, each equal to
 * \f$ t \f$ with probability \f$ 1 / ( 2^f - 1 ) \f$, so that at the load
 * \f$ \alpha \f$ (the fraction of places taken) the false positives are
 * \f[ \varepsilon = 1 - \Bigl( 1 - \frac{1}{2^f - 1} \Bigr)^{8 \alpha}
 *     \le \frac{8 \alpha}{2^f - 1} \;, \f]
 * at most \f$ 3.1 \% \f$ for \f$ f = 8 \f$ and \f$ 0.012 \% \f$ for
 * \f$ f = 16 \f$, in \f$ f / \alpha \f$ bits per key. With 2 choices and
 * buckets of \f$ d \f$ places every key finds a place with high
 * probability up to the load \f$ 1 / ( 1 + \varepsilon ) \f$ for any
 * \f$ \varepsilon > ( 2 / e )^{d-1} \f$ (Dietzfelbinger and Weidling),
 * 0.71 for \f$ d = 4 \f$, a bound that is far from tight: the insertions by
 * 500 evictions reach about 0.95 in practice (Fan et al.), from 0.97 at
 * \f$ 2^{10} \f$ buckets to 0.95 at \f$ 2^{22} \f$ in the tests of ADS. The
 * buckets of a fingerprint are not independent, \f$ b_2 \f$ taking only
 * \f$ 2^f - 1 \f$ values relative to \f$ b_1 \f$, which at \f$ f = 8 \f$
 * costs at most \f$ 1 \% \f$ of that load. The constructor for \f$ n \f$
 * keys takes the fewest buckets, a power of 2, that hold \f$ n \f$ keys at
 * the load 0.94. A key inserted \f$ j \f$ times takes \f$ j \f$ places, and
 * is removed once by each removal; past its 8 places, it fills the filter.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`], and mixed by `hashWord()` otherwise.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{32} \f$ keys; a key
 * inserted is maybe there, a postcondition. A key removed must have been
 * inserted, which no contract can check: the removal of another key may
 * clear the fingerprint of a key of \f$ S \f$, a false negative. The
 * operations are defined in `CuckooFilter.tpp`.
 *
 * \see B. Fan, D. G. Andersen, M. Kaminsky, M. D. Mitzenmacher, Cuckoo
 *      filter: practically better than Bloom, Proceedings of the 10th ACM
 *      International Conference on emerging Networking Experiments and
 *      Technologies (CoNEXT 2014), 75-88, 2014,
 *      doi:10.1145/2674005.2674994
 * \see R. Pagh, F. F. Rodler, Cuckoo hashing, Journal of Algorithms 51(2),
 *      122-144, 2004, doi:10.1016/j.jalgor.2003.12.002
 * \see M. Dietzfelbinger, C. Weidling, Balanced allocation and dictionaries
 *      with tightly packed constant size bins, Theoretical Computer Science
 *      380(1-2), 47-68, 2007, doi:10.1016/j.tcs.2007.02.054
 * \see A. Kirsch, M. Mitzenmacher, U. Wieder, More robust hashing: cuckoo
 *      hashing with a stash, SIAM Journal on Computing 39(4), 1543-1561,
 *      2009, doi:10.1137/080728743 (the stash)
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A,
 *      Combinatorial Algorithms, Part 1, Addison-Wesley, 2011, section
 *      7.1.3 (the broadword computing, the test of a zero byte)
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2013,
 *      section 6-1 (the first zero byte), ISBN 978-0-321-84268-8
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

#ifndef __ADS_CuckooFilter
 #define __ADS_CuckooFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

/// a function inlined even where the compiler would not
#ifndef ADS_FILTER_INLINE
 #if defined( __GNUC__ ) || defined( __clang__ )
  #define ADS_FILTER_INLINE inline __attribute__(( always_inline ))
 #elif defined( _MSC_VER )
  #define ADS_FILTER_INLINE __forceinline
 #else
  #define ADS_FILTER_INLINE inline
 #endif
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS CuckooFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a dynamic filter of the keys of type `T` with fingerprints of type `F`
/// (8 or 16 bits), 2 buckets of 4 places per key
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class CuckooFilter {

 static_assert( std::is_same_v< F , std::uint8_t > ||
                std::is_same_v< F , std::uint16_t > ,
                "the fingerprints have 8 or 16 bits" );

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter for \f$ n \f$ keys, at the load 0.94 when they are
 /// in, in at least one bucket: \f$ 2^\ell \f$ buckets, the least power of
 /// 2 with \f$ 4 \cdot 0.94 \cdot 2^\ell \ge n \f$
 /// \pre \f$ n < 2^{32} \f$
 explicit CuckooFilter( std::size_t n , H h = H() );

 /// \f$ x \f$ into the filter by `insertHash()` of its hash, false (and
 /// nothing changed) if the filter is full
 /// \post `maybe( x )` if true
 bool insert( const T & x );

 /// one insertion of \f$ x \f$ removed by `removeHash()` of its hash, false
 /// if \f$ x \f$ is not there
 /// \pre \f$ x \f$ was inserted (otherwise another key may be lost)
 bool remove( const T & x );

 /// whether \f$ x \f$ may be in the set, by `maybeHash()` of its hash:
 /// false only if \f$ x \f$ is not in the set
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the hash \f$ h \f$ of 64 bits, which must be random, into the filter,
 /// false (and nothing changed) if the filter is full
 ///
 /// In pseudocode (Fan, Andersen, Kaminsky and Mitzenmacher, with the stash
 /// of one entry of Kirsch, Mitzenmacher and Wieder), \f$ v \f$ the victim:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertHash}( h ) \cr
 /// 2 & \quad \textbf{if}\ v \ne \textbf{nil}\ \textbf{then return false} \cr
 /// 3 & \quad t \gets t( h );\ b \gets h_1( h ) \bmod 2^\ell \cr
 /// 4 & \quad \textbf{if}\ \text{a place of } b \text{ or of } b \oplus g( t )
 ///     \text{ is free }\ \textbf{then}\ \text{put } t \text{ there};
 ///     \ \textbf{return true} \cr
 /// 5 & \quad b \gets b \text{ or } b \oplus g( t ) \text{, at random} \cr
 /// 6 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ 500\ \textbf{do} \cr
 /// 7 & \qquad \text{swap } t \text{ with the fingerprint of a random }
 ///     \text{place of } b \cr
 /// 8 & \qquad b \gets b \oplus g( t ) \cr
 /// 9 & \qquad \textbf{if}\ \text{a place of } b \text{ is free }
 ///     \ \textbf{then}\ \text{put } t \text{ there};\ \textbf{return true}
 ///     \cr
 /// 10 & \quad v \gets ( b , t );\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool insertHash( std::uint64_t h );

 /// one insertion of the hash \f$ h \f$ removed, false if it is not there
 ///
 /// In pseudocode (Fan et al.):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{removeHash}( h ) \cr
 /// 2 & \quad t \gets t( h );\ b_1 \gets h_1( h ) \bmod 2^\ell;\ b_2
 ///     \gets b_1 \oplus g( t ) \cr
 /// 3 & \quad \textbf{foreach}\ b \in \{ b_1 , b_2 \}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \text{a place of } b \text{ holds } t
 ///     \ \textbf{then} \cr
 /// 5 & \qquad\quad \text{clear it} \cr
 /// 6 & \qquad\quad \textbf{if}\ v = ( b' , t' ) \ne \textbf{nil}
 ///     \ \textbf{then}\ v \gets \textbf{nil};\ \text{place } t' \text{ from }
 ///     b' \text{ as in insertHash} \cr
 /// 7 & \qquad\quad \textbf{return true} \cr
 /// 8 & \quad \textbf{if}\ v = ( b_1 , t ) \text{ or } ( b_2 , t )
 ///     \ \textbf{then}\ v \gets \textbf{nil};\ \textbf{return true} \cr
 /// 9 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 bool removeHash( std::uint64_t h );

 /// whether the hash \f$ h \f$ may be in the set: false only if it is not
 ///
 /// In pseudocode (Fan et al.), the 4 places of a bucket tested at once by
 /// the test of a zero lane:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybeHash}( h ) \cr
 /// 2 & \quad t \gets t( h );\ b_1 \gets h_1( h ) \bmod 2^\ell;\ b_2
 ///     \gets b_1 \oplus g( t ) \cr
 /// 3 & \quad \textbf{return}\ t \text{ is in a place of } b_1 \text{ or }
 ///     b_2 \text{, or } v \in \{ ( b_1 , t ) , ( b_2 , t ) \}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// removes all the keys
 void clear( void );

 /// the keys in, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 /// whether no key is in
 [[nodiscard]] bool empty( void ) const;

 /// the places, 4 per bucket
 [[nodiscard]] std::size_t capacity( void ) const;

 /// `size() / capacity()`, the load \f$ \alpha \f$
 [[nodiscard]] double load( void ) const;

 /// the bits of the buckets
 [[nodiscard]] std::size_t bits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a bucket: 4 fingerprints in one word
 using W = std::conditional_t< sizeof( F ) == 1 , std::uint32_t ,
                               std::uint64_t >;

 static constexpr int kLane = 8 * sizeof( F );        ///< bits of a place
 static constexpr W kOnes = W( ~W( 0 ) ) / W( F( ~F( 0 ) ) );   ///< 1 each
 static constexpr W kLow = kOnes * W( F( ~F( 0 ) ) >> 1 ); ///< 0x7F.. each
 static constexpr int kMaxKicks = 500;                ///< evictions at most

 /// the places of \f$ w \f$ equal to 0, the top bit of each lane
 [[nodiscard]] static constexpr W zeros( W w ) {
  return( W( ~( ( ( w & kLow ) + kLow ) | w | kLow ) ) );
  }

 /// the hash of \f$ x \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the fingerprint of the hash \f$ h \f$, never 0
 [[nodiscard]] static F tag( std::uint64_t h );

 /// the first bucket of the hash \f$ h \f$
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the other bucket of the fingerprint \f$ t \f$ in the bucket \f$ b \f$
 [[nodiscard]] std::size_t other( std::size_t b , F t ) const;

 /// \f$ t \f$ in a free place of \f$ b \f$, false if \f$ b \f$ is full
 bool put( std::size_t b , F t );

 /// \f$ t \f$ in \f$ b \f$ or in its other bucket, evicting if needed; a
 /// fingerprint with no place goes to the victim
 void place( std::size_t b , F t );

 /// a random word, for the evictions
 std::uint64_t random( void );

 std::vector< W > f_T;                ///< the buckets
 std::size_t f_mask = 0;              ///< buckets - 1
 std::size_t f_n = 0;                 ///< the keys in
 std::size_t f_vb = 0;                ///< the bucket of the victim
 F f_vt = 0;                          ///< the victim, 0 if none
 std::uint64_t f_rng = 0x9E3779B97F4A7C15ULL;   ///< the state of xorshift
 H f_h;                               ///< the hash of the keys

 };  // end( class CuckooFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/CuckooFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CuckooFilter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CuckooFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
