/*--------------------------------------------------------------------------*/
/*--------------------------- File CuckooFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cuckoo filter of Fan, Andersen, Kaminsky and Mitzenmacher, beyond
 * the book: a dynamic filter, which takes insertions and removals one at a
 * time, of a set S of keys, answering maybe for every key of S and for a
 * key out of S with a small probability. It is a cuckoo table [see
 * CuckooHashing.h and BucketCuckooTable.h] of \f$ 2^\ell \f$ buckets of 4
 * places that keeps of each key only a fingerprint of f bits (8 or 16, the
 * type F), never 0, so that 0 marks a free place.
 *
 * A key k with the hash h has the fingerprint \f$ t = t( h ) \f$, the low
 * f bits of a folded product of h by a constant (1 if they are 0), so that
 * t does not follow the bucket when h is a single product of structured
 * keys, such as consecutive integers, and the two buckets
 * \f[ b_1 = h_1( h ) \bmod 2^\ell \;, \qquad
 *     b_2 = b_1 \oplus g( t ) \bmod 2^\ell \;, \f]
 * \f$ h_1( h ) \f$ the high 32 bits of h and \f$ g( t ) \f$ the high half
 * of a product of t by an odd constant (partial-key cuckoo hashing): the
 * other bucket of a fingerprint is known from the fingerprint and either of
 * its buckets, \f$ b_1 = b_2 \oplus g( t ) \f$, so that a fingerprint can
 * be moved with no key at hand. An insertion puts t in a free place of
 * \f$ b_1 \f$ or of \f$ b_2 \f$; if both are full it evicts the fingerprint
 * of a random place of one of them, moves it to its other bucket, and so
 * on, at most 500 times; if the last fingerprint evicted finds no place it
 * is kept aside, in a stash of one entry (the victim) that the queries
 * read too, and the next insertion fails: a filter cannot be rehashed,
 * since it keeps no key, so an insertion into a full filter returns false
 * and changes nothing. A removal clears one place holding t in
 * \f$ b_1 \f$ or \f$ b_2 \f$ and moves the victim back into the table.
 *
 * The 4 fingerprints of a bucket are one word (32 bits for f = 8, 64 for
 * f = 16), and a query loads the two words before comparing: the places
 * equal to t are the lanes of zero bits of \f$ w \oplus t \cdot
 * ( 1 , \dots , 1 ) \f$, found for the 4 lanes at once by
 * \f$ \lnot( ( ( w \wedge m ) + m ) \vee w \vee m ) \f$, m having all the
 * bits but the top one of every lane (exact, with no carry from a lane to
 * the next); the free places are found the same way.
 *
 * Analysis. A query of a key out of S compares t with the at most
 * \f$ 2 b = 8 \f$ fingerprints of its buckets, each equal to t with
 * probability \f$ 1 / ( 2^f - 1 ) \f$, so that at the load
 * \f$ \alpha \f$ (the fraction of places taken) the false positives are
 * \f[ \varepsilon = 1 - \Bigl( 1 - \frac{1}{2^f - 1} \Bigr)^{8 \alpha}
 *     \le \frac{8 \alpha}{2^f - 1} \;, \f]
 * at most 3.1 % for f = 8 and 0.012 % for f = 16, in
 * \f$ f / \alpha \f$ bits per key. With 2 choices and buckets of d places
 * every key finds a place with high probability up to the load
 * \f$ 1 / ( 1 + \varepsilon ) \f$ for any
 * \f$ \varepsilon > ( 2 / e )^{d-1} \f$ (Dietzfelbinger and Weidling),
 * 0.71 for d = 4, a bound that is far from tight: the insertions by 500
 * evictions reach about 0.95 in practice (Fan et al.), from 0.97 at
 * \f$ 2^{10} \f$ buckets to 0.95 at \f$ 2^{22} \f$ in the tests of
 * ADS. The buckets of a fingerprint are not independent, \f$ b_2 \f$
 * taking only \f$ 2^f - 1 \f$ values relative to \f$ b_1 \f$, which at
 * f = 8 costs at most 1 % of that load. The constructor
 * for n keys takes the fewest buckets, a power of 2, that hold n keys at
 * the load 0.94. A key inserted j times takes j places, and is removed
 * once by each removal; past its 8 places, it fills the filter.
 *
 * The hash of a key is h( k ), taken as it is if H declares
 * is_avalanching [see FastHash.h], and mixed by hashWord() otherwise.
 *
 * The contracts [see Contract.h]: fewer than \f$ 2^{32} \f$ keys; a key
 * inserted is maybe there, a postcondition. A key removed must have been
 * inserted, which no contract can check: the removal of another key may
 * clear the fingerprint of a key of S, a false negative. The operations
 * are defined in CuckooFilter.tpp.
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

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS CuckooFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a dynamic filter of the keys of type T with fingerprints of type F (8
/// or 16 bits), 2 buckets of 4 places per key
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class CuckooFilter {

 static_assert( std::is_same_v< F , std::uint8_t > ||
                std::is_same_v< F , std::uint16_t > ,
                "the fingerprints have 8 or 16 bits" );

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter for n keys, at the load 0.94 when they are in, in at
 /// least one bucket
 /// \pre n < 2^32
 explicit CuckooFilter( std::size_t n , H h = H() );

 /// inserts x, false (and nothing changed) if the filter is full
 /// \post maybe( x ) if true
 bool insert( const T & x );

 /// removes one insertion of x, false if x is not there
 /// \pre x was inserted (otherwise another key may be lost)
 bool remove( const T & x );

 /// false only if x is not in the set
 [[nodiscard]] bool maybe( const T & x ) const;

 /// inserts the hash h of 64 bits, which must be random
 bool insertHash( std::uint64_t h );

 /// removes one insertion of the hash h
 bool removeHash( std::uint64_t h );

 /// false only if the hash h is not in the set
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// removes all the keys
 void clear( void );

 /// the keys in, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the places, 4 per bucket
 [[nodiscard]] std::size_t capacity( void ) const;

 /// size() / capacity()
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
 static constexpr int kMaxKicks = 500;

 /// the places of w equal to 0, the top bit of each lane
 [[nodiscard]] static constexpr W zeros( W w ) {
  return( W( ~( ( ( w & kLow ) + kLow ) | w | kLow ) ) );
  }

 /// the hash of x, mixed unless H avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the fingerprint of the hash h, never 0
 [[nodiscard]] static F tag( std::uint64_t h );

 /// the first bucket of the hash h
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the other bucket of the fingerprint t in the bucket b
 [[nodiscard]] std::size_t other( std::size_t b , F t ) const;

 /// puts t in a free place of b, false if b is full
 bool put( std::size_t b , F t );

 /// places t in b or in its other bucket, evicting if needed; a
 /// fingerprint with no place goes to the victim
 void place( std::size_t b , F t );

 /// a random word, for the evictions
 std::uint64_t random( void );

 std::vector< W > f_T;                ///< the buckets
 std::size_t f_mask = 0;              ///< buckets - 1
 std::size_t f_n = 0;                 ///< the keys in
 std::size_t f_vb = 0;                ///< the bucket of the victim
 F f_vt = 0;                          ///< the victim, 0 if none
 std::uint64_t f_rng = 0x9E3779B97F4A7C15ULL;
 H f_h;

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
