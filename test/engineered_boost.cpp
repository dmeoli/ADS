/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_boost.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the structures that delegate to Boost where `ADS_WITH_BOOST` is
 * defined, beyond the book: the table of pointers, the table with control
 * bytes, the vector with room inside and the generators xoshiro256++ and
 * xoshiro256**. Each test checks the contracts that hold on the code of ADS
 * and on that of Boost alike, against a reference of the standard library,
 * and that the backend is the one the macros choose; built with and
 * without `ADS_WITH_BOOST`, it covers both, at the level 3 of the
 * contracts.
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

#include "ADSTest.h"

#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/sequence/SmallVector.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <random>
#include <ranges>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined( ADS_WITH_BOOST )
 #include <boost/version.hpp>
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether the backends of Boost are on, as `ADS_WITH_BOOST` asks
#if defined( ADS_WITH_BOOST )
constexpr bool withBoost = true;
#else
constexpr bool withBoost = false;
#endif

/// whether \f$ c \f$ is a number of places of Boost, \f$ 15 \f$ times a
/// power of 2, less 1, or 0
constexpr bool boostPlaces( std::size_t c )
{
 return( ( c == 0 ) || ( ( ( c + 1 ) % 15 == 0 ) &&
			  std::has_single_bit( ( c + 1 ) / 15 ) ) );
 }

/// a node of a search: its state and its cost
struct Node
{
 std::uint64_t state;
 int g;
 };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the table of pointers against std::unordered_map, on hashes of a mix and
// on hashes whose low bits are all 0; make() called only for a new key, a
// nil one refused with nothing inserted, any equality, a table built with
// slotsFor( n ) places (and on huge pages) that takes n objects with no
// growth, reserve() with no growth up to its n and no shrink, clear()
// keeping the places, the moves, the copies sharing the objects, the views
// visiting each object once, the load factor and the places of the backend
ADS_TEST( "boost-pointer-table" )
{
 static_assert( bool( ADS_POINTERTABLE_BOOST ) == withBoost );
 static_assert(
   std::forward_iterator< PointerTable< Node >::const_iterator > );
 for( int kind = 0 ; kind < 2 ; ++kind ) {
  Arena< Node > N( 64 );
  PointerTable< Node > T;
  std::unordered_map< std::uint64_t , Node * > R;
  std::mt19937 gen( 5 + kind );
  long made = 0;
  for( int op = 0 ; op < 20000 ; ++op ) {
   const std::uint64_t s = gen() % 6000;
   const std::uint64_t h = kind ? s << 20 : splitmix64( s );
   T.prefetch( h );
   const auto [ p , fresh ] = T.find_or_insert( s , h , [ & ]( void ) {
    ++made;
    return( N.make( Node{ s , op } ) );
    } );
   const auto it = R.find( s );
   CHECK( fresh == ( it == R.end() ) );
   if( fresh )
    R[ s ] = p;
   else
    CHECK( it->second == p );
   CHECK( T.find( s , h ) == p && p->state == s );
   }
  CHECK( T.size() == R.size() && std::size_t( made ) == R.size() );
  const std::size_t c = T.capacity();
  CHECK( ADS_POINTERTABLE_BOOST
	   ? boostPlaces( c ) && T.size() * 8 <= c * 7
	   : std::has_single_bit( c ) && T.size() * 10 <= c * 7 );
  CHECK( T.bytes() >= c * 16 );
  std::set< Node * > seen;  // the views, each object once
  for( Node * p : T )
   CHECK( seen.insert( p ).second && R.at( p->state ) == p );
  std::size_t n = 0;
  T.forEach( [ & ]( Node * p ) { n += seen.count( p ); } );
  CHECK( seen.size() == R.size() && n == R.size() );
  CHECK( T.find( std::uint64_t( 999999 ) , splitmix64( 999999 ) ) == nullptr );
  PointerTable< Node > U( T );  // the copy shares the objects
  CHECK( U.size() == T.size() &&
	 U.find( std::uint64_t( 7 ) , kind ? 7 << 20 : splitmix64( 7 ) ) ==
	   T.find( std::uint64_t( 7 ) , kind ? 7 << 20 : splitmix64( 7 ) ) );
  }
 Arena< Node > M;
 auto byG = []( const Node * p , int g ) { return( p->g == g ); };
 PointerTable< Node , decltype( byG ) > G( 4 , byG );  // any equality
 auto make3 = [ & ]( void ) { return( M.make( Node{ 0 , 3 } ) ); };
 const auto a = G.find_or_insert( 3 , 3 , make3 );
 CHECK( a.second && ( G.find_or_insert( 3 , 3 , make3 ).first == a.first ) );
 auto nil = []( void ) { return( static_cast< Node * >( nullptr ) ); };
 CHECK_THROWS( G.find_or_insert( 4 , 4 , nil ) , contract_violation );
 CHECK( G.size() == 1 && G.find( 4 , 4 ) == nullptr );  // not inserted
 using T = PointerTable< Node >;
 for( std::size_t n : { std::size_t( 11 ) , std::size_t( 12 ) ,
			std::size_t( 700 ) , std::size_t( 5000 ) } ) {
  T A( T::slotsFor( n ) );  // as the search sizes its CLOSED list
  PointerTable< Node , StateEqual , HugePageAllocator< std::byte > > H(
    T::slotsFor( n ) );
  T R;
  R.reserve( n );
  const std::size_t ca = A.capacity() , ch = H.capacity() , cr = R.capacity();
  CHECK( ADS_POINTERTABLE_BOOST || ( ca == T::slotsFor( n ) ) );
  CHECK( A.bytes() <= 2 * T::slotsFor( n ) * 16 );  // about as many bytes
  for( std::uint64_t s = 0 ; s < n ; ++s ) {
   const std::uint64_t h = ( splitmix64( s ) << 3 ) | 3;  // owner 3 of 8
   for( auto * t : { &A , &R } )
    t->find_or_insert( s , h ,
		       [ & ]( void ) { return( M.make( Node{ s , 0 } ) ); } );
   CHECK( H.find_or_insert(
	     s , h , [ & ]( void ) { return( M.make( Node{ s , 0 } ) ); } )
	    .second );
   }
  CHECK( A.capacity() == ca && H.capacity() == ch && R.capacity() == cr );
  CHECK( A.size() == n && H.size() == n && R.size() == n );
  R.reserve( 1 );  // below the size: no change
  CHECK( R.capacity() == cr && R.size() == n );
  A.clear();  // the places kept
  CHECK( A.size() == 0 && A.capacity() == ca && A.begin() == A.end() );
  T P( std::move( R ) );  // moved from: empty, refilled
  CHECK( P.size() == n && R.size() == 0 && R.begin() == R.end() );
  CHECK( R.find( std::uint64_t( 1 ) , ( splitmix64( 1 ) << 3 ) | 3 ) ==
	 nullptr );
  CHECK(
    R.find_or_insert( std::uint64_t( 1 ) , 1 ,
		      [ & ]( void ) { return( M.make( Node{ 1 , 2 } ) ); } )
      .second &&
    R.size() == 1 );
  T & self = P;
  P = std::move( self );
  CHECK( P.size() == n );
  }
 }

