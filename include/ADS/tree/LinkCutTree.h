/*--------------------------------------------------------------------------*/
/*--------------------------- File LinkCutTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The link-cut trees of Sleator and Tarjan, beyond the book: a forest of
 * rooted trees on the nodes \f$ 0 , \ldots , n - 1 \f$ that changes by
 * `link()` and `cut()` of an edge, and answers `connected()`,
 * `findRoot()`, the lowest common ancestor `lca()` and the aggregate of
 * the values of the nodes on the path between two nodes, `aggregate()`,
 * each in \f$ O( \log n ) \f$ amortized time (see
 * \ref theory_tree_dynamic). The forest is split into <em>preferred
 * paths</em>, each kept by a splay tree [see `SplayTree.h`] in the order of
 * the depths, whose root points to the parent of the top of the path; the
 * operation `access( v )` makes the path from the root of its tree to
 * \f$ v \f$ preferred, and every other operation is a few calls of it.
 * `evert( v )` makes \f$ v \f$ the root of its tree by reversing that path,
 * with a flag pushed down lazily, so that the trees are unrooted for
 * `link()`, `cut()` and `aggregate()`.
 *
 * The values are of the type `T`, combined by the operation `Op`, which
 * must be associative and commutative with the identity given to the
 * constructor (e.g., the maximum, the minimum or the sum): commutative,
 * since an evert reverses the order of the nodes of a path. The weights of
 * the edges are kept by a node in the middle of each edge, as the dynamic
 * minimum spanning forest of `SpanningForestObserver` does [see
 * `Reoptimization.h`]. The nodes are kept in vectors indexed by their
 * numbers, with no allocation per operation.
 *
 * The libraries of graphs do not have the dynamic trees (neither the Boost
 * Graph Library nor LEMON), and those of competitive programming have them
 * for fixed numbers of nodes; the batch-parallel ones cited by
 * `JoinAlgorithms.h` answer batches of links and cuts instead.
 *
 * The contracts [see `Contract.h`]: the nodes are in \f$ [ 0 , n ) \f$;
 * `link()` joins two nodes of two distinct trees, `cut()` removes an edge
 * that is there, `aggregate()` and `lca()` need two nodes of the same tree;
 * at the level of the audits each operation checks the parents, the
 * children and the aggregates of the splay trees it touched. The
 * operations are defined in `LinkCutTree.tpp`.
 *
 * \see D. D. Sleator, R. E. Tarjan, A data structure for dynamic trees,
 *      Journal of Computer and System Sciences 26(3), 362-391, 1983,
 *      doi:10.1016/0022-0000(83)90006-5
 * \see D. D. Sleator, R. E. Tarjan, Self-adjusting binary search trees,
 *      Journal of the ACM 32(3), 652-686, 1985, doi:10.1145/3828.3835
 * \see R. E. Tarjan, R. F. Werneck, Dynamic trees in practice, ACM Journal
 *      of Experimental Algorithmics 14, article 4.5, 2009,
 *      doi:10.1145/1498698.1594231
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

#ifndef __ADS_LinkCutTree
 #define __ADS_LinkCutTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS LinkCutTree -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a forest of dynamic trees whose nodes have values of the type `T`,
/// aggregated on the paths by the commutative monoid `Op`
template< class T = long long , class Op = std::plus< T > >
class LinkCutTree {

/*--------------------------------------------------------------------------*/
 public:

 using Value = T;   ///< the type of the values

 /// the forest of \f$ n \f$ nodes with no edge, each of value
 /// \f$ \mathit{id} \f$, the identity of `op`
 /// \pre \f$ n \ge 0 \f$
 explicit LinkCutTree( int n = 0 , const T & id = T() , Op op = Op() );

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( int( f_par.size() ) ); }

 /// a new node of value \f$ x \f$, alone, and its number
 int addNode( const T & x );

 /// the value of \f$ v \f$
 /// \pre \f$ v \f$ is a node
 [[nodiscard]] const T & value( int v ) const;

 /// the value of \f$ v \f$ set to \f$ x \f$, in \f$ O( \log n ) \f$
 /// amortized time
 /// \pre \f$ v \f$ is a node
 void setValue( int v , const T & x );

 /// \f$ v \f$ made the root of its tree, in \f$ O( \log n ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book), on `access()`
 /// below:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{evert}( v ) \cr
 /// 2 & \quad \mathrm{access}( v );\ \mathit{rev}[ v ] \gets \textbf{not}
 ///     \ \mathit{rev}[ v ] \quad \triangleright\ \text{the path from the}
 ///     \text{ root to } v \text{ reversed}
 /// \end{array}}
 /// \f]
 /// \pre \f$ v \f$ is a node
 void evert( int v );

 /// the root of the tree of \f$ v \f$, in \f$ O( \log n ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findRoot}( v ) \cr
 /// 2 & \quad \mathrm{access}( v );\ r \gets v \cr
 /// 3 & \quad \textbf{while}\ \mathit{left}[ r ] \ne \textbf{nil}
 ///     \ \textbf{do}\ r \gets \mathit{left}[ r ] \quad \triangleright
 ///     \ \text{the shallowest node of the path} \cr
 /// 4 & \quad \mathrm{splay}( r );\ \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre \f$ v \f$ is a node
 [[nodiscard]] int findRoot( int v );

 /// whether \f$ u \f$ and \f$ v \f$ are in the same tree, in
 /// \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (beyond the book): \f$ \mathrm{findRoot}( u ) =
 /// \mathrm{findRoot}( v ) \f$.
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes
 [[nodiscard]] bool connected( int u , int v );

 /// the edge \f$ [ u , v ] \f$, which joins two trees, \f$ u \f$ made a
 /// child of \f$ v \f$, in \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{link}( u , v ) \cr
 /// 2 & \quad \mathrm{evert}( u );\ \mathit{par}[ u ] \gets v \quad
 ///     \triangleright\ \text{a path-parent pointer}
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes of two distinct trees
 void link( int u , int v );

 /// the edge \f$ [ u , v ] \f$ removed, in \f$ O( \log n ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{cut}( u , v ) \cr
 /// 2 & \quad \mathrm{evert}( u );\ \mathrm{access}( v ) \quad
 ///     \triangleright\ \text{the path is } u , v \text{ alone} \cr
 /// 3 & \quad \mathit{left}[ v ] \gets \textbf{nil};\ \mathit{par}[ u ]
 ///     \gets \textbf{nil}
 /// \end{array}}
 /// \f]
 /// \pre \f$ [ u , v ] \f$ is an edge of the forest
 void cut( int u , int v );

 /// the edge from \f$ v \f$ to its parent removed, in the trees as rooted
 /// now, \f$ v \f$ the root of its new tree; false if \f$ v \f$ is a root
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{cutParent}( v ) \cr
 /// 2 & \quad \mathrm{access}( v ) \cr
 /// 3 & \quad \textbf{if}\ \mathit{left}[ v ] = \textbf{nil}\ \textbf{then}
 ///     \ \textbf{return}\ \textbf{false} \cr
 /// 4 & \quad \mathit{par}[ \mathit{left}[ v ] ] \gets \textbf{nil};
 ///     \ \mathit{left}[ v ] \gets \textbf{nil};\ \textbf{return}
 ///     \ \textbf{true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ v \f$ is a node
 bool cutParent( int v );

 /// the lowest common ancestor of \f$ u \f$ and \f$ v \f$ in the trees as
 /// rooted now, in \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book): the last node at
 /// which `access( v )` enters the preferred path that `access( u )` left:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lca}( u , v ) \cr
 /// 2 & \quad \mathrm{access}( u );\ \textbf{return}\ \mathrm{access}( v )
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes of the same tree
 [[nodiscard]] int lca( int u , int v );

 /// the aggregate by `Op` of the values of the nodes on the path between
 /// \f$ u \f$ and \f$ v \f$, ends included, in \f$ O( \log n ) \f$
 /// amortized time; the root of the tree of \f$ u \f$ becomes \f$ u \f$
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{aggregate}( u , v ) \cr
 /// 2 & \quad \mathrm{evert}( u );\ \mathrm{access}( v );\ \textbf{return}
 ///     \ \mathit{agg}[ v ] \quad \triangleright\ \text{the splay tree of}
 ///     \text{ the path}
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes of the same tree
 [[nodiscard]] T aggregate( int u , int v );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int NONE = -1;     ///< no node

 /// whether \f$ x \f$ is the root of its splay tree
 [[nodiscard]] bool isRoot( int x ) const;

 /// the aggregate of \f$ x \f$ from its value and those of its children
 void pull( int x );

 /// the reversal of \f$ x \f$, if any, given to its children
 void push( int x );

 /// \f$ x \f$ one level up in its splay tree
 void rotate( int x );

 /// \f$ x \f$ made the root of its splay tree by rotations in pairs
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book), with
 /// \f$ p \f$ the parent and \f$ g \f$ the grandparent in the splay tree:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{splay}( x ) \cr
 /// 2 & \quad \text{push the reversals down the path from the root of the}
 ///     \text{ splay tree to } x \cr
 /// 3 & \quad \textbf{while}\ x \text{ is not the root of its splay tree}
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ p \text{ is not the root} \textbf{ and } x
 ///     , p \text{ are children on the same side}\ \textbf{then}
 ///     \ \mathrm{rotate}( p ) \quad \triangleright\ \text{zig-zig} \cr
 /// 5 & \qquad \textbf{else if}\ p \text{ is not the root}\ \textbf{then}
 ///     \ \mathrm{rotate}( x ) \quad \triangleright\ \text{zig-zag} \cr
 /// 6 & \qquad \mathrm{rotate}( x )
 /// \end{array}}
 /// \f]
 void splay( int x );

 /// the path from the root of the tree of \f$ v \f$ to \f$ v \f$ made
 /// preferred, \f$ v \f$ the root of its splay tree with no right child;
 /// the last node at which the climb entered a new path
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( v ) \cr
 /// 2 & \quad \mathit{last} \gets \textbf{nil};\ x \gets v \cr
 /// 3 & \quad \textbf{while}\ x \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \mathrm{splay}( x );\ \mathit{right}[ x ] \gets
 ///     \mathit{last};\ \mathrm{pull}( x ) \quad \triangleright
 ///     \ \text{the path below } x \text{ replaced} \cr
 /// 5 & \qquad \mathit{last} \gets x;\ x \gets \mathit{par}[ x ] \cr
 /// 6 & \quad \mathrm{splay}( v );\ \textbf{return}\ \mathit{last}
 /// \end{array}}
 /// \f]
 int access( int v );

 /// whether the splay trees are sound: the parents of the children, and
 /// the aggregates, up to the reversals still to push (audit)
 [[nodiscard]] bool wellFormed( void ) const;

 std::vector< int > f_par;           ///< the parent in the splay tree, or
                                     ///< the path-parent of its root
 std::vector< int > f_left;          ///< the left child (shallower)
 std::vector< int > f_right;         ///< the right child (deeper)
 std::vector< char > f_rev;          ///< whether the subtree is reversed
 std::vector< T > f_val;             ///< the values
 std::vector< T > f_agg;             ///< the aggregates of the subtrees
 std::vector< int > f_stack;         ///< the path of a splay
 T f_id;                             ///< the identity of `Op`
 Op f_op;                            ///< the operation

 };  // end( class LinkCutTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/LinkCutTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinkCutTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkCutTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
