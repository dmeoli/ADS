/*--------------------------------------------------------------------------*/
/*--------------------------- File MeldableHeap.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The randomized meldable heap of Gambin and Malinowski, beyond the book,
 * as section 10.2 of Morin's book presents it: a binary tree, of any
 * shape, in which each item is not smaller than its parent, and whose only
 * operation is the merge of two such trees: the smaller root stays the
 * root, and the other tree is merged, recursively, with one of its two
 * subtrees chosen by a fair coin. The path followed is a random walk from
 * the root, whose expected length in a tree of \f$ n \f$ nodes is at most
 * \f$ \log_2( n + 1 ) \f$ whatever its shape (Lemma 10.1 of Morin's book),
 * so that a merge of two heaps of \f$ n_1 \f$ and \f$ n_2 \f$ items takes
 * \f$ O( \log n_1 + \log n_2 ) \f$ expected time; `push()` merges a heap of
 * one item, `pop()` merges the two subtrees of the root, `erase( h )`
 * (exercise 10.8 of Morin's book) merges the two subtrees of the node of
 * the handle \f$ h \f$ and hangs the result where the node was, and
 * `meld()` merges a whole heap, all in \f$ O( \log n ) \f$ expected time.
 *
 * Beyond `top()`, the second smallest item is the smaller of the two
 * children of the root, \f$ O( 1 ) \f$ (exercise 10.9), and the \f$ k \f$
 * smallest items in order come from a second heap of the nodes whose
 * parent has been taken, which starts from the root and holds at most
 * \f$ k + 1 \f$ of them, \f$ O( k \log k ) \f$ time (exercise 10.10).
 *
 * Against the code of Morin's book: the merge is a loop, with no
 * recursion, that hangs each node by a pointer to the link that will hold
 * it; the coins are the bits of one word of SplitMix64 [see
 * `RandomHashing.h`] used one at a time, 64 coins per word, where the book
 * calls `rand()` for each (where it is meant to be faster); the nodes come
 * from the allocator `A`, e.g., a `PoolAllocator` [see `PoolAllocator.h`],
 * and two heaps meld only if their allocators are equal, so that each node
 * goes back to the allocator that gave it; and the items are moved, not
 * copied, into the nodes and out of them.
 *
 * The contracts [see `Contract.h`]: `top()`, `pop()` need a heap that is
 * not empty, `erase()` a handle of an item of the heap, `meld()` an equal
 * allocator; the invariant (the order of the heap, the parents and the
 * count of the items) is checked in \f$ O( n ) \f$ time at the level 3
 * while \f$ n \le 256 \f$. The operations are defined in
 * `MeldableHeap.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 10.2 and exercises
 *      10.8 to 10.10
 * \see A. Gambin, A. Malinowski, Randomized meldable priority queues,
 *      Proceedings of SOFSEM'98, Lecture Notes in Computer Science 1521,
 *      344-349, 1998, doi:10.1007/3-540-49477-4_26
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

#ifndef __ADS_MeldableHeap
 #define __ADS_MeldableHeap
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/dictionary/RandomHashing.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MeldableHeap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a randomized meldable heap, the smallest item by `less` on top
template< class T , class Less = std::less< T > ,
	  class A = std::allocator< T > >
class MeldableHeap
{

 struct Node;

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;         ///< the type of the items
 using Handle = Node *;  ///< what `push()` gives, for `erase()`

 /// the empty heap of the order `less`, its coins drawn from the seed
 explicit MeldableHeap( Less less = Less() , std::uint64_t seed = 1 ,
			const A & alloc = A() );

 /// the heap of `o`, which is left empty
 MeldableHeap( MeldableHeap && o ) noexcept;

 /// the heap of `o`, which gets the items of this one
 MeldableHeap & operator=( MeldableHeap && o ) noexcept;

 MeldableHeap( const MeldableHeap & ) = delete;
 MeldableHeap & operator=( const MeldableHeap & ) = delete;

 /// frees the nodes
 ~MeldableHeap();

/*--------------------------------------------------------------------------*/

 /// adds \f$ x \f$, merging the heap with a heap of one node, in
 /// \f$ O( \log n ) \f$ expected time; the handle of its node
 Handle push( T x );

 /// the smallest item
 /// \pre the heap is not empty
 [[nodiscard]] const T & top( void ) const;

 /// the second smallest item, the smaller child of the root, or nil if
 /// there are fewer than two items (exercise 10.9 of Morin's book)
 [[nodiscard]] const T * secondTop( void ) const;

 /// removes the smallest item and returns it, merging the two subtrees of
 /// the root, in \f$ O( \log n ) \f$ expected time
 /// \pre the heap is not empty
 T pop( void );

 /// removes the item of the handle \f$ h \f$ and returns it, merging the
 /// two subtrees of its node in its place, in \f$ O( \log n ) \f$ expected
 /// time
 ///
 /// In pseudocode (exercise 10.8 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{erase}( u ) \cr
 /// 2 & \quad m \gets \mathrm{merge}( u.\mathit{left} , u.\mathit{right} )
 ///     \cr
 /// 3 & \quad \textbf{if}\ m \ne \textbf{nil}\ \textbf{then}
 ///     \ m.\mathit{parent} \gets u.\mathit{parent} \cr
 /// 4 & \quad \textbf{if}\ u = \mathit{root}\ \textbf{then}\ \mathit{root}
 ///     \gets m \cr
 /// 5 & \quad \textbf{else}\ \text{the link of } u.\mathit{parent}
 ///     \text{ to } u \gets m \cr
 /// 6 & \quad \textbf{return}\ u.x
 /// \end{array}}
 /// \f]
 /// since the order holds between \f$ u.\mathit{parent} \f$ and every node
 /// below \f$ u \f$; the walk of the merge starts at \f$ u \f$, with fewer
 /// than \f$ n \f$ nodes below it.
 /// \pre \f$ h \f$ is the handle of an item of this heap
 T erase( Handle h );

 /// the item of the handle \f$ h \f$
 [[nodiscard]] static const T & get( Handle h );

 /// moves the items of `o` into this heap, in \f$ O( \log n ) \f$
 /// expected time; `o` is left empty, and its handles become handles of
 /// this heap
 /// \pre the allocators of the two heaps are equal
 void meld( MeldableHeap & o );

 /// the \f$ k \f$ smallest items in increasing order, or all of them if
 /// fewer, in \f$ O( k \log k ) \f$ time (exercise 10.10 of Morin's book)
 ///
 /// In pseudocode (exercise 10.10 of Morin's book), \f$ Q \f$ a heap of
 /// nodes by their items, each node entering it after its parent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{smallest}( k ) \cr
 /// 2 & \quad Q \gets \{ \mathit{root} \};\ S \gets () \cr
 /// 3 & \quad \textbf{while}\ |S| < k\ \textbf{and}\ Q \ne \emptyset
 ///     \ \textbf{do} \cr
 /// 4 & \qquad u \gets \mathrm{deleteMin}( Q );\ S \gets S \cdot ( u.x ) \cr
 /// 5 & \qquad \text{put the children of } u \text{ in } Q \cr
 /// 6 & \quad \textbf{return}\ S
 /// \end{array}}
 /// \f]
 /// where \f$ Q \f$ holds at most \f$ k + 1 \f$ nodes, since each step
 /// takes one and puts two.
 [[nodiscard]] std::vector< T > smallest( std::size_t k ) const;

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no items
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// removes every item
 void clear( void );

