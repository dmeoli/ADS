/*--------------------------------------------------------------------------*/
/*---------------------------- File beyond.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of what the library has beyond the book, on the classes of the
 * book: the binary trees in a positional vector and with cursors, the
 * ordered trees with cursors and in preorder, the heap with pointers, the
 * graph with pointers, the k stacks in one vector, the deep copies of
 * trees, the triangular inspection and the double hashing with a second
 * function, the rectangular matrices, the algorithms on sequences and on
 * stacks, the exact string matching, and some classic exercises on lists,
 * strings, binary and ordered trees and arrays.
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

#include "ADS/dictionary/HashFunctions.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/graph/PointerGraph.h"
#include "ADS/numeric/Matrix.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/SequenceAlgorithms.h"
#include "ADS/stack/MultiStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/stack/StackAlgorithms.h"
#include "ADS/string/StringMatching.h"
#include "ADS/tree/ArrayBinaryTree.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/CursorBinaryTree.h"
#include "ADS/tree/CursorTree.h"
#include "ADS/tree/PreorderArrayTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

/// the items of a sequence, in order
template< class S >
std::vector< typename S::Item > items( const S & s )
{
 std::vector< typename S::Item > v;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  v.push_back( s.read( p ) );
 return( v );
 }

/// a List of the given items
template< class T >
List< T > list_of( const std::vector< T > & v )
{
 List< T > l;
 for( const T & x : v ) l.insert( l.next( l.tail() ) , x );
 return( l );
 }

/// the values of a binary tree in previsit
template< class T >
std::vector< T > pre_values( const BinaryTree< T > * t )
{
 std::vector< T > v;
 pre_visit( const_cast< BinaryTree< T > * >( t ) ,
            [ & ]( BinaryTree< T > * u ) { v.push_back( u->read() ); } );
 return( v );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- EXERCISES OF THE OLD CODE --------------------------*/
/*--------------------------------------------------------------------------*/

// on lists: the multiples of k, the frequencies in order of
// first occurrence, the items followed by an odd one removed, and the
// items multiple of their place, from 1, removed

int freq( const List< int > & l , int k )
{
 int c = 0;
 for( int x : items( l ) ) c += x % k == 0;
 return( c );
 }

std::vector< std::pair< int , int > > hist( const List< int > & l )
{
 std::vector< std::pair< int , int > > H;
 for( int x : items( l ) ) {
  auto it = std::find_if( H.begin() , H.end() , [ & ]( const auto & h ) {
   return( h.first == x ); } );
  if( it == H.end() ) H.emplace_back( x , 1 );
  else ++it->second;
  }
 return( H );
 }

void remp( List< int > & l )
{
 for( auto p = l.head() ; ! l.finished( p ) ; ) {
  const auto q = l.next( p );
  if( ! l.finished( q ) && ( l.read( q ) % 2 != 0 ) ) l.remove( p );
  p = q;
  }
 }

void del( List< int > & l )
{
 int i = 1;
 for( auto p = l.head() ; ! l.finished( p ) ; ++i )
  if( l.read( p ) % i == 0 ) p = l.remove( p );
  else p = l.next( p );
 }

// the strings of even length in one list and those of odd length in
// another, each new string at the head

class EvenOddStrings {
 public:
  void insert( const std::string & s ) {
   List< std::string > & l = s.size() % 2 ? f_odd : f_even;
   l.insert( l.head() , s );
   }
  /// removes every copy of s
  void removeAll( const std::string & s ) {
   List< std::string > & l = s.size() % 2 ? f_odd : f_even;
   for( auto p = l.head() ; ! l.finished( p ) ; )
    p = l.read( p ) == s ? l.remove( p ) : l.next( p );
   }
  /// removes the even strings that start with c
  void removeStartingWith( char c ) {
   for( auto p = f_even.head() ; ! f_even.finished( p ) ; )
    p = ( ! f_even.read( p ).empty() && ( f_even.read( p )[ 0 ] == c ) )
        ? f_even.remove( p ) : f_even.next( p );
   }
  [[nodiscard]] int evenCount( void ) const {
   return( int( items( f_even ).size() ) );
   }
  /// the odd strings by length, equal lengths in order of insertion
  [[nodiscard]] std::vector< std::string > oddByLength( void ) const {
   auto v = items( f_odd );
   std::reverse( v.begin() , v.end() );
   std::stable_sort( v.begin() , v.end() , []( const auto & a ,
                     const auto & b ) { return( a.size() < b.size() ); } );
   return( v );
   }
  [[nodiscard]] std::vector< std::string > even( void ) const {
   return( items( f_even ) );
   }
 private:
  List< std::string > f_even , f_odd;
 };

