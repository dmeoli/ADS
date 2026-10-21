/*--------------------------------------------------------------------------*/
/*-------------------------- File PoolAllocator.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A standard allocator over pools of size classes, beyond the book, so that
 * the containers of the standard library and std::allocate_shared take
 * their nodes from pages of slots of one size instead of the general
 * allocator.
 *
 * PoolResource< M > keeps the size classes of \f$ g = 8 \f$ bytes up to
 * M: a request of b bytes, \f$ 1 \le b \le M \f$, aligned to \f$ a \le 8
 * \f$, takes a slot of \f$ \lceil b / 8 \rceil 8 \f$ bytes, and one
 * aligned to 16 a slot of \f$ \lceil b / 16 \rceil 16 \f$ bytes, whose
 * class has its slots aligned to 16, so that the internal waste is below
 * a bytes per object and the nodes of the containers, aligned to 8, waste
 * nothing when their size is a multiple of 8; any other request, larger
 * or more aligned, goes to operator new. The slots come from the pages of
 * PageHeap [see PageHeap.h], whose free lists are kept by page: a class
 * allocates from one page at a time, so that the nodes of a container
 * that is emptied in a scrambled order and filled again are given back a
 * page at a time, close to each other, which a single free list (as that
 * of ObjectPool [see ObjectPool.h]) does not do. The memory goes back to
 * the system only with the resource: a structure that shrinks keeps the
 * memory of its largest size, which it takes again at no cost.
 *
 * PoolAllocator< T , R > is the allocator of the standard (the
 * requirements Allocator of [allocator.requirements]): it holds a pointer
 * to its resource R, the PoolResource above or the ConcurrentPoolResource
 * of the threads [see ConcurrentPool.h], it is rebound to any type by
 * std::allocator_traits (a node of std::list, std::map or
 * std::unordered_map, the control block of std::allocate_shared, which
 * then holds the counts and the object in one slot), and two allocators
 * are equal when they share their resource. A container copies, moves and
 * swaps its allocator with its contents (the three propagate_on_container
 * traits are true), so that a node always goes back to the pool it came
 * from. A request of one object, the case of the node containers, finds
 * its class at compile time and costs the pop of a free list; a request of
 * n objects, e.g., the buckets of std::unordered_map, finds it from n.
 *
 * Neither the resource nor its allocators synchronize: one thread at a
 * time uses a PoolResource, and an object made by one thread and freed by
 * another needs ConcurrentPoolResource. The resource must outlive every
 * object and every allocator made from it.
 *
 * The contracts [see Contract.h]: a pointer freed was given by the same
 * resource, and its deallocation gets the size and the alignment of its
 * allocation. The operations are defined in PoolAllocator.tpp.
 *
 * \see D. Leijen, B. Zorn, L. de Moura, Mimalloc: free list sharding in
 *      action, Programming Languages and Systems (APLAS 2019), Lecture
 *      Notes in Computer Science 11893, 244-265, 2019,
 *      doi:10.1007/978-3-030-34175-6_13 (the free lists by page)
 * \see J. Bonwick, The slab allocator: an object-caching kernel memory
 *      allocator, Proceedings of the USENIX Summer 1994 Technical
 *      Conference, 1994,
 *      www.usenix.org/publications/library/proceedings/bos94/bonwick.html (the
 *      slabs of one size)
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

#ifndef __ADS_PoolAllocator
 #define __ADS_PoolAllocator
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/PageHeap.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the memory of operator new, aligned to a, for the requests that no
/// class takes
[[nodiscard]] void * poolNew( std::size_t b , std::size_t a );

/// \pre p was given by poolNew( b , a )
void poolDelete( void * p , std::size_t b , std::size_t a ) noexcept;

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PoolResource ---------------------------*/
/*--------------------------------------------------------------------------*/

/// pools of the size classes of 8 bytes up to M, for one thread at a time
template< std::size_t M = 256 >
class PoolResource {

