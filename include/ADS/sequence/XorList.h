/*--------------------------------------------------------------------------*/
/*----------------------------- File XorList.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The XOR list of the discussion of chapter 3 of Morin's book, beyond the
 * book: a doubly linked list whose cells keep one word of links instead of
 * two, the exclusive or \f$ \mathit{prev} \oplus \mathit{next} \f$ of the
 * addresses of their two neighbours (nil at the ends being 0), so that a
 * walk that knows the cell it comes from finds the next one by an
 * exclusive or, \f$ \mathit{next} = \mathit{link} \oplus \mathit{prev} \f$,
 * in either direction. A cell takes one pointer less than in a `List` [see
 * `List.h`], and `reverse()` takes \f$ O( 1 ) \f$ time, by exchanging the
 * first and the last cell, since the links read the same both ways. The
 * price is that a cell alone is no position: an iterator holds the cell
 * and the one before it, and no cell can be removed from the middle given
 * only its address. The four operations of a double ended queue take
 * \f$ O( 1 ) \f$ time; the cells come from an `ObjectPool` [see
 * `ObjectPool.h`], as those of `LinkedQueue`. The addresses are turned
 * into integers of the type `std::uintptr_t` and back, which C++ defines
 * for the addresses of live objects.
 *
 * The exclusive or that links the cells also swaps two integers without a
 * third (exercise 3.19 of Morin's book): \f$ a \gets a \oplus b \f$,
 * \f$ b \gets a \oplus b \f$, \f$ a \gets a \oplus b \f$ leaves
 * \f$ b = ( a \oplus b ) \oplus b = a \f$ and then
 * \f$ a = ( a \oplus b ) \oplus a = b \f$, since \f$ \oplus \f$ is
 * associative and \f$ x \oplus x = 0 \f$; `xorSwap()` does it, and it fails
 * (zeroes both) on two references to the same integer.
 *
 * The contracts [see `Contract.h`]: the removals and `front()` and
 * `back()` need a list that is not empty; the invariant is that the walk
 * from the first cell ends at the last one after `size()` cells, an
 * \f$ O( n ) \f$ check. The operations are defined in `XorList.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, section 3.4 and
 *      exercise 3.19
 * \see P. Sinha, A memory-efficient doubly linked list, Linux Journal 129,
 *      2004
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

#ifndef __ADS_XorList
 #define __ADS_XorList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/ObjectPool.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS XorList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a doubly linked list with one word of links per cell, the exclusive or
/// of the two neighbours
template< class T >
class XorList {

 struct Cell;

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = T &;                    ///< a reference to an item
 using const_reference = const T &;        ///< a constant reference

 /// the empty list
 XorList( void ) = default;

 /// the items of `l`, in order
 XorList( std::initializer_list< T > l );

 /// a copy of `o`, cell by cell
 XorList( const XorList & o );

 /// the cells of `o`, which is left empty
 XorList( XorList && o ) noexcept;

 /// the items of `o`, by copy and swap
 XorList & operator=( XorList o ) noexcept;

 /// destroys the items
 ~XorList();

 /// exchanges the cells with those of `o`
 void swap( XorList & o ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the first item
 /// \pre the list is not empty
 [[nodiscard]] const T & front( void ) const;

 /// the last item
 /// \pre the list is not empty
 [[nodiscard]] const T & back( void ) const;

 /// \f$ x \f$ at the front, \f$ O( 1 ) \f$
 ///
 /// In pseudocode (Sinha, beyond the book), \f$ h \f$ the first cell:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{pushFront}( x ) \cr
 /// 2 & \quad c \gets \text{a new cell of } x;\ c.\mathit{link} \gets h \cr
 /// 3 & \quad \textbf{if}\ h \ne \textbf{nil}\ \textbf{then}
 ///     \ h.\mathit{link} \gets h.\mathit{link} \oplus c\ \textbf{else}\ t
 ///     \gets c \cr
 /// 4 & \quad h \gets c
 /// \end{array}}
 /// \f]
 /// \post one item more, \f$ x \f$ first
 void push_front( const T & x );

 /// \f$ x \f$ at the back, \f$ O( 1 ) \f$, the mirror of `push_front()`
 /// \post one item more, \f$ x \f$ last
 void push_back( const T & x );

 /// removes the first item, \f$ O( 1 ) \f$
 ///
 /// In pseudocode (Sinha, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{popFront}() \cr
 /// 2 & \quad c \gets h;\ h \gets c.\mathit{link}\quad \triangleright
 ///     \ \text{its only neighbour} \cr
 /// 3 & \quad \textbf{if}\ h \ne \textbf{nil}\ \textbf{then}
 ///     \ h.\mathit{link} \gets h.\mathit{link} \oplus c\ \textbf{else}\ t
 ///     \gets \textbf{nil} \cr
 /// 4 & \quad \text{free } c
 /// \end{array}}
 /// \f]
 /// \pre the list is not empty
 /// \post one item less
 void pop_front( void );

 /// removes the last item, \f$ O( 1 ) \f$, the mirror of `pop_front()`
 /// \pre the list is not empty
 /// \post one item less
 void pop_back( void );

 /// the order of the items reversed in \f$ O( 1 ) \f$, by exchanging the
 /// first and the last cell
 void reverse( void ) { std::swap( f_head , f_tail ); }

 /// removes all the items
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const XorList & o ) const;

 /// the bidirectional iterator, the cell and the one before it, on items
 /// const if `Const`
 template< bool Const >
 class Iterator {
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

  /// on the cell \f$ c \f$, coming from \f$ p \f$
  Iterator( Cell * p , Cell * c ) : f_p( p ) , f_c( c ) {}

  /// the constant iterator from the mutable one
  template< bool K > requires ( Const && ! K )
  Iterator( const Iterator< K > & o ) : f_p( o.f_p ) , f_c( o.f_c ) {}

  /// the item
  reference operator*( void ) const { return( f_c->value ); }
  /// the address of the item
  pointer operator->( void ) const { return( &f_c->value ); }
  /// to the next cell, \f$ \mathit{link} \oplus \mathit{prev} \f$
  Iterator & operator++( void ) {
   Cell * n = other( f_c , f_p );
   f_p = f_c;
   f_c = n;
   return( *this );
   }
  /// to the previous cell
  Iterator & operator--( void ) {
   Cell * p = other( f_p , f_c );
   f_c = f_p;
   f_p = p;
   return( *this );
   }
  /// to the next item, returning the iterator before the step
  Iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// to the previous item, returning the iterator before the step
  Iterator operator--( int ) { auto t = *this; --*this; return( t ); }
  /// equal on the same cell
  bool operator==( const Iterator & o ) const {
   return( ( f_c == o.f_c ) && ( f_p == o.f_p ) );
   }

  private:
  template< bool > friend class Iterator;
  Cell * f_p = nullptr;   ///< the cell before
  Cell * f_c = nullptr;   ///< the cell, nil past the last one
  };  // end( class Iterator )

 using iterator = Iterator< false >;        ///< the iterator on the items
 using const_iterator = Iterator< true >;   ///< on the constant items

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) {
  return( iterator( nullptr , f_head ) );
  }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator( f_tail , nullptr ) ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( nullptr , f_head ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_tail , nullptr ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a cell: the exclusive or of the addresses of its neighbours, and the
 /// item
 struct Cell {
  std::uintptr_t link;   ///< prev XOR next, nil being 0
  T value;               ///< the item
  };

 /// the address of \f$ a \f$ as an integer
 static std::uintptr_t word( const Cell * a ) {
  return( reinterpret_cast< std::uintptr_t >( a ) );
  }

 /// the neighbour of \f$ c \f$ that is not \f$ p \f$
 static Cell * other( const Cell * c , const Cell * p ) {
  return( reinterpret_cast< Cell * >( c->link ^ word( p ) ) );
  }

 /// the walk from the first cell ends at the last after \f$ n \f$ cells
 [[nodiscard]] bool walks( void ) const;

 Cell * f_head = nullptr;    ///< the first cell
 Cell * f_tail = nullptr;    ///< the last cell
 std::size_t f_n = 0;        ///< the number of items
 ObjectPool< Cell > f_pool;  ///< the cells

 };  // end( class XorList )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ a \f$ and \f$ b \f$ swapped by three exclusive ors and no third
/// variable (exercise 3.19 of Morin's book)
///
/// In pseudocode (Morin, exercise 3.19):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{xorSwap}( a , b ) \cr
/// 2 & \quad a \gets a \oplus b;\ b \gets a \oplus b;\ a \gets a \oplus b
/// \end{array}}
/// \f]
/// \pre \f$ a \f$ and \f$ b \f$ are not the same object
template< std::unsigned_integral U >
constexpr void xorSwap( U & a , U & b );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/XorList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* XorList.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File XorList.h ---------------------------*/
/*--------------------------------------------------------------------------*/
