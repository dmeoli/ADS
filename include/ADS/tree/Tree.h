/*--------------------------------------------------------------------------*/
/*------------------------------ File Tree.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered TREE of the book, realized with pointers to the parent, to the
 * first child and to the next sibling, and its visits.
 *
 * A node and the subtree rooted in it are the same thing, a `Tree`: the
 * constructor makes an isolated node, `insertChild()` and `insertSibling()`
 * hang a whole tree under a node in \f$ O( 1 ) \f$ time, `deleteChild()`
 * and `deleteSibling()` destroy a subtree in time linear in its size. A tree
 * owns its subtrees: deleting the root of a tree destroys all of it; a tree
 * whose insertion is refused (its contract is violated) stays the caller's.
 * As in the book, nil is `nullptr`.
 *
 * The visits examine each node once, in \f$ \Theta( n ) \f$ time: the
 * previsit examines the root and then visits the subtrees
 * \f$ T_0 , \ldots , T_{k-1} \f$ of its children in order, the postvisit
 * visits the subtrees and then examines the root, the invisit with index
 * \f$ i \f$ visits the first \f$ i \f$ subtrees, examines the root and
 * visits the others, and the visit by levels (`bfs()`) examines all the
 * nodes of level \f$ l \f$ before those of level \f$ l + 1 \f$, using a
 * queue. `copy()` makes a deep copy of a subtree.
 *
 * The contracts [see `Contract.h`]: the tree inserted is a root (the
 * precondition of the book, `t.parent()` nil) other than the node itself,
 * `insertSibling()` needs a node that is not a root, and the visits a node;
 * the invariant is that each child of a node has that node as its parent,
 * down the subtree. The operations are defined in `Tree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1, 5.2
 *      and 5.3, https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_Tree
 #define __ADS_Tree
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/sequence/List.h"

#include <cstddef>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Tree ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree, with pointers to parent, first child and next sibling
template< class T >
class Tree
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the values

 /// a new tree, made of one node containing \f$ v \f$ (the `Tree( v )` of
 /// the book)
 explicit Tree( const T & v );

 /// no copy, since a tree owns its subtrees [see `copy()`]
 Tree( const Tree & ) = delete;

 /// no copy, since a tree owns its subtrees [see `copy()`]
 Tree & operator=( const Tree & ) = delete;

 /// destroys the subtrees of the children, i.e., the whole tree, with a
 /// stack of its own rather than by recursion
 ///
 /// In pseudocode (section 5.2, with an explicit stack \f$ S \f$, as the
 /// height may be \f$ n \f$):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{destroy}( t )\quad \triangleright
 ///     \ \text{no recursion: the height may be } n \cr
 /// 2 & \quad S \gets \text{a stack of the children of } t \cr
 /// 3 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do}\ u \gets
 ///     \mathrm{pop}( S );\ \text{push the children of } u;\ \text{free } u
 /// \end{array}}
 /// \f]
 ~Tree();

/*--------------------------------------------------------------------------*/

 /// the value of the node
 [[nodiscard]] const T & read( void ) const;

 /// writes \f$ v \f$ in the node
 void write( const T & v );

 /// the parent; nil if the node is a root
 [[nodiscard]] Tree * parent( void ) const;

 /// the first child; nil if the node is a leaf
 [[nodiscard]] Tree * leftmostChild( void ) const;

 /// the next sibling; nil if there is none
 [[nodiscard]] Tree * rightSibling( void ) const;

 /// inserts the tree \f$ t \f$ as the first child of this node, which then
 /// owns it, in \f$ O( 1 ) \f$
 ///
 /// In pseudocode (section 5.2):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertChild}( t ) \cr
 /// 2 & \quad t.\mathit{parent} \gets \mathit{this};\ t.\mathit{sibling}
 ///     \gets \mathit{child};\ \mathit{child} \gets t
 /// \end{array}}
 /// \f]
 /// \pre \f$ t \f$ is a root, not this node nor the root of its tree (which
 ///      would make a cycle)
 /// \post \f$ t \f$ is the first child
 void insertChild( Tree * t );

 /// inserts the tree \f$ t \f$ as the next sibling of this node, which must
 /// not be a root; the parent of this node then owns \f$ t \f$
 ///
 /// In pseudocode (section 5.2):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertSibling}( t ) \cr
 /// 2 & \quad t.\mathit{parent} \gets \mathit{parent};\ t.\mathit{sibling}
 ///     \gets \mathit{sibling};\ \mathit{sibling} \gets t
 /// \end{array}}
 /// \f]
 /// \pre \f$ t \f$ is a root, and this node is not; \f$ t \f$ is not the
 ///      root of the tree of this node (which would make a cycle)
 /// \post \f$ t \f$ is the next sibling
 void insertSibling( Tree * t );

 /// true if the node has no child, i.e., it is a leaf
 [[nodiscard]] bool isLeaf( void ) const;

 /// true if the node has no next sibling
 [[nodiscard]] bool isLastSibling( void ) const;

 /// destroys the subtree rooted in the first child, whose next sibling
 /// becomes the first child
 void deleteChild( void );

 /// destroys the subtree rooted in the next sibling, whose next sibling
 /// takes its place
 void deleteSibling( void );

