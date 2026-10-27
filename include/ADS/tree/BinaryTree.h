/*--------------------------------------------------------------------------*/
/*--------------------------- File BinaryTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary TREE of the book: an ordered tree where each node has at most
 * two children, told apart as the left and the right one, realized with
 * pointers to the parent and to the two children. Moving along the tree,
 * reading, writing and inserting take \f$ O( 1 ) \f$ time, deleting a
 * subtree time linear in its size. As for `Tree`, a node and its subtree are
 * the same thing, a tree owns its subtrees (a tree whose insertion is
 * refused stays the caller's), and nil is `nullptr`.
 *
 * The visit `dfs()` examines a node before its subtrees (previsit), between
 * them (invisit, or symmetric visit) or after them (postvisit). An ordered
 * tree \f$ T \f$ is represented by the binary tree \f$ B \f$ whose left
 * child of a node is its first child in \f$ T \f$ and whose right child is
 * its next sibling in \f$ T \f$ [see `to_binary()`]: the previsits of
 * \f$ T \f$ and of \f$ B \f$ are the same, and the postvisit of \f$ T \f$ is
 * the invisit of \f$ B \f$.
 *
 * `detachLeft()` and `detachRight()` cut a subtree off its parent without
 * destroying it, in \f$ O( 1 ) \f$ time: the subtree becomes a root owned by
 * the caller, ready to be inserted elsewhere, which moves subtrees around (a
 * mirror, a rotation) with no copy.
 *
 * `copy()` makes a deep copy of a subtree and `join()` a new root with the
 * copies of two trees as children, in time linear in their size.
 *
 * The ranges `preorder()`, `inorder()` and `postorder()` walk the nodes of
 * a subtree by the links to the parents, with no stack and no recursion, in
 * \f$ O( n ) \f$ time overall and \f$ O( 1 ) \f$ extra space: each node is
 * left at most once from each of its three visits.
 *
 * `to_list()` writes a binary tree as a parenthesized list, `()` for the
 * empty tree and \f$ ( v \, L \, R ) \f$ for the root \f$ v \f$ with the
 * subtrees \f$ L \f$ and \f$ R \f$, e.g.,
 * `( 8 () ( 5 ( 27 () () ) ( 16 () () ) ) )`, and `from_list()` reads it
 * back; the values are written with `<<` and read with `>>`, and must
 * contain no space nor parenthesis.
 *
 * A complete \f$ d \f$-ary tree, whose internal nodes have \f$ d \f$
 * children, is stored in a vector, the root in the place 0 and the children
 * of the node \f$ i \f$ in the places \f$ d i + 1 , \ldots , d i + d \f$
 * [see `dary_child()`].
 *
 * The contracts [see `Contract.h`]: the tree inserted is a root other than
 * the node itself and goes where no child is; the subtree detached is a root
 * and its place is empty; the invariant is that each child has its parent
 * as parent, down the subtree; the places of a \f$ d \f$-ary tree need
 * \f$ d \geq 1 \f$, a child \f$ 1 \leq j \leq d \f$ and a place
 * \f$ i > 0 \f$ to have a parent. The operations are defined in
 * `BinaryTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.5 and
 *      5.6, https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_BinaryTree
 #define __ADS_BinaryTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/tree/Tree.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BinaryTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary tree with pointers to the parent and to the two children
template< class T >
class BinaryTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the values

 /// a new tree, made of one node containing \f$ v \f$ (the `Tree( v )` of
 /// the book)
 explicit BinaryTree( const T & v );

 /// no copy, since a tree owns its subtrees [see `copy()`]
 BinaryTree( const BinaryTree & ) = delete;

 /// no copy, since a tree owns its subtrees [see `copy()`]
 BinaryTree & operator=( const BinaryTree & ) = delete;

 /// destroys the two subtrees, i.e., the whole tree, with a stack of its
 /// own rather than by recursion
 ~BinaryTree();

/*--------------------------------------------------------------------------*/

 /// the value of the node
 [[nodiscard]] const T & read( void ) const;

 /// writes \f$ v \f$ in the node
 void write( const T & v );

 /// the parent; nil if the node is a root
 [[nodiscard]] BinaryTree * parent( void ) const;

 /// the left child; nil if there is none
 [[nodiscard]] BinaryTree * left( void ) const;

 /// the right child; nil if there is none
 [[nodiscard]] BinaryTree * right( void ) const;

 /// inserts the tree \f$ t \f$ as the left child, which must be missing
 ///
 /// In pseudocode (section 5.5):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertLeft}( t ) \cr
 /// 2 & \quad t.\mathit{parent} \gets \mathit{this};\ \mathit{left} \gets t
 /// \end{array}}
 /// \f]
 /// \pre \f$ t \f$ is a root, not this node nor the root of its tree (which
 ///      would make a cycle), and the left child is missing
 /// \post \f$ t \f$ is the left child
 void insertLeft( BinaryTree * t );

 /// inserts the tree \f$ t \f$ as the right child, which must be missing,
 /// as `insertLeft()` does on the left
 /// \pre \f$ t \f$ is a root, not this node nor the root of its tree (which
 ///      would make a cycle), and the right child is missing
 /// \post \f$ t \f$ is the right child
 void insertRight( BinaryTree * t );

 /// destroys the left subtree
 void deleteLeft( void );

 /// destroys the right subtree
 void deleteRight( void );

 /// detaches the left subtree without destroying it: it becomes a root,
 /// owned by the caller; nil if there is none
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{detachLeft}() \cr
 /// 2 & \quad t \gets \mathit{left};\ \mathit{left} \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{if}\ t \ne \textbf{nil}\ \textbf{then}
 ///     \ t.\mathit{parent} \gets \textbf{nil} \cr
 /// 4 & \quad \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// \post the left child is missing and the result is nil or a root
 [[nodiscard]] BinaryTree * detachLeft( void );

 /// detaches the right subtree without destroying it: it becomes a root,
 /// owned by the caller; nil if there is none, as `detachLeft()` does on
 /// the left
 /// \post the right child is missing and the result is nil or a root
 [[nodiscard]] BinaryTree * detachRight( void );

