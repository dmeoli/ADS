/*--------------------------------------------------------------------------*/
/*------------------------- File contract_set.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the sets: on a boolean vector,
 * on an unsorted and on a sorted list, on a dictionary, with the order of
 * insertion, the multiset, and the generic operations of two sets. For
 * each, the violations of the preconditions, which must throw
 * ADS::contract_violation, and random operations at the level 3, so that
 * each one also checks its postconditions, the audits of the operations
 * of two sets and the invariant of the structure.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/set/BloomFilter.h"
#include "ADS/set/BloomVariants.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/DictionarySet.h"
#include "ADS/set/HashMultiset.h"
#include "ADS/set/LinkedHashSet.h"
#include "ADS/set/ListSet.h"
#include "ADS/set/SetOperations.h"
#include "ADS/set/SortedIntersection.h"
#include "ADS/set/SortedListSet.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <tuple>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// SET on a boolean vector: the dimension N >= 0 and the items in
// [ 0 , N ), 0 and N - 1 just inside, -1 and N just outside, N = 0; the
// removal of an item out of [ 0 , N ) or absent changes nothing; random
// operations against std::set, each checking the size, and the operations
// of two sets item by item, an empty one too
ADS_TEST( "bool-set" )
{
 CHECK_THROWS( BoolSet( -1 ) , contract_violation );
 BoolSet A( 10 );
 CHECK_THROWS( A.insert( 10 ) , contract_violation );
 CHECK_THROWS( A.insert( -1 ) , contract_violation );
 A.insert( 0 );                               // just inside: 0 and N - 1
 A.insert( 9 );
 CHECK( A.contains( 0 ) && A.contains( 9 ) && A.size() == 2 );
 A.remove( -1 );                              // out of [ 0 , N ): nothing
 A.remove( 10 );
 CHECK( A.size() == 2 && ! A.contains( -1 ) && ! A.contains( 10 ) );
 A.remove( 0 );
 A.remove( 9 );
 A.remove( 9 );                               // not there: nothing
 CHECK( A.size() == 0 );
 BoolSet Z( 0 );                              // N = 0: no item at all
 CHECK_THROWS( Z.insert( 0 ) , contract_violation );
 CHECK( setUnion( Z , Z ).size() == 0 && setUnion( Z , A ).dim() == 10 );
 std::mt19937 g( 1 );
 std::set< long > R;
 for( int op = 0 ; op < 500 ; ++op ) {
  const long x = long( g() % 10 );
  if( g() % 2 ) {
   A.insert( x );
   R.insert( x );
   }
  else {
   A.remove( x );
   R.erase( x );
   }
  CHECK( A.size() == R.size() );
  }
 BoolSet B( 14 );
 for( long x : { 1L , 3L , 12L } ) B.insert( x );
 CHECK( setUnion( A , B ).dim() == 14 );
 CHECK( setIntersection( A , B ).dim() == 10 );
 (void) setDifference( A , B );
 (void) setSymmetricDifference( A , B );
 }

// SET on lists: insertNew() of an item already there is refused (an
// audit), the sorted list refuses insertLast() of an item not larger than
// the others (an empty sorted list takes any, the extremes of int);
// random operations against std::set and the four operations
ADS_TEST( "list-sets" )
{
 ListSet< int > L = { 1 , 2 };
 CHECK_THROWS( L.insertNew( 2 ) , contract_violation );
 SortedListSet< int > S = { 1 , 5 };
 CHECK_THROWS( S.insertLast( 5 ) , contract_violation );
 CHECK_THROWS( S.insertLast( 3 ) , contract_violation );
 S.insertLast( 9 );
 CHECK( S.max() == 9 );
 SortedListSet< int > E;                      // empty: any item is last
 E.insertLast( std::numeric_limits< int >::min() );
 E.insertLast( std::numeric_limits< int >::max() );
 CHECK_THROWS( E.insertLast( std::numeric_limits< int >::max() ) ,
               contract_violation );
 CHECK( E.size() == 2 && E.max() == std::numeric_limits< int >::max() );
 ListSet< int > N;
 N.insertNew( 0 );                            // the empty list
 CHECK_THROWS( N.insertNew( 0 ) , contract_violation );
 N.remove( 5 );
 CHECK( N.size() == 1 && setUnion( N , ListSet< int >() ).size() == 1 &&
        setIntersection( N , ListSet< int >() ).size() == 0 );
 std::mt19937 g( 2 );
 ListSet< int > LA , LB;
 SortedListSet< int > SA , SB;
 std::set< int > RA , RB;
 for( int op = 0 ; op < 600 ; ++op ) {
  const int x = int( g() % 40 );
  const bool a = g() % 2;
  if( g() % 3 ) {
   ( a ? LA : LB ).insert( x );
   ( a ? SA : SB ).insert( x );
   ( a ? RA : RB ).insert( x );
   }
  else {
   ( a ? LA : LB ).remove( x );
   ( a ? SA : SB ).remove( x );
   ( a ? RA : RB ).erase( x );
   }
  }
 CHECK( LA.size() == RA.size() && SB.size() == RB.size() );
 std::set< int > U;
 std::set_union( RA.begin() , RA.end() , RB.begin() , RB.end() ,
                 std::inserter( U , U.begin() ) );
 CHECK( setUnion( LA , LB ).size() == U.size() );
 CHECK( setUnion( SA , SB ).size() == U.size() );
 (void) setIntersection( LA , LB );
 (void) setDifference( SA , SB );
 (void) setSymmetricDifference( LA , LB );
 (void) setSymmetricDifference( SA , SB );
 }

// the axioms of SET [see ADT.h] on every realization, for sets A built by
// random operations and replayed for each side: contains( x , create ) =
// false, contains( x , insert( y , A ) ) = ( x = y ) or contains( x , A ),
// contains( x , remove( y , A ) ) = ( x != y ) and contains( x , A ),
// inserting an item already there and removing one that is not leave the
// size as it was
template< class S , class F >
void set_axioms( F create , unsigned seed )
{
 using I = typename S::Item;
 std::mt19937 g( seed );
 const int U = 12;                              // the items in [ 0 , U )
 {
  S E = create();
  for( int x = 0 ; x < U ; ++x ) CHECK( ! E.contains( I( x ) ) );
  }
 std::vector< std::pair< bool , int > > ops;    // insert or remove
 auto build = [ & ]( void ) {
  S A = create();
  for( auto [ in , x ] : ops )
   if( in ) A.insert( I( x ) ); else A.remove( I( x ) );
  return( A );
  };
 for( int t = 0 ; t < 40 ; ++t ) {
  const int y = int( g() % U );
  for( bool in : { true , false } ) {
   S A = build() , B = build();
   const std::size_t n = A.size();
   const bool had = A.contains( I( y ) );
   if( in ) B.insert( I( y ) ); else B.remove( I( y ) );
   for( int x = 0 ; x < U ; ++x ) {
    const bool a = A.contains( I( x ) );
    const bool b = in ? ( x == y ) || a : ( x != y ) && a;
    CHECK( B.contains( I( x ) ) == b );
    }
   if( in == had )                              // nothing to do
    CHECK( B.size() == n );
   }
  ops.emplace_back( g() % 3 != 0 , y );
  }
 }

ADS_TEST( "set-axioms" )
{
 set_axioms< BoolSet >( [] { return( BoolSet( 12 ) ); } , 4 );
 set_axioms< ListSet< int > >( [] { return( ListSet< int >() ); } , 5 );
 set_axioms< SortedListSet< int > >( [] { return( SortedListSet< int >() ); } ,
                                     6 );
 set_axioms< ChainedHashSet< int > >(
  [] { return( ChainedHashSet< int >( 5 , true ) ); } , 7 );
 set_axioms< OpenHashSet< int > >( [] {
  return( OpenHashSet< int >( 7 , Probing::linear() , true ) ); } , 8 );
 set_axioms< TreeSet< int > >( [] { return( TreeSet< int >() ); } , 9 );
 set_axioms< LinkedHashSet< int > >( [] { return( LinkedHashSet< int >() ); } ,
                                     10 );
 }

// SET on a dictionary, with the order of insertion, and the multiset: the
// item there after insert() and not after remove(), the generic operations
// item by item, the order and the counters consistent; the multiset with
// an absent item removed and an item five times
ADS_TEST( "hash-sets" )
{
 std::mt19937 g( 3 );
 ChainedHashSet< int > C( 7 , true );
 TreeSet< int > T;
 LinkedHashSet< int > H;
 HashMultiset< int > M;
 std::multiset< int > R;
 for( int op = 0 ; op < 800 ; ++op ) {
  const int x = int( g() % 60 );
  if( g() % 3 ) {
   C.insert( x ); T.insert( x ); H.insert( x ); M.insert( x ); R.insert( x );
   }
  else {
   C.remove( x ); T.remove( x ); H.remove( x ); M.remove( x );
   if( R.count( x ) ) R.erase( R.find( x ) );
   }
  }
 CHECK( C.size() == T.size() && T.size() == H.size() );
 CHECK( M.size() == R.size() );
 TreeSet< int > E;
 for( int x = 0 ; x < 60 ; x += 3 ) E.insert( x );
 CHECK( setUnion( T , E ).size() + setIntersection( T , E ).size() ==
        T.size() + E.size() );
 CHECK( setDifference( T , E ).size() + setIntersection( T , E ).size() ==
        T.size() );
 (void) setSymmetricDifference( T , E );
 // the multiset: an absent item removed, an item many times, one copy
 // left after a removal
 HashMultiset< int > K;
 K.remove( 1 );
 for( int i = 0 ; i < 5 ; ++i ) K.insert( 1 );
 K.remove( 1 );
 CHECK( K.count( 1 ) == 4 && K.size() == 4 );
 for( int i = 0 ; i < 4 ; ++i ) K.remove( 1 );
 CHECK( K.count( 1 ) == 0 && K.size() == 0 );
 }

// the filters of Bloom: each precondition just inside and just outside (m
// = 1 and 0, k , r = 1 and 0, p in ( 0 , 1 ) at 0 and 1, n = 0, epsilon at
// 0 and 1, m < r, an empty A), a filter of one bit, the spectral filter
// removing what it has and refusing what it has not; the intersections of
// sorted vectors: unsorted or repeated items refused by the audits, L = 0,
// the empty vectors, disjoint and equal ones; the external ones with
// their memory, the files on another memory, an output not empty; the
// permuted buckets with w at 1 , 63 , 0 , 64, a even, L = 0, a value
// above the mask, a repeated value, two sets of different permutations
ADS_TEST( "bloom-and-intersections" )
{
 CHECK_THROWS( BloomFilter< int >( 0 , 1 ) , contract_violation );
 CHECK_THROWS( BloomFilter< int >( 8 , 0 ) , contract_violation );
 BloomFilter< int > one( 1 , 1 );             // one bit: all collide
 CHECK( ! one.maybe( 3 ) );
 one.insert( 3 );
 CHECK( one.maybe( 3 ) && one.maybe( 4 ) && one.bits() == 1 );
 BloomFilter< int > w64( 64 , 2 ) , w65( 65 , 2 );   // a word and a bit
 w64.insert( 7 );
 w65.insert( 7 );
 CHECK( w64.maybe( 7 ) && w65.maybe( 7 ) && w65.bits() == 65 );
 CHECK_THROWS( BloomFilter< int >::forItems( 0 , 0.1 ) , contract_violation );
 CHECK_THROWS( BloomFilter< int >::forItems( 10 , 0 ) , contract_violation );
 CHECK_THROWS( BloomFilter< int >::forItems( 10 , 1 ) , contract_violation );
 CHECK( BloomFilter< int >::forItems( 1 , 0.999 ).hashes() >= 1 );
 CHECK( BloomFilter< int >::forItems( 1 , 1e-300 ).bits() > 1000 );
 CHECK_THROWS( (void) bloomFalsePositive( 0.5 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( (void) bloomFalsePositive( 8 , -1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) bloomFalsePositive( 8 , 1 , 0 ) , contract_violation );
 CHECK( bloomFalsePositive( 8 , 0 , 1 ) == 0 &&
        bloomFalsePositive( 1 , 5 , 1 ) == 1 );
 CHECK_THROWS( (void) bloomBestHashes( 0 , 1 ) , contract_violation );
 CHECK_THROWS( (void) bloomBestHashes( 8 , 0.5 ) , contract_violation );
 CHECK( bloomBestHashes( 1 , 1 ) >= 1 && bloomBestHashes( 1 , 1e9 ) == 1 );
 CHECK( bloomBestHashes( 1e300 , 1 ) >= 1 );           // capped to an int
 CHECK_THROWS( (void) bloomLowerBound( -1 , 0.5 ) , contract_violation );
 CHECK_THROWS( (void) bloomLowerBound( 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) bloomLowerBound( 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) bloomBits( 1 , 1.5 ) , contract_violation );
 CHECK( bloomLowerBound( 0 , 0.5 ) == 0 && bloomLowerBound( 4 , 0.5 ) == 4 );
 CHECK_THROWS( (void) compressedBloom( 0 , 1 ) , contract_violation );
 CHECK_THROWS( (void) compressedBloom( 8 , 0 ) , contract_violation );
 CHECK( compressedBloom( 1e-9 , 1 ).second >= 0 );
 const std::vector< int > A = { 1 , 2 , 3 } , B = { 2 , 3 , 4 , 5 };
 CHECK_THROWS( (void) bloomIntersection( A , B , 0 ) , contract_violation );
 CHECK_THROWS( (void) bloomIntersection( std::vector< int >() , B , 8 ) ,
               contract_violation );
 CHECK( bloomIntersection( A , B , 1 ).second.size() == 2 );   // m = 1
 CHECK( bloomIntersection( A , std::vector< int >() , 8 ).first.empty() );
 CHECK_THROWS( ( SpectralBloomFilter< int >( 0 , 1 ) ) ,
               contract_violation );
 CHECK_THROWS( ( SpectralBloomFilter< int >( 4 , 0 ) ) ,
               contract_violation );
 SpectralBloomFilter< int > SB( 16 , 2 );
 CHECK_THROWS( SB.remove( 5 ) , contract_violation );   // count 0 < 1
 SB.insert( 5 , 0 );                          // f = 0: nothing
 SB.insert( 5 , 2 );
 SB.remove( 5 , 2 );                          // count( x ) = f: inside
 SB.remove( 5 , 0 );
 CHECK_THROWS( SB.remove( 5 , 1 ) , contract_violation );
 CHECK_THROWS( ( PartitionedBloomFilter< int >( 4 , 0 ) ) ,
               contract_violation );
 CHECK_THROWS( ( PartitionedBloomFilter< int >( 2 , 3 ) ) ,
               contract_violation );
 PartitionedBloomFilter< int > PB( 3 , 3 );   // m = r: one bit each
 PB.insert( 1 );
 CHECK( PB.maybe( 1 ) && PB.bits() == 3 );

 // the intersections in memory
 using V = std::vector< int >;
 const V U = { 3 , 1 } , D = { 1 , 1 } , E;
 for( const V & bad : { U , D } ) {
  CHECK_THROWS( (void) intersectMerge( bad , A ) , contract_violation );
  CHECK_THROWS( (void) intersectBinarySearch( A , bad ) ,
                contract_violation );
  CHECK_THROWS( (void) intersectMutualPartition( bad , A ) ,
                contract_violation );
  CHECK_THROWS( (void) intersectDoubling( A , bad ) , contract_violation );
  CHECK_THROWS( (void) intersectTwoLevel( bad , A , 2 ) ,
                contract_violation );
  }
 CHECK_THROWS( (void) intersectTwoLevel( A , B , 0 ) , contract_violation );
 for( const auto & [ X , Y , R ] : { std::tuple( E , E , E ) ,
                                     std::tuple( A , E , E ) ,
                                     std::tuple( E , B , E ) ,
                                     std::tuple( A , B , V{ 2 , 3 } ) ,
                                     std::tuple( A , A , A ) ,
                                     std::tuple( V{ 1 } , V{ 2 } , E ) } ) {
  CHECK( intersectMerge( X , Y ) == R && intersectBinarySearch( X , Y ) == R &&
         intersectMutualPartition( X , Y ) == R &&
         intersectDoubling( X , Y ) == R &&
         intersectTwoLevel( X , Y , 1 ) == R &&
         intersectTwoLevel( X , Y , 100 ) == R );
  }

 // the external ones
 TwoLevelMemory mem( 2 , 8 ) , other( 2 , 8 ) , small( 2 , 4 ) ,
                six( 2 , 6 );
 const DiskFile< int > FA( mem , A ) , FB( mem , B ) , FO( other , B );
 DiskFile< int > H( mem ) , out( mem ) , full( mem , V{ 9 } ) , HO( other );
 CHECK_THROWS( twoLevelHeaders( FA , 0 , H ) , contract_violation );
 CHECK_THROWS( twoLevelHeaders( FA , 2 , HO ) , contract_violation );
 CHECK_THROWS( twoLevelHeaders( FA , 2 , full ) , contract_violation );
 twoLevelHeaders( FA , 2 , H );
 CHECK( H.contents() == V( { 1 , 3 } ) );
 CHECK_THROWS( intersectMergeExternal( FA , FO , out ) , contract_violation );
 CHECK_THROWS( intersectMergeExternal( FA , FB , full ) ,
               contract_violation );
 {
  const DiskFile< int > SA( small , A ) , SB2( small , B );
  DiskFile< int > so( small );
  CHECK_THROWS( intersectMergeExternal( SA , SB2 , so ) ,
                contract_violation );                  // M = 2 B < 3 B
  }
 {
  const DiskFile< int > XA( six , A ) , XB( six , B ) , XU( six , U );
  DiskFile< int > xo( six ) , xh( six ) , xu( six );
  intersectMergeExternal( XA , XB , xo );              // M = 3 B
  CHECK( xo.contents() == V( { 2 , 3 } ) );
  CHECK_THROWS( intersectMergeExternal( XU , XB , xu ) ,
                contract_violation );                  // unsorted
  twoLevelHeaders( XA , 1 , xh );
  DiskFile< int > xo2( six );
  CHECK_THROWS( intersectTwoLevelExternal( XA , xh , 1 , XB , xo2 ) ,
                contract_violation );                  // M = 3 B < 4 B
  }
 intersectMergeExternal( FA , FB , out );
 CHECK( out.contents() == V( { 2 , 3 } ) );
 DiskFile< int > out2( mem ) , out3( mem );
 CHECK_THROWS( intersectTwoLevelExternal( FA , H , 0 , FB , out2 ) ,
               contract_violation );
 CHECK_THROWS( intersectTwoLevelExternal( FA , H , 2 , FO , out2 ) ,
               contract_violation );
 CHECK_THROWS( intersectTwoLevelExternal( FA , H , 2 , FB , full ) ,
               contract_violation );
 intersectTwoLevelExternal( FA , H , 2 , FB , out3 );  // M = 4 B
 CHECK( out3.contents() == V( { 2 , 3 } ) );

 // the permuted buckets
 using W = std::vector< std::uint64_t >;
 CHECK_THROWS( PermutedBuckets( W{ 0 } , 0 , 1 , 1 , 0 ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( W{ 0 } , 64 , 1 , 1 , 0 ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( W{ 0 } , 8 , 1 , 2 , 0 ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( W{ 0 } , 8 , 0 , 1 , 0 ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( W{ 256 } , 8 , 1 , 1 , 0 ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( W{ 5 , 5 } , 8 , 1 , 3 , 1 ) ,
               contract_violation );
 const PermutedBuckets P1( W{ 0 , 1 } , 1 , 1 , 1 , 0 );   // w = 1, full
 CHECK( P1.values() == W( { 0 , 1 } ) );
 const std::uint64_t top = ( std::uint64_t( 1 ) << 63 ) - 1;
 const PermutedBuckets P63( W{ 0 , top } , 63 , 1 , 5 , 7 ) ,
                       Q63( W{ top , 9 } , 63 , 2 , 5 , 7 ) ,
                       R63( W{ top } , 63 , 1 , 3 , 7 );
 CHECK( P63.intersect( Q63 ) == W( { top } ) );
 CHECK_THROWS( (void) P63.intersect( R63 ) , contract_violation );
 const PermutedBuckets P0( W{} , 8 , 1 , 1 , 0 );
 CHECK( P0.size() == 0 && P0.intersect( P0 ).empty() );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File contract_set.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
