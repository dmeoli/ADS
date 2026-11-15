/*--------------------------------------------------------------------------*/
/*--------------------------- File SmallVector.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A vector with room for \f$ N \f$ items inside itself, beyond the book: the
 * vector that doubles of section 4.2 [see `ArraySequence.h`] whose first
 * \f$ N \f$ items need no memory of the allocator, so that the many short
 * vectors of a structure (the children of a node, the arcs of a vertex, the
 * items of a bucket) cost no allocation each and lie next to the object that
 * holds them, at the price of \f$ N \f$ items of room in every vector. Past
 * \f$ N \f$ items the vector moves to the allocator and doubles there, as
 * `std::vector` does: \f$ n \f$ pushes cost fewer than \f$ 2 n \f$ moves of
 * items, and the space is at most \f$ 2 n + N \f$ items.
 *
 * A move takes the memory of the other vector in \f$ O( 1 ) \f$ time when its
 * items are on the allocator, and moves the items one by one, at most
 * \f$ N \f$, when they are inside it; the vector moved from is left empty.
 *
 * With `ADS_WITH_BOOST` defined (by the option of CMake of the same name,
 * which defines it whenever the headers of Boost 1.83 or later are found, by
 * the makefile, which does the same, or by hand), the items are those of
 * `boost::container::small_vector` with the growth factor 2 of its options,
 * the same vector with the same doubling, and `ADS_SMALLVECTOR_BOOST`
 * defined to 0 keeps the code of ADS; the interface, the iterators (the
 * pointers to the items), the costs and the contracts stay those above,
 * `small()` telling whether the room is still the inner one of Boost.
 *
 * The contracts [see `Contract.h`]: an index is below `size()`, and
 * `front()`, `back()` and `pop_back()` need a vector that is not empty; the
 * invariant is \f$ n \leq c \f$ and \f$ c \geq N \f$, \f$ c \f$ the
 * capacity. The operations are defined in `SmallVector.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, section 17.4 (dynamic
 *      tables)
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

#ifndef __ADS_SmallVector
 #define __ADS_SmallVector
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// the items of boost::container::small_vector, with ADS_WITH_BOOST
#if defined( ADS_WITH_BOOST ) && ! defined( ADS_SMALLVECTOR_BOOST )
 #define ADS_SMALLVECTOR_BOOST 1
#endif
#ifndef ADS_SMALLVECTOR_BOOST
 #define ADS_SMALLVECTOR_BOOST 0
#endif
#if ADS_SMALLVECTOR_BOOST
 #include <boost/container/options.hpp>
 #include <boost/container/small_vector.hpp>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SmallVector -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a vector with room for \f$ N \f$ items inside itself
template< class T , std::size_t N = 8 >
class SmallVector
{

 static_assert( N >= 1 , "room for one item at least" );

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
 /// a pointer to an item
 using pointer = T *;
 /// a constant pointer to an item
 using const_pointer = const T *;
 /// the iterator on the items
 using iterator = T *;
 /// the iterator on the constant items
 using const_iterator = const T *;
 /// the iterator on the items, backwards
 using reverse_iterator = std::reverse_iterator< iterator >;
 /// the iterator on the constant items, backwards
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 /// the empty vector, its items inside it
#if ! ADS_SMALLVECTOR_BOOST
 SmallVector( void ) noexcept : f_A( inner() ) { }
#else
 SmallVector( void ) noexcept { }
#endif

 /// \f$ n \f$ items made from \f$ x \f$
 explicit SmallVector( std::size_t n , const T & x = T() );

 /// the vector of the given items, in their order
 SmallVector( std::initializer_list< T > items );

 /// the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 SmallVector( I first , S last );

 /// a copy of `other`
 SmallVector( const SmallVector & other );

 /// the items of `other`, which is left empty
 SmallVector( SmallVector && other ) noexcept(
   std::is_nothrow_move_constructible_v< T > );

 /// a copy of `other`
 SmallVector & operator=( const SmallVector & other );

 /// the items of `other`, which is left empty
 SmallVector & operator=( SmallVector && other ) noexcept(
   std::is_nothrow_move_constructible_v< T > );

 /// destroys the items, and frees the memory of the allocator if any
 ~SmallVector();

 /// exchanges the items with `other`
 void swap( SmallVector & other );

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

#if ! ADS_SMALLVECTOR_BOOST
 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the room, \f$ c \f$ items, at least \f$ N \f$
 [[nodiscard]] std::size_t capacity( void ) const { return( f_cap ); }

 /// the items are inside the vector
 [[nodiscard]] bool small( void ) const { return( f_A == inner() ); }

 /// the vector of the items
 [[nodiscard]] T * data( void ) { return( f_A ); }
 /// the vector of the items, constant
 [[nodiscard]] const T * data( void ) const { return( f_A ); }
#else
 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_v.size() ); }

 /// the room, \f$ c \f$ items, at least \f$ N \f$
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_v.capacity() );
  }

 /// the items are inside the vector: the room is still the inner one,
 /// which Boost leaves only for a larger one
 [[nodiscard]] bool small( void ) const {
  return( f_v.capacity() == Vector::internal_capacity() );
  }

 /// the vector of the items
 [[nodiscard]] T * data( void ) { return( f_v.data() ); }
 /// the vector of the items, constant
 [[nodiscard]] const T * data( void ) const { return( f_v.data() ); }
#endif

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( data() ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( data() + size() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( data() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( data() + size() ); }
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

 /// the item at the index \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < size() );
  return( data()[ i ] );
  }
 /// the item at the index \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < size() );
  return( data()[ i ] );
  }

 /// the first item
 /// \pre the vector is not empty
 [[nodiscard]] T & front( void ) {
  ADS_PRE( ! empty() );
  return( data()[ 0 ] );
  }
 /// the first item, constant
 /// \pre the vector is not empty
 [[nodiscard]] const T & front( void ) const {
  ADS_PRE( ! empty() );
  return( data()[ 0 ] );
  }
 /// the last item
 /// \pre the vector is not empty
 [[nodiscard]] T & back( void ) {
  ADS_PRE( ! empty() );
  return( data()[ size() - 1 ] );
  }
 /// the last item, constant
 /// \pre the vector is not empty
 [[nodiscard]] const T & back( void ) const {
  ADS_PRE( ! empty() );
  return( data()[ size() - 1 ] );
  }

 /// \f$ x \f$ at the end, \f$ O( 1 ) \f$ amortized; \f$ x \f$ may be an
 /// item of the vector
 void push_back( const T & x ) { emplace_back( x ); }

 /// \f$ x \f$ at the end, moved, as `push_back()` above
 void push_back( T && x ) { emplace_back( std::move( x ) ); }

 /// an item made from the arguments at the end, which may refer to items
 /// of the vector, \f$ O( 1 ) \f$ amortized
 ///
 /// In pseudocode (section 4.2), \f$ A[ 0 .. c - 1 ] \f$ the room, inside
 /// the vector while \f$ c = N \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{emplace\_back}( \mathit{args} ) \cr
 /// 2 & \quad x \gets \text{the item of } \mathit{args}\quad \triangleright
 ///     \ \text{built before any move, since } \mathit{args}
 ///     \text{ may lie in } A \cr
 /// 3 & \quad \textbf{if}\ n = c\ \textbf{then}\quad \triangleright
 ///     \ \text{full: a block twice as large} \cr
 /// 4 & \qquad B \gets 2 c \text{ places from the allocator};\ \text{move }
 ///     A[ 0 .. n - 1 ] \text{ into } B \cr
 /// 5 & \qquad \text{give } A \text{ back unless it is the inner buffer};\ (
 ///     A , c ) \gets ( B , 2 c ) \cr
 /// 6 & \quad A[ n ] \gets x;\ n \gets n + 1;\ \textbf{return}\ A[ n - 1 ]
 /// \end{array}}
 /// \f]
 template< class... Args >
 T & emplace_back( Args &&... args );

 /// removes the last item
 /// \pre the vector is not empty
 void pop_back( void );

 /// room for \f$ n \f$ items
 void reserve( std::size_t n );

 /// \f$ n \f$ items, those added made from \f$ x \f$
 void resize( std::size_t n , const T & x = T() );

 /// removes the items, keeping the memory
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const SmallVector & o ) const {
  return( std::equal( begin() , end() , o.begin() , o.end() ) );
  }

/*--------------------------------------------------------------------------*/
 private:
