/*--------------------------------------------------------------------------*/
/*---------------------------- File ods_ints.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the structures and the exercises of chapters 5 (hashing), 10
 * (heaps), 11 (sorting), 12 (graphs), 13 (integers) and 14 (external
 * memory) of P. Morin, Open Data Structures (in C++), each named by the
 * section or the exercise of the book ("ods 13.2", "ods ex 13.4"), with
 * randomized differential tests against the containers of the standard
 * library.
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

#include "ADS/dictionary/BinaryTrie.h"
#include "ADS/dictionary/BitmapTrie.h"
#include "ADS/dictionary/XFastTrie.h"
#include "ADS/dictionary/YFastTrie.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// the successor of \f$ x \f$ in \f$ S \f$, the find( x ) of Morin's SSet
template< class S , class K >
std::optional< K > successorIn( const S & s , const K & x )
{
 const auto it = s.lower_bound( x );
 if( it == s.end() )
  return( std::nullopt );
 return( *it );
 }

/// the keys of W bits
template< class U , int W >
constexpr U topKey( void )
{
 if constexpr( W == std::numeric_limits< U >::digits )
  return( std::numeric_limits< U >::max() );
 else
  return( U( ( U( 1 ) << W ) - 1 ) );
 }

/// a differential test of a sorted set of keys of W bits against a
/// std::set: insertions, removals and searches drawn at random, the keys
/// with b random bits followed by W - b zeros, so that they share prefixes,
/// and the searches near them
template< class T , class U , int W >
void againstSet( T & t , int b , int ops , std::uint64_t seed )
{
 std::set< U > s;
 std::mt19937_64 g( seed );
 const int shift = W - b;
 const U mask = topKey< U , W >();
 auto key = [ & ]( void ) {
  return( U( ( U( g() ) << shift ) & mask ) );
  };
 for( int i = 0 ; i < ops ; ++i ) {
  const U x = key();
  switch( g() % 4 ) {
   case 0: case 1:
    CHECK( t.insert( x ) == s.insert( x ).second );
    break;
   case 2:
    CHECK( t.remove( x ) == ( s.erase( x ) == 1 ) );
    break;
   default:
    CHECK( t.contains( x ) == ( s.count( x ) == 1 ) );
   }
  const U y = U( ( key() + U( g() % 3 ) - U( 1 ) ) & mask );
  CHECK( t.find( y ) == successorIn( s , y ) );
  CHECK( t.size() == s.size() );
  }
 // the keys in order, and all of them removed
 std::vector< U > all( s.begin() , s.end() );
 for( const U x : all )
  CHECK( t.remove( x ) );
 CHECK( t.empty() && ! t.find( 0 ) );
 }

/// the keys of a container with an iterator of pairs, in order
template< class T >
auto keysOf( const T & t )
{
 std::vector< std::remove_cvref_t< decltype( ( *t.begin() ).first ) > > K;
 for( const auto & [ k , v ] : t )
  K.push_back( k );
 return( K );
 }

/// the tests of a trie of keys of 8, 20, 32 and 64 bits, `F::type` the
/// trie
template< class F >
void trieTests( std::uint64_t seed )
{
 using namespace ADS;
 {
  // every key of 8 bits
  typename F::template type< std::uint8_t , std::monostate , 8 > t;
  againstSet< decltype( t ) , std::uint8_t , 8 >( t , 8 , 4000 , seed );
  }
 {
  typename F::template type< std::uint32_t , std::monostate , 32 > t;
  againstSet< decltype( t ) , std::uint32_t , 32 >( t , 10 , 6000 , seed );
  }
 {
  typename F::template type< std::uint64_t , std::monostate , 64 > t;
  againstSet< decltype( t ) , std::uint64_t , 64 >( t , 64 , 3000 , seed );
  }
 {
  // w below the word
  typename F::template type< std::uint64_t , std::monostate , 20 > t;
  againstSet< decltype( t ) , std::uint64_t , 20 >( t , 12 , 3000 , seed );
  }
 {
  // the values, and the extreme keys
  typename F::template type< std::uint32_t , int , 32 > t;
  CHECK( ! t.find( 0 ) );
  CHECK( t.insert( 0xffffffffu , 7 ) && t.insert( 0 , 3 ) );
  CHECK( ! t.insert( 0 , 4 ) );
  CHECK( t.lookup( 0 ) == 4 && t.lookup( 0xffffffffu ) == 7 );
  CHECK( ! t.lookup( 5 ) );
  CHECK( t.find( 1 ) == 0xffffffffu && t.find( 0 ) == 0u );
  CHECK( t.remove( 0xffffffffu ) && ! t.find( 1 ) );
  }
 }

/// the tries as template aliases
struct MakeBinary {
 template< class U , class V , int W >
 using type = ADS::BinaryTrie< U , V , W >;
 };
struct MakeSimple {
 template< class U , class V , int W >
 using type = ADS::SimpleBinaryTrie< U , V , W >;
 };
struct MakeXFast {
 template< class U , class V , int W >
 using type = ADS::XFastTrie< U , V , W >;
 };
struct MakeListXFast {
 template< class U , class V , int W >
 using type = ADS::ListXFastTrie< U , V , W >;
 };
struct MakeYFast {
 template< class U , class V , int W >
 using type = ADS::YFastTrie< U , V , W >;
 };

/// the tests of a trie with the standard interface
template< class T >
void iteratorTests( void )
{
 T t;
 std::set< std::uint32_t > s;
 std::mt19937 g( 5 );
 for( int i = 0 ; i < 500 ; ++i ) {
  const std::uint32_t x = g() % 5000;
  t.insert( x , int( x ) * 2 );
  s.insert( x );
  }
 const auto K = keysOf( t );
 CHECK( std::vector< std::uint32_t >( s.begin() , s.end() ) == K );
 for( const auto & [ k , v ] : t )
  CHECK( v == int( k ) * 2 );
 // backwards from the end, and the lower bounds
 auto it = t.end();
 for( auto r = s.rbegin() ; r != s.rend() ; ++r )
  CHECK( ( *--it ).first == *r );
 CHECK( it == t.begin() );
 for( std::uint32_t x = 0 ; x < 5100 ; x += 7 ) {
  const auto lb = t.lowerBound( x );
  const auto sb = s.lower_bound( x );
  CHECK( ( lb == t.end() ) == ( sb == s.end() ) );
  if( sb != s.end() )
   CHECK( ( *lb ).first == *sb );
  }
 }


/*--------------------------------------------------------------------------*/
/*------------------------- CHAPTER 13, INTEGERS ---------------------------*/
/*--------------------------------------------------------------------------*/

