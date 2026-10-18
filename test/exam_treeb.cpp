/*--------------------------------------------------------------------------*/
/*------------------------- File exam_treeb.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the solutions of the exercises on trees of the written exams
 * collected with the book, from the 29th to the last (the others are in
 * exam_treea.cpp): each exercise solved as in its official solution, in
 * the namespace tree, and checked on the examples of its statement, on
 * random instances against an independent oracle (a brute force, a
 * quadratic visit, a structure of the library), and on its edge cases, at
 * the level 3 of the contracts. Where the official solution has an error,
 * the function implements the correct version and its comment says what
 * differs from the text. The helpers shared by the two programs are in
 * exam_tree.h.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 5,
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
#include "exam_tree.h"

#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/search/LocalOptima.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/ExpressionTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;
using namespace ADS::examtree;

/*--------------- tree-29, compito 2020-02-04, Sum angular -----------------*/

namespace tree {

/// sumAngular(): the sum of the values of the nodes first or last in their
/// level, each once; a visit by levels: when a level ends, the node just
/// extracted is its last and the front of the queue is the first of the
/// next level, added only if that level has two nodes or more, Theta( n )
/// time; the text enqueues t for T, and here an empty tree gives 0
long long sumAngular( BT * T )
{
 if( ! T )
  return( 0 );
 std::size_t count = 1;
 long long sumSoFar = 0;
 std::queue< BT * > Q;
 Q.push( T );
 while( ! Q.empty() ) {
  BT * t = Q.front();
  Q.pop();
  if( t->left() ) Q.push( t->left() );
  if( t->right() ) Q.push( t->right() );
  if( --count == 0 ) {                      // a new level
   count = Q.size();
   sumSoFar += t->read();
   if( count >= 2 )
    sumSoFar += Q.front()->read();
   }
  }
 return( sumSoFar );
 }

}  // end( namespace tree )

// the tree of the statement and that of the solution, whose angular nodes
// of the last level are children of the central ones, against a visit in
// depth that keeps the first and the last node of each level
ADS_TEST( "tree-29" )
{
 CHECK( tree::sumAngular( cat( "16( 10( 9( 2, 2 ), 3 ), 14( 8, 7( 4, 1 ) ) )"
                             ).get() ) == 59 );
 CHECK( tree::sumAngular( cat( "16( 10( 9, 3( -, 2 ) ), 14( 8( 4, - ), 7 ) )"
                             ).get() ) == 62 );
 CHECK( tree::sumAngular( nullptr ) == 0 );
 CHECK( tree::sumAngular( cat( "5( -, 3( 2, - ) )" ).get() ) == 10 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 15 , -9 , 9 );
  std::map< int , std::pair< BT * , BT * > > F;
  pre_visit( t.get() , [ & ]( BT * u ) {
   auto [ it , in ] = F.emplace( depthOf( u ) , std::pair( u , u ) );
   it->second.second = u;
   } );
  long long s = 0;
  for( const auto & [ l , P ] : F )
   s += P.first->read() + ( P.first != P.second ? P.second->read() : 0 );
  CHECK( tree::sumAngular( t.get() ) == s );
  }
 }

/*----------------- tree-30, compito 2020-06-12, Mh rec --------------------*/

namespace tree {

/// true if every value of t is larger than its parent, lb that of the
/// parent of t
bool mhRec( const BT * t , int lb )
{
 if( ! t )
  return( true );
 return( ( t->read() > lb ) && mhRec( t->left() , t->read() ) &&
         mhRec( t->right() , t->read() ) );
 }

/// isMinHeap(): true if the complete tree t has the property of a
/// min-heap, each child larger than its parent; a previsit that carries
/// the value of the parent down, Theta( n ) time
/// \pre t is not nil
bool isMinHeap( const BT * t )
{
 ADS_PRE( t != nullptr );
 return( mhRec( t->left() , t->read() ) && mhRec( t->right() , t->read() ) );
 }

}  // end( namespace tree )

// the two trees of the statement, against std::is_heap on the vector by
// levels
ADS_TEST( "tree-30" )
{
 using V = std::vector< int >;
 PT a( completeTree( V( { 1 , 3 , 5 , 4 , 6 , 10 , 11 , 8 , 9 , 12 } ) ) );
 PT b( completeTree( V( { 7 , 3 , 5 , 4 , 6 , 10 , 11 , 8 , 9 , 5 } ) ) );
 CHECK( tree::isMinHeap( a.get() ) );
 CHECK( ! tree::isMinHeap( b.get() ) );
 CHECK( tree::isMinHeap( cat( "4" ).get() ) );
 CHECK( ! tree::isMinHeap( cat( "4( 5, 4 )" ).get() ) );
 CHECK_THROWS( tree::isMinHeap( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  V A( 1 + r % 15 );
  std::iota( A.begin() , A.end() , 0 );
  std::shuffle( A.begin() , A.end() , g );
  if( r % 2 )
   std::make_heap( A.begin() , A.end() , std::greater<>() );
  CHECK( tree::isMinHeap( PT( completeTree( A ) ).get() ) ==
         std::is_heap( A.begin() , A.end() , std::greater<>() ) );
  }
 }

/*-------------- tree-31, compito 2020-07-03, Binary insert ----------------*/

namespace tree {

/// a node with the size of its subtree and a value
struct Slot {
 int size = 0 , value = 0;
 };

/// fills the values of t with A[ i .. ] in symmetric order
void biRec( BinaryTree< Slot > * t , const std::vector< int > & A ,
            std::size_t i )
{
 const int leftSize = t->left() ? t->left()->read().size : 0;
 if( t->left() )
  biRec( t->left() , A , i );
 t->write( Slot{ t->read().size , A[ i + leftSize ] } );
 if( t->right() )
  biRec( t->right() , A , i + leftSize + 1 );
 }

/// binaryInsert(): fills the values of the tree T, whose nodes know the
/// size of their subtree, with the sorted A, so that T becomes a search
/// tree of the same shape: the value of a node is the one after those of
/// its left subtree, Theta( n ) time
/// \pre T is not nil, A has as many distinct items as T, sorted
/// \post the symmetric visit of T gives A (audit)
void binaryInsert( BinaryTree< Slot > * T , const std::vector< int > & A )
{
 ADS_PRE( T && ( T->read().size == int( A.size() ) ) );
 ADS_PRE( std::adjacent_find( A.begin() , A.end() ,
                              std::greater_equal<>() ) == A.end() );
 biRec( T , A , 0 );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< int > V;
  in_visit( T , [ & ]( BinaryTree< Slot > * u ) {
   V.push_back( u->read().value ); } );
  return( V == A ); }() );
 }

}  // end( namespace tree )

namespace {

/// the tree of the shape of t, each node with the size of its subtree
BinaryTree< tree::Slot > * slotTree( BT * t )
{
 if( ! t )
  return( nullptr );
 auto * s = new BinaryTree< tree::Slot >( tree::Slot{ sizeOf( t ) , 0 } );
 if( auto * L = slotTree( t->left() ) )
  s->insertLeft( L );
 if( auto * R = slotTree( t->right() ) )
  s->insertRight( R );
 return( s );
 }

}  // end( namespace )