/*--------------------------------------------------------------------------*/
 private:
 /// a node of the tree
 struct Node
 {
  T x;                      ///< the item
  Node * left = nullptr;    ///< the left subtree
  Node * right = nullptr;   ///< the right subtree
  Node * parent = nullptr;  ///< the parent, nil for the root
  };

 using NodeAlloc =
   typename std::allocator_traits< A >::template rebind_alloc< Node >;
 using Traits = std::allocator_traits< NodeAlloc >;

 /// a fair coin, a bit of the word in hand
 bool coin( void );

 /// the merge of the heaps of the roots \f$ a \f$ and \f$ b \f$, whose
 /// parents are not set; the new root, its parent not set
 ///
 /// In pseudocode (section 10.2 of Morin's book, its recursion made a
 /// loop):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ a = \textbf{nil}\ \textbf{then return}\ b;
 ///     \ \textbf{if}\ b = \textbf{nil}\ \textbf{then return}\ a \cr
 /// 3 & \quad \textbf{if}\ b.x < a.x\ \textbf{then}\ a \leftrightarrow b \cr
 /// 4 & \quad r \gets a \cr
 /// 5 & \quad \textbf{loop} \quad \triangleright\ b \text{ goes below }
 ///     a \text{, not larger than it} \cr
 /// 6 & \qquad c \gets a.\mathit{left} \text{ or } a.\mathit{right}
 ///     \text{, by a coin} \cr
 /// 7 & \qquad \textbf{if}\ c = \textbf{nil}\ \textbf{then}\ c \gets b;
 ///     \ b.\mathit{parent} \gets a;\ \textbf{return}\ r \cr
 /// 8 & \qquad \textbf{if}\ b.x < c.x\ \textbf{then}\ c \gets b;
 ///     \ b.\mathit{parent} \gets a;\ ( a , b ) \gets ( b , \text{the old }
 ///     c ) \cr
 /// 9 & \qquad \textbf{else}\ a \gets c
 /// \end{array}}
 /// \f]
 Node * merge( Node * a , Node * b );

 /// a new node of \f$ x \f$
 Node * make( T && x );

 /// frees the node \f$ u \f$
 void drop( Node * u );

 /// the order of the heap below \f$ u \f$, the parents, and the nodes
 /// counted in `n`
 [[nodiscard]] bool subtree( const Node * u , std::size_t & n ) const;

 /// the order, the parents and the count; \f$ O( n ) \f$ for
 /// \f$ n \le 256 \f$, the root alone beyond
 [[nodiscard]] bool consistent( void ) const;

 Node * f_root = nullptr;                  ///< the root
 std::size_t f_n = 0;                      ///< the number of items
 ADS_NO_UNIQUE_ADDRESS Less f_less;        ///< the order
 SplitMix64 f_g;                           ///< the coins
 std::uint64_t f_bits = 0;                 ///< the coins in hand
 int f_left = 0;                           ///< how many are left
 ADS_NO_UNIQUE_ADDRESS NodeAlloc f_alloc;  ///< the allocator

 };  // end( class MeldableHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/MeldableHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MeldableHeap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MeldableHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
