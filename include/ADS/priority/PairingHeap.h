/*--------------------------------------------------------------------------*/
/*--------------------------- File PairingHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a pairing heap, beyond the
 * book: the self-adjusting heap of Fredman, Sedgewick, Sleator and Tarjan,
 * the simplest of the heaps with decrease-key and, in the experiments of
 * Stasko and Vitter and of Larkin, Sen and Tarjan, the fastest one with
 * pointers in practice.
 *
 * The heap is one tree, each node not smaller than its parent, kept in the
 * form child-sibling: a node points to its leftmost child and to its right
 * sibling, and back to its left sibling, or to its parent if it is the
 * leftmost child, so that a node is cut from its tree in O( 1 ). The
 * linking of two trees makes the root of larger key the leftmost child of
 * the other one. insert() links a tree of one node with the root;
 * decrease() cuts the node with its subtree, unless it is the leftmost
 * child and still not smaller than its parent, and links it with the root;
 * deleteMin() removes the root and joins its children by two passes: the
 * first links them in pairs from the left, the second links the results
 * from the right into one tree. The amortized bounds known for the two
 * passes are O( log n ) for deleteMin() and
 * \f$ O( 2^{2 \sqrt{\log \log n}} ) \f$ for insert() and decrease()
 * (Pettie; O( 1 ) for insert(), Iacono), while Fredman proved that
 * \f$ \Omega( \log \log n ) \f$ is needed by decrease() in this family of
 * heaps, against the O( 1 ) of the Fibonacci heap [see
 * FibonacciHeapPriorityQueue.h], whose constants are larger.
 *
 * With Aux = true the heap is the auxiliary two-pass variant of Stasko and
 * Vitter: the trees of insert() and decrease() go to a list beside the
 * root, with a pointer to the least of the roots, so that each costs one
 * comparison, and deleteMin() first joins the list by the multipass
 * pairing (the trees linked in pairs from the front of a queue, the
 * result to its back, until one is left) and links it with the root;
 * it pays when the inserts outnumber the deleteMin()s.
 *
 * The nodes come from an ObjectPool [see ObjectPool.h], which takes back
 * the node of deleteMin() for the next insert(), so that a node costs no
 * call to the allocator. The two passes run in place on the sibling
 * pointers, with no stack and no vector.
 *
 * The contracts [see Contract.h]: min(), minPriority() and deleteMin()
 * need a heap that is not empty, decrease() a handle of a node of this
 * heap and a priority not larger than its own; the invariant, checked on
 * heaps of at most 1024 nodes, is the order of the heap, the links back
 * and size() counting the nodes. The operations are defined in
 * PairingHeap.tpp.
 *
 * \see M. L. Fredman, R. Sedgewick, D. D. Sleator, R. E. Tarjan, The
 *      pairing heap: a new form of self-adjusting heap, Algorithmica 1(1-4),
 *      111-129, 1986, doi:10.1007/BF01840439
 * \see J. T. Stasko, J. S. Vitter, Pairing heaps: experiments and
 *      analysis, Communications of the ACM 30(3), 234-249, 1987,
 *      doi:10.1145/214748.214759
 * \see M. L. Fredman, On the efficiency of pairing heaps and related data
 *      structures, Journal of the ACM 46(4), 473-501, 1999,
 *      doi:10.1145/320211.320214
 * \see J. Iacono, Improved upper bounds for pairing heaps, Algorithm
 *      Theory - SWAT 2000, Lecture Notes in Computer Science 1851, 32-45,
 *      2000, doi:10.1007/3-540-44985-X_5
 * \see S. Pettie, Towards a final analysis of pairing heaps, 46th Annual
 *      IEEE Symposium on Foundations of Computer Science (FOCS), 174-183,
 *      2005, doi:10.1109/SFCS.2005.75
 * \see D. H. Larkin, S. Sen, R. E. Tarjan, A back-to-basics empirical
 *      study of priority queues, Proceedings of the 16th Workshop on
 *      Algorithm Engineering and Experiments (ALENEX), 61-72, 2014,
 *      doi:10.1137/1.9781611973198.7
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

#ifndef __ADS_PairingHeap
 #define __ADS_PairingHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PairingHeap -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a pairing heap, two-pass, or auxiliary two-pass if
/// Aux
template< class T , std::totally_ordered P = int , bool Aux = false >
class PairingHeap {

/*--------------------------------------------------------------------------*/
 public:

 /// a node of the heap
 struct Node {
  P priority;
  T value;
  Node * child = nullptr;   ///< the leftmost child
  Node * next = nullptr;    ///< the right sibling
  Node * prev = nullptr;    ///< the left sibling, or the parent
  };

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities
 using Handle = Node *;     ///< what insert() gives

 PairingHeap( void ) = default;
 PairingHeap( const PairingHeap & ) = delete;
 PairingHeap & operator=( const PairingHeap & ) = delete;

 /// the items of other, which is left empty; its handles stay valid here
 PairingHeap( PairingHeap && other ) noexcept;

 ~PairingHeap();

 [[nodiscard]] bool empty( void ) const { return( f_root == nullptr ); }

 [[nodiscard]] int size( void ) const { return( f_n ); }

 /// \pre the heap is not empty
 [[nodiscard]] const T & min( void ) const;

 /// \pre the heap is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// a tree of one node linked with the root, or put in the list
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// the root removed, its children joined by two passes; the handle of
 /// the item is no longer valid
 /// \pre the heap is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

 /// x cut with its subtree and linked with the root, or put in the list
 /// \pre x is a node of this heap and p is not larger than its priority
 /// \post x has priority p
 void decrease( Handle x , const P & p );

/*--------------------------------------------------------------------------*/
 private:

 /// the root of the two trees a and b, the other its leftmost child
 static Node * link( Node * a , Node * b );

 /// the trees of the list from x joined by two passes
 static Node * twoPass( Node * x );

 /// the trees of the list from x joined by multiple passes
 static Node * multiPass( Node * x );

 /// x, not the root, leaves its list
 static void cut( Node * x );

 /// the subtree of x heap ordered with consistent links, its nodes counted
 /// in n
 [[nodiscard]] static bool tree( const Node * x , int & n );

 /// the invariant, on heaps of at most 1024 nodes
 [[nodiscard]] bool isHeap( void ) const;

 Node * f_root = nullptr;     ///< the root, and the list after it if Aux
 Node * f_min = nullptr;      ///< the least among the root and the list
 int f_n = 0;                 ///< the number of items
 ObjectPool< Node > f_pool;   ///< the nodes

 };  // end( class PairingHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/PairingHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PairingHeap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File PairingHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
