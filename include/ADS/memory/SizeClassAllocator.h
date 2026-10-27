/*--------------------------------------------------------------------------*/
/*----------------------- File SizeClassAllocator.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An allocator of small blocks by size classes, beyond the book: the
 * scheme of the allocators of today (mimalloc, jemalloc, tcmalloc), for
 * one thread, the size of a block given back by its owner (sized
 * deallocation, as `std::allocator` does), so that a block needs no header.
 *
 * A request of \f$ n \f$ bytes, \f$ 1 \le n \le 32768 \f$, goes to the least
 * class not below it: 8, the multiples of 16 up to 128, then four classes
 * per power of 2 (160, 192, 224, 256, 320, ...), i.e., the sizes
 * \f$ 2^e + j \, 2^{e - 2} \f$ for \f$ j = 1 , \ldots , 4 \f$ and
 * \f$ 7 \le e \le 14 \f$, so that the waste is below 25% of a block past
 * 128 bytes (and below 16 bytes up to it), and the class is found by a
 * shift and a count of the leading zeros, with no table and no loop. Each
 * class has a free list of the blocks given back, the last one first (whose
 * memory is still in the cache), and a page of 64 KB (the size of the
 * largest block, if larger) whose rest is cut a block at a time; a request
 * takes the first free block, or the next one of the page, or a new page,
 * in \f$ O( 1 ) \f$ time, and a release puts the block at the head of the
 * list of its class, in \f$ O( 1 ) \f$. The pages go back to the system with
 * the allocator (or by `release()`): memory is not returned while the
 * allocator lives, as with a pool, but the blocks given back serve any
 * later request of their class. A request of more than 32768 bytes goes to
 * `operator new`.
 *
 * The contracts [see `Contract.h`]: a request is of at least one byte, and
 * `deallocate()` gets a block of this allocator with the size of its
 * request. The operations are defined in `SizeClassAllocator.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.5 (the lists of available blocks)
 * \see M. Masmano, I. Ripoll, A. Crespo, J. Real, TLSF: a new dynamic
 *      memory allocator for real-time systems, Proceedings of the 16th
 *      Euromicro Conference on Real-Time Systems (ECRTS), 79-88, 2004,
 *      doi:10.1109/EMRTS.2004.1311009
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
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_SizeClassAllocator
 #define __ADS_SizeClassAllocator
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS SizeClassAllocator -------------------------*/
/*--------------------------------------------------------------------------*/

/// an allocator of small blocks by size classes, for one thread
class SizeClassAllocator {

/*--------------------------------------------------------------------------*/
 public:

 /// the largest request served by the classes
 static constexpr std::size_t LARGEST = 32768;

 /// the bytes of a page
 static constexpr std::size_t PAGE = std::size_t( 1 ) << 16;

 /// the allocator with no page
 SizeClassAllocator( void ) = default;
 /// no copy, since its blocks are in its pages
 SizeClassAllocator( const SizeClassAllocator & ) = delete;
 /// no copy, since its blocks are in its pages
 SizeClassAllocator & operator=( const SizeClassAllocator & ) = delete;

 /// the pages of `other`, which is left empty
 SizeClassAllocator( SizeClassAllocator && other ) noexcept;

 /// the pages of `other`, which is left empty, after those of this allocator
 /// are freed
 SizeClassAllocator & operator=( SizeClassAllocator && other ) noexcept;

 /// frees the pages
 ~SizeClassAllocator() { release(); }

