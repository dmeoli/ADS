/*--------------------------------------------------------------------------*/
/*--------------------------- File BinaryTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary TREE of the book: an ordered tree where each node has at most
 * two children, told apart as the left and the right one, realized with
 * pointers to the parent and to the two children. Moving along the tree,
 * reading, writing and inserting take O( 1 ) time, deleting a subtree time
 * linear in its size. As for Tree, a node and its subtree are the same
 * thing, a tree owns its subtrees (a tree whose insertion is refused stays
 * the caller's), and nil is nullptr.
 *
 * The visit dfs() examines a node before its subtrees (previsit), between
 * them (invisit, or symmetric visit) or after them (postvisit). An ordered
 * tree T is represented by the binary tree B whose left child of a node is
 * its first child in T and whose right child is its next sibling in T
 * [see to_binary()]: the previsits of T and of B are the same, and the
 * postvisit of T is the invisit of B.
 *
 * detachLeft() and detachRight() cut a subtree off its parent without
 * destroying it, in O( 1 ) time: the subtree becomes a root owned by the
 * caller, ready to be inserted elsewhere, which moves subtrees around (a
 * mirror, a rotation) with no copy.
 *
 * copy() makes a deep copy of a subtree and join() a new root with the
 * copies of two trees as children, in time linear in their size.
 *
 * The ranges preorder(), inorder() and postorder() walk the nodes of a
 * subtree by the links to the parents, with no stack and no recursion, in
 * O( n ) time overall and O( 1 ) extra space: each node is left at most
 * once from each of its three visits.
 *
 * to_list() writes a binary tree as a parenthesized list, () for the empty
 * tree and ( v L R ) for the root v with the subtrees L and R, e.g.,
 * ( 8 () ( 5 ( 27 () () ) ( 16 () () ) ) ), and from_list() reads it back;
 * the values are written with << and read with >>, and must contain no
 * space nor parenthesis.
 *
 * A complete d-ary tree, whose internal nodes have d children, is stored in
 * a vector, the root in the place 0 and the children of the node i in the
 * places \f$ d i + 1 , \ldots , d i + d \f$ [see dary_child()].
 *
 * The contracts [see Contract.h]: the tree inserted is a root other than
 * the node itself and goes where no child is; the subtree detached is a
 * root and its place is empty; the invariant is that each child has its
 * parent as parent, down the subtree; the places of a d-ary
 * tree need d >= 1, a child 1 <= j <= d and a place i > 0 to have a
 * parent. The operations are defined in BinaryTree.tpp.
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

 /// Tree( v ): a new tree, made of one node containing v
 explicit BinaryTree( const T & v );

 BinaryTree( const BinaryTree & ) = delete;
 BinaryTree & operator=( const BinaryTree & ) = delete;

 ~BinaryTree();

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const T & read( void ) const;

 void write( const T & v );

 /// the parent; nil if the node is a root
 [[nodiscard]] BinaryTree * parent( void ) const;

 /// the left child; nil if there is none
 [[nodiscard]] BinaryTree * left( void ) const;

 /// the right child; nil if there is none
 [[nodiscard]] BinaryTree * right( void ) const;

 /// inserts the tree t as the left child, which must be missing
 /// \pre t is a root, not this node nor the root of its tree (which would
 ///      make a cycle), and the left child is missing
 /// \post t is the left child
 void insertLeft( BinaryTree * t );

 /// inserts the tree t as the right child, which must be missing
 /// \pre t is a root, not this node nor the root of its tree (which would
 ///      make a cycle), and the right child is missing
 /// \post t is the right child
 void insertRight( BinaryTree * t );

 /// destroys the left subtree
 void deleteLeft( void );

 /// destroys the right subtree
 void deleteRight( void );

 /// detaches the left subtree without destroying it: it becomes a root,
 /// owned by the caller; nil if there is none
 /// \post the left child is missing and the result is nil or a root
 [[nodiscard]] BinaryTree * detachLeft( void );

 /// detaches the right subtree without destroying it: it becomes a root,
 /// owned by the caller; nil if there is none
 /// \post the right child is missing and the result is nil or a root
 [[nodiscard]] BinaryTree * detachRight( void );

/*--------------------------------------------------------------------------*/
 private:

 void insert( BinaryTree * & child , BinaryTree * t );

 BinaryTree * detach( BinaryTree * & child );

 /// each child of this node has it as its parent (the links that a
 /// change of the tree touches), O( 1 ) per child
 [[nodiscard]] bool linked( void ) const;

 /// the root of the tree of this node, O( depth )
 [[nodiscard]] const BinaryTree * top( void ) const;

 T f_value;                       ///< the value
 BinaryTree * f_parent = nullptr; ///< the parent
 BinaryTree * f_left = nullptr;   ///< the left child
 BinaryTree * f_right = nullptr;  ///< the right child

 };  // end( class BinaryTree )

