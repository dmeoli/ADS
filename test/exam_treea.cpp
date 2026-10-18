/*--------------------------------------------------------------------------*/
/*------------------------- File exam_treea.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the solutions of the exercises on trees of the written exams
 * collected with the book, from the first to the 28th (the others are in
 * exam_treeb.cpp): each exercise solved as in its official solution, in
 * the namespace tree, and checked on the examples of its statement, on
 * random instances against an independent oracle (a brute force, a
 * quadratic visit, a structure of the library), and on its edge cases, at
 * the level 3 of the contracts. Where the official solution has an error,
 * the function implements the correct version and its comment says what
 * differs from the text. The helpers shared by the two programs are in
 * exam_tree.h.
 *
 * The edge cases of each test, besides those its comment names, are the
 * empty tree and the single node where allowed, the deep chains to the
 * left and to the right (DEEP nodes), the perfect and the complete trees,
 * the extreme values of int, and each precondition violated once.
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

namespace {

/// a degenerate tree of n >= 1 nodes, each one the left (or the right)
/// child of the one before, the node at depth d with the value value( d )
template< class F >
PT chainTree( int n , bool right , F && value )
{
 PT t( new BT( value( 0 ) ) );
 BT * u = t.get();
 for( int d = 1 ; d < n ; ++d ) {
  if( right ) {
   u->insertRight( new BT( value( d ) ) );
   u = u->right();
   }
  else {
   u->insertLeft( new BT( value( d ) ) );
   u = u->left();
   }
  }
 return( t );
 }

/// the length of the deep chains, far beyond the random trees (the
/// recursive visits still fit in the stack with the address sanitizer)
constexpr int DEEP = 5000;

}  // end( namespace )

/*-------- tree-1, compito 2011-05-02, Valutazione di espressioni ----------*/

namespace {

/// the value of the fully parenthesized infix form s of an expression of
/// natural numbers, + - *, from the place i, by recursive descent
long long parseInfix( const std::string & s , std::size_t & i )
{
 while( s[ i ] == ' ' ) ++i;
 if( s[ i ] != '(' ) {
  long long v = 0;
  while( ( i < s.size() ) &&
         std::isdigit( static_cast< unsigned char >( s[ i ] ) ) )
   v = 10 * v + ( s[ i++ ] - '0' );
  return( v );
  }
 ++i;
 const long long a = parseInfix( s , i );
 while( s[ i ] == ' ' ) ++i;
 const char op = s[ i++ ];
 const long long b = parseInfix( s , i );
 while( s[ i ] == ' ' ) ++i;
 ++i;                                       // the closing parenthesis
 return( op == '+' ? a + b : ( op == '-' ? a - b : a * b ) );
 }

/// a random postfix expression with k leaves in [ 0 , 9 ] and + - *
std::string randomPostfix( std::mt19937 & g , int k )
{
 if( k == 1 )
  return( std::to_string( g() % 10 ) );
 const int l = std::uniform_int_distribution< int >( 1 , k - 1 )( g );
 return( randomPostfix( g , l ) + " " + randomPostfix( g , k - l ) + " " +
         "+-*"[ g() % 3 ] );
 }

}  // end( namespace )

// the library evaluates the tree of the statement, and random trees as the
// recursive descent on their infix form; the malformed expressions, the
// variables, the division by 0 and a deep left chain of operators
ADS_TEST( "tree-1" )
{
 using E = std::unique_ptr< BinaryTree< std::string > >;
 E e( expressionTree( "4 10 2 - * 5 +" ) );
 CHECK( to_list( e.get() ) == to_list( cat< std::string >(
                               "+( *( 4, -( 10, 2 ) ), 5 )" ).get() ) );
 CHECK( evaluate( e.get() ) == 37 );
 CHECK( evaluate( E( expressionTree( "7" ) ).get() ) == 7 );
 CHECK( evaluate( E( expressionTree( "2 9 -" ) ).get() ) == -7 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  E t( expressionTree( randomPostfix( g , 1 + r % 8 ) ) );
  std::size_t i = 0;
  CHECK( evaluate( t.get() ) == double( parseInfix( infixForm( t.get() ) ,
                                                     i ) ) );
  }
 CHECK( evaluate( E( expressionTree( "  12   30 *  " ) ).get() ) == 360 );
 CHECK( evaluate( E( expressionTree( "0" ) ).get() ) == 0 );
 CHECK( evaluate( E( expressionTree( "x 2 *" ) ).get() ,
                  { { "x" , 3.5 } } ) == 7 );
 CHECK_THROWS( evaluate( E( expressionTree( "x 2 *" ) ).get() ) ,
               contract_violation );
 CHECK_THROWS( evaluate( E( expressionTree( "1 0 /" ) ).get() ) ,
               contract_violation );
 CHECK_THROWS( evaluate( nullptr ) , contract_violation );
 CHECK_THROWS( expressionTree( "" ) , contract_violation );
 CHECK_THROWS( expressionTree( "+" ) , contract_violation );
 CHECK_THROWS( expressionTree( "1 +" ) , contract_violation );
 CHECK_THROWS( expressionTree( "1 2" ) , contract_violation );
 std::string deep = "1";                    // ( ( 1 + 1 ) + 1 ) + ...
 for( int k = 0 ; k < 5000 ; ++k )
  deep += " 1 +";
 CHECK( evaluate( E( expressionTree( deep ) ).get() ) == 5001 );
 }

/*----------------- tree-2, compito 2011-05-27, Maxdepth -------------------*/

namespace tree {

/// maxdepth(): the edges of the longest strictly increasing path from the
/// root t down to any of its descendants, -1 for an empty tree; a visit
/// that stops at the first child not larger than its parent, Theta( n )
/// time, O( h ) space
/// \pre the keys are distinct
/// \post at least -1
int maxDepth( const BT * t )
{
 if( t && ( ( t->parent() == nullptr ) ||
            ( t->parent()->read() < t->read() ) ) )
  return( 1 + std::max( maxDepth( t->left() ) , maxDepth( t->right() ) ) );
 return( -1 );
 }

}  // end( namespace tree )

// against the walk up from every node to the root
ADS_TEST( "tree-2" )
{
 CHECK( tree::maxDepth( nullptr ) == -1 );
 CHECK( tree::maxDepth( cat( "5" ).get() ) == 0 );
 CHECK( tree::maxDepth( cat( "1( 2( -, 3( 4, - ) ), - )" ).get() ) == 3 );
 CHECK( tree::maxDepth( cat( "9( 2( -, 3 ), 1 )" ).get() ) == 0 );
 CHECK( tree::maxDepth( cat( "3( 2, 4( 1, 6( 5, - ) ) )" ).get() ) == 2 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomDistinct( g , 1 + r % 12 );
  int best = 0;
  for( auto * x : nodesOf( t.get() ) ) {
   bool up = true;
   for( auto * u = x ; u->parent() ; u = u->parent() )
    up = up && ( u->parent()->read() < u->read() );
   if( up )
    best = std::max( best , depthOf( x ) );
   }
  CHECK( tree::maxDepth( t.get() ) == best );
  }
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( tree::maxDepth( cat( "1( 1, 1 )" ).get() ) == 0 );  // not strictly
 CHECK( tree::maxDepth( chainTree( 2 , true , [ & ]( int d ) {
  return( d ? hi : lo ); } ).get() ) == 1 );
 CHECK( tree::maxDepth( chainTree( DEEP , false , []( int d ) {
  return( d ); } ).get() ) == DEEP - 1 );
 CHECK( tree::maxDepth( chainTree( DEEP , true , []( int d ) {
  return( -d ); } ).get() ) == 0 );
 }

/*---------------- tree-3, compito 2011-09-07, Unbalance -------------------*/

namespace tree {

/// the leaves of t and the largest unbalance of a node of t
std::pair< int , int > unbalanceRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0 , 0 ) );
 if( ! t->left() && ! t->right() )
  return( std::pair( 1 , 0 ) );
 const auto [ lL , mL ] = unbalanceRec( t->left() );
 const auto [ lR , mR ] = unbalanceRec( t->right() );
 return( std::pair( lL + lR ,
                    std::max( { mL , mR , std::abs( lL - lR ) } ) ) );
 }

/// unbalance(): the largest difference between the leaves of the left and
/// of the right subtree of a node; a postvisit that returns the leaves and
/// the largest difference of each subtree, Theta( n ) time
/// \post at least 0
int unbalance( const BT * t )
{
 const int u = unbalanceRec( t ).second;
 ADS_POST( u >= 0 );
 return( u );
 }

}  // end( namespace tree )

// against the leaves of the two subtrees counted for every node
ADS_TEST( "tree-3" )
{
 CHECK( tree::unbalance( nullptr ) == 0 );
 CHECK( tree::unbalance( cat( "1" ).get() ) == 0 );
 CHECK( tree::unbalance( cat( "1( 2( 3, 4 ), - )" ).get() ) == 2 );
 CHECK( tree::unbalance( cat( "1( 2( 3, 4( 5, 6 ) ), 7 )" ).get() ) == 2 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 15 , 0 , 0 );
  int best = 0;
  for( auto * u : nodesOf( t.get() ) )
   best = std::max( best , std::abs( int( leavesOf( u->left() ).size() ) -
                                     int( leavesOf( u->right() ).size() ) ) );
  CHECK( tree::unbalance( t.get() ) == best );
  }
 CHECK( tree::unbalance( cat( "1( 2, 3 )" ).get() ) == 0 );
 CHECK( tree::unbalance( cat( "1( 2, - )" ).get() ) == 1 );
 CHECK( tree::unbalance( chainTree( DEEP , false , []( int ) {
  return( 0 ); } ).get() ) == 1 );
 PT c( completeTree( std::vector< int >( 31 , 0 ) ) );  // perfect
 CHECK( tree::unbalance( c.get() ) == 0 );
 }

/*---------------- tree-4, compito 2012-06-18, Same level ------------------*/

namespace tree {

/// the nodes of t whose key is their level, t at the level l
int sameLevelRec( const BT * t , int l )
{
 if( ! t )
  return( 0 );
 return( sameLevelRec( t->right() , l + 1 ) +
         sameLevelRec( t->left() , l + 1 ) + ( t->read() == l ? 1 : 0 ) );
 }

/// sameLevel(): the nodes whose key is their level, the root at level 0;
/// a visit that carries the level down, Theta( n ) time
/// \post at least 0
int sameLevel( const BT * t )
{
 return( sameLevelRec( t , 0 ) );
 }

}  // end( namespace tree )

// against a visit by levels
ADS_TEST( "tree-4" )
{
 CHECK( tree::sameLevel( nullptr ) == 0 );
 CHECK( tree::sameLevel( cat( "0" ).get() ) == 1 );
 CHECK( tree::sameLevel( cat( "0( 1( 2, 5 ), 1( -, 2 ) )" ).get() ) == 5 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 15 , 0 , 4 );
  int count = 0;
  std::queue< std::pair< BT * , int > > Q;
  if( t )
   Q.push( std::pair( t.get() , 0 ) );
  while( ! Q.empty() ) {
   const auto [ u , l ] = Q.front();
   Q.pop();
   count += ( u->read() == l );
   for( auto * c : { u->left() , u->right() } )
    if( c )
     Q.push( std::pair( c , l + 1 ) );
   }
  CHECK( tree::sameLevel( t.get() ) == count );
  }
 CHECK( tree::sameLevel( cat( "1" ).get() ) == 0 );
 CHECK( tree::sameLevel( cat( "-1( 1, 1 )" ).get() ) == 2 );
 CHECK( tree::sameLevel( chainTree( DEEP , true , []( int d ) {
  return( d ); } ).get() ) == DEEP );
 CHECK( tree::sameLevel( chainTree( DEEP , false , []( int d ) {
  return( d + 1 ); } ).get() ) == 0 );
 }

