/*--------------------------------------------------------------------------*/
/*--------------------------- File SequenceHeap.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sequence heap of Sanders, beyond the book: a priority queue of plain
 * items for many items, which works on sorted sequences merged in large
 * batches instead of on a tree walked from the root at each operation, so
 * that its memory is read and written in order and the caches see few
 * misses where a heap [see DaryHeap.h] misses at each level of a large
 * tree.
 *
 * The new items go to an insertion heap of at most m items, small enough
 * to stay in the first level of the cache; the best items wait in a
 * deletion buffer, sorted. When the insertion heap is full, its items are
 * sorted into a sequence that joins the group 1. The group i holds up to k
 * sorted sequences, of at most \f$ m k^{i-1} \f$ items each, and a buffer
 * of its best items, refilled by merging its sequences; when a group is
 * full, its sequences are merged into one that joins the group i + 1
 * together with the buffer of that group. The deletion buffer is refilled
 * by merging the buffers of the groups. The invariants are that the
 * deletion buffer holds items not worse than any other item but those of
 * the insertion heap, and that the buffer of a group holds items not worse
 * than those of the sequences of its group; to keep them, a new sequence
 * from the insertion heap is first merged with the deletion buffer and
 * with the buffer of the group 1, which keep the best of the three. The
 * best item is the better of the top of the insertion heap and of the
 * first item of the deletion buffer.
 *
 * With R groups, \f$ R = \lceil \log_k ( n / m ) \rceil \f$, each item is
 * merged once per group it goes through, and the merges of k sequences
 * cost \f$ O( \log k ) \f$ per item, so that push() and pop() take
 * \f$ O( \log m + \log k \cdot R ) = O( \log n ) \f$ amortized
 * comparisons, as a binary heap, but with \f$ O( ( 1 / B ) \log_{M / B}
 * ( n / M ) ) \f$ amortized misses per item in a cache of M items in
 * blocks of B, against the \f$ O( \log ( n / M ) ) \f$ of a heap (Sanders,
 * theorem 1, with m and k chosen as \f$ \Theta( M ) \f$ and
 * \f$ \Theta( M / B ) \f$). Here m = 256 and k = 64. The merges are
 * done by trees of losers [see LoserTree.h] with the first keys of the
 * sequences at hand, and their matches decided by conditional moves; a
 * new sequence whose best item is not better than the deletion buffer and
 * the buffer of the group 1 joins the group with no merge. The price of
 * the batches is paid on the pushes, which sort and merge every item at
 * least twice, and the gain on the pops, which read the deletion buffer in
 * order: the heap pays against DaryHeap when the items are many more than
 * the last level of the cache holds, or when they are pushed in bulk and
 * then popped.
 *
 * There are no handles and no decrease-key, as in DaryHeap: an item whose
 * key improves is pushed again and the stale copy skipped.
 *
 * The contracts [see Contract.h]: top() and pop() need a heap that is not
 * empty; the invariant, checked on heaps of at most 4096 items, is that
 * the sequences and the buffers are sorted, that the deletion buffer is
 * not worse than the groups and the buffer of a group not worse than its
 * sequences, and that the parts count size() items. The operations are
 * defined in SequenceHeap.tpp.
 *
 * \see P. Sanders, Fast priority queues for cached memory, ACM Journal of
 *      Experimental Algorithmics 5, article 7, 2000,
 *      doi:10.1145/351827.384249
 * \see A. LaMarca, R. E. Ladner, The influence of caches on the
 *      performance of heaps, ACM Journal of Experimental Algorithmics 1,
 *      article 4, 1996, doi:10.1145/235141.235145
 * \see P. Sanders, K. Mehlhorn, M. Dietzfelbinger, R. Dementiev,
 *      Sequential and Parallel Algorithms and Data Structures: The Basic
 *      Toolbox, Springer, 2019, chapter 6, doi:10.1007/978-3-030-25209-0
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

#ifndef __ADS_SequenceHeap
 #define __ADS_SequenceHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

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

/// the sequence heap of plain items, the best one by better() on top
template< class T , class Better = std::less< T > >
class SequenceHeap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the items of the insertion heap and of a buffer
 static constexpr std::size_t M = 256;

 /// the sequences of a group
 static constexpr std::size_t K = 64;

 explicit SequenceHeap( Better better = Better() )
  : f_better( std::move( better ) ) {}

 /// pushes x, O( log n ) amortized
 void push( const T & x );

 /// the best item
 /// \pre the heap is not empty
 [[nodiscard]] const T & top( void );

 /// removes the best item and returns it, O( log n ) amortized
 /// \pre the heap is not empty
 T pop( void );

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 void clear( void );

/*--------------------------------------------------------------------------*/
 private:

 /// a sorted run of items, read from f_at on
 struct Run {
  std::vector< T > a;
  std::size_t at = 0;

  [[nodiscard]] bool done( void ) const { return( at == a.size() ); }
  [[nodiscard]] std::size_t left( void ) const { return( a.size() - at ); }
  };

 /// a group: its sequences and its buffer
 struct Group {
  std::vector< Run > seqs;
  Run buf;
  };

 /// the insertion heap made a sequence, merged with the deletion buffer
 /// and the buffer of the group 1, which keep the best items
 void flush( void );

 /// s joins the group i, which, if full, first moves its sequences to the
 /// group i + 1
 void join( std::size_t i , Run && s );

 /// the buffer of the group g refilled from its sequences, up to M items
 void refill( Group & g );

 /// the deletion buffer refilled from the buffers of the groups
 void refillDeletion( void );

 /// the remaining items of the runs R merged into out, up to n of them
 /// (all if n is 0); the runs keep what is not taken
 void merge( std::vector< Run * > & R , std::vector< T > & out ,
             std::size_t n );

 /// the order of a heap of the insertion heap: a below b if worse
 [[nodiscard]] bool worse( const T & a , const T & b ) const {
  return( f_better( b , a ) );
  }

 /// the invariants, on at most 4096 items
 [[nodiscard]] bool ordered( void ) const;

 std::vector< T > f_ins;          ///< the insertion heap
 Run f_del;                       ///< the deletion buffer
 std::vector< Group > f_groups;   ///< the groups
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
