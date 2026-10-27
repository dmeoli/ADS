/*--------------------------------------------------------------------------*/
/*------------------------ File ConcurrentUnionFind.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The union-find that many threads change at once, beyond the book [see
 * `UnionFind.h` for the sequential ones]: the \f$ n \f$ elements
 * \f$ 0 , \ldots , n - 1 \f$ in a forest of parents, a root being its own
 * parent, each parent an atomic word changed only by compare-and-swap,
 * with no lock.
 *
 * - `find( x )` climbs to the root by splitting: each element on the way is
 *   given, by a compare-and-swap that may fail with no harm, the parent of
 *   its parent, so that the paths halve their lengths as they are walked
 *   (the splitting of Jayanti and Tarjan, the concurrent form of the path
 *   splitting analyzed by Tarjan and van Leeuwen);
 * - `unite( x , y )` finds the two roots and hooks the larger under the
 *   smaller by a compare-and-swap on the parent of the larger, which fails,
 *   and is tried again from the roots found anew, only if another thread
 *   hooked that root first. Since a root is hooked only under a smaller
 *   element, no cycle can form, and the root of a tree is its least
 *   element: the labels \f$ \mathrm{root}( x ) \f$ are canonical, the same
 *   for every order of the operations (the linking by index of Jayanti and
 *   Tarjan with the identity as the order);
 * - `compress( x )` points \f$ x \f$ to its root, and `parent( x )` reads
 *   its parent, so that after the compression of all the elements, with no
 *   union running, `parent( x )` is the label of the set of \f$ x \f$;
 * - `sameSet( x , y )` is true if \f$ x \f$ and \f$ y \f$ were in the same
 *   set at a moment of the call, false if they were not at a moment of the
 *   call (a root found for \f$ x \f$ that is still a root after the root of
 *   \f$ y \f$ is found).
 *
 * The operations are linearizable. On one thread, the linking by index with
 * the splitting takes \f$ O( m \log_{2 + m / n} n ) \f$ steps for \f$ m \f$
 * operations on \f$ n \f$ elements (Tarjan and van Leeuwen); the randomized
 * linking of Jayanti and Tarjan, which hooks by a random order instead of
 * the indices, bounds the expected work on \f$ p \f$ threads by
 * \f$ O( m ( \alpha( n , m / ( n p ) ) + \log( n p / m + 1 ) ) ) \f$, at
 * the price of labels that are not the least elements.
 *
 * The contracts [see `Contract.h`]: the elements are in \f$ [ 0 , n ) \f$,
 * \f$ n < 2^{31} \f$; `size()` and the construction are not concurrent with
 * the other operations.
 *
 * \see S. V. Jayanti, R. E. Tarjan, Concurrent disjoint set union,
 *      Distributed Computing 34(6), 413-436, 2021,
 *      doi:10.1007/s00446-020-00388-x
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ConcurrentUnionFind
 #define __ADS_ConcurrentUnionFind
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ConcurrentUnionFind -----------------------*/
/*--------------------------------------------------------------------------*/

/// the union-find of the elements \f$ [ 0 , n ) \f$ that threads change at
/// once
class ConcurrentUnionFind {

/*--------------------------------------------------------------------------*/
 public:

 /// the \f$ n \f$ singletons \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$
 /// \pre \f$ n < 2^{31} \f$
 explicit ConcurrentUnionFind( std::size_t n )
  : f_n( n ) , f_p( new std::atomic< std::int32_t >[ n ] ) {
  ADS_PRE( n < ( std::size_t( 1 ) << 31 ) );
  for( std::size_t x = 0 ; x < n ; ++x )
   f_p[ x ].store( std::int32_t( x ) , std::memory_order_relaxed );
  }

 /// no copy, since the threads share it by its address
 ConcurrentUnionFind( const ConcurrentUnionFind & ) = delete;
 /// no copy, since the threads share it by its address
 ConcurrentUnionFind & operator=( const ConcurrentUnionFind & ) = delete;

