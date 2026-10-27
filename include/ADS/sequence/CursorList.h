/*--------------------------------------------------------------------------*/
/*--------------------------- File CursorList.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see `ADT.h`] realized with cursors, beyond the book: the cells
 * of the lists are the places of a vector, a `CursorSpace`, and a link is
 * the index of the next cell, 0 being nil (the place 0 is never used).
 * Several lists can share one space, each being the cursor of its first
 * cell, and the free cells form a further list in the same vector, the free
 * list: `insert()` takes its first cell, `remove()` gives the cell back, and
 * a space whose free list is empty doubles its vector, the cursors staying
 * valid. This is how a language with no pointers, or a program that wants
 * its lists in one block of memory, realizes linked lists.
 *
 * As in `SinglyList` [see `SinglyList.h`], the position
 * \f$ \mathit{pos}_i \f$ is the cursor of the cell of \f$ s_{i-1} \f$ for
 * \f$ i > 0 \f$ and 0 for \f$ i = 0 \f$, so that `insert()` and `remove()`
 * find the link to change at hand and take \f$ O( 1 ) \f$ time (amortized,
 * for the growth of the space); \f$ \mathit{pos}_n \f$ is the cursor of the
 * last cell (0 if the list is empty) and \f$ \mathit{pos}_{-1} \f$ is -1.
 * `tail()` and `prev()` scan the list, in \f$ O( n ) \f$.
 *
 * The contracts [see `Contract.h`]: `read()`, `write()` and `remove()` need
 * a position that holds an item; `insert()` and `write()` ensure that
 * \f$ v \f$ is read back in its position; the invariant of the space is that
 * the free cells and the cells in use are all the places but 0, the free
 * list having no cycle. A list made with no space has a space of its own;
 * the lists of a space share it, and the space lives as long as one of them
 * does. The operations are defined in `CursorList.tpp`.
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

/// the vector of the cells of some `CursorList`, with its free list
template< class T >
class CursorSpace {

/*--------------------------------------------------------------------------*/
 public:

 /// a cell: the item and the cursor of the next cell, 0 for nil
 struct Cell {
  T value;    ///< the item
  long next;  ///< the cursor of the next cell, 0 for nil
  };

 /// a space of \f$ \mathit{dim} \f$ free cells, the places
 /// \f$ 1 , \ldots , \mathit{dim} \f$
 /// \pre \f$ \mathit{dim} \ge 1 \f$
 explicit CursorSpace( long dim = 8 );

 /// the cell at the cursor \f$ c \f$
 /// \pre \f$ 1 \le c \le \mathrm{capacity}() \f$
 [[nodiscard]] Cell & operator[]( long c );

 /// the cell at the cursor \f$ c \f$, constant
 /// \pre \f$ 1 \le c \le \mathrm{capacity}() \f$
 [[nodiscard]] const Cell & operator[]( long c ) const;

 /// takes the first free cell, doubling the vector if there is none
 ///
 /// In pseudocode (the free list of Aho, Hopcroft and Ullman, section 2.2,
 /// and of Knuth, section 2.2.3), \f$ A[ 1 .. c ] \f$ the cells:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{take}() \cr
 /// 2 & \quad \textbf{if}\ \mathit{free} = 0\ \textbf{then}\quad
 ///     \triangleright\ \text{no free cell: twice the places} \cr
 /// 3 & \qquad \text{extend } A \text{ to } A[ 1 .. 2 c ] \cr
 /// 4 & \qquad \textbf{for}\ k \gets c + 1\ \textbf{to}\ 2 c - 1
 ///     \ \textbf{do}\ A[ k ].\mathit{next} \gets k + 1 \cr
 /// 5 & \qquad A[ 2 c ].\mathit{next} \gets 0;\ \mathit{free} \gets c + 1;
 ///     \ c \gets 2 c \cr
 /// 6 & \quad k \gets \mathit{free};\ \mathit{free} \gets A[ k
 ///     ].\mathit{next};\ A[ k ].\mathit{next} \gets 0 \cr
 /// 7 & \quad \textbf{return}\ k
 /// \end{array}}
 /// \f]
 /// \post the result is a cell in use
 long take( void );

 /// gives the cell \f$ c \f$ back to the free list
 ///
 /// In pseudocode (as `take()`), the cell pushed on the free list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{give}( k ) \cr
 /// 2 & \quad A[ k ].\mathit{next} \gets \mathit{free};\ \mathit{free}
 ///     \gets k
 /// \end{array}}
 /// \f]
 /// \pre \f$ c \f$ is a cell in use
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

 std::vector< Cell > f_A;   ///< the cells, `f_A[ 0 ]` never used
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

 /// the empty list in the space \f$ S \f$, shared with other lists
 explicit CursorList( std::shared_ptr< CursorSpace< T > > S );

 /// the list of the given items, in a space of its own
 CursorList( std::initializer_list< T > items );

 /// a copy of the items of `other`, in the same space
 CursorList( const CursorList & other );

 /// the cells of `other`, which is left empty in the same space
 CursorList( CursorList && other ) noexcept;

 /// the items of `other`, by copy and swap
 CursorList & operator=( CursorList other ) noexcept;

 /// gives the cells back to the free list of the space
 ~CursorList();

 /// exchanges the cells, and the spaces, with `other`
 void swap( CursorList & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const;

 /// true on \f$ \mathit{pos}_{-1} = -1 \f$ and on \f$ \mathit{pos}_n \f$
 [[nodiscard]] bool finished( Pos p ) const;

 /// \f$ \mathit{pos}_0 \f$, i.e., 0
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item, \f$ O( n ) \f$, by a scan as in
 /// `SinglyList::tail()`
 [[nodiscard]] Pos tail( void ) const;

 /// the position that follows \f$ p \f$
 /// \pre \f$ p \f$ is not \f$ \mathit{pos}_n \f$
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position that precedes \f$ p \f$, \f$ O( n ) \f$, by a scan as in
 /// `SinglyList::prev()`
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts \f$ v \f$ in the position \f$ p \f$, the new item taking the
 /// position \f$ p \f$
 ///
 /// In pseudocode (Aho, Hopcroft and Ullman, section 2.2), \f$ S \f$ the
 /// space and \f$ \ell \f$ the link that leads to the item in \f$ p \f$,
 /// i.e., \f$ S[ p ].\mathit{next} \f$, or the first cell if
 /// \f$ p = 0 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( p , v ) \cr
 /// 2 & \quad k \gets S.\mathrm{take}();\ S[ k ].\mathit{value} \gets v \cr
 /// 3 & \quad S[ k ].\mathit{next} \gets \ell;\ \ell \gets k \cr
 /// 4 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ is not \f$ \mathit{pos}_{-1} \f$
 /// \post `read( p )` is \f$ v \f$
 Pos insert( Pos p , const T & v );

 /// removes the item in the position \f$ p \f$, whose successor takes the
 /// position \f$ p \f$
 ///
 /// In pseudocode (Aho, Hopcroft and Ullman, section 2.2), \f$ \ell \f$ as
 /// in `insert()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( p ) \cr
 /// 2 & \quad k \gets \ell;\ \ell \gets S[ k ].\mathit{next};
 ///     \ S.\mathrm{give}( k ) \cr
 /// 3 & \quad \textbf{return}\ p
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
 [[nodiscard]] bool operator==( const CursorList & o ) const;

 /// the space of the list
 [[nodiscard]] const CursorSpace< T > & space( void ) const;

 /// the cursor of the first cell, 0 if the list is empty
 [[nodiscard]] long first( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a forward iterator holds the space and the index of the cell of its
 // item, never a reference to the cell, since take() in any list of the
 // space may move the cells; the end is the index 0

 /// the iterator, on items const if `Const`
 template< bool Const >
 class Iterator {
  using Space = std::conditional_t< Const , const CursorSpace< T > ,
                                    CursorSpace< T > >;

  public:
  /// the category of the iterator, forward
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;                     ///< the type of the items
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// a reference to an item, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an item, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// an iterator on no space
  Iterator( void ) = default;

  /// on the item of the cell \f$ c \f$ of the space \f$ S \f$, the end
  /// if \f$ c = 0 \f$
  Iterator( Space * S , long c ) : f_S( S ) , f_c( c ) {}

  /// the const iterator from the other one
  template< bool C > requires ( Const && ! C )
  Iterator( const Iterator< C > & o ) : f_S( o.f_S ) , f_c( o.f_c ) {}

  /// the item
  reference operator*( void ) const { return( ( *f_S )[ f_c ].value ); }
  /// the address of the item
  pointer operator->( void ) const { return( &**this ); }
  /// to the next cell
  Iterator & operator++( void ) {
   f_c = ( *f_S )[ f_c ].next;
   return( *this );
   }
  /// to the next cell, returning the iterator before the step
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// equal on the same cell of the same space
  bool operator==( const Iterator & o ) const = default;

  private:
  Space * f_S = nullptr;   ///< the space of the cells
  long f_c = 0;            ///< the cell of the item, 0 at the end

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
 [[nodiscard]] iterator begin( void ) {
  return( iterator( f_S.get() , f_first ) );
  }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator( f_S.get() , 0 ) ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_S.get() , f_first ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_S.get() , 0 ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the list of the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 CursorList( I first , S last );

 /// \f$ v \f$ at the front, for `std::front_inserter`
 void push_front( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// the link that leads to the cell of the item in the position \f$ p \f$
 [[nodiscard]] long & link( Pos p );

 /// the cursor of the cell of the item in the position \f$ p \f$
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
