/*--------------------------------------------------------------------------*/
/*---------------------------- File graphs.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the graph algorithms of the book on the structures of
 * the standard library, of Boost and of this library: each algorithm is
 * the same code [see Visit.h, ShortestPaths.h, MinimumSpanningTree.h], a
 * template on the representation of the graph and on the structures it
 * uses, run three times, once on each family of structures:
 *
 * - std: the rows in a std::vector of std::vectors, std::queue, a
 *   std::set of the pairs ( priority , node ) as the priority queue with
 *   decrease (erase and insert), and no union-find, which the standard
 *   library does not have;
 * - Boost: the compressed_sparse_row_graph of the Boost Graph Library,
 *   boost::queue, the mutable heaps of Boost.Heap (d-ary of arity 2 and 4,
 *   Fibonacci, pairing) and disjoint_sets (the union by rank with the
 *   compression of the paths);
 * - ADS: StaticGraph (the compressed rows with each arc next to its
 *   weight), ArrayQueue, the priority queues of the book with handles
 *   (HeapPriorityQueue of arity 2 and 4, FibonacciHeapPriorityQueue,
 *   PairingHeap), IndexedDaryHeap, and MFSet, UnionFind and DisjointSets.
 *
 * The three graphs hold the same arcs in the same order (the rows sorted
 * by head, the graphs simple), and the priorities are the pairs
 * ( distance , node ), so that every priority queue extracts the same
 * nodes in the same order: the counts of each run (the nodes taken out of
 * the structure, the arcs examined, the decreases of a priority, the
 * merges) are then the same on the three families, and the benchmark
 * stops if they or the results differ. The benchmarks and their cases:
 *
 * - bfs, distance(), the visit in breadth of the book, on random graphs
 *   G( n , 4 n ), square grids, road networks and power-law graphs
 *   (R-MAT, m = 8 n), unoriented;
 * - dijkstra, shortestPaths() on a priority queue (Johnson's algorithm in
 *   the book), on the same graphs oriented and on G( n , 16 n ), costs in
 *   [ 1 , 10^5 ];
 * - bfm, shortestPaths() on a queue (Bellman-Ford-Moore), on the grids
 *   and the road networks;
 * - prim, prim() on a priority queue, on the unoriented graphs;
 * - kruskal, kruskal() on an MFSET, on the unoriented graphs;
 * - scc, scc(), the strongly connected components of the book, on the
 *   oriented graphs of n / 10 nodes;
 * - union-find, n random merges and n finds on each MFSET.
 *
 * The road networks are those of the 9th DIMACS Implementation Challenge
 * (USA-road-d.NY.gr for n up to 300000, USA-road-d.FLA.gr above) when the
 * directory ADS_DIMACS holds them, with their own n; otherwise points in
 * the unit square joined to their 3 nearest neighbours, costs their
 * distances. Each cell is the median of r runs (of one run, if it takes
 * more than 5 s), in nanoseconds per arc (per operation for union-find),
 * the building of the graphs excluded and that of the structures of the
 * run included; the counts are rows of the benchmark "<bench>:count". The
 * contracts are off (ADS_CONTRACTS=0).
 *
 *   [ ADS_GRAPHS=b1,b2,... ] graphs [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * runs the benchmarks b1, b2, ... only, if ADS_GRAPHS is set.
 *
 * \see D. Chakrabarti, Y. Zhan, C. Faloutsos, R-MAT: a recursive model for
 *      graph mining, Proceedings of the SIAM International Conference on
 *      Data Mining, 442-446, 2004, doi:10.1137/1.9781611972740.43
 * \see C. Demetrescu, A. V. Goldberg, D. S. Johnson (eds.), The Shortest
 *      Path Problem: Ninth DIMACS Implementation Challenge, DIMACS Series
 *      74, AMS, 2009, doi:10.1090/dimacs/074
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/queue/ArrayQueue.h"

#if __has_include( <boost/graph/compressed_sparse_row_graph.hpp> )
 #include <boost/property_map/property_map.hpp>
 #include <boost/graph/compressed_sparse_row_graph.hpp>
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/fibonacci_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #include <boost/pending/disjoint_sets.hpp>
 #include <boost/pending/queue.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <queue>
#include <set>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace ADS::bench;

namespace {

/*--------------------------------------------------------------------------*/
/*--------------------------------- INPUTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

using Edge = std::tuple< int , int , int >;
using LEdge = std::tuple< int , int , long long >;

/// a graph: n nodes, its edges (an unoriented one given once), oriented or
/// not
struct Input {
 int n = 0;
 std::vector< Edge > E;
 bool directed = true;
 std::size_t arcs( void ) const {
  return( ( directed ? 1 : 2 ) * E.size() );
  }
 };

constexpr int MAXW = 100000;   ///< the costs are in [ 1 , MAXW ]

/// m edges of G( n , m ), no loop
Input gnm( int n , std::size_t m , bool directed , std::mt19937_64 & g )
{
 Input I{ n , {} , directed };
 I.E.reserve( m );
 while( I.E.size() < m ) {
  const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
  if( u != v )
   I.E.emplace_back( u , v , 1 + int( g() % MAXW ) );
  }
 return( I );
 }

/// the square grid of about n nodes, its edges both ways if oriented
Input grid( int n , bool directed , std::mt19937_64 & g )
{
 const int s = std::max( 2 , int( std::sqrt( double( n ) ) ) );
 Input I{ s * s , {} , directed };
 auto add = [ & ]( int u , int v ) {
  I.E.emplace_back( u , v , 1 + int( g() % MAXW ) );
  if( directed )
   I.E.emplace_back( v , u , 1 + int( g() % MAXW ) );
  };
 for( int r = 0 ; r < s ; ++r )
  for( int c = 0 ; c < s ; ++c ) {
   if( c + 1 < s ) add( r * s + c , r * s + c + 1 );
   if( r + 1 < s ) add( r * s + c , ( r + 1 ) * s + c );
   }
 return( I );
 }

/// R-MAT of Chakrabarti, Zhan and Faloutsos: m edges, each falling in a
/// quadrant of the matrix with probabilities 0.57, 0.19, 0.19, 0.05 at
/// each of the levels, the nodes beyond n drawn again, no loop
Input rmat( int n , std::size_t m , bool directed , std::mt19937_64 & g )
{
 int levels = 0;
 while( ( 1 << levels ) < n ) ++levels;
 Input I{ n , {} , directed };
 I.E.reserve( m );
 std::uniform_real_distribution< double > U( 0 , 1 );
 while( I.E.size() < m ) {
  int u = 0 , v = 0;
  for( int l = 0 ; l < levels ; ++l ) {
   const double x = U( g );
   u = 2 * u + ( x >= 0.76 );
   v = 2 * v + ( ( x >= 0.57 && x < 0.76 ) || x >= 0.95 );
   }
  if( ( u < n ) && ( v < n ) && ( u != v ) )
   I.E.emplace_back( u , v , 1 + int( g() % MAXW ) );
  }
 // the node names of R-MAT follow the degrees: shuffled, as in the
 // generators of Graph500
 std::vector< int > p( n );
 for( int i = 0 ; i < n ; ++i ) p[ i ] = i;
 std::shuffle( p.begin() , p.end() , g );
 for( auto & [ u , v , w ] : I.E ) {
  u = p[ u ];
  v = p[ v ];
  }
 return( I );
 }

/// a road network of the DIMACS challenge, its arcs as they are if
/// oriented, those with u < v if not; empty if not found
Input dimacs( const std::string & path , bool directed )
{
 Input I{ 0 , {} , directed };
 std::ifstream in( path );
 std::string line;
 while( std::getline( in , line ) ) {
  if( line.empty() ) continue;
  if( line[ 0 ] == 'p' ) {
   std::istringstream s( line.substr( 4 ) );
   std::size_t m;
   s >> I.n >> m;
   I.E.reserve( m );
   }
  else if( line[ 0 ] == 'a' ) {
   int u , v , w;
   std::sscanf( line.c_str() + 2 , "%d %d %d" , &u , &v , &w );
   if( ( u != v ) && ( directed || ( u < v ) ) )
    I.E.emplace_back( u - 1 , v - 1 , w );
   }
  }
 return( I );
 }

/// points in the unit square, each joined to its 3 nearest neighbours by
/// a bucket grid, costs the distances times 10^5
Input geometric( int n , bool directed , std::mt19937_64 & g )
{
 std::uniform_real_distribution< double > U( 0 , 1 );
 std::vector< double > x( n ) , y( n );
 for( int i = 0 ; i < n ; ++i ) { x[ i ] = U( g ); y[ i ] = U( g ); }
 const int s = std::max( 1 , int( std::sqrt( n / 2.0 ) ) );
 std::vector< std::vector< int > > B( std::size_t( s ) * s );
 auto cell = [ & ]( double t ) { return( std::min( s - 1 , int( t * s ) ) ); };
 for( int i = 0 ; i < n ; ++i )
  B[ cell( y[ i ] ) * s + cell( x[ i ] ) ].push_back( i );
 Input I{ n , {} , directed };
 std::set< std::pair< int , int > > seen;
 for( int i = 0 ; i < n ; ++i ) {
  std::vector< std::pair< double , int > > near;
  const int cx = cell( x[ i ] ) , cy = cell( y[ i ] );
  for( int dy = -1 ; dy <= 1 ; ++dy )
   for( int dx = -1 ; dx <= 1 ; ++dx ) {
    const int a = cx + dx , b = cy + dy;
    if( a < 0 || b < 0 || a >= s || b >= s ) continue;
    for( int j : B[ b * s + a ] )
     if( j != i )
      near.emplace_back( std::hypot( x[ i ] - x[ j ] , y[ i ] - y[ j ] ) ,
                         j );
    }
  std::sort( near.begin() , near.end() );
  for( std::size_t k = 0 ; k < std::min< std::size_t >( 3 , near.size() ) ;
       ++k ) {
   const int j = near[ k ].second;
   if( ! seen.emplace( std::min( i , j ) , std::max( i , j ) ).second )
    continue;
   const int w = 1 + int( near[ k ].first * 1e5 );
   I.E.emplace_back( i , j , w );
   if( directed )
    I.E.emplace_back( j , i , w );
   }
  }
 return( I );
 }

/// the road network for about n nodes
Input road( int n , bool directed , std::mt19937_64 & g )
{
 if( const char * dir = std::getenv( "ADS_DIMACS" ) ) {
  Input I = dimacs( std::string( dir ) + ( n <= 300000 ?
                    "/USA-road-d.NY.gr" : "/USA-road-d.FLA.gr" ) , directed );
  if( I.n > 0 )
   return( I );
  }
 return( geometric( n , directed , g ) );
 }

/// the input of a case
Input make( const std::string & c , int n , bool directed , unsigned seed )
{
 std::mt19937_64 g( seed + unsigned( n ) );
 if( c == "gnm4" ) return( gnm( n , 4 * std::size_t( n ) , directed , g ) );
 if( c == "gnm16" )
  return( gnm( n , 16 * std::size_t( n ) , directed , g ) );
 if( c == "grid" ) return( grid( n , directed , g ) );
 if( c == "road" ) return( road( n , directed , g ) );
 return( rmat( n , 8 * std::size_t( n ) , directed , g ) );
 }

/// the source of the visits: the tail of the first edge, which is in the
/// giant component of the random graphs
int source( const Input & I )
{
 return( I.E.empty() ? 0 : std::get< 0 >( I.E.front() ) );
 }

/// the arcs of I, both ways if it is unoriented, without loops and
/// parallel arcs (the lightest kept), sorted by tail and then by head: the
/// order of the rows of StaticGraph, which the other graphs get too
std::vector< Edge > arcs( const Input & I )
{
 std::vector< Edge > A;
 A.reserve( I.arcs() );
 for( const auto & [ u , v , w ] : I.E )
  if( u != v ) {
   A.emplace_back( u , v , w );
   if( ! I.directed )
    A.emplace_back( v , u , w );
   }
 std::sort( A.begin() , A.end() );
 A.erase( std::unique( A.begin() , A.end() , []( const Edge & a ,
          const Edge & b ) { return( std::get< 0 >( a ) == std::get< 0 >( b )
                               && std::get< 1 >( a ) == std::get< 1 >( b ) );
          } ) , A.end() );
 return( A );
 }

/// the edges of I for StaticGraph: the arcs, or those with u < v of an
/// unoriented graph (StaticGraph puts them both ways)
std::vector< Edge > edgesOf( const Input & I , const std::vector< Edge > & A )
{
 if( I.directed )
  return( A );
 std::vector< Edge > E;
 for( const auto & e : A )
  if( std::get< 0 >( e ) < std::get< 1 >( e ) )
   E.push_back( e );
 return( E );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE STRUCTURES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the priority of a node: ( distance , node )
using Priority = std::pair< int , int >;

// std: the rows, a queue, a priority queue on a set

/// a GRAPH on a std::vector of rows of std::pairs ( head , weight ), or of
/// heads only if not Weighted
template< bool Weighted = true >
struct StdGraph {
 using Weight = int;
 using Arc = std::conditional_t< Weighted , std::pair< int , int > , int >;
 std::vector< std::vector< Arc > > out;
 bool dir = true;
 StdGraph( int n , const std::vector< Edge > & A , bool d )
  : out( std::size_t( n ) ) , dir( d ) {
  for( const auto & [ u , v , w ] : A )
   if constexpr( Weighted ) out[ u ].emplace_back( v , w );
   else out[ u ].push_back( v );
  }
 static int head( const Arc & a ) {
  if constexpr( Weighted ) return( a.first ); else return( a );
  }
 int n( void ) const { return( int( out.size() ) ); }
 int bound( void ) const { return( n() ); }
 bool directed( void ) const { return( dir ); }
 bool hasNode( int u ) const { return( ( u >= 0 ) && ( u < n() ) ); }
 std::vector< int > V( void ) const {
  std::vector< int > R( out.size() );
  for( int u = 0 ; u < n() ; ++u ) R[ u ] = u;
  return( R );
  }
 std::vector< int > adj( int u ) const {
  std::vector< int > R;
  R.reserve( out[ u ].size() );
  for( const auto & a : out[ u ] ) R.push_back( head( a ) );
  return( R );
  }
 template< class F >
 void forEachAdj( int u , F && f ) const {
  for( const auto & a : out[ u ] ) f( head( a ) );
  }
 template< class F >
 void forEachEdge( int u , F && f ) const requires( Weighted ) {
  for( const auto & a : out[ u ] ) f( a.first , a.second );
  }
 };

/// std::queue with the names of the book
struct StdQueue {
 std::queue< int > q;
 bool empty( void ) const { return( q.empty() ); }
 void enqueue( int v ) { q.push( v ); }
 int dequeue( void ) { const int v = q.front(); q.pop(); return( v ); }
 };

/// a priority queue of the nodes on a std::set of their priorities, the
/// decrease an erase and an insert, the node its own handle
struct StdSetQueue {
 using Item = int;
 using Priority = std::pair< int , int >;
 using Handle = int;
 std::set< Priority > s;
 std::vector< Priority > key;
 explicit StdSetQueue( int n ) : key( std::size_t( n ) ) {}
 bool empty( void ) const { return( s.empty() ); }
 int min( void ) const { return( s.begin()->second ); }
 int deleteMin( void ) {
  const int v = s.begin()->second;
  s.erase( s.begin() );
  return( v );
  }
 Handle insert( int v , const Priority & p ) {
  key[ v ] = p;
  s.insert( p );
  return( v );
  }
 void decrease( Handle v , const Priority & p ) {
  s.erase( key[ v ] );
  key[ v ] = p;
  s.insert( p );
  }
 };

#ifdef ADS_HAS_BOOST
// Boost: the compressed rows of the Boost Graph Library, its queue, the
// mutable heaps of Boost.Heap, its disjoint sets

/// the compressed rows of Boost with nodes and offsets of 32 bits, the
/// weights in the bundle of the arcs or none
template< bool Weighted >
using BCsr = boost::compressed_sparse_row_graph< boost::directedS ,
             boost::no_property , std::conditional_t< Weighted , int ,
             boost::no_property > , boost::no_property , unsigned ,
             unsigned >;

/// a GRAPH on a compressed_sparse_row_graph, weighted or not
template< bool Weighted = true >
struct BoostGraph {
 using Weight = int;
 BCsr< Weighted > g;
 bool dir = true;
 BoostGraph( int n , const std::vector< Edge > & A , bool d ) : dir( d ) {
  std::vector< std::pair< unsigned , unsigned > > P;
  std::vector< int > W;
  for( const auto & [ u , v , w ] : A ) {
   P.emplace_back( unsigned( u ) , unsigned( v ) );
   W.push_back( w );
   }
  if constexpr( Weighted )
   g = BCsr< Weighted >( boost::edges_are_sorted , P.begin() , P.end() ,
                         W.begin() , unsigned( n ) );
  else
   g = BCsr< Weighted >( boost::edges_are_sorted , P.begin() , P.end() ,
                         unsigned( n ) );
  }
 int n( void ) const { return( int( boost::num_vertices( g ) ) ); }
 int bound( void ) const { return( n() ); }
 bool directed( void ) const { return( dir ); }
 bool hasNode( int u ) const { return( ( u >= 0 ) && ( u < n() ) ); }
 std::vector< int > V( void ) const {
  std::vector< int > R( static_cast< std::size_t >( n() ) );
  for( int u = 0 ; u < n() ; ++u ) R[ u ] = u;
  return( R );
  }
 std::vector< int > adj( int u ) const {
  std::vector< int > R;
  for( auto [ a , b ] = boost::out_edges( unsigned( u ) , g ) ; a != b ;
       ++a )
   R.push_back( int( boost::target( *a , g ) ) );
  return( R );
  }
 template< class F >
 void forEachAdj( int u , F && f ) const {
  for( auto [ a , b ] = boost::out_edges( unsigned( u ) , g ) ; a != b ;
       ++a )
   f( int( boost::target( *a , g ) ) );
  }
 template< class F >
 void forEachEdge( int u , F && f ) const requires( Weighted ) {
  for( auto [ a , b ] = boost::out_edges( unsigned( u ) , g ) ; a != b ;
       ++a )
   f( int( boost::target( *a , g ) ) , g[ *a ] );
  }
 };

/// boost::queue with the names of the book
struct BoostQueue {
 boost::queue< int > q;
 bool empty( void ) const { return( q.empty() ); }
 void enqueue( int v ) { q.push( v ); }
 int dequeue( void ) { const int v = q.top(); q.pop(); return( v ); }
 };

/// a mutable heap of Boost.Heap as a priority queue of the nodes: the
/// least priority on top (the order std::greater), a decrease its
/// increase()
template< class H >
struct BoostHeapQueue {
 using Item = int;
 using Priority = std::pair< int , int >;
 using Handle = typename H::handle_type;
 H h;
 explicit BoostHeapQueue( int ) {}
 bool empty( void ) const { return( h.empty() ); }
 int min( void ) const { return( h.top().second ); }
 int deleteMin( void ) {
  const int v = h.top().second;
  h.pop();
  return( v );
  }
 Handle insert( int , const Priority & p ) { return( h.push( p ) ); }
 void decrease( Handle x , const Priority & p ) { h.increase( x , p ); }
 };

using Greater = boost::heap::compare< std::greater< Priority > >;
using BoostDary2 = BoostHeapQueue< boost::heap::d_ary_heap< Priority ,
                   boost::heap::arity< 2 > , boost::heap::mutable_< true > ,
                   Greater > >;
using BoostDary4 = BoostHeapQueue< boost::heap::d_ary_heap< Priority ,
                   boost::heap::arity< 4 > , boost::heap::mutable_< true > ,
                   Greater > >;
using BoostFibonacci = BoostHeapQueue< boost::heap::fibonacci_heap<
                       Priority , Greater > >;
using BoostPairing = BoostHeapQueue< boost::heap::pairing_heap< Priority ,
                     Greater > >;

/// disjoint_sets of Boost, the union by rank with the compression of the
/// paths, as an MFSET: merge() joins the roots and tells whether they were
/// apart
struct BoostSets {
 using Map = boost::iterator_property_map< int * ,
             boost::typed_identity_property_map< std::size_t > >;
 std::vector< int > rank , parent;
 boost::disjoint_sets< Map , Map > D;
 explicit BoostSets( int n ) : rank( std::size_t( n ) ) ,
                               parent( std::size_t( n ) ) ,
                               D( Map( rank.data() ) , Map( parent.data() ) ) {
  for( int i = 0 ; i < n ; ++i ) D.make_set( i );
  }
 bool merge( int a , int b ) {
  const int x = D.find_set( a ) , y = D.find_set( b );
  if( x == y )
   return( false );
  D.link( x , y );
  return( true );
  }
 bool same( int a , int b ) { return( D.find_set( a ) == D.find_set( b ) ); }
 };
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------- CHECKS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the value of the first structure of a cell, which the others match: the
/// result, or a count
struct Check {
 std::string what;
 long long value = 0;
 bool set = false;
 void start( const std::string & c ) { what = c; set = false; }
 void operator()( const std::string & who , long long v ) {
  if( ! set ) { value = v; set = true; return; }
  if( v != value ) {
   std::fprintf( stderr , "MISMATCH %s: %s gives %lld, not %lld\n" ,
                 what.c_str() , who.c_str() , v , value );
   std::exit( 1 );
   }
  }
 };

/// whether any of the structures whos is measured, all of them listed
/// under ADS_BENCH_LIST: the graph they share is built only then
bool measured( std::initializer_list< const char * > whos )
{
 bool m = false;
 for( const char * w : whos )
  m = ! skip( w ) || m;
 return( m );
 }

/// the sum of the finite distances and their count, as one number
long long digest( const std::vector< int > & d , int inf )
{
 long long s = 0;
 for( int x : d )
  if( x != inf ) s += x + 1;
 return( s );
 }

/// the counts of a run, by name
using Counts = std::vector< std::pair< const char * , long long > >;

/// the checks of the result and of each count of a cell
struct Cell {
 const char * bench;
 std::string c;
 std::size_t n , ops;
 int r;
 Check result;
 std::vector< Check > counts;

 Cell( const char * b , const std::string & cs , std::size_t nn ,
       std::size_t o , int rr ) : bench( b ) , c( cs ) , n( nn ) , ops( o ) ,
                                   r( rr ) {
  result.start( std::string( b ) + " " + cs );
  }

 /// the median of r runs of work(), which returns the result and fills
 /// the counts of its last run, in ns per op (one run if it takes more
 /// than 5 s); the result and the counts checked against the first run
 template< class W >
 void operator()( const std::string & who , W && work ) {
  if( skip( who ) )
   return;
  long long res = 0;
  Counts k;
  auto run = [ & ]( int ) { k.clear(); res = work( k ); };
  auto none = []() { return( 0 ); };
  double t = median( none , run , 1 , ops );
  if( ( r > 1 ) && ( t * double( ops ) < 5e9 ) )
   t = median( none , run , r , ops );
  result( who , res );
  if( counts.size() < k.size() )
   for( std::size_t i = counts.size() ; i < k.size() ; ++i ) {
    counts.emplace_back();
    counts.back().start( std::string( bench ) + " " + c + " " +
                         k[ i ].first );
    }
  for( std::size_t i = 0 ; i < k.size() ; ++i ) {
   counts[ i ]( who , k[ i ].second );
   std::printf( "%s:count\t%s\t%zu\t%s %s\t%lld\n" , bench , c.c_str() , n ,
                who.c_str() , k[ i ].first , k[ i ].second );
   }
  print( bench , c , n , who , t );
  }
 };

Counts counts( const ADS::FrontierCounts & k )
{
 return( Counts{ { "adds" , k.adds } , { "extracts" , k.extracts } ,
                 { "decreases" , k.decreases } , { "edges" , k.edges } } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BENCHMARKS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// distance() of the book with the queue Q on the graph g
template< class G , class Q >
long long bfsRun( const G & g , int src , Q S , Counts & k )
{
 ADS::VisitCounts c;
 const auto d = ADS::distance( g , src , S , &c ).first;
 k = { { "nodes" , c.nodes } , { "edges" , c.edges } };
 return( digest( d , ADS::INF ) );
 }

void bfs( const std::string & c , int n0 , int r , unsigned seed )
{
 const Input I = make( c , n0 , false , seed );
 const int n = I.n , src = source( I );
 const auto A = arcs( I );
 Cell C( "bfs" , c , std::size_t( n ) , A.size() , r );
 if( measured( { "std vector queue" , "std vector queue A/A" } ) ) {
  const StdGraph< false > G( n , A , false );
  for( const char * who : { "std vector queue" , "std vector queue A/A" } )
   C( who , [ & ]( Counts & k ) {
    return( bfsRun( G , src , StdQueue() , k ) ); } );
  }
#ifdef ADS_HAS_BOOST
 if( measured( { "Boost csr queue" } ) ) {
  const BoostGraph< false > G( n , A , false );
  C( "Boost csr queue" , [ & ]( Counts & k ) {
   return( bfsRun( G , src , BoostQueue() , k ) ); } );
  }
#endif
 if( ! measured( { "ADS StaticGraph ArrayQueue" } ) )
  return;
 const ADS::StaticGraph<> S( ADS::StaticGraph< int >( n , edgesOf( I , A ) ,
                                                      false ) );
 C( "ADS StaticGraph ArrayQueue" , [ & ]( Counts & k ) {
  return( bfsRun( S , src , ADS::ArrayQueue< int >( std::max( n , 1 ) ) ,
                  k ) ); } );
 }

/// shortestPaths() of the book on the priority queue Q or the queue Q
template< class G , class Q >
long long spRun( const G & g , int src , Q & q , Counts & k )
{
 ADS::FrontierCounts c;
 long long d = 0;
 if constexpr( requires { q.enqueue( 0 ); } ) {
  ADS::QueueFrontier F( q );
  d = digest( ADS::shortestPaths( g , src , F , &c ).d , ADS::INF );
  }
 else {
  ADS::PriorityFrontier F( q , g.bound() );
  d = digest( ADS::shortestPaths( g , src , F , &c ).d , ADS::INF );
  }
 k = counts( c );
 return( d );
 }

/// a run of an algorithm on a priority queue made for n nodes
template< class Q , class G , class Run >
auto withQueue( const G & g , int src , Run && run ) {
 return( [ & g , src , run ]( Counts & k ) {
  Q q( std::max( g.bound() , 1 ) );
  return( run( g , src , q , k ) ); } );
 }

/// the priority queues of each family on the run given
template< class Run >
void priorityQueues( Cell & C , const Input & I ,
                     const std::vector< Edge > & A , int src , Run && run )
{
 const int n = I.n;
 using P = Priority;
 if( measured( { "std vector set" , "std vector set A/A" } ) ) {
  const StdGraph<> G( n , A , I.directed );
  C( "std vector set" , withQueue< StdSetQueue >( G , src , run ) );
  C( "std vector set A/A" , withQueue< StdSetQueue >( G , src , run ) );
  }
#ifdef ADS_HAS_BOOST
 if( measured( { "Boost csr d_ary_heap<4>" , "Boost csr d_ary_heap<2>" ,
                 "Boost csr pairing_heap" , "Boost csr fibonacci_heap" } ) ) {
  const BoostGraph<> G( n , A , I.directed );
  C( "Boost csr d_ary_heap<4>" , withQueue< BoostDary4 >( G , src , run ) );
  C( "Boost csr d_ary_heap<2>" , withQueue< BoostDary2 >( G , src , run ) );
  C( "Boost csr pairing_heap" , withQueue< BoostPairing >( G , src , run ) );
  C( "Boost csr fibonacci_heap" ,
     withQueue< BoostFibonacci >( G , src , run ) );
  }
#endif
 if( ! measured( { "ADS StaticGraph HeapPriorityQueue<4>" ,
                   "ADS StaticGraph HeapPriorityQueue<2>" ,
                   "ADS StaticGraph IndexedDaryHeap<4>" ,
                   "ADS StaticGraph IndexedDaryHeap<2>" ,
                   "ADS StaticGraph PairingHeap" ,
                   "ADS StaticGraph FibonacciHeapPriorityQueue" } ) )
  return;
 const ADS::StaticGraph< int > S( n , edgesOf( I , A ) , I.directed );
 C( "ADS StaticGraph HeapPriorityQueue<4>" ,
    withQueue< ADS::HeapPriorityQueue< int , P , 4 > >( S , src , run ) );
 C( "ADS StaticGraph HeapPriorityQueue<2>" ,
    withQueue< ADS::HeapPriorityQueue< int , P , 2 > >( S , src , run ) );
 C( "ADS StaticGraph IndexedDaryHeap<4>" ,
    withQueue< ADS::IndexedDaryHeap< P , 4 > >( S , src , run ) );
 C( "ADS StaticGraph IndexedDaryHeap<2>" ,
    withQueue< ADS::IndexedDaryHeap< P , 2 > >( S , src , run ) );
 C( "ADS StaticGraph PairingHeap" , [ & ]( Counts & k ) {
  ADS::PairingHeap< int , P > q;
  return( run( S , src , q , k ) ); } );
 C( "ADS StaticGraph FibonacciHeapPriorityQueue" , [ & ]( Counts & k ) {
  ADS::FibonacciHeapPriorityQueue< int , P > q;
  return( run( S , src , q , k ) ); } );
 }

void dijkstra( const std::string & c , int n0 , int r , unsigned seed )
{
 const Input I = make( c , n0 , true , seed );
 const auto A = arcs( I );
 Cell C( "dijkstra" , c , std::size_t( I.n ) , A.size() , r );
 priorityQueues( C , I , A , source( I ) , []( const auto & g , int s ,
                 auto & q , Counts & k ) { return( spRun( g , s , q , k ) );
                 } );
 }

void bfm( const std::string & c , int n0 , int r , unsigned seed )
{
 const Input I = make( c , n0 , true , seed );
 const int n = I.n , src = source( I );
 const auto A = arcs( I );
 Cell C( "bfm" , c , std::size_t( n ) , A.size() , r );
 if( measured( { "std vector queue" , "std vector queue A/A" } ) ) {
  const StdGraph<> G( n , A , true );
  for( const char * who : { "std vector queue" , "std vector queue A/A" } )
   C( who , [ & ]( Counts & k ) {
    StdQueue q;
    return( spRun( G , src , q , k ) ); } );
  }
#ifdef ADS_HAS_BOOST
 if( measured( { "Boost csr queue" } ) ) {
  const BoostGraph<> G( n , A , true );
  C( "Boost csr queue" , [ & ]( Counts & k ) {
   BoostQueue q;
   return( spRun( G , src , q , k ) ); } );
  }
#endif
 if( ! measured( { "ADS StaticGraph ArrayQueue" } ) )
  return;
 const ADS::StaticGraph< int > S( n , edgesOf( I , A ) , true );
 C( "ADS StaticGraph ArrayQueue" , [ & ]( Counts & k ) {
  ADS::ArrayQueue< int > q( std::max( n , 1 ) );
  return( spRun( S , src , q , k ) ); } );
 }

/// prim() of the book on the priority queue q: a digest of the parents,
/// the same on every queue since the priorities break the ties
template< class G , class Q >
long long primRun( const G & g , int src , Q & q , Counts & k )
{
 ADS::FrontierCounts c;
 const auto p = ADS::prim( g , src , q , &c );
 long long s = 0;
 for( int v = 0 ; v < g.bound() ; ++v )
  s += ( long long )( v + 1 ) * ( p[ v ] + 1 );
 k = counts( c );
 return( s );
 }

void prim( const std::string & c , int n0 , int r , unsigned seed )
{
 const Input I = make( c , n0 , false , seed );
 const auto A = arcs( I );
 Cell C( "prim" , c , std::size_t( I.n ) , A.size() , r );
 priorityQueues( C , I , A , source( I ) , []( const auto & g , int s ,
                 auto & q , Counts & k ) { return( primRun( g , s , q , k ) );
                 } );
 }

/// kruskal() of the book on the MFSET M: the weight of the forest
template< class M >
long long kruskalRun( int n ,
                      const std::vector< ADS::WeightedEdge< int > > & E ,
                      Counts & k )
{
 M sets( std::max( n , 1 ) );
 ADS::MergeCounts c;
 long long s = 0;
 for( const auto & e : ADS::kruskal( n , E , sets , 0 , &c ) ) s += e.w;
 k = { { "merges" , c.merges } , { "unions" , c.unions } };
 return( s );
 }

void kruskal( const std::string & c , int n0 , int r , unsigned seed )
{
 const Input I = make( c , n0 , false , seed );
 const int n = I.n;
 std::vector< ADS::WeightedEdge< int > > E;
 for( const auto & [ u , v , w ] : edgesOf( I , arcs( I ) ) )
  E.push_back( { u , v , w } );
 Cell C( "kruskal" , c , std::size_t( n ) , E.size() , r );
 using ADS::Union;
 using ADS::Compression;
#ifdef ADS_HAS_BOOST
 for( const char * who : { "Boost disjoint_sets" ,
                           "Boost disjoint_sets A/A" } )
  C( who , [ & ]( Counts & k ) {
   return( kruskalRun< BoostSets >( n , E , k ) ); } );
#endif
 C( "ADS UnionFind" , [ & ]( Counts & k ) {
  return( kruskalRun< ADS::UnionFind >( n , E , k ) ); } );
 C( "ADS DisjointSets rank full" , [ & ]( Counts & k ) {
  return( kruskalRun< ADS::DisjointSets< Union::RANK , Compression::FULL > >(
          n , E , k ) ); } );
 C( "ADS MFSet" , [ & ]( Counts & k ) {
  return( kruskalRun< ADS::MFSet >( n , E , k ) ); } );
 }

/// scc() of the book on the graph g: the number of components
template< class G >
long long sccRun( const G & g , Counts & k )
{
 const auto c = ADS::scc( g );
 const long long K = c.empty() ? 0 : *std::max_element( c.begin() , c.end() );
 k = { { "components" , K } };
 return( K );
 }

void scc( const std::string & c , int n0 , int r , unsigned seed )
{
 const Input I = make( c , n0 , true , seed );
 const int n = I.n;
 const auto A = arcs( I );
 Cell C( "scc" , c , std::size_t( n ) , A.size() , r );
 if( measured( { "std vector" , "std vector A/A" } ) ) {
  const StdGraph< false > G( n , A , true );
  for( const char * who : { "std vector" , "std vector A/A" } )
   C( who , [ & ]( Counts & k ) { return( sccRun( G , k ) ); } );
  }
#ifdef ADS_HAS_BOOST
 if( measured( { "Boost csr" } ) ) {
  const BoostGraph< false > G( n , A , true );
  C( "Boost csr" , [ & ]( Counts & k ) { return( sccRun( G , k ) ); } );
  }
#endif
 if( ! measured( { "ADS StaticGraph" } ) )
  return;
 const ADS::StaticGraph<> S( ADS::StaticGraph< int >( n , edgesOf( I , A ) ,
                                                      true ) );
 C( "ADS StaticGraph" , [ & ]( Counts & k ) { return( sccRun( S , k ) ); } );
 }

/// n random merges and n finds on the MFSET M
template< class M >
long long setsRun( int n , const std::vector< std::pair< int , int > > & U ,
                   const std::vector< std::pair< int , int > > & F ,
                   Counts & k )
{
 M sets( n );
 long long unions = 0 , s = 0;
 for( const auto & [ a , b ] : U ) unions += sets.merge( a , b );
 for( const auto & [ a , b ] : F ) s += sets.same( a , b );
 k = { { "unions" , unions } };
 return( s );
 }

void unionFind( int n , int r , unsigned seed )
{
 std::mt19937_64 g( seed + unsigned( n ) );
 std::vector< std::pair< int , int > > U( n ) , F( n );
 for( auto & [ a , b ] : U ) { a = int( g() % n ); b = int( g() % n ); }
 for( auto & [ a , b ] : F ) { a = int( g() % n ); b = int( g() % n ); }
 Cell C( "union-find" , "random" , std::size_t( n ) , 2 * std::size_t( n ) ,
         r );
 using ADS::Union;
 using ADS::Compression;
#ifdef ADS_HAS_BOOST
 for( const char * who : { "Boost disjoint_sets" ,
                           "Boost disjoint_sets A/A" } )
  C( who , [ & ]( Counts & k ) {
   return( setsRun< BoostSets >( n , U , F , k ) ); } );
#endif
 C( "ADS UnionFind" , [ & ]( Counts & k ) {
  return( setsRun< ADS::UnionFind >( n , U , F , k ) ); } );
 C( "ADS DisjointSets rank full" , [ & ]( Counts & k ) {
  return( setsRun< ADS::DisjointSets< Union::RANK , Compression::FULL > >(
          n , U , F , k ) ); } );
 C( "ADS MFSet" , [ & ]( Counts & k ) {
  return( setsRun< ADS::MFSet >( n , U , F , k ) ); } );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 const int r = o.runs;
 // ADS_GRAPHS, if set, the benchmarks to run, separated by commas
 const char * only = std::getenv( "ADS_GRAPHS" );
 auto want = [ & ]( const std::string & b ) {
  if( ! only ) return( true );
  const std::string l = std::string( "," ) + only + ",";
  return( l.find( "," + b + "," ) != std::string::npos );
  };
 for( std::size_t n0 : o.sizes ) {
  const int n = int( n0 );
  if( want( "bfs" ) )
   for( const char * c : { "gnm4" , "grid" , "road" , "rmat" } )
    bfs( c , n , r , o.seed );
  if( want( "dijkstra" ) )
   for( const char * c : { "gnm4" , "gnm16" , "grid" , "road" , "rmat" } )
    dijkstra( c , n , r , o.seed );
  if( want( "bfm" ) )
   for( const char * c : { "grid" , "road" } )
    bfm( c , n , r , o.seed );
  if( want( "prim" ) )
   for( const char * c : { "gnm4" , "grid" , "road" , "rmat" } )
    prim( c , n , r , o.seed );
  if( want( "kruskal" ) )
   for( const char * c : { "gnm4" , "grid" , "road" , "rmat" } )
    kruskal( c , n , r , o.seed );
  if( want( "scc" ) )
   for( const char * c : { "gnm4" , "road" , "rmat" } )
    scc( c , n / 10 , r , o.seed );
  if( want( "union-find" ) )
   unionFind( n , r , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File graphs.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
