/*--------------------------------------------------------------------------*/
/*---------------------- File LockFreePriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A priority queue shared by threads that is exact, linearizable and
 * lock-free, beyond the book: the skiplist of Lindén and Jonsson, whose
 * DeleteMin pays one read-modify-write in the common case and leaves the
 * nodes it deletes in the list, to be cut off in batches. It holds pairs
 * \f$ ( k , x ) \f$ of a key and an item, the least key first by `less`,
 * and it is meant as the shared OPEN of a parallel best-first search that
 * wants the exact order (e.g., SPA* of Burns et al.), where the MultiQueue
 * [see `MultiQueue.h`] relaxes it and the queues that are stolen [see
 * `StealablePriorityQueue.h`] keep it only per worker.
 *
 * The pairs are the nodes of a skiplist of \f$ L = 32 \f$ levels, between a
 * head and a tail that hold no pair; the list of level 0 holds all the
 * nodes in the order of their keys, and the list of level \f$ i \f$ a
 * subsequence of it, a node of height \f$ h \f$ being in the levels
 * \f$ 0 , \ldots , h - 1 \f$, with \f$ h \f$ drawn with probability
 * \f$ 2^{-h} \f$. The pointer of level 0 of a node carries in its lowest bit
 * the <em>delete flag</em> \f$ d \f$ of the node that follows: a node is
 * deleted when the pointer that reaches it at level 0 is marked, so that
 * the mark of a deletion sits in the predecessor and an insertion, which
 * swings that pointer by a compare-and-swap from its value unmarked, never
 * puts a node in front of a deleted one. The deleted nodes therefore form a
 * prefix of the list of level 0 (Lindén and Jonsson), and a DeleteMin walks
 * the prefix and deletes the first node not deleted by a fetch-and-or of
 * the mark, its only read-modify-write when nobody deletes the same node.
 * The prefix is not unlinked node by node: a DeleteMin that walked more than
 * \f$ b \f$ nodes (the parameter `offset`) moves the pointer of level 0 of
 * the head past the prefix by one compare-and-swap, the pointers of the
 * upper levels of the head after it (the <em>restructure</em>), and hands
 * the nodes cut off to the reclamation below. The keys equal under `less`
 * are told apart by a stamp of the insertion, a counter of the handle that
 * inserts and its number, so that the keys of the list are distinct, as the
 * algorithm wants them (the device of Sundell and Tsigas); two pairs of
 * equal keys come out in the order of their stamps.
 *
 * The guarantees, proved in \ref theory_concurrent_lfpq:
 * - `insert()` and `tryDeleteMin()` are linearizable, `insert()` at the
 *   compare-and-swap that links its node at level 0, `tryDeleteMin()` at
 *   the fetch-and-or that marks the pointer to the node it returns, or, when
 *   it returns nothing, at the read of a pointer to the tail from the head
 *   or from a deleted node; the abstract queue is the sequence of the nodes
 *   not deleted of level 0, ordered by (key, stamp). `tryMinKey()` is
 *   linearizable at the read of the first unmarked pointer of level 0.
 * - The operations are lock-free but for the allocation of the nodes: a
 *   compare-and-swap or a fetch-and-or fails, or a walk goes on, only
 *   because another operation changed the list, and each operation
 *   completes after a bounded number of its own successful
 *   read-modify-writes, \f$ L + 2 \f$ at most.
 * - The prefix that a DeleteMin walks: in a run of one thread a DeleteMin
 *   walks at most \f$ b + 1 \f$ nodes, the list keeps at most \f$ b \f$
 *   deleted nodes between two operations, and a cut comes once per
 *   \f$ b \f$ DeleteMin that delete a node; with many threads, a
 *   DeleteMin that walked more than \f$ b \f$ nodes cuts the prefix up to
 *   the node it deleted, or up to the first node being inserted that it
 *   walked, unless another cut started since it read the head or is in
 *   progress. A thread stopped
 *   inside an insertion or inside a cut lets the prefix grow without bound
 *   (Lindén and Jonsson remark it of the first case), and the DeleteMin
 *   then walk it all.
 *
 * We depart from the paper in two points. First, an insertion that links
 * its node \f$ n \f$ at an upper level checks that the successor \f$ c \f$
 * found there is not before \f$ n \f$ at level 0 by reading the successor
 * \f$ f \f$ of \f$ c \f$: \f$ c \f$ is before \f$ n \f$ only if it is
 * deleted, and then either its delete flag is set or \f$ f \f$ is \f$ n \f$
 * or a node of a key less than that of \f$ n \f$. The paper checks instead
 * whether \f$ c \f$ is the last deleted node met by the walk of level 0,
 * which misses \f$ c \f$ when the walk starts past it, as it does when the
 * upper lists have branches left by insertions after nodes cut off; the
 * node then points back to \f$ c \f$, and a later walk may reach through it
 * the nodes freed. Second, a
 * DeleteMin that cuts the prefix also sets the bit 1 of the pointer of level
 * 0 of the head, and clears it once its restructure is over, while a
 * DeleteMin that finds the bit set does not try a cut: the restructures run
 * one at a time, which makes the proof of the reclamation a matter of one
 * thread, and costs nothing to the progress, since no operation ever waits
 * for the bit.
 *
 * The memory of the nodes cut off is reclaimed by epochs (Fraser), owned by
 * the queue, since the area has no other reclamation that fits: each thread
 * uses the queue through a `Handle`, whose record announces the epoch it
 * runs in; a node cut off waits in the record of the thread that cut it,
 * stamped with the epoch of the cut, and is freed once the epoch has grown
 * by 2, which happens only after every thread running an operation has
 * announced the newer epoch. No node is leaked: the nodes not yet freed are
 * freed by the destructor. A thread stopped inside an operation stops the
 * epoch and keeps the nodes cut off afterwards from being freed (the
 * reclamation is blocking, as in the paper); the operations themselves
 * never wait for it.
 *
 * The keys must be copyable without exceptions, the items movable without
 * exceptions; `Less` is a strict weak order. A key is lowered by a new
 * insertion, the stale pair being skipped when it comes out
 * (`decrease_key_kind::lazy`). The contracts [see `Contract.h`]: no handle
 * is alive when the queue is destroyed; `size()`, `empty()` and
 * `consistent()` of the queue are called with no operation running, while
 * the operations of the handles may run at once on any threads, each
 * handle used by one thread at a time; `consistent()` checks the invariants
 * of the lists (the prefix, the order, the upper levels as subsequences of
 * level 0, the head not behind a node being inserted). The operations are
 * defined in `LockFreePriorityQueue.tpp`.
 *
 * \see J. Lindén, B. Jonsson, A skiplist-based concurrent priority queue
 *      with minimal memory contention, International Conference on
 *      Principles of Distributed Systems (OPODIS), LNCS 8304, 206-220,
 *      2013, doi:10.1007/978-3-319-03850-6_15
 * \see H. Sundell, P. Tsigas, Fast and lock-free concurrent priority queues
 *      for multi-thread systems, Journal of Parallel and Distributed
 *      Computing 65(5), 609-627, 2005, doi:10.1016/j.jpdc.2004.12.005
 *      (first at IPDPS 2003, doi:10.1109/IPDPS.2003.1213189)
 * \see E. Burns, S. Lemons, W. Ruml, R. Zhou, Best-first heuristic search
 *      for multicore machines, Journal of Artificial Intelligence Research
 *      39, 689-743, 2010, doi:10.1613/jair.3094
 * \see I. Lotan, N. Shavit, Skiplist-based concurrent priority queues,
 *      International Parallel and Distributed Processing Symposium (IPDPS),
 *      263-268, 2000, doi:10.1109/IPDPS.2000.845994
 * \see W. Pugh, Skip lists: a probabilistic alternative to balanced trees,
 *      Communications of the ACM 33(6), 668-676, 1990,
 *      doi:10.1145/78973.78977
 * \see T. L. Harris, A pragmatic implementation of non-blocking
 *      linked-lists, International Symposium on Distributed Computing
 *      (DISC), LNCS 2180, 300-314, 2001, doi:10.1007/3-540-45414-4_21
 * \see K. Fraser, Practical lock-freedom, PhD thesis, technical report
 *      UCAM-CL-TR-579, University of Cambridge, 2004, doi:10.48456/tr-579
 * \see T. E. Hart, P. E. McKenney, A. D. Brown, J. Walpole, Performance of
 *      memory reclamation for lockless synchronization, Journal of Parallel
 *      and Distributed Computing 67(12), 1270-1285, 2007,
 *      doi:10.1016/j.jpdc.2007.04.010
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

#ifndef __ADS_LockFreePriorityQueue
 #define __ADS_LockFreePriorityQueue
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Capabilities.h"
#include "ADS/Contract.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS LockFreePriorityQueue -----------------------*/
/*--------------------------------------------------------------------------*/

