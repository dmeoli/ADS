/*--------------------------------------------------------------------------*/
/*---------------------------- File MinMaxHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two double-ended priority queues of plain items, beyond the book: the
 * best and the worst item by better( a , b ) (a before b) both at hand,
 * both removable in \f$ O( \log n ) \f$ time.
 *
 * - MinMaxHeap is the min-max heap of Atkinson, Sack, Santoro and
 *   Strothotte: a binary heap in a vector whose levels alternate, the even
 *   ones (the root's) ordered for the best item and the odd ones for the
 *   worst, so that a node of an even level is not worse than any item of
 *   its subtree and one of an odd level not better. The best item is the
 *   root, the worst the worse of its two children. A push moves the item
 *   to the right kind of level by at most one swap with its parent and
 *   then up by grandparents; a removal moves the last item down from the
 *   root (or from the child) by the best (worst) of the up to 4
 *   grandchildren and 2 children: \f$ \lfloor \log_2 n \rfloor + 1 \f$
 *   comparisons up and about \f$ 1.5 \log_2 n \f$ down per level pair;
 * - IntervalHeap is the interval heap of van Leeuwen and Wood: the items
 *   two by two in the nodes of a complete binary tree, each node an
 *   interval [ lo , hi ] (lo not worse than hi) contained in the interval
 *   of its parent, so that the lo of the nodes are a heap for the best
 *   and the hi a heap for the worst; the root holds the best and the
 *   worst item. A tree of half the height of a binary heap, each level of
 *   a walk two comparisons at most, and the two items of a node in one line
 *   of the cache: about \f$ \log_2 ( n / 2 ) \f$ levels for a push and a
 *   removal, faster in practice than the min-max heap.
 *
 * A run of removals with no push, as long as an eighth of the items and 64 at
 * least, sorts the items once (by the radix sort for the numbers and the pairs
 * of numbers in the order of std::less or std::greater [see OrderKey.h]), so
 * that the rest of the run takes them from the two ends of the sorted array in
 * \f$ O( 1 ) \f$ time each, as a balanced tree does; the next push makes the
 * array a heap again in \f$ O( n ) \f$ time (an interval heap straight from
 * the sorted items, the node j holding the j-th best and the j-th worst; a
 * min-max heap by the construction from the bottom up of Atkinson et al.).
 * Both costs are paid by the run, \f$ O( \log n ) \f$ amortized per removal.
 *
 * The contracts [see Contract.h]: min(), max(), popMin() and popMax()
 * need a queue that is not empty; the invariant, checked on queues of at
 * most 1024 items, is the order of the levels (the intervals). The
 * operations are defined in MinMaxHeap.tpp.
 *
 * \see M. D. Atkinson, J.-R. Sack, N. Santoro, T. Strothotte, Min-max
 *      heaps and generalized priority queues, Communications of the ACM
 *      29(10), 996-1000, 1986, doi:10.1145/6617.6621
 * \see J. van Leeuwen, D. Wood, Interval heaps, The Computer Journal
 *      36(3), 209-216, 1993, doi:10.1093/comjnl/36.3.209
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

#ifndef __ADS_MinMaxHeap
 #define __ADS_MinMaxHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/OrderKey.h"
#include "ADS/sort/Sort.h"

#include <bit>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MinMaxHeap -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a min-max heap of plain items: the best and the worst by better()
template< class T , class Better = std::less< T > >
class MinMaxHeap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 explicit MinMaxHeap( Better better = Better() )
  : f_better( std::move( better ) ) {}

 void push( const T & x );

 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// \pre the queue is not empty
 [[nodiscard]] const T & max( void ) const;

 /// removes the best item and returns it
 /// \pre the queue is not empty
 T popMin( void );

 /// removes the worst item and returns it
 /// \pre the queue is not empty
 T popMax( void );

 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }
 [[nodiscard]] std::size_t size( void ) const {
  return( f_sorted ? f_hi - f_lo : f_A.size() );
  }
 void reserve( std::size_t n ) { f_A.reserve( n ); }
 void clear( void ) {
  f_A.clear();
  f_sorted = false;
  f_run = 0;
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the place of the worst item
 [[nodiscard]] std::size_t maxAt( void ) const;

 /// the place i is on a level for the best items
 static bool minLevel( std::size_t i ) {
  return( ( std::bit_width( i + 1 ) - 1 ) % 2 == 0 );
  }

 /// a before b on the levels of the kind of Min
 template< bool Min >
 bool first( const T & a , const T & b ) const {
  return( Min ? f_better( a , b ) : f_better( b , a ) );
  }

 /// the item at i up by its grandparents on its kind of level
 template< bool Min >
 void up( std::size_t i );

 /// the item at i down by its children and grandchildren
 template< bool Min >
 void down( std::size_t i );

 /// the order of the levels, on at most 1024 items
 [[nodiscard]] bool ordered( void ) const;

 /// a run of removals: the items sorted, once the run is an eighth of them
 void run( void );

 /// the items made a heap again from their sorted range
 void unsort( void );

 std::vector< T > f_A;   ///< the heap, or the items sorted
 Better f_better;        ///< the order
 bool f_sorted = false;  ///< the items sorted in f_A[ f_lo .. f_hi )
 std::size_t f_lo = 0 , f_hi = 0;
 std::size_t f_run = 0;  ///< the removals since the last push

 };  // end( class MinMaxHeap )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS IntervalHeap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an interval heap of plain items: the best and the worst by better()
template< class T , class Better = std::less< T > >
class IntervalHeap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 explicit IntervalHeap( Better better = Better() )
  : f_better( std::move( better ) ) {}

 void push( const T & x );

 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// \pre the queue is not empty
 [[nodiscard]] const T & max( void ) const;

 /// removes the best item and returns it
 /// \pre the queue is not empty
 T popMin( void );

 /// removes the worst item and returns it
 /// \pre the queue is not empty
 T popMax( void );

 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }
 [[nodiscard]] std::size_t size( void ) const {
  return( f_sorted ? f_hi - f_lo : f_A.size() );
  }
 void reserve( std::size_t n ) { f_A.reserve( n ); }
 void clear( void ) {
  f_A.clear();
  f_sorted = false;
  f_run = 0;
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the place of the hi of the node j, its lo if it has one item
 [[nodiscard]] std::size_t hi( std::size_t j ) const {
  return( 2 * j + 1 < f_A.size() ? 2 * j + 1 : 2 * j );
  }

 /// the intervals contained in those of their parents, on at most 1024
 /// items
 [[nodiscard]] bool ordered( void ) const;

 /// a run of removals: the items sorted, once the run is an eighth of them
 void run( void );

 /// the items made a heap again from their sorted range
 void unsort( void );

 /// the nodes, the node j in 2 j and 2 j + 1, or the items sorted
 std::vector< T > f_A;
 Better f_better;        ///< the order
 bool f_sorted = false;  ///< the items sorted in f_A[ f_lo .. f_hi )
 std::size_t f_lo = 0 , f_hi = 0;
 std::size_t f_run = 0;  ///< the removals since the last push

 };  // end( class IntervalHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/MinMaxHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MinMaxHeap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MinMaxHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
