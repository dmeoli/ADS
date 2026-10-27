/*--------------------------------------------------------------------------*/
/*------------------------- File ConcurrentPool.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A pool of size classes shared by threads, beyond the book: an object
 * made by one thread may be freed by any other, e.g., a message created by
 * the thread that changes a model and released by the thread of a solver
 * that has read it. It is a resource of `PoolAllocator` [see
 * `PoolAllocator.h`], with the classes of `PoolResource`: the multiples of 8
 * bytes up to \f$ M \f$, a request of \f$ b \f$ bytes aligned to at most 8
 * taking a slot of \f$ 8 \lceil b / 8 \rceil \f$ bytes, one aligned to 16 a
 * slot of \f$ 16 \lceil b / 16 \rceil \f$ bytes, and any larger or more
 * aligned one going to `operator new`.
 *
 * Each thread that uses the resource has a heap of its own, found by a
 * cache in its thread-local storage (one comparison with the identity of
 * the resource) and, the first time, on a list of the heaps that grows by
 * a compare-and-swap and is read with no lock. A heap is a `PageHeap` [see
 * `PageHeap.h`]: pages of \f$ 2^{16} \f$ bytes of one class each, cut from
 * segments of \f$ 2^{21} \f$ bytes, with a free list per page and a header
 * that names the heap owning the page. The free of a pointer \f$ p \f$
 * finds its page as \f$ p \wedge \neg ( 2^{16} - 1 ) \f$, i.e., by masking
 * the low bits of \f$ p \f$, and then:
 *
 * - if the page belongs to the heap of the calling thread, the slot goes
 *   on the free list of its page, with no atomic operation, so that a
 *   thread that frees what it made pays what it pays in `PoolResource`;
 * - otherwise the slot joins the batch of the calling thread for its
 *   class, which holds the slots of one owning heap, and the batch goes
 *   back to that heap, by one compare-and-swap that links it whole on the
 *   top of the list of the returns of the class there (a stack of
 *   Treiber), when it has 32 slots, when a slot of another heap arrives,
 *   or when the calling thread needs a new page of the class or calls
 *   `collect()`; many threads push, and only the owner takes, the whole
 *   stack at once by exchanging its top with nil when the current page of
 *   that class is exhausted, and puts each slot back on the list of its
 *   page. Since the owner only takes the whole stack, the ABA problem
 *   cannot occur.
 *
 * Thus an allocation costs the pop of the list of the current page or the
 * bump of its pointer, or, when the page is exhausted, one exchange that
 * takes all the returns at once; a free costs a push on the list of a
 * page, or a push on a batch and a compare-and-swap per batch, i.e., per
 * up to 32 frees of a producer and a consumer that work in pairs. The
 * lists of returns of the classes sit on separate lines of cache, so that
 * the threads that free into different classes do not share a line. A
 * slot waits in a batch for at most 31 other frees, so that each thread
 * holds at most 31 slots per class freed and not yet back. A heap is never
 * destroyed before the resource; a thread that ends leaves its heap, whose
 * returns keep coming, to the next thread that has the same identity, and
 * its memory goes back to the system with the resource.
 *
 * `global()` is a resource that lives as long as the program, the default
 * one of `PoolAllocator< T , ConcurrentPoolResource<> >`.
 *
 * The contracts [see `Contract.h`]: a pointer freed was given by the
 * resource and is freed once, with the size and the alignment of its
 * allocation; the resource outlives its objects and its allocators, and
 * `heaps()` and `bytes()` are read while no other thread uses it. The
 * operations are defined in `ConcurrentPool.tpp`.
 *
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986
 * \see E. D. Berger, K. S. McKinley, R. D. Blumofe, P. R. Wilson, Hoard: a
 *      scalable memory allocator for multithreaded applications,
 *      Proceedings of the Ninth International Conference on Architectural
 *      Support for Programming Languages and Operating Systems (ASPLOS IX),
 *      117-128, 2000, doi:10.1145/378993.379232 (the heaps of the threads,
 *      the owner of a block)
 * \see D. Leijen, B. Zorn, L. de Moura, Mimalloc: free list sharding in
 *      action, Programming Languages and Systems (APLAS 2019), Lecture
 *      Notes in Computer Science 11893, 244-265, 2019,
 *      doi:10.1007/978-3-030-34175-6_13 (the pages of one class with their
 *      local and their thread free lists, the page of a pointer by
 *      masking)
 * \see P. Lietar, T. Butler, S. Clebsch, S. Drossopoulou, J. Franco, M. J.
 *      Parkinson, A. Shamis, C. M. Wintersteiger, D. Chisnall, snmalloc: a
 *      message passing allocator, Proceedings of the 2019 ACM SIGPLAN
 *      International Symposium on Memory Management (ISMM 2019), 122-135,
 *      2019, doi:10.1145/3315573.3329980 (the frees of other threads sent
 *      back to the owner in batches, which it takes all at once)
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

#ifndef __ADS_ConcurrentPool
 #define __ADS_ConcurrentPool
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/PageHeap.h"
#include "ADS/memory/PoolAllocator.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <thread>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ConcurrentPoolResource ----------------------*/
/*--------------------------------------------------------------------------*/

