/*--------------------------------------------------------------------------*/
/*--------------------------- File ObjectPool.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A pool of objects of one type, beyond the book: make() builds an object
 * in a slot of the pool and free() destroys it and keeps its slot, which
 * the next make() takes again (the last one freed first), so that the
 * nodes of a linked structure cost no call to the allocator, the slot just
 * freed is still in the cache when it is used again, and the objects made
 * one after another are next to each other in memory. The slots come in
 * blocks taken from the allocator A, each twice the last one up to n
 * slots, and go back to it only with the pool: a structure that shrinks
 * keeps the memory of its largest size. The default allocator gives the
 * blocks of 2 MB or more on huge pages [see HugePageAllocator.h], which
 * the nodes of a large linked structure, reached in no useful order, need
 * not to miss the TLB at each step. Unlike Arena [see Arena.h], an
 * object can be freed by itself, and the pool does not destroy the objects
 * left in it, which their owner destroys (or does not, if their destructor
 * is trivial) before the pool dies.
 *
 * The contracts [see Contract.h]: a block holds at least one slot, free()
 * needs an object of the pool; the invariant is that size() objects are
 * alive. The operations are defined in ObjectPool.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.2.3 (the list AVAIL)
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

#ifndef __ADS_ObjectPool
 #define __ADS_ObjectPool
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"

#include <cstddef>
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
/*---------------------------- CLASS ObjectPool ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a pool of objects of type T in blocks of at most n slots
template< class T , class A = HugePageAllocator< T > >
class ObjectPool {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty pool whose blocks grow up to n slots
 /// \pre n >= 1
 explicit ObjectPool( std::size_t n = std::size_t( 1 ) << 24 ,
                      const A & alloc = A() );

 ObjectPool( const ObjectPool & ) = delete;
 ObjectPool & operator=( const ObjectPool & ) = delete;

 /// the slots of other, which is left empty; its objects stay where they
 /// are
 ObjectPool( ObjectPool && other ) noexcept;

 ObjectPool & operator=( ObjectPool && other ) noexcept;

 /// frees the blocks; the objects still alive are not destroyed
 ~ObjectPool();

 void swap( ObjectPool & other ) noexcept;

 /// a new object built from the arguments, in the slot freed last if any
 template< class... Args >
 T * make( Args &&... args );

 /// destroys x and keeps its slot for the next make()
 /// \pre x was made by this pool and is alive
 void free( T * x );

 /// the number of objects alive
 [[nodiscard]] std::size_t size( void ) const;

 /// the memory of the blocks, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a slot: an object, or the next free slot
 union Slot {
  Slot * next;
  alignas( T ) unsigned char object[ sizeof( T ) ];
  };

 using SlotAlloc =
  typename std::allocator_traits< A >::template rebind_alloc< Slot >;

 Slot * f_free = nullptr;   ///< the free slots, the last one freed first
 Slot * f_next = nullptr;   ///< the first slot never used of the last block
 Slot * f_end = nullptr;    ///< the end of the last block
 std::size_t f_n;           ///< the slots of the largest block
 std::size_t f_size = 0;    ///< the objects alive

 /// the blocks and their numbers of slots
 std::vector< std::pair< Slot * , std::size_t > > f_blocks;
 SlotAlloc f_alloc;         ///< the allocator of the blocks

 };  // end( class ObjectPool )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/ObjectPool.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ObjectPool.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ObjectPool.h --------------------------*/
/*--------------------------------------------------------------------------*/
