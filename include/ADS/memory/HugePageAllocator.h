/*--------------------------------------------------------------------------*/
/*------------------------ File HugePageAllocator.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An allocator for the large blocks of a structure, beyond the book: a
 * request of at least one huge page (2 MB) is aligned to a huge page,
 * rounded up to whole ones and, on Linux, advised to the kernel for the
 * transparent huge pages (madvise( MADV_HUGEPAGE ), which the kernel may
 * ignore); a smaller one goes to std::allocator. A structure whose nodes
 * are reached in no useful order, e.g., the trees of a heap, then misses
 * the TLB once per 2 MB instead of once per 4 KB page, and its memory is
 * faulted in a huge page at a time; the price is up to one huge page more
 * of address space per block, which is not touched, so that it costs no
 * memory. Elsewhere than on Linux the large requests are only aligned.
 *
 * The allocators are all equal, and stateless.
 *
 * \see J. Navarro, S. Iyer, P. Druschel, A. Cox, Practical, transparent
 *      operating system support for superpages, OSDI 2002
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

#ifndef __ADS_HugePageAllocator
 #define __ADS_HugePageAllocator
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <limits>
#include <memory>
#include <new>

#if defined( __linux__ ) && __has_include( <sys/mman.h> )
 #include <sys/mman.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS HugePageAllocator --------------------------*/
/*--------------------------------------------------------------------------*/

/// an allocator of objects of type T, on huge pages for the large blocks
template< class T >
class HugePageAllocator {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;

 /// the size of a huge page, in bytes
 static constexpr std::size_t PAGE = std::size_t( 1 ) << 21;

 HugePageAllocator( void ) = default;

 template< class U >
 HugePageAllocator( const HugePageAllocator< U > & ) noexcept {}

 /// room for n objects, on huge pages if they fill one at least
 [[nodiscard]] T * allocate( std::size_t n ) {
  if( n > ( std::numeric_limits< std::size_t >::max() - PAGE ) /
          sizeof( T ) )
   throw std::bad_array_new_length();
  const std::size_t b = n * sizeof( T );
  if( b < PAGE )
   return( std::allocator< T >().allocate( n ) );
  const std::size_t r = ( b + PAGE - 1 ) & ~( PAGE - 1 );
  void * p = ::operator new( r , std::align_val_t( PAGE ) );
#if defined( MADV_HUGEPAGE )
  madvise( p , r , MADV_HUGEPAGE );
#endif
  return( static_cast< T * >( p ) );
  }

 /// \pre p was given by allocate( n )
 void deallocate( T * p , std::size_t n ) noexcept {
  const std::size_t b = n * sizeof( T );
  if( b < PAGE )
   std::allocator< T >().deallocate( p , n );
  else
   ::operator delete( p , std::align_val_t( PAGE ) );
  }

 template< class U >
 bool operator==( const HugePageAllocator< U > & ) const noexcept {
  return( true );
  }

 };  // end( class HugePageAllocator )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* HugePageAllocator.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File HugePageAllocator.h ----------------------*/
/*--------------------------------------------------------------------------*/
