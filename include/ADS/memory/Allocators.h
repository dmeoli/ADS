/*--------------------------------------------------------------------------*/
/*--------------------------- File Allocators.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The dynamic storage allocation of Knuth, beyond the book, on a memory of
 * N words simulated by a vector, so that the blocks and their headers can
 * be inspected:
 *
 * - BoundaryTagAllocator keeps the free blocks on a doubly linked list and
 *   gives a request the first block large enough (first fit, Algorithm 2.5A),
 *   the smallest one (best fit), or the first one after the last block given
 *   (next fit, the roving pointer of Knuth); a block is split if the rest is
 *   not too small. Each block has a tag at both ends, its size and whether it
 *   is free (the boundary tags of Algorithm 2.5C), so that a freed block
 *   merges in \f$ O( 1 ) \f$ time with its neighbours if they are free: two
 *   free blocks are never adjacent;
 * - BuddyAllocator (Knowlton, Algorithms 2.5R and 2.5S) splits a memory of
 *   \f$ 2^m \f$ words into blocks of powers of 2, a request taking the
 *   smallest power not below it, split from a larger free block by halves; the
 *   buddy of the block of size \f$ 2^k \f$ at the address a is at
 *   \f$ a \oplus 2^k \f$, so that a freed block merges with its buddy, if
 *   free, and so on upwards, each in \f$ O( m ) \f$ time; the internal waste
 *   is at most half of a block, and two free buddies are never both on the
 *   lists.
 *
 * Both return the address of the first word of the block given, or -1 if
 * no block fits. The contracts [see Contract.h]: a request is positive, and
 * free() gets an address that allocate() gave and that is in use; the
 * invariants (the free blocks tile the free memory, never adjacent ones for
 * the boundary tags, never two free buddies) are audits. The operations are
 * defined in Allocators.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.5
 * \see K. C. Knowlton, A fast storage allocator, Communications of the ACM
 *      8(10), 623-624, 1965, doi:10.1145/365628.365655
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

#ifndef __ADS_Allocators
 #define __ADS_Allocators
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <limits>
#include <map>
#include <set>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BoundaryTagAllocator ----------------------*/
/*--------------------------------------------------------------------------*/

/// the dynamic storage allocation with boundary tags
class BoundaryTagAllocator {

/*--------------------------------------------------------------------------*/
 public:

 /// the block chosen for a request
 enum class Fit { FIRST , BEST , NEXT };

 /// a memory of N words, all free
 /// \pre N >= 4
 explicit BoundaryTagAllocator( long N , Fit fit = Fit::FIRST );

 /// a block of at least n words, or -1
 /// \pre n >= 1
 long allocate( long n );

 /// frees the block at a
 /// \pre a was given by allocate() and is in use
 void free( long a );

 /// the words in free blocks
 [[nodiscard]] long freeWords( void ) const;

 /// the number of free blocks
 [[nodiscard]] long freeBlocks( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 // a block of s words at p: M[ p ] = M[ p + s - 1 ] = +s if free, -s if in
 // use; a free block has also M[ p + 1 ] = next, M[ p + 2 ] = prev on the
 // list of the free blocks, whose head is the place N (two extra words)

 void unlink( long p );
 void link( long p );

 /// the free blocks tile the free memory, none adjacent, \f$ O( N ) \f$
 [[nodiscard]] bool tiled( void ) const;

 std::vector< long > M;   ///< the memory and the head of the list
 long N;
 Fit f_fit;
 long f_rover;            ///< where next fit starts
 std::set< long > f_used; ///< the blocks in use, for the contracts

 };  // end( class BoundaryTagAllocator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BuddyAllocator --------------------------*/
/*--------------------------------------------------------------------------*/

/// the buddy system on 2^m words
class BuddyAllocator {

/*--------------------------------------------------------------------------*/
 public:

 /// a memory of 2^m words, all free
 /// \pre 0 <= m <= 40, and 2^m fits in a long (m <= 30 where long has
 /// 32 bits)
 explicit BuddyAllocator( int m );

 /// a block of at least n words, a power of 2, or -1
 /// \pre n >= 1
 long allocate( long n );

 /// frees the block at a
 /// \pre a was given by allocate() and is in use
 void free( long a );

 /// the words in free blocks
 [[nodiscard]] long freeWords( void ) const;

 /// the size of the block in use at a
 /// \pre a is in use
 [[nodiscard]] long blockSize( long a ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// no two free buddies, and the free blocks disjoint, \f$ O( N \log N ) \f$
 [[nodiscard]] bool merged( void ) const;

 int f_m;
 std::vector< std::set< long > > f_free;   ///< the free blocks of 2^k
 std::map< long , int > f_used;            ///< the blocks in use, by k

 };  // end( class BuddyAllocator )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/Allocators.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Allocators.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Allocators.h --------------------------*/
/*--------------------------------------------------------------------------*/
