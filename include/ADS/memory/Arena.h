/*--------------------------------------------------------------------------*/
/*------------------------------ File Arena.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An arena of objects allocated in blocks, beyond the book: make() builds
 * an object in the current block, a new block of n objects is taken when
 * it is full, and all the objects die with the arena. Allocating costs a
 * pointer bump instead of a call to the allocator, the objects made one
 * after another are next to each other in memory, and their addresses
 * never change, which the pointers among them (the parents of the nodes of
 * a search) rely on; the price is that an object cannot be freed by
 * itself. The blocks come from the allocator A, which may give, e.g., huge
 * pages.
 *
 * The contracts [see Contract.h]: a block holds at least one object; the
 * invariant is that size() objects have been made. The operations are
 * defined in Arena.tpp.
 *
 * \see D. R. Hanson, Fast allocation and deallocation of memory based on
 *      object lifetimes, Software: Practice and Experience 20(1), 5-12,
 *      1990, doi:10.1002/spe.4380200104
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

#ifndef __ADS_Arena
 #define __ADS_Arena
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <compare>
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Arena --------------------------------*/
/*--------------------------------------------------------------------------*/

/// an arena of objects of type T in blocks of n
template< class T , class A = std::allocator< T > >
class Arena {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty arena whose blocks hold n objects
 /// \pre n >= 1
 explicit Arena( std::size_t n = std::size_t( 1 ) << 12 ,
                 const A & alloc = A() );

 Arena( const Arena & ) = delete;
 Arena & operator=( const Arena & ) = delete;

 /// destroys all the objects and frees the blocks
 ~Arena();

 /// a new object built from the arguments, whose address never changes
 template< class... Args >
 T * make( Args &&... args );

 /// the number of objects made
 [[nodiscard]] std::size_t size( void ) const;

 /// the memory of the blocks, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the objects made, in the order of make(), random access across the
 // blocks; their addresses do not change, so the iterators stay valid while
 // the arena lives, and a new object only moves the end

 /// the iterator, on objects const if Const
 template< bool Const >
 class Iterator {
  using Blocks = T * const *;   // the blocks, never changed through it

  public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = std::conditional_t< Const , const T & , T & >;
  using pointer = std::conditional_t< Const , const T * , T * >;

  Iterator( void ) = default;
  Iterator( Blocks B , std::size_t n , std::size_t i )
   : f_B( B ) , f_n( n ) , f_i( i ) {}

  template< bool K > requires ( Const && ! K )
  Iterator( const Iterator< K > & o )
   : f_B( o.f_B ) , f_n( o.f_n ) , f_i( o.f_i ) {}

  reference operator*( void ) const { return( ( *this )[ 0 ] ); }
  pointer operator->( void ) const { return( &**this ); }
  reference operator[]( difference_type k ) const {
   const std::size_t j = f_i + std::size_t( k );
   return( f_B[ j / f_n ][ j % f_n ] );
   }

  Iterator & operator++( void ) { ++f_i; return( *this ); }
  Iterator & operator--( void ) { --f_i; return( *this ); }
  Iterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
  Iterator operator--( int ) { auto t = *this; --f_i; return( t ); }
  Iterator & operator+=( difference_type k ) {
   f_i += std::size_t( k );
   return( *this );
   }
  Iterator & operator-=( difference_type k ) {
   f_i -= std::size_t( k );
   return( *this );
   }
  friend Iterator operator+( Iterator a , difference_type k ) {
   return( a += k );
   }
  friend Iterator operator+( difference_type k , Iterator a ) {
   return( a += k );
   }
  friend Iterator operator-( Iterator a , difference_type k ) {
   return( a -= k );
   }
  friend difference_type operator-( const Iterator & a , const Iterator & b ) {
   return( difference_type( a.f_i ) - difference_type( b.f_i ) );
   }
  bool operator==( const Iterator & o ) const { return( f_i == o.f_i ); }
  auto operator<=>( const Iterator & o ) const { return( f_i <=> o.f_i ); }

  private:
  Blocks f_B = nullptr;
  std::size_t f_n = 1 , f_i = 0;

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
  return( iterator( f_blocks.data() , f_n , 0 ) );
  }
 [[nodiscard]] iterator end( void ) {
  return( iterator( f_blocks.data() , f_n , f_size ) );
  }
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_blocks.data() , f_n , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_blocks.data() , f_n , f_size ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_n;                ///< the objects of a block
 std::size_t f_used = 0;         ///< the objects of the last block
 std::size_t f_size = 0;         ///< the objects made
 std::vector< T * > f_blocks;    ///< the blocks
 A f_alloc;                      ///< the allocator of the blocks

 };  // end( class Arena )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/Arena.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Arena.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Arena.h ----------------------------*/
/*--------------------------------------------------------------------------*/
