/*--------------------------------------------------------------------------*/
/*--------------------------- File ObjectPool.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A pool of objects of one type, beyond the book: `make()` builds an object
 * in a slot of the pool and `free()` destroys it and keeps its slot, which
 * the next `make()` takes again (the last one freed first), so that the
 * nodes of a linked structure cost no call to the allocator, the slot just
 * freed is still in the cache when it is used again, and the objects made
 * one after another are next to each other in memory. The slots come in
 * blocks taken from the allocator `A`, the first of one slot and each next
 * one twice the last up to \f$ n \f$ slots, so that a small structure takes
 * no more than it holds (at most twice its largest size, plus one block of
 * \f$ n \f$); a block begins with a header that links it to the one before,
 * so that the pool is seven words and allocates nothing but its blocks,
 * which go back to `A` only with the pool: a structure that shrinks keeps
 * the memory of its largest size. The default allocator gives the blocks
 * of 2 MB or more on huge pages [see `HugePageAllocator.h`], which the
 * nodes of a large linked structure, reached in no useful order, need not
 * to miss the TLB at each step. Unlike `Arena` [see `Arena.h`], an object
 * can be freed by itself, and the pool does not destroy the objects left
 * in it, which their owner destroys (or does not, if their destructor is
 * trivial) before the pool dies. Each operation costs \f$ O( 1 ) \f$ time,
 * but for the allocation of a block.
 *
 * `allocate()` and `deallocate()` give and take back the raw slots, the
 * objects built and destroyed by the caller, as with `std::allocator`.
 *
 * `clear()` drops all the objects at once in \f$ O( 1 ) \f$ time, with no
 * destructor called (as at the death of the pool), and keeps the blocks,
 * which the next objects take again from the last one down, before a new
 * block is taken: a worker of a parallel search, which owns a pool, drops
 * its nodes at a restart without freeing them one by one. We call an epoch
 * the time from one `clear()` (or from the construction) to the next one:
 * nothing survives a new epoch, neither an object nor a free slot, and a
 * pointer of an old epoch is stale, its slot given again by the `make()` of
 * the new one. Using a stale pointer, `free()` included, violates the
 * contract; `fresh( p )` says whether \f$ p \f$ is a slot given in the
 * current epoch, which `free()` and `deallocate()` check at the level of
 * the audits (a stale pointer passes only when its slot has been given
 * again), and under the sanitizer of addresses the slots dropped by
 * `clear()` stay forbidden until they are given again (see
 * \ref theory_memory_epochs). A pool belongs to one thread: an object is
 * freed by the thread that made it, and an object that another thread
 * wants to drop goes back to the owner by a message, to be freed there (or
 * is dropped by the next `clear()`); other threads may read the objects,
 * each access happening before the `clear()` of the owner (e.g., by a
 * barrier). `ConcurrentPoolResource` [see `ConcurrentPool.h`] takes the
 * frees of any thread instead. With `clear()` never called, `make()` and
 * `free()` are the same as before.
 *
 * The contracts [see `Contract.h`]: a block holds at least one slot,
 * `free()` and `deallocate()` need an object (a slot) of the pool given in
 * the epoch (audit); the invariant is that `size()` objects are alive. The
 * operations are defined in `ObjectPool.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.2.3 (the list AVAIL)
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

#ifndef __ADS_ObjectPool
 #define __ADS_ObjectPool
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/memory/HugePageAllocator.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ObjectPool ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a pool of objects of type `T` in blocks of at most \f$ n \f$ slots
template< class T , class A = HugePageAllocator< T > >
class ObjectPool
{

/*--------------------------------------------------------------------------*/
 public:
 /// the empty pool whose blocks grow up to \f$ n \f$ slots, taken from
 /// `alloc`
 /// \pre \f$ n \ge 1 \f$
 explicit ObjectPool( std::size_t n = std::size_t( 1 ) << 24 ,
		      const A & alloc = A() );

 /// no copy, since its objects are reached by their addresses
 ObjectPool( const ObjectPool & ) = delete;
 /// no copy, since its objects are reached by their addresses
 ObjectPool & operator=( const ObjectPool & ) = delete;

 /// the slots of `other`, which is left empty; its objects stay where they
 /// are
 ObjectPool( ObjectPool && other ) noexcept;

 /// the slots of `other`, which is left empty, after those of this pool are
 /// freed
 ObjectPool & operator=( ObjectPool && other ) noexcept;

 /// frees the blocks; the objects still alive are not destroyed
 ~ObjectPool();

 /// the slots of this pool and of `other` exchanged
 void swap( ObjectPool & other ) noexcept;

 /// the blocks of `other` taken by this pool, with the objects alive in
 /// them, `other` left empty: the objects made by `other` become objects of
 /// this pool, to be freed by it, so that a linked structure takes the
 /// nodes of another one with no copy (the `absorb()` of `List`); in both
 /// cases the limit of the blocks becomes the larger of the two; if this
 /// pool has no block, it takes the blocks and the free slots of `other`,
 /// as `swap( other )` does, else the block chain of `other` goes under the
 /// first block of this pool, in time linear in the blocks of this pool for
 /// the walk to it (over its last block, in time linear in the blocks of
 /// `other`, when this pool has blocks kept by `clear()` not yet used
 /// again), and the free slots of `other` and those it never used are no
 /// longer given, until the pool dies or is cleared
 /// \pre the two allocators are equal
 void adopt( ObjectPool & other ) noexcept;

 /// a new object built from `args`, in the slot freed last if any
 ///
 /// In pseudocode (the list AVAIL of section 2.2.3 of TAOCP, beyond the
 /// book), \f$ F \f$ the free slots and
 /// \f$ [ \mathit{next} , \mathit{end} ) \f$ the slots never used of the
 /// last block, of \f$ k \f$ slots:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{make}( \mathit{args} ) \cr
 /// 2 & \quad \textbf{if}\ F \ne \textbf{nil}\ \textbf{then}\ s \gets
 ///     F;\ F \gets s.\mathit{next} \cr
 /// 3 & \quad \textbf{else} \cr
 /// 4 & \qquad \textbf{if}\ \mathit{next} = \mathit{end}\ \textbf{then}
 ///     \ \text{take a block of } \min( 2 k , n ) \text{ slots, } 1
 ///     \text{ the}
 ///     \text{ first time} \cr
 /// 5 & \qquad s \gets \mathit{next};\ \mathit{next} \gets \mathit{next} + 1
 ///     \cr
 /// 6 & \quad \text{build the object in } s \text{ (on an exception, push}
 ///     \text{ } s \text{ on } F \text{ again)} \cr
 /// 7 & \quad \textbf{return}\ s
 /// \end{array}}
 /// \f]
 template< class... Args >
 T * make( Args &&... args );

 /// destroys \f$ x \f$ and keeps its slot for the next `make()`
 ///
 /// In pseudocode (the list AVAIL of section 2.2.3 of TAOCP, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{free}( x ) \cr
 /// 2 & \quad \text{destroy the object } x;\ x.\mathit{next} \gets F;\ F
 ///     \gets x
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ was made by this pool and is alive
 void free( T * x );

 /// the raw slot of an object, not built, as `std::allocator` gives it: the
 /// caller builds the object in it (and destroys it before `deallocate()`)
 [[nodiscard]] T * allocate( void );

 /// keeps the slot of \f$ x \f$, whose object is already destroyed (or was
 /// never built), for the next `make()` or `allocate()`
 /// \pre \f$ x \f$ was given by this pool
 void deallocate( T * x );

 /// all the objects and the free slots dropped, with no destructor called,
 /// and the blocks kept for the next objects, from the last block down: a
 /// new epoch, in which nothing of the old ones survives
 ///
 /// In pseudocode (beyond the book), \f$ R \f$ the blocks kept and not yet
 /// used again in the epoch, linked from the last one to the first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{clear}() \cr
 /// 2 & \quad F \gets \textbf{nil};\ \mathit{size} \gets 0 \cr
 /// 3 & \quad \textbf{if}\ \text{there is a block}\ \textbf{then}
 ///     \ [ \mathit{next} , \mathit{end} ) \gets \text{the slots of the}
 ///     \text{ last block};\ R \gets \text{the block before it}
 /// \end{array}}
 /// \f]
 /// and `make()` takes, when \f$ F \f$ is empty and
 /// \f$ \mathit{next} = \mathit{end} \f$, the slots of the block
 /// \f$ R \f$ (and \f$ R \f$ moves to the block before it) while
 /// \f$ R \ne \textbf{nil} \f$, a new block otherwise
 /// \pre no other thread accesses an object of the pool: each of its
 ///      accesses happens before the call
 void clear( void ) noexcept;

 /// whether \f$ p \f$ is a slot given in the current epoch, alive or
 /// freed, in time linear in the blocks: false for a pointer of an old
 /// epoch whose slot has not been given again, true for one whose slot has
 [[nodiscard]] bool fresh( const T * p ) const;

 /// the number of objects alive
 [[nodiscard]] std::size_t size( void ) const;

 /// the memory of the blocks, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// a slot: an object, or the next free slot
 union Slot
 {
  /// the next free slot
  Slot * next;
  /// the room of the object
  alignas( T ) unsigned char object[ sizeof( T ) ];
  };

 /// the header of a block, in its first H slots
 struct Block
 {
  Block * prev;   ///< the block before, nil for the first one
  std::size_t k;  ///< the slots of the block, the header's included
  };

 static constexpr std::size_t H =
   ( sizeof( Block ) + sizeof( Slot ) - 1 ) / sizeof( Slot );

 using SlotAlloc =
   typename std::allocator_traits< A >::template rebind_alloc< Slot >;

 /// frees the blocks
 void release( void ) noexcept;

 /// the slot freed last, or the next one never used, a new block if none
 Slot * slot( void );

 /// the slots of the next block kept by clear(), or of a new block
 void refill( void );

 Slot * f_free = nullptr;    ///< the free slots, the last one freed first
 Slot * f_next = nullptr;    ///< the first slot never used of the last block
 Slot * f_end = nullptr;     ///< the end of the last block
 Block * f_last = nullptr;   ///< the last block, which leads to the others
 Block * f_reuse = nullptr;  ///< the next block kept by clear(), if any
 std::size_t f_n;            ///< the slots of the largest block
 std::size_t f_size = 0;     ///< the objects alive
 ADS_NO_UNIQUE_ADDRESS SlotAlloc f_alloc;  ///< the allocator of the blocks

 };  // end( class ObjectPool )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/ObjectPool.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ObjectPool.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ObjectPool.h --------------------------*/
/*--------------------------------------------------------------------------*/
