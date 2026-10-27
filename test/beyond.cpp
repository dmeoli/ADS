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
// two subtrees; the edge cases: the empty tree, one node, the empty tree
// grafted, an absent node removed or read, a second root, the copy of
// the tree into itself, the removal of the root, a chain to the left
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
 CHECK( e.empty() && e.root() == e.NIL && e.preorder().empty() );
 e.setRoot( 7 );
 CHECK( e.read( e.root() ) == 7 );
 CHECK( e.isLeaf( 0 ) && e.parent( 0 ) == e.NIL );
 e.insertLeft( 0 , ArrayBinaryTree< int >() );   // the empty tree: nothing
 CHECK( e.isLeaf( 0 ) && e.preorder() == std::vector< long >( { 0 } ) );
 e.remove( 9 );                                  // an absent node: nothing
 CHECK( e.preorder() == std::vector< long >( { 0 } ) );
 CHECK_THROWS( e.setRoot( 1 ) , contract_violation );
 CHECK_THROWS( (void) e.read( 1 ) , contract_violation );
 CHECK_THROWS( (void) e.left( 3 ) , contract_violation );
 CHECK_THROWS( (void) e.parent( -1 ) , contract_violation );
 CHECK_THROWS( e.insertRight( 2 , r ) , contract_violation );
 e.insertLeft( 0 , e );                          // a copy of itself
 e.insertRight( 0 , e );
 CHECK( e.preorder() == std::vector< long >( { 0 , 1 , 2 , 5 } ) );
 CHECK( e.read( 1 ) == 7 && e.read( 5 ) == 7 && e.isLeaf( 1 ) );
 e.remove( 0 );                                  // the root: all gone
 CHECK( e.empty() && e.root() == e.NIL && e.preorder().empty() );
 ArrayBinaryTree< int > c( 0 );                  // a chain to the left
 long i = 0;
 for( int d = 1 ; d <= 12 ; ++d ) {
  c.insertLeft( i , ArrayBinaryTree< int >( d ) );
  i = c.left( i );
  }
 CHECK( i == 4095 && c.read( i ) == 12 && c.isLeaf( i ) &&
        c.preorder().size() == 13 && c.right( 0 ) == c.NIL );
 c.remove( 1 );
 CHECK( c.preorder() == std::vector< long >( { 0 } ) );
 }

// the nodes with the places of parent and children, the
// copy of a subtree, the removal, and the places reused; the edge cases:
// a child already there, a node absent or freed, a second root, the copy
// of the tree into itself, the removal of the root, a chain of 3000 nodes
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
 CHECK_THROWS( t.insertLeft( t.root() , 6 ) , contract_violation );
 CHECK_THROWS( t.insertRight( t.root() , s , s.root() ) ,
               contract_violation );
 CHECK_THROWS( (void) t.read( 99 ) , contract_violation );
 CHECK_THROWS( t.setRoot( 0 ) , contract_violation );
 t.insertLeft( y , t , t.root() );                // a copy of itself
 v.clear();
 for( int n : t.preorder() ) v.push_back( t.read( n ) );
 CHECK( v == Ints( { 1 , 5 , 1 , 5 , 3 , 3 } ) && t.size() == 6 );
 t.remove( t.root() );                            // the root: all gone
 CHECK( t.empty() && t.size() == 0 && t.root() == t.NIL &&
        t.preorder().empty() );
 CHECK_THROWS( (void) t.read( y ) , contract_violation );   // freed
 const int z = t.setRoot( 8 );
 CHECK( t.isLeaf( z ) && t.parent( z ) == t.NIL && t.size() == 1 );
 CursorBinaryTree< int > d( 0 );                  // a deep chain
 int w = d.root();
 for( int k = 1 ; k < 3000 ; ++k ) w = d.insertRight( w , k );
 CHECK( d.size() == 3000 && d.read( w ) == 2999 &&
        d.preorder().size() == 3000 );
 d.remove( d.right( d.root() ) );
 CHECK( d.size() == 1 && d.isLeaf( d.root() ) );
 }

// first child and next sibling by places, the copies of
// subtrees, the visits against the Tree with pointers; the edge cases: a
// sibling of the root, a freed node, the symmetric visit after no child
// and after all of them, the copy of the tree into itself, the removal
// of the root, a chain of 3000 nodes
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
 CHECK_THROWS( t.insertSiblingTree( t.root() , s , s.root() ) ,
               contract_violation );
 CHECK_THROWS( (void) t.read( a ) , contract_violation );   // freed
 CHECK( s.invisit( 0 ) == s.previsit() && s.invisit( 9 ) == s.postvisit() );
 t.insertChildTree( b , t , t.root() );         // a copy of itself
 CHECK( values( t.previsit() ) == Ints( { 0 , 2 , 0 , 2 } ) &&
        t.size() == 4 );
 t.remove( t.root() );                          // the root: all gone
 CHECK( t.empty() && t.size() == 0 && t.previsit().empty() &&
        t.postvisit().empty() && t.invisit( 1 ).empty() );
 CursorTree< int > d( 0 );                      // a deep chain
 int w = d.root();
 for( int k = 1 ; k < 3000 ; ++k ) w = d.insertChild( w , k );
 CHECK( d.size() == 3000 && d.previsit().size() == 3000 &&
        d.isLeaf( w ) && d.isLastSibling( w ) );
 d.remove( d.leftmostChild( d.root() ) );
 CHECK( d.size() == 1 && d.isLeaf( d.root() ) );
 }

// the tree in preorder against the tree with cursors, on
// random insertions and removals; the edge cases: the empty tree, one
// node, the empty tree inserted, a sibling of the root, a place out of
// range, a subtree of several nodes, the copy of the tree into itself
// (the parents shifted before the copy was taken), the removal of the root
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
 using PT = PreorderArrayTree< int >;
 auto parents = []( const PT & t ) {
  Ints v;
  for( int j = 0 ; j < t.size() ; ++j ) v.push_back( t.parent( j ) );
  return( v );
  };
 PT e;
 CHECK( e.empty() && e.size() == 0 && e.root() == e.NIL );
 PT t( 0 );
 t.insertChild( 0 , e );                        // the empty tree: nothing
 CHECK( t.size() == 1 && t.isLeaf( 0 ) && t.rightSibling( 0 ) == t.NIL &&
        t.subtreeEnd( 0 ) == 1 );
 CHECK_THROWS( t.insertSibling( 0 , PT( 1 ) ) , contract_violation );
 CHECK_THROWS( (void) t.read( 1 ) , contract_violation );
 CHECK_THROWS( (void) t.read( -1 ) , contract_violation );
 PT u( 7 );                                     // a subtree of 3 nodes
 u.insertChild( 0 , PT( 9 ) );
 u.insertChild( 0 , PT( 8 ) );
 t.insertChild( 0 , PT( 1 ) );
 t.insertChild( 1 , PT( 2 ) );                  // 0 ( 1 ( 2 ) )
 t.insertSibling( 1 , u );                      // 0 ( 1 ( 2 ) 7 ( 8 9 ) )
 CHECK( parents( t ) == Ints( { -1 , 0 , 1 , 0 , 3 , 3 } ) );
 t.remove( 3 );
 t.insertChild( 0 , t );                        // a copy of itself
 Ints a;
 for( int j = 0 ; j < t.size() ; ++j ) a.push_back( t.read( j ) );
 CHECK( a == Ints( { 0 , 0 , 1 , 2 , 1 , 2 } ) &&
        parents( t ) == Ints( { -1 , 0 , 1 , 2 , 0 , 4 } ) );
 t.remove( 0 );                                 // the root: all gone
 CHECK( t.empty() && t.root() == t.NIL );
 }

// the tree with lists of children against the tree with cursors, on
// random insertions and removals: the same visits, first children and
// next siblings; the edge cases: a sibling of the root, a node absent or
// freed, one node, the symmetric visit after no child and after all of
// them, the removal of the root, a chain of 3000 nodes
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
 CHECK_THROWS( (void) t.read( -1 ) , contract_violation );
 CHECK( t.isLeaf( t.root() ) && t.isLastSibling( t.root() ) &&
        t.children( t.root() ).empty() && t.size() == 1 );
 const int a = t.insertChild( t.root() , 1 );
 CHECK( t.invisit( 0 ) == t.previsit() && t.invisit( 7 ) == t.postvisit() );
 t.remove( a );
 CHECK_THROWS( (void) t.read( a ) , contract_violation );   // freed
 t.remove( t.root() );                          // the root: all gone
 CHECK( t.empty() && t.size() == 0 && t.previsit().empty() &&
        t.postvisit().empty() && t.invisit( 1 ).empty() );
 ChildListTree< int > d( 0 );                   // a deep chain
 int w = d.root();
 for( int k = 1 ; k < 3000 ; ++k ) w = d.insertChild( w , k );
 CHECK( d.size() == 3000 && d.postvisit().front() == w && d.isLeaf( w ) );
 d.remove( d.leftmostChild( d.root() ) );
 CHECK( d.size() == 1 && d.isLeaf( d.root() ) );
 }

// 5 , 3 , 9 , 2 , 10 come out as 2 , 3 , 5 , 9 , 10,
// and random sequences against sorting; on both ways of finding the last
// leaf (by the counter and by the pointer), the edge cases: the empty
// queue, one item in and out again, the extreme priorities of int,
// all the priorities equal, the items in increasing and in decreasing
// order past the 1024 nodes of the full invariant
ADS_TEST( "linked-heap" )
{
 LinkedHeapPriorityQueue< int > Q;
 for( int x : { 5 , 3 , 9 , 2 , 10 } ) Q.insert( x , x );
 Ints out;
 while( ! Q.empty() ) out.push_back( Q.deleteMin() );
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
 auto edges = [ & ]< bool L >( void ) {
  constexpr int MIN = std::numeric_limits< int >::min() ,
                MAX = std::numeric_limits< int >::max();
  LinkedHeapPriorityQueue< char , int , L > E;
  CHECK( E.empty() && E.size() == 0 );
  CHECK_THROWS( (void) E.min() , contract_violation );
  CHECK_THROWS( (void) E.minPriority() , contract_violation );
  CHECK_THROWS( E.deleteMin() , contract_violation );
  for( int r = 0 ; r < 3 ; ++r ) {              // one item, in and out
   E.insert( 'x' , r );
   CHECK( E.size() == 1 && E.min() == 'x' && E.minPriority() == r );
   CHECK( E.deleteMin() == 'x' && E.empty() );
   }
  for( int p : { MAX , 0 , MIN , -1 , MAX , MIN } ) E.insert( 'e' , p );
  Ints P;
  while( ! E.empty() ) {
   P.push_back( E.minPriority() );
   E.deleteMin();
   }
  CHECK( P == Ints( { MIN , MIN , -1 , 0 , MAX , MAX } ) );
  for( int i = 0 ; i < 33 ; ++i ) E.insert( char( 'a' + i % 26 ) , 7 );
  for( int i = 0 ; i < 33 ; ++i )               // all equal
   CHECK( E.minPriority() == 7 && E.size() == 33 - i && E.deleteMin() );
  for( bool up : { true , false } ) {          // sorted and reversed
   LinkedHeapPriorityQueue< int , int , L > S;
   for( int i = 0 ; i < 1100 ; ++i ) {
    const int x = up ? i : 1099 - i;
    S.insert( x , x );
    }
   for( int i = 0 ; i < 1100 ; ++i ) CHECK( S.deleteMin() == i );
   CHECK( S.empty() );
   }
  std::mt19937 h( 21 );                        // against std::multiset
  LinkedHeapPriorityQueue< int , int , L > H;
  std::multiset< int > M;
  for( int op = 0 ; op < 3000 ; ++op )
   if( M.empty() || h() % 3 ) {
    const int x = int( h() % 50 );
    H.insert( x , x );
    M.insert( x );
    }
   else {
    CHECK( H.min() == *M.begin() && H.deleteMin() == *M.begin() );
    M.erase( M.begin() );
    }
  CHECK( H.size() == int( M.size() ) );
  };
 edges.template operator()< false >();
 edges.template operator()< true >();
 }

