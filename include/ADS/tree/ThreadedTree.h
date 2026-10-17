/*--------------------------------------------------------------------------*/
/*-------------------------- File ThreadedTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The threaded binary search tree of Perlis and Thornton, beyond the book: a
 * missing left child is replaced by a link (a thread) to the predecessor
 * in symmetric order, and a missing right child by a thread to the
 * successor, a bit of each node telling a child from a thread; the
 * symmetric visit then needs neither a stack nor the recursion, the
 * successor of a node being its right thread or the leftmost node of its
 * right subtree (Algorithm 2.3.1S), in O( 1 ) amortized time each. A head
 * node, whose left child is the root and which is the predecessor of the
 * first node and the successor of the last one, closes the threads, as in
 * the representation of Knuth.
 *
 * The contracts [see Contract.h]: insert() ensures that the key is there;
 * the invariant is that the visit by threads gives the keys in increasing
 * order, size() of them, an O( n ) check at the level 3. The operations are
 * defined in ThreadedTree.tpp.
 *
 * \see A. J. Perlis, C. Thornton, Symbol manipulation by threaded lists,
 *      Communications of the ACM 3(4), 195-204, 1960,
 *      doi:10.1145/367177.367202
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.3.1
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

#ifndef __ADS_ThreadedTree
 #define __ADS_ThreadedTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <concepts>
#include <cstddef>
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

 ThreadedTree( void );
 ThreadedTree( const ThreadedTree & ) = delete;
 ThreadedTree & operator=( const ThreadedTree & ) = delete;
 ~ThreadedTree();

 /// inserts k, if it is not there
 /// \post k is there
 void insert( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// f( k ) on the keys by increasing order, by the threads: no stack, no
 /// recursion
 template< class F >
 void forEach( F && f ) const;

 /// the keys by decreasing order, by the left threads
 [[nodiscard]] std::vector< K > reversed( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  K key{};
  Node * left = nullptr , * right = nullptr;
  bool lthread = true , rthread = true;   ///< the links are threads
  };

 /// the successor of x in symmetric order, the head after the last
 [[nodiscard]] const Node * successor( const Node * x ) const;

 /// the predecessor of x in symmetric order, the head before the first
 [[nodiscard]] const Node * predecessor( const Node * x ) const;

 /// the visit by threads gives size() increasing keys, O( n )
 [[nodiscard]] bool threaded( void ) const;

 Node f_head;               ///< left: the root (or itself if empty)
 std::size_t f_size = 0;

 };  // end( class ThreadedTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/ThreadedTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ThreadedTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ThreadedTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
