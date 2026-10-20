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
 * With L = true the heap keeps instead a pointer to the last leaf, and
 * finds the place of the next one by walking the tree: the new leaf is the
 * right sibling of the last one if this is a left child; otherwise one goes
 * up while on a right child, then, unless at the root, to the right
 * sibling, and down along the left children to a leaf, whose left child is
 * the new leaf (from the root this starts a new level). After deleteMin()
 * the new last leaf is found symmetrically: the left sibling of the old one
 * if this was a right child; otherwise up while on a left child, to the left
 * sibling unless at the root, and down along the right children. Both walks
 * take \f$ O( \log n ) \f$ steps, and need no counter of the nodes.
 *
 * The nodes come from an ObjectPool [see ObjectPool.h] of the queue,
 * which takes back the node of deleteMin() for the next insert(), so that
 * a node costs no call to the allocator.
 *
 * The contracts [see Contract.h]: min(), minPriority() and deleteMin()
 * need a queue that is not empty; after deleteMin() no item has a smaller
 * priority than the one removed; the invariant is the order of the heap,
 * the parents, and the shape, i.e., the node k has the children 2 k and
 * 2 k + 1 exactly when they are at most size(), and with L the pointer is
 * the node size(). On a heap of more than
 * 1024 nodes only the root and the last leaf are checked, so that a long
 * sequence of operations at the level 3 stays linear. The operations are
 * defined in LinkedHeapPriorityQueue.tpp.
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

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS LinkedHeapPriorityQueue ----------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a heap with pointers
template< class T , std::totally_ordered P = int , bool L = false >
class LinkedHeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities

 LinkedHeapPriorityQueue( void ) = default;
 LinkedHeapPriorityQueue( const LinkedHeapPriorityQueue & ) = delete;
 LinkedHeapPriorityQueue & operator=( const LinkedHeapPriorityQueue & ) =
  delete;

 /// the items of other, which is left empty
 LinkedHeapPriorityQueue( LinkedHeapPriorityQueue && other ) noexcept;

 LinkedHeapPriorityQueue & operator=( LinkedHeapPriorityQueue && other )
  noexcept;

 ~LinkedHeapPriorityQueue();

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] int size( void ) const;

 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// \pre the queue is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// a new last leaf, moved up while smaller than its parent
 /// \post one item more
 void insert( const T & x , const P & p );

 /// the last leaf into the root, moved down while larger than a child
 /// \pre the queue is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  P priority;
  T value;
  Node * parent;
  Node * left = nullptr , * right = nullptr;
  Node( const P & p , const T & v , Node * u )
   : priority( p ) , value( v ) , parent( u ) {}
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( priority , item ), in the symmetric
 // order of the tree (no useful order), bidirectional

 /// the shape of the tree, for PathIterator
 struct Shape {
  static auto root( const auto & o ) {
   return( static_cast< const Node * >( o.f_root ) );
   }
  static auto left( const auto * n ) {
   return( static_cast< const Node * >( n->left ) );
   }
  static auto right( const auto * n ) {
   return( static_cast< const Node * >( n->right ) );
   }
  static auto read( const auto * n ) {
   return( std::pair< const P & , const T & >( n->priority , n->value ) );
   }
  };

 using value_type = std::pair< const P & , const T & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using const_iterator = PathIterator< LinkedHeapPriorityQueue , Shape >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the node k >= 1 of the levels, by the bits of k
 Node * at( int k ) const;

 static void swap_contents( Node * a , Node * b );

 /// the subtree of u, the node k, in order and in shape; its nodes counted
 /// in n
 [[nodiscard]] bool shaped( const Node * u , int k , int & n ) const;

 /// the invariant, O( n ); on more than 1024 nodes, the root and the last
 /// leaf only
 [[nodiscard]] bool isHeap( void ) const;

 /// with L, the node whose child the next leaf is
 /// \pre the heap is not empty
 [[nodiscard]] Node * nextParent( void ) const;

 /// with L, the node that is the last leaf once f_last is removed
 /// \pre f_last is not the root
 [[nodiscard]] Node * previousLeaf( void ) const;

 /// destroys the nodes of the subtree of u
 void release( Node * u );

 Node * f_root = nullptr;
 Node * f_last = nullptr;   ///< the last leaf, the node f_n
 int f_n = 0;
 ObjectPool< Node > f_pool; ///< the nodes

 };  // end( class LinkedHeapPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/LinkedHeapPriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinkedHeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File LinkedHeapPriorityQueue.h -------------------*/
/*--------------------------------------------------------------------------*/