// the degrees, the isolated deletion refused,
// the forced one, and the visits; the edge cases: an edge inserted twice,
// a loop, the opposite edges, an absent edge deleted, the null nodes, a
// node with a loop deleted, a node not reached, the last node deleted, a
// chain of 3000 nodes
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
 G.insertEdge( a , c , 9 );                     // already there: nothing
 CHECK( a->degree == 1 && c->degree == 1 && a->out.size() == 1 &&
        a->out.front().weight == 0 );
 G.insertEdge( c , c );                         // a loop counts twice
 CHECK( G.hasEdge( c , c ) && c->degree == 3 &&
        G.adjacent( c ) == std::vector< PointerGraph< char >::Node * >(
                              { c } ) );
 s.clear();
 G.dfs( c , [ & ]( auto * u ) { s += u->label; } );   // a not reached
 CHECK( s == "c" );
 G.insertEdge( c , a );                         // the opposite edge
 CHECK( G.hasEdge( a , c ) && G.hasEdge( c , a ) && a->degree == 2 );
 G.deleteEdge( c , a );
 G.deleteEdge( c , a );                         // absent: nothing
 CHECK( ! G.hasEdge( c , a ) && G.hasEdge( a , c ) && a->degree == 1 );
 CHECK_THROWS( G.insertEdge( nullptr , a ) , contract_violation );
 CHECK_THROWS( (void) G.hasEdge( a , nullptr ) , contract_violation );
 CHECK_THROWS( G.deleteEdge( nullptr , nullptr ) , contract_violation );
 CHECK_THROWS( G.deleteNode( nullptr ) , contract_violation );
 CHECK_THROWS( G.forceDeleteNode( nullptr ) , contract_violation );
 CHECK_THROWS( (void) G.adjacent( nullptr ) , contract_violation );
 CHECK_THROWS( G.dfs( nullptr , []( auto * ) {} ) , contract_violation );
 CHECK_THROWS( G.bfs( nullptr , []( auto * ) {} ) , contract_violation );
 G.forceDeleteNode( c );                        // with its loop
 CHECK( G.n() == 1 && a->degree == 0 && a->out.empty() );
 s.clear();
 G.bfs( a , [ & ]( auto * u ) { s += u->label; } );
 CHECK( s == "a" && G.deleteNode( a ) && G.n() == 0 && ! G.find( 'a' ) );
 PointerGraph< int > C;                         // a chain of 3000 nodes
 auto u = C.insertNode( 0 ) , r = u;
 for( int i = 1 ; i < 3000 ; ++i ) {
  auto v = C.insertNode( i );
  C.insertEdge( u , v );
  u = v;
  }
 Ints D , B;
 C.dfs( r , [ & ]( auto * x ) { D.push_back( x->label ); } );
 C.bfs( r , [ & ]( auto * x ) { B.push_back( x->label ); } );
 CHECK( D.size() == 3000 && D == B && D.back() == 2999 );
 }

// k stacks in one vector against k stacks apart; the edge cases: k = 0 or
// negative, a stack out of range, a single stack, top() and pop() on an
// empty one, the top pushed again while the vector grows (the item copied
// before the vector moves), the cells reused after all are freed
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
 CHECK_THROWS( MultiStack< int >( -3 ) , contract_violation );
 CHECK_THROWS( M.push( -1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) M.size( k ) , contract_violation );
 CHECK_THROWS( (void) M.empty( -1 ) , contract_violation );
 MultiStack< std::string > S( 1 );              // a single stack
 CHECK( S.stacks() == 1 && S.empty( 0 ) && S.size( 0 ) == 0 );
 CHECK_THROWS( (void) S.top( 0 ) , contract_violation );
 CHECK_THROWS( S.pop( 0 ) , contract_violation );
 S.push( 0 , std::string( 40 , 'x' ) );
 for( int i = 1 ; i < 100 ; ++i )               // its own top, as the
  S.push( 0 , S.top( 0 ) );                     // vector grows
 CHECK( S.size( 0 ) == 100 && S.top( 0 ) == std::string( 40 , 'x' ) );
 for( int i = 0 ; i < 100 ; ++i ) CHECK( S.pop( 0 ).size() == 40 );
 CHECK( S.empty( 0 ) );
 S.push( 0 , "a" );                             // the cells reused
 CHECK( S.size( 0 ) == 1 && S.pop( 0 ) == "a" && S.empty( 0 ) );
 }

// the deep copies of binary and ordered trees, and join(); the edge
// cases: the nil tree, join() of two nil subtrees and of the same subtree
// twice, the copy of an inner node (its siblings left out) and of a leaf,
// a chain of 10^4 nodes
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
 CHECK( copy( static_cast< const BT * >( nullptr ) ) == nullptr );
 CHECK_THROWS( copy( static_cast< const OT * >( nullptr ) ) ,
               contract_violation );
 std::unique_ptr< BT > j( join( 5 , static_cast< const BT * >( nullptr ) ,
                                static_cast< const BT * >( nullptr ) ) );
 CHECK( j->read() == 5 && ! j->left() && ! j->right() );
 std::unique_ptr< BT > jj( join( 0 , l.get() , l.get() ) );   // twice
 CHECK( pre_values( jj.get() ) == Ints( { 0 , 2 , 4 , 2 , 4 } ) );
 std::unique_ptr< OT > s( copy( o->leftmostChild() ) );   // not the root:
 v.clear();                                    // no sibling, no parent
 pre_visit( s.get() , [ & ]( OT * x ) { v.push_back( x->read() ); } );
 CHECK( v == Ints( { 1 , 3 } ) && ! s->parent() && s->isLastSibling() );
 std::unique_ptr< OT > f( copy( o->leftmostChild()->leftmostChild() ) );
 CHECK( f->read() == 3 && f->isLeaf() && ! f->parent() );
 auto chain = std::make_unique< BT >( 0 );     // a chain of 10^4 nodes
 BT * w = chain.get();
 for( int i = 1 ; i < 10000 ; ++i ) {
  w->insertLeft( new BT( i ) );
  w = w->left();
  }
 std::unique_ptr< BT > cc( copy( chain.get() ) );
 long sum = 0 , k = 0;
 for( const BT * x = cc.get() ; x ; x = x->left() , ++k ) sum += x->read();
 CHECK( k == 10000 && sum == 9999L * 10000 / 2 );
 }

// contains() on a queue and == on a sequence; the edge cases: the queue
// full and wrapped, emptied, of capacity 0 and 1, the empty sequences, a
// prefix on either side
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
 CHECK( Q.size() == 3 && Q.contains( 3 ) );      // full, wrapped
 CHECK_THROWS( Q.enqueue( 5 ) , contract_violation );
 while( ! Q.empty() ) Q.dequeue();
 CHECK( ! Q.contains( 2 ) && ! Q.contains( 4 ) );   // empty
 CHECK_THROWS( Q.dequeue() , contract_violation );
 CHECK_THROWS( (void) Q.top() , contract_violation );
 ArrayQueue< int > Z( 0 );                      // no room at all
 CHECK( Z.empty() && Z.capacity() == 0 && ! Z.contains( 0 ) );
 CHECK_THROWS( Z.enqueue( 1 ) , contract_violation );
 ArrayQueue< int > O( 1 );                      // one place, wrapping
 for( int i = 0 ; i < 5 ; ++i ) {               // at every step
  O.enqueue( i );
  CHECK( O.contains( i ) && ! O.contains( i - 1 ) && O.top() == i );
  CHECK( O.dequeue() == i );
  }
 ArraySequence< int > e , f;
 CHECK( e == f && !( e == c ) && !( c == a ) && c == c );
 }

// the inspection i ( i + 1 ) / 2 visits every place
// of a table of size 2^p; the double hashing with a second function; and
// the hash h = 5 , h = 3 h + c; the edge cases: the empty key, m = 1, m
// base at the limit of 64 bits and past it, the hash at the top of
// size_t, the period 2 m, m = 0 and a size the inspection does not work
// with, the table full (a new key refused, an old one updated, the search
// of an absent key ending), a deleted place reused, a table of one place
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
 CHECK( hash_division( "" , 1000003 , 3 , 5 ) == 5 &&
        hash_division( "abc" , 1 , 3 , 5 ) == 0 );
 CHECK( hash_division( "\xff\xff" , ( 1ull << 32 ) - 1 , 1ull << 32 , 0 )
        == 510 );                               // 2^32 = 1 mod m
 CHECK( hash_division( "\xff\xff" , 1ull << 32 , 1ull << 32 , 0 ) ==
        255 );                        // ( m - 1 ) base + 255 below 2^64
 CHECK_THROWS( hash_division( "a" , 0 , 3 , 5 ) , contract_violation );
 CHECK_THROWS( hash_division( "a" , 7 , 0 , 5 ) , contract_violation );
 CHECK_THROWS( hash_division( "a" , ( 1ull << 32 ) + 1 , 1ull << 32 , 0 ) ,
               contract_violation );            // it would overflow
 const std::size_t H = std::numeric_limits< std::size_t >::max();
 for( auto P : { Probing::triangular() , Probing::double_hashing( g ) ,
                 Probing::double_hashing() } )   // h at the top of size_t
  for( std::size_t m : { 1u , 2u , 32u } ) {
   std::set< std::size_t > S;
   for( std::size_t i = 0 ; i < m ; ++i ) S.insert( P.at( H , i , m ) );
   CHECK( S.size() == m && P.at( H , 0 , m ) == H % m );
   CHECK( P.at( H , 2 * m , m ) == P.at( H , 0 , m ) );   // periodic
   CHECK_THROWS( (void) P.at( H , 0 , 0 ) , contract_violation );
   }
 CHECK( ! Probing::triangular().works( 0 ) &&
        ! Probing::triangular().works( 3 ) );
 CHECK_THROWS( Probing::triangular().check( 0 ) , contract_violation );
 CHECK_THROWS( ( OpenHashTable< int , int >( 12 , Probing::triangular() ) ) ,
               contract_violation );
 CHECK_THROWS( ( OpenHashTable< int , int >( 0 , Probing::linear() ) ) ,
               contract_violation );
 for( auto P : { Probing::triangular() , Probing::double_hashing( g ) } ) {
  OpenHashTable< int , int > T( 8 , P );        // full, then a new key
  for( int k = 0 ; k < 8 ; ++k ) T.insert( k * 37 , k );
  CHECK( T.size() == 8 && ! T.lookup( 1 ) );     // the search ends
  CHECK_THROWS( T.insert( 1 , 0 ) , std::overflow_error );
  T.insert( 37 , 9 );                            // an old key, full table
  CHECK( T.lookup( 37 ) == 9 && T.size() == 8 );
  T.remove( 0 );
  T.remove( 0 );                                 // absent: nothing
  T.insert( 1 , 1 );                             // in the deleted place
  CHECK( T.lookup( 1 ) == 1 && ! T.lookup( 0 ) && T.size() == 8 );
  OpenHashTable< int , int > O( 1 , P );         // a single place
  O.insert( -5 , 1 );
  CHECK( O.lookup( -5 ) == 1 && ! O.lookup( 5 ) );
  CHECK_THROWS( O.insert( 5 , 2 ) , std::overflow_error );
  }
 }

// the examples of the hash functions on names: the hash of the first
// letter on 26 places, a collision resolved by the linear inspection; the
// keys coded with 6 bits a character (a = 1 , ... , z = 26, the space 32)
// on 6 characters and 4 more bits of zeros, 40 bits, where the extraction
// of the bits 15 to 22 has a collision that the XOR of the 5 bytes and the
// division of the 36 bits by 383 do not have; the edge cases: the linear
// inspection around the end of the table, an empty place or one out of
// range, an empty chain, the empty key, the bits past the key, the 8
// bytes of int_bin() and the 9 refused, p at its bounds and past them
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
 T.insert( "zola" , 0 );                        // the last place, and
 T.insert( "zeta" , 0 );                        // around to the first
 CHECK( T.keyAt( 25 ) == "zola" && T.keyAt( 1 ) == "zeta" );
 CHECK_THROWS( (void) T.keyAt( 2 ) , contract_violation );   // empty
 CHECK_THROWS( (void) T.slot( 26 ) , contract_violation );
 ChainedHashTable< std::string , int , Chain::SORTED , First > C( 26 );
 for( const auto & k : K ) C.insert( k , 0 );
 CHECK( C.chain( 15 ) == std::vector< std::string >( { "palestrina" ,
                                                       "prokofev" ,
                                                       "puccini" } ) );
 CHECK( C.chain( 1 ).empty() && C.chain( 0 ) ==
        std::vector< std::string >( { "albinoni" } ) );
 CHECK( int_bin( "" ) == 0 && int_bin( "\x01\x02" ) == 258 &&
        int_bin( "\xff\xff\xff\xff\xff\xff\xff\xff" ) ==
        std::numeric_limits< std::uint64_t >::max() );
 CHECK_THROWS( int_bin( "123456789" ) , contract_violation );
 CHECK( hash_extraction( "" , 8 , 0 ) == 0 &&           // past the key:
        hash_extraction( "\x80" , 3 , 6 ) == 0 &&       // zeros
        hash_extraction( "\x80" , 1 , 0 ) == 1 && hash_xor( "" , 5 ) == 0 );
 CHECK( hash_xor( "\xff\xff" , 63 ) == 0xFFFFull << 47 );
 for( int p : { 0 , 64 } ) {
  CHECK_THROWS( hash_extraction( "a" , p , 0 ) , contract_violation );
  CHECK_THROWS( hash_xor( "a" , p ) , contract_violation );
  CHECK_THROWS( hash_fibonacci( 1 , p ) , contract_violation );
  }
 CHECK( hash_fibonacci( 0 , 1 ) == 0 && hash_fibonacci(
        std::numeric_limits< std::uint64_t >::max() , 63 ) < ( 1ull << 63 ) );
 CHECK( hash_midsquare( 0xFFFFFFFFull , 32 ) < ( 1ull << 32 ) &&
        hash_midsquare( 3 , 2 ) < 4 );
 CHECK_THROWS( hash_midsquare( 1 , 33 ) , contract_violation );
 CHECK( hash_multiplication( 12345 , 100 , 0 ) == 0 &&
        hash_multiplication( 12345 , 0 , 0.618L ) == 0 );
 CHECK_THROWS( hash_multiplication( 1 , 10 , 1.0L ) , contract_violation );
 CHECK_THROWS( hash_multiplication( 1 , 10 , -0.1L ) , contract_violation );
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
// scalar product; the edge cases: a row by a column and a column by a
// row, the 0 x 0 and the 2 x 0 matrices, the factor 0, the largest long
// times 1, the shapes that do not match and the ragged matrices
ADS_TEST( "rectangular-matrix" )
{
 const Matrix< long > A = { { 1 , 2 , 3 } , { 4 , 5 , 6 } };
 const Matrix< long > B = { { 7 , 8 } , { 9 , 10 } , { 11 , 12 } };
 CHECK( multiply( A , B ) == Matrix< long >( { { 58 , 64 } ,
                                               { 139 , 154 } } ) );
 CHECK( transpose( multiply( A , B ) ) ==
        multiply( transpose( B ) , transpose( A ) ) );
 CHECK( scale( A , 2L )[ 1 ][ 2 ] == 12 && transpose( A ).size() == 3 );
 using M = Matrix< long >;
 const M row = { { 1 , 2 , 3 } } , col = { { 4 } , { 5 } , { 6 } };
 CHECK( multiply( row , col ) == M( { { 32 } } ) );   // 1 x 1
 CHECK( multiply( col , row ) == M( { { 4 , 8 , 12 } , { 5 , 10 , 15 } ,
                                      { 6 , 12 , 18 } } ) );
 CHECK( transpose( row ) == M( { { 1 } , { 2 } , { 3 } } ) &&
        transpose( transpose( A ) ) == A );
 CHECK( transpose( M() ).empty() && multiply( M() , M() ).empty() );
 const M z( 2 , std::vector< long >() );        // 2 x 0 times 0 x r
 CHECK( multiply( z , M() ) == z && transpose( z ).empty() );
 CHECK( scale( A , 0L ) == M( { { 0 , 0 , 0 } , { 0 , 0 , 0 } } ) &&
        scale( M() , 3L ).empty() );
 const long big = std::numeric_limits< long >::max();
 CHECK( multiply( M( { { big } } ) , M( { { 1 } } ) ) == M( { { big } } ) );
 CHECK_THROWS( multiply( A , A ) , contract_violation );   // 2 x 3 , 2 x 3
 CHECK_THROWS( multiply( A , M( { { 1 , 2 } , { 3 } , { 4 , 5 } } ) ) ,
               contract_violation );            // B not rectangular
 CHECK_THROWS( transpose( M( { { 1 , 2 } , { 3 } } ) ) , contract_violation );
 }

