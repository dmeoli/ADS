/*--------------------------------------------------------------------------*/
/*----------------------------- File Scratch.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The room of the parallel algorithms, beyond the book: the buffers of
 * \f$ n \f$ items that the parallel sorts and the parallel transform need,
 * taken from a block kept between the calls (the block of the last call that
 * gave its room back, freed at the exit of the program or when a larger
 * block replaces it), from the allocator of huge pages [see
 * `HugePageAllocator.h`]. The pages of a block already used are faulted in
 * and cleared, while a fresh block of 80 MB costs, in the faults that clear
 * its pages, as much as the moves of the items into it; a call that finds
 * the block taken by another call (of another thread) allocates its own.
 * The block is a cache of one entry, the same idea as the spans that the
 * allocators by size classes keep after a free.
 *
 * The contracts [see `Contract.h`]: the items of a `ScratchBuffer` are built
 * and destroyed by its user (they are raw memory).
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

#ifndef __ADS_Scratch
 #define __ADS_Scratch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/HugePageAllocator.h"

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail {

/// the room of the parallel algorithms, kept between their calls: the
/// block of the last call that gave its room back, which the next one
/// takes if it is large enough, so that its pages are already faulted in;
/// the block goes back to the system at the exit, or when a larger one
/// replaces it
class ScratchRoom {
 public:

 /// a block of at least \f$ b \f$ bytes, aligned to a huge page, and its
 /// size in `got`
 static std::byte * take( std::size_t b , std::size_t & got ) {
  {
   std::lock_guard< std::mutex > g( slot().lock );
   if( slot().p && ( slot().size >= b ) ) {
    got = slot().size;
    return( std::exchange( slot().p , nullptr ) );
    }
  }
  got = b;
  return( HugePageAllocator< std::byte >().allocate( b ) );
  }

 /// gives the block of \f$ b \f$ bytes back, kept if no larger one is
 static void give( std::byte * p , std::size_t b ) {
  std::byte * drop = p;
  std::size_t db = b;
  {
   std::lock_guard< std::mutex > g( slot().lock );
   if( ! slot().p || ( slot().size < b ) ) {
    std::swap( drop , slot().p );
    std::swap( db , slot().size );
    }
  }
  if( drop )
   HugePageAllocator< std::byte >().deallocate( drop , db );
  }

 private:

 struct Slot {
  std::mutex lock;
  std::byte * p = nullptr;
  std::size_t size = 0;
  ~Slot() {
   if( p ) HugePageAllocator< std::byte >().deallocate( p , size );
   }
  };

 static Slot & slot( void ) {
  static Slot s;
  return( s );
  }
 };

/// room for \f$ n \f$ items, not constructed, from `ScratchRoom`
template< class T >
class ScratchBuffer {
 public:
 /// the room for \f$ n \f$ items
 explicit ScratchBuffer( std::size_t n )
  : f_p( reinterpret_cast< T * >( ScratchRoom::take(
                                  std::max< std::size_t >( n , 1 ) *
                                  sizeof( T ) , f_b ) ) ) {}
 /// gives the room back to `ScratchRoom`
 ~ScratchBuffer() {
  ScratchRoom::give( reinterpret_cast< std::byte * >( f_p ) , f_b );
  }
 ScratchBuffer( const ScratchBuffer & ) = delete;
 ScratchBuffer & operator=( const ScratchBuffer & ) = delete;
 /// the first item of the room
 [[nodiscard]] T * data( void ) const { return( f_p ); }
 private:
 std::size_t f_b = 0;                      ///< the bytes of the block
 T * f_p;
 };

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/

#endif  /* Scratch.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Scratch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
