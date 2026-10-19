/*--------------------------------------------------------------------------*/
/*----------------------------- File ch10.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 10, Strutture dati speciali: the priority queues on a
 * heap, on lists and on a tree against a reference on random operations,
 * the heap of Figure 10.1-10.4, heapBuild() and heapsort(), the MFSET on a
 * forest with and without its two heuristics on the examples of Figure
 * 10.5 and 10.6, the Teorema 10.1, the MFSET on lists, the incremental
 * components, and the exercises 10.1 to 10.9, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 10,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#include "ADS/graph/Visit.h"
#include "ADS/mfset/ListMFSet.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/priority/TreePriorityQueue.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

static_assert( PriorityQueue< HeapPriorityQueue< int > > );
static_assert( PriorityQueue< HeapPriorityQueue< int , double , 4 > > );
static_assert( PriorityQueue< ListPriorityQueue< int , int , true > > );
static_assert( PriorityQueue< ListPriorityQueue< int , int , false > > );
static_assert( PriorityQueue< TreePriorityQueue< int > > );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue against a vector of the items present, on random
/// insertions, decreases and deletions; the item is its number, so that
/// deleteMin() must give one of the items of smallest priority
template< class Q , class M >
void against_ref( M make , unsigned seed )
{
 std::mt19937 g( seed );
 Q q = make();
 std::map< int , std::pair< typename Q::Handle , int > > in;   // item ->
 int next = 0;
 for( int o = 0 ; o < 20000 ; ++o ) {
  const int what = g() % 10;
  if( ( what < 4 ) && ( in.size() < 1000 ) ) {
   const int p = g() % 1000;
   in.emplace( next , std::pair( q.insert( next , p ) , p ) );
   ++next;
   }
  else if( ( what < 7 ) && ! in.empty() ) {
   auto it = in.begin();
   std::advance( it , g() % in.size() );
   const int p = it->second.second - int( g() % 50 );
   q.decrease( it->second.first , p );
   it->second.second = p;
   }
  else if( ! in.empty() ) {
   int best = in.begin()->second.second;
   for( const auto & [ x , hp ] : in )
    best = std::min( best , hp.second );
   const int x = q.deleteMin();
   CHECK( in.count( x ) && in.at( x ).second == best );
   in.erase( x );
   }
  CHECK( q.empty() == in.empty() );
  }
 while( ! in.empty() ) {
  const int x = q.deleteMin();
  CHECK( in.count( x ) );
  int best = in.at( x ).second;
  for( const auto & [ y , hp ] : in )
   CHECK( best <= hp.second );
  in.erase( x );
  }
 CHECK( q.empty() );
 }

/// the degenerate cases of a priority queue of capacity at least 100: the
/// empty queue (its contracts), one item, a decrease to the same priority
/// and one that would raise it, all the priorities equal (each item leaves
/// once, twice after emptying the queue), the extreme priorities of int
template< class Q , class M >
void degenerate( M make )
{
 Q q = make();
 CHECK( q.empty() && q.size() == 0 );
 CHECK_THROWS( (void) q.min() , contract_violation );
 CHECK_THROWS( q.deleteMin() , contract_violation );
 auto h = q.insert( 1 , 5 );
 CHECK( q.size() == 1 && q.min() == 1 );
 q.decrease( h , 5 );
 CHECK_THROWS( q.decrease( h , 6 ) , contract_violation );
 CHECK( q.min() == 1 && q.deleteMin() == 1 && q.empty() );
 for( int round = 0 ; round < 2 ; ++round ) {
  for( int x = 0 ; x < 100 ; ++x )
   q.insert( x , 7 );
  CHECK( q.size() == 100 );
  Ints out;
  while( ! q.empty() )
   out.push_back( q.deleteMin() );
  std::sort( out.begin() , out.end() );
  bool all = int( out.size() ) == 100;
  for( int x = 0 ; all && ( x < 100 ) ; ++x )
   all = out[ x ] == x;
  CHECK( all );
  }
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 q.insert( 1 , hi );
 auto g = q.insert( 2 , hi );
 q.insert( 3 , lo );
 q.insert( 4 , 0 );
 q.decrease( g , lo );
 const int a = q.deleteMin() , b = q.deleteMin();
 CHECK( std::min( a , b ) == 2 && std::max( a , b ) == 3 );
 CHECK( q.deleteMin() == 4 && q.deleteMin() == 1 && q.empty() );
 }

/// a comparison that counts
struct Counting {
 long * c;
 bool operator()( const auto & a , const auto & b ) const {
  *c += ! checkingContract();               // not those of the contracts
  return( a < b );
  }
 };

/// the maxHeapRestore() of the book, recursive
void restore_rec( Ints & A , std::size_t i , std::size_t dim )
{
 std::size_t max = i;
 if( 2 * i + 1 < dim && A[ 2 * i + 1 ] > A[ max ] ) max = 2 * i + 1;
 if( 2 * i + 2 < dim && A[ 2 * i + 2 ] > A[ max ] ) max = 2 * i + 2;
 if( i != max ) {
  std::swap( A[ i ] , A[ max ] );
  restore_rec( A , max , dim );
  }
 }

/// the height of the tree of x in a forest of parents, from its root
int height( const MFSet & M , int r )
{
 int h = 0;
 for( int x = 0 ; x < M.n() ; ++x ) {
  int d = 0 , y = x;
  while( M.parent( y ) != y ) {
   y = M.parent( y );
   ++d;
   }
  if( y == r )
   h = std::max( h , d );
  }
 return( h );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "spec.HeapPriorityQueue" )
{
 against_ref< HeapPriorityQueue< int > >(
  [] { return( HeapPriorityQueue< int >( 1000 ) ); } , 1 );
 HeapPriorityQueue< int > q( 2 );
 CHECK_THROWS( (void) q.min() , std::logic_error );
 CHECK_THROWS( q.deleteMin() , std::logic_error );
 auto h = q.insert( 7 , 5 );
 q.insert( 8 , 6 );
 CHECK_THROWS( q.insert( 9 , 1 ) , contract_violation );
 CHECK_THROWS( q.decrease( h , 6 ) , contract_violation );
 CHECK( q.min() == 7 && q.minPriority() == 5 && q.size() == 2 );

 // the degenerate cases; the capacities 0 and 1, a full queue emptied and
 // filled again, a null handle and one of another queue, a negative
 // capacity
 degenerate< HeapPriorityQueue< int > >(
  [] { return( HeapPriorityQueue< int >( 100 ) ); } );
 HeapPriorityQueue< int > z( 0 );
 CHECK( z.empty() && z.capacity() == 0 );
 CHECK_THROWS( z.insert( 1 , 1 ) , contract_violation );
 HeapPriorityQueue< int > one( 1 );
 for( int k = 0 ; k < 3 ; ++k ) {
  one.insert( k , -k );
  CHECK_THROWS( one.insert( 9 , 0 ) , contract_violation );
  CHECK( one.minPriority() == -k && one.deleteMin() == k );
  }
 CHECK_THROWS( q.decrease( nullptr , 0 ) , contract_violation );
 HeapPriorityQueue< int > other( 2 );
 auto o = other.insert( 1 , 9 );
 CHECK_THROWS( q.decrease( o , 0 ) , contract_violation );
 CHECK_THROWS( HeapPriorityQueue< int >( -1 ) , contract_violation );
 }

ADS_TEST( "spec.ListPriorityQueue" )
{
 against_ref< ListPriorityQueue< int , int , true > >(
  [] { return( ListPriorityQueue< int , int , true >() ); } , 2 );
 against_ref< ListPriorityQueue< int , int , false > >(
  [] { return( ListPriorityQueue< int , int , false >() ); } , 3 );
 ListPriorityQueue< int > q;
 CHECK_THROWS( (void) q.min() , std::logic_error );

 // the degenerate cases, sorted and unsorted; the handle of an item
 // removed is no longer one of the queue (checked with the audits)
 degenerate< ListPriorityQueue< int , int , true > >(
  [] { return( ListPriorityQueue< int , int , true >() ); } );
 degenerate< ListPriorityQueue< int , int , false > >(
  [] { return( ListPriorityQueue< int , int , false >() ); } );
 auto h = q.insert( 1 , 1 );
 auto k = q.insert( 2 , 2 );
 CHECK( q.deleteMin() == 1 );
 CHECK_THROWS( q.decrease( h , 0 ) , contract_violation );
 q.decrease( k , 0 );
 CHECK( q.deleteMin() == 2 && q.empty() );
 }

ADS_TEST( "spec.TreePriorityQueue" )
{
 against_ref< TreePriorityQueue< int > >(
  [] { return( TreePriorityQueue< int >() ); } , 4 );

 // the degenerate cases; the handle of an item removed, a negative one
 // and one never given
 degenerate< TreePriorityQueue< int > >(
  [] { return( TreePriorityQueue< int >() ); } );
 TreePriorityQueue< int > q;
 auto h = q.insert( 1 , 1 );
 q.insert( 2 , 2 );
 CHECK( q.deleteMin() == 1 );
 CHECK_THROWS( q.decrease( h , 0 ) , contract_violation );
 CHECK_THROWS( q.decrease( -1 , 0 ) , contract_violation );
 CHECK_THROWS( q.decrease( 2 , 0 ) , contract_violation );
 CHECK( q.deleteMin() == 2 && q.empty() );
 }

/*--------------------------------------------------------------------------*/