// ods 13.1: the binary trie against a std::set, on keys of 8, 20, 32 and
// 64 bits; the iterators along the list of the leaves, the lower bounds,
// and at most n w + 1 nodes
ADS_TEST( "ods13.1.binary-trie" )
{
 trieTests< MakeBinary >( 1 );
 iteratorTests< ADS::BinaryTrie< std::uint32_t , int , 13 > >();
 ADS::BinaryTrie< std::uint16_t > t;
 for( unsigned x = 0 ; x < 1000 ; x += 3 )
  t.insert( std::uint16_t( x ) );
 CHECK( t.nodes() <= t.size() * 16 + 1 );
 CHECK( t.min() == 0 && t.max() == 999 );
 // moved, the source left empty and usable
 ADS::BinaryTrie< std::uint16_t > u( std::move( t ) );
 CHECK( u.size() == 334 && t.empty() && ! t.find( 0 ) );
 t.insert( 5 );
 CHECK( t.find( 0 ) == 5 );
 t.clear();
 CHECK( t.empty() && t.nodes() == 1 );
 }

// ods ex 13.1: the binary trie with neither the list nor the jump pointers,
// against a std::set; its nodes in order by forEach
ADS_TEST( "ods.ex13.1.simple-binary-trie" )
{
 trieTests< MakeSimple >( 2 );
 ADS::SimpleBinaryTrie< std::uint32_t , int , 12 > t;
 std::set< std::uint32_t > s;
 std::mt19937 g( 3 );
 for( int i = 0 ; i < 300 ; ++i ) {
  const std::uint32_t x = g() % 4096;
  t.insert( x , int( x ) );
  s.insert( x );
  }
 std::vector< std::uint32_t > K;
 t.forEach( [ & ]( std::uint32_t k , int v ) {
  CHECK( int( k ) == v );
  K.push_back( k );
  } );
 CHECK( K == std::vector< std::uint32_t >( s.begin() , s.end() ) );
 }

