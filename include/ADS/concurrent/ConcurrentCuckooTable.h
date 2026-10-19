/*--------------------------------------------------------------------------*/
/*---------------------- File ConcurrentCuckooTable.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table that many threads read and change at once, beyond the book:
 * the cuckoo hashing with buckets of 4 places and tags of 8 bits of
 * BucketCuckooTable [see BucketCuckooTable.h], made concurrent by the
 * strategy of Li, Andersen, Kaminsky and Freedman (the table of libcuckoo),
 * with locks on stripes of buckets, a search for the cuckoo path that holds
 * no lock, and the moves of the path made one at a time from its end. A
 * key with the hash h has two buckets, \f$ b_1 \f$ given by the top bits
 * of h and \f$ b_2 = b_1 \oplus ( g( t ) \wedge ( B - 1 ) \vee 1 ) \f$ for
 * B buckets and the tag t of h (the partial-key cuckoo hashing of MemC3),
 * so that the other bucket of a key follows from its bucket and its tag.
 *
 * The buckets share \f$ S = 2048 \f$ locks, the stripe of the bucket b
 * being \f$ b \bmod S \f$ (the lock striping of Herlihy and Shavit); each
 * lock is a spin lock, a flag on a line of cache of its own tried by
 * test-and-test-and-set, which spins on a plain load while the flag is
 * held, with the pause of the processor and, after 64 rounds, a yield to
 * the scheduler. An operation on the key k locks the stripes of its two
 * buckets, the one of smaller index first, so that the locks are always
 * taken in increasing order: a thread that holds the stripe i and waits
 * for the stripe j has \f$ i < j \f$, and a cycle of waits
 * \f$ i_1 < i_2 < \dots < i_m < i_1 \f$ cannot exist, whence no deadlock.
 * The lookups take the locks too: an optimistic read under a counter of
 * versions (a seqlock) would copy keys and values while another thread
 * writes them, which is a data race in C++ for every type that is not
 * atomic (e.g., std::string), so that this table takes the simple and
 * general way, whose cost is two uncontended atomic exchanges per
 * operation. With uniform keys, two operations contend for a stripe with
 * probability about \f$ 4 / S \f$.
 *
 * An insertion puts the new key in a free place of its two buckets if any.
 * Otherwise, still under their locks, it looks for one of their 8 keys
 * whose other bucket has a free place, locking that bucket by a try that
 * never waits (so that it cannot close a cycle of waits), and moves the
 * key there, which is the path of one move. Failing that, it releases the
 * locks and searches in breadth, from the two buckets, a path of at most
 * \f$ L = 5 \f$ moves that ends in a bucket with a free place, locking
 * each bucket only while it reads its tags. A search in breadth over
 * buckets of B places that looks at M places finds a path of at most
 * \f[ L = \left\lceil \log_B \left( \frac{M}{2} - \frac{M}{2B} + 1 \right)
 *     \right\rceil \f]
 * moves, since its two trees have \f$ 2 B^l \f$ places at the depth l and
 * \f$ 2B( B^L - 1 ) / ( B - 1 ) \geq M \f$, against the M / 2 moves of a
 * search in depth; the search stops at \f$ L = 5 \f$, the value for
 * \f$ M = 2000 \f$ and B = 4, or at 1024 buckets. The keys are then moved
 * along the path from its end, each move locking the two buckets involved
 * and checking that the place still holds a tag equal to the one seen by
 * the search and that its destination still has a free place; since the
 * other bucket of a key depends only on its bucket and its tag, a move is
 * correct whatever key holds the place. If a check fails, the moves made
 * so far leave a correct table and the insertion starts again; when the
 * path is made, the insertion starts again too, and finds the place freed
 * at the start of the path, unless another thread took it.
 *
 * When no path is found, the table doubles: the thread takes all the
 * locks in use in increasing order (those of the first B stripes while
 * \f$ B < S \f$, since the stripe 0, taken first, keeps B from changing,
 * and those of the new buckets before B changes), checks that no other
 * thread has doubled the table in the meantime, and places every pair
 * again (stop-the-world). With 2 buckets per key and 4 places per
 * bucket, the cuckoo hashing of n random keys succeeds with high
 * probability while
 * \f$ n / ( 4B ) < c^*_{2,4} \approx 0.9803 \f$, the threshold of the
 * 4-orientability of the random graphs (Cain, Sanders and Wormald), so
 * that the table doubles near a load of 95 %, and the doublings cost
 * \f$ O( 1 ) \f$ amortized per insertion. A doubling that would leave the
 * table of at least 256 places less than 1/16 full throws
 * std::length_error, which happens only when more than 8 keys share both
 * buckets at every size, e.g., by equal hashes; the table is unchanged.
 *
 * Each stripe counts the insertions minus the removals made under its
 * lock, so that size() is the sum of S counters read without locks: exact
 * when no operation runs, and otherwise an estimate, since the counters
 * are read at different moments. The operations of a single key are
 * linearizable at the moment they hold the locks of its stripes; reserve(),
 * clear(), forEach() and consistent() take all the locks, and forEach()
 * thus visits the pairs of one moment (as the locked table of libcuckoo).
 * Maier, Sanders and Dementiev compare the concurrent hash tables, those
 * that grow without locks among them.
 *
 * The hash of a key is h( k ), taken as it is if H declares
 * is_avalanching [see FastHash.h] and mixed by hashWord() otherwise. The
 * keys and the values must be default constructible and movable.
 *
 * The contracts [see Contract.h]: the invariant is that each key is in one
 * of its two buckets with its tag, checked on the buckets of the key of
 * the operation under their locks; consistent() checks it on every pair,
 * and that the counters sum to the pairs, under all the locks. No
 * postcondition is stated on the key of an operation, since another
 * thread may change it as soon as the locks are released. The operations
 * are defined in ConcurrentCuckooTable.tpp.
 *
 * \see X. Li, D. G. Andersen, M. Kaminsky, M. J. Freedman, Algorithmic
 *      improvements for fast concurrent cuckoo hashing, Proceedings of
 *      EuroSys 2014, 27, 2014, doi:10.1145/2592798.2592820
 * \see B. Fan, D. G. Andersen, M. Kaminsky, MemC3: compact and concurrent
 *      MemCache with dumber caching and smarter hashing, Proceedings of
 *      NSDI 2013, 371-384, 2013 (the partial-key cuckoo hashing)
 * \see M. Herlihy, N. Shavit, The Art of Multiprocessor Programming,
 *      Morgan Kaufmann, 2008, chapter 13 (the lock striping)
 * \see J. A. Cain, P. Sanders, N. Wormald, The random graph threshold for
 *      k-orientiability and a fast algorithm for optimal multiple-choice
 *      allocation, Proceedings of SODA 2007, 469-476, 2007,
 *      https://publikationen.bibliothek.kit.edu/1000009500
 * \see T. Maier, P. Sanders, R. Dementiev, Concurrent hash tables: fast
 *      and general(?)!, ACM Transactions on Parallel Computing 5(4), 16,
 *      2019, doi:10.1145/3309206
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

#ifndef __ADS_ConcurrentCuckooTable
 #define __ADS_ConcurrentCuckooTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/FastHash.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ConcurrentCuckooTable -----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY shared by threads, cuckoo hashing with locks on stripes
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > >
class ConcurrentCuckooTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty table, with room for n pairs before it grows
 explicit ConcurrentCuckooTable( std::size_t n = 0 , H h = H() ,
                                 Eq eq = Eq() );

 ConcurrentCuckooTable( const ConcurrentCuckooTable & ) = delete;
 ConcurrentCuckooTable & operator=( const ConcurrentCuckooTable & ) =
  delete;

 /// a copy of the value of the key k, or nothing
 [[nodiscard]] std::optional< V > find( const K & k ) const;

 [[nodiscard]] bool contains( const K & k ) const;

 /// associates the value v to the key k if k is not there; true if it
 /// was not, and otherwise the value of k stays
 bool insert( const K & k , const V & v );

 /// associates the value v to the key k; true if k was not there
 bool insertOrAssign( const K & k , const V & v );

 /// applies f( V & ) to the value of the key k under its locks; true if k
 /// is there
 template< class F >
 bool update( const K & k , F && f );

 /// removes the key k, if it is there; true if it was
 bool erase( const K & k );

 /// the pairs, exact when no operation runs
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the places, 4 per bucket
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the doublings of the table
 [[nodiscard]] std::size_t rehashes( void ) const;

 /// room for n pairs before the table grows
 void reserve( std::size_t n );

 /// removes every pair, keeping the capacity
 void clear( void );

 /// applies f( const K & , V & ) to every pair, under all the locks
 template< class F >
 void forEach( F && f );

 /// applies f( const K & , const V & ) to every pair, under all the locks
 template< class F >
 void forEach( F && f ) const;

 /// each key in one of its two buckets with its tag and once, and the
 /// counters summing to the pairs; under all the locks
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Slot {
  K key{};
  V value{};
  };

 static constexpr std::size_t line = 64;          ///< a line of cache
 static constexpr std::size_t kStripes = 2048;    ///< the locks
 static constexpr int kDepth = 5;                 ///< the moves of a path
 static constexpr int kNodes = 1024;              ///< the buckets searched

 /// a lock and the counter of the pairs it added minus those it removed
 struct alignas( line ) Stripe {
  std::atomic< bool > held{ false };
  std::atomic< std::int64_t > count{ 0 };
  };

 /// the locks of one or two stripes, released at the end of the scope
 class Hold {
  public:
  Hold( Stripe * a , Stripe * b ) : f_a( a ) , f_b( b ) {}
  Hold( Hold && o ) noexcept;
  Hold( const Hold & ) = delete;
  Hold & operator=( const Hold & ) = delete;
  Hold & operator=( Hold && ) = delete;
  ~Hold();
  private:
  Stripe * f_a;
  Stripe * f_b;
  };

 /// all the locks in use, those of the stripes below the buckets,
 /// released at the end of the scope
 class HoldAll {
  public:
  explicit HoldAll( const ConcurrentCuckooTable & t );
  HoldAll( const HoldAll & ) = delete;
  HoldAll & operator=( const HoldAll & ) = delete;
  ~HoldAll();
  /// the locks of the stripes of b buckets too
  void extend( std::size_t b );
  /// the locks held
  [[nodiscard]] std::size_t held( void ) const { return( f_n ); }
  private:
  Stripe * f_L;
  std::size_t f_n = 0;
  };

 /// a bucket of the search in breadth: the node it comes from, the place
 /// of that bucket whose key moves into it and the tag of that key
 struct Node {
  std::size_t b;
  int parent , from , depth;
  std::uint32_t tag;
  };

 static void lock( Stripe & s );
 [[nodiscard]] static bool tryLock( Stripe & s );
 static void unlock( Stripe & s );

 /// the hash of k, mixed unless H avalanches
 [[nodiscard]] std::uint64_t hash( const K & k ) const;

 /// the tag of the hash h, never 0
 [[nodiscard]] static std::uint32_t tag( std::uint64_t h );

 /// the first bucket of the hash h among 2^lg
 [[nodiscard]] static std::size_t first( std::uint64_t h , int lg );

 /// the other bucket of a key with the tag t in the bucket b among 2^lg
 [[nodiscard]] static std::size_t other( std::size_t b , std::uint32_t t ,
                                         int lg );

 /// the locks of the stripes of the buckets a and b, in increasing order
 [[nodiscard]] Hold lockBuckets( std::size_t a , std::size_t b ) const;

 /// the locks of the two buckets b1 and b2 of the hash h, which it sets,
 /// in a table that does not change while they are held
 [[nodiscard]] Hold lockKey( std::uint64_t h , std::size_t & b1 ,
                             std::size_t & b2 ) const;

 /// the place of k with the hash h in the buckets b1 and b2, or ~0
 [[nodiscard]] std::size_t locate( const K & k , std::uint32_t t ,
                                   std::size_t b1 , std::size_t b2 ) const;

 /// a free place of the bucket b, or 4
 [[nodiscard]] int freePlace( std::size_t b ) const;

 /// the pair in the free place i of the bucket b with the tag t
 template< class KK , class VV >
 void put( std::size_t b , int i , std::uint32_t t , KK && k , VV && v );

 /// the key of the place i of the bucket a moved to the free place j of
 /// the bucket d
 void move( std::size_t a , int i , std::size_t d , int j );

 /// adds c to the counter of the stripe of the bucket b, whose lock is
 /// held
 void count( std::size_t b , std::int64_t c ) const;

 /// the search in breadth from the buckets of the hash h among 2^lg, the
 /// tags of a bucket by read( b , w ), false if the table changed: the
 /// node whose bucket has a free place, -1 if none within kDepth moves
 /// and kNodes buckets, -2 if the table changed
 template< class R >
 [[nodiscard]] int search( std::uint64_t h , int lg , Node * N ,
                           R && read ) const;

 /// the insertion of k with the value v, assigning it if k is there and
 /// assign; true if k was not there
 bool add( const K & k , const V & v , bool assign );

 /// a free place in the buckets b1 and b2, whose locks are held, made by
 /// moving one of their keys to its other bucket, or ~0
 std::size_t shift( std::size_t b1 , std::size_t b2 );

 /// the moves of a path to a free place for the hash h among 2^lg buckets,
 /// whose two buckets had the tags w1 and w2, under the locks of each
 /// move: 0 if made, 1 if the table changed (or the path broke), 2 if
 /// there is no path
 int cuckoo( std::uint64_t h , int lg , std::uint32_t w1 ,
             std::uint32_t w2 );

 /// the pair placed in the table, all locks held, false if no place
 template< class KK , class VV >
 bool place( std::uint64_t h , KK && k , VV && v );

 /// the table of 2^lg buckets, larger if needed, every pair placed again,
 /// under the locks g, extended to the new buckets
 void rehash( int lg , HoldAll & g );

 /// the table doubled, unless it is no more of 2^lg buckets
 void grow( int lg );

 /// the keys of the bucket b in it with their tags; its lock held
 [[nodiscard]] bool inBucket( std::size_t b ) const;

 std::unique_ptr< Stripe[] > f_L;   ///< the locks
 std::atomic< int > f_lg{ 1 };      ///< log2 of the buckets
 std::vector< std::uint32_t > f_T;  ///< the 4 tags of each bucket
 std::vector< Slot > f_S;           ///< the places, 4 per bucket
 std::atomic< std::size_t > f_rehashes{ 0 };
 H f_h;
 Eq f_eq;

 };  // end( class ConcurrentCuckooTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/ConcurrentCuckooTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ConcurrentCuckooTable.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ConcurrentCuckooTable.h --------------------*/
/*--------------------------------------------------------------------------*/
