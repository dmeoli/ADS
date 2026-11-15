/*--------------------------------------------------------------------------*/
/*---------------------- File BinaryTreeAlgorithms.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The classic problems on binary trees and on binary search trees, written
 * once for any representation of the nodes: a node is reached by a pointer,
 * nil is `nullptr`, and its links and its key are read either by the member
 * functions `left()`, `right()`, `parent()` and `read()` of `BinaryTree`
 * [see `BinaryTree.h`] or by the fields `left`, `right`, `parent` and `key`
 * of a plain struct, as in the pseudocode of the book. The functions that
 * build a tree return a `BinaryTree`, or take a callable `make( v , l , r )`
 * that makes a node of value \f$ v \f$ over the subtrees \f$ l \f$ and
 * \f$ r \f$, for any other representation.
 *
 * - `lowestCommonAncestor( u , v )` walks up the links to the parents: the
 *   deeper node climbs to the depth of the other, then both climb together
 *   until they meet, \f$ O( h ) \f$ time and \f$ O( 1 ) \f$ space for a tree
 *   of height \f$ h \f$. The usual alternative pushes the two paths on two
 *   stacks and pops them while their tops agree, \f$ O( h ) \f$ space; with
 *   many queries on a fixed tree the reduction of Bender and Farach-Colton
 *   to the range minimum on the Euler tour answers in \f$ O( 1 ) \f$ after
 *   \f$ O( n ) \f$ preprocessing [see `RangeMinimum.h`], as the structure
 *   of Harel and Tarjan did first.
 * - `equalTrees( t , s )` tells whether two trees have the same shape and
 *   equal keys in the same places, by a simultaneous visit, \f$ O( n ) \f$
 *   time and \f$ O( h ) \f$ space; with a comparison always true it compares
 *   the shapes only.
 * - `isSearchTree( t )` checks the order of a binary search tree: each key
 *   must lie in the open interval that its ancestors leave to it, and
 *   comparing a node with its children only is not enough. The visit
 *   carries the two ends down, \f$ O( n ) \f$ time and \f$ O( h ) \f$ space,
 *   the same as checking that the invisit is strictly increasing.
 * - `isRedBlackTree( t , red )` checks the constraints of a Red-Black
 *   coloring (the root black, no red node with a red child, the same number
 *   of black nodes on every path down to a leaf nil) by a postvisit that
 *   returns the black height of each subtree, \f$ O( n ) \f$ time.
 * - `searchTreeRange( t , lo , hi , f )` reports in increasing order the
 *   nodes of a binary search tree whose keys are in
 *   \f$ [ \mathit{lo} , \mathit{hi} ] \f$, a visit pruned where the key
 *   leaves the range: \f$ O( h + k ) \f$ time for \f$ k \f$ keys reported,
 *   since the nodes examined outside the range lie on the two paths that
 *   search for \f$ \mathit{lo} \f$ and for \f$ \mathit{hi} \f$.
 * - `minimumHeightSearchTree( A )` builds the binary search tree of the
 *   sorted keys of \f$ A \f$ of the least height,
 *   \f$ \lceil \log_2( n + 1 ) \rceil - 1 \f$, by putting the middle key at
 *   the root and the two halves below, recursively:
 *   \f$ T( n ) = 2 T( n / 2 ) + O( 1 ) = O( n ) \f$ time and
 *   \f$ O( \log n ) \f$ stack. The same tree comes from the sorted list in
 *   \f$ O( n ) \f$ by building the subtrees in symmetric order, and any tree
 *   is rebalanced in place, in \f$ O( n ) \f$ time and \f$ O( 1 ) \f$ space,
 *   by the algorithm of Day, Stout and Warren.
 * - `fromPreorderInorder( pre , in )` rebuilds the binary tree of distinct
 *   values whose previsit is `pre` and whose invisit is `in`. The first value
 *   of `pre` is the root and splits `in` into the two subtrees; rather than
 *   looking the root up in `in`, the algorithm keeps a stack of the nodes
 *   whose right subtree is still open: the next value of `pre` is the left
 *   child of the top, unless the top is the next value of `in`, in which case
 *   the nodes are popped while they match `in` and the value becomes the
 *   right child of the last one. Each node is pushed and popped once,
 *   \f$ O( n ) \f$ time, with no table of the places and only the equality
 *   of the values.
 * - `cartesianBinaryTree( A )` is the Cartesian tree of Vuillemin as a
 *   `BinaryTree`: its root is the leftmost minimum of \f$ A \f$ and its
 *   subtrees the Cartesian trees of the parts before and after it, i.e., the
 *   binary tree whose invisit is \f$ A \f$ and which is a heap; with
 *   `std::greater` it is the maximal tree, with the maximum at the root. The
 *   parents come from `cartesianTree()` of `RangeMinimum.h`, which builds
 *   them with a stack of the right spine in \f$ O( n ) \f$ time, against the
 *   \f$ O( n^2 ) \f$ of the recursion of the definition on a sorted
 *   \f$ A \f$.
 *
 * - The traversals of chapter 6 of Morin and their exercises:
 *   `nextPreorder()`, `nextInorder()` and `nextPostorder()` give the node
 *   after a given one in each visit by the links to the parents
 *   (exercise 6.8), each node met \f$ O( 1 ) \f$ times over a whole visit;
 *   `subtreeSize()` and `subtreeHeight()` count a subtree by the walk of
 *   section 6.1.2 with no recursion and no stack (exercises 6.4 and 6.5);
 *   `isSizeBalanced()` tests that the two subtrees of each node differ by
 *   one node at most in \f$ O( n ) \f$ (exercise 6.6); `orderNumbers()`
 *   gives the three numbers of each node in one visit (exercise 6.7);
 *   `shapeCode()` and `fromShapeCode()` write and read a shape in
 *   \f$ 2 ( n - 1 ) \f$ bits (exercise 6.11); `lessOrEqual()` reports the
 *   keys not larger than \f$ x \f$ in \f$ O( n' + h ) \f$ (exercise 6.14).
 *   The parents give the traversals of `BinaryTree.h` too, as ranges.
 *
 * The contracts [see `Contract.h`]: `lowestCommonAncestor()` needs two nodes
 * of the same tree and returns a common ancestor none of whose children is
 * one (a postcondition); `searchTreeRange()` needs
 * \f$ \mathit{lo} \le \mathit{hi} \f$; `minimumHeightSearchTree()` needs the
 * keys strictly increasing and returns a tree of the least height (an
 * audit); `fromPreorderInorder()` needs two visits of the same length and
 * checks that the invisit of the tree built is `in`, which fails exactly
 * when no tree of distinct values has those two visits;
 * `cartesianBinaryTree()` returns a tree whose invisit is \f$ A \f$ (an
 * audit). The functions are defined in `BinaryTreeAlgorithms.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.5, 6.1
 *      and 6.2, https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 * \see J. Vuillemin, A unifying look at data structures, Communications of
 *      the ACM 23(4), 229-239, 1980, doi:10.1145/358841.358852
 * \see D. Harel, R. E. Tarjan, Fast algorithms for finding nearest common
 *      ancestors, SIAM Journal on Computing 13(2), 338-355, 1984,
 *      doi:10.1137/0213024
 * \see M. A. Bender, M. Farach-Colton, The LCA problem revisited,
 *      Proceedings of the 4th Latin American Symposium on Theoretical
 *      Informatics (LATIN), LNCS 1776, 88-94, 2000,
 *      doi:10.1007/10719839_9
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 6.1 and exercises
 *      6.4 to 6.14
 * \see Q. F. Stout, B. L. Warren, Tree rebalancing in optimal time and
 *      space, Communications of the ACM 29(9), 902-908, 1986,
 *      doi:10.1145/6592.6599
 * \see L. J. Guibas, R. Sedgewick, A dichromatic framework for balanced
 *      trees, Proceedings of the 19th FOCS, 8-21, 1978,
 *      doi:10.1109/SFCS.1978.3
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

#ifndef __ADS_BinaryTreeAlgorithms
 #define __ADS_BinaryTreeAlgorithms
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/tree/BinaryTree.h"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- THE NODES -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

/// the left child of \f$ n \f$, by `left()` or by the field `left`
template< class N >
auto nodeLeft( N * n )
{
 if constexpr( requires { n->left(); } )
  return( n->left() );
 else
  return( n->left );
 }

/// the right child of \f$ n \f$, by `right()` or by the field `right`
template< class N >
auto nodeRight( N * n )
{
 if constexpr( requires { n->right(); } )
  return( n->right() );
 else
  return( n->right );
 }

/// the parent of \f$ n \f$, by `parent()` or by the field `parent`
template< class N >
auto nodeParent( N * n )
{
 if constexpr( requires { n->parent(); } )
  return( n->parent() );
 else
  return( n->parent );
 }

/// the key of \f$ n \f$, by `read()` or by the field `key`
template< class N >
decltype( auto ) nodeKey( N * n )
{
 if constexpr( requires { n->read(); } )
  return( n->read() );
 else
  return( ( n->key ) );
 }

}  // end( namespace detail )

/// a node of a binary tree: a pointer to a struct whose children are read
/// by `left()` and `right()` or by the fields `left` and `right`
template< class N >
concept BinaryNode = std::is_pointer_v< N > && requires( N n )
{
 detail::nodeLeft( n );
 detail::nodeRight( n );
 };

/// a node of a binary tree with the link to its parent
template< class N >
concept ParentBinaryNode =
  BinaryNode< N > && requires( N n )
{ detail::nodeParent( n ); };

/// a node of a binary tree with a key
template< class N >
concept KeyedBinaryNode =
  BinaryNode< N > && requires( N n )
{ detail::nodeKey( n ); };

/*--------------------------------------------------------------------------*/
/*---------------------------- THE ANCESTORS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the deepest node that is an ancestor of both \f$ u \f$ and \f$ v \f$,
/// each node being an ancestor of itself; \f$ O( h ) \f$ time,
/// \f$ O( 1 ) \f$ space
///
/// In pseudocode (section 5.5, beyond the book), \f$ d \f$ the depth of a
/// node:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lowestCommonAncestor}( u , v ) \cr
/// 2 & \quad \textbf{while}\ d( u ) > d( v )\ \textbf{do}\ u \gets
///     u.\mathit{parent} \cr
/// 3 & \quad \textbf{while}\ d( v ) > d( u )\ \textbf{do}\ v \gets
///     v.\mathit{parent} \cr
/// 4 & \quad \textbf{while}\ u \ne v\ \textbf{do}\ u \gets
///     u.\mathit{parent};\ v \gets v.\mathit{parent} \cr
/// 5 & \quad \textbf{return}\ u
/// \end{array}}
/// \f]
/// \pre \f$ u \f$ and \f$ v \f$ are nodes of the same tree
/// \post an ancestor of \f$ u \f$ and of \f$ v \f$, none of whose
///       children is one
template< ParentBinaryNode N >
N lowestCommonAncestor( N u , N v );

/*--------------------------------------------------------------------------*/
/*--------------------------- THE COMPARISONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether \f$ t \f$ and \f$ s \f$ have the same shape and `eq( a , b )`
/// holds on the keys \f$ a \f$ and \f$ b \f$ of each two nodes in the same
/// place; \f$ O( n ) \f$ time, \f$ O( h ) \f$ space
///
/// In pseudocode (section 5.6), a simultaneous previsit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{equalTrees}( t , s ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{or}\ s = \textbf{nil}
///     \ \textbf{then return}\ ( t = \textbf{nil}\ \textbf{and}\ s =
///     \textbf{nil} ) \cr
/// 3 & \quad \textbf{if}\ \textbf{not}\ \mathrm{eq}( t.\mathit{key} ,
///     s.\mathit{key} )\ \textbf{then return false} \cr
/// 4 & \quad \textbf{return}\ \mathrm{equalTrees}( t.\mathit{left} ,
///     s.\mathit{left} )\ \textbf{and}\ \mathrm{equalTrees}(
///     t.\mathit{right} , s.\mathit{right} )
/// \end{array}}
/// \f]
template< KeyedBinaryNode N , KeyedBinaryNode M ,
	  class Equal = std::equal_to<> >
[[nodiscard]] bool equalTrees( N t , M s , Equal eq = {} );

/// whether the keys of \f$ t \f$ are strictly increasing in the invisit,
/// by `less`; \f$ O( n ) \f$ time, \f$ O( h ) \f$ space
///
/// In pseudocode (section 6.1), each key within the open interval
/// \f$ ( a , b ) \f$ that its ancestors leave to it,
/// \f$ a = -\infty \f$ and \f$ b = +\infty \f$ at the root:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{between}( t , a , b )\quad \triangleright
///     \ \text{every key of } t \text{ strictly inside } ( a , b ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return true} \cr
/// 3 & \quad \textbf{return}\ a < t.\mathit{key} < b\ \textbf{and}
///     \ \mathrm{between}( t.\mathit{left} , a , t.\mathit{key} )
///     \ \textbf{and}\ \mathrm{between}( t.\mathit{right} , t.\mathit{key} ,
///     b )
/// \end{array}}
/// \f]
template< KeyedBinaryNode N , class Compare = std::less<> >
[[nodiscard]] bool isSearchTree( N t , Compare less = {} );

/// whether the coloring of \f$ t \f$, `red( u )` telling whether the node
/// \f$ u \f$ is red, respects the constraints of a Red-Black tree: the root
/// black, no red node with a red child, the same number of black nodes on
/// every path from the root down to a leaf nil; \f$ O( n ) \f$ time,
/// \f$ O( h ) \f$ space
///
/// In pseudocode (section 6.2), a postvisit giving the black height of
/// each subtree, the leaf nil counted, or \f$ -1 \f$ if it breaks a
/// constraint:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bh}( t )\quad \triangleright
///     \ \text{the black height counting the leaves nil, or } -1 \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}\ 1 \cr
/// 3 & \quad l \gets \mathrm{bh}( t.\mathit{left} );\ r \gets \mathrm{bh}(
///     t.\mathit{right} ) \cr
/// 4 & \quad \textbf{if}\ l < 0\ \textbf{or}\ r \ne l\ \textbf{or}\ ( t
///     \text{ is red and has a red child} )\ \textbf{then return}\ -1 \cr
/// 5 & \quad \textbf{if}\ t \text{ is red}\ \textbf{then return}\ l
///     \ \textbf{else return}\ l + 1 \cr
/// 6 & \textbf{function}\ \mathrm{isRedBlackTree}( t ) \cr
/// 7 & \quad \textbf{return}\ \mathrm{bh}( t ) > 0\ \textbf{and}\ ( t =
///     \textbf{nil}\ \textbf{or}\ t \text{ is black} )
/// \end{array}}
/// \f]
template< BinaryNode N , class Red >
[[nodiscard]] bool isRedBlackTree( N t , Red red );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE SEARCHES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// `f( u )` on the nodes \f$ u \f$ of the binary search tree \f$ t \f$ whose
/// keys are in \f$ [ \mathit{lo} , \mathit{hi} ] \f$, in increasing order of
/// the keys; the number of the nodes examined, at most
/// \f$ k + 2 ( h + 1 ) \f$ for \f$ k \f$ keys in the range;
/// \f$ O( h + k ) \f$ time
///
/// In pseudocode (section 6.1), an invisit pruned out of the range:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{range}( t , \mathit{lo} , \mathit{hi}
///     )\quad \triangleright\ f \text{ on the keys in } [ \mathit{lo} ,
///     \mathit{hi} ] \text{, in order} \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
/// 3 & \quad \textbf{if}\ \mathit{lo} \le t.\mathit{key}\ \textbf{then}
///     \ \mathrm{range}( t.\mathit{left} , \mathit{lo} , \mathit{hi} )\quad
///     \triangleright\ \text{else no key on the left is in range} \cr
/// 4 & \quad \textbf{if}\ \mathit{lo} \le t.\mathit{key} \le \mathit{hi}
///     \ \textbf{then}\ f( t ) \cr
/// 5 & \quad \textbf{if}\ t.\mathit{key} \le \mathit{hi}\ \textbf{then}
///     \ \mathrm{range}( t.\mathit{right} , \mathit{lo} , \mathit{hi} )
/// \end{array}}
/// \f]
/// \pre not `less( hi , lo )`
template< KeyedBinaryNode N , class K , class F , class Compare = std::less<> >
std::size_t searchTreeRange( N t , const K & lo , const K & hi , F && f ,
			     Compare less = {} );

/*--------------------------------------------------------------------------*/
/*--------------------- THE TRAVERSALS WITH NO STACK -----------------------*/
/*--------------------------------------------------------------------------*/