// search() and searchOrdered() on 1 3 5 7 9; the edge cases: the first
// and the last item, an item below all, the empty sequence (whose result
// must not be the place 0), one item, the copies (the first one found),
// the extremes of int, a sequence not sorted
ADS_TEST( "sequence-search" )
{
 const auto l = list_of< int >( { 1 , 3 , 5 , 7 , 9 } );
 CHECK( search( l , 3 ) == 1 && search( l , 4 ) == -6 );
 CHECK( searchOrdered( l , 3 ) == 1 && searchOrdered( l , 4 ) == -3 );
 CHECK( searchOrdered( l , 10 ) == -5 && search( List< int >() , 1 ) == -1 );
 CHECK( search( l , 1 ) == 0 && search( l , 9 ) == 4 );   // the ends
 CHECK( searchOrdered( l , 1 ) == 0 && searchOrdered( l , 9 ) == 4 );
 CHECK( searchOrdered( l , 0 ) == -1 );         // below all: 1 comparison
 CHECK( searchOrdered( List< int >() , 1 ) == -1 );   // never 0
 const auto o = list_of< int >( { 4 } );
 CHECK( search( o , 4 ) == 0 && search( o , 5 ) == -2 &&
        searchOrdered( o , 4 ) == 0 && searchOrdered( o , 5 ) == -1 &&
        searchOrdered( o , 3 ) == -1 );
 const auto d = list_of< int >( { 2 , 3 , 3 , 3 , 8 } );   // the first copy
 CHECK( search( d , 3 ) == 1 && searchOrdered( d , 3 ) == 1 );
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 const auto x = list_of< int >( { m , 0 , M } );
 CHECK( searchOrdered( x , m ) == 0 && searchOrdered( x , M ) == 2 &&
        searchOrdered( x , M - 1 ) == -3 );
 CHECK_THROWS( searchOrdered( list_of< int >( { 3 , 1 } ) , 1 ) ,
               contract_violation );            // not sorted
 }

// the order of a sequence; the edge cases: two items either way, 50
// items all equal, a repetition at either end, down and then up, the
// extremes of int, the realization on a vector
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
 CHECK( order( list_of< int >( { 3 , 1 , 2 } ) ) == Order::NONE );
 CHECK( order( list_of< int >( { 1 , 2 } ) ) == Order::INCREASING );
 CHECK( order( list_of< int >( { 2 , 1 } ) ) == Order::DECREASING );
 CHECK( order( list_of( Ints( 50 , -7 ) ) ) == Order::CONSTANT );
 CHECK( order( list_of< int >( { 1 , 1 , 1 , 2 } ) ) ==
        Order::NONDECREASING );
 CHECK( order( list_of< int >( { 1 , 2 , 2 , 1 } ) ) == Order::NONE );
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 CHECK( order( list_of< int >( { m , 0 , M } ) ) == Order::INCREASING );
 CHECK( order( list_of< int >( { M , M , m } ) ) == Order::NONINCREASING );
 CHECK( order( ArraySequence< int >( { 5 , 5 , 4 } ) ) ==
        Order::NONINCREASING && order( ArraySequence< int >() ) ==
        Order::EMPTY );
 }

// the natural merge sort and the bubble sort on six names and on random
// sequences, on both realizations; the edge cases: the empty sequence, one
// item, two reversed, all equal, already sorted (no pass), 64 items in
// reverse (log2 64 passes), the extremes of int with copies
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
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 Ints R( 64 );                                  // 64 runs of one item
 for( int i = 0 ; i < 64 ; ++i ) R[ i ] = 64 - i;
 for( const Ints & v : { Ints() , Ints( { 7 } ) , Ints( { 2 , 1 } ) ,
                         Ints( 9 , 3 ) , Ints( { 1 , 2 , 3 , 4 } ) , R ,
                         Ints( { M , m , 0 , M , m } ) } ) {
  Ints s = v;
  std::sort( s.begin() , s.end() );
  auto l1 = list_of( v ) , l2 = list_of( v );
  long passes = -1;
  naturalMergeSort( l1 , &passes );
  bubbleSort( l2 );
  CHECK( items( l1 ) == s && items( l2 ) == s );
  if( std::is_sorted( v.begin() , v.end() ) ) CHECK( passes == 0 );
  if( v == R ) CHECK( passes == 6 );            // log2 64
  }
 }

// the merge that keeps the copies, and the
// first copy of each item kept; the edge cases: the empty sequences, one
// side all before the other, a sequence merged with itself, the items of
// a first among equals, a sequence not sorted; no copy, one item, all
// equal, the copies at the ends
ADS_TEST( "sequence-merge-dedup" )
{
 const auto m = mergeSorted( list_of< int >( { 1 , 3 , 3 , 8 } ) ,
                             list_of< int >( { 2 , 3 , 9 } ) );
 CHECK( items( m ) == Ints( { 1 , 2 , 3 , 3 , 3 , 8 , 9 } ) );
 auto l = list_of< int >( { 4 , 1 , 4 , 2 , 1 , 4 , 3 } );
 removeDuplicates( l );
 CHECK( items( l ) == Ints( { 4 , 1 , 2 , 3 } ) );
 const List< int > e;
 const auto a = list_of< int >( { 5 , 6 } );
 CHECK( items( mergeSorted( e , e ) ).empty() );
 CHECK( items( mergeSorted( e , a ) ) == Ints( { 5 , 6 } ) &&
        items( mergeSorted( a , e ) ) == Ints( { 5 , 6 } ) );
 CHECK( items( mergeSorted( a , list_of< int >( { 1 , 2 } ) ) ) ==
        Ints( { 1 , 2 , 5 , 6 } ) );            // one side all first
 CHECK( items( mergeSorted( a , a ) ) == Ints( { 5 , 5 , 6 , 6 } ) );
 struct Tag {                                   // equal keys: a first
  int k;
  char t;
  bool operator<( const Tag & o ) const { return( k < o.k ); }
  bool operator==( const Tag & o ) const { return( k == o.k ); }
  };
 List< Tag > x , y;
 for( Tag z : { Tag{ 1 , 'a' } , Tag{ 3 , 'a' } } )
  x.insert( x.next( x.tail() ) , z );
 for( Tag z : { Tag{ 1 , 'b' } , Tag{ 3 , 'b' } } )
  y.insert( y.next( y.tail() ) , z );
 std::string tags;
 for( const Tag & z : items( mergeSorted( x , y ) ) ) tags += z.t;
 CHECK( tags == "abab" );
 CHECK_THROWS( mergeSorted( list_of< int >( { 2 , 1 } ) , a ) ,
               contract_violation );            // not sorted
 for( const auto & [ in , out ] : { std::pair( Ints() , Ints() ) ,
       std::pair( Ints( { 7 } ) , Ints( { 7 } ) ) ,
       std::pair( Ints( 20 , 0 ) , Ints( { 0 } ) ) ,
       std::pair( Ints( { 3 , 1 , 2 } ) , Ints( { 3 , 1 , 2 } ) ) ,
       std::pair( Ints( { 1 , 2 , 2 , 1 } ) , Ints( { 1 , 2 } ) ) } ) {
  auto r = list_of( in );
  removeDuplicates( r );
  CHECK( items( r ) == out );
  }
 }

// the palindromes, the sorting of a stack with its own operations, and
// removeGreater(); the edge cases: the strings of one and two characters
// and all equal, the empty stack, one item, all equal, sorted either way,
// the extremes of int; k below all, above all, equal to an item
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
  while( ! S.empty() ) out.push_back( S.pop() );
  std::sort( v.begin() , v.end() );
  CHECK( out == v );                      // the least on top
  }
 SequenceStack< List< int > > S;
 for( int x : { 5 , 1 , 7 , 3 , 9 , 2 } ) S.push( x );
 removeGreater( S , 4 );
 Ints out;
 while( ! S.empty() ) out.push_back( S.pop() );
 CHECK( out == Ints( { 2 , 3 , 1 } ) );
 CHECK( isPalindrome( "x" ) && isPalindrome( "xx" ) &&
        ! isPalindrome( "xy" ) && ! isPalindrome( "aab" ) &&
        isPalindrome( std::string( 101 , 'z' ) ) );
 auto drain = []( SequenceStack< List< int > > & P ) {
  Ints o;
  while( ! P.empty() ) o.push_back( P.pop() );
  return( o );
  };
 auto stack = []( const Ints & v ) {
  SequenceStack< List< int > > P;
  for( int x : v ) P.push( x );
  return( P );
  };
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 for( const Ints & v : { Ints() , Ints( { 4 } ) , Ints( 6 , 2 ) ,
                         Ints( { 1 , 2 , 3 } ) , Ints( { 3 , 2 , 1 } ) ,
                         Ints( { M , m , 0 , m } ) } ) {
  auto P = stack( v );
  sortStack( P );
  Ints s = v;
  std::sort( s.begin() , s.end() );
  CHECK( drain( P ) == s );
  }
 auto P = stack( { 5 , 1 , 7 } );               // k below all: none left
 removeGreater( P , 0 );
 CHECK( P.empty() );
 P = stack( { 5 , 1 , 7 } );                    // k above all: all kept
 removeGreater( P , 7 );                        // (an item equal to k too)
 CHECK( drain( P ) == Ints( { 7 , 1 , 5 } ) );
 removeGreater( P , 3 );                        // the empty stack
 CHECK( P.empty() );
 P = stack( { m , M } );
 removeGreater( P , m );
 CHECK( drain( P ) == Ints( { m } ) );
 }

// "on" in "colonna" at 3, the failure
// function, and the two against each other on random strings; the edge
// cases: the empty text and the empty pattern, the pattern longer than
// the text and equal to it, at either end and absent, the overlapping
// occurrences in a text of one character, the failure function of the
// empty pattern, of one character, of all equal and all distinct ones
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
 using Places = std::vector< std::size_t >;
 using F = std::vector< long >;
 CHECK( naiveMatching( "" , "" ) == Places( { 0 } ) &&
        kmp( "" , "" ) == Places( { 0 } ) );
 CHECK( naiveMatching( "" , "a" ).empty() && kmp( "" , "a" ).empty() );
 CHECK( naiveMatching( "abc" , "" ) == Places( { 0 , 1 , 2 , 3 } ) &&
        kmp( "abc" , "" ) == Places( { 0 , 1 , 2 , 3 } ) );
 CHECK( kmp( "abc" , "abc" ) == Places( { 0 } ) &&
        naiveMatching( "ab" , "abc" ).empty() );
 CHECK( kmp( "abc" , "c" ) == Places( { 2 } ) &&
        kmp( "abc" , "a" ) == Places( { 0 } ) && kmp( "abc" , "d" ).empty() );
 CHECK( kmp( "aaaaa" , "aa" ) == Places( { 0 , 1 , 2 , 3 } ) );  // overlaps
 CHECK( failure( "" ) == F( { -1 } ) && failure( "x" ) == F( { -1 , 0 } ) );
 CHECK( failure( "aaaa" ) == F( { -1 , 0 , 1 , 2 , 3 } ) &&
        failure( "abcd" ) == F( { -1 , 0 , 0 , 0 , 0 } ) );
 long c = 0;
 CHECK( kmp( std::string( 1000 , 'a' ) , "aab" , &c ).empty() &&
        c <= 2000 );
 c = 0;
 CHECK( kmp( "" , "ab" , &c ).empty() && c == 0 );
 }

