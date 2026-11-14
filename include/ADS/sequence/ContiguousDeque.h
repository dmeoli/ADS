/*--------------------------------------------------------------------------*/
/*------------------------- File ContiguousDeque.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A double ended queue with no modular arithmetic, beyond the book: the
 * variant of the `ArrayDeque` of Morin's book asked by its exercise 2.8, the
 * \f$ n \f$ items in order in one contiguous piece \f$ A[ h .. h + n - 1 ] \f$
 * of a vector of \f$ c \f$ places, so that the item of rank \f$ i \f$ is
 * \f$ A[ h + i ] \f$, the iterators are pointers and `data()` gives the
 * items to any function that wants an array. When the items reach an end
 * of the vector that an operation needs, `rebuild()` puts them in the
 * middle of a vector of \f$ 3 n + 8 \f$ places (the same vector, moved in
 * place by `memmove()`, when it is large enough and the items are
 * trivially copyable), so that at least \f$ n + 4 \f$ places are free on
 * each side: \f$ \Omega( n ) \f$ operations come before the next rebuild,
 * which costs \f$ O( n ) \f$, and each operation at an end takes
 * \f$ O( 1 ) \f$ amortized time, as in the `ArrayDeque`. An insertion or a
 * removal at the rank \f$ i \f$ moves the items of the shorter side, in
 * \f$ O( 1 + \min\{ i , n - i \} ) \f$ amortized time. A vector with more
 * than \f$ 9 n + 24 \f$ places after a removal is rebuilt with
 * \f$ 3 n + 8 \f$, so that the space stays \f$ O( n ) \f$. The rebuild is
 * not spread over the operations: keeping the one contiguous piece in use
 * while another is filled would need copies of the items and two pieces
 * at once (see \ref theory_sequence_ods).
 *
 * Compared with `RingDeque` [see `RingDeque.h`], it pays the rebuilds that
 * the circular vector avoids when a queue used as a FIFO runs along the
 * vector, and saves the mask of every access and the split of the items
 * into two pieces.
 *
 * The contracts [see `Contract.h`]: a rank is below `size()` (at most
 * `size()` for an insertion), and the removals and `front()` and `back()`
 * need a deque that is not empty; the invariant is
 * \f$ h + n \le c \f$. The operations are defined in
 * `ContiguousDeque.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercise 2.8
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

#ifndef __ADS_ContiguousDeque
 #define __ADS_ContiguousDeque
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ContiguousDeque --------------------------*/
/*--------------------------------------------------------------------------*/

/// a double ended queue whose items are contiguous, rebuilt in the middle
/// of its vector when they reach an end
template< class T >
class ContiguousDeque {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = T &;                    ///< a reference to an item
 using const_reference = const T &;        ///< a constant reference
 using iterator = T *;                     ///< the iterator on the items
 using const_iterator = const T *;         ///< on the constant items
 /// the iterator on the items, backwards
 using reverse_iterator = std::reverse_iterator< iterator >;
 /// the iterator on the constant items, backwards
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 /// the empty deque, with no places
 ContiguousDeque( void ) = default;

 /// the items of `l`, in order
 ContiguousDeque( std::initializer_list< T > l );

 /// a copy of `o`
 ContiguousDeque( const ContiguousDeque & o );

 /// the vector of `o`, which is left empty with no places
 ContiguousDeque( ContiguousDeque && o ) noexcept;

 /// the items of `o`, by copy and swap
 ContiguousDeque & operator=( ContiguousDeque o ) noexcept;

 /// destroys the items and frees the places
 ~ContiguousDeque();

 /// exchanges the vector with that of `o`
 void swap( ContiguousDeque & o ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number \f$ c \f$ of places
 [[nodiscard]] std::size_t capacity( void ) const { return( f_c ); }

 /// the item of rank \f$ i \f$, \f$ A[ h + i ] \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < f_n );
  return( f_A[ f_h + i ] );
  }

