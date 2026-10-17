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

#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/algorithm/Paradigms.h"
#include "ADS/backtrack/NQueens.h"
#include "ADS/complexity/Problems.h"
#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/HashFunctions.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/PointerGraph.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/intractable/BranchAndBound.h"
#include "ADS/intractable/FifteenPuzzle.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/numeric/Matrix.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/Knapsack.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/LinkedQueue.h"
#include "ADS/queue/QueueAlgorithms.h"
#include "ADS/queue/UniqueQueue.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/search/Minimum.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/SequenceAlgorithms.h"
#include "ADS/sequence/SequenceRange.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/ShellSort.h"
#include "ADS/sort/Sorted.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/Expressions.h"
#include "ADS/stack/MultiStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/stack/StackAlgorithms.h"
#include "ADS/string/StringMatching.h"
#include "ADS/tree/ArrayBinaryTree.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/ChildListTree.h"
#include "ADS/tree/CursorBinaryTree.h"
#include "ADS/tree/CursorTree.h"
#include "ADS/tree/ExpressionTree.h"
#include "ADS/tree/PreorderArrayTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
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
    for( int j = i ; j < p.subtreeEnd( i ) ; ++j )
     gone.push_back( p.read( j ) );
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

// the tree with lists of children against the tree with cursors, on
// random insertions and removals: the same visits, first children and
// next siblings
ADS_TEST( "child-list-tree" )
{
 std::mt19937 g( 14 );
 for( int r = 0 ; r < 50 ; ++r ) {
  ChildListTree< int > l( 0 );
  CursorTree< int > c( 0 );
  std::map< int , std::pair< int , int > > at;   // value -> places
  at[ 0 ] = { l.root() , c.root() };
  int next = 1;
  for( int op = 0 ; op < 40 ; ++op ) {
   auto it = at.begin();
   std::advance( it , g() % at.size() );
   const int v = it->first;
   const auto [ nl , nc ] = it->second;
   const int kind = int( g() % 4 );
   if( ( kind == 3 ) && ( v != 0 ) ) {
    std::vector< int > gone;                // the values below v
    std::function< void( int ) > down = [ & ]( int n ) {
     gone.push_back( c.read( n ) );
     for( int m = c.leftmostChild( n ) ; m != c.NIL ;
          m = c.rightSibling( m ) )
      down( m );
     };
    down( nc );
    l.remove( nl );
    c.remove( nc );
    for( int w : gone ) at.erase( w );
    }
   else if( ( kind == 2 ) && ( v != 0 ) ) {
    at[ next ] = { l.insertSibling( nl , next ) ,
                   c.insertSibling( nc , next ) };
    ++next;
    }
   else {
    at[ next ] = { l.insertChild( nl , next ) , c.insertChild( nc , next ) };
    ++next;
    }
   auto values = []( const auto & t , const std::vector< int > & N ) {
    Ints w;
    for( int n : N ) w.push_back( t.read( n ) );
    return( w );
    };
   CHECK( values( l , l.previsit() ) == values( c , c.previsit() ) );
   CHECK( values( l , l.postvisit() ) == values( c , c.postvisit() ) );
   CHECK( values( l , l.invisit( 1 ) ) == values( c , c.invisit( 1 ) ) );
   CHECK( l.size() == c.size() );
   for( auto [ w , p ] : at ) {
    const int fl = l.leftmostChild( p.first ) ,
              fc = c.leftmostChild( p.second );
    const int sl = l.rightSibling( p.first ) ,
              sc = c.rightSibling( p.second );
    CHECK( ( fl < 0 ) == ( fc < 0 ) && ( sl < 0 ) == ( sc < 0 ) );
    if( fl >= 0 ) CHECK( l.read( fl ) == c.read( fc ) );
    if( sl >= 0 ) CHECK( l.read( sl ) == c.read( sc ) );
    }
   }
  }
 ChildListTree< int > t( 0 );
 CHECK_THROWS( t.insertSibling( t.root() , 1 ) , contract_violation );
 CHECK_THROWS( (void) t.read( 5 ) , contract_violation );
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
 CHECK_THROWS( M.push( k , 0 ) , contract_violation );
 CHECK_THROWS( MultiStack< int >( 0 ) , contract_violation );
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
 CHECK_THROWS( Probing::triangular().check( 12 ) , contract_violation );
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
 for( std::size_t m : { 1u , 7u , 12u , 64u } )   // pseudo-random
  for( std::size_t h0 : { 0u , 5u , 77u } ) {
   std::set< std::size_t > S;
   const auto P = Probing::pseudo_random( 3 );
   for( std::size_t i = 0 ; i < m ; ++i )
    S.insert( P.at( h0 , i , m ) );
   CHECK( S.size() == m && P.at( h0 , 0 , m ) == h0 % m );
   }
 }

