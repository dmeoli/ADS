/*--------------------------------------------------------------------------*/
/*-------------------------- File ThreadedTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The threaded binary search tree of Perlis and Thornton, beyond the book: a
 * missing left child is replaced by a link (a thread) to the predecessor in
 * symmetric order, and a missing right child by a thread to the successor, a
 * bit of each node telling a child from a thread; the symmetric visit then
 * needs neither a stack nor the recursion, the successor of a node being its
 * right thread or the leftmost node of its right subtree (Algorithm S of
 * Knuth, section 2.3.1), in \f$ O( 1 ) \f$ amortized time each. A head node,
 * whose left child is the root and which is the predecessor of the first
 * node and the successor of the last one, closes the threads, as in the
 * representation of Knuth.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is
 * there; the invariant is that the visit by threads gives the keys in
 * increasing order, `size()` of them, an \f$ O( n ) \f$ check at the level
 * 3. The operations are defined in `ThreadedTree.tpp`.
 *
 * \see A. J. Perlis, C. Thornton, Symbol manipulation by threaded lists,
 *      Communications of the ACM 3(4), 195-204, 1960,
 *      doi:10.1145/367177.367202
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.3.1
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

#ifndef __ADS_ThreadedTree
 #define __ADS_ThreadedTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ThreadedTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary search tree with threads in symmetric order
template< std::totally_ordered K >
class ThreadedTree {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty tree, the head node alone, its left link a thread to itself
 ThreadedTree( void );

 /// no copy
 ThreadedTree( const ThreadedTree & ) = delete;

 /// no copy
 ThreadedTree & operator=( const ThreadedTree & ) = delete;

 /// frees the nodes
 ~ThreadedTree();

 /// inserts \f$ k \f$, if it is not there
 ///
 /// In pseudocode (Knuth, section 2.3.1, the insertion into a threaded
 /// tree, beyond the book): the new leaf takes over the thread that the
 /// search ends on, and threads back to its parent on the other side:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k ) \cr
 /// 2 & \quad x \gets \text{the root} \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{if}\ k = x.\mathit{key}\ \textbf{then return} \cr
 /// 5 & \qquad \textbf{if}\ k < x.\mathit{key}\ \textbf{and}\ x.\mathit{left}
 ///     \text{ is a thread}\ \textbf{then} \cr
 /// 6 & \qquad\quad x.\mathit{left} \gets \text{a leaf } ( k ,\ \text{left
 ///     thread } x.\mathit{left} ,\ \text{right thread } x );
 ///     \ \textbf{return} \cr
 /// 7 & \qquad \textbf{if}\ k > x.\mathit{key}\ \textbf{and}
 ///     \ x.\mathit{right} \text{ is a thread}\ \textbf{then} \cr
 /// 8 & \qquad\quad x.\mathit{right} \gets \text{a leaf } ( k ,\ \text{left
 ///     thread } x ,\ \text{right thread } x.\mathit{right} );
 ///     \ \textbf{return} \cr
 /// 9 & \qquad x \gets x.\mathit{left} \text{ if } k < x.\mathit{key}
 ///     \text{, } x.\mathit{right} \text{ otherwise}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is there
 void insert( const K & k );

 /// whether \f$ k \f$ is there, by the search of a binary search tree that
 /// stops at a thread
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// `f( k )` on the keys by increasing order, by the threads: no stack, no
 /// recursion
 template< class F >
 void forEach( F && f ) const;

 /// the keys by decreasing order, by the left threads, as `forEach()` with
 /// the predecessors
 [[nodiscard]] std::vector< K > reversed( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node: the key, the two links and whether they are threads
 struct Node {
  K key{};                                   ///< the key
  Node * left = nullptr ,                    ///< the left link
       * right = nullptr;                    ///< the right link
  bool lthread = true ,                      ///< the left link is a thread
       rthread = true;                       ///< the right link is a thread
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the keys in increasing order, bidirectional by the
 // threads with no stack, the head node being the end

 /// the constant iterator in symmetric order, by the threads
 class const_iterator {
  public:
  /// the category of the iterator, bidirectional
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = K;                     ///< the type of the keys
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  using reference = const K &;              ///< a constant reference
  using pointer = const K *;                ///< a constant pointer

  /// an iterator on no tree
  const_iterator( void ) = default;

  /// on the node \f$ x \f$ of \f$ t \f$, the end if \f$ x \f$ is the head
  const_iterator( const ThreadedTree * t , const Node * x )
   : f_t( t ) , f_x( x ) {}

  /// the key
  reference operator*( void ) const { return( f_x->key ); }
  /// the address of the key
  pointer operator->( void ) const { return( &f_x->key ); }
  /// to the successor, by `successor()`
  const_iterator & operator++( void ) {
   f_x = f_t->successor( f_x );
   return( *this );
   }
  /// to the predecessor, by `predecessor()`
  const_iterator & operator--( void ) {
   f_x = f_t->predecessor( f_x );
   return( *this );
   }
  /// to the successor, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// to the predecessor, returning the iterator before the step
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }
  /// equal on the same node
  bool operator==( const const_iterator & o ) const {
   return( f_x == o.f_x );
   }

  private:
  const ThreadedTree * f_t = nullptr;
  const Node * f_x = nullptr;
  };

 using value_type = K;                     ///< the type of the keys
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = const K &;              ///< a constant reference
 using const_reference = const K &;        ///< a constant reference
 using iterator = const_iterator;          ///< a constant iterator

 /// the iterator on the least key, the successor of the head
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , successor( &f_head ) ) );
  }

 /// the iterator past the largest key, the head
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , &f_head ) );
  }

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the successor of \f$ x \f$ in symmetric order, the head after the last
 ///
 /// In pseudocode (Algorithm S of Knuth, section 2.3.1, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{successor}( x ) \cr
 /// 2 & \quad \textbf{if}\ x.\mathit{right} \text{ is a thread}
 ///     \ \textbf{then return}\ x.\mathit{right} \cr
 /// 3 & \quad y \gets x.\mathit{right} \cr
 /// 4 & \quad \textbf{while}\ y.\mathit{left} \text{ is not a thread}
 ///     \ \textbf{do}\ y \gets y.\mathit{left} \cr
 /// 5 & \quad \textbf{return}\ y
 /// \end{array}}
 /// \f]
 [[nodiscard]] const Node * successor( const Node * x ) const;

 /// the predecessor of \f$ x \f$ in symmetric order, the head before the
 /// first, as `successor()` with left and right swapped
 [[nodiscard]] const Node * predecessor( const Node * x ) const;

 /// the visit by threads gives size() increasing keys, \f$ O( n ) \f$
 [[nodiscard]] bool threaded( void ) const;

 Node f_head;               ///< left: the root (or itself if empty)
 std::size_t f_size = 0;   ///< the number of keys

 };  // end( class ThreadedTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ThreadedTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ThreadedTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ThreadedTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
