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
 * copy() makes a deep copy of a subtree and join() a new root with the
 * copies of two trees as children, in time linear in their size.
 *
 * A complete d-ary tree, whose internal nodes have d children, is stored in
 * a vector, the root in the place 0 and the children of the node i in the
 * places \f$ d i + 1 , \ldots , d i + d \f$ [see dary_child()].
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.5 and
 *      5.6, https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

#include "ADS/tree/Tree.h"

#include <stdexcept>
#include <string>

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
 explicit BinaryTree( const T & v ) : f_value( v ) {}

 BinaryTree( const BinaryTree & ) = delete;
 BinaryTree & operator=( const BinaryTree & ) = delete;

 ~BinaryTree() {
  deleteLeft();
  deleteRight();
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const T & read( void ) const { return( f_value ); }

 void write( const T & v ) { f_value = v; }

 /// the parent; nil if the node is a root
 [[nodiscard]] BinaryTree * parent( void ) const { return( f_parent ); }

 /// the left child; nil if there is none
 [[nodiscard]] BinaryTree * left( void ) const { return( f_left ); }

 /// the right child; nil if there is none
 [[nodiscard]] BinaryTree * right( void ) const { return( f_right ); }

 /// inserts the tree t as the left child, which must be missing
 void insertLeft( BinaryTree * t ) { insert( f_left , t , "insertLeft" ); }

 /// inserts the tree t as the right child, which must be missing
 void insertRight( BinaryTree * t ) {
  insert( f_right , t , "insertRight" );
  }

 /// destroys the left subtree
 void deleteLeft( void ) {
  delete f_left;
  f_left = nullptr;
  }

 /// destroys the right subtree
 void deleteRight( void ) {
  delete f_right;
  f_right = nullptr;
  }

/*--------------------------------------------------------------------------*/
 private:

 void insert( BinaryTree * & child , BinaryTree * t , const char * who ) {
  if( ( ! t ) || t->f_parent )
   throw( std::logic_error( std::string( "ADS::BinaryTree::" ) + who +
                            ": the tree to insert is not a root" ) );
  if( child )
   throw( std::logic_error( std::string( "ADS::BinaryTree::" ) + who +
                            ": the child is there already" ) );
  t->f_parent = this;
  child = t;
  }

 T f_value;                       ///< the value
 BinaryTree * f_parent = nullptr; ///< the parent
 BinaryTree * f_left = nullptr;   ///< the left child
 BinaryTree * f_right = nullptr;  ///< the right child

 };  // end( class BinaryTree )

/*--------------------------------------------------------------------------*/
/*------------------------------- VISITS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// dfs(): pre( t ) before the subtrees, in( t ) between them, post( t )
/// after them; nothing on an empty tree
template< class T , class Pre , class In , class Post >
void dfs( BinaryTree< T > * t , Pre && pre , In && in , Post && post )
{
 if( t ) {
  pre( t );                              // (1)
  dfs( t->left() , pre , in , post );
  in( t );                               // (2)
  dfs( t->right() , pre , in , post );
  post( t );                             // (3)
  }
 }

/// the previsit of a binary tree
template< class T , class F >
void pre_visit( BinaryTree< T > * t , F && f )
{
 auto none = []( BinaryTree< T > * ) {};
 dfs( t , f , none , none );
 }

/// the invisit (symmetric visit) of a binary tree
template< class T , class F >
void in_visit( BinaryTree< T > * t , F && f )
{
 auto none = []( BinaryTree< T > * ) {};
 dfs( t , none , f , none );
 }

/// the postvisit of a binary tree
template< class T , class F >
void post_visit( BinaryTree< T > * t , F && f )
{
 auto none = []( BinaryTree< T > * ) {};
 dfs( t , none , none , f );
 }

/// a deep copy of the subtree t, nil if t is nil; the caller owns it
template< class T >
BinaryTree< T > * copy( const BinaryTree< T > * t )
{
 if( ! t ) return( nullptr );
 auto b = new BinaryTree< T >( t->read() );
 if( auto l = copy( t->left() ) ) b->insertLeft( l );
 if( auto r = copy( t->right() ) ) b->insertRight( r );
 return( b );
 }

/// a new root of value v with copies of l and r, each possibly nil, as its
/// children; the caller owns it
template< class T >
BinaryTree< T > * join( const T & v , const BinaryTree< T > * l ,
                        const BinaryTree< T > * r )
{
 auto b = new BinaryTree< T >( v );
 if( auto x = copy( l ) ) b->insertLeft( x );
 if( auto y = copy( r ) ) b->insertRight( y );
 return( b );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- REPRESENTATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the binary tree that represents the ordered tree t (Figura 5.9): the left
/// child of a node is its first child in t, the right one its next sibling;
/// the caller owns the result
template< class T >
BinaryTree< T > * to_binary( const Tree< T > * t )
{
 auto b = new BinaryTree< T >( t->read() );
 if( auto c = t->leftmostChild() )
  b->insertLeft( to_binary( c ) );
 if( auto s = t->rightSibling() )
  b->insertRight( to_binary( s ) );
 return( b );
 }

/// the place of the j-th child, 1 <= j <= d, of the node in the place i of
/// a complete d-ary tree stored in a vector
inline long dary_child( long i , long d , long j ) { return( d * i + j ); }

/// the place of the parent of the node in the place i > 0
inline long dary_parent( long i , long d ) { return( ( i - 1 ) / d ); }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* BinaryTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BinaryTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
