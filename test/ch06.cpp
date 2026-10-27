/*--------------------------------------------------------------------------*/
/*----------------------------- File ch06.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 6, Alberi bilanciati di ricerca: the binary search tree
 * of figure 6.1 and the three cases of removal of figure 6.2, the Red-Black
 * tree against its constraints and the Teoremi 6.1, 6.2 and 6.3, both trees
 * against std::map on random operations, the B+-tree built by the
 * insertions of figure 6.8 into the one of figure 6.7, and the exercises
 * 6.1 to 6.10, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
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

#include "ADS/ADT.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/SearchTree.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using BST = BinarySearchTree< int , int >;
using RBT = RedBlackTree< int , int >;

static_assert( Dictionary< BST > );
static_assert( Dictionary< RBT > );
// BPlusTree has no remove(), so it is not a Dictionary
static_assert( ! Dictionary< BPlusTree< int , int > > );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the keys of a search tree in previsit, which fixes its shape
template< class Tr >
Ints preorder( const Tr & t )
{
 Ints out;
 std::function< void( typename Tr::Link * ) > rec = [ & ]( auto u ) {
  if( ! t.isNil( u ) ) {
   out.push_back( Tr::key( u ) );
   rec( u->left );
   rec( u->right );
   }
  };
 rec( t.root() );
 return( out );
 }

/// the keys in increasing order, by min() and successorNode()
template< class Tr >
Ints keys( const Tr & t )
{
 Ints out;
 for( auto u = t.min() ; ! t.isNil( u ) ; u = t.successorNode( u ) )
  out.push_back( Tr::key( u ) );
 return( out );
 }

/// the measures of a search tree that its constraints and theorems speak of
struct Shape {
 bool ok = true;           ///< parents, order, and (V1)-(V4) if RB
 int n = 0;                ///< the internal nodes
 int min_leaf = 1 << 30;   ///< the least level of a leaf
 int max_leaf = 0;         ///< the largest level of a leaf, the height
 bool thm61 = true;        ///< each subtree has >= 2^b( u ) - 1 nodes
 };

/// checks a search tree; with RB also its colors. r( u ) is the number of
/// black nodes from u to a leaf, both included, so b( u ) = r( child )
template< class Tr , bool RB >
Shape shape( const Tr & t )
{
 Shape s;
 using L = typename Tr::Link;
 struct R { int r; int size; };
 std::function< R( L * , int ) > rec = [ & ]( L * u , int lev ) -> R {
  if( t.isNil( u ) ) {
   s.min_leaf = std::min( s.min_leaf , lev );
   s.max_leaf = std::max( s.max_leaf , lev );
   return( R{ 1 , 0 } );
   }
  ++s.n;
  for( L * c : { u->left , u->right } )
   if( ! t.isNil( c ) && ( c->parent != u ) )
    s.ok = false;
  if( RB && ( u->color == Color::RED ) &&
      ( ( u->left->color == Color::RED ) ||
        ( u->right->color == Color::RED ) ) )
   s.ok = false;                                         // (V3)
  const R a = rec( u->left , lev + 1 ) , b = rec( u->right , lev + 1 );
  if( RB && ( a.r != b.r ) )
   s.ok = false;                                         // (V4)
  const int size = a.size + b.size + 1;
  if( RB && ( size < ( 1 << a.r ) - 1 ) )
   s.thm61 = false;                                      // Theorem 6.1
  return( R{ a.r + ( u->color == Color::BLACK ) , size } );
  };
 rec( t.root() , 0 );
 if( ! t.isNil( t.root() ) && ! t.isNil( t.root()->parent ) )
  s.ok = false;
 if( RB && ( t.root()->color != Color::BLACK ) )
  s.ok = false;                                          // (V1), (V2)
 const Ints k = keys( t );
 if( ( int( k.size() ) != s.n ) || ( s.n != int( t.size() ) ) ||
     ( std::adjacent_find( k.begin() , k.end() ,
                           std::greater_equal<>() ) != k.end() ) )
  s.ok = false;                                          // the order
 return( s );
 }

template< class Tr >
Shape shape( const Tr & t )
{
 return( shape< Tr , std::is_same_v< Tr , RedBlackTree< typename Tr::Key ,
                                     typename Tr::Value > > >( t ) );
 }

/// the binary tree with the shape and the keys of a search tree
template< class Tr >
BinaryTree< int > * to_binary_tree( const Tr & t , typename Tr::Link * u )
{
 if( t.isNil( u ) )
  return( nullptr );
 auto b = new BinaryTree< int >( Tr::key( u ) );
 if( auto l = to_binary_tree( t , u->left ) ) b->insertLeft( l );
 if( auto r = to_binary_tree( t , u->right ) ) b->insertRight( r );
 return( b );
 }

/// a binary tree to build: a value and the two subtrees, possibly empty
struct BSpec {
 int v;
 std::shared_ptr< BSpec > l , r;
 };

std::shared_ptr< BSpec > B( int v , std::shared_ptr< BSpec > l = {} ,
                            std::shared_ptr< BSpec > r = {} )
{
 return( std::make_shared< BSpec >( BSpec{ v , l , r } ) );
 }

BinaryTree< int > * make( const std::shared_ptr< BSpec > & s )
{
 if( ! s )
  return( nullptr );
 auto t = new BinaryTree< int >( s->v );
 if( s->l ) t->insertLeft( make( s->l ) );
 if( s->r ) t->insertRight( make( s->r ) );
 return( t );
 }

/// Figure 6.1, in the order of a previsit, so that the shape is the same
const Ints fig61 = { 17 , 7 , 1 , 5 , 8 , 13 , 33 , 21 , 27 , 40 , 37 , 42 };

/// the value of k in m, or nothing, as lookup() returns it
std::optional< int > find( const std::map< int , int > & m , int k )
{
 const auto it = m.find( k );
 return( it == m.end() ? std::nullopt : std::optional< int >( it->second ) );
 }

template< class Tr >
Tr build( const Ints & k )
{
 Tr t;
 for( int x : k )
  t.insert( x , 10 * x );
 return( t );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*----------------------- THE BINARY SEARCH TREE ---------------------------*/
/*--------------------------------------------------------------------------*/