// Figure 10.1 and 10.2: the heap of C = { 5 , 10 , 8 , 11 , 13 , 12 , 9 ,
// 18 , 3 , 6 }, built by inserting its vector in order (each item is not
// smaller than its parent, so nothing moves); Figura 10.3, the deletion of
// the minimum; Figura 10.4, the insertion of 4; a decrease of the root and
// one to the same priority, the heap emptied in order
ADS_TEST( "sec10.1.3.heap" )
{
 const Ints fig = { 3 , 5 , 9 , 6 , 8 , 13 , 12 , 11 , 18 , 10 };
 auto build = [ & ] {
  HeapPriorityQueue< int > q( 20 );
  for( int x : fig )
   q.insert( x , x );
  return( q );
  };
 auto q = build();
 CHECK( q.priorities() == fig );
 // h = 3, 2^3 - 1 = 7 nodes above, the 3 leaves of level 3 to the left
 CHECK( std::floor( std::log2( fig.size() ) ) == 3 );
 CHECK( q.deleteMin() == 3 );
 CHECK( q.priorities() == Ints( { 5 , 6 , 9 , 10 , 8 , 13 , 12 , 11 , 18 } ) );
 auto r = build();
 r.insert( 4 , 4 );
 CHECK( r.priorities() ==
        Ints( { 3 , 4 , 9 , 6 , 5 , 13 , 12 , 11 , 18 , 10 , 8 } ) );
 // decrease() moves an item up, its handle follows it
 HeapPriorityQueue< int >::Handle h = nullptr;
 HeapPriorityQueue< int > t( 20 );
 for( int x : fig ) {
  auto y = t.insert( x , x );
  if( x == 18 ) h = y;
  }
 t.decrease( h , 1 );
 CHECK( h->pos == 0 && t.min() == 18 && t.minPriority() == 1 );
 CHECK( t.priorities() ==
        Ints( { 1 , 3 , 9 , 5 , 8 , 13 , 12 , 11 , 6 , 10 } ) );
 // the root decreased stays where it is; a leaf decreased to its own
 // priority does not move; the heap emptied gives its items in order
 t.decrease( h , 0 );
 CHECK( h->pos == 0 && t.priorities()[ 1 ] == 3 );
 const auto leaf = t.insert( 20 , 20 );
 t.decrease( leaf , 20 );
 CHECK( leaf->pos == 10 );
 Ints out;
 while( ! t.empty() )
  out.push_back( t.deleteMin() );
 CHECK( out == Ints( { 18 , 3 , 5 , 6 , 8 , 9 , 10 , 11 , 12 , 13 , 20 } ) );
 }