// the examples of the hash functions on names: the hash of the first
// letter on 26 places, a collision resolved by the linear inspection; the
// keys coded with 6 bits a character (a = 1 , ... , z = 26, the space 32)
// on 6 characters and 4 more bits of zeros, 40 bits, where the extraction
// of the bits 15 to 22 has a collision that the XOR of the 5 bytes and the
// division of the 36 bits by 383 do not have
ADS_TEST( "hash-examples" )
{
 struct First {
  std::size_t operator()( const std::string & k ) const {
   return( std::size_t( k[ 0 ] - 'a' ) );
   }
  };
 OpenHashTable< std::string , int , First > T( 26 , Probing::linear() );
 const std::vector< std::string > K = { "albinoni" , "offenbach" ,
  "palestrina" , "puccini" , "prokofev" , "rossini" };
 for( const auto & k : K ) T.insert( k , 0 );
 const std::vector< std::size_t > at = { 0 , 14 , 15 , 16 , 17 , 18 };
 for( std::size_t i = 0 ; i < K.size() ; ++i )
  CHECK( T.keyAt( at[ i ] ) == K[ i ] );        // rossini overflows to 18
 ChainedHashTable< std::string , int , Chain::SORTED , First > C( 26 );
 for( const auto & k : K ) C.insert( k , 0 );
 CHECK( C.chain( 15 ) == std::vector< std::string >( { "palestrina" ,
                                                       "prokofev" ,
                                                       "puccini" } ) );
 auto code = []( std::string k ) {
  k.resize( 6 , ' ' );
  std::uint64_t v = 0;
  for( char c : k ) v = 64 * v + ( c == ' ' ? 32 : c - 'a' + 1 );
  return( v );                                   // 36 bits
  };
 auto extraction = [ & ]( const std::string & k ) {
  return( ( code( k ) << 4 ) >> ( 40 - 22 ) & 0xFF );
  };
 auto xor5 = [ & ]( const std::string & k ) {
  std::uint64_t v = code( k ) << 4 , x = 0;
  for( int i = 0 ; i < 5 ; ++i ) x ^= ( v >> ( 8 * i ) ) & 0xFF;
  return( x );
  };
 CHECK( code( "weber" ) == 24780493984ULL && code( "webern" ) ==
        24780493966ULL );
 CHECK( extraction( "weber" ) == 33 && extraction( "webern" ) == 33 );
 CHECK( xor5( "weber" ) == 195 && xor5( "webern" ) == 33 );
 CHECK( code( "weber" ) % 383 == 260 && code( "webern" ) % 383 == 242 );
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

// the lists with cursors: three lists in one space of 10 cells, the free
// list giving its first cell to an insertion and getting back the cell of
// a removal, the space doubling when no cell is free; random operations
// against std::list
ADS_TEST( "cursor-list" )
{
 auto S = std::make_shared< CursorSpace< int > >( 10 );
 CursorList< int > l( S ) , m( S ) , s( S );
 for( int x : { 7 , 2 } ) l.insert( l.next( l.tail() ) , x );
 for( int x : { 4 , 9 , 13 } ) m.insert( m.next( m.tail() ) , x );
 for( int x : { 13 , 4 , 9 , 13 } ) s.insert( s.next( s.tail() ) , x );
 CHECK( items( l ) == Ints( { 7 , 2 } ) && items( m ) == Ints( { 4 , 9 , 13 } )
        && items( s ) == Ints( { 13 , 4 , 9 , 13 } ) );
 CHECK( S->used() == 9 && S->capacity() == 10 );
 m.remove( m.next( m.head() ) );                // 9 out, its cell freed
 CHECK( items( m ) == Ints( { 4 , 13 } ) && S->used() == 8 );
 const long freed = s.first();
 s.remove( s.head() );
 l.insert( l.head() , 46 );                     // takes the cell just freed
 CHECK( l.first() == freed && l.read( l.head() ) == 46 );
 for( int i = 0 ; i < 5 ; ++i ) s.insert( s.head() , i );   // grows
 CHECK( S->capacity() == 20 && items( l ) == Ints( { 46 , 7 , 2 } ) );
 std::mt19937 g( 13 );
 CursorList< int > A;
 std::list< int > R;
 for( int op = 0 ; op < 2000 ; ++op ) {
  const int k = R.empty() ? 0 : int( g() % ( R.size() + 1 ) );
  auto pa = A.head();
  auto pr = R.begin();
  for( int i = 0 ; i < k ; ++i , pa = A.next( pa ) , ++pr ) {}
  if( ( g() % 3 ) || R.empty() ) {
   A.insert( pa , op );
   R.insert( pr , op );
   }
  else if( pr != R.end() ) {
   A.remove( pa );
   R.erase( pr );
   }
  }
 CHECK( items( A ) == Ints( R.begin() , R.end() ) );
 CHECK( A.space().used() == long( R.size() ) );
 }

// the search in a sorted list that gives the position where the item goes,
// the insertion there keeping the list sorted; the sublists; the natural
// merge sort on a list of 6 runs, merged in 3 passes
ADS_TEST( "sorted-list" )
{
 List< std::string > L = { "anna" , "antonio" , "carlo" , "davide" ,
                           "elena" , "filippo" , "giulio" };
 auto [ p , found ] = lowerBound( L , std::string( "dario" ) );
 CHECK( ! found && L.read( p ) == "davide" );
 L.insert( p , "dario" );
 CHECK( order( L ) == Order::INCREASING && lowerBound( L , std::string(
        "dario" ) ).second );
 CHECK( L.finished( lowerBound( L , std::string( "zeno" ) ).first ) );
 const auto a = L.next( L.head() ) , b = L.next( L.next( a ) );
 CHECK( items( sublist( L , a , b ) ) ==
        std::vector< std::string >( { "antonio" , "carlo" , "dario" } ) );
 CHECK( sublist( L , L.next( L.tail() ) , L.tail() ).isEmpty() );
 CHECK( items( sublist( L , a , a ) ) == std::vector< std::string >(
        { "antonio" } ) );
 List< int > N = { 82 , 16 , 14 , 15 , 84 , 25 , 77 , 13 , 75 , 4 };
 long passes = 0;
 naturalMergeSort( N , &passes );
 CHECK( passes == 3 && order( N ) == Order::INCREASING );
 }

// the decision tree of the binary search on 18 sorted names: the middle
// item at the root, the path of a search being the trace of
// binarySearch(), the height floor( log2 18 ) = 4, and the invisit giving
// the names back (the audit)
ADS_TEST( "decision-tree" )
{
 const std::vector< std::string > A = { "ALCESTE" , "ALDO" , "ALFIO" ,
  "ARMANDO" , "BALDOVINO" , "CARLO" , "CLORINDA" , "DARIO" , "ERMINIA" ,
  "EUSTACHIO" , "GOFFREDO" , "GUGLIELMO" , "MARCO" , "MARIO" , "RAIMONDO" ,
  "ROBERTO" , "TANCREDI" , "UGO" };
 std::unique_ptr< BinaryTree< std::string > > t( decisionTree( A ) );
 CHECK( t->read() == "ERMINIA" && t->left()->read() == "ARMANDO" &&
        t->right()->read() == "MARIO" );
 std::function< int( const BinaryTree< std::string > * ) > height =
  [ & ]( const BinaryTree< std::string > * u ) -> int {
   return( u ? 1 + std::max( height( u->left() ) , height( u->right() ) )
             : -1 ); };
 CHECK( height( t.get() ) == 4 );
 for( const auto & v : A ) {
  std::vector< std::string > trace , path;
  binarySearch( A , v , [ & ]( long , long , long m ) {
   trace.push_back( A[ m ] ); } );
  for( auto * u = t.get() ; ; u = v < u->read() ? u->left() : u->right() ) {
   path.push_back( u->read() );
   if( u->read() == v ) break;
   }
  CHECK( path == trace );
  if( v == "CLORINDA" )
   CHECK( path == std::vector< std::string >( { "ERMINIA" , "ARMANDO" ,
                                                "CARLO" , "CLORINDA" } ) );
  }
 CHECK( decisionTree( std::vector< int >() ) == nullptr );
 }

// the parenthesized lists of a binary and of an ordered tree, read back
// and written again; the visits of the two examples
ADS_TEST( "list-forms" )
{
 const std::string b = "( 8 () ( 5 ( 27 () () ) ( 16 () () ) ) )";
 std::unique_ptr< BinaryTree< int > > B( from_list< int >( b ) );
 CHECK( to_list( B.get() ) == b && B->right()->left()->read() == 27 );
 CHECK( from_list< int >( "()" ) == nullptr && to_list(
        static_cast< BinaryTree< int > * >( nullptr ) ) == "()" );
 CHECK_THROWS( from_list< int >( "( 8 ()" ) , contract_violation );
 using CS = std::vector< char >;
 std::unique_ptr< BinaryTree< char > > H( from_list< char >(
  "( h ( a ( d () () ) ( l ( o () () ) ( q () () ) ) ) "
  "( c ( p () () ) ( s () () ) ) )" ) );
 CS pre , in , post;
 using BC = BinaryTree< char >;
 dfs( H.get() , [ & ]( BC * u ) { pre.push_back( u->read() ); } ,
      [ & ]( BC * u ) { in.push_back( u->read() ); } ,
      [ & ]( BC * u ) { post.push_back( u->read() ); } );
 CHECK( pre == CS( { 'h' , 'a' , 'd' , 'l' , 'o' , 'q' , 'c' , 'p' , 's' } ) );
 CHECK( in == CS( { 'd' , 'a' , 'o' , 'l' , 'q' , 'h' , 'p' , 'c' , 's' } ) );
 CHECK( post ==
        CS( { 'd' , 'o' , 'q' , 'l' , 'a' , 'p' , 's' , 'c' , 'h' } ) );
 const std::string n = "( 5 ( 4 ( 8 ) ( 9 ( 12 ) ( 11 ) ( 3 ) ) ) ( 15 ) "
                       "( 6 ( 21 ) ( 20 ) ) )";
 std::unique_ptr< Tree< int > > T( tree_from_list< int >( n ) );
 CHECK( to_list( T.get() ) == n );
 Ints p , q , r;
 using TI = Tree< int >;
 pre_visit( T.get() , [ & ]( TI * u ) { p.push_back( u->read() ); } );
 post_visit( T.get() , [ & ]( TI * u ) { q.push_back( u->read() ); } );
 in_visit( T.get() , 1 , [ & ]( TI * u ) { r.push_back( u->read() ); } );
 CHECK( p == Ints( { 5 , 4 , 8 , 9 , 12 , 11 , 3 , 15 , 6 , 21 , 20 } ) );
 CHECK( q == Ints( { 8 , 12 , 11 , 3 , 9 , 4 , 15 , 21 , 20 , 6 , 5 } ) );
 CHECK( r == Ints( { 8 , 4 , 12 , 9 , 11 , 3 , 5 , 15 , 21 , 6 , 20 } ) );
 CHECK( T->leftmostChild()->leftmostChild()->isLeaf() &&
        ! T->isLeaf() && T->isLastSibling() &&
        T->leftmostChild()->rightSibling()->rightSibling()->isLastSibling() );
 }

// the expression tree from the postfix form: the prefix and the infix
// forms, the value with the variables given, against the value computed
// directly
ADS_TEST( "expression-tree" )
{
 std::unique_ptr< BinaryTree< std::string > > E( expressionTree(
  "3 x 2 ^ y * 3 x * + / z 1 - *" ) );
 CHECK( prefixForm( E.get() ) ==
        "(*(/(3)(+(*(^(x)(2))(y))(*(3)(x))))(-(z)(1)))" );
 const double x = 1.5 , y = 2 , z = 3;
 CHECK( std::abs( evaluate( E.get() , { { "x" , x } , { "y" , y } ,
                                        { "z" , z } } ) -
                  ( 3 / ( x * x * y + 3 * x ) ) * ( z - 1 ) ) < 1e-12 );
 std::unique_ptr< BinaryTree< std::string > > F( expressionTree(
  toPostfix( "2 + 3 * 4" ) ) );
 CHECK( infixForm( F.get() ) == "( 2 + ( 3 * 4 ) )" &&
        evaluate( F.get() ) == 14 );
 CHECK_THROWS( expressionTree( "1 +" ) , contract_violation );
 CHECK_THROWS( evaluate( E.get() ) , contract_violation );   // no x
 }

// the unoriented graph on the nodes a , ... , j of the example, its
// adjacencies in alphabetical order
ListGraph<> exampleGraph( void )
{
 ListGraph<> g( 10 , false );
 const std::string E[] = { "ab" , "ae" , "ah" , "bc" , "be" , "cd" , "ce" ,
                           "cf" , "eg" , "eh" , "fj" , "gj" , "hi" , "hj" };
 for( const auto & e : E ) g.insertEdge( e[ 0 ] - 'a' , e[ 1 ] - 'a' );
 return( g );
 }

// the general visit: choosing the newest node it is the visit in depth,
// with the order and the depths of the recursive one on the example, the
// oldest node the visit in breadth, whose tree gives the distances; on
// random graphs the nodes reached are those of the visits
ADS_TEST( "generic-visit" )
{
 const auto g = exampleGraph();
 const auto [ R , B ] = genericVisit( g , 0 , Choice::NEWEST );
 std::string order;
 for( int u : R ) order += char( 'a' + u );
 CHECK( order == "abcdegjfhi" && B.size() == 9 );
 std::vector< bool > seen( g.bound() , false );
 std::vector< int > depth( g.bound() , 0 );
 std::string rec;
 int level = 0;
 dfs( g , 0 , seen , [ & ]( int u ) { rec += char( 'a' + u );
                                      depth[ u ] = level++; } ,
      []( int , int ) {} , [ & ]( int ) { --level; } );
 CHECK( rec == order && depth == std::vector< int >( { 0 , 1 , 2 , 3 , 3 ,
                                                       6 , 4 , 6 , 7 , 5 } ) );
 const auto [ Rb , Bb ] = genericVisit( g , 0 , Choice::OLDEST );
 const auto [ d , p ] = distance( g , 0 );
 for( auto [ j , k ] : Bb ) CHECK( d[ k ] == d[ j ] + 1 );
 CHECK( std::is_sorted( Rb.begin() , Rb.end() , [ & ]( int u , int v ) {
  return( d[ u ] < d[ v ] ); } ) );
 std::mt19937 gen( 15 );
 for( int t = 0 ; t < 30 ; ++t ) {
  ListGraph<> h( 20 , t % 2 );
  for( int e = 0 ; e < 30 ; ++e ) {
   const int u = int( gen() % 20 ) , v = int( gen() % 20 );
   if( u != v ) h.insertEdge( u , v );
   }
  const auto [ dh , ph ] = distance( h , 0 );
  for( Choice c : { Choice::NEWEST , Choice::OLDEST } ) {
   const auto RR = genericVisit( h , 0 , c ).first;
   CHECK( long( RR.size() ) == std::count_if( dh.begin() , dh.end() ,
                                [ & ]( int x ) { return( x != INF ); } ) );
   }
  }
 }

// the shortest paths of the example: the distances and the tree of
// Dijkstra's algorithm on A , ... , G
ADS_TEST( "dijkstra-example" )
{
 ListGraph< int > g( 7 , true );
 const std::pair< std::string , int > E[] = { { "AB" , 2 } , { "AD" , 1 } ,
  { "BC" , 3 } , { "BD" , 3 } , { "DF" , 1 } , { "FC" , 2 } , { "CG" , 2 } ,
  { "FG" , 5 } };
 for( const auto & [ e , w ] : E )
  g.insertEdge( e[ 0 ] - 'A' , e[ 1 ] - 'A' , w );
 const auto R = dijkstra( g , 0 );
 CHECK( R.d == std::vector< int >( { 0 , 2 , 4 , 1 , infinity< int >() , 2 ,
                                     6 } ) );
 CHECK( R.T == std::vector< int >( { NIL , 0 , 5 , 0 , NIL , 3 , 2 } ) );
 }

// the unilateral connection against the transitive closure on random
// oriented graphs, and the example of a graph connected but not strongly;
// the adjacent nodes read back from the matrix of incidence
ADS_TEST( "unilateral-incidence" )
{
 ListGraph<> u( 4 , true );                     // u1 -> u2 -> u3 -> u4
 for( auto [ a , b ] : { std::pair( 0 , 1 ) , { 1 , 2 } , { 2 , 3 } ,
                         { 3 , 2 } } )
  u.insertEdge( a , b );
 CHECK( unilaterallyConnected( u ) );
 CHECK( scc( u )[ 0 ] != scc( u )[ 3 ] );       // no path u4 -> u1
 std::mt19937 gen( 16 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 2 + int( gen() % 6 );
  ListGraph<> h( n , true );
  for( int e = int( gen() % ( 2 * n ) ) ; e > 0 ; --e ) {
   const int a = int( gen() % n ) , b = int( gen() % n );
   if( a != b ) h.insertEdge( a , b );
   }
  std::vector< std::vector< bool > > C( n , std::vector< bool >( n ) );
  for( int a = 0 ; a < n ; ++a ) {
   C[ a ][ a ] = true;
   for( int b : h.adj( a ) ) C[ a ][ b ] = true;
   }
  for( int k = 0 ; k < n ; ++k )                // Warshall
   for( int a = 0 ; a < n ; ++a )
    for( int b = 0 ; b < n ; ++b )
     if( C[ a ][ k ] && C[ k ][ b ] ) C[ a ][ b ] = true;
  bool uni = true;
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b )
    uni = uni && ( C[ a ][ b ] || C[ b ][ a ] );
  CHECK( unilaterallyConnected( h ) == uni );
  for( bool dir : { true , false } ) {
   ListGraph<> k( n , dir );
   for( int a = 0 ; a < n ; ++a )
    for( int b : h.adj( a ) ) k.insertEdge( a , b );
   const auto Inc = incidence( k );
   for( int a = 0 ; a < n ; ++a ) {
    auto x = adjFromIncidence( Inc , a , dir ) , y = k.adj( a );
    std::sort( x.begin() , x.end() );
    std::sort( y.begin() , y.end() );
    CHECK( x == y );
    }
   }
  }
 }