/// the node after \f$ u \f$ in the previsit of its tree, nil after the
/// last; \f$ O( 1 ) \f$ amortized over a whole visit (exercise 6.8 of Morin)
///
/// In pseudocode (exercise 6.8 of Morin, beyond the book), by the links to
/// the parents:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nextPreorder}( u ) \cr
/// 2 & \quad \textbf{if}\ u.\mathit{left} \ne \textbf{nil}\ \textbf{then
///     return}\ u.\mathit{left} \cr
/// 3 & \quad \textbf{if}\ u.\mathit{right} \ne \textbf{nil}\ \textbf{then
///     return}\ u.\mathit{right} \cr
/// 4 & \quad \textbf{while}\ u.\mathit{parent} \ne \textbf{nil}\ \textbf{do}
///     \cr
/// 5 & \qquad p \gets u.\mathit{parent} \cr
/// 6 & \qquad \textbf{if}\ u = p.\mathit{left}\ \textbf{and}
///     \ p.\mathit{right} \ne \textbf{nil}\ \textbf{then return}
///     \ p.\mathit{right} \cr
/// 7 & \qquad u \gets p \cr
/// 8 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
template< ParentBinaryNode N >
[[nodiscard]] N nextPreorder( N u );

/// the node after \f$ u \f$ in the invisit of its tree, nil after the last;
/// \f$ O( 1 ) \f$ amortized over a whole visit (exercise 6.8 of Morin)
///
/// In pseudocode (exercise 6.8 of Morin, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nextInorder}( u ) \cr
/// 2 & \quad \textbf{if}\ u.\mathit{right} \ne \textbf{nil}\ \textbf{then
///     return}\ \text{the leftmost node below } u.\mathit{right} \cr
/// 3 & \quad \textbf{while}\ u.\mathit{parent} \ne \textbf{nil}\ \textbf{and}
///     \ u = u.\mathit{parent}.\mathit{right}\ \textbf{do}\ u \gets
///     u.\mathit{parent} \cr
/// 4 & \quad \textbf{return}\ u.\mathit{parent}
/// \end{array}}
/// \f]
template< ParentBinaryNode N >
[[nodiscard]] N nextInorder( N u );

