/*--------------------------------------------------------------------------*/
/*------------------------------ File List.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The LIST of the book: a SEQUENCE [see `ADT.h`] realized as a bidirectional
 * circular list of cells with a sentinel. A list of \f$ n \f$ items has
 * \f$ n + 1 \f$ cells, the sentinel holding no item; the position
 * \f$ \mathit{pos}_i \f$ is the address of the cell of the item of index
 * \f$ i \f$, for \f$ 0 \leq i < n \f$, while \f$ \mathit{pos}_{-1} \f$ and
 * \f$ \mathit{pos}_n \f$ are both the address of the sentinel, which is
 * therefore what `finished()` looks for. Every operation takes
 * \f$ O( 1 ) \f$ time, the list \f$ \Theta( n ) \f$ space.
 *
 * The sentinel is a member of the list rather than a cell of the heap, so
 * that moving a list relinks its first and last cells to the new sentinel in
 * \f$ O( 1 ) \f$ time and allocates nothing: the list moved from is left
 * empty, and still usable. Copying a list copies its cells (the rule of
 * five, with the copy and swap idiom for the assignment). Beyond the book,
 * the cells come from an `ObjectPool` [see `ObjectPool.h`] of the list,
 * which takes back the cell of `remove()` for the next `insert()`: a cell
 * costs no call to the allocator, the cells made one after another are next
 * to each other in memory, and a list of items with a trivial destructor
 * dies a block at a time; the price is that the list keeps the memory of its
 * largest size until it dies, and that a move takes the pool along with the
 * cells. A pool is no gain for a list of a few cells, which still takes a
 * block per cell and a header per block: a structure of many short lists
 * (the chains of `ChainedHashTable`) takes `List< T , false >`, whose cells
 * come one by one from `new` and `delete`.
 *
 * The contracts [see `Contract.h`]: a position is a cell of this list, which
 * costs a scan of the list and is therefore an audit, checked by the
 * operations that change the list (`insert()`, `remove()` and `write()`)
 * but not by `next()`, `prev()` and `read()`, where it would make every scan
 * quadratic; `read()`, `write()` and `remove()` need a position that holds an
 * item, i.e., not the sentinel; `insert()` and `write()` ensure that
 * \f$ v \f$ is read back in its position. The invariant is that each cell is
 * the predecessor of its successor, which also makes the walk from the
 * sentinel come back to it. The operations are defined in `List.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, section 17.5 (copy and move)
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

#ifndef __ADS_List
 #define __ADS_List
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/ObjectPool.h"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS List ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized as a bidirectional circular list with a sentinel,
/// its cells from a pool if `Pooled`
template< class T , bool Pooled = true >
class List {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// a cell of the list: the sentinel is a `Cell` with no meaningful value
 struct Cell {
  Cell * pred;     ///< the predecessor
  Cell * succ;     ///< the successor
  T value;         ///< the item
  };

 using Pos = Cell *;   ///< a position is the address of a cell

/*--------------------------------------------------------------------------*/

 /// the empty list: the sentinel alone, its own predecessor and successor
 List( void );

 /// the list of the given items, in their order
 List( std::initializer_list< T > items );

 /// the list of the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 List( I first , S last );

 /// a copy of `other`, cell by cell
 List( const List & other );

 /// the cells of `other`, which is left empty, in \f$ O( 1 ) \f$ time
 List( List && other ) noexcept;

 /// the items of `other`, by copy and swap
 List & operator=( List other ) noexcept;

 /// frees the cells
 ~List();

 /// exchanges the cells with `other`, relinking them to the sentinels in
 /// \f$ O( 1 ) \f$ time
 void swap( List & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if the list is empty, i.e., the sentinel is its own successor
 [[nodiscard]] bool empty( void ) const;

 /// true if \f$ p \f$ is \f$ \mathit{pos}_{-1} \f$ or \f$ \mathit{pos}_n \f$,
 /// i.e., the sentinel
 [[nodiscard]] bool finished( Pos p ) const;

 /// the position of the first item
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item
 [[nodiscard]] Pos tail( void ) const;

 /// the position of the item that follows \f$ p \f$
 /// \pre \f$ p \f$ is a position of this list
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position of the item that precedes \f$ p \f$
 /// \pre \f$ p \f$ is a position of this list
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts \f$ v \f$ in the position \f$ p \f$, i.e., before the item in
 /// \f$ p \f$, and returns its position
 ///
 /// In pseudocode (section 4.1), a new cell \f$ t \f$ linked between
 /// \f$ p \f$ and its predecessor, which the sentinel guarantees:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( p , v ) \cr
 /// 2 & \quad t \gets \text{a new cell};\ t.\mathit{value} \gets v \cr
 /// 3 & \quad t.\mathit{pred} \gets p.\mathit{pred};\ t.\mathit{succ} \gets
 ///     p \cr
 /// 4 & \quad p.\mathit{pred}.\mathit{succ} \gets t;\ p.\mathit{pred} \gets
 ///     t \cr
 /// 5 & \quad \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ is a position of this list (an audit)
 /// \post the list is not empty, the result precedes \f$ p \f$ and holds
 ///       \f$ v \f$
 Pos insert( Pos p , const T & v );

 /// removes the item in the position \f$ p \f$, returning the position of
 /// its successor, which is now in its place
 ///
 /// In pseudocode (section 4.1), the cell unlinked from its two neighbors:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( p ) \cr
 /// 2 & \quad p.\mathit{pred}.\mathit{succ} \gets p.\mathit{succ};
 ///     \ p.\mathit{succ}.\mathit{pred} \gets p.\mathit{pred} \cr
 /// 3 & \quad t \gets p.\mathit{succ};\ \text{free the cell } p \cr
 /// 4 & \quad \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ holds an item of this list (an audit)
 /// \post the result follows the old predecessor of \f$ p \f$
 Pos remove( Pos p );

 /// the item in the position \f$ p \f$
 /// \pre \f$ p \f$ holds an item of this list
 [[nodiscard]] const T & read( Pos p ) const;

 /// writes \f$ v \f$ in the position \f$ p \f$
 /// \pre \f$ p \f$ holds an item of this list (an audit)
 /// \post `read( p )` is \f$ v \f$
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the same items in the same order
 [[nodiscard]] bool operator==( const List & o ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a bidirectional iterator walks the cells, the sentinel being the end;
 // it stays valid until its cell is removed, and a change made through it
 // bypasses the audits of write()

 /// the iterator, on items const if `Const`
 template< bool Const >
 class Iterator {
  public:
  /// the category of the iterator, bidirectional
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;                     ///< the type of the items
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// a reference to an item, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an item, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// an iterator on no cell
  Iterator( void ) = default;

  /// on the item of the cell \f$ c \f$, the end if \f$ c \f$ is the
  /// sentinel
  explicit Iterator( Cell * c ) : f_c( c ) {}

  /// the const iterator from the other one
  template< bool C > requires ( Const && ! C )
  Iterator( const Iterator< C > & o ) : f_c( o.cell() ) {}

  /// the item
  reference operator*( void ) const { return( f_c->value ); }
  /// the address of the item
  pointer operator->( void ) const { return( &f_c->value ); }
  /// to the next cell
  Iterator & operator++( void ) { f_c = f_c->succ; return( *this ); }
  /// to the next cell, returning the iterator before the step
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// to the previous cell
  Iterator & operator--( void ) { f_c = f_c->pred; return( *this ); }
  /// to the previous cell, returning the iterator before the step
  Iterator operator--( int ) { auto t = *this; --*this; return( t ); }
  /// equal on the same cell
  bool operator==( const Iterator & o ) const = default;

  /// the cell, which is the position of the book
  [[nodiscard]] Cell * cell( void ) const { return( f_c ); }

  private:
  Cell * f_c = nullptr;
  };

 /// the type of the items
 using value_type = T;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = T &;
 /// a constant reference to an item
 using const_reference = const T &;
 /// the iterator on the items
 using iterator = Iterator< false >;
 /// the iterator on the constant items
 using const_iterator = Iterator< true >;
 /// the iterator on the items, backwards
 using reverse_iterator = std::reverse_iterator< iterator >;
 /// the iterator on the constant items, backwards
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( iterator( head() ) ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator( sentinel() ) ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( head() ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( sentinel() ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }
 /// the iterator on the last item, backwards
 [[nodiscard]] reverse_iterator rbegin( void ) {
  return( reverse_iterator( end() ) );
  }
 /// the iterator before the first item, backwards
 [[nodiscard]] reverse_iterator rend( void ) {
  return( reverse_iterator( begin() ) );
  }
 /// the constant iterator on the last item, backwards
 [[nodiscard]] const_reverse_iterator rbegin( void ) const {
  return( const_reverse_iterator( end() ) );
  }
 /// the constant iterator before the first item, backwards
 [[nodiscard]] const_reverse_iterator rend( void ) const {
  return( const_reverse_iterator( begin() ) );
  }

 /// \f$ v \f$ at the end, for `std::back_inserter`
 void push_back( const T & v );

 /// \f$ v \f$ at the front, for `std::front_inserter`
 void push_front( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// each cell is the predecessor of its successor, \f$ O( n ) \f$
 [[nodiscard]] bool linked( void ) const;

 /// \f$ p \f$ is a cell of this list, the sentinel included,
 /// \f$ O( n ) \f$
 [[nodiscard]] bool owns( Pos p ) const;

 /// the address of the sentinel
 [[nodiscard]] Cell * sentinel( void ) const;

 /// takes the cells of `o`, which is left empty
 /// \pre this list is empty
 void adopt( List & o ) noexcept;

 /// the cells one by one from `new` and `delete`, as a pool would give
 /// them
 struct Plain {
  /// a new cell made from the arguments
  template< class... Args >
  Cell * make( Args &&... args ) {
   return( new Cell{ std::forward< Args >( args )... } );
   }
  /// deletes the cell \f$ c \f$
  void free( Cell * c ) noexcept { delete c; }
  /// nothing to exchange
  void swap( Plain & ) noexcept {}
  };

 mutable Cell f_sentinel;   ///< the sentinel, i.e., the list
 /// the cells
 [[no_unique_address]]
 std::conditional_t< Pooled , ObjectPool< Cell > , Plain > f_pool;

 };  // end( class List )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/List.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* List.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File List.h -----------------------------*/
/*--------------------------------------------------------------------------*/