/// an exact PRIORITY QUEUE shared by threads, linearizable and lock-free:
/// the skiplist of Lindén and Jonsson, its deleted prefix cut in batches
template< class K , class T , class Less = std::less< K > >
class LockFreePriorityQueue
{

 static_assert( std::is_nothrow_copy_constructible_v< K > ,
		"the keys are copied without exceptions" );
 static_assert( std::is_nothrow_move_constructible_v< T > ,
		"the items are moved without exceptions" );

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;   ///< the type of the keys
 using Item = T;  ///< the type of the items

 /// a queue with no handles on the pairs, on which a search lowers a key
 /// by a new insertion [see `decrease_key_of()` of `Capabilities.h`]
 static constexpr decrease_key_kind decrease_key = decrease_key_kind::lazy;

 /// the levels of the skiplist
 static constexpr int kLevels = 32;

 /// a pair of the queue, its key and its item
 struct Entry
 {
  K key;   ///< the key, the least deleted first
  T item;  ///< the item
  };

 /// what a handle counted of its operations, for comparisons that do not
 /// time: the read-modify-writes, those that failed, the nodes walked by
 /// the DeleteMin, and the cuts of the prefix
 struct Counts
 {
  std::uint64_t rmw = 0;     ///< compare-and-swaps and fetch-and-ors
  std::uint64_t failed = 0;  ///< those that changed nothing
  std::uint64_t walked = 0;  ///< the nodes walked by the DeleteMin
  std::uint64_t cuts = 0;    ///< the prefixes cut off
  };

