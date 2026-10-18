/*--------------------------------------------------------------------------*/
/*---------------------- File contract_dictionary.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the dictionaries and of the
 * hash functions: every operation of the specification of the book, its
 * result as the specification states it, and the violations of its
 * preconditions, which must throw ADS::contract_violation. The tests are
 * compiled at the level 3, so that each operation also checks its
 * postconditions and the invariant of the table, on random sequences of
 * operations against std::map.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.1-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
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

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/CuckooHashing.h"
#include "ADS/dictionary/DirectTable.h"
#include "ADS/dictionary/HashFunctions.h"
#include "ADS/dictionary/KnuthHashing.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/PatriciaTrie.h"
#include "ADS/dictionary/PerfectHashing.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/dictionary/TableSize.h"
#include "ADS/dictionary/UniversalHashing.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/SequenceAlgorithms.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/Treap.h"

#include <cstdint>
#include <map>
#include <random>
#include <set>
#include <string>
#include <utility>
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

// a random sequence of insert(), remove() and lookup() on d against
// std::map, keys in [ 0 , K )
template< class D >
void against_map( D & d , long K , int ops , unsigned seed )
{
 std::mt19937 g( seed );
 std::map< long , long > ref;
 for( int o = 0 ; o < ops ; ++o ) {
  const long k = long( g() % K );
  switch( g() % 3 ) {
   case( 0 ):
    d.insert( k , o );
    ref[ k ] = o;
    break;
   case( 1 ):
    d.remove( k );
    ref.erase( k );
    break;
   default: {
    const auto v = d.lookup( k );
    const auto it = ref.find( k );
    CHECK( v.has_value() == ( it != ref.end() ) );
    if( v ) CHECK( *v == it->second );
    }
   }
  CHECK( d.size() == ref.size() );
  }
 }

// the hash functions: the ranges of p, the length of a key for int_bin(),
// the size and the base of the division violated; every result below its
// size
ADS_TEST( "hash-functions" )
{
 CHECK_THROWS( (void) int_bin( "123456789" ) , contract_violation );
 CHECK_THROWS( (void) hash_extraction( "ab" , 0 , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_extraction( "ab" , 64 , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_xor( "ab" , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_division( "ab" , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_division( "ab" , 7 , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_division( "ab" , ~0ull , 256 ) ,
               contract_violation );
 CHECK_THROWS( (void) hash_multiplication( 5 , 8 , 1.5L ) ,
               contract_violation );
 CHECK_THROWS( (void) hash_fibonacci( 5 , 64 ) , contract_violation );
 CHECK_THROWS( (void) hash_midsquare( 5 , 33 ) , contract_violation );
 std::mt19937_64 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const std::uint64_t i = g();
  const int p = 1 + int( g() % 32 );
  CHECK( hash_fibonacci( i , p ) < ( 1ull << p ) );
  CHECK( hash_midsquare( i , p ) < ( 1ull << p ) );
  CHECK( hash_multiplication( i % 100000 , 97 ) < 97 );
  CHECK( hash_division( "chiave" , 1 + g() % 1000 ) < 1001 );
  }
 }

// the sizes of the restructurings: never below m0, powers of two kept,
// primes otherwise; a size below m0, and m0 = 0, refused
ADS_TEST( "table-size" )
{
 CHECK_THROWS( (void) resized( 3 , true , 7 ) , contract_violation );
 CHECK_THROWS( (void) resized( 3 , true , 0 ) , contract_violation );
 for( std::size_t m = 7 ; m < 3000 ; ++m )
  for( bool grow : { true , false } ) {
   const std::size_t r = resized( m , grow , 7 );
   CHECK( r >= 7 );
   if( is_pow2( m ) ) CHECK( is_pow2( r ) || r == 7 );
   else CHECK( is_prime( r ) || r == 7 );
   }
 }

// the table with direct access: an empty interval and the keys out of it
// refused, and random operations against std::map
ADS_TEST( "direct-table" )
{
 CHECK_THROWS( DirectTable< int >( 5 , 4 ) , contract_violation );
 DirectTable< long > D( -10 , 89 );
 CHECK_THROWS( (void) D.lookup( -11 ) , contract_violation );
 CHECK_THROWS( D.insert( 90 , 0 ) , contract_violation );
 CHECK_THROWS( D.remove( 1000 ) , contract_violation );
 D.insert( -10 , 3 );
 CHECK( D.lookup( -10 ) == 3 && D.size() == 1 && D.capacity() == 100 );
 DirectTable< long > E( 0 , 99 );
 against_map( E , 100 , 5000 , 2 );
 }

// the hash table with lists of overflow, its three kinds of list, with
// and without restructuring: m = 0 and a list that does not exist refused,
// and random operations against std::map, each checking that every key is
// in the list of its place
ADS_TEST( "chained-hash-table" )
{
 CHECK_THROWS( ( ChainedHashTable< long , long >( 0 ) ) ,
               contract_violation );
 ChainedHashTable< long , long > T( 5 );
 CHECK_THROWS( (void) T.chain( 5 ) , contract_violation );
 unsigned seed = 3;
 for( bool r : { false , true } ) {
  ChainedHashTable< long , long , Chain::FRONT > a( 7 , r );
  ChainedHashTable< long , long , Chain::MOVE_TO_FRONT > b( 7 , r );
  ChainedHashTable< long , long , Chain::SORTED > c( 7 , r );
  against_map( a , 300 , 3000 , seed++ );
  against_map( b , 300 , 3000 , seed++ );
  against_map( c , 300 , 3000 , seed++ );
  }
 }

// the hash table with open addressing: m = 0 and the inspections that do
// not work with m refused, keyAt() of a place not full refused, and random
// operations against std::map with each inspection, with and without
// restructuring
ADS_TEST( "open-hash-table" )
{
 using Open = OpenHashTable< long , long >;
 CHECK_THROWS( ( Open( 0 , Probing::linear() ) ) , contract_violation );
 CHECK_THROWS( ( Open( 8 , Probing::linear( 4 ) ) ) , contract_violation );
 CHECK_THROWS( ( Open( 8 , Probing::quadratic( 1 , 1 ) ) ) ,
               contract_violation );
 CHECK_THROWS( ( Open( 12 , Probing::double_hashing() ) ) ,
               contract_violation );
 CHECK_THROWS( ( Open( 12 , Probing::triangular() ) ) , contract_violation );
 CHECK( Probing::linear( 3 ).works( 8 ) && ! Probing::linear( 2 ).works( 8 ) );
 CHECK_THROWS( (void) Probing::linear().at( 1 , 0 , 0 ) ,
               contract_violation );
 Open T( 7 , Probing::linear() );
 CHECK_THROWS( (void) T.keyAt( 0 ) , contract_violation );
 CHECK_THROWS( (void) T.slot( 7 ) , contract_violation );
 T.insert( 3 , 1 );
 std::size_t full = 0;
 for( std::size_t i = 0 ; i < T.capacity() ; ++i )
  if( T.slot( i ) == Open::Slot::FULL ) {
   CHECK( T.keyAt( i ) == 3 );
   ++full;
   }
 CHECK( full == 1 );
 unsigned seed = 9;
 for( bool r : { false , true } ) {
  for( auto p : { Probing::linear() , Probing::linear( 3 ) ,
                  Probing::quadratic( 1 , 1 ) , Probing::double_hashing() } ) {
   Open d( r ? 7 : 401 , p , r );
   against_map( d , 150 , 3000 , seed++ );
   }
  for( auto p : { Probing::triangular() , Probing::double_hashing() } ) {
   Open e( r ? 8 : 512 , p , r );
   against_map( e , 150 , 3000 , seed++ );
   }
  }
 }

// the axioms of DICTIONARY [see ADT.h] on every realization, for
// dictionaries D built by random operations and replayed for each side:
// lookup( k , create ) = nil, lookup( k , insert( k' , v , D ) ) is v if
// k = k' and lookup( k , D ) otherwise, lookup( k , remove( k' , D ) ) is
// nil if k = k' and lookup( k , D ) otherwise
template< class D , class F >
void dictionary_axioms( F create , unsigned seed )
{
 std::mt19937 g( seed );
 const int U = 12;                              // the keys in [ 0 , U )
 {
  D E = create();
  for( int k = 0 ; k < U ; ++k ) CHECK( ! E.lookup( k ) );
  }
 std::vector< std::pair< int , int > > ops;     // ( k , v ), v < 0 removes
 auto build = [ & ]( void ) {
  D A = create();
  for( auto [ k , v ] : ops )
   if( v < 0 ) A.remove( k ); else A.insert( k , v );
  return( A );
  };
 for( int t = 0 ; t < 40 ; ++t ) {
  const int kk = int( g() % U ) , v = int( g() % 100 );
  for( bool in : { true , false } ) {
   D A = build() , B = build();
   if( in ) B.insert( kk , v ); else B.remove( kk );
   for( int k = 0 ; k < U ; ++k )
    if( k == kk )
     CHECK( in ? ( B.lookup( k ) == v ) : ! B.lookup( k ) );
    else
     CHECK( B.lookup( k ) == A.lookup( k ) );
   }
  ops.emplace_back( kk , g() % 3 ? v : -1 );
  }
 }

ADS_TEST( "dictionary-axioms" )
{
 using CF = ChainedHashTable< int , int , Chain::FRONT >;
 using CM = ChainedHashTable< int , int , Chain::MOVE_TO_FRONT >;
 using CS = ChainedHashTable< int , int , Chain::SORTED >;
 using O = OpenHashTable< int , int >;
 dictionary_axioms< CF >( [] { return( CF( 5 , true ) ); } , 11 );
 dictionary_axioms< CM >( [] { return( CM( 5 ) ); } , 12 );
 dictionary_axioms< CS >( [] { return( CS( 3 ) ); } , 13 );
 dictionary_axioms< O >( [] { return( O( 31 , Probing::linear() ) ); } , 14 );
 dictionary_axioms< O >( [] {
  return( O( 31 , Probing::quadratic( 0 , 1 ) ) ); } , 15 );
 dictionary_axioms< O >( [] {
  return( O( 31 , Probing::double_hashing() , true ) ); } , 16 );
 dictionary_axioms< O >( [] {
  return( O( 32 , Probing::triangular() ) ); } , 17 );
 dictionary_axioms< O >( [] {
  return( O( 12 , Probing::pseudo_random() , true ) ); } , 22 );
 dictionary_axioms< DirectTable< int > >( [] {
  return( DirectTable< int >( 0 , 11 ) ); } , 18 );
 using BST = BinarySearchTree< int , int >;
 using RB = RedBlackTree< int , int >;
 dictionary_axioms< BST >( [] { return( BST() ); } , 19 );
 dictionary_axioms< RB >( [] { return( RB() ); } , 20 );
 using SA = SortedArrayDictionary< int , int >;
 dictionary_axioms< SA >( [] { return( SA() ); } , 21 );
 using AV = AVLTree< int , int >;
 dictionary_axioms< AV >( [] { return( AV() ); } , 23 );
 using BT = BTree< int , int >;
 dictionary_axioms< BT >( [] { return( BT( 2 ) ); } , 24 );
 using SL = SkipList< int , int >;
 dictionary_axioms< SL >( [] { return( SL( 5 ) ); } , 25 );
 using TR = Treap< int , int >;
 dictionary_axioms< TR >( [] { return( TR( 6 ) ); } , 26 );
 using SP = SplayTree< int , int >;
 dictionary_axioms< SP >( [] { return( SP() ); } , 27 );
 }

// the other dictionaries and hash functions, each bound just inside and
// just outside: the cuckoo table with m = 1 , 2 and the load at 0 and
// 0.5, the coalesced table full, Brent's table of m = 1 , 2 , 4 and full,
// the Patricia trie empty, the perfect hash of a key at 2^61 - 1 and of
// repeated keys, r and c of the minimal one, the Zobrist table at n and m,
// the moduli of the universal hashing at their bounds, the multiply-shift
// hash with l = h, h = 65 and an even multiplier, the d-left table with
// m , d = 0
ADS_TEST( "other-dictionaries" )
{
 using CK = CuckooHashTable< int , int >;
 CHECK_THROWS( CK( 1 ) , contract_violation );
 CHECK_THROWS( CK( 8 , 0.0 ) , contract_violation );
 CHECK_THROWS( CK( 8 , 0.5 ) , contract_violation );
 CK C( 2 , 0.49 );                            // the least table: it grows
 for( int k = 0 ; k < 50 ; ++k ) C.insert( k , -k );
 CHECK( C.size() == 50 && C.lookup( 49 ) == -49 && ! C.lookup( 50 ) );
 C.remove( 7 );
 C.remove( 7 );
 CHECK( C.size() == 49 && ! C.contains( 7 ) );

 using CO = CoalescedHashTable< int , int >;
 CHECK_THROWS( CO( 0 ) , contract_violation );
 CO K( 1 , 1 );                               // one place and one cellar
 K.insert( 1 , 1 );
 K.insert( 2 , 2 );
 K.insert( 1 , 9 );                           // the key again: no room
 CHECK( K.size() == 2 && K.lookup( 1 ) == 9 && K.lookup( 2 ) == 2 );
 CHECK_THROWS( K.insert( 3 , 3 ) , contract_violation );   // full
 using BR = BrentHashTable< int , int >;
 CHECK_THROWS( BR( 1 ) , contract_violation );
 CHECK_THROWS( BR( 4 ) , contract_violation );            // not prime
 BR B( 2 );
 B.insert( 5 , 5 );
 B.insert( 6 , 6 );
 CHECK( B.size() == 2 && B.lookup( 6 ) == 6 && ! B.lookup( 7 ) );
 CHECK_THROWS( B.insert( 7 , 7 ) , contract_violation );

 const PatriciaTrie P0( std::vector< std::string >{} ) ,
                    P1( std::vector< std::string >{ "" , "a" , "ab" } );
 CHECK_THROWS( (void) P0.blindLeaf( "a" ) , contract_violation );
 CHECK_THROWS( (void) P0.string( 0 ) , contract_violation );
 CHECK_THROWS( (void) P1.string( 3 ) , contract_violation );
 CHECK( P1.string( 2 ) == "ab" && P1.contains( "" ) && ! P1.contains( "b" ) &&
        P0.prefixEnd( "" ) == 0 && P1.prefixEnd( "a" ) == 3 &&
        P1.blindLeaf( "zz" ) < 3 );

 CHECK_THROWS( PerfectHashTable( { kMersenne61 } ) , contract_violation );
 CHECK_THROWS( PerfectHashTable( { 4 , 4 } ) , contract_violation );
 const PerfectHashTable PH( { 0 , kMersenne61 - 1 } ) , PE( {} );
 CHECK( PH.contains( 0 ) && PH.contains( kMersenne61 - 1 ) &&
        ! PH.contains( 1 ) && PE.size() == 0 && ! PE.contains( 0 ) );
 using OM = OrderedMinimalPerfectHash< int >;
 CHECK_THROWS( OM( { 1 , 2 } , 2 , 2.0 ) , contract_violation );
 CHECK_THROWS( OM( { 1 , 2 } , 3 , 1.23 ) , contract_violation );
 CHECK_THROWS( OM( { 1 , 2 } , 4 ) , contract_violation );
 CHECK_THROWS( OM( { 1 , 1 } ) , contract_violation );
 const OM O2( { 7 , 3 , 9 } , 2 , 2.01 ) , O3( { 7 , 3 , 9 } , 3 , 1.24 ) ,
          O0( std::vector< int >{} );
 CHECK( O2( 7 ) == 0 && O2( 9 ) == 2 && O3( 3 ) == 1 && O0.size() == 0 );
 CHECK_THROWS( (void) O0( 1 ) , contract_violation );

 CHECK_THROWS( (void) fibonacciPlace( 5 , 0 ) , contract_violation );
 CHECK_THROWS( (void) fibonacciPlace( 5 , 65 ) , contract_violation );
 CHECK( fibonacciPlace( 5 , 1 ) < 2 && fibonacciPlace( ~0ull , 64 ) >= 0 );
 CHECK_THROWS( ZobristTable( 0 , 1 ) , contract_violation );
 CHECK_THROWS( ZobristTable( 1 , 0 ) , contract_violation );
 const ZobristTable Z( 2 , 3 );
 (void) Z( 1 , 2 );
 CHECK_THROWS( (void) Z( 2 , 0 ) , contract_violation );
 CHECK_THROWS( (void) Z( 0 , 3 ) , contract_violation );
 CHECK( Z.hash( std::vector< int >{ 0 , 2 } ) == ( Z( 0 , 0 ) ^ Z( 1 , 2 ) ) );
 CHECK_THROWS( (void) Z.hash( std::vector< int >{ 0 } ) , contract_violation );

 CHECK_THROWS( (void) mulMod61( kMersenne61 , 1 ) , contract_violation );
 CHECK( mulMod61( kMersenne61 - 1 , kMersenne61 - 1 ) == 1 );
 const std::uint64_t top = ( std::uint64_t( 1 ) << 63 ) - 1;
 CHECK( mulMod( top - 1 , top - 1 , top ) == 1 && mulMod( 0 , 0 , 1 ) == 0 );
 CHECK( mulMod( 1 , 1 , top + 1 ) == 1 && mulMod( 5 , 1 , 5 ) == 0 );
 CHECK_THROWS( (void) mulMod( 1 , 1 , 0 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 0 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 1 , 1 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( top + 1 , 1 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 7 , 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 7 , 7 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 7 , 1 , 7 , 1 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 7 , 1 , 0 , 0 ) , contract_violation );
 const CarterWegmanHash CW( 7 , 6 , 6 , 1 );
 CHECK( CW( 6 ) == 0 );
 CHECK_THROWS( (void) CW( 7 ) , contract_violation );
 CHECK_THROWS( DotProductHash( 1 , 2 ) , contract_violation );
 CHECK_THROWS( DotProductHash( ( std::uint64_t( 1 ) << 32 ) + 1 , 2 ) ,
               contract_violation );
 CHECK_THROWS( DotProductHash( 5 , 0 ) , contract_violation );
 CHECK_THROWS( DotProductHash( 5 , std::vector< std::uint64_t >{} ) ,
               contract_violation );
 CHECK_THROWS( DotProductHash( 5 , std::vector< std::uint64_t >{ 5 } ) ,
               contract_violation );
 const DotProductHash DP( 5 , std::vector< std::uint64_t >{ 4 , 1 } ) ,
                      DB( std::uint64_t( 1 ) << 32 , 3 );
 CHECK( DP.universe() == 25 && DP( 24 ) < 5 && DB.universe() == 0 &&
        DB( ~0ull ) < ( std::uint64_t( 1 ) << 32 ) );
 CHECK_THROWS( (void) DP( 25 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash( 8 , 8 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash( 8 , 0 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash( 65 , 1 ) , contract_violation );
 const MultiplyShiftHash M64( 64 , 63 ) ,
                         M8 = MultiplyShiftHash::ofMultiplier( 8 , 3 , 255 );
 CHECK( M64( ~0ull ) < ( std::uint64_t( 1 ) << 63 ) && M8( 255 ) < 8 );
 CHECK_THROWS( (void) M8( 256 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash::ofMultiplier( 8 , 3 , 2 ) ,
               contract_violation );
 CHECK_THROWS( MultiplyShiftHash::ofMultiplier( 8 , 3 , 257 ) ,
               contract_violation );
 using DL = DLeftHashTable< int , int >;
 CHECK_THROWS( DL( 0 , 2 ) , contract_violation );
 CHECK_THROWS( DL( 4 , 0 ) , contract_violation );
 DL L( 1 , 3 );                               // m < d: one bucket each
 L.insert( 1 , 1 );
 CHECK( L.lookup( 1 ) == 1 && ! L.lookup( 2 ) );
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
/*-------------------- End File contract_dictionary.cpp --------------------*/
/*--------------------------------------------------------------------------*/
