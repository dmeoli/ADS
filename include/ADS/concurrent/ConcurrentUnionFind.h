/*--------------------------------------------------------------------------*/
/*------------------------ File ConcurrentUnionFind.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The union-find that many threads change at once, beyond the book [see
 * UnionFind.h for the sequential ones]: the n elements 0 , ... , n - 1 in
 * a forest of parents, a root being its own parent, each parent an atomic
 * word changed only by compare-and-swap, with no lock.
 * - find( x ) climbs to the root by splitting: each element on the way is
 *   given, by a compare-and-swap that may fail with no harm, the parent of
 *   its parent, so that the paths halve their lengths as they are walked
 *   (the splitting of Jayanti and Tarjan, the concurrent form of the
 *   path splitting analysed by Tarjan and van Leeuwen);
 * - unite( x , y ) finds the two roots and hooks the larger under the
 *   smaller by a compare-and-swap on the parent of the larger, which
 *   fails, and is tried again from the roots found anew, only if another
 *   thread hooked that root first. Since a root is hooked only under a
 *   smaller element, no cycle can form, and the root of a tree is its
 *   least element: the labels root( x ) are canonical, the same for every
 *   order of the operations (the linking by index of Jayanti and Tarjan
 *   with the identity as the order);
 * - compress( x ) points x to its root, and parent( x ) reads its parent,
 *   so that after the compression of all the elements, with no union
 *   running, parent( x ) is the label of the set of x;
 * - sameSet( x , y ) is true if x and y were in the same set at a moment
 *   of the call, false if they were not at a moment of the call (a root
 *   found for x that is still a root after the root of y is found).
 * The operations are linearizable. On one thread, the linking by index
 * with the splitting takes \f$ O( m \log_{2 + m / n} n ) \f$ steps for m
 * operations on n elements (Tarjan and van Leeuwen); the randomized linking
 * of Jayanti and Tarjan, which hooks by a random order instead of the
 * indices, bounds the expected work on p threads by \f$ O( m ( \alpha( n ,
 * m / ( n p ) ) + \log( n p / m + 1 ) ) ) \f$, at the price of labels that
 * are not the least elements.
 *
 * The contracts [see Contract.h]: the elements are in [ 0 , n ), n
 * < 2^31; size() and the construction are not concurrent with the other
 * operations.
 * \see S. V. Jayanti, R. E. Tarjan, Concurrent disjoint set union,
 *      Distributed Computing 34(6), 413-436, 2021,
 *      doi:10.1007/s00446-020-00388-x
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// the union-find of the elements [ 0 , n ) that threads change at once
class ConcurrentUnionFind {

/*--------------------------------------------------------------------------*/
 public:

 /// n singletons
 /// \pre n < 2^31
 explicit ConcurrentUnionFind( std::size_t n )
  : f_n( n ) , f_p( new std::atomic< std::int32_t >[ n ] ) {
  ADS_PRE( n < ( std::size_t( 1 ) << 31 ) );
  for( std::size_t x = 0 ; x < n ; ++x )
   f_p[ x ].store( std::int32_t( x ) , std::memory_order_relaxed );
  }

 ConcurrentUnionFind( const ConcurrentUnionFind & ) = delete;
 ConcurrentUnionFind & operator=( const ConcurrentUnionFind & ) = delete;

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the root of x, the least element of its set when no union is running
 /// \pre x < size()
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

 /// the parent of x at a moment of the call (x if a root)
 /// \pre x < size()
 [[nodiscard]] std::int32_t parent( std::int32_t x ) const {
  return( f_p[ x ].load( std::memory_order_acquire ) );
  }

 /// x pointed to its root, unless a union moved x first
 /// \pre x < size()
 void compress( std::int32_t x ) {
  std::int32_t p = f_p[ x ].load( std::memory_order_acquire );
  if( p == x )
   return;
  const std::int32_t r = find( p );
  if( r != p )
   f_p[ x ].compare_exchange_strong( p , r , std::memory_order_release ,
                                     std::memory_order_relaxed );
  }

 /// the union of the sets of x and y; false if they were already one
 /// \pre x , y < size()
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

 /// whether x and y are in the same set
 /// \pre x , y < size()
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