 class Handle;

 /// the empty queue, whose DeleteMin cut the prefix once they walked more
 /// than \f$ b \f$ = `offset` nodes, with the heights drawn from the seed
 /// `seed`
 /// \pre \f$ b \ge 1 \f$
 explicit LockFreePriorityQueue( unsigned offset = 32 ,
				 std::uint64_t seed = 1 ,
				 Less less = Less() );

 /// no copy, since the threads share it by its address
 LockFreePriorityQueue( const LockFreePriorityQueue & ) = delete;
 /// no copy, since the threads share it by its address
 LockFreePriorityQueue & operator=( const LockFreePriorityQueue & ) = delete;

 /// the frees of the nodes, of those waiting for their epoch and of the
 /// records
 /// \pre no handle is alive
 ~LockFreePriorityQueue();

 /// a handle for one thread, on a record of the epochs of its own
 [[nodiscard]] Handle handle( void );

 /// the length \f$ b \f$ of the prefix past which a DeleteMin cuts it
 [[nodiscard]] unsigned offset( void ) const { return( f_b ); }

 /// the number of the pairs, i.e., of the nodes not deleted of level 0;
 /// with no operation running
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0; with no operation running
 [[nodiscard]] bool empty( void ) const;

 /// the number of the deleted nodes still in the list of level 0; with no
 /// operation running
 [[nodiscard]] std::size_t prefix( void ) const;

 /// the invariants of the lists: the deleted nodes a prefix of level 0, the
 /// rest ordered by (key, stamp), each upper level a subsequence of level 0
 /// from the first node of the head, no node being inserted; with no
 /// operation running
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 using Word = std::uintptr_t;  ///< a pointer with its two bits

 static constexpr Word kMark = 1;  ///< the delete flag of the successor
 static constexpr Word kCut = 2;   ///< a cut in progress, on the head

 /// the links of a node, or of the head and of the tail: its height, its
 /// flag of an insertion in progress, and its \f$ h \f$ pointers
 struct Link
 {
  std::atomic< Word > * next;     ///< the pointers, after the node
  Link * later;                   ///< the next node of its limbo
  int height;                     ///< the levels of the node
  std::atomic< bool > inserting;  ///< true until the insertion ends
  };

 /// a node of a pair: its key, its stamp and its item, after its links
 struct Node : Link
 {
  K key;                ///< the key
  std::uint64_t stamp;  ///< the stamp that tells equal keys apart
  T item;               ///< the item, moved out by its DeleteMin
  };

