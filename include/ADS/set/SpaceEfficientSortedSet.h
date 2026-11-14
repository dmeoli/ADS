/*--------------------------------------------------------------------------*/
/*--------------------- File SpaceEfficientSortedSet.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The space-efficient sorted set of Morin (his `SESSet`, exercise 4.13),
 * beyond the book: a SET [see `ADT.h`] of ordered items and an SSet of Morin,
 * with `find()` and `lowerBound()`, that stores \f$ n \f$ items in the space
 * of \f$ n \f$ items plus \f$ O( n / b + b ) \f$ words, \f$ b \f$ a parameter.
 *
 * The items are kept in order in a list of blocks, as in the `SEList` of Morin
 * (section 3.3): each block is a circular buffer of \f$ b + 1 \f$ places, and
 * every block but the last holds from \f$ b - 1 \f$ to \f$ b + 1 \f$ items, so
 * that at most 2 places per block and the \f$ b + 1 \f$ of the last one are
 * empty. The blocks are indexed by a `SkipList` [see `SkipList.h`] ordered by
 * their largest items: since the blocks partition the sorted items into
 * intervals, their order never changes when items move between neighbors, so
 * that the skip list sees only the creation and the removal of a block, and a
 * search compares the item sought with the largest item of a block. A search
 * finds the first block whose largest item is not smaller than \f$ x \f$ and
 * then \f$ x \f$ in it by binary search, \f$ O( \log n ) \f$ expected time.
 *
 * An insertion into a full block looks for a block that is not full among the
 * next \f$ b \f$ ones and moves one item from the end of each block to the
 * front of the next one up to it, \f$ O( 1 ) \f$ per block in a circular
 * buffer; if the \f$ b \f$ blocks are full a new block follows them and the
 * \f$ b ( b + 1 ) \f$ items are spread over the \f$ b + 1 \f$ blocks,
 * \f$ b \f$ each. A removal that leaves a block with \f$ b - 2 \f$ items
 * borrows one item from the next ones in the same way, or, if the next
 * \f$ b - 1 \f$ blocks hold \f$ b - 1 \f$ items each, gathers the items of the
 * \f$ b \f$ blocks into \f$ b - 1 \f$ of them and frees the last. A spread or
 * a gather costs \f$ O( b^2 ) \f$ and leaves the \f$ b \f$ blocks
 * \f$ \Omega( b ) \f$ operations away from the next one, hence an insertion or
 * a removal costs \f$ O( \log n + b ) \f$ amortized time, the argument of
 * Lemma 3.1 of Morin.
 *
 * Morin gives no code for it; what the benchmarks are to check is the
 * space, \f$ n + O( n / b + b ) \f$ items against the \f$ 3 n \f$ words of
 * links and the \f$ n \f$ allocations of `SkipList`, and the time of a
 * search, one search of the skip list on \f$ n / b \f$ blocks plus a
 * binary search in a block of contiguous items, against a search of the
 * skip list on \f$ n \f$ nodes that misses the cache at each step.
 *
 * The contracts [see `Contract.h`]: \f$ b \ge 3 \f$; after `insert()` the item
 * is there and after `remove()` it is not; the invariant, an \f$ O( n ) \f$
 * check at the level 3, is that the items increase along the blocks and that
 * each block but the last holds from \f$ b - 1 \f$ to \f$ b + 1 \f$ items. The
 * operations are defined in `SpaceEfficientSortedSet.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 3.3 and exercise 4.13,
 *      doi:10.15215/aupress/9781927356388.01
 * \see W. Pugh, Skip lists: a probabilistic alternative to balanced trees,
 *      Communications of the ACM 33(6), 668-676, 1990,
 *      doi:10.1145/78973.78977
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

#ifndef __ADS_SpaceEfficientSortedSet
 #define __ADS_SpaceEfficientSortedSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/SkipList.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS SpaceEfficientSortedSet ----------------------*/
/*--------------------------------------------------------------------------*/

