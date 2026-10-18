/*--------------------------------------------------------------------------*/
/*------------------------ File contract_mfset.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the MFSET on a forest, with
 * and without the rank and the compression of the paths, and on lists:
 * the violations of the preconditions, which must throw
 * ADS::contract_violation, and random merges at the level 3, so that each
 * one also checks its postcondition and the invariant of the structure.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#include "ADS/mfset/ListMFSet.h"
#include "ADS/mfset/MFSet.h"

#include <random>
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

// MFSET on a forest and on lists: the elements in [ 0 , n ), find() a
// root, merge() puts its two elements in one component; every
// precondition just inside ( 0 , n - 1 ) and just outside ( -1 , n ), n = 0
// and n = 1, merge( x , x ) and a merge already done, a chain of depth
// n - 1 (no recursion), random merges against a naive labeling, each
// checking the forest (or the lists)
ADS_TEST( "mfset" )
{
 CHECK_THROWS( MFSet( -1 ) , contract_violation );
 CHECK_THROWS( ListMFSet( -1 ) , contract_violation );
 CHECK( MFSet( 0 ).count() == 0 && ListMFSet( 0 ).n() == 0 );
 MFSet Z( 0 );                           // n = 0: no element at all
 ListMFSet LZ( 0 );
 CHECK_THROWS( Z.find( 0 ) , contract_violation );
 CHECK_THROWS( (void) LZ.find( 0 ) , contract_violation );
 MFSet F( 5 );
 ListMFSet L( 5 );
 CHECK( F.find( 0 ) == 0 && F.find( 4 ) == 4 );   // just inside
 CHECK( L.find( 0 ) == 0 && L.find( 4 ) == 4 );
 CHECK_THROWS( F.find( 5 ) , contract_violation );
 CHECK_THROWS( F.find( -1 ) , contract_violation );
 CHECK_THROWS( F.merge( 0 , -1 ) , contract_violation );
 CHECK_THROWS( F.merge( 5 , 0 ) , contract_violation );
 CHECK_THROWS( F.same( 0 , 5 ) , contract_violation );
 CHECK_THROWS( (void) F.parent( 7 ) , contract_violation );
 CHECK_THROWS( (void) F.parent( -1 ) , contract_violation );
 CHECK_THROWS( (void) F.rank( 5 ) , contract_violation );
 CHECK_THROWS( (void) F.rank( -1 ) , contract_violation );
 CHECK_THROWS( (void) F.size( 5 ) , contract_violation );
 CHECK_THROWS( (void) F.size( -1 ) , contract_violation );
 CHECK_THROWS( (void) L.find( 5 ) , contract_violation );
 CHECK_THROWS( (void) L.find( -1 ) , contract_violation );
 CHECK_THROWS( L.merge( -1 , 0 ) , contract_violation );
 CHECK_THROWS( L.merge( 0 , 5 ) , contract_violation );
 CHECK( F.count() == 5 && L.n() == 5 );  // a failed merge changes nothing
 CHECK( ! F.merge( 3 , 3 ) && ! L.merge( 3 , 3 ) && F.count() == 5 );
 CHECK( F.merge( 0 , 4 ) && L.merge( 0 , 4 ) && F.count() == 4 );
 CHECK( ! F.merge( 4 , 0 ) && ! L.merge( 4 , 0 ) && F.same( 4 , 0 ) );
 {                                       // one element
  MFSet O( 1 , MFSet::Union::SIZE );
  CHECK( O.find( 0 ) == 0 && ! O.merge( 0 , 0 ) && O.size( 0 ) == 1 &&
         O.count() == 1 && O.rank( 0 ) == 0 && O.parent( 0 ) == 0 );
  }
 {                     // a chain of depth n - 1: find() is not recursive
  const int n = 3000;
  MFSet C( n , MFSet::Union::NONE , false );
  for( int i = 1 ; i < n ; ++i )
   CHECK( C.merge( i , i - 1 ) );
  CHECK( C.find( 0 ) == n - 1 && C.count() == 1 &&
         C.steps() >= n - 1 );
  }
 std::mt19937 g( 1 );
 for( auto u : { MFSet::Union::NONE , MFSet::Union::RANK ,
                 MFSet::Union::SIZE } )
  for( bool compress : { false , true } ) {
   const int n = 60;
   MFSet M( n , u , compress );
   ListMFSet Q( n );
   std::vector< int > lab( n );
   for( int i = 0 ; i < n ; ++i ) lab[ i ] = i;
   for( int op = 0 ; op < 200 ; ++op ) {
    const int x = int( g() % n ) , y = int( g() % n );
    const bool apart = lab[ x ] != lab[ y ];
    CHECK( M.merge( x , y ) == apart && Q.merge( x , y ) == apart );
    if( apart ) {
     const int a = lab[ x ] , b = lab[ y ];
     for( auto & l : lab ) if( l == b ) l = a;
     }
    }
   for( int x = 0 ; x < n ; ++x )
    for( int y = 0 ; y < n ; ++y )
     CHECK( ( M.find( x ) == M.find( y ) ) == ( lab[ x ] == lab[ y ] ) &&
            ( Q.find( x ) == Q.find( y ) ) == ( lab[ x ] == lab[ y ] ) );
   }
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
/*---------------------- End File contract_mfset.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