 /// the record of a handle for the epochs: the epoch it announces, the
 /// nodes it cut off by the epoch of their cut, and whether a handle holds
 /// it; the records are never freed but by the destructor
 struct alignas( 64 ) Record
 {
  /// \f$ 2 e + 1 \f$ inside an operation of the epoch \f$ e \f$, 0 out
  std::atomic< std::uint64_t > local{ 0 };
  /// held by a handle
  std::atomic< bool > used{ true };
  /// the next record, fixed once the record is published
  Record * next = nullptr;
  /// the number of the record, in the stamps
  std::uint64_t id = 0;
  /// the insertions made on the record, in the stamps
  std::uint64_t count = 0;
  /// the nodes cut off, by the epoch of their cut modulo 3, linked by
  /// their field later
  Link * limbo[ 3 ] = { nullptr , nullptr , nullptr };
  /// the epoch of the nodes of each limbo
  std::uint64_t epoch[ 3 ] = { 0 , 0 , 0 };
  };

 /// the node of w
 [[nodiscard]] static Link * ptr( Word w ) {
  return( reinterpret_cast< Link * >( w & ~( kMark | kCut ) ) );
  }

 /// the word of the node p, unmarked
 [[nodiscard]] static Word word( const Link * p ) {
  return( reinterpret_cast< Word >( p ) );
  }

 /// the delete flag of the node that p.next[ 0 ] reaches
 [[nodiscard]] static bool marked( const Link * p ) {
  return( p->next[ 0 ].load( std::memory_order_acquire ) & kMark );
  }

 /// the memory of a link of n bytes and of its h pointers, the pointers
 /// constructed and null, their address in next
 static void * allocate( std::size_t n , int h , std::atomic< Word > *& next );

 /// the head or the tail, of height h
 static Link * makeLink( int h );

 /// a new node of height h with the pair ( k , x ) and the stamp s
 static Node * make( int h , const K & k , std::uint64_t s , T && x );

 /// the free of the node p
 static void destroy( Node * p );

 /// the free of the head or of the tail p
 static void destroyLink( Link * p );

 /// the nodes of the limbo from p freed
 static void destroyAll( Link * p );

 /// whether the node p is before the key ( k , s )
 [[nodiscard]] bool before( const Link * p , const K & k ,
			    std::uint64_t s ) const;

 /// the predecessors and the successors of the key ( k , s ) at each
 /// level
 void locate( const K & k , std::uint64_t s , Link ** preds ,
	      Link ** succs ) const;

 /// whether the node c, found as the successor at an upper level of the
 /// node n linked at level 0, may be before n in the list of level 0: true
 /// if it is, false only if c is after n
 [[nodiscard]] bool behind( const Link * c , const Link * n ) const;

 /// the upper pointers of the head moved past the deleted nodes, by the
 /// one thread that holds the bit kCut; its read-modify-writes counted
 void restructure( Counts & c );

 /// the nodes of the records freed, and the records
 void release( void );

 unsigned f_b;   ///< the length of a cut
 Less f_less;    ///< the order of the keys
 Link * f_head;  ///< the head
 Link * f_tail;  ///< the tail
 alignas( 64 ) std::atomic< std::uint64_t > f_epoch{ 2 };
 ///< the epoch, from 2
 alignas( 64 ) std::atomic< Record * > f_records{ nullptr };
 ///< the records
 std::atomic< std::uint64_t > f_ids{ 0 };  ///< the numbers given
 std::atomic< std::uint64_t > f_seed;      ///< the seeds of the handles

 };  // end( class LockFreePriorityQueue )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS LockFreePriorityQueue::Handle -----------------*/
/*--------------------------------------------------------------------------*/

/// the access of one thread to a `LockFreePriorityQueue`: its record of the
/// epochs, the generator of its heights and the counter of its stamps
template< class K , class T , class Less >
class LockFreePriorityQueue< K , T , Less >::Handle
{

/*--------------------------------------------------------------------------*/
 public:
 /// the record moved, the other handle left without one
 Handle( Handle && o ) noexcept;

 /// no copy, since a record has one owner
 Handle( const Handle & ) = delete;
 /// no assignment, since a record has one owner
 Handle & operator=( const Handle & ) = delete;
 /// no assignment, since a record has one owner
 Handle & operator=( Handle && ) = delete;

 /// the record given back to the queue, for another handle
 ~Handle();

