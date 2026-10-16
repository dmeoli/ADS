/*--------------------------------------------------------------------------*/
/*--------------------- File LinkedHeapPriorityQueue.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a heap realized as a binary
 * tree with pointers, beyond the book, instead of the vector of section
 * 10.1: the nodes are those of a complete binary tree numbered by levels
 * from 1, and the path from the root to the node k follows the bits of k
 * after the leading one, 0 to the left and 1 to the right, so that the
 * last leaf, where insert() adds a node and from where deleteMin() takes
 * the one that replaces the root, is found in \f$ O( \log n ) \f$ steps.
 * The items move up and down by swapping the contents of the nodes, as in
 * the heap on a vector, and each operation takes \f$ O( \log n ) \f$ time.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#ifndef __ADS_LinkedHeapPriorityQueue
 #define __ADS_LinkedHeapPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <memory>
#include <stdexcept>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS LinkedHeapPriorityQueue ----------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a heap with pointers
template< class T , std::totally_ordered P = int >
class LinkedHeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities

 [[nodiscard]] bool isEmpty( void ) const { return( f_n == 0 ); }

 [[nodiscard]] int size( void ) const { return( f_n ); }

 [[nodiscard]] const T & min( void ) const { return( top()->value ); }

 [[nodiscard]] const P & minPriority( void ) const {
  return( top()->priority );
  }

 /// a new last leaf, moved up while smaller than its parent
 void insert( const T & x , const P & p ) {
  ++f_n;
  if( f_n == 1 ) {
   f_root = std::make_unique< Node >( p , x , nullptr );
   return;
   }
  Node * u = at( f_n / 2 );
  auto & slot = f_n % 2 ? u->right : u->left;
  slot = std::make_unique< Node >( p , x , u );
  for( Node * v = slot.get() ; v->parent &&
       ( v->priority < v->parent->priority ) ; v = v->parent )
   swap_contents( v , v->parent );
  }

 /// the last leaf into the root, moved down while larger than a child
 T deleteMin( void ) {
  Node * r = top();
  T x = std::move( r->value );
  if( f_n == 1 ) {
   f_root.reset();
   f_n = 0;
   return( x );
   }
  Node * last = at( f_n );
  r->value = std::move( last->value );
  r->priority = last->priority;
  Node * p = last->parent;
  ( f_n % 2 ? p->right : p->left ).reset();
  --f_n;
  for( Node * v = r ; ; ) {
   Node * m = v;
   for( Node * c : { v->left.get() , v->right.get() } )
    if( c && ( c->priority < m->priority ) ) m = c;
   if( m == v ) break;
   swap_contents( v , m );
   v = m;
   }
  return( x );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  P priority;
  T value;
  Node * parent;
  std::unique_ptr< Node > left , right;
  Node( const P & p , const T & v , Node * u )
   : priority( p ) , value( v ) , parent( u ) {}
  };

 Node * top( void ) const {
  if( ! f_root )
   throw( std::logic_error( "ADS::LinkedHeapPriorityQueue: empty" ) );
  return( f_root.get() );
  }

 /// the node k >= 1 of the levels, by the bits of k
 Node * at( int k ) const {
  int b = 1;
  while( ( b << 1 ) <= k ) b <<= 1;
  Node * u = f_root.get();
  for( b >>= 1 ; b ; b >>= 1 )
   u = ( k & b ) ? u->right.get() : u->left.get();
  return( u );
  }

 static void swap_contents( Node * a , Node * b ) {
  std::swap( a->priority , b->priority );
  std::swap( a->value , b->value );
  }

 std::unique_ptr< Node > f_root;
 int f_n = 0;

 };  // end( class LinkedHeapPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* LinkedHeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File LinkedHeapPriorityQueue.h -------------------*/
/*--------------------------------------------------------------------------*/
