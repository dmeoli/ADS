/*--------------------------------------------------------------------------*/
/*--------------------------- File bestfirst.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the structures of a best-first search, against the
 * standard library and, if found, Boost:
 *
 * - closed: the set of the nodes of a search, found by their states, on a
 *   stream of \f$ 4 n \f$ requests over \f$ n \f$ distinct states (each one
 *   generated four times, in random order), each request a
 *   `find_or_insert()` with the hash of the state already computed, the
 *   nodes made in an arena; the states have 8 or 32 bytes, and the hashes
 *   are either mixed or biased by an owner (the low 3 bits all equal, as in
 *   the table of one thread of a search that distributes the states by
 *   \f$ h \bmod p \f$). `ADS::PointerTable` (growing, and presized on huge
 *   pages) against `boost::unordered_flat_set` of the pointers, with a
 *   transparent probe that carries the hash, and `std::unordered_set`;
 * - frontier: the addressable heaps on \f$ n \f$ pushes and then \f$ 4 n \f$
 *   operations, 35% pops, 30% changes of the key of an id (up or down),
 *   15% erasures of an id and 20% pushes, on distinct keys of 64 bits, the
 *   same sequence for all: `ADS::IndexedDaryHeap` and `ADS::PairingHeap`
 *   against the mutable `boost::heap::d_ary_heap` (d = 4) and
 *   `boost::heap::pairing_heap`, and `std::set`;
 * - frontier pair: the same with a second key, the bound, changed with the
 *   key and read as the least bound after each operation:
 *   `ADS::IndexedHeapPair` against two mutable `boost::heap::d_ary_heap`
 *   and two `std::set`;
 * - ordered: an ordered frontier of \f$ n \f$ keys ( f , -g , id ), then
 *   \f$ n \f$ rounds each reading the 64 least keys in order, erasing one
 *   of them by its iterator and inserting a new key: `ADS::BTreeMap`
 *   against `std::set`;
 * - packed: \f$ 16 n \f$ fields of 1, 2 and 4 bits and of trits, read and
 *   written at random places and built by a scan of a function:
 *   `ADS::PackedArray` (plain and saturated) and `ADS::TritArray` against
 *   a byte per field (`std::vector< std::uint8_t >`), and, for 1 bit,
 *   `std::vector< bool >` and `boost::dynamic_bitset`.
 *
 * Each cell is the median of r runs, in nanoseconds per operation, and the
 * contracts are off (ADS_CONTRACTS=0); ADS_BESTFIRST_CASE set to closed,
 * frontier, ordered or packed runs only that family.
 *
 *   bestfirst [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "Bench.h"

#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/parallel/Executor.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/IndexedHeapPair.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/succinct/PackedArray.h"
#include "ADS/tree/BTreeMap.h"

#if __has_include( <boost/unordered/unordered_flat_set.hpp> )
 #include <boost/dynamic_bitset.hpp>
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #include <boost/unordered/unordered_flat_set.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <array>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <set>
#include <tuple>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using namespace ADS::bench;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CLOSED ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a node of a search with a state of W words and the hash of the state
template< int W >
struct Node {
 std::array< std::uint64_t , W > state;
 std::uint64_t hash;
 int g;
 };

/// the stream of requests: the states and their hashes
template< int W >
struct Stream {
 std::vector< std::array< std::uint64_t , W > > S;
 std::vector< std::uint64_t > H;
 };

/// 4 n requests over n distinct states, each generated four times
template< int W >
Stream< W > stream( std::size_t n , unsigned seed , bool owner )
{
 const auto K = keys( n , seed );
 std::vector< std::uint32_t > R;
 for( int t = 0 ; t < 4 ; ++t )
  for( std::size_t i = 0 ; i < n ; ++i ) R.push_back( std::uint32_t( i ) );
 std::shuffle( R.begin() , R.end() , std::mt19937_64( seed + 7 ) );
 Stream< W > s;
 for( std::uint32_t i : R ) {
  std::array< std::uint64_t , W > a;
  for( int w = 0 ; w < W ; ++w )
   a[ w ] = ADS::splitmix64( K[ i ] + std::uint64_t( w ) );
  a[ 0 ] = K[ i ];
  std::uint64_t h = ADS::splitmix64( K[ i ] );
  if( owner ) h = ( h << 3 ) | 3;
  s.S.push_back( a );
  s.H.push_back( h );
  }
 return( s );
 }

/// the hash of a node, or of a probe, given
template< int W >
struct NodeHash {
 using is_transparent = void;
 using is_avalanching = void;
 struct Probe {
  const std::array< std::uint64_t , W > * s;
  std::uint64_t h;
  };
 std::size_t operator()( const Node< W > * p ) const { return( p->hash ); }
 std::size_t operator()( const Probe & q ) const { return( q.h ); }
 };

/// the equality of the states of a node and of a probe
template< int W >
struct NodeEq {
 using is_transparent = void;
 using Probe = typename NodeHash< W >::Probe;
 bool operator()( const Node< W > * a , const Node< W > * b ) const {
  return( a->state == b->state );
  }
 bool operator()( const Probe & q , const Node< W > * b ) const {
  return( *q.s == b->state );
  }
 bool operator()( const Node< W > * a , const Probe & q ) const {
  return( a->state == *q.s );
  }
 };

/// the equality of PointerTable
template< int W >
struct StateEq {
 bool operator()( const Node< W > * p ,
                  const std::array< std::uint64_t , W > & s ) const {
  return( p->state == s );
  }
 };

/// a set and the arena of its nodes
template< class T , int W >
struct Closed {
 T t;
 std::unique_ptr< ADS::Arena< Node< W > > > a =
  std::make_unique< ADS::Arena< Node< W > > >( 1 << 16 );
 };

/// the requests of s on the closed set that make() gives, by
/// get( c , i ), which finds or inserts the request i
template< int W , class M , class G >
void closed( const char * what , std::size_t n , const std::string & who ,
             const Stream< W > & s , int r , M && make , G && get )
{
 if( skip( who ) )
  return;
 print( "bestfirst" , what , n , who , median( make , [ & ]( auto & c ) {
  std::uint64_t fresh = 0;
  for( std::size_t i = 0 ; i < s.S.size() ; ++i ) fresh += get( c , i );
  sink = sink + fresh; } , r , s.S.size() ) );
 }

template< int W >
void closedCases( std::size_t n , const Options & o , bool owner )
{
 const Stream< W > s = stream< W >( n , o.seed , owner );
 const std::string what = std::string( "closed " ) + std::to_string( 8 * W )
                          + ( owner ? "B owner" : "B" );
 const char * w = what.c_str();
 const int r = o.runs;
 using PT = ADS::PointerTable< Node< W > , StateEq< W > >;
 using PH = ADS::PointerTable< Node< W > , StateEq< W > ,
                               ADS::HugePageAllocator< std::byte > >;
 auto pt = [ & ]( auto & c , std::size_t i ) {
  return( c.t.find_or_insert( s.S[ i ] , s.H[ i ] , [ & ]( void ) {
   return( c.a->make( Node< W >{ s.S[ i ] , s.H[ i ] , 0 } ) ); } ).second );
  };
 for( const char * who : { "ADS::PointerTable" , "ADS::PointerTable A/A" } )
  closed< W >( w , n , who , s , r , [](){ return( Closed< PT , W >() ); } ,
               pt );
 closed< W >( w , n , "ADS::PointerTable presized huge" , s , r , [ n ](){
  return( Closed< PH , W >{ PH( PH::slotsFor( n ) ) } ); } , pt );
#ifdef ADS_HAS_BOOST
 using BS = boost::unordered_flat_set< Node< W > * , NodeHash< W > ,
                                       NodeEq< W > >;
 closed< W >( w , n , "boost::unordered_flat_set" , s , r ,
              [](){ return( Closed< BS , W >() ); } ,
              [ & ]( auto & c , std::size_t i ) {
  const typename NodeHash< W >::Probe q{ &s.S[ i ] , s.H[ i ] };
  if( c.t.find( q ) != c.t.end() ) return( false );
  c.t.insert( c.a->make( Node< W >{ s.S[ i ] , s.H[ i ] , 0 } ) );
  return( true ); } );
#endif
 using US = std::unordered_set< Node< W > * , NodeHash< W > , NodeEq< W > >;
 closed< W >( w , n , "std::unordered_set" , s , r ,
              [](){ return( Closed< US , W >() ); } ,
              [ & ]( auto & c , std::size_t i ) {
  const typename NodeHash< W >::Probe q{ &s.S[ i ] , s.H[ i ] };
  if( c.t.find( q ) != c.t.end() ) return( false );
  c.t.insert( c.a->make( Node< W >{ s.S[ i ] , s.H[ i ] , 0 } ) );
  return( true ); } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- FRONTIER ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// an operation on a frontier: PUSH, POP, UPDATE or ERASE of an id, with
/// its key and its bound
struct Op {
 enum Kind : std::uint8_t { PUSH , POP , UPDATE , ERASE } kind;
 int id;
 std::int64_t key , bound;
 };

/// n pushes and then 4 n operations on ids [ 0 , n ), keys and bounds
/// distinct, replayed on a set to keep the ids in the frontier
std::vector< Op > frontierOps( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed + 3 );
 const std::int64_t N = std::int64_t( n );
 auto key = [ & ]( int id ) {
  return( std::int64_t( g() % ( 64 * n ) ) * N + id ); };
 std::vector< Op > O;
 std::set< std::pair< std::int64_t , int > > R;
 std::vector< int > in , at( n , -1 ) , out;
 auto add = [ & ]( int id , std::int64_t k ) {
  R.emplace( k , id );
  at[ id ] = int( in.size() );
  in.push_back( id );
  };
 auto drop = [ & ]( int id ) {
  const int i = at[ id ];
  at[ in.back() ] = i;
  in[ i ] = in.back();
  in.pop_back();
  at[ id ] = -1;
  out.push_back( id );
  };
 std::vector< std::int64_t > K( n );
 for( int id = 0 ; id < int( n ) ; ++id ) {
  K[ id ] = key( id );
  O.push_back( { Op::PUSH , id , K[ id ] , key( id ) } );
  add( id , K[ id ] );
  }
 for( std::size_t t = 0 ; t < 4 * n ; ++t ) {
  const unsigned c = unsigned( g() % 100 );
  if( ( c < 35 ) && ! in.empty() ) {
   const int id = R.begin()->second;
   R.erase( R.begin() );
   drop( id );
   O.push_back( { Op::POP , id , 0 , 0 } );
   }
  else if( ( c < 65 ) && ! in.empty() ) {
   const int id = in[ g() % in.size() ];
   R.erase( { K[ id ] , id } );
   K[ id ] = key( id );
   R.emplace( K[ id ] , id );
   O.push_back( { Op::UPDATE , id , K[ id ] , key( id ) } );
   }
  else if( ( c < 80 ) && ! in.empty() ) {
   const int id = in[ g() % in.size() ];
   R.erase( { K[ id ] , id } );
   drop( id );
   O.push_back( { Op::ERASE , id , 0 , 0 } );
   }
  else if( ! out.empty() ) {
   const std::size_t j = g() % out.size();
   const int id = out[ j ];
   out[ j ] = out.back();
   out.pop_back();
   K[ id ] = key( id );
   add( id , K[ id ] );
   O.push_back( { Op::PUSH , id , K[ id ] , key( id ) } );
   }
  }
 return( O );
 }

/// the operations O on the frontier that make() gives, by do( q , op ),
/// which returns a number to sum
template< class M , class D >
void frontier( const char * what , std::size_t n , const std::string & who ,
               const std::vector< Op > & O , int r , M && make , D && apply )
{
 if( skip( who ) )
  return;
 print( "bestfirst" , what , n , who , median( make , [ & ]( auto & q ) {
  std::int64_t s = 0;
  for( const Op & op : O ) s += apply( q , op );
  sink = sink + std::uint64_t( s ); } , r , O.size() ) );
 }

void frontierCases( std::size_t n , const Options & o )
{
 const std::vector< Op > O = frontierOps( n , o.seed );
 const int r = o.runs;
 using KI = std::pair< std::int64_t , int >;
 // the indexed heap
 auto idx = []( auto & q , const Op & op ) -> std::int64_t {
  switch( op.kind ) {
   case Op::PUSH: q.push( op.id , op.key ); return( 0 );
   case Op::POP: return( q.pop().id );
   case Op::UPDATE: q.update( op.id , op.key ); return( 0 );
   default: q.erase( op.id ); return( 0 );
   }
  };
 for( const char * who : { "ADS::IndexedDaryHeap" ,
                           "ADS::IndexedDaryHeap A/A" } )
  frontier( "frontier" , n , who , O , r , [ n ](){
   return( ADS::IndexedDaryHeap< std::int64_t >( int( n ) ) ); } , idx );
 // the heaps with handles, kept by id
 auto handles = []( auto & c , const Op & op ) -> std::int64_t {
  auto & q = c.first;
  auto & h = c.second;
  switch( op.kind ) {
   case Op::PUSH: h[ op.id ] = q.push( KI( op.key , op.id ) ); return( 0 );
   case Op::POP: {
    const int id = q.top().second;
    q.pop();
    return( id ); }
   case Op::UPDATE: q.update( h[ op.id ] , KI( op.key , op.id ) );
    return( 0 );
   default: q.erase( h[ op.id ] ); return( 0 );
   }
  };
 frontier( "frontier" , n , "ADS::PairingHeap" , O , r , [ n ](){
  using Q = ADS::PairingHeap< int , std::int64_t >;
  return( std::pair< Q , std::vector< Q::Handle > >(
           std::piecewise_construct , std::tuple<>() , std::tuple(
            n , nullptr ) ) ); } ,
  []( auto & c , const Op & op ) -> std::int64_t {
   auto & q = c.first;
   auto & h = c.second;
   switch( op.kind ) {
    case Op::PUSH: h[ op.id ] = q.insert( op.id , op.key ); return( 0 );
    case Op::POP: return( q.deleteMin() );
    case Op::UPDATE: q.update( h[ op.id ] , op.key ); return( 0 );
    default: q.erase( h[ op.id ] ); return( 0 );
    }
   } );
#ifdef ADS_HAS_BOOST
 using Greater = boost::heap::compare< std::greater< KI > >;
 using BD = boost::heap::d_ary_heap< KI , boost::heap::arity< 4 > ,
                                     boost::heap::mutable_< true > ,
                                     Greater >;
 using BP = boost::heap::pairing_heap< KI , Greater >;
 frontier( "frontier" , n , "boost::heap::d_ary_heap d=4" , O , r , [ n ](){
  return( std::pair< BD , std::vector< BD::handle_type > >(
           std::piecewise_construct , std::tuple<>() , std::tuple( n ) ) );
  } , handles );
 frontier( "frontier" , n , "boost::heap::pairing_heap" , O , r , [ n ](){
  return( std::pair< BP , std::vector< BP::handle_type > >(
           std::piecewise_construct , std::tuple<>() , std::tuple( n ) ) );
  } , handles );
#endif
 frontier( "frontier" , n , "std::set" , O , r , [ n ](){
  return( std::pair< std::set< KI > , std::vector< std::int64_t > >(
           std::piecewise_construct , std::tuple<>() , std::tuple( n ) ) );
  } , []( auto & c , const Op & op ) -> std::int64_t {
   auto & q = c.first;
   auto & k = c.second;
   switch( op.kind ) {
    case Op::PUSH: q.emplace( k[ op.id ] = op.key , op.id ); return( 0 );
    case Op::POP: {
     const int id = q.begin()->second;
     q.erase( q.begin() );
     return( id ); }
    case Op::UPDATE:
     q.erase( KI( k[ op.id ] , op.id ) );
     q.emplace( k[ op.id ] = op.key , op.id );
     return( 0 );
    default: q.erase( KI( k[ op.id ] , op.id ) ); return( 0 );
    }
   } );

 // the pair of heaps: the least bound read after each operation
 for( const char * who : { "ADS::IndexedHeapPair" ,
                           "ADS::IndexedHeapPair A/A" } )
  frontier( "frontier pair" , n , who , O , r , [ n ](){
   return( ADS::IndexedHeapPair< std::int64_t >( int( n ) ) ); } ,
   []( auto & q , const Op & op ) -> std::int64_t {
    std::int64_t x = 0;
    switch( op.kind ) {
     case Op::PUSH: q.push( op.id , op.key , op.bound ); break;
     case Op::POP: x = q.pop(); break;
     case Op::UPDATE: q.update( op.id , op.key , op.bound ); break;
     default: q.erase( op.id );
     }
    return( x + ( q.empty() ? 0 : q.minBound() ) );
    } );
#ifdef ADS_HAS_BOOST
 struct BPair {
  BD k , b;
  std::vector< BD::handle_type > hk , hb;
  };
 frontier( "frontier pair" , n , "boost::heap::d_ary_heap d=4 x2" , O , r ,
           [ n ](){
  BPair c;
  c.hk.resize( n );
  c.hb.resize( n );
  return( c ); } , []( BPair & c , const Op & op ) -> std::int64_t {
   std::int64_t x = 0;
   switch( op.kind ) {
    case Op::PUSH:
     c.hk[ op.id ] = c.k.push( KI( op.key , op.id ) );
     c.hb[ op.id ] = c.b.push( KI( op.bound , op.id ) );
     break;
    case Op::POP:
     x = c.k.top().second;
     c.k.pop();
     c.b.erase( c.hb[ x ] );
     break;
    case Op::UPDATE:
     c.k.update( c.hk[ op.id ] , KI( op.key , op.id ) );
     c.b.update( c.hb[ op.id ] , KI( op.bound , op.id ) );
     break;
    default:
     c.k.erase( c.hk[ op.id ] );
     c.b.erase( c.hb[ op.id ] );
    }
   return( x + ( c.b.empty() ? 0 : c.b.top().first ) );
   } );
#endif
 struct SPair {
  std::set< KI > k , b;
  std::vector< std::int64_t > kk , kb;
  };
 frontier( "frontier pair" , n , "std::set x2" , O , r , [ n ](){
  SPair c;
  c.kk.resize( n );
  c.kb.resize( n );
  return( c ); } , []( SPair & c , const Op & op ) -> std::int64_t {
   std::int64_t x = 0;
   switch( op.kind ) {
    case Op::PUSH:
     c.k.emplace( c.kk[ op.id ] = op.key , op.id );
     c.b.emplace( c.kb[ op.id ] = op.bound , op.id );
     break;
    case Op::POP:
     x = c.k.begin()->second;
     c.k.erase( c.k.begin() );
     c.b.erase( KI( c.kb[ x ] , int( x ) ) );
     break;
    case Op::UPDATE:
     c.k.erase( KI( c.kk[ op.id ] , op.id ) );
     c.b.erase( KI( c.kb[ op.id ] , op.id ) );
     c.k.emplace( c.kk[ op.id ] = op.key , op.id );
     c.b.emplace( c.kb[ op.id ] = op.bound , op.id );
     break;
    default:
     c.k.erase( KI( c.kk[ op.id ] , op.id ) );
     c.b.erase( KI( c.kb[ op.id ] , op.id ) );
    }
   return( x + ( c.b.empty() ? 0 : c.b.begin()->first ) );
   } );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- ORDERED ---------------------------------*/
