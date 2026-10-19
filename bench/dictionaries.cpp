/*--------------------------------------------------------------------------*/
/*------------------------- File dictionaries.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the dictionaries: the hash tables and the search trees
 * of ADS against std::unordered_map and std::map and, if Boost is found,
 * boost::unordered_flat_map and boost::container::flat_map, on n distinct
 * random keys of 64 bits with values of 64 bits. The cases are the n
 * insertions into an empty structure, n lookups of present keys in random
 * order, n lookups of absent keys, and a visit of all the pairs by the
 * iterators of the standard interface. The sorted vectors (the flat map
 * and SortedArrayDictionary) are built from the sorted keys and enter only
 * the lookups and the visit. The engineered hash table of ADS, the one to
 * compare with boost::unordered_flat_map, is FlatHashTable; OpenHashTable
 * is the table of the book, here with linear probing at load 1 / 2.
 *
 * Each cell is the median of r runs, in nanoseconds per operation; the
 * lookups and the visit run on one structure built once, and the contracts
 * are off (ADS_CONTRACTS=0).
 *
 *   dictionaries [ -r runs ] [ -s seed ] n1 n2 ... *
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

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/memory/Arena.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/Treap.h"

#if __has_include( <boost/unordered/unordered_flat_map.hpp> )
 #include <boost/container/flat_map.hpp>
 #include <boost/unordered/unordered_flat_map.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <map>
#include <optional>
#include <unordered_map>

using namespace ADS::bench;
using Key = std::uint64_t;

namespace {

/// an object of the pointer table, its key in the field state
struct Item {
 Key state;
 Key value;
 };

/// the four cases on the dictionary that make() builds, by the operations
/// ins( d , k ) , find( d , k ) (the value, or 0) and sum( d ) (the sum of
/// the values by the iterators); a nil ins skips the insertions
template< class M , class I , class F , class S >
void run( const std::string & who , const std::vector< Key > & K ,
          const std::vector< Key > & H , const std::vector< Key > & Q ,
          int r , M && make , I && ins , F && find , S && sum ,
          bool inserts = true )
{
 const std::size_t n = K.size();
 auto d = make();
 if( inserts ) {
  print( "dictionaries" , "insert" , n , who , median(
          make , [ & ]( auto & e ) { for( Key k : K ) ins( e , k ); } , r ,
          n ) );
  }
 for( Key k : K ) ins( d , k );
 print( "dictionaries" , "lookup hit" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          Key s = 0;
          for( Key k : H ) s += find( d , k );
          sink = sink + s; } , r , n ) );
 print( "dictionaries" , "lookup miss" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          Key s = 0;
          for( Key k : Q ) s += find( d , k );
          sink = sink + s; } , r , n ) );
 print( "dictionaries" , "visit" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) { sink = sink + sum( d ); } ,
         r , n ) );
 }

/// the sum of the values of a range of pairs
template< class D >
Key total( const D & d )
{
 Key s = 0;
 for( const auto & [ k , v ] : d ) s += v;
 return( s );
 }

template< class O >
Key value( const O & o ) { return( o ? *o : 0 ); }

/// the smallest prime >= x
std::size_t prime( std::size_t x )
{
 for( ; ; ++x ) {
  bool p = x > 1;
  for( std::size_t d = 2 ; p && ( d * d <= x ) ; ++d ) p = x % d;
  if( p ) return( x );
  }
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const auto all = keys( 2 * n , o.seed );
  const std::vector< Key > K( all.begin() , all.begin() + long( n ) ) ,
                           Q( all.begin() + long( n ) , all.end() );
  std::vector< Key > H = K , S = K;         // the hits in random order,
  std::shuffle( H.begin() , H.end() , std::mt19937_64( o.seed + 1 ) );
  std::sort( S.begin() , S.end() );         // the keys in order
  const int r = o.runs;
  auto stdIns = []( auto & d , Key k ) { d.emplace( k , k ); };
  auto stdFind = []( const auto & d , Key k ) {
   const auto it = d.find( k );
   return( it == d.end() ? Key( 0 ) : it->second ); };
  auto adsIns = []( auto & d , Key k ) { d.insert( k , k ); };
  auto adsFind = []( const auto & d , Key k ) {
   return( value( d.lookup( k ) ) ); };
  auto adsSum = []( const auto & d ) { return( total( d ) ); };

  for( const char * who : { "std::unordered_map" ,
                            "std::unordered_map A/A" } )
   run( who , K , H , Q , r ,
        [](){ return( std::unordered_map< Key , Key >() ); } , stdIns ,
        stdFind , adsSum );
#ifdef ADS_HAS_BOOST
  run( "boost::unordered_flat_map" , K , H , Q , r ,
       [](){ return( boost::unordered_flat_map< Key , Key >() ); } , stdIns ,
       stdFind , adsSum );
#endif
  run( "ADS::ChainedHashTable" , K , H , Q , r ,
       [ n ](){ return( ADS::ChainedHashTable< Key , Key >( n ) ); } , adsIns ,
       adsFind , adsSum );
  run( "ADS::OpenHashTable linear" , K , H , Q , r , [ n ](){
       return( ADS::OpenHashTable< Key , Key >( prime( 2 * n ) ,
                                                ADS::Probing::linear() ) ); } ,
       adsIns , adsFind , adsSum );
  run( "ADS::FlatHashTable" , K , H , Q , r ,
       [](){ return( ADS::FlatHashTable< Key , Key >() ); } , adsIns ,
       adsFind , adsSum );
  {                                         // objects owned by an arena
   ADS::Arena< Item > arena;
   run( "ADS::PointerTable" , K , H , Q , r ,
        [](){ return( ADS::PointerTable< Item >() ); } ,
        [ & ]( auto & d , Key k ) {
         d.find_or_insert( k , ADS::splitmix64( k ) , [ & ]( void ) {
          return( arena.make( Item{ k , k } ) ); } ); } ,
        []( const auto & d , Key k ) {
         const Item * p = d.find( k , ADS::splitmix64( k ) );
         return( p ? p->value : Key( 0 ) ); } ,
        []( const auto & d ) {
         Key s = 0;
         for( const Item * p : d ) s += p->value;
         return( s ); } );
   }
  run( "std::map" , K , H , Q , r ,
       [](){ return( std::map< Key , Key >() ); } , stdIns , stdFind ,
       adsSum );
#ifdef ADS_HAS_BOOST
  run( "boost::container::flat_map" , K , H , Q , r , [ & ](){
       boost::container::flat_map< Key , Key > d;
       for( Key k : S ) d.emplace_hint( d.end() , k , k );
       return( d ); } , []( auto & , Key ) {} , stdFind , adsSum , false );
#endif
  run( "ADS::RedBlackTree" , K , H , Q , r ,
       [](){ return( ADS::RedBlackTree< Key , Key >() ); } , adsIns , adsFind ,
       adsSum );
  run( "ADS::AVLTree" , K , H , Q , r ,
       [](){ return( ADS::AVLTree< Key , Key >() ); } , adsIns , adsFind ,
       adsSum );
  run( "ADS::Treap" , K , H , Q , r ,
       [](){ return( ADS::Treap< Key , Key >() ); } , adsIns , adsFind ,
       adsSum );
  run( "ADS::SkipList" , K , H , Q , r ,
       [](){ return( ADS::SkipList< Key , Key >() ); } , adsIns , adsFind ,
       adsSum );
  run( "ADS::SplayTree" , K , H , Q , r ,
       [](){ return( ADS::SplayTree< Key , Key >() ); } , adsIns ,
       []( auto & d , Key k ) { return( value( d.lookup( k ) ) ); } , adsSum );
  run( "ADS::BTree t=16" , K , H , Q , r ,
       [](){ return( ADS::BTree< Key , Key >( 16 ) ); } , adsIns , adsFind ,
       adsSum );
  run( "ADS::BPlusTree m=64" , K , H , Q , r ,
       [](){ return( ADS::BPlusTree< Key , Key >( 64 ) ); } , adsIns ,
       adsFind , adsSum );
  run( "ADS::SortedArrayDictionary" , K , H , Q , r , [ & ](){
       ADS::SortedArrayDictionary< Key , Key > d;
       for( Key k : S ) d.insert( k , k );
       return( d ); } , []( auto & , Key ) {} , adsFind , adsSum , false );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File dictionaries.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
