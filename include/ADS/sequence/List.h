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
 * Against Morin's `DLList`, the claims for the benchmarks: the cells come
 * from the pool, with no call to the allocator after the first ones and
 * next to each other in memory, where Morin's are each a `new` and a
 * `delete`; `position()` walks from the nearer end as Morin's `getNode()`.
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
 * It is also the `DLList` of section 3.2 of Morin's book: the list keeps
 * its size, and has the operations by rank of a LIST [see `IndexedList.h`],
 * `get( i )`, `set( i , x )`, `add( i , x )` and `erase( i )`, which reach
 * the cell of rank \f$ i \f$ from the nearer end (`position( i )`, Morin's
 * `getNode( i )`) in \f$ O( 1 + \min\{ i , n - i \} ) \f$ time, and those of
 * the exercises of chapter 3 of Morin: `checkSize()` (3.5), `insert()`,
 * which is the `addBefore()` of exercise 3.6, `isPalindrome()` (3.7),
 * `rotate()`, which moves the sentinel and no item (3.8), `truncate()`
 * (3.9), `absorb()` (3.10), `deal()` (3.11), `reverse()` (3.12), and
 * `takeFirst()`, `merge()` and `mergeSort()` (3.13). The operations that
 * move cells from a list to another relink them when the cells come from
 * `new` (`Pooled` false), as Morin's do; between pooled lists a cell cannot
 * change pool, so that `truncate()`, `deal()`, `takeFirst()` and `merge()`
 * move the items into cells of the other list (in time linear in the items
 * moved), while `absorb()` takes the pool of the other list along with its
 * cells [see `ObjectPool::adopt()`], in \f$ O( \log n ) \f$, and
 * `mergeSort()` and `reverse()` relink the cells of one list in both
 * cases.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, section 17.5 (copy and move)
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 3.2 and exercises
 *      3.5 to 3.13
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
#include "ADS/Portable.h"
#include "ADS/memory/ObjectPool.h"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS List ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized as a bidirectional circular list with a sentinel,
/// its cells from a pool if `Pooled`
template< class T , bool Pooled = true >
class List
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the items

 /// a cell of the list: the sentinel is a `Cell` with no meaningful value
 struct Cell
 {
  Cell * pred;  ///< the predecessor
  Cell * succ;  ///< the successor
  T value;      ///< the item
  };

 using Pos = Cell *;  ///< a position is the address of a cell

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
 /// 1 & \textbf{function}\ \mathrm{insert}( p , v )\quad \triangleright
 ///     \ \text{before the cell } p \cr
 /// 2 & \quad t \gets \text{a new cell of value } v;\ q \gets
 ///     p.\mathit{pred} \cr
 /// 3 & \quad t.\mathit{succ} \gets p;\ t.\mathit{pred} \gets q \cr
 /// 4 & \quad q.\mathit{succ} \gets t;\ p.\mathit{pred} \gets t\quad
 ///     \triangleright\ \text{the sentinel if } q \text{ or } p
 ///     \text{ is none} \cr
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
 /// 2 & \quad q \gets p.\mathit{pred};\ t \gets p.\mathit{succ} \cr
 /// 3 & \quad q.\mathit{succ} \gets t;\ t.\mathit{pred} \gets q\quad
 ///     \triangleright\ p \text{ is out of the chain} \cr
 /// 4 & \quad \text{free the cell } p;\ \textbf{return}\ t
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
 class Iterator
 {
  public:
  /// the category of the iterator, bidirectional
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;                    ///< the type of the items
  using difference_type = std::ptrdiff_t;  ///< the type of the distances
  /// a reference to an item, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an item, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// an iterator on no cell
  Iterator( void ) = default;

  /// on the item of the cell \f$ c \f$, the end if \f$ c \f$ is the
  /// sentinel
  explicit Iterator( Cell * c ) : f_c( c ) { }

  /// the const iterator from the other one
  template< bool C >
   requires( Const && ! C )
  Iterator( const Iterator< C > & o ) : f_c( o.cell() ) { }

  /// the item
  reference operator*( void ) const { return( f_c->value ); }
  /// the address of the item
  pointer operator->( void ) const { return( &f_c->value ); }
  /// to the next cell
  Iterator & operator++( void ) {
   f_c = f_c->succ;
   return( *this );
   }
  /// to the next cell, returning the iterator before the step
  Iterator operator++( int ) {
   auto t = *this;
   ++*this;
   return( t );
   }
  /// to the previous cell
  Iterator & operator--( void ) {
   f_c = f_c->pred;
   return( *this );
   }
  /// to the previous cell, returning the iterator before the step
  Iterator operator--( int ) {
   auto t = *this;
   --*this;
   return( t );
   }
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

/*------------------------- THE DLList OF MORIN ----------------------------*/

 /// the number \f$ n \f$ of items, kept
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the cell of rank \f$ i \f$, the sentinel for \f$ i = n \f$, reached
 /// from the nearer end in \f$ O( 1 + \min\{ i , n - i \} ) \f$ (Morin's
 /// `getNode( i )`)
 ///
 /// In pseudocode (Morin, section 3.2), \f$ s \f$ the sentinel:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{position}( i ) \cr
 /// 2 & \quad \textbf{if}\ i < n / 2\ \textbf{then}\ p \gets
 ///     s.\mathit{succ};\ \textbf{repeat}\ i\ \text{ times: } p \gets
 ///     p.\mathit{succ} \cr
 /// 3 & \quad \textbf{else}\ p \gets s;\ \textbf{repeat}\ n - i
 ///     \ \text{ times: } p \gets p.\mathit{pred} \cr
 /// 4 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 [[nodiscard]] Pos position( std::size_t i ) const;

 /// the item of rank \f$ i \f$, \f$ O( 1 + \min\{ i , n - i \} ) \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & get( std::size_t i ) const;

 /// \f$ x \f$ written at the rank \f$ i \f$, the old item returned,
 /// \f$ O( 1 + \min\{ i , n - i \} ) \f$
 /// \pre \f$ i < n \f$
 T set( std::size_t i , const T & x );

 /// \f$ x \f$ put at the rank \f$ i \f$, before the cell of rank \f$ i \f$,
 /// \f$ O( 1 + \min\{ i , n - i \} ) \f$
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void add( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$ and returns it,
 /// \f$ O( 1 + \min\{ i , n - i \} ) \f$
 /// \pre \f$ i < n \f$
 /// \post one item less
 T erase( std::size_t i );

 /// counts the cells and throws `std::logic_error` if they are not the
 /// size kept (exercise 3.5 of Morin's book), \f$ O( n ) \f$
 void checkSize( void ) const;

 /// true if the item of rank \f$ i \f$ equals that of rank
 /// \f$ n - 1 - i \f$ for every \f$ i \f$, by two walks from the two ends,
 /// \f$ \lfloor n / 2 \rfloor \f$ comparisons at most (exercise 3.7 of
 /// Morin's book)
 ///
 /// In pseudocode (Morin, exercise 3.7):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{isPalindrome}() \cr
 /// 2 & \quad p \gets s.\mathit{succ};\ q \gets s.\mathit{pred} \cr
 /// 3 & \quad \textbf{for}\ k \gets 1\ \textbf{to}\ \lfloor n / 2
 ///     \rfloor\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ p.\mathit{value} \ne q.\mathit{value}
 ///     \ \textbf{then return false} \cr
 /// 5 & \qquad p \gets p.\mathit{succ};\ q \gets q.\mathit{pred} \cr
 /// 6 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool isPalindrome( void ) const;

 /// the item of rank \f$ i \f$ moved to the rank \f$ ( i + r ) \bmod n \f$
 /// by moving the sentinel before the item of rank \f$ n - r \f$, in
 /// \f$ O( 1 + \min\{ r , n - r \} ) \f$, no item moved and no cell made
 /// (exercise 3.8 of Morin's book)
 ///
 /// In pseudocode (Morin, exercise 3.8), \f$ r \f$ reduced modulo
 /// \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rotate}( r ) \cr
 /// 2 & \quad \textbf{if}\ r = 0\ \textbf{then return} \cr
 /// 3 & \quad u \gets \mathrm{position}( n - r ) \cr
 /// 4 & \quad \text{unlink } s;\ \text{link } s \text{ between }
 ///     u.\mathit{pred} \text{ and } u
 /// \end{array}}
 /// \f]
 void rotate( std::size_t r );

 /// the items of ranks \f$ i , \ldots , n - 1 \f$ cut off into the list
 /// returned, in \f$ O( 1 + \min\{ i , n - i \} ) \f$ by relinking
 /// (exercise 3.9 of Morin's book; in \f$ O( 1 + n - i ) \f$ by moving the
 /// items if `Pooled`)
 ///
 /// In pseudocode (Morin, exercise 3.9):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{truncate}( i ) \cr
 /// 2 & \quad u \gets \mathrm{position}( i );\ L \gets \text{an empty list}
 ///     \cr
 /// 3 & \quad \textbf{if}\ u \ne s\ \textbf{then}\ \text{link } u , \ldots
 ///     , s.\mathit{pred} \text{ between the ends of } L;\ \text{link }
 ///     u.\mathit{pred} \text{ to } s \cr
 /// 4 & \quad |L| \gets n - i;\ n \gets i;\ \textbf{return}\ L
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post \f$ i \f$ items left
 [[nodiscard]] List truncate( std::size_t i );

 /// the items of `l2` appended to this list in their order, `l2` left
 /// empty, in \f$ O( 1 ) \f$ by relinking (exercise 3.10 of Morin's book;
 /// in \f$ O( \log n ) \f$ if `Pooled`, the pool of `l2` taken)
 /// \pre `l2` is not this list
 void absorb( List & l2 );

 /// the items of odd rank moved, in their order, to the list returned
 /// (exercise 3.11 of Morin's book), \f$ O( n ) \f$
 ///
 /// In pseudocode (Morin, exercise 3.11):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deal}() \cr
 /// 2 & \quad L \gets \text{an empty list};\ p \gets s.\mathit{succ} \cr
 /// 3 & \quad \textbf{while}\ p \ne s\ \textbf{and}\ p.\mathit{succ} \ne
 ///     s\ \textbf{do} \cr
 /// 4 & \qquad q \gets p.\mathit{succ};\ \text{unlink } q;\ \text{link } q
 ///     \text{ at the end of } L;\ p \gets p.\mathit{succ} \cr
 /// 5 & \quad \textbf{return}\ L
 /// \end{array}}
 /// \f]
 /// \post \f$ \lceil n / 2 \rceil \f$ items left
 [[nodiscard]] List deal( void );

 /// the order of the items reversed, by exchanging the two links of each
 /// cell and of the sentinel, \f$ O( n ) \f$ (exercise 3.12 of Morin's
 /// book)
 /// \post the items in the reverse order (an audit)
 void reverse( void );

 /// the first cell of `l2` moved to the end of this list, by relinking (a
 /// new cell with the item moved if `Pooled`), \f$ O( 1 ) \f$ (exercise
 /// 3.13 of Morin's book)
 /// \pre `l2` is not empty and is not this list
 void takeFirst( List & l2 );

 /// the merge of the two sorted lists `l1` and `l2`, which are left empty,
 /// stable (the items of `l1` first among equal ones), in
 /// \f$ O( |l1| + |l2| ) \f$ (exercise 3.13 of Morin's book)
 ///
 /// In pseudocode (Morin, exercise 3.13):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( l_1 , l_2 ) \cr
 /// 2 & \quad L \gets \text{an empty list} \cr
 /// 3 & \quad \textbf{while}\ l_1 \ne \emptyset\ \textbf{and}\ l_2 \ne
 ///     \emptyset\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \mathrm{first}( l_2 ) < \mathrm{first}( l_1 )
 ///     \ \textbf{then}\ L.\mathrm{takeFirst}( l_2 )\ \textbf{else}
 ///     \ L.\mathrm{takeFirst}( l_1 ) \cr
 /// 5 & \quad L.\mathrm{absorb}( l_1 );\ L.\mathrm{absorb}( l_2 );
 ///     \ \textbf{return}\ L
 /// \end{array}}
 /// \f]
 /// \pre `l1` and `l2` sorted by `less`
 /// \post the result sorted (an audit)
 template< class C = std::less<> >
 [[nodiscard]] static List merge( List & l1 , List & l2 , C less = C() );

 /// sorts the list, stable, by the merge sort of exercise 3.13 of Morin's
 /// book: the list cut in two halves by a walk to its middle, each sorted
 /// recursively and the two merged, all on the cells of this list, which
 /// are relinked and never copied, \f$ O( n \log n ) \f$ time and
 /// \f$ O( \log n ) \f$ space for the recursion [see `ListSort.h` for the
 /// natural merge sort of the runs, bottom up]
 ///
 /// In pseudocode (Morin, exercise 3.13):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{mergeSort}( L ) \cr
 /// 2 & \quad \textbf{if}\ |L| \le 1\ \textbf{then return} \cr
 /// 3 & \quad R \gets L.\mathrm{truncate}( \lfloor |L| / 2 \rfloor ) \cr
 /// 4 & \quad \mathrm{mergeSort}( L );\ \mathrm{mergeSort}( R ) \cr
 /// 5 & \quad L \gets \mathrm{merge}( L , R )
 /// \end{array}}
 /// \f]
 /// \post the list sorted (an audit)
 template< class C = std::less<> >
 void mergeSort( C less = C() );

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
 struct Plain
 {
  /// a new cell made from the arguments
  template< class... Args >
  Cell * make( Args &&... args ) {
   return( new Cell{ std::forward< Args >( args )... } );
   }
  /// deletes the cell \f$ c \f$
  void free( Cell * c ) noexcept { delete c; }
  /// nothing to exchange
  void swap( Plain & ) noexcept { }
  };

 /// the cell \f$ c \f$ unlinked from its list and linked before \f$ p \f$
 static void relink( Cell * c , Cell * p ) noexcept;

 /// the chain of \f$ k \f$ cells from \f$ a \f$, ended by nil along `succ`,
 /// sorted by the merge sort of `mergeSort()`; its first cell returned
 template< class C >
 static Cell * sortChain( Cell * a , std::size_t k , C & less );

 mutable Cell f_sentinel;  ///< the sentinel, i.e., the list
 std::size_t f_size = 0;   ///< the number of items
 /// the cells
 ADS_NO_UNIQUE_ADDRESS
 std::conditional_t< Pooled , ObjectPool< Cell > , Plain > f_pool;

 };  // end( class List )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/List.tpp"

/*--------------------------------------------------------------------------*/

#endif /* List.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File List.h -----------------------------*/
/*--------------------------------------------------------------------------*/