// sums, maxima, heights and counts on a binary tree of integers

using BT = BinaryTree< int >;

long sumNodes( const BT * t )
{
 return( t ? t->read() + sumNodes( t->left() ) + sumNodes( t->right() ) : 0 );
 }

int maxNode( const BT * t )
{
 int m = t->read();
 for( const BT * c : { t->left() , t->right() } )
  if( c ) m = std::max( m , maxNode( c ) );
 return( m );
 }

/// the largest sum along a path from the root to a leaf
long maxPathCost( const BT * t )
{
 if( ! t->left() && ! t->right() ) return( t->read() );
 long best = std::numeric_limits< long >::min();
 for( const BT * c : { t->left() , t->right() } )
  if( c ) best = std::max( best , maxPathCost( c ) );
 return( t->read() + best );
 }

/// the height counted in nodes, 0 for the empty tree
int heightNodes( const BT * t )
{
 return( t ? 1 + std::max( heightNodes( t->left() ) ,
                           heightNodes( t->right() ) ) : 0 );
 }

/// whether the heights of the two subtrees of every node differ by at
/// most 1
bool heightBalanced( const BT * t )
{
 if( ! t ) return( true );
 return( std::abs( heightNodes( t->left() ) - heightNodes( t->right() ) )
         <= 1 && heightBalanced( t->left() ) && heightBalanced( t->right() ) );
 }

/// the odd nodes of level k, the root of level 0
int oddAtLevel( const BT * t , int k )
{
 if( ! t ) return( 0 );
 if( k == 0 ) return( t->read() % 2 != 0 );
 return( oddAtLevel( t->left() , k - 1 ) + oddAtLevel( t->right() , k - 1 ) );
 }

int evenLeaves( const BT * t )
{
 if( ! t ) return( 0 );
 if( ! t->left() && ! t->right() ) return( t->read() % 2 == 0 );
 return( evenLeaves( t->left() ) + evenLeaves( t->right() ) );
 }

/// the even proper descendants of t
int evenDescendants( const BT * t )
{
 int c = 0;
 for( const BT * u : { t->left() , t->right() } )
  if( u ) c += ( u->read() % 2 == 0 ) + evenDescendants( u );
 return( c );
 }

/// each node gets the even proper descendants it had
void relabelEven( BT * t )
{
 if( ! t ) return;
 const int c = evenDescendants( t );
 relabelEven( t->left() );
 relabelEven( t->right() );
 t->write( c );
 }

// on ordered trees: the sum of the descendants, the relabeling by it, and
// the nodes whose parent is even

using OT = Tree< int >;

long sumDescendants( const OT * t )
{
 long s = 0;
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() )
  s += u->read() + sumDescendants( u );
 return( s );
 }

void relabelSum( OT * t )
{
 const long s = sumDescendants( t );
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() )
  relabelSum( u );
 t->write( int( s ) );
 }

int evenParent( const OT * t )
{
 int c = 0;
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() )
  c += ( t->read() % 2 == 0 ) + evenParent( u );
 return( c );
 }

// on arrays: the items above k, the membership, the largest
// item and its first place, the first copy of k removed with a 0 at the
// end, and the reversal

int greaterThan( const Ints & a , int k )
{
 return( int( std::count_if( a.begin() , a.end() ,
                             [ & ]( int x ) { return( x > k ); } ) ) );
 }

bool member( const Ints & a , int k )
{
 return( std::find( a.begin() , a.end() , k ) != a.end() );
 }

std::pair< int , int > largest( const Ints & a )
{
 int p = 0;
 for( int i = 1 ; i < int( a.size() ) ; ++i ) if( a[ i ] > a[ p ] ) p = i;
 return( std::pair( a[ p ] , p ) );
 }