// heapBuild() in O( n ), at most 2 n comparisons, and heapsort(), and the
// sort() of the example with a priority queue; all equal, sorted, reversed
// and extreme items, a decreasing order, heaps of 0 and 1 items, a restore
// at a leaf and on a prefix, the contracts
ADS_TEST( "sec10.1.4.heapsort" )
{
 std::mt19937 g( 5 );
 for( int n : { 0 , 1 , 2 , 3 , 10 , 1000 , 50000 } ) {
  Ints A( n );
  for( auto & a : A )
   a = g() % 1000;
  Ints B = A , C = A , D = A;
  long c = 0;
  heapBuild( B , Counting{ &c } );
  CHECK( isMaxHeap( B , B.size() ) && c <= 2L * n );
  heapsort( C );
  std::sort( D.begin() , D.end() );
  CHECK( C == D );
  HeapPriorityQueue< int > Q( n );
  for( int a : A )
   Q.insert( a , a );
  for( auto & a : A )
   a = Q.deleteMin();
  CHECK( A == D );
  }
 // the degenerate inputs: all equal, sorted, reversed, the extremes of
 // int, the reversed order (a decreasing sort); a heap of 0 or 1 items,
 // a restore at a leaf, the contracts
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 for( const Ints & A : { Ints( 50 , 4 ) , Ints( { 1 , 2 , 3 , 4 , 5 , 6 } ) ,
                         Ints( { 6 , 5 , 4 , 3 , 2 , 1 } ) ,
                         Ints( { hi , lo , 0 , hi , lo , -1 , 1 } ) } ) {
  Ints C = A , D = A , E = A;
  heapsort( C );
  std::sort( D.begin() , D.end() );
  CHECK( C == D );
  heapsort( E , std::greater<>() );
  CHECK( std::equal( E.begin() , E.end() , D.rbegin() ) );
  heapBuild( D );
  CHECK( isMaxHeap( D , D.size() ) && D[ 0 ] == C.back() );
  }
 Ints Z , O = { 3 };
 CHECK( isMaxHeap( Z , 0 ) && isMaxHeap( O , 1 ) );
 CHECK( isMaxHeap( Ints( { 1 , 2 } ) , 1 ) && ! isMaxHeap( Ints( { 1 , 2 } ) ,
                                                           2 ) );
 Ints L = { 1 , 5 , 7 };
 maxHeapRestore( L , 2 , 3 );
 CHECK( L == Ints( { 1 , 5 , 7 } ) );
 maxHeapRestore( L , 0 , 2 );                      // only the prefix
 CHECK( L == Ints( { 5 , 1 , 7 } ) );
 CHECK_THROWS( maxHeapRestore( L , 3 , 3 ) , contract_violation );
 CHECK_THROWS( maxHeapRestore( L , 0 , 4 ) , contract_violation );
 CHECK_THROWS( maxHeapRestore( Z , 0 , 0 ) , contract_violation );
 CHECK_THROWS( (void) isMaxHeap( L , 4 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

// the example of the MFSET of 4 elements, and the forest of Figura 10.5 by
// merge( 0 , 1 ), merge( 0 , 2 ), merge( 4 , 5 ), merge( 5 , 2 ),
// merge( 3 , 6 ), merge( 2 , 6 ) without the rank (the figure counts the
// elements from 1); no element, one, merges of an element with itself and
// repeated, the tie of the union by size, the contracts
ADS_TEST( "sec10.2.2.forest" )
{
 MFSet S( 4 );
 CHECK( S.find( 0 ) != S.find( 2 ) && S.merge( 0 , 2 ) );
 CHECK( S.find( 3 ) != S.find( 1 ) && S.merge( 3 , 1 ) );
 CHECK( S.find( 2 ) != S.find( 3 ) && S.merge( 2 , 3 ) );
 CHECK( S.count() == 1 && ! S.merge( 0 , 1 ) );

 MFSet F( 7 , MFSet::Union::NONE , false );
 for( auto [ x , y ] : std::vector< std::pair< int , int > >(
       { { 0 , 1 } , { 0 , 2 } , { 4 , 5 } , { 5 , 2 } , { 3 , 6 } ,
         { 2 , 6 } } ) )
  F.merge( x , y );
 Ints p( 7 );
 for( int x = 0 ; x < 7 ; ++x )
  p[ x ] = F.parent( x );
 CHECK( p == Ints( { 4 , 0 , 0 , 4 , 4 , 4 , 3 } ) );
 CHECK_THROWS( F.find( 7 ) , contract_violation );

 // the degenerate cases: no element, one, an element merged with itself
 // or twice with the same one (false, nothing changes), the tie of the
 // union by size (the root of y under that of x), the contracts of each
 // operation, for each union
 for( auto u : { MFSet::Union::NONE , MFSet::Union::RANK ,
                 MFSet::Union::SIZE } ) {
  MFSet E( 0 , u );
  CHECK( E.n() == 0 && E.count() == 0 );
  CHECK_THROWS( E.find( 0 ) , contract_violation );
  MFSet O( 1 , u );
  CHECK( O.find( 0 ) == 0 && ! O.merge( 0 , 0 ) && O.count() == 1 );
  MFSet M( 3 , u );
  CHECK( ! M.merge( 1 , 1 ) && M.count() == 3 && M.parent( 1 ) == 1 );
  CHECK( M.merge( 1 , 2 ) && ! M.merge( 2 , 1 ) && M.count() == 2 );
  CHECK( M.same( 1 , 2 ) && ! M.same( 0 , 2 ) && M.same( 0 , 0 ) );
  if( u == MFSet::Union::SIZE )
   CHECK( M.parent( 2 ) == 1 && M.size( 1 ) == 2 );
  for( int x : { -1 , 3 } ) {
   CHECK_THROWS( M.find( x ) , contract_violation );
   CHECK_THROWS( M.merge( 0 , x ) , contract_violation );
   CHECK_THROWS( M.merge( x , 0 ) , contract_violation );
   CHECK_THROWS( (void) M.same( x , 0 ) , contract_violation );
   CHECK_THROWS( (void) M.parent( x ) , contract_violation );
   CHECK_THROWS( (void) M.rank( x ) , contract_violation );
   CHECK_THROWS( (void) M.size( x ) , contract_violation );
   }
  CHECK_THROWS( MFSet( -1 , u ) , contract_violation );
  }
 }

// without the rank, merge( 4 , 5 ), merge( 3 , 4 ), ..., merge( 0 , 1 )
// make a chain of height n - 1; with it, the height is 1; the Teorema
// 10.1, a tree of root r has at least 2^rank[ r ] nodes; the same with the
// union by size, height at most log2 size[ r ]
ADS_TEST( "sec10.2.3.rank" )
{
 for( bool rank : { false , true } ) {
  MFSet M( 6 , rank ? MFSet::Union::RANK : MFSet::Union::NONE , false );
  for( int x = 4 ; x >= 0 ; --x )
   M.merge( x , x + 1 );
  CHECK( height( M , M.find( 0 ) ) == ( rank ? 1 : 5 ) );
  }
 // the union by size: the same chain of merges has height 1, and a tree
 // of root r has height at most log2 size[ r ], on random merges
 MFSet B( 6 , MFSet::Union::SIZE , false );
 for( int x = 4 ; x >= 0 ; --x )
  B.merge( x , x + 1 );
 CHECK( height( B , B.find( 0 ) ) == 1 && B.size( B.find( 0 ) ) == 6 );
 std::mt19937 r( 17 );
 for( int t = 0 ; t < 10 ; ++t ) {
  const int n = 300;
  MFSet M( n , MFSet::Union::SIZE , false );
  for( int k = 0 ; k < 400 ; ++k )
   M.merge( r() % n , r() % n );
  for( int x = 0 ; x < n ; ++x )
   if( M.parent( x ) == x )
    CHECK( height( M , x ) <= std::log2( double( M.size( x ) ) ) );
  }
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 300;
  MFSet M( n , MFSet::Union::RANK , false );
  for( int k = 0 ; k < 400 ; ++k )
   M.merge( g() % n , g() % n );
  std::map< int , int > size;
  for( int x = 0 ; x < n ; ++x )
   ++size[ M.find( x ) ];
  for( auto [ r , s ] : size ) {
   CHECK( s >= ( 1 << M.rank( r ) ) );
   CHECK( M.rank( r ) <= std::log2( double( n ) ) );
   }
  }
 }

// Figura 10.6: find( 7 ) makes 7 and 4 children of the root (the figure
// counts the elements from 1 and says "trova( 8 , S )")
ADS_TEST( "sec10.2.4.compression" )
{
 MFSet M( 8 , MFSet::Union::NONE , true );
 for( auto [ x , y ] : std::vector< std::pair< int , int > >(
       { { 4 , 7 } , { 3 , 4 } , { 3 , 6 } , { 2 , 5 } , { 0 , 1 } ,
         { 0 , 2 } , { 0 , 3 } } ) )
  M.merge( x , y );
 Ints p( 8 );
 for( int x = 0 ; x < 8 ; ++x )
  p[ x ] = M.parent( x );
 CHECK( p == Ints( { 0 , 0 , 0 , 0 , 3 , 2 , 3 , 4 } ) );
 CHECK( M.find( 7 ) == 0 );
 for( int x = 0 ; x < 8 ; ++x )
  p[ x ] = M.parent( x );
 CHECK( p == Ints( { 0 , 0 , 0 , 0 , 0 , 2 , 3 , 0 } ) );

 // a chain of n, then n finds of its deepest element: without the
 // compression each one climbs n - 1 steps, with it the first climbs
 // n - 1 and the others 1
 const int n = 2000;
 for( bool compress : { false , true } ) {
  MFSet C( n , MFSet::Union::NONE , compress );
  for( int x = n - 2 ; x >= 0 ; --x )
   C.merge( x , x + 1 );
  const long before = C.steps();
  for( int k = 0 ; k < n ; ++k )
   C.find( n - 1 );
  CHECK( C.steps() - before ==
         ( compress ? 2L * ( n - 1 ) : long( n ) * ( n - 1 ) ) );
  }
 }

// the components of a graph by an MFSET, as the cc() of chapter 9, and
// then with new edges; no node, no edge, a deleted node, a path
ADS_TEST( "sec10.2.5.incremental" )
{
 std::mt19937 g( 11 );
 ListGraph<> G( 60 , false );
 for( int k = 0 ; k < 40 ; ++k ) {
  const int u = g() % 60 , v = g() % 60;
  if( u != v )
   G.insertEdge( u , v );
  }
 auto M = components( G );
 for( int k = 0 ; k < 30 ; ++k ) {
  const auto id = cc( G );
  for( int u = 0 ; u < 60 ; ++u )
   for( int v = 0 ; v < 60 ; ++v )
    CHECK( ( id[ u ] == id[ v ] ) == ( M.find( u ) == M.find( v ) ) );
  const int u = g() % 60 , v = ( u + 1 + g() % 59 ) % 60;
  G.insertEdge( u , v );
  M.merge( u , v );
  }
 // no node, no edge (a component per node), a deleted node (a component of
 // its own, as documented), a path (one component)
 CHECK( components( ListGraph<>( 0 , false ) ).count() == 0 );
 CHECK( components( ListGraph<>( 5 , false ) ).count() == 5 );
 ListGraph<> H( 3 , false );
 H.insertEdge( 0 , 2 );
 H.deleteNode( 1 );
 auto C = components( H );
 CHECK( C.count() == 2 && C.same( 0 , 2 ) && ! C.same( 0 , 1 ) );
 const int n = 1000;
 ListGraph<> P( n , false );
 for( int u = 0 ; u + 1 < n ; ++u )
  P.insertEdge( u , u + 1 );
 CHECK( components( P ).count() == 1 );
 }

// the MFSET on lists against the one on a forest; an element is relabeled
// at most log2 n times, exactly in the merges of lists of equal length; no
// element, one, the contracts
ADS_TEST( "sec10.2.1.lists" )
{
 std::mt19937 g( 13 );
 const int n = 1024;
 ListMFSet L( n );
 MFSet F( n );
 for( int k = 0 ; k < 3000 ; ++k ) {
  const int x = g() % n , y = g() % n;
  CHECK( L.merge( x , y ) == F.merge( x , y ) );
  const int a = g() % n , b = g() % n;
  CHECK( ( L.find( a ) == L.find( b ) ) == ( F.find( a ) == F.find( b ) ) );
  }
 CHECK( L.relabels() <= long( n ) * 10 );

 // the worst case of the bound: merging lists of equal length, round by
 // round, relabels n / 2 elements per round, n / 2 log2 n in all; no
 // element, one, an element merged with itself, the contracts
 ListMFSet W( n );
 for( int s = 1 ; s < n ; s *= 2 )
  for( int x = 0 ; x < n ; x += 2 * s )
   CHECK( W.merge( x , x + s ) );
 CHECK( W.relabels() == long( n ) / 2 * 10 );
 CHECK( int( W.members( 7 ).size() ) == n && W.find( 0 ) == W.find( n - 1 ) );
 ListMFSet E( 0 );
 CHECK( E.n() == 0 );
 CHECK_THROWS( (void) E.find( 0 ) , contract_violation );
 ListMFSet O( 1 );
 CHECK( ! O.merge( 0 , 0 ) && O.relabels() == 0 );
 CHECK( O.members( 0 ) == Ints( { 0 } ) );
 for( int x : { -1 , n } ) {
  CHECK_THROWS( (void) L.find( x ) , contract_violation );
  CHECK_THROWS( L.merge( 0 , x ) , contract_violation );
  CHECK_THROWS( (void) L.members( x ) , contract_violation );
  }
 CHECK_THROWS( ListMFSet( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the heap does not order its keys: its invisit is not sorted, and a sort
// by comparisons needs log2( n! ) of them, which heapsort() exceeds, while
// heapBuild() takes O( n ): the order cannot be read from a heap in O( n );
// the bound also on 1, 2 and 3 items
ADS_TEST( "ex10.1" )
{
 // the invisit of the tree of Figura 10.1
 const Ints H = { 3 , 5 , 9 , 6 , 8 , 13 , 12 , 11 , 18 , 10 };
 Ints in;
 auto rec = [ & ]( auto & self , std::size_t i ) -> void {
  if( i >= H.size() ) return;
  self( self , 2 * i + 1 );
  in.push_back( H[ i ] );
  self( self , 2 * i + 2 );
  };
 rec( rec , 0 );
 CHECK( ! std::is_sorted( in.begin() , in.end() ) );
 std::mt19937 g( 1 );
 for( int n : { 100 , 1000 , 10000 } ) {
  Ints A( n );
  for( int i = 0 ; i < n ; ++i )
   A[ i ] = i;
  std::shuffle( A.begin() , A.end() , g );
  long c = 0;
  heapsort( A , Counting{ &c } );
  CHECK( c >= std::lgamma( n + 1.0 ) / std::log( 2.0 ) );
  }
 // the smallest: 0 comparisons on 1 item, 1 on 2, 3 on 3 (heapBuild 2,
 // then 1), at least ceil( log2 n! ) = 0 , 1 , 3
 for( const Ints & B : { Ints( { 0 } ) , Ints( { 1 , 0 } ) ,
                         Ints( { 2 , 0 , 1 } ) } ) {
  Ints A = B;
  long c = 0;
  heapsort( A , Counting{ &c } );
  CHECK( std::is_sorted( A.begin() , A.end() ) );
  CHECK( c == ( A.size() == 3 ? 3 : long( A.size() ) - 1 ) );
  }
 }

// heapsort() on O, R, D, I, N, A, M, E, N, T, O step by step: after each
// step the prefix is a max-heap and the suffix holds the largest items in
// order; the equal items are no difficulty, heapsort is not stable but
// equal letters are indistinguishable; no letter, one, all equal, two
ADS_TEST( "ex10.2" )
{
 const std::string A = "ORDINAMENTO";
 std::vector< char > V( A.begin() , A.end() );
 heapBuild( V );
 CHECK( std::string( V.begin() , V.end() ) == "TRMNOADEINO" );
 for( std::size_t i = V.size() ; i-- > 1 ; ) {
  std::swap( V[ 0 ] , V[ i ] );
  maxHeapRestore( V , 0 , i );
  CHECK( isMaxHeap( V , i ) );
  CHECK( std::is_sorted( V.begin() + i , V.end() ) );
  }
 CHECK( std::string( V.begin() , V.end() ) == "ADEIMNNOORT" );
 // no letter, one, all equal, two in either order
 for( const std::string s : { "" , "A" , "OOOO" , "AB" , "BA" } ) {
  std::vector< char > W( s.begin() , s.end() );
  heapsort( W );
  std::string t = s;
  std::sort( t.begin() , t.end() );
  CHECK( std::string( W.begin() , W.end() ) == t );
  }
 }

// merging k sorted vectors of m items: one after the other costs
// 2 m + 3 m + ... + k m = O( m k^2 ) = O( n k ) moves; a heap of the k
// heads gives O( n log k ); no vector, one, empty ones, lengths that
// differ, equal and extreme items
ADS_TEST( "ex10.3" )
{
 std::mt19937 g( 3 );
 const int k = 20 , m = 50;
 std::vector< Ints > V( k , Ints( m ) );
 for( auto & v : V ) {
  for( auto & x : v )
   x = g() % 10000;
  std::sort( v.begin() , v.end() );
  }
 Ints all;
 for( const auto & v : V )
  all.insert( all.end() , v.begin() , v.end() );
 std::sort( all.begin() , all.end() );

 Ints C = V[ 0 ];
 long moves = 0;
 for( int i = 1 ; i < k ; ++i ) {
  Ints D( C.size() + m );
  std::merge( C.begin() , C.end() , V[ i ].begin() , V[ i ].end() ,
              D.begin() );
  moves += long( D.size() );
  C = std::move( D );
  }
 CHECK( C == all && moves == long( m ) * ( k * ( k + 1 ) / 2 - 1 ) );

 HeapPriorityQueue< std::pair< int , int > > Q( k );   // ( vector , place )
 for( int i = 0 ; i < k ; ++i )
  Q.insert( { i , 0 } , V[ i ][ 0 ] );
 Ints out;
 while( ! Q.empty() ) {
  const auto [ i , j ] = Q.deleteMin();
  out.push_back( V[ i ][ j ] );
  if( j + 1 < m )
   Q.insert( { i , j + 1 } , V[ i ][ j + 1 ] );
  }
 CHECK( out == all );

 // the degenerate inputs of the merge by a heap: no vector, one, empty
 // vectors among the others, lengths that differ, equal items across the
 // vectors, the extremes of int
 auto kmerge = []( const std::vector< Ints > & W ) {
  HeapPriorityQueue< std::pair< int , int > > H( std::max( int( W.size() ) ,
                                                           1 ) );
  for( int i = 0 ; i < int( W.size() ) ; ++i )
   if( ! W[ i ].empty() )
    H.insert( { i , 0 } , W[ i ][ 0 ] );
  Ints o;
  while( ! H.empty() ) {
   const auto [ i , j ] = H.deleteMin();
   o.push_back( W[ i ][ j ] );
   if( j + 1 < int( W[ i ].size() ) )
    H.insert( { i , j + 1 } , W[ i ][ j + 1 ] );
   }
  return( o );
  };
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 CHECK( kmerge( {} ).empty() );
 CHECK( kmerge( { Ints() } ).empty() );
 CHECK( kmerge( { Ints( { 1 , 2 , 3 } ) } ) == Ints( { 1 , 2 , 3 } ) );
 CHECK( kmerge( { Ints() , Ints( { 2 , 2 } ) , Ints() , Ints( { 1 , 2 , 9 } ) ,
                  Ints( { lo , hi } ) } ) ==
        Ints( { lo , 1 , 2 , 2 , 2 , 9 , hi } ) );
 }

// maxHeapRestore() iterative, O( 1 ) extra space, as the recursive one,
// also on all equal items, on prefixes, at the leaves, on 1 and 2 items
ADS_TEST( "ex10.4" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Ints A( 1 + g() % 100 );
  for( auto & a : A )
   a = g() % 50;
  Ints B = A;
  for( std::size_t i = A.size() / 2 ; i-- > 0 ; ) {
   maxHeapRestore( A , i , A.size() );
   restore_rec( B , i , B.size() );
   }
  CHECK( A == B );
  }
 // all equal items (nothing moves), a restore at the root of a prefix
 // (the rest untouched), at a leaf (nothing moves), on 1 and 2 items
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t n = 1 + g() % 40 , dim = 1 + g() % n , i = g() % dim;
  Ints A( n );
  for( auto & a : A )
   a = t % 2 ? 3 : int( g() % 10 );
  Ints B = A;
  maxHeapRestore( A , i , dim );
  restore_rec( B , i , dim );
  CHECK( A == B );
  }
 Ints S = { 1 , 2 };
 maxHeapRestore( S , 0 , 1 );
 CHECK( S == Ints( { 1 , 2 } ) );
 maxHeapRestore( S , 0 , 2 );
 CHECK( S == Ints( { 2 , 1 } ) );
 }

// the heap counts its places from 0: l( i ) = 2 i + 1, r( i ) = 2 i + 2,
// p( i ) = ( i - 1 ) / 2 (the text of the exercise says that the chapter
// counts them from 1, which it does not)
ADS_TEST( "ex10.5" )
{
 using Q = HeapPriorityQueue< int >;
 for( int i = 0 ; i < 100 ; ++i ) {
  CHECK( Q::child( i , 0 ) == 2 * i + 1 && Q::child( i , 1 ) == 2 * i + 2 );
  CHECK( Q::parent( Q::child( i , 0 ) ) == i );
  CHECK( Q::parent( Q::child( i , 1 ) ) == i );
  }
 }

// the d-ary heap: children d i + 1 , ... , d i + d, parent ( i - 1 ) / d,
// height log_d n; the degenerate cases, a full queue with a complete last
// level emptied in order, the children of the root
ADS_TEST( "ex10.6" )
{
 against_ref< HeapPriorityQueue< int , int , 3 > >(
  [] { return( HeapPriorityQueue< int , int , 3 >( 1000 ) ); } , 6 );
 against_ref< HeapPriorityQueue< int , int , 5 > >(
  [] { return( HeapPriorityQueue< int , int , 5 >( 1000 ) ); } , 7 );
 using Q = HeapPriorityQueue< int , int , 4 >;
 for( int i = 0 ; i < 100 ; ++i )
  for( int j = 0 ; j < 4 ; ++j )
   CHECK( Q::child( i , j ) == 4 * i + j + 1 &&
          Q::parent( Q::child( i , j ) ) == i );
 // a 4-ary heap of 1365 = 1 + 4 + 16 + 64 + 256 + 1024 items has height 5
 Q q( 1365 );
 for( int x = 0 ; x < 1365 ; ++x )
  q.insert( x , x );
 int h = 0;
 for( int i = 1364 ; i > 0 ; i = Q::parent( i ) )
  ++h;
 CHECK( h == 5 );
 // the degenerate cases for d = 3 and d = 4, a full queue whose last level
 // is complete, all the children of the root (places 1 to d) under it
 degenerate< HeapPriorityQueue< int , int , 3 > >(
  [] { return( HeapPriorityQueue< int , int , 3 >( 100 ) ); } );
 degenerate< Q >( [] { return( Q( 100 ) ); } );
 CHECK_THROWS( q.insert( 0 , 0 ) , contract_violation );
 for( int x = 0 ; x < 1365 ; ++x )
  CHECK( q.deleteMin() == x );
 CHECK( Q::parent( 1 ) == 0 && Q::parent( 4 ) == 0 && Q::parent( 5 ) == 1 );
 }

// without the compression the rank of a root is the height of its tree,
// with it an upper bound; a single element, the tree of 64 elements of
// rank 6, flattened by the compression
ADS_TEST( "ex10.7" )
{
 std::mt19937 g( 8 );
 for( bool compress : { false , true } )
  for( int t = 0 ; t < 10 ; ++t ) {
   const int n = 200;
   MFSet M( n , MFSet::Union::RANK , compress );
   for( int k = 0 ; k < 300 ; ++k ) {
    M.merge( g() % n , g() % n );
    if( k % 3 == 0 )
     M.find( g() % n );
    }
   for( int r = 0 ; r < n ; ++r )
    if( M.parent( r ) == r ) {
     if( compress )
      CHECK( height( M , r ) <= M.rank( r ) );
     else
      CHECK( height( M , r ) == M.rank( r ) );
     }
   }
 // the extreme trees: a single element (rank and height 0), 64 elements
 // merged in rounds of equal trees (rank and height log2 64 = 6, the bound
 // of the Teorema 10.1 reached), and with the compression after a find of
 // each element (height 1, rank still 6)
 MFSet O( 1 , MFSet::Union::RANK , false );
 CHECK( O.rank( 0 ) == 0 && height( O , 0 ) == 0 );
 for( bool compress : { false , true } ) {
  MFSet M( 64 , MFSet::Union::RANK , compress );
  for( int s = 1 ; s < 64 ; s *= 2 )
   for( int x = 0 ; x < 64 ; x += 2 * s )
    M.merge( x , x + s );
  const int r = M.find( 0 );
  CHECK( M.rank( r ) == 6 && height( M , r ) <= 6 );
  CHECK( compress || height( M , r ) == 6 );
  if( compress ) {
   for( int x = 0 ; x < 64 ; ++x )
    M.find( x );
   CHECK( M.rank( r ) == 6 && height( M , r ) == 1 );
   }
  }
 }

// find() iterative in two passes, with no stack, as the recursive one of
// the book: the same roots and the same parents; a deep chain flattened by
// one find, a root and a single element
ADS_TEST( "ex10.8" )
{
 std::mt19937 g( 9 );
 const int n = 500;
 MFSet M( n , MFSet::Union::RANK , true );
 Ints p( n ) , rank( n , 0 );
 for( int i = 0 ; i < n ; ++i )
  p[ i ] = i;
 auto find = [ & ]( auto & self , int x ) -> int {
  if( p[ x ] != x )
   p[ x ] = self( self , p[ x ] );
  return( p[ x ] );
  };
 for( int k = 0 ; k < 2000 ; ++k ) {
  const int x = g() % n , y = g() % n;
  if( k % 2 ) {
   CHECK( M.find( x ) == find( find , x ) );
   continue;
   }
  M.merge( x , y );
  const int rx = find( find , x ) , ry = find( find , y );
  if( rx != ry ) {
   if( rank[ rx ] > rank[ ry ] ) p[ ry ] = rx;
   else if( rank[ ry ] > rank[ rx ] ) p[ rx ] = ry;
   else { p[ rx ] = ry; ++rank[ ry ]; }
   }
  }
 for( int x = 0 ; x < n ; ++x )
  CHECK( M.parent( x ) == p[ x ] );
 // a chain of 2000 without the union heuristic: a find of its deepest
 // element makes every element a child of the root, a second one climbs
 // one step; a find of a root or of a single element changes nothing
 const int m = 2000;
 MFSet C( m , MFSet::Union::NONE , true );
 for( int x = m - 2 ; x >= 0 ; --x )
  C.merge( x , x + 1 );
 CHECK( C.parent( m - 1 ) == m - 2 && C.find( 0 ) == 0 );
 CHECK( C.find( m - 1 ) == 0 );
 bool flat = true;
 for( int x = 0 ; x < m ; ++x )
  flat = flat && ( C.parent( x ) == 0 );
 const long s = C.steps();
 CHECK( flat && C.find( m - 1 ) == 0 && C.steps() == s + 1 );
 MFSet O( 1 );
 CHECK( O.find( 0 ) == 0 && O.parent( 0 ) == 0 && O.steps() == 0 );
 }

// the components of a weighted unoriented graph and the edge of least
// weight of each one: an MFSET merges the ends of each edge, and the
// least edge of a component is the least of those whose ends are in it
// (the solution of the book says a heap, but an MFSET is what is needed);
// no edge, edges of equal weight, a single component
ADS_TEST( "ex10.9" )
{
 std::mt19937 g( 10 );
 const int n = 50;
 ListGraph< int > G( n , false );
 std::vector< std::tuple< int , int , int > > E;
 for( int k = 0 ; k < 45 ; ++k ) {
  const int u = g() % n , v = g() % n , w = g() % 1000;
  if( ( u != v ) && ! G.hasEdge( u , v ) ) {
   G.insertEdge( u , v , w );
   E.emplace_back( w , std::min( u , v ) , std::max( u , v ) );
   }
  }
 MFSet M( n );
 for( const auto & [ w , u , v ] : E )
  M.merge( u , v );
 std::map< int , std::tuple< int , int , int > > least;   // root -> edge
 for( const auto & e : E ) {
  const int r = M.find( std::get< 1 >( e ) );
  if( ! least.count( r ) || e < least[ r ] )
   least[ r ] = e;
  }
 const auto id = cc( G );
 CHECK( int( std::set< int >( id.begin() , id.end() ).size() ) == M.count() );
 for( int c : std::set< int >( id.begin() , id.end() ) ) {
  std::optional< std::tuple< int , int , int > > ref;
  for( const auto & e : E )
   if( id[ std::get< 1 >( e ) ] == c && ( ! ref || e < *ref ) )
    ref = e;
  int x = 0;
  while( id[ x ] != c )
   ++x;
  const int r = M.find( x );
  CHECK( ref.has_value() == ( least.count( r ) == 1 ) );
  if( ref )
   CHECK( least[ r ] == *ref );
  }
 // no edge: a component per node, none with a least edge; edges of equal
 // weight: the least is the one of smaller ends, in the order of the
 // triples; one component holding all the nodes
 MFSet Z( 4 );
 CHECK( Z.count() == 4 && components( ListGraph< int >( 4 , false ) ).count()
                          == 4 );
 const std::vector< std::tuple< int , int , int > > T = {
  { 5 , 2 , 3 } , { 5 , 0 , 3 } , { 5 , 1 , 2 } };
 MFSet A( 4 );
 std::optional< std::tuple< int , int , int > > lt;
 for( const auto & e : T ) {
  A.merge( std::get< 1 >( e ) , std::get< 2 >( e ) );
  if( ! lt || e < *lt )
   lt = e;
  }
 CHECK( A.count() == 1 && lt == std::tuple( 5 , 0 , 3 ) );
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
/*---------------------------- End File ch10.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