 /// a block of \f$ n \f$ bytes at least, aligned to 16 bytes (8 for the
 /// class of 8 bytes)
 ///
 /// In pseudocode (the lists of available blocks of section 2.5 of TAOCP,
 /// by size class, beyond the book), \f$ F_c \f$ the blocks given back of
 /// the class \f$ c \f$ and \f$ [ \mathit{next}_c , \mathit{end}_c ) \f$
 /// the rest of its page:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{allocate}( n ) \cr
 /// 2 & \quad \textbf{if}\ n > 32768\ \textbf{then return}\ \text{the}
 ///     \text{ memory of operator new} \cr
 /// 3 & \quad c \gets \mathrm{sizeClass}( n );\ s \gets \mathrm{classSize}(
 ///     c ) \cr
 /// 4 & \quad \textbf{if}\ F_c \ne \textbf{nil}\ \textbf{then return}
 ///     \ \text{the pop of } F_c \cr
 /// 5 & \quad \textbf{if}\ \mathit{end}_c - \mathit{next}_c \ge s
 ///     \ \textbf{then} \cr
 /// 6 & \qquad p \gets \mathit{next}_c;\ \mathit{next}_c \gets
 ///     \mathit{next}_c + s;\ \textbf{return}\ p \cr
 /// 7 & \quad \text{take a page of } \max( 2^{16} , s + 16 ) \text{ bytes,}
 ///     \text{ a header of 16 first} \cr
 /// 8 & \quad \textbf{return}\ \text{its first block, the rest in }
 ///     [ \mathit{next}_c , \mathit{end}_c )
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 1 \f$
 [[nodiscard]] void * allocate( std::size_t n );

 /// gives back the block \f$ p \f$ of a request of \f$ n \f$ bytes, pushed
 /// on \f$ F_c \f$ for its class \f$ c \f$
 /// \pre \f$ p \f$ was given by `allocate( n )` of this allocator
 void deallocate( void * p , std::size_t n ) noexcept;

 /// the class of a request of \f$ n \f$ bytes: 0 for \f$ n \le 8 \f$,
 /// \f$ \lceil n / 16 \rceil \f$ up to 128, and otherwise, with
 /// \f$ 2^e < n \le 2^{e + 1} \f$,
 /// \f$ 9 + 4 ( e - 7 ) + \lceil ( n - 2^e ) / 2^{e - 2} \rceil - 1 \f$
 /// \pre \f$ 1 \le n \le 32768 \f$
 [[nodiscard]] static unsigned sizeClass( std::size_t n );

 /// the size of the class \f$ c \f$: 8, \f$ 16 c \f$ up to 128, and
 /// otherwise \f$ 2^e + j \, 2^{e - 2} \f$ with
 /// \f$ e = 7 + \lfloor ( c - 9 ) / 4 \rfloor \f$ and
 /// \f$ j = ( c - 9 ) \bmod 4 + 1 \f$
 [[nodiscard]] static std::size_t classSize( unsigned c );

 /// the memory of the pages, in bytes
 [[nodiscard]] std::size_t bytes( void ) const { return( f_bytes ); }

 /// frees all the pages; the blocks given are no longer valid
 void release( void ) noexcept;

/*--------------------------------------------------------------------------*/
 private:

 /// the classes: 8, 16 .. 128 by 16, then 4 per power of 2 up to LARGEST
 static constexpr unsigned CLASSES = 9 + 4 * 8;

 /// a block given back: the next one of its list
 struct Free {
  /// the next block of the list
  Free * next;
  };

 /// the header of a page, the pages linked for release()
 struct Page {
  /// the page taken before
  Page * prev;
  /// the bytes of the page
  std::size_t bytes;
  };

 /// a new page for the class c, its blocks cut from f_next[ c ]
 void * fresh( unsigned c );

 Free * f_free[ CLASSES ] = {};      ///< the blocks given back
 char * f_next[ CLASSES ] = {};      ///< the next block of the page
 char * f_end[ CLASSES ] = {};       ///< the end of the page
 Page * f_pages = nullptr;           ///< the pages, the last first
 std::size_t f_bytes = 0;            ///< the memory of the pages

 };  // end( class SizeClassAllocator )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/SizeClassAllocator.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SizeClassAllocator.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File SizeClassAllocator.h ---------------------*/
/*--------------------------------------------------------------------------*/