// the definitions of O, Omega and Theta with their constants c and m,
// checked on [ m , N ]: 3 n^2 + 3 n - 1 <= 4 n^2 from n = 3 on and not
// at n = 2; 3 n^2 + 2 n + 1 is O( n^2 ), Omega( n ) and Theta( n^2 ), and
// not O( n ), its ratio to n going past any constant; n + 3 and
// 100 n + 3027 are both Theta( n ); the hierarchy of the orders of growth
ADS_TEST( "notations" )
{
 using F = std::function< double( double ) >;
 auto holds = []( F g , F f , double c , long m , long N ) {
  for( long n = m ; n <= N ; ++n )
   if( g( double( n ) ) > c * f( double( n ) ) ) return( false );
  return( true );
  };
 const F n1 = []( double n ) { return( n ); } ,
         n2 = []( double n ) { return( n * n ); };
 const F g1 = []( double n ) { return( 3 * n * n + 3 * n - 1 ); };
 CHECK( holds( g1 , n2 , 4 , 3 , 100000 ) && ! holds( g1 , n2 , 4 , 2 , 2 ) );
 const F g2 = []( double n ) { return( 3 * n * n + 2 * n + 1 ); };
 CHECK( holds( g2 , n2 , 6 , 1 , 100000 ) );           // O( n^2 )
 CHECK( holds( n1 , g2 , 1 , 1 , 100000 ) );           // Omega( n )
 CHECK( holds( n2 , g2 , 1 , 1 , 100000 ) );           // Theta( n^2 )
 CHECK( g2( 1e6 ) / n1( 1e6 ) > 1e6 );                 // not O( n )
 const F a = []( double n ) { return( n + 3 ); } ,
         b = []( double n ) { return( 100 * n + 3027 ); };
 CHECK( holds( a , b , 1 , 1 , 100000 ) &&
        holds( b , a , 3127 , 1 , 100000 ) );
 const std::vector< Growth > H = { { 1 , 0 } , { 1 , 0 , 1 } , { 1 , 1 } ,
  { 1 , 1 , 1 } , { 1 , 2 } , { 1 , 3 } , { 2 , 0 } };
 for( std::size_t i = 1 ; i < H.size() ; ++i )
  CHECK( H[ i - 1 ] < H[ i ] );
 CHECK( master( 2 , 2 , 1 ) == Growth( { 1 , 1 , 1 } ) );   // n log n
 }