/*--------------------------------------------------------------------------*/
 private:

 /// \f$ t \f$ as the child `child`, missing
 void insert( BinaryTree * & child , BinaryTree * t );

 /// the subtree of the child `child`, detached
 BinaryTree * detach( BinaryTree * & child );

 /// each child of this node has it as its parent (the links that a
 /// change of the tree touches), \f$ O( 1 ) \f$ per child
 [[nodiscard]] bool linked( void ) const;

 /// the root of the tree of this node, \f$ O( \mathit{depth} ) \f$
 [[nodiscard]] const BinaryTree * top( void ) const;

 T f_value;                       ///< the value
 BinaryTree * f_parent = nullptr; ///< the parent
 BinaryTree * f_left = nullptr;   ///< the left child
 BinaryTree * f_right = nullptr;  ///< the right child

 };  // end( class BinaryTree )

/*--------------------------------------------------------------------------*/
/*------------------------------- VISITS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the visit in depth: `pre( t )` before the subtrees, `in( t )` between
/// them, `post( t )` after them; nothing on an empty tree. It runs with a
/// stack of its own, not by recursion, so that the height of the tree may be
/// \f$ n \f$
///
/// In pseudocode (section 5.6), recursive as in the book:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dfs}( t ) \cr
/// 2 & \quad \textbf{if}\ t \ne \textbf{nil}\ \textbf{then} \cr
/// 3 & \qquad \mathrm{pre}( t );\ \mathrm{dfs}( t.\mathrm{left}() );
///     \ \mathrm{in}( t );\ \mathrm{dfs}( t.\mathrm{right}() );
///     \ \mathrm{post}( t )
/// \end{array}}
/// \f]
template< class T , class Pre , class In , class Post >
void dfs( BinaryTree< T > * t , Pre && pre , In && in , Post && post );

/// the previsit of a binary tree, `dfs()` with `f` as `pre` alone
template< class T , class F >
void pre_visit( BinaryTree< T > * t , F && f );

/// the invisit (symmetric visit) of a binary tree, `dfs()` with `f` as
/// `in` alone
template< class T , class F >
void in_visit( BinaryTree< T > * t , F && f );

/// the postvisit of a binary tree, `dfs()` with `f` as `post` alone
template< class T , class F >
void post_visit( BinaryTree< T > * t , F && f );

/// a deep copy of the subtree \f$ t \f$, nil if \f$ t \f$ is nil; the
/// caller owns it
///
/// In pseudocode (section 5.6), a previsit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{copy}( t ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return nil} \cr
/// 3 & \quad c \gets \mathrm{BinaryTree}( t.\mathrm{read}() ) \cr
/// 4 & \quad c.\mathrm{insertLeft}( \mathrm{copy}( t.\mathrm{left}() ) );
///     \ c.\mathrm{insertRight}( \mathrm{copy}( t.\mathrm{right}() ) ) \cr
/// 5 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
template< class T >
BinaryTree< T > * copy( const BinaryTree< T > * t );

/// a new root of value \f$ v \f$ with copies of \f$ l \f$ and \f$ r \f$,
/// each possibly nil, as its children; the caller owns it
///
/// In pseudocode (section 5.6):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{join}( v , l , r ) \cr
/// 2 & \quad t \gets \mathrm{BinaryTree}( v ) \cr
/// 3 & \quad t.\mathrm{insertLeft}( \mathrm{copy}( l ) );
///     \ t.\mathrm{insertRight}( \mathrm{copy}( r ) );\ \textbf{return}\ t
/// \end{array}}
/// \f]
template< class T >
BinaryTree< T > * join( const T & v , const BinaryTree< T > * l ,
                        const BinaryTree< T > * r );

/*--------------------------------------------------------------------------*/
/*--------------------------- REPRESENTATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the parenthesized list of \f$ t \f$, `()` if \f$ t \f$ is nil
///
/// In pseudocode (section 5.6), a previsit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{to\_list}( t ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}
///     \ \text{“()”} \cr
/// 3 & \quad \textbf{return}\ \text{“( ”},\ t.\mathrm{read}(),
///     \ \mathrm{to\_list}( t.\mathrm{left}() ),\ \mathrm{to\_list}(
///     t.\mathrm{right}() ),\ \text{“ )”} \text{ separated by spaces}
/// \end{array}}
/// \f]
template< class T >
std::string to_list( const BinaryTree< T > * t );

/// the binary tree of the parenthesized list \f$ s \f$, nil for `()`; the
/// caller owns it
///
/// In pseudocode (section 5.6), a descent by the grammar
/// \f$ B \to ( \, ) \mid ( \, v \, B \, B \, ) \f$ on the tokens of
/// \f$ s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{readTree}() \cr
/// 2 & \quad \text{read “(”} \cr
/// 3 & \quad \textbf{if}\ \text{the next token is “)”}\ \textbf{then}
///     \ \text{read it};\ \textbf{return nil} \cr
/// 4 & \quad t \gets \mathrm{BinaryTree}( \text{the value read} ) \cr
/// 5 & \quad t.\mathrm{insertLeft}( \mathrm{readTree}() );
///     \ t.\mathrm{insertRight}( \mathrm{readTree}() ) \cr
/// 6 & \quad \text{read “)”};\ \textbf{return}\ t
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is the list of a binary tree, and `>>` reads each atom
///      whole (e.g., `8x` is not an `int`)
template< class T >
BinaryTree< T > * from_list( std::string_view s );

/// the binary tree that represents the ordered tree \f$ t \f$ (figure 5.9):
/// the left child of a node is its first child in \f$ t \f$, the right one
/// its next sibling; the caller owns the result
///
/// In pseudocode (section 5.5, figure 5.9):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{to\_binary}( t ) \cr
/// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return nil} \cr
/// 3 & \quad b \gets \mathrm{BinaryTree}( t.\mathrm{read}() ) \cr
/// 4 & \quad b.\mathrm{insertLeft}( \mathrm{to\_binary}(
///     t.\mathrm{leftmostChild}() ) ) \cr
/// 5 & \quad b.\mathrm{insertRight}( \mathrm{to\_binary}(
///     t.\mathrm{rightSibling}() ) ) \cr
/// 6 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node
template< class T >
BinaryTree< T > * to_binary( const Tree< T > * t );

/// the place \f$ d i + j \f$ of the \f$ j \f$-th child,
/// \f$ 1 \le j \le d \f$, of the node in the place \f$ i \f$ of a complete
/// \f$ d \f$-ary tree stored in a vector
/// \pre \f$ i \ge 0 \f$, \f$ d \ge 1 \f$, \f$ 1 \le j \le d \f$, and
///      \f$ d i + j \f$ fits in a `long`
inline long dary_child( long i , long d , long j );

/// the place \f$ \lfloor ( i - 1 ) / d \rfloor \f$ of the parent of the
/// node in the place \f$ i > 0 \f$
/// \pre \f$ i > 0 \f$, \f$ d \ge 1 \f$
inline long dary_parent( long i , long d );

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------- THE STANDARD INTERFACE -------------------------*/
/*--------------------------------------------------------------------------*/