 static_assert( ( M >= 16 ) && ( M % 16 == 0 ) && ( M <= 4096 ) ,
                "the largest class is a multiple of 16 bytes, up to 4096" );

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t GRAIN = 8;           ///< the step of a class
 static constexpr std::size_t ALIGN = 16;          ///< the largest alignment
 static constexpr std::size_t MAX = M;             ///< the largest class
 static constexpr std::size_t CLASSES = M / GRAIN; ///< the classes

 /// true if a request of b bytes aligned to a takes a class
 [[nodiscard]] static constexpr bool pooled( std::size_t b ,
                                             std::size_t a ) {
  return( ( b >= 1 ) && ( b <= M ) && ( a <= ALIGN ) );
  }

 /// an empty resource
 PoolResource( void );

 PoolResource( const PoolResource & ) = delete;
 PoolResource & operator=( const PoolResource & ) = delete;

 /// frees the blocks; the objects still alive are not destroyed
 ~PoolResource() = default;

 /// b bytes aligned to a, b and a known at compile time
 template< std::size_t b , std::size_t a = alignof( std::max_align_t ) >
 [[nodiscard]] void * allocate( void );

 /// \pre p was given by allocate< b , a >()
 template< std::size_t b , std::size_t a = alignof( std::max_align_t ) >
 void deallocate( void * p );

 /// b bytes aligned to a
 [[nodiscard]] void * allocate( std::size_t b ,
                                std::size_t a = alignof( std::max_align_t )
                                );

 /// \pre p was given by allocate( b , a )
 void deallocate( void * p , std::size_t b ,
                  std::size_t a = alignof( std::max_align_t ) );

 /// the number of objects alive in the classes
 [[nodiscard]] std::size_t size( void ) const;

 /// the memory of the blocks of the classes, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the class of b bytes aligned to a: b rounded up to a multiple of 8,
 /// or of 16 if a > 8, so that the slots of the class are aligned to a
 [[nodiscard]] static constexpr std::size_t classOf( std::size_t b ,
                                                     std::size_t a ) {
  return( ( a > GRAIN ? ( b + 15 ) / 16 * 2 : ( b + 7 ) / 8 ) - 1 );
  }

 /// a slot of the class c
 [[nodiscard]] void * take( std::size_t c ) {
  if( void * p = f_heap->local( c ) ) [[likely]]
   return( p );
  return( f_heap->more( c ) );
  }

 /// the pages of the classes, apart from the resource, so that their
 /// state, read and written at each operation, never shares the lines or
 /// the low bits of the addresses of the variables around the resource
 /// (e.g., on a stack)
 std::unique_ptr< detail::PageHeap< CLASSES > > f_heap;

 };  // end( class PoolResource )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PoolAllocator --------------------------*/
/*--------------------------------------------------------------------------*/

/// the standard allocator of objects of type T over the resource R
template< class T , class R = PoolResource<> >
class PoolAllocator {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using propagate_on_container_copy_assignment = std::true_type;
 using propagate_on_container_move_assignment = std::true_type;
 using propagate_on_container_swap = std::true_type;
 using is_always_equal = std::false_type;

 template< class U >
 struct rebind { using other = PoolAllocator< U , R >; };

 /// the allocator of the resource r
 PoolAllocator( R & r ) noexcept : f_r( &r ) {}

 /// the allocator of the default resource of R, if it has one
 PoolAllocator( void ) noexcept requires requires { R::global(); }
  : f_r( &R::global() ) {}

 template< class U >
 PoolAllocator( const PoolAllocator< U , R > & o ) noexcept
  : f_r( o.resource() ) {}

 /// room for n objects
 [[nodiscard]] T * allocate( std::size_t n );

 /// \pre p was given by allocate( n ) of an allocator equal to this one
 void deallocate( T * p , std::size_t n );

 /// the resource
 [[nodiscard]] R * resource( void ) const noexcept { return( f_r ); }

 template< class U >
 bool operator==( const PoolAllocator< U , R > & o ) const noexcept {
  return( f_r == o.resource() );
  }

/*--------------------------------------------------------------------------*/
 private:

 R * f_r;   ///< the resource, never nil

 };  // end( class PoolAllocator )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/PoolAllocator.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PoolAllocator.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PoolAllocator.h ------------------------*/
/*--------------------------------------------------------------------------*/
