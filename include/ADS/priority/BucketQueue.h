/*--------------------------------------------------------------------------*/
/*--------------------------- File BucketQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A priority queue for small nonnegative integer keys, beyond the book: a
 * two-level bucket queue, whose outer level is a vector indexed by the
 * first key \f$ f \f$ and whose inner one, for each \f$ f \f$, a set of LIFO
 * lists, one per second key \f$ g \f$. `top()` is an item of least \f$ f \f$
 * and, among those, of largest \f$ g \f$, the last one pushed (the order that
 * A* wants: least \f$ f \f$, ties broken towards the deeper nodes). The least
 * \f$ f \f$ is found by a scan that only moves forward between two pushes of
 * a smaller \f$ f \f$, so that the outer level costs \f$ O( 1 ) \f$ amortized
 * per operation when the keys are bounded, against the \f$ O( \log n ) \f$ of
 * a heap: the queue of Dial for the shortest paths with integer weights,
 * nested as in the search code of Burns et al.
 *
 * A layer (the items of one \f$ f \f$) keeps the heads of its lists in two
 * ways. The lists of \f$ g < D \f$ (\f$ D = 256 \f$) are dense: an array
 * indexed by \f$ g \f$, the first \f$ S = 4 \f$ heads in the layer itself and
 * the others in a vector, whose largest \f$ g \f$ with items is found by a
 * scan backward from a bound that only moves down between two pushes of a
 * larger \f$ g \f$. The lists of \f$ g \ge D \f$ are sparse while they are at
 * most \f$ R = 64 \f$: a vector of runs \f$ ( g , h ) \f$, sorted by
 * increasing \f$ g \f$, whose last run is the top of the layer. A push of a
 * new \f$ g \ge D \f$ into a layer of \f$ R \f$ runs turns it dense on all its
 * \f$ g \f$, moving the heads of the runs, not the items, into the vector,
 * and the layer stays dense until it empties, when it frees its vector and is
 * sparse again; the vectors of runs are kept, empty, for the next layer that
 * needs one. A filling of a layer with \f$ m \f$ items costs
 * \f$ O( m + D ) \f$ if they have at most \f$ R \f$ distinct
 * \f$ g \ge D \f$, and \f$ O( m + G_f ) \f$ otherwise, \f$ G_f \f$ the
 * largest \f$ g \f$ of the layer \f$ f \f$, paid by more than \f$ R \f$
 * pushes, where a dense array alone would cost \f$ \Theta( G_f ) \f$ at each
 * filling, even of one item; the argument and the memory,
 * \f$ O( n + F + \sum_f H_f + R L ) \f$ words, are in the theory of the area
 * (see \ref theory_priority_monotone).
 *
 * The items lie in one vector of cells, each with the index of the next cell
 * of its list, and the cells freed by `pop()` form a list of their own that
 * `push()` takes from first, so that a push or a pop costs no call to the
 * allocator once the vector has grown to the largest size of the queue.
 * There is no decrease-key: as in the lazy deletion of A*, an item whose key
 * improves is pushed again, and the caller skips the stale copies when they
 * come out.
 *
 * The contracts [see `Contract.h`]: the keys are nonnegative, and `top()`,
 * `topF()`, `topG()` and `pop()` need a queue that is not empty; the
 * invariant is that the counts of the layers add up to `size()`, and that the
 * runs of the layer changed are at most \f$ R \f$, of \f$ g \ge D \f$,
 * increasing and with items, and none in a dense layer. The operations are
 * defined in `BucketQueue.tpp`.
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
 *         Università di Pisa \n
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

/// a two-level bucket queue: least \f$ f \f$ first, then largest \f$ g \f$,
/// then LIFO
template< class T , std::integral K = int >
class BucketQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Key = K;    ///< the type of the keys

 /// pushes \f$ x \f$ with the keys \f$ f \f$ and \f$ g \f$
 ///
 /// In pseudocode (the bucket queue of Dial nested as in Burns et al.,
 /// beyond the book), \f$ \ell \f$ the layer of \f$ f \f$ and \f$ v \f$ its
 /// runs:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x , f , g ) \cr
 /// 2 & \quad \ell \gets \text{the layer of } f \cr
 /// 3 & \quad \textbf{if}\ g \ge D\ \textbf{and}\ \ell \text{ is sparse}
 ///     \ \textbf{and}\ |v| = R\ \textbf{then}\quad \triangleright\ \text{to}
 ///     \text{ dense} \cr
 /// 4 & \qquad \textbf{foreach}\ ( g' , h ) \in v\ \textbf{do}
 ///     \ \ell.\mathit{head}[ g' ] \gets h \cr
 /// 5 & \qquad v \gets \emptyset;\ \ell.\mathit{topG} \gets \text{the }
 ///     \text{largest } g' \cr
 /// 6 & \quad \textbf{if}\ g < D\ \textbf{or}\ \ell \text{ is dense}
 ///     \ \textbf{then} \cr
 /// 7 & \qquad \text{prepend } x \text{ to the list of }
 ///     \ell.\mathit{head}[ g ];\ \ell.\mathit{topG} \gets \max\{
 ///     \ell.\mathit{topG} , g \} \cr
 /// 8 & \quad \textbf{else if}\ v = \emptyset\ \textbf{or}\ g > \text{the }
 ///     \text{last } g' \text{ of } v\ \textbf{then}\ \text{append the run }
 ///     ( g , ( x ) ) \text{ to } v \cr
 /// 9 & \quad \textbf{else}\ \text{find the run of } g \text{ in } v
 ///     \text{ by binary search, or insert it;}\ \text{prepend } x
 ///     \text{ to it} \cr
 /// 10 & \quad \ell.\mathit{count} \gets \ell.\mathit{count} + 1;
 ///     \ \mathit{minF} \gets \min\{ \mathit{minF} , f \}
 /// \end{array}}
 /// \f]
 /// \pre \f$ f \ge 0 \f$ and \f$ g \ge 0 \f$
 void push( const T & x , K f , K g );

 /// an item of least \f$ f \f$ and, among those, of largest \f$ g \f$, the
 /// last pushed
 ///
 /// In pseudocode, the two scans moving only forward in \f$ f \f$ and
 /// backward in \f$ g \f$ between two pushes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{top}() \cr
 /// 2 & \quad \textbf{while}\ \text{the layer of } \mathit{minF} \text{ is }
 ///     \text{empty}\ \textbf{do}\ \mathit{minF} \gets \mathit{minF} + 1 \cr
 /// 3 & \quad \ell \gets \text{the layer of } \mathit{minF} \cr
 /// 4 & \quad \textbf{if}\ \ell \text{ has runs}\ \textbf{then return}
 ///     \ \text{the head of its last run} \cr
 /// 5 & \quad \textbf{while}\ \ell.\mathit{head}[ \ell.\mathit{topG} ] =
 ///     \textbf{nil}\ \textbf{do}\ \ell.\mathit{topG} \gets
 ///     \ell.\mathit{topG} - 1 \cr
 /// 6 & \quad \textbf{return}\ \text{the item of } \ell.\mathit{head}[
 ///     \ell.\mathit{topG} ]
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void );

 /// the \f$ f \f$ of `top()`, i.e., the least \f$ f \f$ in the queue
 /// \pre the queue is not empty
 [[nodiscard]] K topF( void );

 /// the \f$ g \f$ of `top()`, i.e., the largest \f$ g \f$ of the least
 /// \f$ f \f$
 /// \pre the queue is not empty
 [[nodiscard]] K topG( void );

 /// removes `top()` and returns it
 ///
 /// In pseudocode, \f$ \ell \f$ the layer of `top()` found as there:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad \ell \gets \text{the layer of } \mathrm{top}() \cr
 /// 3 & \quad \textbf{if}\ \ell \text{ has runs}\ \textbf{then} \cr
 /// 4 & \qquad x \gets \text{the item unlinked from the head of the last run}
 ///     \cr
 /// 5 & \qquad \textbf{if}\ \text{its list is empty}\ \textbf{then}
 ///     \ \text{drop the run, and spare the vector if no run is left} \cr
 /// 6 & \quad \textbf{else}\ x \gets \text{the item unlinked from }
 ///     \ell.\mathit{head}[ \ell.\mathit{topG} ] \cr
 /// 7 & \quad \text{its cell to the list of the free cells};
 ///     \ \ell.\mathit{count} \gets \ell.\mathit{count} - 1 \cr
 /// 8 & \quad \textbf{if}\ \ell.\mathit{count} = 0\ \textbf{then}
 ///     \ \text{free the dense heads of } g \ge S;\ \ell.\mathit{topG} \gets 0
 ///     \cr
 /// 9 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T pop( void );

 /// an item with its keys
 struct Entry {
  T item;   ///< the item
  K f ,     ///< its first key
    g;      ///< its second key
  };

 /// removes `top()` and returns it with its keys \f$ f \f$ and \f$ g \f$,
 /// finding the top once instead of three times through `topF()`, `topG()`
 /// and `pop()`
 /// \pre the queue is not empty
 Entry popEntry( void );

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, the stale ones included
 [[nodiscard]] std::size_t size( void ) const;

 /// removes all the items and frees the layers
 void clear( void );

 /// the \f$ g \f$ below which the lists of a layer are always dense
 static constexpr std::size_t D = 256;

 /// the runs of \f$ g \ge D \f$ that a layer keeps sparse, at most
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
  using iterator_category = std::forward_iterator_tag;   ///< forward
  using value_type = T;                ///< the type of the items
  using difference_type = std::ptrdiff_t;   ///< the distances
  using reference = const T &;         ///< a constant reference to an item
  using pointer = const T *;           ///< a constant pointer to an item

  /// the singular iterator
  const_iterator( void ) = default;
  /// on the first item of the layers from \f$ f \f$ of the queue `q`
  const_iterator( const BucketQueue * q , std::size_t f )
   : f_q( q ) , f_f( f ) { settle(); }

  /// the item
  reference operator*( void ) const { return( f_q->f_cells[ f_c ].item ); }
  /// the address of the item
  pointer operator->( void ) const { return( &**this ); }

  /// on the next item in the order of the extractions
  const_iterator & operator++( void ) {
   f_c = f_q->f_cells[ f_c ].next;
   if( f_c == NIL ) {
    ++f_g;
    settle();
    }
   return( *this );
   }
  /// on the next item, returning the iterator before
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// whether on the same item, or both at the end
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

 using value_type = T;                    ///< the type of the items
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the distances of iterators
 using reference = const T &;             ///< a constant reference
 using const_reference = const T &;       ///< a constant reference
 using iterator = const_iterator;         ///< the same as `const_iterator`

 /// on the item of `top()`
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_layers.size() ) );
  }
 /// `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// `end()`
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