/*--------------------------------------------------------------------------*/
/*------------------------------- VISITS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// dfs(): pre( t ) before the subtrees, in( t ) between them, post( t )
/// after them; nothing on an empty tree. It runs with a stack of its own,
/// not by recursion, so that the height of the tree may be n
template< class T , class Pre , class In , class Post >
void dfs( BinaryTree< T > * t , Pre && pre , In && in , Post && post );

/// the previsit of a binary tree
template< class T , class F >
void pre_visit( BinaryTree< T > * t , F && f );

/// the invisit (symmetric visit) of a binary tree
template< class T , class F >
void in_visit( BinaryTree< T > * t , F && f );

/// the postvisit of a binary tree
template< class T , class F >
void post_visit( BinaryTree< T > * t , F && f );

/// a deep copy of the subtree t, nil if t is nil; the caller owns it
template< class T >
BinaryTree< T > * copy( const BinaryTree< T > * t );

/// a new root of value v with copies of l and r, each possibly nil, as its
/// children; the caller owns it
template< class T >
BinaryTree< T > * join( const T & v , const BinaryTree< T > * l ,
                        const BinaryTree< T > * r );

/*--------------------------------------------------------------------------*/
/*--------------------------- REPRESENTATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the parenthesized list of t, () if t is nil
template< class T >
std::string to_list( const BinaryTree< T > * t );

/// the binary tree of the parenthesized list s, nil for (); the caller owns
/// it
/// \pre s is the list of a binary tree, and >> reads each atom whole
///      (e.g., 8x is not an int)
template< class T >
BinaryTree< T > * from_list( std::string_view s );

/// the binary tree that represents the ordered tree t (Figura 5.9): the left
/// child of a node is its first child in t, the right one its next sibling;
/// the caller owns the result
/// \pre t is a node
template< class T >
BinaryTree< T > * to_binary( const Tree< T > * t );

/// the place of the j-th child, 1 <= j <= d, of the node in the place i of
/// a complete d-ary tree stored in a vector
/// \pre i >= 0 , d >= 1 , 1 <= j <= d , and d i + j fits in a long
inline long dary_child( long i , long d , long j );

/// the place of the parent of the node in the place i > 0
/// \pre i > 0 , d >= 1
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

/// the nodes of the subtree of t in preorder, t first
template< class T >
auto preorder( BinaryTree< T > * t )
{
 using It = WalkIterator< void , BinaryTree< T > * ,
                          detail::BinaryPreorder >;
 return( std::ranges::subrange( It( nullptr , t , t ) , It() ) );
 }

/// the nodes of the subtree of t in symmetric order
template< class T >
auto inorder( BinaryTree< T > * t )
{
 using It = WalkIterator< void , BinaryTree< T > * , detail::BinaryInorder >;
 return( std::ranges::subrange(
          It( nullptr , detail::BinaryInorder::leftmost( t ) , t ) , It() ) );
 }

/// the nodes of the subtree of t in postorder, t last
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