/// the node after \f$ u \f$ in the postvisit of its tree, nil after the
/// root; \f$ O( 1 ) \f$ amortized over a whole visit (exercise 6.8 of Morin)
///
/// In pseudocode (exercise 6.8 of Morin, beyond the book), the first node
/// of the postvisit of a subtree reached by going left when possible and
/// right otherwise:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nextPostorder}( u ) \cr
/// 2 & \quad p \gets u.\mathit{parent} \cr
/// 3 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{or}\ u =
///     p.\mathit{right}\ \textbf{or}\ p.\mathit{right} = \textbf{nil}
///     \ \textbf{then return}\ p \cr
/// 4 & \quad v \gets p.\mathit{right} \cr
/// 5 & \quad \textbf{while}\ v \text{ has a child}\ \textbf{do}\ v \gets
///     \text{its left child if any, its right one otherwise} \cr
/// 6 & \quad \textbf{return}\ v
/// \end{array}}
/// \f]
template< ParentBinaryNode N >
[[nodiscard]] N nextPostorder( N u );

/// the first node of the invisit of the subtree of \f$ u \f$, its leftmost
/// node, from which `nextInorder()` starts; \f$ O( h ) \f$ time
/// \pre \f$ u \f$ is not nil
template< BinaryNode N >
[[nodiscard]] N firstInorder( N u );

