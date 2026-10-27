/*--------------------------------------------------------------------------*/
/*--------------------------- File SequenceHeap.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sequence heap of Sanders, beyond the book: a priority queue of plain
 * items for many items, which works on sorted sequences merged in large
 * batches instead of on a tree walked from the root at each operation, so that
 * its memory is read and written in order and the caches see few misses where
 * a heap [see `DaryHeap.h`] misses at each level of a large tree.
 *
 * The new items go to an insertion heap of at most \f$ m \f$ items, small
 * enough to stay in the first level of the cache; the best items wait in a
 * deletion buffer, sorted. When the insertion heap is full, its items are
 * sorted into a sequence that joins the group 1. The group \f$ i \f$ holds up
 * to \f$ k \f$ sorted sequences, of at most \f$ m k^{i-1} \f$ items each, and
 * a buffer of its best items, refilled by merging its sequences; when a group
 * is full, its sequences are merged into one that joins the group
 * \f$ i + 1 \f$ together with the buffer of that group. The deletion buffer is
 * refilled by merging the buffers of the groups. The invariants are that the
 * deletion buffer holds items not worse than any other item but those of the
 * insertion heap, and that the buffer of a group holds items not worse than
 * those of the sequences of its group; to keep them, a new sequence from the
 * insertion heap is first merged with the deletion buffer and with the buffer
 * of the group 1, which keep the best of the three. The best item is the
 * better of the top of the insertion heap and of the first item of the
 * deletion buffer.
 *
 * With \f$ R \f$ groups, \f$ R = \lceil \log_k ( n / m ) \rceil \f$, each item
 * is merged once per group it goes through, and the merges of \f$ k \f$
 * sequences cost \f$ O( \log k ) \f$ per item, so that `push()` and `pop()`
 * take \f$ O( \log m + \log k \cdot R ) = O( \log n ) \f$ amortized
 * comparisons, as a binary heap, but with
 * \f$ O( ( 1 / B ) \log_{M / B} ( n / M ) ) \f$ amortized misses per item in a
 * cache of \f$ M \f$ items in blocks of \f$ B \f$, against the
 * \f$ O( \log ( n / M ) ) \f$ of a heap (Sanders, theorem 1, with \f$ m \f$
 * and \f$ k \f$ chosen as \f$ \Theta( M ) \f$ and \f$ \Theta( M / B ) \f$).
 * Here \f$ m = 4096 \f$, so that the insertion heap of the pairs of two words
 * fills the first level of the cache and its sequences the second, and
 * \f$ k = 16 \f$. The merges are done by trees of losers [see `LoserTree.h`]
 * with the first keys of the sequences at hand and their matches decided by
 * conditional moves; the merge of a new sequence with the two buffers is a
 * merge of two (the deletion buffer and the buffer of the group 1 make one
 * sorted sequence), and a new sequence whose best item is not better than both
 * joins the group with no merge. The numbers and the pairs of numbers ordered
 * by `std::less` or `std::greater` are compared in the merges by their keys
 * [see `OrderKey.h`], one comparison of words with no jump, and their
 * sequences sorted by the radix sort [see `RadixSort.h`]; the insertion heap
 * is 4-ary, as `DaryHeap`, and compares by `better()`, whose jumps the
 * processor predicts ahead where the conditional moves of the keys would chain
 * the loads of the walk down.
 *
 * The pushes into an empty heap, before any `top()` or `pop()`, go to a bulk,
 * an array in the order of the pushes: an insertion is a store. The first look
 * at a large bulk finds its best item by a scan of the array, which costs one
 * read per item and serves a `top()` or a `pop()` after a load; at the next
 * one the bulk becomes an incremental quicksort, which splits the array, by
 * the median of three, into the better items, the pivot and the others, keeps
 * the places of the pivots on a stack and splits again only the leftmost part,
 * until it holds at most \f$ m \f$ items, which are sorted into the buffer of
 * the bulk, a further source of the deletion buffer besides the groups:
 * \f$ O( n + k \log k ) \f$ expected time for the \f$ k \f$ best items of
 * \f$ n \f$, and \f$ O( \log n ) \f$ amortized per extraction (Paredes and
 * Navarro), with the passes over the array in order and a split with no jump,
 * a swap at every step. The items equal to a pivot that has no better one are
 * split off with it, so that many equal keys cost no quadratic time. A small
 * bulk goes to the insertion heap, which is made a heap only when a `top()` or
 * a `pop()` needs it.
 *
 * There are no handles and no decrease-key, as in `DaryHeap`: an item whose
 * key improves is pushed again and the stale copy skipped.
 *
 * The contracts [see `Contract.h`]: `top()` and `pop()` need a heap that is
 * not empty; the invariant, checked on heaps of at most 4096 items, is that
 * the sequences and the buffers are sorted, that the deletion buffer is not
 * worse than the groups and the bulk, the buffer of a group not worse than its
 * sequences and that of the bulk not worse than the bulk, and that the parts
 * count `size()` items. The operations are defined in `SequenceHeap.tpp`.
 *
 * \see P. Sanders, Fast priority queues for cached memory, ACM Journal of
 *      Experimental Algorithmics 5, article 7, 2000,
 *      doi:10.1145/351827.384249
 * \see R. Paredes, G. Navarro, Optimal incremental sorting, Proceedings of
 *      the 8th Workshop on Algorithm Engineering and Experiments (ALENEX),
 *      171-182, 2006, doi:10.1137/1.9781611972863.16
 * \see G. Navarro, R. Paredes, On sorting, heaps, and minimum spanning
 *      trees, Algorithmica 57(4), 585-620, 2010,
 *      doi:10.1007/s00453-010-9400-6
 * \see A. LaMarca, R. E. Ladner, The influence of caches on the
 *      performance of heaps, ACM Journal of Experimental Algorithmics 1,
 *      article 4, 1996, doi:10.1145/235141.235145
 * \see P. Sanders, K. Mehlhorn, M. Dietzfelbinger, R. Dementiev,
 *      Sequential and Parallel Algorithms and Data Structures: The Basic
 *      Toolbox, Springer, 2019, chapter 6, doi:10.1007/978-3-030-25209-0
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

#ifndef __ADS_SequenceHeap
 #define __ADS_SequenceHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/priority/OrderKey.h"
#include "ADS/sort/Sort.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SequenceHeap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the sequence heap of plain items, the best one by `better()` on top
template< class T , class Better = std::less< T > >
class SequenceHeap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the items of the insertion heap and of a buffer, \f$ m \f$
 static constexpr std::size_t M = 4096;

 /// the sequences of a group, \f$ k \f$
 static constexpr std::size_t K = 16;

 /// the empty heap of the order `better`
 explicit SequenceHeap( Better better = Better() )
  : f_better( std::move( better ) ) {}

 /// pushes \f$ x \f$, in \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (Sanders, beyond the book), \f$ I \f$ the insertion
 /// heap, \f$ D \f$ the deletion buffer, \f$ G_i \f$ the group \f$ i \f$
 /// with its buffer \f$ G_i.\mathit{buf} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{then}\ \text{open a bulk} \cr
 /// 3 & \quad n \gets n + 1 \cr
 /// 4 & \quad \textbf{if}\  \text{the bulk is open }\ \textbf{then}
 ///     \ \text{append } x \text{ to it};\ \textbf{return} \cr
 /// 5 & \quad \textbf{if}\ |I| = m\ \textbf{then}\ \mathrm{flush}() \cr
 /// 6 & \quad \text{insert } x \text{ in } I \text{, moved up if } I
 ///     \text{ is a heap} \cr
 /// 7 & \textbf{procedure}\ \mathrm{flush}() \cr
 /// 8 & \quad S \gets \text{the items of } I \text{, sorted};\ I \gets
 ///     \emptyset \cr
 /// 9 & \quad \textbf{if}\  \text{the first of } S \text{ is not better }
 ///     \text{than the last of } D \text{ and of } G_1.\mathit{buf}
 ///     \ \textbf{then} \cr
 /// 10 & \qquad \mathrm{join}( 1 , S );\ \textbf{return} \cr
 /// 11 & \quad s \gets \text{the merge of } S \text{ with } D \text{ and }
 ///     G_1.\mathit{buf} \cr
 /// 12 & \quad D \gets \text{the first } |D| \text{ items of } s;
 ///     \ G_1.\mathit{buf} \gets \text{the next } |G_1.\mathit{buf}| \cr
 /// 13 & \quad \mathrm{join}( 1 , \text{the rest of } s ) \cr
 /// 14 & \textbf{procedure}\ \mathrm{join}( i , s ) \cr
 /// 15 & \quad \textbf{if}\ G_i \text{ has } k \text{ sequences}
 ///     \ \textbf{then} \cr
 /// 16 & \qquad \textbf{if}\  \text{they hold at most half of their room }
 ///     \textbf{then}\ \text{merge them into fewer full ones} \cr
 /// 17 & \qquad \textbf{else}\ \mathrm{join}( i + 1 , \text{the merge of}
 ///     \text{ the sequences of } G_i \text{ and of } G_{i+1}.\mathit{buf}
 ///     );\ G_i \gets \emptyset \cr
 /// 18 & \quad \text{add } s \text{ to the sequences of } G_i
 /// \end{array}}
 /// \f]
 void push( const T & x );

 /// the best item
 /// \pre the heap is not empty
 [[nodiscard]] const T & top( void );

 /// removes the best item and returns it, in \f$ O( \log n ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Sanders, beyond the book; the incremental quicksort of
 /// Paredes and Navarro for the bulk), with the names of `push()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad \textbf{if}\  \text{the bulk is open }\ \textbf{then} \cr
 /// 3 & \qquad \textbf{if}\  \text{it is the first look at more than } m
 ///     \text{ items }\ \textbf{then}\ \text{put its best item last by a}
 ///     \text{ scan} \cr
 /// 4 & \qquad \textbf{else if}\  \text{its best item is not last }
 ///     \ \textbf{then}\ \text{close it: to } I \text{ if at most } m
 ///     \text{, else to the quicksort} \cr
 /// 5 & \qquad \textbf{if}\  \text{the bulk is still open }
 ///     \ \textbf{then}\ n \gets n - 1;\ \textbf{return}\ \text{its last}
 ///     \text{ item, removed} \cr
 /// 6 & \quad \textbf{if}\ I \text{ is not a heap}\ \textbf{then}
 ///     \ \text{make it one from the bottom up} \cr
 /// 7 & \quad \textbf{if}\ D = \emptyset\ \textbf{then}
 ///     \ \mathrm{refillDeletion}() \cr
 /// 8 & \quad n \gets n - 1 \cr
 /// 9 & \quad \textbf{if}\ D = \emptyset\ \textbf{or}\ \mathrm{better}(
 ///     \mathrm{top}( I ) , \text{the first of } D )\ \textbf{then}
 ///     \ \textbf{return}\ \mathrm{deleteMin}( I ) \cr
 /// 10 & \quad \textbf{return}\ \text{the first of } D \text{, removed} \cr
 /// 11 & \textbf{procedure}\ \mathrm{refillDeletion}() \cr
 /// 12 & \quad \textbf{foreach}\ i\ \textbf{do}\ \textbf{if}
 ///     \ G_i.\mathit{buf} = \emptyset\ \textbf{then}\ \text{refill it}
 ///     \text{ with the first } m \text{ items of the merge of } G_i \cr
 /// 13 & \quad \text{refill the buffer of the bulk, if empty, with its next}
 ///     \text{ best items by the quicksort} \cr
 /// 14 & \quad \textbf{while}\ |D| < m\ \textbf{and}\  \text{some buffer}
 ///     \text{ has items }\ \textbf{do} \cr
 /// 15 & \qquad \text{move to } D \text{ the best first item of the}
 ///     \text{ buffers, by a heap of them} \cr
 /// 16 & \qquad \text{refill that buffer from its source if it ran out}
 /// \end{array}}
 /// \f]
 /// \pre the heap is not empty
 T pop( void );

 /// whether the heap has no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// removes all the items
 void clear( void );

/*--------------------------------------------------------------------------*/
 private:

 /// a sorted run of items, read from `at` on
 struct Run {
  std::vector< T > a;     ///< the items
  std::size_t at = 0;     ///< the first item not yet read

  /// whether all the items have been read
  [[nodiscard]] bool done( void ) const { return( at == a.size() ); }

  /// the number of items not yet read
  [[nodiscard]] std::size_t left( void ) const { return( a.size() - at ); }
  };

 /// a group: its sequences and its buffer
 struct Group {
  std::vector< Run > seqs;   ///< the sequences
  Run buf;                   ///< the buffer of its best items
  };

 /// the insertion heap made a sequence, merged with the deletion buffer
 /// and the buffer of the group 1, which keep the best items
 void flush( void );

 /// s joins the group i, which, if full, first moves its sequences to the
 /// group i + 1
 void join( std::size_t i , Run && s );

 /// the buffer of the group g refilled from its sequences, up to M items
 void refill( Group & g );

 /// the deletion buffer refilled from the buffers of the groups and of
 /// the bulk
 void refillDeletion( void );

 /// the arity of the insertion heap
 static constexpr std::size_t D = 4;

 /// the item at i of the insertion heap moved up while better than its
 /// parent
 void up( std::size_t i );

 /// the hole at i of the insertion heap moved down until x fits in it
 void down( std::size_t i , T x );

 /// the bulk closed, and the insertion heap made, before a top() or a
 /// pop(), and the deletion buffer refilled if empty
 void ready( void );

 /// the bulk closed: a few items to the insertion heap, many to the
 /// incremental quicksort
 void close( void );

 /// the buffer of the bulk refilled with its next best items, up to M
 void refillBulk( void );

 /// a[ 0 .. b - a ) sorted
 void sortRange( T * a , T * b );

 /// a[ 0 .. b - a ) split into the items better than pv, first, and the
 /// others; the first of the others
 T * partition( T * a , T * b , const T & pv );

 /// the remaining items of the runs R merged into out, up to n of them
 /// (all if n is 0); the runs keep what is not taken
 void merge( std::vector< Run * > & R , std::vector< T > & out ,
             std::size_t n );

 /// the sorted a[ 0 .. na ) and b[ 0 .. nb ) merged at the end of out, a
 /// first on equal items
 void merge2( const T * a , std::size_t na , const T * b , std::size_t nb ,
              std::vector< T > & out ) const;

 /// a is better than b: by their keys, for the numbers and the pairs of
 /// numbers in the order of std::less or std::greater, or by better()
 [[nodiscard]] bool bt( const T & a , const T & b ) const {
  if constexpr( detail::kKeyed< T , Better > )
   return( detail::orderKeyBy< Better >( a ) <
           detail::orderKeyBy< Better >( b ) );
  else
   return( f_better( a , b ) );
  }

 /// the order of a heap of the insertion heap: a below b if worse
 [[nodiscard]] bool worse( const T & a , const T & b ) const {
  return( bt( b , a ) );
  }

 /// the invariants, on at most 4096 items
 [[nodiscard]] bool ordered( void ) const;

 std::vector< T > f_ins;          ///< the insertion heap
 Run f_del;                       ///< the deletion buffer
 std::vector< Group > f_groups;   ///< the groups
 std::vector< T > f_scratch;      ///< room for the merges
 bool f_heaped = true;            ///< the insertion buffer is a heap
 bool f_open = false;             ///< the pushes go to the bulk
 int f_scans = 0;                 ///< the scans of the bulk while open
 bool f_minLast = false;          ///< the best of the open bulk is last
 /// the bulk, by incremental quicksort, on huge pages when large
 std::vector< T , HugePageAllocator< T > > f_bulk;
 std::size_t f_lo = 0;            ///< the items of the bulk taken
 /// the blocks [ q , r ) of the bulk in their place, equal and not better
 /// than A[ r .. ), the next one on top
 std::vector< std::pair< std::size_t , std::size_t > > f_piv;
 Run f_bbuf;                      ///< the best items of the bulk
 std::vector< T > f_key;          ///< the first keys of the runs merged
 std::vector< unsigned char > f_dead;      ///< the runs done
 std::vector< std::size_t > f_lose;        ///< the tree of losers
 std::vector< std::size_t > f_lose2;       ///< and of the winners
 std::vector< T * > f_cur , f_end;         ///< the runs merged
 std::size_t f_n = 0;             ///< the number of items
 Better f_better;                 ///< the order

 };  // end( class SequenceHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/SequenceHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceHeap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SequenceHeap.h ------------------------*/
/*--------------------------------------------------------------------------*/