// the tree of the statement, and the search property of random shapes
ADS_TEST( "tree-31" )
{
 using ST = std::unique_ptr< BinaryTree< tree::Slot > >;
 PT e = cat( "25( 19( 13( 11, 15 ), 22( 21, - ) ), 28( 27, 31 ) )" );
 ST s( slotTree( e.get() ) );
 tree::binaryInsert( s.get() , { 11 , 13 , 15 , 19 , 21 , 22 , 25 , 27 , 28 ,
                                 31 } );
 std::vector< int > a , b;
 pre_visit( e.get() , [ & ]( BT * u ) { a.push_back( u->read() ); } );
 pre_visit( s.get() , [ & ]( BinaryTree< tree::Slot > * u ) {
  b.push_back( u->read().value ); } );
 CHECK( a == b );
 CHECK_THROWS( tree::binaryInsert( s.get() , { 1 , 2 } ) ,
               contract_violation );
 CHECK_THROWS( tree::binaryInsert( s.get() , { 1 , 2 , 3 , 4 , 5 , 6 , 7 ,
                                               8 , 8 , 9 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = 1 + r % 15;
  PT t = randomInts( g , n , 0 , 0 );
  ST T( slotTree( t.get() ) );
  std::vector< int > A( n );
  for( int i = 0 ; i < n ; ++i )
   A[ i ] = 3 * i - 20;
  tree::binaryInsert( T.get() , A );
  bool search = true;
  pre_visit( T.get() , [ & ]( BinaryTree< tree::Slot > * u ) {
   const int v = u->read().value;
   if( u->left() )
    in_visit( u->left() , [ & ]( BinaryTree< tree::Slot > * x ) {
     search = search && ( x->read().value < v ); } );
   if( u->right() )
    in_visit( u->right() , [ & ]( BinaryTree< tree::Slot > * x ) {
     search = search && ( x->read().value > v ); } );
   } );
  CHECK( search );
  }
 }

/*---------------- tree-32, compito 2020-07-24, Specchio -------------------*/

namespace tree {

/// true if the shape of b is the mirror image of that of a
bool isMirror( const BT * a , const BT * b )
{
 if( ! a && ! b )
  return( true );
 if( a && b )
  return( isMirror( a->right() , b->left() ) &&
          isMirror( a->left() , b->right() ) );
 return( false );
 }

/// isSymmetric(): true if the left subtree of the root has the shape of
/// the mirror image of the right one; a simultaneous visit of the two,
/// Theta( n ) time
/// \pre T is not nil
bool isSymmetric( const BT * T )
{
 ADS_PRE( T != nullptr );
 return( isMirror( T->left() , T->right() ) );
 }

}  // end( namespace tree )

// against the shape of the left subtree compared with the mirror shape of
// the right one; half of the random trees are made symmetric
ADS_TEST( "tree-32" )
{
 CHECK( tree::isSymmetric( cat( "1" ).get() ) );
 CHECK( tree::isSymmetric( cat( "1( 2( 3, - ), 4( -, 5 ) )" ).get() ) );
 CHECK( ! tree::isSymmetric( cat( "1( 2( 3, - ), 4( 5, - ) )" ).get() ) );
 CHECK( ! tree::isSymmetric( cat( "1( 2, - )" ).get() ) );
 CHECK_THROWS( tree::isSymmetric( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( new BT( 0 ) );
  PT L = randomInts( g , r % 8 , 0 , 9 );
  if( L ) {
   PT R( r % 2 ? randomInts( g , sizeOf( L.get() ) , 0 , 9 ).release()
               : nullptr );
   if( ! R ) {                              // the mirror image of L
    std::function< BT * ( BT * ) > mirror = [ & ]( BT * u ) -> BT * {
     if( ! u )
      return( nullptr );
     auto * m = new BT( u->read() );
     if( auto * a = mirror( u->right() ) ) m->insertLeft( a );
     if( auto * b = mirror( u->left() ) ) m->insertRight( b );
     return( m );
     };
    R.reset( mirror( L.get() ) );
    }
   t->insertLeft( L.release() );
   t->insertRight( R.release() );
   }
  CHECK( tree::isSymmetric( t.get() ) ==
         ( shapeOf( t->left() ) == mirrorShapeOf( t->right() ) ) );
  }
 }

/*------------ tree-33, compito 2021-01-18, Profondita minima --------------*/

namespace tree {

/// minDepth(): the least depth of a leaf, +inf for nil; a postvisit,
/// Theta( n ) time
/// \post at least 0
int minDepth( const BT * T )
{
 if( ! T )
  return( IINF );
 if( ! T->left() && ! T->right() )
  return( 0 );
 return( 1 + std::min( minDepth( T->left() ) , minDepth( T->right() ) ) );
 }

}  // end( namespace tree )

// against the depths of all the leaves
ADS_TEST( "tree-33" )
{
 CHECK( tree::minDepth( nullptr ) == IINF );
 CHECK( tree::minDepth( cat( "1" ).get() ) == 0 );
 CHECK( tree::minDepth( cat( "1( 2( 3, - ), - )" ).get() ) == 2 );
 CHECK( tree::minDepth( cat( "1( 2( 3, 4 ), 5( 6, - ) )" ).get() ) == 2 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 16 , 0 , 0 );
  int best = IINF;
  for( auto * l : leavesOf( t.get() ) )
   best = std::min( best , depthOf( l ) );
  CHECK( tree::minDepth( t.get() ) == best );
  }
 }

/*----------- tree-34, compito 2021-02-19, Da foglia a foglia --------------*/

namespace tree {

/// the heaviest path from t down to a leaf, and the heaviest path between
/// two distinct leaves of t, -inf for none
std::pair< long long , long long > leafToLeafRec( const BT * t )
{
 if( ! t )
  return( std::pair( -INFTY , -INFTY ) );
 const long long v = t->read();
 if( ! t->left() && ! t->right() )
  return( std::pair( v , -INFTY ) );
 const auto [ hL , bL ] = leafToLeafRec( t->left() );
 const auto [ hR , bR ] = leafToLeafRec( t->right() );
 return( std::pair( std::max( hL , hR ) + v ,
                    std::max( { bL , bR , hL + hR + v } ) ) );
 }

/// leafToLeaf(): the heaviest simple path between two distinct leaves,
/// the sum of its values; as maxPath(), Theta( n ) time; the text returns
/// ( v , v ) at a leaf, i.e., also a lone leaf as a path, here ( v , -inf )
/// \pre t has two leaves at least
long long leafToLeaf( const BT * t )
{
 ADS_PRE( t != nullptr );
 const long long b = leafToLeafRec( t ).second;
 ADS_PRE( b > -INFTY / 2 );                 // two leaves
 return( b );
 }

}  // end( namespace tree )

// the tree of the statement, against every pair of distinct leaves; with a
// leaf heavier than every path the code of the text would return it
ADS_TEST( "tree-34" )
{
 CHECK( tree::leafToLeaf( cat( "-5( 3( 5( 2, 4 ), 1( 7, - ) ), 9( -4, -9 ) )"
                             ).get() ) == 20 );
 CHECK( tree::leafToLeaf( cat( "0( 100, -200 )" ).get() ) == -100 );
 CHECK( tree::leafToLeaf( cat( "1( 2, 3 )" ).get() ) == 6 );
 CHECK_THROWS( tree::leafToLeaf( cat( "1( 2, - )" ).get() ) ,
               contract_violation );
 CHECK_THROWS( tree::leafToLeaf( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 3 + r % 14 , -9 , 9 );
  if( leavesOf( t.get() ).size() >= 2 )
   CHECK( tree::leafToLeaf( t.get() ) == leafPathsBrute( t.get() , false ) );
  }
 }

/*------------- tree-35, compito 2021-06-16, Albero completo ---------------*/

namespace tree {

/// true if every node of t, at place i of a heap, is within n
bool isInside( const BT * t , long long i , long long n )
{
 if( ! t )
  return( true );
 if( i > n )
  return( false );
 return( isInside( t->left() , 2 * i , n ) &&
         isInside( t->right() , 2 * i + 1 , n ) );
 }

/// isComplete(): true if t has the shape of a heap; with n nodes, every
/// node has a place at most n in the numbering of a heap, the root 1,
/// Theta( n ) time
bool isComplete( BT * t )
{
 return( isInside( t , 1 , sizeOf( t ) ) );
 }

}  // end( namespace tree )

// against a visit by levels in which no node follows an empty place;
// half of the random trees are complete
ADS_TEST( "tree-35" )
{
 CHECK( tree::isComplete( nullptr ) );
 CHECK( tree::isComplete( cat( "1( 2( 4, - ), 3 )" ).get() ) );
 CHECK( ! tree::isComplete( cat( "1( 2( -, 4 ), 3 )" ).get() ) );
 CHECK( ! tree::isComplete( cat( "1( 2( 4, 5 ), - )" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = 1 + r % 16;
  PT t( r % 2 ? completeTree( std::vector< int >( n , 0 ) )
              : randomInts( g , n , 0 , 0 ).release() );
  bool hole = false , ok = true;
  std::queue< BT * > Q;
  Q.push( t.get() );
  while( ! Q.empty() ) {
   BT * u = Q.front();
   Q.pop();
   if( ! u ) {
    hole = true;
    continue;
    }
   ok = ok && ! hole;
   Q.push( u->left() );
   Q.push( u->right() );
   }
  CHECK( tree::isComplete( t.get() ) == ok );
  CHECK( ok || ( r % 2 == 0 ) );
  }
 }

/*---------- tree-36, compito 2021-07-26, Alberi ben bilanciati ------------*/

namespace tree {

/// the size of t and whether t is well balanced
std::pair< int , bool > wbRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0 , true ) );
 const auto [ nL , bL ] = wbRec( t->left() );
 const auto [ nR , bR ] = wbRec( t->right() );
 return( std::pair( nL + nR + 1 , bL && bR && ( std::abs( nL - nR ) <= 1 ) ) );
 }

/// wellBalanced(): true if the sizes of the two subtrees of every node
/// differ by 1 at most; a postvisit returning the sizes, O( n ) time
bool wellBalanced( const BT * t )
{
 return( wbRec( t ).second );
 }

}  // end( namespace tree )

namespace {

/// a random well balanced tree of n nodes
BT * randomBalanced( std::mt19937 & g , int n )
{
 if( n == 0 )
  return( nullptr );
 const int a = ( n - 1 ) / 2 , b = n - 1 - a;
 const bool swap = g() % 2;
 auto * t = new BT( 0 );
 if( auto * L = randomBalanced( g , swap ? b : a ) ) t->insertLeft( L );
 if( auto * R = randomBalanced( g , swap ? a : b ) ) t->insertRight( R );
 return( t );
 }

}  // end( namespace )

// against the sizes of the two subtrees counted for every node; half of
// the random trees are well balanced
ADS_TEST( "tree-36" )
{
 CHECK( tree::wellBalanced( nullptr ) );
 CHECK( tree::wellBalanced( cat( "1( 2, - )" ).get() ) );
 CHECK( ! tree::wellBalanced( cat( "1( 2( 3, - ), - )" ).get() ) );
 CHECK( tree::wellBalanced( cat( "1( 2( 3, - ), 4( -, 5 ) )" ).get() ) );
 CHECK( ! tree::wellBalanced( cat( "1( 2( 3, 4 ), - )" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( r % 2 ? randomBalanced( g , r % 20 )
              : randomInts( g , r % 12 , 0 , 0 ).release() );
  bool ok = true;
  for( auto * u : nodesOf( t.get() ) )
   ok = ok && ( std::abs( sizeOf( u->left() ) - sizeOf( u->right() ) ) <= 1 );
  CHECK( tree::wellBalanced( t.get() ) == ok );
  CHECK( ok || ( r % 2 == 0 ) );
  }
 }

/*------------- tree-37, compito 2022-01-17, Alberi made up ----------------*/

namespace tree {

/// true if the subtree of t, at level l, is made up
bool makeupRec( const BT * t , int l )
{
 if( ! t )
  return( true );
 if( degreeOf( t ) % 2 != l % 2 )
  return( false );
 return( makeupRec( t->left() , l + 1 ) && makeupRec( t->right() , l + 1 ) );
 }

/// madeUp(): true if the nodes of odd level have one child and those of
/// even level 0 or 2, the root at level 0; a visit that carries the level
/// down, O( n ) time
/// \pre T is not nil
bool madeUp( const BT * T )
{
 ADS_PRE( T != nullptr );
 return( makeupRec( T , 0 ) );
 }

}  // end( namespace tree )

namespace {

/// a random made up tree below level l, with no node of even level beyond
/// D with children
BT * randomMadeUp( std::mt19937 & g , int l , int D )
{
 auto * t = new BT( l );
 if( l % 2 ) {
  if( g() % 2 ) t->insertLeft( randomMadeUp( g , l + 1 , D ) );
  else t->insertRight( randomMadeUp( g , l + 1 , D ) );
  }
 else
  if( ( l < D ) && ( g() % 3 ) ) {
   t->insertLeft( randomMadeUp( g , l + 1 , D ) );
   t->insertRight( randomMadeUp( g , l + 1 , D ) );
   }
 return( t );
 }

}  // end( namespace )

// against the children of every node by levels; half of the random trees
// are made up, and some of those are spoiled
ADS_TEST( "tree-37" )
{
 CHECK( tree::madeUp( cat( "1" ).get() ) );
 CHECK( tree::madeUp( cat( "1( 2( 3, - ), 4( -, 5 ) )" ).get() ) );
 CHECK( ! tree::madeUp( cat( "1( 2, - )" ).get() ) );
 CHECK( ! tree::madeUp( cat( "1( 2( 3, - ), 4 )" ).get() ) );
 CHECK_THROWS( tree::madeUp( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( r % 2 ? randomMadeUp( g , 0 , 6 )
              : randomInts( g , 1 + r % 10 , 0 , 0 ).release() );
  if( r % 6 == 1 ) {                        // a new leaf somewhere
   const auto N = nodesOf( t.get() );
   BT * u = N[ g() % N.size() ];
   if( ! u->left() ) u->insertLeft( new BT( 0 ) );
   else if( ! u->right() ) u->insertRight( new BT( 0 ) );
   }
  bool ok = true;
  for( auto * u : nodesOf( t.get() ) )
   ok = ok && ( depthOf( u ) % 2 ? degreeOf( u ) == 1
                                 : degreeOf( u ) != 1 );
  CHECK( tree::madeUp( t.get() ) == ok );
  }
 }

/*------------- tree-38, compito 2022-02-07, Alberi pasciuti ---------------*/

namespace tree {

/// isPlump(): true if every level l of the tree, up to its height, has
/// 2^( l - 1 ) nodes at least; a visit by levels that checks the size of
/// each level when the previous one ends, O( n ) time
/// \pre T is not nil
bool isPlump( BT * T )
{
 ADS_PRE( T != nullptr );
 std::queue< BT * > Q;
 Q.push( T );
 std::size_t count = 1;
 int l = 0;
 while( ! Q.empty() ) {
  BT * t = Q.front();
  Q.pop();
  if( t->left() ) Q.push( t->left() );
  if( t->right() ) Q.push( t->right() );
  if( --count == 0 ) {                      // the size of level l + 1
   count = Q.size();
   ++l;
   if( ( count > 0 ) && ( ( l > 63 ) ||
                          ( count < ( std::uint64_t( 1 ) << ( l - 1 ) ) ) ) )
    return( false );
   }
  }
 return( true );
 }

}  // end( namespace tree )

// against the sizes of the levels counted by a visit in depth; half of
// the random trees are complete trees with a few nodes cut
ADS_TEST( "tree-38" )
{
 CHECK( tree::isPlump( cat( "1" ).get() ) );
 CHECK( tree::isPlump( cat( "1( 2, - )" ).get() ) );
 CHECK( ! tree::isPlump( cat( "1( 2( 3, - ), - )" ).get() ) );
 CHECK( tree::isPlump( cat( "1( 2( 3, - ), 4( -, 5 ) )" ).get() ) );
 CHECK( tree::isPlump( cat( "1( 2( 3( 6, 7 ), 8( 9, 10 ) ), 4( -, 5 ) )"
                          ).get() ) );
 CHECK( ! tree::isPlump( cat( "1( 2( 3( 6, 7 ), 8( 9, - ) ), 4( -, 5 ) )"
                            ).get() ) );
 CHECK_THROWS( tree::isPlump( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( r % 2 ? completeTree( std::vector< int >( 1 + r % 31 , 0 ) )
              : randomInts( g , 1 + r % 12 , 0 , 0 ).release() );
  for( int k = r % 2 ? int( g() % 6 ) : 0 ; k > 0 ; --k ) {
   const auto L = leavesOf( t.get() );
   BT * u = L[ g() % L.size() ];
   if( u->parent() ) {
    if( u == u->parent()->left() ) u->parent()->deleteLeft();
    else u->parent()->deleteRight();
    }
   }
  std::map< int , std::uint64_t > level;
  for( auto * u : nodesOf( t.get() ) )
   ++level[ depthOf( u ) ];
  bool ok = true;
  for( const auto & [ l , c ] : level )
   ok = ok && ( ( l == 0 ) || ( c >= ( std::uint64_t( 1 ) << ( l - 1 ) ) ) );
  CHECK( tree::isPlump( t.get() ) == ok );
  }
 }

/*--------------- tree-39, compito 2022-06-21, Lato destro -----------------*/

namespace tree {

/// rightSide(): the last node of every level, by increasing level; a
/// visit by levels that counts the nodes left in the current level,
/// Theta( n ) time
/// \pre T is not nil
/// \post a node per level
template< class T >
std::vector< BinaryTree< T > * > rightSide( BinaryTree< T > * T0 )
{
 ADS_PRE( T0 != nullptr );
 std::vector< BinaryTree< T > * > S;
 std::queue< BinaryTree< T > * > Q;
 Q.push( T0 );
 std::size_t count = 1;
 while( ! Q.empty() ) {
  auto * t = Q.front();
  Q.pop();
  if( t->left() ) Q.push( t->left() );
  if( t->right() ) Q.push( t->right() );
  if( --count == 0 ) {                      // the last of its level
   S.push_back( t );
   count = Q.size();
   }
  }
 ADS_POST( int( S.size() ) == heightOf( T0 ) + 1 );
 return( S );
 }

}  // end( namespace tree )

// a tree like that of the statement, whose node d is a left child and e
// is not below the right side; against a visit in depth, left before
// right, that keeps the last node of each level
ADS_TEST( "tree-39" )
{
 auto e = cat< char >( "a( f( g( h( e, - ), - ), - ), b( -, c( d, - ) ) )" );
 std::string S;
 for( auto * u : tree::rightSide( e.get() ) )
  S += u->read();
 CHECK( S == "abcde" );
 CHECK( tree::rightSide( cat( "1" ).get() ).size() == 1 );
 CHECK_THROWS( tree::rightSide< int >( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 15 , 0 , 0 );
  std::map< int , BT * > last;
  pre_visit( t.get() , [ & ]( BT * u ) { last[ depthOf( u ) ] = u; } );
  std::vector< BT * > V;
  for( const auto & [ l , u ] : last )
   V.push_back( u );
  CHECK( tree::rightSide( t.get() ) == V );
  }
 }

/*-------- tree-40, compito 2023-01-17, Albero binario massimale -----------*/

namespace tree {

/// the maximal tree of A[ i .. j ]
BT * mtRec( const std::vector< int > & A , int i , int j )
{
 if( j < i )
  return( nullptr );
 int m = i;
 for( int k = i + 1 ; k <= j ; ++k )
  if( A[ k ] > A[ m ] )
   m = k;
 auto * t = new BT( A[ m ] );
 if( auto * L = mtRec( A , i , m - 1 ) ) t->insertLeft( L );
 if( auto * R = mtRec( A , m + 1 , j ) ) t->insertRight( R );
 return( t );
 }

/// maxTree(): the maximal binary tree of A, the maximum at the root and the
/// maximal trees of the items on its left and on its right as subtrees;
/// the recursion of the definition, Theta( n log n ) time in the best case
/// and Theta( n^2 ) in the worst (A sorted); the caller owns the result
/// \pre the items of A are distinct
BT * maxTree( const std::vector< int > & A )
{
 ADS_PRE( std::set< int >( A.begin() , A.end() ).size() == A.size() );
 return( mtRec( A , 0 , int( A.size() ) - 1 ) );
 }

/// maxTreeLinear(): the maximal binary tree of A, the Cartesian tree with
/// the maximum at the root, by cartesianBinaryTree() of the library with a
/// stack of the right spine: each item pops the smaller ones, the last of
/// which becomes its left child, and becomes the right child of the new
/// top; Theta( n ) time; the caller owns the result
/// \pre the items of A are distinct
BT * maxTreeLinear( const std::vector< int > & A )
{
 ADS_PRE( std::set< int >( A.begin() , A.end() ).size() == A.size() );
 return( cartesianBinaryTree( A , std::greater< int >() ) );
 }

}  // end( namespace tree )

// the two examples of the statement and of the solution, both algorithms
// against the symmetric visit (A itself) and the heap property
ADS_TEST( "tree-40" )
{
 using V = std::vector< int >;
 for( auto * f : { tree::maxTree , tree::maxTreeLinear } ) {
  CHECK( to_list( PT( f( V( { 2 , 3 , -1 , 8 , 4 , 5 } ) ) ).get() ) ==
         to_list( cat( "8( 3( 2, -1 ), 5( 4, - ) )" ).get() ) );
  CHECK( to_list( PT( f( V( { 2 , 1 , 5 , 3 , 0 , 4 } ) ) ).get() ) ==
         to_list( cat( "5( 2( -, 1 ), 4( 3( -, 0 ), - ) )" ).get() ) );
  CHECK( f( V() ) == nullptr );
  CHECK( heightOf( PT( f( V( { 1 , 2 , 3 , 4 , 5 , 6 } ) ) ).get() ) == 5 );
  CHECK_THROWS( f( V( { 1 , 2 , 1 } ) ) , contract_violation );
  }
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  V A( r % 20 );
  std::iota( A.begin() , A.end() , -5 );
  std::shuffle( A.begin() , A.end() , g );
  PT a( tree::maxTree( A ) ) , b( tree::maxTreeLinear( A ) );
  CHECK( to_list( a.get() ) == to_list( b.get() ) );
  V I;
  bool heap = true;
  in_visit( a.get() , [ & ]( BT * u ) {
   I.push_back( u->read() );
   heap = heap && ! ( u->parent() && ( u->parent()->read() < u->read() ) );
   } );
  CHECK( ( I == A ) && heap );
  }
 }

/*---------------- tree-42, compito 2023-06-20, Tree fire ------------------*/

namespace tree {

/// the time to burn the nodes reached from t not through prev
int fireRec( const BT * t , const BT * prev )
{
 int max = 0;
 for( const BT * u : { t->left() , t->right() , t->parent() } )
  if( u && ( u != prev ) )
   max = std::max( max , 1 + fireRec( u , t ) );
 return( max );
 }

/// fire(): the time to burn all the tree from t, the fire passing in a unit
/// of time to the children and to the parent; a visit of the tree as an
/// unoriented graph that never goes back where it came from, O( n ) time
/// \pre t is not nil
/// \post at least 0
int fire( const BT * t )
{
 ADS_PRE( t != nullptr );
 return( fireRec( t , nullptr ) );
 }

}  // end( namespace tree )

// against the largest distance from t of a visit in breadth of the library
ADS_TEST( "tree-42" )
{
 PT e = cat( "1( 2( 4, 5( 6, - ) ), 3 )" );
 CHECK( tree::fire( e.get() ) == 3 );
 CHECK( tree::fire( findValue( e.get() , 6 ) ) == 4 );
 CHECK( tree::fire( e->left() ) == 2 );
 CHECK( tree::fire( cat( "1" ).get() ) == 0 );
 CHECK_THROWS( tree::fire( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  PT t = randomInts( g , 1 + r % 15 , 0 , 0 );
  const auto N = nodesOf( t.get() );
  const auto G = graphOf( N );
  for( std::size_t i = 0 ; i < N.size() ; ++i ) {
   const auto d = distance( G , int( i ) ).first;
   CHECK( tree::fire( N[ i ] ) == *std::max_element( d.begin() , d.end() ) );
   }
  }
 }

/*------------- tree-43, compito 2023-06-20, Somma su albero ---------------*/

namespace tree {

/// the sum of the values of t, +inf if t is not a sum tree
long long isRec( const BT * t )
{
 if( ! t )
  return( 0 );
 if( ! t->left() && ! t->right() )
  return( t->read() );
 const long long sL = isRec( t->left() ) , sR = isRec( t->right() );
 if( ( sL != INFTY ) && ( sR != INFTY ) && ( t->read() == sL + sR ) )
  return( 2LL * t->read() );
 return( INFTY );
 }

/// isSumTree(): true if the value of each internal node is the sum of the
/// values of its two subtrees; a postvisit returning the sum of each
/// subtree, or +inf if it is not a sum tree, O( n ) time
bool isSumTree( const BT * T )
{
 return( isRec( T ) != INFTY );
 }

}  // end( namespace tree )

// against the sums of the two subtrees computed for every internal node;
// half of the random trees are made sum trees
ADS_TEST( "tree-43" )
{
 CHECK( tree::isSumTree( cat( "20( 5( 1, 4 ), 5( 5, - ) )" ).get() ) );
 CHECK( ! tree::isSumTree( cat( "20( 5( 1, 4 ), 5( 4, - ) )" ).get() ) );
 CHECK( tree::isSumTree( nullptr ) );
 CHECK( tree::isSumTree( cat( "-3" ).get() ) );
 CHECK( tree::isSumTree( cat( "0( -2, 2 )" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 14 , -3 , 3 );
  if( r % 2 ) {
   auto N = nodesOf( t.get() );
   std::reverse( N.begin() , N.end() );     // the children first
   for( auto * u : N )
    if( ! isLeafNode( u ) )
     u->write( int( sumOf( u->left() ) + sumOf( u->right() ) ) );
   if( r % 6 == 1 )
    N[ g() % N.size() ]->write( 99 );
   }
  bool ok = true;
  for( auto * u : nodesOf( t.get() ) )
   ok = ok && ( isLeafNode( u ) ||
                ( u->read() == sumOf( u->left() ) + sumOf( u->right() ) ) );
  CHECK( tree::isSumTree( t.get() ) == ok );
  }
 }

/*------------- tree-44, compito 2023-07-11, Perfect inside ----------------*/

namespace tree {

/// the height of the largest perfect tree in t, and that of the largest
/// one rooted in t, -1 for nil
std::pair< int , int > lpRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0 , -1 ) );
 const auto [ maxL , curL ] = lpRec( t->left() );
 const auto [ maxR , curR ] = lpRec( t->right() );
 const int current = std::min( curL , curR ) + 1;
 return( std::pair( std::max( { maxL , maxR , current } ) , current ) );
 }

/// largestPerfect(): the height of the largest perfect tree made of a node
/// and its descendants down to some depth; the largest one rooted in a
/// node is one level taller than the smaller of those of its children, a
/// postvisit, O( n ) time
/// \post at least 0
int largestPerfect( const BT * T )
{
 return( lpRec( T ).first );
 }

}  // end( namespace tree )

namespace {

/// true if the nodes of t down to depth h - 1 have two children
bool perfectTop( const BT * t , int h )
{
 if( h == 0 )
  return( true );
 return( t->left() && t->right() && perfectTop( t->left() , h - 1 ) &&
         perfectTop( t->right() , h - 1 ) );
 }

}  // end( namespace )

// the tree of the figure of the solution, rebuilt from the values that it
// shows at each node, against every node and every height
ADS_TEST( "tree-44" )
{
 CHECK( tree::largestPerfect( cat( "1( 2( 3( 4( 5, 6 ), 7 ), 8( 9, 10 ) ), "
                                   "11( -, 12( -, 13 ) ) )" ).get() ) == 2 );
 CHECK( tree::largestPerfect( nullptr ) == 0 );
 CHECK( tree::largestPerfect( cat( "1" ).get() ) == 0 );
 CHECK( tree::largestPerfect( cat( "1( 2( 3, - ), 4( 5, 6 ) )" ).get() )
        == 1 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( r % 3 ? randomInts( g , 1 + r % 16 , 0 , 0 ).release()
              : completeTree( std::vector< int >( 1 + r % 40 , 0 ) ) );
  int best = 0;
  for( auto * u : nodesOf( t.get() ) )
   for( int h = 1 ; perfectTop( u , h ) ; ++h )
    best = std::max( best , h );
  CHECK( tree::largestPerfect( t.get() ) == best );
  }
 }

/*--------------- tree-45, compito 2023-08-23, Prossimita ------------------*/

namespace tree {

/// the level and the place in a heap (the root 0) of x below t, which is
/// at level l and place pos; ( -1 , -1 ) if x is not there
std::pair< int , long long > locate( const BT * t , const BT * x , int l ,
                                     long long pos )
{
 if( ! t )
  return( std::pair( -1 , -1LL ) );
 if( t == x )
  return( std::pair( l , pos ) );
 const auto L = locate( t->left() , x , l + 1 , 2 * pos + 1 );
 if( L.first >= 0 )
  return( L );
 const auto R = locate( t->right() , x , l + 1 , 2 * pos + 2 );
 if( R.first >= 0 )
  return( R );
 return( std::pair( -1 , -1LL ) );
 }

/// proximity(): the nodes of the perfect tree of the same height between
/// t1 and t2 if they are on the same level, +inf otherwise; their places
/// in the numbering of a heap differ by the proximity plus one, O( n )
/// time
/// \pre t1 and t2 are distinct nodes of T, of depth at most 61
/// \post at least 0
long long proximity( const BT * T , const BT * t1 , const BT * t2 )
{
 ADS_PRE( t1 != t2 );
 const auto [ l1 , p1 ] = locate( T , t1 , 0 , 0 );
 const auto [ l2 , p2 ] = locate( T , t2 , 0 , 0 );
 ADS_PRE( ( l1 >= 0 ) && ( l2 >= 0 ) && ( l1 <= 61 ) && ( l2 <= 61 ) );
 if( l1 == l2 )
  return( std::abs( p2 - p1 ) - 1 );
 return( INFTY );
 }

}  // end( namespace tree )

namespace {

/// the path from the root to t as a binary number, left 0 and right 1, and
/// its length; the first step from the root is the most significant bit
std::pair< long long , int > pathBits( const BT * t )
{
 long long b = 0;
 int l = 0;
 for( ; t->parent() ; t = t->parent() , ++l )
  if( t == t->parent()->right() )
   b |= 1LL << l;
 return( std::pair( b , l ) );
 }

}  // end( namespace )

// a tree like that of the statement, with t1 and t2 three places apart on
// the same level and t3 on another, against the paths as binary numbers
ADS_TEST( "tree-45" )
{
 PT e = cat( "0( 1( 2( 3, - ), 4 ), 5( 6( 7( 8, - ), - ), 9 ) )" );
 BT * t1 = findValue( e.get() , 3 ) , * t2 = findValue( e.get() , 7 ) ,
    * t3 = findValue( e.get() , 9 );
 CHECK( tree::proximity( e.get() , t1 , t2 ) == 3 );
 CHECK( tree::proximity( e.get() , t2 , t1 ) == 3 );
 CHECK( tree::proximity( e.get() , t1 , t3 ) == INFTY );
 CHECK( tree::proximity( e.get() , e->left() , e->right() ) == 0 );
 CHECK_THROWS( tree::proximity( e.get() , t1 , t1 ) , contract_violation );
 PT o = cat( "1" );
 CHECK_THROWS( tree::proximity( e.get() , t1 , o.get() ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  PT t = randomInts( g , 2 + r % 15 , 0 , 0 );
  const auto N = nodesOf( t.get() );
  for( auto * a : N )
   for( auto * b : N )
    if( a != b ) {
     const auto [ x , lx ] = pathBits( a );
     const auto [ y , ly ] = pathBits( b );
     CHECK( tree::proximity( t.get() , a , b ) ==
            ( lx == ly ? std::abs( x - y ) - 1 : INFTY ) );
     }
  }
 }

/*--------------- tree-46, compito 2024-07-02, Piu o meno ------------------*/

namespace tree {

/// the edges of the longest plus-or-minus-k path from t down to a leaf
/// (-inf if none), and the longest one in t
std::pair< long long , long long > PMRec( const BT * t , int k )
{
 if( ! t )
  return( std::pair( -INFTY , -INFTY ) );
 if( ! t->left() && ! t->right() )
  return( std::pair( 0LL , 0LL ) );
 auto [ currL , maxL ] = PMRec( t->left() , k );
 auto [ currR , maxR ] = PMRec( t->right() , k );
 if( t->left() && ( currL != -INFTY ) &&
     ( std::abs( t->read() - t->left()->read() ) <= k ) )
  ++currL;
 else
  currL = -INFTY;
 if( t->right() && ( currR != -INFTY ) &&
     ( std::abs( t->read() - t->right()->read() ) <= k ) )
  ++currR;
 else
  currR = -INFTY;
 const long long curr = std::max( currL , currR );
 return( std::pair( curr , std::max( { maxL , maxR , curr } ) ) );
 }

/// longestPlusMinus(): the edges of the longest path from a node down to a
/// leaf below it whose every node differs from its parent by k at most; a
/// postvisit returning the longest such path from the node, -inf when
/// none, and the longest one inside, Theta( n ) time
/// \pre T is not nil, k >= 0
/// \post at least 0
long long longestPlusMinus( const BT * T , int k )
{
 ADS_PRE( ( T != nullptr ) && ( k >= 0 ) );
 return( PMRec( T , k ).second );
 }

}  // end( namespace tree )

// the tree of the statement, against every node and every leaf below it
ADS_TEST( "tree-46" )
{
 PT e = cat( "5( 3( 4( 3, 4( 3, - ) ), 2 ), 6( 4, 8( -, 7 ) ) )" );
 CHECK( tree::longestPlusMinus( e.get() , 1 ) == 3 );
 CHECK( tree::longestPlusMinus( e.get() , 0 ) == 0 );
 CHECK( tree::longestPlusMinus( e.get() , 3 ) == 4 );
 CHECK( tree::longestPlusMinus( cat( "1( 9( 8, - ), - )" ).get() , 1 ) == 1 );
 CHECK_THROWS( tree::longestPlusMinus( nullptr , 1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 15 , 0 , 6 );
  const int k = r % 3;
  long long best = 0;
  for( auto * s : nodesOf( t.get() ) )
   for( auto * l : leavesOf( s ) ) {
    bool ok = true;
    for( auto * u = l ; u != s ; u = u->parent() )
     ok = ok && ( std::abs( u->read() - u->parent()->read() ) <= k );
    if( ok )
     best = std::max( best , 1LL * ( depthOf( l ) - depthOf( s ) ) );
    }
  CHECK( tree::longestPlusMinus( t.get() , k ) == best );
  }
 }

/*------------- tree-47, compito 2024-08-26, Alberi isobati ----------------*/

namespace tree {

/// -1 for nil, -inf if the leaves of t are not all at the same depth,
/// their depth below t otherwise
long long helper( const BT * t )
{
 if( ! t )
  return( -1 );
 const long long pL = helper( t->left() ) , pR = helper( t->right() );
 if( ( pL == -INFTY ) || ( pR == -INFTY ) )
  return( -INFTY );
 if( pL == -1 )
  return( pR + 1 );
 if( pR == -1 )
  return( pL + 1 );
 return( pL == pR ? pL + 1 : -INFTY );
 }

/// isIsobath(): true if all the leaves are at the same depth, true for an
/// empty tree; a postvisit returning the depth of the leaves below each
/// node, O( n ) time
bool isIsobath( const BT * T )
{
 return( helper( T ) != -INFTY );
 }

}  // end( namespace tree )

namespace {

/// a random tree whose leaves are all at depth d below the root
BT * randomIsobath( std::mt19937 & g , int d )
{
 auto * t = new BT( 0 );
 if( d > 0 ) {
  const unsigned c = 1 + g() % 3;           // left, right or both
  if( c & 1u ) t->insertLeft( randomIsobath( g , d - 1 ) );
  if( c & 2u ) t->insertRight( randomIsobath( g , d - 1 ) );
  }
 return( t );
 }

}  // end( namespace )

// against the depths of all the leaves; half of the random trees are
// isobath
ADS_TEST( "tree-47" )
{
 CHECK( tree::isIsobath( nullptr ) );
 CHECK( tree::isIsobath( cat( "1" ).get() ) );
 CHECK( tree::isIsobath( cat( "1( 2( 3, - ), 4( -, 5 ) )" ).get() ) );
 CHECK( ! tree::isIsobath( cat( "1( 2( 3, - ), 4 )" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( r % 2 ? randomIsobath( g , r % 6 )
              : randomInts( g , 1 + r % 12 , 0 , 0 ).release() );
  std::set< int > D;
  for( auto * l : leavesOf( t.get() ) )
   D.insert( depthOf( l ) );
  CHECK( tree::isIsobath( t.get() ) == ( D.size() == 1 ) );
  }
 }

/*------------ tree-48, compito 2025-02-03, Somma ramificata ---------------*/

namespace tree {

/// the sum of the values at distance at most k from curr, not through prev
long long bsRec( const BT * curr , int k , const BT * prev )
{
 if( ! curr )
  return( 0 );
 long long res = curr->read();
 if( k > 0 )
  for( const BT * nb : { curr->left() , curr->right() , curr->parent() } )
   if( nb != prev )
    res += bsRec( nb , k - 1 , curr );
 return( res );
 }

/// branchSum(): the sum of the values of the nodes at distance at most k
/// from t, t included; a visit of the tree as an unoriented graph that
/// never goes back where it came from, O( min( n , 3^k ) ) time
/// \pre t is not nil, k >= 0
long long branchSum( const BT * t , int k )
{
 ADS_PRE( ( t != nullptr ) && ( k >= 0 ) );
 return( bsRec( t , k , nullptr ) );
 }

}  // end( namespace tree )

// the tree of the statement, against the distances of a visit in breadth
// of the library from every node
ADS_TEST( "tree-48" )
{
 PT e = cat( "7( 7( 5( 8, 3 ), 7( 3( -2, -1 ), - ) ), "
             "8( 6( 1, 2 ), 6( -, 3 ) ) )" );
 BT * t = e->left()->right();
 CHECK( tree::branchSum( t , 2 ) == 26 );
 CHECK( tree::branchSum( t , 0 ) == 7 );
 CHECK( tree::branchSum( t , 100 ) == sumOf( e.get() ) );
 CHECK_THROWS( tree::branchSum( t , -1 ) , contract_violation );
 CHECK_THROWS( tree::branchSum( nullptr , 1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  PT u = randomInts( g , 1 + r % 15 , -9 , 9 );
  const auto N = nodesOf( u.get() );
  const auto G = graphOf( N );
  for( std::size_t i = 0 ; i < N.size() ; ++i ) {
   const auto d = distance( G , int( i ) ).first;
   for( int k = 0 ; k <= 4 ; ++k ) {
    long long s = 0;
    for( std::size_t j = 0 ; j < N.size() ; ++j )
     if( d[ j ] <= k )
      s += N[ j ]->read();
    CHECK( tree::branchSum( N[ i ] , k ) == s );
    }
   }
  }
 }

/*-------------- tree-49, compito 2025-07-14, Albero bello -----------------*/

namespace tree {

/// the size of the largest full subtree in t, and that of t if t is full,
/// -1 otherwise
std::pair< int , int > abRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0 , -1 ) );
 if( ! t->left() && ! t->right() )
  return( std::pair( 1 , 1 ) );
 const auto [ maxL , currL ] = abRec( t->left() );
 const auto [ maxR , currR ] = abRec( t->right() );
 if( ( currL == -1 ) || ( currR == -1 ) )
  return( std::pair( std::max( maxL , maxR ) , -1 ) );
 const int curr = currL + currR + 1;
 return( std::pair( curr , curr ) );
 }

/// largestAB(): the size of the largest subtree (a node and all its
/// descendants) whose nodes have 0 or 2 children; a postvisit returning the
/// largest one inside and the size of the node's own, Theta( n ) time
/// \post at least 0
int largestAB( const BT * T )
{
 return( abRec( T ).first );
 }

}  // end( namespace tree )

namespace {

/// true if every node of t has 0 or 2 children
bool fullBrute( BT * t )
{
 const auto N = nodesOf( t );
 return( std::all_of( N.begin() , N.end() , []( BT * u ) {
  return( degreeOf( u ) != 1 ); } ) );
 }

}  // end( namespace )

// the tree of the figure of the solution, rebuilt from the values that it
// shows at each node, against the subtree of every node
ADS_TEST( "tree-49" )
{
 CHECK( tree::largestAB( cat( "1( 2( 3( 4( 5, 6 ), 7 ), 8( 9, 10 ) ), "
                              "11( -, 12( -, 13 ) ) )" ).get() ) == 9 );
 CHECK( tree::largestAB( nullptr ) == 0 );
 CHECK( tree::largestAB( cat( "1" ).get() ) == 1 );
 CHECK( tree::largestAB( cat( "1( 2( 3, 4 ), - )" ).get() ) == 3 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , 0 , 0 );
  int best = 0;
  for( auto * u : nodesOf( t.get() ) )
   if( fullBrute( u ) )
    best = std::max( best , sizeOf( u ) );
  CHECK( tree::largestAB( t.get() ) == best );
  }
 }

/*---------- tree-50, compito 2025-08-25, Scendere nell'albero -------------*/

namespace tree {

/// localMinimum(): a node of the perfect tree T smaller than its parent
/// and its children: localMinimum() of the library, from the root down to
/// the first smaller child, so that the node reached is smaller than its
/// parent, until a node smaller than its children or a leaf, O( log n )
/// time
/// \pre T is a perfect tree with distinct values
/// \post a local minimum
BT * localMinimum( BT * T )
{
 ADS_PRE( T != nullptr );
 ADS_AUDIT( std::ranges::all_of( nodesOf( T ) , []( const BT * u ) {
  return( ! u->left() == ! u->right() ); } ) );
 return( ADS::localMinimum( T ) );
 }

}  // end( namespace tree )

namespace {

/// true if t is smaller than its parent and its children
bool isLocalMinimum( const BT * t )
{
 for( const BT * u : { t->left() , t->right() , t->parent() } )
  if( u && ( u->read() < t->read() ) )
   return( false );
 return( true );
 }

}  // end( namespace )

// the tree of the statement, against the local minima of random perfect
// trees found by a visit of all the nodes
ADS_TEST( "tree-50" )
{
 using V = std::vector< int >;
 PT e( completeTree( V( { 13 , 7 , 5 , 15 , 11 , 12 , 4 , 2 , 8 , 14 , 6 , 1 ,
                          9 , 10 , 3 } ) ) );
 BT * m = tree::localMinimum( e.get() );
 CHECK( ( m->read() == 7 ) && isLocalMinimum( m ) );
 std::set< int > M;
 for( auto * u : nodesOf( e.get() ) )
  if( isLocalMinimum( u ) )
   M.insert( u->read() );
 CHECK( M == std::set< int >( { 7 , 2 , 8 , 6 , 1 , 9 , 3 } ) );
 CHECK( tree::localMinimum( cat( "5" ).get() )->read() == 5 );
 CHECK_THROWS( tree::localMinimum( cat( "5( 4, - )" ).get() ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  V A( ( 1 << ( 1 + r % 6 ) ) - 1 );
  std::iota( A.begin() , A.end() , 0 );
  std::shuffle( A.begin() , A.end() , g );
  PT t( completeTree( A ) );
  BT * u = tree::localMinimum( t.get() );
  CHECK( isLocalMinimum( u ) && ( depthOf( u ) <= 5 ) );
  }
 }

/*--------------- tree-51, compito 2026-01-12, Alberobello -----------------*/

namespace tree {

/// the full subtrees in t, and whether t is full (false for nil)
std::pair< int , bool > abCountRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0 , false ) );
 if( ! t->left() && ! t->right() )
  return( std::pair( 1 , true ) );
 const auto [ totL , niceL ] = abCountRec( t->left() );
 const auto [ totR , niceR ] = abCountRec( t->right() );
 if( niceL && niceR )
  return( std::pair( totL + totR + 1 , true ) );
 return( std::pair( totL + totR , false ) );
 }

/// countAB(): the nodes whose subtree has only nodes with 0 or 2
/// children; a postvisit returning their number and whether the node's
/// own subtree is such, O( n ) time
/// \post at least 0
int countAB( const BT * T )
{
 return( abCountRec( T ).first );
 }

}  // end( namespace tree )

// the tree of the figure of the solution, rebuilt from the values that it
// shows at each node, against the subtree of every node
ADS_TEST( "tree-51" )
{
 CHECK( tree::countAB( cat( "1( 2( 3( 4( 5, 6 ), 7 ), 8( 9, 10 ) ), "
                            "11( 12( 13, 14 ), 15( 16, - ) ) )" ).get() )
        == 13 );
 CHECK( tree::countAB( nullptr ) == 0 );
 CHECK( tree::countAB( cat( "1" ).get() ) == 1 );
 CHECK( tree::countAB( cat( "1( 2( 3, 4 ), - )" ).get() ) == 3 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , 0 , 0 );
  int count = 0;
  for( auto * u : nodesOf( t.get() ) )
   count += fullBrute( u );
  CHECK( tree::countAB( t.get() ) == count );
  }
 }

