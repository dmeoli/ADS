/*--------------------------------------------------------------------------*/
/*---------------------- File exam_esercitazionia.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems of the exercise sessions on trees, analysis of algorithms
 * and divide et impera (the first half; exam_esercitazionib.cpp has the
 * second): each one solved as in its official solution, or by the
 * function of the library that already solves it, and checked on the
 * examples of its text, against an independent oracle (a brute force, the
 * standard library, another algorithm of the library) on random instances
 * with a fixed seed, and on the edge cases. The exercises of analysis are
 * checked by evaluating their recurrences and the bounds claimed for them.
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

#include "ADS/Contract.h"
#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

namespace esercitazioni {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a uniform integer in [ a , b ]
int rnd( std::mt19937 & g , int a , int b )
{
 return( std::uniform_int_distribution< int >( a , b )( g ) );
 }

/// a random vector of n integers in [ a , b ]
std::vector< int > rndVector( std::mt19937 & g , int n , int a , int b )
{
 std::vector< int > V( n );
 for( auto & v : V ) v = rnd( g , a , b );
 return( V );
 }

using BT = BinaryTree< int >;
using Owner = std::unique_ptr< BT >;

/// a new node of value v with the children l and r, each possibly nil
template< class T >
BinaryTree< T > * mk( const T & v ,
                      std::type_identity_t< BinaryTree< T > * > l = nullptr ,
                      std::type_identity_t< BinaryTree< T > * > r = nullptr )
{
 auto * t = new BinaryTree< T >( v );
 if( l ) t->insertLeft( l );
 if( r ) t->insertRight( r );
 return( t );
 }

/// a random binary tree of n nodes, the values drawn by v(); nil if n = 0
template< class T , class V >
BinaryTree< T > * randomTree( std::mt19937 & g , int n , V && v )
{
 if( n == 0 ) return( nullptr );
 const int l = rnd( g , 0 , n - 1 );
 auto * t = new BinaryTree< T >( v() );
 if( auto * L = randomTree< T >( g , l , v ) ) t->insertLeft( L );
 if( auto * R = randomTree< T >( g , n - 1 - l , v ) ) t->insertRight( R );
 return( t );
 }

/// the nodes of t by levels, from the left
template< class T >
std::vector< const BinaryTree< T > * > byLevels( const BinaryTree< T > * t )
{
 std::vector< const BinaryTree< T > * > out;
 if( t ) out.push_back( t );
 for( std::size_t i = 0 ; i < out.size() ; ++i ) {
  if( out[ i ]->left() ) out.push_back( out[ i ]->left() );
  if( out[ i ]->right() ) out.push_back( out[ i ]->right() );
  }
 return( out );
 }

/// the depth of the node t, the root at depth 0
template< class T >
int depthOf( const BinaryTree< T > * t )
{
 int d = 0;
 for( ; t->parent() ; t = t->parent() ) ++d;
 return( d );
 }

/// the number of nodes of t
template< class T >
int nodes( const BinaryTree< T > * t )
{
 return( t ? 1 + nodes( t->left() ) + nodes( t->right() ) : 0 );
 }

/// the height of t, -1 if empty
template< class T >
int heightOf( const BinaryTree< T > * t )
{
 return( t ? 1 + std::max( heightOf( t->left() ) , heightOf( t->right() ) )
           : -1 );
 }

/// the values of t in symmetric order
std::vector< int > inorder( BT * t )
{
 std::vector< int > out;
 in_visit( t , [ & ]( BT * u ) { out.push_back( u->read() ); } );
 return( out );
 }

/// a random graph of n nodes, each edge present with probability p
ListGraph<> randomGraph( std::mt19937 & g , int n , double p ,
                         bool directed )
{
 ListGraph<> G( n , directed );
 std::bernoulli_distribution e( p );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && e( g ) )
    G.insertEdge( u , v );
 return( G );
 }

/*--------------------------------------------------------------------------*/
/*------------ 1 - Alberi, cammino radice-discendente crescente ------------*/
/*--------------------------------------------------------------------------*/


/// the length in edges of the longest strictly increasing path from the
/// root of t down to some node: a child extends the path of its parent
/// only if its value is greater; Theta( n ) time, 0 for an empty tree
int monotone( const BT * t )
{
 int maxl = 0 , maxr = 0;
 if( t ) {
  if( t->left() && ( t->left()->read() > t->read() ) )
   maxl = 1 + monotone( t->left() );
  if( t->right() && ( t->right()->read() > t->read() ) )
   maxr = 1 + monotone( t->right() );
  }
 return( std::max( maxl , maxr ) );
 }

/// the brute force: every path from the root, checked whole
void monotoneBrute( const BT * t , std::vector< int > & path , int & best )
{
 if( ! t ) return;
 path.push_back( t->read() );
 bool up = true;
 for( std::size_t i = 1 ; i < path.size() ; ++i )
  up = up && ( path[ i - 1 ] < path[ i ] );
 if( up ) best = std::max( best , int( path.size() ) - 1 );
 monotoneBrute( t->left() , path , best );
 monotoneBrute( t->right() , path , best );
 path.pop_back();
 }

// every path from the root of 500 random trees, the empty tree, one node,
// a chain of equal values (0) and a strictly increasing chain (n - 1)
ADS_TEST( "esercitazioni-1" )
{
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  Owner T( randomTree< int >( g , rnd( g , 0 , 30 ) ,
                              [ & ] { return( rnd( g , 0 , 6 ) ); } ) );
  std::vector< int > path;
  int best = 0;
  monotoneBrute( T.get() , path , best );
  CHECK( monotone( T.get() ) == best );
  }
 CHECK( monotone( nullptr ) == 0 );
 Owner one( mk( 5 ) );
 CHECK( monotone( one.get() ) == 0 );
 Owner eq( mk( 1 , mk( 1 , mk( 1 ) ) ) ) ,
       up( mk( 1 , nullptr , mk( 2 , mk( 3 , nullptr , mk( 4 ) ) ) ) );
 CHECK( monotone( eq.get() ) == 0 && monotone( up.get() ) == 3 );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 2 - Inserisci l'albero (2020-07-03) -------------------*/
/*--------------------------------------------------------------------------*/


/// a value and the size of its subtree
struct Sized {
 int size = 0;    ///< the number of nodes of the subtree
 int value = 0;   ///< the value
 };

using SBT = BinaryTree< Sized >;

/// the size of the subtree t, 0 if nil
int sizeOf( const SBT * t ) { return( t ? t->read().size : 0 ); }

/// writes the sizes of the subtrees of t
int setSizes( SBT * t )
{
 if( ! t ) return( 0 );
 const int s = 1 + setSizes( t->left() ) + setSizes( t->right() );
 t->write( Sized{ s , 0 } );
 return( s );
 }

/// biRec(): the values A[ i ... ] in the subtree t, as in a symmetric visit
void biRec( SBT * t , const std::vector< int > & A , int i )
{
 if( ! t ) return;
 const int l = sizeOf( t->left() );
 biRec( t->left() , A , i );
 t->write( Sized{ t->read().size , A[ i + l ] } );
 biRec( t->right() , A , i + l + 1 );
 }

/// binaryInsert(): the sorted distinct values of A written in the tree T,
/// whose size fields are set, so that it becomes a search tree: the root
/// of each subtree takes the item after those of its left subtree; Theta( n )
/// \pre T has A.size() >= 1 nodes, A strictly increasing
void binaryInsert( SBT * T , const std::vector< int > & A )
{
 ADS_PRE( T && ( sizeOf( T ) == int( A.size() ) ) );
 ADS_PRE( std::ranges::adjacent_find( A , std::greater_equal<>() ) ==
          A.end() );
 biRec( T , A , 0 );
 }

/// the values of t in symmetric order
std::vector< int > inorderSized( SBT * t )
{
 std::vector< int > out;
 in_visit( t , [ & ]( SBT * u ) { out.push_back( u->read().value ); } );
 return( out );
 }

