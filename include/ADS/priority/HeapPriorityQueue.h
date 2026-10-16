/*--------------------------------------------------------------------------*/
/*------------------------ File HeapPriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a heap.
 *
 * HeapPriorityQueue is the heap of section 10.1.3, a vector H of capacity
 * n holding a d-ary tree that is full but for the rightmost leaves of
 * the last level, each node not smaller than its parent: the children of
 * the place i are \f$ d i + 1 , \ldots , d i + d \f$ and its parent is
 * \f$ \lfloor ( i - 1 ) / d \rfloor \f$ (with d = 2, l( i ) = 2 i + 1,
 * r( i ) = 2 i + 2, p( i ) = \f$ \lfloor ( i - 1 ) / 2 \rfloor \f$).
 * insert() and decrease() move the item up, deleteMin() moves the last
 * leaf down from the root [see minHeapRestore()], in
 * \f$ O( \log_d n ) \f$ and \f$ O( d \log_d n ) \f$ time; each item knows
 * its place, kept by swap(), and so the handle finds it (exercise 10.6
 * for d > 2).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.512284
 * \see D. B. Johnson, Priority queues with update and finding minimum
 *      spanning trees, Information Processing Letters 4(3), 53-57, 1975,
 *      doi:10.1016/0020-0190(75)90001-0

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

#ifndef __ADS_HeapPriorityQueue
 #define __ADS_HeapPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS HeapPriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a d-ary min-heap of bounded capacity
template< class T , std::totally_ordered P = int , int D = 2 >
 requires( D >= 2 )
class HeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 /// the PRIORITYITEM of the book: priority, item, place in the vector
 struct PriorityItem {
  P priority;
  T value;
  int pos;
  };

 using Item = T;                  ///< the type of the items
 using Priority = P;              ///< the type of the priorities
 using Handle = PriorityItem *;   ///< what insert() gives

 /// PriorityQueue( n ): the empty queue of capacity n
 explicit HeapPriorityQueue( int n ) : f_capacity( n ) {
  if( n < 0 )
   throw( std::invalid_argument( "ADS::HeapPriorityQueue: n < 0" ) );
  f_H.reserve( n );
  }

 [[nodiscard]] bool isEmpty( void ) const { return( f_H.empty() ); }

 [[nodiscard]] int size( void ) const { return( int( f_H.size() ) ); }

 [[nodiscard]] int capacity( void ) const { return( f_capacity ); }

 [[nodiscard]] const T & min( void ) const {
  if( f_H.empty() )
   throw( std::logic_error( "ADS::HeapPriorityQueue::min: empty" ) );
  return( f_H[ 0 ]->value );
  }

 /// the smallest priority
 [[nodiscard]] const P & minPriority( void ) const {
  if( f_H.empty() )
   throw( std::logic_error( "ADS::HeapPriorityQueue::min: empty" ) );
  return( f_H[ 0 ]->priority );
  }

 /// the last leaf goes to the root and down, O( d log_d n ) time; the
 /// handle of the item removed is no longer valid
 T deleteMin( void ) {
  if( f_H.empty() )
   throw( std::logic_error( "ADS::HeapPriorityQueue::deleteMin: empty" ) );
  const int dim = size() - 1;
  swap( 0 , dim );
  minHeapRestore( 0 , dim );
  T v = std::move( f_H.back()->value );
  f_H.pop_back();
  return( v );
  }

 /// a new leaf goes up, O( log_d n ) time
 Handle insert( const T & x , const P & p ) {
  if( size() >= f_capacity )
   throw( std::overflow_error( "ADS::HeapPriorityQueue::insert: full" ) );
  const int i = size();
  f_H.push_back( std::make_unique< PriorityItem >(
                                           PriorityItem{ p , x , i } ) );
  Handle h = f_H.back().get();
  up( i );
  return( h );
  }

 /// lowers the priority of y to p, which must not be larger
 void decrease( Handle y , const P & p ) {
  if( y->priority < p )
   throw( std::invalid_argument(
                      "ADS::HeapPriorityQueue::decrease: larger priority" ) );
  y->priority = p;
  up( y->pos );
  }

 /// the heap: the items, by their place
 [[nodiscard]] std::vector< P > priorities( void ) const {
  std::vector< P > out;
  for( const auto & h : f_H )
   out.push_back( h->priority );
  return( out );
  }

 /// the parent of the place i > 0
 static int parent( int i ) { return( ( i - 1 ) / D ); }

 /// the j-th child, 0 <= j < d, of the place i
 static int child( int i , int j ) { return( D * i + j + 1 ); }

/*--------------------------------------------------------------------------*/
 private:

 /// swaps the places i and j, keeping the positions of the items
 void swap( int i , int j ) {
  std::swap( f_H[ i ] , f_H[ j ] );
  f_H[ i ]->pos = i;
  f_H[ j ]->pos = j;
  }

 void up( int i ) {
  while( ( i > 0 ) &&
         ( f_H[ i ]->priority < f_H[ parent( i ) ]->priority ) ) {
   swap( i , parent( i ) );
   i = parent( i );
   }
  }

 /// the subtrees of the children of i are heaps in [ 0 , dim ): the item in
 /// i goes down, swapped with its smallest child
 void minHeapRestore( int i , int dim ) {
  for( ; ; ) {
   int min = i;
   for( int j = 0 ; j < D ; ++j ) {
    const int c = child( i , j );
    if( ( c < dim ) && ( f_H[ c ]->priority < f_H[ min ]->priority ) )
     min = c;
    }
   if( min == i )
    return;
   swap( i , min );
   i = min;
   }
  }

 int f_capacity;                                    ///< the capacity
 std::vector< std::unique_ptr< PriorityItem > > f_H; ///< the heap

 };  // end( class HeapPriorityQueue )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* HeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File HeapPriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