/// pools of the size classes of 8 bytes up to \f$ M \f$, a heap per thread,
/// the objects freed by any thread
template< std::size_t M = 256 >
class ConcurrentPoolResource {

 static_assert( ( M >= 16 ) && ( M % 16 == 0 ) && ( M <= 4096 ) ,
                "the largest class is a multiple of 16 bytes, up to 4096" );

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t GRAIN = 8;           ///< the step of a class
 static constexpr std::size_t ALIGN = 16;          ///< the largest alignment
 static constexpr std::size_t MAX = M;             ///< the largest class
 static constexpr std::size_t CLASSES = M / GRAIN; ///< the classes
 /// the bytes of a segment of the heaps
 static constexpr std::size_t SEGMENT = detail::PageHeap< CLASSES >::SEGMENT;

 /// true if a request of \f$ b \f$ bytes aligned to \f$ a \f$ takes a class,
 /// i.e., if \f$ 1 \le b \le M \f$ and \f$ a \le 16 \f$
 [[nodiscard]] static constexpr bool pooled( std::size_t b ,
                                             std::size_t a ) {
  return( ( b >= 1 ) && ( b <= M ) && ( a <= ALIGN ) );
  }

 /// the empty resource, of a new identity
 ConcurrentPoolResource( void );

 /// no copy, since its objects and allocators point to it
 ConcurrentPoolResource( const ConcurrentPoolResource & ) = delete;
 /// no copy, since its objects and allocators point to it
 ConcurrentPoolResource & operator=( const ConcurrentPoolResource & ) =
  delete;

 /// frees the segments and the heaps; the objects still alive are not
 /// destroyed
 ~ConcurrentPoolResource();

 /// the resource of the whole program, never destroyed
 [[nodiscard]] static ConcurrentPoolResource & global( void );

 /// \f$ b \f$ bytes aligned to \f$ a \f$, \f$ b \f$ and \f$ a \f$ known at
 /// compile time
 ///
 /// In pseudocode (the heaps of the threads of Hoard and the batches of
 /// snmalloc, beyond the book), \f$ H \f$ the heap of the calling thread and
 /// \f$ c \f$ the class of the request:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{allocate}( b , a ) \cr
 /// 2 & \quad \textbf{if}\ b > M\ \textbf{or}\ a > 16
 ///     \ \textbf{then return}\ \text{the memory of operator new} \cr
 /// 3 & \quad H \gets \text{the cached heap, else the one of this thread on}
 ///     \text{ the list, else a new one pushed by a CAS} \cr
 /// 4 & \quad s \gets H.\mathrm{local}( c );\ \textbf{if}\ s \ne
 ///     \textbf{nil}\ \textbf{then return}\ s \cr
 /// 5 & \quad \text{send the batch of } H \text{ for } c \cr
 /// 6 & \quad R \gets \mathrm{exchange}( H.\mathit{returns}_c , \textbf{nil}
 ///     )\quad \triangleright\ \text{acquire} \cr
 /// 7 & \quad \textbf{foreach}\ s \in R\ \textbf{do}\ \text{push } s
 ///     \text{ on the list of its page} \cr
 /// 8 & \quad s \gets H.\mathrm{local}( c );\ \textbf{if}\ s \ne
 ///     \textbf{nil}\ \textbf{then return}\ s \cr
 /// 9 & \quad \textbf{return}\ H.\mathrm{more}( c )
 /// \end{array}}
 /// \f]
 template< std::size_t b , std::size_t a = alignof( std::max_align_t ) >
 [[nodiscard]] void * allocate( void );

 /// the memory \f$ p \f$ of `allocate< b , a >()` given back, by any thread
 ///
 /// In pseudocode (the batches of snmalloc, beyond the book), \f$ H \f$ the
 /// heap of the calling thread and \f$ B \f$ its batch for the class
 /// \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{deallocate}( p ) \cr
 /// 2 & \quad g \gets p \wedge \neg ( 2^{16} - 1 );\ O \gets \text{the}
 ///     \text{ owner of the page } g \cr
 /// 3 & \quad \textbf{if}\ O = H\ \textbf{then}\ \text{push } p \text{ on}
 ///     \text{ the list of } g;\ \textbf{return} \cr
 /// 4 & \quad \textbf{if}\ B \text{ goes to a heap other than } O
 ///     \ \textbf{then}\ \mathrm{send}( B );\ B \gets \text{a batch for } O
 ///     \cr
 /// 5 & \quad \text{push } p \text{ on } B \cr
 /// 6 & \quad \textbf{if}\ |B| = 32\ \textbf{then}\ \mathrm{send}( B ) \cr
 /// 7 & \textbf{procedure}\ \mathrm{send}( B ) \cr
 /// 8 & \quad \textbf{if}\ B = \emptyset\ \textbf{then return} \cr
 /// 9 & \quad \textbf{repeat}\ B.\mathit{tail}.\mathit{next} \gets
 ///     O.\mathit{returns}_c\ \textbf{until}\ \mathrm{CAS}(
 ///     O.\mathit{returns}_c , B.\mathit{tail}.\mathit{next} ,
 ///     B.\mathit{head} ) \cr
 /// 10 & \quad B \gets \emptyset\quad \triangleright\ \text{the CAS with}
 ///     \text{ release}
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ was given by `allocate< b , a >()`, by any thread
 template< std::size_t b , std::size_t a = alignof( std::max_align_t ) >
 void deallocate( void * p );

