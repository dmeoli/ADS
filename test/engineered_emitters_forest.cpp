/*--------------------------------------------------------------------------*/
/*------------------- File engineered_emitters_forest.cpp ------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tests of the forests that tell their changes, beyond the book: the
 * links of the roots of DisjointSets and the edges and values of
 * LinkCutTree, each as the exact records of a sequence of operations, as
 * the state that an observer rebuilds from the records alone (the
 * partition, the unrooted forest and its values) compared with the
 * structure after random operations, as the records of a batch coalesced,
 * and as the same answers of the plain and of the observed structure; and
 * the incumbents of the branch-&-bound searches, which improve strictly up
 * to the optimum returned, with the same results as with no observer.
 *
 *   engineered_emitters_forest <test>
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

#include "ADS/Modification.h"
#include "ADS/intractable/BranchAndBound.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/tree/LinkCutTree.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <set>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PARTITIONS ----------------------------*/
/*--------------------------------------------------------------------------*/

using Link = DisjointSets<>::Linked;

/// an observer that keeps the records of a union-find
struct LinkLog
{
 std::vector< Link > links;
 int rebuilt = 0;
 void on( const Link & e ) { links.push_back( e ); }
 void on( const mod::Rebuilt & ) noexcept { ++rebuilt; }
 };

/// an observer that keeps a parent for each element from the links alone,
/// and from the structure at a `Rebuilt`
template< class S >
struct LinkMirror
{
 std::vector< int > par;
 S * src = nullptr;
 bool stale = false;
 int roots = 0;
 explicit LinkMirror( int n ) : par( std::size_t( n ) , -1 ) , roots( n ) { }
 void on( const Link & e ) {
  par[ std::size_t( e.h ) ] = e.to;
  --roots;
  }
 void on( const mod::Rebuilt & ) noexcept { stale = true; }
 /// the root of x in the mirror
 [[nodiscard]] int root( int x ) const {
  while( par[ std::size_t( x ) ] >= 0 )
   x = par[ std::size_t( x ) ];
  return( x );
  }
 /// the parents read again from the structure, after a `Rebuilt`
 void sync( void ) {
  if( ! stale )
   return;
  par.assign( std::size_t( src->n() ) , -1 );
  roots = 0;
  for( int x = 0 ; x < src->n() ; ++x ) {
   const int r = src->find( x );
   par[ std::size_t( x ) ] = r == x ? -1 : r;
   roots += r == x ? 1 : 0;
   }
  stale = false;
  }
 /// the number of components agrees (level 3 of the contracts)
 template< class T >
 [[nodiscard]] bool audit( const T & s ) const {
  return( stale || ( roots == s.count() ) );
  }
 };