// freq(), hist(), remp() and del(); the edge cases: the empty list, one
// item, all odd or all even, the negative items and 0, all equal
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
 const List< int > e;
 CHECK( freq( e , 3 ) == 0 && hist( e ).empty() );
 const auto n = list_of< int >( { -4 , -3 , 0 , 5 } );   // negatives, 0
 CHECK( freq( n , 2 ) == 2 && freq( n , 1 ) == 4 && freq( n , -3 ) == 2 );
 using HV = std::vector< std::pair< int , int > >;
 CHECK( hist( list_of< int >( { 7 , 7 , 7 } ) ) == HV( { { 7 , 3 } } ) );
 for( const auto & [ in , out ] : { std::pair( Ints() , Ints() ) ,
       std::pair( Ints( { 2 } ) , Ints( { 2 } ) ) ,
       std::pair( Ints( { 1 , 3 , 5 } ) , Ints( { 5 } ) ) ,   // all odd
       std::pair( Ints( { 2 , 4 , 6 } ) , Ints( { 2 , 4 , 6 } ) ) ,
       std::pair( Ints( { 2 , -1 } ) , Ints( { -1 } ) ) } ) {
  auto x = list_of( in );
  remp( x );
  CHECK( items( x ) == out );
  }
 for( const auto & [ in , out ] : { std::pair( Ints() , Ints() ) ,
       std::pair( Ints( { 5 } ) , Ints() ) ,          // all multiples of 1
       std::pair( Ints( { 2 , 2 , 2 } ) , Ints( { 2 } ) ) ,
       std::pair( Ints( { 0 , 0 } ) , Ints() ) ,
       std::pair( Ints( { -3 , -3 , -3 } ) , Ints( { -3 } ) ) } ) {
  auto x = list_of( in );
  del( x );
  CHECK( items( x ) == out );
  }
 }

// the strings of even and odd length; the edge cases: nothing inserted,
// the empty string (of even length, starting with no character), the
// copies and the ties of length, an absent string removed
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
 EvenOddStrings F;                              // nothing in it
 F.removeAll( "x" );
 F.removeStartingWith( 'x' );
 CHECK( F.evenCount() == 0 && F.oddByLength().empty() );
 for( const char * s : { "" , "ab" , "" , "c" , "d" , "c" } ) F.insert( s );
 CHECK( F.evenCount() == 3 && F.even() ==       // the newest first
        std::vector< std::string >( { "" , "ab" , "" } ) );
 CHECK( F.oddByLength() ==                      // ties: insertion order
        std::vector< std::string >( { "c" , "d" , "c" } ) );
 F.removeStartingWith( 'a' );                   // the empty ones stay
 F.removeAll( "zz" );                           // absent: nothing
 F.removeAll( "c" );                            // every copy
 CHECK( F.even() == std::vector< std::string >( { "" , "" } ) &&
        F.oddByLength() == std::vector< std::string >( { "d" } ) );
 F.removeAll( "" );
 CHECK( F.evenCount() == 0 );
 }

// the exercises on the tree 0 ( 1 ( 3 , 4 ) , 2 ( 5 , 6 ) ) and on a
// chain; the edge cases: the empty tree, a single node, the negative
// values with a child missing, a level below the leaves, a chain of 10^4
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
 CHECK( sumNodes( nullptr ) == 0 && heightNodes( nullptr ) == 0 &&
        heightBalanced( nullptr ) && oddAtLevel( nullptr , 0 ) == 0 &&
        evenLeaves( nullptr ) == 0 );
 relabelEven( nullptr );
 std::unique_ptr< BT > one( new BT( 4 ) );      // a single node
 CHECK( maxPathCost( one.get() ) == 4 && maxNode( one.get() ) == 4 &&
        heightBalanced( one.get() ) && evenLeaves( one.get() ) == 1 &&
        evenDescendants( one.get() ) == 0 &&
        oddAtLevel( one.get() , 0 ) == 0 && oddAtLevel( one.get() , 1 ) == 0 );
 std::unique_ptr< BT > n( new BT( -5 ) );       // negatives, one child
 n->insertLeft( new BT( -1 ) );                 // missing
 n->insertRight( new BT( -10 ) );
 n->right()->insertRight( new BT( -2 ) );
 CHECK( maxPathCost( n.get() ) == -6 && maxNode( n.get() ) == -1 &&
        sumNodes( n.get() ) == -18 && heightBalanced( n.get() ) );
 CHECK( oddAtLevel( n.get() , 0 ) == 1 && oddAtLevel( n.get() , 7 ) == 0 &&
        evenLeaves( n.get() ) == 1 && evenDescendants( n.get() ) == 2 );
 std::unique_ptr< BT > deep( new BT( 1 ) );     // a chain of 10^4 nodes
 BT * w = deep.get();
 for( int i = 1 ; i < 10000 ; ++i ) {
  w->insertRight( new BT( 1 ) );
  w = w->right();
  }
 CHECK( heightNodes( deep.get() ) == 10000 && sumNodes( deep.get() ) ==
        10000 && maxPathCost( deep.get() ) == 10000 );
 }

// the sum of the descendants, the relabeling, the nodes of
// even parent, on 2 ( 3 ( 5 ) , 4 , 6 ( 7 , 8 ) ); the edge cases: a
// single node, 100 children of negative values, an odd root
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
 std::unique_ptr< OT > one( new OT( 8 ) );      // a single node
 CHECK( sumDescendants( one.get() ) == 0 && evenParent( one.get() ) == 0 );
 relabelSum( one.get() );
 CHECK( one->read() == 0 );
 std::unique_ptr< OT > w( new OT( -2 ) );       // 100 children, negative
 for( int i = 1 ; i <= 100 ; ++i ) w->insertChild( new OT( -i ) );
 CHECK( sumDescendants( w.get() ) == -5050 && evenParent( w.get() ) == 100 );
 std::unique_ptr< OT > o( new OT( 1 ) );        // an odd parent: none
 o->insertChild( new OT( 2 ) );
 o->leftmostChild()->insertChild( new OT( 3 ) );
 CHECK( evenParent( o.get() ) == 1 && sumDescendants( o.get() ) == 5 );
 }

// the functions on arrays; the edge cases: the empty array, one item, the
// extremes of int, all equal (the first place), the largest last, an item
// absent, the only and the last item removed, an even length reversed
ADS_TEST( "array-exercises" )
{
 Ints a = { 3 , 9 , 1 , 9 , 4 };
 CHECK( greaterThan( a , 3 ) == 3 && member( a , 4 ) && ! member( a , 5 ) );
 CHECK( largest( a ) == std::pair( 9 , 1 ) );
 removeElem( a , 9 );
 CHECK( a == Ints( { 3 , 1 , 9 , 4 , 0 } ) );
 Ints e;
 CHECK( greaterThan( e , 0 ) == 0 && ! member( e , 0 ) );
 removeElem( e , 1 );
 reverseArray( e );
 CHECK( e.empty() );
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 const Ints x = { m , M , -1 , M };
 CHECK( greaterThan( x , M ) == 0 && greaterThan( x , m ) == 3 &&
        member( x , m ) && largest( x ) == std::pair( M , 1 ) );
 CHECK( largest( Ints( 4 , -3 ) ) == std::pair( -3 , 0 ) &&   // all equal
        largest( Ints( { 7 } ) ) == std::pair( 7 , 0 ) &&
        largest( Ints( { 1 , 2 , 8 } ) ) == std::pair( 8 , 2 ) );
 Ints s = { 5 };
 removeElem( s , 6 );                           // absent: unchanged
 CHECK( s == Ints( { 5 } ) );
 removeElem( s , 5 );                           // the only item
 CHECK( s == Ints( { 0 } ) );
 Ints t = { 1 , 2 , 3 };
 removeElem( t , 3 );                           // the last item
 CHECK( t == Ints( { 1 , 2 , 0 } ) );
 reverseArray( s );
 CHECK( s == Ints( { 0 } ) );
 Ints u = { 1 , 2 , 3 , 4 };                    // an even length
 reverseArray( u );
 CHECK( u == Ints( { 4 , 3 , 2 , 1 } ) );
 reverseArray( a );
 CHECK( a == Ints( { 0 , 4 , 9 , 1 , 3 } ) );
 }

// the lists with cursors: three lists in one space of 10 cells, the free
// list giving its first cell to an insertion and getting back the cell of
// a removal, the space doubling when no cell is free; random operations
// against std::list; the edge cases: a space of 0 cells and of 1, the
// empty list read, removed from, inserted at pos_-1, an item of the list
// inserted while the space grows, a copy in a full space and its cells
// given back, the assignment to itself, the list moved, all removed
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
 CHECK_THROWS( CursorSpace< int >( 0 ) , contract_violation );
 using SL = CursorList< std::string >;
 auto T = std::make_shared< CursorSpace< std::string > >( 1 );
 SL a( T );                                     // one cell, then doubling
 CHECK( a.empty() && a.tail() == -1 && T->used() == 0 );
 CHECK_THROWS( (void) a.read( a.head() ) , contract_violation );
 CHECK_THROWS( a.remove( a.head() ) , contract_violation );
 CHECK_THROWS( a.insert( -1 , "x" ) , contract_violation );
 const std::string big( 40 , 'b' );
 a.insert( a.head() , big );
 CHECK( T->capacity() == 1 && T->used() == 1 );
 a.insert( a.head() , a.read( a.head() ) );     // its own item, growing
 CHECK( T->capacity() == 2 && items( a ) ==
        std::vector< std::string >( { big , big } ) );
 {
  const SL b( a );                              // a copy in the full space
  CHECK( items( b ) == items( a ) && T->used() == 4 &&
         T->capacity() == 4 );
  }
 CHECK( T->used() == 2 );                       // its cells given back
 const SL & same = a;
 a = same;                                      // itself
 CHECK( items( a ) == std::vector< std::string >( { big , big } ) &&
        T->used() == 2 );
 SL c( std::move( a ) );                        // a left empty
 CHECK( a.empty() && items( c ).size() == 2 && T->used() == 2 );
 while( ! c.empty() ) c.remove( c.head() );
 CHECK( T->used() == 0 && c.tail() == -1 );
 }

// the search in a sorted list that gives the position where the item goes,
// the insertion there keeping the list sorted; the sublists; the natural
// merge sort on a list of 6 runs, merged in 3 passes; the edge cases: the
// empty list, an item below all, at the first and the last position, the
// first of the copies, the whole list as a sublist, q before p, a list
// not sorted
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
 CHECK( sublist( L , L.next( L.tail() ) , L.tail() ).empty() );
 CHECK( items( sublist( L , a , a ) ) == std::vector< std::string >(
        { "antonio" } ) );
 List< int > N = { 82 , 16 , 14 , 15 , 84 , 25 , 77 , 13 , 75 , 4 };
 long passes = 0;
 naturalMergeSort( N , &passes );
 CHECK( passes == 3 && order( N ) == Order::INCREASING );
 const List< int > E;
 const auto [ pe , fe ] = lowerBound( E , 5 );
 CHECK( E.finished( pe ) && ! fe && sublist( E , pe , pe ).empty() );
 List< int > D = { 2 , 4 , 4 , 4 , 9 };
 const auto [ p0 , f0 ] = lowerBound( D , 1 );  // below all: the head
 CHECK( p0 == D.head() && ! f0 );
 const auto [ p2 , f2 ] = lowerBound( D , 2 );  // the first item
 CHECK( p2 == D.head() && f2 );
 const auto [ p4 , f4 ] = lowerBound( D , 4 );  // the first copy
 CHECK( f4 && p4 == D.next( D.head() ) );
 const auto [ p9 , f9 ] = lowerBound( D , 9 );  // the last item
 CHECK( f9 && p9 == D.tail() );
 D.insert( p0 , 1 );
 D.insert( D.next( D.tail() ) , 10 );           // at pos_n
 CHECK( items( D ) == Ints( { 1 , 2 , 4 , 4 , 4 , 9 , 10 } ) );
 CHECK( items( sublist( D , D.head() , D.tail() ) ) == items( D ) );
 CHECK_THROWS( sublist( D , D.tail() , D.head() ) , contract_violation );
 CHECK_THROWS( lowerBound( List< int >( { 3 , 1 } ) , 2 ) ,
               contract_violation );            // not sorted
 }