/// the first node of the postvisit of the subtree of \f$ u \f$, reached by
/// going left when possible and right otherwise, from which
/// `nextPostorder()` starts; \f$ O( h ) \f$ time (the first node of the
/// previsit is \f$ u \f$ itself)
/// \pre \f$ u \f$ is not nil
template< BinaryNode N >
[[nodiscard]] N firstPostorder( N u );

/// the number of nodes of the subtree of \f$ u \f$, by a walk along the
/// links to the parents that never leaves the subtree, with no recursion
/// and no stack: \f$ O( n_u ) \f$ time and \f$ O( 1 ) \f$ space (the
/// `size2(u)` of exercise 6.4 of Morin)
///
/// In pseudocode (section 6.1.2 and exercise 6.4 of Morin), \f$ p \f$ the
/// node the walk comes from, the walk entering a node from its parent,
/// from its left child or from its right child:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{size2}( u ) \cr
/// 2 & \quad r \gets u;\ p \gets r.\mathit{parent};\ s \gets 0 \cr
/// 3 & \quad \textbf{while}\ u \ne r.\mathit{parent}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ p = u.\mathit{parent}\ \textbf{then}\ s \gets s
///     + 1;\ \textit{next} \gets u.\mathit{left}, \text{ or }
///     u.\mathit{right}, \text{ or } u.\mathit{parent} \cr
/// 5 & \qquad \textbf{else if}\ p = u.\mathit{left}\ \textbf{then}
///     \ \mathit{next} \gets u.\mathit{right}, \text{ or }
///     u.\mathit{parent} \cr
/// 6 & \qquad \textbf{else}\ \mathit{next} \gets u.\mathit{parent} \cr
/// 7 & \qquad p \gets u;\ u \gets \mathit{next} \cr
/// 8 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// where "or" takes the first that is not nil
template< ParentBinaryNode N >
[[nodiscard]] std::size_t subtreeSize( N u );

