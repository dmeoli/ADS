/*--------------------------------------------------------------------------*/
/*----------------------- File ConcurrentBTreeMap.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered map shared by threads, beyond the book: the B+-tree with
 * optimistic lock coupling of Leis, Haubenschild and Neumann, in which a
 * lookup writes no shared word and a modification locks only the nodes it
 * changes, with the erasures that merge the nodes left short and the
 * memory of the nodes merged away reclaimed by epochs. It holds pairs
 * \f$ ( k , v ) \f$ of distinct keys in the order of `less`, and gives,
 * besides the operations of one key, the least key not smaller than a
 * given one (`lowerBound()`), the least key greater than it
 * (`upperBound()`) and the visit of the pairs in order from a key on
 * (`scan()`), the range queries of an ordered index (e.g., the YCSB mix
 * of scans) that the concurrent hash tables of the area [see
 * `ConcurrentFlatTable.h`, `ConcurrentCuckooTable.h`] cannot give.
 *
 * The tree is that of `BTreeMap` [see `BTreeMap.h`]: the pairs are in the
 * leaves, sorted, up to \f$ L \f$ in a leaf, the keys in one array and
 * the values in another; an internal node holds up to \f$ I \f$ keys and
 * one child more, the child \f$ c[ i ] \f$ having the keys in
 * \f$ ( k[ i - 1 ] , k[ i ] ] \f$; \f$ L \f$ and \f$ I \f$ come from the
 * bytes \f$ B \f$ of a node (1024 by default, 16 lines of cache), each
 * between 4 and 255. All
 * the leaves are at the same depth. Each node has a <em>version</em>, a
 * word of 64 bits whose bit 1 is the lock of the node, whose bit 0 marks
 * it <em>obsolete</em>, i.e., unlinked from the tree, and whose other bits
 * count the times the node was unlocked; every field of a node is an
 * atomic word, so that the reads that race with a writer are defined.
 *
 * A thread reads a node <em>optimistically</em>: it reads its version,
 * waiting while the lock is held and starting the operation over if the
 * node is obsolete, then the fields it needs, and then the version again;
 * the fields are those of one state of the node if the version did not
 * change (a <em>check</em>), since every writer holds the lock and raises
 * the version when it lets it go. A descent reads the child of a node and
 * checks the node before reading the version of the child, then checks
 * the node again (the <em>coupling</em>), so that the child it reaches is
 * the one of its key while the version of the child holds. A modification
 * takes the lock of a node by a compare-and-swap of the version read
 * (an <em>upgrade</em>), which fails if the node changed since, and then
 * the whole operation starts over: no thread waits for a lock while it
 * holds one, and no deadlock can occur. An insertion splits on its way
 * down every full node it meets, locking it with its parent, as in the
 * paper, so that the parent of a node that splits always has room; an
 * erasure that meets on its way down a child with fewer than
 * \f$ \lfloor C / 4 \rfloor \f$ keys (\f$ C \f$ its capacity) locks the
 * node, the child and a sibling of it, and either merges the two children,
 * if their pairs fit in \f$ 3 C / 4 \f$, or shares them evenly, and a root
 * left with one child gives way to it. A node merged away, or a root
 * replaced, is unlocked as obsolete and retired to the epochs.
 *
 * The guarantees, which \ref theory_concurrent_omap proves, are these:
 * - `insert()`, `assign()` and `erase()` are linearizable at the upgrade
 *   that locks the leaf of their key for the last time; `find()` and
 *   `contains()` at the check of their leaf; `lowerBound()` and
 *   `upperBound()` at the first of the checks with which they read again,
 *   at their end, the versions of all the leaves they read. `scan()` reads
 *   each leaf as one state of it, and is therefore linearizable leaf by
 *   leaf: a pair that it reports was in the map while it ran, and a pair
 *   in the map during the whole scan, in the range it visits, is
 *   reported.
 * - The modifications are blocking (a thread stopped holding the lock of
 *   a node holds up those that need the node, and the lookups that read
 *   it), but deadlock-free; a lookup writes no shared word. An operation
 *   starts over only because another one changed a node it read; the
 *   restarts may in principle repeat, as in the paper.
 * - The memory of a node unlinked is reclaimed by the epochs of Fraser
 *   [see `impl/OwnedEpochs.tpp`], owned by the map: each thread uses the map
 *   through a `Handle`, whose record announces the epoch in which it runs
 *   an operation, and a node retired is freed once the epoch has grown by
 *   2, i.e., once every thread that might still read it has left its
 *   operation. The reclamation is blocking: a thread stopped inside an
 *   operation keeps the nodes retired afterwards from being freed. No node
 *   is leaked: the destructor frees the tree and the nodes still retired.
 *
 * The keys and the values are trivially copyable and default
 * constructible, and their atomic words are free of locks (8 bytes at
 * most on the usual processors), since a reader copies them out of a node
 * that a writer may be changing; a value too large for a word is kept
 * elsewhere, by its index. `Less` is a strict weak order that does not
 * throw. The contracts [see `Contract.h`]: no handle is alive when the
 * map is destroyed; `size()`, `empty()`, `height()`, `forEach()` and
 * `consistent()` of the map are called with no operation running, while
 * the operations of the handles may run at once on any threads, each
 * handle used by one thread at a time; `consistent()` checks the
 * invariants of the tree (the keys sorted, each key of a node within the
 * range that its parent gives it, the leaves at one depth, the nodes
 * neither locked nor obsolete). The operations are defined in
 * `ConcurrentBTreeMap.tpp`.
 *
 * \see V. Leis, M. Haubenschild, T. Neumann, Optimistic lock coupling: a
 *      scalable and efficient general-purpose synchronization method, IEEE
 *      Data Engineering Bulletin 42(1), 73-84, 2019
 * \see V. Leis, F. Scheibner, A. Kemper, T. Neumann, The ART of practical
 *      synchronization, International Workshop on Data Management on New
 *      Hardware (DaMoN), 3:1-3:8, 2016, doi:10.1145/2933349.2933352
 * \see Z. Wang, A. Pavlo, H. Lim, V. Leis, H. Zhang, M. Kaminsky, D. G.
 *      Andersen, Building a Bw-tree takes more than just buzz words,
 *      International Conference on Management of Data (SIGMOD), 473-488,
 *      2018, doi:10.1145/3183713.3196895
 * \see A. Srivastava, T. Brown, Elimination (a,b)-trees with fast, durable
 *      updates, Symposium on Principles and Practice of Parallel
 *      Programming (PPoPP), 416-430, 2022, doi:10.1145/3503221.3508441
 * \see P. L. Lehman, S. B. Yao, Efficient locking for concurrent operations
 *      on B-trees, ACM Transactions on Database Systems 6(4), 650-670,
 *      1981, doi:10.1145/319628.319663
 * \see S. K. Cha, S. Hwang, K. Kim, K. Kwon, Cache-conscious concurrency
 *      control of main-memory indexes on shared-memory multiprocessor
 *      systems, International Conference on Very Large Data Bases (VLDB),
 *      181-190, 2001
 * \see M. Herlihy, Y. Lev, V. Luchangco, N. Shavit, A simple optimistic
 *      skiplist algorithm, International Colloquium on Structural
 *      Information and Communication Complexity (SIROCCO), LNCS 4474,
 *      124-138, 2007, doi:10.1007/978-3-540-72951-8_11
 * \see K. Fraser, Practical lock-freedom, PhD thesis, technical report
 *      UCAM-CL-TR-579, University of Cambridge, 2004, doi:10.48456/tr-579
 * \see H.-J. Boehm, Can seqlocks get along with programming language
 *      memory models?, Workshop on Memory Systems Performance and
 *      Correctness (MSPC), 12-20, 2012, doi:10.1145/2247684.2247688
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

#ifndef __ADS_ConcurrentBTreeMap
 #define __ADS_ConcurrentBTreeMap
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/impl/OwnedEpochs.tpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ConcurrentBTreeMap ------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered MAP shared by threads: the B+-tree with optimistic lock
/// coupling of Leis et al., its nodes merged on erasure and reclaimed by
/// epochs
template< class K , class V , class Less = std::less< K > ,
	  std::size_t B = 1024 >
class ConcurrentBTreeMap
{

 static_assert( std::is_trivially_copyable_v< K > &&
		  std::is_default_constructible_v< K > ,
		"the keys are trivially copyable and default constructible" );
 static_assert( std::is_trivially_copyable_v< V > &&
		  std::is_default_constructible_v< V > ,
		"the values are trivially copyable and default "
		"constructible" );
 static_assert( std::atomic< K >::is_always_lock_free &&
		  std::atomic< V >::is_always_lock_free ,
		"the atomic words of the keys and of the values are free of "
		"locks" );

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;    ///< the type of the keys
 using Value = V;  ///< the type of the values

 /// the bytes of a node taken by its header
 static constexpr std::size_t kHeader = 32;

 /// the pairs of a leaf, \f$ L = ( B - 32 ) / ( |K| + |V| ) \f$, between 4
 /// and 255
 static constexpr int kLeaf = int( std::max< std::size_t >(
   4 , std::min< std::size_t >( 255 , ( B > kHeader ? B - kHeader : 0 ) /
				      ( sizeof( K ) + sizeof( V ) ) ) ) );

 /// the keys of an internal node,
 /// \f$ I = ( B - 40 ) / ( |K| + |\mathit{void}\,*| ) \f$, between 4 and
 /// 255
 static constexpr int kInner = int( std::max< std::size_t >(
   4 ,
   std::min< std::size_t >( 255 , ( B > kHeader + 8 ? B - kHeader - 8 : 0 ) /
				      ( sizeof( K ) + sizeof( void * ) ) ) ) );

 /// what a handle counted of its operations, for comparisons that do not
 /// time: the restarts, the splits, the merges and the shares of nodes
 struct Counts
 {
  std::uint64_t restarts = 0;  ///< the operations started over
  std::uint64_t splits = 0;    ///< the nodes split
  std::uint64_t merges = 0;    ///< the nodes merged into a sibling
  std::uint64_t shares = 0;    ///< the pairs of siblings evened
  };

 class Handle;

 /// the empty map, a leaf alone
 explicit ConcurrentBTreeMap( Less less = Less() );

 /// no copy, since the threads share it by its address
 ConcurrentBTreeMap( const ConcurrentBTreeMap & ) = delete;
 /// no copy, since the threads share it by its address
 ConcurrentBTreeMap & operator=( const ConcurrentBTreeMap & ) = delete;

 /// the frees of the nodes, of the tree and of those retired
 /// \pre no handle is alive
 ~ConcurrentBTreeMap();

 /// a handle for one thread, on a record of the epochs of its own
 [[nodiscard]] Handle handle( void );

 /// the number of the pairs; with no operation running
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0; with no operation running
 [[nodiscard]] bool empty( void ) const;

 /// the levels of the tree, 1 for a leaf alone; with no operation running
 [[nodiscard]] int height( void ) const;

 /// the nodes retired and not yet freed; with no operation running
 [[nodiscard]] std::size_t retired( void ) const {
  return( f_epochs.pending() );
  }

 /// f( k , v ) called on each pair in the order of the keys; with no
 /// operation running
 template< class F >
 void forEach( F && f ) const;

 /// the invariants of the tree: the keys of each node sorted, each key
 /// within the range that its parent gives to the node, the leaves at one
 /// depth, the counts within the capacities, no node locked or obsolete;
 /// with no operation running
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 static constexpr std::uint64_t kObsolete = 1;  ///< unlinked
 static constexpr std::uint64_t kLocked = 2;    ///< the lock

 /// the header of a node: its link of the epochs, its version, the number
 /// of its keys and its kind
 struct Node : detail::EpochLink
 {
  std::atomic< std::uint64_t > version{ 0 };  ///< lock, obsolete, count
  std::atomic< int > count{ 0 };              ///< the keys held
  bool leaf;                                  ///< a leaf, not internal

  explicit Node( bool l );
  };

 /// a leaf: its keys and its values, in the order of the keys
 struct alignas( 64 ) Leaf : Node
 {
  std::atomic< K > key[ kLeaf ];    ///< the keys
  std::atomic< V > value[ kLeaf ];  ///< their values

  Leaf( void ) : Node( true ) { }
  };

 /// an internal node: its keys and its children, the child \f$ c[ i ] \f$
 /// with the keys in \f$ ( k[ i - 1 ] , k[ i ] ] \f$
 struct alignas( 64 ) Inner : Node
 {
  std::atomic< K > key[ kInner ];             ///< the keys
  std::atomic< Node * > child[ kInner + 1 ];  ///< the children

  Inner( void ) : Node( false ) { }
  };

 /// the free of the node p, by its kind
 static void destroy( detail::EpochLink * p ) noexcept;

 /// the free of the subtree of n
 static void destroyTree( Node * n ) noexcept;

 /// the value of x, read with acquire
 template< class T >
 [[nodiscard]] static T ld( const std::atomic< T > & x ) {
  return( x.load( std::memory_order_acquire ) );
  }

 /// y written to x with release
 template< class T >
 static void st( std::atomic< T > & x , T y ) {
  x.store( y , std::memory_order_release );
  }

 /// the number of the keys of n, at most its capacity
 [[nodiscard]] static int count( const Node * n );

 /// the number of the keys of a[ 0 .. n - 1 ] smaller than k, or not
 /// greater than k if Strict, by a binary search without branches
 template< bool Strict >
 [[nodiscard]] int place( const std::atomic< K > * a , int n ,
			  const K & k ) const;

 /// the version of n, read once its lock is free, and true; false if n
 /// is obsolete
 [[nodiscard]] static bool stable( const Node * n , std::uint64_t & v );

 /// whether the version of n is still v
 [[nodiscard]] static bool check( const Node * n , std::uint64_t v );

 /// whether the lock of n was taken from its version v
 [[nodiscard]] static bool lock( Node * n , std::uint64_t v );

 /// the lock of n let go, its version raised
 static void unlock( Node * n );

 /// the lock of n let go, n marked obsolete
 static void unlockObsolete( Node * n );

 /// the subtree of n checked: its keys within ( lo , hi ], the leaves at
 /// the depth d; the depth of its leaves in depth, -1 if broken
 [[nodiscard]] bool consistent( const Node * n , const K * lo , const K * hi ,
				int d , int & depth ) const;

 Less f_less;  ///< the order of the keys
 alignas( 64 ) std::atomic< Node * > f_root;
 ///< the root
 mutable detail::OwnedEpochs f_epochs;  ///< the epochs and their records

 };  // end( class ConcurrentBTreeMap )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS ConcurrentBTreeMap::Handle -------------------*/
