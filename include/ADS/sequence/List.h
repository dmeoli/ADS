/*--------------------------------------------------------------------------*/
/*------------------------------ File List.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The LIST of the book: a SEQUENCE [see ADT.h] realized as a bidirectional
 * circular list of cells with a sentinel. A list of n items has n + 1
 * cells, the sentinel holding no item; the position \f$ pos_i \f$ is the
 * address of the cell of the item of index i, for \f$ 0 \leq i < n \f$,
 * while \f$ pos_{-1} \f$ and \f$ pos_n \f$ are both the address of the
 * sentinel, which is therefore what finished() looks for. Every operation
 * takes O( 1 ) time, the list \f$ \Theta( n ) \f$ space.
 *
 * The sentinel is a member of the list rather than a cell of the heap, so
 * that moving a list relinks its first and last cells to the new sentinel in
 * O( 1 ) time and allocates nothing: the list moved from is left empty, and
 * still usable. Copying a list copies its cells (the rule of five, with the
 * copy and swap idiom for the assignment).
 *
 * The contracts [see Contract.h]: a position is a cell of this list, which
 * costs a scan of the list and is therefore an audit, checked by the
 * operations that change the list (insert(), remove() and write()) but not
 * by next(), prev() and read(), where it would make every scan quadratic;
 * read(), write() and remove() need a position that holds an item, i.e.,
 * not the sentinel;
 * insert() and write() ensure that v is read back in its position. The
 * invariant is that each cell is the predecessor of its successor, which
 * also makes the walk from the sentinel come back to it. The operations are
 * defined in List.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, section 17.5 (copy and move)
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

#ifndef __ADS_List
 #define __ADS_List
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

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

/// a SEQUENCE realized as a bidirectional circular list with a sentinel
template< class T >
class List {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// a cell of the list: the sentinel is a Cell with no meaningful value
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

 List( const List & other );

 /// the cells of other, which is left empty, in O( 1 ) time
 List( List && other ) noexcept;

 List & operator=( List other ) noexcept;

 ~List();

 void swap( List & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if the list is empty, i.e., the sentinel is its own successor
 [[nodiscard]] bool isEmpty( void ) const;

 /// true if p is \f$ pos_{-1} \f$ or \f$ pos_n \f$, i.e., the sentinel
 [[nodiscard]] bool finished( Pos p ) const;

 /// the position of the first item
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item
 [[nodiscard]] Pos tail( void ) const;

 /// the position of the item that follows p
 /// \pre p is a position of this list
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position of the item that precedes p
 /// \pre p is a position of this list
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts v in position p, i.e., before the item in p, and returns its
 /// position
 /// \pre p is a position of this list (an audit)
 /// \post the list is not empty, the result precedes p and holds v
 Pos insert( Pos p , const T & v );

 /// removes the item in position p, returning the position of its
 /// successor, which is now in its place
 /// \pre p holds an item of this list (an audit)
 /// \post the result follows the old predecessor of p
 Pos remove( Pos p );

 /// the item in position p
 /// \pre p holds an item of this list
 [[nodiscard]] const T & read( Pos p ) const;

 /// writes v in position p
 /// \pre p holds an item of this list (an audit)
 /// \post read( p ) is v
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

 /// the iterator, on items const if Const
 template< bool Const >
 class Iterator {
  public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = std::conditional_t< Const , const T & , T & >;
  using pointer = std::conditional_t< Const , const T * , T * >;

  Iterator( void ) = default;
  explicit Iterator( Cell * c ) : f_c( c ) {}

  /// the const iterator from the other one
  template< bool C > requires ( Const && ! C )
  Iterator( const Iterator< C > & o ) : f_c( o.cell() ) {}

  reference operator*( void ) const { return( f_c->value ); }
  pointer operator->( void ) const { return( &f_c->value ); }
  Iterator & operator++( void ) { f_c = f_c->succ; return( *this ); }
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  Iterator & operator--( void ) { f_c = f_c->pred; return( *this ); }
  Iterator operator--( int ) { auto t = *this; --*this; return( t ); }
  bool operator==( const Iterator & o ) const = default;

  /// the cell, which is the position of the book
  [[nodiscard]] Cell * cell( void ) const { return( f_c ); }

  private:
  Cell * f_c = nullptr;
  };

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;
 using iterator = Iterator< false >;
 using const_iterator = Iterator< true >;
 using reverse_iterator = std::reverse_iterator< iterator >;
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 [[nodiscard]] iterator begin( void ) { return( iterator( head() ) ); }
 [[nodiscard]] iterator end( void ) { return( iterator( sentinel() ) ); }
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( head() ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( sentinel() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }
 [[nodiscard]] reverse_iterator rbegin( void ) {
  return( reverse_iterator( end() ) );
  }
 [[nodiscard]] reverse_iterator rend( void ) {
  return( reverse_iterator( begin() ) );
  }
 [[nodiscard]] const_reverse_iterator rbegin( void ) const {
  return( const_reverse_iterator( end() ) );
  }
 [[nodiscard]] const_reverse_iterator rend( void ) const {
  return( const_reverse_iterator( begin() ) );
  }

 /// v at the end and at the front, for the inserters of std
 void push_back( const T & v );
 void push_front( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// each cell is the predecessor of its successor, O( n )
 [[nodiscard]] bool linked( void ) const;

 /// p is a cell of this list, the sentinel included, O( n )
 [[nodiscard]] bool owns( Pos p ) const;

 /// the address of the sentinel
 [[nodiscard]] Cell * sentinel( void ) const;

 /// takes the cells of o, which is left empty
 /// \pre this list is empty
 void adopt( List & o ) noexcept;

 mutable Cell f_sentinel;   ///< the sentinel, i.e., the list

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
