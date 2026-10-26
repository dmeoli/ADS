/*--------------------------------------------------------------------------*/
/*--------------------------- File SmallVector.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A vector with room for N items inside itself, beyond the book: the
 * vector that doubles of section 4.2 [see ArraySequence.h] whose first N
 * items need no memory of the allocator, so that the many short vectors
 * of a structure (the children of a node, the arcs of a vertex, the
 * items of a bucket) cost no allocation each and lie next to the object
 * that holds them, at the price of N items of room in every vector. Past
 * N items the vector moves to the allocator and doubles there, as
 * std::vector does: n pushes cost fewer than 2 n moves of items, and the
 * space is at most 2 n + N items.
 *
 * A move takes the memory of the other vector in \f$ O( 1 ) \f$ time when its
 * items are on the allocator, and moves the items one by one, at most N,
 * when they are inside it; the vector moved from is left empty.
 *
 * The contracts [see Contract.h]: an index is below size(), and front(),
 * back() and pop_back() need a vector that is not empty; the invariant is
 * \f$ size \leq capacity \f$ and \f$ capacity \geq N \f$. The operations
 * are defined in SmallVector.tpp.
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

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SmallVector -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a vector with room for N items inside itself
template< class T , std::size_t N = 8 >
class SmallVector {

 static_assert( N >= 1 , "room for one item at least" );

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;
 using pointer = T *;
 using const_pointer = const T *;
 using iterator = T *;
 using const_iterator = const T *;
 using reverse_iterator = std::reverse_iterator< iterator >;
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 SmallVector( void ) noexcept : f_A( inner() ) {}

 /// n items made from x
 explicit SmallVector( std::size_t n , const T & x = T() );

 SmallVector( std::initializer_list< T > items );

 /// the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 SmallVector( I first , S last );

 SmallVector( const SmallVector & other );

 /// the items of other, which is left empty
 SmallVector( SmallVector && other )
  noexcept( std::is_nothrow_move_constructible_v< T > );

 SmallVector & operator=( const SmallVector & other );

 SmallVector & operator=( SmallVector && other )
  noexcept( std::is_nothrow_move_constructible_v< T > );

 ~SmallVector();

 void swap( SmallVector & other );

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }
 [[nodiscard]] std::size_t capacity( void ) const { return( f_cap ); }

 /// the items are inside the vector
 [[nodiscard]] bool small( void ) const { return( f_A == inner() ); }

 [[nodiscard]] T * data( void ) { return( f_A ); }
 [[nodiscard]] const T * data( void ) const { return( f_A ); }

 [[nodiscard]] iterator begin( void ) { return( f_A ); }
 [[nodiscard]] iterator end( void ) { return( f_A + f_n ); }
 [[nodiscard]] const_iterator begin( void ) const { return( f_A ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_A + f_n ); }
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

 /// \pre i < size()
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < f_n );
  return( f_A[ i ] );
  }
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < f_n );
  return( f_A[ i ] );
  }

 /// \pre the vector is not empty
 [[nodiscard]] T & front( void ) { ADS_PRE( f_n > 0 ); return( f_A[ 0 ] ); }
 [[nodiscard]] const T & front( void ) const {
  ADS_PRE( f_n > 0 );
  return( f_A[ 0 ] );
  }
 [[nodiscard]] T & back( void ) {
  ADS_PRE( f_n > 0 );
  return( f_A[ f_n - 1 ] );
  }
 [[nodiscard]] const T & back( void ) const {
  ADS_PRE( f_n > 0 );
  return( f_A[ f_n - 1 ] );
  }

 /// x at the end, \f$ O( 1 ) \f$ amortized; x may be an item of the vector
 void push_back( const T & x ) { emplace_back( x ); }
 void push_back( T && x ) { emplace_back( std::move( x ) ); }

 /// an item made from the arguments at the end, which may refer to items
 /// of the vector
 template< class... Args >
 T & emplace_back( Args &&... args );

 /// \pre the vector is not empty
 void pop_back( void );

 /// room for n items
 void reserve( std::size_t n );

 /// n items, those added made from x
 void resize( std::size_t n , const T & x = T() );

 /// removes the items, keeping the memory
 void clear( void );

 [[nodiscard]] bool operator==( const SmallVector & o ) const {
  return( std::equal( begin() , end() , o.begin() , o.end() ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 T * inner( void ) { return( reinterpret_cast< T * >( f_room ) ); }
 const T * inner( void ) const {
  return( reinterpret_cast< const T * >( f_room ) );
  }

 /// the capacity c > f_cap on the allocator, the items moved there
 void grow( std::size_t c );

 /// the items of o moved here, o emptied; this is empty and small
 void take( SmallVector & o );

 T * f_A;                      ///< the items, inside or on the allocator
 std::size_t f_n = 0;          ///< the number of items
 std::size_t f_cap = N;        ///< the room
 alignas( T ) unsigned char f_room[ N * sizeof( T ) ];   ///< N items

 };  // end( class SmallVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/SmallVector.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SmallVector.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SmallVector.h -------------------------*/
/*--------------------------------------------------------------------------*/