/*--------------------------------------------------------------------------*/

/// the access of one thread to a `ConcurrentBTreeMap`: its record of the
/// epochs, a spare node of each kind for its splits, and its counts
template< class K , class V , class Less , std::size_t B >
class ConcurrentBTreeMap< K , V , Less , B >::Handle
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

 /// the record given back to the map, the spare nodes freed
 ~Handle();

 /// inserts the pair \f$ ( k , x ) \f$ if \f$ k \f$ is not in the map, and
 /// tells whether it did; linearizable at the upgrade of its leaf
 ///
 /// In pseudocode (Leis et al., beyond the book), \f$ \mathrm{stable}( n )
 /// \f$ the version of \f$ n \f$ read once its lock is free (a restart if
 /// \f$ n \f$ is obsolete), \f$ \mathrm{check}( n , v ) \f$ a restart if
 /// the version of \f$ n \f$ is no longer \f$ v \f$, \f$ \mathrm{lock}( n ,
 /// v ) \f$ the CAS of the version of \f$ n \f$ from \f$ v \f$ to \f$ v \f$
 /// with the lock, a restart if it fails (after the locks held are let
 /// go), and \f$ |n| \f$ the keys of \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( k , x ) \cr
 /// 2 & \quad \textbf{loop}\quad \triangleright\ \text{a restart comes
 ///     back here} \cr
 /// 3 & \qquad n \gets \mathit{root};\ v \gets \mathrm{stable}( n );
 ///     \ \textbf{if}\ n \ne \mathit{root}\ \textbf{then restart};
 ///     \ p \gets \mathrm{nil} \cr
 /// 4 & \qquad \textbf{while}\ n \text{ is internal}\ \textbf{do} \cr
 /// 5 & \qquad\quad \textbf{if}\ |n| = I\ \textbf{then}\ \mathrm{split}( p
 ///     , n );\ \textbf{restart} \cr
 /// 6 & \qquad\quad i \gets | \{ j : n.k[ j ] < k \} |;\ c \gets n.c[ i ];
 ///     \ \mathrm{check}( n , v ) \cr
 /// 7 & \qquad\quad w \gets \mathrm{stable}( c );\ \mathrm{check}( n , v );
 ///     \ ( p , v_p ) \gets ( n , v );\ ( n , v ) \gets ( c , w ) \cr
 /// 8 & \qquad \textbf{if}\ k \in n\ \textbf{then}\ \mathrm{check}( n , v
 ///     );\ \textbf{return}\ \mathit{false} \cr
 /// 9 & \qquad \textbf{if}\ |n| = L\ \textbf{then}\ \mathrm{split}( p , n
 ///     );\ \textbf{restart} \cr
 /// 10 & \qquad \mathrm{lock}( n , v )\quad \triangleright\ \text{the
 ///     linearization point} \cr
 /// 11 & \qquad \text{put } ( k , x ) \text{ at its place in } n;
 ///     \ \mathrm{unlock}( n );\ \textbf{return}\ \mathit{true} \cr
 /// 12 & \textbf{procedure}\ \mathrm{split}( p , n ) \cr
 /// 13 & \quad \textbf{if}\ p \ne \mathrm{nil}\ \textbf{then}
 ///     \ \mathrm{lock}( p , v_p ) \cr
 /// 14 & \quad \mathrm{lock}( n , v );\ \textbf{if}\ p = \mathrm{nil}
 ///     \ \textbf{and}\ n \ne \mathit{root}\ \textbf{then restart} \cr
 /// 15 & \quad r \gets \text{a new node with the upper half of } n;
 ///     \ s \gets \text{the greatest key left in } n \cr
 /// 16 & \quad \textbf{if}\ p = \mathrm{nil}\ \textbf{then}\ \mathit{root}
 ///     \gets \text{a new internal node } ( n , s , r ) \cr
 /// 17 & \quad \textbf{else}\ \text{put } s \text{ and } r \text{ in } p
 ///     \text{ right after } n \cr
 /// 18 & \quad \mathrm{unlock}( n );\ \textbf{if}\ p \ne \mathrm{nil}
 ///     \ \textbf{then}\ \mathrm{unlock}( p )
 /// \end{array}}
 /// \f]
 /// where the line 8, an optimistic read of the leaf, keeps a key already
 /// there from splitting a full leaf, and the node \f$ r \f$ is allocated
 /// before any lock is taken.
 bool insert( K k , V x );

 /// inserts the pair \f$ ( k , x ) \f$, or gives \f$ x \f$ to the key
 /// \f$ k \f$ already in the map; true if it inserted; linearizable at the
 /// upgrade of its leaf, as `insert()` with no line 8
 bool assign( K k , V x );

 /// erases the key \f$ k \f$, and tells whether it was in the map;
 /// linearizable at the upgrade of its leaf, or at the check of the leaf
 /// that does not hold \f$ k \f$
 ///
 /// In pseudocode (Leis et al., with our merges on the way down, beyond
 /// the book), with the notation of `insert()` above, \f$ C( c ) \f$ the
 /// capacity of the node \f$ c \f$ (\f$ L \f$ or \f$ I \f$), and the keys
 /// of a pair of internal siblings counted with the key of their parent
 /// that separates them:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{erase}( k ) \cr
 /// 2 & \quad \textbf{loop}\quad \triangleright\ \text{a restart comes
 ///     back here} \cr
 /// 3 & \qquad n \gets \mathit{root};\ v \gets \mathrm{stable}( n );
 ///     \ \textbf{if}\ n \ne \mathit{root}\ \textbf{then restart} \cr
 /// 4 & \qquad \textbf{if}\ n \text{ is internal and } |n| = 0\ \textbf{then}
 ///     \ \mathrm{lock}( n , v );\ \mathit{root} \gets n.c[ 0 ];
 ///     \ \mathrm{retire}( n );\ \textbf{restart} \cr
 /// 5 & \qquad \textbf{while}\ n \text{ is internal}\ \textbf{do} \cr
 /// 6 & \qquad\quad i \gets | \{ j : n.k[ j ] < k \} |;\ c \gets n.c[ i ];
 ///     \ \mathrm{check}( n , v );\ w \gets \mathrm{stable}( c );
 ///     \ \mathrm{check}( n , v ) \cr
 /// 7 & \qquad\quad \textbf{if}\ |n| > 0\ \textbf{and}\ |c| < \lfloor C( c
 ///     ) / 4 \rfloor\ \textbf{then} \cr
 /// 8 & \qquad\qquad j \gets \min( i , |n| - 1 );\ ( a , b ) \gets ( n.c[ j
 ///     ] , n.c[ j + 1 ] );\ \mathrm{lock}( n , v ),\ \mathrm{lock}( a ),
 ///     \ \mathrm{lock}( b ) \cr
 /// 9 & \qquad\qquad \textbf{if}\ |a| + |b| \le 3 C( c ) / 4\ \textbf{then}
 ///     \ \text{move } b \text{ into } a \text{, drop } n.k[ j ] \text{ and }
 ///     n.c[ j + 1 ];\ \mathrm{retire}( b ) \cr
 /// 10 & \qquad\qquad \textbf{else}\ \text{even the keys of } a \text{ and
 ///     } b \text{, and set } n.k[ j ] \text{ between them} \cr
 /// 11 & \qquad\qquad \mathrm{unlock}( n , a , b );\ \textbf{restart} \cr
 /// 12 & \qquad\quad ( n , v ) \gets ( c , w ) \cr
 /// 13 & \qquad \textbf{if}\ k \notin n\ \textbf{then}\ \mathrm{check}( n
 ///     , v );\ \textbf{return}\ \mathit{false} \cr
 /// 14 & \qquad \mathrm{lock}( n , v )\quad \triangleright\ \text{the
 ///     linearization point} \cr
 /// 15 & \qquad \text{take } k \text{ out of } n;\ \mathrm{unlock}( n );
 ///     \ \textbf{return}\ \mathit{true}
 /// \end{array}}
 /// \f]
 /// where a node retired is unlocked as obsolete, and the version of the
 /// sibling of \f$ c \f$ is read by \f$ \mathrm{stable} \f$, before any
 /// lock is taken, and checked by its lock.
 bool erase( K k );

 /// the value of the key \f$ k \f$, nothing if \f$ k \f$ is not in the
 /// map; linearizable at the check of its leaf
 [[nodiscard]] std::optional< V > find( K k );

 /// whether the key \f$ k \f$ is in the map, as `find()`
 [[nodiscard]] bool contains( K k ) { return( find( k ).has_value() ); }

 /// the pair of the least key not smaller than \f$ k \f$, nothing if
 /// there is none; linearizable at the first of the checks of its line 10
 ///
 /// In pseudocode (beyond the book), with the notation of `insert()`
 /// above:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( k ) \cr
 /// 2 & \quad \textbf{loop}\quad \triangleright\ \text{a restart comes
 ///     back here} \cr
 /// 3 & \qquad x \gets k;\ \mathit{strict} \gets \mathit{false};\ P
 ///     \gets \emptyset \cr
 /// 4 & \qquad \textbf{loop} \cr
 /// 5 & \qquad\quad \text{descend to the leaf } l \text{ of } x \text{ as
 ///     in } \mathrm{insert} \text{ (by } | \{ j : n.k[ j ] \le x \} |
 ///     \text{ if } \mathit{strict} \text{), with its version } w \cr
 /// 6 & \qquad\quad f \gets \text{the key } n.k[ i ] \text{ of the last
 ///     node passed with } i < |n| \text{, or } \infty \cr
 /// 7 & \qquad\quad e \gets \text{the first pair of } l \text{ of key }
 ///     \ge x \text{ (} > x \text{ if } \mathit{strict} \text{)};
 ///     \ \mathrm{check}( l , w );\ P \gets P \cup \{ ( l , w ) \} \cr
 /// 8 & \qquad\quad \textbf{if}\ e \ne \mathrm{nil}\ \textbf{or}\ f =
 ///     \infty\ \textbf{then break} \cr
 /// 9 & \qquad\quad x \gets f;\ \mathit{strict} \gets \mathit{true} \cr
 /// 10 & \qquad \mathrm{check}( m , w_m ) \text{ for each } ( m , w_m )
 ///     \in P\quad \triangleright\ \text{the linearization point at the
 ///     first} \cr
 /// 11 & \qquad \textbf{return}\ e
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound( K k );

 /// the pair of the least key greater than \f$ k \f$, nothing if there is
 /// none; as `lowerBound()`, starting strict
 [[nodiscard]] std::optional< std::pair< K , V > > upperBound( K k );

 /// f( k , v ) called on the pairs of key not smaller than `from`, in the
 /// order of the keys, until f returns false (when it returns a bool) or
 /// the pairs end; the number of the calls. Each leaf is read as one state
 /// of it, before f is called on its pairs, so that f may call the handle
 template< class F >
 std::size_t scan( K from , F && f );

 /// what the handle counted so far
 [[nodiscard]] const Counts & counts( void ) const { return( f_c ); }

