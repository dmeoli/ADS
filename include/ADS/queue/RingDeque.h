/*--------------------------------------------------------------------------*/
/*---------------------------- File RingDeque.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A double ended queue on a circular vector that grows, beyond the book:
 * the circular vector of ArrayQueue [see ArrayQueue.h] whose capacity is a
 * power of 2, so that a place is reduced by a mask instead of a division,
 * and that doubles when full, the items moved in order to the front of the
 * new vector (O( 1 ) amortized time per insertion). The four operations
 * at the ends take O( 1 ) time, an item is read by its rank in O( 1 ), and
 * the items lie in at most two contiguous pieces, which the moves and the
 * visits run through at the speed of the memory.
 *
 * Compared with std::deque, which keeps the items in blocks of 512 bytes
 * reached by a map of pointers, there is no indirection and no allocation
 * but at the doublings; compared with boost::circular_buffer, the vector
 * grows by itself; compared with boost::container::devector, the items
 * freed at one end serve the insertions at the other, so that a queue
 * used as a FIFO never moves its items. The places are raw memory, the
 * items being built in them when inserted and destroyed when removed.
 *
 * The contracts [see Contract.h]: front(), back() and the removals need a
 * deque that is not empty, and operator[] a rank below size(); the
 * invariant is that the capacity is 0 or a power of 2 at least size().
 * The operations are defined in RingDeque.tpp.
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

/// the operations at the ends, inlined even where the compiler would not,
/// and the growth, never inlined
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_RING_INLINE inline __attribute__(( always_inline ))
 #define ADS_RING_NOINLINE __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_RING_INLINE __forceinline
 #define ADS_RING_NOINLINE __declspec( noinline )
#else
 #define ADS_RING_INLINE inline
 #define ADS_RING_NOINLINE
#endif

namespace ADS {

namespace detail {

/// the random-access iterator of a circular vector of 2^k places, the
/// place of the rank i being ( head + i ) & mask
template< class T , bool Const >
class MaskIterator {
 using Item = std::conditional_t< Const , const T , T >;

 public:
 using iterator_category = std::random_access_iterator_tag;
 using value_type = T;
 using difference_type = std::ptrdiff_t;
 using reference = Item &;
 using pointer = Item *;

 MaskIterator( void ) = default;
 MaskIterator( Item * A , std::size_t head , std::size_t mask ,
               std::size_t i )
  : f_A( A ) , f_head( head ) , f_mask( mask ) , f_i( i ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 MaskIterator( const MaskIterator< T , K > & o )
  : f_A( o.f_A ) , f_head( o.f_head ) , f_mask( o.f_mask ) , f_i( o.f_i ) {}

 reference operator*( void ) const {
  return( f_A[ ( f_head + f_i ) & f_mask ] );
  }
 pointer operator->( void ) const { return( &**this ); }
 reference operator[]( difference_type k ) const {
  return( f_A[ ( f_head + f_i + std::size_t( k ) ) & f_mask ] );
  }

 MaskIterator & operator++( void ) { ++f_i; return( *this ); }
 MaskIterator & operator--( void ) { --f_i; return( *this ); }
 MaskIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 MaskIterator operator--( int ) { auto t = *this; --f_i; return( t ); }
 MaskIterator & operator+=( difference_type k ) {
  f_i += std::size_t( k );
  return( *this );
  }
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

/// a double ended queue on a circular vector of 2^k places that doubles
template< class T >
class RingDeque {
/*--------------------------------------------------------------------------*/
 public:
 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;
 using iterator = detail::MaskIterator< T , false >;
 using const_iterator = detail::MaskIterator< T , true >;
 using reverse_iterator = std::reverse_iterator< iterator >;
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 RingDeque( void ) = default;

 /// the items of l, in order
 RingDeque( std::initializer_list< T > l );

 RingDeque( const RingDeque & o );

 /// the vector of o, which is left empty with no capacity
 RingDeque( RingDeque && o ) noexcept;

 RingDeque & operator=( RingDeque o ) noexcept;

 ~RingDeque();

 void swap( RingDeque & o ) noexcept;

 [[nodiscard]] bool empty( void ) const { return( f_head == f_tail ); }
 [[nodiscard]] std::size_t size( void ) const { return( f_tail - f_head ); }

 /// the places, 0 or a power of 2
 [[nodiscard]] std::size_t capacity( void ) const { return( f_mask + 1 ); }

 /// at least n places
 void reserve( std::size_t n );

 /// \pre the deque is not empty
 [[nodiscard]] T & front( void );
 [[nodiscard]] const T & front( void ) const;
 [[nodiscard]] T & back( void );
 [[nodiscard]] const T & back( void ) const;

 /// the item of rank i from the front
 /// \pre i < size()
 [[nodiscard]] T & operator[]( std::size_t i );
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 void push_back( const T & v ) { emplace_back( v ); }
 void push_back( T && v ) { emplace_back( std::move( v ) ); }
 void push_front( const T & v ) { emplace_front( v ); }
 void push_front( T && v ) { emplace_front( std::move( v ) ); }

 /// the item built from args at the back
 /// \post one item more, the new one at the back
 template< class ... Args >
 T & emplace_back( Args && ... args );

 /// the item built from args at the front
 /// \post one item more, the new one at the front
 template< class ... Args >
 T & emplace_front( Args && ... args );

 /// \pre the deque is not empty
 /// \post one item less
 void pop_front( void );

 /// \pre the deque is not empty
 /// \post one item less
 void pop_back( void );

 /// removes all the items, the places kept
 void clear( void );

 [[nodiscard]] bool operator==( const RingDeque & o ) const;

 [[nodiscard]] iterator begin( void ) {
  return( iterator( f_A , f_head , f_mask , 0 ) );
  }
 [[nodiscard]] iterator end( void ) {
  return( iterator( f_A , f_head , f_mask , size() ) );
  }
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_A , f_head , f_mask , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_A , f_head , f_mask , size() ) );
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

/*--------------------------------------------------------------------------*/
 private:
 /// a vector of 2 m places, m the current one or 8 if none, the items
 /// moved to its front in order
 void grow( void );

 /// v, which may have been an item, at the back or at the front of a deque
 /// that grows first; out of line, so that the insertions that need no
 /// growth are small enough to inline
 ADS_RING_NOINLINE T & growBack( T && v );
 ADS_RING_NOINLINE T & growFront( T && v );

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