// section 6.1, figure 6.1: the keys inserted in previsit give back the
// tree, the invisit is increasing, min(), max(), successorNode() and
// predecessorNode() walk it, an inserted key updates its value. The empty
// tree and a node alone, the extreme keys of int, nil after the sentinel,
// the iterators from both ends, and nil violating the contracts
ADS_TEST( "sec6.1.search-tree" )
{
 BST t = build< BST >( fig61 );
 CHECK( preorder( t ) == fig61 );
 CHECK( shape( t ).ok && ( t.size() == 12 ) );
 CHECK( keys( t ) == Ints( { 1 , 5 , 7 , 8 , 13 , 17 , 21 , 27 , 33 , 37 ,
                             40 , 42 } ) );
 CHECK( ( BST::key( t.min() ) == 1 ) && ( BST::key( t.max() ) == 42 ) );
 CHECK( t.lookup( 27 ) == 270 && ! t.lookup( 26 ) && ! t.lookup( 99 ) );
 CHECK( BST::key( t.successorNode( t.lookupNode( 13 ) ) ) == 17 );
 CHECK( BST::key( t.successorNode( t.lookupNode( 27 ) ) ) == 33 );
 CHECK( BST::key( t.predecessorNode( t.lookupNode( 21 ) ) ) == 17 );
 CHECK( BST::key( t.predecessorNode( t.lookupNode( 37 ) ) ) == 33 );
 CHECK( t.isNil( t.successorNode( t.max() ) ) );
 CHECK( t.isNil( t.predecessorNode( t.min() ) ) );
 t.insert( 21 , -1 );                      // the key is there
 CHECK( ( t.lookup( 21 ) == -1 ) && ( t.size() == 12 ) );
 BST e;
 CHECK( e.empty() && e.isNil( e.min() ) && ! e.lookup( 1 ) );
 CHECK( e.isNil( e.max() ) && e.isNil( e.root() ) && ( e.size() == 0 ) );
 CHECK( e.isNil( e.lookupNode( 1 ) ) && ! e.contains( 1 ) );
 CHECK( e.begin() == e.end() );
 CHECK( e.isNil( e.successorNode( e.root() ) ) &&
        e.isNil( e.predecessorNode( e.root() ) ) );   // nil to nil
 int visits = 0;
 e.forEach( [ & ]( int , int ) { ++visits; } );
 CHECK( visits == 0 );

 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 BST one;
 one.insert( hi , 1 );
 CHECK( ( one.min() == one.root() ) && ( one.max() == one.root() ) );
 CHECK( one.isNil( one.successorNode( one.root() ) ) &&
        one.isNil( one.predecessorNode( one.root() ) ) );
 CHECK( ( *one.begin() ).first == hi && ( *--one.end() ).first == hi );
 one.insert( lo , 2 );
 one.insert( 0 , 3 );
 CHECK( keys( one ) == Ints( { lo , 0 , hi } ) && shape( one ).ok );
 CHECK( ( one.lookup( lo ) == 2 ) && ! one.lookup( lo + 1 ) &&
        ! one.lookup( hi - 1 ) );
 Ints back;
 for( auto it = one.end() ; it != one.begin() ; )
  back.push_back( ( *--it ).first );
 CHECK( back == Ints( { hi , 0 , lo } ) );
 CHECK_THROWS( (void) t.successorNode( nullptr ) , contract_violation );
 CHECK_THROWS( (void) t.predecessorNode( nullptr ) , contract_violation );
 CHECK_THROWS( (void) BST::key( nullptr ) , contract_violation );
 CHECK_THROWS( (void) BST::value( nullptr ) , contract_violation );
 }