/*------------- tree-52, compito 2026-02-02, Albero sommato ----------------*/

namespace tree {

/// summedTree(): a new tree of the shape of t whose nodes hold the sums of
/// the values of the subtrees of t; a postvisit that builds the children
/// first, Theta( n ) time; the caller owns the result
/// \post the shape of t
BT * summedTree( const BT * t )
{
 if( ! t )
  return( nullptr );
 auto * s = new BT( 0 );
 if( auto * L = summedTree( t->left() ) ) s->insertLeft( L );
 if( auto * R = summedTree( t->right() ) ) s->insertRight( R );
 s->write( t->read() + ( s->left() ? s->left()->read() : 0 ) +
           ( s->right() ? s->right()->read() : 0 ) );
 return( s );
 }

}  // end( namespace tree )

// the trees of the statement, against the sum of the subtree of every
// node; the input is not modified
ADS_TEST( "tree-52" )
{
 PT e = cat( "6( 4( 2( 1, 3 ), 1 ), 2( 1( 3, 2 ), 1( -, 1 ) ) )" );
 const std::string before = to_list( e.get() );
 CHECK( to_list( PT( tree::summedTree( e.get() ) ).get() ) ==
        to_list( cat( "27( 11( 6( 1, 3 ), 1 ), 10( 6( 3, 2 ), 2( -, 1 ) ) )"
                    ).get() ) );
 CHECK( to_list( e.get() ) == before );
 CHECK( tree::summedTree( nullptr ) == nullptr );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 15 , -1000 , 1000 );
  PT s( tree::summedTree( t.get() ) );
  const auto N = nodesOf( t.get() ) , S = nodesOf( s.get() );
  CHECK( shapeOf( t.get() ) == shapeOf( s.get() ) );
  for( std::size_t i = 0 ; i < N.size() ; ++i )
   CHECK( S[ i ]->read() == sumOf( N[ i ] ) );
  }
 }