/*--------------------------------------------------------------------------*/

using OKey = std::tuple< std::int64_t , std::int64_t , int >;

/// the rounds on the ordered frontier that make() gives: the 64 least
/// keys read, the first whose id is a multiple of 4 (or the least) erased
/// by its iterator, a new key inserted
template< class M , class I >
void ordered( std::size_t n , const std::string & who ,
              const std::vector< OKey > & A , const std::vector< OKey > & B ,
              int r , M && make , I && ins )
{
 if( skip( who ) )
  return;
 print( "bestfirst" , "ordered" , n , who , median( [ & ](){
  auto t = make();
  for( const OKey & k : A ) ins( t , k );
  return( t ); } , [ & ]( auto & t ) {
  std::int64_t s = 0;
  for( const OKey & k : B ) {
   auto it = t.begin() , pick = it;
   for( int i = 0 ; ( i < 64 ) && ( it != t.end() ) ; ++i , ++it ) {
    const OKey & x = ( *it ).first;
    s += std::get< 0 >( x );
    if( std::get< 2 >( x ) % 4 == 0 ) {
     pick = it;
     break;
     }
    }
   t.erase( pick );
   ins( t , k );
   }
  sink = sink + std::uint64_t( s ); } , r , B.size() ) );
 }

/// a set seen as a map, its items as pairs with first
template< class K >
struct SetView : std::set< K > {
 struct It : std::set< K >::iterator {
  It( typename std::set< K >::iterator i ) : std::set< K >::iterator( i ) {}
  std::pair< const K & , char > operator*( void ) const {
   const auto & i = static_cast< const typename std::set< K >::iterator & >(
                     *this );
   return( std::pair< const K & , char >( *i , 0 ) );
   }
  };
 It begin( void ) { return( It( std::set< K >::begin() ) ); }
 It end( void ) { return( It( std::set< K >::end() ) ); }
 It erase( It i ) { return( It( std::set< K >::erase( i ) ) ); }
 };