// the table with control bytes against std::unordered_map on random
// insertions, replacements, removals and try_emplace(); a value of the table
// inserted with a new key that grows it, the iterators across removals and
// erase( it ) while iterating, the copies, the moves and the swaps, room by
// reserve() with no growth and no shrink, clear() keeping the places, and
// the places and the deleted ones of the backend
ADS_TEST( "boost-flat-hash-table" )
{
 static_assert( bool( ADS_FLATHASH_BOOST ) == withBoost );
 static_assert( std::ranges::forward_range< FlatHashTable< int , int > > );
 FlatHashTable< std::uint64_t , std::uint64_t > T;
 std::unordered_map< std::uint64_t , std::uint64_t > R;
 std::mt19937_64 g( 11 );
 for( int op = 0 ; op < 60000 ; ++op ) {
  const std::uint64_t k = g() % 4000 , v = g();
  switch( g() % 4 ) {
   case( 0 ):
   case( 1 ):
    CHECK( T.insert( k , v ) == ( R.count( k ) == 0 ) );
    R[ k ] = v;
    break;
   case( 2 ): {
    const auto [ p , fresh ] = T.try_emplace( k , v );
    CHECK( fresh == R.try_emplace( k , v ).second && *p == R[ k ] );
    ++*p;
    ++R[ k ];
    break;
    }
   default:
    CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) && ! T.contains( k ) );
   }
  const std::size_t c = T.capacity();
  CHECK( T.size() == R.size() && T.size() <= c );
  CHECK( T.deleted() <= c && ( T.size() + T.deleted() <= c ) );
  }
 for( const auto & [ k , v ] : R )
  CHECK( T.lookup( k ) == v && *T.find( k ) == v );
 for( const auto & [ k , v ] : T )
  CHECK( R.at( k ) == v );
 std::size_t n = 0;
 T.forEach( [ & ]( const std::uint64_t & k , std::uint64_t & v ) {
  n += ( R.at( k ) == v );
  } );
 CHECK( n == R.size() && T.load() == double( T.size() ) / T.capacity() );
 CHECK( ADS_FLATHASH_BOOST ? boostPlaces( T.capacity() )
			   : T.capacity() % 15 == 0 );
 {  // a value of the table, inserted as the table grows
  FlatHashTable< std::uint64_t , std::string > V;
  for( std::uint64_t k = 0 ; k < 27 ; ++k )
   V.insert( k , "a value long enough for the heap " + std::to_string( k ) );
  const std::size_t c = V.capacity();
  std::uint64_t k = 100;
  for( ; V.capacity() == c ; ++k )
   V.insert( k , *V.find( 3 ) );
  CHECK( *V.find( k - 1 ) == *V.find( 3 ) );
  }
 {  // the iterators across removals, erase( it ) while iterating
  FlatHashTable< std::uint64_t , int > I;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   I.insert( k , int( k ) );
  auto it = I.begin();
  const std::uint64_t k0 = ( *it ).first;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   if( k != k0 && k % 3 )
    I.remove( k );
  I.insert( k0 , -1 );  // there: no rebuild
  CHECK( ( *it ).first == k0 && ( *it ).second == -1 );
  for( auto e = I.begin() ; e != I.end() ; )
   if( ( *e ).first % 2 )
    e = I.erase( e );
   else
    ++e;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   CHECK( I.contains( k ) == ( ( k == k0 || k % 3 == 0 ) && k % 2 == 0 ) );
  CHECK( std::size_t( std::distance( I.begin() , I.end() ) ) == I.size() );
  for( auto e = I.begin() ; e != I.end() ; )
   e = I.erase( e );
  CHECK( I.empty() && I.begin() == I.end() );
  CHECK_THROWS( I.erase( I.end() ) , contract_violation );
  }
 {  // copies, moves, swaps, reserve(), clear()
  FlatHashTable< int , int > A( 100 );
  const std::size_t c = A.capacity();
  for( int k = 0 ; k < 100 ; ++k )
   A.insert( k , -k );
  CHECK( A.capacity() == c && A.size() == 100 );
  A.reserve( 10 );
  CHECK( A.capacity() == c );
  FlatHashTable< int , int > B( A ) , C;  // the same places
  CHECK( B.size() == 100 && B.lookup( 7 ) == -7 && B.capacity() == c );
  C = B;
  B.clear();
  CHECK( B.empty() && B.capacity() > 0 && ! B.contains( 7 ) );
  B.swap( C );
  CHECK( B.size() == 100 && C.empty() );
  FlatHashTable< int , int > D( std::move( B ) );
  CHECK( D.size() == 100 && B.empty() && B.capacity() == 0 &&
	 B.begin() == B.end() && ! B.lookup( 7 ) );
  CHECK( B.insert( 7 , 7 ) && B.lookup( 7 ) == 7 );
  FlatHashTable< int , int > E;
  CHECK( E.capacity() == 0 && E.load() == 0.0 && E.deleted() == 0 );
  }
 }

