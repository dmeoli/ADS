/*--------------------------------------------------------------------------*/
/*--------------------------- File BucketQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A priority queue for small nonnegative integer keys, beyond the book: a
 * two-level bucket queue, whose outer level is a vector indexed by the
 * first key f and whose inner one, for each f, a set of LIFO lists, one
 * per second key g. top() is an item of least f and, among those, of
 * largest g, the last one pushed (the order that A* wants: least f, ties
 * broken towards the deeper nodes). The least f is found by a scan that
 * only moves forward between two pushes of a smaller f, so that the outer
 * level costs \f$ O( 1 ) \f$ amortized per operation when the keys are
 * bounded, against the \f$ O( \log n ) \f$ of a heap: the queue of Dial
 * for the shortest paths with integer weights, nested as in the search
 * code of Burns et al.
 *
 * A layer (the items of one f) keeps the heads of its lists in two ways.
 * The lists of g < D (D = 256) are dense: an array indexed by g, the
 * first S = 4 heads in the layer itself and the others in a vector, and
 * the largest g is found by a scan backward from a bound that only moves
 * down between two pushes of a larger g. The lists of g >= D are sparse
 * while they are at most R = 64: a vector of runs ( g , head ), sorted by
 * increasing g, whose last run is the top of the layer. A push of g >= D
 * appends a run when g is the largest (the common case of a search, whose
 * children are deeper than their parent), prepends to the list of the
 * last run when g is equal to it, and otherwise finds its place by a
 * binary search and moves the larger runs up by one; a pop takes the
 * head of the last run and drops the run when its list empties. A push of
 * g >= D into a layer of R runs first turns it dense on all its g: its
 * vector grows up to the largest g, takes the heads of the runs, whose
 * lists stay as they are, and the bound of the scan goes to the largest
 * g; the layer stays dense until it empties, when it frees its vector and
 * is sparse again. The vectors of runs are not freed but kept, empty, for
 * the next layer that needs one, so that a layer that fills again
 * allocates nothing. In pseudocode, for a layer l:
 *
 *     push( x , g ): if g >= D and l is sparse and | l.runs | = R:
 *                          l.head[ g' ] = h for each run ( g' , h );
 *                          l.runs = {}; l is dense
 *                    if g < D or l is dense: prepend x to l.head[ g ]
 *                    else: find the run of g in l.runs by binary search,
 *                          or insert it there; prepend x to its list
 *     pop():         if l.runs is not empty: take from the last run, and
 *                          drop the run if its list is now empty
 *                    else: scan l.topG down to a list with items, take
 *                          from it
 *                    if l is empty: free l.head; l is sparse
 *
 * The order is the same in the two ways: inside a list it is LIFO, the
 * runs and the dense heads are both by g, and the conversion moves the
 * heads, not the items. What changes is the cost of a layer that empties
 * and fills again, as the least layer of a search whose nodes come in
 * small batches, where a g of a few thousands meets a layer of a few
 * items. A dense array costs its whole length at each filling, that is,
 * \f$ \Theta( G_f ) \f$ for the largest g \f$ G_f \f$ of the layer f,
 * to allocate and set its heads to empty and as much to scan it down, even
 * for one item. The runs cost \f$ O( 1 ) \f$ per push of a g not below
 * the largest, \f$ O( \log R + R ) = O( 1 ) \f$ per other push (R is a
 * constant: a push moves at most R runs) and \f$ O( 1 ) \f$ per pop.
 * A filling of m items with at most R distinct g >= D thus costs
 * \f$ O( m + D ) \f$; one with more costs \f$ O( m + G_f ) \f$, but then
 * m > R and the length of the array is paid by more than R pushes,
 * \f$ O( G_f / R ) \f$ each. A binary heap per layer would cost
 * \f$ O( \log m ) \f$ per operation, more than the dense lists on a
 * layer of many items, and would lose the order of the lists, which the
 * view of the standard interface walks; the dense lists of the g < D keep
 * the cost of the keys of a small range as it was.
 *
 * The items lie in one vector of cells, each with the index of the next
 * cell of its list, and the cells freed by pop() form a list of their own
 * that push() takes from first, so that a push or a pop costs no call to
 * the allocator once the vector has grown to the largest size of the
 * queue. A layer of g < S costs no allocation, and a layer frees its
 * dense heads when it empties, so that the memory follows the band of the
 * keys in the queue: \f$ O( n + F + \sum_f H_f + R L ) \f$ words for n
 * items, F the largest f pushed, \f$ H_f \f$ the size of the layer f
 * while it is in use, at most D if it is sparse and its largest g
 * \f$ G_f \f$ if it is dense, and L the most layers that had runs at
 * the same time, each vector of runs holding at most R.
 *
 * There is no decrease-key: as in the lazy deletion of A*, an item whose
 * key improves is pushed again, and the caller skips the stale copies when
 * they come out.
 *
 * The contracts [see Contract.h]: the keys are nonnegative, and top(),
 * topF(), topG() and pop() need a queue that is not empty; the invariant is
 * that the counts of the layers add up to size(), and that the runs of the
 * layer changed are at most R, of g >= D, increasing and with items, and
 * none in a dense layer. The operations are defined in BucketQueue.tpp.
 *
 * \see R. B. Dial, Algorithm 360: shortest-path forest with topological
 *      ordering, Communications of the ACM 12(11), 632-633, 1969,
 *      doi:10.1145/363269.363610
 * \see E. Burns, M. Hatem, M. J. Leighton, W. Ruml, Implementing fast
 *      heuristic search code, Proceedings of the 5th Symposium on
 *      Combinatorial Search (SoCS), 25-32, 2012
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

#ifndef __ADS_BucketQueue
 #define __ADS_BucketQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BucketQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a two-level bucket queue: least f first, then largest g, then LIFO
template< class T , std::integral K = int >
class BucketQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Key = K;    ///< the type of the keys

 /// pushes x with the keys f and g
 /// \pre f >= 0 and g >= 0
 void push( const T & x , K f , K g );

 /// an item of least f and, among those, of largest g, the last pushed
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void );

 /// the f of top()
 /// \pre the queue is not empty
 [[nodiscard]] K topF( void );

 /// the g of top()
 /// \pre the queue is not empty
 [[nodiscard]] K topG( void );

 /// removes top() and returns it
 /// \pre the queue is not empty
 T pop( void );

 /// an item with its keys
 struct Entry {
  T item;
  K f , g;
  };

 /// removes top() and returns it with its keys f and g, finding the top
 /// once instead of three times through topF(), topG() and pop()
 /// \pre the queue is not empty
 Entry popEntry( void );

 [[nodiscard]] bool empty( void ) const;

 /// the number of items, the stale ones included
 [[nodiscard]] std::size_t size( void ) const;

 /// removes all the items and frees the layers
 void clear( void );

 /// the g below which the lists of a layer are always dense
 static constexpr std::size_t D = 256;

 /// the runs of g >= D that a layer keeps sparse, at most
 static constexpr std::size_t R = 64;

/*--------------------------------------------------------------------------*/
 private:

 using Index = std::uint32_t;
 static constexpr Index NIL = ~Index( 0 );   ///< no cell
 static constexpr std::size_t S = 4;          ///< the heads in a layer

 /// a cell: an item and the next cell of its list
 struct Cell {
  T item;
  Index next;
  };

 /// a sparse list: its g and its head
 struct Run {
  K g;
  Index head;
  };

 using Runs = std::vector< Run >;   ///< the runs of a layer, by g

 /// the items of one f
 struct Layer {
  Index count = 0;                      ///< the items in the layer
  Index topG = 0;                       ///< no dense list above has items
  Index runs = NIL;                     ///< its runs in f_runs, if any
  Index small[ S ] = { NIL , NIL , NIL , NIL };   ///< the heads, g < S
  std::vector< Index > big;             ///< the dense heads, g >= S

  Index & head( std::size_t g ) {
   return( g < S ? small[ g ] : big[ g - S ] );
   }
  Index head( std::size_t g ) const {
   return( g < S ? small[ g ] : g - S < big.size() ? big[ g - S ] : NIL );
   }
  /// the dense lists, S at least
  std::size_t lists( void ) const { return( S + big.size() ); }
  /// whether the lists of g >= D are dense too
  bool dense( void ) const { return( big.size() > D - S ); }
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of the extractions (by f
 // increasing, then by g decreasing, each list from the last item pushed),
 // forward: in a layer the runs from the last, then the dense lists from
 // the last; it skips the empty lists without touching the lazy bounds
 // that top() moves

 class const_iterator {
  public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = const T &;
  using pointer = const T *;

  const_iterator( void ) = default;
  const_iterator( const BucketQueue * q , std::size_t f )
   : f_q( q ) , f_f( f ) { settle(); }

  reference operator*( void ) const { return( f_q->f_cells[ f_c ].item ); }
  pointer operator->( void ) const { return( &**this ); }

  const_iterator & operator++( void ) {
   f_c = f_q->f_cells[ f_c ].next;
   if( f_c == NIL ) {
    ++f_g;
    settle();
    }
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( ( f_f == o.f_f ) && ( f_g == o.f_g ) && ( f_c == o.f_c ) );
   }

  private:
  /// on the head of the list f_g from the top of the layer f_f (the runs
  /// first, then the dense lists), or at the end ( size of the layers , 0
  /// , NIL )
  void settle( void ) {
   const auto & L = f_q->f_layers;
   for( ; f_f < L.size() ; ++f_f , f_g = 0 ) {
    const Layer & l = L[ f_f ];
    const std::size_t r = l.runs == NIL ? 0 : f_q->f_runs[ l.runs ].size();
    if( l.count )
     for( ; f_g < r + l.lists() ; ++f_g )
      if( ( f_c = f_g < r ? f_q->f_runs[ l.runs ][ r - 1 - f_g ].head :
                  l.head( r + l.lists() - 1 - f_g ) ) != NIL ) return;
    }
   f_g = 0;
   f_c = NIL;
   }

  const BucketQueue * f_q = nullptr;
  std::size_t f_f = 0 , f_g = 0;
  Index f_c = NIL;
  };

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_layers.size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the layer of the least f, its topG moved to its largest dense g if
 /// it has no run
 Layer & front( void );

 /// the head of the list of top() in l, after front()
 Index & top( Layer & l );

 /// the g of top() in l, after front()
 [[nodiscard]] K topKey( const Layer & l ) const;

 /// a cell with the item x, its next to be set
 Index cell( const T & x );

 /// x pushed with g >= D to the runs of the sparse layer l and true, or
 /// l turned dense and false if it has R runs
 bool pushRun( Layer & l , const T & x , K g );

 /// the head of the list of g >= D in the runs v, a new run if there is
 /// none
 static Index & run( Runs & v , K g );

 /// pop() from the last run of l, the layer of the least f
 T popRun( Layer & l );

 /// the item of the head h, unlinked and its cell freed
 T take( Index & h );

 /// one item fewer in l, which frees its dense heads if it empties
 void taken( Layer & l );

 /// l dense on all its g: the heads of the runs moved to the vector
 void densify( Layer & l );

 /// the runs of l, empty, back to the spare ones
 void release( Layer & l );

 /// the counts of the layers add up to size(), \f$ O( F ) \f$
 [[nodiscard]] bool counted( void ) const;

 /// the runs of l are at most R, of g >= D, increasing and with items,
 /// and none if l is dense, \f$ O( R ) \f$
 [[nodiscard]] bool ordered( const Layer & l ) const;

 std::vector< Layer > f_layers;   ///< a layer per f
 std::vector< Cell > f_cells;     ///< the cells of the items
 Index f_free = NIL;              ///< the cells freed, the last first
 std::size_t f_minF = 0;          ///< no layer below it has items
 std::size_t f_size = 0;          ///< the number of items
 std::vector< Runs > f_runs;      ///< the runs of the layers that have
 std::vector< Index > f_spare;    ///< the runs of no layer, empty

 };  // end( class BucketQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/BucketQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BucketQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BucketQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
