/*--------------------------------------------------------------------------*/
/*---------------------------- File zerocost.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The program that zerocost.py runs under callgrind: n changes of one kind
 * on one variant of a structure [see zerocost.h], after a preparation of a
 * fixed size, so that the difference of the instructions of two runs of
 * n and 2 n changes, divided by n, is the count of one change:
 *
 *   zerocost <workload> <variant> <n>
 *
 * with the workloads array-weight (`setWeight()` of ArrayGraph), list-weight
 * (`insertEdge()` of an edge of ListGraph that is there), list-edge
 * (`insertEdge()` of a new edge of ListGraph and its `deleteEdge()`), key
 * (the array of keys of zerocost.h), grid-cost (`setCost()` of GridGraph),
 * arena-make (`make()` of Arena) and pool-make (`make()` of ObjectPool,
 * each second object freed), the last two with the variants plain (no
 * `clear()`) and epochs (a `clear()` per 4096 objects), and the variants
 * plain,
 * observable (`Observable< G >`, nobody listening), observable-excl
 * (`Observable< G , O >`, O wanting only the insertions of nodes), and,
 * with Modification.h, none, static-excl, static-count and dynamic [see
 * zerocost.h]. It prints a checksum, so that no change is optimized away.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

#include "zerocost.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

using namespace zc;

namespace {

constexpr int N = 512;                 ///< the nodes of the graphs
constexpr std::size_t OPS = 4096;      ///< the changes, used in a cycle

/// a change: an edge ( u , v ) and a weight, or a place u and a key w
struct Op {
 int u , v;
 long long w;
 };

/// the next number of a linear congruential generator
unsigned next( unsigned & x )
{
 x = x * 1664525u + 1013904223u;
 return( x >> 8 );
 }

/// the edges of the graphs: each node to the next 1, 7 and 31
std::vector< std::tuple< int , int , long long > > edges( void )
{
 std::vector< std::tuple< int , int , long long > > E;
 for( int u = 0 ; u < N ; ++u )
  for( int d : { 1 , 7 , 31 } )
   E.emplace_back( u , ( u + d ) % N , 100 );
 return( E );
 }

/// changes of the weights of edges that are there
std::vector< Op > weightOps( void )
{
 std::vector< Op > P;
 unsigned x = 12345;
 static const int D[ 3 ] = { 1 , 7 , 31 };
 for( std::size_t i = 0 ; i < OPS ; ++i ) {
  const int u = int( next( x ) % N );
  P.push_back( { u , ( u + D[ next( x ) % 3 ] ) % N ,
                 (long long)( next( x ) % 1000 ) } );
  }
 return( P );
 }

/// insertions of edges that are not there (each node to the next 100)
std::vector< Op > edgeOps( void )
{
 std::vector< Op > P;
 unsigned x = 54321;
 for( std::size_t i = 0 ; i < OPS ; ++i ) {
  const int u = int( next( x ) % N );
  P.push_back( { u , ( u + 100 ) % N , (long long)( next( x ) % 1000 ) } );
  }
 return( P );
 }

/// n changes f( op ), the ops in a cycle
template< class F >
[[gnu::noinline]] void loop( F && f , const std::vector< Op > & P ,
                             long long n )
{
 for( long long i = 0 ; i < n ; ++i )
  f( P[ std::size_t( i ) & ( OPS - 1 ) ] );
 }

/// the sum of the weights of g
template< class G >
long long weights( const G & g )
{
 long long s = 0;
 for( int u = 0 ; u < N ; ++u )
  g.forEachEdge( u , [ & ]( int , long long w ) { s += w; } );
 return( s );
 }

/// the graph of N nodes and the edges of edges()
template< class G >
G build( void )
{
 if constexpr( ADS::DynamicGraph< G > ) {
  G g( N , true );
  for( const auto & [ u , v , w ] : edges() )
   g.insertEdge( u , v , w );
  return( g );
  }
 else
  return( G( N , edges() , true ) );
 }

/// the workloads of the graph G on the wrapper made by make( g )
template< class G , class Make >
long long graphCase( const std::string & work , Make && make , long long n )
{
 G g = build< G >();
 auto && og = make( g );
 if( work == "array-weight" || work == "list-weight" ) {
  const auto P = weightOps();
  if constexpr( requires { og.setWeight( 0 , 1 , 1LL ); } ) {
   if( work == "array-weight" ) {
    loop( [ & ]( const Op & o ) { og.setWeight( o.u , o.v , o.w ); } ,
          P , n );
    return( weights( g ) );
    }
   }
  if constexpr( requires { og.insertEdge( 0 , 1 , 1LL ); } ) {
   loop( [ & ]( const Op & o ) { og.insertEdge( o.u , o.v , o.w ); } ,
         P , n );
   return( weights( g ) );
   }
  }
 if constexpr( requires { og.deleteEdge( 0 , 1 ); } ) {
  if( work == "list-edge" ) {
   const auto P = edgeOps();
   loop( [ & ]( const Op & o ) {
    og.insertEdge( o.u , o.v , o.w );
    og.deleteEdge( o.u , o.v ); } , P , n );
   return( weights( g ) );
   }
  }
 std::fprintf( stderr , "no workload %s here\n" , work.c_str() );
 std::exit( 2 );
 }

/// the plain graph, for which ListGraph has no setWeight(): the plain
/// counterpart of `Observable::setWeight()` reads the weight and replaces
/// it, as that does
template< class G >
struct Plain {
 G & g;
 bool setWeight( int u , int v , long long w ) {
  if constexpr( requires { g.setWeight( u , v , w ); } )
   return( g.setWeight( u , v , w ) );
  else {
   if( ! g.weight( u , v ) )
    return( false );
   g.insertEdge( u , v , w );
   return( true );
   }
  }
 void insertEdge( int u , int v , long long w )
  requires ADS::DynamicGraph< G > { g.insertEdge( u , v , w ); }
 void deleteEdge( int u , int v )
  requires ADS::DynamicGraph< G > { g.deleteEdge( u , v ); }
 };

/// the workloads of the graph G on the variant var
template< class G >
long long graph( const std::string & work , const std::string & var ,
                 long long n )
{
 if( var == "plain" )
  return( graphCase< G >( work , []( G & g ) { return( Plain< G >{ g } ); } ,
                          n ) );
 if( var == "observable" )
  return( graphCase< G >( work , []( G & g ) {
                           return( ADS::Observable< G >( g ) ); } , n ) );
 if( var == "observable-excl" ) {
  static LegacyExcluding x;
  return( graphCase< G >( work , []( G & g ) {
                           return( ADS::Observable< G , LegacyExcluding >(
                                    g , x ) ); } , n ) );
  }
#ifdef ZC_POLICIES
 if( var == "none" )
  return( graphCase< G >( work , []( G & g ) {
                           return( ADS::ObservedGraph< G >( g ) ); } , n ) );
 if( var == "static-excl" ) {
  static Excluding x;
  return( graphCase< G >( work , []( G & g ) {
                           return( ADS::ObservedGraph< G ,
                                    ADS::Static< Excluding > >( g , x ) ); } ,
                          n ) );
  }
 if( var == "static-count" ) {
  static CountDecreases x;
  return( graphCase< G >( work , []( G & g ) {
                           return( ADS::ObservedGraph< G ,
                                    ADS::Static< CountDecreases > >( g ,
                                                                     x ) ); } ,
                          n ) + x.n );
  }
 if( var == "dynamic" )
  return( graphCase< G >( work , []( G & g ) {
                           return( ADS::ObservedGraph< G , ADS::Dynamic<> >(
                                    g ) ); } , n ) );
#endif
 std::fprintf( stderr , "no variant %s\n" , var.c_str() );
 std::exit( 2 );
 }

#ifdef ZC_POLICIES

/// the workload key on the variant var
long long keys( const std::string & var , long long n )
{
 const auto P = weightOps();
 auto run = [ & ]( auto & a ) {
  loop( [ & ]( const Op & o ) { a.set( std::size_t( o.u ) , int( o.w ) ); } ,
        P , n );
  long long s = 0;
  for( int u = 0 ; u < N ; ++u )
   s += a.get( std::size_t( u ) );
  return( s );
  };
 if( var == "plain" ) {
  struct { std::vector< int > k = std::vector< int >( N );
           void set( std::size_t h , int x ) { k[ h ] = x; }
           int get( std::size_t h ) const { return( k[ h ] ); } } a;
  return( run( a ) );
  }
 if( var == "none" ) {
  KeyArray< ADS::NoObservers > a( N );
  return( run( a ) );
  }
 if( var == "static-excl" ) {
  KeyExcluding x;
  KeyArray< ADS::Static< KeyExcluding > > a( N , x );
  return( run( a ) + x.n );
  }
 if( var == "static-count" ) {
  KeyCountDecreases x;
  KeyArray< ADS::Static< KeyCountDecreases > > a( N , x );
  return( run( a ) + x.n );
  }
 if( var == "dynamic" ) {
  KeyArray< ADS::Dynamic<> > a( N );
  return( run( a ) );
  }
 std::fprintf( stderr , "no variant %s\n" , var.c_str() );
 std::exit( 2 );
 }

/// the workload grid-cost on the variant var: the cells of a grid of
/// N x 8, set to costs from 0 to 9
long long grid( const std::string & var , long long n )
{
 const auto P = weightOps();
 auto run = [ & ]( auto & g ) {
  loop( [ & ]( const Op & o ) {
   g.setCost( o.u * 8 + int( o.w & 7 ) , int( o.w % 10 ) ); } , P , n );
  return( (long long)( g.n() ) + g.minCost() );
  };
 const std::vector< int > cost( std::size_t( N ) * 8 , 1 );
 if( ( var == "plain" ) || ( var == "none" ) ) {
  Grid< ADS::NoObservers > g( N , 8 , cost );
  return( run( g ) );
  }
 if( var == "static-count" ) {
  GridCountDecreases x;
  Grid< ADS::Static< GridCountDecreases > > g( N , 8 , cost ,
                                               ADS::GridMoves::FOUR ,
                                               ADS::Corners::NO_CUT , 1 ,
                                               std::nullopt ,
                                               ADS::MoveCost::ENTERED , x );
  return( run( g ) + x.n );
  }
 if( var == "dynamic" ) {
  Grid< ADS::Dynamic<> > g( N , 8 , cost );
  return( run( g ) );
  }
 std::fprintf( stderr , "no variant %s\n" , var.c_str() );
 std::exit( 2 );
 }

#endif

/// the workloads arena-make and pool-make on the variant var (plain or
/// epochs): n objects made, each second one freed in the pool, and with
/// epochs a clear() per OPS objects
long long allocation( const std::string & work , const std::string & var ,
                      long long n )
{
 const bool epochs = var == "epochs";
 if( ! epochs && ( var != "plain" ) ) {
  std::fprintf( stderr , "no variant %s\n" , var.c_str() );
  std::exit( 2 );
  }
 long long s = 0;
 if( work == "arena-make" ) {
  ADS::Arena< Op > a;
  loop( [ & ]( const Op & o ) {
   s += a.make( o )->u;
   if( epochs && ( a.size() == OPS ) )
    a.clear(); } , weightOps() , n );
  return( s + (long long)( a.size() ) );
  }
 ADS::ObjectPool< Op > p;
 Op * last = nullptr;
 std::size_t made = 0;
 loop( [ & ]( const Op & o ) {
  Op * x = p.make( o );
  s += x->u;
  if( ( ++made & 1 ) == 0 )
   p.free( std::exchange( last , nullptr ) );
  else
   last = x;
  if( epochs && ( made % OPS == 0 ) ) {
   p.clear();
   last = nullptr;
   } } , weightOps() , n );
 return( s + (long long)( p.size() ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 if( argc != 4 ) {
  std::fprintf( stderr , "zerocost <workload> <variant> <n>\n" );
  return( 2 );
  }
 const std::string work = argv[ 1 ] , var = argv[ 2 ];
 const long long n = std::atoll( argv[ 3 ] );
 long long s = 0;
 if( work == "array-weight" )
  s = graph< AG >( work , var , n );
 else if( ( work == "list-weight" ) || ( work == "list-edge" ) )
  s = graph< LG >( work , var , n );
#ifdef ZC_POLICIES
 else if( work == "key" )
  s = keys( var , n );
 else if( work == "grid-cost" )
  s = grid( var , n );
#endif
 else if( ( work == "arena-make" ) || ( work == "pool-make" ) )
  s = allocation( work , var , n );
 else {
  std::fprintf( stderr , "no workload %s\n" , work.c_str() );
  return( 2 );
  }
 std::printf( "%lld\n" , s );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File zerocost.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
