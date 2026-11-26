/*--------------------------------------------------------------------------*/
/*-------------------- File engineered_capabilities.cpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the capabilities of Capabilities.h, beyond the book: the two
 * concepts of the priority queues and their customization points on the
 * queues of ADS and on std::priority_queue; decrease_key_of() and its
 * opt-in; the shortest paths and Prim's algorithm on lazy queues, against
 * the native ones; each capability concept on a structure of ADS that
 * models it and on one that does not, where they exist; the semantic
 * opt-ins; the queries, their defaults, their forwarding by ObservedGraph
 * and their meet in Shadow; the customization points reserve(),
 * prefetch() and insert_bulk(); the capabilities of the incremental
 * searches on the structures that model them, i.e., extract() of every
 * queue that has it against repeated pops, rebase() of the monotone
 * queues between two episodes, rekey() of IndexedDaryHeap, TableHeap as
 * an observer of an ObservableArray against a heap fed by hand, and the
 * values of ConcurrentCuckooTable raised and lowered by writers while
 * readers check that each pair they read is whole. At the level 3 of the
 * contracts.
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

#include "ADS/Capabilities.h"
#include "ADS/Modification.h"
#include "ADS/concurrent/ConcurrentCuckooTable.h"
#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/concurrent/MultiQueue.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/mfset/RollbackUnionFind.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/DialQueue.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/priority/TreePriorityQueue.h"
#include "ADS/sequence/ObservableArray.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/tree/JoinTree.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <list>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

using P = std::pair< int , int >;
using Edges = std::vector< std::tuple< int , int , int > >;

/*--------------------------------------------------------------------------*/
/*------------------------------ THE QUEUES --------------------------------*/
/*--------------------------------------------------------------------------*/

// the two concepts: every queue with handles models both, the others the
// base one only
static_assert( AddressablePriorityQueue< HeapPriorityQueue< int , P > > );
static_assert( AddressablePriorityQueue< ListPriorityQueue< int > > );
static_assert( AddressablePriorityQueue< TreePriorityQueue< int > > );
static_assert( AddressablePriorityQueue< FibonacciHeapPriorityQueue< int > > );
static_assert( AddressablePriorityQueue< PairingHeap< int > > );
static_assert( AddressablePriorityQueue< IndexedDaryHeap< int > > );
static_assert( PriorityQueue< LinkedHeapPriorityQueue< int > > &&
	       ! AddressablePriorityQueue< LinkedHeapPriorityQueue< int > > );
static_assert( PriorityQueue< DaryHeap< int > > &&
	       ! AddressablePriorityQueue< DaryHeap< int > > );
static_assert( PriorityQueue< BucketQueue< int > > );
static_assert( PriorityQueue< DialQueue< int > > );
static_assert( PriorityQueue< RadixHeap< int > > );
static_assert( PriorityQueue< std::priority_queue< int > > &&
	       ! AddressablePriorityQueue< std::priority_queue< int > > );
// a relaxed queue, with no exact min(): not a PriorityQueue
static_assert( ! PriorityQueue< MultiQueue< int , int > > );
static_assert( ! PriorityQueue< std::vector< int > > );

// how they lower a key
static_assert( decrease_key_of< HeapPriorityQueue< int > >() ==
	       decrease_key_kind::native );
static_assert( decrease_key_of< IndexedDaryHeap< int > >() ==
	       decrease_key_kind::native );
static_assert( decrease_key_of< LinkedHeapPriorityQueue< int > >() ==
	       decrease_key_kind::lazy );
static_assert( decrease_key_of< DaryHeap< int > >() ==
	       decrease_key_kind::lazy );
static_assert( decrease_key_of< std::priority_queue< int > >() ==
	       decrease_key_kind::lazy );
static_assert( decrease_key_of< MultiQueue< int , int > >() ==
	       decrease_key_kind::lazy );  // its opt-in
static_assert( decrease_key_of< std::vector< int > >() ==
	       decrease_key_kind::none );

