/*--------------------------------------------------------------------------*/
/*--------------------------- File lib_trees.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the classic problems on binary trees [see
 * BinaryTreeAlgorithms.h], of the heap built by insertions [see
 * HeapByInsertion.h], and of the subtrees detached and the postorder of
 * BinaryTree [see BinaryTree.h]: each one against an independent oracle (a
 * brute force on small random trees, the recursive definition, the
 * standard library), at the level 3 of the contracts, on BinaryTree and on
 * the nodes of a plain struct, with the edge cases.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 5, 6 and 10
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

#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/HeapByInsertion.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using BT = BinaryTree< int >;
using Owner = std::unique_ptr< BT >;
using Ints = std::vector< int >;

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a uniform integer in [ a , b ]
int rnd( std::mt19937 & g , int a , int b )
{
 return( std::uniform_int_distribution< int >( a , b )( g ) );
 }

/// a random shape of n nodes, the values given by next() in previsit
BT * randomTree( std::mt19937 & g , int n ,
                 const std::function< int( void ) > & next )
{
 if( n == 0 ) return( nullptr );
 auto * t = new BT( next() );
 const int k = rnd( g , 0 , n - 1 );
 if( auto * l = randomTree( g , k , next ) ) t->insertLeft( l );
 if( auto * r = randomTree( g , n - 1 - k , next ) ) t->insertRight( r );
 return( t );
 }

/// a random shape of n nodes with values in [ lo , hi ]
BT * randomTree( std::mt19937 & g , int n , int lo , int hi )
{
 return( randomTree( g , n , [ & ]() { return( rnd( g , lo , hi ) ); } ) );
 }

/// the previsit (0), the invisit (1) or the postvisit (2) of t
Ints visit( BT * t , int which )
{
 Ints V;
 auto add = [ & ]( BT * u ) { V.push_back( u->read() ); };
 auto none = []( BT * ) {};
 if( which == 0 ) dfs( t , add , none , none );
 else if( which == 1 ) dfs( t , none , add , none );
 else dfs( t , none , none , add );
 return( V );
 }

/// the nodes of t in previsit
std::vector< BT * > nodesOf( BT * t )
{
 std::vector< BT * > N;
 pre_visit( t , [ & ]( BT * u ) { N.push_back( u ); } );
 return( N );
 }

/// the height of t, -1 if empty
int heightOf( const BT * t )
{
 return( t ? 1 + std::max( heightOf( t->left() ) , heightOf( t->right() ) )
           : -1 );
 }

/// the insertion of k into the search tree t, made of BT
void bstInsert( Owner & t , int k )
{
 if( ! t ) {
  t.reset( new BT( k ) );
  return;
  }
 BT * u = t.get();
 for( ; ; ) {
  BT * c = k < u->read() ? u->left() : u->right();
  if( ! c ) break;
  u = c;
  }
 if( k < u->read() ) u->insertLeft( new BT( k ) );
 else u->insertRight( new BT( k ) );
 }

/// a node of a plain struct, as in the pseudocode of the book
struct Node {
 int key = 0;                   ///< the key
 bool red = false;              ///< the color
 Node * left = nullptr;         ///< the left child
 Node * right = nullptr;        ///< the right child
 Node * parent = nullptr;       ///< the parent
 };

/// the nodes, at fixed addresses
using Pool = std::deque< Node >;

/// a new node of P over l and r, whose parent it becomes
Node * makeNode( Pool & P , int key , Node * l , Node * r )
{
 Node & t = P.emplace_back();
 t.key = key;
 t.left = l;
 t.right = r;
 if( l ) l->parent = &t;
 if( r ) r->parent = &t;
 return( &t );
 }

/// the copy of t as nodes of P
Node * toNodes( Pool & P , const BT * t )
{
 if( ! t ) return( nullptr );
 Node * l = toNodes( P , t->left() );
 Node * r = toNodes( P , t->right() );
 return( makeNode( P , t->read() , l , r ) );
 }

/// the mirror of t, in place, by detaching and reinserting its subtrees
void mirror( BT * t )
{
 if( ! t ) return;
 BT * l = t->detachLeft();
 BT * r = t->detachRight();
 mirror( l );
 mirror( r );
 if( r ) t->insertLeft( r );
 if( l ) t->insertRight( l );
 }

/// the Cartesian tree of A[ i .. j - 1 ] by its definition, the leftmost
/// best item by less at the root
template< class Compare >
BT * cartesianRec( const Ints & A , int i , int j , Compare less )
{
 if( i >= j ) return( nullptr );
 int m = i;
 for( int k = i + 1 ; k < j ; ++k )
  if( less( A[ k ] , A[ m ] ) ) m = k;
 auto * t = new BT( A[ m ] );
 if( auto * l = cartesianRec( A , i , m , less ) ) t->insertLeft( l );
 if( auto * r = cartesianRec( A , m + 1 , j , less ) ) t->insertRight( r );
 return( t );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// detachLeft() and detachRight() give the subtree back as a root, which
// goes elsewhere: the mirror in place equals the mirror read from the
// visits (reversed invisit), twice gives the tree back; nil when the child
// is missing, and a detached subtree refused where a child is
ADS_TEST( "binarytree-detach" )
{
 Owner t( from_list< int >( "( 1 ( 2 ( 4 () () ) () ) ( 3 () () ) )" ) );
 BT * two = t->left();
 Owner l( t->detachLeft() );
 CHECK( l.get() == two && ! l->parent() && ! t->left() );
 CHECK( to_list( t.get() ) == "( 1 () ( 3 () () ) )" );
 CHECK( to_list( l.get() ) == "( 2 ( 4 () () ) () )" );
 CHECK( t->detachLeft() == nullptr && l->detachRight() == nullptr );
 CHECK_THROWS( t->insertRight( l.get() ) , contract_violation );
 t->right()->insertLeft( l.release() );
 CHECK( to_list( t.get() ) == "( 1 () ( 3 ( 2 ( 4 () () ) () ) () ) )" );
 Owner r( t->detachRight() );
 CHECK( to_list( t.get() ) == "( 1 () () )" && ! r->parent() );
 std::mt19937 g( 1 );
 for( int k = 0 ; k < 500 ; ++k ) {
  Owner u( randomTree( g , rnd( g , 0 , 60 ) , 0 , 99 ) );
  const std::string s = to_list( u.get() );
  Ints in = visit( u.get() , 1 );
  std::reverse( in.begin() , in.end() );
  mirror( u.get() );
  CHECK( visit( u.get() , 1 ) == in );
  mirror( u.get() );
  CHECK( to_list( u.get() ) == s );
  }
 }

// the ranges preorder(), inorder() and postorder() against the recursive
// visits on random trees, and on a path of 100000 nodes, which they walk
// with no stack
ADS_TEST( "binarytree-postorder" )
{
 std::mt19937 g( 2 );
 for( int k = 0 ; k < 500 ; ++k ) {
  Owner t( randomTree( g , rnd( g , 1 , 80 ) , 0 , 99 ) );
  Ints a , b , c;
  for( BT * u : preorder( t.get() ) ) a.push_back( u->read() );
  for( BT * u : inorder( t.get() ) ) b.push_back( u->read() );
  for( BT * u : postorder( t.get() ) ) c.push_back( u->read() );
  CHECK( a == visit( t.get() , 0 ) && b == visit( t.get() , 1 ) &&
         c == visit( t.get() , 2 ) );
  BT * sub = nodesOf( t.get() )[ rnd( g , 0 , int( a.size() ) - 1 ) ];
  Ints d;
  for( BT * u : postorder( sub ) ) d.push_back( u->read() );
  CHECK( d == visit( sub , 2 ) );
  }
 const int n = 100000;
 std::vector< BT * > P( n );
 for( int i = 0 ; i < n ; ++i ) P[ i ] = new BT( i );
 for( int i = n - 1 ; i > 0 ; --i )
  if( i % 2 ) P[ i - 1 ]->insertLeft( P[ i ] );
  else P[ i - 1 ]->insertRight( P[ i ] );
 int expect = n - 1 , count = 0;
 bool ok = true;
 for( BT * u : postorder( P[ 0 ] ) ) {
  ok = ok && ( u->read() == expect-- );
  ++count;
  }
 CHECK( ok && ( count == n ) );
 for( int i = n - 1 ; i > 0 ; --i )        // freed from the bottom
  delete ( i % 2 ? P[ i - 1 ]->detachLeft() : P[ i - 1 ]->detachRight() );
 delete P[ 0 ];
 Owner one( new BT( 7 ) );
 for( BT * u : postorder( one.get() ) ) CHECK( u == one.get() );
 }

// the lowest common ancestor against the first ancestor of v among those
// of u, on every pair of nodes of random trees, on BinaryTree and on the
// nodes of a plain struct; two nodes of different trees refused; a chain
// of 100000 nodes, one node alone
ADS_TEST( "lowest-common-ancestor" )
{
 Owner e( from_list< int >( "( 1 ( 2 ( 4 () () ) ( 5 ( 6 () () ) () ) ) ( 3 "
                            "() () ) )" ) );
 BT * n4 = e->left()->left() , * n6 = e->left()->right()->left();
 CHECK( lowestCommonAncestor( n4 , n6 ) == e->left() );
 CHECK( lowestCommonAncestor( n6 , e->right() ) == e.get() );
 CHECK( lowestCommonAncestor( e->left() , n6 ) == e->left() );
 CHECK( lowestCommonAncestor( n6 , n6 ) == n6 );
 Owner o( new BT( 1 ) );
 CHECK_THROWS( lowestCommonAncestor( n4 , o.get() ) , contract_violation );
 CHECK_THROWS( lowestCommonAncestor( n4 , static_cast< BT * >( nullptr ) ) ,
               contract_violation );
 std::mt19937 g( 3 );
 for( int k = 0 ; k < 300 ; ++k ) {
  Owner t( randomTree( g , 1 + k % 25 , 0 , 0 ) );
  Pool P;
  Node * s = toNodes( P , t.get() );
  std::vector< Node * > M;
  for( Node & x : P ) M.push_back( &x );
  const auto N = nodesOf( t.get() );
  for( BT * u : N )
   for( BT * v : N ) {
    std::set< BT * > A;
    for( BT * x = u ; x ; x = x->parent() ) A.insert( x );
    BT * a = v;
    while( ! A.count( a ) ) a = a->parent();
    CHECK( lowestCommonAncestor( u , v ) == a );
    }
  for( Node * u : M )
   for( Node * v : M ) {
    std::set< Node * > A;
    for( Node * x = u ; x ; x = x->parent ) A.insert( x );
    Node * a = v;
    while( ! A.count( a ) ) a = a->parent;
    CHECK( lowestCommonAncestor( u , v ) == a );
    }
  CHECK( lowestCommonAncestor( M.front() , s ) == s );
  }
 // a chain of 100000 nodes: the climb is not recursive; one node alone
 const int n = 100000;
 std::vector< BT * > C( n );
 for( int i = 0 ; i < n ; ++i ) C[ i ] = new BT( i );
 for( int i = n - 1 ; i > 0 ; --i ) C[ i - 1 ]->insertLeft( C[ i ] );
 CHECK( lowestCommonAncestor( C[ n - 1 ] , C[ 0 ] ) == C[ 0 ] &&
        lowestCommonAncestor( C[ n / 2 ] , C[ n - 1 ] ) == C[ n / 2 ] );
 for( int i = n - 1 ; i > 0 ; --i )        // freed from the bottom
  delete C[ i - 1 ]->detachLeft();
 delete C[ 0 ];
 CHECK( lowestCommonAncestor( o.get() , o.get() ) == o.get() );
 }

// equalTrees() against the parenthesized lists, on trees of the same and of
// different shapes and keys, between BinaryTree and plain nodes too; the
// shapes alone with a comparison always true
ADS_TEST( "equal-trees" )
{
 std::mt19937 g( 4 );
 auto any = []( const auto & , const auto & ) { return( true ); };
 for( int k = 0 ; k < 2000 ; ++k ) {
  Owner a( randomTree( g , rnd( g , 0 , 6 ) , 0 , 1 ) );
  Owner b( randomTree( g , rnd( g , 0 , 6 ) , 0 , 1 ) );
  CHECK( equalTrees( a.get() , b.get() ) ==
         ( to_list( a.get() ) == to_list( b.get() ) ) );
  CHECK( equalTrees( a.get() , a.get() ) );
  Pool P;
  CHECK( equalTrees( a.get() , toNodes( P , b.get() ) ) ==
         ( to_list( a.get() ) == to_list( b.get() ) ) );
  Owner sa( copy( a.get() ) ) , sb( copy( b.get() ) );
  for( BT * u : nodesOf( sa.get() ) ) u->write( 0 );
  for( BT * u : nodesOf( sb.get() ) ) u->write( 0 );
  CHECK( equalTrees( a.get() , b.get() , any ) ==
         ( to_list( sa.get() ) == to_list( sb.get() ) ) );
  }
 CHECK( equalTrees( static_cast< BT * >( nullptr ) ,
                    static_cast< BT * >( nullptr ) ) );
 Owner one( new BT( 1 ) );
 CHECK( ! equalTrees( one.get() , static_cast< BT * >( nullptr ) ) );
 }

// isSearchTree() against the invisit strictly increasing on random small
// trees, the trees built by insertions, plain nodes, a violation at a
// grandchild only, and the order std::greater
ADS_TEST( "is-search-tree" )
{
 std::mt19937 g( 5 );
 int yes = 0;
 for( int k = 0 ; k < 3000 ; ++k ) {
  Owner t( randomTree( g , rnd( g , 0 , 8 ) , 0 , 9 ) );
  const Ints in = visit( t.get() , 1 );
  const bool e = std::adjacent_find( in.begin() , in.end() ,
                                     std::greater_equal<>() ) == in.end();
  CHECK( isSearchTree( t.get() ) == e );
  Pool P;
  CHECK( isSearchTree( toNodes( P , t.get() ) ) == e );
  const bool d = std::adjacent_find( in.begin() , in.end() ,
                                     std::less_equal<>() ) == in.end();
  CHECK( isSearchTree( t.get() , std::greater<>() ) == d );
  yes += e;
  }
 CHECK( yes > 200 );
 for( int k = 0 ; k < 100 ; ++k ) {
  Owner t;
  std::set< int > S;
  for( int i = 0 ; i < 60 ; ++i )
   if( int x = rnd( g , 0 , 999 ) ; S.insert( x ).second ) bstInsert( t , x );
  CHECK( isSearchTree( t.get() ) );
  }
 Owner w( from_list< int >( "( 10 ( 5 () ( 12 () () ) ) ( 15 () () ) )" ) );
 CHECK( ! isSearchTree( w.get() ) );        // each parent agrees
 w->left()->right()->write( 7 );
 CHECK( isSearchTree( w.get() ) );
 w->left()->right()->write( 10 );           // equal keys refused
 CHECK( ! isSearchTree( w.get() ) );
 CHECK( isSearchTree( static_cast< BT * >( nullptr ) ) );
 }

// isRedBlackTree() against the constraints checked path by path, on every
// colouring of random shapes of up to 9 nodes, on BinaryTree and on plain
// nodes; the empty tree is a Red-Black tree
ADS_TEST( "is-red-black-tree" )
{
 std::mt19937 g( 6 );
 int yes = 0;
 for( int k = 0 ; k < 300 ; ++k ) {
  const int n = rnd( g , 1 , 9 );
  int next = 0;
  Owner t( randomTree( g , n , [ & ]() { return( next++ ); } ) );
  Pool P;
  Node * s = toNodes( P , t.get() );
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   auto red = [ & ]( BT * u ) { return( ( mask >> u->read() ) & 1 ); };
   bool ok = ! red( t.get() );                  // by the paths
   std::set< int > bh;
   std::function< void( BT * , int ) > down = [ & ]( BT * u , int b ) {
    if( ! u ) {
     bh.insert( b );
     return;
     }
    if( red( u ) && u->parent() && red( u->parent() ) ) ok = false;
    down( u->left() , b + ( red( u ) ? 0 : 1 ) );
    down( u->right() , b + ( red( u ) ? 0 : 1 ) );
    };
   down( t.get() , 0 );
   std::function< void( BT * ) > each = [ & ]( BT * u ) {   // every node
    if( ! u ) return;
    std::set< int > h;
    std::function< void( BT * , int ) > d = [ & ]( BT * v , int b ) {
     if( ! v ) {
      h.insert( b );
      return;
      }
     d( v->left() , b + ( red( v ) ? 0 : 1 ) );
     d( v->right() , b + ( red( v ) ? 0 : 1 ) );
     };
    d( u , 0 );
    ok = ok && ( h.size() == 1 );
    each( u->left() );
    each( u->right() );
    };
   each( t.get() );
   ok = ok && ( bh.size() == 1 );
   CHECK( isRedBlackTree( t.get() , red ) == ok );
   for( Node & x : P ) x.red = ( mask >> x.key ) & 1;
   CHECK( isRedBlackTree( s , []( const Node * u ) { return( u->red ); } )
          == ok );
   yes += ok;
   }
  }
 CHECK( yes > 100 );
 CHECK( isRedBlackTree( static_cast< BT * >( nullptr ) ,
                        []( BT * ) { return( true ); } ) );
 }

// searchTreeRange() against the keys of the invisit in the range, reported
// in increasing order, examining at most k + 2 ( h + 1 ) nodes, on search
// trees built by insertions and on plain nodes; an empty range refused
ADS_TEST( "search-tree-range" )
{
 Owner T;
 for( int k : { 15 , 10 , 20 , 3 , 12 , 18 , 23 } ) bstInsert( T , k );
 Ints got;
 searchTreeRange( T.get() , 11 , 18 , [ & ]( BT * u ) {
  got.push_back( u->read() ); } );
 CHECK( got == Ints( { 12 , 15 , 18 } ) );
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 500 ; ++t ) {
  Owner R;
  std::set< int > S;
  for( int i = rnd( g , 0 , 60 ) ; i > 0 ; --i )
   if( int x = rnd( g , 0 , 239 ) ; S.insert( x ).second ) bstInsert( R , x );
  int lo = rnd( g , -5 , 245 ) , hi = rnd( g , -5 , 245 );
  if( lo > hi ) std::swap( lo , hi );
  Ints K;
  for( int x : S )
   if( ( lo <= x ) && ( x <= hi ) ) K.push_back( x );
  Ints out;
  const std::size_t seen = searchTreeRange( R.get() , lo , hi ,
                            [ & ]( BT * u ) { out.push_back( u->read() ); } );
  CHECK( out == K );
  CHECK( seen <= K.size() + 2 * std::size_t( heightOf( R.get() ) + 1 ) );
  Pool P;
  Ints pn;
  searchTreeRange( toNodes( P , R.get() ) , lo , hi ,
                   [ & ]( Node * u ) { pn.push_back( u->key ); } );
  CHECK( pn == K );
  }
 CHECK( searchTreeRange( static_cast< BT * >( nullptr ) , 0 , 0 ,
                         []( BT * ) {} ) == 0 );
 CHECK_THROWS( searchTreeRange( T.get() , 2 , 1 , []( BT * ) {} ) ,
               contract_violation );
 }

// minimumHeightSearchTree() for n = 0 to 1000: a search tree whose invisit
// is A and whose height is ceil( log2( n + 1 ) ) - 1, as a BinaryTree and
// as plain nodes made by a callable; keys not increasing refused
ADS_TEST( "minimum-height-search-tree" )
{
 std::mt19937 g( 8 );
 for( int n = 0 ; n <= 1000 ; ++n ) {
  Ints A( n );
  for( int i = 0 , v = -50 ; i < n ; ++i ) A[ i ] = v += rnd( g , 1 , 9 );
  Owner T( minimumHeightSearchTree( A ) );
  int h = -1;
  while( ( 1 << ( h + 1 ) ) - 1 < n ) ++h;
  CHECK( heightOf( T.get() ) == h && visit( T.get() , 1 ) == A );
  CHECK( isSearchTree( T.get() ) );
  Pool P;
  Node * s = minimumHeightSearchTree( A , [ & ]( int k , Node * l ,
                                                 Node * r ) {
   return( makeNode( P , k , l , r ) ); } );
  CHECK( ( n == 0 ) == ( s == nullptr ) );
  CHECK( equalTrees( T.get() , s ) );
  if( n > 0 ) CHECK( s->parent == nullptr );
  }
 Ints D = { 9 , 5 , 1 };
 Owner d( minimumHeightSearchTree( D , std::greater<>() ) );
 CHECK( to_list( d.get() ) == "( 5 ( 9 () () ) ( 1 () () ) )" );
 CHECK( minimumHeightSearchTree( Ints() ) == nullptr );
 CHECK_THROWS( minimumHeightSearchTree( Ints( { 1 , 1 } ) ) ,
               contract_violation );
 CHECK_THROWS( minimumHeightSearchTree( Ints( { 1 , 3 , 2 } ) ) ,
               contract_violation );
 }

// fromPreorderInorder() on the example of the book's exercises, on random
// trees of distinct values rebuilt from their two visits, of up to 2000
// nodes, and on paths; the empty tree; inconsistent visits refused
ADS_TEST( "from-preorder-inorder" )
{
 const std::string pre = "AEBFGCDIH" , in = "BEGFCADHI";
 std::unique_ptr< BinaryTree< char > > T( fromPreorderInorder(
  std::vector< char >( pre.begin() , pre.end() ) ,
  std::vector< char >( in.begin() , in.end() ) ) );
 CHECK( to_list( T.get() ) == "( A ( E ( B () () ) ( F ( G () () ) ( C () () "
                              ") ) ) ( D () ( I ( H () () ) () ) ) )" );
 std::mt19937 g( 9 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = r < 590 ? rnd( g , 0 , 40 ) : 2000;
  Ints L( n );
  std::iota( L.begin() , L.end() , 0 );
  std::shuffle( L.begin() , L.end() , g );
  std::size_t next = 0;
  Owner u( randomTree( g , n , [ & ]() { return( L[ next++ ] ); } ) );
  Owner v( fromPreorderInorder( visit( u.get() , 0 ) ,
                                visit( u.get() , 1 ) ) );
  CHECK( to_list( u.get() ) == to_list( v.get() ) );
  }
 Ints up( 50 ) , down( 50 );
 std::iota( up.begin() , up.end() , 0 );
 std::reverse_copy( up.begin() , up.end() , down.begin() );
 Owner left( fromPreorderInorder( up , down ) );
 Owner right( fromPreorderInorder( up , up ) );
 CHECK( heightOf( left.get() ) == 49 && ! left->right() );
 CHECK( heightOf( right.get() ) == 49 && ! right->left() );
 CHECK( fromPreorderInorder( Ints() , Ints() ) == nullptr );
 CHECK_THROWS( fromPreorderInorder( Ints( { 1 , 2 } ) , Ints( { 1 , 3 } ) ) ,
               contract_violation );
 CHECK_THROWS( fromPreorderInorder( Ints( { 1 , 2 , 3 } ) ,
                                    Ints( { 3 , 1 , 2 } ) ) ,
               contract_violation );
 CHECK_THROWS( fromPreorderInorder( Ints( { 1 } ) , Ints() ) ,
               contract_violation );
 }

// cartesianBinaryTree() against the recursive definition, with the minimum
// and with the maximum (std::greater) at the root, on random arrays with
// repeated items (the leftmost best item at the root); the maximal trees
// of two examples; the empty array
ADS_TEST( "cartesian-binary-tree" )
{
 Owner a( cartesianBinaryTree( Ints( { 2 , 3 , -1 , 8 , 4 , 5 } ) ,
                               std::greater< int >() ) );
 CHECK( to_list( a.get() ) == "( 8 ( 3 ( 2 () () ) ( -1 () () ) ) ( 5 ( 4 () "
                              "() ) () ) )" );
 Owner b( cartesianBinaryTree( Ints( { 2 , 1 , 5 , 3 , 0 , 4 } ) ,
                               std::greater< int >() ) );
 CHECK( to_list( b.get() ) == "( 5 ( 2 () ( 1 () () ) ) ( 4 ( 3 () ( 0 () () "
                              ") ) () ) )" );
 std::mt19937 g( 10 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  Ints A( r % 40 );
  for( int & x : A ) x = rnd( g , 0 , r % 3 ? 99 : 4 );
  const int n = int( A.size() );
  Owner m( cartesianBinaryTree( A ) );
  Owner mr( cartesianRec( A , 0 , n , std::less< int >() ) );
  Owner M( cartesianBinaryTree( A , std::greater< int >() ) );
  Owner Mr( cartesianRec( A , 0 , n , std::greater< int >() ) );
  CHECK( to_list( m.get() ) == to_list( mr.get() ) );
  CHECK( to_list( M.get() ) == to_list( Mr.get() ) );
  CHECK( visit( m.get() , 1 ) == A );
  }
 CHECK( cartesianBinaryTree( Ints() ) == nullptr );
 Ints S( 300 );
 std::iota( S.begin() , S.end() , 0 );
 Owner s( cartesianBinaryTree( S , std::greater< int >() ) );
 CHECK( heightOf( s.get() ) == 299 );
 }

// heapBuildByInsertion() against std::push_heap on each prefix, which is
// the same sift up, on random vectors with repeated items; a max-heap of
// the same items; the example [ 4 , 7 , 9 , 10 ] and how it differs from
// heapBuild(); the order std::greater gives a min-heap; maxHeapSiftUp()
// refuses a place out of the vector
ADS_TEST( "heap-by-insertion" )
{
 Ints A = { 4 , 7 , 9 , 10 } , B = A;
 heapBuildByInsertion( A );
 heapBuild( B );
 CHECK( A == Ints( { 10 , 9 , 7 , 4 } ) && B == Ints( { 10 , 7 , 9 , 4 } ) );
 std::mt19937 g( 11 );
 int differ = 0;
 for( int r = 0 ; r < 1000 ; ++r ) {
  Ints V( rnd( g , 0 , 60 ) );
  for( int & x : V ) x = rnd( g , -20 , 20 );
  Ints H = V , S = V , F = V;
  heapBuildByInsertion( H );
  for( std::size_t i = 1 ; i <= S.size() ; ++i )
   std::push_heap( S.begin() , S.begin() + i );
  CHECK( H == S && isMaxHeap( H , H.size() ) );
  CHECK( std::is_permutation( H.begin() , H.end() , V.begin() ) );
  heapBuild( F );
  differ += ( F != H );
  Ints m = V;
  heapBuildByInsertion( m , std::greater<>() );
  CHECK( std::is_heap( m.begin() , m.end() , std::greater<>() ) );
  }
 CHECK( differ > 100 );
 Ints E;
 heapBuildByInsertion( E );
 CHECK( E.empty() );
 Ints one = { 5 };
 maxHeapSiftUp( one , 0 );
 CHECK( one == Ints( { 5 } ) );
 CHECK_THROWS( maxHeapSiftUp( one , 1 ) , contract_violation );
 Ints bad = { 1 , 5 , 3 };                   // A[ 0 .. 1 ] not a heap
 CHECK_THROWS( maxHeapSiftUp( bad , 2 ) , contract_violation );
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
/*------------------------ End File lib_trees.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