 /// the number \f$ n \f$ of the elements
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the root of \f$ x \f$, the least element of its set when no union is
 /// running
 ///
 /// In pseudocode (the splitting of Jayanti and Tarjan, beyond the book),
 /// \f$ p[ x ] \f$ the parent of \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( x ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad u \gets p[ x ];\ g \gets p[ u ]\quad \triangleright
 ///     \ \text{acquire} \cr
 /// 4 & \qquad \textbf{if}\ u = g\ \textbf{then return}\ u \cr
 /// 5 & \qquad \mathrm{CAS}( p[ x ] , u , g )\quad \triangleright
 ///     \ \text{release, its failure harmless} \cr
 /// 6 & \qquad x \gets u
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le x < n \f$
 [[nodiscard]] std::int32_t find( std::int32_t x ) {
  ADS_PRE( ( x >= 0 ) && ( std::size_t( x ) < f_n ) );
  for( ;; ) {
   std::int32_t p = f_p[ x ].load( std::memory_order_acquire );
   const std::int32_t g = f_p[ p ].load( std::memory_order_acquire );
   if( p == g )
    return( p );
   f_p[ x ].compare_exchange_weak( p , g , std::memory_order_release ,
                                   std::memory_order_relaxed );
   x = p;
   }
  }

 /// the parent of \f$ x \f$ at a moment of the call (\f$ x \f$ if a root)
 /// \pre \f$ 0 \le x < n \f$
 [[nodiscard]] std::int32_t parent( std::int32_t x ) const {
  return( f_p[ x ].load( std::memory_order_acquire ) );
  }

 /// \f$ x \f$ pointed to its root, unless a union moved \f$ x \f$ first
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{compress}( x ) \cr
 /// 2 & \quad u \gets p[ x ] \cr
 /// 3 & \quad \textbf{if}\ u \ne x\ \textbf{then} \cr
 /// 4 & \qquad r \gets \mathrm{find}( u ) \cr
 /// 5 & \qquad \textbf{if}\ r \ne u\ \textbf{then}\ \mathrm{CAS}( p[ x ] ,
 ///     u , r )
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le x < n \f$
 void compress( std::int32_t x ) {
  std::int32_t p = f_p[ x ].load( std::memory_order_acquire );
  if( p == x )
   return;
  const std::int32_t r = find( p );
  if( r != p )
   f_p[ x ].compare_exchange_strong( p , r , std::memory_order_release ,
                                     std::memory_order_relaxed );
  }

 /// the union of the sets of \f$ x \f$ and \f$ y \f$; false if they were
 /// already one
 ///
 /// In pseudocode (the linking by index of Jayanti and Tarjan, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{unite}( x , y ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad x \gets \mathrm{find}( x );\ y \gets \mathrm{find}( y ) \cr
 /// 4 & \qquad \textbf{if}\ x = y\ \textbf{then return false} \cr
 /// 5 & \qquad \textbf{if}\ x < y\ \textbf{then}\ \text{swap } x
 ///     \text{ and } y \cr
 /// 6 & \qquad \textbf{if}\ \mathrm{CAS}( p[ x ] , x , y )
 ///     \ \textbf{then return true}\quad \triangleright\ \text{the larger}
 ///     \text{ root hooked}
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le x , y < n \f$
 bool unite( std::int32_t x , std::int32_t y ) {
  for( ;; ) {
   x = find( x );
   y = find( y );
   if( x == y )
    return( false );
   if( x < y )
    std::swap( x , y );                     // hook x, the larger root
   std::int32_t r = x;
   if( f_p[ x ].compare_exchange_strong( r , y , std::memory_order_acq_rel ,
                                         std::memory_order_relaxed ) )
    return( true );
   }
  }

 /// whether \f$ x \f$ and \f$ y \f$ are in the same set
 ///
 /// In pseudocode (Jayanti and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{sameSet}( x , y ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad x \gets \mathrm{find}( x );\ y \gets \mathrm{find}( y ) \cr
 /// 4 & \qquad \textbf{if}\ x = y\ \textbf{then return true} \cr
 /// 5 & \qquad \textbf{if}\ p[ x ] = x\ \textbf{then return false}\quad
 ///     \triangleright\ x \text{ still a root after } y \text{ was found}
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le x , y < n \f$
 [[nodiscard]] bool sameSet( std::int32_t x , std::int32_t y ) {
  for( ;; ) {
   x = find( x );
   y = find( y );
   if( x == y )
    return( true );
   if( f_p[ x ].load( std::memory_order_acquire ) == x )
    return( false );                        // x was a root after y found
   }
  }

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_n;                          ///< the elements
 std::unique_ptr< std::atomic< std::int32_t >[] > f_p;   ///< the parents
 };  // end( class ConcurrentUnionFind )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ConcurrentUnionFind.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File ConcurrentUnionFind.h ---------------------*/
/*--------------------------------------------------------------------------*/
