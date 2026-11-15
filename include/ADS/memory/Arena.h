/*--------------------------------------------------------------------------*/
/*------------------------------ File Arena.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An arena of objects allocated in blocks, beyond the book: `make()` builds
 * an object in the current block, a new block of \f$ n \f$ objects is taken
 * when it is full, and all the objects die with the arena. Allocating costs
 * a pointer bump instead of a call to the allocator, the objects made one
 * after another are next to each other in memory, and their addresses
 * never change, which the pointers among them (the parents of the nodes of
 * a search) rely on; the price is that an object cannot be freed by
 * itself. The blocks come from the allocator `A`, which may give, e.g.,
 * huge pages.
 *
 * `clear()` destroys all the objects at once and keeps the blocks, which
 * the next objects take again in their order, so that a search restarted
 * many times (an incremental search that gives up its repair) drops its
 * nodes in \f$ O( 1 ) \f$ time when they are trivially destructible,
 * instead of freeing them one by one or giving the blocks back to `A`. We
 * call an epoch the time from one `clear()` (or from the construction) to
 * the next one: no object survives a new epoch, and a pointer made in an
 * epoch is stale in the next ones, whose `make()` may give its place to
 * another object. Using a stale pointer violates the contract; `fresh( p )`
 * says whether \f$ p \f$ is a place given in the current epoch, which a
 * stale pointer is until its place is given again, and under the sanitizer
 * of addresses the places dropped by `clear()` stay forbidden until
 * `make()` gives them again, so that the first access through a stale
 * pointer to one of them is stopped there (see \ref theory_memory_epochs).
 * An arena belongs to one thread: other threads may read its objects, but
 * each of their accesses must happen before the `clear()` of the owner
 * (e.g., by a barrier), which takes no lock and has no atomic; with
 * `clear()` never called, the arena is the same as before, function by
 * function.
 *
 * The contracts [see `Contract.h`]: a block holds at least one object; the
 * invariant is that `size()` objects have been made in the epoch. The
 * operations are defined in `Arena.tpp`.
 *
 * \see D. R. Hanson, Fast allocation and deallocation of memory based on
 *      object lifetimes, Software: Practice and Experience 20(1), 5-12,
 *      1990, doi:10.1002/spe.4380200104
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

#ifndef __ADS_Arena
 #define __ADS_Arena
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"

#include <compare>
#include <functional>
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

/// an arena of objects of type `T` in blocks of \f$ n \f$
template< class T , class A = std::allocator< T > >
class Arena {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty arena whose blocks hold \f$ n \f$ objects, taken from `alloc`
 /// \pre \f$ n \ge 1 \f$
 explicit Arena( std::size_t n = std::size_t( 1 ) << 12 ,
                 const A & alloc = A() );

 /// no copy, since the objects are reached by their addresses
 Arena( const Arena & ) = delete;
 /// no copy, since the objects are reached by their addresses
 Arena & operator=( const Arena & ) = delete;

 /// destroys all the objects and frees the blocks
 ~Arena();

 /// a new object built from `args`, whose address never changes in the
 /// epoch
 ///
 /// In pseudocode (the arena of Hanson, beyond the book), with
 /// \f$ \mathit{used} \f$ the objects of the block \f$ c \f$ in use and
 /// \f$ \beta \f$ the blocks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{make}( \mathit{args} ) \cr
 /// 2 & \quad \textbf{if}\ \text{there is no block}\ \textbf{or}
 ///     \ \mathit{used} = n\ \textbf{then} \cr
 /// 3 & \qquad \textbf{if}\ c + 1 < \beta\ \textbf{then}\ c \gets c + 1
 ///     \quad \triangleright\ \text{a block of an old epoch} \cr
 /// 4 & \qquad \textbf{else}\ \text{append a block of } n \text{ places}
 ///     \text{ from the allocator};\ c \gets \beta - 1 \cr
 /// 5 & \qquad \mathit{used} \gets 0 \cr
 /// 6 & \quad p \gets \text{the place } \mathit{used} \text{ of the}
 ///     \text{ block } c;\ \text{build the object in } p \cr
 /// 7 & \quad \mathit{used} \gets \mathit{used} + 1;\ \textbf{return}\ p
 /// \end{array}}
 /// \f]
 template< class... Args >
 T * make( Args &&... args );

 /// all the objects destroyed and the blocks kept for the next ones, the
 /// first of which goes to the first place of the first block: a new epoch,
 /// in which no object of the old ones survives
 ///
 /// In pseudocode (beyond the book), \f$ \beta \f$ the blocks and
 /// \f$ c \f$ the index of the block in use:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{clear}() \cr
 /// 2 & \quad \textbf{if}\ T \text{ has a destructor}\ \textbf{then}
 ///     \ \text{destroy the objects of the blocks } 0 , \ldots , c \cr
 /// 3 & \quad \textbf{if}\ \beta > 0\ \textbf{then}\ c \gets 0;
 ///     \ \mathit{used} \gets 0 \cr
 /// 4 & \quad \mathit{size} \gets 0
 /// \end{array}}
 /// \f]
 /// \pre no other thread accesses an object of the arena: each of its
 ///      accesses happens before the call
 void clear( void ) noexcept;

 /// whether \f$ p \f$ is the address of an object made in the current
 /// epoch, in time linear in the blocks: false for a pointer of an old
 /// epoch whose place has not been given again, true for one whose place
 /// has
 [[nodiscard]] bool fresh( const T * p ) const;

 /// the number of objects made in the epoch
 [[nodiscard]] std::size_t size( void ) const;

 /// the memory of the blocks, in bytes, those kept for the epoch included
 [[nodiscard]] std::size_t bytes( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the objects made in the epoch, in the order of `make()`, random access
 // across the blocks; an iterator holds the arena's vector of the blocks,
 // not its array, which an append may move, so that the iterators stay
 // valid while the epoch lasts, and a new object only moves the end

 /// the iterator, on constant objects if `Const`
 template< bool Const >
 class Iterator {
  using Blocks = const std::vector< T * > *;   // the vector of the blocks

  public:
  /// random access
  using iterator_category = std::random_access_iterator_tag;
  /// the objects
  using value_type = T;
  /// a distance in objects
  using difference_type = std::ptrdiff_t;
  /// a reference to an object, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an object, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// an iterator of no arena
  Iterator( void ) = default;
  /// the object \f$ i \f$ of the vector of the blocks \f$ B \f$, of
  /// \f$ n \f$ objects each
  Iterator( Blocks B , std::size_t n , std::size_t i )
   : f_B( B ) , f_n( n ) , f_i( i ) {}

  /// the constant iterator of the object of \f$ o \f$
  template< bool K > requires ( Const && ! K )
  Iterator( const Iterator< K > & o )
   : f_B( o.f_B ) , f_n( o.f_n ) , f_i( o.f_i ) {}

  /// the object
  reference operator*( void ) const { return( ( *this )[ 0 ] ); }
  /// the address of the object
  pointer operator->( void ) const { return( &**this ); }
  /// the object \f$ k \f$ places ahead: the index \f$ j \f$ is at the place
  /// \f$ j \bmod n \f$ of the block \f$ \lfloor j / n \rfloor \f$
  reference operator[]( difference_type k ) const {
   const std::size_t j = f_i + std::size_t( k );
   return( ( *f_B )[ j / f_n ][ j % f_n ] );
   }

  /// the next object
  Iterator & operator++( void ) { ++f_i; return( *this ); }
  /// the previous object
  Iterator & operator--( void ) { --f_i; return( *this ); }
  /// the next object, the iterator before the step returned
  Iterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
  /// the previous object, the iterator before the step returned
  Iterator operator--( int ) { auto t = *this; --f_i; return( t ); }
  /// \f$ k \f$ objects ahead
  Iterator & operator+=( difference_type k ) {
   f_i += std::size_t( k );
   return( *this );
   }
  /// \f$ k \f$ objects back
  Iterator & operator-=( difference_type k ) {
   f_i -= std::size_t( k );
   return( *this );
   }
  /// the iterator \f$ k \f$ objects after \f$ a \f$
  friend Iterator operator+( Iterator a , difference_type k ) {
   return( a += k );
   }
  /// the iterator \f$ k \f$ objects after \f$ a \f$
  friend Iterator operator+( difference_type k , Iterator a ) {
   return( a += k );
   }
  /// the iterator \f$ k \f$ objects before \f$ a \f$
  friend Iterator operator-( Iterator a , difference_type k ) {
   return( a -= k );
   }
  /// the number of objects from \f$ b \f$ to \f$ a \f$
  friend difference_type operator-( const Iterator & a , const Iterator & b ) {
   return( difference_type( a.f_i ) - difference_type( b.f_i ) );
   }
  /// whether the two are at the same object
  bool operator==( const Iterator & o ) const { return( f_i == o.f_i ); }
  /// the order of the objects
  auto operator<=>( const Iterator & o ) const { return( f_i <=> o.f_i ); }

  private:
  Blocks f_B = nullptr;
  std::size_t f_n = 1 , f_i = 0;

  template< bool > friend class Iterator;
  };

 using value_type = T;                      ///< the objects
 using size_type = std::size_t;             ///< a number of objects
 using difference_type = std::ptrdiff_t;    ///< a distance in objects
 using reference = T &;                     ///< a reference to an object
 using const_reference = const T &;         ///< a constant one
 using iterator = Iterator< false >;        ///< the iterator
 using const_iterator = Iterator< true >;   ///< the constant iterator

 /// the first object made
 [[nodiscard]] iterator begin( void ) {
  return( iterator( &f_blocks , f_n , 0 ) );
  }

 /// past the last object made
 [[nodiscard]] iterator end( void ) {
  return( iterator( &f_blocks , f_n , f_size ) );
  }

 /// the first object made, constant
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_blocks , f_n , 0 ) );
  }

 /// past the last object made, constant
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( &f_blocks , f_n , f_size ) );
  }

 /// the first object made, constant
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// past the last object made, constant
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the block after the one in use, kept from an old epoch or new
 void next( void );

 /// the objects of the block in use
 [[nodiscard]] std::size_t used( void ) const {
  return( f_blocks.empty() ? 0 : std::size_t( f_place - f_blocks[ f_cur ] ) );
  }

 std::size_t f_n;                ///< the objects of a block
 T * f_place = nullptr;          ///< the next place of the block in use
 T * f_end = nullptr;            ///< the end of the block in use
 std::size_t f_cur = 0;          ///< the index of the block in use
 std::size_t f_size = 0;         ///< the objects made in the epoch
 std::vector< T * > f_blocks;    ///< the blocks
 A f_alloc;                      ///< the allocator of the blocks

 };  // end( class Arena )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/Arena.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Arena.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Arena.h ----------------------------*/
/*--------------------------------------------------------------------------*/
