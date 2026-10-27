/*--------------------------------------------------------------------------*/
/*--------------------------- File SinglyList.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see `ADT.h`] realized as a singly linked list, not circular
 * and with no sentinel (exercise 4.6 of the book). The position
 * \f$ \mathit{pos}_i \f$ is the address of the cell of \f$ s_{i-1} \f$ for
 * \f$ i > 0 \f$ and nil for \f$ i = 0 \f$, so that the cell before an item,
 * which `insert()` and `remove()` change, is always at hand: every operation
 * takes \f$ O( 1 ) \f$ time but `tail()` and `prev()`, which scan the list
 * and take \f$ O( n ) \f$. \f$ \mathit{pos}_n \f$ is the address of the last
 * cell (nil if the list is empty), and \f$ \mathit{pos}_{-1} \f$ a
 * distinguished address of the list itself. This is the convention of the
 * realizations of a list with pointers or with cursors in which a position
 * is the cell that precedes the item, e.g., in the book by Aho, Hopcroft and
 * Ullman.
 *
 * The contracts [see `Contract.h`]: `read()`, `write()` and `remove()` need
 * a position that holds an item, i.e., one that is not `finished()`;
 * `insert()` and `write()` ensure that \f$ v \f$ is read back in its
 * position. A list is copied cell by cell and moved in \f$ O( 1 ) \f$ time,
 * the one moved from left empty. Beyond the book, the cells come from an
 * `ObjectPool` [see `ObjectPool.h`] of the list, which takes back the cell
 * of `remove()` for the next `insert()`, so that a cell costs no call to the
 * allocator, the cells made one after another are next to each other in
 * memory, and a list of items with a trivial destructor dies in one step per
 * block; the price is that the list keeps the memory of its largest size
 * until it dies. The operations are defined in `SinglyList.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see A. V. Aho, J. E. Hopcroft, J. D. Ullman, Data Structures and
 *      Algorithms, Addison-Wesley, 1983, section 2.2
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

#ifndef __ADS_SinglyList
 #define __ADS_SinglyList
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
/*-------------------------- CLASS SinglyList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized as a singly linked list with no sentinel
template< class T >
class SinglyList {

/*--------------------------------------------------------------------------*/
 public:

 /// a cell of the list
 struct Cell {
  T value;      ///< the item
  Cell * next;  ///< the next cell, nil after the last one
  };

 using Item = T;       ///< the type of the items
 using Pos = Cell *;   ///< the cell before the item, nil for the first one

 /// the empty list
 SinglyList( void ) = default;

 /// the list of the given items, in their order
 SinglyList( std::initializer_list< T > items );

 /// a copy of `other`, cell by cell
 SinglyList( const SinglyList & other );

 /// the cells of `other`, which is left empty
 SinglyList( SinglyList && other ) noexcept;

 /// the items of `other`, by copy and swap
 SinglyList & operator=( SinglyList other ) noexcept;

 /// frees the cells
 ~SinglyList();

 /// exchanges the cells with `other`
 void swap( SinglyList & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const;

 /// true on \f$ \mathit{pos}_{-1} \f$ and on \f$ \mathit{pos}_n \f$, the
 /// address of the last cell (nil if the list is empty)
 [[nodiscard]] bool finished( Pos p ) const;

 /// \f$ \mathit{pos}_0 \f$, i.e., nil
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item, \f$ O( n ) \f$
 ///
 /// In pseudocode (exercise 4.6), the cell before the last one found by a
 /// scan:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tail}() \cr
 /// 2 & \quad \textbf{if}\ \mathit{first} = \textbf{nil}\ \textbf{then
 ///     return}\ \mathit{pos}_{-1} \cr
 /// 3 & \quad p \gets \textbf{nil} \cr
 /// 4 & \quad \textbf{while not}\ \mathrm{finished}( \mathrm{next}( p ) )
 ///     \ \textbf{do}\ p \gets \mathrm{next}( p ) \cr
 /// 5 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 [[nodiscard]] Pos tail( void ) const;

 /// the position that follows \f$ p \f$
 /// \pre \f$ p \f$ is not \f$ \mathit{pos}_n \f$
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position that precedes \f$ p \f$, \f$ O( n ) \f$
 ///
 /// In pseudocode (exercise 4.6), a scan for the cell whose successor is
 /// \f$ p \f$, since no link goes back:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{prev}( p ) \cr
 /// 2 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then return}
 ///     \ \mathit{pos}_{-1} \cr
 /// 3 & \quad \textbf{if}\ p = \mathit{first}\ \textbf{then return nil} \cr
 /// 4 & \quad q \gets \mathit{first} \cr
 /// 5 & \quad \textbf{while}\ q.\mathit{next} \ne p\ \textbf{do}\ q \gets
 ///     q.\mathit{next} \cr
 /// 6 & \quad \textbf{return}\ q
 /// \end{array}}
 /// \f]
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts \f$ v \f$ in the position \f$ p \f$, i.e., before the item in
 /// \f$ p \f$; the new item takes the position \f$ p \f$
 ///
 /// In pseudocode (exercise 4.6), the new cell after the cell \f$ p \f$,
 /// or first if \f$ p \f$ is nil:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( p , v ) \cr
 /// 2 & \quad t \gets \text{a new cell};\ t.\mathit{value} \gets v \cr
 /// 3 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then}\ t.\mathit{next}
 ///     \gets \mathit{first};\ \mathit{first} \gets t \cr
 /// 4 & \quad \textbf{else}\ t.\mathit{next} \gets p.\mathit{next};
 ///     \ p.\mathit{next} \gets t \cr
 /// 5 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ is not \f$ \mathit{pos}_{-1} \f$
 /// \post `read( p )` is \f$ v \f$
 Pos insert( Pos p , const T & v );

 /// removes the item in the position \f$ p \f$, whose successor takes the
 /// position \f$ p \f$
 ///
 /// In pseudocode (exercise 4.6), the cell after \f$ p \f$ unlinked:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( p ) \cr
 /// 2 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then}\ t \gets
 ///     \mathit{first};\ \mathit{first} \gets t.\mathit{next} \cr
 /// 3 & \quad \textbf{else}\ t \gets p.\mathit{next};\ p.\mathit{next}
 ///     \gets t.\mathit{next} \cr
 /// 4 & \quad \text{free the cell } t;\ \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ holds an item
 Pos remove( Pos p );

 /// the item in the position \f$ p \f$
 /// \pre \f$ p \f$ holds an item
 [[nodiscard]] const T & read( Pos p ) const;

 /// writes \f$ v \f$ in the position \f$ p \f$
 /// \pre \f$ p \f$ holds an item
 /// \post `read( p )` is \f$ v \f$
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the same items in the same order
 [[nodiscard]] bool operator==( const SinglyList & o ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a forward iterator holds the cell of its item, not the cell before it
 // as a position does, so that the end is nil; it stays valid until its
 // cell is removed, and a change made through it bypasses the audits of
 // write()

 /// the iterator, on items const if `Const`
 template< bool Const >
 class Iterator {
  public:
  /// the category of the iterator, forward
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;                     ///< the type of the items
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// a reference to an item, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an item, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// the end
  Iterator( void ) = default;

  /// on the item of the cell \f$ c \f$, the end if \f$ c \f$ is nil
  explicit Iterator( Cell * c ) : f_c( c ) {}

  /// the const iterator from the other one
  template< bool C > requires ( Const && ! C )
  Iterator( const Iterator< C > & o ) : f_c( o.f_c ) {}

  /// the item
  reference operator*( void ) const { return( f_c->value ); }
  /// the address of the item
  pointer operator->( void ) const { return( &**this ); }
  /// to the next cell
  Iterator & operator++( void ) { f_c = f_c->next; return( *this ); }
  /// to the next cell, returning the iterator before the step
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// equal on the same cell
  bool operator==( const Iterator & o ) const = default;

  private:
  Cell * f_c = nullptr;   ///< the cell of the item, nil at the end

  template< bool > friend class Iterator;
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

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( iterator( f_first ) ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_first ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the list of the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 SinglyList( I first , S last );

 /// \f$ v \f$ at the front, for `std::front_inserter`
 void push_front( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// the cell of the item in the position \f$ p \f$
 [[nodiscard]] Cell * cell( Pos p ) const;

 Cell * f_first = nullptr;               ///< the first cell
 ObjectPool< Cell > f_pool;              ///< the cells
 /// its address is \f$ \mathit{pos}_{-1} \f$
 mutable Cell f_before{ T() , nullptr };

 };  // end( class SinglyList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/SinglyList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SinglyList.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SinglyList.h --------------------------*/
/*--------------------------------------------------------------------------*/