// the model of unit costs: an assignment, an operation or a comparison
// costs 1, a for of n rounds costs 2 + ( cost of the body + 3 ) n; the
// factorial f = 1 , for i = 1 to n do f = f i costs 1 + 2 + 5 n, 18 for
// n = 3; the minimum of n items costs a n + b; the sequential search costs
// 3 + 2 n in the worst case, and makes ( n + 1 ) / 2 comparisons on average
// when the item is there; the search of each item of w in v makes N^2
// comparisons when each is found last
ADS_TEST( "cost-model" )
{
 auto factorial = []( long n , long & cost ) {
  long f = 1;
  cost = 1;                                     // f = 1
  cost += 2;                                    // i = 1 , the last test
  for( long i = 1 ; i <= n ; ++i ) {
   f = f * i;
   cost += 2 + 3;                               // the body, test and i++
   }
  return( f );
  };
 long c = 0;
 CHECK( factorial( 3 , c ) == 6 && c == 18 );
 for( long n = 0 ; n <= 10 ; ++n ) {
  factorial( n , c );
  CHECK( c == 3 + 5 * n );
  }
 auto minCost = []( const Ints & B ) {          // C1 = ... = C6 = 1
  long cost = 1 + 1;                            // the call, m = B[ j ]
  int m = B[ 0 ];
  for( std::size_t i = 1 ; i < B.size() ; ++i ) {
   cost += 1;                                   // the test of the for
   cost += 1;                                   // B[ i ] < m
   if( B[ i ] < m ) m = B[ i ];
   cost += 1;                                   // counted as C5 ( n - 1 )
   }
  return( cost + 1 + 1 );                       // the last test, return
  };
 Ints B;
 std::vector< long > T;
 for( int n = 1 ; n <= 6 ; ++n ) {
  B.push_back( 10 - n );
  T.push_back( minCost( B ) );
  }
 for( std::size_t i = 2 ; i < T.size() ; ++i )  // T( n ) = a n + b
  CHECK( T[ i ] - T[ i - 1 ] == T[ 1 ] - T[ 0 ] );
 List< int > L;
 const int n = 50;
 for( int i = 0 ; i < n ; ++i ) L.insert( L.next( L.tail() ) , i );
 long total = 0;
 for( int i = 0 ; i < n ; ++i ) total += search( L , i ) + 1;
 CHECK( total == long( n ) * ( n + 1 ) / 2 );   // ( n + 1 ) / 2 on average
 CHECK( search( L , -1 ) == -( n + 1 ) );       // n comparisons, worst
 long cmp = 0;
 Ints v( n ) , w( n );
 for( int i = 0 ; i < n ; ++i ) { v[ i ] = i; w[ i ] = n - 1 - i; }
 for( int x : w )                               // CercaInTutti
  for( int y : v ) {
   ++cmp;
   if( y == x ) break;
   }
 CHECK( cmp == long( n ) * ( n + 1 ) / 2 );
 }

