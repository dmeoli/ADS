/*--------------------------------------------------------------------------*/
/*---------------------- File ConcurrentCuckooTable.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table that many threads read and change at once, beyond the book:
 * the cuckoo hashing with buckets of 4 places and tags of 8 bits of
 * `BucketCuckooTable` [see `BucketCuckooTable.h`], made concurrent with one
 * lock per region of buckets and, for keys and values that can be read
 * atomically, lookups that take no lock at all. A key with the hash
 * \f$ h \f$ has two buckets, \f$ b_1 \f$ given by the top bits of \f$ h \f$
 * and
 * \f[ b_2 = b_1 \oplus \left( \left\lfloor g( t ) / 2^{64 - r} \right\rfloor
 *     \vee 1 \right) \f]
 * for the tag \f$ t \f$ of \f$ h \f$, a mixing \f$ g \f$ of the tag (the
 * partial-key cuckoo hashing of MemC3) and regions of \f$ R = 2^r \f$
 * buckets, so that the other bucket of a key follows from its bucket and
 * its tag and both are in the same region.
 *
 * The \f$ B \f$ buckets form \f$ P = \min( S , \max( 1 , B / 256 ) ) \f$
 * regions of \f$ R = B / P \f$ buckets each, \f$ S = 2048 \f$, and a region
 * has one lock (the lock striping of Herlihy and Shavit, with a stripe per
 * region instead of a stripe of scattered buckets). Each lock is a counter
 * of versions on a line of cache of its own, odd while held, taken by a
 * compare-and-swap from an even value after spinning on plain loads, with
 * the pause of the processor and, after 64 rounds, a yield to the
 * scheduler. An operation on a key thus takes one lock, that of its
 * region, and the cuckoo path of an insertion, which never leaves the
 * region, is searched in breadth and its keys moved under that lock alone,
 * with nothing to check again (where Li et al. search without locks and
 * lock two buckets per move). Only the doubling, `reserve()`, `clear()`,
 * `forEach()` and `consistent()` take more locks, all those in use, in
 * increasing order from the stripe 0: a thread that holds the stripe
 * \f$ i \f$ and waits for the stripe \f$ j \f$ has \f$ i < j \f$, so that a
 * cycle of waits \f$ i_1 < \dots < i_m < i_1 \f$ cannot exist, whence no
 * deadlock. With uniform keys, two operations contend for a lock with
 * probability \f$ 1 / P \f$.
 *
 * When the keys and the values are trivially copyable and
 * `std::atomic_ref` reads and writes them without locks (e.g., integers and
 * pointers; `optimistic` is then true), a lookup is a read under the
 * version of the region (a seqlock): it reads the even version \f$ v \f$,
 * the table in use, the tags and the places of the key with atomic loads
 * that acquire, and returns what it read if the version is still \f$ v \f$
 * after them; otherwise it tries again, and takes the lock after 4 tries.
 * A writer makes the version odd and then stores with release, so that a
 * reader that sees one of its stores sees the odd version afterwards (the
 * analysis of Boehm, with the orders on the accesses in place of the two
 * fences, which the sanitizer of threads does not model, at the cost of
 * the plain loads and stores on x86-64). No access is a data
 * race in the sense of C++, and a lookup costs no atomic read-modify-write
 * and no store to a shared line. The tables replaced by a doubling are
 * kept until the table is destroyed, since a reader may still be on one;
 * with \f$ B_0 < \dots < B_k \f$ buckets they total
 * \f$ \sum_{j < k} B_j < B_k \f$, less than the table in use. Other keys
 * and values (e.g., `std::string`) are read under the lock, and the tables
 * replaced are freed at once.
 *
 * Each bucket has 4 bits of overflow, in a vector of their own of
 * \f$ B / 2 \f$ bytes, \f$ 1 / 8 \f$ of the tags: the bit \f$ j \f$ of
 * the bucket \f$ b \f$ is set when a key with \f$ b \f$ as its first
 * bucket and with \f$ j \f$ in the top 2 bits of its tag goes into its
 * other bucket, by an insertion or by a move along a path, and is cleared
 * only by a doubling or by `clear()` (the remap entries of the Horton
 * tables of Breslow et al., here reduced to bits). A key in its other
 * bucket thus has its bit set in its first one, and a lookup reads the
 * other bucket only when that bit is set: a key that is not there costs
 * the tags of one bucket, and a bit read from a vector that the cache
 * keeps. Since a key enters its other bucket after its bit is set, and
 * leaves its first bucket by a store of the tags that releases after the
 * bit, an absent key whose tag does not match in its first bucket, whose
 * bit is clear, is in neither bucket at the moment of the read of those
 * tags, if the table is still in use then: such a lookup returns without
 * reading the version of the region, which only the lookups that read
 * places need. The bits set by keys removed since stay, and make lookups
 * read the second bucket for nothing until the next doubling.
 *
 * A lookup, an insertion or a removal fetches ahead, at once, the tags
 * and the line of the places of the first bucket of its key (the 4 places
 * of a bucket are aligned to share a line of cache when their size is a
 * power of 2 up to 64 bytes), so that the misses overlap, and an
 * insertion the tags of the second bucket too, where the key goes if the
 * first is full; a lookup reads the second bucket only after its bit of
 * overflow, and its places only when a tag matches there. The tags and
 * the places of a table of 2 MB or more are on transparent huge pages
 * [see `HugePageAllocator.h`], so that the random accesses miss the TLB
 * once per 2 MB instead of once per 4 KB (Navarro et al.).
 *
 * An insertion puts the new key in a free place of its two buckets if
 * any; otherwise it searches in breadth, from the two buckets, a path of
 * at most \f$ L = 5 \f$ moves that ends in a bucket with a free place, and
 * moves its keys from the end. A search in breadth over buckets of
 * \f$ b \f$ places that looks at \f$ M \f$ places finds a path of at most
 * \f[ L = \left\lceil \log_b \left( \frac{M}{2} - \frac{M}{2b} + 1 \right)
 *     \right\rceil \f]
 * moves, since its two trees have \f$ 2 b^l \f$ places at the depth
 * \f$ l \f$ and \f$ 2 b ( b^L - 1 ) / ( b - 1 ) \ge M \f$, against the
 * \f$ M / 2 \f$ moves of a search in depth (Li et al.); the search stops at
 * \f$ L = 5 \f$, the value for \f$ M = 2000 \f$ and \f$ b = 4 \f$, or at
 * 1024 buckets, all in the region, whose tags are a block of \f$ 8 R \f$
 * bytes.
 *
 * The table doubles when a region would pass the load 0.98 or no path is
 * found: the thread takes all the locks in use, those of the new regions
 * too before the new table is seen, checks that no other thread has
 * doubled the table in the meantime, and places every pair again in a new
 * table, while no other thread changes the old one. Since the first
 * bucket of a key is the top bits of its hash, the pairs of the old
 * buckets \f$ [ l , h ) \f$ go to the new buckets
 * \f$ [ 2 l , 2 h ) \f$, which hold whole regions and receive no other
 * pair; the old table is therefore cut in parts of whole regions and of
 * at least 4096 buckets, which the threads waiting for a lock claim, by a
 * compare-and-swap of a counter that carries the number of the doubling,
 * and migrate with the thread that doubles, each making its part of the
 * new table empty and counting the pairs of its new regions (the
 * cooperative migration of Maier, Sanders and Dementiev, here under the
 * locks); the thread that doubles waits for all the parts, and only then
 * is the new table seen. With 2 buckets per key and 4 places per bucket,
 * the cuckoo hashing of \f$ n \f$ random keys succeeds with high
 * probability while \f$ n / ( 4 B ) < c^*_{2,4} \approx 0.9803 \f$, the
 * threshold of the 4-orientability of the random graphs (Cain, Sanders and
 * Wormald), here in each region; a region of \f$ 4 R \f$ places holds a
 * binomial number of the \f$ n \f$ keys, of mean \f$ \mu = n / P \f$ and
 * standard deviation \f$ \sqrt{ \mu ( 1 - 1 / P ) } \f$, i.e., a relative
 * deviation of about \f$ 1 / \sqrt{ 4 R \alpha } \f$ at the load
 * \f$ \alpha \f$, 3.3 % for \f$ R = 256 \f$ and \f$ \alpha = 0.9 \f$, so
 * that the fullest region of a large table reaches 0.98 at a mean load of
 * about 0.88 to 0.92 (the restriction of the choices to a part of the
 * table, there a page, is also in Dietzfelbinger, Mitzenmacher and Rink).
 * The doublings cost \f$ O( 1 ) \f$ amortized per insertion.
 *
 * A doubling that would leave a table of at least 256 places less than
 * 1/16 full is not made, since the key that finds no place then shares
 * both buckets with 8 others at every size (e.g., by equal hashes): the key
 * goes instead to a stash (Kirsch, Mitzenmacher and Wieder), a vector of
 * pairs under a mutex of its own, after the bit of overflow of its first
 * bucket is set, so that the lookups without locks treat it as a key in its
 * other bucket and then read the stash, under its mutex, inside the read of
 * the version. A pair enters the stash only under all the locks, and
 * leaves it or changes there under the lock of its region and the mutex;
 * a doubling places the pairs of the stash again with the others, and
 * those that find no place, as those that a doubling cannot place, go to
 * the new stash. The table thus has at most
 * \f$ \max( P_0 , 512 , 32 n ) \f$ places, \f$ P_0 \f$ those of the empty
 * table and \f$ n \f$ the most pairs it has held in its buckets, and a
 * lookup reads the stash only while it is not empty, which it is with any
 * but a degenerate hash.
 *
 * Each region counts its pairs, under its lock, so that `size()` is the sum
 * of \f$ P \f$ counters read without locks: exact when no operation runs,
 * and otherwise an estimate, since the counters are read at different
 * moments. The operations of a single key are linearizable at the moment
 * they hold the lock of its region, or at the second read of the version
 * for a lookup without locks; `reserve()`, `clear()`, `forEach()` and
 * `consistent()` take all the locks, and `forEach()` thus visits the pairs
 * of one moment. Maier, Sanders and Dementiev compare the concurrent hash
 * tables, those that grow without locks among them.
 *
 * The hash of a key is \f$ h( k ) \f$, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise.
 * The keys and the values must be default constructible and movable.
 *
 * The contracts [see `Contract.h`]: the invariant is that each key is in
 * one of its two buckets with its tag, and in the second only with its
 * bit of overflow set in the first, or in the stash with that bit set,
 * checked on the buckets of the key of the operation under its lock;
 * `consistent()` checks it on every pair, with the counters of the regions
 * and of the stash, under all the locks. No postcondition
 * is stated on the key of an operation, since another thread may change
 * it as soon as the lock is released. The operations are defined in
 * `ConcurrentCuckooTable.tpp`.
 *
 * \see B. Fan, D. G. Andersen, M. Kaminsky, MemC3: compact and concurrent
 *      MemCache with dumber caching and smarter hashing, Proceedings of
 *      NSDI 2013, 371-384, 2013 (the partial-key cuckoo hashing)
 * \see M. Herlihy, N. Shavit, The Art of Multiprocessor Programming,
 *      Morgan Kaufmann, 2008, chapter 13 (the lock striping)
 * \see X. Li, D. G. Andersen, M. Kaminsky, M. J. Freedman, Algorithmic
 *      improvements for fast concurrent cuckoo hashing, Proceedings of
 *      EuroSys 2014, 27, 2014, doi:10.1145/2592798.2592820
 * \see H.-J. Boehm, Can seqlocks get along with programming language
 *      memory models?, Proceedings of MSPC 2012, 12-20, 2012,
 *      doi:10.1145/2247684.2247688
 * \see J. Navarro, S. Iyer, P. Druschel, A. Cox, Practical, transparent
 *      operating system support for superpages, Proceedings of OSDI 2002,
 *      89-104, 2002
 * \see J. A. Cain, P. Sanders, N. Wormald, The random graph threshold for
 *      k-orientiability and a fast algorithm for optimal multiple-choice
 *      allocation, Proceedings of SODA 2007, 469-476, 2007,
 *      https://publikationen.bibliothek.kit.edu/1000009500
 * \see M. Dietzfelbinger, M. Mitzenmacher, M. Rink, Cuckoo hashing with
 *      pages, Proceedings of ESA 2011, 615-627, 2011,
 *      https://arxiv.org/abs/1104.5111
 * \see A. D. Breslow, D. P. Zhang, J. L. Greathouse, N. Jayasena,
 *      D. M. Tullsen, Horton tables: fast hash tables for in-memory
 *      data-intensive computing, Proceedings of USENIX ATC 2016, 281-294,
 *      2016,
 *      www.usenix.org/conference/atc16/technical-sessions/presentation/breslow
 * \see A. Kirsch, M. Mitzenmacher, U. Wieder, More robust hashing: cuckoo
 *      hashing with a stash, SIAM Journal on Computing 39(4), 1543-1561,
 *      2009, doi:10.1137/080728743 (the stash)
 * \see T. Maier, P. Sanders, R. Dementiev, Concurrent hash tables: fast
 *      and general(?)!, ACM Transactions on Parallel Computing 5(4), 16,
 *      2019, doi:10.1145/3309206
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

#ifndef __ADS_ConcurrentCuckooTable
 #define __ADS_ConcurrentCuckooTable
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/memory/HugePageAllocator.h"

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ConcurrentCuckooTable -----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY shared by threads, by cuckoo hashing with a lock per region
template< class K , class V , class H = DefaultHash< K > ,
	  class Eq = std::equal_to< K > >
class ConcurrentCuckooTable
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;    ///< the type of the keys
 using Value = V;  ///< the type of the values

 /// the empty table, with room for \f$ n \f$ pairs before it grows, the
 /// hash `h` and the equality `eq`
 explicit ConcurrentCuckooTable( std::size_t n = 0 , H h = H() ,
				 Eq eq = Eq() );

 /// no copy, since the threads share it by its address
 ConcurrentCuckooTable( const ConcurrentCuckooTable & ) = delete;
 /// no copy, since the threads share it by its address
 ConcurrentCuckooTable & operator=( const ConcurrentCuckooTable & ) = delete;

 /// a copy of the value of the key \f$ k \f$, or nothing
 ///
 /// In pseudocode (the seqlock of Boehm on the partial-key cuckoo hashing,
 /// beyond the book), \f$ v_R \f$ the version of the region \f$ R \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( k ) \cr
 /// 2 & \quad h \gets \mathrm{hash}( k );\ t \gets \mathrm{tag}( h ) \cr
 /// 3 & \quad \textbf{if}\ \mathit{optimistic}\ \textbf{then} \cr
 /// 4 & \qquad \textbf{for}\ a \gets 1\ \textbf{to}\ 4\ \textbf{do} \cr
 /// 5 & \qquad\quad T \gets \text{the table in use};\ b_1 \gets
 ///     \text{the first bucket of } h \text{ in } T;\ \text{fetch its lines}
 ///     \cr
 /// 6 & \qquad\quad \textbf{if}\ t \text{ is not among the tags of } b_1
 ///     \text{, its bit of overflow for } t \text{ is clear and } T
 ///     \text{ is still in use}\ \textbf{then return nil} \cr
 /// 7 & \qquad\quad v \gets v_R\quad \triangleright\ \text{acquire, } R
 ///     \text{ the region of } b_1 \cr
 /// 8 & \qquad\quad \textbf{if}\ v \text{ is odd}\ \textbf{or}\ T
 ///     \text{ is no longer in use}\ \textbf{then}\ \text{a pause};
 ///     \ \textbf{continue} \cr
 /// 9 & \qquad\quad x \gets \text{the value of the place of } b_1
 ///     \text{, or of } b_2 \text{ or of the stash if that bit is set, with}
 ///     \text{ the tag } t \text{ and the key } k \text{, or}\textbf{ nil}
 ///     \quad \triangleright\ \text{acquire} \cr
 /// 10 & \qquad\quad \textbf{if}\ v_R = v\ \textbf{then return}\ x \cr
 /// 11 & \quad \text{lock the region of } h \text{ in the table in use} \cr
 /// 12 & \quad \textbf{return}\ \text{the value of } k \text{ in } b_1 , b_2
 ///     \text{ or the stash, or}\ \textbf{nil}\quad \triangleright
 ///     \ \text{and unlock}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > find( const K & k ) const;

 /// whether the key \f$ k \f$ is there, as `find()`
 [[nodiscard]] bool contains( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$ if \f$ k \f$ is not
 /// there; true if it was not, and otherwise the value of \f$ k \f$ stays
 ///
 /// In pseudocode (the partial-key cuckoo hashing of MemC3 with the search
 /// in breadth of Li et al., beyond the book), \f$ n_R \f$ the pairs of the
 /// region \f$ R \f$ of \f$ 4 |R| \f$ places:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad h \gets \mathrm{hash}( k );\ t \gets \mathrm{tag}( h ) \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad T \gets \text{the table in use, with the lock of the region}
 ///     \text{ } R \text{ of } h \cr
 /// 5 & \qquad b_1 \gets \text{the top bits of } h;\ b_2 \gets b_1 \oplus (
 ///     g( t ) \text{ in } R ) \cr
 /// 6 & \qquad \textbf{if}\ k \text{ is in } b_1 \text{, in } b_2 \text{ or}
 ///     \text{ in the stash}\ \textbf{then}\ \text{unlock};\ \textbf{return
 ///     false} \cr
 /// 7 & \qquad p \gets \text{a free place of } b_1 \text{, else of } b_2
 ///     \text{ after the bit of overflow of } b_1 \text{ for } t \cr
 /// 8 & \qquad \textbf{if}\ p = \textbf{nil}\ \textbf{and}\ n_R + 1 \le
 ///     0.98 \cdot 4 |R|\ \textbf{then}\ p \gets \mathrm{makeRoom}( T , h )
 ///     \cr
 /// 9 & \qquad \textbf{if}\ p \ne \textbf{nil}\ \textbf{then} \cr
 /// 10 & \qquad\quad \text{put } ( k , v ) \text{ in } p \text{ with the}
 ///     \text{ tag } t;\ n_R \gets n_R + 1;\ \text{unlock};
 ///     \textbf{return true} \cr
 /// 11 & \qquad \text{unlock};\ \textbf{if}\ \mathrm{grow}( T , k , v )
 ///     \ \textbf{then return true} \cr
 /// 12 & \textbf{function}\ \mathrm{makeRoom}( T , h ) \cr
 /// 13 & \quad Q \gets \text{a queue of the buckets } b_1 , b_2 \text{ of }
 ///     h \text{ at the depth } 0 \cr
 /// 14 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 15 & \qquad b \gets \mathrm{dequeue}( Q ) \cr
 /// 16 & \qquad \textbf{if}\ b \text{ has a free place}\ \textbf{then} \cr
 /// 17 & \qquad\quad \textbf{foreach}\ \text{bucket } c \ne b_1 , b_2
 ///     \text{ on the path to } b \text{, from } b\ \textbf{do} \cr
 /// 18 & \qquad\qquad \text{move into } c \text{ the key of its parent that}
 ///     \text{ led to it, setting the bit of overflow of the parent for}
 ///     \text{ its tag} \cr
 /// 19 & \qquad\quad \textbf{return}\ \text{the place freed in } b_1
 ///     \text{ or } b_2 \cr
 /// 20 & \qquad \textbf{if}\ \text{the depth of } b < 5\ \textbf{then} \cr
 /// 21 & \qquad\quad \textbf{foreach}\ \text{tag } u \text{ of the 4 keys}
 ///     \text{ of } b\ \textbf{do} \cr
 /// 22 & \qquad\qquad c \gets \text{the other bucket of } u \text{ from }
 ///     b \cr
 /// 23 & \qquad\qquad \textbf{if}\ c \text{ is not on the path to } b
 ///     \ \textbf{and}\ |Q| \text{ in all} < 1024\ \textbf{then}
 ///     \ \mathrm{enqueue}( Q , c ) \cr
 /// 24 & \quad \textbf{return nil} \cr
 /// 25 & \textbf{function}\ \mathrm{grow}( T , k , v ) \cr
 /// 26 & \quad \text{lock all the stripes in use, in increasing order} \cr
 /// 27 & \quad \textbf{if}\ T \text{ is not in use}\ \textbf{then}
 ///     \ \text{unlock all};\ \textbf{return false} \cr
 /// 28 & \quad \textbf{if}\ 4 B \ge 256\ \textbf{and}\ 16 n < 4 B
 ///     \ \textbf{then}\quad \triangleright\ \text{too many equal hashes}
 ///     \cr
 /// 29 & \qquad \textbf{if}\ k \text{ is not in } T \text{ nor in the}
 ///     \text{ stash, and finds no place in } T \textbf{ then} \text{ set}
 ///     \text{ its bit of overflow in } b_1 \cr
 /// 30 & \qquad\quad \text{put } ( k , v ) \text{ in the stash, under its}
 ///     \text{ mutex} \cr
 /// 31 & \qquad \textbf{else if}\ k \text{ is not there}\ \textbf{then}
 ///     \ \text{put } ( k , v ) \text{ in its place} \cr
 /// 32 & \qquad \text{unlock all};\ \textbf{return true} \cr
 /// 33 & \quad T' \gets \text{a table of } 2 B \text{ buckets; place each}
 ///     \text{ pair of } T \text{ in } T' \text{, with the threads that}
 ///     \text{ wait for a lock, each on its parts of } T \cr
 /// 34 & \quad \text{place the pairs of the stash in } T' \text{; those}
 ///     \text{ that find no place, there and in the parts, go to the new}
 ///     \text{ stash} \cr
 /// 35 & \quad \text{lock the new stripes too};\ \text{count the pairs of}
 ///     \text{ each region};\ \text{make } T' \text{ the table in use}\quad
 ///     \triangleright\ \text{release} \cr
 /// 36 & \quad \text{unlock all};\ \textbf{return false}
 /// \end{array}}
 /// \f]
 bool insert( const K & k , const V & v );

 /// associates the value \f$ v \f$ to the key \f$ k \f$; true if \f$ k \f$
 /// was not there
 ///
 /// It is `insert()` above, but for a key found, whose value becomes
 /// \f$ v \f$ (at line 6, before the unlock).
 bool insertOrAssign( const K & k , const V & v );

 /// applies `f( V & )` to the value of the key \f$ k \f$ under its locks;
 /// true if \f$ k \f$ is there
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{update}( k , f ) \cr
 /// 2 & \quad \text{lock the region of } h = \mathrm{hash}( k ) \text{ in }
 ///     \text{the table in use} \cr
 /// 3 & \quad \textbf{if}\ k \text{ is not in its buckets nor in the}
 ///     \text{ stash}\ \textbf{then}\ \text{unlock};\ \textbf{return false}
 ///     \cr
 /// 4 & \quad x \gets \text{the value of } k;\ f( x );\ \text{store } x
 ///     \text{ as the value of } k\quad \triangleright\ \text{atomically if}
 ///     \text{ optimistic} \cr
 /// 5 & \quad \text{unlock};\ \textbf{return true}
 /// \end{array}}
 /// \f]
 template< class F >
 bool update( const K & k , F && f );

 /// removes the key \f$ k \f$, if it is there; true if it was
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{erase}( k ) \cr
 /// 2 & \quad \text{lock the region } R \text{ of } h = \mathrm{hash}( k )
 ///     \text{ in the table in use} \cr
 /// 3 & \quad \textbf{if}\ k \text{ is in the stash}\ \textbf{then}
 ///     \ \text{take it out, under its mutex};\ \text{unlock};
 ///     \ \textbf{return true} \cr
 /// 4 & \quad \textbf{if}\ k \text{ is not in its buckets}\ \textbf{then}
 ///     \ \text{unlock};\ \textbf{return false} \cr
 /// 5 & \quad \text{set the tag of its place to } 0;\ n_R \gets n_R - 1 \cr
 /// 6 & \quad \text{unlock};\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool erase( const K & k );

 /// the number of the pairs, the sum of the counters of the regions and of
 /// the stash, exact when no operation runs
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0
 [[nodiscard]] bool empty( void ) const;

 /// the number of the places, 4 per bucket
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the number of the doublings of the table so far
 [[nodiscard]] std::size_t rehashes( void ) const;

 /// the pairs in the stash, 0 but for degenerate hashes
 [[nodiscard]] std::size_t stashed( void ) const;

 /// room for \f$ n \f$ pairs before the table grows: the buckets doubled,
 /// under all the locks, up to the power of 2 at least \f$ n / 3.6 \f$
 void reserve( std::size_t n );

 /// removes every pair, keeping the capacity, under all the locks
 void clear( void );

 /// applies `f( const K & , V & )` to every pair, under all the locks
 template< class F >
 void forEach( F && f );

 /// applies `f( const K & , const V & )` to every pair, under all the locks
 template< class F >
 void forEach( F && f ) const;

 /// each key once in one of its two buckets with its tag or in the stash,
 /// and the counters summing to the pairs; under all the locks
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// the alignment of an object of type T read by std::atomic_ref
 template< class T >
 static constexpr std::size_t refAlign( void ) {
  if constexpr( std::is_trivially_copyable_v< T > )
   return( std::atomic_ref< T >::required_alignment );
  else
   return( alignof( T ) );
  }

 /// true if std::atomic_ref< T > never takes a lock
 template< class T >
 static constexpr bool refLockFree( void ) {
  if constexpr( std::is_trivially_copyable_v< T > )
   return( std::atomic_ref< T >::is_always_lock_free );
  else
   return( false );
  }

 public:
 /// true if the lookups read without locks, under the versions of the
 /// stripes: the keys and the values trivially copyable, and read and
 /// written atomically without locks
 static constexpr bool optimistic = refLockFree< K >() && refLockFree< V >();

/*--------------------------------------------------------------------------*/
 private:
 struct Slot
 {
  /// the key
  alignas( refAlign< K >() ) K key{};
  /// the value
  alignas( refAlign< V >() ) V value{};
  };

 static constexpr std::size_t line = 64;  ///< a line of cache

 /// the 4 places of a bucket, on one line of cache when they fit in one
 /// and their size is a power of 2
 struct alignas( ( 4 * sizeof( Slot ) <= line ) &&
		     std::has_single_bit( 4 * sizeof( Slot ) )
		   ? 4 * sizeof( Slot )
		   : alignof( Slot ) ) Bucket
 {
  /// the places
  Slot s[ 4 ];
  };
 static constexpr std::size_t kStripes = 2048;  ///< the locks, at most
 static constexpr int kLgStripes = 11;          ///< log2 of kStripes
 static constexpr int kLgRegion = 8;            ///< log2 of the least
						///< buckets of a region
 static constexpr int kDepth = 5;               ///< the moves of a path
 static constexpr int kNodes = 1024;            ///< the buckets searched
 static constexpr int kOverflow = 4;            ///< the bits of overflow
						///< of a bucket

 /// a lock with its version, odd while held, and the pairs of its region
 struct alignas( line ) Stripe
 {
  /// the version, odd while the lock is held
  std::atomic< std::uint64_t > version{ 0 };
  /// the pairs of the region
  std::atomic< std::int64_t > count{ 0 };
  };

 /// the buckets: 2^lg of them in regions of 2^rg, the tags and the places
 struct Table
 {
  /// log2 of the buckets and of the buckets of a region
  int lg , rg;
  /// the tags, 4 per bucket
  std::uint32_t * T;
  /// the bits of overflow, kOverflow per bucket
  std::uint64_t * O;
  /// the buckets
  Bucket * B;
  /// the empty table of \f$ 2^l \f$ buckets, or its memory only, whose
  /// parts are made empty by the migration [see migrate()], if not init
  explicit Table( int l , bool init = true );
  /// no copy, since it owns its buckets
  Table( const Table & ) = delete;
  /// no copy, since it owns its buckets
  Table & operator=( const Table & ) = delete;
  /// the words of the bits of overflow
  [[nodiscard]] std::size_t overflowWords( void ) const {
   return( ( ( buckets() * kOverflow ) + 63 ) >> 6 );
   }
  /// frees the tags, the bits of overflow and the buckets
  ~Table();
  /// the place i, the place i % 4 of the bucket i / 4
  [[nodiscard]] Slot & S( std::size_t i ) const {
   return( B[ i >> 2 ].s[ i & 3 ] );
   }
  /// the number of the buckets
  [[nodiscard]] std::size_t buckets( void ) const {
   return( std::size_t( 1 ) << lg );
   }
  };

 /// the lock of a stripe, released at the end of the scope
 class Hold
 {
  public:
  /// the lock of \f$ s \f$, already taken
  explicit Hold( Stripe & s ) : f_s( s ) { }
  /// no copy, since it releases its lock once
  Hold( const Hold & ) = delete;
  /// no copy, since it releases its lock once
  Hold & operator=( const Hold & ) = delete;
  /// releases the lock
  ~Hold() { unlock( f_s ); }

  private:
  Stripe & f_s;
  };

 /// all the locks in use, those of the regions, released at the end of
 /// the scope
 class HoldAll
 {
  public:
  /// all the locks in use of \f$ t \f$, taken from the stripe 0 up
  explicit HoldAll( const ConcurrentCuckooTable & t );
  /// no copy, since it releases its locks once
  HoldAll( const HoldAll & ) = delete;
  /// no copy, since it releases its locks once
  HoldAll & operator=( const HoldAll & ) = delete;
  /// releases the locks, the last first
  ~HoldAll();
  /// the locks of the regions of a table of 2^lg buckets too
  void extend( int lg );
  /// the locks held
  [[nodiscard]] std::size_t held( void ) const { return( f_n ); }

  private:
  Stripe * f_L;
  std::size_t f_n = 0;
  };

 /// a bucket of the search in breadth: the node it comes from, the place
 /// of that bucket whose key moves into it and the tag of that key
 struct Node
 {
  /// the bucket
  std::size_t b;
  /// the node it comes from, the place of that bucket, the moves from the
  /// buckets of the key
  int parent , from , depth;
  /// the tag of the key that moves
  std::uint32_t tag;
  };

 static void lock( Stripe & s );
 static void unlock( Stripe & s );

 /// the lock of s, helping a migration while it waits
 void lockHelping( Stripe & s ) const;

 /// a part of the migration in progress claimed in c; false if none is
 /// left
 bool claim( std::size_t & c ) const;

 /// the parts of the migration in progress that are left, migrated
 void help( void ) const;

 /// the part c of the migration in progress: its buckets of the new table
 /// made empty, its pairs of the old table placed in them (those that find
 /// no place kept apart), and the pairs of its new regions counted
 void migrate( std::size_t c ) const;

 /// log2 of the buckets of a region of a table of 2^lg buckets
 [[nodiscard]] static int regionLg( int lg );

 /// the hash of k, mixed unless H avalanches
 [[nodiscard]] std::uint64_t hash( const K & k ) const;

 /// the tag of the hash h, never 0
 [[nodiscard]] static std::uint32_t tag( std::uint64_t h );

 /// the first bucket of the hash h among 2^lg
 [[nodiscard]] static std::size_t first( std::uint64_t h , int lg );

 /// the other bucket of a key with the tag t in the bucket b, in the same
 /// region of 2^rg buckets
 [[nodiscard]] static std::size_t other( std::size_t b , std::uint32_t t ,
					 int rg );

 /// the tags of the bucket b of the table T
 [[nodiscard]] static std::uint32_t tags( const Table & T , std::size_t b );

 /// the tags of the bucket b of the table T set to w
 static void setTags( Table & T , std::size_t b , std::uint32_t w );

 /// the index of the bit of overflow of the bucket b for the tag t
 [[nodiscard]] static std::size_t overflowBit( std::size_t b ,
					       std::uint32_t t );

 /// whether some key with the tag t whose first bucket is b may be in its
 /// other bucket
 [[nodiscard]] static bool overflowed( const Table & T , std::size_t b ,
				       std::uint32_t t );

 /// the bit of overflow of the bucket b of T for the tag t set
 static void markOverflow( Table & T , std::size_t b , std::uint32_t t );

 /// the lock of the region of the hash h, and the table it is of
 [[nodiscard]] Table * lockRegion( std::uint64_t h ) const;

 /// lockRegion() after a lock found held, waiting for it
 [[nodiscard]] Table * waitRegion( std::uint64_t h ) const;

 /// the place of k with the tag t in the buckets b1 and b2 of T, or ~0;
 /// b2 read only if the bit of overflow of b1 for t is set, if bit
 [[nodiscard]] std::size_t locate( const Table & T , const K & k ,
				   std::uint32_t t , std::size_t b1 ,
				   std::size_t b2 , bool bit = true ) const;

 /// the tags and the places of the bucket b of T fetched ahead
 static void fetch( const Table & T , std::size_t b );

 /// the value of the key k with the hash h read without locks, if the
 /// versions allow it: 0 absent, 1 present (in *out if not nil), 2 not
 /// read
 int peek( const K & k , std::uint64_t h , V * out ) const;

 /// whether the key k with the hash h is there, read under the lock of its
 /// region, and its value in *out if not nil
 bool lockedFind( const K & k , std::uint64_t h , V * out ) const;

 /// a free place of the bucket b of T, or 4
 [[nodiscard]] static int freePlace( const Table & T , std::size_t b );

 /// the pair in the free place i of the bucket b of T with the tag t
 template< class KK , class VV >
 static void put( Table & T , std::size_t b , int i , std::uint32_t t ,
		  KK && k ,
		  VV && v );

 /// the value of the place i of T set to v
 static void assign( Table & T , std::size_t i , const V & v );

 /// the place i of T emptied
 static void clearPlace( Table & T , std::size_t i );

 /// the key of the place i of the bucket a of T moved to the free place j
 /// of the bucket d
 static void move( Table & T , std::size_t a , int i , std::size_t d , int j );

 /// the search in breadth in T from the buckets of the hash h: the node
 /// whose bucket has a free place, -1 if none within kDepth moves and
 /// kNodes buckets
 [[nodiscard]] int search( const Table & T , std::uint64_t h ,
			   Node * N ) const;

 /// a free place for the hash h in T, made by moving keys along a path
 /// within its region, or ~0
 std::size_t makeRoom( Table & T , std::uint64_t h ) const;

 /// a free place for the hash h in T, whose region holds n pairs: one of
 /// its buckets, or made by moving keys if the load stays within 0.98; ~0
 /// if none
 std::size_t room( Table & T , std::uint64_t h , std::int64_t n ) const;

 /// the pair x with the hash h placed in the table N, not yet seen by
 /// other threads, and taken from x; false if it found no place, x
 /// unchanged
 bool placeInto( Table & N , std::uint64_t h , Slot && x ) const;

 /// the insertion of k with the value v, assigning it if k is there and
 /// assign; true if k was not there
 bool add( const K & k , const V & v , bool assign );

 /// the table of 2^lg buckets, larger if needed, every pair placed again,
 /// under the locks g, extended to the new regions
 void rehash( int lg , HoldAll & g );

 /// the table doubled, unless it is no more of 2^lg buckets (then -1), or,
 /// if that would leave it less than 1/16 full, k with the hash h put in
 /// the stash (then 1) unless it is there (then 0, and its value v if
 /// assign) or finds a place
 int grow( int lg , std::uint64_t h , const K & k , const V & v ,
	   bool assign );

 /// whether k is in the stash, with its value in out if not nil
 bool fromStash( const K & k , V * out ) const;

 /// f( j ) applied, under the mutex of the stash, to the place j of k in
 /// the stash, if k is there; true if it is
 template< class F >
 bool inStash( const K & k , F && f );

 /// the pair x with the hash h in the stash, the bit of overflow of its
 /// first bucket in T set first; under all the locks
 void stashPut( Table & T , std::uint64_t h , Slot && x );

 /// the keys of the bucket b of T in it with their tags
 [[nodiscard]] bool inBucket( const Table & T , std::size_t b ) const;

 std::unique_ptr< Stripe[] > f_L;                ///< the locks
 std::atomic< int > f_lg{ 1 };                   ///< log2 of the buckets
 std::atomic< Table * > f_table{ nullptr };      ///< the table in use
 std::vector< std::unique_ptr< Table > > f_old;  ///< the tables replaced,
						 ///< if optimistic
 std::unique_ptr< Table > f_cur;                 ///< the table in use, owned
 std::atomic< std::size_t > f_rehashes{ 0 };

 /// the migration in progress: its number in the bits 32 and up, its parts
 /// in the bits 16 to 31, the next part to claim in the bits 0 to 15
 mutable std::atomic< std::uint64_t > f_claim{ 0 };
 mutable std::atomic< std::size_t > f_done{ 0 };    ///< its parts migrated
 mutable std::atomic< Table * > f_from{ nullptr };  ///< its old table
 mutable std::atomic< Table * > f_to{ nullptr };    ///< its new table
 mutable std::atomic< std::int64_t * > f_count{ nullptr };  ///< the pairs
							    ///< of its regions
 mutable std::atomic< std::size_t > f_part{ 0 };   ///< the old buckets of
						   ///< a part
 mutable std::vector< Slot > f_left;               ///< the pairs that found
						   ///< no place
 mutable std::atomic< bool > f_leftLock{ false };  ///< the lock of f_left
 mutable std::mutex f_xm;                          ///< the lock of the stash
 std::vector< Slot > f_X;                          ///< the stash
 std::atomic< std::size_t > f_xn{ 0 };             ///< the pairs in f_X
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

#endif /* ConcurrentCuckooTable.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ConcurrentCuckooTable.h --------------------*/
/*--------------------------------------------------------------------------*/