// the decision tree of the binary search on 18 sorted names: the middle
// item at the root, the path of a search being the trace of
// binarySearch(), the height floor( log2 18 ) = 4, and the invisit giving
// the names back (the audit); the edge cases: the empty vector, every n
// from 1 to 33 (the height floor( log2 n ), the paths of the items there
// and of those absent, below all and above all), a vector not sorted
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
 CHECK( binarySearch( Ints() , 1 ) == -1 );
 for( int n = 1 ; n <= 33 ; ++n ) {             // 1 , 2 , 2^k - 1 , 2^k
  Ints B( n );
  for( int i = 0 ; i < n ; ++i ) B[ i ] = 2 * i;
  std::unique_ptr< BinaryTree< int > > d( decisionTree( B ) );
  std::function< int( const BinaryTree< int > * ) > h =
   [ & ]( const BinaryTree< int > * u ) -> int {
    return( u ? 1 + std::max( h( u->left() ) , h( u->right() ) ) : -1 ); };
  CHECK( h( d.get() ) == int( std::floor( std::log2( n ) ) ) );
  for( int v = -1 ; v <= 2 * n ; ++v ) {        // present and absent
   Ints trace , path;
   const long r = binarySearch( B , v , [ & ]( long , long , long m ) {
    trace.push_back( B[ m ] ); } );
   for( auto * u = d.get() ; u ; u = v < u->read() ? u->left()
                                                   : u->right() ) {
    path.push_back( u->read() );
    if( u->read() == v ) break;
    }
   CHECK( path == trace && ( r == ( v % 2 == 0 && v < 2 * n ? v / 2 : -1 ) ) );
   }
  }
 std::unique_ptr< BinaryTree< int > > one( decisionTree( Ints( { 7 } ) ) );
 CHECK( one->read() == 7 && ! one->left() && ! one->right() );
 CHECK_THROWS( decisionTree( Ints( { 3 , 1 } ) ) , contract_violation );
 }

// the parenthesized lists of a binary and of an ordered tree, read back
// and written again; the visits of the two examples; the edge cases: a
// single node, the negative values and the largest int, a list with no
// spaces, the malformed lists (empty, unbalanced, a tree after the tree,
// a third subtree, no root value, a value read only in part or not at
// all), the nil ordered tree, the values that are strings
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
 for( const std::string & s : { std::string( "( 5 () () )" ) ,
                                std::string( "( -3 () ( 0 () () ) )" ) ,
                                std::string( "( 2147483647 () () )" ) } ) {
  std::unique_ptr< BinaryTree< int > > X( from_list< int >( s ) );
  CHECK( to_list( X.get() ) == s );
  }
 std::unique_ptr< BinaryTree< int > > S( from_list< int >( "(7()())" ) );
 CHECK( S->read() == 7 && ! S->left() && ! S->right() );   // no spaces
 for( const char * s : { "" , "(" , ")" , "( 8 () () ) )" ,
                         "( 8 () () ) ( 1 () () )" , "( 8 () () () )" ,
                         "( () () )" , "( 8x () () )" , "( x () () )" } )
  CHECK_THROWS( from_list< int >( s ) , contract_violation );
 for( const std::string & s : { std::string( "( 5 )" ) ,
                                std::string( "( -1 ( -2 ( -3 ) ) )" ) } ) {
  std::unique_ptr< Tree< int > > X( tree_from_list< int >( s ) );
  CHECK( to_list( X.get() ) == s );
  }
 for( const char * s : { "" , "()" , "( 5 ) )" , "( 5 ) ( 6 )" , "( 5" ,
                         "( ( 5 ) )" , "( 5x )" } )
  CHECK_THROWS( tree_from_list< int >( s ) , contract_violation );
 CHECK_THROWS( to_list( static_cast< Tree< int > * >( nullptr ) ) ,
               contract_violation );
 std::unique_ptr< Tree< std::string > > W( tree_from_list< std::string >(
  "( ab ( cd ) )" ) );
 CHECK( W->leftmostChild()->read() == "cd" && to_list( W.get() ) ==
        "( ab ( cd ) )" );
 }

// the expression tree from the postfix form: the prefix and the infix
// forms, the value with the variables given, against the value computed
// directly; the edge cases: a single leaf, the signed numbers, a variable
// twice, the malformed expressions (empty, too many operands or
// operators), a division by 0, the nil tree, 10^4 operators in a chain
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
 using U = std::unique_ptr< BinaryTree< std::string > >;
 const U one( expressionTree( "  7  " ) );      // a single leaf
 CHECK( prefixForm( one.get() ) == "(7)" && infixForm( one.get() ) == "7" &&
        evaluate( one.get() ) == 7 );
 const U neg( expressionTree( "-3 +2.5 * .5 -" ) );   // signed numbers
 CHECK( infixForm( neg.get() ) == "( ( -3 * +2.5 ) - .5 )" &&
        evaluate( neg.get() ) == -8 );
 const U v( expressionTree( "x x -" ) );        // the same variable twice
 CHECK( evaluate( v.get() , { { "x" , 1e300 } } ) == 0 );
 for( const char * e : { "" , "   " , "1 2" , "+" , "+ 1 2" , "1 2 + +" } )
  CHECK_THROWS( expressionTree( e ) , contract_violation );
 const U d( expressionTree( "1 x 1 - /" ) );    // a division by 0
 CHECK_THROWS( evaluate( d.get() , { { "x" , 1 } } ) , contract_violation );
 CHECK( evaluate( d.get() , { { "x" , 3 } } ) == 0.5 );
 CHECK_THROWS( evaluate( nullptr ) , contract_violation );
 CHECK_THROWS( prefixForm( nullptr ) , contract_violation );
 CHECK_THROWS( infixForm( nullptr ) , contract_violation );
 std::string chain = "1";                       // 10^4 operators deep
 for( int i = 0 ; i < 10000 ; ++i ) chain += " 1 +";
 const U c( expressionTree( chain ) );
 CHECK( evaluate( c.get() ) == 10001 );
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
// random graphs the nodes reached are those of the visits; the edge cases:
// one node, a root with no edge out (an edge in only), an isolated node,
// a root that is not a node, the distances of the unreachable nodes, a
// star
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
 ListGraph<> one( 1 , true );                   // one node
 ListGraph<> back( 3 , true );                  // only 1 -> 0 , 2 alone
 back.insertEdge( 1 , 0 );
 for( Choice c : { Choice::NEWEST , Choice::OLDEST } ) {
  const auto [ R1 , B1 ] = genericVisit( one , 0 , c );
  CHECK( R1 == std::vector< int >( { 0 } ) && B1.empty() );
  CHECK( genericVisit( back , 0 , c ).first == std::vector< int >( { 0 } ) );
  CHECK( genericVisit( back , 2 , c ).first == std::vector< int >( { 2 } ) );
  CHECK( genericVisit( back , 1 , c ).first ==
         std::vector< int >( { 1 , 0 } ) );
  CHECK_THROWS( genericVisit( back , 3 , c ) , contract_violation );
  CHECK_THROWS( genericVisit( back , -1 , c ) , contract_violation );
  }
 const auto [ d1 , p1 ] = distance( back , 0 );
 CHECK( d1 == std::vector< int >( { 0 , INF , INF } ) &&
        p1 == std::vector< int >( { NIL , NIL , NIL } ) );
 ListGraph<> s( 6 , false );                    // a star: OLDEST takes the
 for( int v = 1 ; v < 6 ; ++v ) s.insertEdge( 0 , v );   // leaves in order
 const auto [ Rs , Bs ] = genericVisit( s , 0 , Choice::OLDEST );
 CHECK( Rs == std::vector< int >( { 0 , 1 , 2 , 3 , 4 , 5 } ) &&
        Bs.size() == 5 );
 }

// the shortest paths of the example: the distances and the tree of
// Dijkstra's algorithm on A , ... , G; the edge cases: a root with no
// edge, one node, a root that is not a node, the weights 0, an edge
// inserted again with a new weight, a loop refused, a negative edge with
// no cycle and a negative cycle, the distances just below infinity< int >,
// the infinite distance of double
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
 const auto R4 = dijkstra( g , 4 );             // a root with no edge
 CHECK( R4.d[ 4 ] == 0 && R4.T[ 4 ] == NIL &&
        std::count( R4.d.begin() , R4.d.end() , infinity< int >() ) == 6 );
 ListGraph< int > one( 1 , true );
 CHECK( dijkstra( one , 0 ).d == std::vector< int >( { 0 } ) &&
        dijkstra( one , 0 ).T == std::vector< int >( { NIL } ) );
 CHECK_THROWS( dijkstra( one , 1 ) , contract_violation );
 CHECK_THROWS( dijkstra( one , -1 ) , contract_violation );
 ListGraph< int > h( 3 , true );                // weights 0, then changed
 h.insertEdge( 0 , 1 , 0 );
 h.insertEdge( 1 , 2 , 0 );
 h.insertEdge( 0 , 2 , 5 );
 CHECK( dijkstra( h , 0 ).d == std::vector< int >( { 0 , 0 , 0 } ) );
 h.insertEdge( 0 , 1 , 7 );                     // no parallel edge
 CHECK( dijkstra( h , 0 ).d == std::vector< int >( { 0 , 7 , 5 } ) &&
        dijkstra( h , 0 ).T == std::vector< int >( { NIL , 0 , 0 } ) );
 CHECK_THROWS( h.insertEdge( 2 , 2 , 1 ) , contract_violation );   // loop
 ListGraph< int > k( 3 , true );                // negative, no cycle
 k.insertEdge( 0 , 1 , 5 );
 k.insertEdge( 0 , 2 , 2 );
 k.insertEdge( 2 , 1 , -4 );
 CHECK( dijkstra( k , 0 ).d == std::vector< int >( { 0 , -2 , 2 } ) &&
        dijkstra( k , 0 ).T[ 1 ] == 2 );
 k.insertEdge( 1 , 2 , 1 );                     // a negative cycle
 CHECK_THROWS( dijkstra( k , 0 ) , contract_violation );
 const int big = std::numeric_limits< int >::max() / 2 - 1;
 ListGraph< int > b( 3 , true );                // sums just below INF
 b.insertEdge( 0 , 1 , big );
 b.insertEdge( 1 , 2 , big );
 CHECK( dijkstra( b , 0 ).d[ 2 ] == 2 * big &&
        2 * big < infinity< int >() );
 ListGraph< double > f( 2 , false );            // floating point: +inf
 CHECK( dijkstra( f , 0 ).d[ 1 ] == infinity< double >() &&
        std::isinf( dijkstra( f , 0 ).d[ 1 ] ) );
 }

// the unilateral connection against the transitive closure on random
// oriented graphs, and the example of a graph connected but not strongly;
// the adjacent nodes read back from the matrix of incidence; the edge
// cases: one node, two with no edge (a matrix of no column), with an edge
// one way and both ways, two leaves of one root, a node with only edges
// in, a row that is not in the matrix
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
 CHECK( unilaterallyConnected( ListGraph<>( 1 , true ) ) );   // one node
 ListGraph<> two( 2 , true );                   // two, no edge
 CHECK( ! unilaterallyConnected( two ) );
 CHECK( incidence( two ) == std::vector< std::vector< int > >( 2 ) );
 CHECK( adjFromIncidence( incidence( two ) , 1 , true ).empty() );
 two.insertEdge( 1 , 0 );                       // one way is enough
 CHECK( unilaterallyConnected( two ) );
 two.insertEdge( 0 , 1 );                       // both ways
 CHECK( unilaterallyConnected( two ) && scc( two )[ 0 ] == scc( two )[ 1 ] );
 ListGraph<> out( 3 , true );                   // 0 -> 1 , 0 -> 2: no
 out.insertEdge( 0 , 1 );                       // path between 1 and 2
 out.insertEdge( 0 , 2 );
 CHECK( ! unilaterallyConnected( out ) );
 const auto Inc = incidence( out );
 CHECK( Inc == std::vector< std::vector< int > >( { { -1 , -1 } , { 1 , 0 } ,
                                                    { 0 , 1 } } ) );
 CHECK( adjFromIncidence( Inc , 1 , true ).empty() );   // only edges in
 CHECK_THROWS( adjFromIncidence( Inc , 3 , true ) , contract_violation );
 CHECK_THROWS( adjFromIncidence( Inc , -1 , false ) , contract_violation );
 }

