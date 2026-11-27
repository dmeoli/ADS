/*--------------------------------------------------------------------------*/
/*------------------------- File modifications.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the typed modifications themselves [see Modification.h]:
 * the time of one change of a structure that tells its changes, by the
 * subject policy and by the observers attached, against the change of the
 * plain structure, so that the cost of the events is known both when nobody
 * listens and when somebody does. Four structures, each on n items and n
 * changes of random handles ("random") or of handles among the first 64
 * ("hot", whose changes a batch coalesces into at most 64 events):
 *
 * - modifications graph, setWeight() of a random edge of G( n , 4 n ),
 *   oriented, to a random weight, on the ListGraph alone ("plain", the code
 *   that the wrapper runs when nobody listens) and on ObservedGraph;
 * - modifications array, set() of a random index of ObservableArray, against
 *   the store into a std::vector ("plain");
 * - modifications heap, decrease() of the key of a random id of the
 *   IndexedDaryHeap of n ids, by a random amount;
 * - modifications dictionary, the removal of a random key of the
 *   DenseHashMap of n keys alternated with the insertion of a new one, on the
 *   case "insert-erase";
 *
 * each on the variants "NoObservers" (the default policy, which must take
 * the time of the plain structure, a check of the claim that a change
 * nobody listens to costs nothing), "Static 1" (one observer called
 * directly), "Dynamic 0", "Dynamic 1" and "Dynamic 4" (none, one and four
 * observers attached at run time), "Dynamic 1 batch 16" and "Dynamic 1
 * batch 256" (the changes in batches of 16 and of 256, each sent coalesced
 * at its close) and "Dynamic 1 quiet" (all the changes under one mute,
 * which ends with Rebuilt, or with nothing for the graph, whose events have
 * no Rebuilt); the observers add up what they are told. The cells are in
 * nanoseconds per change, the building of the structure and of its items
 * not timed. The heap and the dictionary have no plain structure other than
 * their own code with NoObservers. Each structure runs only if
 * ADS_BENCH_WHO names it, or with no such variable [see Bench.h]; the first
 * structure of each cell is run twice, the second time as "A/A". The
 * contracts are off.
 *
 *   modifications [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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

#include "ADS/Modification.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/sequence/ObservableArray.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace ADS;
using ADS::bench::Options;

namespace
{

using LG = ListGraph< long long >;
using Key = std::uint64_t;

/*--------------------------------------------------------------------------*/
/*------------------------------ THE OBSERVERS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an observer of the weights of a graph, which adds them up
struct GraphSum
{
 std::uint64_t h = 0;
 void on( const WeightChanged< long long > & e ) {
  h += std::uint64_t( e.to );
  }
 };

/// an observer of the values of an array, which adds them up
struct ArraySum
{
 std::uint64_t h = 0;
 void on( const mod::Assigned< std::size_t , long long > & e ) {
  h += std::uint64_t( e.to );
  }
 void on( const mod::Rebuilt & ) noexcept { ++h; }
 };

/// an observer of the keys of a heap, which adds them up
struct HeapSum
{
 std::uint64_t h = 0;
 void on( const mod::KeyChanged< int , long long > & e ) {
  h += std::uint64_t( e.to );
  }
 void on( const mod::Rebuilt & ) noexcept { ++h; }
 };

/// an observer of the keys of a dictionary, which adds them up
struct DictSum
{
 std::uint64_t h = 0;
 void on( const mod::Inserted< Key , Key , Key > & e ) { h += e.key; }
 void on( const mod::Erased< Key , Key , Key > & e ) noexcept { h -= e.key; }
 void on( const mod::Rebuilt & ) noexcept { ++h; }
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ THE VARIANTS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a variant: the policy, the observers attached, the batches or the mute
enum class Var { NONE , STATIC , DYN0 , DYN1 , DYN4 , BATCH16 , BATCH256 ,
		 QUIET };

/// the variants, by their labels, in the order of the cells
const std::vector< std::pair< Var , std::string > > VARS = {
 { Var::NONE , "NoObservers" } ,
 { Var::STATIC , "Static 1" } ,
 { Var::DYN0 , "Dynamic 0" } ,
 { Var::DYN1 , "Dynamic 1" } ,
 { Var::DYN4 , "Dynamic 4" } ,
 { Var::BATCH16 , "Dynamic 1 batch 16" } ,
 { Var::BATCH256 , "Dynamic 1 batch 256" } ,
 { Var::QUIET , "Dynamic 1 quiet" }
 };

/// the observers attached at run time in the variant v
int attachedIn( Var v )
{
 return(
   v == Var::DYN4 ? 4
   : ( ( v == Var::NONE ) || ( v == Var::STATIC ) || ( v == Var::DYN0 ) )
     ? 0
     : 1 );
 }

/// whether P is a static policy
template< class P >
inline constexpr bool isStatic = false;

template< class... O >
inline constexpr bool isStatic< Static< O... > > = true;

/// f( std::type_identity< P >() ), P the policy of the variant v, the
/// observer O for the static one
template< class O , class F >
double byPolicy( Var v , F && f )
{
 if( v == Var::NONE )
  return( f( std::type_identity< NoObservers >() ) );
 if( v == Var::STATIC )
  return( f( std::type_identity< Static< O > >() ) );
 return( f( std::type_identity< Dynamic<> >() ) );
 }

/// a structure S, the observers O that it may tell and what it needs to
/// live (the graph that a wrapper changes): the first observer given to a
/// static policy, the first k attached to a dynamic one; the observers come
/// before the structure, so that they outlive it
template< class S , class O , class B = std::monostate >
struct Held
{
 B base;                  ///< what the structure needs
 std::array< O , 4 > o{};  ///< the observers
 S s;                     ///< the structure

 template< class Make >
 Held( B b , Make && make , int k )
     : base( std::move( b ) ) , s( make( base , o[ 0 ] ) ) {
  if constexpr( S::Subject::dynamic )
   for( int i = 0 ; i < k ; ++i )
    s.attach( o[ std::size_t( i ) ] );
  }

 /// what the observers were told
 [[nodiscard]] std::uint64_t sum( void ) const {
  std::uint64_t x = 0;
  for( const auto & y : o )
   x += y.h;
  return( x );
  }
 };

/// a mute of \f$ s \f$, with Rebuilt at its end when \f$ s \f$ has it
template< class S >
auto mute( S & s )
{
 if constexpr( mod::contains_v< mod::Rebuilt , typename S::modifications > )
  return( ADS::quiet( s ) );
 else
  return( ADS::quiet( s , ADS::no_rebuild ) );
 }

/// the time of the changes mutate( s , j ), j in [ 0 , ops ), of the
/// structure that prepare() gives, on the variant v: in batches, under a
/// mute or one at a time, in nanoseconds per change
template< class P , class M >
double pass( P && prepare , M && mutate , Var v , int r , std::size_t ops )
{
 return( ADS::bench::median(
   prepare ,
   [ & ]( auto & h ) {
    auto & s = h->s;
    using S = std::remove_reference_t< decltype( s ) >;
    if constexpr( S::Subject::dynamic ) {
     if( ( v == Var::BATCH16 ) || ( v == Var::BATCH256 ) ) {
      const std::size_t b = v == Var::BATCH16 ? 16 : 256;
      for( std::size_t i = 0 ; i < ops ; i += b ) {
       auto B = ADS::batch( s );
       for( std::size_t j = i ; j < std::min( ops , i + b ) ; ++j )
	mutate( s , j );
       }
      ADS::bench::sink = h->sum();
      return;
      }
     if( v == Var::QUIET ) {
      {
       auto q = mute( s );
       for( std::size_t j = 0 ; j < ops ; ++j )
	mutate( s , j );
       }
      ADS::bench::sink = h->sum();
      return;
      }
     }
    for( std::size_t j = 0 ; j < ops ; ++j )
     mutate( s , j );
    ADS::bench::sink = h->sum();
    } ,
   r , ops ) );
 }

/// the cells of a structure on a case: plain() the plain structure first,
/// if there is one, then run( v ) on each variant v; the first one twice,
/// the second time as "A/A"
template< class Plain , class Run >
void cells( const char * bench , const char * c , std::size_t n ,
	    bool hasPlain ,
	    Plain && plain , Run && run )
{
 std::vector< std::pair< int , std::string > > L;  // -1 the plain one
 if( hasPlain )
  L.emplace_back( -1 , "plain" );
 for( std::size_t i = 0 ; i < VARS.size() ; ++i )
  L.emplace_back( int( i ) , VARS[ i ].second );
 for( std::size_t i = 0 ; i < L.size() ; ++i )
  for( int twice = 0 ; twice < ( i == 0 ? 2 : 1 ) ; ++twice ) {
   const std::string who = L[ i ].second + ( twice ? " A/A" : "" );
   if( ADS::bench::skip( who ) )
    continue;
   const double ns = L[ i ].first < 0
		       ? plain()
		       : run( VARS[ std::size_t( L[ i ].first ) ].first );
   ADS::bench::print( bench , c , n , who , ns );
   }
 }

/// n handles in [ 0 , range ), at random, or among the first 64 if hot
std::vector< std::size_t > handles( std::size_t n , std::size_t range ,
				    bool hot ,
				    unsigned seed )
{
 std::mt19937_64 g( seed );
 const std::size_t w = hot ? std::min< std::size_t >( range , 64 ) : range;
 std::vector< std::size_t > H( n );
 for( auto & h : H )
  h = std::size_t( g() % w );
 return( H );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE STRUCTURES ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the edges ( u , v , w ) of G( n , 4 n ), oriented, no loop, no
/// repetition
std::vector< std::tuple< int , int , long long > > gnm( int n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::uint64_t > seen;
 std::vector< std::tuple< int , int , long long > > E;
 while( int( E.size() ) < 4 * n ) {
  const int u = int( g() % std::uint64_t( n ) ) ,
	    v = int( g() % std::uint64_t( n ) );
  if( ( u == v ) ||
      ! seen.insert( std::uint64_t( u ) << 32 | std::uint64_t( v ) ).second )
   continue;
  E.emplace_back( u , v , 1 + static_cast< long long >( g() % 1000 ) );
  }
 return( E );
 }

/// the cells of the graph: setWeight() of n random edges
void graph( std::size_t n , int r , unsigned seed )
{
 const auto E = gnm( int( n ) , seed );
 LG g0( int( n ) , true );
 for( const auto & [ u , v , w ] : E )
  g0.insertEdge( u , v , w );
 std::mt19937_64 g( seed + 1 );
 std::vector< long long > X( n );
 for( auto & x : X )
  x = 1 + static_cast< long long >( g() % 1000 );
 for( const bool hot : { false , true } ) {
  const auto H = handles( n , E.size() , hot , seed + 2 );
  auto mutate = [ & ]( auto & s , std::size_t j ) {
   const auto & [ u , v , w0 ] = E[ H[ j ] ];
   ( void )w0;
   ( void )s.setWeight( u , v , X[ j ] );
   };
  cells(
    "modifications graph" , hot ? "hot" : "random" , n , true ,
    [ & ]( void ) {
     return( ADS::bench::median(
       [ & ]( void ) { return( std::make_unique< LG >( g0 ) ); } ,
       [ & ]( auto & p ) {
	for( std::size_t j = 0 ; j < n ; ++j ) {
	 // the code of ObservedGraph::setWeight() when nobody listens
	 const auto & [ u , v , w0 ] = E[ H[ j ] ];
	 ( void )w0;
	 if( p->weight( u , v ) )
	  p->insertEdge( u , v , X[ j ] );
	 }
	ADS::bench::sink = std::uint64_t( p->n() );
        } ,
       r , n ) );
     } ,
    [ & ]( Var v ) {
     return( byPolicy< GraphSum >( v , [ & ]( auto t ) {
      using P = typename decltype( t )::type;
      using S = ObservedGraph< LG , P >;
      using Hd = Held< S , GraphSum , LG >;
      return( pass(
	[ & ]( void ) {
	 return( std::make_unique< Hd >(
	   g0 ,
	   []( LG & b , GraphSum & o ) {
	    if constexpr( isStatic< P > )
	     return( S( b , o ) );
	    else {
	     ( void )o;
	     return( S( b ) );
	     }
	    } ,
	   attachedIn( v ) ) );
	 } ,
	mutate , v , r , n ) );
      } ) );
     } );
  }
 }

/// the cells of the array: set() of n random indices
void array( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed + 3 );
 std::vector< long long > X( n );
 for( auto & x : X )
  x = static_cast< long long >( g() % 1000000 );
 for( const bool hot : { false , true } ) {
  const auto H = handles( n , n , hot , seed + 4 );
  auto mutate = [ & ]( auto & s ,
		       std::size_t j ) { s.set( H[ j ] , X[ j ] ); };
  cells(
    "modifications array" , hot ? "hot" : "random" , n , true ,
    [ & ]( void ) {
     return( ADS::bench::median(
       [ & ]( void ) {
	return( std::make_unique< std::vector< long long > >( n , 0 ) );
        } ,
       [ & ]( auto & p ) {
	auto & a = *p;
	for( std::size_t j = 0 ; j < n ; ++j )
	 a[ H[ j ] ] = X[ j ];
	ADS::bench::sink = std::uint64_t( a[ H[ 0 ] ] );
        } ,
       r , n ) );
     } ,
    [ & ]( Var v ) {
     return( byPolicy< ArraySum >( v , [ & ]( auto t ) {
      using P = typename decltype( t )::type;
      using S = ObservableArray< long long , P >;
      using Hd = Held< S , ArraySum >;
      return( pass(
	[ & ]( void ) {
	 return( std::make_unique< Hd >(
	   std::monostate() ,
	   [ n ]( std::monostate & , ArraySum & o ) {
	    if constexpr( isStatic< P > )
	     return( S( n , 0 , o ) );
	    else {
	     ( void )o;
	     return( S( n , 0 ) );
	     }
	    } ,
	   attachedIn( v ) ) );
	 } ,
	mutate , v , r , n ) );
      } ) );
     } );
  }
 }

/// the cells of the heap: decrease() of n random ids
void heap( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed + 5 );
 std::vector< long long > K0( n );
 for( auto & k : K0 )
  k = ( 1LL << 40 ) + static_cast< long long >( g() % ( 1u << 20 ) );
 for( const bool hot : { false , true } ) {
  const auto H = handles( n , n , hot , seed + 6 );
  // the new keys, each below the key of its id at that point
  std::vector< long long > X( n ) , cur( K0 );
  for( std::size_t j = 0 ; j < n ; ++j )
   X[ j ] = cur[ H[ j ] ] -= 1 + static_cast< long long >( g() % 1000 );
  auto mutate = [ & ]( auto & s , std::size_t j ) {
   s.decrease( int( H[ j ] ) , X[ j ] );
   };
  cells(
    "modifications heap" , hot ? "hot" : "random" , n , false ,
    []( void ) { return( 0.0 ); } ,
    [ & ]( Var v ) {
     return( byPolicy< HeapSum >( v , [ & ]( auto t ) {
      using P = typename decltype( t )::type;
      using S = IndexedDaryHeap< long long , 4 , P >;
      using Hd = Held< S , HeapSum >;
      return( pass(
	[ & ]( void ) {
	 auto h = std::make_unique< Hd >(
	   std::monostate() ,
	   [ n ]( std::monostate & , HeapSum & o ) {
	    if constexpr( isStatic< P > )
	     return( S( int( n ) , o ) );
	    else {
	     ( void )o;
	     return( S( int( n ) ) );
	     }
	    } ,
	   attachedIn( v ) );
	 for( std::size_t i = 0 ; i < n ; ++i )
	  h->s.push( int( i ) , K0[ i ] );
	 return( h );
	 } ,
	mutate , v , r , n ) );
      } ) );
     } );
  }
 }

/// the dictionary of the policy P
template< class P >
using Dict = DenseHashMap< Key , Key , DefaultHash< Key > ,
			   std::equal_to< Key > ,
			   std::allocator< std::pair< const Key , Key > > ,
			   P >;

/// the cells of the dictionary: n / 2 removals of random keys alternated
/// with n / 2 insertions of new ones
void dictionary( std::size_t n , int r , unsigned seed )
{
 const auto K = ADS::bench::keys( n + n / 2 , seed + 7 );
 // the keys removed, a random half of the first n
 std::vector< Key > out( K.begin() , K.begin() + std::ptrdiff_t( n ) );
 std::shuffle( out.begin() , out.end() , std::mt19937_64( seed + 8 ) );
 auto mutate = [ & ]( auto & s , std::size_t j ) {
  if( j % 2 )
   ( void )s.insert( K[ n + j / 2 ] , Key( j ) );
  else
   ( void )s.remove( out[ j / 2 ] );
  };
 cells(
   "modifications dictionary" , "insert-erase" , n , false ,
   []( void ) { return( 0.0 ); } ,
   [ & ]( Var v ) {
    return( byPolicy< DictSum >( v , [ & ]( auto t ) {
     using P = typename decltype( t )::type;
     using S = Dict< P >;
     using Hd = Held< S , DictSum >;
     return( pass(
       [ & ]( void ) {
	auto h = std::make_unique< Hd >(
	  std::monostate() ,
	  [ n ]( std::monostate & , DictSum & o ) {
	   if constexpr( isStatic< P > )
	    return( S( n , DefaultHash< Key >() , std::equal_to< Key >() ,
			std::allocator< std::pair< const Key , Key > >() ,
			o ) );
	   else {
	    ( void )o;
	    return( S( n ) );
	    }
	   } ,
	  attachedIn( v ) );
	for( std::size_t i = 0 ; i < n ; ++i )
	 ( void )h->s.insert( K[ i ] , Key( i ) );
	return( h );
        } ,
       mutate , v , r , n ) );
     } ) );
    } );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  graph( n , o.runs , o.seed );
  array( n , o.runs , o.seed );
  heap( n , o.runs , o.seed );
  dictionary( n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File modifications.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