// the nodes of a binary tree as ranges, forward, by the links to the
// parents, with no stack; *it is a node

namespace detail {

/// the preorder below a root
struct BinaryPreorder {
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * next( const void * , N * n , N * root ) {
  if( n->left() ) return( n->left() );
  if( n->right() ) return( n->right() );
  for( ; n != root ; n = n->parent() )      // up to a right sibling
   if( ( n == n->parent()->left() ) && n->parent()->right() )
    return( n->parent()->right() );
  return( nullptr );
  }
 };

/// the symmetric order below a root
struct BinaryInorder {
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * leftmost( N * n ) {
  while( n->left() ) n = n->left();
  return( n );
  }
 template< class N >
 static N * next( const void * , N * n , N * root ) {
  if( n->right() ) return( leftmost( n->right() ) );
  for( ; n != root ; n = n->parent() )      // up from a left child
   if( n == n->parent()->left() ) return( n->parent() );
  return( nullptr );
  }
 };

/// the postorder below a root
struct BinaryPostorder {
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * firstLeaf( N * n ) {
  while( n->left() || n->right() )
   n = n->left() ? n->left() : n->right();
  return( n );
  }
 template< class N >
 static N * next( const void * , N * n , N * root ) {
  if( n == root ) return( nullptr );
  N * p = n->parent();
  if( ( n == p->left() ) && p->right() )   // down the right sibling
   return( firstLeaf( p->right() ) );
  return( p );
  }
 };

}  // end( namespace detail )

