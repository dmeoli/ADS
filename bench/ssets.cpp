/*--------------------------------------------------------------------------*/
/*---------------------------- File ssets.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sorted sets of Open Data Structures (exercise 9.14
 * of Morin): the skip list, the scapegoat trees, the treaps, the
 * randomized search trees, the left-leaning red-black tree, the trees of
 * partial rebuilding, the Red-Black tree and the AVL tree of ADS against
 * std::set and, with ODS=<dir> (the directory of the C++ sources of the
 * book), the SkiplistSSet, ScapegoatTree1, Treap1 and RedBlackTree1 of
 * Morin, on n distinct random keys of 31 bits, the sets of Morin holding
 * the keys alone and those of ADS a value of 4 bytes with each.
 *
 * Two defects of the code of Morin show in these runs, and the later
 * comparisons must patch them: the constructor of his ScapegoatTree leaves
 * the bound q uninitialized (ScapegoatTree.h, the constructor sets
 * nothing), so that a tree may rebuild at random or climb past its root,
 * and his SkiplistSSet frees with delete the nodes it allocates with malloc
 * (SkiplistSSet.h, remove(x)); both crashed the runs of 2000 keys here.
 *
 * The cases are those that the exercise asks for: the n insertions in
 * random order and in increasing order into an empty set, n searches of
 * present keys in random order, n searches of the first key not smaller
 * than an absent one (the find(x) of an SSet), and the removal of all the
 * keys in random order, in increasing order and in decreasing order from
 * a set built in random order. Each cell is the median of r runs, in
 * nanoseconds per operation, with the contracts off; a structure that has
 * no operation for a case is left out of it.
 *
 *   ssets [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/dictionary/SkipList.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/FingerTreap.h"
#include "ADS/tree/HashedTreap.h"
#include "ADS/tree/LeftLeaningRedBlackTree.h"
#include "ADS/tree/PartialRebuildingTree.h"
#include "ADS/tree/RandomizedSearchTree.h"
#include "ADS/tree/ScapegoatTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Treap.h"

#if defined( ADS_HAS_ODS )
 #include "RedBlackTree.h"
 #include "ScapegoatTree.h"
 #include "SkiplistSSet.h"
 #include "Treap.h"
#endif

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace ADS::bench;
using Key = int;

namespace
{

/// the keys of a run: K in the order of insertion, H the same in another
/// order, Q absent ones, S sorted, D sorted down
struct Data
{
 std::vector< Key > K , H , Q , S , D;
 };

/// the cases on the set that make() builds empty, by the operations
/// ins( d , k ), find( d , k ) (1 if present), low( d , k ) (the key found,
/// or 0) and del( d , k )
template< class M , class I , class F , class L , class E >
void run( const std::string & who , const Data & X , int r , M && make ,
	  I && ins ,
	  F && find , L && low , E && del )
{
 if( skip( who ) )
  return;
 const std::size_t n = X.K.size();
 const char * b = "ssets";
 auto built = [ & ]( void ) {
  auto d = make();
  for( Key k : X.K )
   ins( d , k );
  return( d );
  };
 print( b , "insert random" , n , who ,
	median(
	  make ,
	  [ & ]( auto & e ) {
	   for( Key k : X.K )
	    ins( e , k );
	   } ,
	  r , n ) );
 print( b , "insert sorted" , n , who ,
	median(
	  make ,
	  [ & ]( auto & e ) {
	   for( Key k : X.S )
	    ins( e , k );
	   } ,
	  r , n ) );
 {
  auto d = built();
  print( b , "find hit" , n , who ,
	 median( []() { return( 0 ); } ,
		 [ & ]( int ) {
		  std::uint64_t s = 0;
		  for( Key k : X.H )
		   s += find( d , k );
		  sink = sink + s;
		  } ,
		 r , n ) );
  print( b , "find(x) miss" , n , who ,
	 median( []() { return( 0 ); } ,
		 [ & ]( int ) {
		  std::uint64_t s = 0;
		  for( Key k : X.Q )
		   s += std::uint64_t( low( d , k ) );
		  sink = sink + s;
		  } ,
		 r , n ) );
  }
 print( b , "remove random" , n , who ,
	median(
	  built ,
	  [ & ]( auto & e ) {
	   for( Key k : X.H )
	    del( e , k );
	   } ,
	  r , n ) );
 print( b , "remove sorted" , n , who ,
	median(
	  built ,
	  [ & ]( auto & e ) {
	   for( Key k : X.S )
	    del( e , k );
	   } ,
	  r , n ) );
 print( b , "remove sorted down" , n , who ,
	median(
	  built ,
	  [ & ]( auto & e ) {
	   for( Key k : X.D )
	    del( e , k );
	   } ,
	  r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937 g( o.seed );
  std::vector< Key > all;
  {
   const auto w = keys( 2 * n , o.seed );  // 31 bits, distinct
   std::set< Key > seen;
   for( std::uint64_t x : w ) {
    const Key k = Key( x & 0x7FFFFFFF );
    if( ( k > 0 ) && seen.insert( k ).second )
     all.push_back( k );
    }
   while( all.size() < 2 * n ) {
    const Key k = Key( g() & 0x7FFFFFFF );
    if( ( k > 0 ) && seen.insert( k ).second )
     all.push_back( k );
    }
   }
  Data X;
  X.K.assign( all.begin() , all.begin() + long( n ) );
  X.Q.assign( all.begin() + long( n ) , all.end() );
  X.H = X.K;
  std::shuffle( X.H.begin() , X.H.end() , std::mt19937_64( o.seed + 1 ) );
  X.S = X.K;
  std::sort( X.S.begin() , X.S.end() );
  X.D.assign( X.S.rbegin() , X.S.rend() );
  const int r = o.runs;

  auto adsIns = []( auto & d , Key k ) { d.insert( k , k ); };
  auto adsFind = []( const auto & d , Key k ) {
   return( std::uint64_t( d.contains( k ) ) );
   };
  auto adsLow = []( const auto & d , Key k ) {
   const auto v = d.lowerBound( k );
   return( v ? v->first : Key( 0 ) );
   };
  auto adsDel = []( auto & d , Key k ) { d.remove( k ); };

  run(
    "std::set" , X , r , []() { return( std::set< Key >() ); } ,
    []( auto & d , Key k ) { d.insert( k ); } ,
    []( const auto & d , Key k ) { return( std::uint64_t( d.count( k ) ) ); } ,
    []( const auto & d , Key k ) {
     const auto it = d.lower_bound( k );
     return( it == d.end() ? Key( 0 ) : *it );
     } ,
    []( auto & d , Key k ) { d.erase( k ); } );
  run(
    "ADS::SkipList" , X , r ,
    []() { return( ADS::SkipList< Key , Key >() ); } ,
    adsIns , adsFind ,
    []( const auto & d , Key k ) {
     const auto it = d.lowerBound( k );
     return( it == d.end() ? Key( 0 ) : ( *it ).first );
     } ,
    adsDel );
  run(
    "ADS::ScapegoatTree" , X , r ,
    []() { return( ADS::ScapegoatTree< Key , Key >() ); } , adsIns , adsFind ,
    adsLow , adsDel );
  run(
    "ADS::ScapegoatTree sizes" , X , r ,
    []() { return( ADS::ScapegoatTree< Key , Key , true >() ); } , adsIns ,
    adsFind , adsLow , adsDel );
  run(
    "ADS::ScapegoatTree recompute" , X , r ,
    []() {
     using S = ADS::ScapegoatTree< Key , Key >;
     return( S( 1.5 , S::Sizes::RECOMPUTE ) );
     } ,
    adsIns , adsFind , adsLow , adsDel );
  run(
    "ADS::Treap" , X , r , []() { return( ADS::Treap< Key , Key >() ); } ,
    adsIns ,
    adsFind , adsLow , adsDel );
  run(
    "ADS::HashedTreap" , X , r ,
    []() { return( ADS::HashedTreap< Key , Key >() ); } , adsIns , adsFind ,
    adsLow , adsDel );
  run(
    "ADS::FingerTreap" , X , r ,
    []() { return( ADS::FingerTreap< Key , Key >() ); } , adsIns , adsFind ,
    adsLow , adsDel );
  run(
    "ADS::RandomizedSearchTree" , X , r ,
    []() { return( ADS::RandomizedSearchTree< Key , Key >() ); } , adsIns ,
    adsFind , adsLow , adsDel );
  run(
    "ADS::LeftLeaningRedBlackTree" , X , r ,
    []() { return( ADS::LeftLeaningRedBlackTree< Key , Key >() ); } , adsIns ,
    adsFind , adsLow , adsDel );
  run(
    "ADS::WeightRebuildTree" , X , r ,
    []() { return( ADS::WeightRebuildTree< Key , Key >() ); } , adsIns ,
    adsFind ,
    adsLow , adsDel );
  run(
    "ADS::CountdownTree" , X , r ,
    []() { return( ADS::CountdownTree< Key , Key >() ); } , adsIns , adsFind ,
    adsLow , adsDel );
  run(
    "ADS::DynamiteTree" , X , r ,
    []() { return( ADS::DynamiteTree< Key , Key >() ); } , adsIns , adsFind ,
    adsLow , adsDel );
  run(
    "ADS::RedBlackTree" , X , r ,
    []() { return( ADS::RedBlackTree< Key , Key >() ); } , adsIns , adsFind ,
    []( const auto & , Key ) { return( Key( 0 ) ); } , adsDel );
  run(
    "ADS::AVLTree" , X , r , []() { return( ADS::AVLTree< Key , Key >() ); } ,
    adsIns , adsFind , []( const auto & , Key ) { return( Key( 0 ) ); } ,
    adsDel );
#if defined( ADS_HAS_ODS )
  auto odsIns = []( auto & d , Key k ) { d->add( k ); };
  auto odsFind = []( const auto & d , Key k ) {
   return( std::uint64_t( d->find( k ) == k ) );
   };
  auto odsLow = []( const auto & d , Key k ) { return( d->find( k ) ); };
  auto odsDel = []( auto & d , Key k ) { d->remove( k ); };
  run(
    "ods::SkiplistSSet" , X , r ,
    []() { return( std::make_unique< ods::SkiplistSSet< Key > >() ); } ,
    odsIns , odsFind , odsLow , odsDel );
  run(
    "ods::ScapegoatTree1" , X , r ,
    []() { return( std::make_unique< ods::ScapegoatTree1< Key > >() ); } ,
    odsIns , odsFind , odsLow , odsDel );
  run(
    "ods::Treap1" , X , r ,
    []() { return( std::make_unique< ods::Treap1< Key > >() ); } , odsIns ,
    odsFind , odsLow , odsDel );
  run(
    "ods::RedBlackTree1" , X , r ,
    []() { return( std::make_unique< ods::RedBlackTree1< Key > >() ); } ,
    odsIns , odsFind , odsLow , odsDel );
#endif
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ssets.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