 /// the item of rank \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < f_n );
  return( f_A[ f_h + i ] );
  }

 /// the first item
 /// \pre the deque is not empty
 [[nodiscard]] T & front( void ) { ADS_PRE( f_n > 0 ); return( f_A[ f_h ] ); }

 /// the last item
 /// \pre the deque is not empty
 [[nodiscard]] T & back( void ) {
  ADS_PRE( f_n > 0 );
  return( f_A[ f_h + f_n - 1 ] );
  }

 /// the items, contiguous
 [[nodiscard]] T * data( void ) { return( f_A + f_h ); }

 /// the items, contiguous and constant
 [[nodiscard]] const T * data( void ) const { return( f_A + f_h ); }

 /// puts \f$ x \f$ at the rank \f$ i \f$, moving the items of the shorter
 /// side by one place, in \f$ O( 1 + \min\{ i , n - i \} ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Morin, exercise 2.8), \f$ A[ 0 .. c - 1 ] \f$ the vector:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ i < n / 2\ \textbf{then} \cr
 /// 3 & \qquad \textbf{if}\ h = 0\ \textbf{then}\ \mathrm{rebuild}() \cr
 /// 4 & \qquad A[ h - 1 .. h + i - 2 ] \gets A[ h .. h + i - 1 ];
 ///     \ h \gets h - 1 \cr
 /// 5 & \quad \textbf{else} \cr
 /// 6 & \qquad \textbf{if}\ h + n = c\ \textbf{then}\ \mathrm{rebuild}() \cr
 /// 7 & \qquad A[ h + i + 1 .. h + n ] \gets A[ h + i .. h + n - 1 ] \cr
 /// 8 & \quad A[ h + i ] \gets x;\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void insert( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$, moving the items of the shorter
 /// side by one place, in \f$ O( 1 + \min\{ i , n - i \} ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Morin, exercise 2.8):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad \textbf{if}\ i < n / 2\ \textbf{then}\ A[ h + 1 .. h + i ]
 ///     \gets A[ h .. h + i - 1 ];\ h \gets h + 1 \cr
 /// 3 & \quad \textbf{else}\ A[ h + i .. h + n - 2 ] \gets A[ h + i + 1 .. h
 ///     + n - 1 ] \cr
 /// 4 & \quad n \gets n - 1 \cr
 /// 5 & \quad \textbf{if}\ c > 9 n + 24\ \textbf{then}\ \mathrm{rebuild}()
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 /// \post one item less
 void erase( std::size_t i );

 /// \f$ x \f$ at the front, \f$ O( 1 ) \f$ amortized
 void push_front( const T & x );

 /// \f$ x \f$ at the back, \f$ O( 1 ) \f$ amortized
 void push_back( const T & x );

 /// removes the first item
 /// \pre the deque is not empty
 void pop_front( void ) { erase( 0 ); }

 /// removes the last item
 /// \pre the deque is not empty
 void pop_back( void ) { erase( f_n - 1 ); }

 /// removes all the items, the places kept
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const ContiguousDeque & o ) const {
  return( std::equal( begin() , end() , o.begin() , o.end() ) );
  }

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( f_A + f_h ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( f_A + f_h + f_n ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_A + f_h ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A + f_h + f_n );
  }
 /// the iterator on the last item, backwards
 [[nodiscard]] reverse_iterator rbegin( void ) {
  return( reverse_iterator( end() ) );
  }
 /// the iterator before the first item, backwards
 [[nodiscard]] reverse_iterator rend( void ) {
  return( reverse_iterator( begin() ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the items in the middle of a vector of \f$ 3 n + 8 \f$ places, the
 /// same one if it is large enough and the items are trivially copyable
 ///
 /// In pseudocode (Morin, exercise 2.8):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rebuild}() \cr
 /// 2 & \quad c' \gets 3 n + 8;\ h' \gets \lfloor ( c' - n ) / 2 \rfloor \cr
 /// 3 & \quad B[ h' .. h' + n - 1 ] \gets A[ h .. h + n - 1 ] \cr
 /// 4 & \quad A \gets B;\ c \gets c';\ h \gets h'
 /// \end{array}}
 /// \f]
 void rebuild( void );

 /// \f$ h + n \le c \f$
 [[nodiscard]] bool fits( void ) const { return( f_h + f_n <= f_c ); }

 T * f_A = nullptr;     ///< the places
 std::size_t f_c = 0;   ///< the number of places
 std::size_t f_h = 0;   ///< the place of the first item
 std::size_t f_n = 0;   ///< the number of items

 };  // end( class ContiguousDeque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/ContiguousDeque.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ContiguousDeque.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ContiguousDeque.h -----------------------*/
/*--------------------------------------------------------------------------*/
