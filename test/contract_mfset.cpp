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
// root, merge() puts its two elements in one component; random merges
// against a naive labeling, each checking the forest (or the lists)
ADS_TEST( "mfset" )
{
 CHECK_THROWS( MFSet( -1 ) , contract_violation );
 CHECK_THROWS( ListMFSet( -1 ) , contract_violation );
 MFSet F( 5 );
 ListMFSet L( 5 );
 CHECK_THROWS( F.find( 5 ) , contract_violation );
 CHECK_THROWS( F.merge( 0 , -1 ) , contract_violation );
 CHECK_THROWS( (void) F.parent( 7 ) , contract_violation );
 CHECK_THROWS( (void) L.find( 5 ) , contract_violation );
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
