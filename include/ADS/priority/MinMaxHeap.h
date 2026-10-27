/*--------------------------------------------------------------------------*/
/*---------------------------- File MinMaxHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two double-ended priority queues of plain items, beyond the book: the
 * best and the worst item by `better( a , b )` (\f$ a \f$ before \f$ b \f$)
 * both at hand, both removable in \f$ O( \log n ) \f$ time.
 *
 * - `MinMaxHeap` is the min-max heap of Atkinson, Sack, Santoro and
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
 * - `IntervalHeap` is the interval heap of van Leeuwen and Wood: the items
 *   two by two in the nodes of a complete binary tree, each node an
 *   interval \f$ [ \mathit{lo} , \mathit{hi} ] \f$ (\f$ \mathit{lo} \f$ not
 *   worse than \f$ \mathit{hi} \f$) contained in the interval of its parent,
 *   so that the \f$ \mathit{lo} \f$ of the nodes are a heap for the best and
 *   the \f$ \mathit{hi} \f$ a heap for the worst; the root holds the best
 *   and the worst item. A tree of half the height of a binary heap, each
 *   level of a walk two comparisons at most, and the two items of a node in
 *   one line of the cache: about \f$ \log_2 ( n / 2 ) \f$ levels for a push
 *   and a removal, faster in practice than the min-max heap.
 *
 * A run of removals with no push, as long as an eighth of the items and 64 at
 * least, sorts the items once (by the radix sort for the numbers and the
 * pairs of numbers in the order of `std::less` or `std::greater` [see
 * `OrderKey.h`]), so that the rest of the run takes them from the two ends of
 * the sorted array in \f$ O( 1 ) \f$ time each, as a balanced tree does; the
 * next push makes the array a heap again in \f$ O( n ) \f$ time (an interval
 * heap straight from the sorted items, the node \f$ j \f$ holding the
 * \f$ j \f$-th best and the \f$ j \f$-th worst; a min-max heap by the
 * construction from the bottom up of Atkinson et al.). Both costs are paid
 * by the run, \f$ O( \log n ) \f$ amortized per removal.
 *
 * The contracts [see `Contract.h`]: `min()`, `max()`, `popMin()` and
 * `popMax()` need a queue that is not empty; the invariant, checked on
 * queues of at most 1024 items, is the order of the levels (the intervals).
 * The operations are defined in `MinMaxHeap.tpp`.
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

/// a min-max heap of plain items: the best and the worst by `better()`
template< class T , class Better = std::less< T > >
class MinMaxHeap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty queue of the order `better`
 explicit MinMaxHeap( Better better = Better() )
  : f_better( std::move( better ) ) {}

 /// pushes \f$ x \f$, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Atkinson, Sack, Santoro and Strothotte, beyond the
 /// book), \f$ \mathrm{up}_{\min} \f$ and \f$ \mathrm{up}_{\max} \f$
 /// moving an item up by its grandparents while better (worse) than them:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad \textbf{if}\  \text{the items are sorted }\ \textbf{then}
 ///     \ \text{make them a heap from the bottom up} \cr
 /// 3 & \quad i \gets n;\ A[ i ] \gets x;\ n \gets n + 1;\ p \gets
 ///     \lfloor ( i - 1 ) / 2 \rfloor \cr
 /// 4 & \quad \textbf{if}\ i = 0\ \textbf{then return} \cr
 /// 5 & \quad \textbf{if}\ i \text{ is on a level for the best}
 ///     \ \textbf{then} \cr
 /// 6 & \qquad \textbf{if}\ \mathrm{better}( A[ p ] , A[ i ] )
 ///     \ \textbf{then}\ \text{swap } A[ i ] \text{ and } A[ p ];
 ///     \ \mathrm{up}_{\max}( p ) \cr
 /// 7 & \qquad \textbf{else}\ \mathrm{up}_{\min}( i ) \cr
 /// 8 & \quad \textbf{else if}\ \mathrm{better}( A[ i ] , A[ p ] )
 ///     \ \textbf{then}\ \text{swap } A[ i ] \text{ and } A[ p ];
 ///     \ \mathrm{up}_{\min}( p ) \cr
 /// 9 & \quad \textbf{else}\ \mathrm{up}_{\max}( i )
 /// \end{array}}
 /// \f]
 void push( const T & x );

 /// the best item, at the root
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the worst item, the worse of the two children of the root
 /// \pre the queue is not empty
 [[nodiscard]] const T & max( void ) const;

 /// removes the best item and returns it, in \f$ O( \log n ) \f$
 /// amortized time
 ///
 /// In pseudocode (Atkinson, Sack, Santoro and Strothotte, beyond the
 /// book), \f$ \mathrm{down}_{\min} \f$ the walk down for the best;
 /// \f$ \mathrm{down}_{\max} \f$ is the same with the order reversed:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popMin}() \cr
 /// 2 & \quad \textbf{if}\  \text{a run of removals reached}
 ///     \max\{ 64 , n / 8 \} \text{ items }\ \textbf{then}
 ///     \ \text{sort the items} \cr
 /// 3 & \quad \textbf{if}\  \text{the items are sorted }\ \textbf{then}
 ///     \textbf{return}\ \text{the first of them, removed} \cr
 /// 4 & \quad x \gets A[ 0 ];\ n \gets n - 1;\ A[ 0 ] \gets A[ n ];
 ///     \ \mathrm{down}_{\min}( 0 );\ \textbf{return}\ x \cr
 /// 5 & \textbf{procedure}\ \mathrm{down}_{\min}( i ) \cr
 /// 6 & \quad \textbf{while}\ 2 i + 1 < n\ \textbf{do} \cr
 /// 7 & \qquad m \gets \text{the best of the children and the}
 ///     \text{ grandchildren of } i \cr
 /// 8 & \qquad \textbf{if not}\ \mathrm{better}( A[ m ] , A[ i ] )
 ///     \ \textbf{then return} \cr
 /// 9 & \qquad \text{swap } A[ m ] \text{ and } A[ i ] \cr
 /// 10 & \qquad \textbf{if}\ m \text{ is a child of } i\ \textbf{then}
 ///     \textbf{return} \cr
 /// 11 & \qquad p \gets \lfloor ( m - 1 ) / 2 \rfloor \cr
 /// 12 & \qquad \textbf{if}\ \mathrm{better}( A[ p ] , A[ m ] )
 ///     \ \textbf{then}\ \text{swap } A[ p ] \text{ and } A[ m ] \cr
 /// 13 & \qquad i \gets m
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T popMin( void );

 /// removes the worst item and returns it, in \f$ O( \log n ) \f$
 /// amortized time
 ///
 /// In pseudocode (Atkinson, Sack, Santoro and Strothotte, beyond the
 /// book), with \f$ \mathrm{down}_{\max} \f$ the walk of `popMin()` with
 /// the order reversed:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popMax}() \cr
 /// 2 & \quad \textbf{if}\  \text{a run of removals reached}
 ///     \max\{ 64 , n / 8 \} \text{ items }\ \textbf{then}
 ///     \ \text{sort the items} \cr
 /// 3 & \quad \textbf{if}\  \text{the items are sorted }\ \textbf{then}
 ///     \textbf{return}\ \text{the last of them, removed} \cr
 /// 4 & \quad k \gets \text{the place of the worst item, } 0 , 1
 ///     \text{ or } 2 \cr
 /// 5 & \quad x \gets A[ k ];\ n \gets n - 1 \cr
 /// 6 & \quad \textbf{if}\ k < n\ \textbf{then}\ A[ k ] \gets A[ n ];
 ///     \ \mathrm{down}_{\max}( k ) \cr
 /// 7 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T popMax( void );

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const {
  return( f_sorted ? f_hi - f_lo : f_A.size() );
  }

 /// room for \f$ n \f$ items, so that the pushes up to \f$ n \f$ do not
 /// reallocate
 void reserve( std::size_t n ) { f_A.reserve( n ); }

 /// removes all the items
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

/// an interval heap of plain items: the best and the worst by `better()`
template< class T , class Better = std::less< T > >
class IntervalHeap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty queue of the order `better`
 explicit IntervalHeap( Better better = Better() )
  : f_better( std::move( better ) ) {}

 /// pushes \f$ x \f$, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (van Leeuwen and Wood, beyond the book), the node
 /// \f$ j \f$ in the places \f$ 2 j \f$ (its \f$ \mathit{lo} \f$) and
 /// \f$ 2 j + 1 \f$ (its \f$ \mathit{hi} \f$), \f$ q \f$ the parent of
 /// \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad \textbf{if}\  \text{the items are sorted }\ \textbf{then}
 ///     \ \text{make them the nodes of the } j \text{-th best and worst} \cr
 /// 3 & \quad i \gets n;\ A[ i ] \gets x;\ n \gets n + 1;\ j \gets
 ///     \lfloor i / 2 \rfloor \cr
 /// 4 & \quad \textbf{if}\ i \text{ is odd}\ \textbf{then}\quad
 ///     \triangleright\ \text{the second item of its node} \cr
 /// 5 & \qquad \mathit{low} \gets \mathrm{better}( A[ i ] , A[ i - 1 ] );
 ///     \ \textbf{if}\ \mathit{low}\ \textbf{then}\ \text{swap } A[ i ]
 ///     \text{ and } A[ i - 1 ] \cr
 /// 6 & \quad \textbf{else if}\ j = 0\ \textbf{then return} \cr
 /// 7 & \quad \textbf{else if}\ \mathrm{better}( x , A[ 2 q ] )
 ///     \ \textbf{then}\ \mathit{low} \gets \textbf{true} \cr
 /// 8 & \quad \textbf{else if}\ \mathrm{better}( A[ 2 q + 1 ] , x )
 ///     \ \textbf{then}\ \mathit{low} \gets \textbf{false} \cr
 /// 9 & \quad \textbf{else return}\quad \triangleright\ \text{inside}
 ///     \text{ the interval of its parent} \cr
 /// 10 & \quad \textbf{if}\ \mathit{low}\ \textbf{then}\ \text{move the}
 ///     \text{ item up by the } \mathit{lo} \text{ of the ancestors while}
 ///     \text{ better} \cr
 /// 11 & \quad \textbf{else}\ \text{move it up by the } \mathit{hi}
 ///     \text{ of the ancestors while worse}
 /// \end{array}}
 /// \f]
 void push( const T & x );

 /// the best item, the \f$ \mathit{lo} \f$ of the root
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the worst item, the \f$ \mathit{hi} \f$ of the root
 /// \pre the queue is not empty
 [[nodiscard]] const T & max( void ) const;

 /// removes the best item and returns it, in \f$ O( \log n ) \f$
 /// amortized time
 ///
 /// In pseudocode (van Leeuwen and Wood, beyond the book), the hole of the
 /// \f$ \mathit{lo} \f$ of the root moved down by the better
 /// \f$ \mathit{lo} \f$ of the children:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popMin}() \cr
 /// 2 & \quad \textbf{if}\  \text{a run of removals reached}
 ///     \max\{ 64 , n / 8 \} \text{ items }\ \textbf{then}
 ///     \ \text{sort the items} \cr
 /// 3 & \quad \textbf{if}\  \text{the items are sorted }\ \textbf{then}
 ///     \textbf{return}\ \text{the first of them, removed} \cr
 /// 4 & \quad x \gets A[ 0 ] \cr
 /// 5 & \quad \textbf{if}\ n \le 2\ \textbf{then}\ \text{remove } A[ 0 ]
 ///     \text{, the other item to its place};\ \textbf{return}\ x \cr
 /// 6 & \quad n \gets n - 1;\ y \gets A[ n ];\ j \gets 0 \cr
 /// 7 & \quad \textbf{loop} \cr
 /// 8 & \qquad c \gets \text{the child of } j \text{ of better }
 ///     \mathit{lo} \text{, if any} \cr
 /// 9 & \qquad \textbf{if}\ c = \textbf{nil}\ \textbf{or not}
 ///     \ \mathrm{better}( A[ 2 c ] , y )\ \textbf{then}\ \textbf{break} \cr
 /// 10 & \qquad A[ 2 j ] \gets A[ 2 c ];\ j \gets c \cr
 /// 11 & \qquad \textbf{if}\ 2 j + 1 < n\ \textbf{and}\ \mathrm{better}(
 ///     A[ 2 j + 1 ] , y )\ \textbf{then}\ \text{swap } y \text{ and }
 ///     A[ 2 j + 1 ] \cr
 /// 12 & \quad A[ 2 j ] \gets y;\ \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T popMin( void );

 /// removes the worst item and returns it, in \f$ O( \log n ) \f$
 /// amortized time
 ///
 /// In pseudocode (van Leeuwen and Wood, beyond the book): the walk of
 /// `popMin()` on the \f$ \mathit{hi} \f$ of the nodes with the order
 /// reversed, the \f$ \mathit{hi} \f$ of a node of one item being its
 /// \f$ \mathit{lo} \f$, and the item carried down swapped with the
 /// \f$ \mathit{lo} \f$ of a node when better than it.
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popMax}() \cr
 /// 2 & \quad \textbf{if}\  \text{a run of removals reached}
 ///     \max\{ 64 , n / 8 \} \text{ items }\ \textbf{then}
 ///     \ \text{sort the items} \cr
 /// 3 & \quad \textbf{if}\  \text{the items are sorted }\ \textbf{then}
 ///     \textbf{return}\ \text{the last of them, removed} \cr
 /// 4 & \quad \textbf{if}\ n \le 2\ \textbf{then}\ n \gets n - 1;
 ///     \ \textbf{return}\ A[ n ] \cr
 /// 5 & \quad x \gets A[ 1 ];\ n \gets n - 1;\ y \gets A[ n ];\ h \gets
 ///     1;\ j \gets 0 \cr
 /// 6 & \quad \textbf{loop} \cr
 /// 7 & \qquad c \gets \text{the child of } j \text{ of worse }
 ///     \mathit{hi} \text{, if any};\ h' \gets \text{the place of its }
 ///     \mathit{hi} \cr
 /// 8 & \qquad \textbf{if}\ c = \textbf{nil}\ \textbf{or not}
 ///     \ \mathrm{better}( y , A[ h' ] )\ \textbf{then}\ \textbf{break} \cr
 /// 9 & \qquad A[ h ] \gets A[ h' ];\ h \gets h';\ j \gets c \cr
 /// 10 & \qquad \textbf{if}\ h = 2 j + 1\ \textbf{and}\ \mathrm{better}(
 ///     y , A[ 2 j ] )\ \textbf{then}\ \text{swap } y \text{ and } A[ 2 j ]
 ///     \cr
 /// 11 & \quad A[ h ] \gets y;\ \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T popMax( void );

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const {
  return( f_sorted ? f_hi - f_lo : f_A.size() );
  }

 /// room for \f$ n \f$ items, so that the pushes up to \f$ n \f$ do not
 /// reallocate
 void reserve( std::size_t n ) { f_A.reserve( n ); }

 /// removes all the items
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