// the handles with increase() and erase() as well
static_assert( addressable< PairingHeap< int > > );
static_assert( addressable< IndexedDaryHeap< int > > );
static_assert( ! addressable< HeapPriorityQueue< int > > );

/// a heap with handles taken as lazy by its opt-in: decrease() unused
template< class X , class Y >
struct LazyHeap : HeapPriorityQueue< X , Y , 4 >
{
 using HeapPriorityQueue< X , Y , 4 >::HeapPriorityQueue;
 static constexpr decrease_key_kind decrease_key = decrease_key_kind::lazy;
 };

static_assert( AddressablePriorityQueue< LazyHeap< int , P > > );
static_assert( decrease_key_of< LazyHeap< int , P > >() ==
	       decrease_key_kind::lazy );

/// a queue of another library with free functions of its own, found by
/// argument-dependent lookup
struct Foreign
{
 std::priority_queue< int , std::vector< int > , std::greater<> > q;
 using value_type = int;
 [[nodiscard]] bool empty( void ) const { return( q.empty() ); }
 };
int queue_min( Foreign & f )
{
 return( f.q.top() );
 }
int queue_deleteMin( Foreign & f )
{
 const int x = f.q.top();
 f.q.pop();
 return( x );
 }
void queue_insert( Foreign & f , int x )
{
 f.q.push( x );
 }

static_assert( PriorityQueue< Foreign > );

/*--------------------------------------------------------------------------*/
/*---------------------- THE CONCEPTS OF CAPABILITIES ----------------------*/
/*--------------------------------------------------------------------------*/

using Sum = JoinTree< int , int , AVLBalance , SumAugmentation< int > >;
using Max = JoinTree< int , int , AVLBalance , MaxAugmentation< int > >;
using Epochs =
  ConcurrentFlatTable< std::uint64_t , std::uint64_t ,
		       DefaultHash< std::uint64_t > ,
		       std::equal_to< std::uint64_t > , EpochClear<> >;
using Zeroing = ConcurrentFlatTable< std::uint64_t , std::uint64_t >;
using OG = ObservedGraph< ListGraph< int > >;

/// the members of the capabilities no structure of ADS has yet
struct Every
{
 using Key = int;
 using Item = int;
 using Priority = int;
 void insert_bulk( std::span< const int > ) { }
 void erase_bulk( std::span< const int > ) { }
 void applyRange( int , int , int ) { }
 void update( int ) { }
 void extract( int , std::vector< int > & ) { }
 void shift_all( int ) { }
 template< class F >
 void rekey( F ) { }
 [[nodiscard]] int snapshot( void ) const { return( 0 ); }
 template< class F >
 void forEachIn( int , F ) const { }
 [[nodiscard]] std::size_t arcId( int , int ) const { return( 0 ); }
 static constexpr unsigned monotone = mod::effect::down;
 };

static_assert( bulk_insertable< Every > && ! bulk_insertable< Sum > );
static_assert( bulk_erasable< Every > && ! bulk_erasable< Sum > );
static_assert( splittable< Sum > && ! splittable< DaryHeap< int > > );
static_assert( joinable< Sum > && ! joinable< DaryHeap< int > > );
static_assert( rankable< RoaringBitmap > && ! rankable< DaryHeap< int > > );
static_assert( selectable< RoaringBitmap > &&
	       ! selectable< DaryHeap< int > > &&
	       selectable< Sum , SumAugmentation< int > > &&
	       ! selectable< DaryHeap< int > , SumAugmentation< int > > );
static_assert( prefetchable< Zeroing > && ! prefetchable< DaryHeap< int > > );
static_assert( reservable< DaryHeap< int > > &&
	       ! reservable< ListPriorityQueue< int > > );
static_assert( augmented_by< Sum , SumAugmentation< int > > &&
	       ! augmented_by< Max , SumAugmentation< int > > );
static_assert( range_actionable< Every , int > &&
	       ! range_actionable< Sum , int > );
static_assert( observes_values< Every > &&
	       observes_values< TableHeap< std::vector< int > > > &&
	       observes_values< TableHeap< ObservableArray< long > > > &&
	       ! observes_values< IndexedDaryHeap< int > > );