// ods 13.2: the x-fast trie against a std::set, and the binary trie with
// the same keys has the same nodes
ADS_TEST( "ods13.2.x-fast-trie" )
{
 trieTests< MakeXFast >( 3 );
 iteratorTests< ADS::XFastTrie< std::uint32_t , int , 13 > >();
 ADS::XFastTrie< std::uint32_t > x;
 ADS::BinaryTrie< std::uint32_t > b;
 std::mt19937 g( 4 );
 for( int i = 0 ; i < 2000 ; ++i ) {
  const std::uint32_t k = g();
  x.insert( k );
  b.insert( k );
  }
 CHECK( x.nodes() == b.nodes() );
 }

// ods ex 13.2: the x-fast trie of a list and w + 1 tables, against a
// std::set; one entry of the tables per node of the binary trie but the
// root
ADS_TEST( "ods.ex13.2.list-x-fast-trie" )
{
 trieTests< MakeListXFast >( 4 );
 iteratorTests< ADS::ListXFastTrie< std::uint32_t , int , 13 > >();
 ADS::ListXFastTrie< std::uint32_t > x;
 ADS::BinaryTrie< std::uint32_t > b;
 std::mt19937 g( 5 );
 for( int i = 0 ; i < 2000 ; ++i ) {
  const std::uint32_t k = g() >> ( i % 20 );
  x.insert( k );
  b.insert( k );
  if( i % 3 == 0 ) {
   x.remove( k / 2 );
   b.remove( k / 2 );
   }
  }
 CHECK( x.entries() + 1 == b.nodes() );
 }

// ods ex 13.4: findNear() of both x-fast tries equals find() on every key
// near the keys and far from them, also when the successor is past a carry
// (0111...1 and 1000...0, at distance 1, with no common prefix)
ADS_TEST( "ods.ex13.4.find-near" )
{
 ADS::XFastTrie< std::uint64_t > x;
 ADS::ListXFastTrie< std::uint64_t > l;
 CHECK( ! x.findNear( 5 ) && ! l.findNear( 5 ) );
 const std::uint64_t half = std::uint64_t( 1 ) << 63;
 x.insert( half );
 l.insert( half );
 CHECK( x.findNear( half - 1 ) == half && l.findNear( half - 1 ) == half );
 CHECK( x.findNear( 0 ) == half && l.findNear( 0 ) == half );
 CHECK( ! x.findNear( half + 1 ) && ! l.findNear( half + 1 ) );
 std::mt19937_64 g( 6 );
 std::vector< std::uint64_t > K;
 for( int i = 0 ; i < 3000 ; ++i ) {
  const std::uint64_t k = g() >> ( g() % 64 );
  x.insert( k );
  l.insert( k );
  K.push_back( k );
  }
 for( const std::uint64_t k : K )
  for( std::uint64_t d : { 0ULL , 1ULL , 2ULL , 3ULL , 100ULL , 1ULL << 20 ,
                           1ULL << 40 } ) {
   const std::uint64_t y = k - d;
   CHECK( x.findNear( y ) == x.find( y ) );
   CHECK( l.findNear( y ) == x.find( y ) );
   CHECK( l.find( y ) == x.find( y ) );
   }
 for( int i = 0 ; i < 20000 ; ++i ) {
  const std::uint64_t y = g();
  CHECK( x.findNear( y ) == x.find( y ) && l.findNear( y ) == x.find( y ) );
  }
 // a trie of w = 10 bits, every key searched
 ADS::ListXFastTrie< std::uint16_t , std::monostate , 10 > s;
 ADS::XFastTrie< std::uint16_t , std::monostate , 10 > t;
 for( unsigned k = 0 ; k < 1024 ; k += 37 ) {
  s.insert( std::uint16_t( k ) );
  t.insert( std::uint16_t( k ) );
  }
 for( unsigned y = 0 ; y < 1024 ; ++y ) {
  CHECK( s.findNear( std::uint16_t( y ) ) == s.find( std::uint16_t( y ) ) );
  CHECK( t.findNear( std::uint16_t( y ) ) == s.find( std::uint16_t( y ) ) );
  }
 }

