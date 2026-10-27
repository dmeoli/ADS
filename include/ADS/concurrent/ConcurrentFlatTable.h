/*--------------------------------------------------------------------------*/
/*----------------------- File ConcurrentFlatTable.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table that many threads read and grow at once, beyond the book:
 * open addressing with linear probing on a vector of atomic words of 64
 * bits, the <em>folklore</em> table that Maier, Sanders and Dementiev found
 * the fastest to read, made to grow while the threads work and to keep its
 * pairs where they are. It is meant as the CLOSED set of a parallel search:
 * `findOrInsert( k , h , v )` takes a key with its hash of 64 bits,
 * computed once by the caller, and either finds the key or inserts it with
 * the value \f$ v \f$, returning in both cases the pair that the table
 * holds, so that the threads that reach the same state at once agree on
 * one pair and only one of them is told that it inserted it.
 *
 * A word of the vector is 0 when its place is free, and is written once,
 * by a compare-and-swap from 0, never changed afterwards but by
 * `update()`: there are no removals. The pairs are kept in one of two
 * ways, chosen by their types.
 * - When the key and the value are trivially copyable and fit in 8 bytes
 *   together (e.g., a key of 32 bits with a value of 32 bits, or a key of
 *   64 bits with an empty value, a set), the word is the pair itself
 *   (`packed` is then true): a lookup reads one line of cache, compares
 *   the keys in the words and returns a copy of the value. The pairs whose
 *   key is made of zero bytes, whose word could be 0, are kept apart in
 *   one atomic word.
 * - Otherwise the word holds the index of a node, where the pair lies
 *   with its hash, and 24 bits of the hash, so that a lookup follows the
 *   index only when those bits match (once in \f$ 2^{24} \f$ for another
 *   key); the nodes never move, and the pair returned is a reference to
 *   the node, valid as long as the table. The nodes are cut from segments
 *   of \f$ 2^{12} , 2^{13} , \ldots \f$ nodes, of which each thread
 *   reserves 64 at a time, so that an insertion takes no lock and calls
 *   no allocator but once per 64 nodes; the segments of 2 MB or more are
 *   on huge pages [see `HugePageAllocator.h`], and those of the pairs for
 *   which the table is made, or reserved, are faulted in at once.
 *
 * The place of a key with the hash \f$ h \f$ in a vector of \f$ 2^l \f$
 * words is the top \f$ l \f$ bits of \f$ h \f$, and a lookup reads the
 * words from there on, cyclically, until it finds the key or a free place.
 * Its reads are atomic loads with acquire: no lock, no read-modify-write,
 * no store, and no check of versions, since a word, once written, stays.
 * A lookup that ends on a free place reads the vector in use again, and
 * starts over on the new one if the table grew meanwhile; otherwise the
 * key was absent at the moment it read that free place, since the places
 * before it held other keys and a key is inserted at the first free place
 * of its sequence. A lookup is thus wait-free when the table does not grow.
 *
 * The insertions (and `update()`) exclude a growth, and the growth
 * excludes them, by a protocol of Dekker over 64 counters on lines of
 * their own, as in the growth of Maier et al. that synchronizes the
 * threads: a thread adds 1 to the counter of its own (a sequentially
 * consistent read-modify-write) and then reads the flag of the growth,
 * while the thread that grows the table raises the flag and then reads
 * the counters; in the single order of the sequentially consistent
 * accesses one of the two comes first, and the other sees it, so that no
 * insertion runs on a vector that is being copied. The thread that grows
 * the table waits for the counters to drop to 0, allocates a vector twice
 * as large, and the threads that come to insert meanwhile help it copy
 * the old vector in blocks of 4096 words, claimed by a counter; the new
 * vector replaces the old one when all the blocks are copied. The lookups
 * never wait: they read the old vector, which no longer changes, until
 * the new one is in use. The vectors replaced are kept until the table is
 * destroyed, since a reader may still be on one; with sizes that double
 * they occupy less than the vector in use.
 *
 * The table grows when its pairs pass half its places, a load at which a
 * lookup of linear probing reads 1.5 words when it succeeds and 2.5 when
 * it fails, on average, most of them on the same line (Knuth); the pairs
 * are counted on the 64 counters above, the count in their upper 48 bits,
 * so that the insertion that leaves the critical section counts its pair
 * in the same read-modify-write, and the sum, which reads 64 lines, is
 * taken once per 1024 insertions of a counter (fewer in a vector of less
 * than \f$ 2^{20} \f$ places, whose half 64 counters would otherwise pass
 * by much). An insertion that reads more than 128 words
 * also grows the table, if it is at least one eighth full; a table less
 * full than that, which only keys of equal hash can make so crowded, is
 * read on to its first free place instead (see
 * \ref theory_concurrent_flat).
 *
 * Each operation of a key is linearizable: a lookup at the read of the
 * word that holds the key or of the free place, an insertion at its
 * compare-and-swap or at the read of the word that holds the key.
 * `clear()`, `forEach()` and `consistent()` may not run with other
 * operations. The hash of a key is \f$ h( k ) \f$, taken as it is if `H`
 * declares `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()`
 * otherwise; `hash()` gives it, and the hash passed to the operations must
 * be that one.
 *
 * The contracts [see `Contract.h`]: the hash passed is `hash( k )`
 * (audit); the invariant is that each key is reached from its place
 * without a free place in between, checked on the key of each operation,
 * and `consistent()` checks it on every pair, with the counters. The
 * operations are defined in `ConcurrentFlatTable.tpp`.
 *
 * \see T. Maier, P. Sanders, R. Dementiev, Concurrent hash tables: fast
 *      and general(?)!, ACM Transactions on Parallel Computing 5(4), 16,
 *      2019, doi:10.1145/3309206
 * \see J. Shun, G. E. Blelloch, Phase-concurrent hash tables for
 *      determinism, ACM Symposium on Parallelism in Algorithms and
 *      Architectures (SPAA), 96-107, 2014, doi:10.1145/2612669.2612687
 * \see C. Purcell, T. Harris, Non-blocking hashtables with open
 *      addressing, International Symposium on Distributed Computing (DISC),
 *      LNCS 3724, 108-121, 2005, doi:10.1007/11561927_10
 * \see D. E. Knuth, The Art of Computer Programming, Volume 3: Sorting and
 *      Searching, 2nd edition, Addison-Wesley, 1998, section 6.4
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

#ifndef __ADS_ConcurrentFlatTable
 #define __ADS_ConcurrentFlatTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/memory/HugePageAllocator.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS ConcurrentFlatTable ------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY shared by threads, by linear probing on atomic words, that
/// grows while they work and never removes
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > >
class ConcurrentFlatTable {

 /// the bytes of the value in a word, none if it is empty
 static constexpr std::size_t valueBytes( void ) {
  return( std::is_empty_v< V > ? 0 : sizeof( V ) );
  }

 /// whether the bytes of the key say all of it
 static constexpr bool plainKey( void ) {
  return( std::has_unique_object_representations_v< K > ||
          std::is_floating_point_v< K > );
  }

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// true if the pairs lie in the words: the key and the value trivially
 /// copyable, the bytes of the key all significant, 8 bytes at most
 static constexpr bool packed = std::is_trivially_copyable_v< K > &&
                                std::is_trivially_copyable_v< V > &&
                                plainKey() &&
                                ( sizeof( K ) + valueBytes() <= 8 );

 /// a pair of a table that is not `packed`, in a node that never moves
 struct Entry {
  const K key;   ///< the key
  V value;       ///< the value, the caller's to synchronize
  };

 /// what an operation returns of the pair held: a copy of the value if
 /// `packed`, the pair itself otherwise
 using Ref = std::conditional_t< packed , V , Entry & >;

 /// the empty table, with room for \f$ n \f$ pairs (and their nodes)
 /// before it grows, the hash `h` and the equality `eq`
 explicit ConcurrentFlatTable( std::size_t n = 0 , H h = H() ,
                               Eq eq = Eq() );

 /// no copy, since the threads share it by its address
 ConcurrentFlatTable( const ConcurrentFlatTable & ) = delete;
 /// no copy, since the threads share it by its address
 ConcurrentFlatTable & operator=( const ConcurrentFlatTable & ) = delete;

 /// the frees of the pairs and of the vectors
 ~ConcurrentFlatTable();

 /// the hash of 64 bits of the key \f$ k \f$ that the operations take
 [[nodiscard]] std::uint64_t hash( const K & k ) const;

 /// the pair of the key \f$ k \f$, of hash \f$ h \f$, inserted with the
 /// value \f$ v \f$ if absent; true if it was inserted
 ///
 /// In pseudocode (the folklore table of Maier et al. that grows, beyond
 /// the book), \f$ W[ 0 .. 2^l - 1 ] \f$ the words of the vector in use,
 /// \f$ c_t \f$ the counter of the thread and \f$ g \f$ the flag of the
 /// growth:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findOrInsert}( k , h , v ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad c_t \gets c_t + 1\quad \triangleright\ \text{sequentially}
 ///     \text{ consistent} \cr
 /// 4 & \qquad \textbf{if}\ g\ \textbf{then}\ c_t \gets c_t - 1;
 ///     \ \mathrm{help}();\ \textbf{continue} \cr
 /// 5 & \qquad w \gets \text{the word of } ( k , v ) \text{, or of a new}
 ///     \text{ node of } ( h , k , v ) \cr
 /// 6 & \qquad i \gets \lfloor h / 2^{64 - l} \rfloor;\ r \gets 0 \cr
 /// 7 & \qquad \textbf{loop} \cr
 /// 8 & \qquad\quad x \gets W[ i ] \cr
 /// 9 & \qquad\quad \textbf{if}\ x = 0\ \textbf{and}\ \mathrm{CAS}( W[ i ] ,
 ///     0 , w )\ \textbf{then}\ c_t \gets c_t - 1 + 2^{16};
 ///     \ \textbf{return}\ ( w , \textbf{true} ) \cr
 /// 10 & \qquad\quad \textbf{if}\ x = 0\ \textbf{then}\ x \gets W[ i ]
 ///     \quad \triangleright\ \text{written by another thread} \cr
 /// 11 & \qquad\quad \textbf{if}\ x \text{ holds } k\ \textbf{then}
 ///     \ c_t \gets c_t - 1;\ \textbf{return}\ ( x , \textbf{false} ) \cr
 /// 12 & \qquad\quad i \gets ( i + 1 ) \bmod 2^l;\ r \gets r + 1 \cr
 /// 13 & \qquad\quad \textbf{if}\ r > 128\ \textbf{and}\ \text{the table}
 ///     \text{ is } 1/8 \text{ full}\ \textbf{then break} \cr
 /// 14 & \qquad c_t \gets c_t - 1;\ \mathrm{grow}( l + 1 ) \cr
 /// 15 & \textbf{procedure}\ \mathrm{grow}( l' ) \cr
 /// 16 & \quad \textbf{if}\ \mathrm{CAS}( g , \textbf{false} , \textbf{true}
 ///     )\ \textbf{then} \cr
 /// 17 & \qquad \textbf{while}\ \text{some } c_t > 0\ \textbf{do}
 ///     \ \text{a pause} \cr
 /// 18 & \qquad W' \gets 2^{l'} \text{ words } 0;\ \text{publish } W'
 ///     \text{ for the helpers} \cr
 /// 19 & \qquad \mathrm{help}();\ \textbf{wait until}\ \text{all the}
 ///     \text{ blocks are copied} \cr
 /// 20 & \qquad W \gets W';\ g \gets \textbf{false}\quad \triangleright
 ///     \ \text{release} \cr
 /// 21 & \quad \textbf{else}\ \mathrm{help}() \cr
 /// 22 & \textbf{procedure}\ \mathrm{help}() \cr
 /// 23 & \quad \textbf{while}\ g\ \textbf{do} \cr
 /// 24 & \qquad \textbf{foreach}\ \text{block of } 4096 \text{ words of } W
 ///     \text{ claimed by a counter}\ \textbf{do} \cr
 /// 25 & \qquad\quad \textbf{foreach}\ x \ne 0 \text{ in the block}
 ///     \ \textbf{do}\ \text{put } x \text{ at the first free place of its}
 ///     \text{ key in } W' \text{, by a CAS}
 /// \end{array}}
 /// \f]
 /// The insertions of the pairs counted at line 9 make the table grow,
 /// by `grow()`, when the sum of the counters passes half the places.
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 std::pair< Ref , bool > findOrInsert( const K & k , std::uint64_t h ,
                                       const V & v );

 /// `findOrInsert()` above, with the hash computed
 std::pair< Ref , bool > findOrInsert( const K & k , const V & v );

 /// inserts the pair \f$ ( k , v ) \f$ if \f$ k \f$ is absent; true if it
 /// was, as `findOrInsert()`
 bool insert( const K & k , const V & v );

 /// a copy of the value of the key \f$ k \f$ of hash \f$ h \f$, or nothing
 ///
 /// In pseudocode (beyond the book), with the notation of
 /// `findOrInsert()` above:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( k , h ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad T \gets \text{the vector in use } W\quad \triangleright
 ///     \ \text{acquire} \cr
 /// 4 & \qquad i \gets \lfloor h / 2^{64 - l} \rfloor \cr
 /// 5 & \qquad \textbf{while}\ T[ i ] \ne 0\ \textbf{do}\quad
 ///     \triangleright\ \text{acquire} \cr
 /// 6 & \qquad\quad \textbf{if}\ T[ i ] \text{ holds } k\ \textbf{then}
 ///     \ \textbf{return}\ \text{its value} \cr
 /// 7 & \qquad\quad i \gets ( i + 1 ) \bmod 2^l \cr
 /// 8 & \qquad \textbf{if}\ T \text{ is still the vector in use}
 ///     \ \textbf{then return nil}
 /// \end{array}}
 /// \f]
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 [[nodiscard]] std::optional< V > find( const K & k ,
                                        std::uint64_t h ) const;

 /// `find()` above, with the hash computed
 [[nodiscard]] std::optional< V > find( const K & k ) const;

 /// whether the key \f$ k \f$ of hash \f$ h \f$ is there, as `find()`
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 [[nodiscard]] bool contains( const K & k , std::uint64_t h ) const;

 /// `contains()` above, with the hash computed
 [[nodiscard]] bool contains( const K & k ) const;

 /// the pair of the key \f$ k \f$ of hash \f$ h \f$, or nil, as `find()`;
 /// for a table that is not `packed`
 /// \pre `hash( k )` is \f$ h \f$ (audit)
 [[nodiscard]] Entry * entry( const K & k , std::uint64_t h ) const
  requires( ! packed );

 /// sets the value \f$ x \f$ of the key \f$ k \f$ to `f( x )`, by a
 /// compare-and-swap of its word, for a `packed` table; true if \f$ k \f$
 /// is there
 template< class F >
 bool update( const K & k , F && f ) requires( packed );

 /// the number of the pairs, the sum of the counters, exact when no
 /// operation runs
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the number of the places of the vector in use
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the number of the growths of the table so far
 [[nodiscard]] std::size_t rehashes( void ) const;

 /// room for \f$ n \f$ pairs before the table grows: the vector grown to
 /// \f$ 2 n \f$ places at least, as by an insertion, and the nodes of
 /// \f$ n \f$ pairs allocated
 void reserve( std::size_t n );

 /// removes every pair, keeping the capacity; with no other operation
 /// running
 void clear( void );

 /// applies `f( const K & , const V & )` to every pair; with no insertion
 /// running
 template< class F >
 void forEach( F && f ) const;

 /// each key once, reached from its place without a free place in
 /// between, and the counters summing to the pairs; with no other
 /// operation running
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kLeast = 6;         ///< log2 of the least vector
 static constexpr int kStripes = 64;      ///< the counters of the threads
 static constexpr int kProbe = 128;       ///< the words read before a
                                          ///< growth
 static constexpr int kBlock = 4096;      ///< the words of a block copied
 static constexpr int kSlab = 64;         ///< the nodes a thread reserves
 static constexpr int kLgSeg = 12;        ///< log2 of the first segment
 static constexpr int kSegs = 29;         ///< the segments, at most
 static constexpr int kFp = 40;           ///< the bits of a node index

 /// a node: the pair with its hash
 struct Node {
  /// the hash of the key
  std::uint64_t h;
  /// the pair
  Entry e;
  };

 /// a counter of the threads: in the critical section in the low 16 bits,
 /// pairs inserted above them
 struct alignas( 64 ) Stripe {
  /// the two counters in one word
  std::atomic< std::uint64_t > w{ 0 };
  };

 /// a vector of 2^lg words, and its copy into the next one
 struct Table {
  /// log2 of the places
  int lg;
  /// the words
  std::atomic< std::uint64_t > * W;
  /// the vector it is being copied into, or nil
  std::atomic< Table * > next{ nullptr };
  /// the blocks claimed and those copied
  std::atomic< std::size_t > claimed{ 0 } , copied{ 0 };
  /// the vector of 2^l words 0
  explicit Table( int l );
  /// no copy, since it owns its words
  Table( const Table & ) = delete;
  /// no copy, since it owns its words
  Table & operator=( const Table & ) = delete;
  /// frees the words
  ~Table();
  /// the number of the places
  [[nodiscard]] std::size_t places( void ) const {
   return( std::size_t( 1 ) << lg );
   }
  /// the number of the blocks copied by the growth
  [[nodiscard]] std::size_t blocks( void ) const {
   return( ( places() + kBlock - 1 ) / kBlock );
   }
  };

 /// the reservation of a thread: the nodes [ next , end ) of the table id
 struct Slab {
  /// the table
  std::uint64_t id = 0;
  /// the nodes reserved and not used
  std::uint64_t next = 0 , end = 0;
  };

 /// the bytes of x as a word, the rest 0
 template< class X >
 [[nodiscard]] static std::uint64_t bits( const X & x , std::size_t at );

 /// the word of the pair ( k , v ) of a packed table
 [[nodiscard]] static std::uint64_t pack( const K & k , const V & v );

 /// the key of the word w of a packed table
 [[nodiscard]] static K keyOf( std::uint64_t w );

 /// the value of the word w of a packed table
 [[nodiscard]] static V valueOf( std::uint64_t w );

 /// the mark of the word of the key of zero bytes, kept apart
 [[nodiscard]] static std::uint64_t mark( void );

 /// whether the bytes of k are all 0, the key kept apart if packed
 [[nodiscard]] static bool zeroKey( const K & k );

 /// the place of the hash h in T
 [[nodiscard]] static std::size_t home( std::uint64_t h , const Table & T );

 /// the 24 bits of the hash h in a word that is not packed
 [[nodiscard]] static std::uint64_t print( std::uint64_t h );

 /// the node of the index i, 0 on
 [[nodiscard]] Node * node( std::uint64_t i ) const;

 /// whether the word w holds the key k of hash h
 [[nodiscard]] bool holds( std::uint64_t w , const K & k ,
                           std::uint64_t h ) const;

 /// the hash of the pair of the word w
 [[nodiscard]] std::uint64_t hashOf( std::uint64_t w ) const;

 /// the word that holds k of hash h in the vector in use, or 0
 [[nodiscard]] std::uint64_t lookup( const K & k , std::uint64_t h ) const;

 /// the counter of this thread
 [[nodiscard]] Stripe & stripe( void ) const;

 /// the critical section entered, waiting out a growth; the vector in use
 Table * enter( Stripe & s );

 /// the segment j of the nodes, allocated if it is not yet, its pages
 /// faulted if touch
 void segment( int j , bool touch );

 /// the segments of the first n nodes allocated and faulted, if not
 /// packed
 void nodes( std::size_t n );

 /// the index of a free node of the reservation of this thread
 std::uint64_t claim( void );

 /// the node of the index i, the first free one of the reservation of
 /// this thread, used
 void consume( std::uint64_t i );

 /// the reservations of this thread, for the tables it used last
 [[nodiscard]] static Slab * slabs( void );

 /// the copy of T into its next vector helped while a growth runs
 void help( void );

 /// the blocks of T copied into N, as many as claimed
 void copy( Table & T , Table & N );

 /// the vector grown from T to 2^lg places, by this thread or another
 void grow( Table * T , int lg );

 /// the insertions of a counter between two sums of the counters, for
 /// a vector of 2^lg places
 [[nodiscard]] static std::uint64_t period( int lg );

 /// the sum of the pairs of the counters
 [[nodiscard]] std::size_t counted( void ) const;

 /// whether the word w is reached from its place in T without a free
 /// place in between
 [[nodiscard]] bool reached( const Table & T , std::size_t i ) const;

 std::unique_ptr< Stripe[] > f_S;         ///< the counters of the threads
 std::atomic< bool > f_grow{ false };     ///< the flag of a growth
 std::atomic< Table * > f_table{ nullptr };   ///< the vector in use
 std::vector< std::unique_ptr< Table > > f_old;  ///< the vectors replaced
 std::unique_ptr< Table > f_cur;          ///< the vector in use, owned
 std::atomic< std::uint64_t > f_zero{ 0 };   ///< the pair of the key of
                                          ///< zero bytes, if packed
 std::atomic< Node * > f_seg[ kSegs ];    ///< the segments of the nodes
 std::atomic< std::uint64_t > f_nodes{ 0 };   ///< the nodes reserved
 std::atomic< std::uint64_t > f_id;       ///< the identity of the nodes
 std::atomic< std::size_t > f_rehashes{ 0 };  ///< the growths
 H f_h;
 Eq f_eq;

 };  // end( class ConcurrentFlatTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/ConcurrentFlatTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ConcurrentFlatTable.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ConcurrentFlatTable.h --------------------*/
/*--------------------------------------------------------------------------*/