void orderedCases( std::size_t n , const Options & o )
{
 std::mt19937_64 g( o.seed + 5 );
 std::vector< OKey > A , B;
 int id = 0;
 for( std::size_t i = 0 ; i < 2 * n ; ++i ) {
  const OKey k( std::int64_t( g() % ( n / 8 + 1 ) ) ,
                - std::int64_t( g() % 64 ) , id++ );
  ( i < n ? A : B ).push_back( k );
  }
 for( const char * who : { "ADS::BTreeMap" , "ADS::BTreeMap A/A" } )
  ordered( n , who , A , B , o.runs , [](){
   return( ADS::BTreeMap< OKey , char >() ); } ,
   []( auto & t , const OKey & k ) { t.insert( k , 0 ); } );
 ordered( n , "std::set" , A , B , o.runs , [](){
  return( SetView< OKey >() ); } ,
  []( auto & t , const OKey & k ) { t.insert( k ); } );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- PACKED ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the three cases of an array of m fields of the values below V: reads
/// at the places P, writes of the values X at the places P, and the
/// construction from f( i ), by get( a , i ), set( a , i , x ) and
/// build( m , f )
template< unsigned V , class G , class S , class B >
void packed( const std::string & what , std::size_t m ,
             const std::string & who ,
             const std::vector< std::uint32_t > & P ,
             const std::vector< std::uint8_t > & X , int r ,
             G && get , S && set , B && build )
{
 if( skip( who ) )
  return;
 // V a constant, so that the modulus is the same cheap code wherever
 // f is inlined
 auto f = []( std::size_t i ) {
  return( unsigned( ADS::splitmix64( i ) % V ) ); };
 print( "bestfirst" , ( what + " build" ).c_str() , m , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto a = build( m , f );
          sink = sink + get( a , m / 2 ); } , r , m ) );
 auto a = build( m , f );
 print( "bestfirst" , ( what + " get" ).c_str() , m , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t s = 0;
          for( std::uint32_t i : P ) s += get( a , i );
          sink = sink + s; } , r , P.size() ) );
 print( "bestfirst" , ( what + " set" ).c_str() , m , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          for( std::size_t j = 0 ; j < P.size() ; ++j )
           set( a , P[ j ] , X[ j ] );
          sink = sink + get( a , P[ 0 ] ); } , r , P.size() ) );
 }

