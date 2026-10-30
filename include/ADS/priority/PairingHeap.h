/*--------------------------------------------------------------------------*/
/*--------------------------- File PairingHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see `PriorityQueue.h`] on a pairing heap, beyond the
 * book: the self-adjusting heap of Fredman, Sedgewick, Sleator and Tarjan,
 * the simplest of the heaps with decrease-key and, in the experiments of
 * Stasko and Vitter and of Larkin, Sen and Tarjan, the fastest one with
 * pointers in practice.
 *
 * The heap is one tree, each node not smaller than its parent, kept in the
 * form child-sibling: a node points to its leftmost child and to its right
 * sibling, and back to its left sibling, or to its parent if it is the
 * leftmost child, so that a node is cut from its tree in \f$ O( 1 ) \f$. The
 * linking of two trees makes the root of larger key the leftmost child of
 * the other one. `insert()` links a tree of one node with the root;
 * `decrease()` cuts the node with its subtree, unless it is the leftmost
 * child and still not smaller than its parent, and links it with the root;
 * `deleteMin()` removes the root and joins its children by two passes: the
 * first links them in pairs from the left, the second links the results
 * from the right into one tree. The amortized bounds known for the two
 * passes are \f$ O( \log n ) \f$ for `deleteMin()` and
 * \f$ O( 2^{2 \sqrt{\log \log n}} ) \f$ for `insert()` and `decrease()`
 * (Pettie; \f$ O( 1 ) \f$ for `insert()`, Iacono), while Fredman proved that
 * \f$ \Omega( \log \log n ) \f$ is needed by `decrease()` in this family of
 * heaps, against the \f$ O( 1 ) \f$ of the Fibonacci heap [see
 * `FibonacciHeapPriorityQueue.h`], whose constants are larger.
 *
 * With `Aux` true the heap is the auxiliary two-pass variant of Stasko and
 * Vitter: the trees of `insert()` and `decrease()` go to a list beside the
 * root, with a pointer to the least of the roots, so that each costs one
 * comparison, and `deleteMin()` first joins the list by the multipass
 * pairing (the trees linked in pairs from the front of a queue, the
 * result to its back, until one is left) and links it with the root;
 * it pays when the inserts outnumber the calls of `deleteMin()`.
 *
 * The nodes come from an `ObjectPool` [see `ObjectPool.h`], which takes back
 * the node of `deleteMin()` for the next `insert()`, so that a node costs no
 * call to the allocator. The two passes run in place on the sibling
 * pointers, with no stack and no vector.
 *
 * Beyond the PRIORITYQUEUE, `erase()` removes the item of any handle,
 * joining the children of its node by two passes in its place, and
 * `increase()` raises a priority by taking the node out in the same way
 * and inserting it again, alone, so that its handle stays valid; both
 * cost as a `deleteMin()` on the subtree of the node, \f$ O( \log n ) \f$
 * amortized, and `update()` takes `decrease()` or `increase()` as the new
 * priority asks.
 *
 * The contracts [see `Contract.h`]: `min()`, `minPriority()` and `deleteMin()`
 * need a heap that is not empty, `decrease()` a handle of a node of this
 * heap and a priority not larger than its own, `increase()` one not
 * smaller, `erase()` and `update()` a handle of this heap; the invariant,
 * checked on
 * heaps of at most 1024 nodes, is the order of the heap, the links back
 * and `size()` counting the nodes. The operations are defined in
 * `PairingHeap.tpp`.
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
/// `Aux`
template< class T , std::totally_ordered P = int , bool Aux = false >
class PairingHeap {

/*--------------------------------------------------------------------------*/
 public:

 /// a node of the heap
 struct Node {
  P priority;               ///< the priority
  T value;                  ///< the item
  Node * child = nullptr;   ///< the leftmost child
  Node * next = nullptr;    ///< the right sibling
  Node * prev = nullptr;    ///< the left sibling, or the parent
  };

 using Item = T;            ///< the type of the items
 using Priority = P;        ///< the type of the priorities
 using Handle = Node *;     ///< what `insert()` gives

 /// the empty heap
 PairingHeap( void ) = default;

 /// no copy: the handles point into the heap
 PairingHeap( const PairingHeap & ) = delete;

 /// no copy: the handles point into the heap
 PairingHeap & operator=( const PairingHeap & ) = delete;

 /// the items of `other`, which is left empty; its handles stay valid here
 PairingHeap( PairingHeap && other ) noexcept;

 /// the heap and its nodes destroyed
 ~PairingHeap();

 /// whether the heap has no item
 [[nodiscard]] bool empty( void ) const { return( f_root == nullptr ); }

 /// the number of items, \f$ n \f$
 [[nodiscard]] int size( void ) const { return( f_n ); }

 /// the item of smallest priority
 /// \pre the heap is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the smallest priority
 /// \pre the heap is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// inserts \f$ x \f$ with the priority \f$ p \f$ and gives its handle, a
 /// tree of one node linked with the root, or put in the list if `Aux`
 ///
 /// In pseudocode (Fredman, Sedgewick, Sleator and Tarjan, beyond the
 /// book; the list of Stasko and Vitter if `Aux`):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , p ) \cr
 /// 2 & \quad z \gets \text{a node } ( p , x ) \text{ with no child};
 ///     \ n \gets n + 1 \cr
 /// 3 & \quad \textbf{if}\ \mathit{root} = \textbf{nil}\ \textbf{then}
 ///     \ \mathit{root} \gets z;\ \mathit{min} \gets z \cr
 /// 4 & \quad \textbf{else if}\ \mathit{Aux}\ \textbf{then} \cr
 /// 5 & \qquad \text{put } z \text{ in the list after } \mathit{root} \cr
 /// 6 & \qquad \textbf{if}\ p < \mathit{min}.\mathit{priority}
 ///     \ \textbf{then}\ \mathit{min} \gets z \cr
 /// 7 & \quad \textbf{else}\ \mathit{root} \gets \mathrm{link}(
 ///     \mathit{root} , z );\ \mathit{min} \gets \mathit{root} \cr
 /// 8 & \quad \textbf{return}\ z \cr
 /// 9 & \textbf{function}\ \mathrm{link}( a , b ) \cr
 /// 10 & \quad \textbf{if}\ b.\mathit{priority} < a.\mathit{priority}
 ///     \ \textbf{then}\ \text{swap } a \text{ and } b \cr
 /// 11 & \quad \text{make } b \text{ the leftmost child of } a \cr
 /// 12 & \quad \textbf{return}\ a
 /// \end{array}}
 /// \f]
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// removes the item of smallest priority and returns it, its children
 /// joined by two passes; the handle of the item is no longer valid
 ///
 /// In pseudocode (Fredman, Sedgewick, Sleator and Tarjan, beyond the
 /// book; the multipass pairing of the list of Stasko and Vitter if `Aux`):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deleteMin}() \cr
 /// 2 & \quad z \gets \mathit{min};\ n \gets n - 1 \cr
 /// 3 & \quad \textbf{if not}\ \mathit{Aux}\ \textbf{then}
 ///     \ \mathit{root} \gets \mathrm{twoPass}( \text{the children of } z )
 ///     \cr
 /// 4 & \quad \textbf{else} \cr
 /// 5 & \qquad \textbf{if}\ z \ne \mathit{root}\ \textbf{then}
 ///     \ \text{remove } z \text{ from the list} \cr
 /// 6 & \qquad \ell \gets \mathrm{multiPass}( \text{the list after }
 ///     \mathit{root} ) \cr
 /// 7 & \qquad \mathit{root} \gets \text{the link of } \mathit{root}
 ///     \text{ (if not } z \text{), } \mathrm{twoPass}( \text{the}
 ///     \text{ children of } z ) \text{ and } \ell \cr
 /// 8 & \quad \mathit{min} \gets \mathit{root};\ \textbf{return}
 ///     \ z.\mathit{value} \cr
 /// 9 & \textbf{function}\ \mathrm{twoPass}( t_1 , \ldots , t_k ) \cr
 /// 10 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ \lfloor k / 2
 ///     \rfloor\ \textbf{do}\ u_i \gets \mathrm{link}( t_{2 i - 1} ,
 ///     t_{2 i} )\quad \triangleright\ \text{from the left} \cr
 /// 11 & \quad \text{with } k \text{ odd, } u_{\lceil k / 2 \rceil}
 ///     \gets t_k \cr
 /// 12 & \quad r \gets u_{\lceil k / 2 \rceil} \cr
 /// 13 & \quad \textbf{for}\ i \gets \lceil k / 2 \rceil - 1
 ///     \ \textbf{downto}\ 1\ \textbf{do}\ r \gets \mathrm{link}( r , u_i )
 ///     \quad \triangleright\ \text{from the right} \cr
 /// 14 & \quad \textbf{return}\ r \cr
 /// 15 & \textbf{function}\ \mathrm{multiPass}( t_1 , \ldots , t_k ) \cr
 /// 16 & \quad Q \gets \text{a queue of } t_1 , \ldots , t_k \cr
 /// 17 & \quad \textbf{while}\ |Q| > 1\ \textbf{do}\ \mathrm{enqueue}(
 ///     Q , \mathrm{link}( \mathrm{dequeue}( Q ) , \mathrm{dequeue}( Q ) ) )
 ///     \cr
 /// 18 & \quad \textbf{return}\ \text{the tree of } Q
 /// \end{array}}
 /// \f]
 /// \pre the heap is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

 /// lowers to \f$ p \f$ the priority of the node \f$ x \f$, cut with its
 /// subtree and linked with the root, or put in the list if `Aux`
 ///
 /// In pseudocode (Fredman, Sedgewick, Sleator and Tarjan, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( x , p ) \cr
 /// 2 & \quad x.\mathit{priority} \gets p \cr
 /// 3 & \quad \textbf{if}\ x \ne \mathit{root}\ \textbf{and}
 ///     \ ( x \text{ is not a leftmost child, or } p <
 ///     x.\mathit{parent}.\mathit{priority} )\ \textbf{then} \cr
 /// 4 & \qquad \text{cut } x \text{ with its subtree} \cr
 /// 5 & \qquad \textbf{if}\ \mathit{Aux}\ \textbf{then}\ \text{put } x
 ///     \text{ in the list after } \mathit{root} \cr
 /// 6 & \qquad \textbf{else}\ \mathit{root} \gets \mathrm{link}(
 ///     \mathit{root} , x ) \cr
 /// 7 & \quad \mathit{min} \gets x\ \text{if } \mathit{Aux} \text{ and }
 ///     p < \mathit{min}.\mathit{priority} \text{, else } \mathit{root}
 ///     \text{ if not } \mathit{Aux}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is a node of this heap and \f$ p \f$ is not larger than
 ///      its priority
 /// \post \f$ x \f$ has the priority \f$ p \f$
 void decrease( Handle x , const P & p );

 /// removes the item of the node \f$ x \f$, wherever it is, and returns
 /// it: the subtrees of its children joined by two passes take its place;
 /// the handle is no longer valid
 ///
 /// In pseudocode (the delete of Fredman, Sedgewick, Sleator and Tarjan,
 /// beyond the book), \f$ \mathrm{twoPass}() \f$ that of `deleteMin()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{erase}( x ) \cr
 /// 2 & \quad \textbf{if}\ x = \mathit{min}\ \textbf{then return}
 ///     \ \mathrm{deleteMin}() \cr
 /// 3 & \quad t \gets \mathrm{twoPass}( \text{the children of } x );
 ///     \ n \gets n - 1 \cr
 /// 4 & \quad \textbf{if}\ x = \mathit{root}\ \textbf{then}
 ///     \ \mathit{root} \gets t \text{, or the first tree of the list}
 ///     \text{ if } t = \textbf{nil}\quad \triangleright\ \text{only if }
 ///     \mathit{Aux} \cr
 /// 5 & \quad \textbf{else} \cr
 /// 6 & \qquad \text{cut } x \text{ from its list of siblings} \cr
 /// 7 & \qquad \textbf{if}\ t \ne \textbf{nil}\ \textbf{and}
 ///     \ \mathit{Aux}\ \textbf{then}\ \text{put } t \text{ in the list }
 ///     \text{after } \mathit{root} \cr
 /// 8 & \qquad \textbf{else if}\ t \ne \textbf{nil}\ \textbf{then}
 ///     \ \mathit{root} \gets \mathrm{link}( \mathit{root} , t );
 ///     \ \mathit{min} \gets \mathit{root} \cr
 /// 9 & \quad \textbf{return}\ x.\mathit{value}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is a node of this heap
 /// \post one item less
 T erase( Handle x );

 /// raises to \f$ p \f$ the priority of the node \f$ x \f$, which keeps
 /// its handle: the node leaves the heap as in `erase()` and comes back as
 /// in `insert()`, alone, since its children may now be smaller than it
 ///
 /// In pseudocode (beyond the book), \f$ \mathrm{detach}( x ) \f$ the
 /// steps 2 to 8 of `erase()` that leave the node out and keep it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{increase}( x , p ) \cr
 /// 2 & \quad \mathrm{detach}( x );\ x.\mathit{priority} \gets p \cr
 /// 3 & \quad \text{link } x \text{ with } \mathit{root} \text{, or put}
 ///     \text{ it in the list if } \mathit{Aux} \text{, as }
 ///     \mathrm{insert}() \text{ does}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is a node of this heap and \f$ p \f$ is not smaller than
 ///      its priority
 /// \post \f$ x \f$ has the priority \f$ p \f$
 void increase( Handle x , const P & p );

 /// sets the priority of the node \f$ x \f$ to \f$ p \f$, by `decrease()`
 /// or by `increase()`
 /// \pre \f$ x \f$ is a node of this heap
 /// \post \f$ x \f$ has the priority \f$ p \f$
 void update( Handle x , const P & p );

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

 /// f_min out of the heap, the children joined, the size unchanged
 void unlinkMin( void );

 /// x out of the heap, the trees of its children joined in its place,
 /// the size unchanged; x left with no link
 void detach( Node * x );

 /// x, with no link, into the heap as insert() puts a node
 void attach( Node * x );

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