/*--------------------------------------------------------------------------*/
 private:
 /// each child of this node has it as its parent (the links that a
 /// change of the tree touches), \f$ O( 1 ) \f$ per child
 [[nodiscard]] bool linked( void ) const;

 /// the root of the tree of this node, \f$ O( \mathit{depth} ) \f$
 [[nodiscard]] const Tree * top( void ) const;

 T f_value;                   ///< the value
 Tree * f_parent = nullptr;   ///< the parent
 Tree * f_child = nullptr;    ///< the first child
 Tree * f_sibling = nullptr;  ///< the next sibling

 };  // end( class Tree )

/*--------------------------------------------------------------------------*/
/*------------------------------- VISITS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the visit in depth: `pre( u )` on each node before its subtrees are
/// visited, `post( u )` after; `pre_visit()` and `post_visit()` are `dfs()`
/// with one of them. It and `in_visit()` run with a stack of their own, not
/// by recursion, so that the height of the tree may be \f$ n \f$
///
/// In pseudocode (section 5.3), recursive as in the book, while the code
/// keeps on a stack each node with the child it is visiting:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dfs}( t ) \cr
/// 2 & \quad \mathrm{pre}( t )\quad \triangleright\ \text{on the way down}
///     \cr
/// 3 & \quad \textbf{for}\ c \gets t.\mathrm{leftmostChild}() ,
///     c.\mathrm{rightSibling}() , \ldots \text{ while } c \ne \textbf{nil}
///     \ \textbf{do}\ \mathrm{dfs}( c ) \cr
/// 4 & \quad \mathrm{post}( t )\quad \triangleright\ \text{on the way up}
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node
template< class T , class Pre , class Post >
void dfs( Tree< T > * t , Pre && pre , Post && post );

/// the previsit: `f( u )` on the root, then on the subtrees in order
///
/// In pseudocode (section 5.3), `dfs()` with no postvisit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{pre\_visit}( t , f ) \cr
/// 2 & \quad \mathrm{dfs}( t ) \text{ with } \mathrm{pre} = f \text{ and }
///     \text{nothing as } \mathrm{post}
/// \end{array}}
/// \f]
template< class T , class F >
void pre_visit( Tree< T > * t , F && f );

/// the postvisit: `f( u )` on the subtrees in order, then on the root
///
/// In pseudocode (section 5.3), `dfs()` with no previsit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{post\_visit}( t , f ) \cr
/// 2 & \quad \mathrm{dfs}( t ) \text{ with nothing as } \mathrm{pre}
///     \text{ and } \mathrm{post} = f
/// \end{array}}
/// \f]
template< class T , class F >
void post_visit( Tree< T > * t , F && f );

/// the invisit: the first \f$ i \f$ subtrees, the root, the other subtrees
///
/// In pseudocode (section 5.3), recursive as in the book, while the code
/// keeps its own stack as `dfs()` does:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{in\_visit}( t , i , f )\quad
///     \triangleright\ t \text{ after its first } i \text{ children} \cr
/// 2 & \quad k \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ \text{child } c \text{ of } t
///     \text{, from the leftmost}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ k = i\ \textbf{then}\ f( t ) \cr
/// 5 & \qquad \mathrm{in\_visit}( c , i , f );\ k \gets k + 1 \cr
/// 6 & \quad \textbf{if}\ k \le i\ \textbf{then}\ f( t )\quad
///     \triangleright\ \text{at most } i \text{ children}
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node and \f$ i \ge 0 \f$
template< class T , class F >
void in_visit( Tree< T > * t , int i , F && f );

/// a deep copy of the subtree \f$ t \f$ and of its children, not of its
/// siblings; the caller owns it
///
/// In pseudocode (section 5.3), a previsit that copies each node and then
/// the subtrees of its children, in order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{copy}( t ) \cr
/// 2 & \quad c \gets \mathrm{Tree}( t.\mathrm{read}() );\ \ell \gets
///     \textbf{nil}\quad \triangleright\ \ell \text{ the last child copied}
///     \cr
/// 3 & \quad \textbf{foreach}\ \text{child } u \text{ of } t
///     \text{, from the leftmost}\ \textbf{do} \cr
/// 4 & \qquad x \gets \mathrm{copy}( u ) \cr
/// 5 & \qquad \textbf{if}\ \ell \ne \textbf{nil}\ \textbf{then}
///     \ \ell.\mathrm{insertSibling}( x )\ \textbf{else}
///     \ c.\mathrm{insertChild}( x ) \cr
/// 6 & \qquad \ell \gets x \cr
/// 7 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node
/// \post the copy is a root
template< class T >
Tree< T > * copy( const Tree< T > * t );

/// the visit by levels, with a queue of the nodes to examine
///
/// In pseudocode (section 5.3):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bfs}( t , f )\quad \triangleright
///     \ \text{by levels, each from left to right} \cr
/// 2 & \quad Q \gets \text{a queue holding only } t \cr
/// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathrm{dequeue}( Q );\ f( u );
///     \ \text{enqueue the children of } u \text{, from the leftmost}
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node
template< class T , class F >
void bfs( Tree< T > * t , F && f );

/// the first level of largest weight, the weight of a level being the sum
/// of the values of its nodes (the root at level 0), by a visit by levels
///
/// In pseudocode (section 5.3), the levels kept one at a time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxWeightLevel}( t ) \cr
/// 2 & \quad L \gets ( t );\ \ell \gets 0;\ b \gets 0;\ W \gets -\infty\quad
///     \triangleright\ L \text{ the nodes of level } \ell \cr
/// 3 & \quad \textbf{while}\ L \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad w \gets \text{the sum of the values of the nodes of } L \cr
/// 5 & \qquad \textbf{if}\ w > W\ \textbf{then}\ ( b , W ) \gets ( \ell , w
///     )\quad \triangleright\ \text{the first level of the largest weight}
///     \cr
/// 6 & \qquad L \gets \text{the children of the nodes of } L;\ \ell \gets
///     \ell + 1 \cr
/// 7 & \quad \textbf{return}\ b
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node
template< class T >
int maxWeightLevel( Tree< T > * t );

/// the parenthesized list of \f$ t \f$, \f$ ( r \, T_1 \cdots T_k ) \f$ for
/// the root \f$ r \f$ with the subtrees \f$ T_1 , \ldots , T_k \f$, e.g.,
/// `( a ( b ( e ) ( f ) ) ( c ( g ) ) )`; the values are written with `<<`
/// and must contain no space nor parenthesis
///
/// In pseudocode (section 5.3), a previsit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{to\_list}( t ) \cr
/// 2 & \quad \textbf{return}\ \text{“(”, } t.\mathrm{read}() \text{, the }
///     \mathrm{to\_list} \text{ of each child of } t
///     \text{ and “)”, joined by single spaces}
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is a node
template< class T >
std::string to_list( const Tree< T > * t );

/// the tree of the parenthesized list \f$ s \f$, its values read with
/// `>>`; the caller owns it
///
/// In pseudocode (section 5.3), a descent by the grammar
/// \f$ T \to ( \, v \, T^* \, ) \f$ on the tokens of \f$ s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{readTree}()\quad \triangleright
///     \ \text{the inverse of } \mathrm{to\_list} \cr
/// 2 & \quad \text{read “(” and the value } v \text{ that follows};\ t \gets
///     \mathrm{Tree}( v ) \cr
/// 3 & \quad \textbf{while}\ \text{“(” follows}\ \textbf{do}\ \text{make }
///     \mathrm{readTree}() \text{ the last child of } t \cr
/// 4 & \quad \text{read “)”};\ \textbf{return}\ t
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is the list of a tree, and `>>` reads each atom whole
///      (e.g., `8x` is not an `int`)
template< class T >
Tree< T > * tree_from_list( std::string_view s );

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------- THE STANDARD INTERFACE -------------------------*/
/*--------------------------------------------------------------------------*/

