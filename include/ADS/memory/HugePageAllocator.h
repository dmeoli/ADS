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
 * A large block given back is kept by its thread, up to 16 blocks and
 * 64 MB in all, and given again to the next request of the same size, as
 * the size-class allocators keep their freed spans (Berger et al.;
 * Leijen et al.): its pages are already faulted in, cleared and made huge,
 * so that a structure that is built and destroyed again and again, or
 * that grows through the same sizes (a vector that doubles, a pool whose
 * blocks double), pays the faults once; the blocks kept go back to the
 * system when the thread ends.
 *
 * The allocators are all equal, and stateless.
 *
 * \see J. Navarro, S. Iyer, P. Druschel, A. Cox, Practical, transparent
 *      operating system support for superpages, ACM SIGOPS Operating
 *      Systems Review 36(SI), 89-104, 2002 (OSDI 2002),
 *      doi:10.1145/844128.844138
 * \see E. D. Berger, B. G. Zorn, K. S. McKinley, Reconsidering custom
 *      memory allocation, Proceedings of the 17th ACM SIGPLAN Conference
 *      on Object-Oriented Programming, Systems, Languages, and
 *      Applications (OOPSLA), 1-12, 2002, doi:10.1145/582419.582421
 * \see D. Leijen, B. Zorn, L. de Moura, Mimalloc: free list sharding in
 *      action, Programming Languages and Systems (APLAS 2019), Lecture
 *      Notes in Computer Science 11893, 244-265, 2019,
 *      doi:10.1007/978-3-030-34175-6_13
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

namespace detail {

/// the large blocks given back and kept by a thread, for the requests of
/// the same size
struct HugeCache {
 static constexpr int N = 16;                            ///< the blocks
 static constexpr std::size_t BYTES = std::size_t( 64 ) << 20;   ///< all

 void * p[ N ];
 std::size_t r[ N ];
 int n = 0;
 std::size_t bytes = 0;

 /// a block of r bytes kept, if any, and no more kept
 void * take( std::size_t s ) {
  for( int i = n ; i-- > 0 ; )
   if( r[ i ] == s ) {
    void * q = p[ i ];
    bytes -= s;
    p[ i ] = p[ --n ];
    r[ i ] = r[ n ];
    return( q );
    }
  return( nullptr );
  }

 /// the block q of s bytes kept, if there is room
 bool keep( void * q , std::size_t s ) {
  if( ( n == N ) || ( bytes + s > BYTES ) )
   return( false );
  p[ n ] = q;
  r[ n++ ] = s;
  bytes += s;
  return( true );
  }

 ~HugeCache();
 };

/// the blocks of this thread are no longer kept: its cache is gone, and
/// the blocks given back after it (by the objects of static storage that
/// die later) go to the system
inline bool & hugeCacheGone( void )
{
 thread_local bool gone = false;           // no destructor: always there
 return( gone );
 }

inline HugeCache::~HugeCache()
{
 hugeCacheGone() = true;
 while( n > 0 ) {
  --n;
  ::operator delete( p[ n ] , std::align_val_t( std::size_t( 1 ) << 21 ) );
  }
 }

/// the blocks kept by this thread
inline HugeCache & hugeCache( void )
{
 thread_local HugeCache c;
 return( c );
 }

}  // end( namespace detail )

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
  if( ! detail::hugeCacheGone() )           // a block kept
   if( void * q = detail::hugeCache().take( r ) )
    return( static_cast< T * >( q ) );
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
  else if( detail::hugeCacheGone() ||
           ! detail::hugeCache().keep( p ,
                                       ( b + PAGE - 1 ) & ~( PAGE - 1 ) ) )
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
