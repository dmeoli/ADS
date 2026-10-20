/*--------------------------------------------------------------------------*/
/*---------------------------- File RadixHeap.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A monotone priority queue of integer keys, beyond the book: the radix
 * heap of Ahuja, Mehlhorn, Orlin and Tarjan, in the form with digits of B
 * bits of their two-level heap. It serves the algorithms whose extracted
 * keys never decrease, i.e., Dijkstra's algorithm with nonnegative integer
 * weights and the event simulations, where a key pushed is never smaller
 * than the last one extracted, \f$ \ell \f$.
 *
 * A key x is a word of W bits (a signed key has its sign bit flipped,
 * which keeps the order), split into \f$ \lceil W / B \rceil \f$ digits of
 * B bits. Let r be the digit of the highest bit in which x and
 * \f$ \ell \f$ differ (r = 0 if \f$ x = \ell \f$), and \f$ x_r \f$ the
 * digit r of x: x lies in the bucket \f$ ( r , x_r ) \f$, of index
 * \f$ 2^B r + x_r \f$. Since \f$ x \geq \ell \f$ and the two agree above
 * the digit r, every key of a bucket is smaller than every key of a bucket
 * of larger index, and the keys of a bucket of the row 0 are all equal to
 * \f$ \ell \f$ but for the last digit, i.e., they are equal. Hence the
 * minimum is in the first bucket that is not empty, found by the lowest
 * bit of a mask of the buckets in use; if this bucket is in the row
 * \f$ r > 0 \f$, its least key, kept as the items are pushed, becomes the
 * new \f$ \ell \f$ and its items are moved to their buckets for it, all in
 * rows below r. The other buckets keep their items: a key of the row
 * \f$ r' \geq r \f$ agrees with the new \f$ \ell \f$ in the same digits as
 * with the old one.
 *
 * Each item thus moves at most \f$ \lceil W / B \rceil - 1 \f$ times, so
 * that, with the potential \f$ \Phi = \sum_x row( x ) \f$, push() takes
 * O( 1 ) time and pop() \f$ O( W / B + 2^B W / ( B w ) ) \f$ amortized,
 * the second term the scan of the mask of \f$ 2^B \lceil W / B \rceil \f$
 * bits in words of w = 64; for the keys of a band of width C above
 * \f$ \ell \f$ only the \f$ \lceil \log_2 ( C + 1 ) / B \rceil \f$ lowest
 * rows are ever used. With B = 1 it is the radix heap of the paper,
 * \f$ O( \log C ) \f$ amortized per pop(); larger digits trade fewer moves
 * for more buckets, as in the two-level heap of the paper. The buckets are
 * vectors that keep their memory, so that a queue in a steady state calls
 * no allocator, and the items of a bucket come out last pushed first.
 *
 * The contracts [see Contract.h]: a key pushed is not smaller than the
 * last one extracted or read by top() and topKey() (any key before the
 * first of them and after clear()), and top(), topKey(), peekKey() and
 * pop() need a
 * queue that is not empty; the invariant, checked on queues of at most 1024
 * items, is that each item is in its bucket and that the mask and the
 * least keys of the buckets are right. The operations are defined in
 * RadixHeap.tpp.
 *
 * \see R. K. Ahuja, K. Mehlhorn, J. Orlin, R. E. Tarjan, Faster algorithms
 *      for the shortest path problem, Journal of the ACM 37(2), 213-223,
 *      1990, doi:10.1145/77600.77615
 * \see B. V. Cherkassky, A. V. Goldberg, T. Radzik, Shortest paths
 *      algorithms: theory and experimental evaluation, Mathematical
 *      Programming 73(2), 129-174, 1996, doi:10.1007/BF02592101
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

#ifndef __ADS_RadixHeap
 #define __ADS_RadixHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RadixHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a monotone queue of items with integer keys K, the least key first, in
/// buckets by digits of B bits
template< class T , std::integral K = std::uint32_t , unsigned B = 4 >
class RadixHeap {

 static_assert( ( B >= 1 ) && ( B <= 8 ) , "a digit of 1 to 8 bits" );

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Key = K;    ///< the type of the keys

 /// pushes x with the key k
 /// \pre k is not smaller than the last key extracted or read by top() and
 ///      topKey() (any key before the first of them and after clear())
 void push( const T & x , K k );

 /// an item of least key, the last pushed among those
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void );

 /// the least key
 /// \pre the queue is not empty
 [[nodiscard]] K topKey( void );

 /// the least key, read from the least key of the first bucket in use,
 /// which does not move the buckets: the keys pushed next need only be not
 /// smaller than the last one extracted, as if topKey() were not called
 /// \pre the queue is not empty
 [[nodiscard]] K peekKey( void ) const;

 /// removes top() and returns it
 /// \pre the queue is not empty
 T pop( void );

 [[nodiscard]] bool empty( void ) const { return( f_size == 0 ); }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// removes the items, the buckets keeping their memory; any key may be
 /// pushed next
 void clear( void );

/*--------------------------------------------------------------------------*/
 private:

 using U = std::make_unsigned_t< K >;

 static constexpr unsigned W = std::numeric_limits< U >::digits;
 static constexpr unsigned ROWS = ( W + B - 1 ) / B;
 static constexpr unsigned R = 1u << B;
 static constexpr unsigned NB = ROWS * R;          ///< the buckets
 static constexpr unsigned NW = ( NB + 63 ) / 64;  ///< words of the mask

 /// an item with its encoded key
 struct Entry {
  U key;
  T item;
  };

 /// the key as an unsigned word of the same order
 static U encode( K k ) {
  if constexpr( std::is_signed_v< K > )
   return( U( k ) ^ ( U( 1 ) << ( W - 1 ) ) );
  else
   return( k );
  }

 static K decode( U u ) {
  if constexpr( std::is_signed_v< K > )
   return( K( u ^ ( U( 1 ) << ( W - 1 ) ) ) );
  else
   return( u );
  }

 /// the bucket of the key x for the last key extracted
 unsigned bucket( U x ) const {
  const U d = x ^ f_last;
  const unsigned r = d ? unsigned( std::bit_width( d ) - 1 ) / B : 0;
  return( r * R + unsigned( ( x >> ( r * B ) ) & U( R - 1 ) ) );
  }

 /// puts e in the bucket i
 void place( unsigned i , Entry && e ) {
  if( e.key < f_min[ i ] ) f_min[ i ] = e.key;
  f_mask[ i >> 6 ] |= std::uint64_t( 1 ) << ( i & 63 );
  f_B[ i ].push_back( std::move( e ) );
  }

 /// the first bucket that is not empty
 unsigned first( void ) const {
  for( unsigned w = 0 ; ; ++w )
   if( f_mask[ w ] )
    return( w * 64 + unsigned( std::countr_zero( f_mask[ w ] ) ) );
  }

 /// f_cur on a bucket of the row 0 holding the least key, f_last that key
 void pull( void );

 /// every item in its bucket, the mask and the least keys right
 [[nodiscard]] bool bucketed( void ) const;

 std::array< std::vector< Entry > , NB > f_B;   ///< the buckets
 std::array< U , NB > f_min = fill_max();       ///< their least keys
 std::array< std::uint64_t , NW > f_mask = {};  ///< the buckets in use
 U f_last = 0;                ///< the last key extracted
 unsigned f_cur = 0;          ///< the bucket of the top, if pulled
 std::size_t f_size = 0;      ///< the number of items

 static std::array< U , NB > fill_max( void ) {
  std::array< U , NB > a;
  a.fill( std::numeric_limits< U >::max() );
  return( a );
  }

 };  // end( class RadixHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/RadixHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RadixHeap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RadixHeap.h --------------------------*/
/*--------------------------------------------------------------------------*/