// the definitions of O, Omega and Theta with their constants c and m,
// checked on [ m , N ]: 3 n^2 + 3 n - 1 <= 4 n^2 from n = 3 on and not
// at n = 2; 3 n^2 + 2 n + 1 is O( n^2 ), Omega( n ) and Theta( n^2 ), and
// not O( n ), its ratio to n going past any constant; n + 3 and
// 100 n + 3027 are both Theta( n ); the hierarchy of the orders of growth;
// the edge cases of the master theorem: a = 1, k = -1 and k < -1 on the
// boundary, beta above and below log_b a, log_b a an integer that the
// logarithms compute only nearly (10^e and 10), a < 1 and b = 1; the
// linear recurrences with a = 1, a = 2, 1 < a < 2, a < 1
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
 CHECK( master( 1 , 2 , 0 ) == Growth( { 1 , 0 , 1 } ) );   // log n
 CHECK( master( 1 , 2 , 0 , -1 ) == Growth( { 1 , 0 , 0 , 1 } ) &&
        master( 1 , 2 , 0 , -2 ) == Growth( { 1 , 0 } ) );  // k = -1 , -2
 CHECK( master( 2 , 2 , 1 , -1 ) == Growth( { 1 , 1 , 0 , 1 } ) &&
        master( 2 , 2 , 1 , 3 ) == Growth( { 1 , 1 , 4 } ) );
 CHECK( master( 7 , 2 , 2 ) > Growth( { 1 , 2.8 } ) &&
        master( 7 , 2 , 2 ) < Growth( { 1 , 2.81 } ) );       // n^log2 7
 CHECK( master( 2 , 2 , 2 , 5 ) == Growth( { 1 , 2 , 5 } ) );   // beta big
 for( int e = 1 ; e <= 6 ; ++e ) {               // log_b a an integer that
  const double b = 10 , a = std::pow( b , e );  // the logarithms miss
  CHECK( master( a , b , e ) == Growth( { 1 , double( e ) , 1 } ) );
  CHECK( master( a , b , 0 ) == Growth( { 1 , double( e ) } ) );
  CHECK( master( a , b , e + 1 ) == Growth( { 1 , double( e + 1 ) } ) );
  }
 CHECK_THROWS( master( 0.5 , 2 , 0 ) , contract_violation );
 CHECK_THROWS( master( 2 , 1 , 0 ) , contract_violation );
 CHECK( linear( 1 , 0 ) == Growth( { 1 , 1 } ) && ! linear( 1 , 0 ).upper );
 CHECK( linear( 2 , 1 ) == Growth( { 2 , 1 } ) && linear( 2 , 1 ).upper );
 CHECK( linear( 1.5 , 0 ) == Growth( { 1.5 , 0 } ) );   // 1 < a < 2
 CHECK_THROWS( linear( 0.5 , 0 ) , contract_violation );
 CHECK( Growth( { 1 , 0 } ) == Growth() && !( Growth() < Growth() ) );
 }

// the model of unit costs: an assignment, an operation or a comparison
// costs 1, a for of n rounds costs 2 + ( cost of the body + 3 ) n; the
// factorial f = 1 , for i = 1 to n do f = f i costs 1 + 2 + 5 n, 18 for
// n = 3; the minimum of n items costs a n + b; the sequential search costs
// 3 + 2 n in the worst case, and makes ( n + 1 ) / 2 comparisons on average
// when the item is there; the search of each item of w in v makes N^2
// comparisons when each is found last; the edge cases: n = 0, one item,
// the cost of the minimum the same on any data, the item found first, the
// empty list, the best case N of the search of each item
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
 CHECK( minCost( Ints( { 5 } ) ) == 4 );        // one item: no round
 CHECK( minCost( Ints( { 1 , 2 , 3 } ) ) ==     // the data do not count
        minCost( Ints( { 3 , 2 , 1 } ) ) );
 CHECK( search( L , 0 ) == 0 && search( List< int >() , 0 ) == -1 );
 cmp = 0;
 for( int i = 0 ; i < n ; ++i )                 // each found first: N
  for( int y : v ) {
   ++cmp;
   if( y == v[ 0 ] ) break;
   }
 CHECK( cmp == n );
 }

// the comparisons of the merge of two sorted lists of m and n items: at
// least min( m , n ), when the shorter one comes all first, at most
// m + n - 1, and m + k - 1 when the last item of the first list goes
// before the k-th of the second one (the comparisons of the audits
// skipped); the edge cases: the empty lists, one item each, the second
// list all first, all the items equal (those of the first list first)
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
 CHECK( merges( list( {} ) , list( {} ) ) == 0 &&
        merges( list( {} ) , list( { 1 , 2 } ) ) == 0 &&
        merges( list( { 1 , 2 } ) , list( {} ) ) == 0 );
 CHECK( merges( list( { 4 } ) , list( { 4 } ) ) == 1 );
 CHECK( merges( list( { 5 , 6 , 7 } ) , list( { 1 , 2 } ) ) == 2 );
 CHECK( merges( list( { 3 , 3 } ) , list( { 3 , 3 , 3 } ) ) == 2 );
 }

// KMP with the strong failure function: the occurrences of the naive
// search with no more comparisons than KMP, on the example and on random
// binary strings; the edge cases: the empty pattern and text, the pattern
// equal to the text and longer, all the characters equal, the overlapping
// occurrences, an occurrence at the end
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
 using Places = std::vector< std::size_t >;
 using FV = std::vector< long >;
 CHECK( strongFailure( "" ) == FV( { -1 } ) &&
        strongFailure( "aaaa" ) == FV( { -1 , -1 , -1 , -1 , 3 } ) &&
        strongFailure( "ab" ) == FV( { -1 , 0 , 0 } ) );
 for( const auto & [ T , P ] : { std::pair( "" , "" ) , std::pair( "" , "a" ) ,
       std::pair( "abc" , "" ) , std::pair( "abc" , "abc" ) ,
       std::pair( "ab" , "abc" ) , std::pair( "aaaaa" , "aa" ) ,
       std::pair( "aaaab" , "aab" ) , std::pair( "ba" , "a" ) } ) {
  long a = 0 , b = 0;
  CHECK( kmpStrong( T , P , &a ) == naiveMatching( T , P ) &&
         kmp( T , P , &b ) == naiveMatching( T , P ) && a <= b );
  }
 CHECK( kmpStrong( "aaaaa" , "aa" ) == Places( { 0 , 1 , 2 , 3 } ) );
 }

// ZAINO: the greedy heuristic by profit per volume against the optimum on
// the example, and the backtracking with the simple bound against the
// branch-and-bound and the dynamic programming on random instances; the
// edge cases, the objects given checked against the profit and C: no
// object, C = 0, one object or none fitting, all fitting, no profit, the
// ratios all equal, a volume 0, C < 0, the sizes that differ, a profit < 0
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
  CHECK( nb >= 1 && nk >= 1 && b.first == knapsack( P , V , C ).first );
  }
 using VL = std::vector< long >;
 auto all = [ & ]( const VL & P , const VL & V , long C , long best ) {
  for( const auto & r : { knapsackBacktrack( P , V , C ) ,
                          knapsackBB( P , V , C ) } ) {
   long pr = 0 , vo = 0;
   for( auto i : r.second ) { pr += P[ i ]; vo += V[ i ]; }
   CHECK( r.first == best && pr == best && vo <= C );
   }
  const auto gr = knapsackGreedy( P , V , C );
  long vo = 0;
  for( auto i : gr.second ) vo += V[ i ];
  CHECK( gr.first <= best && vo <= C && knapsack( P , V , C ).first == best );
  };
 all( VL() , VL() , 10 , 0 );                   // no object
 all( p , v , 0 , 0 );                          // no capacity
 all( p , v , 1 , 4 );                          // only the smallest fits
 all( VL( { 5 , 9 } ) , VL( { 6 , 7 } ) , 5 , 0 );   // none fits
 all( p , v , 100 , 17 );                       // all fit
 all( VL( 4 , 0 ) , VL( { 1 , 2 , 3 , 4 } ) , 5 , 0 );   // no profit
 all( VL( { 2 , 4 , 6 } ) , VL( { 1 , 2 , 3 } ) , 4 , 8 );   // equal ratios
 CHECK( knapsackGreedy( VL( { 2 , 4 , 6 } ) , VL( { 1 , 2 , 3 } ) , 4 ).first
        <= 8 );
 CHECK_THROWS( knapsackBacktrack( p , VL( { 2 , 0 , 3 } ) , 5 ) ,
               contract_violation );            // a volume 0
 CHECK_THROWS( knapsackBB( p , v , -1 ) , contract_violation );
 CHECK_THROWS( knapsackGreedy( p , VL( { 2 , 1 } ) , 5 ) ,
               contract_violation );            // sizes that differ
 CHECK_THROWS( knapsackBB( VL( { -1 , 4 , 7 } ) , v , 5 ) ,
               contract_violation );            // a negative profit
 // profits and volumes near a quarter of the largest long, whose cross
 // products overflow it, the denser object first all the same
 const long h = std::numeric_limits< long >::max() / 4;
 const VL bp = { h , 2 * h , 1 } , bv = { h , h , h };
 for( const auto & r : { knapsackGreedy( bp , bv , h ) ,
                         knapsackBB( bp , bv , h ) ,
                         knapsackBacktrack( bp , bv , h ) } )
  CHECK( r.first == 2 * h && r.second == Nodes( { 1 } ) );
 }

// the iterative backtracking of the n queens, the same numbers of
// solutions as the recursive one; the edge cases: n = 1, a stop at the
// first solution (and none to stop at for n = 2 , 3), the first solution
// in order, admissible() on the empty board, on a diagonal and on a
// column, n = 0 , -1 and 33
ADS_TEST( "nqueens-iterative" )
{
 const long known[] = { 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 };
 for( int n = 1 ; n <= 8 ; ++n ) {
  CHECK( nQueensIterative( n , []( const std::vector< int > & ) {
   return( false ); } ) == known[ n - 1 ] );
  CHECK( nQueens( n , []( const std::vector< int > & ) {
   return( false ); } ) == known[ n - 1 ] );
  }
 auto stop = []( const std::vector< int > & Q ) {   // the first one only
  CHECK( admissible( Q ) );
  return( true ); };
 for( int n : { 1 , 4 , 8 } )
  CHECK( nQueensIterative( n , stop ) == 1 && nQueens( n , stop ) == 1 );
 CHECK( nQueensIterative( 3 , stop ) == 0 && nQueens( 2 , stop ) == 0 );
 std::vector< int > first;                      // the same first solution
 nQueensIterative( 6 , [ & ]( const std::vector< int > & Q ) {
  first = Q; return( true ); } );
 CHECK( first == std::vector< int >( { 1 , 3 , 5 , 0 , 2 , 4 } ) );
 CHECK( admissible( {} ) && admissible( { 0 } ) && ! admissible( { 0 , 1 } ) &&
        ! admissible( { 2 , 2 } ) && admissible( { 1 , 3 , 0 , 2 } ) );
 for( auto f : { 0 , -1 } ) {
  CHECK_THROWS( nQueensIterative( f , stop ) , contract_violation );
  CHECK_THROWS( nQueens( f , stop ) , contract_violation );
  }
 CHECK_THROWS( nQueens( 33 , stop ) , contract_violation );
 }

// the sorts of the slides: Merge Sort skipping the merge of parts in
// order makes n - 1 comparisons on a sorted vector; the passes of Shell
// Sort on the example with the gaps 4, 2 and 1, and the gaps of Hibbard;
// Insertion Sort with the sentinel, 2 n - 3 comparisons on a sorted vector;
// the partition around a value; the minimum and the maximum together; the
// edge cases: the empty vector, one item, two reversed, all equal, the
// extremes of int, the cutoffs 0 (taken as 1) and past n, the stability
// of Merge Sort, n = 2 for the sentinel, the gaps of Hibbard for n <= 4, a
// gap past n, a gap 0 and the gaps not increasing, the partition of one
// item, around the extremes, of a range out of order or out of the vector
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
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 for( const Ints & v : { Ints() , Ints( { 5 } ) , Ints( { 2 , 1 } ) ,
                         Ints( 7 , 4 ) , Ints( { M , m , 0 , m , M } ) } ) {
  Ints s = v;
  std::sort( s.begin() , s.end() );
  for( std::size_t cut : { 0u , 1u , 3u , 100u } ) {   // 0 taken as 1
   Ints w = v;
   mergeSort( w , cut );
   CHECK( w == s );
   }
  Ints w = v;
  insertionSortSentinel( w );
  CHECK( w == s );
  w = v;
  shellSort( w , hibbardGaps( v.size() ) );
  CHECK( w == s );
  if( ! v.empty() ) {
   CHECK( minMax( v ) == std::pair( s.front() , s.back() ) &&
          minMaxDivide( v ) == std::pair( s.front() , s.back() ) );
   w = v;
   const std::size_t e = v.size() - 1;
   CHECK( partitionByValue( w , 0 , e , M ) == v.size() &&   // all <= M
          partitionByValue( w , 0 , e , m ) ==
          std::size_t( std::count( v.begin() , v.end() , m ) ) );
   }
  }
 using PI = std::pair< int , int >;            // stable: the earlier first
 std::vector< PI > St = { { 2 , 0 } , { 1 , 1 } , { 2 , 2 } , { 1 , 3 } ,
                          { 2 , 4 } };
 for( std::size_t cut : { 1u , 3u } ) {
  auto w = St;
  mergeSort( w , cut , []( const PI & a , const PI & b ) {
   return( a.first < b.first ); } );
  CHECK( w == std::vector< PI >( { { 1 , 1 } , { 1 , 3 } , { 2 , 0 } ,
                                   { 2 , 2 } , { 2 , 4 } } ) );
  }
 c = 0;
 A = { 1 , 2 };                                 // 2 n - 3 = 1 for n = 2
 insertionSortSentinel( A , count );
 CHECK( c == 1 );
 using G = std::vector< std::size_t >;
 CHECK( hibbardGaps( 0 ) == G( { 1 } ) && hibbardGaps( 3 ) == G( { 1 } ) &&
        hibbardGaps( 4 ) == G( { 1 , 3 } ) );
 Ints h = { 3 , 1 , 2 };
 shellSort( h , G( { 1 , 10 } ) );              // a gap past n
 CHECK( h == Ints( { 1 , 2 , 3 } ) );
 CHECK_THROWS( shellSort( h , G( { 0 } ) ) , contract_violation );
 CHECK_THROWS( shellSort( h , G( { 3 , 1 } ) ) , contract_violation );
 Ints q = { 9 , 1 , 9 };
 CHECK( partitionByValue( q , 1 , 1 , 0 ) == 0 &&   // one item
        partitionByValue( q , 1 , 1 , 1 ) == 1 && q == Ints( { 9 , 1 , 9 } ) );
 CHECK_THROWS( partitionByValue( q , 2 , 1 , 0 ) , contract_violation );
 CHECK_THROWS( partitionByValue( q , 0 , 3 , 0 ) , contract_violation );
 CHECK_THROWS( minMax( Ints() ) , contract_violation );
 CHECK_THROWS( minMaxDivide( Ints() ) , contract_violation );
 }