/// a SET of ordered items in blocks of about \f$ b \f$ items, indexed by a
/// skip list, the `SESSet` of Morin
template< std::totally_ordered T >
 requires std::semiregular< T >
class SpaceEfficientSortedSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;    ///< the type of the items

 /// the empty set, with blocks of \f$ b + 1 \f$ places
 /// \pre \f$ b \ge 3 \f$
 explicit SpaceEfficientSortedSet( std::size_t b = 32 ,
                                   std::uint64_t seed = 1 );

 /// no copy, since the blocks are linked by pointers
 SpaceEfficientSortedSet( const SpaceEfficientSortedSet & ) = delete;

 /// no copy, since the blocks are linked by pointers
 SpaceEfficientSortedSet & operator=( const SpaceEfficientSortedSet & ) =
  delete;

 /// frees the blocks
 ~SpaceEfficientSortedSet();

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_size == 0 ); }

 /// the number of places of the blocks, \f$ n + O( n / b + b ) \f$
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_blocks * ( f_b + 1 ) );
  }

 /// true if \f$ x \f$ is an item, by `lowerBound()`
 [[nodiscard]] bool contains( const T & x ) const;

 /// the address of the item equal to \f$ x \f$, nil if there is none (the
 /// `find(x)` of a USet), valid until the next change, by `lowerBound()`
 [[nodiscard]] const T * find( const T & x ) const;

 /// the address of the smallest item not smaller than \f$ x \f$, nil if
 /// there is none (the `find(x)` of an SSet), valid until the next change,
 /// in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (exercise 4.13 of Morin, beyond the book), \f$ S \f$ the
 /// skip list of the blocks by their largest items:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( x ) \cr
 /// 2 & \quad B \gets \text{the first block of } S \text{ whose largest }
 ///     \text{item is} \ge x \cr
 /// 3 & \quad \textbf{if}\ B = \textbf{nil}\ \textbf{then return nil} \cr
 /// 4 & \quad \textbf{return}\ \text{the first item of } B \text{ not }
 ///     \text{smaller than } x \text{, by binary search}
 /// \end{array}}
 /// \f]
 [[nodiscard]] const T * lowerBound( const T & x ) const;

 /// \f$ x \f$ added, if it is not there, in \f$ O( \log n + b ) \f$
 /// amortized expected time; true if it was not there
 ///
 /// In pseudocode (exercise 4.13 of Morin, beyond the book), the blocks
 /// \f$ B_0 , B_1 , \ldots \f$ from the block of \f$ x \f$ on:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad B_0 \gets \text{the block of } \mathrm{lowerBound}( x ) \text{,
 ///     or the last one} \cr
 /// 3 & \quad \textbf{if}\ x \in B_0\ \textbf{then return} \cr
 /// 4 & \quad \textbf{if}\ |B_0| = b + 1\ \textbf{then} \cr
 /// 5 & \qquad r \gets \text{the least } r \ge 1 \text{ with } |B_r| \le b
 ///     \text{ or } B_r = \textbf{nil} \text{, if } r < b \cr
 /// 6 & \qquad \textbf{if}\ r < b\ \textbf{and}\ B_r = \textbf{nil}
 ///     \ \textbf{then}\ B_r \gets \text{a new last block} \cr
 /// 7 & \qquad \textbf{if}\ r < b\ \textbf{then}\ \textbf{for}\ j \gets r
 ///     \ \textbf{downto}\ 1\ \textbf{do}\ \text{move the last item of }
 ///     B_{j-1} \text{ to the front of } B_j \cr
 /// 8 & \qquad \textbf{else}\ B_b \gets \text{a new block};\ \textbf{for}\ j
 ///     \gets b\ \textbf{downto}\ 1\ \textbf{do} \cr
 /// & \qquad\qquad \textbf{while}\ |B_j| < b\ \textbf{do}\ \text{move the }
 ///     \text{last item of } B_{j-1} \text{ to the front of } B_j \cr
 /// 9 & \qquad \mathrm{insert}( x );\ \textbf{return} \cr
 /// 10 & \quad \text{put } x \text{ in its place in } B_0
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is there
 bool insert( const T & x );

 /// \f$ x \f$ removed, if it is there, in \f$ O( \log n + b ) \f$ amortized
 /// expected time: a block left with \f$ b - 2 \f$ items takes the first
 /// item of each next block up to the first one with more than
 /// \f$ b - 1 \f$ items, or the last one, within \f$ b \f$ blocks, and if
 /// there is none the items of the \f$ b \f$ blocks are gathered into the
 /// first \f$ b - 1 \f$, the last one freed (the reverse of `insert()`)
 /// \post \f$ x \f$ is not there, true if it was
 bool remove( const T & x );

 /// `f( x )` on the items in increasing order
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a block: a circular buffer of \f$ b + 1 \f$ places, in the list of the
 /// blocks
 struct Block {
  std::unique_ptr< T[] > a;        ///< the places
  std::size_t h = 0;               ///< the place of the first item
  std::size_t s = 0;               ///< the number of items
  Block * prev = nullptr;          ///< the previous block
  Block * next = nullptr;          ///< the next block
  };

 /// the key of a block in the skip list, or of an item sought: the
 /// largest item of the block, or the item
 struct Key {
  const SpaceEfficientSortedSet * o;   ///< the set
  const Block * b;                     ///< the block, or nil
  const T * x;                         ///< the item, if no block
  /// the item compared
  const T & item( void ) const { return( b ? o->at( b , b->s - 1 ) : *x ); }
  /// by the items
  friend bool operator==( const Key & p , const Key & q ) {
   return( p.item() == q.item() );
   }
  /// by the items
  friend bool operator<( const Key & p , const Key & q ) {
   return( p.item() < q.item() );
   }
  /// by the items
  friend bool operator>( const Key & p , const Key & q ) { return( q < p ); }
  /// by the items
  friend bool operator<=( const Key & p , const Key & q ) {
   return( !( q < p ) );
   }
  /// by the items
  friend bool operator>=( const Key & p , const Key & q ) {
   return( !( p < q ) );
   }
  };

 /// the item \f$ i \f$ of \f$ B \f$
 const T & at( const Block * B , std::size_t i ) const {
  return( B->a[ ( B->h + i ) % ( f_b + 1 ) ] );
  }
 /// the item \f$ i \f$ of \f$ B \f$
 T & at( Block * B , std::size_t i ) {
  return( B->a[ ( B->h + i ) % ( f_b + 1 ) ] );
  }

 /// the block of the first item not smaller than \f$ x \f$, nil if none
 Block * blockOf( const T & x ) const;

 /// the place in \f$ B \f$ of the first item not smaller than \f$ x \f$
 std::size_t place( const Block * B , const T & x ) const;

 /// a new empty block after \f$ B \f$ (first if nil), not yet indexed
 Block * newBlock( Block * B );

 /// \f$ B \f$ out of the index and of the list, and freed
 void freeBlock( Block * B );

 /// the last item of \f$ B \f$ moved to the front of \f$ C \f$
 void shiftForward( Block * B , Block * C );

 /// the first item of \f$ C \f$ moved to the end of \f$ B \f$
 void shiftBack( Block * B , Block * C );

 /// the items increasing and the blocks filled within their bounds
 [[nodiscard]] bool valid( void ) const;

 std::size_t f_b;                  ///< the parameter \f$ b \f$
 std::size_t f_size = 0;           ///< the number of items
 std::size_t f_blocks = 0;         ///< the number of blocks
 Block * f_first = nullptr;        ///< the first block
 Block * f_last = nullptr;         ///< the last block
 SkipList< Key , Block * > f_index;   ///< the blocks by their largest items

 };  // end( class SpaceEfficientSortedSet )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/SpaceEfficientSortedSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SpaceEfficientSortedSet.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File SpaceEfficientSortedSet.h -------------------*/
/*--------------------------------------------------------------------------*/
