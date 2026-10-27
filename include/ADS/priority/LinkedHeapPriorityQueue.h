/*--------------------------------------------------------------------------*/
/*--------------------- File LinkedHeapPriorityQueue.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see `PriorityQueue.h`] on a heap realized as a binary
 * tree with pointers, beyond the book, instead of the vector of section
 * 10.1: the nodes are those of a complete binary tree numbered by levels
 * from 1, and the path from the root to the node \f$ k \f$ follows the bits
 * of \f$ k \f$ after the leading one, 0 to the left and 1 to the right, so
 * that the last leaf, where `insert()` adds a node and from where
 * `deleteMin()` takes the one that replaces the root, is found in
 * \f$ O( \log n ) \f$ steps. The items move up and down by swapping the
 * contents of the nodes, as in the heap on a vector, and each operation
 * takes \f$ O( \log n ) \f$ time.
 *
 * With `L` true the heap keeps instead a pointer to the last leaf, and
 * finds the place of the next one by walking the tree: the new leaf is the
 * right sibling of the last one if this is a left child; otherwise one goes
 * up while on a right child, then, unless at the root, to the right
 * sibling, and down along the left children to a leaf, whose left child is
 * the new leaf (from the root this starts a new level). After `deleteMin()`
 * the new last leaf is found symmetrically: the left sibling of the old one
 * if this was a right child; otherwise up while on a left child, to the left
 * sibling unless at the root, and down along the right children. Both walks
 * take \f$ O( \log n ) \f$ steps, and need no counter of the nodes.
 *
 * The nodes come from an `ObjectPool` [see `ObjectPool.h`] of the queue,
 * which takes back the node of `deleteMin()` for the next `insert()`, so
 * that a node costs no call to the allocator.
 *
 * The contracts [see `Contract.h`]: `min()`, `minPriority()` and
 * `deleteMin()` need a queue that is not empty; after `deleteMin()` no item
 * has a smaller priority than the one removed; the invariant is the order of
 * the heap, the parents, and the shape, i.e., the node \f$ k \f$ has the
 * children \f$ 2 k \f$ and \f$ 2 k + 1 \f$ exactly when they are at most
 * `size()`, and with `L` the pointer is the node `size()`. On a heap of more
 * than 1024 nodes only the root and the last leaf are checked, so that a
 * long sequence of operations at the level 3 stays linear. The operations
 * are defined in `LinkedHeapPriorityQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

/// a priority queue on a heap with pointers, which keeps a pointer to the
/// last leaf if `L`
template< class T , std::totally_ordered P = int , bool L = false >
class LinkedHeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities

 /// the empty queue
 LinkedHeapPriorityQueue( void ) = default;

 /// no copy: the nodes belong to the queue
 LinkedHeapPriorityQueue( const LinkedHeapPriorityQueue & ) = delete;

 /// no copy: the nodes belong to the queue
 LinkedHeapPriorityQueue & operator=( const LinkedHeapPriorityQueue & ) =
  delete;

 /// the items of `other`, which is left empty
 LinkedHeapPriorityQueue( LinkedHeapPriorityQueue && other ) noexcept;

 /// the items of `other`, which is left empty, in place of these
 LinkedHeapPriorityQueue & operator=( LinkedHeapPriorityQueue && other )
  noexcept;

 /// the queue and its nodes destroyed
 ~LinkedHeapPriorityQueue();

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] int size( void ) const;

 /// the item of smallest priority, at the root
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the smallest priority
 /// \pre the queue is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// inserts \f$ x \f$ with the priority \f$ p \f$, a new last leaf moved up
 /// while smaller than its parent, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (the heap of section 10.1.3 on pointers, beyond the
 /// book), \f$ \mathrm{at}( k ) \f$ the node \f$ k \f$ of the levels, by
 /// the bits of \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x , p ) \cr
 /// 2 & \quad n \gets n + 1 \cr
 /// 3 & \quad \textbf{if}\ n = 1\ \textbf{then}\ \mathit{root} \gets
 ///     \text{a node } ( p , x );\ \textbf{return} \cr
 /// 4 & \quad u \gets \mathrm{nextParent}()\ \text{if } L \text{, else }
 ///     \mathrm{at}( \lfloor n / 2 \rfloor ) \cr
 /// 5 & \quad v \gets \text{a node } ( p , x ) \text{ as the left child of }
 ///     u \text{, or the right one if the left is taken} \cr
 /// 6 & \quad \textbf{while}\ v \ne \mathit{root}\ \textbf{and}
 ///     \ v.\mathit{priority} < v.\mathit{parent}.\mathit{priority}
 ///     \ \textbf{do} \cr
 /// 7 & \qquad \text{swap the contents of } v \text{ and }
 ///     v.\mathit{parent};\ v \gets v.\mathit{parent} \cr
 /// 8 & \textbf{function}\ \mathrm{at}( k ) \cr
 /// 9 & \quad u \gets \mathit{root} \cr
 /// 10 & \quad \textbf{foreach}\ \text{bit } b \text{ of } k
 ///     \text{ after the leading one, from the highest}\ \textbf{do} \cr
 /// 11 & \qquad u \gets u.\mathit{left}\ \text{if } b = 0 \text{, else }
 ///     u.\mathit{right} \cr
 /// 12 & \quad \textbf{return}\ u \cr
 /// 13 & \textbf{function}\ \mathrm{nextParent}()\quad \triangleright
 ///     \ \text{the node whose child the next leaf is} \cr
 /// 14 & \quad u \gets \mathit{last} \cr
 /// 15 & \quad \textbf{if}\ u = \mathit{root}\ \textbf{then return}\ u \cr
 /// 16 & \quad \textbf{if}\ u \text{ is a left child}\ \textbf{then}
 ///     \textbf{return}\ u.\mathit{parent} \cr
 /// 17 & \quad \textbf{while}\ u \ne \mathit{root}\ \textbf{and}\ u
 ///     \text{ is a right child}\ \textbf{do}\ u \gets u.\mathit{parent}
 ///     \cr
 /// 18 & \quad \textbf{if}\ u \ne \mathit{root}\ \textbf{then}\ u \gets
 ///     u.\mathit{parent}.\mathit{right} \cr
 /// 19 & \quad \textbf{while}\ u.\mathit{left} \ne \textbf{nil}
 ///     \ \textbf{do}\ u \gets u.\mathit{left} \cr
 /// 20 & \quad \textbf{return}\ u
 /// \end{array}}
 /// \f]
 /// \post one item more
 void insert( const T & x , const P & p );

 /// removes the item of smallest priority and returns it, the last leaf
 /// moved into the root and down while larger than a child, in
 /// \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (the heap of section 10.1.3 on pointers, beyond the
 /// book), \f$ \mathit{last} \f$ found as by `insert()`, or by the walk
 /// symmetric to \f$ \mathrm{nextParent}() \f$ if `L`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deleteMin}() \cr
 /// 2 & \quad x \gets \mathit{root}.\mathit{value} \cr
 /// 3 & \quad \textbf{if}\ n = 1\ \textbf{then}\ \mathit{root} \gets
 ///     \textbf{nil};\ n \gets 0;\ \textbf{return}\ x \cr
 /// 4 & \quad \ell \gets \mathit{last}\ \text{if } L \text{, else }
 ///     \mathrm{at}( n ) \cr
 /// 5 & \quad \text{move the contents of } \ell \text{ to }
 ///     \mathit{root};\ \text{detach } \ell \text{ from its parent};
 ///     \ n \gets n - 1 \cr
 /// 6 & \quad v \gets \mathit{root} \cr
 /// 7 & \quad \textbf{loop} \cr
 /// 8 & \qquad m \gets \text{the child of } v \text{ of least priority,}
 ///     \text{ if smaller than } v \text{, else } v \cr
 /// 9 & \qquad \textbf{if}\ m = v\ \textbf{then return}\ x \cr
 /// 10 & \qquad \text{swap the contents of } v \text{ and } m;\ v \gets m
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

/*--------------------------------------------------------------------------*/
 private:

 /// a node of the heap
 struct Node {
  P priority;                ///< the priority
  T value;                   ///< the item
  Node * parent;             ///< the parent, `nullptr` for the root
  Node * left = nullptr ,    ///< the left child
       * right = nullptr;    ///< the right child
  /// the node of the priority \f$ p \f$ and of the item \f$ v \f$, a child
  /// of \f$ u \f$
  Node( const P & p , const T & v , Node * u )
   : priority( p ) , value( v ) , parent( u ) {}
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( priority , item ), in the symmetric
 // order of the tree (no useful order), bidirectional

 /// the shape of the tree, for `PathIterator`
 struct Shape {
  /// the root of the queue \f$ o \f$
  static auto root( const auto & o ) {
   return( static_cast< const Node * >( o.f_root ) );
   }
  /// the left child of the node \f$ n \f$
  static auto left( const auto * n ) {
   return( static_cast< const Node * >( n->left ) );
   }
  /// the right child of the node \f$ n \f$
  static auto right( const auto * n ) {
   return( static_cast< const Node * >( n->right ) );
   }
  /// the priority and the item of the node \f$ n \f$
  static auto read( const auto * n ) {
   return( std::pair< const P & , const T & >( n->priority , n->value ) );
   }
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
 /// the iterator of the view, constant
 using const_iterator = PathIterator< LinkedHeapPriorityQueue , Shape >;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the first item of the view
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 /// the end of the view
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the node k >= 1 of the levels, by the bits of k
 Node * at( int k ) const;

 static void swap_contents( Node * a , Node * b );

 /// the subtree of u, the node k, in order and in shape; its nodes counted
 /// in n
 [[nodiscard]] bool shaped( const Node * u , int k , int & n ) const;

 /// the invariant, \f$ O( n ) \f$; on more than 1024 nodes, the root and the
 /// last leaf only
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
