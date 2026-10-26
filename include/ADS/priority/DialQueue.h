/*--------------------------------------------------------------------------*/
/*---------------------------- File DialQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A monotone bucket queue with overflow, beyond the book: the queue of
 * Dial on a circular window of W buckets, W a power of 2, the keys out of
 * the window in a radix heap [see RadixHeap.h]. It serves the algorithms
 * whose extracted keys never decrease and whose pushed keys stay close to
 * the last one extracted, \f$ \ell \f$: Dijkstra's algorithm with integer
 * weights at most C < W, where every key pushed is in
 * \f$ [ \ell , \ell + C ] \f$, and the event simulations with bounded
 * delays.
 *
 * A key k in \f$ [ \ell , \ell + W ) \f$ goes to the bucket
 * \f$ k \bmod W \f$, a vector used as a stack, which holds a single key
 * since the window is W wide; a key beyond it goes to the overflow. A bit
 * per bucket tells whether it has items, so that the next key in use is
 * found by scanning W / 64 words at most, from the bucket of
 * \f$ \ell \f$ around the circle. When the overflow holds a key smaller
 * than the first bucket in use (or the buckets are empty), its items in
 * the window
 * \f$ [ \ell' , \ell' + W ) \f$ of its least key \f$ \ell' \f$ move to
 * the buckets, each once, a bucket of the radix heap at a time [see
 * RadixHeap::extractBelow()]. Hence, with all the keys pushed in
 * \f$ [ \ell , \ell + C ] \f$, C < W, the overflow is never used, push()
 * takes O( 1 ) time and pop() \f$ O( 1 + W / w ) \f$, w = 64, against
 * the \f$ O( 1 + C ) \f$ of the scan of the buckets one by one; a key
 * pushed farther costs its moves in the radix heap and one move into the
 * window. The buckets keep their memory, so that a queue in a steady state
 * calls no allocator, and their items are contiguous.
 *
 * The contracts [see Contract.h]: a key pushed is not smaller than the
 * last one extracted (any key before the first and after clear()), top(),
 * topKey() and pop() need a
 * queue that is not empty; the invariant, checked on queues of at most
 * 1024 items, is that the bits tell the buckets in use and that the
 * buckets and the overflow count size() items. The operations are defined
 * in DialQueue.tpp.
 *
 * \see R. B. Dial, Algorithm 360: shortest-path forest with topological
 *      ordering, Communications of the ACM 12(11), 632-633, 1969,
 *      doi:10.1145/363269.363610
 * \see B. V. Cherkassky, A. V. Goldberg, T. Radzik, Shortest paths
 *      algorithms: theory and experimental evaluation, Mathematical
 *      Programming 73(2), 129-174, 1996, doi:10.1007/BF02592101
 * \see R. K. Ahuja, K. Mehlhorn, J. Orlin, R. E. Tarjan, Faster algorithms
 *      for the shortest path problem, Journal of the ACM 37(2), 213-223,
 *      1990, doi:10.1145/77600.77615
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

#ifndef __ADS_DialQueue
 #define __ADS_DialQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/RadixHeap.h"

#include <bit>
#include <limits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DialQueue ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a monotone queue of items with integer keys K on a circular window of
/// buckets, the keys beyond it in a radix heap
template< class T , std::integral K = std::uint32_t >
class DialQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Key = K;    ///< the type of the keys

 /// an empty queue whose window has room for the keys up to c above the
 /// last one extracted: W, the least power of 2 above c, buckets
 /// \pre c >= 0
 explicit DialQueue( std::size_t c = 1023 );

 /// pushes x with the key k
 /// \pre k is not smaller than the last key extracted (any key before the
 ///      first extraction and after clear())
 void push( const T & x , K k );

 /// an item of least key, the last pushed among those of its bucket that
 /// did not come through the overflow (equal keys in no given order)
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void );

 /// the least key
 /// \pre the queue is not empty
 [[nodiscard]] K topKey( void );

 /// removes top() and returns it
 /// \pre the queue is not empty
 T pop( void );

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// removes the items, the buckets keeping their memory; any key may be
 /// pushed next
 void clear( void );

 /// the number of buckets of the window
 [[nodiscard]] std::size_t window( void ) const { return( f_B.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 using U = std::make_unsigned_t< K >;
 static U encode( K k ) {
  if constexpr( std::is_signed_v< K > )
   return( U( k ) ^ ( U( 1 ) << ( std::numeric_limits< U >::digits - 1 ) ) );
  else
   return( k );
  }

 static K decode( U u ) {
  if constexpr( std::is_signed_v< K > )
   return( K( u ^ ( U( 1 ) << ( std::numeric_limits< U >::digits - 1 ) ) ) );
  else
   return( u );
  }

 /// x in the bucket of the key u, in the window
 void put( const T & x , U u );

 /// f_cur on the least key, its bucket not empty
 void pull( void );

 /// the first bucket in use from the one of f_cur around the circle, as
 /// the distance from it, or the window if none
 [[nodiscard]] std::size_t next( void ) const;

 /// the bits tell the buckets in use, and the buckets and the overflow
 /// count size() items
 [[nodiscard]] bool counted( void ) const;

 std::vector< std::vector< T > > f_B;   ///< the buckets, LIFO
 std::vector< std::uint64_t > f_bits;   ///< the buckets in use
 U f_cur = 0;                           ///< the last key extracted
 std::size_t f_in = 0;                  ///< the items in the buckets
 std::size_t f_n = 0;                   ///< the number of items
 RadixHeap< T , U > f_over;             ///< the keys beyond the window

 };  // end( class DialQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/DialQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DialQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File DialQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