// the shape of the text filled with its array (by levels), 500 random
// shapes whose symmetric visit must give A back, a chain, one node
ADS_TEST( "esercitazioni-2" )
{
 const auto z = Sized();
 std::unique_ptr< SBT > T( mk( z , mk( z , mk( z , mk( z ) , mk( z ) ) ,
                                         mk( z , mk( z ) ) ) ,
                                  mk( z , mk( z ) , mk( z ) ) ) );
 setSizes( T.get() );
 binaryInsert( T.get() , { 11 , 13 , 15 , 19 , 21 , 22 , 25 , 27 , 28 ,
                           31 } );
 std::vector< int > lv;
 for( const auto * u : byLevels( static_cast< const SBT * >( T.get() ) ) )
  lv.push_back( u->read().value );
 CHECK( lv == std::vector< int >( { 25 , 19 , 28 , 13 , 22 , 27 , 31 , 11 ,
                                    15 , 21 } ) );
 std::mt19937 g( 2 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 40 );
  std::unique_ptr< SBT > S( randomTree< Sized >( g , n , [ & ] {
                                                  return( Sized() ); } ) );
  setSizes( S.get() );
  std::set< int > V;
  while( int( V.size() ) < n ) V.insert( rnd( g , -1000 , 1000 ) );
  const std::vector< int > A( V.begin() , V.end() );
  binaryInsert( S.get() , A );
  CHECK( inorderSized( S.get() ) == A );
  }
 std::unique_ptr< SBT > C( mk( z , mk( z , mk( z ) ) ) ) , one( mk( z ) );
 setSizes( C.get() );
 setSizes( one.get() );
 binaryInsert( C.get() , { 1 , 2 , 3 } );
 binaryInsert( one.get() , { 7 } );
 CHECK( C->read().value == 3 && C->left()->left()->read().value == 1 );
 CHECK( one->read().value == 7 );
 CHECK_THROWS( binaryInsert( C.get() , { 1 , 1 , 2 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 3 - Alberi, grado di sbilanciamento -------------------*/
/*--------------------------------------------------------------------------*/


/// unbalanceRec(): the leaves of t and the largest unbalance in it
std::pair< int , int > unbalanceRec( const BT * t )
{
 if( ! t ) return( std::pair( 0 , 0 ) );
 if( ! t->left() && ! t->right() ) return( std::pair( 1 , 0 ) );
 const auto [ ll , lm ] = unbalanceRec( t->left() );
 const auto [ rl , rm ] = unbalanceRec( t->right() );
 return( std::pair( ll + rl ,
                    std::max( { lm , rm , std::abs( ll - rl ) } ) ) );
 }

/// the unbalance degree of t, the largest difference over its nodes between
/// the leaves of the left and of the right subtree, by one postvisit that
/// returns the leaves and the largest unbalance; Theta( n ), 0 if empty
int unbalance( const BT * t ) { return( unbalanceRec( t ).second ); }

/// the leaves of t, naively
int leaves( const BT * t )
{
 if( ! t ) return( 0 );
 if( ! t->left() && ! t->right() ) return( 1 );
 return( leaves( t->left() ) + leaves( t->right() ) );
 }

// against the quadratic count of the leaves of each subtree on 500 random
// trees, the empty tree, one node, a chain (1 at each node)
ADS_TEST( "esercitazioni-3" )
{
 std::mt19937 g( 3 );
 for( int r = 0 ; r < 500 ; ++r ) {
  Owner T( randomTree< int >( g , rnd( g , 0 , 40 ) , [] { return( 0 ); } ) );
  int best = 0;
  pre_visit( T.get() , [ & ]( BT * u ) {
   best = std::max( best , std::abs( leaves( u->left() ) -
                                     leaves( u->right() ) ) ); } );
  CHECK( unbalance( T.get() ) == best );
  }
 Owner one( mk( 0 ) ) , chain( mk( 0 , mk( 0 , mk( 0 ) ) ) );
 CHECK( unbalance( nullptr ) == 0 && unbalance( one.get() ) == 0 );
 CHECK( unbalance( chain.get() ) == 1 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 4 - Alberi, larghezza albero ----------------------*/
/*--------------------------------------------------------------------------*/


/// the width of the ordered tree t, the largest number of nodes on a level,
/// by a visit by levels that keeps in the queue exactly one level at a
/// time; Theta( n )
/// \pre t is not nil
int width( Tree< int > * t )
{
 ADS_PRE( t != nullptr );
 std::queue< Tree< int > * > Q;
 Q.push( t );
 int best = 0;
 while( ! Q.empty() ) {
  const int level = int( Q.size() );
  best = std::max( best , level );
  for( int i = 0 ; i < level ; ++i ) {
   Tree< int > * u = Q.front();
   Q.pop();
   for( auto * c = u->leftmostChild() ; c ; c = c->rightSibling() )
    Q.push( c );
   }
  }
 return( best );
 }

// against the largest class of the depths, found by the previsit of the
// library, on 500 random ordered trees, one node and a star
ADS_TEST( "esercitazioni-4" )
{
 std::mt19937 g( 4 );
 for( int r = 0 ; r < 501 ; ++r ) {
  const int n = r < 500 ? rnd( g , 1 , 60 ) : 1;
  std::vector< Tree< int > * > N( 1 , new Tree< int >( 0 ) );
  std::unique_ptr< Tree< int > > root( N[ 0 ] );
  for( int i = 1 ; i < n ; ++i ) {
   N.push_back( new Tree< int >( i ) );
   N[ rnd( g , 0 , i - 1 ) ]->insertChild( N.back() );
   }
  std::map< int , int > byDepth;
  pre_visit( root.get() , [ & ]( Tree< int > * u ) {
   int d = 0;
   for( ; u->parent() ; u = u->parent() ) ++d;
   ++byDepth[ d ]; } );
  int best = 0;
  for( const auto & [ d , c ] : byDepth ) best = std::max( best , c );
  CHECK( width( root.get() ) == best );
  }
 std::unique_ptr< Tree< int > > star( new Tree< int >( 0 ) );
 for( int i = 1 ; i <= 9 ; ++i ) star->insertChild( new Tree< int >( i ) );
 CHECK( width( star.get() ) == 9 );
 CHECK_THROWS( width( nullptr ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 5 - Alberi, albero livello-valore --------------------*/
/*--------------------------------------------------------------------------*/


/// sameLevelRec(): the nodes of t, at level l, whose value is their level
int sameLevelRec( const BT * t , int l )
{
 if( ! t ) return( 0 );
 return( sameLevelRec( t->left() , l + 1 ) +
         sameLevelRec( t->right() , l + 1 ) + ( t->read() == l ? 1 : 0 ) );
 }

/// the number of nodes of t whose value equals their level, the root at
/// level 0, by a previsit that carries the level; Theta( n )
int sameLevel( const BT * t ) { return( sameLevelRec( t , 0 ) ); }

// against a visit by levels on 500 random trees, the empty tree, one node
ADS_TEST( "esercitazioni-5" )
{
 std::mt19937 g( 5 );
 for( int r = 0 ; r < 500 ; ++r ) {
  Owner T( randomTree< int >( g , rnd( g , 0 , 40 ) ,
                              [ & ] { return( rnd( g , 0 , 5 ) ); } ) );
  int c = 0;
  for( const auto * u : byLevels( static_cast< const BT * >( T.get() ) ) )
   c += ( u->read() == depthOf( u ) ) ? 1 : 0;
  CHECK( sameLevel( T.get() ) == c );
  }
 Owner zero( mk( 0 ) ) , one( mk( 1 ) ) , all( mk( 0 , mk( 1 , mk( 2 ) ) ,
                                                       mk( 1 ) ) );
 CHECK( sameLevel( nullptr ) == 0 && sameLevel( zero.get() ) == 1 );
 CHECK( sameLevel( one.get() ) == 0 && sameLevel( all.get() ) == 4 );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 6 - Alberi simmetrici (2020-07-24) -------------------*/
/*--------------------------------------------------------------------------*/


/// isMirror(): whether the shape of tL is the mirror image of that of tR
bool isMirror( const BT * tL , const BT * tR )
{
 if( ! tL && ! tR ) return( true );
 if( tL && tR )
  return( isMirror( tL->right() , tR->left() ) &&
          isMirror( tL->left() , tR->right() ) );
 return( false );
 }

/// whether the left subtree of the root of T is the mirror image of the
/// right one, as shapes (the values are not compared, as in the official
/// solution); Theta( n )
/// \pre T is not nil
bool isSymmetric( const BT * T )
{
 ADS_PRE( T != nullptr );
 return( isMirror( T->left() , T->right() ) );
 }

/// the mirror image of t, a new tree
BT * mirrorCopy( const BT * t )
{
 return( t ? mk( t->read() , mirrorCopy( t->right() ) ,
                 mirrorCopy( t->left() ) ) : nullptr );
 }

/// whether a and b have the same shape
bool sameShape( const BT * a , const BT * b )
{
 if( ! a || ! b ) return( a == b );
 return( sameShape( a->left() , b->left() ) &&
         sameShape( a->right() , b->right() ) );
 }

// against the comparison of the right subtree with the mirror copy of the
// left one, on 500 random trees, half of them built symmetric and some of
// those perturbed by one leaf; one node is symmetric
ADS_TEST( "esercitazioni-6" )
{
 std::mt19937 g( 6 );
 for( int r = 0 ; r < 500 ; ++r ) {
  Owner T;
  if( r % 2 ) {
   Owner L( randomTree< int >( g , rnd( g , 0 , 15 ) , [] { return( 0 ); } ) );
   T.reset( mk( 0 , copy( L.get() ) , mirrorCopy( L.get() ) ) );
   if( r % 4 == 1 ) {                       // a new leaf somewhere
    BT * u = T.get();
    while( u->left() && u->right() )
     u = rnd( g , 0 , 1 ) ? u->left() : u->right();
    if( ! u->left() ) u->insertLeft( mk( 0 ) );
    else u->insertRight( mk( 0 ) );
    }
   }
  else
   T.reset( randomTree< int >( g , rnd( g , 1 , 15 ) , [] { return( 0 ); } ) );
  Owner M( mirrorCopy( T->left() ) );
  CHECK( isSymmetric( T.get() ) == sameShape( M.get() , T->right() ) );
  if( r % 4 == 3 ) CHECK( isSymmetric( T.get() ) );
  }
 Owner one( mk( 1 ) ) , two( mk( 1 , mk( 2 ) ) );
 CHECK( isSymmetric( one.get() ) && ! isSymmetric( two.get() ) );
 CHECK_THROWS( isSymmetric( nullptr ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 7 - Alberi, sopra e sotto ------------------------*/
/*--------------------------------------------------------------------------*/


/// countTreeRec(): the size of the subtree t, whose root has a ancestors,
/// and the nodes in it with as many ancestors as descendants
std::pair< int , int > countTreeRec( const BT * t , int a )
{
 if( ! t ) return( std::pair( 0 , 0 ) );
 const auto [ sl , cl ] = countTreeRec( t->left() , a + 1 );
 const auto [ sr , cr ] = countTreeRec( t->right() , a + 1 );
 return( std::pair( sl + sr + 1 , cl + cr + ( a == sl + sr ? 1 : 0 ) ) );
 }

/// the number of nodes of t with as many ancestors as descendants, by a
/// visit that passes the ancestors down and returns the sizes up; Theta( n )
int aboveBelow( const BT * t ) { return( countTreeRec( t , 0 ).second ); }

// against the depth and the size of each node computed separately, on 500
// random trees, the empty tree, one node (0 ancestors, 0 descendants), a
// chain of three nodes (the middle one)
ADS_TEST( "esercitazioni-7" )
{
 std::mt19937 g( 7 );
 for( int r = 0 ; r < 500 ; ++r ) {
  Owner T( randomTree< int >( g , rnd( g , 0 , 40 ) , [] { return( 0 ); } ) );
  int c = 0;
  pre_visit( T.get() , [ & ]( BT * u ) {
   c += ( depthOf( u ) == nodes( u ) - 1 ) ? 1 : 0; } );
  CHECK( aboveBelow( T.get() ) == c );
  }
 Owner one( mk( 0 ) ) , chain( mk( 0 , mk( 0 , mk( 0 ) ) ) );
 CHECK( aboveBelow( nullptr ) == 0 && aboveBelow( one.get() ) == 1 );
 CHECK( aboveBelow( chain.get() ) == 1 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 8 - Alberi, indovina l'albero ----------------------*/
/*--------------------------------------------------------------------------*/


/// the subtree of the previsit pre[ p ... ] and of the symmetric visit
/// in[ a ... b - 1 ], the place of each value of in given by at
template< class T >
BinaryTree< T > * fromPreInRec( const std::vector< T > & pre ,
                                const std::map< T , int > & at , int & p ,
                                int a , int b )
{
 if( a == b ) return( nullptr );
 const T & v = pre[ p++ ];
 const int r = at.at( v );
 auto * L = fromPreInRec( pre , at , p , a , r );
 auto * R = fromPreInRec( pre , at , p , r + 1 , b );
 return( mk( v , L , R ) );
 }

// candidate for the library
/// the binary tree of distinct values whose previsit is pre and whose
/// symmetric visit is in: the first item of pre is the root, which splits
/// in into the two subtrees; Theta( n log n ) with the map of the places;
/// nil if empty; the caller owns the result
/// \pre pre and in are permutations of the same distinct values
template< class T >
BinaryTree< T > * fromPreIn( const std::vector< T > & pre ,
                             const std::vector< T > & in )
{
 std::map< T , int > at;
 for( int i = 0 ; i < int( in.size() ) ; ++i ) at[ in[ i ] ] = i;
 ADS_PRE( ( pre.size() == in.size() ) && ( at.size() == in.size() ) &&
          std::ranges::is_permutation( pre , in ) );
 int p = 0;
 return( fromPreInRec( pre , at , p , 0 , int( in.size() ) ) );
 }

/// the tree as v( left , right ), - for a missing child, v for a leaf
std::string show( const BinaryTree< char > * t )
{
 if( ! t ) return( "-" );
 std::string s( 1 , t->read() );
 if( t->left() || t->right() )
  s += "(" + show( t->left() ) + ", " + show( t->right() ) + ")";
 return( s );
 }

/// the three visits of t
template< class T >
std::array< std::vector< T > , 3 > visits( BinaryTree< T > * t )
{
 std::array< std::vector< T > , 3 > V;
 dfs( t , [ & ]( auto * u ) { V[ 0 ].push_back( u->read() ); } ,
      [ & ]( auto * u ) { V[ 1 ].push_back( u->read() ); } ,
      [ & ]( auto * u ) { V[ 2 ].push_back( u->read() ); } );
 return( V );
 }

// the tree of the text from its previsit and symmetric visit, checked on
// its shape and on the postvisit of the text; 500 random trees rebuilt
// from their visits; the empty tree; inconsistent visits rejected
ADS_TEST( "esercitazioni-8" )
{
 const std::string pre = "AEBFGCDIH" , post = "BGCFEHIDA" , in = "BEGFCADHI";
 std::unique_ptr< BinaryTree< char > > T( fromPreIn(
  std::vector< char >( pre.begin() , pre.end() ) ,
  std::vector< char >( in.begin() , in.end() ) ) );
 CHECK( show( T.get() ) == "A(E(B, F(G, C)), D(-, I(H, -)))" );
 CHECK( visits( T.get() )[ 2 ] == std::vector< char >( post.begin() ,
                                                         post.end() ) );
 std::mt19937 g( 8 );
 for( int r = 0 ; r < 500 ; ++r ) {
  int next = 0;
  Owner U( randomTree< int >( g , rnd( g , 0 , 40 ) ,
                              [ & ] { return( next++ * 7 % 101 ); } ) );
  const auto V = visits( U.get() );
  Owner W( fromPreIn( V[ 0 ] , V[ 1 ] ) );
  CHECK( visits( W.get() ) == V && sameShape( W.get() , U.get() ) );
  }
 CHECK( fromPreIn( std::vector< int >() , std::vector< int >() ) == nullptr );
 CHECK_THROWS( fromPreIn( std::vector< int >( { 1 , 2 } ) ,
                          std::vector< int >( { 1 , 3 } ) ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- 10 - Crazy -------------------------------*/
/*--------------------------------------------------------------------------*/


/// the cost of crazy( n ), T( n ) = 2 T( n / 4 ) + T( n / 2 ) + n^2 for
/// n > 1 and 1 otherwise, for each n <= N, by its recurrence with floors
std::vector< double > crazyCost( int N )
{
 std::vector< double > T( N + 1 , 1 );
 for( int n = 2 ; n <= N ; ++n )
  T[ n ] = 2 * T[ n / 4 ] + T[ n / 2 ] + double( n ) * n;
 return( T );
 }

// T( 2 ) = 7, T( 3 ) = 12 as in the text, and n^2 <= T( n ) <= 7 / 4 n^2,
// the constant of the substitution, up to 10^6: Theta( n^2 )
ADS_TEST( "esercitazioni-10" )
{
 const int N = 1000000;
 const auto T = crazyCost( N );
 CHECK( T[ 0 ] == 1 && T[ 1 ] == 1 && T[ 2 ] == 7 && T[ 3 ] == 12 );
 for( int n = 1 ; n <= N ; ++n ) {
  const double q = T[ n ] / ( double( n ) * n );
  CHECK( ( q >= 1 ) && ( q <= 1.75 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 11 - Famiglia (2020-07-24) -----------------------*/
/*--------------------------------------------------------------------------*/


/// T( n ) = a T( n / 2 ) + n^( a - 1 ) for n >= 2, 1 otherwise, n <= N
std::vector< double > family( int a , int N )
{
 std::vector< double > T( N + 1 , 1 );
 for( int n = 2 ; n <= N ; ++n )
  T[ n ] = a * T[ n / 2 ] + std::pow( double( n ) , a - 1 );
 return( T );
 }

// the three cases of the text by the master theorem of the library for
// a = 1 ... 10, and the ratio of the recurrence to the claimed order
// bounded on the powers of 2 up to 2^20, for a = 1 ... 4
ADS_TEST( "esercitazioni-11" )
{
 CHECK( master( 1 , 2 , 0 ) == ( Growth{ 1 , 0 , 1 , 0 , false } ) );
 CHECK( master( 2 , 2 , 1 ) == ( Growth{ 1 , 1 , 1 , 0 , false } ) );
 for( int a = 3 ; a <= 10 ; ++a )
  CHECK( master( a , 2 , a - 1 ) == ( Growth{ 1 , double( a - 1 ) , 0 , 0 ,
                                               false } ) );
 const int N = 1 << 20;
 for( int a = 1 ; a <= 4 ; ++a ) {
  const auto T = family( a , N );
  const Growth G = master( a , 2 , a - 1 );
  double lo = 1e300 , hi = 0;
  for( int n = 1 << 10 ; n <= N ; n *= 2 ) {
   const double q = T[ n ] / ( std::pow( double( n ) , G.p ) *
                               std::pow( std::log2( double( n ) ) , G.q ) );
   lo = std::min( lo , q );
   hi = std::max( hi , q );
   }
  CHECK( hi / lo < 1.2 );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------- 12 - Albero di ricorsione su MergeSortK -----------------*/
/*--------------------------------------------------------------------------*/


/// T( k^h ) for T( n ) = k T( n / k ) + k n, T( 1 ) = 1
std::uint64_t mergeTree( std::uint64_t k , int h )
{
 std::uint64_t T = 1 , n = 1;
 for( int i = 1 ; i <= h ; ++i ) {
  n *= k;
  T = k * T + k * n;
  }
 return( T );
 }

/// whether T( k^h ) = k n log_k n + n, the cost of the recursion tree, for
/// each h with k^h <= 2^40, and Theta( n log n ) by the master theorem
bool mergeTreeClosed( std::uint64_t k )
{
 bool ok = master( double( k ) , double( k ) , 1 ) ==
           ( Growth{ 1 , 1 , 1 , 0 , false } );
 std::uint64_t n = 1;
 for( int h = 0 ; n <= ( std::uint64_t( 1 ) << 40 ) ; ++h , n *= k )
  ok = ok && ( mergeTree( k , h ) == k * n * std::uint64_t( h ) + n );
 return( ok );
 }

// 2 n log n + n for k = 2
ADS_TEST( "esercitazioni-12-1" )
{
 CHECK( mergeTreeClosed( 2 ) );
 CHECK( mergeTree( 2 , 3 ) == 2 * 8 * 3 + 8 );
 }

// 3 n log_3 n + n for k = 3
ADS_TEST( "esercitazioni-12-2" )
{
 CHECK( mergeTreeClosed( 3 ) );
 CHECK( mergeTree( 3 , 2 ) == 3 * 9 * 2 + 9 );
 }

// k n log_k n + n for k = 2 ... 16
ADS_TEST( "esercitazioni-12-3" )
{
 for( std::uint64_t k = 2 ; k <= 16 ; ++k )
  CHECK( mergeTreeClosed( k ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 13 - Analisi, MergeSortK ------------------------*/
/*--------------------------------------------------------------------------*/


/// mergeK(): merges the k sorted parts A[ start[ i ] ... start[ i + 1 ] - 1 ]
/// by choosing each time the least head, the first one among equals (so
/// that it is stable); Theta( n k ); the heads examined are added to *steps
/// \pre start is non-decreasing with k + 1 places, and the parts are sorted
template< class T , class C = std::less<> >
void mergeK( std::vector< T > & A , const std::vector< int > & start , int k ,
             long * steps = nullptr , C less = C() )
{
 ADS_PRE( ( k > 0 ) && ( int( start.size() ) == k + 1 ) &&
          std::ranges::is_sorted( start ) );
 for( int i = 0 ; i < k ; ++i )
  ADS_PRE( std::is_sorted( A.begin() + start[ i ] ,
                           A.begin() + start[ i + 1 ] , less ) );
 std::vector< int > index( start.begin() , start.end() - 1 );
 const int length = start[ k ] - start[ 0 ];
 std::vector< T > B;
 B.reserve( length );
 while( int( B.size() ) < length ) {
  int m = -1;
  for( int i = 0 ; i < k ; ++i ) {
   if( steps ) ++*steps;
   if( ( index[ i ] < start[ i + 1 ] ) &&
       ( ( m == -1 ) || less( A[ index[ i ] ] , A[ index[ m ] ] ) ) )
    m = i;
   }
  B.push_back( A[ index[ m ]++ ] );
  }
 std::copy( B.begin() , B.end() , A.begin() + start[ 0 ] );
 }

/// mergeSortK(): sorts A[ i ... j - 1 ] by splitting it into k parts of
/// sizes as equal as possible, sorting them recursively and merging them
/// with mergeK(); Theta( n k log_k n ), Theta( n log n ) for k constant
/// \pre k >= 2 and 0 <= i <= j <= A.size()
template< class T , class C = std::less<> >
void mergeSortK( std::vector< T > & A , int i , int j , int k ,
                 long * steps = nullptr , C less = C() )
{
 ADS_PRE( ( k >= 2 ) && ( 0 <= i ) && ( i <= j ) && ( j <= int( A.size() ) ) );
 const int n = j - i;
 if( n <= 1 ) return;
 std::vector< int > start( k + 1 );
 for( int t = 0 ; t <= k ; ++t ) start[ t ] = i + int( long( t ) * n / k );
 for( int t = 0 ; t < k ; ++t )
  mergeSortK( A , start[ t ] , start[ t + 1 ] , k , steps , less );
 mergeK( A , start , k , steps , less );
 }

// mergeSortK against std::stable_sort for k = 2 ... 7 on random vectors
// with repetitions, keyed pairs to see the stability; mergeK against the
// sort of the concatenation of k random sorted runs, some empty
ADS_TEST( "esercitazioni-13-1" )
{
 std::mt19937 g( 13 );
 const auto byKey = []( const std::pair< int , int > & a ,
                        const std::pair< int , int > & b ) {
  return( a.first < b.first ); };
 for( int r = 0 ; r < 600 ; ++r ) {
  const int k = 2 + r % 6 , n = rnd( g , 0 , 80 );
  std::vector< std::pair< int , int > > A( n );
  for( int i = 0 ; i < n ; ++i ) A[ i ] = std::pair( rnd( g , 0 , 9 ) , i );
  auto B = A;
  mergeSortK( A , 0 , n , k , nullptr , byKey );
  std::stable_sort( B.begin() , B.end() , byKey );
  CHECK( A == B );
  std::vector< int > V , start( 1 , 0 );
  for( int t = 0 ; t < k ; ++t ) {
   auto run = rndVector( g , rnd( g , 0 , 6 ) , -5 , 5 );
   std::ranges::sort( run );
   V.insert( V.end() , run.begin() , run.end() );
   start.push_back( int( V.size() ) );
   }
  auto W = V;
  mergeK( V , start , k );
  std::ranges::sort( W );
  CHECK( V == W );
  }
 std::vector< int > one( 1 , 4 ) , unsorted( { 2 , 1 } );
 mergeSortK( one , 0 , 1 , 3 );
 CHECK( one == std::vector< int >( 1 , 4 ) );
 CHECK_THROWS( mergeK( unsorted , { 0 , 2 } , 1 ) , contract_violation );
 CHECK_THROWS( mergeSortK( one , 0 , 1 , 1 ) , contract_violation );
 }

// T( n ) = k T( n / k ) + Theta( n k ): on n = k^h mergeK examines exactly
// k heads per item on each of the h levels, n k log_k n in all; the master
// theorem gives Theta( n log n )
ADS_TEST( "esercitazioni-13-2" )
{
 std::mt19937 g( 132 );
 for( int k = 2 ; k <= 5 ; ++k ) {
  CHECK( master( k , k , 1 ) == ( Growth{ 1 , 1 , 1 , 0 , false } ) );
  int n = 1;
  for( int h = 0 ; n <= 50000 ; ++h , n *= k ) {
   auto A = rndVector( g , n , 0 , 1000 );
   long steps = 0;
   mergeSortK( A , 0 , n , k , &steps );
   CHECK( steps == long( n ) * k * h && std::ranges::is_sorted( A ) );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ 15 - Ricorrenza 4T(sqrt n) + log^2 n ------------------*/
/*--------------------------------------------------------------------------*/


/// the integer square root of n
std::uint64_t isqrt( std::uint64_t n )
{
 std::uint64_t r = std::uint64_t( std::sqrt( double( n ) ) );
 while( r * r > n ) --r;
 while( ( r + 1 ) * ( r + 1 ) <= n ) ++r;
 return( r );
 }

/// T( n ) = 4 T( floor( sqrt( n ) ) ) + log^2 n for n > 1, T( 1 ) = 1
double sqrtRec( std::uint64_t n , std::map< std::uint64_t , double > & M )
{
 if( n <= 1 ) return( 1 );
 if( auto it = M.find( n ) ; it != M.end() ) return( it->second );
 const double l = std::log2( double( n ) );
 return( M[ n ] = 4 * sqrtRec( isqrt( n ) , M ) + l * l );
 }

// with n = 2^k, S( k ) = 4 S( k / 2 ) + k^2 is Theta( k^2 log k ) by the
// master theorem; exactly T( n ) = log^2 n ( log log n + 5 ) on
// n = 2^( 2^j ), and T( n ) / ( log^2 n log log n ) bounded on all n
ADS_TEST( "esercitazioni-15" )
{
 CHECK( master( 4 , 2 , 2 ) == ( Growth{ 1 , 2 , 1 , 0 , false } ) );
 std::map< std::uint64_t , double > M;
 for( int j = 0 ; j <= 5 ; ++j ) {
  const double k = std::ldexp( 1.0 , j );
  const std::uint64_t n = std::uint64_t( 1 ) << ( 1 << j );
  CHECK( std::abs( sqrtRec( n , M ) - k * k * ( j + 5 ) ) < 1e-6 );
  }
 for( std::uint64_t n = 4 ; n <= 2000000 ; n += 1 + n / 1000 ) {
  const double l = std::log2( double( n ) ) ,
               q = sqrtRec( n , M ) / ( l * l * std::log2( l ) );
  CHECK( ( q >= 1 ) && ( q <= 6 + 1e-9 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------- 16 - Ricorrenza 2T(n/8)+2T(n/4)+n --------------------*/
/*--------------------------------------------------------------------------*/


/// T( n ) = 2 T( n / 8 ) + 2 T( n / 4 ) + n for n > 1, 1 otherwise
std::vector< std::uint64_t > rec16( int N )
{
 std::vector< std::uint64_t > T( N + 1 , 1 );
 for( int n = 2 ; n <= N ; ++n )
  T[ n ] = 2 * T[ n / 8 ] + 2 * T[ n / 4 ] + std::uint64_t( n );
 return( T );
 }

// the bases T( n ) = n + 4 for 2 <= n <= 7, and n <= T( n ) <= 4 n, the
// constant of the substitution, up to 10^6: Theta( n )
ADS_TEST( "esercitazioni-16" )
{
 const int N = 1000000;
 const auto T = rec16( N );
 for( int n = 2 ; n <= 7 ; ++n ) CHECK( T[ n ] == std::uint64_t( n + 4 ) );
 for( int n = 1 ; n <= N ; ++n )
  CHECK( ( T[ n ] >= std::uint64_t( n ) ) &&
         ( T[ n ] <= 4 * std::uint64_t( n ) ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 17 - Ricorrenza T(n-1)+log n ----------------------*/
/*--------------------------------------------------------------------------*/


// T( n ) = T( n - 1 ) + log n = 1 + log2( n! ), against lgamma; T( 2 ) = 2
// is the base of the substitution, T( n ) <= n log n for n >= 2, and the
// last n / 2 terms give T( n ) >= ( n / 2 ) log( n / 2 ): Theta( n log n )
ADS_TEST( "esercitazioni-17" )
{
 double T = 1;
 for( int n = 2 ; n <= 1000000 ; ++n ) {
  T += std::log2( double( n ) );
  const double lg = 1 + std::lgamma( n + 1.0 ) / std::log( 2.0 ) ,
               nl = n * std::log2( double( n ) );
  CHECK( std::abs( T - lg ) <= 1e-9 * lg );
  CHECK( T <= nl + 1e-9 && T >= n / 2.0 * std::log2( n / 2.0 ) );
  if( n == 2 ) CHECK( std::abs( T - 2 ) < 1e-12 );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------- 18 - Ricorrenza 2T(n/2)+4T(n/4)+15T(n/8)+n^2 --------------*/
/*--------------------------------------------------------------------------*/


// T( n ) = 2 T( n / 2 ) + 4 T( n / 4 ) + 15 T( n / 8 ) + n^2 for n > 8, 1
// otherwise: 2 / 4 + 4 / 16 + 15 / 64 = 63 / 64 < 1, so n^2 <= T( n ) <=
// 64 n^2 for n > 8, checked up to 2 10^6: Theta( n^2 )
ADS_TEST( "esercitazioni-18" )
{
 const int N = 2000000;
 std::vector< std::uint64_t > T( N + 1 , 1 );
 for( int n = 9 ; n <= N ; ++n ) {
  const std::uint64_t n2 = std::uint64_t( n ) * std::uint64_t( n );
  T[ n ] = 2 * T[ n / 2 ] + 4 * T[ n / 4 ] + 15 * T[ n / 8 ] + n2;
  CHECK( ( T[ n ] >= n2 ) && ( T[ n ] <= 64 * n2 ) );
  }
 CHECK( 2 * 16 + 4 * 4 + 15 == 63 );
 }

/*--------------------------------------------------------------------------*/
/*---------- 19 - Analisi, algoritmo di selezione deterministico -----------*/
/*--------------------------------------------------------------------------*/


/// 5 T( n ) for T( n ) = T( n / 5 ) + T( 7 n / 10 ) + 11 / 5 n for n > 1, 1
/// otherwise: the integer U( n ) = U( n / 5 ) + U( 7 n / 10 ) + 11 n
std::vector< std::uint64_t > select5( int N )
{
 std::vector< std::uint64_t > U( N + 1 , 5 );
 for( int n = 2 ; n <= N ; ++n )
  U[ n ] = U[ n / 5 ] + U[ std::int64_t( 7 ) * n / 10 ] +
           11 * std::uint64_t( n );
 return( U );
 }

// the bases T( 2 ) = 32 / 5, T( 3 ) = 70 / 5, T( 4 ) = 81 / 5 of the text,
// and 11 / 5 n <= T( n ) <= 22 n for n >= 2, up to 10^6: Theta( n )
ADS_TEST( "esercitazioni-19" )
{
 const int N = 1000000;
 const auto U = select5( N );
 CHECK( U[ 2 ] == 32 && U[ 3 ] == 70 && U[ 4 ] == 81 );
 for( int n = 2 ; n <= N ; ++n )
  CHECK( ( U[ n ] >= 11 * std::uint64_t( n ) ) &&
         ( U[ n ] <= 5 * 22 * std::uint64_t( n ) ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 20 - SortinoSort (2010-09-12) ----------------------*/
/*--------------------------------------------------------------------------*/


/// the insertion sort of A[ i ... j ]
void insertionSort( std::vector< int > & A , int i , int j )
{
 for( int k = i + 1 ; k <= j ; ++k )
  for( int h = k ; ( h > i ) && ( A[ h - 1 ] > A[ h ] ) ; --h )
   std::swap( A[ h - 1 ] , A[ h ] );
 }

/// SortinoSort(): sorts A[ i ... j ] by sorting the first two thirds, the
/// last two thirds and the first two thirds again, s = ceil( n / 3 );
/// T( n ) = 3 T( 2 n / 3 ) + 1, Theta( n^( log_{3/2} 3 ) ) ~ n^2.71; the
/// calls are added to *calls
/// \pre 0 <= i and j < A.size()
void sortinoSort( std::vector< int > & A , int i , int j ,
                  long * calls = nullptr )
{
 ADS_PRE( ( 0 <= i ) && ( j < int( A.size() ) ) );
 if( calls ) ++*calls;
 if( j - i + 1 <= 6 ) {
  insertionSort( A , i , j );
  return;
  }
 const int s = ( j - i + 1 + 2 ) / 3;
 sortinoSort( A , i , i + 2 * s - 1 , calls );
 sortinoSort( A , i + s , j , calls );
 sortinoSort( A , i , i + 2 * s - 1 , calls );
 }

// against std::sort on random vectors with repetitions; the exponent of
// the calls between 2.5 and 2.9 from n = 100 to 1000, that of the master
// theorem about 2.71, worse than insertion sort
ADS_TEST( "esercitazioni-20-1" )
{
 std::mt19937 g( 20 );
 for( int r = 0 ; r < 400 ; ++r ) {
  auto A = rndVector( g , rnd( g , 0 , 60 ) , 0 , r % 2 ? 5 : 1000 );
  auto B = A;
  if( ! A.empty() ) sortinoSort( A , 0 , int( A.size() ) - 1 );
  std::ranges::sort( B );
  CHECK( A == B );
  }
 long c1 = 0 , c2 = 0;
 auto A = rndVector( g , 100 , 0 , 1000 ) , B = rndVector( g , 1000 , 0 , 9 );
 sortinoSort( A , 0 , 99 , &c1 );
 sortinoSort( B , 0 , 999 , &c2 );
 CHECK( std::ranges::is_sorted( A ) && std::ranges::is_sorted( B ) );
 const double e = std::log10( double( c2 ) / double( c1 ) ) ,
              p = master( 3 , 1.5 , 0 ).p;
 CHECK( ( e > 2.5 ) && ( e < 2.9 ) && ( p > 2.70 ) && ( p < 2.72 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 21 - Analisi, problema T(m)+T(n-m) -------------------*/
/*--------------------------------------------------------------------------*/


// T( n ) = T( m ) + T( n - m ) + 1 for n > m, 1 otherwise: n / m <= T( n )
// <= 2 n, the constants of the substitution, for m = 1 ... 5 and n up to
// 10^5: Theta( n )
ADS_TEST( "esercitazioni-21" )
{
 const int N = 100000;
 for( int m = 1 ; m <= 5 ; ++m ) {
  std::vector< long > T( N + 1 , 1 );
  for( int n = m + 1 ; n <= N ; ++n ) T[ n ] = T[ m ] + T[ n - m ] + 1;
  for( int n = 1 ; n <= N ; ++n )
   CHECK( ( m * T[ n ] >= n ) && ( T[ n ] <= 2L * n ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 22 - k-cammini (2018-07-25) -----------------------*/
/*--------------------------------------------------------------------------*/


/// the simple paths of k edges from path[ i ] = u, extended into out
template< Graph G >
void kPathsRec( const G & g , int k , int u , int i ,
                std::vector< int > & path ,
                std::vector< bool > & visited ,
                std::vector< std::vector< int > > & out )
{
 path[ i ] = u;
 if( i == k ) {
  out.push_back( path );
  return;
  }
 visited[ u ] = true;
 for( int v : g.adj( u ) )
  if( ! visited[ v ] )
   kPathsRec( g , k , v , i + 1 , path , visited , out );
 visited[ u ] = false;
 }

/// visit(): the simple paths (no repeated node) of exactly k edges from s,
/// by backtracking; O( n^k k ) time in the worst case, a complete graph
/// \pre s is a node and k >= 0
template< Graph G >
std::vector< std::vector< int > > kPaths( const G & g , int k , int s )
{
 ADS_PRE( g.hasNode( s ) && ( k >= 0 ) );
 std::vector< int > path( k + 1 );
 std::vector< bool > visited( g.n() , false );
 std::vector< std::vector< int > > out;
 kPathsRec( g , k , s , 0 , path , visited , out );
 return( out );
 }

// against all the sequences of k + 1 distinct nodes from s whose
// consecutive pairs are edges, on 300 random graphs (oriented and not);
// (n - 1)! / (n - k - 1)! paths in a complete graph; k = 0 gives ( s )
ADS_TEST( "esercitazioni-22" )
{
 std::mt19937 g( 22 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 1 , 6 ) , k = rnd( g , 0 , n ) ,
            s = rnd( g , 0 , n - 1 );
  const auto G = randomGraph( g , n , 0.5 , r % 2 );
  std::set< std::vector< int > > brute;
  std::vector< int > seq( k + 1 , 0 );
  for( long code = 0 , all = std::lround( std::pow( n , k ) ) ; code < all ;
       ++code ) {
   seq[ 0 ] = s;
   long c = code;
   for( int i = 1 ; i <= k ; ++i , c /= n ) seq[ i ] = int( c % n );
   bool ok = std::set< int >( seq.begin() , seq.end() ).size() ==
             std::size_t( k + 1 );
   for( int i = 0 ; ok && ( i < k ) ; ++i )
    ok = G.hasEdge( seq[ i ] , seq[ i + 1 ] );
   if( ok ) brute.insert( seq );
   }
  const auto P = kPaths( G , k , s );
  CHECK( std::set< std::vector< int > >( P.begin() , P.end() ) == brute &&
         P.size() == brute.size() );
  }
 const auto K = randomGraph( g , 6 , 1.0 , false );
 CHECK( kPaths( K , 3 , 0 ).size() == 5 * 4 * 3 );
 CHECK( kPaths( K , 0 , 2 ) ==
        std::vector< std::vector< int > >( 1 , std::vector< int >( 1 , 2 ) ) );
 CHECK( kPaths( K , 6 , 0 ).empty() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 25 - Tessere del domino -------------------------*/
/*--------------------------------------------------------------------------*/


/// the tilings of a 2 x n rectangle with 2 x 1 dominoes: the last column is
/// a vertical tile or the last two hold two horizontal ones, so Count( n ) =
/// Count( n - 1 ) + Count( n - 2 ), Count( 0 ) = Count( 1 ) = 1; Theta( n )
/// \pre 0 <= n <= 92
std::uint64_t dominoCount( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 92 ) );
 std::uint64_t a = 1 , b = 1;          // Count( i - 1 ) , Count( i )
 for( int i = 1 ; i < n ; ++i ) {
  const std::uint64_t c = a + b;
  a = b;
  b = c;
  }
 return( b );
 }

// n = 4 has 5 tilings; Count( n ) is F_n of the library (F_0 = F_1 = 1),
// up to the largest n whose count fits 64 bits
ADS_TEST( "esercitazioni-25-1" )
{
 CHECK( dominoCount( 4 ) == 5 && dominoCount( 0 ) == 1 &&
        dominoCount( 1 ) == 1 );
 for( int n = 0 ; n <= 92 ; ++n ) CHECK( dominoCount( n ) == fibonacci( n ) );
 CHECK_THROWS( dominoCount( 93 ) , contract_violation );
 }

using Tiling = std::array< std::vector< int > , 2 >;

/// printDominoRec(): the tilings of the first i columns of S, into out
void printDominoRec( Tiling & S , int i , std::vector< Tiling > & out )
{
 if( i == 0 ) {
  out.push_back( S );
  return;
  }
 S[ 0 ][ i - 1 ] = S[ 1 ][ i - 1 ] = i;              // a vertical tile
 printDominoRec( S , i - 1 , out );
 if( i >= 2 ) {                                     // two horizontal tiles
  S[ 0 ][ i - 2 ] = S[ 0 ][ i - 1 ] = i;
  S[ 1 ][ i - 2 ] = S[ 1 ][ i - 1 ] = i - 1;
  printDominoRec( S , i - 2 , out );
  }
 }

/// printDomino(): every tiling of a 2 x n rectangle as a 2 x n matrix, the
/// two cells of a tile holding the same number; Theta( n phi^n )
/// \pre n >= 0
std::vector< Tiling > printDomino( int n )
{
 ADS_PRE( n >= 0 );
 Tiling S{ std::vector< int >( n ) , std::vector< int >( n ) };
 std::vector< Tiling > out;
 printDominoRec( S , n , out );
 return( out );
 }

// for n = 0 ... 16, Count( n ) distinct tilings, each number on exactly two
// adjacent cells; the five tilings of n = 4
ADS_TEST( "esercitazioni-25-2" )
{
 for( int n = 0 ; n <= 16 ; ++n ) {
  const auto P = printDomino( n );
  CHECK( P.size() == dominoCount( n ) );
  CHECK( std::set< Tiling >( P.begin() , P.end() ).size() == P.size() );
  for( const auto & S : P ) {
   std::map< int , std::vector< std::pair< int , int > > > cells;
   for( int r = 0 ; r < 2 ; ++r )
    for( int c = 0 ; c < n ; ++c ) cells[ S[ r ][ c ] ].emplace_back( r , c );
   for( const auto & [ v , C ] : cells )
    CHECK( C.size() == 2 && std::abs( C[ 0 ].first - C[ 1 ].first ) +
                            std::abs( C[ 0 ].second - C[ 1 ].second ) == 1 );
   }
  }
 CHECK( printDomino( 4 ).size() == 5 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- 26 - Zaino -------------------------------*/
/*--------------------------------------------------------------------------*/


/// knapsackRec(): the subsets that extend S[ 0 ... i - 1 ] within c
void knapsackRec( const std::vector< int > & w , std::vector< bool > & S ,
                  int i , int c , std::vector< std::vector< bool > > & out )
{
 if( i == int( w.size() ) ) {
  out.push_back( S );
  return;
  }
 S[ i ] = false;
 knapsackRec( w , S , i + 1 , c , out );
 if( w[ i ] <= c ) {
  S[ i ] = true;
  knapsackRec( w , S , i + 1 , c - w[ i ] , out );
  S[ i ] = false;
  }
 }

/// every subset of the items, as a vector of flags, of total weight at
/// most C, by backtracking that prunes the items that do not fit;
/// O( 2^n n ) time
/// \pre n >= 1, weights >= 0, C >= 0
std::vector< std::vector< bool > > knapsackAll( const std::vector< int > & w ,
                                                int C )
{
 ADS_PRE( ! w.empty() && ( C >= 0 ) &&
          std::ranges::all_of( w , []( int x ) { return( x >= 0 ); } ) );
 std::vector< bool > S( w.size() , false );
 std::vector< std::vector< bool > > out;
 knapsackRec( w , S , 0 , C , out );
 return( out );
 }

// against the filter of all the 2^n subsets on 400 random instances; one
// item that does not fit (only the empty set), all the items of weight 0
ADS_TEST( "esercitazioni-26" )
{
 std::mt19937 g( 26 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const int n = rnd( g , 1 , 10 ) , C = rnd( g , 0 , 30 );
  const auto w = rndVector( g , n , 0 , 12 );
  std::set< std::vector< bool > > brute;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   std::vector< bool > S( n );
   int s = 0;
   for( int i = 0 ; i < n ; ++i )
    if( ( S[ i ] = ( mask >> i ) & 1 ) ) s += w[ i ];
   if( s <= C ) brute.insert( S );
   }
  const auto all = knapsackAll( w , C );
  CHECK( std::set< std::vector< bool > >( all.begin() , all.end() ) == brute &&
         all.size() == brute.size() );
  }
 CHECK( knapsackAll( { 5 } , 4 ).size() == 1 );
 CHECK( knapsackAll( { 0 , 0 , 0 } , 0 ).size() == 8 );
 CHECK_THROWS( knapsackAll( {} , 3 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------- 27 - Per fare un albero (binario di ricerca) ci vuole... --------*/
/*--------------------------------------------------------------------------*/


/// the search tree of A[ i ... j - 1 ] rooted at its middle item
BT * minHeightRec( const std::vector< int > & A , int i , int j )
{
 if( i >= j ) return( nullptr );
 const int m = ( i + j - 1 ) / 2;
 return( mk( A[ m ] , minHeightRec( A , i , m ) ,
             minHeightRec( A , m + 1 , j ) ) );
 }

// candidate for the library
/// buildMinHeightBST(): the search tree of minimum height of the strictly
/// increasing A, the middle item at the root of each subtree; T( n ) =
/// T( floor( ( n - 1 ) / 2 ) ) + T( ceil( ( n - 1 ) / 2 ) ) + 1 = Theta( n );
/// nil if empty; the caller owns the result
/// \pre A strictly increasing
/// \post height ceil( log2( n + 1 ) ) - 1
BT * buildMinHeightBST( const std::vector< int > & A )
{
 ADS_PRE( std::ranges::adjacent_find( A , std::greater_equal<>() ) ==
          A.end() );
 BT * t = minHeightRec( A , 0 , int( A.size() ) );
 ADS_POST( heightOf( t ) == int( std::bit_width( A.size() ) ) - 1 );
 return( t );
 }

// (also exercise 75.2) for n = 0 ... 300: symmetric visit A, height
// ceil( log2( n + 1 ) ) - 1, on random strictly increasing vectors
ADS_TEST( "esercitazioni-27" )
{
 std::mt19937 g( 27 );
 for( int n = 0 ; n <= 300 ; ++n ) {
  std::vector< int > A( n );
  for( int i = 0 , v = -50 ; i < n ; ++i ) A[ i ] = v += rnd( g , 1 , 9 );
  Owner T( buildMinHeightBST( A ) );
  CHECK( inorder( T.get() ) == A );
  CHECK( heightOf( T.get() ) ==
         int( std::ceil( std::log2( n + 1.0 ) - 1e-12 ) ) - 1 );
  }
 Owner one( buildMinHeightBST( { 4 } ) );
 CHECK( one->read() == 4 && ! one->left() && ! one->right() );
 CHECK_THROWS( buildMinHeightBST( { 1 , 1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- 28 - Vicini-vicini ---------------------------*/
/*--------------------------------------------------------------------------*/


/// a point of the plane
struct Point {
 std::int64_t x , y;
 };

/// the square of the distance of p and q
std::int64_t dist2( const Point & p , const Point & q )
{
 return( ( p.x - q.x ) * ( p.x - q.x ) + ( p.y - q.y ) * ( p.y - q.y ) );
 }

/// the closest pair among P[ I[ lo ... hi - 1 ] ], sorted by x; leaves the
/// range sorted by y
void closestRec( const std::vector< Point > & P , std::vector< int > & I ,
                 int lo , int hi , std::int64_t & best ,
                 std::pair< int , int > & pair )
{
 const auto byY = [ & ]( int a , int b ) { return( P[ a ].y < P[ b ].y ); };
 if( hi - lo <= 3 ) {
  for( int i = lo ; i < hi ; ++i )
   for( int j = i + 1 ; j < hi ; ++j )
    if( const auto d = dist2( P[ I[ i ] ] , P[ I[ j ] ] ) ; d < best ) {
     best = d;
     pair = std::pair( I[ i ] , I[ j ] );
     }
  std::sort( I.begin() + lo , I.begin() + hi , byY );
  return;
  }
 const int mid = ( lo + hi ) / 2;
 const std::int64_t xm = P[ I[ mid ] ].x;
 closestRec( P , I , lo , mid , best , pair );
 closestRec( P , I , mid , hi , best , pair );
 std::inplace_merge( I.begin() + lo , I.begin() + mid , I.begin() + hi , byY );
 std::vector< int > strip;                   // within sqrt( best ) of xm
 for( int i = lo ; i < hi ; ++i )
  if( ( P[ I[ i ] ].x - xm ) * ( P[ I[ i ] ].x - xm ) < best )
   strip.push_back( I[ i ] );
 for( std::size_t i = 0 ; i < strip.size() ; ++i )
  for( std::size_t j = i + 1 ; ( j < strip.size() ) &&
       ( ( P[ strip[ j ] ].y - P[ strip[ i ] ].y ) *
         ( P[ strip[ j ] ].y - P[ strip[ i ] ].y ) < best ) ; ++j )
   if( const auto d = dist2( P[ strip[ i ] ] , P[ strip[ j ] ] ) ; d < best ) {
    best = d;
    pair = std::pair( strip[ i ] , strip[ j ] );
    }
 }

// candidate for the library
/// the closest pair of points, by divide et impera: split at the median
/// x, solve the halves, then compare each point of the strip of width
/// delta about the split with its next ones by y, at most 7 of them;
/// O( n log n ); the places of the pair and the square of its distance
/// \pre n >= 2 and |x|, |y| <= 10^9
std::pair< std::pair< int , int > , std::int64_t > closestPair(
                                              const std::vector< Point > & P )
{
 ADS_PRE( P.size() >= 2 );
 for( const auto & p : P )
  ADS_PRE( ( std::abs( p.x ) <= 1000000000 ) &&
           ( std::abs( p.y ) <= 1000000000 ) );
 std::vector< int > I( P.size() );
 std::iota( I.begin() , I.end() , 0 );
 std::ranges::sort( I , [ & ]( int a , int b ) {
  return( P[ a ].x < P[ b ].x ); } );
 std::int64_t best = std::numeric_limits< std::int64_t >::max();
 std::pair< int , int > pair( -1 , -1 );
 closestRec( P , I , 0 , int( I.size() ) , best , pair );
 ADS_POST( dist2( P[ pair.first ] , P[ pair.second ] ) == best );
 return( std::pair( pair , best ) );
 }

// against the quadratic minimum on 500 random point sets, small coordinates
// (repeated points, distance 0) and large ones, collinear points; two points
ADS_TEST( "esercitazioni-28" )
{
 std::mt19937 g( 28 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 2 , 120 ) ,
            M = r % 3 == 0 ? 10 : ( r % 3 == 1 ? 1000000000 : 1000 );
  std::vector< Point > P( n );
  for( auto & p : P ) p = Point{ rnd( g , -M , M ) , r % 7 ? rnd( g , -M , M )
                                                          : 0 };
  std::int64_t best = std::numeric_limits< std::int64_t >::max();
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 1 ; j < n ; ++j ) best = std::min( best , dist2( P[ i ] ,
                                                                P[ j ] ) );
  const auto [ pq , d ] = closestPair( P );
  CHECK( d == best && pq.first != pq.second );
  }
 CHECK( closestPair( { Point{ 0 , 0 } , Point{ 3 , 4 } } ).second == 25 );
 CHECK_THROWS( closestPair( { Point{ 0 , 0 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 29 - Esercizi divide-et-impera ---------------------*/
/*--------------------------------------------------------------------------*/


/// fixedPoint(): whether some A[ i ] = i, for A sorted and distinct, by a
/// binary search on the sign of A[ m ] - m, which does not decrease;
/// O( log n )
/// \pre A strictly increasing
bool fixedPoint( const std::vector< int > & A )
{
 ADS_PRE( std::ranges::adjacent_find( A , std::greater_equal<>() ) ==
          A.end() );
 int i = 0 , j = int( A.size() ) - 1;
 while( i <= j ) {
  const int m = ( i + j ) / 2;
  if( A[ m ] == m ) return( true );
  if( A[ m ] < m ) i = m + 1;
  else j = m - 1;
  }
 return( false );
 }

// against a linear scan on 1000 random strictly increasing vectors; the
// empty vector; one item
ADS_TEST( "esercitazioni-29-1" )
{
 std::mt19937 g( 291 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  std::vector< int > A( rnd( g , 0 , 30 ) );
  for( int i = 0 , v = rnd( g , -20 , 5 ) ; i < int( A.size() ) ; ++i )
   A[ i ] = v += rnd( g , 1 , 3 );
  bool lin = false;
  for( int i = 0 ; i < int( A.size() ) ; ++i ) lin = lin || ( A[ i ] == i );
  CHECK( fixedPoint( A ) == lin );
  }
 CHECK( ! fixedPoint( {} ) && fixedPoint( { 0 } ) && ! fixedPoint( { 1 } ) );
 }

/// missing(): the value of 0 ... n missing from the n sorted distinct
/// values of A, the first place i with A[ i ] != i, by a binary search with
/// the sentinel n; O( log n )
/// \pre A strictly increasing, its values in [ 0 , n ]
int missing( const std::vector< int > & A )
{
 const int n = int( A.size() );
 ADS_PRE( std::ranges::adjacent_find( A , std::greater_equal<>() ) ==
          A.end() );
 ADS_PRE( n == 0 || ( ( A[ 0 ] >= 0 ) && ( A[ n - 1 ] <= n ) ) );
 int i = 0 , j = n;                 // A[ x ] = x before i, the answer <= j
 while( i < j ) {
  const int m = ( i + j ) / 2;
  if( A[ m ] == m ) i = m + 1;
  else j = m;
  }
 return( i );
 }

// against n( n + 1 ) / 2 minus the sum, for each n <= 60 and each missing
// value
ADS_TEST( "esercitazioni-29-2" )
{
 for( int n = 0 ; n <= 60 ; ++n )
  for( int x = 0 ; x <= n ; ++x ) {
   std::vector< int > A;
   for( int v = 0 ; v <= n ; ++v ) if( v != x ) A.push_back( v );
   CHECK( missing( A ) == n * ( n + 1 ) / 2 -
                          std::accumulate( A.begin() , A.end() , 0 ) );
   CHECK( missing( A ) == x );
   }
 CHECK_THROWS( missing( { 0 , 5 } ) , contract_violation );
 }

/// unimodalMinimum(): the minimum of A, strictly decreasing and then
/// strictly increasing, by a binary search on the sign of A[ m + 1 ] -
/// A[ m ]; O( log n )
/// \pre A unimodal, n >= 1
int unimodalMinimum( const std::vector< int > & A )
{
 ADS_PRE( ! A.empty() );
 int i = 0 , j = int( A.size() ) - 1;
 while( i < j ) {
  const int m = ( i + j ) / 2;
  if( A[ m ] < A[ m + 1 ] ) j = m;            // increasing: left, m included
  else i = m + 1;
  }
 return( A[ i ] );
 }

// against std::min_element on 1000 random unimodal vectors, among them
// the monotone ones; one item
ADS_TEST( "esercitazioni-29-3" )
{
 std::mt19937 g( 293 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  std::set< int > V;
  const int n = rnd( g , 1 , 40 );
  while( int( V.size() ) < n ) V.insert( rnd( g , -500 , 500 ) );
  std::vector< int > S( V.begin() , V.end() ) , A;
  std::vector< int > down , up;
  for( int i = 1 ; i < n ; ++i ) ( rnd( g , 0 , 1 ) ? down : up ).push_back(
                                                                   S[ i ] );
  A.assign( down.rbegin() , down.rend() );
  A.push_back( S[ 0 ] );
  A.insert( A.end() , up.begin() , up.end() );
  CHECK( unimodalMinimum( A ) == *std::ranges::min_element( A ) );
  }
 CHECK( unimodalMinimum( { 3 } ) == 3 );
 CHECK( unimodalMinimum( { 1 , 2 , 3 } ) == 1 );
 CHECK( unimodalMinimum( { 3 , 2 , 1 } ) == 1 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- 30 - Doppio mediano ---------------------------*/
/*--------------------------------------------------------------------------*/


/// the two middle values of X[ xs ... xe ] and Y[ ys ... ye ], four in all
std::pair< int , int > medianOfFour( const std::vector< int > & X ,
                                     const std::vector< int > & Y , int xs ,
                                     int xe , int ys , int ye )
{
 std::array< int , 4 > v{ X[ xs ] , X[ xe ] , Y[ ys ] , Y[ ye ] };
 std::ranges::sort( v );
 return( std::pair( v[ 1 ] , v[ 2 ] ) );
 }

/// the two medians of X[ xs ... xe ] and Y[ ys ... ye ], of equal lengths
std::pair< int , int > doubleMedianRec( const std::vector< int > & X ,
                                        const std::vector< int > & Y ,
                                        int xs , int xe , int ys , int ye )
{
 if( xe - xs == 1 ) return( medianOfFour( X , Y , xs , xe , ys , ye ) );
 const int xm = ( xs + xe ) / 2 , ym = ( ys + ye + 1 ) / 2;
 if( X[ xm ] < Y[ ym ] )
  return( doubleMedianRec( X , Y , xm , xe , ys , ym ) );
 if( Y[ ym ] < X[ xm ] )
  return( doubleMedianRec( X , Y , xs , xm , ym , ye ) );
 return( std::pair( X[ xm ] , Y[ ym ] ) );
 }

// candidate for the library
/// doubleMedian(): the two medians, of ranks n and n + 1, of the 2 n values
/// of the sorted X and Y together: the lower half of the one with the
/// smaller middle and the upper half of the other are discarded, the same
/// number from each; O( log n )
/// \pre X and Y sorted, of the same size n >= 2
std::pair< int , int > doubleMedian( const std::vector< int > & X ,
                                     const std::vector< int > & Y )
{
 const int n = int( X.size() );
 ADS_PRE( ( n >= 2 ) && ( int( Y.size() ) == n ) &&
          std::ranges::is_sorted( X ) && std::ranges::is_sorted( Y ) );
 return( doubleMedianRec( X , Y , 0 , n - 1 , 0 , n - 1 ) );
 }

// against the places n - 1 and n of the merge of X and Y, on 2000 random
// pairs with and without repeated values; n = 2
ADS_TEST( "esercitazioni-30" )
{
 std::mt19937 g( 30 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const int n = rnd( g , 2 , 40 ) , M = r % 2 ? 5 : 1000;
  auto X = rndVector( g , n , 0 , M ) , Y = rndVector( g , n , 0 , M );
  std::ranges::sort( X );
  std::ranges::sort( Y );
  std::vector< int > Z( 2 * n );
  std::merge( X.begin() , X.end() , Y.begin() , Y.end() , Z.begin() );
  CHECK( doubleMedian( X , Y ) == std::pair( Z[ n - 1 ] , Z[ n ] ) );
  }
 CHECK( doubleMedian( { 1 , 2 } , { 3 , 4 } ) == std::pair( 2 , 3 ) );
 CHECK_THROWS( doubleMedian( { 1 } , { 2 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- 31 - Gap --------------------------------*/
/*--------------------------------------------------------------------------*/


/// gap(): a gap of L[ i ... j ], a place g with L[ g - 1 ] < L[ g ], by
/// halving with the invariant L[ i ] < L[ j ]; O( log n )
int gapRec( const std::vector< int > & L , int i , int j )
{
 if( j == i + 1 ) return( j );
 const int m = ( i + j ) / 2;
 if( L[ m ] <= L[ i ] ) return( gapRec( L , m , j ) );
 return( gapRec( L , i , m ) );
 }

/// a gap of L, a place 0 < g < n with L[ g - 1 ] < L[ g ]
/// \pre n >= 2 and L[ 0 ] < L[ n - 1 ]
/// \post L[ g - 1 ] < L[ g ]
int gap( const std::vector< int > & L )
{
 ADS_PRE( ( L.size() >= 2 ) && ( L.front() < L.back() ) );
 const int g = gapRec( L , 0 , int( L.size() ) - 1 );
 ADS_POST( ( g > 0 ) && ( L[ g - 1 ] < L[ g ] ) );
 return( g );
 }

// the gap checked on 2000 random lists with L[ 0 ] < L[ n - 1 ], many of
// them with one gap only; two items
ADS_TEST( "esercitazioni-31-2" )
{
 std::mt19937 g( 31 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const int n = rnd( g , 2 , 50 );
  auto L = rndVector( g , n , 0 , r % 2 ? 3 : 100 );
  if( r % 5 == 0 ) {                     // non-increasing but one jump
   std::ranges::sort( L , std::greater<>() );
   const int k = rnd( g , 1 , n - 1 );
   for( int i = k ; i < n ; ++i ) L[ i ] += 1000;
   }
  L.front() = std::min( L.front() , L.back() - 1 );
  const int x = gap( L );
  CHECK( ( x > 0 ) && ( x < n ) && ( L[ x - 1 ] < L[ x ] ) );
  }
 CHECK( gap( { 1 , 2 } ) == 1 );
 CHECK_THROWS( gap( { 2 , 2 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- 33 - Samarcanda -----------------------------*/
/*--------------------------------------------------------------------------*/


/// the minimum, the maximum and the best gain of P[ s ... e ]
std::array< long , 3 > bestGainRec( const std::vector< int > & P , int s ,
                                    int e )
{
 if( s == e )
  return( std::array< long , 3 >{ P[ s ] , P[ s ] ,
                                  std::numeric_limits< long >::min() } );
 const int m = ( s + e ) / 2;
 const auto L = bestGainRec( P , s , m ) , R = bestGainRec( P , m + 1 , e );
 return( std::array< long , 3 >{ std::min( L[ 0 ] , R[ 0 ] ) ,
                                 std::max( L[ 1 ] , R[ 1 ] ) ,
                                 std::max( { L[ 2 ] , R[ 2 ] ,
                                             R[ 1 ] - L[ 0 ] } ) } );
 }

/// bestGain(): the largest P[ y ] - P[ x ] with x < y, by divide et impera
/// returning the minimum, the maximum and the best gain of each half: the
/// best pair across buys at the left minimum and sells at the right
/// maximum; T( n ) = 2 T( n / 2 ) + Theta( 1 ) = Theta( n )
/// \pre n >= 2
long bestGain( const std::vector< int > & P )
{
 ADS_PRE( P.size() >= 2 );
 return( bestGainRec( P , 0 , int( P.size() ) - 1 )[ 2 ] );
 }

// against all the pairs on 1000 random vectors, decreasing ones (negative
// gain), all equal (0); two items
ADS_TEST( "esercitazioni-33" )
{
 std::mt19937 g( 33 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  auto P = rndVector( g , rnd( g , 2 , 40 ) , -1000 , 1000 );
  if( r % 10 == 0 ) std::ranges::sort( P , std::greater<>() );
  long best = std::numeric_limits< long >::min();
  for( int x = 0 ; x < int( P.size() ) ; ++x )
   for( int y = x + 1 ; y < int( P.size() ) ; ++y )
    best = std::max( best , long( P[ y ] ) - P[ x ] );
  CHECK( bestGain( P ) == best );
  }
 CHECK( bestGain( { 5 , 5 , 5 } ) == 0 && bestGain( { 3 , 1 } ) == -2 );
 CHECK_THROWS( bestGain( { 1 } ) , contract_violation );
 }

}  // end( namespace esercitazioni )

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File exam_esercitazionia.cpp --------------------*/
/*--------------------------------------------------------------------------*/
