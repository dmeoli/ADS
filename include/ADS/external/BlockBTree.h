/*--------------------------------------------------------------------------*/
/*---------------------------- File BlockBTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The B-tree of chapter 14 of Morin's book, beyond the book, on a
 * `BlockStore` [see `BlockStore.h`]: a SORTED SET whose nodes are blocks
 * of the external memory, each read and written by one I/O, and whose
 * `lowerBound( x )` (the smallest key \f$ \ge x \f$, the `find( x )` of
 * the SSet of the book), `insert()` and `remove()`
 * take \f$ O( \log_B n ) \f$ I/Os, \f$ O( B \log_B n ) \f$ time with a
 * binary search in each node.
 *
 * A node holds from \f$ B - 1 \f$ to \f$ 2 B - 1 \f$ keys (the root from 0
 * or 1) and an internal node one child more than its keys, all the leaves
 * at the same depth. `insert()` puts the key into its leaf and splits a
 * node of \f$ 2 B \f$ keys into two of \f$ B \f$ and \f$ B - 1 \f$, the
 * middle key going up to the parent, up to the root, which may split and
 * give a new root; `remove()` takes the key out of its leaf, or replaces it
 * in an internal node by its successor taken out of a leaf, and a node
 * left with \f$ B - 2 \f$ keys borrows from its sibling on the left (on
 * the right for the first child), which takes so many keys that both have
 * about half, or is merged with it, when the sibling has \f$ B - 1 \f$
 * keys only. A sequence of \f$ m \f$ operations from the empty tree makes
 * at most \f$ 3 m / 2 \f$ splits, merges and borrows (Lemma 14.1 of Morin's
 * book), \f$ \Theta( m ) \f$ in the worst case, e.g., an insertion and a
 * removal alternating at a full leaf. The two variants of the exercises
 * make them \f$ O( m / B ) \f$, by leaving each node that they touch at
 * \f$ \Omega( B ) \f$ keys from both of its bounds [see `BTreePolicy`]:
 *
 * - `WIDE` (exercise 14.6): from \f$ B \f$ to \f$ 3 B \f$ children, a node
 *   of \f$ 3 B \f$ keys split into two of about \f$ 1.5 B \f$, and a node
 *   in underflow merged with its sibling whenever the two have at most
 *   \f$ 2.5 B \f$ keys, before it is needed, so that the merged node is
 *   \f$ B / 2 \f$ keys below the top, the two otherwise sharing their keys
 *   at about \f$ 1.25 B \f$ or more each;
 * - `THREE` (exercise 14.7): the bounds of the book, a node in overflow
 *   sharing its keys with a sibling, into two nodes if the two have at most
 *   about \f$ 3.75 B \f$ keys and into three otherwise, and a node in
 *   underflow sharing them with two siblings, into two nodes or three in
 *   the same way, so that every node touched has between about
 *   \f$ 7 B / 6 \f$ and \f$ 7 B / 4 \f$ keys.
 *
 * In both, a node touched needs \f$ \Omega( B ) \f$ operations below it
 * before it is touched again, so that a potential of the distance of each
 * node from its bounds pays the restructurings, \f$ O( m / B ) \f$ in all
 * (see \ref theory_external_btree).
 *
 * The internal memory is \f$ O( B + \log_B n ) \f$, the claim of the
 * introduction of chapter 14 that the recursive code of the book does not
 * keep (exercise 14.4), since its recursion holds a node per level,
 * \f$ \Theta( B \log_B n ) \f$: here the descent keeps the path as the
 * indices of its blocks and of the children taken, \f$ O( \log_B n ) \f$
 * words, and at most five nodes counted in memory at once (a node, its
 * parent and the at most three children whose keys a restructuring
 * gathers, the node among them); a node is read again on
 * the way up only when a split, a merge, a borrow or the replacement of a
 * removed key reaches it, so that the I/Os stay \f$ O( \log_B n ) \f$.
 * `peakNodes()` gives the most nodes held at once, and `restructures()`
 * the splits, merges and borrows.
 *
 * Every restructuring is one routine, which gathers the keys of \f$ k \f$
 * consecutive children of a node and the separators between them and
 * deals them out again to \f$ k' \f$ children, the first ones one key more
 * when they do not divide evenly: a split is \f$ 1 \to 2 \f$, a borrow
 * \f$ 2 \to 2 \f$, a merge \f$ 2 \to 1 \f$, and the three-node moves
 * \f$ 2 \to 3 \f$, \f$ 3 \to 3 \f$ and \f$ 3 \to 2 \f$.
 *
 * Against the code of Morin's book: the memory above; a borrow from a
 * sibling of \f$ B \f$ keys, as the text says, where the code of the book
 * merges unless the sibling has more than \f$ B \f$; the keys of any type
 * with an order, where the book marks the empty places by a null key; and
 * the blocks copied by their I/Os [see `BlockStore.h`].
 *
 * The contracts [see `Contract.h`]: \f$ B \ge 2 \f$; `insert()` ensures
 * that the key is there and `remove()` that it is not; the invariant (the
 * order of the keys, their number in each node, the depth of the leaves
 * and `size()`) is checked with no I/O at the level 3 while
 * \f$ n \le 1024 \f$. The operations are defined in `BlockBTree.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, chapter 14 and
 *      exercises 14.1 to 14.7
 * \see R. Bayer, E. M. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see S. Huddleston, K. Mehlhorn, A new data structure for representing
 *      sorted lists, Acta Informatica 17(2), 157-184, 1982,
 *      doi:10.1007/BF00288968
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.4 (the B*-trees, whose overflow
 *      is shared with a sibling)
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

#ifndef __ADS_BlockBTree
 #define __ADS_BlockBTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/BlockStore.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BlockBTree -----------------------------*/
