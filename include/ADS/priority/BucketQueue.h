/*--------------------------------------------------------------------------*/
/*--------------------------- File BucketQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A priority queue for small nonnegative integer keys, beyond the book: a
 * two-level bucket queue, whose outer level is a vector indexed by the
 * first key f and whose inner one, for each f, a vector of LIFO lists
 * indexed by the second key g. top() is an item of least f and, among
 * those, of largest g, the last one pushed (the order that A* wants: least
 * f, ties broken towards the deeper nodes). The least f is found by a scan
 * that only moves forward between two pushes of a smaller f, and the
 * largest g of a layer likewise backward, so that push() takes O( 1 ) time
 * and pop() O( 1 ) amortized when the keys are bounded by C, against the
 * \f$ O( \log n ) \f$ of a heap: the queue of Dial for the shortest
 * paths with integer weights, nested as in the search code of Burns et al.
 *
 * The items lie in one vector of cells, each with the index of the next
 * cell of its list, and the cells freed by pop() form a list of their own
 * that push() takes from first, so that a push or a pop costs no call to
 * the allocator once the vector has grown to the largest size of the
 * queue. A layer keeps the heads of its first four lists in itself and
 * those of larger g in a vector, which it frees when it empties, so that
 * a layer of small g costs no allocation and the memory follows the band
 * of the keys in the queue: \f$ O( n + F + \sum_f G_f ) \f$ words for n
 * items, F the largest f pushed and \f$ G_f \f$ the largest g in the
 * layer f while it is in use.
 *
 * There is no decrease-key: as in the lazy deletion of A*, an item whose
 * key improves is pushed again, and the caller skips the stale copies when
 * they come out.
 *
 * The contracts [see Contract.h]: the keys are nonnegative, and top(),
 * topF(), topG() and pop() need a queue that is not empty; the invariant is
 * that the counts of the layers add up to size(). The operations are
 * defined in BucketQueue.tpp.
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

 /// the items of one f
 struct Layer {
  Index count = 0;                      ///< the items in the layer
  Index topG = 0;                       ///< no list above it has items
  Index small[ S ] = { NIL , NIL , NIL , NIL };   ///< the heads, g < S
  std::vector< Index > big;             ///< the heads, g >= S

  Index & head( std::size_t g ) {
   return( g < S ? small[ g ] : big[ g - S ] );
   }
  Index head( std::size_t g ) const {
   return( g < S ? small[ g ] : g - S < big.size() ? big[ g - S ] : NIL );
   }
  /// the lists, S at least
  std::size_t lists( void ) const { return( S + big.size() ); }
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of the extractions (by f
 // increasing, then by g decreasing, each list from the last item pushed),
 // forward; it skips the empty lists without touching the lazy bounds that
 // top() moves

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
  /// on the head of the list f_g from the top of the layer f_f, or at the
  /// end ( size of the layers , 0 , NIL )
  void settle( void ) {
   const auto & L = f_q->f_layers;
   for( ; f_f < L.size() ; ++f_f , f_g = 0 ) {
    const Layer & l = L[ f_f ];
    if( l.count )
     for( ; f_g < l.lists() ; ++f_g )
      if( ( f_c = l.head( l.lists() - 1 - f_g ) ) != NIL ) return;
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

 /// the layer of the least f, its topG moved to its largest g
 Layer & front( void );

 /// the counts of the layers add up to size(), O( layers )
 [[nodiscard]] bool counted( void ) const;

 std::vector< Layer > f_layers;   ///< a layer per f
 std::vector< Cell > f_cells;     ///< the cells of the items
 Index f_free = NIL;              ///< the cells freed, the last first
 std::size_t f_minF = 0;          ///< no layer below it has items
 std::size_t f_size = 0;          ///< the number of items

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
