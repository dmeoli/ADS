/*--------------------------------------------------------------------------*/
/*---------------------- File BinaryTreeAlgorithms.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The classic problems on binary trees and on binary search trees, written
 * once for any representation of the nodes: a node is reached by a pointer,
 * nil is nullptr, and its links and its key are read either by the member
 * functions left(), right(), parent() and read() of BinaryTree [see
 * BinaryTree.h] or by the fields left, right, parent and key of a plain
 * struct, as in the pseudocode of the book. The functions that build a tree
 * return a BinaryTree, or take a callable make( v , l , r ) that makes a
 * node of value v over the subtrees l and r, for any other representation.
 *
 * - lowestCommonAncestor( u , v ) walks up the links to the parents: the
 *   deeper node climbs to the depth of the other, then both climb together
 *   until they meet, \f$ O( h ) \f$ time and \f$ O( 1 ) \f$ space for a tree
 *   of height h. The usual alternative pushes the two paths on two stacks and
 *   pops them while their tops agree, \f$ O( h ) \f$ space; with many queries
 *   on a fixed tree the reduction of Bender and Farach-Colton to the range
 *   minimum on the Euler tour answers in \f$ O( 1 ) \f$ after \f$ O( n ) \f$
 *   preprocessing [see RangeMinimum.h], as the structure of Harel and Tarjan
 *   did first.
 * - equalTrees( t , s ) tells whether two trees have the same shape and equal
 *   keys in the same places, by a simultaneous visit, \f$ O( n ) \f$ time and
 *   \f$ O( h ) \f$ space; with a comparison always true it compares the shapes
 *   only.
 * - isSearchTree( t ) checks the order of a binary search tree: each key must
 *   lie in the open interval that its ancestors leave to it, and comparing a
 *   node with its children only is not enough. The visit carries the two ends
 *   down, \f$ O( n ) \f$ time and \f$ O( h ) \f$ space, the same as checking
 *   that the invisit is strictly increasing.
 * - isRedBlackTree( t , red ) checks the constraints of a Red-Black
 *   colouring (the root black, no red node with a red child, the same
 *   number of black nodes on every path down to a leaf nil) by a postvisit
 *   that returns the black height of each subtree, \f$ O( n ) \f$ time.
 * - searchTreeRange( t , lo , hi , f ) reports in increasing order the
 *   nodes of a binary search tree whose keys are in [ lo , hi ], a visit
 *   pruned where the key leaves the range: \f$ O( h + k ) \f$ time for k keys
 *   reported, since the nodes examined outside the range lie on the two
 *   paths that search for lo and for hi.
 * - minimumHeightSearchTree( A ) builds the binary search tree of the sorted
 *   keys of A of the least height, \f$ \lceil \log_2( n + 1 ) \rceil - 1 \f$,
 *   by putting the middle key at the root and the two halves below,
 *   recursively: \f$ T( n ) = 2 T( n / 2 ) + O( 1 ) = O( n ) \f$ time and
 *   \f$ O( \log n ) \f$ stack. The same tree comes from the sorted list in
 *   \f$ O( n ) \f$ by building the subtrees in symmetric order, and any tree
 *   is rebalanced in place, in \f$ O( n ) \f$ time and \f$ O( 1 ) \f$ space,
 *   by the algorithm of Day, Stout and Warren.
 * - fromPreorderInorder( pre , in ) rebuilds the binary tree of distinct
 *   values whose previsit is pre and whose invisit is in. The first value of
 *   pre is the root and splits in into the two subtrees; rather than looking
 *   the root up in in, the algorithm keeps a stack of the nodes whose right
 *   subtree is still open: the next value of pre is the left child of the top,
 *   unless the top is the next value of in, in which case the nodes are popped
 *   while they match in and the value becomes the right child of the last one.
 *   Each node is pushed and popped once, \f$ O( n ) \f$ time, with no table of
 *   the places and only the equality of the values.
 * - cartesianBinaryTree( A ) is the Cartesian tree of Vuillemin as a
 *   BinaryTree: its root is the leftmost minimum of A and its subtrees the
 *   Cartesian trees of the parts before and after it, i.e., the binary tree
 *   whose invisit is A and which is a heap; with std::greater it is the
 *   maximal tree, with the maximum at the root. The parents come from
 *   cartesianTree() of RangeMinimum.h, which builds them with a stack of the
 *   right spine in \f$ O( n ) \f$ time, against the \f$ O( n^2 ) \f$ of the
 *   recursion of the definition on a sorted A.
 *
 * The contracts [see Contract.h]: lowestCommonAncestor() needs two nodes of
 * the same tree and returns a common ancestor none of whose children is one
 * (a postcondition); searchTreeRange() needs lo <= hi;
 * minimumHeightSearchTree() needs the keys strictly increasing and returns
 * a tree of the least height (an audit); fromPreorderInorder() needs two
 * visits of the same length and checks that the invisit of the tree built
 * is in, which fails exactly when no tree of distinct values has those two
 * visits; cartesianBinaryTree() returns a tree whose invisit is A (an
 * audit). The functions are defined in BinaryTreeAlgorithms.tpp.
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

/// the left child of n, by left() or by the field left
template< class N >
auto nodeLeft( N * n )
{
 if constexpr( requires { n->left(); } ) return( n->left() );
 else return( n->left );
 }

/// the right child of n, by right() or by the field right
template< class N >
auto nodeRight( N * n )
{
 if constexpr( requires { n->right(); } ) return( n->right() );
 else return( n->right );
 }

/// the parent of n, by parent() or by the field parent
template< class N >
auto nodeParent( N * n )
{
 if constexpr( requires { n->parent(); } ) return( n->parent() );
 else return( n->parent );
 }

/// the key of n, by read() or by the field key
template< class N >
decltype( auto ) nodeKey( N * n )
{
 if constexpr( requires { n->read(); } ) return( n->read() );
 else return( ( n->key ) );
 }

}  // end( namespace detail )

/// a node of a binary tree: a pointer to a struct whose children are read
/// by left() and right() or by the fields left and right
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

/// lowestCommonAncestor(): the deepest node that is an ancestor of both u and
/// v, each node being an ancestor of itself; \f$ O( h ) \f$ time,
/// \f$ O( 1 ) \f$ space
/// \pre u and v are nodes of the same tree
/// \post an ancestor of u and of v, none of whose children is one
template< ParentBinaryNode N >
N lowestCommonAncestor( N u , N v );

/*--------------------------------------------------------------------------*/
/*--------------------------- THE COMPARISONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// equalTrees(): t and s have the same shape and eq( a , b ) holds on the keys
/// a and b of each two nodes in the same place; \f$ O( n ) \f$ time,
/// \f$ O( h ) \f$ space
template< KeyedBinaryNode N , KeyedBinaryNode M ,
          class Equal = std::equal_to<> >
[[nodiscard]] bool equalTrees( N t , M s , Equal eq = {} );

/// isSearchTree(): the keys of t are strictly increasing in the invisit,
/// by less; \f$ O( n ) \f$ time, \f$ O( h ) \f$ space
template< KeyedBinaryNode N , class Compare = std::less<> >
[[nodiscard]] bool isSearchTree( N t , Compare less = {} );

/// isRedBlackTree(): the colouring of t, red( u ) telling whether the node u
/// is red, respects the constraints of a Red-Black tree: the root black, no
/// red node with a red child, the same number of black nodes on every path
/// from the root down to a leaf nil; \f$ O( n ) \f$ time, \f$ O( h ) \f$ space
template< BinaryNode N , class Red >
[[nodiscard]] bool isRedBlackTree( N t , Red red );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE SEARCHES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// searchTreeRange(): f( u ) on the nodes u of the binary search tree t
/// whose keys are in [ lo , hi ], in increasing order of the keys; the
/// number of the nodes examined, at most k + 2 ( h + 1 ) for k keys in the
/// range; \f$ O( h + k ) \f$ time
/// \pre ! less( hi , lo )
template< KeyedBinaryNode N , class K , class F ,
          class Compare = std::less<> >
std::size_t searchTreeRange( N t , const K & lo , const K & hi , F && f ,
                             Compare less = {} );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE BUILDERS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// minimumHeightSearchTree(): the binary search tree of the keys of A of
/// the least height, the lower middle key at the root of each subtree,
/// whose nodes are made by make( v , l , r ) over the subtrees l and r,
/// nil if A is empty; \f$ O( n ) \f$ time
/// \pre A strictly increasing by less
/// \post the height is ceil( log2( n + 1 ) ) - 1 (an audit)
template< class T , class Make , class Compare = std::less<> >
auto minimumHeightSearchTree( const std::vector< T > & A , Make make ,
                              Compare less = {} )
 -> std::invoke_result_t< Make & , const T & , std::nullptr_t ,
                          std::nullptr_t >;

/// minimumHeightSearchTree() as a BinaryTree, which the caller owns
/// \pre A strictly increasing by less
template< class T , class Compare = std::less<> >
 requires std::predicate< Compare & , const T & , const T & >
BinaryTree< T > * minimumHeightSearchTree( const std::vector< T > & A ,
                                           Compare less = {} );

/// fromPreorderInorder(): the binary tree whose previsit is pre and whose
/// invisit is in, nil if they are empty; \f$ O( n ) \f$ time; the caller owns
/// it
/// \pre pre and in are the visits of a binary tree of distinct values
template< class T >
BinaryTree< T > * fromPreorderInorder( const std::vector< T > & pre ,
                                       const std::vector< T > & in );

/// cartesianBinaryTree(): the Cartesian tree of A, the leftmost minimum by
/// less at the root, a heap whose invisit is A, nil if A is empty;
/// \f$ O( n ) \f$ time; the caller owns it
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