 /// inserts the pair \f$ ( k , x ) \f$, linearizable at the
 /// compare-and-swap that links its node at level 0
 ///
 /// In pseudocode (Lindén and Jonsson, with our test of a successor before
 /// the node, beyond the book), \f$ d( p ) \f$ the mark of
 /// \f$ p.\mathit{next}[ 0 ] \f$, i.e., the delete flag of the node after
 /// \f$ p \f$, and \f$ \kappa = ( k , \text{the stamp} ) \f$ the key made
 /// distinct:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , x ) \cr
 /// 2 & \quad n \gets \text{a node } ( \kappa , x ) \text{ of height } h
 ///     \text{, drawn with probability } 2^{-h};\ n.\mathit{inserting}
 ///     \gets \mathit{true} \cr
 /// 3 & \quad \textbf{repeat}\ ( P , S ) \gets \mathrm{locate}( \kappa );
 ///     \ n.\mathit{next}[ 0 ] \gets S_0 \cr
 /// 4 & \quad \textbf{until}\ \mathrm{CAS}( P_0.\mathit{next}[ 0 ] ,
 ///     \langle S_0 , 0 \rangle , \langle n , 0 \rangle )\quad
 ///     \triangleright\ \text{the linearization point} \cr
 /// 5 & \quad i \gets 1 \cr
 /// 6 & \quad \textbf{while}\ i < h\ \textbf{do} \cr
 /// 7 & \qquad \textbf{if}\ d( n )\ \textbf{or}\ \mathrm{behind}( S_i , n )
 ///     \ \textbf{then break}\quad \triangleright\ n \text{ deleted, or }
 ///     S_i \text{ maybe before } n \cr
 /// 8 & \qquad n.\mathit{next}[ i ] \gets S_i \cr
 /// 9 & \qquad \textbf{if}\ \mathrm{CAS}( P_i.\mathit{next}[ i ] , S_i ,
 ///     n )\ \textbf{then}\ i \gets i + 1 \cr
 /// 10 & \qquad \textbf{else}\ ( P , S ) \gets \mathrm{locate}( \kappa );
 ///     \ \textbf{if}\ S_0 \ne n\ \textbf{then break} \cr
 /// 11 & \quad n.\mathit{inserting} \gets \mathit{false} \cr
 /// 12 & \textbf{function}\ \mathrm{locate}( \kappa ) \cr
 /// 13 & \quad p \gets \mathit{head} \cr
 /// 14 & \quad \textbf{for}\ i \gets L - 1\ \textbf{downto}\ 0\ \textbf{do}
 ///     \cr
 /// 15 & \qquad c \gets p.\mathit{next}[ i ] \cr
 /// 16 & \qquad \textbf{while}\ c.\kappa < \kappa\ \textbf{or}\ d( c )
 ///     \ \textbf{or}\ ( i = 0\ \textbf{and}\ d( p ) )\ \textbf{do}
 ///     \ p \gets c;\ c \gets p.\mathit{next}[ i ] \cr
 /// 17 & \qquad P_i \gets p;\ S_i \gets c \cr
 /// 18 & \quad \textbf{return}\ ( P , S ) \cr
 /// 19 & \textbf{function}\ \mathrm{behind}( c , n ) \cr
 /// 20 & \quad \textbf{if}\ c = \mathit{tail}\ \textbf{then return}
 ///     \ \mathit{false} \cr
 /// 21 & \quad \langle f , m \rangle \gets c.\mathit{next}[ 0 ] \cr
 /// 22 & \quad \textbf{return}\ m\ \textbf{or}\ f = n\ \textbf{or}\ ( f \ne
 ///     \mathit{tail}\ \textbf{and}\ f.\kappa < n.\kappa )
 /// \end{array}}
 /// \f]
 /// where the tail has the greatest key and is never deleted.
 void insert( K k , T x );

