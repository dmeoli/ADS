/*--------------------------------------------------------------------------*/
/*----------------------------- File SEList.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The space-efficient list of section 3.3 of Morin's book, beyond the book:
 * a doubly linked list of blocks, each a circular buffer of at most
 * \f$ b + 1 \f$ items (an unrolled list), the blocks but the first and the
 * last holding between \f$ b - 1 \f$ and \f$ b + 1 \f$ items, so that the
 * \f$ n \f$ items take \f$ n + O( b + n / b ) \f$ places and pointers where a
 * `List` [see `List.h`] takes \f$ 2 n \f$ pointers more, and a walk reads
 * \f$ b \f$ items per block. The item of rank \f$ i \f$ is reached from the
 * nearer end, block by block, in \f$ O( 1 + \min\{ i , n - i \} / b ) \f$
 * time; an insertion or a removal at the rank \f$ i \f$ takes
 * \f$ O( b + \min\{ i , n - i \} / b ) \f$ amortized time.
 *
 * An insertion into a full block looks at the next \f$ b \f$ blocks at
 * most for one with room: if one is found, the last item of each block in
 * between passes to the front of the next one, \f$ O( 1 ) \f$ per block;
 * if the walk reaches the end, a block is added there; if the \f$ b \f$
 * blocks are all full, `spread()` makes \f$ b + 1 \f$ blocks of \f$ b \f$
 * items out of them, in \f$ O( b^2 ) \f$. A removal that leaves a middle
 * block with \f$ b - 2 \f$ items is the symmetric, `gather()` making
 * \f$ b - 1 \f$ blocks out of \f$ b \f$ blocks that hold
 * \f$ b^2 - b - 1 \f$ items. A spread or a gather leaves its blocks with
 * \f$ b \f$ items, or \f$ b - 1 \f$, so that \f$ \Omega( b ) \f$ operations
 * on them come before the next one, which pays for its \f$ O( b^2 ) \f$
 * (section 3.3.5 of Morin's book).
 *
 * Unlike Morin's, the first and the last block may hold any number of items
 * from 1 to \f$ b + 1 \f$: `push_back()` adds a block when the last one is
 * full and `pop_back()` drops it when it is empty, and the same at the
 * front, so that the four operations of a double ended queue never spread
 * nor gather and take \f$ O( 1 ) \f$ time, independent of \f$ b \f$ (which
 * exercise 3.18 asks for, and exercise 3.17 for the stack); the bound on the
 * space loses the \f$ 2 b \f$ places of the two ends at most. A block is one
 * allocation, its header followed by its places, which are raw memory, and
 * one block emptied is kept for the next one needed, so that the
 * operations at a boundary of the blocks do not call the allocator each
 * time.
 *
 * Against Morin's code, the claims for the benchmarks: a block is one
 * allocation of a power of 2 places, reduced by a mask where Morin's
 * `BDeque`, an `ArrayDeque`, divides by \f$ b + 1 \f$, and holds raw places
 * where Morin's builds \f$ b + 1 \f$ items by default; the operations at the
 * two ends never spread nor gather; one block emptied is kept, so that a
 * queue that crosses the boundary of a block back and forth allocates
 * nothing.
 *
 * The contracts [see `Contract.h`]: \f$ b \ge 3 \f$, a rank is below
 * `size()` (at most `size()` for an insertion), and the removals at the ends
 * need a list that is not empty; the invariant is the bound on the items of
 * each block, none empty, and that the blocks count `size()` items, an
 * \f$ O( n / b ) \f$ check. The operations are defined in `SEList.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 3.3 and exercises
 *      3.17 and 3.18
 * \see Z. Shao, J. H. Reppy, A. W. Appel, Unrolling lists, Proceedings of
 *      the 1994 ACM Conference on LISP and Functional Programming, 185-195,
 *      1994, doi:10.1145/182409.182453
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

#ifndef __ADS_SEList
 #define __ADS_SEList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <bit>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS SEList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a list of blocks of \f$ b - 1 \f$ to \f$ b + 1 \f$ items, with
/// \f$ O( n / b ) \f$ pointers
template< class T >
class SEList {

 struct Node;

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = T &;                    ///< a reference to an item
 using const_reference = const T &;        ///< a constant reference

 /// the empty list with blocks of \f$ b + 1 \f$ items at most
 /// \pre \f$ b \ge 3 \f$
 explicit SEList( std::size_t b = 63 );

 /// the items of `l`, in order, with blocks of \f$ 64 \f$ items at most
 SEList( std::initializer_list< T > l );

 /// a copy of `o`, with its \f$ b \f$
 SEList( const SEList & o );

 /// the blocks of `o`, which is left empty
 SEList( SEList && o ) noexcept;

 /// the items of `o`, by copy and swap
 SEList & operator=( SEList o ) noexcept;

 /// destroys the items and frees the blocks
 ~SEList();

 /// exchanges the blocks with those of `o`
 void swap( SEList & o ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the parameter \f$ b \f$
 [[nodiscard]] std::size_t block( void ) const { return( f_b ); }

 /// the number of blocks
 [[nodiscard]] std::size_t blocks( void ) const { return( f_blocks ); }

 /// the item of rank \f$ i \f$, \f$ O( 1 + \min\{ i , n - i \} / b ) \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i );

 /// the item of rank \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// the first item
 /// \pre the list is not empty
 [[nodiscard]] const T & front( void ) const;

 /// the last item
 /// \pre the list is not empty
 [[nodiscard]] const T & back( void ) const;

 /// puts \f$ x \f$ at the rank \f$ i \f$, in
 /// \f$ O( b + \min\{ i , n - i \} / b ) \f$ amortized time
 ///
 /// In pseudocode (Morin, section 3.3), \f$ u \f$ the block of \f$ i \f$
 /// and \f$ j \f$ the place of \f$ i \f$ in it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad ( u , j ) \gets \mathrm{locate}( i );\ v \gets u;\ r \gets 0
 ///     \cr
 /// 3 & \quad \textbf{while}\ r < b\ \textbf{and}\ v \ne \textbf{nil}
 ///     \ \textbf{and}\ |v| = b + 1\ \textbf{do}\ v \gets v.\mathit{next};
 ///     \ r \gets r + 1 \cr
 /// 4 & \quad \textbf{if}\ r = b\ \textbf{then}\ \mathrm{spread}( u );
 ///     \ ( u , j ) \gets \mathrm{locate}( i ) \cr
 /// 5 & \quad \textbf{else} \cr
 /// 6 & \qquad \textbf{if}\ v = \textbf{nil}\ \textbf{then}\ v \gets
 ///     \text{a new block at the end} \cr
 /// 7 & \qquad \textbf{while}\ v \ne u\ \textbf{do}\ v.\mathrm{pushFront}(
 ///     v.\mathit{prev}.\mathrm{popBack}() );\ v \gets v.\mathit{prev} \cr
 /// 8 & \quad u.\mathrm{insert}( j , x );\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void insert( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$, in
 /// \f$ O( b + \min\{ i , n - i \} / b ) \f$ amortized time
 ///
 /// In pseudocode (Morin, section 3.3), a middle block left with
 /// \f$ b - 2 \f$ items borrowing from the next ones:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad ( u , j ) \gets \mathrm{locate}( i );\ u.\mathrm{erase}( j );
 ///     \ n \gets n - 1 \cr
 /// 3 & \quad \textbf{if}\ |u| = 0\ \textbf{then}\ \text{drop } u;
 ///     \ \textbf{return} \cr
 /// 4 & \quad \textbf{if}\ u \text{ is the first or the last block}
 ///     \ \textbf{or}\ |u| \ge b - 1\ \textbf{then return} \cr
 /// 5 & \quad v \gets u.\mathit{next};\ r \gets 0 \cr
 /// 6 & \quad \textbf{while}\ r < b\ \textbf{and}\ v \text{ is a middle }
 ///     \text{block}\ \textbf{and}\ |v| = b - 1\ \textbf{do}\ v \gets
 ///     v.\mathit{next};\ r \gets r + 1 \cr
 /// 7 & \quad \textbf{if}\ r = b\ \textbf{then}\ \mathrm{gather}( u );
 ///     \ \textbf{return} \cr
 /// 8 & \quad \textbf{while}\ u \ne v\ \textbf{do}\ u.\mathrm{pushBack}(
 ///     u.\mathit{next}.\mathrm{popFront}() );\ u \gets u.\mathit{next} \cr
 /// 9 & \quad \textbf{if}\ |v| = 0\ \textbf{then}\ \text{drop } v
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 /// \post one item less
 void erase( std::size_t i );

 /// \f$ x \f$ at the front, \f$ O( 1 ) \f$
 void push_front( const T & x );

 /// \f$ x \f$ at the back, \f$ O( 1 ) \f$
 void push_back( const T & x );

 /// removes the first item, \f$ O( 1 ) \f$
 /// \pre the list is not empty
 void pop_front( void );

 /// removes the last item, \f$ O( 1 ) \f$
 /// \pre the list is not empty
 void pop_back( void );

 /// removes all the items and frees the blocks
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const SEList & o ) const;

 /// the bidirectional iterator, on items const if `Const`
 template< bool Const >
 class Iterator {
  using Owner = std::conditional_t< Const , const SEList , SEList >;

  public:
  /// bidirectional
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;                     ///< the items
  using difference_type = std::ptrdiff_t;   ///< the distances
  /// a reference to an item, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an item, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// an iterator on no list
  Iterator( void ) = default;

  /// on the item \f$ j \f$ of the block \f$ u \f$ of `l`
  Iterator( Owner * l , Node * u , std::size_t j )
   : f_l( l ) , f_u( u ) , f_j( j ) {}

  /// the constant iterator from the mutable one
  template< bool K > requires ( Const && ! K )
  Iterator( const Iterator< K > & o )
   : f_l( o.f_l ) , f_u( o.f_u ) , f_j( o.f_j ) {}

  /// the item
  reference operator*( void ) const { return( f_l->at( f_u , f_j ) ); }
  /// the address of the item
  pointer operator->( void ) const { return( &**this ); }
  /// to the next item, the next block after the last one of a block
  Iterator & operator++( void ) {
   if( ++f_j == f_u->n ) { f_u = f_u->next; f_j = 0; }
   return( *this );
   }
  /// to the previous item
  Iterator & operator--( void ) {
   if( f_j == 0 ) { f_u = f_u->prev; f_j = f_u->n; }
   --f_j;
   return( *this );
   }
  /// to the next item, returning the iterator before the step
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// to the previous item, returning the iterator before the step
  Iterator operator--( int ) { auto t = *this; --*this; return( t ); }
  /// equal on the same item
  bool operator==( const Iterator & o ) const {
   return( ( f_u == o.f_u ) && ( f_j == o.f_j ) );
   }

  private:
  template< bool > friend class Iterator;
  Owner * f_l = nullptr;
  Node * f_u = nullptr;
  std::size_t f_j = 0;
  };  // end( class Iterator )

 using iterator = Iterator< false >;        ///< the iterator on the items
 using const_iterator = Iterator< true >;   ///< on the constant items

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) {
  return( iterator( this , f_dummy.next , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) {
  return( iterator( this , &f_dummy , 0 ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , f_dummy.next , 0 ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , &f_dummy , 0 ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a block: its header, followed by its places
 struct Node {
  Node * prev;        ///< the block before, the dummy before the first
  Node * next;        ///< the block after, the dummy after the last
  std::size_t head;   ///< the place of the first item
  std::size_t n;      ///< the number of items
  };

 /// the offset of the places from the header
 static constexpr std::size_t OFF = ( sizeof( Node ) + alignof( T ) - 1 ) /
                                    alignof( T ) * alignof( T );
 /// the alignment of a block
 static constexpr std::size_t ALIGN = alignof( T ) > alignof( Node ) ?
                                      alignof( T ) : alignof( Node );

 /// the places of the block \f$ u \f$
 T * places( Node * u ) const {
  return( reinterpret_cast< T * >( reinterpret_cast< unsigned char * >( u )
                                   + OFF ) );
  }

 /// the item \f$ j \f$ of the block \f$ u \f$
 T & at( Node * u , std::size_t j ) const {
  return( places( u )[ ( u->head + j ) & f_mask ] );
  }

 /// the block of rank \f$ i \f$, from the nearer end, \f$ j \f$ the place
 /// of \f$ i \f$ in it
 Node * locate( std::size_t i , std::size_t & j ) const;

 /// a middle block, neither the first nor the last
 bool middle( const Node * u ) const {
  return( ( u->prev != &f_dummy ) && ( u->next != &f_dummy ) );
  }

 /// an empty block linked before \f$ p \f$, the one kept or a new one
 Node * newBlock( Node * p );

 /// the empty block \f$ u \f$ unlinked, kept if none is, freed otherwise
 void dropBlock( Node * u );

 /// \f$ x \f$ at the front of \f$ u \f$
 void pushFront( Node * u , T && x );

 /// \f$ x \f$ at the back of \f$ u \f$
 void pushBack( Node * u , T && x );

 /// the first item of \f$ u \f$, removed
 T popFront( Node * u );

 /// the last item of \f$ u \f$, removed
 T popBack( Node * u );

 /// \f$ x \f$ at the place \f$ j \f$ of \f$ u \f$, the items of the shorter
 /// side moved
 void insert( Node * u , std::size_t j , T && x );

 /// the item \f$ j \f$ of \f$ u \f$ removed, the items of the shorter side
 /// moved
 void erase( Node * u , std::size_t j );

 /// the \f$ b \f$ full blocks from \f$ u \f$ made \f$ b + 1 \f$ blocks of
 /// \f$ b \f$ items, in \f$ O( b^2 ) \f$
 ///
 /// In pseudocode (Morin, section 3.3):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{spread}( u ) \cr
 /// 2 & \quad w \gets \text{a new block after the } b \text{ blocks from } u
 ///     \cr
 /// 3 & \quad \textbf{while}\ w \ne u\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ |w| < b\ \textbf{do}\ w.\mathrm{pushFront}(
 ///     w.\mathit{prev}.\mathrm{popBack}() ) \cr
 /// 5 & \qquad w \gets w.\mathit{prev}
 /// \end{array}}
 /// \f]
 void spread( Node * u );

 /// the \f$ b \f$ middle blocks from \f$ u \f$, \f$ u \f$ with \f$ b - 2 \f$
 /// items and the others with \f$ b - 1 \f$, made \f$ b - 1 \f$ blocks, in
 /// \f$ O( b^2 ) \f$
 ///
 /// In pseudocode (Morin, section 3.3):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{gather}( u ) \cr
 /// 2 & \quad \textbf{for}\ k \gets 1\ \textbf{to}\ b - 1\ \textbf{do} \cr
 /// 3 & \qquad \textbf{while}\ |u| < b\ \textbf{and}\ |u.\mathit{next}| > 0
 ///     \ \textbf{do}\ u.\mathrm{pushBack}(
 ///     u.\mathit{next}.\mathrm{popFront}() ) \cr
 /// 4 & \qquad u \gets u.\mathit{next} \cr
 /// 5 & \quad \text{drop } u
 /// \end{array}}
 /// \f]
 void gather( Node * u );

 /// the blocks hold between 1 and \f$ b + 1 \f$ items, the middle ones
 /// \f$ b - 1 \f$ at least, and count \f$ n \f$ items
 [[nodiscard]] bool bounded( void ) const;

 mutable Node f_dummy;        ///< the dummy, before the first block
 Node * f_spare = nullptr;    ///< a block kept for later
 std::size_t f_n = 0;         ///< the number of items
 std::size_t f_blocks = 0;    ///< the number of blocks
 std::size_t f_b;             ///< the parameter \f$ b \f$
 std::size_t f_mask;          ///< the places of a block, less 1

 };  // end( class SEList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/SEList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SEList.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File SEList.h ----------------------------*/
/*--------------------------------------------------------------------------*/