 /// \f$ b \f$ bytes aligned to \f$ a \f$, as `allocate< b , a >()` above
 [[nodiscard]] void * allocate( std::size_t b ,
                                std::size_t a = alignof( std::max_align_t )
                                );

 /// the memory \f$ p \f$ of `allocate( b , a )` given back, by any thread,
 /// as `deallocate< b , a >()` above
 /// \pre \f$ p \f$ was given by `allocate( b , a )`, by any thread
 void deallocate( void * p , std::size_t b ,
                  std::size_t a = alignof( std::max_align_t ) );

 /// the batches of the calling thread go back to their heaps, and the
 /// slots that other threads freed into its heap go on its free lists
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{collect}() \cr
 /// 2 & \quad \textbf{foreach}\ \text{class } c\ \textbf{do} \cr
 /// 3 & \qquad \text{send the batch of } H \text{ for } c \cr
 /// 4 & \qquad \textbf{foreach}\ s \in \mathrm{exchange}(
 ///     H.\mathit{returns}_c , \textbf{nil} )\ \textbf{do}\ \text{push } s
 ///     \text{ on the list of its page}
 /// \end{array}}
 /// \f]
 void collect( void );

 /// the number of heaps, one per thread that used the resource
 [[nodiscard]] std::size_t heaps( void ) const;

 /// the memory of the segments, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Pages = detail::PageHeap< CLASSES >;
 using Free = typename Pages::Free;
 using Page = typename Pages::Page;

 /// the returns of a class, on a line of cache of their own
 struct alignas( 64 ) Returns {
  /// the top of the stack of the returns
  std::atomic< Free * > top{ nullptr };
  };

 struct Heap;

 /// a batch of slots of a class freed by a thread for another heap
 struct Batch {
  Heap * to = nullptr;                ///< the heap they go back to
  Free * head = nullptr;              ///< the slots, linked
  /// the last slot of the batch
  Free * tail = nullptr;
  std::uint32_t n = 0;                ///< their number
  };

 /// the slots of a batch, at most
 static constexpr std::uint32_t BATCH = 32;

 /// the heap of a thread
 struct Heap {
  std::thread::id id;                 ///< its thread
  Heap * link = nullptr;              ///< the heap made before
  Pages pages{ this };                ///< its pages
  Batch out[ CLASSES ];               ///< the slots freed for others
  Returns returns[ CLASSES ];         ///< the slots freed by others
  };

 /// sends the batch b to its heap, one compare-and-swap
 static void send( Batch & b , std::size_t c );

 /// the class of b bytes aligned to a: b rounded up to a multiple of 8,
 /// or of 16 if a > 8, so that the slots of the class are aligned to a
 [[nodiscard]] static constexpr std::size_t classOf( std::size_t b ,
                                                     std::size_t a ) {
  return( ( a > GRAIN ? ( b + 15 ) / 16 * 2 : ( b + 7 ) / 8 ) - 1 );
  }

 /// the identity of the resource and the heap of a thread
 struct Cache {
  /// the identity of the resource, 0 for none
  std::uint64_t id = 0;
  /// the heap of the thread in that resource
  Heap * heap = nullptr;
  };

 /// the heap of the calling thread
 [[nodiscard]] Heap * mine( void );

 /// the heap of the calling thread, found or made on the list
 Heap * find( void );

 /// a slot of the class c
 [[nodiscard]] void * take( std::size_t c );

 /// a slot of the class c when the list of the current page is empty
 void * refill( Heap & h , std::size_t c );

 /// frees a slot of the class c
 void give( void * p , std::size_t c );

 inline static thread_local Cache s_cache;
 inline static std::atomic< std::uint64_t > s_ids{ 0 };

 const std::uint64_t f_id;                    ///< never 0, never reused
 std::atomic< Heap * > f_heaps{ nullptr };    ///< the last heap made

 };  // end( class ConcurrentPoolResource )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/ConcurrentPool.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ConcurrentPool.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ConcurrentPool.h ------------------------*/
/*--------------------------------------------------------------------------*/
