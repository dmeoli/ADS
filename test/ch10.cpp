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
  CHECK( q.isEmpty() == in.empty() );
  }
 while( ! in.empty() ) {
  const int x = q.deleteMin();
  CHECK( in.count( x ) );
  int best = in.at( x ).second;
  for( const auto & [ y , hp ] : in )
   CHECK( best <= hp.second );
  in.erase( x );
  }
 CHECK( q.isEmpty() );
 }

/// a comparison that counts
struct Counting {
 long * c;
 bool operator()( const auto & a , const auto & b ) const {
  ++*c;
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
 }

ADS_TEST( "spec.ListPriorityQueue" )
{
 against_ref< ListPriorityQueue< int , int , true > >(
  [] { return( ListPriorityQueue< int , int , true >() ); } , 2 );
 against_ref< ListPriorityQueue< int , int , false > >(
  [] { return( ListPriorityQueue< int , int , false >() ); } , 3 );
 ListPriorityQueue< int > q;
 CHECK_THROWS( (void) q.min() , std::logic_error );
 }

ADS_TEST( "spec.TreePriorityQueue" )
{
 against_ref< TreePriorityQueue< int > >(
  [] { return( TreePriorityQueue< int >() ); } , 4 );
 }

/*--------------------------------------------------------------------------*/

// Figure 10.1 and 10.2: the heap of C = { 5 , 10 , 8 , 11 , 13 , 12 , 9 ,
// 18 , 3 , 6 }, built by inserting its vector in order (each item is not
// smaller than its parent, so nothing moves); Figura 10.3, the deletion of
// the minimum; Figura 10.4, the insertion of 4
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
 }

// heapBuild() in O( n ), at most 2 n comparisons, and heapsort(), and the
// sort() of the example with a priority queue
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
 }

/*--------------------------------------------------------------------------*/

// the example of the MFSET of 4 elements, and the forest of Figura 10.5 by
// merge( 0 , 1 ), merge( 0 , 2 ), merge( 4 , 5 ), merge( 5 , 2 ),
// merge( 3 , 6 ), merge( 2 , 6 ) without the rank (the figure counts the
// elements from 1)
ADS_TEST( "sec10.2.2.forest" )
{
 MFSet S( 4 );
 CHECK( S.find( 0 ) != S.find( 2 ) && S.merge( 0 , 2 ) );
 CHECK( S.find( 3 ) != S.find( 1 ) && S.merge( 3 , 1 ) );
 CHECK( S.find( 2 ) != S.find( 3 ) && S.merge( 2 , 3 ) );
 CHECK( S.count() == 1 && ! S.merge( 0 , 1 ) );

 MFSet F( 7 , false , false );
 for( auto [ x , y ] : std::vector< std::pair< int , int > >(
       { { 0 , 1 } , { 0 , 2 } , { 4 , 5 } , { 5 , 2 } , { 3 , 6 } ,
         { 2 , 6 } } ) )
  F.merge( x , y );
 Ints p( 7 );
 for( int x = 0 ; x < 7 ; ++x )
  p[ x ] = F.parent( x );
 CHECK( p == Ints( { 4 , 0 , 0 , 4 , 4 , 4 , 3 } ) );
 CHECK_THROWS( F.find( 7 ) , std::out_of_range );
 }

// without the rank, merge( 4 , 5 ), merge( 3 , 4 ), ..., merge( 0 , 1 )
// make a chain of height n - 1; with it, the height is 1; the Teorema
// 10.1, a tree of root r has at least 2^rank[ r ] nodes
ADS_TEST( "sec10.2.3.rank" )
{
 for( bool rank : { false , true } ) {
  MFSet M( 6 , rank , false );
  for( int x = 4 ; x >= 0 ; --x )
   M.merge( x , x + 1 );
  CHECK( height( M , M.find( 0 ) ) == ( rank ? 1 : 5 ) );
  }
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 300;
  MFSet M( n , true , false );
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
 MFSet M( 8 , false , true );
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
  MFSet C( n , false , compress );
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
// then with new edges
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
 }

// the MFSET on lists against the one on a forest; an element is relabeled
// at most log2 n times
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
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the heap does not order its keys: its invisit is not sorted, and a sort
// by comparisons needs log2( n! ) of them, which heapsort() exceeds, while
// heapBuild() takes O( n ): the order cannot be read from a heap in O( n )
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
 }

// heapsort() on O, R, D, I, N, A, M, E, N, T, O step by step: after each
// step the prefix is a max-heap and the suffix holds the largest items in
// order; the equal items are no difficulty, heapsort is not stable but
// equal letters are indistinguishable
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
 }

// merging k sorted vectors of m items: one after the other costs
// 2 m + 3 m + ... + k m = O( m k^2 ) = O( n k ) moves; a heap of the k
// heads gives O( n log k )
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
 while( ! Q.isEmpty() ) {
  const auto [ i , j ] = Q.deleteMin();
  out.push_back( V[ i ][ j ] );
  if( j + 1 < m )
   Q.insert( { i , j + 1 } , V[ i ][ j + 1 ] );
  }
 CHECK( out == all );
 }

// maxHeapRestore() iterative, O( 1 ) extra space, as the recursive one
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
// height log_d n
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
 }

// without the compression the rank of a root is the height of its tree,
// with it an upper bound
ADS_TEST( "ex10.7" )
{
 std::mt19937 g( 8 );
 for( bool compress : { false , true } )
  for( int t = 0 ; t < 10 ; ++t ) {
   const int n = 200;
   MFSet M( n , true , compress );
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
 }

// find() iterative in two passes, with no stack, as the recursive one of
// the book: the same roots and the same parents
ADS_TEST( "ex10.8" )
{
 std::mt19937 g( 9 );
 const int n = 500;
 MFSet M( n , true , true );
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
 }

// the components of a weighted unoriented graph and the edge of least
// weight of each one: an MFSET merges the ends of each edge, and the
// least edge of a component is the least of those whose ends are in it
// (the solution of the book says a heap, but an MFSET is what is needed)
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
