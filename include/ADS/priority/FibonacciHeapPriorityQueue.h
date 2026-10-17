/*--------------------------------------------------------------------------*/
/*------------------- File FibonacciHeapPriorityQueue.h --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a Fibonacci heap, the
 * structure that section 11.5 names and does not describe: insert() and
 * decrease() in O( 1 ) amortized time, deleteMin() in O( log n ), so that
 * the algorithm of Fredman and Tarjan computes the shortest paths in
 * O( m + n log n ).
 *
 * The heap is a collection of trees, each node not smaller than its
 * parent, whose roots are in a circular list; the children of a node are in
 * a circular list as well. insert() adds a tree of one node to the roots.
 * deleteMin() moves the children of the minimum among the roots and then
 * consolidates them, linking two roots of equal degree, the larger under
 * the smaller, until all the degrees differ. decrease() cuts the node from
 * its parent, if it became smaller than it, and makes it a root; a node
 * that loses a second child is cut in turn (the cascading cut), so that a
 * node of degree k has at least \f$ F_{k+2} \geq \phi^k \f$ descendants,
 * \f$ \phi = ( 1 + \sqrt{5} ) / 2 \f$, and the degrees are
 * \f$ O( \log_\phi n ) \f$.
 *
 * The contracts [see Contract.h]: min(), minPriority() and deleteMin()
 * need a heap that is not empty, decrease() a handle of a node of this
 * heap (checked in O( n ) with the invariants) and a priority not larger
 * than its own; the invariant is the order of the heap, the circular lists
 * linked both ways, the parents, the degrees equal to the number of
 * children, the roots not marked, the minimum among the roots and size()
 * counting the nodes. On a heap of more than 1024 nodes it is checked on
 * the roots only, so that a long sequence of operations at the level 3
 * stays linear. The operations are defined in
 * FibonacciHeapPriorityQueue.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 11.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 * \see M. L. Fredman, R. E. Tarjan, Fibonacci heaps and their uses in
 *      improved network optimization algorithms, Journal of the ACM 34(3),
 *      596-615, 1987, doi:10.1145/28869.28874
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

#ifndef __ADS_FibonacciHeapPriorityQueue
 #define __ADS_FibonacciHeapPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------- CLASS FibonacciHeapPriorityQueue ---------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a Fibonacci heap
template< class T , std::totally_ordered P = int >
class FibonacciHeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 /// a node of the heap
 struct Node {
  P priority;
  T value;
  Node * parent = nullptr;
  Node * child = nullptr;
  Node * left = this;
  Node * right = this;
  int degree = 0;
  bool mark = false;
  };

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities
 using Handle = Node *;     ///< what insert() gives

 FibonacciHeapPriorityQueue( void ) = default;
 FibonacciHeapPriorityQueue( const FibonacciHeapPriorityQueue & ) = delete;
 FibonacciHeapPriorityQueue & operator=(
                              const FibonacciHeapPriorityQueue & ) = delete;

 ~FibonacciHeapPriorityQueue();

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] int size( void ) const;

 /// \pre the heap is not empty
 [[nodiscard]] const T & min( void ) const;

 /// \pre the heap is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// O( 1 ) time: a tree of one node among the roots
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// O( log n ) amortized time; the handle of the item is no longer valid
 /// \pre the heap is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

 /// O( 1 ) amortized time
 /// \pre x is a node of this heap and p is not larger than its priority
 /// \post x has priority p
 void decrease( Handle x , const P & p );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( priority , item ), in preorder over the
 // circular lists of the roots and of the children (no useful order),
 // forward; a list is done when the walk is back at its first node, the
 // child of the parent (or the minimum, for the roots)

 class const_iterator {
  public:
  using reference = std::pair< const P & , const T & >;
  using value_type = reference;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;
  explicit const_iterator( const Node * min ) : f_x( min ) , f_min( min ) {}

  reference operator*( void ) const {
   return( reference( f_x->priority , f_x->value ) );
   }

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

  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( f_x == o.f_x );
   }

  private:
  const Node * f_x = nullptr;
  const Node * f_min = nullptr;
  };

 using value_type = std::pair< const P & , const T & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_min ) );
  }
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
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

 /// links the roots of equal degree until all the degrees differ
 void consolidate( void );

 /// whether x is a node of the heap, O( n )
 [[nodiscard]] bool owns( const Node * x ) const;

 /// the list of x linked both ways, its nodes children of parent, each
 /// not smaller than parent, and the trees below them as well; their
 /// nodes counted in n
 [[nodiscard]] bool tree( const Node * x , const Node * parent ,
                          int & n ) const;

 /// the invariant of the heap, O( n ); on more than 1024 nodes, the list
 /// of the roots and the minimum among them only
 [[nodiscard]] bool isHeap( void ) const;

 Node * f_min = nullptr;   ///< the root of least priority
 int f_n = 0;              ///< the number of items

 };  // end( class FibonacciHeapPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/FibonacciHeapPriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FibonacciHeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*----------------- End File FibonacciHeapPriorityQueue.h ------------------*/
/*--------------------------------------------------------------------------*/