/// the nodes of the subtree of \f$ t \f$ in preorder, \f$ t \f$ first, by
/// the links to the parents
///
/// In pseudocode (section 5.6, beyond the book for the walk with no
/// stack), the step to the next node:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{next}( u , t ) \cr
/// 2 & \quad \textbf{if}\ u.\mathit{left} \ne \textbf{nil}\ \textbf{then
///     return}\ u.\mathit{left} \cr
/// 3 & \quad \textbf{if}\ u.\mathit{right} \ne \textbf{nil}
///     \ \textbf{then return}\ u.\mathit{right} \cr
/// 4 & \quad \textbf{while}\ u \ne t\ \textbf{do} \cr
/// 5 & \qquad p \gets u.\mathit{parent} \cr
/// 6 & \qquad \textbf{if}\ u = p.\mathit{left}\ \textbf{and}
///     \ p.\mathit{right} \ne \textbf{nil}\ \textbf{then return}
///     \ p.\mathit{right} \cr
/// 7 & \qquad u \gets p \cr
/// 8 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
template< class T >
auto preorder( BinaryTree< T > * t )
{
 using It = WalkIterator< void , BinaryTree< T > * ,
                          detail::BinaryPreorder >;
 return( std::ranges::subrange( It( nullptr , t , t ) , It() ) );
 }

/// the nodes of the subtree of \f$ t \f$ in symmetric order, by the links to
/// the parents
///
/// In pseudocode (section 5.6, beyond the book for the walk with no
/// stack), the first node the leftmost one, and the step to the next node:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{next}( u , t ) \cr
/// 2 & \quad \textbf{if}\ u.\mathit{right} \ne \textbf{nil}
///     \ \textbf{then return}\ \text{the leftmost node below }
///     u.\mathit{right} \cr
/// 3 & \quad \textbf{while}\ u \ne t\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ u = u.\mathit{parent}.\mathit{left}
///     \ \textbf{then return}\ u.\mathit{parent} \cr
/// 5 & \qquad u \gets u.\mathit{parent} \cr
/// 6 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
template< class T >
auto inorder( BinaryTree< T > * t )
{
 using It = WalkIterator< void , BinaryTree< T > * , detail::BinaryInorder >;
 return( std::ranges::subrange(
          It( nullptr , detail::BinaryInorder::leftmost( t ) , t ) , It() ) );
 }

/// the nodes of the subtree of \f$ t \f$ in postorder, \f$ t \f$ last, by
/// the links to the parents
///
/// In pseudocode (section 5.6, beyond the book for the walk with no
/// stack), the first node the first leaf, down the left children when
/// there are, and the step to the next node:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{next}( u , t ) \cr
/// 2 & \quad \textbf{if}\ u = t\ \textbf{then return nil} \cr
/// 3 & \quad p \gets u.\mathit{parent} \cr
/// 4 & \quad \textbf{if}\ u = p.\mathit{left}\ \textbf{and}
///     \ p.\mathit{right} \ne \textbf{nil}\ \textbf{then return}
///     \ \text{the first leaf below } p.\mathit{right} \cr
/// 5 & \quad \textbf{return}\ p
/// \end{array}}
/// \f]
template< class T >
auto postorder( BinaryTree< T > * t )
{
 using It = WalkIterator< void , BinaryTree< T > * ,
                          detail::BinaryPostorder >;
 return( std::ranges::subrange(
          It( nullptr , detail::BinaryPostorder::firstLeaf( t ) , t ) ,
          It() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/BinaryTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BinaryTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BinaryTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