// the comparisons of the merge of two sorted lists of m and n items: at
// least min( m , n ), when the shorter one comes all first, at most
// m + n - 1, and m + k - 1 when the last item of the first list goes
// before the k-th of the second one (the comparisons of the audits skipped)
struct Counted {
 int v;
 static long count;
 bool operator<( const Counted & o ) const {
  count += ! checkingContract();
  return( v < o.v );
  }
 bool operator==( const Counted & o ) const { return( v == o.v ); }
 };
long Counted::count = 0;

ADS_TEST( "merge-comparisons" )
{
 auto list = []( std::initializer_list< int > I ) {
  List< Counted > L;
  for( int x : I ) L.insert( L.next( L.tail() ) , Counted{ x } );
  return( L );
  };
 using LC = List< Counted >;
 auto merges = [ & ]( const LC & a , const LC & b ) {
  Counted::count = 0;
  (void) mergeSorted( a , b );
  return( Counted::count );
  };
 CHECK( merges( list( { 1 , 2 } ) , list( { 5 , 6 , 7 } ) ) == 2 );
 CHECK( merges( list( { 1 , 3 , 5 } ) , list( { 2 , 4 , 6 } ) ) == 5 );
 const int m = 4 , n = 6;
 for( int k = 1 ; k <= n ; ++k ) {              // a ends before b[ k ]
  List< Counted > a , b;
  for( int i = 0 ; i < n ; ++i ) b.insert( b.next( b.tail() ) ,
                                           Counted{ 10 * i } );
  for( int i = 0 ; i < m ; ++i ) a.insert( a.next( a.tail() ) ,
                                           Counted{ 10 * ( k - 1 ) - 5 + i } );
  CHECK( merges( a , b ) == m + k - 1 );
  }
 }

// KMP with the strong failure function: the occurrences of the naive
// search with no more comparisons than KMP, on the example and on random
// binary strings
ADS_TEST( "kmp-strong" )
{
 CHECK( kmpStrong( "110111011001" , "110110" ) ==
        std::vector< std::size_t >( { 4 } ) );
 const auto G = strongFailure( "110110" ) , F = failure( "110110" );
 for( std::size_t q = 0 ; q < G.size() ; ++q ) CHECK( G[ q ] <= F[ q ] );
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::string T , P;
  for( int i = int( g() % 60 ) ; i > 0 ; --i ) T += char( '0' + g() % 2 );
  for( int i = 1 + int( g() % 6 ) ; i > 0 ; --i ) P += char( '0' + g() % 2 );
  long a = 0 , b = 0;
  CHECK( kmpStrong( T , P , &a ) == kmp( T , P , &b ) );
  CHECK( a <= b );
  }
 }

// ZAINO: the greedy heuristic by profit per volume against the optimum on
// the example, and the backtracking with the simple bound against the
// branch-and-bound and the dynamic programming on random instances
ADS_TEST( "knapsack-heuristics" )
{
 const std::vector< long > p = { 6 , 4 , 7 } , v = { 2 , 1 , 3 };
 CHECK( knapsackGreedy( p , v , 5 ).first == 10 );
 CHECK( knapsackBacktrack( p , v , 5 ).first == 13 );
 std::mt19937 g( 18 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 10 );
  std::vector< long > P( n ) , V( n );
  for( int i = 0 ; i < n ; ++i ) {
   P[ i ] = long( g() % 30 );
   V[ i ] = 1 + long( g() % 15 );
   }
  const long C = long( g() % 40 );
  long nb = 0 , nk = 0;
  const auto b = knapsackBacktrack( P , V , C , &nb );
  CHECK( b.first == knapsackBB( P , V , C , &nk ).first );
  CHECK( knapsackGreedy( P , V , C ).first <= b.first );
  CHECK( nb >= 1 && nk >= 1 );
  }
 }

// the iterative backtracking of the n queens, the same numbers of
// solutions as the recursive one
ADS_TEST( "nqueens-iterative" )
{
 const long known[] = { 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 };
 for( int n = 1 ; n <= 8 ; ++n ) {
  CHECK( nQueensIterative( n , []( const std::vector< int > & ) {
   return( false ); } ) == known[ n - 1 ] );
  CHECK( nQueens( n , []( const std::vector< int > & ) {
   return( false ); } ) == known[ n - 1 ] );
  }
 }

// the sorts of the slides: Merge Sort skipping the merge of parts in
// order makes n - 1 comparisons on a sorted vector; the passes of Shell
// Sort on the example with the gaps 4, 2 and 1, and the gaps of Hibbard;
// Insertion Sort with the sentinel, 2 n - 3 comparisons on a sorted vector;
// the partition around a value; the minimum and the maximum together
ADS_TEST( "slide-sorts" )
{
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract();
  return( a < b ); };
 Ints S( 100 );
 for( int i = 0 ; i < 100 ; ++i ) S[ i ] = i;
 Ints A = S;
 mergeSort( A , 4 , count );
 CHECK( c == 99 && A == S );
 Ints H = { 20 , 35 , 18 , 8 , 14 , 41 , 3 , 39 };
 shellSort( H , std::vector< std::size_t >( { 4 } ) );
 CHECK( H == Ints( { 14 , 35 , 3 , 8 , 20 , 41 , 18 , 39 } ) );
 shellSort( H , std::vector< std::size_t >( { 2 } ) );
 CHECK( H == Ints( { 3 , 8 , 14 , 35 , 18 , 39 , 20 , 41 } ) );
 shellSort( H , std::vector< std::size_t >( { 1 } ) );
 CHECK( std::is_sorted( H.begin() , H.end() ) );
 CHECK( hibbardGaps( 100 ) == std::vector< std::size_t >( { 1 , 3 , 7 , 15 ,
                                                            31 , 63 } ) );
 c = 0;
 A = S;
 insertionSortSentinel( A , count );
 CHECK( c == 2 * 100 - 3 && A == S );
 std::mt19937 g( 19 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints B( int( g() % 30 ) );
  for( auto & x : B ) x = int( g() % 50 );
  Ints C = B;
  insertionSortSentinel( B );
  std::sort( C.begin() , C.end() );
  CHECK( B == C );
  }
 Ints P = { 4 , 13 , 28 , 18 , 7 , 36 , 11 , 24 };
 CHECK( partitionByValue( P , 0 , 7 , 20 ) == 5 );
 CHECK( partitionByValue( P , 0 , 7 , 0 ) == 0 &&
        partitionByValue( P , 0 , 7 , 99 ) == 8 );
 for( std::size_t n : { 1u , 2u , 7u , 16u , 33u } ) {
  Ints M( n );
  for( auto & x : M ) x = int( g() % 1000 );
  c = 0;
  const auto r = minMax( M , count );
  CHECK( c == long( ( 3 * n + 1 ) / 2 ) - 2 );   // ceil( 3 n / 2 ) - 2
  CHECK( r.first == *std::min_element( M.begin() , M.end() ) &&
         r.second == *std::max_element( M.begin() , M.end() ) );
  c = 0;
  const auto d = minMaxDivide( M , count );
  if( n == 16 ) CHECK( c == 3 * 16 / 2 - 2 );
  CHECK( d == r );
  }
 }