/*--------- tree-53, compito 2026-06-22, Nodi valore-bilanciati ------------*/

namespace tree {

/// the sum of the values of t and its value-balanced nodes
std::pair< long long , int > valueBalancedRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0LL , 0 ) );
 const auto [ sL , cL ] = valueBalancedRec( t->left() );
 const auto [ sR , cR ] = valueBalancedRec( t->right() );
 return( std::pair( sL + sR + t->read() , cL + cR + ( sL == sR ? 1 : 0 ) ) );
 }

/// valueBalanced(): the nodes whose two subtrees have the same sum of
/// values, 0 for an empty one; a postvisit returning the sums, Theta( n )
/// time
/// \post at least the leaves
int valueBalanced( const BT * T )
{
 return( valueBalancedRec( T ).second );
 }

}  // end( namespace tree )

// the tree of the statement, against the sums of the two subtrees of every
// node
ADS_TEST( "tree-53" )
{
 CHECK( tree::valueBalanced( cat( "5( 6( 5( -1, 1 ), 5( 0, - ) ), "
                                  "7( 5( 3, -3 ), 3( 1, 1 ) ) )" ).get() )
        == 11 );
 CHECK( tree::valueBalanced( nullptr ) == 0 );
 CHECK( tree::valueBalanced( cat( "4" ).get() ) == 1 );
 CHECK( tree::valueBalanced( cat( "4( 0, - )" ).get() ) == 2 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , -2 , 2 );
  int count = 0;
  for( auto * u : nodesOf( t.get() ) )
   count += ( sumOf( u->left() ) == sumOf( u->right() ) );
  CHECK( tree::valueBalanced( t.get() ) == count );
  }
 }