void removeElem( Ints & a , int k )
{
 auto it = std::find( a.begin() , a.end() , k );
 if( it == a.end() ) return;
 std::copy( it + 1 , a.end() , it );
 a.back() = 0;
 }

void reverseArray( Ints & a )
{
 for( std::size_t i = 0 , j = a.size() ; i + 1 < j ; ++i , --j )
  std::swap( a[ i ] , a[ j - 1 ] );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the places 2 i + 1 and 2 i + 2, the copy
// of a tree under a node, the removal of a subtree, the constructor with
// two subtrees
ADS_TEST( "array-binary-tree" )
{
 ArrayBinaryTree< int > t( 1 ) , l( 2 ) , r( 3 );
 l.insertLeft( 0 , ArrayBinaryTree< int >( 4 ) );
 CHECK( l.left( 0 ) == 1 && l.read( 1 ) == 4 && l.right( 0 ) == l.NIL );
 t.insertLeft( 0 , l );
 t.insertRight( 0 , r );
 CHECK( t.preorder() == std::vector< long >( { 0 , 1 , 3 , 2 } ) );
 CHECK( t.read( 3 ) == 4 && t.parent( 3 ) == 1 && t.isLeaf( 2 ) );
 CHECK_THROWS( t.insertLeft( 0 , r ) , std::logic_error );
 t.remove( 1 );
 CHECK( t.preorder() == std::vector< long >( { 0 , 2 } ) );
 const ArrayBinaryTree< int > u( 0 , l , r );
 CHECK( u.preorder() == std::vector< long >( { 0 , 1 , 3 , 2 } ) );
 CHECK( u.read( 0 ) == 0 && u.read( 3 ) == 4 && u.read( 2 ) == 3 );
 ArrayBinaryTree< int > e;
 CHECK( e.isEmpty() && e.root() == e.NIL );
 e.setRoot( 7 );
 CHECK( e.read( e.root() ) == 7 );
 }

// the nodes with the places of parent and children, the
// copy of a subtree, the removal, and the places reused
ADS_TEST( "cursor-binary-tree" )
{
 CursorBinaryTree< int > t( 1 );
 const int a = t.insertLeft( t.root() , 2 ) ,
           b = t.insertRight( t.root() , 3 );
 const int c = t.insertLeft( a , 4 );
 CHECK( t.parent( c ) == a && t.left( a ) == c && t.right( a ) == t.NIL );
 CHECK( t.size() == 4 && t.isLeaf( b ) );
 CursorBinaryTree< int > s( 9 );
 const int x = s.insertRight( s.root() , t , a );   // copy of 2 ( 4 )
 CHECK( s.read( x ) == 2 && s.read( s.left( x ) ) == 4 && s.size() == 3 );
 t.remove( a );
 CHECK( t.size() == 2 && t.left( t.root() ) == t.NIL );
 const int y = t.insertLeft( t.root() , 5 );
 CHECK( ( y == a ) || ( y == c ) );               // a free place reused
 Ints v;
 for( int n : t.preorder() ) v.push_back( t.read( n ) );
 CHECK( v == Ints( { 1 , 5 , 3 } ) );
 }

// first child and next sibling by places, the copies of
// subtrees, the visits against the Tree with pointers
ADS_TEST( "cursor-tree" )
{
 CursorTree< int > t( 0 );
 const int a = t.insertChild( t.root() , 1 );
 const int b = t.insertSibling( a , 2 );
 t.insertChild( a , 3 );
 t.insertSibling( t.leftmostChild( a ) , 4 );
 auto values = [ & ]( const std::vector< int > & N ) {
  Ints v;
  for( int n : N ) v.push_back( t.read( n ) );
  return( v );
  };
 CHECK( values( t.previsit() ) == Ints( { 0 , 1 , 3 , 4 , 2 } ) );
 CHECK( values( t.postvisit() ) == Ints( { 3 , 4 , 1 , 2 , 0 } ) );
 CHECK( values( t.invisit( 1 ) ) == Ints( { 3 , 1 , 4 , 0 , 2 } ) );
 CHECK( t.isLastSibling( b ) && ! t.isLeaf( a ) );
 CursorTree< int > s( 9 );
 s.insertChildTree( s.root() , t , a );
 s.insertSiblingTree( s.leftmostChild( s.root() ) , t , b );
 Ints v;
 for( int n : s.previsit() ) v.push_back( s.read( n ) );
 CHECK( v == Ints( { 9 , 1 , 3 , 4 , 2 } ) && s.size() == 5 );
 t.remove( a );
 CHECK( values( t.previsit() ) == Ints( { 0 , 2 } ) && t.size() == 2 );
 CHECK_THROWS( t.insertSibling( t.root() , 5 ) , std::logic_error );
 }

// the tree in preorder against the tree with cursors, on
// random insertions and removals
ADS_TEST( "preorder-array-tree" )
{
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 50 ; ++r ) {
  PreorderArrayTree< int > p( 0 );
  CursorTree< int > c( 0 );
  std::map< int , int > at;               // value -> place in c
  at[ 0 ] = c.root();
  int next = 1;
  for( int op = 0 ; op < 40 ; ++op ) {
   std::vector< int > vals;
   for( auto [ v , n ] : at ) vals.push_back( v );
   const int v = vals[ g() % vals.size() ];
   int i = 0;                             // the place of v in p
   while( p.read( i ) != v ) ++i;
   const int kind = int( g() % 4 );
   if( ( kind == 3 ) && ( v != 0 ) ) {    // remove the subtree of v
    std::vector< int > gone;
    for( int j = i ; j < p.end( i ) ; ++j ) gone.push_back( p.read( j ) );
    p.remove( i );
    c.remove( at[ v ] );
    for( int w : gone ) at.erase( w );
    }
   else if( ( kind == 2 ) && ( v != 0 ) ) {
    p.insertSibling( i , PreorderArrayTree< int >( next ) );
    at[ next ] = c.insertSibling( at[ v ] , next );
    ++next;
    }
   else {
    p.insertChild( i , PreorderArrayTree< int >( next ) );
    at[ next ] = c.insertChild( at[ v ] , next );
    ++next;
    }
   Ints a , b;
   for( int j = 0 ; j < p.size() ; ++j ) a.push_back( p.read( j ) );
   for( int n : c.previsit() ) b.push_back( c.read( n ) );
   CHECK( a == b );
   for( int j = 0 ; j < p.size() ; ++j ) {
    const int f = p.leftmostChild( j ) , s = p.rightSibling( j );
    const int cf = c.leftmostChild( at[ p.read( j ) ] ) ,
              cs = c.rightSibling( at[ p.read( j ) ] );
    CHECK( ( f < 0 ) == ( cf < 0 ) && ( s < 0 ) == ( cs < 0 ) );
    if( f >= 0 ) CHECK( p.read( f ) == c.read( cf ) );
    if( s >= 0 ) CHECK( p.read( s ) == c.read( cs ) );
    }
   }
  }
 }