/*------------- tree-6, compito 2012-09-10, Longest sequence ---------------*/

namespace tree {

/// longestSequence(): the nodes of the longest strictly increasing path
/// from t down; a visit of the children larger than their parent, O( n )
/// time
/// \pre t is not nil
/// \post at least 1
int longestSequence( const BT * t )
{
 ADS_PRE( t != nullptr );
 int max = 1;
 if( t->left() && ( t->left()->read() > t->read() ) )
  max = std::max( max , longestSequence( t->left() ) + 1 );
 if( t->right() && ( t->right()->read() > t->read() ) )
  max = std::max( max , longestSequence( t->right() ) + 1 );
 return( max );
 }

}  // end( namespace tree )

// against the walk up from each descendant of t
ADS_TEST( "tree-6" )
{
 PT e = cat( "7( 8( 9, 5 ), 3( 4, 5 ) )" );
 CHECK( tree::longestSequence( e.get() ) == 3 );
 CHECK( tree::longestSequence( e->left() ) == 2 );
 CHECK( tree::longestSequence( e->right() ) == 2 );
 CHECK( tree::longestSequence( e->right()->left() ) == 1 );
 CHECK( tree::longestSequence( cat( "4( 4, 4 )" ).get() ) == 1 );
 CHECK_THROWS( tree::longestSequence( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  PT t = randomInts( g , 1 + r % 12 , 0 , 6 );
  for( auto * s : nodesOf( t.get() ) ) {
   int best = 1;
   for( auto * x : nodesOf( s ) ) {
    bool up = true;
    for( auto * u = x ; u != s ; u = u->parent() )
     up = up && ( u->parent()->read() < u->read() );
    if( up )
     best = std::max( best , depthOf( x ) - depthOf( s ) + 1 );
    }
   CHECK( tree::longestSequence( s ) == best );
   }
  }
 CHECK( tree::longestSequence( cat( "5" ).get() ) == 1 );
 CHECK( tree::longestSequence( cat( "5( 4, 6 )" ).get() ) == 2 );
 CHECK( tree::longestSequence( chainTree( DEEP , false , []( int d ) {
  return( d ); } ).get() ) == DEEP );
 CHECK( tree::longestSequence( chainTree( DEEP , true , []( int d ) {
  return( -d ); } ).get() ) == 1 );
 }

/*----------------- tree-7, compito 2013-01-28, Is full --------------------*/

namespace tree {

/// isFull(): true if every node of t has 0 or 2 children; a visit that
/// stops at the first node with one child, Theta( n ) time
/// \pre t is not nil
bool isFull( const BT * t )
{
 ADS_PRE( t != nullptr );
 if( ! t->left() && ! t->right() )
  return( true );
 if( ! t->left() || ! t->right() )
  return( false );
 return( isFull( t->left() ) && isFull( t->right() ) );
 }

}  // end( namespace tree )

// against the degree of every node
ADS_TEST( "tree-7" )
{
 CHECK( tree::isFull( cat( "1" ).get() ) );
 CHECK( tree::isFull( cat( "1( 2, 3( 4, 5 ) )" ).get() ) );
 CHECK( ! tree::isFull( cat( "1( 2, 3( 4, - ) )" ).get() ) );
 CHECK( ! tree::isFull( cat( "1( -, 2 )" ).get() ) );
 CHECK_THROWS( tree::isFull( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t( r % 2 ? randomInts( g , 1 + r % 11 , 0 , 0 )
              : PT( randomFull( g , r % 7 , 0 ) ) );
  const auto N = nodesOf( t.get() );
  CHECK( tree::isFull( t.get() ) ==
         std::all_of( N.begin() , N.end() , []( BT * u ) {
          return( degreeOf( u ) != 1 ); } ) );
  }
 CHECK( ! tree::isFull( cat( "1( 2, - )" ).get() ) );
 CHECK( ! tree::isFull( chainTree( DEEP , true , []( int ) {
  return( 0 ); } ).get() ) );
 PT c( completeTree( std::vector< int >( 7 , 0 ) ) );   // perfect
 CHECK( tree::isFull( c.get() ) );
 PT d( completeTree( std::vector< int >( 6 , 0 ) ) );   // one child at 3
 CHECK( ! tree::isFull( d.get() ) );
 }

/*----------------- tree-8, compito 2013-05-03, Ancestor -------------------*/

// lowestCommonAncestor() of the library, by the links to the parents,
// against the first ancestor of v among those of u, on every pair of nodes
ADS_TEST( "tree-8" )
{
 PT e = cat( "1( 2( 4, 5( 6, - ) ), 3 )" );
 BT * n4 = findValue( e.get() , 4 ) , * n6 = findValue( e.get() , 6 );
 CHECK( lowestCommonAncestor( n4 , n6 ) == e->left() );
 CHECK( lowestCommonAncestor( n6 , e->right() ) == e.get() );
 CHECK( lowestCommonAncestor( e->left() , n6 ) == e->left() );
 CHECK( lowestCommonAncestor( n6 , n6 ) == n6 );
 PT o = cat( "1" );
 CHECK_THROWS( lowestCommonAncestor( n4 , o.get() ) , contract_violation );
 CHECK( lowestCommonAncestor( o.get() , o.get() ) == o.get() );
 CHECK( lowestCommonAncestor( e.get() , n6 ) == e.get() );     // the root
 CHECK_THROWS( lowestCommonAncestor( n4 , static_cast< BT * >( nullptr ) ) ,
               contract_violation );
 CHECK_THROWS( lowestCommonAncestor( static_cast< BT * >( nullptr ) , n4 ) ,
               contract_violation );
 PT c = chainTree( DEEP , false , []( int d ) { return( d ); } );
 BT * bottom = c.get();
 while( bottom->left() )
  bottom = bottom->left();
 CHECK( lowestCommonAncestor( bottom , c.get() ) == c.get() );
 CHECK( lowestCommonAncestor( bottom , bottom->parent() ) ==
        bottom->parent() );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  PT t = randomInts( g , 1 + r % 12 , 0 , 0 );
  const auto N = nodesOf( t.get() );
  for( auto * u : N )
   for( auto * v : N ) {
    std::set< BT * > A;
    for( auto * x = u ; x ; x = x->parent() )
     A.insert( x );
    BT * a = v;
    while( ! A.count( a ) )
     a = a->parent();
    CHECK( lowestCommonAncestor( u , v ) == a );
    }
  }
 }

/*------------- tree-9, compito 2013-05-31, Un arco pesante ----------------*/

namespace tree {

/// the parents of a minimum spanning tree of the connected unoriented
/// graph g whose edges weigh all the same but one, which weighs one less:
/// that edge [ u , v ] is in every minimum spanning tree, and the tree of
/// a visit in breadth from both u and v (with v the child of u) is one,
/// O( n + m ) time
/// \pre g is unoriented and connected, with at least one edge
/// \post n - 1 parents
std::vector< int > lightEdgeTree( const ListGraph< int > & g )
{
 ADS_PRE( ! g.directed() );
 int u = NIL , v = NIL , w = 0;
 for( int x : g.V() )                       // the light edge
  g.forEachEdge( x , [ & ]( int y , int wy ) {
   if( ( u == NIL ) || ( wy < w ) ) {
    u = x;
    v = y;
    w = wy;
    }
   } );
 ADS_PRE( u != NIL );
 std::vector< int > p( g.bound() , NIL );
 std::vector< bool > seen( g.bound() , false );
 std::queue< int > Q;
 seen[ u ] = seen[ v ] = true;
 p[ v ] = u;
 Q.push( u );
 Q.push( v );
 while( ! Q.empty() ) {
  const int x = Q.front();
  Q.pop();
  g.forEachAdj( x , [ & ]( int y ) {
   if( ! seen[ y ] ) {
    seen[ y ] = true;
    p[ y ] = x;
    Q.push( y );
    }
   } );
  }
 ADS_PRE( std::count( seen.begin() , seen.end() , false ) == 0 );
 ADS_POST( std::count( p.begin() , p.end() , NIL ) == 1 );
 return( p );
 }

}  // end( namespace tree )

// a minimum spanning forest of the library's check, as heavy as Kruskal's;
// a negative light edge, a star, a graph not connected or oriented
ADS_TEST( "tree-9" )
{
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = 2 + r % 9 , w = 2 + int( g() % 8 );
  ListGraph< int > G( n , false );
  for( int i = 1 ; i < n ; ++i )            // a spanning tree
   G.insertEdge( int( g() % i ) , i , w );
  for( int k = int( g() % ( 2 * n ) ) ; k > 0 ; --k ) {
   const int a = int( g() % n ) , b = int( g() % n );
   if( a != b )
    G.insertEdge( a , b , w );
   }
  auto A = edges( G );
  auto & L = A[ g() % A.size() ];
  G.insertEdge( L.u , L.v , w - 1 );
  L.w = w - 1;
  const auto p = tree::lightEdgeTree( G );
  std::vector< WeightedEdge< int > > T;
  long long total = 0;
  for( int x = 0 ; x < n ; ++x )
   if( p[ x ] != NIL ) {
    T.push_back( { std::min( p[ x ] , x ) , std::max( p[ x ] , x ) ,
                   *G.weight( p[ x ] , x ) } );
    total += T.back().w;
    }
  CHECK( isMinimumSpanningForest( n , A , T ) );
  CHECK( ( p[ L.u ] == L.v ) || ( p[ L.v ] == L.u ) );
  long long k = 0;
  for( const auto & e : kruskal( G ) )
   k += e.w;
  CHECK( total == k );
  }
 ListGraph< int > G( 2 , false );
 G.insertEdge( 0 , 1 , 4 );
 CHECK( tree::lightEdgeTree( G ) == std::vector< int >( { NIL , 0 } ) );
 CHECK_THROWS( tree::lightEdgeTree( ListGraph< int >( 3 , false ) ) ,
               contract_violation );
 G.insertEdge( 0 , 1 , -7 );                // a negative light edge
 CHECK( tree::lightEdgeTree( G ) == std::vector< int >( { NIL , 0 } ) );
 ListGraph< int > S( 5 , false );           // a star, the light edge far
 for( int i = 1 ; i < 5 ; ++i )             // from the centre's first one
  S.insertEdge( 0 , i , 3 );
 S.insertEdge( 0 , 4 , 2 );
 const auto ps = tree::lightEdgeTree( S );
 CHECK( ( ps[ 4 ] == 0 ) || ( ps[ 0 ] == 4 ) );
 CHECK( std::count( ps.begin() , ps.end() , NIL ) == 1 );
 ListGraph< int > D( 4 , false );           // not connected
 D.insertEdge( 0 , 1 , 1 );
 D.insertEdge( 2 , 3 , 2 );
 CHECK_THROWS( tree::lightEdgeTree( D ) , contract_violation );
 ListGraph< int > O( 2 , true );            // oriented
 O.insertEdge( 0 , 1 , 1 );
 CHECK_THROWS( tree::lightEdgeTree( O ) , contract_violation );
 }

/*----- tree-10, compiti 2014-02-03 (Turni) e 2023-02-09 (Broadcast) -------*/