// ods 13.3: the y-fast trie against a std::set; about n / w groups
ADS_TEST( "ods13.3.y-fast-trie" )
{
 trieTests< MakeYFast >( 5 );
 ADS::YFastTrie< std::uint32_t , int > t;
 std::mt19937 g( 7 );
 std::map< std::uint32_t , int > m;
 for( int i = 0 ; i < 64000 ; ++i ) {
  const std::uint32_t k = g();
  t.insert( k , i );
  m[ k ] = i;
  }
 CHECK( t.size() == m.size() );
 // the number of representatives is binomial, of mean n / w
 CHECK( t.groups() > 1500 && t.groups() < 2500 );
 for( const auto & [ k , v ] : m )
  CHECK( t.lookup( k ) == v );
 std::vector< std::pair< std::uint32_t , int > > P;
 t.forEach( [ & ]( std::uint32_t k , int v ) { P.emplace_back( k , v ); } );
 using Pairs = std::vector< std::pair< std::uint32_t , int > >;
 CHECK( P == Pairs( m.begin() , m.end() ) );
 ADS::YFastTrie< std::uint32_t , int > u( std::move( t ) );
 CHECK( u.size() == m.size() && t.empty() && ! t.find( 0 ) );
 }

// ods ex 13.3: the trie of strings of any length against a std::set of
// strings, the empty string, the prefixes of one another and the bytes
// above 127 included
ADS_TEST( "ods.ex13.3.bitmap-trie" )
{
 ADS::BitmapTrie< int > t;
 std::map< std::string , int > s;
 std::mt19937 g( 8 );
 auto word = [ & ]( void ) {
  std::string w( g() % 6 , 'a' );
  for( auto & c : w )
   c = "ab\x7f\x80\xff"[ g() % 5 ];
  return( w );
  };
 auto succ = [ & ]( const std::string & w ) -> std::optional< std::string > {
  const auto it = s.lower_bound( w );
  if( it == s.end() )
   return( std::nullopt );
  return( it->first );
  };
 CHECK( ! t.find( "" ) );
 for( int i = 0 ; i < 6000 ; ++i ) {
  const std::string w = word();
  switch( g() % 3 ) {
   case 0: case 1:
    CHECK( t.insert( w , i ) == ( s.count( w ) == 0 ) );
    s[ w ] = i;
    break;
   default:
    CHECK( t.remove( w ) == ( s.erase( w ) == 1 ) );
   }
  const std::string y = word();
  CHECK( t.find( y ) == succ( y ) );
  CHECK( t.contains( y ) == ( s.count( y ) == 1 ) );
  CHECK( t.size() == s.size() );
  }
 for( const auto & [ w , v ] : s )
  CHECK( t.lookup( w ) == v );
 std::vector< std::string > K;
 for( const auto & [ w , v ] : t )
  K.push_back( w );
 CHECK( K.size() == s.size() &&
        std::equal( K.begin() , K.end() , s.begin() ,
                    []( const std::string & a , const auto & b ) {
                     return( a == b.first ); } ) );
 // a long string, and all removed
 const std::string L( 100000 , 'z' );
 CHECK( t.insert( L ) && t.find( "z" ) == L );
 CHECK( t.remove( L ) );
 for( const auto & [ w , v ] : s )
  CHECK( t.remove( w ) );
 CHECK( t.empty() && t.nodes() == 1 && ! t.find( "" ) );
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
/*------------------------- End File ods_ints.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