/// the height of the subtree of \f$ u \f$, \f$ -1 \f$ for nil, by the walk
/// of `subtreeSize()` that keeps the depth below \f$ u \f$:
/// \f$ O( n_u ) \f$ time and \f$ O( 1 ) \f$ space (the `height2(u)` of
/// exercise 6.5 of Morin)
template< ParentBinaryNode N >
[[nodiscard]] long subtreeHeight( N u );

/// whether every node \f$ u \f$ of \f$ t \f$ has subtrees whose sizes differ
/// by 1 at most (exercise 6.6 of Morin), by a postvisit that returns the
/// size of each subtree, or stops at the first that is not balanced:
/// \f$ O( n ) \f$ time, against the \f$ \Theta( n \log n ) \f$ to
/// \f$ \Theta( n^2 ) \f$ of a test that computes each size again
///
/// In pseudocode (exercise 6.6 of Morin), with a stack of its own in the
/// code since the height may be \f$ n \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sizeIfBalanced}( t ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}\ 0 \cr
/// 3 & \quad a \gets \mathrm{sizeIfBalanced}( t.\mathit{left} );\ b \gets
///     \mathrm{sizeIfBalanced}( t.\mathit{right} ) \cr
/// 4 & \quad \textbf{if}\ a = \bot\ \textbf{or}\ b = \bot\ \textbf{or}
///     \ |a - b| > 1\ \textbf{then return}\ \bot \cr
/// 5 & \quad \textbf{return}\ a + b + 1
/// \end{array}}
/// \f]
template< BinaryNode N >
[[nodiscard]] bool isSizeBalanced( N t );