namespace tree {

/// turni(): the fewest rounds in which the root informs all the nodes of
/// t, each informed node telling one child per round; a postvisit, -1 for
/// nil: a node tells first the child that needs more rounds, and needs two
/// more rounds when its children need the same, Theta( n ) time
/// \post at least -1, and -1 only for nil
int turni( const BT * t )
{
 if( ! t )
  return( -1 );
 if( ! t->left() && ! t->right() )
  return( 0 );
 const int tl = turni( t->left() ) , tr = turni( t->right() );
 return( tl == tr ? tl + 2 : std::max( tl , tr ) + 1 );
 }

/// minRounds(): the same as turni(), with the three cases of the number of
/// children written out, Theta( n ) time
/// \pre t is not nil
/// \post at least 0
int minRounds( const BT * t )
{
 ADS_PRE( t != nullptr );
 if( ! t->left() && ! t->right() )
  return( 0 );
 if( ! t->left() || ! t->right() )
  return( 1 + minRounds( t->left() ? t->left() : t->right() ) );
 const int nl = minRounds( t->left() ) , nr = minRounds( t->right() );
 return( std::max( nl , nr ) + ( nl == nr ? 2 : 1 ) );
 }

}  // end( namespace tree )

namespace {

/// the round in which the last node of t is informed, t informed in the
/// round r, a node with two children telling first the right one if its
/// bit is set in mask
int broadcast( const BT * t , int r , unsigned mask ,
               std::map< const BT * , int > & bit )
{
 if( ! t->left() && ! t->right() )
  return( r );
 if( ! t->left() || ! t->right() )
  return( broadcast( t->left() ? t->left() : t->right() , r + 1 , mask ,
                     bit ) );
 const bool right = ( mask >> bit[ t ] ) & 1u;
 return( std::max( broadcast( t->left() , r + 1 + right , mask , bit ) ,
                   broadcast( t->right() , r + 2 - right , mask , bit ) ) );
 }

}  // end( namespace )

// the perfect tree of height 2 of the 2023 statement, and the best of all
// the orders in which the nodes tell their children
ADS_TEST( "tree-10" )
{
 PT p = cat( "1( 2( 4, 5 ), 3( 6, 7 ) )" );
 CHECK( tree::minRounds( p.get() ) == 4 );
 CHECK( tree::turni( p.get() ) == 4 );
 CHECK( tree::turni( nullptr ) == -1 );
 CHECK( tree::minRounds( cat( "1" ).get() ) == 0 );
 CHECK( tree::minRounds( cat( "1( 2( 3( 4, - ), - ), - )" ).get() ) == 3 );
 CHECK_THROWS( tree::minRounds( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 14 , 0 , 0 );
  std::map< const BT * , int > bit;
  for( auto * u : nodesOf( t.get() ) )
   if( degreeOf( u ) == 2 ) {
    const int b = int( bit.size() );
    bit[ u ] = b;
    }
  int best = IINF;
  for( unsigned mask = 0 ; mask < ( 1u << bit.size() ) ; ++mask )
   best = std::min( best , broadcast( t.get() , 0 , mask , bit ) );
  CHECK( tree::turni( t.get() ) == best );
  CHECK( tree::minRounds( t.get() ) == best );
  }
 CHECK( tree::turni( cat( "1" ).get() ) == 0 );
 CHECK( ( tree::turni( cat( "1( 2, 3 )" ).get() ) == 2 ) &&
        ( tree::minRounds( cat( "1( 2, 3 )" ).get() ) == 2 ) );
 CHECK( ( tree::turni( cat( "1( -, 2 )" ).get() ) == 1 ) &&
        ( tree::minRounds( cat( "1( 2, - )" ).get() ) == 1 ) );
 PT c = chainTree( DEEP , true , []( int ) { return( 0 ); } );
 CHECK( ( tree::turni( c.get() ) == DEEP - 1 ) &&
        ( tree::minRounds( c.get() ) == DEEP - 1 ) );
 PT q( completeTree( std::vector< int >( 63 , 0 ) ) );  // perfect, height 5
 CHECK( ( tree::turni( q.get() ) == 10 ) &&
        ( tree::minRounds( q.get() ) == 10 ) );
 }

/*------------ tree-11, compito 2014-04-24, Check balance ric --------------*/

namespace tree {

/// the nodes of t and whether t is balanced enough
std::pair< int , bool > checkBalanceRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0 , true ) );
 const auto [ nL , bL ] = checkBalanceRec( t->left() );
 const auto [ nR , bR ] = checkBalanceRec( t->right() );
 return( std::pair( nL + nR + 1 , bL && bR && ( nL <= 2 * nR + 1 ) &&
                                  ( nR <= 2 * nL + 1 ) ) );
 }

/// checkBalanceRic(): true if each node of t has at most 2 c + 1 nodes in
/// one subtree, with c those of the other; a postvisit that returns the
/// nodes of each subtree, Theta( n ) time
bool checkBalance( const BT * t )
{
 return( checkBalanceRec( t ).second );
 }

}  // end( namespace tree )