// instances of the classical examples: the selection of activities, the
// minimum spanning tree of the graph on a , ... , i, and a partition; the
// edge cases: no interval, one, two touching, all equal, nested, an empty
// one, the size against brute force on random sets; no node and one node,
// a forest of two trees with a parallel edge, all weights 0, an end out of
// range; the empty set, { 0 }, one item, an odd sum, an even sum that
// cannot be split, two equal items
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
 using IV = std::vector< std::pair< long , long > >;
 using SV = std::vector< std::size_t >;
 CHECK( independentSet( IV() ).empty() &&
        independentSet( IV( { { 2 , 3 } } ) ) == SV( { 0 } ) );
 CHECK( independentSet( IV( { { 3 , 5 } , { 1 , 3 } } ) ) ==
        SV( { 0 , 1 } ) );                      // touching: disjoint
 CHECK( independentSet( IV( 4 , { 0 , 9 } ) ) == SV( { 0 } ) );   // equal
 CHECK( independentSet( IV( { { 0 , 10 } , { 1 , 2 } , { 3 , 4 } } ) ) ==
        SV( { 1 , 2 } ) );                      // nested
 CHECK_THROWS( independentSet( IV( { { 4 , 4 } } ) ) , contract_violation );
 std::mt19937 g( 22 );                          // the size against brute force
 for( int t = 0 ; t < 200 ; ++t ) {
  IV J( g() % 9 );
  for( auto & [ a , b ] : J ) {
   a = long( g() % 10 );
   b = a + 1 + long( g() % 4 );
   }
  std::size_t best = 0;
  for( unsigned S = 0 ; S < ( 1u << J.size() ) ; ++S ) {
   bool ok = true;
   for( std::size_t x = 0 ; x < J.size() ; ++x )
    for( std::size_t y = x + 1 ; y < J.size() ; ++y )
     if( ( S >> x & 1 ) && ( S >> y & 1 ) && ( J[ x ].first < J[ y ].second )
         && ( J[ y ].first < J[ x ].second ) )
      ok = false;
   std::size_t k = 0;
   for( unsigned s = S ; s ; s &= s - 1 ) ++k;
   if( ok ) best = std::max( best , k );
   }
  CHECK( independentSet( J ).size() == best );
  }
 using WE = std::vector< WeightedEdge< int > >;
 CHECK( kruskal( 0 , WE() ).empty() && kruskal( 1 , WE() ).empty() );
 const auto F = kruskal( 4 , WE( { { 0 , 1 , 5 } , { 2 , 3 , 1 } ,
                                   { 0 , 1 , 2 } } ) );   // two trees,
 total = 0;                                     // a parallel edge
 for( const auto & e : F ) total += e.w;
 CHECK( F.size() == 2 && total == 3 );
 const auto Z = kruskal( 3 , WE( { { 0 , 1 , 0 } , { 1 , 2 , 0 } ,
                                   { 0 , 2 , 0 } } ) );   // all weights 0
 CHECK( Z.size() == 2 );
 CHECK_THROWS( kruskal( 2 , WE( { { 0 , 2 , 1 } } ) ) , contract_violation );
 CHECK( partition( std::vector< long >() ).has_value() &&   // two empty
        partition( std::vector< long >( { 0 } ) ).has_value() );
 CHECK( ! partition( std::vector< long >( { 1 } ) ) &&
        ! partition( std::vector< long >( { 1 , 2 } ) ) &&    // odd sum
        ! partition( std::vector< long >( { 1 , 1 , 4 } ) ) );   // even
 const auto Q = partition( std::vector< long >( { 7 , 7 } ) );
 CHECK( Q && Q->size() == 1 );
 }

// the eight puzzle with the spiral goal: the visit in breadth finds the
// 5 moves of the classical start, the backtracking in depth finds a
// solution within 5 moves and none within 4; the edge cases: the start
// already the goal (and 0 moves allowed), a board of the other parity,
// the boards 1 x 1 and 2 x 2, the hole in a corner, on a side and in the
// middle, n = 0, nmax < 0, a board that is not a configuration, n wrong
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
 const auto G = puzzleBFS( goal , goal , 3 );   // already at the goal
 CHECK( G && G->size() == 1 && puzzleDFS( goal , goal , 3 , 0 ) &&
        puzzleDFS( goal , goal , 3 , 0 )->size() == 1 );
 CHECK( ! puzzleDFS( start , goal , 3 , 0 ) );
 const auto H = solvePuzzle( puzzleGoal( 3 ) , 3 );
 CHECK( H && H->size() == 1 );
 Board odd = goal;                              // two tiles swapped: the
 std::swap( odd[ 0 ] , odd[ 1 ] );              // other half of the boards
 CHECK( ! puzzleBFS( odd , goal , 3 ) );
 Board unsolved = puzzleGoal( 3 );
 std::swap( unsolved[ 0 ] , unsolved[ 1 ] );
 CHECK( ! solvable( unsolved , 3 ) && ! solvePuzzle( unsolved , 3 ) );
 CHECK( puzzleGoal( 1 ) == Board( { 0 } ) &&
        puzzleSpiralGoal( 1 ) == Board( { 0 } ) &&
        puzzleMoves( Board( { 0 } ) , 1 ).empty() );
 const auto One = puzzleBFS( Board( { 0 } ) , Board( { 0 } ) , 1 );
 CHECK( One && One->size() == 1 );
 CHECK( puzzleSpiralGoal( 2 ) == Board( { 1 , 2 , 0 , 3 } ) );
 CHECK( puzzleMoves( goal , 3 ).size() == 4 &&  // the hole in the middle,
        puzzleMoves( puzzleGoal( 3 ) , 3 ).size() == 2 &&   // in a corner,
        puzzleMoves( start , 3 ).size() == 3 );            // on a side
 CHECK_THROWS( puzzleGoal( 0 ) , contract_violation );
 CHECK_THROWS( puzzleDFS( start , goal , 3 , -1 ) , contract_violation );
 CHECK_THROWS( puzzleMoves( Board( { 1 , 1 , 0 , 2 } ) , 2 ) ,
               contract_violation );            // not a configuration
 CHECK_THROWS( puzzleBFS( start , goal , 4 ) , contract_violation );
 }

// the general schemes: the enumerative search of an item against
// search(), the sort by enumerating the permutations until one has no
// inversion, the queens by the backtracking scheme, the knapsack by
// enumerating the subsets against the dynamic programming, the minimum and
// the maximum by divide et impera, the greedy change, the level of largest
// weight of a tree, and the maximal words for three orders; the edge
// cases: no word, two equal, none comparable; one state, found or not;
// nothing admissible; the root pruned, the root a leaf, a stop at the
// first leaf; no stage and no candidate; an instance small at once; one
// node, the negative weights, all the levels of equal weight, the nil tree
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
 const auto gt = []( const std::string & a , const std::string & b ) {
  return( a > b ); };
 CHECK( maximal( SV() , gt ).empty() &&
        maximal( SV( { "A" , "A" } ) , gt ) == SV( { "A" , "A" } ) );
 CHECK( maximal( W , []( const std::string & , const std::string & ) {
  return( false ); } ) == W );                   // none comparable: all
 for( int v : { 4 , 5 } ) {                     // one state, first and last
  const auto r = enumerateSearch( 0 , []( int i ) { return( i + 1 ); } ,
   []( int ) { return( true ); } , [ & ]( int ) { return( v == 4 ); } ,
   []( int i ) { return( i ); } );
  CHECK( bool( r ) == ( v == 4 ) );
  }
 CHECK( ! enumerateOptimize( 0 , []( int i ) { return( i + 1 ); } ,
   []( int i ) { return( i == 9 ); } , []( int ) { return( false ); } ,
   []( int i ) { return( i ); } , []( int i ) { return( i ); } ,
   []( int a , int b ) { return( a > b ); } ) );   // none admissible
 long leaves = 0;
 auto kids = []( const Ints & Q ) {
  return( std::vector< Ints >( { Q , Q } ) ); };
 CHECK( ! backtrack( Ints() , kids , []( const Ints & ) { return( true ); } ,
                     []( const Ints & ) { return( false ); } ,
                     [ & ]( const Ints & ) { ++leaves; return( false ); } ) &&
        leaves == 0 );                          // the root pruned
 CHECK( ! backtrack( Ints() , kids , []( const Ints & ) { return( false ); } ,
                     []( const Ints & ) { return( true ); } ,
                     [ & ]( const Ints & ) { ++leaves; return( false ); } ) &&
        leaves == 1 );                          // the root a leaf
 CHECK( backtrack( Ints() , []( const Ints & Q ) {
                    std::vector< Ints > C;
                    for( int c = 0 ; c < 3 ; ++c ) {
                     Ints R = Q;
                     R.push_back( c );
                     C.push_back( R ); }
                    return( C ); } ,
                   []( const Ints & ) { return( false ); } ,
                   []( const Ints & Q ) { return( Q.size() == 2 ); } ,
                   [ & ]( const Ints & ) { ++leaves; return( true ); } ) &&
        leaves == 2 );                          // stopped at the first
 using VL = std::vector< long >;
 CHECK( greedy< long >( 0 , []( int , const VL & ) { return( VL( { 1 } ) ); } ,
                        []( const VL & c ) { return( c[ 0 ] ); } ).empty() );
 CHECK( greedy< long >( 5 , []( int , const VL & ) { return( VL() ); } ,
                        []( const VL & c ) { return( c[ 0 ] ); } ).empty() );
 CHECK( divideEtImpera( 7 , []( int ) { return( true ); } ,
                        []( int x ) { return( 2 * x ); } ,
                        []( int ) { return( Ints() ); } ,
                        []( const Ints & ) { return( -1 ); } ) == 14 );
 std::unique_ptr< Tree< int > > O( tree_from_list< int >( "( 5 )" ) );
 CHECK( maxWeightLevel( O.get() ) == 0 );       // one node
 std::unique_ptr< Tree< int > > N( tree_from_list< int >(
  "( -9 ( -1 ( 3 ) ) ( -2 ( -3 ) ) )" ) );      // negative: the last level
 CHECK( maxWeightLevel( N.get() ) == 2 );       // -9 , -3 , 0
 std::unique_ptr< Tree< int > > Eq( tree_from_list< int >(
  "( 2 ( 1 ( 2 ) ) ( 1 ) )" ) );                // all 2: the first
 CHECK( maxWeightLevel( Eq.get() ) == 0 );
 CHECK_THROWS( maxWeightLevel( static_cast< Tree< int > * >( nullptr ) ) ,
               contract_violation );
 }

