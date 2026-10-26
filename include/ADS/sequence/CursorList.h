/*--------------------------------------------------------------------------*/
/*--------------------------- File CursorList.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see ADT.h] realized with cursors, beyond the book: the cells
 * of the lists are the places of a vector, a CursorSpace, and a link is the
 * index of the next cell, 0 being nil (the place 0 is never used). Several
 * lists can share one space, each being the cursor of its first cell, and
 * the free cells form a further list in the same vector, the free list:
 * insert() takes its first cell, remove() gives the cell back, and a space
 * whose free list is empty doubles its vector, the cursors staying valid.
 * This is how a language with no pointers, or a program that wants its
 * lists in one block of memory, realizes linked lists.
 *
 * As in SinglyList [see SinglyList.h], the position \f$ pos_i \f$ is the
 * cursor of the cell of \f$ a_{i-1} \f$ for i > 0 and 0 for i = 0, so that
 * insert() and remove() find the link to change at hand and take
 * \f$ O( 1 ) \f$ time (amortized, for the growth of the space); \f$ pos_n \f$
 * is the cursor of the last cell (0 if the list is empty) and \f$ pos_{-1} \f$
 * is -1. tail() and prev() scan the list, in \f$ O( n ) \f$.
 *
 * The contracts [see Contract.h]: read(), write() and remove() need a
 * position that holds an item; insert() and write() ensure that v is read
 * back in its position; the invariant of the space is that the free cells
 * and the cells in use are all the places but 0, the free list having no
 * cycle. A list made with no space has a space of its own; the lists of a
 * space share it, and the space lives as long as one of them does. The
 * operations are defined in CursorList.tpp.
 *
 * \see A. V. Aho, J. E. Hopcroft, J. D. Ullman, Data Structures and
 *      Algorithms, Addison-Wesley, 1983, section 2.2 (the realization with
 *      cursors, the free list and the procedure move)
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.2.3 (the available space list)
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

#ifndef __ADS_CursorList
 #define __ADS_CursorList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CursorSpace -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the vector of the cells of some CursorList, with its free list
template< class T >
class CursorSpace {

/*--------------------------------------------------------------------------*/
 public:

 /// a cell: the item and the cursor of the next cell, 0 for nil
 struct Cell {
  T value;
  long next;
  };

 /// a space of dim free cells, the places 1 , ... , dim
 /// \pre dim >= 1
 explicit CursorSpace( long dim = 8 );

 /// the cell at the cursor c
 /// \pre 1 <= c <= capacity()
 [[nodiscard]] Cell & operator[]( long c );

 /// \pre 1 <= c <= capacity()
 [[nodiscard]] const Cell & operator[]( long c ) const;

 /// takes the first free cell, doubling the vector if there is none
 /// \post the result is a cell in use
 long take( void );

 /// gives the cell c back to the free list
 /// \pre c is a cell in use
 void give( long c );

 /// the number of places, 0 excluded
 [[nodiscard]] long capacity( void ) const;

 /// the number of cells in use
 [[nodiscard]] long used( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the free list has no cycle, and with the cells in use it makes all the
 /// places, \f$ O( \mathit{capacity} ) \f$
 [[nodiscard]] bool partition( void ) const;

 std::vector< Cell > f_A;   ///< the cells, f_A[ 0 ] never used
 long f_free;               ///< the first free cell, 0 if none
 long f_used = 0;           ///< the number of cells in use

 };  // end( class CursorSpace )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CursorList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized with cursors in a CursorSpace
template< class T >
class CursorList {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;     ///< the type of the items
 using Pos = long;   ///< the cursor of the cell before the item

 /// the empty list, in a space of its own
 CursorList( void );

 /// the empty list in the space S, shared with other lists
 explicit CursorList( std::shared_ptr< CursorSpace< T > > S );

 /// the list of the given items, in a space of its own
 CursorList( std::initializer_list< T > items );

 /// a copy of the items of other, in the same space
 CursorList( const CursorList & other );

 /// the cells of other, which is left empty in the same space
 CursorList( CursorList && other ) noexcept;

 CursorList & operator=( CursorList other ) noexcept;

 ~CursorList();

 void swap( CursorList & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool empty( void ) const;

 /// true on \f$ pos_{-1} \f$ = -1 and on \f$ pos_n \f$
 [[nodiscard]] bool finished( Pos p ) const;

 /// \f$ pos_0 \f$, i.e., 0
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item, \f$ O( n ) \f$
 [[nodiscard]] Pos tail( void ) const;

 /// \pre p is not \f$ pos_n \f$
 [[nodiscard]] Pos next( Pos p ) const;

 /// \f$ O( n ) \f$
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts v in position p, the new item taking the position p
 /// \pre p is not \f$ pos_{-1} \f$
 /// \post read( p ) is v
 Pos insert( Pos p , const T & v );

 /// removes the item in position p, whose successor takes the position p
 /// \pre p holds an item
 Pos remove( Pos p );

 /// \pre p holds an item
 [[nodiscard]] const T & read( Pos p ) const;

 /// \pre p holds an item
 /// \post read( p ) is v
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the same items in the same order
 [[nodiscard]] bool operator==( const CursorList & o ) const;

 /// the space of the list
 [[nodiscard]] const CursorSpace< T > & space( void ) const;

 /// the cursor of the first cell, 0 if the list is empty
 [[nodiscard]] long first( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a forward iterator holds the space and the index of the cell of its
 // item, never a reference to the cell, since take() in any list of the
 // space may move the cells; the end is the index 0

 /// the iterator, on items const if Const
 template< bool Const >
 class Iterator {
  using Space = std::conditional_t< Const , const CursorSpace< T > ,
                                    CursorSpace< T > >;

  public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = std::conditional_t< Const , const T & , T & >;
  using pointer = std::conditional_t< Const , const T * , T * >;

  Iterator( void ) = default;
  Iterator( Space * S , long c ) : f_S( S ) , f_c( c ) {}

  /// the const iterator from the other one
  template< bool C > requires ( Const && ! C )
  Iterator( const Iterator< C > & o ) : f_S( o.f_S ) , f_c( o.f_c ) {}

  reference operator*( void ) const { return( ( *f_S )[ f_c ].value ); }
  pointer operator->( void ) const { return( &**this ); }
  Iterator & operator++( void ) {
   f_c = ( *f_S )[ f_c ].next;
   return( *this );
   }
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  bool operator==( const Iterator & o ) const = default;

  private:
  Space * f_S = nullptr;   ///< the space of the cells
  long f_c = 0;            ///< the cell of the item, 0 at the end

  template< bool > friend class Iterator;
  };

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;
 using iterator = Iterator< false >;
 using const_iterator = Iterator< true >;

 [[nodiscard]] iterator begin( void ) {
  return( iterator( f_S.get() , f_first ) );
  }
 [[nodiscard]] iterator end( void ) { return( iterator( f_S.get() , 0 ) ); }
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_S.get() , f_first ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_S.get() , 0 ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the list of the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 CursorList( I first , S last );

 /// v at the front, for std::front_inserter
 void push_front( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// the link that leads to the cell of the item in position p
 [[nodiscard]] long & link( Pos p );

 /// the cursor of the cell of the item in position p
 [[nodiscard]] long cell( Pos p ) const;

 std::shared_ptr< CursorSpace< T > > f_S;   ///< the space
 long f_first = 0;                          ///< the first cell, 0 if none

 };  // end( class CursorList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/CursorList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CursorList.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CursorList.h --------------------------*/
/*--------------------------------------------------------------------------*/
