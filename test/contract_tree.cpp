/*--------------------------------------------------------------------------*/
/*------------------------- File contract_tree.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the trees: the tree with
 * pointers and its visits, the binary tree, the vector of parents, the
 * binary trees in a positional vector and with cursors, the ordered trees
 * with cursors and in preorder, the binary search and Red-Black trees and
 * the B+-tree. For each, the violations of the preconditions, which must
 * throw ADS::contract_violation, and random operations at the level 3, so
 * that each one also checks its postconditions and the invariant of the
 * structure.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 5 and 6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/tree/ArrayBinaryTree.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/CursorBinaryTree.h"
#include "ADS/tree/CursorTree.h"
#include "ADS/tree/ParentTree.h"
#include "ADS/tree/PreorderArrayTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Tree.h"

#include <map>
#include <memory>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// TREE with pointers: the tree inserted must be a root other than the
// node, a root has no sibling, the visits need a node; random insertions
// and deletions, each checking that the children know their parent
ADS_TEST( "tree" )
{
 std::unique_ptr< Tree< int > > r( new Tree< int >( 0 ) );
 auto a = new Tree< int >( 1 );
 r->insertChild( a );
 CHECK( r->leftmostChild() == a && a->parent() == r.get() );
 Tree< int > b( 2 );
 CHECK_THROWS( r->insertChild( a ) , contract_violation );    // not a root
 CHECK_THROWS( r->insertSibling( &b ) , contract_violation ); // a root
 CHECK_THROWS( r->insertChild( r.get() ) , contract_violation );
 CHECK_THROWS( r->insertChild( nullptr ) , contract_violation );
 CHECK_THROWS( pre_visit( ( Tree< int > * ) nullptr , []( auto ) {} ) ,
               contract_violation );
 CHECK_THROWS( in_visit( r.get() , -1 , []( auto ) {} ) ,
               contract_violation );
 CHECK_THROWS( copy( ( const Tree< int > * ) nullptr ) ,
               contract_violation );
 std::mt19937 g( 1 );
 std::vector< Tree< int > * > N = { r.get() , a };
 for( int op = 0 ; op < 300 ; ++op ) {
  Tree< int > * u = N[ g() % N.size() ];
  if( ( g() % 2 ) || ! u->parent() ) {
   auto c = new Tree< int >( op );
   u->insertChild( c );
   N.push_back( c );
   }
  else {
   auto s = new Tree< int >( op );
   u->insertSibling( s );
   N.push_back( s );
   }
  }
 int n = 0;
 pre_visit( r.get() , [ & ]( Tree< int > * ) { ++n; } );
 CHECK( n == int( N.size() ) );
 std::unique_ptr< Tree< int > > c( copy( r.get() ) );
 int m = 0;
 bfs( c.get() , [ & ]( Tree< int > * ) { ++m; } );
 CHECK( m == n && ! c->parent() );
 r->deleteChild();
 }

// binary TREE with pointers: a child inserted where none is, a root other
// than the node; the places of a d-ary tree in their domain
ADS_TEST( "binary-tree" )
{
 std::unique_ptr< BinaryTree< int > > t( new BinaryTree< int >( 0 ) );
 t->insertLeft( new BinaryTree< int >( 1 ) );
 BinaryTree< int > x( 2 );
 CHECK_THROWS( t->insertLeft( &x ) , contract_violation );    // one there
 CHECK_THROWS( t->insertRight( t->left() ) , contract_violation );
 CHECK_THROWS( t->insertRight( t.get() ) , contract_violation );
 CHECK_THROWS( to_binary( ( const Tree< int > * ) nullptr ) ,
               contract_violation );
 CHECK_THROWS( dary_child( 0 , 2 , 0 ) , contract_violation );
 CHECK_THROWS( dary_child( 0 , 2 , 3 ) , contract_violation );
 CHECK_THROWS( dary_parent( 0 , 2 ) , contract_violation );
 CHECK_THROWS( dary_child( 1 , 0 , 1 ) , contract_violation );
 for( long d = 1 ; d <= 4 ; ++d )
  for( long i = 0 ; i < 50 ; ++i )
   for( long j = 1 ; j <= d ; ++j )
    CHECK( dary_parent( dary_child( i , d , j ) , d ) == i );
 std::unique_ptr< BinaryTree< int > > j( join( 7 , t.get() , t.get() ) );
 int n = 0;
 pre_visit( j.get() , [ & ]( BinaryTree< int > * ) { ++n; } );
 CHECK( n == 5 && j->left()->parent() == j.get() );
 }

// vector of parents: the nodes in [ 0 , n ), no node its own parent
ADS_TEST( "parent-tree" )
{
 CHECK_THROWS( ParentTree( -1 ) , contract_violation );
 ParentTree T( 4 );
 T.setParent( 1 , 0 );
 T.setParent( 2 , 1 );
 CHECK( T.parent( 2 ) == 1 && T.parent( 0 ) == -1 );
 CHECK_THROWS( (void) T.parent( 4 ) , contract_violation );
 CHECK_THROWS( T.setParent( 3 , 3 ) , contract_violation );
 CHECK_THROWS( T.setParent( 3 , 4 ) , contract_violation );
 CHECK_THROWS( T.setParent( -1 , 0 ) , contract_violation );
 T.setParent( 2 , -1 );
 CHECK( T.parent( 2 ) == -1 );
 }

// binary trees in a positional vector and with cursors, ordered trees with
// cursors and in preorder: the nodes must exist, the children be missing,
// the root have no sibling; random insertions and removals, each checking
// the invariant of the structure
ADS_TEST( "tree-realizations" )
{
 ArrayBinaryTree< int > A;
 CHECK_THROWS( (void) A.read( 0 ) , contract_violation );
 A.setRoot( 1 );
 CHECK_THROWS( A.setRoot( 2 ) , contract_violation );
 A.insertLeft( 0 , ArrayBinaryTree< int >( 2 ) );
 CHECK_THROWS( A.insertLeft( 0 , ArrayBinaryTree< int >( 3 ) ) ,
               contract_violation );
 CHECK_THROWS( A.insertLeft( 5 , ArrayBinaryTree< int >( 3 ) ) ,
               contract_violation );
 CHECK_THROWS( (void) A.left( 2 ) , contract_violation );   // no node 2
 CHECK_THROWS( (void) A.parent( 9 ) , contract_violation );
 CHECK( A.parent( A.left( 0 ) ) == 0 && A.right( 0 ) == A.NIL );
 CursorBinaryTree< int > B( 0 );
 CHECK_THROWS( B.setRoot( 1 ) , contract_violation );
 const int bl = B.insertLeft( B.root() , 1 );
 CHECK_THROWS( B.insertLeft( B.root() , 2 ) , contract_violation );
 CHECK_THROWS( (void) B.read( 99 ) , contract_violation );
 B.remove( bl );
 CHECK_THROWS( (void) B.read( bl ) , contract_violation );   // freed
 CursorTree< int > C( 0 );
 CHECK_THROWS( C.insertSibling( C.root() , 1 ) , contract_violation );
 CHECK_THROWS( (void) C.leftmostChild( 7 ) , contract_violation );
 PreorderArrayTree< int > P( 0 );
 CHECK_THROWS( P.insertSibling( 0 , PreorderArrayTree< int >( 1 ) ) ,
               contract_violation );
 CHECK_THROWS( P.write( 3 , 1 ) , contract_violation );
 std::mt19937 g( 2 );
 for( int op = 0 ; op < 400 ; ++op ) {
  const auto pre = B.preorder();
  const int u = pre[ g() % pre.size() ];
  if( ( B.left( u ) == B.NIL ) && ( g() % 3 ) ) B.insertLeft( u , op );
  else if( ( B.right( u ) == B.NIL ) && ( g() % 3 ) ) B.insertRight( u , op );
  else if( u != B.root() ) B.remove( u );
  const auto pc = C.previsit();
  const int v = pc[ g() % pc.size() ];
  if( ( v == C.root() ) || ( g() % 2 ) ) C.insertChild( v , op );
  else if( g() % 4 ) C.insertSibling( v , op );
  else C.remove( v );
  const int w = int( g() % P.size() );
  if( ( w == 0 ) || ( g() % 2 ) )
   P.insertChild( w , PreorderArrayTree< int >( op ) );
  else if( g() % 4 ) P.insertSibling( w , PreorderArrayTree< int >( op ) );
  else P.remove( w );
  const auto pa = A.preorder();
  const long i = pa[ g() % pa.size() ];
  if( ( A.left( i ) == A.NIL ) && ( i < 500 ) && ( g() % 2 ) )
   A.insertLeft( i , ArrayBinaryTree< int >( op ) );
  else if( ( A.right( i ) == A.NIL ) && ( i < 500 ) && ( g() % 2 ) )
   A.insertRight( i , ArrayBinaryTree< int >( op ) );
  else if( i != 0 ) A.remove( i );
  }
 CHECK( B.size() == int( B.preorder().size() ) );
 CHECK( C.size() == int( C.previsit().size() ) );
 }

// binary search and Red-Black trees: after insert() the key is there and
// after remove() it is not, and each operation checks the order of the
// keys and the constraints (V1)-(V4), against std::map
ADS_TEST( "search-tree" )
{
 std::mt19937 g( 3 );
 BinarySearchTree< int , int > S;
 RedBlackTree< int , int > R;
 std::map< int , int > M;
 CHECK_THROWS( (void) R.successorNode( nullptr ) , contract_violation );
 CHECK_THROWS( ( (void) RedBlackTree< int , int >::key( nullptr ) ) ,
               contract_violation );
 for( int op = 0 ; op < 3000 ; ++op ) {
  const int k = int( g() % 300 );
  if( g() % 3 ) {
   S.insert( k , op );
   R.insert( k , op );
   M[ k ] = op;
   }
  else {
   S.remove( k );
   R.remove( k );
   M.erase( k );
   }
  }
 CHECK( S.size() == M.size() && R.size() == M.size() );
 for( const auto & [ k , v ] : M )
  CHECK( S.lookup( k ) == v && R.lookup( k ) == v );
 }

// B+-tree: the order m > 2, and the shape after each insertion against
// std::map
ADS_TEST( "b-plus-tree" )
{
 CHECK_THROWS( ( BPlusTree< int , int >( 2 ) ) , contract_violation );
 std::mt19937 g( 4 );
 for( int m = 3 ; m <= 6 ; ++m ) {
  BPlusTree< int , int > T( m );
  std::map< int , int > M;
  for( int op = 0 ; op < 500 ; ++op ) {
   const int k = int( g() % 400 );
   T.insert( k , op );
   M[ k ] = op;
   }
  T.check();
  CHECK( T.size() == M.size() );
  for( const auto & [ k , v ] : M ) CHECK( T.lookup( k ) == v );
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File contract_tree.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