static_assert( batch_extract< Every > &&
	       batch_extract< IndexedDaryHeap< int > > &&
	       batch_extract< TableHeap< std::vector< int > > > &&
	       batch_extract< RadixHeap< int > > &&
	       batch_extract< DialQueue< int > > &&
	       batch_extract< BucketQueue< int > > &&
	       ! batch_extract< DaryHeap< int > > );
static_assert( shiftable_keys< Every > &&
	       ! shiftable_keys< DaryHeap< int > > );
static_assert( monotone_extract< RadixHeap< int > > &&
	       monotone_extract< DialQueue< int > > &&
	       ! monotone_extract< IndexedDaryHeap< int > > );
static_assert( rekeyable< Every > && rekeyable< IndexedDaryHeap< int > > &&
	       ! rekeyable< DaryHeap< int > > );
static_assert( erasable< ConcurrentCuckooTable< int , int > > &&
	       ! erasable< Zeroing > );
static_assert( epoch_clearable< Epochs > && ! epoch_clearable< Zeroing > );
static_assert( undoable< RollbackUnionFind > && ! undoable< DisjointSets<> > );
static_assert( snapshotable< Every > && ! snapshotable< Sum > );
static_assert( in_arcs< Every > && ! in_arcs< ListGraph< int > > );
static_assert( stable_edge_ids< Every > &&
	       ! stable_edge_ids< ListGraph< int > > );
static_assert( batch_updatable< OG > &&
	       ! batch_updatable< ListGraph< int > > );

/*--------------------------------------------------------------------------*/
/*--------------------------- THE SEMANTIC OPT-INS -------------------------*/
/*--------------------------------------------------------------------------*/

static_assert( enable_stable_references< PairingHeap< int > > &&
	       enable_stable_references< FibonacciHeapPriorityQueue< int > > &&
	       ! enable_stable_references< IndexedDaryHeap< int > > );
static_assert( declares_monotone< Every > &&
	       monotone_of< Every >() == mod::effect::down );
static_assert( ! declares_monotone< ListGraph< int > > &&
	       ( monotone_of< ListGraph< int > >() & mod::effect::up ) );
static_assert( ! nonmonotone_values< Every > &&
	       nonmonotone_values< ConcurrentCuckooTable< int , int > > &&
	       ! nonmonotone_values< Zeroing > );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE QUERIES -------------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph that answers the query of the stable references itself
struct StableGraph : ListGraph< int >
{
 using ListGraph< int >::ListGraph;
 static constexpr bool query( stable_references_t ) { return( true ); }
 static constexpr complexity query( rank_complexity_t ) {
  return( complexity::logarithmic );
  }
 };

/// an observer of the edges, with the stable references by its opt-in
struct Edgy
{
 int n = 0;
 void on( const EdgeInserted< int > & ) { ++n; }
 };

/// an observer of the edges that answers its queries
struct Ranked
{
 int n = 0;
 void on( const EdgeInserted< int > & ) { ++n; }
 static constexpr bool query( stable_references_t ) { return( true ); }
 static constexpr complexity query( rank_complexity_t ) {
  return( complexity::constant );
  }
 };

}  // end( namespace )

template<>
inline constexpr bool ADS::enable_stable_references< Edgy > = true;