/*------------ tree-54, compito 2026-07-20, Parenti serpenti ---------------*/

namespace tree {

/// the nodes at distance at most d from t, t included, not through p
int crRec( const Tree< int > * t , const Tree< int > * p , int d )
{
 int count = 1;
 if( d > 0 ) {
  const Tree< int > * u = t->parent();
  if( u && ( u != p ) )
   count += crRec( u , t , d - 1 );
  for( u = t->leftmostChild() ; u ; u = u->rightSibling() )
   if( u != p )
    count += crRec( u , t , d - 1 );
  }
 return( count );
 }

/// countRelatives(): the nodes other than t at distance at most d from it
/// in the tree; a visit along the parents and the children that never
/// goes back where it came from, O( n ) time
/// \pre t is not nil, d >= 0
/// \post at least 0
int countRelatives( const Tree< int > * t , int d )
{
 ADS_PRE( ( t != nullptr ) && ( d >= 0 ) );
 return( crRec( t , nullptr , d ) - 1 );
 }

}  // end( namespace tree )

// a tree like that of the statement, with 8 relatives at distance 2,
// against the distances of a visit in breadth of the library
ADS_TEST( "tree-54" )
{
 using GT = Tree< int >;
 std::unique_ptr< GT > e( tree_from_list< int >(
  "( 1 ( 2 ( 5 ) ( 6 ( 11 ) ) ) ( 3 ( 7 ) ) "
  "( 4 ( 8 ( 12 ) ( 13 ) ) ( 9 ) ( 10 ) ) )" ) );
 GT * t = nullptr;
 pre_visit( e.get() , [ & ]( GT * u ) { if( u->read() == 4 ) t = u; } );
 CHECK( tree::countRelatives( t , 2 ) == 8 );
 CHECK( tree::countRelatives( t , 0 ) == 0 );
 CHECK( tree::countRelatives( t , 9 ) == 12 );
 CHECK_THROWS( tree::countRelatives( t , -1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = 1 + r % 15;
  std::vector< GT * > N( 1 , new GT( 0 ) );
  std::unique_ptr< GT > root( N[ 0 ] );
  ListGraph<> G( n , false );
  for( int i = 1 ; i < n ; ++i ) {
   const int p = int( g() % i );
   N.push_back( new GT( i ) );
   N[ p ]->insertChild( N.back() );
   G.insertEdge( p , i );
   }
  for( int i = 0 ; i < n ; ++i ) {
   const auto dist = distance( G , i ).first;
   for( int d = 0 ; d <= 4 ; ++d )
    CHECK( tree::countRelatives( N[ i ] , d ) ==
           std::count_if( dist.begin() , dist.end() , [ & ]( int x ) {
            return( ( x > 0 ) && ( x <= d ) ); } ) );
   }
  }
 }

/*--------- tree-55, compito 2026-08-28, Sottoalbero ABR massimo -----------*/

namespace tree {

/// the least and the largest key of t, its size and whether t is a search
/// tree; if it is not, the size of the largest search tree in t
struct BSTInfo {
 long long min , max;
 int size;
 bool isBST;
 };

/// the summary of the subtree of t
BSTInfo largestBSTRec( const BT * t )
{
 if( ! t )
  return( BSTInfo{ INFTY , -INFTY , 0 , true } );
 const BSTInfo L = largestBSTRec( t->left() ) ,
               R = largestBSTRec( t->right() );
 const long long k = t->read();
 if( L.isBST && R.isBST && ( L.max < k ) && ( k < R.min ) )
  return( BSTInfo{ std::min( L.min , k ) , std::max( k , R.max ) ,
                   L.size + R.size + 1 , true } );
 return( BSTInfo{ 0 , 0 , std::max( L.size , R.size ) , false } );
 }

/// largestBST(): the nodes of the largest subtree (a node and all its
/// descendants) that is a binary search tree, 0 for an empty tree; a
/// postvisit returning the extremes, the size and the validity of each
/// subtree, Theta( n ) time
/// \post at least 0
int largestBST( const BT * T )
{
 return( largestBSTRec( T ).size );
 }

}  // end( namespace tree )

// the tree of the solution, whose comparisons with the children all look
// right, against the symmetric visit of every subtree
ADS_TEST( "tree-55" )
{
 CHECK( tree::largestBST( cat( "10( 5, 15( 6, - ) )" ).get() ) == 2 );
 CHECK( tree::largestBST( cat( "10( 5, 15( 12, - ) )" ).get() ) == 4 );
 CHECK( tree::largestBST( nullptr ) == 0 );
 CHECK( tree::largestBST( cat( "3" ).get() ) == 1 );
 CHECK( tree::largestBST( cat( "3( 3, - )" ).get() ) == 1 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , 0 , 9 );
  int best = 0;
  for( auto * u : nodesOf( t.get() ) ) {
   std::vector< int > I;
   in_visit( u , [ & ]( BT * x ) { I.push_back( x->read() ); } );
   if( std::adjacent_find( I.begin() , I.end() , std::greater_equal<>() ) ==
       I.end() )
    best = std::max( best , int( I.size() ) );
   }
  CHECK( tree::largestBST( t.get() ) == best );
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
/*----------------------- End File exam_treeb.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
