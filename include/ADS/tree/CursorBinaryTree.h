/*--------------------------------------------------------------------------*/
/*------------------------ File CursorBinaryTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A binary tree with cursors, beyond the book: the nodes in a vector,
 * each with its value and the places of its parent and of its two
 * children, NIL if missing, so that the links are indices instead of
 * pointers and the tree can be copied, saved or moved as a whole. A
 * removed subtree returns its places to a free list, which the next
 * insertions reuse. Moving along the tree, reading, writing and inserting a
 * node take O( 1 ) amortized time; removing or copying a subtree takes time
 * linear in its size.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_CursorBinaryTree
 #define __ADS_CursorBinaryTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CursorBinaryTree -------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary tree on a vector of nodes linked by their places
template< class T >
class CursorBinaryTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 CursorBinaryTree( void ) = default;

 /// a tree of one node of value v
 explicit CursorBinaryTree( const T & v ) { f_root = make( v , NIL ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_root == NIL ); }

 [[nodiscard]] int root( void ) const { return( f_root ); }

 [[nodiscard]] int parent( int n ) const { return( node( n ).parent ); }

 [[nodiscard]] int left( int n ) const { return( node( n ).left ); }

 [[nodiscard]] int right( int n ) const { return( node( n ).right ); }

 [[nodiscard]] bool isLeaf( int n ) const {
  return( ( left( n ) == NIL ) && ( right( n ) == NIL ) );
  }

 [[nodiscard]] const T & read( int n ) const { return( node( n ).value ); }

 void write( int n , const T & v ) { node( n ).value = v; }

 /// the number of nodes
 [[nodiscard]] int size( void ) const { return( f_size ); }

 /// the root of value v of the empty tree
 int setRoot( const T & v ) {
  if( f_root != NIL )
   throw( std::logic_error( "ADS::CursorBinaryTree::setRoot: not empty" ) );
  return( f_root = make( v , NIL ) );
  }

 /// a new left child of n of value v, which must be missing
 int insertLeft( int n , const T & v ) {
  check_free( node( n ).left , "insertLeft" );
  const int c = make( v , n );
  return( f_N[ n ].left = c );
  }

 /// a new right child of n of value v, which must be missing
 int insertRight( int n , const T & v ) {
  check_free( node( n ).right , "insertRight" );
  const int c = make( v , n );
  return( f_N[ n ].right = c );
  }

 /// a copy of the subtree of t in s as the left subtree of n
 int insertLeft( int n , const CursorBinaryTree & t , int s ) {
  check_free( node( n ).left , "insertLeft" );
  const int c = copy( t , s , n );
  return( f_N[ n ].left = c );
  }

 /// a copy of the subtree of t in s as the right subtree of n
 int insertRight( int n , const CursorBinaryTree & t , int s ) {
  check_free( node( n ).right , "insertRight" );
  const int c = copy( t , s , n );
  return( f_N[ n ].right = c );
  }

 /// removes the subtree of n, detached from its parent
 void remove( int n ) {
  const int p = node( n ).parent;
  if( p == NIL ) f_root = NIL;
  else if( f_N[ p ].left == n ) f_N[ p ].left = NIL;
  else f_N[ p ].right = NIL;
  release( n );
  }

 /// the nodes in preorder
 [[nodiscard]] std::vector< int > preorder( void ) const {
  std::vector< int > P;
  auto rec = [ & ]( auto & self , int n ) -> void {
   if( n == NIL ) return;
   P.push_back( n );
   self( self , f_N[ n ].left );
   self( self , f_N[ n ].right );
   };
  rec( rec , f_root );
  return( P );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  T value;
  int parent , left , right;
  bool used;
  };

 const Node & node( int n ) const {
  if( ( n < 0 ) || ( n >= int( f_N.size() ) ) || ! f_N[ n ].used )
   throw( std::out_of_range( "ADS::CursorBinaryTree: no node" ) );
  return( f_N[ n ] );
  }

 Node & node( int n ) {
  return( const_cast< Node & >( std::as_const( *this ).node( n ) ) );
  }

 static void check_free( int c , const char * who ) {
  if( c != NIL )
   throw( std::logic_error( std::string( "ADS::CursorBinaryTree::" ) + who +
                            ": child present" ) );
  }

 int make( const T & v , int p ) {
  ++f_size;
  if( f_free != NIL ) {
   const int n = f_free;
   f_free = f_N[ n ].left;
   f_N[ n ] = { v , p , NIL , NIL , true };
   return( n );
   }
  f_N.push_back( { v , p , NIL , NIL , true } );
  return( int( f_N.size() ) - 1 );
  }

 void release( int n ) {
  if( n == NIL ) return;
  release( f_N[ n ].left );
  release( f_N[ n ].right );
  f_N[ n ].used = false;
  f_N[ n ].left = f_free;                // the free list
  f_free = n;
  --f_size;
  }

 int copy( const CursorBinaryTree & t , int s , int p ) {
  if( s == NIL ) return( NIL );
  const int n = make( t.node( s ).value , p );
  const int l = copy( t , t.f_N[ s ].left , n );
  const int r = copy( t , t.f_N[ s ].right , n );
  f_N[ n ].left = l;
  f_N[ n ].right = r;
  return( n );
  }

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;
 int f_free = NIL;          ///< the first free place
 int f_size = 0;

 };  // end( class CursorBinaryTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* CursorBinaryTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File CursorBinaryTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