/*--------------------------------------------------------------------------*/

/// how a `BlockBTree` keeps its nodes within their bounds
enum class BTreePolicy {
 STANDARD ,   ///< \f$ B - 1 \f$ to \f$ 2 B - 1 \f$ keys, the book's rules
 WIDE ,       ///< \f$ B - 1 \f$ to \f$ 3 B - 1 \f$ keys (exercise 14.6)
 THREE        ///< the book's bounds, by three nodes (exercise 14.7)
 };

/// a B-tree of keys whose nodes are the blocks of a `BlockStore`
template< std::totally_ordered K >
class BlockBTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;    ///< the type of the keys
 using Item = K;   ///< the items of the sorted set, the keys

 /// the empty tree of the parameter \f$ B \f$ and of the policy `p`, whose
 /// I/Os are also counted by `mem`, if given
 /// \pre \f$ B \ge 2 \f$
 explicit BlockBTree( int B , BTreePolicy p = BTreePolicy::STANDARD ,
                      TwoLevelMemory * mem = nullptr );

/*--------------------------------------------------------------------------*/

 /// the smallest key \f$ \ge x \f$, or nothing, in \f$ O( \log_B n ) \f$
 /// I/Os: a copy, since the blocks of the external memory are copied into
 /// the internal one, not pointed to
 ///
 /// In pseudocode (section 14.2.1 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( x ) \cr
 /// 2 & \quad u \gets \mathit{root};\ z \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \text{read } u;\ i \gets \text{the first place with }
 ///     u.\mathit{keys}[ i ] \ge x \cr
 /// 5 & \qquad \textbf{if}\ u.\mathit{keys}[ i ] = x\ \textbf{then return}
 ///     \ x \cr
 /// 6 & \qquad \textbf{if}\ i < |u.\mathit{keys}|\ \textbf{then}\ z \gets
 ///     u.\mathit{keys}[ i ] \cr
 /// 7 & \qquad \textbf{if}\ u \text{ is a leaf}\ \textbf{then return}\ z \cr
 /// 8 & \qquad u \gets u.\mathit{children}[ i ]
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< K > lowerBound( const K & x ) const;

 /// true if \f$ x \f$ is there, in \f$ O( \log_B n ) \f$ I/Os
 [[nodiscard]] bool contains( const K & x ) const;

 /// adds \f$ x \f$; true if it was not there, in \f$ O( \log_B n ) \f$
 /// I/Os
 ///
 /// In pseudocode (section 14.2.2 of Morin's book, with the path kept
 /// instead of the nodes):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad P \gets \text{the blocks and the children from the root to }
 ///     \text{the leaf } u \text{ of } x \cr
 /// 3 & \quad \textbf{if}\ x \text{ was met}\ \textbf{then return false} \cr
 /// 4 & \quad \text{add } x \text{ to } u;\ \text{write } u \cr
 /// 5 & \quad \textbf{while}\ u \text{ has too many keys}\ \textbf{do} \cr
 /// 6 & \qquad ( p , i ) \gets \mathrm{pop}( P ) \text{, or a new root }
 ///     \text{with the only child } u \cr
 /// 7 & \qquad \text{read } p;\ \text{deal the keys of the child } i
 ///     \text{ (and of its siblings, by the policy) to one node more} \cr
 /// 8 & \qquad \text{write } p;\ u \gets p \cr
 /// 9 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is there
 bool insert( const K & x );

 /// removes \f$ x \f$; true if it was there, in \f$ O( \log_B n ) \f$ I/Os
 ///
 /// In pseudocode (section 14.2.3 of Morin's book, with the path kept
 /// instead of the nodes):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad P \gets \text{the path to } x \text{ and, if } x \text{ is }
 ///     \text{in an internal node } t \text{, on to the leftmost leaf of }
 ///     \text{its right subtree} \cr
 /// 3 & \quad \textbf{if}\ x \text{ was not met}\ \textbf{then return false}
 ///     \cr
 /// 4 & \quad \text{take } x \text{, or the first key } y \text{ of the }
 ///     \text{leaf, out of the leaf } u;\ \text{write } u \cr
 /// 5 & \quad \textbf{while}\ u \text{ has too few keys}\ \textbf{do} \cr
 /// 6 & \qquad ( p , i ) \gets \mathrm{pop}( P );\ \text{read } p
 ///     ;\ \textbf{if}\ p = t\ \textbf{then}\ \text{put } y
 ///     \text{ in place of } x \cr
 /// 7 & \qquad \text{deal the keys of the child } i \text{ and of its }
 ///     \text{siblings to as many nodes or one fewer} \cr
 /// 8 & \qquad \text{write } p;\ u \gets p \cr
 /// 9 & \quad \textbf{if}\ t \text{ was not reached}\ \textbf{then}
 ///     \ \text{read } t \text{, put } y \text{ in place of } x
 ///     \text{, write } t \cr
 /// 10 & \quad \textbf{if}\ \text{the root has no key and a child}
 ///     \ \textbf{then}\ \text{the child becomes the root} \cr
 /// 11 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not there
 bool remove( const K & x );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the parameter \f$ B \f$
 [[nodiscard]] int order( void ) const { return( f_B ); }

 /// the number of levels, 1 for a root that is a leaf
 [[nodiscard]] int height( void ) const;

 /// the number of internal nodes
 [[nodiscard]] std::size_t internalNodes( void ) const;

 /// the reads of blocks so far
 [[nodiscard]] std::uint64_t reads( void ) const { return( f_bs.reads() ); }

 /// the writes of blocks so far
 [[nodiscard]] std::uint64_t writes( void ) const {
  return( f_bs.writes() );
  }

 /// the counters of the I/Os back to 0
 void resetCounters( void ) { f_bs.resetCounters(); }

 /// the number of blocks in use, the nodes
 [[nodiscard]] std::size_t nodes( void ) const { return( f_bs.blocks() ); }

 /// the splits, the merges and the borrows so far
 [[nodiscard]] std::size_t restructures( void ) const {
  return( f_splits + f_merges + f_borrows );
  }

 /// the splits so far
 [[nodiscard]] std::size_t splits( void ) const { return( f_splits ); }

 /// the merges so far
 [[nodiscard]] std::size_t merges( void ) const { return( f_merges ); }

 /// the borrows so far
 [[nodiscard]] std::size_t borrows( void ) const { return( f_borrows ); }

 /// the most nodes held in memory at once by an operation
 [[nodiscard]] std::size_t peakNodes( void ) const { return( f_peak ); }

 /// the longest path kept by an operation, in pairs of indices
 [[nodiscard]] std::size_t peakPath( void ) const { return( f_peakPath ); }

 /// the keys of the nodes level by level, from the root, each level from
 /// left to right, with no I/O
 [[nodiscard]] std::vector< std::vector< std::vector< K > > > levels( void )
  const;

 /// the tree of the levels \f$ L \f$, as `levels()` gives them, in place of
 /// this one (the trees of the figures of the book)
 /// \pre \f$ L \f$ is a B-tree of the bounds of the policy
 void assign( const std::vector< std::vector< std::vector< K > > > & L );

 /// `f( x )` on the keys in increasing order, with no I/O
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, as a block: its keys, then its children (none for a leaf)
 struct Node {
  std::vector< K > keys;               ///< the keys
  std::vector< std::size_t > kids;     ///< the blocks of the children
  bool operator==( const Node & ) const = default;
  };

 using Store = BlockStore< Node >;   ///< the disk of the nodes

 /// a node counted while it is in memory
 struct Held {
  Node node;              ///< the node
  const BlockBTree * t;   ///< the tree that counts it
  /// counts one node more
  explicit Held( const BlockBTree * tree , Node n = Node() );
  /// counts one node less
  ~Held();
  Held( const Held & ) = delete;
  Held & operator=( const Held & ) = delete;
  };

 /// the largest number of keys of a node
 [[nodiscard]] std::size_t maxKeys( void ) const;

 /// the smallest number of keys of a node but the root
 [[nodiscard]] std::size_t minKeys( void ) const {
  return( std::size_t( f_B - 1 ) );
  }

 /// the place of the first key \f$ \ge x \f$ of \f$ u \f$
 [[nodiscard]] static std::size_t place( const Node & u , const K & x );

 /// the keys of the children \f$ f , \ldots , f + k - 1 \f$ of \f$ p \f$
 /// and the separators between them dealt out to \f$ k' \f$ children, the
 /// number that `decide( t )` gives for the \f$ t \f$ keys gathered; the
 /// child \f$ c \f$ is `known`, already in memory
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{deal}( p , f , k ) \cr
 /// 2 & \quad A \gets \text{the keys of the children } f , \ldots ,
 ///     f + k - 1 \text{ with the separators of } p \text{ between them} \cr
 /// 3 & \quad C \gets \text{their children};\ k' \gets
 ///     \mathrm{decide}( |A| ) \cr
 /// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ k' - 1\ \textbf{do} \cr
 /// 5 & \qquad \text{the next } \lfloor ( |A| - k' + 1 ) / k' \rfloor
 ///     \text{ keys of } A \text{, one more for the first ones, to the }
 ///     \text{child } f + j \cr
 /// 6 & \qquad \textbf{if}\ j < k' - 1\ \textbf{then}\ \text{the next key }
 ///     \text{of } A \text{ to } p \text{ as a separator} \cr
 /// 7 & \quad \text{write the } k' \text{ children, freeing or placing the }
 ///     \text{blocks of } | k - k' | \text{ of them}
 /// \end{array}}
 /// \f]
 template< class D >
 void deal( Node & p , std::size_t f , std::size_t k , std::size_t c ,
            const Node & known , D && decide );

 /// the overflow of the child \f$ i \f$ of \f$ p \f$, `u`, fixed by the
 /// policy
 void fixOverflow( Node & p , std::size_t i , const Node & u );

 /// the underflow of the child \f$ i \f$ of \f$ p \f$, `u`, fixed by the
 /// policy
 void fixUnderflow( Node & p , std::size_t i , const Node & u );

 /// the keys of the block \f$ u \f$ and below, at depth \f$ d \f$, within
 /// \f$ ( \mathit{lo} , \mathit{hi} ) \f$; the depth of the leaves in `h`
 [[nodiscard]] bool valid( std::size_t u , const K * lo , const K * hi ,
                           int d , int & h , std::size_t & n ) const;

 /// the order of the keys, their number per node, the depth of the
 /// leaves and the count, with no I/O, for \f$ n \le 1024 \f$
 [[nodiscard]] bool consistent( void ) const;

 /// records the length of a path
 void path( std::size_t len ) const;

 Store f_bs;                            ///< the blocks
 std::size_t f_root;                    ///< the block of the root
 int f_B;                               ///< the parameter \f$ B \f$
 BTreePolicy f_policy;                  ///< the rules of the bounds
 std::size_t f_n = 0;                   ///< the number of keys
 std::size_t f_splits = 0;              ///< see `splits()`
 std::size_t f_merges = 0;              ///< see `merges()`
 std::size_t f_borrows = 0;             ///< see `borrows()`
 mutable std::size_t f_held = 0;        ///< the nodes in memory
 mutable std::size_t f_peak = 0;        ///< see `peakNodes()`
 mutable std::size_t f_peakPath = 0;    ///< see `peakPath()`

 };  // end( class BlockBTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/BlockBTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BlockBTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BlockBTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
