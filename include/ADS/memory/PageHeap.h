/*--------------------------------------------------------------------------*/
/*---------------------------- File PageHeap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The heap of pages of size classes under `PoolResource` [see
 * `PoolAllocator.h`] and `ConcurrentPoolResource` [see `ConcurrentPool.h`],
 * beyond the book: slots of \f$ ( c + 1 ) g \f$ bytes, \f$ g = 8 \f$, for
 * the classes \f$ c = 0 , \dots , C - 1 \f$, with the free lists sharded by
 * page.
 *
 * The heap takes segments of \f$ 2^{21} \f$ bytes, aligned to their size
 * and advised for the huge pages [see `HugePageAllocator.h`], and cuts them
 * into pages of \f$ 2^{16} \f$ bytes; a page holds the slots of one class
 * after a header of 64 bytes, so that the page of a slot \f$ p \f$ is
 * \f$ p \wedge \neg ( 2^{16} - 1 ) \f$. Each page has a free list of its
 * own, and the slots never given yet are cut from it in the order of their
 * addresses. A class allocates from its current page, the pop of its free
 * list or the bump of its pointer; when the page is exhausted, the class
 * moves to the next page of its list of the pages with free slots, or to a
 * new page, and the exhausted page leaves the list until a slot of it is
 * freed, which puts it back. A free is the push on the list of the page of
 * the slot, plus the test of a flag; both operations cost \f$ O( 1 ) \f$
 * time, amortized over the slots of a page for the change of page, and a
 * page of the class \f$ c \f$ wastes less than \f$ ( c + 1 ) g \f$ bytes at
 * its end besides its header.
 *
 * So the objects made one after another come from one page, even after
 * frees in a scrambled order: a single free list, as that of `ObjectPool`
 * [see `ObjectPool.h`], gives back the slots in the order of the frees,
 * scattered over all the memory, while the lists by page give them back a
 * page at a time, so that the nodes of a container built again are close
 * to each other and the walks over them miss the cache less. The memory
 * goes back to the system only with the heap.
 *
 * The contracts [see `Contract.h`]: a slot freed belongs to the heap and is
 * freed once. The operations are defined at the end of the header.
 *
 * \see D. Leijen, B. Zorn, L. de Moura, Mimalloc: free list sharding in
 *      action, Programming Languages and Systems (APLAS 2019), Lecture
 *      Notes in Computer Science 11893, 244-265, 2019,
 *      doi:10.1007/978-3-030-34175-6_13 (the free lists by page, the page
 *      of a pointer by masking)
 * \see J. Bonwick, The slab allocator: an object-caching kernel memory
 *      allocator, Proceedings of the USENIX Summer 1994 Technical
 *      Conference, 1994,
 *      www.usenix.org/publications/library/proceedings/bos94/bonwick.html (the
 *      slabs of one size with their own free lists, the list of the slabs
 *      partly free)
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

#ifndef __ADS_PageHeap
 #define __ADS_PageHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS PageHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// pages of \f$ C \f$ size classes of 8 bytes, a free list per page
template< std::size_t C >
class alignas( 64 ) PageHeap {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t GRAIN = 8;           ///< the step of a class
 /// the bytes of a page
 static constexpr std::size_t PAGE = std::size_t( 1 ) << 16;
 /// the bytes of a segment
 static constexpr std::size_t SEGMENT = std::size_t( 1 ) << 21;
 static constexpr std::size_t HEADER = 64;         ///< the header of a page

 static_assert( C * GRAIN <= ( PAGE - HEADER ) / 8 ,
                "a page holds at least 8 slots of the largest class" );

 /// a free slot
 struct Free {
  /// the next free slot of the page
  Free * next;
  };

 /// the header of a page
 struct Page {
  Free * free;           ///< the free slots
  char * next;           ///< the first slot never given
  char * end;            ///< the end of the slots
  Page * link;           ///< the next page with free slots
  void * owner;          ///< the owner of the heap
  std::uint32_t c;       ///< the class
  bool listed;           ///< current, or on the list of its class
  };

 static_assert( sizeof( Page ) <= HEADER );

 /// the empty heap of the owner `owner`
 explicit PageHeap( void * owner = nullptr );

 /// no copy, since its pages name it as their owner
 PageHeap( const PageHeap & ) = delete;
 /// no copy, since its pages name it as their owner
 PageHeap & operator=( const PageHeap & ) = delete;

 /// frees the segments
 ~PageHeap();

 /// the page of the slot \f$ p \f$, \f$ p \wedge \neg ( 2^{16} - 1 ) \f$
 [[nodiscard]] static Page * pageOf( void * p ) {
  return( reinterpret_cast< Page * >( reinterpret_cast< std::uintptr_t >(
           p ) & ~std::uintptr_t( PAGE - 1 ) ) );
  }

 /// a slot of the class \f$ c \f$ from its current page, the free list or
 /// the slots never given, nil if the page is exhausted
 ///
 /// In pseudocode (the free lists by page of Leijen et al., beyond the
 /// book), \f$ P \f$ the current page of \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{local}( c ) \cr
 /// 2 & \quad \textbf{if}\ P.\mathit{free} \ne \textbf{nil}\ \textbf{then}
 ///     \ \textbf{return}\ \text{the pop of } P.\mathit{free} \cr
 /// 3 & \quad \textbf{if}\ P.\mathit{next} \ne P.\mathit{end}
 ///     \ \textbf{then} \cr
 /// 4 & \qquad s \gets P.\mathit{next};\ P.\mathit{next} \gets
 ///     P.\mathit{next} + 8 ( c + 1 );\ \textbf{return}\ s \cr
 /// 5 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] void * local( std::size_t c ) {
  Page * const p = f_class[ c ].current;
  if( Free * const f = p->free ) [[likely]] {
   p->free = f->next;
   ++f_live;
   return( f );
   }
  if( p->next != p->end ) {                 // a slot never given
   void * const s = p->next;
   p->next += ( c + 1 ) * GRAIN;
   ++f_live;
   return( s );
   }
  return( nullptr );
  }

 /// a slot of the class \f$ c \f$ when `local()` gives nil
 ///
 /// In pseudocode (the free lists by page of Leijen et al. and the slabs
 /// partly free of Bonwick, beyond the book), \f$ A_c \f$ the list of the
 /// pages of \f$ c \f$ with free slots:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{more}( c ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad s \gets \mathrm{local}( c );\ \textbf{if}\ s \ne
 ///     \textbf{nil}\ \textbf{then return}\ s \cr
 /// 4 & \qquad \text{mark } P \text{ as not listed} \cr
 /// 5 & \qquad \textbf{if}\ A_c \ne \emptyset\ \textbf{then}\ P \gets
 ///     \text{the pop of } A_c \cr
 /// 6 & \qquad \textbf{else}\ P \gets \text{a new page of } c \text{, from}
 ///     \text{ the segment, a new segment if it is used up}
 /// \end{array}}
 /// \f]
 [[nodiscard]] void * more( std::size_t c );

 /// frees the slot \f$ p \f$ of the page \f$ g \f$
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{give}( g , p ) \cr
 /// 2 & \quad \text{push } p \text{ on } g.\mathit{free} \cr
 /// 3 & \quad \textbf{if}\ g \text{ is not listed}\ \textbf{then}
 ///     \ \text{mark it listed and push it on } A_{g.c}
 /// \end{array}}
 /// \f]
 /// \pre \f$ g \f$ is `pageOf( p )`, a page of the heap
 void give( Page * g , void * p ) {
  Free * const f = static_cast< Free * >( p );
  f->next = g->free;
  g->free = f;
  --f_live;
  if( ! g->listed ) [[unlikely]] {          // back among the pages to use
   g->listed = true;
   g->link = f_class[ g->c ].avail;
   f_class[ g->c ].avail = g;
   }
  }

 /// the number of the slots given and not freed
 [[nodiscard]] std::size_t live( void ) const { return( f_live ); }

 /// the memory of the segments, in bytes
 [[nodiscard]] std::size_t bytes( void ) const {
  return( f_segments.size() * SEGMENT );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a class: its current page and the other pages with free slots
 struct Class {
  Page * current;
  Page * avail = nullptr;
  };

 /// the page of no slot, current of every class at the start
 inline static Page s_none{ nullptr , nullptr , nullptr , nullptr ,
                            nullptr , 0 , true };

 /// a new page of the class c
 Page * fresh( std::size_t c );

 Class f_class[ C ];                  ///< the classes
 char * f_page = nullptr;             ///< the next page of the segment
 char * f_last = nullptr;             ///< the end of the segment
 std::size_t f_live = 0;              ///< the slots given
 void * f_owner;                      ///< the owner of the heap
 std::vector< char * > f_segments;    ///< all the segments

 };  // end( class PageHeap )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::size_t C >
PageHeap< C >::PageHeap( void * owner ) : f_owner( owner )
{
 for( Class & k : f_class )
  k.current = &s_none;
 }

template< std::size_t C >
PageHeap< C >::~PageHeap()
{
 for( char * s : f_segments )
  HugePageAllocator< char >().deallocate( s , SEGMENT );
 }

template< std::size_t C >
typename PageHeap< C >::Page * PageHeap< C >::fresh( std::size_t c )
{
 if( f_page == f_last ) {                   // a new segment
  f_segments.reserve( f_segments.size() + 1 );
  char * s = HugePageAllocator< char >().allocate( SEGMENT );
  f_segments.push_back( s );
  f_page = s;
  f_last = s + SEGMENT;
  }
 char * const p = f_page;
 f_page += PAGE;
 const std::size_t b = ( c + 1 ) * GRAIN;
 return( ::new( static_cast< void * >( p ) ) Page{ nullptr , p + HEADER ,
          p + HEADER + ( ( PAGE - HEADER ) / b ) * b , nullptr , f_owner ,
          std::uint32_t( c ) , true } );
 }

template< std::size_t C >
void * PageHeap< C >::more( std::size_t c )
{
 Class & k = f_class[ c ];
 for( ; ; ) {
  Page * const p = k.current;
  if( Free * f = p->free ) {
   p->free = f->next;
   ++f_live;
   return( f );
   }
  if( p->next != p->end ) {                 // a slot never given
   void * s = p->next;
   p->next += ( c + 1 ) * GRAIN;
   ++f_live;
   return( s );
   }
  if( p != &s_none )                        // exhausted: off the list
   p->listed = false;
  if( Page * q = k.avail ) {
   k.avail = q->link;
   k.current = q;
   }
  else
   k.current = fresh( c );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/

#endif  /* PageHeap.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File PageHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
