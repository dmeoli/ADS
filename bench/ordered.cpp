/*--------------------------------------------------------------------------*/
/*--------------------------- File ordered.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the ordered dictionaries: BTreeMap and the search trees
 * of ADS against std::map and, where they are found, absl::btree_map,
 * tlx::btree_map, boost::container::flat_map and the intrusive trees of
 * Boost (the Red-Black tree of boost::intrusive::set, avl_set, sg_set,
 * splay_set and treap_set, whose nodes are allocated beforehand and only
 * linked in the clock), on n distinct random keys of 64 bits with values
 * of 64 bits. The cases are the n insertions in random order into an empty
 * structure, the n insertions in increasing order, n lookups of present
 * keys in random order, n lookups of absent keys, n searches of the first
 * key not smaller than an absent one (lower bound), the removal of all the
 * keys in random order, a visit of all the pairs by the iterators, and n /
 * 100 scans of 100 pairs from the lower bound of a random key. The sorted
 * vectors (the flat map and SortedArrayDictionary) are built from the
 * sorted keys and enter only the searches and the visits; a structure
 * without a case is left out of it.
 *
 * Each cell is the median of r runs, in nanoseconds per operation (per
 * pair for the visit and the scans); the searches run on one structure
 * built once, the removals on a structure built before each run, and the
 * contracts are off (ADS_CONTRACTS=0).
 *
 *   ordered [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/BTreeMap.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/Treap.h"

#if __has_include( <boost/intrusive/set.hpp> )
 #include <boost/container/flat_map.hpp>
 #include <boost/intrusive/avl_set.hpp>
 #include <boost/intrusive/set.hpp>
 #include <boost/intrusive/sg_set.hpp>
 #include <boost/intrusive/splay_set.hpp>
 #include <boost/intrusive/treap_set.hpp>
 #define ADS_HAS_BOOST 1
#endif

#if __has_include( <absl/container/btree_map.h> )
 #include <absl/container/btree_map.h>
 #define ADS_HAS_ABSL 1
#endif

#if __has_include( <tlx/container/btree_map.hpp> )
 #include <tlx/container/btree_map.hpp>
 #define ADS_HAS_TLX 1
#endif

#include <map>
#include <memory>
#include <optional>
#include <type_traits>

using namespace ADS::bench;
using Key = std::uint64_t;

namespace {

/// a missing operation of a structure
struct None {};

/// whether to run the structure who: all, or those whose name has in it
/// one of the texts separated by | in the variable ONLY
bool wanted( const std::string & who )
{
 const char * o = std::getenv( "ONLY" );
 if( ! o || ! *o )
  return( true );
 const std::string s = o;
 for( std::size_t a = 0 , b ; a <= s.size() ; a = b + 1 ) {
  b = std::min( s.find( '|' , a ) , s.size() );
  if( ( b > a ) && ( who.find( s.substr( a , b - a ) ) !=
                     std::string::npos ) )
   return( true );
  }
 return( false );
 }

/// the keys of a run: K in the order of insertion, H the same in another
/// order, Q absent ones, S sorted
struct Data {
 std::vector< Key > K , H , Q , S;
 };

/// the cases on the structure that make() builds empty, by the operations
/// ins( d , k ), find( d , k ) (the value, or 0), low( d , k ) (the key of
/// the lower bound, or 0), del( d , k ), sum( d ) (the sum of the values by
/// the iterators) and scan( d , k , c ) (the sum of the values of the c
/// pairs from the lower bound of k); a None skips its cases, and sorted()
/// builds a structure that has no insertion from the sorted keys
template< class M , class I , class F , class Lo , class D , class S ,
          class Sc , class So = None >
void run( const std::string & who , const Data & X , int r , M && make ,
          I && ins , F && find , Lo && low , D && del , S && sum ,
          Sc && scan , So && sorted = None() )
{
 if( ! wanted( who ) || skip( who ) )
  return;
 const std::size_t n = X.K.size();
 const char * b = "ordered";
 auto built = [ & ]( void ) {
  if constexpr( std::is_same_v< std::decay_t< So > , None > ) {
   auto d = make();
   for( Key k : X.K ) ins( d , k );
   return( d );
   }
  else
   return( sorted() );
  };
 if constexpr( ! std::is_same_v< std::decay_t< I > , None > ) {
  print( b , "insert random" , n , who , median( make , [ & ]( auto & e ) {
          for( Key k : X.K ) ins( e , k ); } , r , n ) );
  print( b , "insert sorted" , n , who , median( make , [ & ]( auto & e ) {
          for( Key k : X.S ) ins( e , k ); } , r , n ) );
  }
 {
  auto d = built();
  print( b , "lookup hit" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           Key s = 0;
           for( Key k : X.H ) s += find( d , k );
           sink = sink + s; } , r , n ) );
  print( b , "lookup miss" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           Key s = 0;
           for( Key k : X.Q ) s += find( d , k );
           sink = sink + s; } , r , n ) );
  if constexpr( ! std::is_same_v< std::decay_t< Lo > , None > )
   print( b , "lower bound" , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) {
            Key s = 0;
            for( Key k : X.Q ) s += low( d , k );
            sink = sink + s; } , r , n ) );
  print( b , "visit" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) { sink = sink + sum( d ); } , r , n ) );
  if constexpr( ! std::is_same_v< std::decay_t< Sc > , None > )
   print( b , "scan 100" , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) {
            Key s = 0;
            for( std::size_t i = 0 ; i < n / 100 ; ++i )
             s += scan( d , X.Q[ i ] , 100 );
            sink = sink + s; } , r , n / 100 * 100 ) );
  }
 if constexpr( ! std::is_same_v< std::decay_t< D > , None > )
  print( b , "erase" , n , who , median( built , [ & ]( auto & e ) {
          for( Key k : X.H ) del( e , k ); } , r , n ) );
 }

/// the sum of the values of a range of pairs
template< class D >
Key total( const D & d )
{
 Key s = 0;
 for( const auto & [ k , v ] : d ) s += v;
 return( s );
 }

/// the sum of the values of c pairs from it, of a standard map
template< class D , class It >
Key walk( const D & d , It it , std::size_t c )
{
 Key s = 0;
 for( ; ( c > 0 ) && ( it != d.end() ) ; --c , ++it )
  s += ( *it ).second;
 return( s );
 }

#ifdef ADS_HAS_BOOST

namespace bi = boost::intrusive;

/// a node of an intrusive tree with the hook H
template< class H >
struct Node : H {
 Key k = 0;
 Key v = 0;
 /// the priority of the treap, a hash of the key
 friend bool priority_order( const Node & a , const Node & b ) {
  return( a.k * 0x9E3779B97F4A7C15ull < b.k * 0x9E3779B97F4A7C15ull );
  }
 };

template< class N >
struct KeyOf {
 using type = Key;
 const Key & operator()( const N & x ) const { return( x.k ); }
 };

/// the nodes and the intrusive tree T that links them
template< class N , class T >
struct Linked {
 std::vector< N > nodes;
 T tree;
 explicit Linked( const std::vector< Key > & K ) : nodes( K.size() ) {
  for( std::size_t i = 0 ; i < K.size() ; ++i ) nodes[ i ].k =
                                                  nodes[ i ].v = K[ i ];
  }
 ~Linked() { tree.clear(); }
 };

/// the cases of the intrusive tree T on the nodes N; the insertions link
/// the nodes, allocated beforehand in the order of the keys inserted
template< class N , class T >
void intrusive( const std::string & who , const Data & X , int r )
{
 if( ! wanted( who ) || skip( who ) )
  return;
 using P = std::unique_ptr< Linked< N , T > >;
 const std::size_t n = X.K.size();
 const char * b = "ordered";
 auto ins = [ & ]( const std::vector< Key > & K ) {
  return( median( [ & ](){ return( P( new Linked< N , T >( K ) ) ); } ,
                  [ & ]( P & p ) {
                   for( auto & x : p->nodes ) p->tree.insert( x ); } ,
                  r , n ) );
  };
 print( b , "insert random" , n , who , ins( X.K ) );
 print( b , "insert sorted" , n , who , ins( X.S ) );
 auto built = [ & ](){
  P p( new Linked< N , T >( X.K ) );
  for( auto & x : p->nodes ) p->tree.insert( x );
  return( p );
  };
 {
  P p = built();
  T & t = p->tree;
  auto find = [ & ]( const std::vector< Key > & Q ) {
   return( median( [](){ return( 0 ); } , [ & ]( int ) {
    Key s = 0;
    for( Key k : Q ) {
     const auto it = t.find( k );
     s += it == t.end() ? 0 : it->v;
     }
    sink = sink + s; } , r , n ) );
   };
  print( b , "lookup hit" , n , who , find( X.H ) );
  print( b , "lookup miss" , n , who , find( X.Q ) );
  print( b , "lower bound" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           Key s = 0;
           for( Key k : X.Q ) {
            const auto it = t.lower_bound( k );
            s += it == t.end() ? 0 : it->k;
            }
           sink = sink + s; } , r , n ) );
  print( b , "visit" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           Key s = 0;
           for( const auto & x : t ) s += x.v;
           sink = sink + s; } , r , n ) );
  print( b , "scan 100" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           Key s = 0;
           for( std::size_t i = 0 ; i < n / 100 ; ++i ) {
            auto it = t.lower_bound( X.Q[ i ] );
            for( int c = 0 ; ( c < 100 ) && ( it != t.end() ) ; ++c , ++it )
             s += it->v;
            }
           sink = sink + s; } , r , n / 100 * 100 ) );
  }
 print( b , "erase" , n , who , median( built , [ & ]( P & p ) {
         for( Key k : X.H ) p->tree.erase( k ); } , r , n ) );
 }

template< class H >
using Opts = bi::key_of_value< KeyOf< Node< H > > >;

#endif

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const auto all = keys( 2 * n , o.seed );
  Data X;
  X.K.assign( all.begin() , all.begin() + long( n ) );
  X.Q.assign( all.begin() + long( n ) , all.end() );
  X.H = X.K;
  std::shuffle( X.H.begin() , X.H.end() , std::mt19937_64( o.seed + 1 ) );
  X.S = X.K;
  std::sort( X.S.begin() , X.S.end() );
  const int r = o.runs;

  auto stdIns = []( auto & d , Key k ) { d.emplace( k , k ); };
  auto stdFind = []( const auto & d , Key k ) {
   const auto it = d.find( k );
   return( it == d.end() ? Key( 0 ) : it->second ); };
  auto stdLow = []( const auto & d , Key k ) {
   const auto it = d.lower_bound( k );
   return( it == d.end() ? Key( 0 ) : it->first ); };
  auto stdDel = []( auto & d , Key k ) { d.erase( k ); };
  auto stdScan = []( const auto & d , Key k , std::size_t c ) {
   return( walk( d , d.lower_bound( k ) , c ) ); };
  auto adsIns = []( auto & d , Key k ) { d.insert( k , k ); };
  auto adsFind = []( const auto & d , Key k ) {
   const auto v = d.lookup( k );
   return( v ? *v : Key( 0 ) ); };
  auto adsDel = []( auto & d , Key k ) { d.remove( k ); };
  auto adsSum = []( const auto & d ) { return( total( d ) ); };

  for( const char * who : { "std::map" , "std::map A/A" } )
   run( who , X , r , [](){ return( std::map< Key , Key >() ); } , stdIns ,
        stdFind , stdLow , stdDel , adsSum , stdScan );
#ifdef ADS_HAS_ABSL
  run( "absl::btree_map" , X , r ,
       [](){ return( absl::btree_map< Key , Key >() ); } , stdIns , stdFind ,
       stdLow , stdDel , adsSum , stdScan );
#endif
#ifdef ADS_HAS_TLX
  run( "tlx::btree_map" , X , r ,
       [](){ return( tlx::btree_map< Key , Key >() ); } ,
       []( auto & d , Key k ) { d.insert( std::make_pair( k , k ) ); } ,
       stdFind , stdLow , stdDel , adsSum , stdScan );
#endif
#ifdef ADS_HAS_BOOST
  run( "boost::container::flat_map" , X , r ,
       [](){ return( boost::container::flat_map< Key , Key >() ); } , None() ,
       stdFind , stdLow , None() , adsSum , stdScan , [ & ](){
        boost::container::flat_map< Key , Key > d;
        for( Key k : X.S ) d.emplace_hint( d.end() , k , k );
        return( d ); } );
  {
   using H = bi::set_base_hook< bi::link_mode< bi::normal_link > >;
   intrusive< Node< H > , bi::set< Node< H > , Opts< H > > >(
    "boost::intrusive::set" , X , r );
   }
  {
   using H = bi::avl_set_base_hook< bi::link_mode< bi::normal_link > >;
   intrusive< Node< H > , bi::avl_set< Node< H > , Opts< H > > >(
    "boost::intrusive::avl_set" , X , r );
   }
  {
   using H = bi::bs_set_base_hook< bi::link_mode< bi::normal_link > >;
   intrusive< Node< H > , bi::sg_set< Node< H > , Opts< H > > >(
    "boost::intrusive::sg_set" , X , r );
   intrusive< Node< H > , bi::splay_set< Node< H > , Opts< H > > >(
    "boost::intrusive::splay_set" , X , r );
   intrusive< Node< H > , bi::treap_set< Node< H > , Opts< H > > >(
    "boost::intrusive::treap_set" , X , r );
   }
#endif
  run( "ADS::BTreeMap" , X , r ,
       [](){ return( ADS::BTreeMap< Key , Key >() ); } , adsIns , adsFind ,
       []( const auto & d , Key k ) {
        const auto it = d.lowerBound( k );
        return( it == d.end() ? Key( 0 ) : ( *it ).first ); } ,
       adsDel , adsSum , []( const auto & d , Key k , std::size_t c ) {
        return( walk( d , d.lowerBound( k ) , c ) ); } );
  run( "ADS::RedBlackTree" , X , r ,
       [](){ return( ADS::RedBlackTree< Key , Key >() ); } , adsIns ,
       adsFind , None() , adsDel , adsSum , None() );
  run( "ADS::AVLTree" , X , r ,
       [](){ return( ADS::AVLTree< Key , Key >() ); } , adsIns , adsFind ,
       None() , adsDel , adsSum , None() );
  run( "ADS::Treap" , X , r ,
       [](){ return( ADS::Treap< Key , Key >() ); } , adsIns , adsFind ,
       None() , adsDel , adsSum , None() );
  run( "ADS::SplayTree" , X , r ,
       [](){ return( ADS::SplayTree< Key , Key >() ); } , adsIns ,
       []( auto & d , Key k ) {
        const auto v = d.lookup( k );
        return( v ? *v : Key( 0 ) ); } , None() , adsDel , adsSum ,
       None() );
  run( "ADS::SkipList" , X , r ,
       [](){ return( ADS::SkipList< Key , Key >() ); } , adsIns , adsFind ,
       None() , adsDel , adsSum , None() );
  run( "ADS::BTree t=16" , X , r ,
       [](){ return( ADS::BTree< Key , Key >( 16 ) ); } , adsIns , adsFind ,
       None() , adsDel , adsSum , None() );
  run( "ADS::SortedArrayDictionary" , X , r ,
       [](){ return( ADS::SortedArrayDictionary< Key , Key >() ); } , None() ,
       adsFind , None() , None() , adsSum , None() , [ & ](){
        ADS::SortedArrayDictionary< Key , Key > d;
        for( Key k : X.S ) d.insert( k , k );
        return( d ); } );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ordered.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