/// `f( u , pre , in , post )` on each node \f$ u \f$ of \f$ t \f$ with its
/// numbers in the previsit, in the invisit and in the postvisit, from 0, in
/// one visit: \f$ O( n ) \f$ time, \f$ O( h ) \f$ space (exercise 6.7 of
/// Morin). By them \f$ u \f$ is an ancestor of \f$ w \f$ if and only if
/// \f$ \mathrm{pre}( u ) \le \mathrm{pre}( w ) \f$ and
/// \f$ \mathrm{post}( w ) \le \mathrm{post}( u ) \f$, and
/// \f$ \mathrm{post}( u ) - \mathrm{pre}( u ) = n_u - 1 - d( u ) \f$
/// (exercise 6.9, see \ref theory_tree_morin)
///
/// In pseudocode (exercise 6.7 of Morin), three counters:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{number}( t ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
/// 3 & \quad \mathrm{pre}( t ) \gets a;\ a \gets a + 1 \cr
/// 4 & \quad \mathrm{number}( t.\mathit{left} );\ \mathrm{in}( t ) \gets
///     b;\ b \gets b + 1 \cr
/// 5 & \quad \mathrm{number}( t.\mathit{right} );\ \mathrm{post}( t )
///     \gets c;\ c \gets c + 1
/// \end{array}}
/// \f]
template< BinaryNode N , class F >
void orderNumbers( N t , F && f );

