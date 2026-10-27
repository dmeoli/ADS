/*--------------------------------------------------------------------------*/
/*---------------------------- File RingDeque.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A double ended queue on a circular vector that grows, beyond the book: the
 * circular vector of `ArrayQueue` [see `ArrayQueue.h`] whose capacity is a
 * power of 2, so that a place is reduced by a mask instead of a division,
 * and that doubles when full, the items moved in order to the front of the
 * new vector (\f$ O( 1 ) \f$ amortized time per insertion). The four
 * operations at the ends take \f$ O( 1 ) \f$ time, an item is read by its
 * rank in \f$ O( 1 ) \f$, and the items lie in at most two contiguous pieces,
 * which the moves and the visits run through at the speed of the memory. The
 * doubling of a deque of \f$ 2^j \f$ places moves its \f$ 2^j \f$ items, so
 * that \f$ n \f$ insertions move fewer than \f$ 2 n \f$ items in all, and the
 * places are fewer than \f$ 2 n + 8 \f$ for \f$ n \f$ items at the largest.
 *
 * Compared with `std::deque`, which keeps the items in blocks of 512 bytes
 * reached by a map of pointers, there is no indirection and no allocation
 * but at the doublings; compared with `boost::circular_buffer`, the vector
 * grows by itself; compared with `boost::container::devector`, the items
 * freed at one end serve the insertions at the other, so that a queue used
 * as a FIFO never moves its items. The places are raw memory, the items
 * being built in them when inserted and destroyed when removed.
 *
 * The contracts [see `Contract.h`]: `front()`, `back()` and the removals
 * need a deque that is not empty, and `operator[]` a rank below `size()`;
 * the invariant is that the capacity is 0 or a power of 2 at least
 * `size()`. The operations are defined in `RingDeque.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.2.2 (the circular queue)
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

#ifndef __ADS_RingDeque
 #define __ADS_RingDeque
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <bit>
#include <compare>
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

#if defined( __GNUC__ ) || defined( __clang__ )
 /// the operations at the ends, inlined even where the compiler would not
 #define ADS_RING_INLINE inline __attribute__(( always_inline ))
 /// the growth, never inlined
 #define ADS_RING_NOINLINE __attribute__(( noinline ))
#elif defined( _MSC_VER )
 /// the operations at the ends, inlined even where the compiler would not
 #define ADS_RING_INLINE __forceinline
 /// the growth, never inlined
 #define ADS_RING_NOINLINE __declspec( noinline )
#else
 /// the operations at the ends, inlined where the compiler chooses
 #define ADS_RING_INLINE inline
 /// the growth, inlined where the compiler chooses
 #define ADS_RING_NOINLINE
#endif

namespace ADS {

namespace detail {

/// the random-access iterator of a circular vector of \f$ 2^k \f$ places,
/// the place of the rank \f$ i \f$ being `( head + i ) & mask`
template< class T , bool Const >
class MaskIterator {
 using Item = std::conditional_t< Const , const T , T >;

 public:
 /// the category of the iterator, random access
 using iterator_category = std::random_access_iterator_tag;
 /// the type of the items
 using value_type = T;
 /// the type of the distances
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = Item &;
 /// a pointer to an item
 using pointer = Item *;

 /// an iterator on nothing
 MaskIterator( void ) = default;
 MaskIterator( Item * A , std::size_t head , std::size_t mask ,
               std::size_t i )
  : f_A( A ) , f_head( head ) , f_mask( mask ) , f_i( i ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 MaskIterator( const MaskIterator< T , K > & o )
  : f_A( o.f_A ) , f_head( o.f_head ) , f_mask( o.f_mask ) , f_i( o.f_i ) {}

 /// the item
 reference operator*( void ) const {
  return( f_A[ ( f_head + f_i ) & f_mask ] );
  }
 /// the address of the item
 pointer operator->( void ) const { return( &**this ); }
 /// the item \f$ k \f$ places on
 reference operator[]( difference_type k ) const {
  return( f_A[ ( f_head + f_i + std::size_t( k ) ) & f_mask ] );
  }

 /// to the next item
 MaskIterator & operator++( void ) { ++f_i; return( *this ); }
 /// to the previous item
 MaskIterator & operator--( void ) { --f_i; return( *this ); }
 /// to the next item, returning the iterator before the step
 MaskIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 /// to the previous item, returning the iterator before the step
 MaskIterator operator--( int ) { auto t = *this; --f_i; return( t ); }
 /// \f$ k \f$ items on
 MaskIterator & operator+=( difference_type k ) {
  f_i += std::size_t( k );
  return( *this );
  }
 /// \f$ k \f$ items back
 MaskIterator & operator-=( difference_type k ) {
  f_i -= std::size_t( k );
  return( *this );
  }
 friend MaskIterator operator+( MaskIterator a , difference_type k ) {
  return( a += k );
  }
 friend MaskIterator operator+( difference_type k , MaskIterator a ) {
  return( a += k );
  }
 friend MaskIterator operator-( MaskIterator a , difference_type k ) {
  return( a -= k );
  }
 friend difference_type operator-( const MaskIterator & a ,
                                   const MaskIterator & b ) {
  return( difference_type( a.f_i - b.f_i ) );
  }
 bool operator==( const MaskIterator & o ) const { return( f_i == o.f_i ); }
 std::strong_ordering operator<=>( const MaskIterator & o ) const {
  return( f_i <=> o.f_i );
  }

 private:
 template< class , bool > friend class MaskIterator;

 Item * f_A = nullptr;
 std::size_t f_head = 0 , f_mask = 0;
 std::size_t f_i = 0;          ///< the rank from the head
 };  // end( class MaskIterator )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS RingDeque -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a double ended queue on a circular vector of \f$ 2^k \f$ places that
/// doubles
template< class T >
class RingDeque {
/*--------------------------------------------------------------------------*/
 public:
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
 using iterator = detail::MaskIterator< T , false >;
 /// the iterator on the constant items
 using const_iterator = detail::MaskIterator< T , true >;
 /// the iterator on the items, backwards
 using reverse_iterator = std::reverse_iterator< iterator >;
 /// the iterator on the constant items, backwards
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 /// the empty deque, with no places
 RingDeque( void ) = default;

 /// the items of `l`, in order
 RingDeque( std::initializer_list< T > l );

 /// a copy of `o`
 RingDeque( const RingDeque & o );

 /// the vector of `o`, which is left empty with no capacity
 RingDeque( RingDeque && o ) noexcept;

 /// the items of `o`, by copy and swap
 RingDeque & operator=( RingDeque o ) noexcept;

 /// destroys the items and frees the places
 ~RingDeque();

 /// exchanges the vector with that of `o`
 void swap( RingDeque & o ) noexcept;

 /// true if there is no item, i.e., the two counters are equal
 [[nodiscard]] bool empty( void ) const { return( f_head == f_tail ); }

 /// the number of items, the difference of the two counters
 [[nodiscard]] std::size_t size( void ) const { return( f_tail - f_head ); }

 /// the places, 0 or a power of 2
 [[nodiscard]] std::size_t capacity( void ) const { return( f_mask + 1 ); }

 /// at least \f$ n \f$ places
 void reserve( std::size_t n );

 /// the item at the front
 /// \pre the deque is not empty
 [[nodiscard]] T & front( void );

 /// the item at the front, constant
 /// \pre the deque is not empty
 [[nodiscard]] const T & front( void ) const;

 /// the item at the back
 /// \pre the deque is not empty
 [[nodiscard]] T & back( void );

 /// the item at the back, constant
 /// \pre the deque is not empty
 [[nodiscard]] const T & back( void ) const;

 /// the item of rank \f$ i \f$ from the front, at the place
 /// \f$ ( h + i ) \mathbin{\&} ( m - 1 ) \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i );

 /// the item of rank \f$ i \f$ from the front, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// \f$ v \f$ at the back, `emplace_back( v )`
 ADS_RING_INLINE void push_back( const T & v ) { emplace_back( v ); }

 /// \f$ v \f$ at the back, moved
 ADS_RING_INLINE void push_back( T && v ) { emplace_back( std::move( v ) ); }

 /// \f$ v \f$ at the front, `emplace_front( v )`
 ADS_RING_INLINE void push_front( const T & v ) { emplace_front( v ); }

 /// \f$ v \f$ at the front, moved
 ADS_RING_INLINE void push_front( T && v ) {
  emplace_front( std::move( v ) );
  }

 /// the item built from `args` at the back, \f$ O( 1 ) \f$ amortized
 ///
 /// In pseudocode (Knuth, section 2.2.2, the circular queue, with the
 /// doubling of section 4.2 of the book), \f$ A[ 0 .. m - 1 ] \f$ the places,
 /// \f$ h \f$ and \f$ t \f$ the counters of the front and of the back:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{emplace\_back}( \mathit{args} ) \cr
 /// 2 & \quad \textbf{if}\ t - h = m\ \textbf{then}\quad \triangleright
 ///     \ \text{full} \cr
 /// 3 & \qquad B \gets \text{a vector of } \max( 2 m , 8 ) \text{ places} \cr
 /// 4 & \qquad \text{move } A[ h \bmod m .. ) \text{ and then the part }
 ///     \text{that wrapped around to } B[ 0 .. t - h ) \cr
 /// 5 & \qquad A \gets B;\ m \gets \max( 2 m , 8 );\ t \gets t - h;\ h
 ///     \gets 0 \cr
 /// 6 & \quad A[ t \bmod m ] \gets \text{the item built from }
 ///     \mathit{args};\ t \gets t + 1 \cr
 /// 7 & \quad \textbf{return}\ A[ ( t - 1 ) \bmod m ]
 /// \end{array}}
 /// \f]
 /// \post one item more, the new one at the back
 template< class ... Args >
 T & emplace_back( Args && ... args );

 /// the item built from `args` at the front, \f$ O( 1 ) \f$ amortized
 ///
 /// In pseudocode (as `emplace_back()`), the front moving back by one place:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{emplace\_front}( \mathit{args} ) \cr
 /// 2 & \quad \textbf{if}\ t - h = m\ \textbf{then}\ \text{double } A
 ///     \text{ as in } \mathrm{emplace\_back} \cr
 /// 3 & \quad h \gets h - 1;\ A[ h \bmod m ] \gets \text{the item built }
 ///     \text{from } \mathit{args} \cr
 /// 4 & \quad \textbf{return}\ A[ h \bmod m ]
 /// \end{array}}
 /// \f]
 /// \post one item more, the new one at the front
 template< class ... Args >
 T & emplace_front( Args && ... args );

 /// removes the item at the front, advancing \f$ h \f$
 /// \pre the deque is not empty
 /// \post one item less
 void pop_front( void );

 /// removes the item at the back, moving \f$ t \f$ back
 /// \pre the deque is not empty
 /// \post one item less
 void pop_back( void );

 /// removes all the items, the places kept
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const RingDeque & o ) const;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) {
  return( iterator( f_A , f_head , f_mask , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) {
  return( iterator( f_A , f_head , f_mask , size() ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_A , f_head , f_mask , 0 ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_A , f_head , f_mask , size() ) );
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

/*--------------------------------------------------------------------------*/
 private:
 /// the vector of \f$ 2 m \f$ places (8 if \f$ m = 0 \f$) with the
 /// \f$ n \f$ items of the vector \f$ A \f$ of \f$ m \f$ places from the
 /// place \f$ h \f$ moved to its front, \f$ A \f$ freed; out of
 /// line and with no access to the deque, which the loops of operations
 /// can then keep in registers, its address never given away
 ADS_RING_NOINLINE static T * relocate( T * A , std::size_t m ,
                                        std::size_t h , std::size_t n );

 /// the vector of twice the places, the items moved to its front in order
 void grow( void );

 /// \f$ v \f$, which may have been an item, at the back or at the front of a
 /// deque that grows first: the slow path, which only calls `relocate()`, kept
 /// out of the insertions by the hint of a cold branch
 T & growBack( T && v );
 T & growFront( T && v );

 /// the capacity 0 or a power of 2 at least size()
 [[nodiscard]] bool fits( void ) const;

 // the front and the back are counters modulo 2^64, apart by the size,
 // so that the insertions at the back and the removals from the front
 // change one each, two chains that do not wait for each other
 using Count = std::size_t;

 T * f_A = nullptr;            ///< the places, raw memory
 Count f_mask = Count( -1 );   ///< places - 1; -1 with none
 Count f_head = 0;             ///< the front, at the place head & mask
 Count f_tail = 0;             ///< the back + 1
 };  // end( class RingDeque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/RingDeque.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RingDeque.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RingDeque.h --------------------------*/
/*--------------------------------------------------------------------------*/
