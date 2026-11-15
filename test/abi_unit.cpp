/*--------------------------------------------------------------------------*/
/*--------------------------- File abi_unit.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A translation unit of the test of the configurations of the layouts [see
 * abi_units.cpp], compiled twice into the same program with different
 * flags: `ADS_ABI_UNIT` is the letter of the unit (`A` or `B`), which names
 * its two functions. Each fills a `FlatHashTable`, a `DenseHashMap` and a
 * `ConcurrentFlatTable` (on 4 threads) and checks them; the other tells the
 * name of the configuration of its unit and that of the type of its
 * `ConcurrentFlatTable`, as the compiler gives them.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FlatHashTable.h"

#include <cstdint>
#include <string>
#include <thread>
#include <typeinfo>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

#define ADS_ABI_STR2( x ) #x
#define ADS_ABI_STR( x ) ADS_ABI_STR2( x )
#define ADS_ABI_CAT2( a, b ) a##b
#define ADS_ABI_CAT( a, b ) ADS_ABI_CAT2( a, b )
#define ADS_ABI_FN( f ) ADS_ABI_CAT( f, ADS_ABI_UNIT )

/// whether the tables of the unit hold what was put in them
bool ADS_ABI_FN( abiWork )( void )
{
 const std::uint64_t n = 20000;
 ADS::FlatHashTable< std::uint64_t , std::uint64_t > flat;
 ADS::DenseHashMap< std::uint64_t , std::uint64_t > dense;
 ADS::ConcurrentFlatTable< std::uint64_t , std::uint64_t > shared( 1024 );
 for( std::uint64_t k = 0 ; k < n ; ++k ) {
  flat.insert( k * 7919 , k );
  dense.insert( k * 7919 , k );
  }
 std::vector< std::thread > T;
 for( std::uint64_t t = 0 ; t < 4 ; ++t )
  T.emplace_back( [ & , t ] {
   for( std::uint64_t k = t ; k < n ; k += 4 )
    ( void )shared.findOrInsert( k * 7919 , k );
   } );
 for( auto & t : T )
  t.join();
 bool ok =
   ( flat.size() == n ) && ( dense.size() == n ) && ( shared.size() == n );
 for( std::uint64_t k = 0 ; ok && ( k < n ) ; ++k ) {
  const auto * a = flat.find( k * 7919 );
  const auto * b = dense.find( k * 7919 );
  const auto c = shared.find( k * 7919 );
  ok = a && b && c && ( *a == k ) && ( *b == k ) && ( *c == k );
  }
 return( ok && ! flat.find( 1 ) );
 }

/// the name of the configuration of the unit, and that of the type of its
/// ConcurrentFlatTable
std::string ADS_ABI_FN( abiName )( std::string & type )
{
 type =
   typeid( ADS::ConcurrentFlatTable< std::uint64_t , std::uint64_t > ).name();
 return( ADS_ABI_STR( ADS_ABI_NAME ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File abi_unit.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
