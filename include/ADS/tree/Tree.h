/*--------------------------------------------------------------------------*/
/*------------------------------ File Tree.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered TREE of the book, realized with pointers to the parent, to
 * the first child and to the next sibling, and its visits.
 *
 * A node and the subtree rooted in it are the same thing, a Tree: the
 * constructor makes an isolated node, insertChild() and insertSibling()
 * hang a whole tree under a node in O( 1 ) time, deleteChild() and
 * deleteSibling() destroy a subtree in time linear in its size. A tree owns
 * its subtrees: deleting the root of a tree destroys all of it; a tree whose
 * insertion is refused (an exception is thrown) stays the caller's. As in
 * the book, nil is nullptr.
 *
 * The visits examine each node once, in \f$ \Theta( n ) \f$ time: the
 * previsit examines the root and then visits the subtrees
 * \f$ T_0 , \ldots , T_{k-1} \f$ of its children in order, the postvisit
 * visits the subtrees and then examines the root, the invisit with index i
 * visits the first i subtrees, examines the root and visits the others, and
 * the visit by levels (bfs) examines all the nodes of level l before those
 * of level l + 1, using a queue. copy() makes a deep copy of a subtree.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1, 5.2
 *      and 5.3, https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_Tree
 #define __ADS_Tree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.h"

#include <stdexcept>

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
 explicit Tree( const T & v ) : f_value( v ) {}

 Tree( const Tree & ) = delete;
 Tree & operator=( const Tree & ) = delete;

 /// destroys the subtrees of the children, i.e., the whole tree
 ~Tree() {
  while( f_child )
   deleteChild();
  }

/*--------------------------------------------------------------------------*/

 /// the value of the node
 [[nodiscard]] const T & read( void ) const { return( f_value ); }

 /// writes v in the node
 void write( const T & v ) { f_value = v; }

 /// the parent; nil if the node is a root
 [[nodiscard]] Tree * parent( void ) const { return( f_parent ); }

 /// the first child; nil if the node is a leaf
 [[nodiscard]] Tree * leftmostChild( void ) const { return( f_child ); }

 /// the next sibling; nil if there is none
 [[nodiscard]] Tree * rightSibling( void ) const { return( f_sibling ); }

 /// inserts the tree t as the first child of this node, which then owns it
 void insertChild( Tree * t ) {
  check_root( t , "insertChild" );
  t->f_parent = this;
  t->f_sibling = f_child;
  f_child = t;
  }

 /// inserts the tree t as the next sibling of this node, which must not be
 /// a root; the parent of this node then owns t
 void insertSibling( Tree * t ) {
  check_root( t , "insertSibling" );
  if( ! f_parent )
   throw( std::logic_error( "ADS::Tree::insertSibling: a root has no "
                            "siblings" ) );
  t->f_parent = f_parent;
  t->f_sibling = f_sibling;
  f_sibling = t;
  }

 /// destroys the subtree rooted in the first child
 void deleteChild( void ) {
  if( Tree * c = f_child ) {
   f_child = c->f_sibling;
   c->f_sibling = nullptr;
   delete c;
   }
  }

 /// destroys the subtree rooted in the next sibling
 void deleteSibling( void ) {
  if( Tree * s = f_sibling ) {
   f_sibling = s->f_sibling;
   s->f_sibling = nullptr;
   delete s;
   }
  }

/*--------------------------------------------------------------------------*/
 private:

 static void check_root( const Tree * t , const char * who ) {
  if( ( ! t ) || t->f_parent )
   throw( std::logic_error( std::string( "ADS::Tree::" ) + who +
                            ": the tree to insert is not a root" ) );
  }

 T f_value;                    ///< the value
 Tree * f_parent = nullptr;    ///< the parent
 Tree * f_child = nullptr;     ///< the first child
 Tree * f_sibling = nullptr;   ///< the next sibling

 };  // end( class Tree )

/*--------------------------------------------------------------------------*/
/*------------------------------- VISITS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// dfs(): pre( u ) is called on each node before its subtrees are visited,
/// post( u ) after; pre-visit() and post-visit() are dfs() with one of them
template< class T , class Pre , class Post >
void dfs( Tree< T > * t , Pre && pre , Post && post )
{
 pre( t );                                    // (1)
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() )
  dfs( u , pre , post );
 post( t );                                   // (2)
 }

/// the previsit: f( u ) on the root, then on the subtrees in order
template< class T , class F >
void pre_visit( Tree< T > * t , F && f )
{
 dfs( t , f , []( Tree< T > * ) {} );
 }

/// the postvisit: f( u ) on the subtrees in order, then on the root
template< class T , class F >
void post_visit( Tree< T > * t , F && f )
{
 dfs( t , []( Tree< T > * ) {} , f );
 }

/// in-visit(): the first i subtrees, the root, the other subtrees
template< class T , class F >
void in_visit( Tree< T > * t , int i , F && f )
{
 auto u = t->leftmostChild();
 for( int k = 0 ; u && ( k < i ) ; ++k , u = u->rightSibling() )
  in_visit( u , i , f );
 f( t );
 for( ; u ; u = u->rightSibling() )
  in_visit( u , i , f );
 }

/// a deep copy of the subtree t and of its children, not of its siblings;
/// the caller owns it
template< class T >
Tree< T > * copy( const Tree< T > * t )
{
 auto c = new Tree< T >( t->read() );
 Tree< T > * last = nullptr;
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() ) {
  Tree< T > * x = copy( u );
  if( last ) last->insertSibling( x );
  else c->insertChild( x );
  last = x;
  }
 return( c );
 }

/// bfs(): the visit by levels, with a queue of the nodes to examine
template< class T , class F >
void bfs( Tree< T > * t , F && f )
{
 List< Tree< T > * > Q;   // enqueue at the tail, dequeue from the head
 Q.insert( Q.next( Q.tail() ) , t );
 while( ! Q.isEmpty() ) {
  Tree< T > * u = Q.read( Q.head() );
  Q.remove( Q.head() );
  f( u );
  for( u = u->leftmostChild() ; u ; u = u->rightSibling() )
   Q.insert( Q.next( Q.tail() ) , u );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Tree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Tree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