/// the shape of \f$ t \f$ in \f$ 2 ( n - 1 ) \f$ bits, none for
/// \f$ n \le 1 \f$ (exercise 6.11 of Morin): for each node in the previsit
/// but the last, which is a leaf, whether it has a left and whether it has
/// a right child; \f$ O( n ) \f$ time. The \f$ C_n \f$ shapes of
/// \f$ n \f$ nodes need \f$ \log_2 C_n = 2 n - O( \log n ) \f$ bits, so the
/// code is within \f$ O( \log n ) \f$ bits of the least
template< BinaryNode N >
[[nodiscard]] std::vector< bool > shapeCode( N t );

/// the tree of the shape of `shapeCode()` above, its values the numbers of
/// the previsit; the caller owns it, nil if \f$ n = 0 \f$; \f$ O( n ) \f$
///
/// In pseudocode (exercise 6.11 of Morin), the previsit read back, the
/// nodes whose right child is still due on a stack:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fromShapeCode}( c , n ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad u \gets \text{a node } i;\ \text{link it as the child due}
///     \text{ (the left of the last node, or the right of the top)} \cr
/// 4 & \qquad ( \ell , r ) \gets \text{the next two bits of } c \text{, }
///     ( 0 , 0 ) \text{ for the last node} \cr
/// 5 & \qquad \textbf{if}\ r\ \textbf{then}\ \text{push } u \cr
/// 6 & \qquad \textbf{if not}\ \ell\ \textbf{then}\ \text{the next is the
///     right child of a popped node}
/// \end{array}}
/// \f]
/// \pre \f$ c \f$ has \f$ 2 ( n - 1 ) \f$ bits, the code of a tree
inline BinaryTree< std::size_t > *
fromShapeCode( const std::vector< bool > & c , std::size_t n );

/// `f( u )` on the nodes \f$ u \f$ of the binary search tree \f$ t \f$ whose
/// keys are not larger than \f$ x \f$, in increasing order, and their
/// number \f$ n' \f$; \f$ O( n' + h ) \f$ time (the `getLE(x)` of exercise
/// 6.14 of Morin)
///
/// In pseudocode (exercise 6.14 of Morin), down the path of \f$ x \f$,
/// every left subtree hanging from a node not larger than \f$ x \f$ visited
/// whole:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{getLE}( t , x ) \cr
/// 2 & \quad \textbf{while}\ t \ne \textbf{nil}\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ t.\mathit{key} \le x\ \textbf{then}
///     \ \mathrm{invisit}( t.\mathit{left} , f );\ f( t );\ t \gets
///     t.\mathit{right} \cr
/// 4 & \qquad \textbf{else}\ t \gets t.\mathit{left}
/// \end{array}}
/// \f]
template< KeyedBinaryNode N , class K , class F , class Compare = std::less<> >
std::size_t lessOrEqual( N t , const K & x , F && f , Compare less = {} );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE BUILDERS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the binary search tree of the keys of \f$ A \f$ of the least height, the
/// lower middle key at the root of each subtree, whose nodes are made by
/// `make( v , l , r )` over the subtrees \f$ l \f$ and \f$ r \f$, nil if
/// \f$ A \f$ is empty; \f$ O( n ) \f$ time
///
/// In pseudocode (section 6.1), on \f$ A[ i .. j ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{build}( A , i , j )\quad \triangleright
///     \ \text{a tree of the sorted } A[ i .. j - 1 ] \cr
/// 2 & \quad \textbf{if}\ j \le i\ \textbf{then return nil} \cr
/// 3 & \quad m \gets i + \lfloor ( j - i - 1 ) / 2 \rfloor\quad
///     \triangleright\ \text{the median, the lower one if two} \cr
/// 4 & \quad \textbf{return}\ \mathrm{make}( A[ m ] , \mathrm{build}( A , i
///     , m ) , \mathrm{build}( A , m + 1 , j ) )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ strictly increasing by `less`
/// \post the height is \f$ \lceil \log_2( n + 1 ) \rceil - 1 \f$ (an
///       audit)
template< class T , class Make , class Compare = std::less<> >
auto minimumHeightSearchTree( const std::vector< T > & A , Make make ,
			      Compare less = {} )
  -> std::invoke_result_t< Make & , const T & , std::nullptr_t ,
			   std::nullptr_t >;