// the vector with room inside: the items inside until N of them, then on
// the allocator with the room doubled, a push of one of its own items as
// it moves out, at most 2 n + N items of room, the pointers as iterators,
// the moves leaving an empty vector inside, the copies, swap(), resize()
// and the contracts of the indices and of the ends
ADS_TEST( "boost-small-vector" )
{
 static_assert( bool( ADS_SMALLVECTOR_BOOST ) == withBoost );
 static_assert( std::same_as< SmallVector< int , 4 >::iterator , int * > );
 SmallVector< int , 4 > E;
 CHECK( E.empty() && E.small() && E.capacity() == 4 );
 CHECK_THROWS( ( void )E.front() , contract_violation );
 CHECK_THROWS( ( void )E.back() , contract_violation );
 CHECK_THROWS( E.pop_back() , contract_violation );
 CHECK_THROWS( ( void )E[ 0 ] , contract_violation );
 for( int i = 0 ; i < 4 ; ++i )
  E.push_back( i );
 CHECK( E.small() && E.capacity() == 4 );
 E.push_back( E[ 2 ] );  // out, with an item of its own
 CHECK( ! E.small() && E.capacity() == 8 && E.back() == 2 );
 for( int i = 5 ; i < 1000 ; ++i ) {
  E.emplace_back( E.front() + i );
  CHECK( E.capacity() <= 2 * E.size() + 4 && E.back() == i );
  }
 CHECK( std::ranges::equal( std::ranges::subrange( E.begin() + 5 , E.end() ) ,
			    std::views::iota( 5 , 1000 ) ) );
 SmallVector< int , 4 > M( std::move( E ) );  // the memory taken
 CHECK( E.empty() && E.small() && M.size() == 1000 );
 SmallVector< int , 4 > C( M ) , S{ 1 , 2 };
 CHECK( C == M && S.small() && S.size() == 2 );
 C.swap( S );
 CHECK( C.size() == 2 && S == M );
 S = C;
 CHECK( S.size() == 2 && S[ 1 ] == 2 );
 S = std::move( M );
 CHECK( S.size() == 1000 && M.empty() && M.small() );
 S.resize( 3 );
 CHECK( S.size() == 3 && S[ 2 ] == 2 );
 S.resize( 2000 , S[ 1 ] );  // an item of its own
 CHECK( S.size() == 2000 && S[ 1999 ] == 1 && S.capacity() >= 2000 );
 S.clear();
 CHECK( S.empty() && S.capacity() >= 2000 );
 SmallVector< std::string , 2 > W( 5 , std::string( 30 , 'w' ) );
 W.pop_back();
 CHECK( W.size() == 4 && ! W.small() && W.back() == std::string( 30 , 'w' ) );
 std::vector< int > V{ 9 , 8 , 7 };
 SmallVector< int , 8 > F( V.begin() , V.end() );
 CHECK( F.small() && F.size() == 3 && F[ 0 ] == 9 && F.data()[ 2 ] == 7 );
 }