/*--------------------------------------------------------------------------*/
 private:
 friend class ConcurrentBTreeMap;

 /// the handle of m on the record r
 Handle( ConcurrentBTreeMap & m , detail::OwnedEpochs::Record * r )
     : f_m( &m ) , f_r( r ) { }

 /// the span of an operation: the epoch announced, then left
 struct Guard
 {
  Handle & h;  ///< the handle

  explicit Guard( Handle & x ) : h( x ) { h.f_m->f_epochs.enter( *h.f_r ); }
  ~Guard() { detail::OwnedEpochs::leave( *h.f_r ); }
  Guard( const Guard & ) = delete;
  Guard & operator=( const Guard & ) = delete;
  };

 /// the insertion or, if Assign, the assignment of ( k , x )
 template< bool Assign >
 bool put( const K & k , const V & x );

 /// the full node n, of version v, split under its lock and that of its
 /// parent p, of version vp, n being the child i of p; false if a lock
 /// failed or n is no longer the root (p nil)
 bool split( Inner * p , std::uint64_t vp , int i , Node * n ,
	     std::uint64_t v );

 /// the children j and j + 1 of n merged or evened, under the locks of
 /// the three, then let go
 void rebalance( Inner * n , int j , Node * a , Node * b );

 /// the leaf whose range holds x (keys >= x, or > x if Strict), its
 /// version, and the upper bound of its range in f if any (bounded);
 /// false for a restart
 template< bool Strict >
 bool leafOf( const K & x , Leaf *& l , std::uint64_t & w , K & f ,
	      bool & bounded );

 /// the first pair of key >= k (> k if Strict), all the leaves read
 /// checked again at the end
 template< bool Strict >
 std::optional< std::pair< K , V > > bound( const K & k );

 ConcurrentBTreeMap * f_m;           ///< the map
 detail::OwnedEpochs::Record * f_r;  ///< the record, nil once moved
 Leaf * f_leaf = nullptr;            ///< a spare leaf
 Inner * f_inner = nullptr;          ///< a spare internal node
 std::vector< std::pair< const Node * , std::uint64_t > > f_read;
 ///< the leaves read by a bound, with their versions
 Counts f_c;  ///< the counts

 };  // end( class ConcurrentBTreeMap::Handle )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/ConcurrentBTreeMap.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ConcurrentBTreeMap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ConcurrentBTreeMap.h -----------------------*/
/*--------------------------------------------------------------------------*/