// the nodes of a tree as ranges, forward, by the links to the parents and
// the siblings, with no stack; *it is a node

namespace detail
{

/// the walk of the children of a node
struct TreeChildren
{
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * next( const void * , N * n , N * ) {
  return( n->rightSibling() );
  }
 };

/// the preorder below a root
struct TreePreorder
{
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * next( const void * , N * n , N * root ) {
  if( n->leftmostChild() )
   return( n->leftmostChild() );
  for( ; n != root ; n = n->parent() )  // up to a right sibling
   if( n->rightSibling() )
    return( n->rightSibling() );
  return( nullptr );
  }
 };

}  // end( namespace detail )

/// the children of \f$ t \f$, from the leftmost, as a range
template< class T >
auto children( Tree< T > * t )
{
 using It = WalkIterator< void , Tree< T > * , detail::TreeChildren >;
 return(
   std::ranges::subrange( It( nullptr , t->leftmostChild() , t ) , It() ) );
 }

/// the nodes of the subtree of \f$ t \f$ in preorder, \f$ t \f$ first, as a
/// range by the links to the parents and the siblings, with no stack
///
/// In pseudocode (section 5.3), the step to the next node of the preorder:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{next}( u , t )\quad \triangleright
///     \ \text{the node after } u \text{ in the previsit of } t \cr
/// 2 & \quad \textbf{if}\ u.\mathrm{leftmostChild}() \ne \textbf{nil}
///     \ \textbf{then return}\ u.\mathrm{leftmostChild}() \cr
/// 3 & \quad \textbf{while}\ u \ne t \text{ and } u.\mathrm{rightSibling}()
///     = \textbf{nil}\ \textbf{do}\ u \gets u.\mathrm{parent}() \cr
/// 4 & \quad \textbf{if}\ u = t\ \textbf{then return nil}
///     \ \textbf{else return}\ u.\mathrm{rightSibling}()
/// \end{array}}
/// \f]
template< class T >
auto preorder( Tree< T > * t )
{
 using It = WalkIterator< void , Tree< T > * , detail::TreePreorder >;
 return( std::ranges::subrange( It( nullptr , t , t ) , It() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/Tree.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Tree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Tree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