// section 6.1, figure 6.2: the removal of 33, with two children, moves its
// successor 37 in its place (cases (3) and (1)); the removal of 8, with one
// child, hangs 13 to 7 (case (2)); both give the last tree of the figure.
// The removal from the empty tree, of the only key, twice of the same key,
// of a root with one child on either side, of a node whose successor is its
// right child, of the minimum and of the maximum
ADS_TEST( "sec6.1.remove" )
{
 const BST t = build< BST >( fig61 );
 BST a = t , b = t , c = t;
 a.remove( 33 );
 CHECK( preorder( a ) == Ints( { 17 , 7 , 1 , 5 , 8 , 13 , 37 , 21 , 27 ,
                                 40 , 42 } ) );
 b.remove( 8 );
 CHECK( preorder( b ) == Ints( { 17 , 7 , 1 , 5 , 13 , 33 , 21 , 27 , 40 ,
                                 37 , 42 } ) );
 a.remove( 8 );
 CHECK( preorder( a ) == Ints( { 17 , 7 , 1 , 5 , 13 , 37 , 21 , 27 , 40 ,
                                 42 } ) );
 CHECK( shape( a ).ok && shape( b ).ok );
 c.remove( 99 );                           // absent: nothing changes
 CHECK( preorder( c ) == fig61 );
 for( std::size_t i = 0 ; i < fig61.size() ; ++i ) {   // the root
  c.remove( BST::key( c.root() ) );
  CHECK( shape( c ).ok );
  }
 CHECK( c.empty() && c.isNil( c.root() ) );
 CHECK( preorder( t ) == fig61 );          // the copies were deep

 c.remove( 1 );                            // empty: nothing
 CHECK( c.empty() && shape( c ).ok );
 c.insert( 5 , 5 );
 c.remove( 5 );                            // the only key
 c.remove( 5 );                            // twice
 CHECK( c.empty() && c.isNil( c.root() ) && c.isNil( c.min() ) );
 for( const Ints & k : { Ints{ 5 , 3 } , Ints{ 5 , 7 } } ) {
  BST r = build< BST >( k );               // a root with one child
  r.remove( 5 );
  CHECK( ( preorder( r ) == Ints( { k[ 1 ] } ) ) && shape( r ).ok );
  CHECK( r.isNil( r.root()->parent ) );
  }
 BST s = build< BST >( { 5 , 3 , 8 , 9 } );  // the successor of 5 is 8,
 s.remove( 5 );                              // its right child
 CHECK( ( preorder( s ) == Ints( { 8 , 3 , 9 } ) ) && shape( s ).ok );
 BST m = t;
 m.remove( 1 );                            // the minimum, with a child
 m.remove( 42 );                           // the maximum, a leaf
 CHECK( ( keys( m ).front() == 5 ) && ( keys( m ).back() == 40 ) &&
        shape( m ).ok && ( m.size() == 10 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE RED-BLACK TREE ----------------------------*/
/*--------------------------------------------------------------------------*/

// section 6.2, figure 6.5: the insertion of 1, 2, 3 ends in the case (5b),
// that of 3, 1, 2 goes through (4a) and (5a); both give 2 black over 1 and
// 3 red. Then the case (3) recolors and the root stays black. Increasing
// and decreasing insertions keep the height logarithmic, removals in
// increasing and decreasing order down to the empty tree keep the
// constraints, and a node alone is a black root
ADS_TEST( "spec.RedBlackTree" )
{
 for( const Ints & k : { Ints{ 1 , 2 , 3 } , Ints{ 3 , 1 , 2 } ,
                         Ints{ 3 , 2 , 1 } , Ints{ 1 , 3 , 2 } } ) {
  RBT t = build< RBT >( k );
  CHECK( preorder( t ) == Ints( { 2 , 1 , 3 } ) );
  CHECK( t.root()->color == Color::BLACK );
  CHECK( ( t.root()->left->color == Color::RED ) &&
         ( t.root()->right->color == Color::RED ) );
  t.insert( 4 , 40 );                      // case (3), then (1)
  CHECK( shape( t ).ok );
  CHECK( ( t.root()->left->color == Color::BLACK ) &&
         ( t.root()->right->color == Color::BLACK ) );
  }
 RBT t;
 for( int i = 1 ; i <= 1023 ; ++i )        // increasing: a path as a BST
  t.insert( i , i );
 const Shape s = shape( t );
 CHECK( s.ok && ( s.max_leaf <= 2 * std::log2( 1024.0 ) ) );
 for( int i = 1 ; i <= 1023 ; i += 2 )
  t.remove( i );
 CHECK( shape( t ).ok && ( t.size() == 511 ) );
 for( int i = 2 ; i <= 1022 ; i += 2 ) {   // increasing, to the empty tree
  t.remove( i );
  if( i % 64 == 0 ) CHECK( shape( t ).ok );
  }
 CHECK( t.empty() && shape( t ).ok && t.isNil( t.root() ) );

 RBT d;
 for( int i = 1023 ; i >= 1 ; --i )        // decreasing
  d.insert( i , i );
 const Shape sd = shape( d );
 CHECK( sd.ok && sd.thm61 && ( sd.max_leaf <= 2 * std::log2( 1024.0 ) ) );
 for( int i = 1023 ; i >= 1 ; --i ) {      // decreasing, to the empty tree
  d.remove( i );
  if( i % 64 == 0 ) CHECK( shape( d ).ok );
  }
 CHECK( d.empty() && shape( d ).ok );
 d.insert( 7 , 7 );
 CHECK( ( d.root()->color == Color::BLACK ) && shape( d ).ok );
 d.insert( 7 , 8 );                        // the key is there
 CHECK( ( d.size() == 1 ) && ( d.lookup( 7 ) == 8 ) );
 d.remove( 7 );
 CHECK( d.empty() && shape( d ).ok );
 }

// section 6.2, figure 6.3: the tree of 10 internal nodes and 11 leaves,
// inserted in previsit, is a Red-Black tree and keeps its shape
ADS_TEST( "sec6.2.figure" )
{
 const Ints k = { 17 , 8 , 1 , 7 , 11 , 23 , 20 , 18 , 22 , 27 };
 RBT t = build< RBT >( k );
 const Shape s = shape( t );
 CHECK( s.ok && ( s.n == 10 ) );
 CHECK( keys( t ) == Ints( { 1 , 7 , 8 , 11 , 17 , 18 , 20 , 22 , 23 ,
                             27 } ) );
 int leaves = 0;
 std::function< void( RBT::Link * ) > rec = [ & ]( RBT::Link * u ) {
  if( t.isNil( u ) ) ++leaves;
  else { rec( u->left ); rec( u->right ); }
  };
 rec( t.root() );
 CHECK( leaves == 11 );
 }

// section 6.2, Teoremi 6.1, 6.2 and 6.3 on random Red-Black trees: each
// subtree has at least 2^b( u ) - 1 internal nodes, the deepest leaf is at
// most twice as deep as the highest one, and h <= 2 log2( n + 1 ), also
// for the empty tree (one leaf, at level 0)
ADS_TEST( "sec6.2.theorems" )
{
 std::mt19937 rng( 62 );
 for( int n : { 0 , 1 , 2 , 3 , 10 , 100 , 1000 , 5000 } ) {
  RBT t;
  std::uniform_int_distribution< int > key( 0 , 4 * n );
  while( int( t.size() ) < n )
   t.insert( key( rng ) , 0 );
  const Shape s = shape( t );
  CHECK( s.ok && s.thm61 );
  CHECK( s.max_leaf <= 2 * s.min_leaf );
  CHECK( s.max_leaf <= 2 * std::log2( n + 1.0 ) );
  }
 }

// sections 6.1 and 6.2: random insertions, removals and lookups against
// std::map, the shape checked along the way; copies are deep, moves empty,
// a tree assigned to itself stays the same. With few keys the tree is often
// empty or a node alone; the keys around the extremes of int
template< class Tr >
void against_map( unsigned seed , int lo = 0 , int hi = 499 )
{
 std::mt19937 rng( seed );
 std::uniform_int_distribution< int > key( lo , hi ) , op( 0 , 9 );
 Tr t;
 std::map< int , int > m;
 for( int i = 0 ; i < 20000 ; ++i ) {
  const int k = key( rng ) , o = op( rng );
  if( o < 5 ) {
   t.insert( k , i );
   m[ k ] = i;
   }
  else if( o < 8 ) {
   t.remove( k );
   m.erase( k );
   }
  else {
   CHECK( t.lookup( k ) == find( m , k ) );
   }
  if( i % 1000 == 0 ) {
   CHECK( shape( t ).ok && ( t.size() == m.size() ) );
   Tr c = t;
   CHECK( preorder( c ) == preorder( t ) && shape( c ).ok );
   Tr d = std::move( c );
   CHECK( c.empty() && ( d.size() == t.size() ) );
   c.insert( 1 , 1 );                      // a moved-from tree works
   CHECK( c.size() == 1 && shape( c ).ok );
   Tr & self = d;
   d = self;                               // self-assignment
   CHECK( ( preorder( d ) == preorder( t ) ) && shape( d ).ok );
   d = Tr();                               // an empty one
   CHECK( d.empty() && shape( d ).ok );
   }
  }
 Ints mk;
 for( auto & [ k , v ] : m )
  mk.push_back( k );
 CHECK( keys( t ) == mk );
 while( ! m.empty() ) {
  t.remove( m.begin()->first );
  m.erase( m.begin() );
  }
 CHECK( t.empty() && shape( t ).ok );
 }

ADS_TEST( "sec6.2.against-map" )
{
 against_map< BST >( 61 );
 against_map< RBT >( 62 );
 against_map< BST >( 63 , 0 , 2 );
 against_map< RBT >( 64 , 0 , 2 );
 const int M = std::numeric_limits< int >::max();
 against_map< BST >( 65 , -M - 1 , -M + 400 );
 against_map< RBT >( 66 , M - 400 , M );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE B+-TREE -------------------------------*/
/*--------------------------------------------------------------------------*/

// section 6.3, figure 6.8: the insertion of 10, 12, 13, then 15, 8, 11 and
// 9 in a B+-tree of order 3, and at last of 6, which gives figure 6.7; the
// search of 12 goes from b3 to b1 to f2. The empty tree (a leaf with no
// key), a key inserted again (its value changes, the shape does not), the
// ranges empty, reversed, of one key, below and above
// all the keys, the extreme keys of int, and the orders below 3
ADS_TEST( "sec6.3.bplus-figure" )
{
 using Lv = std::vector< std::vector< Ints > >;
 BPlusTree< int , int > t( 3 );
 CHECK( ( t.size() == 0 ) && ! t.lookup( 10 ) && t.range( 0 , 100 ).empty() );
 CHECK( ( t.levels() == Lv( { { {} } } ) ) && ( t.begin() == t.end() ) );
 CHECK( t.order() == 3 );
 t.check();
 for( int x : { 10 , 12 , 13 } )
  t.insert( x , x );
 CHECK( t.levels() == Lv( { { { 10 , 12 , 13 } } } ) );                // (a)
 t.insert( 15 , 15 );
 CHECK( t.levels() == Lv( { { { 12 } } ,
                            { { 10 , 12 } , { 13 , 15 } } } ) );        // (b)
 t.insert( 8 , 8 );
 CHECK( t.levels() == Lv( { { { 12 } } ,
                            { { 8 , 10 , 12 } , { 13 , 15 } } } ) );    // (c)
 t.insert( 11 , 11 );
 CHECK( t.levels() == Lv( { { { 10 , 12 } } , { { 8 , 10 } , { 11 , 12 } ,
                                                 { 13 , 15 } } } ) );   // (d)
 t.insert( 9 , 9 );
 CHECK( t.levels() == Lv( { { { 10 , 12 } } , { { 8 , 9 , 10 } ,
                                                 { 11 , 12 } ,
                                                 { 13 , 15 } } } ) );   // (e)
 t.insert( 6 , 6 );
 CHECK( t.levels() == Lv( { { { 10 } } , { { 8 } , { 12 } } ,
                            { { 6 , 8 } , { 9 , 10 } , { 11 , 12 } ,
                              { 13 , 15 } } } ) );                      // 6.7
 t.check();
 CHECK( ( t.size() == 8 ) && ( t.lookup( 12 ) == 12 ) && ! t.lookup( 7 ) );
 using P = std::vector< std::pair< int , int > >;
 CHECK( t.range( 7 , 11 ) == P( { { 8 , 8 } , { 9 , 9 } , { 10 , 10 } ,
                                  { 11 , 11 } } ) );
 CHECK( t.range( 16 , 20 ).empty() && ( t.range( 0 , 100 ).size() == 8 ) );
 CHECK( t.range( 11 , 7 ).empty() && t.range( 0 , 5 ).empty() );
 CHECK( ( t.range( 12 , 12 ) == P( { { 12 , 12 } } ) ) &&
        t.range( 7 , 7 ).empty() );
 CHECK( ( t.range( 6 , 6 ) == P( { { 6 , 6 } } ) ) &&
        ( t.range( 15 , 15 ) == P( { { 15 , 15 } } ) ) );
 const auto before = t.levels();
 for( int x : { 6 , 9 , 10 , 15 } )        // the first and the last too
  t.insert( x , -x );
 CHECK( ( t.levels() == before ) && ( t.size() == 8 ) );
 CHECK( ( t.lookup( 9 ) == -9 ) && ( t.lookup( 15 ) == -15 ) );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 t.insert( hi , 1 );
 t.insert( lo , 2 );
 t.check();
 CHECK( ( t.range( lo , hi ).size() == 10 ) && ( t.lookup( lo ) == 2 ) );
 CHECK( ( t.range( lo , lo ) == P( { { lo , 2 } } ) ) &&
        ( t.range( hi , hi ) == P( { { hi , 1 } } ) ) );
 int n = 0 , last = lo;
 bool sorted = true;
 for( auto [ k , v ] : t ) {
  sorted = sorted && ( ( n == 0 ) || ( last < k ) );
  last = k;
  ++n;
  }
 CHECK( sorted && ( n == 10 ) && ( last == hi ) );
 CHECK_THROWS( ( BPlusTree< int , int >( 2 ) ) , contract_violation );
 CHECK_THROWS( ( BPlusTree< int , int >( 0 ) ) , contract_violation );
 CHECK_THROWS( ( BPlusTree< int , int >( -3 ) ) , contract_violation );
 }

// section 6.3: random, increasing and decreasing insertions in B+-trees of
// several orders against std::map; the shape holds, and the height is at
// most 2 + log_q( L / 2 ), q = ceil( m / 2 ), L the number of leaves; with
// few keys or a large order the root stays a leaf
ADS_TEST( "sec6.3.bplus-random" )
{
 std::mt19937 rng( 63 );
 for( int s = 0 ; s < 3 ; ++s )
  for( int m : { 3 , 4 , 5 , 8 , 64 } ) {
   BPlusTree< int , int > t( m );
   std::map< int , int > ref;
   std::uniform_int_distribution< int > key( 0 , 9999 );
   for( int i = 0 ; i < 5000 ; ++i ) {      // random, increasing, decreasing
    const int k = s == 0 ? key( rng ) : s == 1 ? 2 * i : 9999 - 2 * i;
    t.insert( k , i );
    ref[ k ] = i;
    }
   t.check();
   CHECK( t.size() == ref.size() );
   for( int k = 0 ; k < 10000 ; k += 7 ) {
    CHECK( t.lookup( k ) == find( ref , k ) );
    }
   const auto r = t.range( 2500 , 7499 );
   CHECK( std::equal( r.begin() , r.end() , ref.lower_bound( 2500 ) ,
                      ref.upper_bound( 7499 ) ,
                      []( auto & a , auto & b ) {
                       return( ( a.first == b.first ) &&
                               ( a.second == b.second ) ); } ) );
   const auto lv = t.levels();
   const double q = ( m + 1 ) / 2;
   CHECK( lv.size() <=
          2 + std::log( lv.back().size() / 2.0 ) / std::log( q ) );
   }
 for( int m : { 3 , 1000 } ) {             // the root stays a leaf
  BPlusTree< int , int > t( m );
  for( int k = 0 ; k < std::min( m , 500 ) ; ++k )
   t.insert( k , k );
  t.check();
  CHECK( ( t.levels().size() == 1 ) &&
         ( int( t.size() ) == std::min( m , 500 ) ) );
  t.insert( 2 , 0 );                       // in a full leaf, again
  CHECK( t.levels().size() == 1 );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- EXERCISES --------------------------------*/
/*--------------------------------------------------------------------------*/

// Exercise 6.1: building a search tree sorts, as the invisit reads the keys
// in order: so it costs Omega( n log n ) comparisons, as sorting does; with
// no key, one key, all equal keys (one is left) and repetitions
ADS_TEST( "ex6.1" )
{
 std::mt19937 rng( 601 );
 Ints k( 2000 );
 std::iota( k.begin() , k.end() , 0 );
 std::shuffle( k.begin() , k.end() , rng );
 const RBT t = build< RBT >( k );
 Ints s = k;
 std::sort( s.begin() , s.end() );
 CHECK( keys( t ) == s );
 CHECK( keys( build< RBT >( {} ) ).empty() );
 CHECK( keys( build< RBT >( { 4 } ) ) == Ints( { 4 } ) );
 CHECK( keys( build< RBT >( Ints( 50 , 3 ) ) ) == Ints( { 3 } ) );
 Ints r( 500 );
 for( int & x : r )
  x = int( rng() % 40 );
 std::set< int > u( r.begin() , r.end() );
 CHECK( keys( build< RBT >( r ) ) == Ints( u.begin() , u.end() ) );
 }

namespace {

// Exercise 6.2: insertNode() recursive, on a binary tree of keys: a new
// node where the search ends, nothing if the key is there
BinaryTree< int > * insertRec( BinaryTree< int > * T , int x )
{
 if( ! T )
  return( new BinaryTree< int >( x ) );
 if( x < T->read() ) {
  if( T->left() ) insertRec( T->left() , x );
  else T->insertLeft( new BinaryTree< int >( x ) );
  }
 else if( x > T->read() ) {
  if( T->right() ) insertRec( T->right() , x );
  else T->insertRight( new BinaryTree< int >( x ) );
  }
 return( T );
 }

Ints previsit( BinaryTree< int > * t )
{
 Ints out;
 pre_visit( t , [ & ]( BinaryTree< int > * u ) {
  out.push_back( u->read() ); } );
 return( out );
 }

}  // end( namespace )

// Exercise 6.2: the recursive insertion builds the same tree as the
// iterative insertNode(), also from no key, from one key, and from
// increasing and decreasing keys (a chain)
ADS_TEST( "ex6.2" )
{
 std::mt19937 rng( 602 );
 std::uniform_int_distribution< int > key( 0 , 300 );
 Ints k( 400 );
 for( int & x : k )
  x = key( rng );                          // with repetitions
 std::unique_ptr< BinaryTree< int > > t;
 for( int x : k )
  t.reset( insertRec( t.release() , x ) );
 CHECK( previsit( t.get() ) == preorder( build< BST >( k ) ) );
 Ints up( 300 ) , down( 300 );
 std::iota( up.begin() , up.end() , 0 );
 std::iota( down.rbegin() , down.rend() , 0 );
 for( const Ints & q : { Ints{} , Ints{ 8 } , up , down } ) {
  t.reset();
  for( int x : q )
   t.reset( insertRec( t.release() , x ) );
  CHECK( previsit( t.get() ) == preorder( build< BST >( q ) ) );
  CHECK( previsit( t.get() ) == q );       // a chain: the order given
  }
 }

namespace {

// Exercise 6.3: a binary tree is a search tree if each key is in the
// interval that its ancestors leave to it, ( lo , hi ), checked going down
bool isABR( BinaryTree< int > * t , const int * lo = nullptr ,
            const int * hi = nullptr )
{
 if( ! t )
  return( true );
 const int & v = t->read();
 if( ( lo && ( v <= *lo ) ) || ( hi && ( v >= *hi ) ) )
  return( false );
 return( isABR( t->left() , lo , &v ) && isABR( t->right() , &v , hi ) );
 }

}  // end( namespace )

// Exercise 6.3: the tree of figure 6.1 is a search tree; 10 with 5 on the
// left and 12 right of 5 is not, though each parent and child agree. The
// empty tree and a node alone are; a key equal to its parent is not, on
// either side, nor one equal to a farther ancestor; the extreme keys of int
// are within their intervals
ADS_TEST( "ex6.3" )
{
 std::unique_ptr< BinaryTree< int > > t;
 for( int x : fig61 )
  t.reset( insertRec( t.release() , x ) );
 CHECK( isABR( t.get() ) );
 std::unique_ptr< BinaryTree< int > > w( make( B( 10 , B( 5 , {} , B( 12 ) ) ,
                                                  B( 15 ) ) ) );
 CHECK( ! isABR( w.get() ) );
 w->left()->right()->write( 7 );
 CHECK( isABR( w.get() ) );
 CHECK( isABR( nullptr ) );
 BinaryTree< int > one( 0 );
 CHECK( isABR( &one ) );
 w->left()->right()->write( 10 );          // equal to the root
 CHECK( ! isABR( w.get() ) );
 for( auto s : { B( 5 , B( 5 ) ) , B( 5 , {} , B( 5 ) ) } ) {
  std::unique_ptr< BinaryTree< int > > e( make( s ) );
  CHECK( ! isABR( e.get() ) );
  }
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 std::unique_ptr< BinaryTree< int > > x( make( B( 0 , B( lo ) , B( hi ) ) ) );
 CHECK( isABR( x.get() ) );
 std::unique_ptr< BinaryTree< int > > y( make( B( lo , {} , B( hi ,
                                                  B( lo + 1 ) ) ) ) );
 CHECK( isABR( y.get() ) );
 y->right()->left()->write( lo );
 CHECK( ! isABR( y.get() ) );
 }

namespace {

// Solution of exercise 6.4: printNodes(), the invisit that skips the
// subtrees out of [ A , B ], in O( h + k ) time
void printNodes( BinaryTree< int > * t , int A , int B , Ints & out ,
                 int & seen )
{
 ++seen;
 const int v = t->read();
 if( t->left() && ( A < v ) ) printNodes( t->left() , A , B , out , seen );
 if( ( A <= v ) && ( v <= B ) ) out.push_back( v );
 if( t->right() && ( B > v ) ) printNodes( t->right() , A , B , out , seen );
 }

}  // end( namespace )

// Exercise 6.4: the keys of [ A , B ] in order, visiting few nodes more
// than they are; the intervals reversed, below and above all the keys, of
// one key present or absent, of all the keys, and a node alone
ADS_TEST( "ex6.4" )
{
 std::unique_ptr< BinaryTree< int > > t;
 for( int x : fig61 )
  t.reset( insertRec( t.release() , x ) );
 Ints out;
 int seen = 0;
 printNodes( t.get() , 8 , 30 , out , seen );
 CHECK( out == Ints( { 8 , 13 , 17 , 21 , 27 } ) );
 out.clear();
 seen = 0;
 printNodes( t.get() , 41 , 50 , out , seen );
 CHECK( ( out == Ints( { 42 } ) ) && ( seen == 4 ) );   // 17, 33, 40, 42
 // random: the keys of the interval, against a filter
 std::mt19937 rng( 604 );
 Ints k( 3000 );
 std::iota( k.begin() , k.end() , 1 );
 std::shuffle( k.begin() , k.end() , rng );
 t.reset();
 for( int x : k )
  t.reset( insertRec( t.release() , x ) );
 out.clear();
 seen = 0;
 printNodes( t.get() , 1000 , 1099 , out , seen );
 Ints ref( 100 );
 std::iota( ref.begin() , ref.end() , 1000 );
 CHECK( out == ref );
 CHECK( seen < 3 * 100 );   // O( h + k ), h about 2.99 log n on average
 auto range = [ & ]( int A , int B ) {
  out.clear();
  seen = 0;
  printNodes( t.get() , A , B , out , seen );
  return( out );
  };
 CHECK( range( 1099 , 1000 ).empty() && range( -5 , 0 ).empty() );
 CHECK( range( 3001 , 4000 ).empty() );
 CHECK( ( range( 7 , 7 ) == Ints( { 7 } ) ) &&
        ( range( 1 , 1 ) == Ints( { 1 } ) ) );
 CHECK( range( 3000 , 3000 ) == Ints( { 3000 } ) );
 Ints all( 3000 );
 std::iota( all.begin() , all.end() , 1 );
 CHECK( range( 0 , 3001 ) == all );
 BinaryTree< int > one( 5 );
 out.clear();
 printNodes( &one , 5 , 5 , out , seen );
 CHECK( out == Ints( { 5 } ) );
 out.clear();
 printNodes( &one , 6 , 9 , out , seen );
 CHECK( out.empty() );
 }

// Exercise 6.5: the iterative invisit with min() and successorNode(),
// with no stack, in Theta( n ) time; and backwards with max() and
// predecessorNode(); on random keys, on no key, one key, and the chains of
// increasing and decreasing keys
ADS_TEST( "ex6.5" )
{
 std::mt19937 rng( 605 );
 for( int c = 0 ; c < 5 ; ++c ) {
  Ints k( c == 1 ? 0 : c == 2 ? 1 : 1000 );
  std::iota( k.begin() , k.end() , 0 );
  if( c == 0 ) std::shuffle( k.begin() , k.end() , rng );
  if( c == 4 ) std::reverse( k.begin() , k.end() );
  const BST t = build< BST >( k );
  Ints in , back;
  for( auto u = t.min() ; ! t.isNil( u ) ; u = t.successorNode( u ) )
   in.push_back( BST::key( u ) );
  for( auto u = t.max() ; ! t.isNil( u ) ; u = t.predecessorNode( u ) )
   back.push_back( BST::key( u ) );
  Ints s = k;
  std::sort( s.begin() , s.end() );
  CHECK( in == s );
  std::reverse( back.begin() , back.end() );
  CHECK( back == s );
  }
 }

namespace {

// Exercise 6.6: the concatenation of T1 and T2, the keys of T1 smaller, in
// O( h ) time: the largest key of T1 is taken out (it has no right child)
// and becomes the root, T1 on its left and T2 on its right
struct N {
 int k;
 std::unique_ptr< N > l , r;
 };

using P = std::unique_ptr< N >;

P insertN( P t , int k )
{
 if( ! t )
  return( P( new N{ k , {} , {} } ) );
 if( k < t->k ) t->l = insertN( std::move( t->l ) , k );
 else t->r = insertN( std::move( t->r ) , k );
 return( t );
 }

P concatenate( P T1 , P T2 )
{
 if( ! T1 )
  return( T2 );
 P * m = &T1;                              // to the largest key of T1
 while( ( *m )->r )
  m = &( *m )->r;
 P root = std::move( *m );
 *m = std::move( root->l );                // its left subtree takes its place
 root->l = std::move( T1 );
 root->r = std::move( T2 );
 return( root );
 }

void inorder( const N * t , Ints & out )
{
 if( t ) {
  inorder( t->l.get() , out );
  out.push_back( t->k );
  inorder( t->r.get() , out );
  }
 }

int height( const N * t )
{
 return( t ? 1 + std::max( height( t->l.get() ) , height( t->r.get() ) )
           : 0 );
 }

}  // end( namespace )

// Exercise 6.6: all the keys, in order, and the height grows by one at most
ADS_TEST( "ex6.6" )
{
 std::mt19937 rng( 606 );
 for( int n1 : { 0 , 1 , 5 , 100 } )
  for( int n2 : { 0 , 1 , 7 , 100 } ) {
   Ints a( n1 ) , b( n2 );
   std::iota( a.begin() , a.end() , 0 );
   std::iota( b.begin() , b.end() , n1 );
   std::shuffle( a.begin() , a.end() , rng );
   std::shuffle( b.begin() , b.end() , rng );
   P T1 , T2;
   for( int x : a ) T1 = insertN( std::move( T1 ) , x );
   for( int x : b ) T2 = insertN( std::move( T2 ) , x );
   const int h = std::max( height( T1.get() ) , height( T2.get() ) );
   P T = concatenate( std::move( T1 ) , std::move( T2 ) );
   Ints out , all( n1 + n2 );
   inorder( T.get() , out );
   std::iota( all.begin() , all.end() , 0 );
   CHECK( out == all );
   CHECK( height( T.get() ) <= h + 1 );
   }
 }

namespace {

// Solution of exercise 6.7: union() and difference() start from a copy of A,
// in O( n_A ) time, and add or remove the keys of B; intersection() starts
// empty and visits the smaller set
template< class Tr >
Tr set_union( const Tr & A , const Tr & B )
{
 Tr C = A;
 for( auto u = B.min() ; ! B.isNil( u ) ; u = B.successorNode( u ) )
  if( ! A.contains( Tr::key( u ) ) )
   C.insert( Tr::key( u ) , 0 );
 return( C );
 }

template< class Tr >
Tr set_difference( const Tr & A , const Tr & B )
{
 Tr C = A;
 for( auto u = B.min() ; ! B.isNil( u ) ; u = B.successorNode( u ) )
  if( A.contains( Tr::key( u ) ) )
   C.remove( Tr::key( u ) );
 return( C );
 }

template< class Tr >
Tr set_intersection( const Tr & A , const Tr & B )
{
 const Tr & S = A.size() < B.size() ? A : B;
 const Tr & L = A.size() < B.size() ? B : A;
 Tr C;
 for( auto u = S.min() ; ! S.isNil( u ) ; u = S.successorNode( u ) )
  if( L.contains( Tr::key( u ) ) )
   C.insert( Tr::key( u ) , 0 );
 return( C );
 }

template< class Tr >
void sets( unsigned seed )
{
 std::mt19937 rng( seed );
 for( auto [ na , nb ] : { std::pair{ 0 , 0 } , std::pair{ 0 , 50 } ,
                           std::pair{ 50 , 0 } , std::pair{ 1 , 1 } ,
                           std::pair{ 300 , 20 } , std::pair{ 500 , 500 } } ) {
  std::uniform_int_distribution< int > key( 0 , 999 );
  std::set< int > a , b;
  Tr A , B;
  while( int( a.size() ) < na ) { int x = key( rng ); a.insert( x );
                                  A.insert( x , 0 ); }
  while( int( b.size() ) < nb ) { int x = key( rng ); b.insert( x );
                                  B.insert( x , 0 ); }
  Ints u , d , i;
  std::set_union( a.begin() , a.end() , b.begin() , b.end() ,
                  std::back_inserter( u ) );
  std::set_difference( a.begin() , a.end() , b.begin() , b.end() ,
                       std::back_inserter( d ) );
  std::set_intersection( a.begin() , a.end() , b.begin() , b.end() ,
                         std::back_inserter( i ) );
  const Tr U = set_union( A , B ) , D = set_difference( A , B ) ,
           I = set_intersection( A , B );
  CHECK( keys( U ) == u && keys( D ) == d && keys( I ) == i );
  CHECK( shape( U ).ok && shape( D ).ok && shape( I ).ok );
  CHECK( keys( A ) == Ints( a.begin() , a.end() ) );   // A unchanged
  // A with itself, with a subset of its own, with a disjoint set
  const Ints ka = keys( A );
  CHECK( ( keys( set_union( A , A ) ) == ka ) &&
         set_difference( A , A ).empty() &&
         ( keys( set_intersection( A , A ) ) == ka ) );
  Tr H , F;
  for( std::size_t j = 0 ; j < ka.size() ; j += 2 ) H.insert( ka[ j ] , 0 );
  for( int x : ka ) F.insert( x + 1000 , 0 );
  CHECK( ( keys( set_union( A , H ) ) == ka ) &&
         ( keys( set_intersection( A , H ) ) == keys( H ) ) &&
         ( set_difference( A , H ).size() == ka.size() / 2 ) );
  CHECK( set_intersection( A , F ).empty() &&
         ( keys( set_difference( A , F ) ) == ka ) &&
         ( set_union( A , F ).size() == 2 * ka.size() ) );
  }
 }

}  // end( namespace )

// Exercise 6.7: the three operations on sets as search trees, with empty
// sets, single keys, a set with itself, with a subset and with a disjoint
// set
ADS_TEST( "ex6.7" )
{
 sets< BST >( 607 );
 }

// Exercise 6.8: the same code on Red-Black trees, now O( log n ) per key,
// whose results are Red-Black trees again
ADS_TEST( "ex6.8" )
{
 sets< RBT >( 608 );
 }

namespace {

// Exercise 6.9: a binary tree can be colored if the root can be black. For
// each node, the sets of the numbers d of black nodes from it to a leaf
// (both included) that it can have if black and if red: a nil leaf has d = 1
// and is black; a black node needs its children, of any color, with the same
// d, and has d + 1; a red one needs black children with the same d, and has d
struct Colorings { std::set< int > black , red; };

Colorings colorings( BinaryTree< int > * t )
{
 if( ! t )
  return( Colorings{ { 1 } , {} } );
 const Colorings l = colorings( t->left() ) , r = colorings( t->right() );
 std::set< int > la = l.black , ra = r.black;
 la.insert( l.red.begin() , l.red.end() );
 ra.insert( r.red.begin() , r.red.end() );
 Colorings c;
 for( int d : la )
  if( ra.count( d ) ) c.black.insert( d + 1 );
 for( int d : l.black )
  if( r.black.count( d ) ) c.red.insert( d );
 return( c );
 }

bool colorable( BinaryTree< int > * t )
{
 return( ! colorings( t ).black.empty() );
 }

}  // end( namespace )

// Exercise 6.9: the trees (a), (c) and (d) of figure 6.9 can be colored;
// (b) cannot, as its root has a leaf at level 2 below 3 and one at level 5
// below 10, more than twice as deep (Theorem 6.2). The shapes that the
// Red-Black insertions build can all be colored, a path of 3 cannot; the
// empty tree, a node alone and a path of 2 can
ADS_TEST( "ex6.9" )
{
 std::unique_ptr< BinaryTree< int > > a( make(
  B( 1 , B( 2 ) , B( 3 , B( 4 ) , B( 5 ) ) ) ) );
 std::unique_ptr< BinaryTree< int > > b( make(
  B( 1 , B( 2 , B( 4 , B( 6 ) , B( 7 , {} , B( 10 ) ) ) ,
                B( 5 , B( 8 ) , B( 9 ) ) ) ,
         B( 3 ) ) ) );
 std::unique_ptr< BinaryTree< int > > c( make(
  B( 1 , B( 2 , B( 4 , B( 9 ) , B( 10 , B( 13 ) ) ) ,
                B( 5 , B( 11 ) , B( 12 ) ) ) ,
         B( 3 , B( 7 ) , B( 8 ) ) ) ) );
 std::unique_ptr< BinaryTree< int > > d( make(
  B( 1 , B( 2 , B( 4 , B( 9 ) , B( 10 , B( 13 ) , B( 14 ) ) ) ,
                B( 5 , B( 11 ) , B( 12 ) ) ) ,
         B( 3 , B( 7 ) , B( 8 ) ) ) ) );
 CHECK( colorable( a.get() ) );
 CHECK( ! colorable( b.get() ) );
 CHECK( colorable( c.get() ) );
 CHECK( colorable( d.get() ) );

 std::unique_ptr< BinaryTree< int > > path( make(
  B( 1 , {} , B( 2 , {} , B( 3 ) ) ) ) );
 CHECK( ! colorable( path.get() ) );
 CHECK( colorable( nullptr ) );
 BinaryTree< int > one( 1 );
 CHECK( colorable( &one ) );
 path->right()->deleteRight();
 CHECK( colorable( path.get() ) );         // black over red
 CHECK( colorings( path.get() ).black == std::set< int >( { 2 } ) );
 std::mt19937 rng( 609 );
 std::uniform_int_distribution< int > key( 0 , 999 );
 for( int n = 1 ; n <= 200 ; n += 13 ) {
  RBT t;
  while( int( t.size() ) < n )
   t.insert( key( rng ) , 0 );
  std::unique_ptr< BinaryTree< int > > s( to_binary_tree( t , t.root() ) );
  CHECK( colorable( s.get() ) );
  }
 }

namespace {

// a perfect binary tree of the given height, the levels from the root
// colored by color( level ); it returns the internal nodes
using CT = BinaryTree< Color >;

CT * perfect( int h , const std::function< Color( int ) > & color ,
              int level = 0 )
{
 if( h == 0 )
  return( nullptr );
 auto t = new CT( color( level ) );
 if( auto l = perfect( h - 1 , color , level + 1 ) ) t->insertLeft( l );
 if( auto r = perfect( h - 1 , color , level + 1 ) ) t->insertRight( r );
 return( t );
 }

/// the black height r( t ), leaves included, -1 if a constraint fails
int black_height( CT * t )
{
 if( ! t )
  return( 1 );
 const int a = black_height( t->left() ) , b = black_height( t->right() );
 if( ( a < 0 ) || ( a != b ) )
  return( -1 );
 if( t->read() == Color::RED )
  for( CT * c : { t->left() , t->right() } )
   if( c && ( c->read() == Color::RED ) )
    return( -1 );
 return( a + ( t->read() == Color::BLACK ) );
 }

int count( CT * t )
{
 return( t ? 1 + count( t->left() ) + count( t->right() ) : 0 );
 }

/// the shape of a perfect binary tree of the given height
BinaryTree< int > * perfect_shape( int h )
{
 if( h == 0 )
  return( nullptr );
 auto t = new BinaryTree< int >( h );
 if( auto l = perfect_shape( h - 1 ) ) t->insertLeft( l );
 if( auto r = perfect_shape( h - 1 ) ) t->insertRight( r );
 return( t );
 }

}  // end( namespace )

// Exercise 6.10: with black height b, at least 2^b - 1 internal nodes, the
// perfect black tree of height b, and at most 2^( 2 b ) - 1, the perfect
// tree of height 2 b with the levels black and red in turn; the perfect tree
// of height 2 b + 1 cannot have black height b, whatever the colors (the
// sets of exercise 6.9, where d = b + 1 counts the leaves too); b = 0 is
// the empty tree, and a node alone cannot have black height 0
ADS_TEST( "ex6.10" )
{
 for( int b = 0 ; b <= 6 ; ++b ) {
  std::unique_ptr< CT > lo( perfect( b , []( int ) { return( Color::BLACK );
                                                     } ) );
  std::unique_ptr< CT > hi( perfect( 2 * b , []( int l ) {
   return( l % 2 ? Color::RED : Color::BLACK ); } ) );
  CHECK( black_height( lo.get() ) == b + 1 );   // + 1 for the leaves
  CHECK( black_height( hi.get() ) == b + 1 );
  CHECK( count( lo.get() ) == ( 1 << b ) - 1 );
  CHECK( count( hi.get() ) == ( 1 << ( 2 * b ) ) - 1 );
  std::unique_ptr< BinaryTree< int > > s( perfect_shape( 2 * b ) ) ,
                                       m( perfect_shape( 2 * b + 1 ) );
  CHECK( colorings( s.get() ).black.count( b + 1 ) );
  CHECK( ! colorings( m.get() ).black.count( b + 1 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch06.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