namespace
{

// the defaults
static_assert( query_v< PairingHeap< int > , stable_references_t > );
static_assert( ! query_v< HeapPriorityQueue< int > , stable_references_t > );
static_assert( ! query_v< PairingHeap< int > , thread_safe_reads_t > );
static_assert( query_v< RoaringBitmap , rank_complexity_t > ==
	       complexity::linear );
static_assert(
  std::same_as<
    std::remove_cvref_t<
      decltype( query_v< HeapPriorityQueue< int > , event_set_t > ) >::type ,
    mod::list<> > );
// the answers of the structures
static_assert( query_v< StableGraph , stable_references_t > &&
	       query_v< StableGraph , rank_complexity_t > ==
		 complexity::logarithmic );
// forwarded by the graph wrapper, the events its own
static_assert( ! query_v< OG , stable_references_t > );
static_assert( query_v< ObservedGraph< StableGraph > , stable_references_t > );
static_assert( query_v< Observable< StableGraph > , rank_complexity_t > ==
	       complexity::logarithmic );
static_assert(
  std::same_as<
    std::remove_cvref_t< decltype( query_v< OG , event_set_t > ) >::type ,
    GraphModifications< int > > );
static_assert(
  std::same_as< std::remove_cvref_t<
		  decltype( query_v< ListGraph< int > ,
				     event_set_t > ) >::type ,
		mod::list<> > );
// met by Shadow: both must have them, the worse of the times
static_assert( query_v< Shadow< Edgy , Ranked > , stable_references_t > );
static_assert( ! query_v< Shadow< Edgy , Every > , stable_references_t > );
static_assert( query_v< Shadow< Ranked , Ranked > , rank_complexity_t > ==
	       complexity::constant );
static_assert( query_v< Shadow< Edgy , Ranked > , rank_complexity_t > ==
	       complexity::linear );

/*--------------------------------------------------------------------------*/
/*------------------------------- THE GRAPHS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// m random edges on the nodes [ 0 , n ), of weights in [ lo , hi ]
Edges randomEdges( int n , int m , int lo , int hi , std::mt19937 & g )
{
 Edges E;
 for( int i = 0 ; i < m ; ++i ) {
  const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
  if( u != v )
   E.emplace_back( u , v , lo + int( g() % unsigned( hi - lo + 1 ) ) );
  }
 return( E );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE TESTS -------------------------------*/
/*--------------------------------------------------------------------------*/

// the customization points of the queues: the members of the book, the
// free functions of a foreign type, the spelling of the standard (pop()
// with and without the item), the keys of each order
ADS_TEST( "cap-queue-points" )
{
 std::priority_queue< int , std::vector< int > , std::greater<> > s;
 for( int x : { 5 , 1 , 4 , 2 , 3 } )
  ADS::queue_insert( s , x );
 CHECK( ADS::queue_min( s ) == 1 );
 for( int x = 1 ; x <= 5 ; ++x )
  CHECK( ADS::queue_deleteMin( s ) == x );
 CHECK( s.empty() );
 Foreign f;
 ADS::queue_insert( f , 7 );
 ADS::queue_insert( f , 3 );
 CHECK( ADS::queue_min( f ) == 3 && ADS::queue_deleteMin( f ) == 3 );
 DaryHeap< int , std::greater< int > > d;
 ADS::queue_insert( d , 2 );
 ADS::queue_insert( d , 9 );
 CHECK( ADS::queue_min( d ) == 9 && ADS::queue_deleteMin( d ) == 9 );
 BucketQueue< int > b;
 ADS::queue_insert( b , 10 , 3 , 0 );
 ADS::queue_insert( b , 20 , 1 , 0 );
 CHECK( ADS::queue_min( b ) == 20 && ADS::queue_deleteMin( b ) == 20 );
 RadixHeap< int > r;
 ADS::queue_insert( r , 4 , 8u );
 ADS::queue_insert( r , 6 , 2u );
 CHECK( ADS::queue_deleteMin( r ) == 6 && ADS::queue_deleteMin( r ) == 4 );
 LinkedHeapPriorityQueue< int > l;
 ADS::queue_insert( l , 1 , 5 );
 ADS::queue_insert( l , 2 , 4 );
 CHECK( ADS::queue_min( l ) == 2 && ADS::queue_deleteMin( l ) == 2 );
 }

// the shortest paths and Prim's algorithm on lazy queues, a new insertion
// for each fall of a key: the same distances, parents, order of extraction
// and counts as on a native one, also with negative weights and no
// negative cycle (a node then extracted more than once)
ADS_TEST( "cap-lazy-frontier" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 12 ; ++t ) {
  const int n = 2 + int( g() % 200 );
  const Edges E = randomEdges( n , 1 + int( g() % unsigned( 5 * n ) ) ,
			       t % 3 ? 1 : 0 , t % 2 ? 1000 : 3 , g );
  const StaticGraph< int > S( n + 1 , E , true );
  HeapPriorityQueue< int , P , 4 > h( n + 1 );
  PriorityFrontier Fh( h , S.bound() );
  FrontierCounts kh;
  const auto R = shortestPaths( S , 0 , Fh , &kh );
  auto same = [ & ]( auto & q ) {
   PriorityFrontier F( q , S.bound() );
   FrontierCounts k;
   const auto L = shortestPaths( S , 0 , F , &k );
   CHECK( L.d == R.d && L.T == R.T && L.order == R.order );
   CHECK( k.adds == kh.adds && k.extracts == kh.extracts &&
	  k.decreases == kh.decreases && k.edges == kh.edges );
   };
  {
   LinkedHeapPriorityQueue< int , P > q;
   same( q );
   }
  {
   LazyHeap< int , P > q( n + 1 + int( kh.decreases ) );
   same( q );
   }
  // a DAG with negative weights: the label-correcting case
  Edges D;
  for( const auto & [ u , v , w ] : E )
   if( u < v )
    D.emplace_back( u , v , w - 500 );
  const StaticGraph< int > A( n , D , true );
  HeapPriorityQueue< int , P , 2 > a( n );
  PriorityFrontier Fa( a , A.bound() );
  FrontierCounts ka;
  const auto RA = shortestPaths( A , 0 , Fa , &ka );
  LinkedHeapPriorityQueue< int , P > la;
  PriorityFrontier Fl( la , A.bound() );
  FrontierCounts kl;
  const auto LA = shortestPaths( A , 0 , Fl , &kl );
  CHECK( LA.d == RA.d && LA.T == RA.T && LA.order == RA.order );
  CHECK( kl.adds == ka.adds && kl.extracts == ka.extracts &&
	 kl.decreases == ka.decreases );
  // Prim, on an unoriented graph without parallel edges
  std::set< std::pair< int , int > > seen;
  Edges SE;
  for( const auto & [ u , v , w ] : E )
   if( seen.emplace( std::min( u , v ) , std::max( u , v ) ).second )
    SE.emplace_back( u , v , w );
  const StaticGraph< int > U( n , SE , false );
  HeapPriorityQueue< int , P , 4 > ph( n );
  FrontierCounts cn , cl;
  const auto pn = prim( U , 0 , ph , &cn );
  LinkedHeapPriorityQueue< int , P > pl;
  const auto pp = prim( U , 0 , pl , &cl );
  CHECK( pp == pn && cl.extracts == cn.extracts &&
	 cl.decreases == cn.decreases && cl.edges == cn.edges );
  }
 }

