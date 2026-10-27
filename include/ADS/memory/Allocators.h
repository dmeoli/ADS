/*--------------------------------------------------------------------------*/
/*--------------------------- File Allocators.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The dynamic storage allocation of Knuth, beyond the book, on a memory of
 * \f$ N \f$ words simulated by a vector, so that the blocks and their
 * headers can be inspected:
 *
 * - `BoundaryTagAllocator` keeps the free blocks on a doubly linked list and
 *   gives a request the first block large enough (first fit, Algorithm
 *   2.5A), the smallest one (best fit), or the first one after the last
 *   block given (next fit, the roving pointer of Knuth); a block is split
 *   if the rest is not too small. Each block has a tag at both ends, its
 *   size and whether it is free (the boundary tags of Algorithm 2.5C), so
 *   that a freed block merges in \f$ O( 1 ) \f$ time with its neighbors if
 *   they are free: two free blocks are never adjacent. A request costs
 *   \f$ O( f ) \f$ time for \f$ f \f$ free blocks, all of them for the best
 *   fit;
 * - `BuddyAllocator` (Knowlton, Algorithms 2.5R and 2.5S) splits a memory
 *   of \f$ 2^m \f$ words into blocks of powers of 2, a request taking the
 *   smallest power not below it, split from a larger free block by halves;
 *   the buddy of the block of size \f$ 2^k \f$ at the address \f$ a \f$ is
 *   at \f$ a \oplus 2^k \f$, so that a freed block merges with its buddy, if
 *   free, and so on upwards. A request and a free cost \f$ O( m ) \f$
 *   steps, each a search of an ordered set of the free blocks of one size,
 *   \f$ O( m \log N ) \f$ time in all; the internal waste is less than half
 *   of a block, and two free buddies are never both on the lists.
 *
 * Both return the address of the first word of the block given, or
 * \f$ -1 \f$ if no block fits. The contracts [see `Contract.h`]: a request
 * is positive, and `free()` gets an address that `allocate()` gave and that
 * is in use; the invariants (the free blocks tile the free memory, never
 * adjacent ones for the boundary tags, never two free buddies) are audits.
 * The operations are defined in `Allocators.tpp`.
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
 enum class Fit {
  FIRST ,   ///< the first one large enough on the list (Algorithm 2.5A)
  BEST ,    ///< the smallest one large enough
  NEXT      ///< the first one large enough after the last one given
  };

 /// the memory of \f$ N \f$ words, one free block, with the choice `fit`
 /// \pre \f$ N \ge 4 \f$
 explicit BoundaryTagAllocator( long N , Fit fit = Fit::FIRST );

 /// a block of at least \f$ n \f$ words, its first word, or \f$ -1 \f$
 ///
 /// In pseudocode (Algorithm 2.5A of TAOCP with the boundary tags of
 /// Algorithm 2.5C, beyond the book), \f$ s( p ) \f$ the words of the block
 /// at \f$ p \f$, two of them its tags:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{allocate}( n ) \cr
 /// 2 & \quad s_0 \gets \max( n + 2 , 4 )\quad \triangleright\ \text{the}
 ///     \text{ tags, and room for the links when free} \cr
 /// 3 & \quad p \gets \text{the free block with } s( p ) \ge s_0 \text{,}
 ///     \text{ first, best or next on the list} \cr
 /// 4 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then return}\ -1 \cr
 /// 5 & \quad \textbf{if}\ s( p ) - s_0 \ge 4\ \textbf{then}\quad
 ///     \triangleright\ \text{split, the upper part given} \cr
 /// 6 & \qquad s( p ) \gets s( p ) - s_0 \text{ in both its tags};\ a \gets p
 ///     + s( p );\ \mathit{rover} \gets \text{the block after } p \cr
 /// 7 & \qquad \text{tag } [ a , a + s_0 ) \text{ in use, of } s_0
 ///     \text{ words} \cr
 /// 8 & \quad \textbf{else} \cr
 /// 9 & \qquad \text{unlink } p \text{ from the list};\ a \gets p;
 ///     \ \text{tag it in use} \cr
 /// 10 & \quad \textbf{return}\ a + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 1 \f$
 long allocate( long n );

 /// frees the block whose first word is \f$ a \f$
 ///
 /// In pseudocode (Algorithm 2.5C of TAOCP, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{free}( a ) \cr
 /// 2 & \quad p \gets a - 1;\ s \gets s( p ) \cr
 /// 3 & \quad \textbf{if}\ \text{the tag at } p - 1 \text{ is free}
 ///     \ \textbf{then}\quad \triangleright\ \text{the block below} \cr
 /// 4 & \qquad q \gets p - s( p - 1 );\ \text{unlink } q;\ s \gets s + s( q
 ///     );\ p \gets q \cr
 /// 5 & \quad \textbf{if}\ p + s < N\ \textbf{and}\ \text{the tag at } p + s
 ///     \text{ is free}\ \textbf{then}\quad \triangleright\ \text{the block}
 ///     \text{ above} \cr
 /// 6 & \qquad \text{unlink } p + s;\ s \gets s + s( p + s ) \cr
 /// 7 & \quad \text{tag } [ p , p + s ) \text{ free};\ \text{link } p
 ///     \text{ at the head of the list}
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \f$ was given by `allocate()` and is in use
 void free( long a );

 /// the number of the words in free blocks, tags included
 [[nodiscard]] long freeWords( void ) const;

 /// the number of the free blocks
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

/// the buddy system on \f$ 2^m \f$ words
class BuddyAllocator {

/*--------------------------------------------------------------------------*/
 public:

 /// the memory of \f$ 2^m \f$ words, one free block
 /// \pre \f$ 0 \le m \le 40 \f$, and \f$ 2^m \f$ fits in a `long`
 ///      (\f$ m \le 30 \f$ where `long` has 32 bits)
 explicit BuddyAllocator( int m );

 /// a block of at least \f$ n \f$ words, a power of 2, its address, or
 /// \f$ -1 \f$
 ///
 /// In pseudocode (Algorithm 2.5R of TAOCP, the buddy system of Knowlton,
 /// beyond the book), \f$ F_k \f$ the free blocks of \f$ 2^k \f$ words:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{allocate}( n ) \cr
 /// 2 & \quad k \gets \lceil \log_2 n \rceil \cr
 /// 3 & \quad j \gets \text{the least } j \ge k \text{ with } F_j \ne
 ///     \emptyset \cr
 /// 4 & \quad \textbf{if}\ \text{there is none}\ \textbf{then return}\ -1 \cr
 /// 5 & \quad a \gets \text{the least block of } F_j;\ F_j \gets F_j
 ///     \setminus \{ a \} \cr
 /// 6 & \quad \textbf{while}\ j > k\ \textbf{do}\quad \triangleright
 ///     \ \text{split, the upper halves free} \cr
 /// 7 & \qquad j \gets j - 1;\ F_j \gets F_j \cup \{ a + 2^j \} \cr
 /// 8 & \quad \textbf{return}\ a
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 1 \f$
 long allocate( long n );

 /// frees the block at \f$ a \f$
 ///
 /// In pseudocode (Algorithm 2.5S of TAOCP, beyond the book), \f$ 2^k \f$
 /// the words of the block:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{free}( a ) \cr
 /// 2 & \quad \textbf{while}\ k < m\ \textbf{and}\ a \oplus 2^k \in F_k
 ///     \ \textbf{do}\quad \triangleright\ \text{the buddy is free} \cr
 /// 3 & \qquad F_k \gets F_k \setminus \{ a \oplus 2^k \};\ a \gets \min( a
 ///     , a \oplus 2^k );\ k \gets k + 1 \cr
 /// 4 & \quad F_k \gets F_k \cup \{ a \}
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \f$ was given by `allocate()` and is in use
 void free( long a );

 /// the number of the words in free blocks
 [[nodiscard]] long freeWords( void ) const;

 /// the size of the block in use at \f$ a \f$
 /// \pre \f$ a \f$ is in use
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