// against the sizes of the two subtrees counted for every node
ADS_TEST( "tree-11" )
{
 CHECK( tree::checkBalance( nullptr ) );
 CHECK( tree::checkBalance( cat( "1" ).get() ) );
 CHECK( tree::checkBalance( cat( "1( 2, - )" ).get() ) );
 CHECK( ! tree::checkBalance( cat( "1( 2( 3, - ), - )" ).get() ) );
 CHECK( tree::checkBalance( cat( "1( 2( 3, 4 ), 5 )" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , 0 , 0 );
  bool ok = true;
  for( auto * u : nodesOf( t.get() ) ) {
   const int a = sizeOf( u->left() ) , b = sizeOf( u->right() );
   ok = ok && ( a <= 2 * b + 1 ) && ( b <= 2 * a + 1 );
   }
  CHECK( tree::checkBalance( t.get() ) == ok );
  }
 CHECK( tree::checkBalance( cat( "1( -, 2 )" ).get() ) );
 CHECK( ! tree::checkBalance( chainTree( DEEP , false , []( int ) {
  return( 0 ); } ).get() ) );
 CHECK( tree::checkBalance( cat( "1( 2( 3, 4 ), 5( 6, - ) )" ).get() ) );
 CHECK( tree::checkBalance( cat( "1( 2( 3, - ), 4 )" ).get() ) ); // 2 = 2 1
 CHECK( ! tree::checkBalance( cat( "1( 2( 3, 4 ), - )" ).get() ) );
 PT c( completeTree( std::vector< int >( 1000 , 0 ) ) );
 CHECK( tree::checkBalance( c.get() ) );
 }

/*--------------- tree-12, compito 2014-06-16, Count trees -----------------*/

namespace tree {

/// true if the component of u, reached from parent, has no cycle; it is
/// visited all anyway
/// \pre visited has a place per node
template< class G >
bool acyclicDfs( const G & g , int u , int parent ,
                 std::vector< bool > & visited )
{
 visited[ u ] = true;
 bool acyclic = true;
 g.forEachAdj( u , [ & ]( int v ) {
  if( ! visited[ v ] ) {
   if( ! acyclicDfs( g , v , u , visited ) )
    acyclic = false;
   }
  else
   if( v != parent )                        // not the edge back up
    acyclic = false;
  } );
 return( acyclic );
 }

/// countTrees(): the connected components of the unoriented graph g that
/// are trees; a visit in depth of each component that finds a cycle when
/// it meets a visited node other than the parent, O( n + m ) time
/// \pre g is unoriented
/// \post at most n
int countTrees( const ListGraph<> & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< bool > visited( g.bound() , false );
 int count = 0;
 for( int u : g.V() )
  if( ! visited[ u ] )
   count += acyclicDfs( g , u , NIL , visited );
 ADS_POST( count <= g.n() );
 return( count );
 }

}  // end( namespace tree )

// against the components of the library, a tree when it has one edge less
// than its nodes; a cycle, a deleted node, a deep path, an oriented graph
ADS_TEST( "tree-12" )
{
 CHECK( tree::countTrees( ListGraph<>( 0 , false ) ) == 0 );
 CHECK( tree::countTrees( ListGraph<>( 3 , false ) ) == 3 );
 ListGraph<> T( 6 , false );
 for( auto [ a , b ] : { std::pair( 0 , 1 ) , std::pair( 1 , 2 ) ,
                         std::pair( 2 , 0 ) , std::pair( 3 , 4 ) } )
  T.insertEdge( a , b );
 CHECK( tree::countTrees( T ) == 2 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = 1 + r % 11;
  ListGraph<> G( n , false );
  for( int k = int( g() % ( n + 2 ) ) ; k > 0 ; --k ) {
   const int a = int( g() % n ) , b = int( g() % n );
   if( a != b )
    G.insertEdge( a , b );
   }
  const auto id = cc( G );
  std::vector< int > nodes( n + 1 , 0 ) , arcs( n + 1 , 0 );
  for( int u = 0 ; u < n ; ++u )
   ++nodes[ id[ u ] ];
  for( const auto & e : edges( G ) )
   ++arcs[ id[ e.u ] ];
  int count = 0;
  for( int c = 1 ; c <= n ; ++c )
   count += ( nodes[ c ] > 0 ) && ( arcs[ c ] == nodes[ c ] - 1 );
  CHECK( tree::countTrees( G ) == count );
  }
 ListGraph<> C( 4 , false );                // a cycle of all the nodes
 for( int i = 0 ; i < 4 ; ++i )
  C.insertEdge( i , ( i + 1 ) % 4 );
 CHECK( tree::countTrees( C ) == 0 );
 C.deleteNode( 2 );                         // a path, the node 2 deleted
 CHECK( tree::countTrees( C ) == 1 );
 const int L = 5000;
 ListGraph<> P( L , false );                // a deep path
 for( int i = 0 ; i + 1 < L ; ++i )
  P.insertEdge( i , i + 1 );
 CHECK( tree::countTrees( P ) == 1 );
 P.insertEdge( 0 , L - 1 );
 CHECK( tree::countTrees( P ) == 0 );
 CHECK_THROWS( tree::countTrees( ListGraph<>( 2 , true ) ) ,
               contract_violation );
 }

/*--------------- tree-13, compito 2014-09-01, Sottoalbero -----------------*/

namespace tree {

/// count(): writes in each node of t the size of its subtree, and returns
/// it
int countSizes( BT * t )
{
 if( ! t )
  return( 0 );
 const int n = 1 + countSizes( t->left() ) + countSizes( t->right() );
 t->write( n );
 return( n );
 }

/// the size written in t by count(), 0 for nil
int sizeField( const BT * t )
{
 return( t ? t->read() : 0 );
 }

/// true if P has the shape of a subtree of T, both counted
bool checkShape( const BT * T , const BT * P )
{
 if( ! T && ! P )
  return( true );
 if( sizeField( T ) < sizeField( P ) )
  return( false );
 if( sizeField( T ) == sizeField( P ) )
  return( checkShape( T->right() , P->right() ) &&
          checkShape( T->left() , P->left() ) );
 return( checkShape( T->right() , P ) || checkShape( T->left() , P ) );
 }

/// sottoalbero(): true if P has the shape of a subtree of T (a node of T
/// with all its descendants); each node holds the size of its subtree
/// (its value is overwritten), and only the subtrees of T as large as P
/// are compared with it, which visit disjoint parts of T, O( np + nt )
/// time
bool subtreeShape( BT * T , BT * P )
{
 countSizes( T );
 countSizes( P );
 return( checkShape( T , P ) );
 }

}  // end( namespace tree )

// against the shape of every subtree of T, P often a copy of one of them
ADS_TEST( "tree-13" )
{
 CHECK( tree::subtreeShape( cat( "1( 2( 3, 4 ), 5 )" ).get() ,
                            cat( "9( 9, 9 )" ).get() ) );
 CHECK( ! tree::subtreeShape( cat( "1( 2( 3, - ), 5 )" ).get() ,
                              cat( "9( -, 9 )" ).get() ) );
 CHECK( tree::subtreeShape( cat( "1" ).get() , cat( "1" ).get() ) );
 CHECK( tree::subtreeShape( nullptr , nullptr ) );
 CHECK( ! tree::subtreeShape( nullptr , cat( "1" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT T = randomInts( g , 1 + r % 14 , 0 , 0 );
  const auto N = nodesOf( T.get() );
  PT P( r % 2 ? copy( N[ g() % N.size() ] )
              : randomInts( g , 1 + r % 5 , 0 , 0 ).release() );
  if( ( r % 4 == 1 ) && ! P->left() )
   P->insertLeft( new BT( 0 ) );
  bool some = false;
  for( auto * u : N )
   some = some || ( shapeOf( u ) == shapeOf( P.get() ) );
  CHECK( tree::subtreeShape( T.get() , P.get() ) == some );
  }
 }

/*----- tree-14.1, compito 2014-10-31, Insert edge (weaklyConnected) -------*/

namespace tree {

/// undirected(): adds the edge ( v , u ) for each edge ( u , v ) of the
/// matrix, Theta( n^2 ) time
void undirected( MatrixGraph<> & g )
{
 for( int u : g.V() )
  for( int v : g.adj( u ) )
   g.insertEdge( v , u );
 }

/// weaklyConnected(): true if each two nodes of the oriented graph g have
/// a path between them when the orientation is forgotten; the matrix is
/// made symmetric and its components counted, Theta( n^2 ) time
/// \pre n >= 1
bool weaklyConnected( MatrixGraph<> g )
{
 ADS_PRE( g.n() >= 1 );
 undirected( g );
 const auto id = cc( g );
 for( int u : g.V() )
  if( id[ u ] != 1 )
   return( false );
 return( true );
 }

}  // end( namespace tree )

namespace {

/// a random oriented graph of n nodes, each edge with probability q
MatrixGraph<> randomDigraph( std::mt19937 & g , int n , double q )
{
 MatrixGraph<> G( n , true );
 std::bernoulli_distribution B( q );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( ( u != v ) && B( g ) )
    G.insertEdge( u , v );
 return( G );
 }

}  // end( namespace )

// against the components of an MFSet that merges the ends of every edge
ADS_TEST( "tree-14.1" )
{
 CHECK( tree::weaklyConnected( MatrixGraph<>( 1 , true ) ) );
 CHECK( ! tree::weaklyConnected( MatrixGraph<>( 2 , true ) ) );
 MatrixGraph<> P( 3 , true );
 P.insertEdge( 0 , 1 );
 P.insertEdge( 2 , 1 );
 CHECK( tree::weaklyConnected( P ) );
 CHECK( ! P.hasEdge( 1 , 0 ) );             // g is not modified
 CHECK_THROWS( tree::weaklyConnected( MatrixGraph<>( 0 , true ) ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = 1 + r % 10;
  const auto G = randomDigraph( g , n , 1.5 / n );
  MFSet M( n );
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) )
    M.merge( u , v );
  CHECK( tree::weaklyConnected( G ) == ( M.count() == 1 ) );
  }
 MatrixGraph<> S( 4 , true );               // all the edges into one node
 for( int i = 1 ; i < 4 ; ++i )
  S.insertEdge( i , 0 );
 CHECK( tree::weaklyConnected( S ) );
 S.deleteNode( 0 );                         // three isolated nodes left
 CHECK( ! tree::weaklyConnected( S ) );
 MatrixGraph<> Q( 3 , true );               // the node 0 deleted
 Q.insertEdge( 1 , 2 );
 Q.deleteNode( 0 );
 CHECK( tree::weaklyConnected( Q ) );
 }

/*--- tree-14.2, compito 2014-10-31, Insert edge (singularlyConnected) -----*/

namespace tree {

/// singularlyConnected(): true if each two distinct nodes of the oriented
/// graph g have exactly one simple path between them when the orientation
/// is forgotten, i.e., if the graph without orientation is a tree: the
/// matrix is made symmetric, and the graph must be weakly connected and
/// with no cycle, Theta( n^2 ) time
/// \pre n >= 1
bool singularlyConnected( MatrixGraph<> g )
{
 ADS_PRE( g.n() >= 1 );
 undirected( g );
 std::vector< bool > visited( g.bound() , false );
 return( weaklyConnected( g ) &&
         acyclicDfs( g , g.V().front() , NIL , visited ) );
 }

}  // end( namespace tree )

namespace {

/// the simple paths from u to t in the symmetric matrix g, on the nodes
/// not in the path P
int simplePaths( const MatrixGraph<> & g , int u , int t ,
                 std::vector< bool > & P )
{
 if( u == t )
  return( 1 );
 P[ u ] = true;
 int count = 0;
 for( int v : g.adj( u ) )
  if( ! P[ v ] )
   count += simplePaths( g , v , t , P );
 P[ u ] = false;
 return( count );
 }

}  // end( namespace )

// against the simple paths counted between every two nodes, and the
// unordered pairs of adjacent nodes, n - 1 for a tree; a graph whose node
// 0 is deleted, so that the visit starts from another one
ADS_TEST( "tree-14.2" )
{
 CHECK( tree::singularlyConnected( MatrixGraph<>( 1 , true ) ) );
 MatrixGraph<> P( 3 , true );
 P.insertEdge( 0 , 1 );
 P.insertEdge( 1 , 0 );
 P.insertEdge( 2 , 1 );
 CHECK( tree::singularlyConnected( P ) );
 P.insertEdge( 2 , 0 );
 CHECK( ! tree::singularlyConnected( P ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = 1 + r % 7;
  const auto G = randomDigraph( g , n , 1.2 / n );
  auto S = G;
  tree::undirected( S );
  bool one = true;
  std::vector< bool > Q( n , false );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    one = one && ( simplePaths( S , u , v , Q ) == 1 );
  CHECK( tree::singularlyConnected( G ) == one );
  int pairs = 0;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    pairs += S.hasEdge( u , v );
  CHECK( one == ( tree::weaklyConnected( G ) && ( pairs == n - 1 ) ) );
  }
 CHECK( ! tree::singularlyConnected( MatrixGraph<>( 2 , true ) ) );
 MatrixGraph<> T( 2 , true );               // both orientations: one path
 T.insertEdge( 0 , 1 );
 T.insertEdge( 1 , 0 );
 CHECK( tree::singularlyConnected( T ) );
 MatrixGraph<> Q( 4 , true );               // the node 0 deleted, a path
 Q.insertEdge( 1 , 2 );                     // of the three left
 Q.insertEdge( 3 , 2 );
 Q.deleteNode( 0 );
 CHECK( tree::singularlyConnected( Q ) );
 Q.insertEdge( 1 , 3 );
 CHECK( ! tree::singularlyConnected( Q ) );
 CHECK_THROWS( tree::singularlyConnected( MatrixGraph<>( 0 , true ) ) ,
               contract_violation );
 }

/*----------------- tree-15, compito 2015-01-12, Balance -------------------*/

namespace tree {

/// balance(): -1 if t is not k-balanced (the heights of the two subtrees
/// of some node differ by more than k), its height otherwise, 0 for nil;
/// a postvisit, O( n ) time
/// \pre k >= 0
/// \post at least -1
int balance( const BT * t , int k )
{
 ADS_PRE( k >= 0 );
 if( ! t )
  return( 0 );
 const int L = balance( t->left() , k ) , R = balance( t->right() , k );
 if( ( L < 0 ) || ( R < 0 ) || ( std::abs( L - R ) > k ) )
  return( -1 );
 return( std::max( L , R ) + 1 );
 }

}  // end( namespace tree )

// against the heights of the two subtrees computed for every node
ADS_TEST( "tree-15" )
{
 CHECK( tree::balance( nullptr , 0 ) == 0 );
 CHECK( tree::balance( cat( "1" ).get() , 0 ) == 1 );
 CHECK( tree::balance( cat( "1( 2( 3, - ), - )" ).get() , 1 ) == -1 );
 CHECK( tree::balance( cat( "1( 2( 3, - ), - )" ).get() , 2 ) == 3 );
 CHECK_THROWS( tree::balance( nullptr , -1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , 0 , 0 );
  const int k = r % 4;
  bool ok = true;
  for( auto * u : nodesOf( t.get() ) )
   ok = ok && ( std::abs( heightOf( u->left() ) -
                          heightOf( u->right() ) ) <= k );
  const int b = tree::balance( t.get() , k );
  CHECK( ( b >= 0 ) == ok );
  CHECK( ! ok || ( b == heightOf( t.get() ) + 1 ) );
  }
 CHECK( tree::balance( cat( "1( 2, - )" ).get() , 0 ) == -1 );
 CHECK( tree::balance( cat( "1( 2, - )" ).get() , 1 ) == 2 );
 CHECK( tree::balance( cat( "1( 2, 3 )" ).get() , 0 ) == 2 );
 PT c = chainTree( DEEP , true , []( int ) { return( 0 ); } );
 CHECK( tree::balance( c.get() , DEEP ) == DEEP );
 CHECK( tree::balance( c.get() , DEEP - 1 ) == DEEP );
 CHECK( tree::balance( c.get() , DEEP - 2 ) == -1 );
 CHECK( tree::balance( c.get() , std::numeric_limits< int >::max() ) ==
        DEEP );
 PT q( completeTree( std::vector< int >( 31 , 0 ) ) );
 CHECK( tree::balance( q.get() , 0 ) == 5 );
 CHECK_THROWS( tree::balance( cat( "1" ).get() , -1 ) , contract_violation );
 }

/*------------- tree-16, compito 2015-11-05, Compute average ---------------*/

namespace tree {

/// the smallest and the largest key of t, +inf and -inf for nil
std::pair< long long , long long > computeDiffRec( const BT * t )
{
 if( ! t )
  return( std::pair( INFTY , -INFTY ) );
 const auto [ minL , maxL ] = computeDiffRec( t->left() );
 const auto [ minR , maxR ] = computeDiffRec( t->right() );
 const long long v = t->read();
 return( std::pair( std::min( { minL , minR , v } ) ,
                    std::max( { maxL , maxR , v } ) ) );
 }

/// computeDiff(): the largest minus the smallest key of the subtree of t;
/// a postvisit returning both, O( n ) time
/// \pre t is not nil
/// \post at least 0
long long computeDiff( const BT * t )
{
 ADS_PRE( t != nullptr );
 const auto [ min , max ] = computeDiffRec( t );
 return( max - min );
 }

/// the sum and the number of the keys of t
std::pair< long long , int > computeAverageRec( const BT * t )
{
 if( ! t )
  return( std::pair( 0LL , 0 ) );
 const auto [ sL , nL ] = computeAverageRec( t->left() );
 const auto [ sR , nR ] = computeAverageRec( t->right() );
 return( std::pair( sL + sR + t->read() , nL + nR + 1 ) );
 }

/// computeAverage(): the average of the keys of the subtree of t (the
/// second version of the exam); a postvisit returning their sum and their
/// number, O( n ) time
/// \pre t is not nil
double computeAverage( const BT * t )
{
 ADS_PRE( t != nullptr );
 const auto [ s , n ] = computeAverageRec( t );
 return( double( s ) / n );
 }

}  // end( namespace tree )

// against std::minmax_element and std::accumulate on the keys of the
// subtree of every node
ADS_TEST( "tree-16" )
{
 PT e = cat( "2( 6( 10, 12 ), 8( -, 4 ) )" );
 CHECK( tree::computeDiff( e.get() ) == 10 );
 CHECK( tree::computeAverage( e.get() ) == 7 );
 CHECK( tree::computeDiff( e->right() ) == 4 );
 CHECK( tree::computeDiff( cat( "5" ).get() ) == 0 );
 CHECK_THROWS( tree::computeDiff( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  PT t = randomInts( g , 1 + r % 12 , -1000000000 , 1000000000 );
  for( auto * u : nodesOf( t.get() ) ) {
   std::vector< long long > K;
   for( auto * x : nodesOf( u ) )
    K.push_back( x->read() );
   const auto [ a , b ] = std::minmax_element( K.begin() , K.end() );
   CHECK( tree::computeDiff( u ) == *b - *a );
   CHECK( std::abs( tree::computeAverage( u ) -
                    double( std::accumulate( K.begin() , K.end() , 0LL ) ) /
                    K.size() ) < 1e-6 );
   }
  }
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 PT x = chainTree( 2 , false , [ & ]( int d ) { return( d ? lo : hi ); } );
 CHECK( tree::computeDiff( x.get() ) == 1LL * hi - lo );
 CHECK( tree::computeAverage( x.get() ) == -0.5 );
 CHECK( tree::computeAverage( cat( "-3" ).get() ) == -3 );
 CHECK( tree::computeDiff( cat( "4( 4, 4 )" ).get() ) == 0 );
 PT c = chainTree( DEEP , true , [ & ]( int ) { return( hi ); } );
 CHECK( tree::computeDiff( c.get() ) == 0 );
 CHECK( tree::computeAverage( c.get() ) == hi );     // no overflow of the sum
 CHECK_THROWS( tree::computeAverage( nullptr ) , contract_violation );
 }

/*--------------- tree-17, compito 2016-01-07, Dfs search ------------------*/

namespace tree {

/// the shortest distance from x to t in the looped tree g, never entering
/// r, +inf if there is none
long long dfsSearch( const ListGraph< int > & g , int x , int t , int r )
{
 if( x == t )
  return( 0 );
 long long min = INFTY;
 g.forEachEdge( x , [ & ]( int y , int w ) {
  if( y != r ) {
   const long long d = dfsSearch( g , y , t , r );
   if( ( d < INFTY ) && ( d + w < min ) )
    min = d + w;
   }
  } );
 return( min );
 }

/// computeDist(): the shortest distance from u to v in the looped tree g
/// of root r (a binary tree with an edge from each node to its children
/// and from each leaf to r, no cycle of negative weight); if v is below u
/// its path never enters r, otherwise it goes from u to r through a leaf
/// and then down to v; m < 2 n, O( n ) time; the text adds the weight of
/// an edge also to an infinite distance, which here stays infinite
/// \pre r , u , v are nodes, n >= 2
long long computeDist( const ListGraph< int > & g , int r , int u , int v )
{
 ADS_PRE( g.hasNode( r ) && g.hasNode( u ) && g.hasNode( v ) );
 ADS_PRE( g.n() >= 2 );
 const long long d = dfsSearch( g , u , v , r );
 if( d < INFTY )
  return( d );
 return( dfsSearch( g , u , r , NIL ) + dfsSearch( g , r , v , r ) );
 }

}  // end( namespace tree )

// against Bellman-Ford-Moore of the library from every node
ADS_TEST( "tree-17" )
{
 std::mt19937 g( 1 );
 std::uniform_int_distribution< int > W( -5 , 5 );
 for( int r = 0 ; r < 300 ; ++r ) {
  PT t = randomInts( g , 2 + r % 12 , 0 , 0 );
  const auto N = nodesOf( t.get() );
  const int n = int( N.size() );
  std::map< BT * , int > id;
  for( int i = 0 ; i < n ; ++i )
   id[ N[ i ] ] = i;
  ListGraph< int > G( n , true );
  std::vector< long long > down( n , 0 );   // from the root
  for( int i = 1 ; i < n ; ++i ) {          // a parent before its children
   const int w = W( g ) , p = id[ N[ i ]->parent() ];
   G.insertEdge( p , i , w );
   down[ i ] = down[ p ] + w;
   }
  for( int i = 1 ; i < n ; ++i )            // no negative cycle
   if( isLeafNode( N[ i ] ) )
    G.insertEdge( i , 0 , int( std::max( W( g ) * 1LL , -down[ i ] ) ) );
  for( int u = 0 ; u < n ; ++u ) {
   const auto S = bellmanFordMoore( G , u );
   CHECK( ! S.negativeCycle );
   for( int v = 0 ; v < n ; ++v )
    CHECK( tree::computeDist( G , 0 , u , v ) == S.d[ v ] );
   }
  }
 ListGraph< int > C( 2 , true );
 C.insertEdge( 0 , 1 , -3 );
 C.insertEdge( 1 , 0 , 3 );
 CHECK( tree::computeDist( C , 0 , 1 , 1 ) == 0 );
 CHECK( tree::computeDist( C , 0 , 1 , 0 ) == 3 );
 CHECK( tree::computeDist( C , 0 , 0 , 1 ) == -3 );
 CHECK_THROWS( tree::computeDist( ListGraph< int >( 1 , true ) , 0 , 0 ,
                                  0 ) , contract_violation );
 CHECK( tree::computeDist( C , 0 , 0 , 0 ) == 0 );     // u = v = r
 CHECK_THROWS( tree::computeDist( C , 0 , 0 , 2 ) , contract_violation );
 CHECK_THROWS( tree::computeDist( C , 0 , -1 , 1 ) , contract_violation );
 CHECK_THROWS( tree::computeDist( C , 5 , 0 , 1 ) , contract_violation );
 ListGraph< int > P( 4 , true );            // a chain 0 , 1 , 2 , 3 , back
 for( int i = 0 ; i < 3 ; ++i )             // to the root from the leaf
  P.insertEdge( i , i + 1 , 1 );
 P.insertEdge( 3 , 0 , 0 );
 CHECK( tree::computeDist( P , 0 , 3 , 1 ) == 1 );     // up and down
 CHECK( tree::computeDist( P , 0 , 1 , 3 ) == 2 );     // only down
 CHECK( tree::computeDist( P , 0 , 2 , 1 ) == 2 );
 }

/*------------ tree-18, compito 2016-06-06, Silicon Valley 1 ---------------*/

namespace tree {

/// an employee
struct Employee {
 int salary , productivity;
 };

/// the profit of the subtree of u and the least profit of a division in
/// it, +inf if there is none
std::pair< long long , long long > minProfitRec( const Tree< Employee > * u )
{
 long long profit = u->read().productivity - u->read().salary , min = INFTY;
 for( auto * f = u->leftmostChild() ; f ; f = f->rightSibling() ) {
  const auto [ tot , m ] = minProfitRec( f );
  profit += tot;
  min = std::min( min , m );
  }
 if( u->leftmostChild() )                   // a manager
  min = std::min( min , profit );
 return( std::pair( profit , min ) );
 }

/// minProfit(): the least profit (productivity minus salary of its
/// employees) of a division, a manager with all the employees below, +inf
/// if there is none; a postvisit returning the profit of each subtree and
/// the least one inside, O( n ) time; the text takes the minimum over the
/// leaves too, which are not divisions
/// \pre u is not nil
long long minProfit( const Tree< Employee > * u )
{
 ADS_PRE( u != nullptr );
 return( minProfitRec( u ).second );
 }

}  // end( namespace tree )

// against the profit of the subtree of every manager, summed apart
ADS_TEST( "tree-18" )
{
 using E = tree::Employee;
 using TE = Tree< E >;
 std::unique_ptr< TE > s( new TE( E{ 1 , 10 } ) );
 s->insertChild( new TE( E{ 5 , 0 } ) );    // a leaf of profit -5
 CHECK( tree::minProfit( s.get() ) == 4 );
 CHECK( tree::minProfit( std::unique_ptr< TE >( new TE( E{ 1 , 1 } ) ).get() )
        == INFTY );
 CHECK_THROWS( tree::minProfit( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 std::uniform_int_distribution< int > V( 0 , 20 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = 1 + r % 14;
  std::vector< TE * > N( 1 , new TE( E{ V( g ) , V( g ) } ) );
  std::unique_ptr< TE > t( N[ 0 ] );
  for( int i = 1 ; i < n ; ++i ) {
   N.push_back( new TE( E{ V( g ) , V( g ) } ) );
   N[ g() % i ]->insertChild( N.back() );
   }
  long long best = INFTY;
  for( auto * u : N )
   if( ! u->isLeaf() ) {
    long long p = 0;
    pre_visit( u , [ & ]( TE * x ) {
     p += x->read().productivity - x->read().salary; } );
    best = std::min( best , p );
    }
  CHECK( tree::minProfit( t.get() ) == best );
  }
 std::unique_ptr< TE > c( new TE( E{ 0 , 0 } ) );   // a chain of managers
 TE * u = c.get();
 for( int d = 1 ; d < 1000 ; ++d ) {
  u->insertChild( new TE( E{ d % 2 ? 3 : 0 , d % 2 ? 0 : 1 } ) );
  u = u->leftmostChild();
  }
 long long best = INFTY , below = 0;        // the profits from the bottom
 for( TE * x = u ; x ; x = x->parent() ) {
  below += x->read().productivity - x->read().salary;
  if( x != u )
   best = std::min( best , below );
  }
 CHECK( tree::minProfit( c.get() ) == best );
 std::unique_ptr< TE > f( new TE( E{ 0 , 0 } ) );   // a flat team
 for( int i = 0 ; i < 50 ; ++i )
  f->insertChild( new TE( E{ 1 , 0 } ) );
 CHECK( tree::minProfit( f.get() ) == -50 );
 }

/*----------- tree-19, compito 2016-11-03, Longest decreasing --------------*/

namespace tree {

/// longestDecreasing(): the edges of the longest path of the acyclic
/// oriented graph g along which the weights p strictly decrease; a visit
/// in depth whose vector longest[ u ], the edges of the longest such path
/// from u, also marks the visited nodes, O( n + m ) time; the code of the
/// text is that of the increasing paths, here p[ u ] > p[ v ]
/// \pre g has no cycle, p has a place per node
/// \post at least 0
int longestDecreasing( const ListGraph<> & g , const std::vector< int > & p )
{
 ADS_PRE( ! hasCycle( g ) && ( int( p.size() ) >= g.bound() ) );
 std::vector< int > longest( g.bound() , -1 );
 auto rec = [ & ]( auto & self , int u ) -> void {
  longest[ u ] = 0;
  g.forEachAdj( u , [ & ]( int v ) {
   if( longest[ v ] < 0 )
    self( self , v );
   if( ( p[ u ] > p[ v ] ) && ( longest[ u ] < longest[ v ] + 1 ) )
    longest[ u ] = longest[ v ] + 1;
   } );
  };
 int max = 0;
 for( int u : g.V() ) {
  if( longest[ u ] < 0 )
   rec( rec , u );
  max = std::max( max , longest[ u ] );
  }
 return( max );
 }

}  // end( namespace tree )

// against the longest paths by a topological order of the library
ADS_TEST( "tree-19" )
{
 ListGraph<> E( 4 , true );
 E.insertEdge( 0 , 1 );
 E.insertEdge( 1 , 2 );
 E.insertEdge( 2 , 3 );
 CHECK( tree::longestDecreasing( E , { 9 , 5 , 7 , 1 } ) == 1 );
 CHECK( tree::longestDecreasing( E , { 9 , 7 , 5 , 1 } ) == 3 );
 CHECK( tree::longestDecreasing( E , { 1 , 1 , 1 , 1 } ) == 0 );
 CHECK( tree::longestDecreasing( ListGraph<>( 0 , true ) , {} ) == 0 );
 E.insertEdge( 3 , 0 );
 CHECK_THROWS( tree::longestDecreasing( E , { 9 , 7 , 5 , 1 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = 1 + r % 12;
  std::vector< int > name( n );
  std::iota( name.begin() , name.end() , 0 );
  std::shuffle( name.begin() , name.end() , g );
  ListGraph<> G( n , true );
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 1 ; j < n ; ++j )
    if( g() % 3 == 0 )
     G.insertEdge( name[ i ] , name[ j ] );
  std::vector< int > p( n );
  for( auto & x : p )
   x = int( g() % 6 ) - 3;
  const auto S = topsort( G );
  std::vector< int > L( n , 0 );
  for( auto it = S.rbegin() ; it != S.rend() ; ++it )
   G.forEachAdj( *it , [ & ]( int v ) {
    if( p[ *it ] > p[ v ] )
     L[ *it ] = std::max( L[ *it ] , L[ v ] + 1 );
    } );
  CHECK( tree::longestDecreasing( G , p ) ==
         *std::max_element( L.begin() , L.end() ) );
  }
 CHECK( tree::longestDecreasing( ListGraph<>( 5 , true ) ,
                                 { 5 , 4 , 3 , 2 , 1 } ) == 0 );  // no edge
 ListGraph<> P( 5000 , true );              // a deep path, decreasing
 std::vector< int > w( 5000 );
 for( int i = 0 ; i < 5000 ; ++i ) {
  w[ i ] = -i;
  if( i + 1 < 5000 )
   P.insertEdge( i , i + 1 );
  }
 CHECK( tree::longestDecreasing( P , w ) == 4999 );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 ListGraph<> T( 2 , true );
 T.insertEdge( 1 , 0 );
 CHECK( tree::longestDecreasing( T , { lo , hi } ) == 1 );
 CHECK( tree::longestDecreasing( T , { hi , lo } ) == 0 );
 CHECK_THROWS( tree::longestDecreasing( T , { 1 } ) , contract_violation );
 }

/*---------------- tree-20, compito 2016-11-03, Min count ------------------*/

namespace tree {

/// the least cost of a leaf of t and the leaves of that cost, soFar the
/// product of the factors above t
std::pair< double , int > minCountRec( const BinaryTree< double > * t ,
                                       double soFar )
{
 if( ! t )
  return( std::pair( std::numeric_limits< double >::infinity() , 1 ) );
 soFar *= t->read();
 if( ! t->left() && ! t->right() )
  return( std::pair( soFar , 1 ) );
 const auto [ cL , nL ] = minCountRec( t->left() , soFar );
 const auto [ cR , nR ] = minCountRec( t->right() , soFar );
 if( cL == cR )
  return( std::pair( cL , nL + nR ) );
 return( cL < cR ? std::pair( cL , nL ) : std::pair( cR , nR ) );
 }

/// minCount(): the leaves of least cost, the cost of a leaf the product of
/// the factors of the nodes from the root to it; a visit that carries the
/// product down and returns the least cost and its leaves, O( n ) time;
/// the text sums the factors instead of multiplying them, and returns the
/// least cost instead of the number of leaves
/// \pre t is not nil, the factors are positive
/// \post at least 1
int minCount( const BinaryTree< double > * t )
{
 ADS_PRE( t != nullptr );
 return( minCountRec( t , 1 ).second );
 }

}  // end( namespace tree )

// against the cost of every leaf, multiplied up to the root; the factors
// are powers of 2, so that the products are exact
ADS_TEST( "tree-20" )
{
 CHECK( tree::minCount( cat< double >( "2( 0.5, 4 )" ).get() ) == 1 );
 CHECK( tree::minCount( cat< double >( "2( 0.5( 4, - ), 2 )" ).get() ) == 2 );
 CHECK( tree::minCount( cat< double >( "3" ).get() ) == 1 );
 CHECK( tree::minCount( cat< double >( "1( 1( 1, 1 ), 1 )" ).get() ) == 3 );
 CHECK_THROWS( tree::minCount( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 const double F[] = { 0.5 , 1 , 2 , 4 };
 for( int r = 0 ; r < 600 ; ++r ) {
  std::unique_ptr< BinaryTree< double > > t( randomTree< double >( g ,
   1 + r % 14 , [ & ]( void ) { return( F[ g() % 4 ] ); } ) );
  std::vector< double > C;
  for( auto * l : leavesOf( t.get() ) ) {
   double c = 1;
   for( auto * u = l ; u ; u = u->parent() )
    c *= u->read();
   C.push_back( c );
   }
  const double m = *std::min_element( C.begin() , C.end() );
  CHECK( tree::minCount( t.get() ) == std::count( C.begin() , C.end() , m ) );
  }
 CHECK( tree::minCount( cat< double >( "2( 4, - )" ).get() ) == 1 );
 CHECK( tree::minCount( cat< double >( "1( 2( 0.5, - ), 1 )" ).get() ) == 2 );
 std::unique_ptr< BinaryTree< double > > q( completeTree(
  std::vector< double >( 127 , 1 ) ) );    // 64 leaves, all of cost 1
 CHECK( tree::minCount( q.get() ) == 64 );
 std::unique_ptr< BinaryTree< double > > c( new BinaryTree< double >( 1 ) );
 auto * u = c.get();                        // a deep chain
 for( int d = 1 ; d < DEEP ; ++d ) {
  u->insertRight( new BinaryTree< double >( 1 ) );
  u = u->right();
  }
 CHECK( tree::minCount( c.get() ) == 1 );
 }

/*--------------- tree-21, compito 2017-02-07, Count tree ------------------*/

namespace tree {

/// the size of t and its nodes with as many ancestors as descendants, a
/// the ancestors of t
std::pair< int , int > countTreeRec( const BT * t , int a )
{
 if( ! t )
  return( std::pair( 0 , 0 ) );
 const auto [ sL , cL ] = countTreeRec( t->left() , a + 1 );
 const auto [ sR , cR ] = countTreeRec( t->right() , a + 1 );
 return( std::pair( sL + sR + 1 , cL + cR + ( a == sL + sR ? 1 : 0 ) ) );
 }

/// countTree(): the nodes with as many ancestors as descendants; a visit
/// that carries the ancestors down and returns the sizes up, O( n ) time
/// \post at least 0
int countTree( const BT * t )
{
 return( countTreeRec( t , 0 ).second );
 }

}  // end( namespace tree )

// against the depth and the size of every node computed apart
ADS_TEST( "tree-21" )
{
 CHECK( tree::countTree( nullptr ) == 0 );
 CHECK( tree::countTree( cat( "1" ).get() ) == 1 );
 CHECK( tree::countTree( cat( "1( 2, 3 )" ).get() ) == 0 );
 CHECK( tree::countTree( cat( "1( 2( 3, 4 ), 5 )" ).get() ) == 0 );
 CHECK( tree::countTree( cat( "1( 2( 3, - ), - )" ).get() ) == 1 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , r % 16 , 0 , 0 );
  int count = 0;
  for( auto * u : nodesOf( t.get() ) )
   count += ( depthOf( u ) == sizeOf( u ) - 1 );
  CHECK( tree::countTree( t.get() ) == count );
  }
 CHECK( tree::countTree( cat( "1( 2, - )" ).get() ) == 0 );
 CHECK( tree::countTree( chainTree( 2001 , false , []( int ) {
  return( 0 ); } ).get() ) == 1 );          // the middle node, depth 1000
 CHECK( tree::countTree( chainTree( 2000 , true , []( int ) {
  return( 0 ); } ).get() ) == 0 );          // no middle node
 PT q( completeTree( std::vector< int >( 15 , 0 ) ) );
 CHECK( tree::countTree( q.get() ) == 4 );  // the level 2: 2 and 2
 }

/*---------------- tree-22, compito 2018-01-08, Nicetree -------------------*/

namespace tree {

/// the place after the subtree whose preorder starts at pos, and its height
std::pair< std::size_t , int > nicetreeRec( std::string_view S ,
                                            std::size_t pos )
{
 ADS_PRE( pos < S.size() );
 if( S[ pos ] == 'L' )
  return( std::pair( pos + 1 , 0 ) );
 const auto [ p , hL ] = nicetreeRec( S , pos + 1 );
 const auto [ q , hR ] = nicetreeRec( S , p );
 return( std::pair( q , std::max( hL , hR ) + 1 ) );
 }

/// nicetree(): the height of the full binary tree whose preorder S marks
/// the internal nodes I and the leaves L; each character read once, by a
/// recursion that returns where its subtree ends, O( n ) time
/// \pre S is the preorder of a full binary tree
/// \post at least 0
int nicetree( std::string_view S )
{
 const auto [ p , h ] = nicetreeRec( S , 0 );
 ADS_PRE( p == S.size() );
 return( h );
 }

}  // end( namespace tree )

// against the heights of random full trees whose preorder is written
ADS_TEST( "tree-22" )
{
 CHECK( tree::nicetree( "ILIILLL" ) == 3 );
 CHECK( tree::nicetree( "L" ) == 0 );
 CHECK( tree::nicetree( "ILL" ) == 1 );
 CHECK_THROWS( tree::nicetree( "IL" ) , contract_violation );
 CHECK_THROWS( tree::nicetree( "LL" ) , contract_violation );
 CHECK_THROWS( tree::nicetree( "" ) , contract_violation );
 CHECK_THROWS( tree::nicetree( "I" ) , contract_violation );
 CHECK_THROWS( tree::nicetree( "ILLL" ) , contract_violation );
 std::string left( 5000 , 'I' ) , right;    // the deep chains of internal
 left += std::string( 5001 , 'L' );         // nodes to the left and to the
 for( int k = 0 ; k < 5000 ; ++k )          // right
  right += "IL";
 right += 'L';
 CHECK( tree::nicetree( left ) == 5000 );
 CHECK( tree::nicetree( right ) == 5000 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  std::unique_ptr< BinaryTree< char > > t( randomFull( g , r % 20 , 'x' ) );
  std::string S;
  pre_visit( t.get() , [ & ]( BinaryTree< char > * u ) {
   S += isLeafNode( u ) ? 'L' : 'I'; } );
  CHECK( tree::nicetree( S ) == heightOf( t.get() ) );
  }
 }

/*-------------- tree-23, compito 2018-01-31, Search forest ----------------*/

namespace tree {

/// the cells of the forest of the cell ( i , j ), which are cleared
int forestDfs( std::vector< std::vector< int > > & A , int i , int j )
{
 const int n = int( A.size() );
 if( ( i < 0 ) || ( j < 0 ) || ( i >= n ) || ( j >= n ) ||
     ( A[ i ][ j ] != 1 ) )
  return( 0 );
 A[ i ][ j ] = 0;
 return( 1 + forestDfs( A , i - 1 , j ) + forestDfs( A , i + 1 , j ) +
         forestDfs( A , i , j - 1 ) + forestDfs( A , i , j + 1 ) );
 }

/// searchForest(): the cells of the largest forest, a maximal set of cells
/// 1 joined by their sides, 0 if there is none; a visit in depth from each
/// cell not yet visited, on a copy of the matrix, Theta( n^2 ) time
/// \pre A is square with entries 0 and 1
/// \post at most n^2
int searchForest( std::vector< std::vector< int > > A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( const auto & R ) {
  return( ( R.size() == A.size() ) &&
          std::all_of( R.begin() , R.end() , []( int a ) {
           return( ( a == 0 ) || ( a == 1 ) ); } ) ); } ) );
 int max = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = 0 ; j < A.size() ; ++j )
   max = std::max( max , forestDfs( A , int( i ) , int( j ) ) );
 return( max );
 }

}  // end( namespace tree )

// against the largest set of an MFSet that merges the adjacent cells 1;
// the matrices not square (checked before any visit, which would read
// beyond a short row), the diagonals, a long winding forest
ADS_TEST( "tree-23" )
{
 CHECK( tree::searchForest( { { 0 , 0 , 1 , 1 , 1 } , { 0 , 0 , 0 , 0 , 1 } ,
                              { 1 , 0 , 1 , 0 , 1 } , { 1 , 1 , 1 , 0 , 0 } ,
                              { 1 , 1 , 0 , 1 , 1 } } ) == 7 );
 CHECK( tree::searchForest( {} ) == 0 );
 CHECK( tree::searchForest( { { 1 } } ) == 1 );
 CHECK( tree::searchForest( { { 0 , 0 } , { 0 , 0 } } ) == 0 );
 CHECK( tree::searchForest( std::vector< std::vector< int > >(
                             30 , std::vector< int >( 30 , 1 ) ) ) == 900 );
 CHECK_THROWS( tree::searchForest( { { 1 , 2 } , { 0 , 0 } } ) ,
               contract_violation );
 CHECK_THROWS( tree::searchForest( { { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( tree::searchForest( { { 1 , 1 } } ) , contract_violation );
 CHECK( tree::searchForest( { { 0 } } ) == 0 );
 CHECK( tree::searchForest( { { 1 , 0 , 1 } , { 0 , 1 , 0 } ,
                              { 1 , 0 , 1 } } ) == 1 );    // diagonals only
 std::vector< std::vector< int > > snake( 70 , std::vector< int >( 70 , 0 ) );
 for( int i = 0 ; i < 70 ; i += 2 ) {       // a winding path of 1s
  for( int j = 0 ; j < 70 ; ++j )
   snake[ i ][ j ] = 1;
  if( i + 1 < 70 )
   snake[ i + 1 ][ ( i / 2 ) % 2 ? 0 : 69 ] = 1;
  }
 CHECK( tree::searchForest( snake ) == 35 * 70 + 35 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = 1 + r % 9;
  std::vector< std::vector< int > > A( n , std::vector< int >( n ) );
  for( auto & R : A )
   for( auto & a : R )
    a = ( g() % 5 < 3 );
  MFSet M( n * n );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( A[ i ][ j ] ) {
     if( ( i + 1 < n ) && A[ i + 1 ][ j ] )
      M.merge( i * n + j , ( i + 1 ) * n + j );
     if( ( j + 1 < n ) && A[ i ][ j + 1 ] )
      M.merge( i * n + j , i * n + j + 1 );
     }
  std::map< int , int > size;
  int max = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( A[ i ][ j ] )
     max = std::max( max , ++size[ M.find( i * n + j ) ] );
  CHECK( tree::searchForest( A ) == max );
  }
 }

/*--------------- tree-24.a, compito 2018-06-28, K minimal -----------------*/

namespace tree {

/// kMinimal(): the thinness (heaviest minus lightest edge) of a minimum
/// spanning tree of the n nodes on the edges A of weight at least k, +inf
/// if there is none; Kruskal on the edges of weight at least k, which
/// keeps their least and largest weight, O( m log n ) time (all the
/// minimum spanning trees have the same weights, so the thinness is that
/// of each of them)
/// \pre n >= 2, the ends of the edges in [ 0 , n )
/// \post at least 0
long long kMinimal( int n , std::vector< WeightedEdge< int > > A , int k )
{
 ADS_PRE( n >= 2 );
 std::sort( A.begin() , A.end() , []( const auto & a , const auto & b ) {
  return( a.w < b.w ); } );
 MFSet M( n );
 long long min = INFTY , max = -INFTY;
 int count = 0;
 for( const auto & e : A ) {
  ADS_PRE( ( e.u >= 0 ) && ( e.u < n ) && ( e.v >= 0 ) && ( e.v < n ) );
  if( ( e.w >= k ) && ( count < n - 1 ) && M.merge( e.u , e.v ) ) {
   min = std::min( min , 1LL * e.w );
   max = std::max( max , 1LL * e.w );
   ++count;
   }
  }
 return( count == n - 1 ? max - min : INFTY );
 }

}  // end( namespace tree )

namespace {

/// the weight and the thinness of the n - 1 edges of A in s, a weight of
/// +inf if they are not a spanning tree of edges of weight at least k
std::pair< long long , long long > treeOf( int n ,
                                           const std::vector<
                                            WeightedEdge< int > > & A ,
                                           int k , unsigned s )
{
 MFSet M( n );
 bool tree = std::popcount( s ) == n - 1;
 long long w = 0 , lo = INFTY , hi = -INFTY;
 for( int i = 0 ; tree && ( i < int( A.size() ) ) ; ++i )
  if( ( s >> i ) & 1u ) {
   tree = ( A[ i ].w >= k ) && M.merge( A[ i ].u , A[ i ].v );
   w += A[ i ].w;
   lo = std::min( lo , 1LL * A[ i ].w );
   hi = std::max( hi , 1LL * A[ i ].w );
   }
 return( tree ? std::pair( w , hi - lo ) : std::pair( INFTY , INFTY ) );
 }

/// the thinness of the k-minimal tree by enumerating the sets of n - 1
/// edges of A, +inf if there is none; all the trees of least weight must
/// have the same thinness
long long kMinimalBrute( int n , const std::vector< WeightedEdge< int > > & A ,
                         int k )
{
 const unsigned S = 1u << A.size();
 long long best = INFTY , thin = INFTY;
 for( unsigned s = 0 ; s < S ; ++s )
  best = std::min( best , treeOf( n , A , k , s ).first );
 for( unsigned s = 0 ; s < S ; ++s ) {
  const auto [ w , t ] = treeOf( n , A , k , s );
  if( ( w == best ) && ( best < INFTY ) ) {
   CHECK( ( thin == INFTY ) || ( thin == t ) );
   thin = t;
   }
  }
 return( thin );
 }

/// a random unoriented graph of n nodes and at most m edges of weights in
/// [ 1 , 5 ], each pair once
std::vector< WeightedEdge< int > > randomEdges( std::mt19937 & g , int n ,
                                                int m )
{
 std::set< std::pair< int , int > > P;
 std::vector< WeightedEdge< int > > A;
 for( int k = 0 ; k < m ; ++k ) {
  const int a = int( g() % n ) , b = int( g() % n );
  if( ( a != b ) && P.insert( std::minmax( a , b ) ).second )
   A.push_back( { a , b , 1 + int( g() % 5 ) } );
  }
 return( A );
 }

}  // end( namespace )

// the three trees of the statement, against the enumeration of the trees
// and against Kruskal of the library on the edges kept
ADS_TEST( "tree-24.a" )
{
 const std::vector< WeightedEdge< int > > E = { { 0 , 1 , 1 } ,
                                                { 1 , 2 , 2 } ,
                                                { 0 , 3 , 3 } ,
                                                { 3 , 2 , 3 } };
 CHECK( tree::kMinimal( 4 , E , 1 ) == 2 );
 CHECK( tree::kMinimal( 4 , E , 2 ) == 1 );
 CHECK( tree::kMinimal( 4 , E , 3 ) == INFTY );
 CHECK( tree::kMinimal( 2 , { { 0 , 1 , 7 } } , 7 ) == 0 );
 CHECK( tree::kMinimal( 2 , {} , 0 ) == INFTY );
 CHECK_THROWS( tree::kMinimal( 1 , {} , 0 ) , contract_violation );
 CHECK( tree::kMinimal( 4 , E , -100 ) == 2 );          // k below all
 CHECK( tree::kMinimal( 4 , E , 100 ) == INFTY );       // k above all
 CHECK( tree::kMinimal( 3 , { { 0 , 1 , 5 } , { 1 , 2 , 5 } , { 0 , 2 , 5 } } ,
                        5 ) == 0 );         // all the weights equal
 CHECK( tree::kMinimal( 3 , { { 0 , 1 , 1 } } , 0 ) == INFTY ); // disconnected
 CHECK( tree::kMinimal( 2 , { { 0 , 1 , -3 } , { 0 , 1 , 8 } } , -5 ) == 0 );
 CHECK_THROWS( tree::kMinimal( 2 , { { 0 , 2 , 1 } } , 0 ) ,
               contract_violation );
 CHECK_THROWS( tree::kMinimal( 2 , { { -1 , 1 , 1 } } , 0 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const int n = 2 + r % 5 , k = 1 + int( g() % 5 );
  const auto A = randomEdges( g , n , 9 );
  const long long t = tree::kMinimal( n , A , k );
  CHECK( t == kMinimalBrute( n , A , k ) );
  std::vector< WeightedEdge< int > > B;
  for( const auto & e : A )
   if( e.w >= k )
    B.push_back( e );
  const auto T = kruskal( n , B );
  if( int( T.size() ) == n - 1 ) {
   const auto [ lo , hi ] = std::minmax_element( T.begin() , T.end() ,
    []( const auto & a , const auto & b ) { return( a.w < b.w ); } );
   CHECK( t == hi->w - lo->w );
   }
  else
   CHECK( t == INFTY );
  }
 }

/*------ tree-24.b, compito 2018-06-28, K minimal (bonus: thinnest) --------*/

namespace tree {

/// thinnest(): the least thinness of a k-minimal tree for any k >= 1, +inf
/// if there is none; kMinimal() for each distinct weight k, since the
/// k-minimal tree for any other k is that of the next weight, O( m^2 log
/// n ) time (the text gives no solution of the bonus)
/// \pre n >= 2, the weights at least 1
/// \post at least 0
long long thinnest( int n , const std::vector< WeightedEdge< int > > & A )
{
 ADS_PRE( n >= 2 );
 std::set< int > K;
 for( const auto & e : A ) {
  ADS_PRE( e.w >= 1 );
  K.insert( e.w );
  }
 long long best = INFTY;
 for( int k : K )
  best = std::min( best , kMinimal( n , A , k ) );
 return( best );
 }

}  // end( namespace tree )

// the graph of the statement, against the enumeration for every k
ADS_TEST( "tree-24.b" )
{
 const std::vector< WeightedEdge< int > > E = { { 0 , 1 , 1 } ,
                                                { 1 , 2 , 2 } ,
                                                { 0 , 3 , 3 } ,
                                                { 3 , 2 , 3 } };
 CHECK( tree::thinnest( 4 , E ) == 1 );
 CHECK( tree::thinnest( 3 , { { 0 , 1 , 4 } } ) == INFTY );
 CHECK( tree::thinnest( 2 , { { 0 , 1 , 1 } , { 0 , 1 , 9 } } ) == 0 );
 CHECK_THROWS( tree::thinnest( 2 , { { 0 , 1 , 0 } } ) , contract_violation );
 CHECK( tree::thinnest( 2 , {} ) == INFTY );
 CHECK( tree::thinnest( 3 , { { 0 , 1 , 1 } , { 1 , 2 , 9 } } ) == 8 );
 CHECK( tree::thinnest( 3 , { { 0 , 1 , 1 } , { 1 , 2 , 9 } ,
                              { 0 , 2 , 9 } } ) == 0 );   // k = 9
 CHECK_THROWS( tree::thinnest( 1 , {} ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = 2 + r % 5;
  const auto A = randomEdges( g , n , 9 );
  long long best = INFTY;
  for( int k = 1 ; k <= 6 ; ++k )
   best = std::min( best , kMinimalBrute( n , A , k ) );
  CHECK( tree::thinnest( n , A ) == best );
  }
 }

/*-------------- tree-25, compito 2018-06-28, Min coloring -----------------*/

namespace tree {

/// the cost of the colouring of t with color at its root
int countRec( const BT * t , int color )
{
 if( ! t )
  return( 0 );
 return( color + countRec( t->left() , 3 - color ) +
         countRec( t->right() , 3 - color ) );
 }

/// minColoring(): the least sum of the colours 1 and 2 of the nodes, a
/// child coloured differently from its parent; the colours alternate by
/// level, so there are only two colourings, O( n ) time
/// \post at least the nodes
int minColoring( const BT * t )
{
 return( std::min( countRec( t , 1 ) , countRec( t , 2 ) ) );
 }

}  // end( namespace tree )

// against all the 2^n colourings that are valid
ADS_TEST( "tree-25" )
{
 CHECK( tree::minColoring( cat( "1( 2, 3 )" ).get() ) == 4 );
 CHECK( tree::minColoring( nullptr ) == 0 );
 CHECK( tree::minColoring( cat( "1" ).get() ) == 1 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  PT t = randomInts( g , 1 + r % 12 , 0 , 0 );
  const auto N = nodesOf( t.get() );
  const int n = int( N.size() );
  for( int i = 0 ; i < n ; ++i )
   N[ i ]->write( i );
  int best = IINF;
  for( unsigned c = 0 ; c < ( 1u << n ) ; ++c ) {
   bool ok = true;
   for( int i = 1 ; i < n ; ++i )
    ok = ok && ( ( ( c >> i ) & 1u ) != ( ( c >> N[ i ]->parent()->read() ) &
                                          1u ) );
   if( ok )
    best = std::min( best , n + std::popcount( c ) );
   }
  CHECK( tree::minColoring( t.get() ) == best );
  int even = 0;
  for( auto * u : N )
   even += ( depthOf( u ) % 2 == 0 );
  CHECK( best == n + std::min( even , n - even ) );
  }
 CHECK( tree::minColoring( cat( "1( 2, - )" ).get() ) == 3 );
 CHECK( tree::minColoring( chainTree( DEEP , false , []( int ) {
  return( 0 ); } ).get() ) == DEEP + DEEP / 2 );
 CHECK( tree::minColoring( chainTree( DEEP + 1 , true , []( int ) {
  return( 0 ); } ).get() ) == DEEP + 1 + DEEP / 2 );
 PT q( completeTree( std::vector< int >( 15 , 0 ) ) );  // 1 + 4 against
 CHECK( tree::minColoring( q.get() ) == 15 + 5 );       // 2 + 8
 }

/*------------- tree-26, compito 2019-01-21, Is christmassy ----------------*/

namespace tree {

/// isChristmassy(): true if the brilliance of every level (the sum of its
/// values) is the same, true for an empty tree; a visit by levels that
/// counts the nodes left in the current level, Theta( n ) time
bool isChristmassy( BT * t )
{
 if( ! t )
  return( true );
 std::queue< BT * > Q;
 Q.push( t );
 long long sum = 0;
 std::size_t count = 1;
 while( ! Q.empty() ) {
  BT * u = Q.front();
  Q.pop();
  sum += u->read();
  if( u->left() ) Q.push( u->left() );
  if( u->right() ) Q.push( u->right() );
  if( --count == 0 ) {                      // the end of a level
   if( sum != t->read() )
    return( false );
   sum = 0;
   count = Q.size();
   }
  }
 return( true );
 }

}  // end( namespace tree )

// against the sums of the levels by a visit in depth; half of the random
// trees have the values of each level made to sum to the same
ADS_TEST( "tree-26" )
{
 CHECK( tree::isChristmassy( cat( "16( 3( -6( 3, 7 ), 6( 3, - ) ), "
                                  "13( 23( 3, - ), -7 ) )" ).get() ) );
 CHECK( ! tree::isChristmassy( cat( "16( 3( -6( 3, 7 ), 6( 3, - ) ), "
                                    "13( 23( 4, - ), -7 ) )" ).get() ) );
 CHECK( tree::isChristmassy( nullptr ) );
 CHECK( tree::isChristmassy( cat( "-4" ).get() ) );
 CHECK( tree::isChristmassy( cat( "0( -5, 5 )" ).get() ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 14 , -9 , 9 );
  std::map< int , std::vector< BT * > > level;
  for( auto * u : nodesOf( t.get() ) )
   level[ depthOf( u ) ].push_back( u );
  if( r % 2 )
   for( auto & [ l , V ] : level ) {
    long long s = 0;
    for( std::size_t i = 0 ; i + 1 < V.size() ; ++i )
     s += V[ i ]->read();
    V.back()->write( int( t->read() - s ) );
    }
  std::set< long long > S;
  for( auto & [ l , V ] : level ) {
   long long s = 0;
   for( auto * u : V )
    s += u->read();
   S.insert( s );
   }
  CHECK( tree::isChristmassy( t.get() ) == ( S.size() == 1 ) );
  }
 CHECK( tree::isChristmassy( cat( "0( 0, 0 )" ).get() ) );
 CHECK( ! tree::isChristmassy( cat( "2( 1, 2( 2, - ) )" ).get() ) );
 CHECK( tree::isChristmassy( cat( "2( 1, 1( 2, - ) )" ).get() ) );
 CHECK( tree::isChristmassy( chainTree( DEEP , true , []( int ) {
  return( 7 ); } ).get() ) );
 CHECK( ! tree::isChristmassy( chainTree( DEEP , false , []( int d ) {
  return( d + 1 == DEEP ? 6 : 7 ); } ).get() ) );   // only the last level
 const int hi = std::numeric_limits< int >::max();
 CHECK( tree::isChristmassy( cat( std::to_string( hi ) + "( " +
                                  std::to_string( hi ) + ", 0 )" ).get() ) );
 CHECK( ! tree::isChristmassy( cat( "0( " + std::to_string( hi ) + ", " +
                                    std::to_string( hi ) + " )" ).get() ) );
 }

/*------------- tree-27, compito 2019-06-06, Check priority ----------------*/

// isMaxHeap() of the library with std::greater checks a min-heap; against
// std::is_heap
ADS_TEST( "tree-27" )
{
 const std::greater<> G;
 CHECK( isMaxHeap( std::vector< int >( { 1 , 3 , 2 , 7 , 4 } ) , 5 , G ) );
 CHECK( ! isMaxHeap( std::vector< int >( { 1 , 3 , 2 , 7 , 0 } ) , 5 , G ) );
 CHECK( isMaxHeap( std::vector< int >() , 0 , G ) );
 CHECK( isMaxHeap( std::vector< int >( { 5 } ) , 1 , G ) );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( isMaxHeap( std::vector< int >( { lo , hi } ) , 2 , G ) );
 CHECK( ! isMaxHeap( std::vector< int >( { hi , lo } ) , 2 , G ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 800 ; ++r ) {
  std::vector< int > V( r % 16 );
  std::iota( V.begin() , V.end() , 0 );
  std::shuffle( V.begin() , V.end() , g );
  if( r % 2 )
   std::make_heap( V.begin() , V.end() , G );
  if( ( r % 4 == 1 ) && ( V.size() > 2 ) )
   std::swap( V[ 1 + g() % ( V.size() - 1 ) ] , V[ g() % V.size() ] );
  CHECK( isMaxHeap( V , V.size() , G ) ==
         std::is_heap( V.begin() , V.end() , G ) );
  }
 CHECK( isMaxHeap( std::vector< int >( 9 , 4 ) , 9 , G ) );   // all equal
 CHECK( isMaxHeap( std::vector< int >( { 1 , 1 , 2 , 1 } ) , 4 , G ) );
 const std::vector< int > P = { 1 , 2 , 3 , 0 };       // a heap in its
 CHECK( isMaxHeap( P , 3 , G ) && ! isMaxHeap( P , 4 , G ) );  // prefix
 CHECK( isMaxHeap( P , 0 , G ) );
 std::vector< int > S( 10000 );             // sorted, a heap
 std::iota( S.begin() , S.end() , 0 );
 CHECK( isMaxHeap( S , S.size() , G ) );
 std::reverse( S.begin() , S.end() );
 CHECK( ! isMaxHeap( S , S.size() , G ) );
 CHECK_THROWS( isMaxHeap( P , 5 , G ) , contract_violation );
 }

/*---------------- tree-28, compito 2019-06-17, Max path -------------------*/

namespace tree {

/// the heaviest path from t down to a leaf, and the heaviest path between
/// two leaves of t, possibly the same, -inf for nil
std::pair< long long , long long > maxPathRec( const BT * t )
{
 if( ! t )
  return( std::pair( -INFTY , -INFTY ) );
 const long long w = t->read();
 if( ! t->left() && ! t->right() )
  return( std::pair( w , w ) );
 const auto [ hL , mL ] = maxPathRec( t->left() );
 const auto [ hR , mR ] = maxPathRec( t->right() );
 return( std::pair( std::max( hL , hR ) + w ,
                    std::max( { mL , mR , hL + hR + w } ) ) );
 }

/// maxPath(): the heaviest path between two leaves, possibly the same,
/// whose weight is the sum of its nodes; a postvisit returning the
/// heaviest path down to a leaf and the heaviest one inside, Theta( n )
/// time
/// \pre t is not nil
long long maxPath( const BT * t )
{
 ADS_PRE( t != nullptr );
 return( maxPathRec( t ).second );
 }

}  // end( namespace tree )

// the two trees of the statement, against every pair of leaves
ADS_TEST( "tree-28" )
{
 CHECK( tree::maxPath( cat( "8( -3( 2, 3 ), -3( 1, 5( 6, 6 ) ) )" ).get() )
        == 17 );
 CHECK( tree::maxPath( cat( "8( -3( 2, 3 ), -3( 1, 5( 4, 5 ) ) )" ).get() )
        == 15 );
 CHECK( tree::maxPath( cat( "-7" ).get() ) == -7 );
 CHECK( tree::maxPath( cat( "1( 2( 3, - ), - )" ).get() ) == 3 );
 CHECK_THROWS( tree::maxPath( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  PT t = randomInts( g , 1 + r % 14 , -9 , 9 );
  CHECK( tree::maxPath( t.get() ) == leafPathsBrute( t.get() , true ) );
  }
 CHECK( tree::maxPath( cat( "-1( -2, -3 )" ).get() ) == -2 );  // one leaf
 CHECK( tree::maxPath( cat( "9( 1, 1 )" ).get() ) == 11 );
 CHECK( tree::maxPath( cat( "-9( 1, 1 )" ).get() ) == 1 );
 CHECK( tree::maxPath( chainTree( DEEP , false , []( int d ) {
  return( d + 1 ); } ).get() ) == DEEP );   // the lone leaf
 const int hi = std::numeric_limits< int >::max();
 PT q( completeTree( std::vector< int >( 63 , hi ) ) );  // 11 nodes of hi
 CHECK( tree::maxPath( q.get() ) == 11LL * hi );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File exam_treea.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