/// the tree of `minimumHeightSearchTree()` above as a `BinaryTree`, which the
/// caller owns
/// \pre \f$ A \f$ strictly increasing by `less`
template< class T , class Compare = std::less<> >
 requires std::predicate< Compare & , const T & , const T & >
BinaryTree< T > * minimumHeightSearchTree( const std::vector< T > & A ,
					   Compare less = {} );

/// the binary tree whose previsit is `pre` and whose invisit is `in`, nil
/// if they are empty; \f$ O( n ) \f$ time; the caller owns it
///
/// In pseudocode (section 5.6, beyond the book for the stack),
/// \f$ S \f$ the nodes whose right subtree is still open and \f$ j \f$ the
/// next place of `in`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fromPreorderInorder}( P , I ) \cr
/// 2 & \quad r \gets \mathrm{BinaryTree}( P[ 0 ] );\ S \gets ( r );\ j
///     \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad t \gets \mathrm{BinaryTree}( P[ i ] );\ \ell \gets
///     \textbf{nil} \cr
/// 5 & \qquad \textbf{while}\ S \ne \emptyset\ \textbf{and}
///     \ \mathrm{top}( S ).\mathit{key} = I[ j ]\ \textbf{do}\ \ell \gets
///     \mathrm{pop}( S );\ j \gets j + 1 \cr
/// 6 & \qquad \textbf{if}\ \ell \ne \textbf{nil}\ \textbf{then}
///     \ \ell.\mathrm{insertRight}( t )\ \textbf{else}\ \mathrm{top}( S
///     ).\mathrm{insertLeft}( t ) \cr
/// 7 & \qquad \mathrm{push}( S , t ) \cr
/// 8 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre `pre` and `in` are the visits of a binary tree of distinct values
template< class T >
BinaryTree< T > * fromPreorderInorder( const std::vector< T > & pre ,
				       const std::vector< T > & in );

/// the Cartesian tree of \f$ A \f$, the leftmost minimum by `less` at the
/// root, a heap whose invisit is \f$ A \f$, nil if \f$ A \f$ is empty;
/// \f$ O( n ) \f$ time; the caller owns it
///
/// In pseudocode (Vuillemin, beyond the book), \f$ p \f$ the parents that
/// `cartesianTree()` gives by the stack of the right spine:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cartesianBinaryTree}( A ) \cr
/// 2 & \quad p \gets \mathrm{cartesianTree}( A );\ N[ i ] \gets
///     \mathrm{BinaryTree}( A[ i ] ) \text{ for each } i \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ i \text{ is the root}\ \textbf{then}\ r \gets
///     N[ i ] \cr
/// 5 & \qquad \textbf{else if}\ i < p[ i ]\ \textbf{then}\ N[ p[ i ]
///     ].\mathrm{insertLeft}( N[ i ] ) \cr
/// 6 & \qquad \textbf{else}\ N[ p[ i ] ].\mathrm{insertRight}( N[ i ] ) \cr
/// 7 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \post the invisit is A (an audit)
template< class T , class Compare = std::less< T > >
BinaryTree< T > * cartesianBinaryTree( const std::vector< T > & A ,
				       Compare less = {} );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/BinaryTreeAlgorithms.tpp"

/*--------------------------------------------------------------------------*/

#endif /* BinaryTreeAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File BinaryTreeAlgorithms.h ---------------------*/
/*--------------------------------------------------------------------------*/