// the queries asked of objects through the customization point, the same
// answers as query_v
ADS_TEST( "cap-queries" )
{
 PairingHeap< int > q;
 CHECK( ADS::query( q , stable_references ) );
 CHECK( ! ADS::query( q , thread_safe_reads ) );
 StableGraph g( 3 );
 ObservedGraph< StableGraph > og( g );
 CHECK( ADS::query( og , stable_references ) );
 CHECK( og.query( stable_references_t() ) );
 CHECK( ADS::query( og , rank_complexity ) == complexity::logarithmic );
 Edgy e;
 Ranked k;
 Shadow< Edgy , Ranked > sh( e , k );
 CHECK( ADS::query( sh , stable_references ) );
 CHECK( sh.query( rank_complexity_t() ) == complexity::linear );
 // the query of Shadow by a function still compares the two answers
 ObservedGraph< StableGraph , Static< Shadow< Edgy , Ranked > > > ow( g , sh );
 ow.insertEdge( 0 , 1 , 2 );
 CHECK( sh.query( []( const auto & o ) { return( o.n ); } ) == 1 );
 CHECK( sh.counts().mismatches == 0 );
 }

// the customization points of the structures that ADS does not own:
// reserve(), prefetch() and insert_bulk(), with their defaults
ADS_TEST( "cap-points" )
{
 std::vector< int > v;
 ADS::reserve( v , 100 );
 CHECK( v.capacity() >= 100 );
 std::list< int > l;
 ADS::reserve( l , 100 );  // nothing
 CHECK( l.empty() );
 DaryHeap< int > d;
 ADS::reserve( d , 50 );
 CHECK( d.empty() );
 Zeroing z( 16 );
 z.insert( 3 , 4 );
 ADS::prefetch( z , z.hash( 3 ) );
 ADS::prefetch( v , 3 );  // nothing
 CHECK( z.find( 3 ) == std::optional< std::uint64_t >( 4 ) );
 std::set< int > s;
 const std::vector< int > r{ 3 , 1 , 2 , 3 };
 ADS::insert_bulk( s , r );  // one by one
 CHECK( s == std::set< int >( { 1 , 2 , 3 } ) );
 RoaringBitmap b;
 ADS::insert_bulk( b , std::vector< std::uint32_t >{ 7 , 9 } );
 CHECK( b.rank( 9 ) == 1 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- THE CAPABILITIES OF THE SEARCHES ------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "cap-extract" )
{
 // extract( theta ) of each queue against pops while the key is at most
 // theta, on the same random keys, a band at a time as a stepping
 // algorithm takes them
 std::mt19937 rng( 5 );
 const int n = 300;
 std::vector< int > key( n );
 for( int & k : key )
  k = int( rng() % 1000 );
 IndexedDaryHeap< int > h( n ) , h2( n );
 RadixHeap< int > r , r2;
 DialQueue< int > q( 255 ) , q2( 255 );
 BucketQueue< int > b , b2;
 for( int i = 0 ; i < n ; ++i ) {
  h.push( i , key[ i ] );
  h2.push( i , key[ i ] );
  r.push( i , unsigned( key[ i ] ) );
  r2.push( i , unsigned( key[ i ] ) );
  q.push( i , unsigned( key[ i ] ) );
  q2.push( i , unsigned( key[ i ] ) );
  b.push( i , key[ i ] , i % 7 );
  b2.push( i , key[ i ] , i % 7 );
  }
 for( int theta = 99 ; theta < 1100 ; theta += 100 ) {
  std::vector< int > a , c;
  h.extract( theta , a );
  while( ! h2.empty() && ( h2.top().key <= theta ) )
   c.push_back( h2.pop().id );
  CHECK( a == c );
  a.clear();
  c.clear();
  r.extract( unsigned( theta ) , a );
  while( ! r2.empty() && ( r2.peekKey() <= unsigned( theta ) ) )
   c.push_back( r2.pop() );
  CHECK( ( a.size() == c.size() ) &&
	 std::is_permutation( a.begin() , a.end() , c.begin() ) );
  for( std::size_t i = 1 ; i < a.size() ; ++i )
   CHECK( key[ a[ i - 1 ] ] <= key[ a[ i ] ] );
  a.clear();
  c.clear();
  q.extract( unsigned( theta ) , a );
  while( ! q2.empty() && ( q2.peekKey() <= unsigned( theta ) ) )
   c.push_back( q2.pop() );
  CHECK( a == c );
  // the keys pushed next need only be above the last one extracted
  if( ! q.empty() )
   CHECK( q.peekKey() > unsigned( theta ) );
  a.clear();
  c.clear();
  b.extract( theta , a );
  while( ! b2.empty() && ( b2.topF() <= theta ) )
   c.push_back( b2.pop() );
  CHECK( a == c );
  }
 CHECK( h.empty() && r.empty() && q.empty() && b.empty() );
 // a push between the band extracted and the next key, which a top key
 // read by topKey() would forbid
 DialQueue< int > d( 63 );
 d.push( 1 , 10 );
 d.push( 2 , 50 );
 std::vector< int > o;
 d.extract( 20 , o );
 d.push( 3 , 30 );
 CHECK( ( o == std::vector< int >{ 1 } ) && ( d.pop() == 3 ) );
 }

ADS_TEST( "cap-rebase" )
{
 // the monotone queues between two episodes: the keys of the second start
 // below the last key extracted in the first
 RadixHeap< int > r;
 DialQueue< int > q( 63 );
 for( int i = 0 ; i < 50 ; ++i ) {
  r.push( i , unsigned( 100 + 7 * i ) );
  q.push( i , unsigned( 100 + 7 * i ) );
  }
 for( int i = 0 ; i < 20 ; ++i )
  CHECK( ( r.pop() == i ) && ( q.pop() == i ) );
 // the keys left are at least 240: the next episode starts from 50
 r.rebase( 50 );
 q.rebase( 50 );
 r.push( 100 , 60 );
 q.push( 100 , 60 );
 r.push( 101 , 1000 );  // into the overflow of the Dial queue
 q.push( 101 , 1000 );
 CHECK( ( r.size() == 32 ) && ( q.size() == 32 ) );
 CHECK( ( r.topKey() == 60 ) && ( q.topKey() == 60 ) );
 CHECK( ( r.pop() == 100 ) && ( q.pop() == 100 ) );
 for( int i = 20 ; i < 50 ; ++i ) {
  CHECK( ( r.topKey() == unsigned( 100 + 7 * i ) ) &&
	 ( q.topKey() == unsigned( 100 + 7 * i ) ) );
  CHECK( ( r.pop() == i ) && ( q.pop() == i ) );
  }
 CHECK( ( r.pop() == 101 ) && ( q.pop() == 101 ) );
 CHECK( r.empty() && q.empty() );
 // on an empty queue, and with signed keys below zero
 RadixHeap< int , int > s;
 s.push( 1 , 5 );
 CHECK( s.pop() == 1 );
 s.rebase( -3 );
 s.push( 2 , -3 );
 s.push( 3 , -1 );
 CHECK( ( s.topKey() == -3 ) && ( s.pop() == 2 ) && ( s.pop() == 3 ) );
 }

ADS_TEST( "cap-rekey" )
{
 // the keys of a heap recomputed, against a heap built from scratch on
 // them; the observers told Rebuilt
 struct Rb
 {
  int r = 0;
  void on( const mod::Rebuilt & ) noexcept { ++r; }
  };
 const int n = 200;
 Rb rb;
 IndexedDaryHeap< int , 4 , Static< Rb > > h( n , rb );
 std::mt19937 rng( 3 );
 for( int i = 0 ; i < n ; i += 2 )
  h.push( i , int( rng() % 100 ) );
 auto f = []( int id ) { return( ( id * 37 ) % 101 ); };
 h.rekey( f );
 CHECK( rb.r == 1 );
 IndexedDaryHeap< int > p( n );
 for( int i = 0 ; i < n ; i += 2 )
  p.push( i , f( i ) );
 while( ! p.empty() ) {
  CHECK( h.top().key == p.top().key );
  CHECK( h.key( h.top().id ) == f( h.top().id ) );
  h.pop();
  p.pop();
  }
 CHECK( h.empty() );
 }

ADS_TEST( "cap-tableheap" )
{
 // the queue of LaB-PQ as an observer of an observable table, against a
 // heap fed by hand with the same keys
 using Table = ObservableArray< long , Dynamic<> >;
 const int n = 64;
 Table t( std::size_t( n ) , 1000 );
 TableHeap< Table > q( t , n );
 t.attach( q );
 IndexedDaryHeap< long > p( n );
 std::mt19937 rng( 9 );
 for( int s = 0 ; s < 3000 ; ++s ) {
  const int id = int( rng() % unsigned( n ) );
  const int r = int( rng() % 5 );
  if( r < 3 ) {  // a write of the table: the id enters or moves
   const long k = long( rng() % 500 );
   t.set( std::size_t( id ) , k );
   if( p.contains( id ) )
    p.update( id , k );
   else
    p.push( id , k );
   }
  else if( r == 3 ) {  // the frontier extracts a band
   const long theta = long( rng() % 500 );
   std::vector< int > a , c;
   q.extract( theta , a );
   p.extract( theta , c );
   CHECK( a.size() == c.size() );
   for( std::size_t i = 0 ; i < a.size() ; ++i )
    CHECK( t[ std::size_t( a[ i ] ) ] == t[ std::size_t( c[ i ] ) ] );
   for( int x : c )
    CHECK( ! q.contains( x ) );
   }
  else if( ! q.empty() ) {
   CHECK( t[ std::size_t( q.min() ) ] == p.top().key );
   const int x = q.deleteMin();
   p.erase( x );
   }
  CHECK( q.size() == p.size() );
  for( int i = 0 ; i < n ; ++i )
   CHECK( ( q.contains( i ) == p.contains( i ) ) &&
	  ( ! q.contains( i ) || ( q.key( i ) == p.key( i ) ) ) );
  }
 // a value of the table erased, then the table rebuilt smaller
 t.push_back( 3 );
 CHECK( q.contains( n ) && ( q.min() == n ) );
 t.pop_back();
 CHECK( ! q.contains( n ) );
 for( int i = 0 ; i < 10 ; ++i )
  q.update( i );
 const std::vector< long > w{ 9 , 8 , 7 , 6 , 5 };
 t.assign( w.begin() , w.end() );
 CHECK( ( q.size() == 5 ) && ( q.min() == 4 ) && ( q.key( 0 ) == 9 ) );
 t.clear();
 CHECK( q.empty() );
 // a plain vector as the table, updated by hand
 std::vector< int > v{ 5 , 3 , 8 };
 TableHeap< std::vector< int > > u( v );
 for( int i = 0 ; i < 3 ; ++i )
  u.update( i );
 v[ 2 ] = 1;
 u.update( 2 );
 CHECK( ( u.deleteMin() == 2 ) && ( u.deleteMin() == 1 ) );
 }

ADS_TEST( "cap-nonmonotone" )
{
 // the values of a concurrent table raised and lowered by one writer per
 // key while readers check that each pair they read is whole, g and its
 // parent written together (R2); the writers start after the first read
 // and each reader reads 1000 pairs at least, so that the two overlap on
 // any number of processors
 struct Node
 {
  std::int32_t g , parent;
  };
 struct Wide
 {
  std::int64_t g , parent , pad;
  };
 auto run = [ & ]< class V >( V ) {
  auto make = []( auto g ) {  // pad, if any, 0
   V x{};
   x.g = decltype( V::g )( g );
   x.parent = decltype( V::parent )( 3 * g + 1 );
   return( x );
   };
  ConcurrentCuckooTable< int , V > t( 64 );
  const int keys = 32;
  for( int k = 0 ; k < keys ; ++k )
   t.insert( k , make( 0 ) );
  std::atomic< bool > stop{ false };
  std::atomic< long > reads{ 0 } , missing{ 0 } , torn{ 0 };
  std::vector< std::thread > W , R;
  for( int r = 0 ; r < 2 ; ++r )
   R.emplace_back( [ & , r ] {
    std::mt19937 rng( unsigned( 31 + r ) );
    for( long n = 0 ; ( n < 1000 ) || ! stop.load() ; ++n ) {
     const auto x = t.find( int( rng() % keys ) );
     reads.fetch_add( 1 );
     if( ! x )
      missing.fetch_add( 1 );
     else if( x->parent != 3 * x->g + 1 )
      torn.fetch_add( 1 );
     }
    } );
  for( int w = 0 ; w < 2 ; ++w )
   W.emplace_back( [ & , w ] {
    while( reads.load() == 0 )
     std::this_thread::yield();
    std::mt19937 rng( unsigned( 11 + w ) );
    for( int s = 0 ; s < 20000 ; ++s ) {
     const int k = 2 * int( rng() % ( keys / 2 ) ) + w;  // its own keys
     t.insertOrAssign( k , make( rng() % 1000 ) );        // up or down
     }
    } );
  for( auto & x : W )
   x.join();
  stop.store( true );
  for( auto & x : R )
   x.join();
  CHECK( reads.load() >= 2000 );
  CHECK( missing.load() == 0 );
  CHECK( torn.load() == 0 );
  };
 run( Node{} );
 run( Wide{} );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File engineered_capabilities.cpp ------------------*/
/*--------------------------------------------------------------------------*/
