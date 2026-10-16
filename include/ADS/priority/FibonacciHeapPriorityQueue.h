/*--------------------------------------------------------------------------*/
/*-------------------- File FibonacciHeapPriorityQueue.h -------------------*/
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

#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <stdexcept>
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

 ~FibonacciHeapPriorityQueue() {
  std::vector< Node * > S;
  if( f_min )
   S.push_back( f_min );
  while( ! S.empty() ) {                     // each list, then its children
   Node * x = S.back();
   S.pop_back();
   Node * y = x;
   do {
    Node * next = y->right;
    if( y->child )
     S.push_back( y->child );
    delete y;
    y = next;
    } while( y != x );
   }
  }

 [[nodiscard]] bool isEmpty( void ) const { return( f_min == nullptr ); }

 [[nodiscard]] int size( void ) const { return( f_n ); }

 [[nodiscard]] const T & min( void ) const { return( top()->value ); }

 [[nodiscard]] const P & minPriority( void ) const {
  return( top()->priority );
  }

 /// O( 1 ) time: a tree of one node among the roots
 Handle insert( const T & x , const P & p ) {
  Node * z = new Node{ p , x };
  z->left = z->right = z;
  add_root( z );
  ++f_n;
  return( z );
  }

 /// O( log n ) amortized time; the handle of the item is no longer valid
 T deleteMin( void ) {
  Node * z = top();
  while( Node * c = z->child ) {             // the children become roots
   remove( c , z->child );
   c->parent = nullptr;
   c->mark = false;
   splice( c , f_min );
   }
  if( z->right == z )
   f_min = nullptr;
  else {
   f_min = z->right;
   unlink( z );
   consolidate();
   }
  --f_n;
  T v = std::move( z->value );
  delete z;
  return( v );
  }

 /// O( 1 ) amortized time
 void decrease( Handle x , const P & p ) {
  if( x->priority < p )
   throw( std::invalid_argument(
             "ADS::FibonacciHeapPriorityQueue::decrease: larger priority" ) );
  x->priority = p;
  Node * y = x->parent;
  if( y && ( x->priority < y->priority ) ) {
   cut( x , y );
   for( Node * z = y ; z->parent ; ) {       // the cascading cut
    if( ! z->mark ) {
     z->mark = true;
     break;
     }
    Node * w = z->parent;
    cut( z , w );
    z = w;
    }
   }
  if( x->priority < f_min->priority )
   f_min = x;
  }

/*--------------------------------------------------------------------------*/
 private:

 Node * top( void ) const {
  if( ! f_min )
   throw( std::logic_error( "ADS::FibonacciHeapPriorityQueue: empty" ) );
  return( f_min );
  }

 /// x leaves its circular list
 static void unlink( Node * x ) {
  x->left->right = x->right;
  x->right->left = x->left;
  x->left = x->right = x;
  }

 /// x, alone, goes into the circular list of y
 static void splice( Node * x , Node * y ) {
  x->right = y->right;
  x->left = y;
  y->right->left = x;
  y->right = x;
  }

 /// x leaves the list whose entry is head, which follows it if needed
 static void remove( Node * x , Node * & head ) {
  head = ( x->right == x ) ? nullptr : x->right;
  unlink( x );
  }

 void add_root( Node * z ) {
  if( ! f_min )
   f_min = z;
  else {
   splice( z , f_min );
   if( z->priority < f_min->priority )
    f_min = z;
   }
  }

 /// x, a child of y, becomes a root
 void cut( Node * x , Node * y ) {
  if( y->child == x )
   remove( x , y->child );
  else
   unlink( x );
  --y->degree;
  x->parent = nullptr;
  x->mark = false;
  splice( x , f_min );
  }

 /// links the roots of equal degree until all the degrees differ
 void consolidate( void ) {
  std::vector< Node * > roots;
  Node * x = f_min;
  do {
   roots.push_back( x );
   x = x->right;
   } while( x != f_min );
  std::vector< Node * > A;
  for( Node * w : roots ) {
   unlink( w );
   Node * y = w;
   for( int d = y->degree ; ; d = y->degree ) {
    if( int( A.size() ) <= d )
     A.resize( d + 1 , nullptr );
    Node * z = A[ d ];
    if( ! z )
     break;
    if( z->priority < y->priority )
     std::swap( y , z );
    z->parent = y;                           // z under y
    z->mark = false;
    if( y->child )
     splice( z , y->child );
    else
     y->child = z;
    ++y->degree;
    A[ d ] = nullptr;
    }
   A[ y->degree ] = y;
   }
  f_min = nullptr;
  for( Node * y : A )
   if( y )
    add_root( y );
  }

 Node * f_min = nullptr;   ///< the root of least priority
 int f_n = 0;              ///< the number of items

 };  // end( class FibonacciHeapPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* FibonacciHeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------ End File FibonacciHeapPriorityQueue.h -----------------*/
/*--------------------------------------------------------------------------*/
