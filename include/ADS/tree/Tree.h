/*--------------------------------------------------------------------------*/
/*------------------------------ File Tree.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered TREE of the book, realized with pointers to the parent, to
 * the first child and to the next sibling, and its visits.
 *
 * A node and the subtree rooted in it are the same thing, a Tree: the
 * constructor makes an isolated node, insertChild() and insertSibling()
 * hang a whole tree under a node in \f$ O( 1 ) \f$ time, deleteChild() and
 * deleteSibling() destroy a subtree in time linear in its size. A tree owns
 * its subtrees: deleting the root of a tree destroys all of it; a tree whose
 * insertion is refused (its contract is violated) stays the caller's. As
 * in the book, nil is nullptr.
 *
 * The visits examine each node once, in \f$ \Theta( n ) \f$ time: the
 * previsit examines the root and then visits the subtrees
 * \f$ T_0 , \ldots , T_{k-1} \f$ of its children in order, the postvisit
 * visits the subtrees and then examines the root, the invisit with index i
 * visits the first i subtrees, examines the root and visits the others, and
 * the visit by levels (bfs) examines all the nodes of level l before those
 * of level l + 1, using a queue. copy() makes a deep copy of a subtree.
 *
 * The contracts [see Contract.h]: the tree inserted is a root (the
 * precondition of the book, t.parent() = nil) other than the node itself,
 * insertSibling() needs a node that is not a root, and the visits a node;
 * the invariant is that each child of a node has that node as its parent,
 * down the subtree. The operations are defined in Tree.tpp.
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Tree ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree, with pointers to parent, first child and next sibling
template< class T >
class Tree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the values

 /// Tree( v ): a new tree, made of one node containing v
 explicit Tree( const T & v );

 Tree( const Tree & ) = delete;
 Tree & operator=( const Tree & ) = delete;

 /// destroys the subtrees of the children, i.e., the whole tree, with a
 /// stack of its own rather than by recursion
 ~Tree();

/*--------------------------------------------------------------------------*/

 /// the value of the node
 [[nodiscard]] const T & read( void ) const;

 /// writes v in the node
 void write( const T & v );

 /// the parent; nil if the node is a root
 [[nodiscard]] Tree * parent( void ) const;

 /// the first child; nil if the node is a leaf
 [[nodiscard]] Tree * leftmostChild( void ) const;

 /// the next sibling; nil if there is none
 [[nodiscard]] Tree * rightSibling( void ) const;

 /// inserts the tree t as the first child of this node, which then owns it
 /// \pre t is a root, not this node nor the root of its tree (which would
 ///      make a cycle)
 /// \post t is the first child
 void insertChild( Tree * t );

 /// inserts the tree t as the next sibling of this node, which must not be
 /// a root; the parent of this node then owns t
 /// \pre t is a root, and this node is not; t is not the root of the tree
 ///      of this node (which would make a cycle)
 /// \post t is the next sibling
 void insertSibling( Tree * t );

 /// true if the node has no child (FOGLIA)
 [[nodiscard]] bool isLeaf( void ) const;

 /// true if the node has no next sibling (ULTIMOFRATELLO)
 [[nodiscard]] bool isLastSibling( void ) const;

 /// destroys the subtree rooted in the first child
 void deleteChild( void );

 /// destroys the subtree rooted in the next sibling
 void deleteSibling( void );

/*--------------------------------------------------------------------------*/
 private:

 /// each child of this node has it as its parent (the links that a
 /// change of the tree touches), \f$ O( 1 ) \f$ per child
 [[nodiscard]] bool linked( void ) const;

 /// the root of the tree of this node, \f$ O( \mathit{depth} ) \f$
 [[nodiscard]] const Tree * top( void ) const;

 T f_value;                    ///< the value
 Tree * f_parent = nullptr;    ///< the parent
 Tree * f_child = nullptr;     ///< the first child
 Tree * f_sibling = nullptr;   ///< the next sibling

 };  // end( class Tree )

/*--------------------------------------------------------------------------*/
/*------------------------------- VISITS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// dfs(): pre( u ) is called on each node before its subtrees are visited,
/// post( u ) after; pre-visit() and post-visit() are dfs() with one of them.
/// It and in_visit() run with a stack of their own, not by recursion, so
/// that the height of the tree may be n
/// \pre t is a node
template< class T , class Pre , class Post >
void dfs( Tree< T > * t , Pre && pre , Post && post );

/// the previsit: f( u ) on the root, then on the subtrees in order
template< class T , class F >
void pre_visit( Tree< T > * t , F && f );

/// the postvisit: f( u ) on the subtrees in order, then on the root
template< class T , class F >
void post_visit( Tree< T > * t , F && f );

/// in-visit(): the first i subtrees, the root, the other subtrees
/// \pre t is a node and i >= 0
template< class T , class F >
void in_visit( Tree< T > * t , int i , F && f );

/// a deep copy of the subtree t and of its children, not of its siblings;
/// the caller owns it
/// \pre t is a node
/// \post the copy is a root
template< class T >
Tree< T > * copy( const Tree< T > * t );

/// bfs(): the visit by levels, with a queue of the nodes to examine
template< class T , class F >
void bfs( Tree< T > * t , F && f );

/// the first level of largest weight, the weight of a level being the sum
/// of the values of its nodes (the root at level 0), by a visit by levels
/// \pre t is a node
template< class T >
int maxWeightLevel( Tree< T > * t );

/// the parenthesized list of t, ( r T1 ... Tk ) for the root r with the
/// subtrees T1 , ... , Tk, e.g., ( a ( b ( e ) ( f ) ) ( c ( g ) ) ); the
/// values are written with << and must contain no space nor parenthesis
/// \pre t is a node
template< class T >
std::string to_list( const Tree< T > * t );

/// the tree of the parenthesized list s, its values read with >>; the
/// caller owns it
/// \pre s is the list of a tree, and >> reads each atom whole (e.g., 8x
///      is not an int)
template< class T >
Tree< T > * tree_from_list( std::string_view s );

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------- THE STANDARD INTERFACE -------------------------*/
/*--------------------------------------------------------------------------*/

// the nodes of a tree as ranges, forward, by the links to the parents and
// the siblings, with no stack; *it is a node

namespace detail {

/// the walk of the children of a node
struct TreeChildren {
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * next( const void * , N * n , N * ) {
  return( n->rightSibling() );
  }
 };

/// the preorder below a root
struct TreePreorder {
 static constexpr std::nullptr_t none = nullptr;
 template< class N >
 static N * next( const void * , N * n , N * root ) {
  if( n->leftmostChild() ) return( n->leftmostChild() );
  for( ; n != root ; n = n->parent() )      // up to a right sibling
   if( n->rightSibling() ) return( n->rightSibling() );
  return( nullptr );
  }
 };

}  // end( namespace detail )

/// the children of t, from the leftmost
template< class T >
auto children( Tree< T > * t )
{
 using It = WalkIterator< void , Tree< T > * , detail::TreeChildren >;
 return( std::ranges::subrange( It( nullptr , t->leftmostChild() , t ) ,
                                It() ) );
 }

/// the nodes of the subtree of t in preorder, t first
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

#endif  /* Tree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Tree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
