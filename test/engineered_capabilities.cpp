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
 * and their meet in Shadow; and the customization points reserve(),
 * prefetch() and insert_bulk(). At the level 3 of the contracts.
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
#include "ADS/set/RoaringBitmap.h"
#include "ADS/tree/JoinTree.h"

#include <cstdint>
#include <list>
#include <queue>
#include <random>
#include <set>
#include <span>
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
	       ! observes_values< IndexedDaryHeap< int > > );
static_assert( batch_extract< Every > && ! batch_extract< RadixHeap< int > > );
static_assert( shiftable_keys< Every > &&
	       ! shiftable_keys< DaryHeap< int > > );
static_assert( ! monotone_extract< RadixHeap< int > > );
static_assert( rekeyable< Every > && ! rekeyable< DaryHeap< int > > );
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
static_assert( ! nonmonotone_values< Every > );

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

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File engineered_capabilities.cpp ------------------*/
/*--------------------------------------------------------------------------*/