// xoshiro256++ and xoshiro256** against the reference algorithms of
// Blackman and Vigna on the words of SplitMix64 of the seed (the seed 0
// included, which Boost would take for another one), the default seed 1,
// the jump and the equality, and the delegation to Boost.Random from
// Boost 1.86 on
ADS_TEST( "boost-xoshiro" )
{
#if defined( ADS_WITH_BOOST )
 static_assert( bool( ADS_RANDOM_BOOST ) == ( BOOST_VERSION >= 108600 ) );
#else
 static_assert( ! ADS_RANDOM_BOOST );
#endif
 static_assert( FullWordGenerator< Xoshiro256 > &&
		FullWordGenerator< Xoshiro256StarStar > );
 for( std::uint64_t seed : { 0ULL , 1ULL , 123456789ULL } ) {
  std::uint64_t a[ 4 ] , x = seed;
  for( auto & w : a ) {
   w = splitmix64( x );
   x += 0x9e3779b97f4a7c15ULL;
   }
  std::uint64_t b[ 4 ] = { a[ 0 ] , a[ 1 ] , a[ 2 ] , a[ 3 ] };
  auto step = []( std::uint64_t * q ) {
   const std::uint64_t t = q[ 1 ] << 17;
   q[ 2 ] ^= q[ 0 ];
   q[ 3 ] ^= q[ 1 ];
   q[ 1 ] ^= q[ 2 ];
   q[ 0 ] ^= q[ 3 ];
   q[ 2 ] ^= t;
   q[ 3 ] = std::rotl( q[ 3 ] , 45 );
   };
  Xoshiro256 P( seed );
  Xoshiro256StarStar S( seed );
  for( int i = 0 ; i < 1000 ; ++i ) {
   CHECK( P() == std::rotl( a[ 0 ] + a[ 3 ] , 23 ) + a[ 0 ] );
   step( a );
   CHECK( S() == std::rotl( b[ 1 ] * 5 , 7 ) * 9 );
   step( b );
   }
  }
 Xoshiro256 d , e( 1 ) , j( 5 ) , k( 5 );
 CHECK( d == e && d() == e() );
 j.jump();
 k.jump();
 CHECK( j == k && ! ( j == Xoshiro256( 5 ) ) && j() == k() );
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
/*---------------------- End File engineered_boost.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
