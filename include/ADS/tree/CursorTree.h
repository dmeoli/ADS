/*--------------------------------------------------------------------------*/
/*--------------------------- File CursorTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree with cursors, beyond the book: the nodes in a vector,
 * each with its value and the places of its parent, of its first child and
 * of its next sibling, NIL if missing, as the Tree with pointers [see
 * Tree.h] but with indices. A removed subtree returns its places to a free
 * list. insertChild() and insertSibling() hang a new node, insertChildTree()
 * and insertSiblingTree() a copy of a subtree of another tree, as first
 * child or as next sibling; the visits are those of the book.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1-5.3,
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

#ifndef __ADS_CursorTree
 #define __ADS_CursorTree
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
/*---------------------------- CLASS CursorTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree on a vector of nodes linked by their places
template< class T >
class CursorTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 CursorTree( void ) = default;

 /// a tree of one node of value v
 explicit CursorTree( const T & v ) { f_root = make( v , NIL ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_root == NIL ); }

 [[nodiscard]] int root( void ) const { return( f_root ); }

 [[nodiscard]] int parent( int n ) const { return( node( n ).parent ); }

 [[nodiscard]] int leftmostChild( int n ) const { return( node( n ).child ); }

 [[nodiscard]] int rightSibling( int n ) const {
  return( node( n ).sibling );
  }

 [[nodiscard]] bool isLeaf( int n ) const { return( node( n ).child == NIL ); }

 /// whether n is the last of its siblings
 [[nodiscard]] bool isLastSibling( int n ) const {
  return( node( n ).sibling == NIL );
  }

 [[nodiscard]] const T & read( int n ) const { return( node( n ).value ); }

 void write( int n , const T & v ) { node( n ).value = v; }

 [[nodiscard]] int size( void ) const { return( f_size ); }

 /// a new first child of n of value v
 int insertChild( int n , const T & v ) {
  const int c = make( v , n );
  f_N[ c ].sibling = node( n ).child;
  return( f_N[ n ].child = c );
  }

 /// a new next sibling of n, not the root, of value v
 int insertSibling( int n , const T & v ) {
  const int p = check_sibling( n );
  const int s = make( v , p );
  f_N[ s ].sibling = f_N[ n ].sibling;
  return( f_N[ n ].sibling = s );
  }

 /// a copy of the subtree of t in s as the first child of n
 int insertChildTree( int n , const CursorTree & t , int s ) {
  node( n );
  const int c = copy( t , s , n );
  f_N[ c ].sibling = f_N[ n ].child;
  return( f_N[ n ].child = c );
  }

 /// a copy of the subtree of t in s as the next sibling of n
 int insertSiblingTree( int n , const CursorTree & t , int s ) {
  const int p = check_sibling( n );
  const int c = copy( t , s , p );
  f_N[ c ].sibling = f_N[ n ].sibling;
  return( f_N[ n ].sibling = c );
  }

 /// removes the subtree of n, detached from its parent
 void remove( int n ) {
  const int p = node( n ).parent;
  if( p == NIL ) f_root = NIL;
  else if( f_N[ p ].child == n ) f_N[ p ].child = f_N[ n ].sibling;
  else {
   int u = f_N[ p ].child;
   while( f_N[ u ].sibling != n ) u = f_N[ u ].sibling;
   f_N[ u ].sibling = f_N[ n ].sibling;
   }
  release_children( n );
  free_node( n );
  }

 /// the nodes in previsit, postvisit, and invisit with index i
 [[nodiscard]] std::vector< int > previsit( void ) const {
  std::vector< int > P;
  visit( f_root , -1 , P , 0 );
  return( P );
  }

 [[nodiscard]] std::vector< int > postvisit( void ) const {
  std::vector< int > P;
  visit( f_root , -1 , P , 1 );
  return( P );
  }

 [[nodiscard]] std::vector< int > invisit( int i ) const {
  std::vector< int > P;
  visit( f_root , i , P , 2 );
  return( P );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  T value;
  int parent , child , sibling;
  bool used;
  };

 const Node & node( int n ) const {
  if( ( n < 0 ) || ( n >= int( f_N.size() ) ) || ! f_N[ n ].used )
   throw( std::out_of_range( "ADS::CursorTree: no node" ) );
  return( f_N[ n ] );
  }

 Node & node( int n ) {
  return( const_cast< Node & >( std::as_const( *this ).node( n ) ) );
  }

 int check_sibling( int n ) const {
  const int p = node( n ).parent;
  if( p == NIL )
   throw( std::logic_error( "ADS::CursorTree: the root has no sibling" ) );
  return( p );
  }

 int make( const T & v , int p ) {
  ++f_size;
  if( f_free != NIL ) {
   const int n = f_free;
   f_free = f_N[ n ].sibling;
   f_N[ n ] = { v , p , NIL , NIL , true };
   return( n );
   }
  f_N.push_back( { v , p , NIL , NIL , true } );
  return( int( f_N.size() ) - 1 );
  }

 void free_node( int n ) {
  f_N[ n ].used = false;
  f_N[ n ].sibling = f_free;
  f_free = n;
  --f_size;
  }

 void release_children( int n ) {
  for( int u = f_N[ n ].child ; u != NIL ; ) {
   const int next = f_N[ u ].sibling;
   release_children( u );
   free_node( u );
   u = next;
   }
  }

 /// a copy of the node s of t, with its children, under p
 int copy( const CursorTree & t , int s , int p ) {
  const int n = make( t.node( s ).value , p );
  int last = NIL;
  for( int u = t.f_N[ s ].child ; u != NIL ; u = t.f_N[ u ].sibling ) {
   const int c = copy( t , u , n );
   if( last == NIL ) f_N[ n ].child = c;
   else f_N[ last ].sibling = c;
   last = c;
   }
  return( n );
  }

 /// kind 0 previsit, 1 postvisit, 2 invisit with index i
 void visit( int n , int i , std::vector< int > & P , int kind ) const {
  if( n == NIL ) return;
  if( kind == 0 ) P.push_back( n );
  int k = 0;
  for( int u = f_N[ n ].child ; u != NIL ; u = f_N[ u ].sibling , ++k ) {
   if( ( kind == 2 ) && ( k == i ) ) P.push_back( n );
   visit( u , i , P , kind );
   }
  if( ( kind == 1 ) || ( ( kind == 2 ) && ( k <= i ) ) ) P.push_back( n );
  }

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;
 int f_free = NIL;          ///< the first free place
 int f_size = 0;

 };  // end( class CursorTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* CursorTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CursorTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