// instances of the classical examples: the selection of activities, the
// minimum spanning tree of the graph on a , ... , i, and a partition
ADS_TEST( "classical-instances" )
{
 const long s[] = { 1 , 3 , 0 , 5 , 3 , 5 , 6 , 8 , 8 , 2 , 12 } ,
            f[] = { 4 , 5 , 6 , 7 , 9 , 9 , 10 , 11 , 12 , 14 , 16 };
 std::vector< std::pair< long , long > > I;
 for( int i = 0 ; i < 11 ; ++i ) I.emplace_back( s[ i ] , f[ i ] );
 CHECK( independentSet( I ) ==
        std::vector< std::size_t >( { 0 , 3 , 7 , 10 } ) );
 std::vector< WeightedEdge< int > > E;
 const std::pair< std::string , int > W[] = { { "ab" , 4 } , { "ah" , 8 } ,
  { "bc" , 8 } , { "bh" , 11 } , { "cd" , 7 } , { "ci" , 2 } , { "cf" , 4 } ,
  { "de" , 9 } , { "df" , 14 } , { "ef" , 10 } , { "fg" , 2 } , { "gh" , 1 } ,
  { "gi" , 6 } , { "hi" , 7 } };
 for( const auto & [ e , w ] : W )
  E.push_back( { e[ 0 ] - 'a' , e[ 1 ] - 'a' , w } );
 int total = 0;
 for( const auto & e : kruskal( 9 , E ) ) total += e.w;
 CHECK( total == 37 );
 const auto P = partition( std::vector< long >( { 1 , 2 , 2 , 4 , 7 } ) );
 CHECK( P.has_value() );
 }

// the eight puzzle with the spiral goal: the visit in breadth finds the
// 5 moves of the classical start, the backtracking in depth finds a
// solution within 5 moves and none within 4
ADS_TEST( "eight-puzzle" )
{
 const Board goal = puzzleSpiralGoal( 3 );
 CHECK( goal == Board( { 1 , 2 , 3 , 8 , 0 , 4 , 7 , 6 , 5 } ) );
 CHECK( puzzleSpiralGoal( 4 ) ==
        Board( { 1 , 2 , 3 , 4 , 12 , 13 , 14 , 5 , 11 , 0 , 15 , 6 , 10 , 9 ,
                 8 , 7 } ) );
 const Board start = { 2 , 8 , 3 , 1 , 6 , 4 , 7 , 0 , 5 };
 long visited = 0;
 const auto S = puzzleBFS( start , goal , 3 , &visited );
 CHECK( S && S->size() == 6 && visited > 6 );
 const auto D5 = puzzleDFS( start , goal , 3 , 5 );
 CHECK( D5 && D5->size() <= 6 && D5->back() == goal );
 CHECK( ! puzzleDFS( start , goal , 3 , 4 ) );
 const auto A = solvePuzzle( puzzleMoves( puzzleGoal( 3 ) , 3 )[ 0 ] , 3 );
 const auto Bf = puzzleBFS( puzzleMoves( puzzleGoal( 3 ) , 3 )[ 0 ] ,
                            puzzleGoal( 3 ) , 3 );
 CHECK( A && Bf && A->size() == Bf->size() );
 }