// 5 , 3 , 9 , 2 , 10 come out as 2 , 3 , 5 , 9 , 10,
// and random sequences against sorting
ADS_TEST( "linked-heap" )
{
 LinkedHeapPriorityQueue< int > Q;
 for( int x : { 5 , 3 , 9 , 2 , 10 } ) Q.insert( x , x );
 Ints out;
 while( ! Q.isEmpty() ) out.push_back( Q.deleteMin() );
 CHECK( out == Ints( { 2 , 3 , 5 , 9 , 10 } ) );
 CHECK_THROWS( Q.min() , std::logic_error );
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 100 ; ++t ) {
  LinkedHeapPriorityQueue< int > H;
  std::multiset< int > M;
  for( int op = 0 ; op < 200 ; ++op )
   if( M.empty() || g() % 3 ) {
    const int x = int( g() % 50 );
    H.insert( x , x );
    M.insert( x );
    }
   else {
    CHECK( H.minPriority() == *M.begin() && H.deleteMin() == *M.begin() );
    M.erase( M.begin() );
    }
  CHECK( H.size() == int( M.size() ) );
  }
 }

// the degrees, the isolated deletion refused,
// the forced one, and the visits
ADS_TEST( "pointer-graph" )
{
 PointerGraph< char > G;
 auto a = G.insertNode( 'a' ) , b = G.insertNode( 'b' ) ,
      c = G.insertNode( 'c' ) , d = G.insertNode( 'd' );
 G.insertEdge( a , b );
 G.insertEdge( a , c );
 G.insertEdge( b , d );
 G.insertEdge( c , d );
 G.insertEdge( d , a , 7 );
 CHECK( a->degree == 3 && d->degree == 3 && G.hasEdge( d , a ) );
 CHECK( G.find( 'c' ) == c && ! G.find( 'z' ) );
 std::string s;
 G.dfs( a , [ & ]( auto * u ) { s += u->label; } );
 CHECK( s == "abdc" );
 s.clear();
 G.bfs( a , [ & ]( auto * u ) { s += u->label; } );
 CHECK( s == "abcd" );
 CHECK( ! G.deleteNode( d ) );
 G.forceDeleteNode( d );
 CHECK( G.n() == 3 && a->degree == 2 && b->degree == 1 && c->degree == 1 );
 G.deleteEdge( a , b );
 CHECK( b->degree == 0 && G.deleteNode( b ) && G.n() == 2 );
 }

