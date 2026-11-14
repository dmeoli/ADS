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
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/dictionary/IndexedSkipList.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/sequence/OrderMaintenanceList.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sequence/SkipListSequence.h"
#include "ADS/sequence/TreapList.h"
#include "ADS/set/DictionarySet.h"
#include "ADS/set/SpaceEfficientSortedSet.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/BTreeMap.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/FingerTreap.h"
#include "ADS/tree/HashedTreap.h"
#include "ADS/tree/JoinTree.h"
#include "ADS/tree/LeftLeaningRedBlackTree.h"
#include "ADS/tree/MeasuredSearchTree.h"
#include "ADS/tree/PartialRebuildingTree.h"
#include "ADS/tree/RandomizedSearchTree.h"
#include "ADS/tree/ScapegoatTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/Treap.h"

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
#include <sstream>
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
 d.forEach( [ & ]( const int & k , const int & v ) {
  P.emplace_back( k , v ); } );
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
/*----------------------------- CHAPTER 1 ----------------------------------*/
/*--------------------------------------------------------------------------*/

// every ordered dictionary of the library is an SSet of Morin through
// DictionarySet, and every hash table a USet
template< class D >
using SetOf = DictionarySet< D >;

static_assert( SSet< SetOf< BinarySearchTree< int , bool > > > );
static_assert( SSet< SetOf< RedBlackTree< int , bool > > > );
static_assert( SSet< SetOf< Treap< int , bool > > > );
static_assert( SSet< SetOf< AVLTree< int , bool > > > );
static_assert( SSet< SetOf< SplayTree< int , bool > > > );
static_assert( SSet< SetOf< ScapegoatTree< int , bool > > > );
static_assert( SSet< SetOf< ScapegoatTree< int , bool , true > > > );
static_assert( SSet< SetOf< LeftLeaningRedBlackTree< int , bool > > > );
static_assert( SSet< SetOf< RandomizedSearchTree< int , bool > > > );
static_assert( SSet< SetOf< MeasuredSearchTree< int , bool > > > );
static_assert( SSet< SetOf< JoinAVLTree< int , bool > > > );
static_assert( SSet< SetOf< JoinRedBlackTree< int , bool > > > );
static_assert( SSet< SetOf< WeightBalancedTree< int , bool > > > );
static_assert( SSet< SetOf< JoinTreap< int , bool > > > );
static_assert( SSet< SetOf< FingerTreap< int , bool > > > );
static_assert( SSet< SetOf< HashedTreap< int , bool > > > );
static_assert( SSet< SetOf< WeightRebuildTree< int , bool > > > );
static_assert( SSet< SetOf< CountdownTree< int , bool > > > );
static_assert( SSet< SetOf< DynamiteTree< int , bool > > > );
static_assert( SSet< SetOf< SkipList< int , bool > > > );
static_assert( SSet< SetOf< IndexedSkipList< int , bool > > > );
static_assert( SSet< SetOf< BTree< int , bool > > > );
static_assert( SSet< SetOf< BTreeMap< int , bool > > > );
static_assert( SSet< SetOf< BPlusTree< int , bool > > > );
static_assert( SSet< SetOf< SortedArrayDictionary< int , bool > > > );
static_assert( USet< ChainedHashSet< int > > &&
               ! SSet< ChainedHashSet< int > > );
static_assert( USet< OpenHashSet< int > > && ! SSet< OpenHashSet< int > > );

/// random insertions and removals on the set `S` and on a `std::set`, the
/// answers of `insert()`, `remove()`, `find()` and, for an SSet,
/// `lowerBound()` checked against each other at every step
template< class S >
void setDifferential( S & s , unsigned seed )
{
 std::mt19937 g( seed );
 std::set< int > ref;
 for( int t = 0 ; t < 4000 ; ++t ) {
  const int k = int( g() % 600 );
  if( g() % 3 )
   CHECK( s.insert( k ) == ref.insert( k ).second );
  else
   CHECK( s.remove( k ) == ( ref.erase( k ) == 1 ) );
  CHECK( s.size() == ref.size() );
  const int x = int( g() % 602 ) - 1;
  const int * f = s.find( x );
  CHECK( ref.contains( x ) ? ( f && ( *f == x ) ) : ! f );
  if constexpr( SSet< S > ) {
   const int * p = s.lowerBound( x );
   const auto it = ref.lower_bound( x );
   CHECK( ( it == ref.end() ) ? ! p : ( p && ( *p == *it ) ) );
   }
  }
 std::vector< int > all;
 s.forEach( [ & ]( const int & x ) { all.push_back( x ); } );
 if constexpr( SSet< S > )
  CHECK( all == std::vector< int >( ref.begin() , ref.end() ) );
 else {
  std::sort( all.begin() , all.end() );
  CHECK( all == std::vector< int >( ref.begin() , ref.end() ) );
  }
 }

