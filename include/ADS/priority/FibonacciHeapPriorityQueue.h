/*--------------------------------------------------------------------------*/
/*------------------- File FibonacciHeapPriorityQueue.h --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see `PriorityQueue.h`] on a Fibonacci heap, the
 * structure that section 11.5 names and does not describe: `insert()` and
 * `decrease()` in \f$ O( 1 ) \f$ amortized time, `deleteMin()` in
 * \f$ O( \log n ) \f$, so that the algorithm of Fredman and Tarjan computes
 * the shortest paths in \f$ O( m + n \log n ) \f$.
 *
 * The heap is a collection of trees, each node not smaller than its
 * parent, whose roots are in a circular list; the children of a node are in
 * a circular list as well. `insert()` adds a tree of one node to the roots.
 * `deleteMin()` moves the children of the minimum among the roots and then
 * consolidates them, linking two roots of equal degree, the larger under
 * the smaller, until all the degrees differ. `decrease()` cuts the node
 * from its parent, if it became smaller than it, and makes it a root; a
 * node that loses a second child is cut in turn (the cascading cut), so
 * that a node of degree \f$ k \f$ has at least
 * \f$ F_{k+2} \geq \phi^k \f$ descendants,
 * \f$ \phi = ( 1 + \sqrt{5} ) / 2 \f$, and the degrees are
 * \f$ O( \log_\phi n ) \f$.
 *
 * Beyond the book, the nodes come from an `ObjectPool` [see `ObjectPool.h`]
 * of the heap, which takes back the node of `deleteMin()` for the next
 * `insert()`, so that a node costs no call to the allocator and the nodes in
 * use stay close in memory; the children of the minimum join the roots in
 * \f$ O( 1 ) \f$, and the consolidation walks the roots where they are, with
 * the trees by degree in an array on the stack (a degree is at most
 * \f$ \log_\phi n < 64 \f$).
 *
 * The contracts [see `Contract.h`]: `min()`, `minPriority()` and
 * `deleteMin()` need a heap that is not empty, `decrease()` a handle of a
 * node of this heap (checked in \f$ O( n ) \f$ with the invariants) and a
 * priority not larger than its own; the invariant is the order of the heap,
 * the circular lists linked both ways, the parents, the degrees equal to the
 * number of children, the roots not marked, the minimum among the roots and
 * `size()` counting the nodes. On a heap of more than 1024 nodes it is
 * checked on the roots only, so that a long sequence of operations at the
 * level 3 stays linear. The operations are defined in
 * `FibonacciHeapPriorityQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 11.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 * \see M. L. Fredman, R. E. Tarjan, Fibonacci heaps and their uses in
 *      improved network optimization algorithms, Journal of the ACM 34(3),
 *      596-615, 1987, doi:10.1145/28869.28874
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

#ifndef __ADS_FibonacciHeapPriorityQueue
 #define __ADS_FibonacciHeapPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------- CLASS FibonacciHeapPriorityQueue ---------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a Fibonacci heap, a collection of trees heap ordered
/// whose roots are in a circular list
template< class T , std::totally_ordered P = int >
class FibonacciHeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 /// a node of the heap
 struct Node {
  P priority;                ///< the priority
  T value;                   ///< the item
  Node * parent = nullptr;   ///< the parent, `nullptr` for a root
  Node * child = nullptr;    ///< a child, the entry of their circular list
  Node * left = this;        ///< the previous node of its circular list
  Node * right = this;       ///< the next node of its circular list
  int degree = 0;            ///< the number of children
  bool mark = false;         ///< whether it lost a child since it became one
  };

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities
 using Handle = Node *;     ///< what `insert()` gives

 /// the empty heap
 FibonacciHeapPriorityQueue( void ) = default;

 /// no copy: the handles point into the heap
 FibonacciHeapPriorityQueue( const FibonacciHeapPriorityQueue & ) = delete;

 /// no copy: the handles point into the heap
 FibonacciHeapPriorityQueue & operator=(
                              const FibonacciHeapPriorityQueue & ) = delete;

 /// the items of `other`, which is left empty; its handles stay valid here
 FibonacciHeapPriorityQueue( FibonacciHeapPriorityQueue && other ) noexcept;

 /// the heap and its nodes destroyed
 ~FibonacciHeapPriorityQueue();

 /// whether the heap has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] int size( void ) const;

 /// the item of smallest priority, the least of the roots
 /// \pre the heap is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the smallest priority
 /// \pre the heap is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// inserts \f$ x \f$ with the priority \f$ p \f$ and gives its handle, a
 /// tree of one node among the roots, in \f$ O( 1 ) \f$ time
 ///
 /// In pseudocode (Fredman and Tarjan, beyond the book), \f$ \mathit{min}
 /// \f$ the root of least priority:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , p ) \cr
 /// 2 & \quad z \gets \text{a node } ( p , x ) \text{ of degree 0, not}
 ///     \text{ marked};\ n \gets n + 1 \cr
 /// 3 & \quad \textbf{if}\ \mathit{min} = \textbf{nil}\ \textbf{then}
 ///     \ \mathit{min} \gets z \cr
 /// 4 & \quad \textbf{else} \cr
 /// 5 & \qquad \text{add } z \text{ to the list of the roots} \cr
 /// 6 & \qquad \textbf{if}\ z.\mathit{priority} <
 ///     \mathit{min}.\mathit{priority}\ \textbf{then}\ \mathit{min} \gets
 ///     z \cr
 /// 7 & \quad \textbf{return}\ z
 /// \end{array}}
 /// \f]
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// removes the item of smallest priority and returns it, in
 /// \f$ O( \log n ) \f$ amortized time; the handle of the item is no
 /// longer valid
 ///
 /// In pseudocode (Fredman and Tarjan, beyond the book), \f$ A \f$ the
 /// roots by degree, the degrees below 64:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deleteMin}() \cr
 /// 2 & \quad z \gets \mathit{min};\ n \gets n - 1 \cr
 /// 3 & \quad \text{splice the list of the children of } z \text{ into}
 ///     \text{ the list of the roots} \cr
 /// 4 & \quad \text{remove } z \text{ from the list of the roots} \cr
 /// 5 & \quad \textbf{if}\  \text{the list is empty }\ \textbf{then}
 ///     \ \mathit{min} \gets \textbf{nil}\ \textbf{else}
 ///     \ \mathrm{consolidate}() \cr
 /// 6 & \quad \textbf{return}\ z.\mathit{value} \cr
 /// 7 & \textbf{procedure}\ \mathrm{consolidate}() \cr
 /// 8 & \quad A[ 0 .. 63 ] \gets \textbf{nil} \cr
 /// 9 & \quad \textbf{foreach}\ \text{root } y\ \textbf{do} \cr
 /// 10 & \qquad y.\mathit{parent} \gets \textbf{nil};\ y.\mathit{mark}
 ///     \gets \textbf{false};\ d \gets y.\mathit{degree} \cr
 /// 11 & \qquad \textbf{while}\ A[ d ] \ne \textbf{nil}\ \textbf{do} \cr
 /// 12 & \qquad\quad z \gets A[ d ];\ A[ d ] \gets \textbf{nil} \cr
 /// 13 & \qquad\quad \textbf{if}\ z.\mathit{priority} <
 ///     y.\mathit{priority}\ \textbf{then}\ \text{swap } y \text{ and } z
 ///     \cr
 /// 14 & \qquad\quad \text{make } z \text{ a child of } y \text{, not}
 ///     \text{ marked};\ d \gets d + 1;\ y.\mathit{degree} \gets d \cr
 /// 15 & \qquad A[ d ] \gets y \cr
 /// 16 & \quad \text{the roots} \gets \text{the trees of } A
 ///     \text{ in a circular list};\ \mathit{min} \gets \text{the least}
 ///     \text{ of them}
 /// \end{array}}
 /// \f]
 /// \pre the heap is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

 /// lowers to \f$ p \f$ the priority of the node \f$ x \f$, in
 /// \f$ O( 1 ) \f$ amortized time
 ///
 /// In pseudocode (Fredman and Tarjan, beyond the book), the cascading cut
 /// keeping a node of degree \f$ k \f$ with at least \f$ F_{k+2} \f$
 /// descendants:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( x , p ) \cr
 /// 2 & \quad x.\mathit{priority} \gets p;\ y \gets x.\mathit{parent} \cr
 /// 3 & \quad \textbf{if}\ y \ne \textbf{nil}\ \textbf{and}
 ///     \ x.\mathit{priority} < y.\mathit{priority}\ \textbf{then} \cr
 /// 4 & \qquad \mathrm{cut}( x , y );\ z \gets y \cr
 /// 5 & \qquad \textbf{while}\ z.\mathit{parent} \ne \textbf{nil}
 ///     \ \textbf{do}\quad \triangleright\ \text{the cascading cut} \cr
 /// 6 & \qquad\quad \textbf{if not}\ z.\mathit{mark}\ \textbf{then}
 ///     \ z.\mathit{mark} \gets \textbf{true};\ \textbf{break} \cr
 /// 7 & \qquad\quad w \gets z.\mathit{parent};\ \mathrm{cut}( z , w
 ///     );\ z \gets w \cr
 /// 8 & \quad \textbf{if}\ x.\mathit{priority} <
 ///     \mathit{min}.\mathit{priority}\ \textbf{then}\ \mathit{min} \gets
 ///     x \cr
 /// 9 & \textbf{procedure}\ \mathrm{cut}( x , y ) \cr
 /// 10 & \quad \text{remove } x \text{ from the children of } y;
 ///     \ y.\mathit{degree} \gets y.\mathit{degree} - 1 \cr
 /// 11 & \quad x.\mathit{parent} \gets \textbf{nil};\ x.\mathit{mark}
 ///     \gets \textbf{false};\ \text{add } x \text{ to the list of the}
 ///     \text{ roots}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is a node of this heap and \f$ p \f$ is not larger than
 ///      its priority
 /// \post \f$ x \f$ has the priority \f$ p \f$
 void decrease( Handle x , const P & p );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( priority , item ), in preorder over the
 // circular lists of the roots and of the children (no useful order),
 // forward; a list is done when the walk is back at its first node, the
 // child of the parent (or the minimum, for the roots)

 /// the iterator of the view, constant, forward
 class const_iterator {
  public:
  /// the pair of the priority and of the item of a node
  /// the type of a reference to an item of the view
  using reference = std::pair< const P & , const T & >;
  /// the type of the items of the view
  using value_type = reference;
  /// the type of the distances between iterators
  using difference_type = std::ptrdiff_t;
  /// a forward iterator for the concepts of C++20
  using iterator_concept = std::forward_iterator_tag;
  /// an input iterator for the older algorithms, since
  /// `reference` is not a reference
  using iterator_category = std::input_iterator_tag;

  /// the end of the view
  const_iterator( void ) = default;

  /// the first node of the view, the root \f$ \mathit{min} \f$
  explicit const_iterator( const Node * min ) : f_x( min ) , f_min( min ) {}

  /// the priority and the item of the node
  reference operator*( void ) const {
   return( reference( f_x->priority , f_x->value ) );
   }

  /// the next node in preorder
  const_iterator & operator++( void ) {
   if( f_x->child ) {
    f_x = f_x->child;
    return( *this );
    }
   while( f_x ) {
    const Node * first = f_x->parent ? f_x->parent->child : f_min;
    if( f_x->right != first ) {
     f_x = f_x->right;
     return( *this );
     }
    f_x = f_x->parent;                      // the list is done
    }
   return( *this );
   }

  /// the next node in preorder, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// whether the two iterators are on the same node
  bool operator==( const const_iterator & o ) const {
   return( f_x == o.f_x );
   }

  private:
  const Node * f_x = nullptr;
  const Node * f_min = nullptr;
  };

 /// the type of the items of the view
 using value_type = std::pair< const P & , const T & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between iterators
 using difference_type = std::ptrdiff_t;
 /// the type of a reference to an item of the view
 using reference = value_type;
 /// the type of a constant reference to an item
 using const_reference = value_type;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the first item of the view
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_min ) );
  }
 /// the end of the view
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// x leaves its circular list
 static void unlink( Node * x );

 /// x, alone, goes into the circular list of y
 static void splice( Node * x , Node * y );

 /// x leaves the list whose entry is head, which follows it if needed
 static void remove( Node * x , Node * & head );

 void add_root( Node * z );

 /// x, a child of y, becomes a root
 void cut( Node * x , Node * y );

 /// links the roots of equal degree, from w around its circular list,
 /// until all the degrees differ, and puts the minimum in f_min
 void consolidate( Node * w );

 /// whether x is a node of the heap, \f$ O( n ) \f$
 [[nodiscard]] bool owns( const Node * x ) const;

 /// the list of x linked both ways, its nodes children of parent, each
 /// not smaller than parent, and the trees below them as well; their
 /// nodes counted in n
 [[nodiscard]] bool tree( const Node * x , const Node * parent ,
                          int & n ) const;

 /// the invariant of the heap, \f$ O( n ) \f$; on more than 1024 nodes, the
 /// list of the roots and the minimum among them only
 [[nodiscard]] bool isHeap( void ) const;

 Node * f_min = nullptr;      ///< the root of least priority
 int f_n = 0;                 ///< the number of items
 ObjectPool< Node > f_pool;   ///< the nodes

 };  // end( class FibonacciHeapPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/FibonacciHeapPriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FibonacciHeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*----------------- End File FibonacciHeapPriorityQueue.h ------------------*/
/*--------------------------------------------------------------------------*/