#if ! ADS_SMALLVECTOR_BOOST
 T * inner( void ) { return( reinterpret_cast< T * >( f_room ) ); }
 const T * inner( void ) const {
  return( reinterpret_cast< const T * >( f_room ) );
  }

 /// the capacity \f$ c \f$, larger than `f_cap`, on the allocator, the
 /// items moved there
 void grow( std::size_t c );

 /// the items of `o` moved here, `o` emptied; this is empty and small
 void take( SmallVector & o );

 T * f_A;                ///< the items, inside or on the allocator
 std::size_t f_n = 0;    ///< the number of items
 std::size_t f_cap = N;  ///< the room
 /// the room for \f$ N \f$ items inside
 alignas( T ) unsigned char f_room[ N * sizeof( T ) ];
#else
 /// the growth factor 2 of the options of Boost
 using Doubling =
   boost::container::growth_factor< boost::container::growth_factor_100 >;

 /// the small vector of Boost, which doubles its room as this one does, on
 /// std::allocator, which aligns any T as the code of ADS does
 using Vector = boost::container::small_vector<
   T , N , std::allocator< T > ,
   typename boost::container::small_vector_options< Doubling >::type >;

 /// empty, the items inside, as a vector moved from is left
 void renew( void ) noexcept;

 Vector f_v;  ///< the items
#endif

 };  // end( class SmallVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/SmallVector.tpp"

/*--------------------------------------------------------------------------*/

#endif /* SmallVector.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SmallVector.h -------------------------*/
/*--------------------------------------------------------------------------*/