// k stacks in one vector against k stacks apart
ADS_TEST( "multistack" )
{
 std::mt19937 g( 3 );
 const int k = 4;
 MultiStack< int > M( k );
 std::vector< Ints > R( k );
 for( int op = 0 ; op < 2000 ; ++op ) {
  const int i = int( g() % k );
  if( R[ i ].empty() || g() % 2 ) {
   M.push( i , op );
   R[ i ].push_back( op );
   }
  else {
   CHECK( M.top( i ) == R[ i ].back() && M.pop( i ) == R[ i ].back() );
   R[ i ].pop_back();
   }
  CHECK( M.size( i ) == int( R[ i ].size() ) );
  }
 CHECK_THROWS( M.push( k , 0 ) , std::out_of_range );
 CHECK_THROWS( MultiStack< int >( 0 ) , std::invalid_argument );
 }

// the deep copies of binary and ordered trees, and join()
ADS_TEST( "deep-copy" )
{
 auto l = std::make_unique< BT >( 2 );
 l->insertLeft( new BT( 4 ) );
 auto r = std::make_unique< BT >( 3 );
 std::unique_ptr< BT > t( join( 1 , l.get() , r.get() ) );
 CHECK( pre_values( t.get() ) == Ints( { 1 , 2 , 4 , 3 } ) );
 std::unique_ptr< BT > u( copy( t.get() ) );
 u->left()->write( 8 );
 CHECK( pre_values( t.get() ) == Ints( { 1 , 2 , 4 , 3 } ) );
 CHECK( pre_values( u.get() ) == Ints( { 1 , 8 , 4 , 3 } ) );
 auto o = std::make_unique< OT >( 0 );
 o->insertChild( new OT( 2 ) );
 o->insertChild( new OT( 1 ) );
 o->leftmostChild()->insertChild( new OT( 3 ) );
 std::unique_ptr< OT > c( copy( o.get() ) );
 Ints v;
 pre_visit( c.get() , [ & ]( OT * x ) { v.push_back( x->read() ); } );
 CHECK( v == Ints( { 0 , 1 , 3 , 2 } ) );
 c->write( 9 );
 CHECK( o->read() == 0 );
 }

// contains() on a queue and == on a sequence
ADS_TEST( "queue-contains-sequence-equal" )
{
 ArrayQueue< int > Q( 3 );
 Q.enqueue( 1 );
 Q.enqueue( 2 );
 Q.dequeue();
 Q.enqueue( 3 );
 Q.enqueue( 4 );                          // wraps around
 CHECK( Q.contains( 4 ) && Q.contains( 2 ) && ! Q.contains( 1 ) );
 ArraySequence< int > a = { 1 , 2 , 3 } , b = { 1 , 2 , 3 } , c = { 1 , 2 };
 CHECK( a == b && !( a == c ) );
 b.write( 2 , 9 );
 CHECK( !( a == b ) );
 }

