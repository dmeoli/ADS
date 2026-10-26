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
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/tree/ArrayBinaryTree.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/ChildListTree.h"
#include "ADS/tree/CursorBinaryTree.h"
#include "ADS/tree/CursorTree.h"
#include "ADS/tree/ExpressionTree.h"
#include "ADS/tree/HeapByInsertion.h"
#include "ADS/tree/ParentTree.h"
#include "ADS/tree/PreorderArrayTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Tree.h"

#include <limits>
#include <map>
#include <memory>
#include <random>
#include <string>
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
 CHECK_THROWS( ( BPlusTree< int , int >( 0 ) ) , contract_violation );
 BPlusTree< int , int > E( 3 );               // the least order, empty
 CHECK( E.size() == 0 && ! E.lookup( 1 ) );
 }

// the other trees, each bound just inside and just outside: the B-tree of
// t = 2 and 1, the d-ary places at the largest i and one past it, the
// lowest common ancestor of nil, of one node and of two trees, a range
// with hi < lo and lo = hi, keys not increasing, preorder and inorder of
// other sizes or values, the nodes of the child lists and of the cursors,
// the expressions not well formed, a variable with no value, a division
// by 0, the sift up at n, the lists of the trees not well formed, the
// visits of nil
ADS_TEST( "other-trees" )
{
 CHECK_THROWS( ( BTree< int , int >( 1 ) ) , contract_violation );
 BTree< int , int > T2( 2 );
 for( int k = 0 ; k < 50 ; ++k ) T2.insert( k , -k );
 CHECK( T2.lookup( 49 ) == -49 && ! T2.lookup( 50 ) );

 const long d = 3 , top = ( std::numeric_limits< long >::max() - 3 ) / 3;
 CHECK( dary_child( top , d , 3 ) <= std::numeric_limits< long >::max() );
 CHECK_THROWS( dary_child( top + 1 , d , 3 ) , contract_violation );
 CHECK_THROWS( dary_child( -1 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( dary_parent( 3 , 0 ) , contract_violation );
 CHECK( dary_parent( 1 , 1 ) == 0 && dary_child( 0 , 1 , 1 ) == 1 );

 using B = BinaryTree< int >;
 std::unique_ptr< B > r( new B( 5 ) ) , o( new B( 9 ) );
 r->insertLeft( new B( 3 ) );
 r->insertRight( new B( 8 ) );
 CHECK( lowestCommonAncestor( r->left() , r->right() ) == r.get() &&
        lowestCommonAncestor( r->left() , r->left() ) == r->left() &&
        lowestCommonAncestor( r.get() , r->right() ) == r.get() );
 CHECK_THROWS( lowestCommonAncestor( r.get() , ( B * ) nullptr ) ,
               contract_violation );
 CHECK_THROWS( lowestCommonAncestor( ( B * ) nullptr , r.get() ) ,
               contract_violation );
 CHECK_THROWS( lowestCommonAncestor( r->left() , o.get() ) ,
               contract_violation );                   // two trees
 int seen = 0;
 auto count = [ & ]( B * ) { ++seen; };
 (void) searchTreeRange( r.get() , 5 , 5 , count );   // lo = hi
 CHECK( seen == 1 );
 (void) searchTreeRange( r.get() , 6 , 7 , count );   // no key in it
 CHECK( seen == 1 &&
        searchTreeRange( ( B * ) nullptr , 0 , 9 , count ) == 0 );
 CHECK_THROWS( searchTreeRange( r.get() , 6 , 5 , count ) ,
               contract_violation );
 CHECK( minimumHeightSearchTree( std::vector< int >{} ) == nullptr );
 delete minimumHeightSearchTree( std::vector< int >{ 4 } );
 CHECK_THROWS( delete minimumHeightSearchTree( std::vector< int >{ 1 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( delete minimumHeightSearchTree( std::vector< int >{ 2 , 1 } ) ,
               contract_violation );
 CHECK( fromPreorderInorder( std::vector< int >{} , std::vector< int >{} ) ==
        nullptr );
 std::unique_ptr< B > p( fromPreorderInorder( std::vector< int >{ 1 , 2 } ,
                                              std::vector< int >{ 2 , 1 } ) );
 CHECK( p && p->read() == 1 && p->left() && p->left()->read() == 2 );
 CHECK_THROWS( delete fromPreorderInorder( std::vector< int >{ 1 } ,
                                           std::vector< int >{} ) ,
               contract_violation );
 CHECK_THROWS( delete fromPreorderInorder( std::vector< int >{ 1 , 2 } ,
                                           std::vector< int >{ 1 , 3 } ) ,
               contract_violation );

 ChildListTree< int > L( 0 );
 CHECK_THROWS( L.insertSibling( L.root() , 1 ) , contract_violation );
 const int c = L.insertChild( L.root() , 1 );
 CHECK( L.insertSibling( c , 2 ) >= 0 && L.size() == 3 );
 CHECK_THROWS( (void) L.read( -1 ) , contract_violation );
 CHECK_THROWS( (void) L.read( 99 ) , contract_violation );
 CHECK_THROWS( L.write( 99 , 0 ) , contract_violation );
 L.remove( c );
 CHECK_THROWS( (void) L.read( c ) , contract_violation );   // freed
 CursorTree< int > C( 0 ) , S( 7 );
 CHECK_THROWS( C.insertSiblingTree( C.root() , S , S.root() ) ,
               contract_violation );
 const int cc = C.insertChild( C.root() , 1 );
 C.insertSiblingTree( cc , S , S.root() );
 CHECK( C.size() == 3 );
 CHECK_THROWS( (void) C.read( -1 ) , contract_violation );
 CursorBinaryTree< int > CB( 0 ) , SB( 4 );
 CB.insertRight( CB.root() , SB , SB.root() );
 CHECK_THROWS( CB.insertRight( CB.root() , 1 ) , contract_violation );
 CHECK_THROWS( CB.insertRight( CB.root() , SB , SB.root() ) ,
               contract_violation );
 CB.insertLeft( CB.root() , SB , SB.root() );
 CHECK_THROWS( CB.insertLeft( CB.root() , SB , SB.root() ) ,
               contract_violation );

 CHECK_THROWS( delete expressionTree( "+" ) , contract_violation );
 CHECK_THROWS( delete expressionTree( "1 2" ) , contract_violation );
 CHECK_THROWS( delete expressionTree( "" ) , contract_violation );
 std::unique_ptr< BinaryTree< std::string > > x( expressionTree( "x" ) ) ,
                                              q( expressionTree( "1 x /" ) );
 const std::map< std::string , double > env = { { "x" , 0 } };
 CHECK( evaluate( x.get() , env ) == 0 && prefixForm( x.get() ) == "(x)" &&
        infixForm( x.get() ) == "x" );
 CHECK_THROWS( evaluate( x.get() , {} ) , contract_violation );
 CHECK_THROWS( evaluate( q.get() , env ) , contract_violation );   // 1 / 0
 CHECK_THROWS( evaluate( nullptr , env ) , contract_violation );
 CHECK_THROWS( prefixForm( nullptr ) , contract_violation );
 CHECK_THROWS( infixForm( nullptr ) , contract_violation );

 std::vector< int > H = { 1 , 5 };
 maxHeapSiftUp( H , 1 );                      // i = n - 1: just inside
 CHECK( H == std::vector< int >( { 5 , 1 } ) );
 CHECK_THROWS( maxHeapSiftUp( H , 2 ) , contract_violation );
 std::vector< int > H0;
 CHECK_THROWS( maxHeapSiftUp( H0 , 0 ) , contract_violation );
 heapBuildByInsertion( H0 );
 CHECK( H0.empty() );

 std::unique_ptr< B > l( from_list< int >( "( 1 ( 2 () () ) () )" ) );
 CHECK( l && l->left() && ! l->right() && to_list( l.get() ).size() > 0 );
 for( const char * bad : { "" , "1" , "( 1" , "( 1 ( 2 ) () ) )" ,
                           "( x )" , "( 8x )" , "( ( 1 ) )" } )
  CHECK_THROWS( delete from_list< int >( bad ) , contract_violation );
 std::unique_ptr< Tree< int > > tl( tree_from_list< int >( "( 1 ( 2 ) )" ) );
 CHECK( tl && tl->leftmostChild() );
 for( const char * bad : { "" , "( 1" , "( 1 ) )" , "( 8x )" , "( ( 1 ) )" } )
  CHECK_THROWS( delete tree_from_list< int >( bad ) , contract_violation );
 const auto none = []( auto ) {};
 CHECK_THROWS( dfs( ( Tree< int > * ) nullptr , none , none ) ,
               contract_violation );
 CHECK_THROWS( bfs( ( Tree< int > * ) nullptr , none ) , contract_violation );
 CHECK_THROWS( maxWeightLevel( ( Tree< int > * ) nullptr ) ,
               contract_violation );
 CHECK_THROWS( to_list( ( const Tree< int > * ) nullptr ) ,
               contract_violation );
 CHECK( maxWeightLevel( tl.get() ) >= 0 );

 RedBlackTree< int , int > R;
 CHECK_THROWS( (void) R.predecessorNode( nullptr ) , contract_violation );
 CHECK_THROWS( ( (void) RedBlackTree< int , int >::value( nullptr ) ) ,
               contract_violation );
 ArrayBinaryTree< int > A;
 CHECK_THROWS( (void) A.isLeaf( 0 ) , contract_violation );
 CHECK_THROWS( (void) A.right( 0 ) , contract_violation );
 A.setRoot( 1 );
 CHECK( A.isLeaf( 0 ) );
 CHECK_THROWS( (void) A.isLeaf( -1 ) , contract_violation );
 CHECK_THROWS( (void) A.parent( 1 ) , contract_violation );
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