// the general schemes: the enumerative search of an item against
// search(), the sort by enumerating the permutations until one has no
// inversion, the queens by the backtracking scheme, the knapsack by
// enumerating the subsets against the dynamic programming, the minimum and
// the maximum by divide et impera, the greedy change, the level of largest
// weight of a tree, and the maximal words for three orders
ADS_TEST( "paradigms" )
{
 const Ints V = { 7 , 3 , 9 , 3 , 1 };
 for( int v : { 3 , 1 , 8 } ) {
  const auto r = enumerateSearch( 0 , []( int i ) { return( i + 1 ); } ,
   [ & ]( int i ) { return( i + 1 == int( V.size() ) ); } ,
   [ & ]( int i ) { return( V[ i ] == v ); } ,
   []( int i ) { return( i ); } );
  const auto e = std::find( V.begin() , V.end() , v );
  CHECK( r ? ( *r == e - V.begin() ) : ( e == V.end() ) );
  }
 std::mt19937 g( 20 );
 for( int t = 0 ; t < 20 ; ++t ) {
  Ints A( int( g() % 6 ) + 1 );
  for( auto & x : A ) x = int( g() % 10 );
  Ints z( A.size() );
  std::iota( z.begin() , z.end() , 0 );
  auto apply = [ & ]( const Ints & p ) {
   Ints B;
   for( int i : p ) B.push_back( A[ i ] );
   return( B ); };
  const auto sorted = enumerateSearch( z ,
   []( Ints p ) { std::next_permutation( p.begin() , p.end() );
                  return( p ); } ,
   []( const Ints & p ) { return( std::is_sorted( p.rbegin() ,
                                                  p.rend() ) ); } ,
   [ & ]( const Ints & p ) {
    const Ints B = apply( p );
    return( std::is_sorted( B.begin() , B.end() ) ); } ,
   apply );
  Ints C = A;
  std::sort( C.begin() , C.end() );
  CHECK( sorted && *sorted == C );
  }
 for( int n = 1 ; n <= 7 ; ++n ) {
  long count = 0;
  backtrack( Ints() , [ & ]( const Ints & Q ) {
   std::vector< Ints > C;
   for( int c = 0 ; c < n ; ++c ) { Ints R = Q; R.push_back( c );
                                    C.push_back( R ); }
   return( C ); } ,
   []( const Ints & Q ) { return( ! admissible( Q ) ); } ,
   [ & ]( const Ints & Q ) { return( int( Q.size() ) == n ); } ,
   [ & ]( const Ints & ) { ++count; return( false ); } );
  CHECK( count == nQueens( n , []( const std::vector< int > & ) {
   return( false ); } ) );
  }
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 1 + int( g() % 8 );
  std::vector< long > p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) { p[ i ] = g() % 20; v[ i ] = 1 + g() % 9; }
  const long C = long( g() % 25 );
  auto vol = [ & ]( unsigned s ) { long w = 0;
   for( int i = 0 ; i < n ; ++i ) if( s >> i & 1 ) w += v[ i ];
   return( w ); };
  auto prof = [ & ]( unsigned s ) { long w = 0;
   for( int i = 0 ; i < n ; ++i ) if( s >> i & 1 ) w += p[ i ];
   return( w ); };
  const auto best = enumerateOptimize( 0u ,
   []( unsigned s ) { return( s + 1 ); } ,
   [ & ]( unsigned s ) { return( s + 1 == ( 1u << n ) ); } ,
   [ & ]( unsigned s ) { return( vol( s ) <= C ); } , prof , prof ,
   []( long a , long b ) { return( a > b ); } );
  CHECK( best && *best == knapsack( p , v , C ).first );
  }
 using MM = std::pair< int , int >;
 const Ints M = { 5 , 9 , -2 , 7 , 3 , 11 , 0 };
 const MM mm = divideEtImpera( std::pair( 0 , int( M.size() ) - 1 ) ,
  []( MM r ) { return( r.second - r.first <= 1 ); } ,
  [ & ]( MM r ) { return( std::pair( std::min( M[ r.first ] , M[ r.second ] ) ,
                          std::max( M[ r.first ] , M[ r.second ] ) ) ); } ,
  []( MM r ) { const int h = r.first + ( r.second - r.first ) / 2;
               return( std::vector< MM >( { { r.first , h } ,
                                            { h + 1 , r.second } } ) ); } ,
  []( const std::vector< MM > & P ) {
   return( std::pair( std::min( P[ 0 ].first , P[ 1 ].first ) ,
                      std::max( P[ 0 ].second , P[ 1 ].second ) ) ); } );
 CHECK( mm == MM( -2 , 11 ) );
 long rest = 63;                                   // the greedy change
 using VL = std::vector< long >;
 const auto coins = greedy< long >( 6 , [ & ]( int , const VL & ) {
   VL c;
   for( long d : { 50L , 20L , 10L , 5L , 2L , 1L } )
    if( d <= rest ) c.push_back( d );
   return( c ); } ,
  [ & ]( const VL & c ) { rest -= c[ 0 ]; return( c[ 0 ] ); } );
 CHECK( coins == std::vector< long >( { 50 , 10 , 2 , 1 } ) && rest == 0 );
 std::unique_ptr< Tree< int > > T( tree_from_list< int >(
  "( 1 ( 9 ( 2 ) ( 2 ) ) ( 3 ( 8 ) ) )" ) );
 CHECK( maxWeightLevel( T.get() ) == 1 );           // 12 > 1 , 12 = 12
 const std::vector< std::string > W = { "TORRE" , "ERRORE" , "ZERO" ,
                                        "TENORE" };
 using SV = std::vector< std::string >;
 CHECK( maximal( W , []( const std::string & a , const std::string & b ) {
  return( a > b ); } ) == SV( { "ZERO" } ) );
 CHECK( maximal( W , []( const std::string & a , const std::string & b ) {
  return( a.size() > b.size() ); } ) == SV( { "ERRORE" , "TENORE" } ) );
 auto letters = []( const std::string & a ) {
  return( std::set< char >( a.begin() , a.end() ) ); };
 CHECK( maximal( W , [ & ]( const std::string & a , const std::string & b ) {
  const auto A = letters( a ) , B = letters( b );
  return( A != B && std::includes( A.begin() , A.end() , B.begin() ,
                                   B.end() ) ); } ) ==
        SV( { "ZERO" , "TENORE" } ) );
 }

// the factorial by a loop and by the recursion, the same specification
// realized twice, up to the 20! that fits in 64 bits; the binomial by the
// factorials against the recursion; the numbers of Fibonacci kept between
// the calls, a number already known costing no new step
ADS_TEST( "factorial-fibonacci" )
{
 for( int n = 0 ; n <= 20 ; ++n )
  CHECK( factorial( n ) == factorialRec( n ) );
 CHECK( factorial( 20 ) == 2432902008176640000ULL );
 CHECK_THROWS( factorial( 21 ) , contract_violation );
 CHECK_THROWS( fibonacci( 93 ) , contract_violation );
 CHECK_THROWS( binomial( 68 , 34 ) , contract_violation );
 for( int n = 0 ; n <= 20 ; ++n )
  for( int k = 0 ; k <= n ; ++k )
   CHECK( binomialByFactorials( n , k ) == tartaglia( 20 )[ n ][ k ] );
 FibonacciCache F;
 for( int n : { 50 , 10 , 92 , 0 } ) CHECK( F.value( n ) == fibonacci( n ) );
 const int kept = F.size();
 CHECK( F.value( 50 ) == fibonacci( 50 ) && F.size() == kept && kept == 93 );
 }

// the for on a range of C++ over every realization of SEQUENCE, the same
// items as the loop on the positions
ADS_TEST( "sequence-range" )
{
 auto check = []( const auto & s ) {
  Ints a , b;
  for( const auto & x : range( s ) ) a.push_back( x );
  for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
   b.push_back( s.read( p ) );
  CHECK( a == b );
  };
 check( List< int >( { 3 , 1 , 4 , 1 , 5 } ) );
 check( ArraySequence< int >( { 9 , 2 , 6 } ) );
 check( SinglyList< int >( { 5 , 3 , 5 } ) );
 check( CursorList< int >( { 8 , 9 , 7 , 9 } ) );
 check( List< int >() );
 List< int > L = { 1 , 2 , 3 };
 static_assert( std::ranges::input_range< SequenceRange< List< int > > > );
 CHECK( std::ranges::count( range( L ) , 2 ) == 1 );   // a range of C++
 }

// a specification by axioms of the integers from zero, the successor and
// the predecessor, checked on a realization that hides the integer:
// iszero( zero ) = true, iszero( succ( n ) ) = false,
// pred( succ( n ) ) = n and succ( pred( n ) ) = n
ADS_TEST( "algebraic-spec" )
{
 class Peano {
  long f_n = 0;
  explicit Peano( long n ) : f_n( n ) {}
  public:
  static Peano zero( void ) { return( Peano( 0 ) ); }
  [[nodiscard]] Peano succ( void ) const { return( Peano( f_n + 1 ) ); }
  [[nodiscard]] Peano pred( void ) const { return( Peano( f_n - 1 ) ); }
  [[nodiscard]] bool iszero( void ) const { return( f_n == 0 ); }
  [[nodiscard]] bool operator==( const Peano & ) const = default;
  };
 CHECK( Peano::zero().iszero() );
 Peano n = Peano::zero();
 for( int i = 0 ; i < 20 ; ++i , n = n.succ() ) {
  CHECK( ! n.succ().iszero() );
  CHECK( n.succ().pred() == n && n.pred().succ() == n );
  }
 }