template< unsigned Bits >
void packedBits( std::size_t m , const std::vector< std::uint32_t > & P ,
                 const std::vector< std::uint8_t > & X0 , int r )
{
 const std::string what = "packed " + std::to_string( Bits ) + "b";
 constexpr unsigned v = 1u << Bits;
 std::vector< std::uint8_t > X( X0 );
 for( auto & x : X ) x = std::uint8_t( x % v );
 auto ag = []( const auto & a , std::size_t i ) {
  return( std::uint64_t( a.get( i ) ) ); };
 auto as = []( auto & a , std::size_t i , std::uint8_t x ) {
  a.set( i , x ); };
 ADS::Sequential seq;
 for( const char * who : { "ADS::PackedArray" , "ADS::PackedArray A/A" } )
  packed< v >( what , m , who , P , X , r , ag , as ,
          [ & ]( std::size_t k , auto f ) {
           return( ADS::PackedArray< Bits >( seq , k , f ) ); } );
 packed< v >( what , m , "ADS::PackedArray saturated" , P , X , r , ag , as ,
         [ & ]( std::size_t k , auto f ) {
          return( ADS::PackedArray< Bits , true >( seq , k , f ) ); } );
 packed< v >( what , m , "std::vector<uint8_t>" , P , X , r ,
         []( const auto & a , std::size_t i ) {
          return( std::uint64_t( a[ i ] ) ); } ,
         []( auto & a , std::size_t i , std::uint8_t x ) { a[ i ] = x; } ,
         []( std::size_t k , auto f ) {
          std::vector< std::uint8_t > a( k );
          for( std::size_t i = 0 ; i < k ; ++i )
           a[ i ] = std::uint8_t( f( i ) );
          return( a ); } );
 if constexpr( Bits == 1 ) {
  packed< v >( what , m , "std::vector<bool>" , P , X , r ,
          []( const auto & a , std::size_t i ) {
           return( std::uint64_t( a[ i ] ) ); } ,
          []( auto & a , std::size_t i , std::uint8_t x ) {
           a[ i ] = x != 0; } ,
          []( std::size_t k , auto f ) {
           std::vector< bool > a( k );
           for( std::size_t i = 0 ; i < k ; ++i ) a[ i ] = f( i ) != 0;
           return( a ); } );
#ifdef ADS_HAS_BOOST
  packed< v >( what , m , "boost::dynamic_bitset" , P , X , r ,
          []( const auto & a , std::size_t i ) {
           return( std::uint64_t( a[ i ] ) ); } ,
          []( auto & a , std::size_t i , std::uint8_t x ) {
           a[ i ] = x != 0; } ,
          []( std::size_t k , auto f ) {
           boost::dynamic_bitset< std::uint64_t > a( k );
           for( std::size_t i = 0 ; i < k ; ++i ) a[ i ] = f( i ) != 0;
           return( a ); } );
#endif
  }
 }