ADS_TEST( "ods1.2.dictionary-sets" )
{
 // sections 1.2.3 and 1.2.4: the USet and the SSet of every dictionary
 unsigned seed = 11;
 auto run = [ & ]< class D >( auto... a ) {
  SetOf< D > s( a... );
  setDifferential( s , seed++ );
  };
 run.operator()< BinarySearchTree< int , bool > >();
 run.operator()< RedBlackTree< int , bool > >();
 run.operator()< Treap< int , bool > >();
 run.operator()< AVLTree< int , bool > >();
 run.operator()< SplayTree< int , bool > >();
 run.operator()< ScapegoatTree< int , bool > >();
 run.operator()< ScapegoatTree< int , bool , true > >();
 run.operator()< LeftLeaningRedBlackTree< int , bool > >();
 run.operator()< RandomizedSearchTree< int , bool > >();
 run.operator()< MeasuredSearchTree< int , bool > >();
 run.operator()< JoinAVLTree< int , bool > >();
 run.operator()< JoinRedBlackTree< int , bool > >();
 run.operator()< WeightBalancedTree< int , bool > >();
 run.operator()< JoinTreap< int , bool > >();
 run.operator()< FingerTreap< int , bool > >();
 run.operator()< HashedTreap< int , bool > >();
 run.operator()< WeightRebuildTree< int , bool > >();
 run.operator()< CountdownTree< int , bool > >();
 run.operator()< DynamiteTree< int , bool > >();
 run.operator()< SkipList< int , bool > >();
 run.operator()< IndexedSkipList< int , bool > >();
 run.operator()< BTree< int , bool > >( 2 );
 run.operator()< BTreeMap< int , bool > >();
 run.operator()< BPlusTree< int , bool > >( 4 );
 run.operator()< SortedArrayDictionary< int , bool > >();
 run.operator()< ChainedHashTable< int , bool > >( 16 );
 run.operator()< OpenHashTable< int , bool > >( 16 , Probing::linear() ,
                                                  true );
 // the values that are not bool, and a splay tree searched through a
 // constant set
 SetOf< Treap< int , std::string > > T;
 CHECK( T.insert( 3 ) && ! T.insert( 3 ) && ( T.dictionary().lookup( 3 ) ==
                                             std::string() ) );
 SetOf< SplayTree< int , bool > > Y;
 for( int i = 0 ; i < 100 ; i += 2 ) Y.insert( i );
 const auto & C = Y;
 CHECK( ( *C.lowerBound( 31 ) == 32 ) && ! C.find( 31 ) &&
        ( *C.find( 40 ) == 40 ) && ! C.lowerBound( 99 ) );
 // the keys found by the dictionaries themselves
 SkipList< int , int > S;
 BTreeMap< int , int > M;
 for( int i = 0 ; i < 50 ; ++i ) {
  S.insert( 3 * i , i );
  M.insert( 3 * i , i );
  }
 CHECK( ( *S.lowerBoundKey( 4 ) == 6 ) && ! S.findKey( 4 ) &&
        ( *S.findKey( 6 ) == 6 ) && ! S.lowerBoundKey( 148 ) );
 CHECK( ( *M.lowerBoundKey( 4 ) == 6 ) && ! M.findKey( 4 ) &&
        ( *M.findKey( 6 ) == 6 ) && ! M.lowerBoundKey( 148 ) );
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
 [[maybe_unused]] friend bool operator==( const Counted & a ,
                                         const Counted & b ) {
  return( a.k == b.k );
  }
 [[maybe_unused]] friend auto operator<=>( const Counted & a ,
                                           const Counted & b ) {
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
   if( g() % 5 < 3 ) CHECK( S.insert( x ) == ref.insert( x ).second );
   else CHECK( S.remove( x ) == ( ref.erase( x ) == 1 ) );
   CHECK( S.size() == ref.size() );
   const auto it = ref.lower_bound( x / 2 );
   const auto lb = S.lowerBound( x / 2 );
   CHECK( ( it == ref.end() ) ? ! lb : ( lb && ( *lb == *it ) ) );
   CHECK( ( S.find( x ) != nullptr ) == ( ref.count( x ) == 1 ) );
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

ADS_TEST( "ods6.2.searchtree-lowerbound" )
{
 // section 6.2.1: the successor search of the binary search tree and of
 // the red-black tree of the book, against std::map::lower_bound
 auto check = [ & ]< class T >( T & t , unsigned seed ) {
  std::mt19937 g( seed );
  std::map< int , int > ref;
  for( int s = 0 ; s < 6000 ; ++s ) {
   const int k = int( g() % 800 );
   if( g() % 3 ) { t.insert( k , s ); ref[ k ] = s; }
   else { t.remove( k ); ref.erase( k ); }
   const int x = int( g() % 802 ) - 1;
   const auto it = ref.lower_bound( x );
   const auto lb = t.lowerBound( x );
   const int * p = t.lowerBoundKey( x ) , * q = t.findKey( x );
   const auto u = t.lowerBoundNode( x );
   if( it == ref.end() )
    CHECK( ! lb && ! p && t.isNil( u ) );
   else {
    CHECK( lb && ( lb->first == it->first ) && ( lb->second == it->second ) );
    CHECK( p && ( *p == it->first ) && ( p == &T::key( u ) ) );
    }
   CHECK( ( it != ref.end() && it->first == x ) ? ( q && ( *q == x ) &&
                                                    ( q == p ) ) : ! q );
   }
  };
 BinarySearchTree< int , int > B;
 RedBlackTree< int , int > R;
 check( B , 61 );
 check( R , 62 );
 const RedBlackTree< int , int > E;         // nothing in it
 CHECK( ! E.lowerBound( 0 ) && ! E.lowerBoundKey( 0 ) && ! E.findKey( 0 ) );
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

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 7 ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// random and sorted insertions, the height within c log2 n
template< class D >
void shallow( D & d , int n , double c )
{
 for( int i = 0 ; i < n ; ++i ) d.insert( i , i );
 CHECK( double( d.height() ) <= c * std::log2( double( n ) ) );
 }

ADS_TEST( "ods-ex7.4.permute" )
{
 // the 24 orders of 4 items, 24000 draws: chi-square with 23 degrees of
 // freedom below 49.7, its 99.9% quantile
 SplitMix64 g( 3 );
 std::map< std::vector< int > , int > count;
 for( int s = 0 ; s < 24000 ; ++s ) {
  std::vector< int > A = { 0 , 1 , 2 , 3 };
  shuffleFast( A.begin() , A.end() , g );
  ++count[ A ];
  }
 CHECK( count.size() == 24 );
 double chi = 0;
 for( const auto & [ p , c ] : count ) chi += ( c - 1000.0 ) * ( c - 1000.0 )
                                              / 1000.0;
 CHECK( chi < 49.7 );
 for( std::uint64_t m : { 1ull , 2ull , 3ull , 7ull , 1000000007ull ,
                          ( 1ull << 63 ) + 5 } )
  for( int s = 0 ; s < 100 ; ++s ) CHECK( uniformBelow( g , m ) < m );
 std::vector< int > E;
 shuffleFast( E.begin() , E.end() , g );
 CHECK( E.empty() );
 }

ADS_TEST( "ods-ex7.3.count-sequences" )
{
 // f( h ) = C( 2^{h+1} - 2 , 2^h - 1 ) f( h - 1 )^2 orders build the
 // complete tree of height h: 80 for h = 2, counted, and 21964800 for h = 3
 auto binom = []( std::uint64_t n , std::uint64_t k ) {
  std::uint64_t r = 1;
  for( std::uint64_t i = 1 ; i <= k ; ++i ) r = r * ( n - k + i ) / i;
  return( r );
  };
 std::uint64_t f = 1;
 for( int h = 1 ; h <= 3 ; ++h ) {
  f = binom( ( 2ull << h ) - 2 , ( 1ull << h ) - 1 ) * f * f;
  if( h == 2 ) CHECK( f == 80 );
  }
 CHECK( f == 21964800 );
 std::vector< int > P = { 1 , 2 , 3 , 4 , 5 , 6 , 7 };
 int c = 0;
 do {
  MeasuredSearchTree< int , int > S;
  for( int x : P ) S.insert( x , x );
  c += ( S.height() == 2 );
  } while( std::next_permutation( P.begin() , P.end() ) );
 CHECK( c == 80 );
 }

ADS_TEST( "ods7.2.treap-exercises" )
{
 Treap< int , int > T( 5 );
 differential( T , 8000 , 400 , 21 );
 std::set< int > ref;
 for( const auto & [ k , v ] : T ) ref.insert( k );
 std::size_t r = 0;
 for( int k : ref ) {                       // exercise 7.10
  CHECK( T.select( r ).first == k );
  CHECK( T.rank( k ) == r++ );
  }
 for( int x = -2 ; x < 405 ; ++x ) {
  const auto it = ref.lower_bound( x );
  const auto lb = T.lowerBound( x );
  CHECK( ( it == ref.end() ) ? ! lb : ( lb && ( lb->first == *it ) ) );
  }
 CHECK_THROWS( ( void ) T.select( T.size() ) , contract_violation );
 // exercise 7.8: the same tree as the insertions in order, in O( n )
 std::vector< std::pair< int , int > > A;
 for( int i = 0 ; i < 5000 ; ++i ) A.emplace_back( 3 * i , i );
 Treap< int , int > B = Treap< int , int >::fromSorted( A , 77 ) , C( 77 );
 for( const auto & [ k , v ] : A ) C.insert( k , v );
 using TN = TreapNodes< int , int >;
 std::vector< std::pair< const void * , const void * > > S = {
  { TN::peek( B ) , TN::peek( C ) } };
 while( ! S.empty() ) {                     // the same shape and keys
  const auto [ p , q ] = S.back();
  S.pop_back();
  auto * u = static_cast< const TN::Node * >( p );
  auto * w = static_cast< const TN::Node * >( q );
  CHECK( ( u == nullptr ) == ( w == nullptr ) );
  if( ! u ) continue;
  CHECK( ( TN::key( u ) == TN::key( w ) ) &&
         ( TN::size( u ) == TN::size( w ) ) );
  S.push_back( { TN::left( u ) , TN::left( w ) } );
  S.push_back( { TN::right( u ) , TN::right( w ) } );
  }
 CHECK( B.size() == 5000 );
 // exercises 7.12 and 7.13: split( x ) and absorb( t2 )
 auto [ L , m , R ] = std::move( B ).split( 3000 );
 CHECK( m && ( *m == 1000 ) && ( L.size() == 1000 ) && ( R.size() == 3999 ) );
 Treap< int , int > J = Treap< int , int >::join2( std::move( L ) ,
                                                   std::move( R ) );
 CHECK( ( J.size() == 4999 ) && ! J.contains( 3000 ) && J.contains( 2997 ) );
 const std::vector< std::pair< int , int > > U = { { 2 , 0 } , { 1 , 0 } };
 using TI = Treap< int , int >;
 CHECK_THROWS( ( void ) TI::fromSorted( U ) , contract_violation );
 }

ADS_TEST( "ods-ex7.6.hashed-treap" )
{
 HashedTreap< int , int > T( 9 );
 differential( T , 20000 , 1000 , 23 );
 HashedTreap< int , int > S( 4 );
 shallow( S , 1 << 12 , 4.0 );              // sorted keys, random shape
 const auto lb = S.lowerBound( 100 );
 CHECK( lb && ( lb->first == 100 ) );
 }

ADS_TEST( "ods-ex7.9.finger-treap" )
{
 FingerTreap< int , int > T( 2 );
 differential( T , 20000 , 800 , 29 );
 FingerTreap< int , int > F( 6 );
 shallow( F , 1 << 12 , 4.0 );
 for( int s = 0 ; s < 40000 ; ++s ) {       // near the previous key
  const int x = ( s * 7 ) % ( 1 << 12 ) + ( s % 3 ) - 1;
  const auto v = F.fingerLookup( x );
  CHECK( ( x >= 0 ) && ( x < ( 1 << 12 ) ) ? ( v && ( *v == x ) )
                                           : ! v.has_value() );
  }
 const auto f = F.finger( 2000 );
 for( int x = 1900 ; x < 2100 ; ++x ) CHECK( F.lookupFrom( x , f ) == x );
 CHECK( ! F.lookupFrom( -3 , f ) && ( F.lookupFrom( 0 , {} ) == 0 ) );
 for( std::size_t i = 0 ; i < F.size() ; i += 97 )
  CHECK( ( F.select( i ).first == int( i ) ) &&
         ( F.rank( int( i ) ) == i ) );
 F.remove( 2000 );                          // the finger back to the root
 CHECK( ! F.fingerLookup( 2000 ) && ( F.fingerLookup( 2001 ) == 2001 ) );
 }

ADS_TEST( "ods-ex7.11.treap-list" )
{
 TreapList< int > L( 5 );
 std::vector< int > ref;
 std::mt19937 g( 12 );
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
 for( int c = 0 ; c < 50 ; ++c ) {          // cut and glue
  const std::size_t i = g() % ( ref.size() + 1 );
  TreapList< int > R = L.truncate( i );
  CHECK( ( L.size() == i ) && ( R.size() == ref.size() - i ) );
  CHECK( ( R.empty() || ( R.get( 0 ) == ref[ i ] ) ) );
  L.absorb( R );
  CHECK( R.empty() );
  }
 CHECK( std::vector< int >( L.begin() , L.end() ) == ref );
 CHECK_THROWS( L.add( ref.size() + 1 , 0 ) , contract_violation );
 }

ADS_TEST( "ods-ex7.14.randomized-bst" )
{
 RandomizedSearchTree< int , int > T( 3 );
 differential( T , 20000 , 900 , 31 );
 RandomizedSearchTree< int , int > S( 8 );
 shallow( S , 1 << 12 , 4.0 );
 for( std::size_t i = 0 ; i < S.size() ; i += 101 )
  CHECK( ( S.select( i ).first == int( i ) ) && ( S.rank( int( i ) ) == i ) );
 // still a random tree after a removal and an insertion: a path, of
 // height 4, has probability 2^4 / 5! as for a random order
 int roots = 0;
 for( int s = 0 ; s < 10000 ; ++s ) {
  RandomizedSearchTree< int , int > R( std::uint64_t( s ) + 100 );
  for( int i = 1 ; i <= 5 ; ++i ) R.insert( i , i );
  R.remove( 1 );
  R.insert( 1 , 1 );                        // still random after both
  roots += ( R.height() == 4 );
  }
 CHECK( std::abs( roots / 10000.0 - 16.0 / 120.0 ) < 0.02 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 8 ----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "ods8.1.scapegoat" )
{
 using S = ScapegoatTree< int , int >;
 for( double b : { 1.05 , 1.5 , 1.95 } )
  for( auto z : { S::Sizes::RECOMPUTE , S::Sizes::REUSE } )
   for( auto r : { S::Rebuilds::ARRAY , S::Rebuilds::LIST } )
    for( auto w : { S::Scapegoats::HEIGHT , S::Scapegoats::WEIGHT } ) {
     S T( b , z , r , w );
     differential( T , 4000 , 500 , 37 );
     }
 ScapegoatTree< int , int , true > U( 1.5 );
 differential( U , 6000 , 500 , 41 );
 ScapegoatTree< int , int > V;
 for( int i = 0 ; i < 1000 ; ++i ) V.insert( 2 * i , i );
 for( int x = -1 ; x < 2001 ; ++x ) {
  const auto lb = V.lowerBound( x );
  CHECK( ( x > 1998 ) ? ! lb : ( lb && ( lb->first == x + ( x & 1 ) ) ) );
  }
 ScapegoatTree< int , int > M( std::move( V ) );
 CHECK( ( M.size() == 1000 ) && ( V.size() == 0 ) && ! V.contains( 0 ) );
 CHECK_THROWS( S( 2.0 ) , contract_violation );
 CHECK_THROWS( S( 1.0 ) , contract_violation );
 }

ADS_TEST( "ods-ex8.5.scapegoat-variants" )
{
 // exercises 8.5 to 8.7: the sizes recomputed, reused or stored, the
 // rebuilds by an array or by a list, make the same decisions and the same
 // trees, hence the same rebuilds
 using S = ScapegoatTree< int , int >;
 std::vector< std::pair< std::size_t , std::size_t > > W;
 std::vector< long > H;
 auto run = [ & ]( auto & T ) {
  std::mt19937 g( 43 );
  for( int s = 0 ; s < 20000 ; ++s ) {
   const int k = int( g() % 3000 );
   if( g() % 3 ) T.insert( k , s );
   else T.remove( k );
   }
  W.emplace_back( T.rebuilds() , T.rebuiltNodes() );
  H.push_back( T.height() );
  };
 for( auto w : { S::Scapegoats::HEIGHT , S::Scapegoats::WEIGHT } ) {
  W.clear();
  H.clear();
  for( auto z : { S::Sizes::RECOMPUTE , S::Sizes::REUSE } )
   for( auto r : { S::Rebuilds::ARRAY , S::Rebuilds::LIST } ) {
    S T( 1.5 , z , r , w );
    run( T );
    }
  using U = ScapegoatTree< int , int , true >;
  U T( 1.5 , U::Sizes::REUSE , U::Rebuilds::LIST ,
       U::Scapegoats( int( w ) ) );
  run( T );
  for( std::size_t i = 1 ; i < W.size() ; ++i )
   CHECK( ( W[ i ] == W[ 0 ] ) && ( H[ i ] == H[ 0 ] ) );
  CHECK( W[ 0 ].first > 0 );
  }
 }

ADS_TEST( "ods-ex8.3.rebuild-nlogn" )
{
 // 1 , 2 , ... , n inserted: the rebuilds take Theta( n log n ) nodes
 double lo = 1e9 , hi = 0;
 for( int e = 8 ; e <= 12 ; e += 2 ) {
  const int n = 1 << e;
  ScapegoatTree< int , int , true > T;
  for( int i = 0 ; i < n ; ++i ) T.insert( i , i );
  const double r = double( T.rebuiltNodes() ) / ( n * double( e ) );
  lo = std::min( lo , r );
  hi = std::max( hi , r );
  CHECK( double( T.height() ) <= std::log( double( n ) ) /
                                 std::log( 1.5 ) + 1e-9 );
  }
 CHECK( ( lo > 0.1 ) && ( hi < 2 * lo ) );
 }

ADS_TEST( "ods-ex8.4.scapegoat-base" )
{
 // the height within log_b q, and the work of the rebuilds larger as b
 // nears 2, the height bound tighter
 std::size_t last = 0;
 for( double b : { 1.05 , 1.2 , 1.5 , 1.9 } ) {
  ScapegoatTree< int , int , true > T( b );
  std::mt19937 g( 47 );
  for( int i = 0 ; i < 4000 ; ++i ) T.insert( int( g() % 100000 ) , i );
  CHECK( double( T.height() ) <=
         std::log( double( T.size() ) * 2 ) / std::log( b ) );
  CHECK( T.rebuiltNodes() >= last );
  last = T.rebuiltNodes();
  }
 }

ADS_TEST( "ods-ex8.8.weight-rebuild" )
{
 WeightRebuildTree< int , int > T;
 differential( T , 20000 , 900 , 53 );      // the invariant: the weights
 WeightRebuildTree< int , int > S;
 for( int i = 0 ; i < 1 << 12 ; ++i ) S.insert( i , i );
 CHECK( double( S.height() ) <= 12 * std::log( 2.0 ) / std::log( 1.5 ) );
 const auto lb = S.lowerBound( 77 );
 CHECK( lb && ( lb->first == 77 ) );
 }

ADS_TEST( "ods-ex8.9.countdown" )
{
 CountdownTree< int , int > T;
 differential( T , 20000 , 900 , 59 );
 CountdownTree< int , int > S;
 for( int i = 0 ; i < 1 << 12 ; ++i ) S.insert( i , i );
 CHECK( double( S.height() ) <= 12 * std::log( 2.0 ) / std::log( 4.0 / 3 )
                                + 3 );
 for( int i = 0 ; i < 1 << 12 ; i += 2 ) S.remove( i );
 CHECK( double( S.height() ) <= 11 * std::log( 2.0 ) / std::log( 4.0 / 3 )
                                + 3 );
 // O( log n ) amortized: the rebuilds within a constant per level
 CHECK( double( S.rebuiltNodes() ) <= 8.0 * ( 1 << 12 ) * 1.5 * 12 );
 }

ADS_TEST( "ods-ex8.10.dynamite" )
{
 DynamiteTree< int , int > T( 3 );
 differential( T , 20000 , 900 , 61 );
 DynamiteTree< int , int > S( 5 );
 for( int i = 0 ; i < 1 << 12 ; ++i ) S.insert( i , i );
 CHECK( S.height() <= 3 * 12 );
 CHECK( double( S.rebuiltNodes() ) <= 4.0 * ( 1 << 12 ) * 12 );
 }

ADS_TEST( "ods-ex8.11.order-maintenance" )
{
 OrderMaintenanceList< int > L;
 using E = OrderMaintenanceList< int >::Element;
 std::vector< E > ref;                      // the elements in order
 std::mt19937 g( 67 );
 for( int s = 0 ; s < 20000 ; ++s ) {
  if( ref.empty() || ( g() % 4 != 0 ) ) {
   const std::size_t i = g() % ( ref.size() + 1 );   // after ref[ i - 1 ]
   const E e = L.addAfter( i ? ref[ i - 1 ] : nullptr , s );
   ref.insert( ref.begin() + std::ptrdiff_t( i ) , e );
   }
  else {
   const std::size_t i = g() % ref.size();
   L.remove( ref[ i ] );
   ref.erase( ref.begin() + std::ptrdiff_t( i ) );
   }
  CHECK( L.size() == ref.size() );
  if( ref.size() > 1 )
   for( int t = 0 ; t < 4 ; ++t ) {
    const std::size_t a = g() % ref.size() , b = g() % ref.size();
    CHECK( L.testBefore( ref[ a ] , ref[ b ] ) == ( a < b ) );
    }
  }
 std::size_t i = 0;
 for( E e = L.first() ; e ; e = L.next( e ) ) CHECK( e == ref[ i++ ] );
 CHECK( i == ref.size() );
 // always at the front, or always after the last: the worst cases of the
 // depth, relabeled by the rebuilds
 OrderMaintenanceList< int > F , B;
 E last = nullptr;
 for( int j = 0 ; j < 5000 ; ++j ) {
  F.addAfter( nullptr , j );
  last = B.addAfter( last , j );
  }
 E x = F.first();
 for( E y = F.next( x ) ; y ; x = y , y = F.next( y ) )
  CHECK( F.testBefore( x , y ) && ( F.value( x ) == F.value( y ) + 1 ) );
 CHECK_THROWS( L.remove( nullptr ) , contract_violation );
 }

ADS_TEST( "ods-ex8.11.order-constant" )
{
 // the labels written per insertion stay bounded as n grows (O( 1 )
 // amortized), at the front, always after the newest element, always
 // after the first one, and at random places, with removals among them
 auto work = []( int n , int pattern ) {
  OrderMaintenanceList< int > L;
  using E = OrderMaintenanceList< int >::Element;
  std::vector< E > all;
  std::mt19937 g( unsigned( 89 + pattern ) );
  E last = nullptr , head = nullptr;
  for( int s = 0 ; s < n ; ++s ) {
   E e = nullptr;
   if( pattern == 1 ) e = last;
   else if( pattern == 2 ) e = head;
   else if( pattern == 3 && ! all.empty() ) e = all[ g() % all.size() ];
   last = L.addAfter( e , s );
   if( ! head ) head = last;
   all.push_back( last );
   if( ( pattern == 3 ) && ( g() % 5 == 0 ) ) {
    const std::size_t i = g() % all.size();
    if( all[ i ] != head ) {
     if( all[ i ] == last ) last = nullptr;
     L.remove( all[ i ] );
     all[ i ] = all.back();
     all.pop_back();
     }
    }
   }
  for( E x = L.first() , y = x ? L.next( x ) : nullptr ; y ;
       x = y , y = L.next( y ) )
   CHECK( L.testBefore( x , y ) && ! L.testBefore( y , x ) );
  return( double( L.relabelWork().second ) / n );
  };
 for( int pattern = 0 ; pattern < 4 ; ++pattern ) {
  const double small = work( 1 << 12 , pattern ) ,
               large = work( 1 << 17 , pattern );
  CHECK( ( large <= 8 ) && ( large <= small + 1 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 9 ----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "ods9.2.left-leaning-red-black" )
{
 LeftLeaningRedBlackTree< int , int > T;
 differential( T , 30000 , 1500 , 71 );     // the invariant: the three
 LeftLeaningRedBlackTree< int , int > S;    // properties at each step
 const int n = 1 << 12;
 for( int i = 0 ; i < n ; ++i ) S.insert( i , i );
 CHECK( double( S.height() ) <= 2 * std::log2( double( n + 1 ) ) );
 for( int i = 0 ; i < n ; i += 3 ) S.remove( i );
 for( int x = -1 ; x <= n ; ++x ) {
  const auto lb = S.lowerBound( x );
  int y = std::max( x , 0 );
  while( y % 3 == 0 ) ++y;
  CHECK( ( y >= n ) ? ! lb : ( lb && ( lb->first == y ) ) );
  }
 for( int i = 0 ; i < n ; ++i ) S.remove( i );
 CHECK( ( S.size() == 0 ) && ( S.height() == -1 ) );
 LeftLeaningRedBlackTree< int , int > M( std::move( T ) );
 CHECK( T.size() == 0 );
 }

ADS_TEST( "ods-ex9.1.two-four-tree" )
{
 // exercises 9.1 and 9.8: a black node and its red children are a 2-4
 // node; n = n2 + 2 n3 + 3 n4 keys, the red nodes are n3 + 2 n4, and the
 // 2-4 tree has n_i internal nodes for n_l = n + 1 leaves (the nil), with
 // ( n_l - 1 ) / 3 <= n_i <= n_l - 1
 LeftLeaningRedBlackTree< int , int > T;
 std::mt19937 g( 73 );
 for( int s = 0 ; s < 3000 ; ++s ) {
  T.insert( int( g() % 2000 ) , s );
  if( s % 3 == 0 ) T.remove( int( g() % 2000 ) );
  const auto D = T.twoFourDegrees();
  const std::size_t ni = D[ 0 ] + D[ 1 ] + D[ 2 ] , nl = T.size() + 1;
  CHECK( D[ 0 ] + 2 * D[ 1 ] + 3 * D[ 2 ] == T.size() );
  CHECK( ( 3 * ni >= nl - 1 ) && ( ni <= nl - 1 ) );
  }
 // exercises 9.4 and 9.6: the keys in decreasing order give the height
 // 2 log2 n - 3
 for( int k = 6 ; k <= 12 ; ++k ) {
  LeftLeaningRedBlackTree< int , int > D;
  for( int i = 1 << k ; i > 0 ; --i ) D.insert( i , i );
  CHECK( D.height() == 2 * k - 3 );
  }
 // the 2-4 tree of the black height: all its leaves at that depth, and
 // the height of the red-black tree at most twice it
 CHECK( T.height() < 2 * T.blackHeight() );
 }

ADS_TEST( "ods-ex9.10.red-black-join" )
{
 // exercises 9.10 and 9.11: equal and different black heights
 std::mt19937 g( 79 );
 for( int c = 0 ; c < 60 ; ++c ) {
  const int a = int( g() % 600 ) , b = int( g() % ( c < 30 ? 600 : 6 ) );
  LeftLeaningRedBlackTree< int , int > L , R;
  for( int i = 0 ; i < a ; ++i ) L.insert( 2 * i , i );
  for( int i = 0 ; i < b ; ++i ) R.insert( 2 * ( a + i ) + 1 , i );
  L.absorb( R );
  CHECK( ( L.size() == std::size_t( a + b ) ) && ( R.size() == 0 ) );
  std::size_t i = 0;
  for( const auto & [ k , v ] : L ) {
   CHECK( k == ( int( i ) < a ? 2 * int( i ) : 2 * int( i ) + 1 ) );
   ++i;
   }
  }
 LeftLeaningRedBlackTree< int , int > X , Y;
 X.insert( 5 , 5 );
 Y.insert( 3 , 3 );
 CHECK_THROWS( X.absorb( Y ) , contract_violation );
 }

ADS_TEST( "ods-ex9.12.avl-rebalancing" )
{
 // exercise 9.12: an insertion rebalances once at most, by one or two
 // rotations; the removal of the deepest leaf on the shorter side of a
 // tree of Fibonacci rebalances on the order of its height
 AVLTree< int , int > T;
 std::mt19937 g( 81 );
 for( int s = 0 ; s < 20000 ; ++s ) {
  const auto [ r0 , t0 ] = T.rebalancings();
  T.insert( int( g() % 100000 ) , s );
  const auto [ r1 , t1 ] = T.rebalancings();
  CHECK( ( r1 - r0 <= 1 ) && ( t1 - t0 <= 2 ) );
  }
 for( int i = 0 ; i < 1 << 12 ; ++i ) {      // increasing, the worst order
  const auto [ r0 , t0 ] = T.rebalancings();
  T.insert( 200000 + i , i );
  const auto [ r1 , t1 ] = T.rebalancings();
  CHECK( ( r1 - r0 <= 1 ) && ( t1 - t0 <= 2 ) );
  }
 for( int h = 6 ; h <= 22 ; h += 4 ) {
  // the tree of Fibonacci of height h, left subtree of height h - 1 and
  // right of h - 2, its keys in symmetric order, inserted level by level,
  // which keeps every prefix balanced and needs no rotation
  struct Shape { int key , left , right , h; };
  std::vector< Shape > S;
  int next = 0;
  auto build = [ & ]( auto & self , int k ) -> int {
   if( k < 0 ) return( -1 );
   const int u = int( S.size() );
   S.push_back( { 0 , -1 , -1 , k } );
   const int l = self( self , k - 1 );
   S[ u ].key = next++;
   const int r = self( self , k - 2 );
   S[ u ].left = l;
   S[ u ].right = r;
   return( u );
   };
  build( build , h );
  AVLTree< int , int > F;
  std::vector< int > level = { 0 };
  while( ! level.empty() ) {
   std::vector< int > below;
   for( const int u : level ) {
    F.insert( S[ u ].key , 0 );
    for( const int c : { S[ u ].left , S[ u ].right } )
     if( c >= 0 ) below.push_back( c );
    }
   level = below;
   }
  CHECK( ( F.height() == h ) && ( F.rebalancings().first == 0 ) );
  int u = 0;                                // the shorter side, down to a
  for( ; ; ) {                              // leaf
   const int c = S[ u ].right >= 0 ? S[ u ].right : S[ u ].left;
   if( c < 0 ) break;
   u = c;
   }
  F.remove( S[ u ].key );
  CHECK( int( F.rebalancings().first ) >= h / 2 - 1 );
  }
 }

ADS_TEST( "ods-ex9.13.avl" )
{
 // exercise 9.13: the AVL tree of ADS, height at most 1.4405 log2 n
 AVLTree< int , int > T;
 differential( T , 20000 , 1500 , 83 );
 AVLTree< int , int > S;
 const int n = 1 << 12;
 for( int i = 0 ; i < n ; ++i ) S.insert( i , i );
 CHECK( double( S.height() ) <= 1.4405 * std::log2( double( n + 2 ) ) );
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
/*------------------ THE FIGURES OF THE EXERCISES OF MORIN -----------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// a binary search tree as ( key left right ), "-" for nil and no children
/// written for a leaf
template< class T >
std::string bstShape( const T & t , typename T::Link * u )
{
 if( t.isNil( u ) )
  return( "-" );
 std::ostringstream o;
 o << "(" << T::key( u );
 if( ! t.isNil( u->left ) || ! t.isNil( u->right ) )
  o << " " << bstShape( t , u->left ) << " " << bstShape( t , u->right );
 o << ")";
 return( o.str() );
 }

/// a tree read through its `Shape`, as `bstShape()` writes it, each red
/// node of a red-black tree followed by r if `colours`
template< class T , class N >
std::string shapeOf( const N * n , bool colours )
{
 using S = typename T::Shape;
 if( ! n )
  return( "-" );
 std::ostringstream o;
 o << "(" << S::read( n ).first;
 if constexpr( requires { n->colour; } )
  if( colours && ( n->colour == 0 ) )
   o << "r";
 if( S::left( n ) || S::right( n ) )
  o << " " << shapeOf< T >( S::left( n ) , colours ) << " "
    << shapeOf< T >( S::right( n ) , colours );
 o << ")";
 return( o.str() );
 }

/// the whole tree `t` as `shapeOf()` writes it
template< class T >
std::string shapeOf( const T & t , bool colours = false )
{
 return( shapeOf< T >( T::Shape::root( t ) , colours ) );
 }

/// the binary search tree of figure 6.5 of Morin's book
BinarySearchTree< double , int > figureSearchTree( void )
{
 BinarySearchTree< double , int > t;
 for( double k : { 7 , 3 , 11 , 1 , 5 , 9 , 13 , 4 , 6 , 8 , 12 , 14 } )
  t.insert( k , 0 );
 return( t );
 }

/// the left-leaning red-black tree of the exercises 9.2 and 9.3 of Morin's
/// book, built by insertions and two removals
LeftLeaningRedBlackTree< double , int > figureRedBlackTree( void )
{
 LeftLeaningRedBlackTree< double , int > t;
 for( double k : { 7 , 0 , 6 , 10 , 8 , 12 , 1 , 4 , 5 , 2 , 3 , 9 , 11 ,
                   13 } )
  t.insert( k , 0 );
 t.remove( 0 );
 t.remove( 13 );
 return( t );
 }

}  // end( namespace )

// ods ex 6.12: 3.5 and then 4.5 inserted into the tree of figure 6.5, both
// below 4 (figure 8.23 of the manual)
ADS_TEST( "ods-ex6.12.bst-insert-figure" )
{
 auto t = figureSearchTree();
 const std::string right = "(11 (9 (8) -) (13 (12) (14)))";
 CHECK( bstShape( t , t.root() ) == "(7 (3 (1) (5 (4) (6))) " + right + ")" );
 t.insert( 3.5 , 0 );
 CHECK( bstShape( t , t.root() ) ==
        "(7 (3 (1) (5 (4 (3.5) -) (6))) " + right + ")" );
 t.insert( 4.5 , 0 );
 CHECK( bstShape( t , t.root() ) ==
        "(7 (3 (1) (5 (4 (3.5) (4.5)) (6))) " + right + ")" );
 }

// ods ex 6.13: 3 removed, its successor 4 taking its node, and then 5,
// whose only child 6 is hung to 4 (figure 8.24 of the manual)
ADS_TEST( "ods-ex6.13.bst-remove-figure" )
{
 auto t = figureSearchTree();
 const std::string right = "(11 (9 (8) -) (13 (12) (14)))";
 t.remove( 3 );
 CHECK( bstShape( t , t.root() ) == "(7 (4 (1) (5 - (6))) " + right + ")" );
 t.remove( 5 );
 CHECK( bstShape( t , t.root() ) == "(7 (4 (1) (6)) " + right + ")" );
 }

// ods ex 8.1: 1.5 and then 1.6 inserted into the scapegoat tree of figure
// 8.1, at the depths 5 and 6, within log_{3/2} q, with no rebuild (figure
// 8.46 of the manual)
ADS_TEST( "ods-ex8.1.scapegoat-figure" )
{
 ScapegoatTree< double , int > t;
 for( double k : { 7 , 6 , 8 , 5 , 9 , 2 , 1 , 4 , 0 , 3 } )
  t.insert( k , 0 );
 const std::string up = "(7 (6 (5 (2 " , down = " (4 (3) -)) -) -) (8 - (9)))";
 CHECK( ( shapeOf( t ) == up + "(1 (0) -)" + down ) && ( t.height() == 5 ) );
 t.insert( 1.5 , 0 );
 CHECK( shapeOf( t ) == up + "(1 (0) (1.5))" + down );
 t.insert( 1.6 , 0 );
 CHECK( shapeOf( t ) == up + "(1 (0) (1.5 - (1.6)))" + down );
 CHECK( ( t.height() == 6 ) && ( t.rebuilds() == 0 ) );
 }

// ods ex 8.2: 1, 5, 2, 4, 3 inserted into an empty scapegoat tree; the
// insertion of 3 rebuilds the subtree of 5 (4 nodes) by the rule WEIGHT,
// and the whole tree (5 nodes) by the rule HEIGHT (figure 8.47 of the
// manual)
ADS_TEST( "ods-ex8.2.scapegoat-credits" )
{
 using S = ScapegoatTree< double , int >;
 for( auto w : { S::Scapegoats::WEIGHT , S::Scapegoats::HEIGHT } ) {
  S t( 1.5 , S::Sizes::REUSE , S::Rebuilds::ARRAY , w );
  for( double k : { 1 , 5 , 2 , 4 } )
   t.insert( k , 0 );
  CHECK( ( shapeOf( t ) == "(1 - (5 (2 - (4)) -))" ) &&
         ( t.rebuilds() == 0 ) );
  t.insert( 3 , 0 );
  CHECK( t.rebuilds() == 1 );
  if( w == S::Scapegoats::WEIGHT )
   CHECK( ( shapeOf( t ) == "(1 - (3 (2) (4 - (5))))" ) &&
          ( t.rebuiltNodes() == 4 ) );
  else
   CHECK( ( shapeOf( t ) == "(3 (1 - (2)) (4 - (5)))" ) &&
          ( t.rebuiltNodes() == 5 ) );
  }
 }

// ods ex 9.2: 13, 3.5 and 3.3 inserted into the tree of the exercises,
// with their colours (figure 8.54 of the manual)
ADS_TEST( "ods-ex9.2.red-black-insert-figure" )
{
 auto t = figureRedBlackTree();
 CHECK( shapeOf( t , true ) ==
        "(6 (4 (2r (1) (3)) (5)) (10 (8r (7) (9)) (12 (11r) -)))" );
 t.insert( 13 , 0 );
 CHECK( shapeOf( t , true ) ==
        "(6 (4 (2r (1) (3)) (5)) (10 (8r (7) (9)) (12 (11r) (13r))))" );
 t.insert( 3.5 , 0 );
 CHECK( shapeOf( t , true ) == "(6 (4 (2r (1) (3.5 (3r) -)) (5)) "
                               "(10 (8r (7) (9)) (12 (11r) (13r))))" );
 t.insert( 3.3 , 0 );
 CHECK( shapeOf( t , true ) == "(6 (4 (2r (1) (3.3 (3r) (3.5r))) (5)) "
                               "(10 (8r (7) (9)) (12 (11r) (13r))))" );
 }

// ods ex 9.3: 11, 9 and 5 removed from the same tree, a red leaf, case 3
// and then case 1 followed by case 3 of removeFixup, and 1, case 2
// (figures 8.55 and 8.53 of the manual)
ADS_TEST( "ods-ex9.3.red-black-remove-figure" )
{
 auto t = figureRedBlackTree();
 t.remove( 11 );
 CHECK( shapeOf( t , true ) ==
        "(6 (4 (2r (1) (3)) (5)) (10 (8r (7) (9)) (12)))" );
 t.remove( 9 );
 CHECK( shapeOf( t , true ) ==
        "(6 (4 (2r (1) (3)) (5)) (10 (8 (7r) -) (12)))" );
 t.remove( 5 );
 CHECK( shapeOf( t , true ) == "(6 (2 (1) (4 (3r) -)) (10 (8 (7r) -) (12)))" );
 auto u = figureRedBlackTree();
 u.remove( 1 );
 CHECK( shapeOf( u , true ) ==
        "(6 (4 (3 (2r) -) (5)) (10 (8r (7) (9)) (12 (11r) -)))" );
 }

// ods ex 14.1 and 14.2 on BTree, the insertion and the removal in one
// pass: the class shows no node, and the test checks the height, the keys
// and, at the contract level 3, the invariant of the B-tree after each
// step, on the keys 0, ..., 23 of figure 14.2 (figures 8.56 to 8.58 of the
// manual)
ADS_TEST( "ods-ex14.1.btree-one-pass" )
{
 auto keys = []( const BTree< double , int > & t ) {
  std::vector< double > k;
  for( const auto & [ x , v ] : t )
   k.push_back( x );
  return( k );
  };
 BTree< double , int > t( 2 );
 for( int k = 0 ; k < 24 ; ++k )
  t.insert( k , k );
 std::vector< double > want( 24 );
 std::iota( want.begin() , want.end() , 0.0 );
 CHECK( keys( t ) == want );
 const int h = t.height();
 for( double k : { 1.5 , 7.5 } ) {
  t.insert( k , 0 );
  want.insert( std::lower_bound( want.begin() , want.end() , k ) , k );
  CHECK( ( keys( t ) == want ) && ( t.height() >= h ) &&
         ( t.height() <= h + 1 ) );
  }
 for( double k : { 3.0 , 4.0 , 5.0 , 0.0 , 6.0 } ) {
  t.remove( k );
  want.erase( std::lower_bound( want.begin() , want.end() , k ) );
  CHECK( ( keys( t ) == want ) && ! t.contains( k ) &&
         ( t.size() == want.size() ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File ods_trees.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
