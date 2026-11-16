/*--------------------------------------------------------------------------*/
/*------------------------- File AllocatorAware.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The rules of the allocator-aware containers of the standard
 * ([container.reqmts], [allocator.requirements]), beyond the book, written
 * once for the containers of ADS that take an allocator `A`: a container
 * rebinds `A` to the type of what it allocates (its nodes, its places, its
 * blocks) by `std::allocator_traits`, a copy takes the allocator that
 * `select_on_container_copy_construction()` gives, and a copy assignment,
 * a move assignment and a swap take the allocator of the other container
 * only when the trait `propagate_on_container_copy_assignment`,
 * `propagate_on_container_move_assignment` or
 * `propagate_on_container_swap` of `A` is true. A move assignment between
 * two containers whose allocators are not equal and do not propagate moves
 * the items one by one into memory of the allocator of the target, since
 * its memory must go back to the allocator that gave it; a swap of such
 * containers is not allowed (as in the standard).
 *
 * Any allocator of the standard plugs in: `std::pmr::polymorphic_allocator`
 * over a resource (e.g., `std::pmr::monotonic_buffer_resource`), whose
 * allocators do not propagate; `mi_stl_allocator` of mimalloc, stateless;
 * an allocator written over `mallocx()` and `sdallocx()` of jemalloc; and
 * those of ADS, `HugePageAllocator` [see `HugePageAllocator.h`] and
 * `PoolAllocator` [see `PoolAllocator.h`] over a `PoolResource`, a
 * `ConcurrentPoolResource` or an `ArenaResource` [see `Arena.h`], which
 * propagate. With the default allocator of a container, `std::allocator`
 * or the `HugePageAllocator` of its `ObjectPool` [see `ObjectPool.h`], the
 * code and the layout of the container are those it had before it took an
 * allocator: the default allocator is empty, it takes no room, and the
 * paths that only a stateful allocator needs are compiled out.
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

#ifndef __ADS_AllocatorAware
 #define __ADS_AllocatorAware
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// `A` rebound to the objects of type `T`
template< class A , class T >
using RebindAlloc =
  typename std::allocator_traits< A >::template rebind_alloc< T >;

/// a vector of `T` on `A` rebound to `T`
template< class A , class T >
using AllocVector = std::vector< T , RebindAlloc< A , T > >;

/// whether `A` is a `std::allocator`, whose containers keep the code they
/// had before they took an allocator
template< class A >
inline constexpr bool isStdAllocator = false;

/// a `std::allocator` of any type
template< class T >
inline constexpr bool isStdAllocator< std::allocator< T > > = true;

/// whether the allocators of `A` are all equal, so that a container may
/// take the memory of another one whatever its allocator, and copy and
/// swap as it did with no allocator
template< class A >
inline constexpr bool alwaysEqualAlloc =
  std::allocator_traits< A >::is_always_equal::value;

/// an empty container of the type of \f$ v \f$ and on its allocator,
/// default-constructed for a `std::allocator`
template< class V >
[[nodiscard]] V emptyLike( const V & v )
{
 if constexpr( alwaysEqualAlloc< typename V::allocator_type > &&
	       std::is_default_constructible_v< typename V::allocator_type > )
  return( V() );
 else
  return( V( v.get_allocator() ) );
 }

/// an empty container of the type of \f$ v \f$ for a copy of it, on the
/// allocator that `select_on_container_copy_construction()` gives,
/// default-constructed for a `std::allocator`
template< class V >
[[nodiscard]] V emptyCopy( const V & v )
{
 using Al = typename V::allocator_type;
 if constexpr( alwaysEqualAlloc< Al > &&
	       std::is_default_constructible_v< Al > )
  return( V() );
 else
  return(
    V( std::allocator_traits< Al >::select_on_container_copy_construction(
      v.get_allocator() ) ) );
 }

/// whether a copy assignment takes the allocator of the source
template< class A >
inline constexpr bool copyPropagates =
  std::allocator_traits< A >::propagate_on_container_copy_assignment::value;

/// whether a move assignment takes the allocator of the source
template< class A >
inline constexpr bool movePropagates =
  std::allocator_traits< A >::propagate_on_container_move_assignment::value;

/// whether a swap exchanges the allocators
template< class A >
inline constexpr bool swapPropagates =
  std::allocator_traits< A >::propagate_on_container_swap::value;

/// the allocator of a copy of a container of allocator \f$ a \f$
template< class A >
[[nodiscard]] A copyAlloc( const A & a )
{
 using Traits = std::allocator_traits< A >;
 return( Traits::select_on_container_copy_construction( a ) );
 }

/// whether a move assignment from a container of allocator \f$ b \f$ to
/// one of allocator \f$ a \f$ may take the memory of the first one
template< class A >
[[nodiscard]] bool stealAlloc( const A & a , const A & b ) noexcept
{
 if constexpr( movePropagates< A > || alwaysEqualAlloc< A > )
  return( true );
 else
  return( a == b );
 }

/// \f$ a \gets b \f$ in a copy assignment, if `A` propagates on it
template< class A >
void copyAssignAlloc( A & a , const A & b ) noexcept
{
 if constexpr( copyPropagates< A > )
  a = b;
 }

/// \f$ a \gets b \f$ in a move assignment, if `A` propagates on it
template< class A >
void moveAssignAlloc( A & a , A & b ) noexcept
{
 if constexpr( movePropagates< A > )
  a = std::move( b );
 }

/// \f$ a \f$ and \f$ b \f$ exchanged in a swap, if `A` propagates on it
/// \pre \f$ a = b \f$ otherwise
template< class A >
void swapAlloc( A & a , A & b ) noexcept
{
 if constexpr( swapPropagates< A > ) {
  using std::swap;
  swap( a , b );
  }
 else if constexpr( ! alwaysEqualAlloc< A > )
  ADS_PRE( a == b );
 }

/// the allocator \f$ a \f$ of a container that manages its memory by
/// itself, held as a base, so that an empty allocator takes no room and
/// leaves the layout of the members as it was with no allocator
template< class A , bool = std::is_empty_v< A > && ! std::is_final_v< A > >
class AllocHolder
{
 public:
 /// the default allocator
 AllocHolder( void ) = default;
 /// the allocator \f$ a \f$
 explicit AllocHolder( const A & a ) noexcept : f_a( a ) { }
 /// the allocator
 [[nodiscard]] A & alloc( void ) noexcept { return( f_a ); }
 /// the allocator, constant
 [[nodiscard]] const A & alloc( void ) const noexcept { return( f_a ); }

 private:
 A f_a;  ///< the allocator
 };

/// an empty allocator, a base of its own
template< class A >
class AllocHolder< A , true > : private A
{
 public:
 /// the default allocator
 AllocHolder( void ) = default;
 /// the allocator \f$ a \f$
 explicit AllocHolder( const A & a ) noexcept : A( a ) { }
 /// the allocator
 [[nodiscard]] A & alloc( void ) noexcept { return( *this ); }
 /// the allocator, constant
 [[nodiscard]] const A & alloc( void ) const noexcept { return( *this ); }
 };

/// an array of \f$ n \f$ objects of type `T` value-initialized on `A`
/// rebound to `T`, which frees them by the allocator it holds: the
/// `std::unique_ptr< T[] >` of `std::make_unique< T[] >( n )` for any
/// allocator, moved with its allocator and never copied
template< class A , class T >
class AllocArray : private AllocHolder< RebindAlloc< A , T > >
{
 using Al = RebindAlloc< A ,
			 T >;                 ///< the allocator of the objects
 using Traits = std::allocator_traits< Al >;  ///< and its traits
 using Holder = AllocHolder< Al >;            ///< the base that holds it

 public:
 /// no array
 AllocArray( void ) = default;

 /// \f$ n \f$ objects `T()` on \f$ a \f$
 AllocArray( const A & a , std::size_t n ) : Holder( Al( a ) ) , f_n( n ) {
  f_p = Traits::allocate( this->alloc() , n );
  std::size_t i = 0;
  try {
   for( ; i < n ; ++i )
    ::new( static_cast< void * >( f_p + i ) ) T();
   }
  catch( ... ) {
   std::destroy_n( f_p , i );
   Traits::deallocate( this->alloc() , f_p , n );
   throw;
   }
  }

 /// the array of `o`, which is left with none
 AllocArray( AllocArray && o ) noexcept
     : Holder( std::move( o.alloc() ) ) ,
       f_p( std::exchange( o.f_p , nullptr ) ) ,
       f_n( std::exchange( o.f_n , 0 ) ) { }

 /// the array of `o` with its allocator, this one freed (an allocator
 /// need not be assignable, as `std::pmr::polymorphic_allocator` is not)
 AllocArray & operator=( AllocArray && o ) noexcept {
  if( this != &o ) {
   std::destroy_at( this );
   std::construct_at( this , std::move( o ) );
   }
  return( *this );
  }

 /// frees the objects
 ~AllocArray() { release(); }

 /// the object \f$ i \f$
 [[nodiscard]] T & operator[]( std::size_t i ) const noexcept {
  return( f_p[ i ] );
  }

 /// the first object, nil if none
 [[nodiscard]] T * get( void ) const noexcept { return( f_p ); }

 private:
 /// the objects destroyed and freed
 void release( void ) noexcept {
  if( f_p ) {
   std::destroy_n( f_p , f_n );
   Traits::deallocate( this->alloc() , f_p , f_n );
   f_p = nullptr;
   }
  }

 T * f_p = nullptr;    ///< the objects
 std::size_t f_n = 0;  ///< their number
 };

/// the precondition of a swap of two containers of allocators \f$ a \f$
/// and \f$ b \f$ that do not exchange them
/// \pre \f$ a = b \f$ if `A` does not propagate on a swap
template< class A >
void swapCheck( [[maybe_unused]] const A & a ,
		[[maybe_unused]] const A & b ) noexcept
{
 if constexpr( ! swapPropagates< A > && ! alwaysEqualAlloc< A > )
  ADS_PRE( a == b );
 }

/// the contents of \f$ c \f$ replaced by those of \f$ t \f$, its
/// allocator included, by the move constructor of `C`
template< class C >
void replaceBy( C & c , C && t ) noexcept
{
 std::destroy_at( &c );
 std::construct_at( &c , std::move( t ) );
 }

/// \f$ c \gets o \f$ by copy, for a container `C` whose allocators are not
/// all equal and that has the constructors of the standard with an
/// allocator: the copy is made on the allocator of \f$ o \f$ if it
/// propagates on the copy, on that of \f$ c \f$ otherwise
template< class C >
C & copyAssign( C & c , const C & o )
{
 using A = typename C::allocator_type;
 if( &c != &o ) {
  C t( o , copyPropagates< A > ? o.get_allocator() : c.get_allocator() );
  replaceBy( c , std::move( t ) );
  }
 return( c );
 }

/// \f$ c \gets o \f$ by move, for a container `C` as in `copyAssign()`:
/// the memory of \f$ o \f$ if its allocator propagates on the move or is
/// equal to that of \f$ c \f$, else the items of \f$ o \f$ moved one by
/// one to the allocator of \f$ c \f$
template< class C >
C & moveAssign( C & c , C && o )
{
 if( &c != &o ) {
  if( stealAlloc( c.get_allocator() , o.get_allocator() ) )
   replaceBy( c , std::move( o ) );
  else {
   C t( std::move( o ) , c.get_allocator() );
   replaceBy( c , std::move( t ) );
   }
  }
 return( c );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* AllocatorAware.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File AllocatorAware.h ------------------------*/
/*--------------------------------------------------------------------------*/
