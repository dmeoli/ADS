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

#include "ADS/set/BoolSet.h"
#include "ADS/set/DictionarySet.h"
#include "ADS/set/HashMultiset.h"
#include "ADS/set/LinkedHashSet.h"
#include "ADS/set/ListSet.h"
#include "ADS/set/SetOperations.h"
#include "ADS/set/SortedListSet.h"

#include <algorithm>
#include <iterator>
#include <random>
#include <set>

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
// [ 0 , N ); random operations against std::set, each checking the size,
// and the operations of two sets item by item
ADS_TEST( "bool-set" )
{
 CHECK_THROWS( BoolSet( -1 ) , contract_violation );
 BoolSet A( 10 );
 CHECK_THROWS( A.insert( 10 ) , contract_violation );
 CHECK_THROWS( A.insert( -1 ) , contract_violation );
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
// the others; random operations against std::set and the four operations
ADS_TEST( "list-sets" )
{
 ListSet< int > L = { 1 , 2 };
 CHECK_THROWS( L.insertNew( 2 ) , contract_violation );
 SortedListSet< int > S = { 1 , 5 };
 CHECK_THROWS( S.insertLast( 5 ) , contract_violation );
 CHECK_THROWS( S.insertLast( 3 ) , contract_violation );
 S.insertLast( 9 );
 CHECK( S.max() == 9 );
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
// item by item, the order and the counters consistent
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