// the inspection i ( i + 1 ) / 2 visits every place
// of a table of size 2^p; the double hashing with a second function; and
// the hash h = 5 , h = 3 h + c
ADS_TEST( "hashing" )
{
 for( std::size_t m : { 1u , 2u , 8u , 64u } )
  for( std::size_t h : { 0u , 5u , 77u } ) {
   std::set< std::size_t > S;
   for( std::size_t i = 0 ; i < m ; ++i )
    S.insert( Probing::triangular().at( h , i , m ) );
   CHECK( S.size() == m );
   }
 CHECK_THROWS( Probing::triangular().check( 12 ) , std::invalid_argument );
 const auto g = []( std::size_t h ) { return( hash_midsquare( h , 16 ) ); };
 for( auto P : { Probing::triangular() , Probing::double_hashing( g ) } ) {
  OpenHashTable< int , int > T( 64 , P );
  for( int k = 0 ; k < 50 ; ++k ) T.insert( k * 37 , k );
  for( int k = 0 ; k < 50 ; ++k ) CHECK( T.lookup( k * 37 ) == k );
  CHECK( ! T.lookup( 1 ) );
  }
 std::uint64_t h = 5;
 for( unsigned char ch : std::string( "abc" ) ) h = 3 * h + ch;
 CHECK( hash_division( "abc" , 1000003 , 3 , 5 ) == h % 1000003 );
 }

// the product of rectangular matrices, the transpose, the
// scalar product
ADS_TEST( "rectangular-matrix" )
{
 const Matrix< long > A = { { 1 , 2 , 3 } , { 4 , 5 , 6 } };
 const Matrix< long > B = { { 7 , 8 } , { 9 , 10 } , { 11 , 12 } };
 CHECK( multiply( A , B ) == Matrix< long >( { { 58 , 64 } ,
                                               { 139 , 154 } } ) );
 CHECK( transpose( multiply( A , B ) ) ==
        multiply( transpose( B ) , transpose( A ) ) );
 CHECK( scale( A , 2L )[ 1 ][ 2 ] == 12 && transpose( A ).size() == 3 );
 }

// search() and searchOrdered() on 1 3 5 7 9
ADS_TEST( "sequence-search" )
{
 const auto l = list_of< int >( { 1 , 3 , 5 , 7 , 9 } );
 CHECK( search( l , 3 ) == 1 && search( l , 4 ) == -6 );
 CHECK( searchOrdered( l , 3 ) == 1 && searchOrdered( l , 4 ) == -3 );
 CHECK( searchOrdered( l , 10 ) == -5 && search( List< int >() , 1 ) == -1 );
 }

// the order of a sequence
ADS_TEST( "sequence-order" )
{
 CHECK( order( List< int >() ) == Order::EMPTY );
 CHECK( order( list_of< int >( { 4 } ) ) == Order::CONSTANT );
 CHECK( order( list_of< int >( { 2 , 2 } ) ) == Order::CONSTANT );
 CHECK( order( list_of< int >( { 1 , 2 , 5 } ) ) == Order::INCREASING );
 CHECK( order( list_of< int >( { 5 , 2 , 1 } ) ) == Order::DECREASING );
 CHECK( order( list_of< int >( { 1 , 2 , 2 } ) ) == Order::NONDECREASING );
 CHECK( order( list_of< int >( { 3 , 3 , 1 } ) ) == Order::NONINCREASING );
 CHECK( order( list_of< int >( { 1 , 3 , 2 } ) ) == Order::NONE );
 }

// the natural merge sort and the bubble sort on six names and on random
// sequences, on both realizations
ADS_TEST( "sequence-sorting" )
{
 auto names = list_of< std::string >( { "melissa" , "marta" , "luigi" ,
                                        "mirko" , "antonio" , "gnomo" } );
 naturalMergeSort( names );
 CHECK( items( names ) == std::vector< std::string >( { "antonio" , "gnomo" ,
        "luigi" , "marta" , "melissa" , "mirko" } ) );
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints v( g() % 30 );
  for( auto & x : v ) x = int( g() % 20 );
  Ints s = v;
  std::sort( s.begin() , s.end() );
  auto l1 = list_of( v ) , l2 = list_of( v );
  ArraySequence< int > a;
  for( int x : v ) a.insert( a.next( a.tail() ) , x );
  long passes = 0;
  naturalMergeSort( l1 , &passes );
  bubbleSort( l2 );
  naturalMergeSort( a );
  CHECK( items( l1 ) == s && items( l2 ) == s && items( a ) == s );
  long runs = v.empty() ? 0 : 1;
  for( std::size_t i = 1 ; i < v.size() ; ++i ) runs += v[ i ] < v[ i - 1 ];
  long bound = 0;
  for( long r = runs ; r > 1 ; r = ( r + 1 ) / 2 ) ++bound;
  CHECK( passes <= bound );
  }
 }