// the factorial by a loop and by the recursion, the same specification
// realized twice, up to the 20! that fits in 64 bits; the binomial by the
// factorials against the recursion; the numbers of Fibonacci kept between
// the calls, a number already known costing no new step; the edge cases:
// 0! and 1!, n = -1 and 21, F_0 , F_1 , F_2 and F_92, C( n , 0 ) and
// C( n , n ) in one call, the 2 C - 1 calls, k > n and the negative
// arguments, the row 67 of the triangle, the triangle of n = 0, a cache
// asked for out of range
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
 CHECK( factorial( 0 ) == 1 && factorialRec( 0 ) == 1 &&
        factorial( 1 ) == 1 );
 for( int n : { -1 , 21 } ) {
  CHECK_THROWS( factorial( n ) , contract_violation );
  CHECK_THROWS( factorialRec( n ) , contract_violation );
  }
 CHECK( fibonacci( 0 ) == 1 && fibonacci( 1 ) == 1 && fibonacci( 2 ) == 2 &&
        fibonacci( 92 ) == 12200160415121876738ULL );   // F_0 = F_1 = 1
 for( int n : { 0 , 1 , 2 , 92 } )
  CHECK( fibonacciTable( n ) == fibonacci( n ) );
 CHECK_THROWS( fibonacci( -1 ) , contract_violation );
 CHECK_THROWS( fibonacciTable( 93 ) , contract_violation );
 long calls = 0;
 CHECK( binomial( 67 , 0 , &calls ) == 1 && calls == 1 &&
        binomial( 67 , 67 ) == 1 && binomial( 0 , 0 ) == 1 );
 calls = 0;
 CHECK( binomial( 10 , 5 , &calls ) == 252 && calls == 2 * 252 - 1 );
 for( auto [ n , k ] : { std::pair( 5 , 6 ) , std::pair( 5 , -1 ) ,
                         std::pair( -1 , 0 ) } )
  CHECK_THROWS( binomial( n , k ) , contract_violation );
 const auto T = tartaglia( 67 );                // the last row that fits
 CHECK( T[ 67 ][ 33 ] == 14226520737620288370ULL && T[ 67 ][ 0 ] == 1 &&
        T[ 67 ][ 67 ] == 1 && T[ 67 ][ 34 ] == T[ 67 ][ 33 ] );
 CHECK( tartaglia( 0 ) == std::vector< std::vector< std::uint64_t > >(
        { { 1 } } ) );
 CHECK_THROWS( tartaglia( 68 ) , contract_violation );
 CHECK_THROWS( tartaglia( -1 ) , contract_violation );
 CHECK( binomialByFactorials( 0 , 0 ) == 1 &&
        binomialByFactorials( 20 , 10 ) == 184756 );
 CHECK_THROWS( binomialByFactorials( 21 , 1 ) , contract_violation );
 CHECK_THROWS( binomialByFactorials( 3 , 4 ) , contract_violation );
 FibonacciCache C;                              // F_0 and F_1 at the start
 CHECK( C.size() == 2 && C.value( 1 ) == 1 && C.size() == 2 );
 CHECK_THROWS( C.value( -1 ) , contract_violation );
 CHECK_THROWS( C.value( 93 ) , contract_violation );
 CHECK( C.size() == 2 );
 }

// the for on a range of C++ over every realization of SEQUENCE, the same
// items as the loop on the positions; the edge cases: the empty sequence
// of each realization, one item
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
 check( ArraySequence< int >() );               // the empty ones
 check( SinglyList< int >() );
 check( CursorList< int >() );
 check( List< int >( { 7 } ) );                 // one item
 check( SinglyList< int >( { 7 } ) );
 CHECK( std::ranges::distance( range( ArraySequence< int >() ) ) == 0 &&
        std::ranges::count( range( List< int >() ) , 1 ) == 0 );
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
// expressions; the edge cases: a single number, a negative value, the
// truncated division, the largest and the smallest long and every
// operation past them, a division by 0, the malformed expressions (empty,
// too many operands or operators, parentheses, a character that is not
// allowed), a number too large; the conversions of a single number, of
// redundant parentheses, of the same precedence, of unbalanced ones
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
 CHECK( evalPostfix( "42" ) == 42 && evalPostfix( "3 5 -" ) == -2 &&
        evalPostfix( "7 2 /" ) == 3 && evalPostfix( "0 7 /" ) == 0 );
 const std::string M =                          // the largest long, of
  std::to_string( std::numeric_limits< long >::max() );   // 32 or 64 bits
 const std::string m = "0 " + M + " - 1 -";     // the smallest one
 CHECK( evalPostfix( M + " 0 +" ) == std::numeric_limits< long >::max() &&
        evalPostfix( m ) == std::numeric_limits< long >::min() );
 for( const std::string & e : { M + " 1 +" , m + " 1 -" , m + " 0 1 - /" ,
                                m + " 0 1 - *" , M + " 2 *" ,
                                std::string( "7 0 /" ) , std::string( "" ) ,
                                std::string( "1 2" ) , std::string( "1 +" ) ,
                                std::string( "+" ) , std::string( "( 1 )" ) ,
                                std::string( "1 & 2" ) } )
  CHECK_THROWS( evalPostfix( e ) , contract_violation );
 CHECK_THROWS( evalPostfix( "99999999999999999999" ) , std::logic_error );
 CHECK( toPostfix( "7" ) == "7" && toPostfix( "((7))" ) == "7" &&
        toPostfix( "" ).empty() );
 CHECK( toPostfix( "1 * 2 / 3" ) == "1 2 * 3 /" &&   // the same precedence
        toPostfix( "1 + 2 * 3 - 4" ) == "1 2 3 * + 4 -" );
 CHECK_THROWS( toPostfix( "( 1 + 2" ) , contract_violation );
 CHECK_THROWS( toPostfix( "1 + 2 )" ) , contract_violation );
 CHECK( toPostfixParenthesized( "7" ) == "7" &&
        toPostfixParenthesized( "( 1 - 2 )" ) == "1 2 -" );
 CHECK_THROWS( toPostfixParenthesized( "1 + 2" ) , contract_violation );
 CHECK_THROWS( toPostfixParenthesized( "( 1 ) )" ) , contract_violation );
 }

// the order of a STACK and of a QUEUE on the classical traces, a letter
// being pushed (or enqueued) and an asterisk popping (or dequeuing), on
// every realization; the edge cases: the empty trace, the realizations on
// a vector of one place and of just the places needed (5 for the queue),
// one place too few, a pop or a dequeue of nothing
template< class S >
std::string lifo( S P , const std::string & trace )
{
 std::string out;
 for( char c : trace )
  if( c == '*' ) out += P.pop(); else P.push( c );
 CHECK( P.empty() );
 return( out );
 }

template< class Q >
std::string fifo( Q q , const std::string & trace )
{
 std::string out;
 for( char c : trace )
  if( c == '*' ) out += q.dequeue(); else q.enqueue( c );
 CHECK( q.empty() );
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
 CHECK( lifo( ArrayStack< char >( 1 ) , "" ).empty() &&   // no operation
        fifo( LinkedQueue< char >() , "" ).empty() );
 CHECK( lifo( ArrayStack< char >( 1 ) , "a*b*c*" ) == "abc" &&   // full
        fifo( ArrayQueue< char >( 1 ) , "a*b*c*" ) == "abc" );   // each time
 CHECK( fifo( ArrayQueue< char >( 5 ) , F ) == "FIRSTINFIRSTOUT" );   // just
 CHECK_THROWS( lifo( ArrayStack< char >( 1 ) , "ab**" ) ,     // big enough
               contract_violation );            // one push too many
 CHECK_THROWS( fifo( ArrayQueue< char >( 4 ) , F ) , contract_violation );
 CHECK_THROWS( lifo( SequenceStack< List< char > >() , "*" ) ,
               contract_violation );            // a pop of nothing
 CHECK_THROWS( fifo( LinkedQueue< char >() , "a**" ) , contract_violation );
 }

// the items of a stack from the bottom, the stack left as it was; the
// edge cases: the empty stack, one item in a full vector, 10^4 items
ADS_TEST( "stack-bottom-up" )
{
 ArrayStack< int > P( 10 );
 for( int i : { 4 , 8 , 15 , 16 } ) P.push( i );
 Ints v;
 forEachBottomUp( P , [ & ]( int x ) { v.push_back( x ); } );
 CHECK( v == Ints( { 4 , 8 , 15 , 16 } ) && P.top() == 16 );
 SequenceStack< List< int > > E;
 forEachBottomUp( E , [ & ]( int ) { CHECK( false ); } );
 CHECK( E.empty() );
 ArrayStack< int > O( 1 );                      // one item, a full vector
 O.push( 9 );
 v.clear();
 forEachBottomUp( O , [ & ]( int x ) { v.push_back( x ); } );
 CHECK( v == Ints( { 9 } ) && O.top() == 9 && O.size() == 1 );
 SequenceStack< List< int > > D;                // 10^4 items deep
 for( int i = 0 ; i < 10000 ; ++i ) D.push( i );
 long sum = 0 , last = -1;
 bool rising = true;
 forEachBottomUp( D , [ & ]( int x ) {
  rising = rising && ( x == last + 1 );
  last = x;
  sum += x; } );
 CHECK( rising && sum == 9999L * 10000 / 2 && D.top() == 9999 );
 }

// the positive items of a queue, the queue emptied or kept, and the queue
// with no duplicates under its two policies; the edge cases: the empty
// queue, no positive item (0 and the smallest int), an out queue not
// empty, the same queue twice, an item enqueued again after it left, an
// item moved to the back many times, one item, top() of nothing
ADS_TEST( "queue-algorithms" )
{
 LinkedQueue< int > q , p , r;
 for( int x : { 3 , -1 , 0 , 7 , -5 , 2 } ) q.enqueue( x );
 copyPositive( q , p );
 CHECK( q.size() == 6 && q.top() == 3 );
 extractPositive( q , r );
 CHECK( q.empty() && p == r );
 Ints v;
 while( ! r.empty() ) v.push_back( r.dequeue() );
 CHECK( v == Ints( { 3 , 7 , 2 } ) );
 CHECK_THROWS( copyPositive( p , p ) , contract_violation );
 UniqueQueue< int , Duplicate::IGNORE_NEW > I;
 UniqueQueue< int , Duplicate::FORGET_OLD > O;
 for( int x : { 1 , 2 , 3 , 2 } ) {
  I.enqueue( x );
  O.enqueue( x );
  }
 Ints a , b;
 while( ! I.empty() ) a.push_back( I.dequeue() );
 while( ! O.empty() ) b.push_back( O.dequeue() );
 CHECK( a == Ints( { 1 , 2 , 3 } ) && b == Ints( { 1 , 3 , 2 } ) );
 CHECK_THROWS( I.dequeue() , contract_violation );
 auto drain = []( auto & Q ) {
  Ints w;
  while( ! Q.empty() ) w.push_back( Q.dequeue() );
  return( w );
  };
 LinkedQueue< int > e , out;
 copyPositive( e , out );                       // the empty queue
 extractPositive( e , out );
 CHECK( e.empty() && out.empty() );
 for( int x : { 0 , -1 , std::numeric_limits< int >::min() } )
  q.enqueue( x );                               // nothing positive
 copyPositive( q , out );
 CHECK( out.empty() && q.size() == 3 );
 extractPositive( q , out );
 CHECK( out.empty() && q.empty() );
 out.enqueue( 9 );                              // out not empty: the back
 for( int x : { 1 , std::numeric_limits< int >::max() } ) q.enqueue( x );
 extractPositive( q , out );
 CHECK( drain( out ) == Ints( { 9 , 1 , std::numeric_limits< int >::max() } )
        && q.empty() );
 CHECK_THROWS( extractPositive( q , q ) , contract_violation );
 I.enqueue( 5 );
 CHECK( I.dequeue() == 5 && I.empty() );
 I.enqueue( 5 );                                // again, after it left
 I.enqueue( 5 );
 CHECK( I.size() == 1 && I.top() == 5 && I.contains( 5 ) &&
        ! I.contains( 6 ) );
 for( int x : { 1 , 1 , 2 , 1 , 2 , 1 } ) O.enqueue( x );   // to the back
 CHECK( drain( O ) == Ints( { 2 , 1 } ) );                // each time
 O.enqueue( 4 );                                // one item, again
 O.enqueue( 4 );
 CHECK( O.size() == 1 && drain( O ) == Ints( { 4 } ) );
 CHECK_THROWS( (void) O.top() , contract_violation );
 }

// the sieve of Eratosthenes on a set: the 25 primes up to 100, and the
// primes up to 10^4 as the divisions find them (the audit); the edge
// cases: n = 0 , 1 , 2 , 3 , 4, n the square of a prime, n prime or not
ADS_TEST( "sieve" )
{
 CHECK( sieve( 100 ).size() == 25 && sieve( 100 ).back() == 97 );
 CHECK( sieve( 1 ).empty() && sieve( 2 ) == std::vector< U64 >( { 2 } ) );
 CHECK( sieve( 10000 ).size() == 1229 );
 using UV = std::vector< U64 >;
 CHECK( sieve( 0 ).empty() && sieve( 3 ) == UV( { 2 , 3 } ) &&
        sieve( 4 ) == UV( { 2 , 3 } ) );
 CHECK( sieve( 25 ).back() == 23 && sieve( 49 ).back() == 47 &&   // squares
        sieve( 97 ).back() == 97 && sieve( 96 ).back() == 89 );   // n prime
 }

// the dictionary on a sorted vector: random operations against std::map,
// the keys always sorted; the edge cases: the empty dictionary, an absent
// key removed, the extreme keys of int at either end, a value replaced,
// all the keys removed
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
 SortedArrayDictionary< int , int > E;          // nothing in it
 CHECK( E.size() == 0 && ! E.lookup( 0 ) );
 E.remove( 0 );                                 // absent: nothing
 const int M = std::numeric_limits< int >::max() ,
           m = std::numeric_limits< int >::min();
 for( int k : { 0 , M , m } ) E.insert( k , k );   // at either end
 E.insert( M , 1 );                             // the value replaced
 CHECK( E.size() == 3 && E.lookup( M ) == 1 && E.lookup( m ) == m &&
        E.pairs().front().first == m && E.pairs().back().first == M );
 E.remove( m );
 E.remove( M );
 CHECK( E.size() == 1 && E.pairs().front().first == 0 && ! E.lookup( m ) );
 E.remove( 0 );
 CHECK( E.size() == 0 );
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
