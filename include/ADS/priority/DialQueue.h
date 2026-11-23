/*--------------------------------------------------------------------------*/
/*---------------------------- File DialQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A monotone bucket queue with overflow, beyond the book: the queue of
 * Dial on a circular window of \f$ W \f$ buckets, \f$ W \f$ a power of 2,
 * the keys out of the window in a radix heap [see `RadixHeap.h`]. It serves
 * the algorithms whose extracted keys never decrease and whose pushed keys
 * stay close to the last one extracted, \f$ \ell \f$: Dijkstra's algorithm
 * with integer weights at most \f$ C < W \f$, where every key pushed is in
 * \f$ [ \ell , \ell + C ] \f$, and the event simulations with bounded
 * delays.
 *
 * A key \f$ k \f$ in \f$ [ \ell , \ell + W ) \f$ goes to the bucket
 * \f$ k \bmod W \f$, a vector used as a stack, which holds a single key
 * since the window is \f$ W \f$ wide; a key beyond it goes to the overflow.
 * A bit per bucket tells whether it has items, so that the next key in use
 * is found by scanning \f$ W / 64 \f$ words at most, from the bucket of
 * \f$ \ell \f$ around the circle. When the overflow holds a key smaller
 * than the first bucket in use (or the buckets are empty), its items in the
 * window \f$ [ \ell' , \ell' + W ) \f$ of its least key \f$ \ell' \f$ move
 * to the buckets, each once, a bucket of the radix heap at a time [see
 * `RadixHeap::extractBelow()`]. Hence, with all the keys pushed in
 * \f$ [ \ell , \ell + C ] \f$, \f$ C < W \f$, the overflow is never used,
 * `push()` takes \f$ O( 1 ) \f$ time and `pop()` \f$ O( 1 + W / w ) \f$,
 * \f$ w = 64 \f$, against the \f$ O( 1 + C ) \f$ of the scan of the buckets
 * one by one; a key pushed farther costs its moves in the radix heap and one
 * move into the window. The buckets keep their memory, so that a queue in a
 * steady state calls no allocator, and their items are contiguous.
 *
 * Between two episodes of a search, `rebase( k )` lets the keys pushed next
 * start from \f$ k \f$, below the last one extracted, provided that no key
 * in the queue is below \f$ k \f$: the items of the buckets and of the
 * overflow are placed again for \f$ \ell = k \f$, in \f$ O( n + W ) \f$
 * time. `peekKey()` gives the least key without moving \f$ \ell \f$, and
 * `extract( theta , out )` moves to `out` every item of key at most
 * \f$ \theta \f$, in increasing order of the keys, reading the least key
 * by `peekKey()`, so that the keys pushed next need only be not smaller
 * than the last one moved.
 *
 * The queue tells its changes through the subject policy `Policy` [see
 * `Modification.h`], `NoObservers` by default, with which every operation
 * compiles to the code of the plain queue, by the events of `RadixHeap`:
 * `Inserted` by `push()`, after it, and `Erased` by `pop()` and
 * `extract()`, before the removal, both with the item as the handle and
 * its key, `Cleared` after `clear()` and `Rebuilt` at the end of a mute;
 * the moves between the overflow and the buckets, and `rebase()`, change
 * neither the items nor their keys, and tell nothing. The same item may be
 * in the queue several times, and a batch coalesces its events by its key
 * as well [see `RadixHeap.h`].
 *
 * The contracts [see `Contract.h`]: a key pushed is not smaller than the
 * last one extracted (any key before the first and after `clear()`),
 * `top()`, `topKey()` and `pop()` need a queue that is not empty; the
 * invariant, checked on queues of at most 1024 items, is that the bits tell
 * the buckets in use and that the buckets and the overflow count `size()`
 * items. The operations are defined in `DialQueue.tpp`.
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
#include "ADS/Modification.h"
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

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DialQueue ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a monotone queue of items with integer keys of the type `K` on a
/// circular window of buckets, the keys beyond it in a radix heap
template< class T , std::integral K = std::uint32_t ,
	  class Policy = NoObservers >
class DialQueue
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the items
 using Key = K;   ///< the type of the keys

 /// an empty queue whose window has room for the keys up to \f$ c \f$
 /// above the last one extracted: \f$ W \f$ buckets, the least power of 2
 /// above \f$ c \f$, and 64 at least
 /// \pre \f$ c \ge 0 \f$
 explicit DialQueue( std::size_t c = 1023 );

 /// the events: an item inserted or erased with its key, the queue emptied
 /// or rebuilt
 using modifications = mod::list< mod::Inserted< T , K > ,
				  mod::Erased< T , K > ,
				  mod::Cleared , mod::Rebuilt >;
 /// the subject
 using Subject = typename Policy::template bind< modifications , DialQueue >;

 /// an empty queue as `DialQueue( c )`, the static observers `o...`
 /// \pre \f$ c \ge 0 \f$
 template< class... O >
  requires( sizeof...( O ) > 0 )
 DialQueue( std::size_t c , O &... o );

 /// pushes \f$ x \f$ with the key \f$ k \f$, in \f$ O( 1 ) \f$ time if
 /// \f$ k < \ell + W \f$
 ///
 /// In pseudocode (the queue of Dial, beyond the book), \f$ \ell \f$ the
 /// last key extracted and \f$ \mathit{used} \f$ the bits of the buckets:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x , k ) \cr
 /// 2 & \quad \textbf{if}\ k - \ell < W\ \textbf{then}\ \text{push } x
 ///     \text{ on } B[ k \bmod W ];\ \mathit{used}[ k \bmod W ] \gets
 ///     \textbf{true} \cr
 /// 3 & \quad \textbf{else}\ \mathrm{push}( O , x , k )\quad
 ///     \triangleright\ \text{the radix heap of the overflow} \cr
 /// 4 & \quad n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \f$ is not smaller than the last key extracted (any key
 ///      before the first extraction and after `clear()`)
 void push( const T & x , K k );

 /// an item of least key, the last pushed among those of its bucket that
 /// did not come through the overflow (equal keys in no given order)
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void );

 /// the least key
 /// \pre the queue is not empty
 [[nodiscard]] K topKey( void );

 /// the least key, which does not move \f$ \ell \f$: the keys pushed next
 /// need only be not smaller than the last one extracted, as if `peekKey()`
 /// were not called
 /// \pre the queue is not empty
 [[nodiscard]] K peekKey( void ) const;

 /// removes `top()` and returns it, in \f$ O( 1 + W / 64 ) \f$ time if the
 /// overflow is not used
 ///
 /// In pseudocode (the queue of Dial, beyond the book), the window moved to
 /// the least key first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad \textbf{if}\ B[ \ell \bmod W ] = \emptyset\ \textbf{then} \cr
 /// 3 & \qquad d \gets \text{the distance from } \ell \bmod W \text{ of}
 ///     \text{ the next bucket in use, by the words of } \mathit{used}
 ///     \text{, or } W \text{ if none} \cr
 /// 4 & \qquad \textbf{if}\ O \ne \emptyset\ \textbf{and}\ ( d = W
 ///     \ \textbf{or}\ \mathrm{peekKey}( O ) < \ell + d )\ \textbf{then} \cr
 /// 5 & \qquad\quad \ell \gets \mathrm{peekKey}( O ) \cr
 /// 6 & \qquad\quad \text{move the items of } O \text{ of keys below }
 ///     \ell + W \text{ to their buckets} \cr
 /// 7 & \qquad\quad d \gets \text{the distance of the next bucket in }
 ///     \text{use, as above} \cr
 /// 8 & \qquad \ell \gets \ell + d \cr
 /// 9 & \quad x \gets \text{the item popped from } B[ \ell \bmod W ] \cr
 /// 10 & \quad \textbf{if}\ B[ \ell \bmod W ] = \emptyset\ \textbf{then}
 ///     \ \mathit{used}[ \ell \bmod W ] \gets \textbf{false} \cr
 /// 11 & \quad n \gets n - 1;\ \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T pop( void );

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// removes the items, the buckets keeping their memory; any key may be
 /// pushed next
 void clear( void );

 /// the number of buckets of the window, \f$ W \f$
 [[nodiscard]] std::size_t window( void ) const { return( f_B.size() ); }

 /// the keys pushed next may start from \f$ k \f$, the items placed again
 /// for \f$ \ell = k \f$, in \f$ O( n + W ) \f$ time
 ///
 /// In pseudocode (beyond the book), the bucket \f$ b \f$ holding the key
 /// \f$ \ell + ( ( b - \ell ) \bmod W ) \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rebase}( k ) \cr
 /// 2 & \quad E \gets \text{the pairs } ( x , k' ) \text{ of the buckets}
 ///     \text{ and of } O;\ \text{empty the buckets and } O \cr
 /// 3 & \quad \ell \gets k;\ n \gets 0 \cr
 /// 4 & \quad \textbf{foreach}\ ( x , k' ) \in E\ \textbf{do}
 ///     \ \mathrm{push}( x , k' )
 /// \end{array}}
 /// \f]
 /// \pre no key in the queue is below \f$ k \f$
 void rebase( K k );

 /// every item of key at most \f$ \theta \f$ appended to `out`, by
 /// increasing keys, each extracted as by `pop()`
 void extract( K theta , std::vector< T > & out );

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class O >
 int attach( O & o )
  requires( Subject::dynamic )
 {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open
 void detach( int id )
  requires( Subject::dynamic )
 {
  f_sub.detach( id );
  }

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

 using Ins = mod::Inserted< T , K >;  ///< an insertion
 using Era = mod::Erased< T , K >;    ///< an erasure

 /// whether someone may want `E`, at compile time
 template< class E >
 static constexpr bool heard = Subject::template listened< E >;

 /// `push()` and `pop()` with no event
 void pushPlain( const T & x , K k );
 T popPlain( void );  ///< as pushPlain()

 /// the changes when someone listens, out of the hot code
 ADS_COLD_PATH void pushCold( const T & x , K k );
 ADS_COLD_PATH T popCold( void );  ///< as pushCold()

 std::vector< std::vector< T > > f_B;  ///< the buckets, LIFO
 std::vector< std::uint64_t > f_bits;  ///< the buckets in use
 U f_cur = 0;                          ///< the last key extracted
 std::size_t f_in = 0;                 ///< the items in the buckets
 std::size_t f_n = 0;                  ///< the number of items
 RadixHeap< T , U > f_over;             ///< the keys beyond the window
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class DialQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/DialQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif /* DialQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File DialQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