// the merge that keeps the copies, and the
// first copy of each item kept
ADS_TEST( "sequence-merge-dedup" )
{
 const auto m = mergeSorted( list_of< int >( { 1 , 3 , 3 , 8 } ) ,
                             list_of< int >( { 2 , 3 , 9 } ) );
 CHECK( items( m ) == Ints( { 1 , 2 , 3 , 3 , 3 , 8 , 9 } ) );
 auto l = list_of< int >( { 4 , 1 , 4 , 2 , 1 , 4 , 3 } );
 removeDuplicates( l );
 CHECK( items( l ) == Ints( { 4 , 1 , 2 , 3 } ) );
 }

// the palindromes, the sorting of a stack with its own operations, and
// removeGreater()
ADS_TEST( "stack-algorithms" )
{
 CHECK( isPalindrome( "acbca" ) && isPalindrome( "acbbca" ) );
 CHECK( ! isPalindrome( "acbde" ) && isPalindrome( "" ) );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 50 ; ++t ) {
  SequenceStack< List< int > > S;
  Ints v( g() % 15 );
  for( auto & x : v ) {
   x = int( g() % 10 );
   S.push( x );
   }
  sortStack( S );
  Ints out;
  while( ! S.isEmpty() ) out.push_back( S.pop() );
  std::sort( v.begin() , v.end() );
  CHECK( out == v );                      // the least on top
  }
 SequenceStack< List< int > > S;
 for( int x : { 5 , 1 , 7 , 3 , 9 , 2 } ) S.push( x );
 removeGreater( S , 4 );
 Ints out;
 while( ! S.isEmpty() ) out.push_back( S.pop() );
 CHECK( out == Ints( { 2 , 3 , 1 } ) );
 }

// "on" in "colonna" at 3, the failure
// function, and the two against each other on random strings
ADS_TEST( "string-matching" )
{
 const std::vector< std::size_t > at3 = { 3 };
 CHECK( naiveMatching( "colonna" , "on" ) == at3 );
 CHECK( kmp( "colonna" , "on" ) == at3 );
 CHECK( failure( "abab" ) == std::vector< long >( { -1 , 0 , 0 , 1 , 2 } ) );
 CHECK( kmp( "abc" , "" ).size() == 4 && kmp( "ab" , "abc" ).empty() );
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::string T( g() % 40 , 'a' ) , P( 1 + g() % 4 , 'a' );
  for( auto & c : T ) c = "ab"[ g() % 2 ];
  for( auto & c : P ) c = "ab"[ g() % 2 ];
  long comparisons = 0;
  CHECK( kmp( T , P , &comparisons ) == naiveMatching( T , P ) );
  CHECK( comparisons <= 2 * long( T.size() ) );
  }
 }

// freq(), hist(), remp() and del()
ADS_TEST( "list-exercises" )
{
 const auto l = list_of< int >( { 4 , 7 , 4 , 6 , 9 , 7 , 4 } );
 CHECK( freq( l , 2 ) == 4 && freq( l , 7 ) == 2 );
 const std::vector< std::pair< int , int > > H = { { 4 , 3 } , { 7 , 2 } ,
                                                    { 6 , 1 } , { 9 , 1 } };
 CHECK( hist( l ) == H );
 auto r = l;
 remp( r );                               // 4 before 7, 6 before 9, 9 before 7
 CHECK( items( r ) == Ints( { 7 , 4 , 7 , 4 } ) );
 auto d = list_of< int >( { 1 , 3 , 6 , 5 , 10 , 7 } );
 del( d );                                // 1 at 1, 6 at 3, 10 at 5
 CHECK( items( d ) == Ints( { 3 , 5 , 7 } ) );
 }