/// whether the forest of parents par and the structure d have the same
/// components, by a bijection of their roots
template< class S , class M >
bool samePartition( const M & m , S & d )
{
 if( int( m.par.size() ) != d.n() )
  return( false );
 std::vector< int > to( m.par.size() , -1 ) , from( m.par.size() , -1 );
 for( int x = 0 ; x < d.n() ; ++x ) {
  const int a = m.root( x ) , b = d.find( x );
  if( ( to[ std::size_t( a ) ] >= 0 ) && ( to[ std::size_t( a ) ] != b ) )
   return( false );
  if( ( from[ std::size_t( b ) ] >= 0 ) && ( from[ std::size_t( b ) ] != a ) )
   return( false );
  to[ std::size_t( a ) ] = b;
  from[ std::size_t( b ) ] = a;
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE FORESTS ------------------------------*/
/*--------------------------------------------------------------------------*/

using LCT = LinkCutTree<>;
using NIns = LCT::NodeInserted;
using VChg = LCT::ValueChanged;
using EIns = LCT::EdgeInserted;
using EEra = LCT::EdgeErased;
using LEvent = std::variant< NIns , VChg , EIns , EEra , mod::Rebuilt >;

/// an observer that keeps the records of a forest
struct ForestLog
{
 std::vector< LEvent > ev;
 void on( const NIns & e ) { ev.emplace_back( e ); }
 void on( const VChg & e ) { ev.emplace_back( e ); }
 void on( const EIns & e ) { ev.emplace_back( e ); }
 void on( const EEra & e ) noexcept {
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 };

/// an observer that keeps the unrooted forest and the values from the
/// records alone
struct ForestMirror
{
 std::vector< long long > val;
 std::vector< std::set< int > > adj;
 int edges = 0;
 ForestMirror( int n , long long id )
     : val( std::size_t( n ) , id ) , adj( std::size_t( n ) ) { }
 void on( const NIns & e ) {
  val.push_back( e.value );
  adj.emplace_back();
  }
 void on( const VChg & e ) { val[ std::size_t( e.h ) ] = e.to; }
 void on( const EIns & e ) {
  adj[ std::size_t( e.key.first ) ].insert( e.key.second );
  adj[ std::size_t( e.key.second ) ].insert( e.key.first );
  ++edges;
  }
 void on( const EEra & e ) noexcept {
  adj[ std::size_t( e.key.first ) ].erase( e.key.second );
  adj[ std::size_t( e.key.second ) ].erase( e.key.first );
  --edges;
  }
 /// the path from u to v in the mirror, empty if they are not joined
 [[nodiscard]] std::vector< int > path( int u , int v ) const {
  std::vector< int > from( val.size() , -2 ) , q{ u };
  from[ std::size_t( u ) ] = -1;
  for( std::size_t i = 0 ; i < q.size() ; ++i )
   for( int w : adj[ std::size_t( q[ i ] ) ] )
    if( from[ std::size_t( w ) ] == -2 ) {
     from[ std::size_t( w ) ] = q[ i ];
     q.push_back( w );
     }
  std::vector< int > p;
  if( from[ std::size_t( v ) ] == -2 )
   return( p );
  for( int x = v ; x != -1 ; x = from[ std::size_t( x ) ] )
   p.push_back( x );
  return( p );
  }
 /// the values agree (level 3 of the contracts)
 template< class T >
 [[nodiscard]] bool audit( const T & t ) const {
  if( int( val.size() ) != t.n() )
   return( false );
  for( int v = 0 ; v < t.n() ; ++v )
   if( val[ std::size_t( v ) ] != t.value( v ) )
    return( false );
  return( true );
  }
 };

/// random operations on the forest t of n nodes, checked against the
/// mirror m after each: the values, the connections and the sums of the
/// paths
template< class T >
void forestRun( T & t , ForestMirror & m , unsigned seed , int ops )
{
 std::mt19937 rng( seed );
 for( int k = 0 ; k < ops ; ++k ) {
  const int n = t.n();
  const int u = int( rng() % unsigned( n ) ) ,
    v = int( rng() % unsigned( n ) );
  const int r = int( rng() % 10 );
  if( r < 4 ) {
   if( ( u != v ) && m.path( u , v ).empty() )
    t.link( u , v );
   }
  else if( r < 6 ) {
   if( ! m.adj[ std::size_t( u ) ].empty() ) {
    const int w = *m.adj[ std::size_t( u ) ].begin();
    if( rng() % 2 )
     t.cut( u , w );
    else
     t.cut( w , u );
    }
   }
  else if( r == 6 ) {
   const bool had = ! m.adj[ std::size_t( u ) ].empty();
   const int before = m.edges;
   const bool cut = t.cutParent( u );
   CHECK( ( ! cut || had ) && ( m.edges == before - ( cut ? 1 : 0 ) ) );
   }
  else if( r == 7 )
   t.setValue( u , ( long long )( rng() % 100 ) );
  else if( r == 8 ) {
   if( n < 60 )
    t.addNode( ( long long )( rng() % 100 ) );
   }
  else
   t.evert( u );  // the rooting only: nothing told
  CHECK( m.audit( t ) );
  const auto p = m.path( u , v );
  CHECK( t.connected( u , v ) == ! p.empty() );
  if( ! p.empty() ) {
   long long s = 0;
   for( int x : p )
    s += m.val[ std::size_t( x ) ];
   CHECK( t.aggregate( u , v ) == s );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE INCUMBENTS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an observer of the incumbents of a search
template< class T >
struct Incumbents
{
 std::vector< IncumbentChanged< T > > ev;
 void on( const IncumbentChanged< T > & e ) { ev.push_back( e ); }
 /// whether the values improve strictly, from one to the next, by
 /// `better`, and the last one is \f$ x \f$
 template< class B >
 [[nodiscard]] bool chain( const T & x , B better ) const {
  for( std::size_t i = 0 ; i < ev.size() ; ++i )
   if( ! better( ev[ i ].to , ev[ i ].from ) ||
       ( ( i > 0 ) && ! ( ev[ i ].from == ev[ i - 1 ].to ) ) ||
       ( ev[ i ].h != 0 ) )
    return( false );
  return( ev.empty() || ( ev.back().to == x ) );
  }
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------------- UNION-FIND -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-unionfind" )
{
 // nobody listens: the plain forest, no storage for the subject
 struct Plain
 {
  std::vector< int > p;
  int c;
  };
 static_assert( sizeof( DisjointSets<> ) == sizeof( Plain ) );
 static_assert( std::is_same_v< UnionFind , DisjointSets<> > );
 static_assert( ! DisjointSets<>::Subject::template listened< Link > );
 // the links of a sequence, exactly
 LinkLog L;
 DisjointSets< Union::SIZE , Compression::HALVING , Static< LinkLog > > d( 5 ,
									 L );
 CHECK( d.merge( 0 , 1 ) && d.merge( 2 , 1 ) && ! d.merge( 0 , 2 ) );
 CHECK( d.merge( 3 , 4 ) && d.merge( 4 , 0 ) );
 const std::vector< Link > want{
  { 0 , -1 , 1 } , { 2 , -1 , 1 } , { 3 , -1 , 4 } , { 4 , -1 , 1 }
  };
 CHECK( ( L.links == want ) && ( L.rebuilt == 0 ) );
 d.reset( 3 );
 CHECK( ( L.rebuilt == 1 ) && ( d.count() == 3 ) );
 // a mirror of the parents from the links alone, against the structure on
 // random merges, each policy of union and of compression
 auto run = [ & ]< Union U , Compression C >( unsigned seed ) {
  using DS = DisjointSets< U , C , Dynamic<> >;
  const int n = 200;
  DS s( n );
  DisjointSets< U , C > plain( n );
  LinkMirror< DS > m( n );
  m.src = &s;
  s.attach( m );
  std::mt19937 rng( seed );
  for( int k = 0 ; k < 600 ; ++k ) {
   const int x = int( rng() % n ) , y = int( rng() % n );
   CHECK( s.merge( x , y ) == plain.merge( x , y ) );
   if( k % 7 == 0 )
    CHECK( samePartition( m , s ) && ( s.count() == plain.count() ) );
   if( k == 300 ) {
    s.reset( n );
    plain.reset( n );
    m.sync();
    }
   }
  CHECK( samePartition( m , s ) && ( m.roots == s.count() ) );
  for( int x = 0 ; x < n ; ++x )
   CHECK( s.same( x , 0 ) == plain.same( x , 0 ) );
  };
 run.template operator()< Union::SIZE , Compression::HALVING >( 1 );
 run.template operator()< Union::SIZE , Compression::FULL >( 2 );
 run.template operator()< Union::RANK , Compression::HALVING >( 3 );
 run.template operator()< Union::RANK , Compression::FULL >( 4 );
 }

ADS_TEST( "emit-unionfind-batch" )
{
 using DS = DisjointSets< Union::SIZE , Compression::HALVING , Dynamic<> >;
 const int n = 64;
 DS s( n );
 LinkMirror< DS > m( n );
 m.src = &s;
 LinkLog L;
 s.attach( m );
 s.attach( L );
 std::mt19937 rng( 5 );
 {
  auto b = batch( s );
  for( int k = 0 ; k < 80 ; ++k )
   s.merge( int( rng() % n ) , int( rng() % n ) );
  CHECK( L.links.empty() );
  }
 // each root linked once: the batch tells every link, the partition the
 // same
 CHECK( ( int( L.links.size() ) == n - s.count() ) && samePartition( m , s ) );
 // a reset in a batch: Rebuilt and the links after it
 {
  auto b = batch( s );
  s.merge( 0 , 1 );
  s.reset( n );
  s.merge( 2 , 3 );
  s.merge( 3 , 4 );
  }
 m.sync();
 CHECK( ( L.rebuilt == 1 ) && samePartition( m , s ) &&
	( s.count() == n - 2 ) );
 // a mute: Rebuilt only
 const std::size_t before = L.links.size();
 {
  auto q = quiet( s );
  s.merge( 10 , 11 );
  s.merge( 12 , 11 );
  }
 m.sync();
 CHECK( ( L.links.size() == before ) && ( L.rebuilt == 2 ) &&
	samePartition( m , s ) );
 // an observer that throws: the link undone
 struct Thrower
 {
  void on( const Link & ) { throw( 1 ); }
  };
 Thrower t;
 DisjointSets< Union::RANK , Compression::FULL , Static< Thrower > > r( 4 ,
									t );
 bool threw = false;
 try {
  r.merge( 0 , 1 );
  }
 catch( int ) {
  threw = true;
  }
 CHECK( threw && ( r.count() == 4 ) && ! r.same( 0 , 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- LINK-CUT ---------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-linkcut" )
{
 static_assert( std::is_empty_v< LCT::Subject > );
 static_assert( ! LCT::Subject::template listened< EIns > );
 static_assert( LCT::edgeOf( 3 , 5 ) == LCT::edgeOf( 5 , 3 ) );
 // the records of a sequence, exactly
 ForestLog L;
 LinkCutTree< long long , std::plus< long long > , Static< ForestLog > > t(
   4 , 0 , std::plus< long long >() , L );
 CHECK( t.addNode( 5 ) == 4 );
 t.setValue( 1 , 7 );
 t.link( 0 , 1 );
 t.link( 2 , 1 );
 t.cut( 1 , 2 );
 t.link( 3 , 0 );
 CHECK( t.aggregate( 3 , 1 ) == 7 );  // the rooting only: nothing told
 t.evert( 1 );
 CHECK( t.cutParent( 3 ) );  // 3 under 0, rooted at 1
 CHECK( ! t.cutParent( 3 ) );
 const std::vector< LEvent > want{ NIns{ 4 , 4 , 5 } ,
				   VChg{ 1 , 0 , 7 } ,
				   EIns{ LCT::edgeOf( 0 , 1 ) , { 0 , 1 } } ,
				   EIns{ LCT::edgeOf( 1 , 2 ) , { 1 , 2 } } ,
				   EEra{ LCT::edgeOf( 1 , 2 ) , { 1 , 2 } } ,
				   EIns{ LCT::edgeOf( 0 , 3 ) , { 0 , 3 } } ,
				   EEra{ LCT::edgeOf( 0 , 3 ) , { 0 , 3 } } };
 CHECK( L.ev == want );
 // a mirror from the records alone, on the vector of parents and on the
 // splay trees
 for( bool splay : { false , true } ) {
  LinkCutTree< long long , std::plus< long long > , Dynamic<> > d( 40 );
  if( splay )
   d.useSplay();
  ForestMirror m( 40 , 0 );
  d.attach( m );
  forestRun( d , m , splay ? 11 : 7 , 3000 );
  CHECK( d.splayed() == splay );
  }
 // the plain forest and the observed one give the same answers
 LCT p( 30 );
 LinkCutTree< long long , std::plus< long long > , Dynamic<> > o( 30 );
 ForestMirror m( 30 , 0 );
 o.attach( m );
 std::mt19937 rng( 3 );
 for( int k = 0 ; k < 2000 ; ++k ) {
  const int u = int( rng() % 30 ) , v = int( rng() % 30 );
  if( ( u != v ) && ! p.connected( u , v ) ) {
   p.link( u , v );
   o.link( u , v );
   }
  else if( k % 3 == 0 )
   CHECK( p.cutParent( u ) == o.cutParent( u ) );
  else {
   p.setValue( u , k );
   o.setValue( u , k );
   }
  if( p.connected( u , v ) )
   CHECK( p.aggregate( u , v ) == o.aggregate( u , v ) );
  CHECK( ( p.findRoot( u ) == o.findRoot( u ) ) && m.audit( o ) );
  }
 }

ADS_TEST( "emit-linkcut-batch" )
{
 using D = LinkCutTree< long long , std::plus< long long > , Dynamic<> >;
 D t( 8 );
 ForestMirror m( 8 , 0 );
 ForestLog L;
 t.attach( m );
 t.attach( L );
 {
  auto b = batch( t );
  t.link( 0 , 1 );
  t.cut( 1 , 0 );  // the same edge: they cancel
  t.link( 2 , 3 );
  t.setValue( 2 , 9 );
  t.setValue( 2 , 4 );  // one change, from 0 to 4
  t.link( 4 , 5 );
  t.cutParent( 4 );  // the edge of 4 and 5: they cancel
  t.addNode( 6 );
  t.setValue( 8 , 1 );  // into the insertion
  CHECK( L.ev.empty() );
  }
 const std::vector< LEvent > want{ EIns{ D::edgeOf( 2 , 3 ) , { 2 , 3 } } ,
				   VChg{ 2 , 0 , 4 } , NIns{ 8 , 8 , 1 } };
 CHECK( L.ev == want );
 CHECK( m.audit( t ) && ( m.edges == 1 ) && t.connected( 2 , 3 ) &&
	! t.connected( 0 , 1 ) && ! t.connected( 4 , 5 ) );
 // an observer that throws on a link: the edge removed again
 struct Thrower
 {
  void on( const EIns & ) { throw( 1 ); }
  };
 Thrower x;
 LinkCutTree< long long , std::plus< long long > , Static< Thrower > > r(
   3 , 0 , std::plus< long long >() , x );
 bool threw = false;
 try {
  r.link( 0 , 1 );
  }
 catch( int ) {
  threw = true;
  }
 CHECK( threw && ! r.connected( 0 , 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ BRANCH-&-BOUND ----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-bnb" )
{
 std::mt19937 rng( 17 );
 for( int k = 0 ; k < 6 ; ++k ) {
  // the circuits, symmetric, by both bounds
  const int n = 6 + k % 3;
  std::vector< std::vector< long > > D( n , std::vector< long >( n , 0 ) );
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 1 ; j < n ; ++j )
    D[ i ][ j ] = D[ j ][ i ] = 1 + long( rng() % 50 );
  for( TspBound how : { TspBound::BOOK , TspBound::ONE_TREE } ) {
   long g1 = 0 , g2 = 0;
   Incumbents< long > I;
   const auto a = bbTsp( D , {} , how , &g1 );
   const auto b = bbTsp( D , {} , how , &g2 , I );
   CHECK( ( a == b ) && ( g1 == g2 ) && ! I.ev.empty() );
   CHECK( I.chain( a.first , std::less<>() ) );
   CHECK( I.ev.front().from == std::numeric_limits< long >::max() );
   // from the optimum: nothing to improve
   Incumbents< long > J;
   const auto c = bbTsp( D , a.second , how , nullptr , J );
   CHECK( ( c.first == a.first ) && J.ev.empty() );
   }
  // the knapsack and the clique, by profits that grow
  const int m = 12;
  std::vector< long > p( m ) , v( m ) , w( m );
  for( int i = 0 ; i < m ; ++i ) {
   p[ i ] = 1 + long( rng() % 40 );
   v[ i ] = 1 + long( rng() % 20 );
   w[ i ] = long( rng() % 10 );
   }
  long g1 = 0 , g2 = 0;
  Incumbents< long > K;
  const auto a = knapsackBB( p , v , 50 , &g1 );
  const auto b = knapsackBB( p , v , 50 , &g2 , K );
  CHECK( ( a == b ) && ( g1 == g2 ) && K.chain( a.first , std::greater<>() ) );
  CHECK( ( a.first == 0 ) || ! K.ev.empty() );
  Adjacency G( m , std::vector< bool >( m , false ) );
  for( int i = 0 ; i < m ; ++i )
   for( int j = i + 1 ; j < m ; ++j )
    G[ i ][ j ] = G[ j ][ i ] = ( rng() % 2 ) != 0;
  Incumbents< long > Q;
  const auto c = maxWeightClique( G , w , &g1 );
  const auto d = maxWeightClique( G , w , &g2 , Q );
  CHECK( ( c == d ) && ( g1 == g2 ) && Q.chain( c.first , std::greater<>() ) );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------- End File engineered_emitters_forest.cpp -----------------*/
/*--------------------------------------------------------------------------*/