// the postfix expressions: their value, the conversions of an infix
// expression, fully parenthesized or with the precedences, and the value
// of the conversion against the one computed directly on random
// expressions
ADS_TEST( "postfix" )
{
 CHECK( evalPostfix( "5 9 8 + 4 6 * * 7 + *" ) == 2075 );
 CHECK( evalPostfix( "35 17 40 9 - * + 7 -" ) == 555 );
 CHECK( toPostfixParenthesized( "( 5 * ( ( ( 9 + 8 ) * ( 4 * 6 ) ) + 7 ) )" )
        == "5 9 8 + 4 6 * * 7 + *" );
 CHECK( toPostfix( "35 + 17 * ( 40 - 9 ) - 7" ) == "35 17 40 9 - * + 7 -" );
 CHECK( toPostfix( "8 - 3 - 2" ) == "8 3 - 2 -" );   // to the left
 CHECK( evalPostfix( toPostfix( "8 - 3 - 2" ) ) == 3 );
 CHECK( toPostfix( "2*(3+4)" ) == "2 3 4 + *" );     // no spaces
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 200 ; ++t ) {                // a + b * c - d / e
  const long a = g() % 50 , b = g() % 50 , c = g() % 50 , d = g() % 50 ,
             e = 1 + g() % 9;
  const std::string x = std::to_string( a ) + " + " + std::to_string( b ) +
   " * " + std::to_string( c ) + " - ( " + std::to_string( d ) + " / " +
   std::to_string( e ) + " )";
  CHECK( evalPostfix( toPostfix( x ) ) == a + b * c - d / e );
  }
 }

// the order of a STACK and of a QUEUE on the classical traces, a letter
// being pushed (or enqueued) and an asterisk popping (or dequeuing), on
// every realization
template< class S >
std::string lifo( S P , const std::string & trace )
{
 std::string out;
 for( char c : trace )
  if( c == '*' ) out += P.pop(); else P.push( c );
 CHECK( P.isEmpty() );
 return( out );
 }

template< class Q >
std::string fifo( Q q , const std::string & trace )
{
 std::string out;
 for( char c : trace )
  if( c == '*' ) out += q.dequeue(); else q.enqueue( c );
 CHECK( q.isEmpty() );
 return( out );
 }

ADS_TEST( "lifo-fifo-traces" )
{
 const std::string L = "LA*STI*N*FIR*ST**OU*T*******";
 CHECK( lifo( ArrayStack< char >( 30 ) , L ) == "AINRTSUTOIFTSL" );
 CHECK( lifo( SequenceStack< List< char > >() , L ) == "AINRTSUTOIFTSL" );
 const std::string F = "FIRS*T*IN***FI*RS***T*OUT*****";
 CHECK( fifo( LinkedQueue< char >() , F ) == "FIRSTINFIRSTOUT" );
 CHECK( fifo( ArrayQueue< char >( 8 ) , F ) == "FIRSTINFIRSTOUT" );
 }

// the items of a stack from the bottom, the stack left as it was
ADS_TEST( "stack-bottom-up" )
{
 ArrayStack< int > P( 10 );
 for( int i : { 4 , 8 , 15 , 16 } ) P.push( i );
 Ints v;
 forEachBottomUp( P , [ & ]( int x ) { v.push_back( x ); } );
 CHECK( v == Ints( { 4 , 8 , 15 , 16 } ) && P.top() == 16 );
 SequenceStack< List< int > > E;
 forEachBottomUp( E , [ & ]( int ) { CHECK( false ); } );
 }

// the positive items of a queue, the queue emptied or kept, and the queue
// with no duplicates under its two policies
ADS_TEST( "queue-algorithms" )
{
 LinkedQueue< int > q , p , r;
 for( int x : { 3 , -1 , 0 , 7 , -5 , 2 } ) q.enqueue( x );
 copyPositive( q , p );
 CHECK( q.size() == 6 && q.top() == 3 );
 extractPositive( q , r );
 CHECK( q.isEmpty() && p == r );
 Ints v;
 while( ! r.isEmpty() ) v.push_back( r.dequeue() );
 CHECK( v == Ints( { 3 , 7 , 2 } ) );
 CHECK_THROWS( copyPositive( p , p ) , contract_violation );
 UniqueQueue< int , Duplicate::IGNORE_NEW > I;
 UniqueQueue< int , Duplicate::FORGET_OLD > O;
 for( int x : { 1 , 2 , 3 , 2 } ) {
  I.enqueue( x );
  O.enqueue( x );
  }
 Ints a , b;
 while( ! I.isEmpty() ) a.push_back( I.dequeue() );
 while( ! O.isEmpty() ) b.push_back( O.dequeue() );
 CHECK( a == Ints( { 1 , 2 , 3 } ) && b == Ints( { 1 , 3 , 2 } ) );
 CHECK_THROWS( I.dequeue() , contract_violation );
 }

// the sieve of Eratosthenes on a set: the 25 primes up to 100, and the
// primes up to 10^4 as the divisions find them (the audit)
ADS_TEST( "sieve" )
{
 CHECK( sieve( 100 ).size() == 25 && sieve( 100 ).back() == 97 );
 CHECK( sieve( 1 ).empty() && sieve( 2 ) == std::vector< U64 >( { 2 } ) );
 CHECK( sieve( 10000 ).size() == 1229 );
 }

// the dictionary on a sorted vector: random operations against std::map,
// the keys always sorted
ADS_TEST( "sorted-array-dictionary" )
{
 SortedArrayDictionary< int , int > D;
 std::map< int , int > R;
 std::mt19937 g( 12 );
 for( int op = 0 ; op < 3000 ; ++op ) {
  const int k = int( g() % 200 );
  if( g() % 3 ) {
   D.insert( k , op );
   R[ k ] = op;
   }
  else {
   D.remove( k );
   R.erase( k );
   }
  const auto it = R.find( k );
  const auto r = it == R.end() ? std::optional< int >()
                               : std::optional< int >( it->second );
  CHECK( D.lookup( k ) == r );
  }
 CHECK( D.size() == R.size() );
 std::size_t i = 0;
 for( const auto & [ k , v ] : R ) {
  CHECK( D.pairs()[ i ].first == k && D.pairs()[ i ].second == v );
  ++i;
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
/*-------------------------- End File beyond.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