// the strings of even and odd length
ADS_TEST( "even-odd-strings" )
{
 EvenOddStrings E;
 for( const char * s : { "esame" , "algoritmi" , "e" , "strutture" , "dati" } )
  E.insert( s );
 CHECK( E.evenCount() == 1 );
 CHECK( E.oddByLength() == std::vector< std::string >( { "e" , "esame" ,
        "algoritmi" , "strutture" } ) );
 E.insert( "esame" );
 E.removeAll( "esame" );
 CHECK( E.oddByLength() == std::vector< std::string >( { "e" ,
        "algoritmi" , "strutture" } ) );
 E.removeStartingWith( 'd' );
 CHECK( E.evenCount() == 0 && E.even().empty() );
 }

// the exercises on the tree 0 ( 1 ( 3 , 4 ) , 2 ( 5 , 6 ) ) and on a
// chain
ADS_TEST( "binary-tree-exercises" )
{
 std::unique_ptr< BT > t( new BT( 0 ) );
 t->insertLeft( new BT( 1 ) );
 t->insertRight( new BT( 2 ) );
 t->left()->insertLeft( new BT( 3 ) );
 t->left()->insertRight( new BT( 4 ) );
 t->right()->insertLeft( new BT( 5 ) );
 t->right()->insertRight( new BT( 6 ) );
 CHECK( sumNodes( t.get() ) == 21 && maxNode( t.get() ) == 6 );
 CHECK( maxPathCost( t.get() ) == 8 && heightNodes( t.get() ) == 3 );
 CHECK( heightBalanced( t.get() ) );
 CHECK( oddAtLevel( t.get() , 1 ) == 1 && oddAtLevel( t.get() , 2 ) == 2 );
 CHECK( evenLeaves( t.get() ) == 2 && evenDescendants( t.get() ) == 3 );
 CHECK( evenDescendants( t->left() ) == 1 );
 relabelEven( t.get() );
 CHECK( pre_values( t.get() ) == Ints( { 3 , 1 , 0 , 0 , 1 , 0 , 0 } ) );
 std::unique_ptr< BT > c( new BT( 1 ) );   // a chain is not balanced
 c->insertLeft( new BT( 2 ) );
 c->left()->insertLeft( new BT( 3 ) );
 CHECK( ! heightBalanced( c.get() ) && heightNodes( c.get() ) == 3 );
 }

// the sum of the descendants, the relabeling, the nodes of
// even parent, on 2 ( 3 ( 5 ) , 4 , 6 ( 7 , 8 ) )
ADS_TEST( "tree-exercises" )
{
 std::unique_ptr< OT > t( new OT( 2 ) );
 auto six = new OT( 6 );
 six->insertChild( new OT( 8 ) );
 six->insertChild( new OT( 7 ) );
 t->insertChild( six );
 t->insertChild( new OT( 4 ) );
 auto three = new OT( 3 );
 three->insertChild( new OT( 5 ) );
 t->insertChild( three );
 CHECK( sumDescendants( t.get() ) == 33 );
 CHECK( evenParent( t.get() ) == 5 );      // 3 , 4 , 6 of 2 , 7 , 8 of 6
 relabelSum( t.get() );
 Ints v;
 pre_visit( t.get() , [ & ]( OT * x ) { v.push_back( x->read() ); } );
 CHECK( v == Ints( { 33 , 5 , 0 , 0 , 15 , 0 , 0 } ) );
 }

// the functions on arrays
ADS_TEST( "array-exercises" )
{
 Ints a = { 3 , 9 , 1 , 9 , 4 };
 CHECK( greaterThan( a , 3 ) == 3 && member( a , 4 ) && ! member( a , 5 ) );
 CHECK( largest( a ) == std::pair( 9 , 1 ) );
 removeElem( a , 9 );
 CHECK( a == Ints( { 3 , 1 , 9 , 4 , 0 } ) );
 reverseArray( a );
 CHECK( a == Ints( { 0 , 4 , 9 , 1 , 3 } ) );
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
/*-------------------------- End File beyond.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
