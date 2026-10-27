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

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- THE NODES -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// the left child of \f$ n \f$, by `left()` or by the field `left`
template< class N >
auto nodeLeft( N * n )
{
 if constexpr( requires { n->left(); } ) return( n->left() );
 else return( n->left );
 }

/// the right child of \f$ n \f$, by `right()` or by the field `right`
template< class N >
auto nodeRight( N * n )
{
 if constexpr( requires { n->right(); } ) return( n->right() );
 else return( n->right );
 }

/// the parent of \f$ n \f$, by `parent()` or by the field `parent`
template< class N >
auto nodeParent( N * n )
{
 if constexpr( requires { n->parent(); } ) return( n->parent() );
 else return( n->parent );
 }

/// the key of \f$ n \f$, by `read()` or by the field `key`
template< class N >
decltype( auto ) nodeKey( N * n )
{
 if constexpr( requires { n->read(); } ) return( n->read() );
 else return( ( n->key ) );
 }

}  // end( namespace detail )

/// a node of a binary tree: a pointer to a struct whose children are read
/// by `left()` and `right()` or by the fields `left` and `right`
template< class N >
concept BinaryNode = std::is_pointer_v< N > && requires( N n ) {
 detail::nodeLeft( n );
 detail::nodeRight( n );
 };

/// a node of a binary tree with the link to its parent
template< class N >
concept ParentBinaryNode = BinaryNode< N > && requires( N n ) {
 detail::nodeParent( n );
 };

/// a node of a binary tree with a key
template< class N >
concept KeyedBinaryNode = BinaryNode< N > && requires( N n ) {
 detail::nodeKey( n );
 };

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
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{or}\ s =
///     \textbf{nil}\ \textbf{then return}\ t = s = \textbf{nil} \cr
/// 3 & \quad \textbf{return}\ \mathrm{eq}( t.\mathit{key} , s.\mathit{key}
///     )\ \textbf{and}\ \mathrm{equalTrees}( t.\mathit{left} ,
///     s.\mathit{left} ) \cr
/// & \qquad \textbf{and}\ \mathrm{equalTrees}( t.\mathit{right} ,
///     s.\mathit{right} )
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
/// 1 & \textbf{function}\ \mathrm{between}( t , a , b ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ t.\mathit{key} \le a\ \textbf{or}
///     \ t.\mathit{key} \ge b\ \textbf{then return false} \cr
/// 4 & \quad \textbf{return}\ \mathrm{between}( t.\mathit{left} , a ,
///     t.\mathit{key} )\ \textbf{and}\ \mathrm{between}(
///     t.\mathit{right} , t.\mathit{key} , b )
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
/// 1 & \textbf{function}\ \mathrm{bh}( t ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}\ 1 \cr
/// 3 & \quad \textbf{if}\ t \text{ and a child of it are red}
///     \ \textbf{then return}\ -1 \cr
/// 4 & \quad l \gets \mathrm{bh}( t.\mathit{left} );\ r \gets \mathrm{bh}(
///     t.\mathit{right} ) \cr
/// 5 & \quad \textbf{if}\ l < 0\ \textbf{or}\ l \ne r\ \textbf{then
///     return}\ -1 \cr
/// 6 & \quad \textbf{return}\ l + ( 0 \text{ if } t \text{ is red, } 1
///     \text{ otherwise} ) \cr
/// 7 & \textbf{function}\ \mathrm{isRedBlackTree}( t ) \cr
/// 8 & \quad \textbf{return}\ ( t = \textbf{nil}\ \textbf{or}\ t \text{ is }
///     \text{black} )\ \textbf{and}\ \mathrm{bh}( t ) > 0
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
/// 1 & \textbf{procedure}\ \mathrm{range}( t , \mathit{lo} , \mathit{hi} )
///     \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
/// 3 & \quad \textbf{if}\ t.\mathit{key} < \mathit{lo}\ \textbf{then}
///     \ \mathrm{range}( t.\mathit{right} , \mathit{lo} , \mathit{hi} );
///     \ \textbf{return} \cr
/// 4 & \quad \textbf{if}\ t.\mathit{key} > \mathit{hi}\ \textbf{then}
///     \ \mathrm{range}( t.\mathit{left} , \mathit{lo} , \mathit{hi} );
///     \ \textbf{return} \cr
/// 5 & \quad \mathrm{range}( t.\mathit{left} , \mathit{lo} , \mathit{hi} );
///     \ f( t );\ \mathrm{range}( t.\mathit{right} , \mathit{lo} ,
///     \mathit{hi} )
/// \end{array}}
/// \f]
/// \pre not `less( hi , lo )`
template< KeyedBinaryNode N , class K , class F ,
          class Compare = std::less<> >
std::size_t searchTreeRange( N t , const K & lo , const K & hi , F && f ,
                             Compare less = {} );

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
/// 1 & \textbf{function}\ \mathrm{build}( A , i , j ) \cr
/// 2 & \quad \textbf{if}\ i \ge j\ \textbf{then return nil} \cr
/// 3 & \quad m \gets i + \lfloor ( j - i - 1 ) / 2 \rfloor \cr
/// 4 & \quad \textbf{return}\ \mathrm{make}( A[ m ] , \mathrm{build}( A , i ,
///     m ) , \mathrm{build}( A , m + 1 , j ) )
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

#endif  /* BinaryTreeAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File BinaryTreeAlgorithms.h ---------------------*/
/*--------------------------------------------------------------------------*/