 /// the pair of least key, removed; nothing if the queue was empty at the
 /// linearization point
 ///
 /// In pseudocode (Lindén and Jonsson, with the bit of the cut of our
 /// variant, beyond the book), with the notation of `insert()` above and
 /// \f$ b \f$ = `offset()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryDeleteMin}() \cr
 /// 2 & \quad x \gets \mathit{head};\ o \gets 0;\ \mathit{nh} \gets
 ///     \mathrm{nil};\ \mathit{obs} \gets \mathit{head}.\mathit{next}[ 0 ]
 ///     \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad o \gets o + 1;\ \langle y , m \rangle \gets
 ///     x.\mathit{next}[ 0 ] \cr
 /// 5 & \qquad \textbf{if}\ y = \mathit{tail}\ \textbf{then return nil}
 ///     \quad \triangleright\ \text{empty: linearized at this read} \cr
 /// 6 & \qquad \textbf{if}\ \mathit{nh} = \mathrm{nil}\ \textbf{and}
 ///     \ x.\mathit{inserting}\ \textbf{then}\ \mathit{nh} \gets x \cr
 /// 7 & \qquad \textbf{if not}\ m\ \textbf{then}\ \langle y , m \rangle
 ///     \gets \mathrm{FAO}( x.\mathit{next}[ 0 ] , 1 )\quad \triangleright
 ///     \ \text{the linearization point if } m = 0 \cr
 /// 8 & \qquad x \gets y;\ \textbf{if not}\ m\ \textbf{then break} \cr
 /// 9 & \quad e \gets ( x.k , x.x )\text{, the item moved out};
 ///     \ \textbf{if}\ \mathit{nh} = \mathrm{nil}\ \textbf{then}\ \mathit{nh}
 ///     \gets x \cr
 /// 10 & \quad \textbf{if}\ o > b\ \textbf{and}\ \mathit{obs} \text{ has no
 ///     cut bit}\ \textbf{and}\ \mathit{nh} \ne \mathit{obs}\ \textbf{and}
 ///     \ \mathrm{CAS}( \mathit{head}.\mathit{next}[ 0 ] , \mathit{obs} ,
 ///     \langle \mathit{nh} , 1 , \mathit{cut} \rangle )\ \textbf{then} \cr
 /// 11 & \qquad \mathrm{restructure}();\ \text{clear the cut bit} \cr
 /// 12 & \qquad \text{retire the nodes from } \mathit{obs} \text{ to }
 ///     \mathit{nh} \text{, the latter excluded} \cr
 /// 13 & \quad \textbf{return}\ e \cr
 /// 14 & \textbf{procedure}\ \mathrm{restructure}() \cr
 /// 15 & \quad i \gets L - 1;\ p \gets \mathit{head} \cr
 /// 16 & \quad \textbf{while}\ i > 0\ \textbf{do} \cr
 /// 17 & \qquad h \gets \mathit{head}.\mathit{next}[ i ];\ c \gets
 ///     p.\mathit{next}[ i ]\quad \triangleright\ \text{in this order} \cr
 /// 18 & \qquad \textbf{if not}\ d( h )\ \textbf{then}\ i \gets i - 1;
 ///     \ \textbf{continue} \cr
 /// 19 & \qquad \textbf{while}\ d( c )\ \textbf{do}\ p \gets c;\ c \gets
 ///     p.\mathit{next}[ i ] \cr
 /// 20 & \qquad \textbf{if}\ \mathrm{CAS}( \mathit{head}.\mathit{next}[ i ]
 ///     , h , c )\ \textbf{then}\ i \gets i - 1
 /// \end{array}}
 /// \f]
 std::optional< Entry > tryDeleteMin( void );

 /// the least key, linearizable at the read of the first pointer of level
 /// 0 not marked; nothing if the queue was empty there
 std::optional< K > tryMinKey( void );

 /// whether the queue was empty at the linearization point of
 /// `tryMinKey()`
 [[nodiscard]] bool empty( void ) { return( ! tryMinKey() ); }

 /// what the handle counted so far
 [[nodiscard]] const Counts & counts( void ) const { return( f_c ); }

/*--------------------------------------------------------------------------*/
 private:
 friend class LockFreePriorityQueue;

 /// the handle of q on the record r, its heights drawn from the seed
 Handle( LockFreePriorityQueue & q , Record * r , std::uint64_t seed )
     : f_q( &q ) , f_r( r ) , f_x( seed ) { }

 /// a random number of 64 bits (the splitmix64 of Steele et al.)
 std::uint64_t next( void );

 /// the height of a new node, \f$ h \f$ with probability \f$ 2^{-h} \f$,
 /// at most \f$ L \f$
 int height( void );

 /// the epoch announced, and the nodes of the record freed whose epoch is
 /// 2 behind
 void enter( void );

 /// out of the operation
 void leave( void );

 /// the node p handed to the reclamation, in the epoch now
 void retire( Node * p );

 /// the epoch raised if every record inside an operation announces it
 void advance( void );

 LockFreePriorityQueue * f_q;  ///< the queue
 Record * f_r;                 ///< the record, nil once moved
 std::uint64_t f_x;            ///< the state of the generator
 Counts f_c;                   ///< the counts

 };  // end( class LockFreePriorityQueue::Handle )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/LockFreePriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif /* LockFreePriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File LockFreePriorityQueue.h --------------------*/
/*--------------------------------------------------------------------------*/
