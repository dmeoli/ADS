/*--------------------------------------------------------------------------*/
/*--------------------------- File ods_trees.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the skip lists, the binary trees, the random binary search
 * trees, the scapegoat trees and the red-black trees of chapters 4 and 6
 * to 9 of P. Morin, Open Data Structures (in C++), with their exercises:
 * each test is named after the section ("ods4.2") or the exercise
 * ("ods-ex4.9") of the book it checks, and the structures are compared
 * with the containers of the standard library on random operations, at
 * the level 3 of the contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ #include "ADS/dictionary/IndexedSkipList.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/sequence/SkipListSequence.h"
#include "ADS/set/SpaceEfficientSortedSet.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/MeasuredSearchTree.h" ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/dictionary/IndexedSkipList.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/sequence/SkipListSequence.h"
#include "ADS/set/SpaceEfficientSortedSet.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/MeasuredSearchTree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <numeric>
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

namespace {

/// the pairs of a dictionary by `forEach()`
template< class D >
std::vector< std::pair< int , int > > pairs( const D & d )
{
 std::vector< std::pair< int , int > > P;
 d.forEach( [ & ]( const int & k , const int & v ) { P.emplace_back( k , v ); } );
 return( P );
 }

/// the pairs of a map
std::vector< std::pair< int , int > > pairs( const std::map< int , int > & m )
{
 return( std::vector< std::pair< int , int > >( m.begin() , m.end() ) );
 }

/// random insertions, lookups and removals on `d` and on a `std::map`, with
/// the keys in [ 0 , m ), checked against each other every `every` steps
template< class D >
void differential( D & d , int steps , int m , unsigned seed , int every = 97 )
{
 std::mt19937 g( seed );
 std::map< int , int > ref;
 for( int s = 0 ; s < steps ; ++s ) {
  const int k = int( g() % unsigned( m ) ) , op = int( g() % 3 );
  if( op == 0 ) {
   d.insert( k , s );
   ref[ k ] = s;
   }
  else if( op == 1 ) {
   d.remove( k );
   ref.erase( k );
   }
  else {
   const auto it = ref.find( k );
   const auto v = d.lookup( k );
   CHECK( ( it == ref.end() ) ? ! v.has_value()
                              : ( v.has_value() && ( *v == it->second ) ) );
   }
  CHECK( d.size() == ref.size() );
  if( s % every == 0 )
   CHECK( pairs( d ) == pairs( ref ) );
  }
 CHECK( pairs( d ) == pairs( ref ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 4 ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a key that records its comparisons with a key searched (`q` true): the
/// other key, doubled, plus 1 when it is on the right of `<`
struct Counted {
 int k;
 bool q = false;
 static inline std::vector< int > seen;
 friend bool operator<( const Counted & a , const Counted & b ) {
  seen.push_back( a.q ? 2 * b.k + 1 : 2 * a.k );
  return( a.k < b.k );
  }
 friend bool operator==( const Counted & a , const Counted & b ) {
  return( a.k == b.k );
  }
 friend auto operator<=>( const Counted & a , const Counted & b ) {
  return( a.k <=> b.k );
  }
 };

ADS_TEST( "ods4.2.skiplist-sset" )
{
 SkipList< int , int > S( 7 );
 differential( S , 20000 , 500 , 11 );
 SkipList< int , int > T( 3 );
 std::set< int > ref;
 std::mt19937 g( 5 );
 for( int i = 0 ; i < 2000 ; ++i ) {
  const int k = int( g() % 10000 );
  T.insert( k , -k );
  ref.insert( k );
  }
 for( int x = -5 ; x < 10010 ; x += 7 ) {   // find(x) of an SSet
  const auto it = ref.lower_bound( x );
  const auto lb = T.lowerBound( x );
  CHECK( ( it == ref.end() ) ? ( lb == T.end() )
                             : ( ( lb != T.end() ) && ( ( *lb ).first == *it )
                                 && ( ( *lb ).second == -*it ) ) );
  }
 SkipList< int , int > M( std::move( T ) );
 CHECK( ( M.size() == ref.size() ) && T.empty() && ( T.begin() == T.end() ) );
 }

ADS_TEST( "ods-ex4.8.no-redundant-comparisons" )
{
 // the search compares the key once with each node
 SkipList< Counted , int > S( 9 );
 for( int i = 0 ; i < 3000 ; ++i ) S.insert( Counted{ 2 * i } , i );
 std::size_t total = 0 , searches = 0;
 for( int x = -1 ; x < 6002 ; x += 3 ) {
  Counted::seen.clear();
  ( void ) S.lookup( Counted{ x , true } );
  std::vector< int > s = Counted::seen;
  std::sort( s.begin() , s.end() );
  CHECK( std::adjacent_find( s.begin() , s.end() ) == s.end() );
  CHECK( s.size() < 200 );
  total += s.size();
  ++searches;
  }
 // about 1.5 log2 n comparisons on average, against 2 log2 n of the plain
 // search (see the theory of the dictionaries)
 const double avg = double( total ) / double( searches ) ,
              lg = std::log2( 3000.0 );
 std::printf( "comparisons per search %.2f, log2 n %.2f\n" , avg , lg );
 CHECK( ( avg > 1.2 * lg ) && ( avg < 1.8 * lg + 2 ) );
 }

ADS_TEST( "ods-ex4.10.finger-search" )
{
 SkipList< int , int > S( 4 );
 for( int i = 0 ; i < 5000 ; ++i ) S.insert( 3 * i , i );
 std::mt19937 g( 1 );
 for( int s = 0 ; s < 20000 ; ++s ) {
  const int x = ( s % 2 ) ? int( g() % 15010 ) : ( s % 15000 );   // near
  const auto v = S.fingerLookup( x );
  CHECK( ( x % 3 == 0 && x < 15000 ) ? ( v && ( *v == x / 3 ) )
                                     : ! v.has_value() );
  if( s % 1000 == 0 ) {                     // the finger survives a change
   S.insert( 3 * ( s % 4000 ) , s % 4000 );
   S.remove( 1 );
   }
  }
 }

ADS_TEST( "ods4.3.skiplist-list" )
{
 SkipListSequence< int > L( 5 );
 std::vector< int > ref;
 std::mt19937 g( 2 );
 for( int s = 0 ; s < 20000 ; ++s ) {
  const int op = int( g() % 4 );
  if( ( op <= 1 ) || ref.empty() ) {
   const std::size_t i = g() % ( ref.size() + 1 );
   L.add( i , s );
   ref.insert( ref.begin() + std::ptrdiff_t( i ) , s );
   }
  else if( op == 2 ) {
   const std::size_t i = g() % ref.size();
   CHECK( L.remove( i ) == ref[ i ] );
   ref.erase( ref.begin() + std::ptrdiff_t( i ) );
   }
  else {
   const std::size_t i = g() % ref.size();
   CHECK( L.get( i ) == ref[ i ] );
   L.set( i , -s );
   ref[ i ] = -s;
   }
  CHECK( L.size() == ref.size() );
  }
 CHECK( std::vector< int >( L.begin() , L.end() ) == ref );
 for( int & x : L ) x += 1;
 for( int & x : ref ) x += 1;
 CHECK( std::equal( L.cbegin() , L.cend() , ref.begin() , ref.end() ) );
 CHECK_THROWS( L.get( ref.size() ) , contract_violation );
 }

ADS_TEST( "ods-ex4.11.truncate-absorb" )
{
 // exercises 4.11 and 4.12: cut at every place and glue back
 std::mt19937 g( 3 );
 for( int n : { 0 , 1 , 2 , 7 , 100 , 1000 } ) {
  SkipListSequence< int > L( unsigned( n ) + 1 );
  for( int i = 0 ; i < n ; ++i ) L.push_back( i );
  for( int c = 0 ; c < 20 ; ++c ) {
   const std::size_t i = g() % std::size_t( n + 1 );
   SkipListSequence< int > R = L.truncate( i );
   CHECK( ( L.size() == i ) && ( R.size() == std::size_t( n ) - i ) );
   for( std::size_t j = 0 ; j < R.size() ; ++j )
    CHECK( R.get( j ) == int( i + j ) );
   L.absorb( R );
   CHECK( R.empty() && ( L.size() == std::size_t( n ) ) );
   for( int j = 0 ; j < n ; ++j ) CHECK( L.get( std::size_t( j ) ) == j );
   }
  SkipListSequence< int > E;
  L.absorb( E );
  E.absorb( L );
  CHECK( L.empty() && ( E.size() == std::size_t( n ) ) );
  E.push_back( n );
  CHECK( E.get( std::size_t( n ) ) == n );
  }
 }

ADS_TEST( "ods-ex4.9.rank-skiplist" )
{
 IndexedSkipList< int , int > S( 8 );
 differential( S , 20000 , 700 , 13 );
 std::set< int > ref;
 for( const auto & [ k , v ] : S ) ref.insert( k );
 std::size_t r = 0;
 for( int k : ref ) {                       // get(i) and the ranks
  CHECK( ( *S.select( r ) ).first == k );
  CHECK( S.rank( k ) == r );
  ++r;
  }
 for( int x = -3 ; x < 705 ; ++x ) {
  const auto it = ref.lower_bound( x );
  const auto lb = S.lowerBound( x );
  CHECK( ( it == ref.end() ) ? ( lb == S.end() ) : ( ( *lb ).first == *it ) );
  CHECK( S.rank( x ) == std::size_t( std::distance( ref.begin() , it ) ) );
  }
 CHECK_THROWS( ( void ) S.select( S.size() ) , contract_violation );
 }

ADS_TEST( "ods-ex4.13.sesset" )
{
 for( std::size_t b : { 3u , 4u , 8u , 33u } ) {
  SpaceEfficientSortedSet< int > S( b , b );
  std::set< int > ref;
  std::mt19937 g( static_cast< unsigned >( b ) );
  for( int s = 0 ; s < 30000 ; ++s ) {
   const int x = int( g() % 4000 );
   if( g() % 5 < 3 ) {
    S.insert( x );
    ref.insert( x );
    }
   else {
    S.remove( x );
    ref.erase( x );
    }
   CHECK( S.size() == ref.size() );
   const auto it = ref.lower_bound( x / 2 );
   const auto lb = S.lowerBound( x / 2 );
   CHECK( ( it == ref.end() ) ? ! lb : ( lb && ( *lb == *it ) ) );
   // n items and O( n / b + b ) places wasted
   CHECK( S.capacity() - S.size() <=
          2 * ( S.size() / ( b - 1 ) + 1 ) + b + 1 );
   }
  std::vector< int > V;
  S.forEach( [ & ]( int x ) { V.push_back( x ); } );
  CHECK( V == std::vector< int >( ref.begin() , ref.end() ) );
  for( int x : V ) S.remove( x );
  CHECK( S.empty() && ( S.capacity() == 0 ) );
  }
 CHECK_THROWS( SpaceEfficientSortedSet< int >( 2 ) , contract_violation );
 }

ADS_TEST( "ods-ex4.14.substring-search" )
{
 // every substring is a prefix of a suffix: the suffixes, as views of the
 // text, in an SSet, and a query answered by find( q ) as it is typed
 const std::string text =
  "it was the best of times it was the worst of times it was the age of "
  "wisdom it was the age of foolishness it was the epoch of belief";
 SkipList< std::string_view , std::size_t > S( 3 );
 for( std::size_t i = 0 ; i < text.size() ; ++i )
  S.insert( std::string_view( text ).substr( i ) , i );
 for( const std::string q : { "the w" , "age of f" , "epoch" , "times it" ,
                              "zz" , "wisdom it was the agf" , "" } ) {
  std::string typed;
  for( char c : q ) {                       // a match after each keystroke
   typed += c;
   const auto it = S.lowerBound( typed );
   const bool found = ( it != S.end() ) && ( *it ).first.starts_with( typed );
   CHECK( found == ( text.find( typed ) != std::string::npos ) );
   if( found ) CHECK( text.compare( ( *it ).second , typed.size() , typed )
                      == 0 );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 6 ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a random binary tree of n nodes, its values 0 .. n - 1 in the order of
/// creation; the caller owns it
BinaryTree< int > * randomTree( int n , unsigned seed )
{
 std::mt19937 g( seed );
 std::vector< BinaryTree< int > * > N;
 for( int i = 0 ; i < n ; ++i ) {
  auto * u = new BinaryTree< int >( i );
  if( i > 0 )
   for( ; ; ) {                             // a free place below a node
    BinaryTree< int > * p = N[ g() % N.size() ];
    if( ( g() % 2 ) && ! p->left() ) { p->insertLeft( u ); break; }
    if( ! p->right() ) { p->insertRight( u ); break; }
    }
  N.push_back( u );
  }
 return( n ? N[ 0 ] : nullptr );
 }

/// the size and the height of a subtree, by recursion
std::pair< std::size_t , long > measure( const BinaryTree< int > * t )
{
 using P = std::pair< std::size_t , long >;
 if( ! t ) return( P( 0 , -1 ) );
 const auto [ a , h ] = measure( t->left() );
 const auto [ b , k ] = measure( t->right() );
 return( P( a + b + 1 , 1 + std::max( h , k ) ) );
 }

ADS_TEST( "ods-ex6.8.next-in-traversals" )
{
 for( int n : { 1 , 2 , 3 , 10 , 300 } ) {
  BinaryTree< int > * t = randomTree( n , unsigned( n ) );
  std::vector< BinaryTree< int > * > A , B;
  for( auto * u : preorder( t ) ) A.push_back( u );
  for( auto * u = t ; u ; u = nextPreorder( u ) ) B.push_back( u );
  CHECK( A == B );
  A.clear(); B.clear();
  for( auto * u : inorder( t ) ) A.push_back( u );
  for( auto * u = firstInorder( t ) ; u ; u = nextInorder( u ) )
   B.push_back( u );
  CHECK( A == B );
  A.clear(); B.clear();
  for( auto * u : postorder( t ) ) A.push_back( u );
  for( auto * u = firstPostorder( t ) ; u ; u = nextPostorder( u ) )
   B.push_back( u );
  CHECK( A == B );
  delete t;
  }
 }

ADS_TEST( "ods-ex6.4.size2-height2" )
{
 for( int n : { 1 , 2 , 5 , 64 , 1000 } ) {
  BinaryTree< int > * t = randomTree( n , unsigned( 7 * n ) );
  for( auto * u : preorder( t ) ) {         // exercises 6.4 and 6.5
   const auto [ s , h ] = measure( u );
   CHECK( ( subtreeSize( u ) == s ) && ( subtreeHeight( u ) == h ) );
   }
  delete t;
  }
 // a chain of 10^5 nodes: no recursion, no stack
 const int n = 100000;
 auto * t = new BinaryTree< int >( 0 );
 auto * u = t;
 for( int i = 1 ; i < n ; ++i ) {
  auto * v = new BinaryTree< int >( i );
  ( i % 2 ? u->insertLeft( v ) : u->insertRight( v ) );
  u = v;
  }
 CHECK( ( subtreeSize( t ) == std::size_t( n ) ) &&
        ( subtreeHeight( t ) == n - 1 ) );
 CHECK( subtreeSize( static_cast< BinaryTree< int > * >( nullptr ) ) == 0 );
 CHECK( ! isSizeBalanced( t ) );
 // exercise 6.8 on the chain, O( n ) over the whole visit
 std::size_t c = 0;
 for( auto * w = firstPostorder( t ) ; w ; w = nextPostorder( w ) ) ++c;
 CHECK( c == std::size_t( n ) );
 delete t;
 }

ADS_TEST( "ods-ex6.6.size-balanced" )
{
 for( int n : { 0 , 1 , 2 , 3 , 100 , 100000 } ) {
  std::vector< int > A( static_cast< std::size_t >( n ) , 0 );
  std::iota( A.begin() , A.end() , 0 );
  BinaryTree< int > * t = minimumHeightSearchTree( A );
  CHECK( isSizeBalanced( t ) );
  if( n >= 3 ) {                            // one node more on one side
   auto * u = t;
   while( u->right() ) u = u->right();
   u->insertRight( new BinaryTree< int >( n ) );
   u->right()->insertRight( new BinaryTree< int >( n + 1 ) );
   CHECK( ! isSizeBalanced( t ) );
   }
  delete t;
  }
 for( int s = 0 ; s < 200 ; ++s ) {          // against the definition
  BinaryTree< int > * t = randomTree( 1 + s % 9 , unsigned( s ) );
  bool ok = true;
  for( auto * u : preorder( t ) ) {
   const long a = long( measure( u->left() ).first ) ,
              b = long( measure( u->right() ).first );
   ok = ok && ( std::abs( a - b ) <= 1 );
   }
  CHECK( isSizeBalanced( t ) == ok );
  delete t;
  }
 }

ADS_TEST( "ods-ex6.7.order-numbers" )
{
 BinaryTree< int > * t = randomTree( 500 , 3 );
 std::map< const BinaryTree< int > * , std::array< std::size_t , 3 > > N;
 orderNumbers( t , [ & ]( BinaryTree< int > * u , std::size_t a ,
                          std::size_t b , std::size_t c ) {
  N[ u ] = { a , b , c }; } );
 std::size_t i = 0;
 for( auto * u : preorder( t ) ) CHECK( N[ u ][ 0 ] == i++ );
 i = 0;
 for( auto * u : inorder( t ) ) CHECK( N[ u ][ 1 ] == i++ );
 i = 0;
 for( auto * u : postorder( t ) ) CHECK( N[ u ][ 2 ] == i++ );
 // exercise 6.9: the ancestors from pre and post, in O( 1 ), and
 // post - pre = size - 1 - depth
 for( auto * u : preorder( t ) )
  for( auto * w : preorder( t ) ) {
   bool anc = false;
   for( auto * x = w ; x ; x = x->parent() ) anc = anc || ( x == u );
   CHECK( anc == ( ( N[ u ][ 0 ] <= N[ w ][ 0 ] ) &&
                   ( N[ w ][ 2 ] <= N[ u ][ 2 ] ) ) );
   }
 for( auto * u : preorder( t ) ) {
  std::size_t d = 0;
  for( auto * x = u->parent() ; x ; x = x->parent() ) ++d;
  CHECK( N[ u ][ 2 ] + d + 1 == N[ u ][ 0 ] + measure( u ).first );
  }
 delete t;
 // the three numbers alone do not give the size nor the depth: the node 2
 // has the numbers ( 2 , 2 , 2 ) in both trees, size 3 and depth 2 in the
 // first, size 2 and depth 1 in the second
 BinaryTree< int > * a = from_list< int >(
  "( 0 () ( 1 () ( 2 () ( 3 () ( 4 () () ) ) ) ) )" );
 BinaryTree< int > * b = from_list< int >(
  "( 0 ( 1 () () ) ( 2 () ( 3 () () ) ) )" );
 for( auto * r : { a , b } ) {
  orderNumbers( r , [ & ]( BinaryTree< int > * u , std::size_t x ,
                           std::size_t y , std::size_t z ) {
   if( u->read() == 2 ) CHECK( ( x == 2 ) && ( y == 2 ) && ( z == 2 ) ); } );
  }
 CHECK( subtreeSize( a->right()->right() ) == 3 );
 CHECK( subtreeSize( b->right() ) == 2 );
 delete a;
 delete b;
 }

ADS_TEST( "ods-ex6.10.pre-in-rebuild" )
{
 BinaryTree< int > * t = randomTree( 300 , 4 );
 std::vector< int > P , I;
 for( auto * u : preorder( t ) ) P.push_back( u->read() );
 for( auto * u : inorder( t ) ) I.push_back( u->read() );
 BinaryTree< int > * s = fromPreorderInorder( P , I );
 CHECK( equalTrees( t , s ) );
 delete t;
 delete s;
 }

ADS_TEST( "ods-ex6.11.shape-code" )
{
 for( int n = 0 ; n < 300 ; n += ( n < 20 ? 1 : 37 ) ) {
  BinaryTree< int > * t = randomTree( n , unsigned( n ) + 1 );
  const std::vector< bool > c = shapeCode( t );
  CHECK( c.size() == std::size_t( n ? 2 * ( n - 1 ) : 0 ) );
  BinaryTree< std::size_t > * s = fromShapeCode( c , std::size_t( n ) );
  CHECK( equalTrees( t , s , []( int , std::size_t ) { return( true ); } ) );
  std::size_t i = 0;                        // the values: the previsit
  if( s ) for( auto * u : preorder( s ) ) CHECK( u->read() == i++ );
  delete t;
  delete s;
  }
 // the shapes of 3 nodes, C_3 = 5 codes in 4 bits
 std::set< std::vector< bool > > codes;
 for( unsigned s = 0 ; s < 200 ; ++s ) {
  BinaryTree< int > * t = randomTree( 3 , s );
  codes.insert( shapeCode( t ) );
  delete t;
  }
 CHECK( codes.size() == 5 );
 CHECK_THROWS( ( void ) fromShapeCode( { false , false , false , false } ,
                                       3 ) , contract_violation );
 }

ADS_TEST( "ods-ex6.14.getLE" )
{
 MeasuredSearchTree< int , int > T;
 std::mt19937 g( 6 );
 std::map< int , int > ref;
 for( int i = 0 ; i < 3000 ; ++i ) {
  const int k = int( g() % 10000 );
  T.insert( k , i );
  ref[ k ] = i;
  }
 for( int x = -1 ; x < 10001 ; x += 37 ) {
  const auto P = T.getLE( x );
  const std::vector< std::pair< int , int > > R( ref.begin() ,
                                                 ref.upper_bound( x ) );
  CHECK( std::equal( P.begin() , P.end() , R.begin() , R.end() ) );
  const auto lb = T.lowerBound( x );
  const auto it = ref.lower_bound( x );
  CHECK( ( it == ref.end() ) ? ! lb : ( lb && ( lb->first == it->first ) ) );
  }
 }

ADS_TEST( "ods-ex6.19.size-depth-height" )
{
 MeasuredSearchTree< int , int > T;
 differential( T , 6000 , 300 , 17 );       // the invariant checks the
 std::mt19937 g( 8 );                       // three measures at each step
 for( int i = 0 ; i < 2000 ; ++i ) {
  const long h = T.height();
  const int k = int( g() % 5000 );
  T.insert( k , i );
  CHECK( T.height() <= h + 1 );             // exercise 6.18
  const auto m = T.measures( k );
  CHECK( m && ( m->height == 0 || m->size > 1 ) );
  }
 // exercise 6.15: 1 .. n in increasing order give the height n - 1, and
 // 2^( n - 1 ) of the n! orders do
 for( int n = 1 ; n <= 7 ; ++n ) {
  std::vector< int > P( static_cast< std::size_t >( n ) );
  std::iota( P.begin() , P.end() , 1 );
  int count = 0;
  do {
   MeasuredSearchTree< int , int > S;
   for( int x : P ) S.insert( x , x );
   count += ( S.height() == n - 1 );
   } while( std::next_permutation( P.begin() , P.end() ) );
  CHECK( count == ( 1 << ( n - 1 ) ) );
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
/*------------------------- End File ods_trees.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