void packedCases( std::size_t n , const Options & o )
{
 const std::size_t m = 16 * n;
 std::mt19937_64 g( o.seed + 9 );
 std::vector< std::uint32_t > P( std::size_t( 1 ) << 22 );
 std::vector< std::uint8_t > X( P.size() );
 for( std::size_t j = 0 ; j < P.size() ; ++j ) {
  P[ j ] = std::uint32_t( g() % m );
  X[ j ] = std::uint8_t( g() );
  }
 packedBits< 1 >( m , P , X , o.runs );
 packedBits< 2 >( m , P , X , o.runs );
 packedBits< 4 >( m , P , X , o.runs );
 std::vector< std::uint8_t > T( X );
 for( auto & x : T ) x = std::uint8_t( x % 3 );
 ADS::Sequential seq;
 auto ag = []( const auto & a , std::size_t i ) {
  return( std::uint64_t( a.get( i ) ) ); };
 auto as = []( auto & a , std::size_t i , std::uint8_t x ) {
  a.set( i , x ); };
 for( const char * who : { "ADS::TritArray" , "ADS::TritArray A/A" } )
  packed< 3 >( "trits" , m , who , P , T , o.runs , ag , as ,
          [ & ]( std::size_t k , auto f ) {
           return( ADS::TritArray( seq , k , f ) ); } );
 packed< 3 >( "trits" , m , "ADS::PackedArray 2b" , P , T , o.runs , ag , as ,
         [ & ]( std::size_t k , auto f ) {
          return( ADS::PackedArray< 2 >( seq , k , f ) ); } );
 packed< 3 >( "trits" , m , "std::vector<uint8_t>" , P , T , o.runs ,
         []( const auto & a , std::size_t i ) {
          return( std::uint64_t( a[ i ] ) ); } ,
         []( auto & a , std::size_t i , std::uint8_t x ) { a[ i ] = x; } ,
         []( std::size_t k , auto f ) {
          std::vector< std::uint8_t > a( k );
          for( std::size_t i = 0 ; i < k ; ++i )
           a[ i ] = std::uint8_t( f( i ) );
          return( a ); } );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 , 10000000 } );
 const char * only = std::getenv( "ADS_BESTFIRST_CASE" );
 auto on = [ only ]( const char * c ) {
  return( ! only || ! *only || ! std::strcmp( only , c ) ); };
 for( std::size_t n : o.sizes ) {
  if( on( "closed" ) )
   for( bool owner : { false , true } ) {
    closedCases< 1 >( n , o , owner );
    closedCases< 4 >( n , o , owner );
    }
  if( on( "frontier" ) ) frontierCases( n , o );
  if( on( "ordered" ) ) orderedCases( n , o );
  if( on( "packed" ) ) packedCases( n , o );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File bestfirst.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
