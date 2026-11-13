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
#include "ADS/memory/PoolAllocator.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/MeldableHeap.h"
#include "ADS/priority/MultiwayMerge.h"

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


/*--------------------------------------------------------------------------*/
/*--------------------------- CHAPTER 10, HEAPS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the binary heap of figures 10.2 and 10.3 of Morin's book, after the
/// insertion of 6: pushed in the order of its array, which is a heap, no
/// item moves
ADS::DaryHeap< int , std::less< int > , 2 > figureHeap( void )
{
 ADS::DaryHeap< int , std::less< int > , 2 > h;
 for( int x : { 4 , 9 , 8 , 17 , 26 , 50 , 16 , 19 , 69 , 32 , 93 , 55 } )
  h.push( x );
 return( h );
 }

/// the array of a heap
template< class H >
std::vector< int > arrayOf( const H & h )
{
 return( std::vector< int >( h.begin() , h.end() ) );
 }

// ods 10.1 and ods ex 10.1, ex 10.2: the binary heap of figure 10.2, the
// insertion of 6 of the figure, then those of 7 and 3 (exercise 10.1); the
// removal of 4 of figure 10.3, then those of 6 and 8 (exercise 10.2), each
// array as the book computes it
ADS_TEST( "ods10.1.binary-heap" )
{
 auto h = figureHeap();
 h.push( 6 );
 CHECK( arrayOf( h ) == ( std::vector< int >{ 4 , 9 , 6 , 17 , 26 , 8 , 16 ,
                                              19 , 69 , 32 , 93 , 55 , 50 } ) );
 auto r = h;
 h.push( 7 );
 CHECK( arrayOf( h ) == ( std::vector< int >{ 4 , 9 , 6 , 17 , 26 , 8 , 7 ,
                                              19 , 69 , 32 , 93 , 55 , 50 ,
                                              16 } ) );
 h.push( 3 );
 CHECK( arrayOf( h ) == ( std::vector< int >{ 3 , 9 , 4 , 17 , 26 , 8 , 6 ,
                                              19 , 69 , 32 , 93 , 55 , 50 ,
                                              16 , 7 } ) );
 CHECK( r.pop() == 4 );
 CHECK( arrayOf( r ) == ( std::vector< int >{ 6 , 9 , 8 , 17 , 26 , 50 , 16 ,
                                              19 , 69 , 32 , 93 , 55 } ) );
 CHECK( r.pop() == 6 );
 CHECK( arrayOf( r ) == ( std::vector< int >{ 8 , 9 , 16 , 17 , 26 , 50 , 55 ,
                                              19 , 69 , 32 , 93 } ) );
 CHECK( r.pop() == 8 );
 CHECK( arrayOf( r ) == ( std::vector< int >{ 9 , 17 , 16 , 19 , 26 , 50 , 55 ,
                                              93 , 69 , 32 } ) );
 }

/// a differential test of a heap against a std::multiset, `erase( i )` of
/// a random place included
template< class H >
void heapAgainstMultiset( H & h , int ops , std::uint64_t seed )
{
 std::multiset< int > s;
 std::mt19937_64 g( seed );
 for( int i = 0 ; i < ops ; ++i ) {
  const auto c = g() % 8;
  if( ( c < 4 ) || s.empty() ) {
   const int x = int( g() % 1000 );
   h.push( x );
   s.insert( x );
   }
  else if( c < 6 ) {
   CHECK( h.pop() == *s.begin() );
   s.erase( s.begin() );
   }
  else {
   const std::size_t j = g() % h.size();
   const int x = h.erase( j );
   CHECK( s.count( x ) > 0 );
   s.erase( s.find( x ) );
   }
  CHECK( h.size() == s.size() );
  if( ! s.empty() )
   CHECK( h.top() == *s.begin() );
  if( s.size() >= 2 )
   CHECK( *h.secondTop() == *std::next( s.begin() ) );
  else
   CHECK( h.secondTop() == nullptr );
  }
 }

// ods ex 10.3: the removal of the item in a place, against a
// std::multiset, for 2, 3 and 4 children; the places of the parent and of
// the children of the general layout of Eytzinger (exercise 10.4) are
// inverse to each other
ADS_TEST( "ods.ex10.3.remove-place" )
{
 ADS::DaryHeap< int , std::less< int > , 2 > h2;
 ADS::DaryHeap< int , std::less< int > , 3 > h3;
 ADS::DaryHeap< int , std::less< int > , 4 , true > h4;
 heapAgainstMultiset( h2 , 20000 , 1 );
 heapAgainstMultiset( h3 , 20000 , 2 );
 heapAgainstMultiset( h4 , 20000 , 3 );
 using H5 = ADS::DaryHeap< int , std::less< int > , 5 >;
 for( std::size_t i = 0 ; i < 1000 ; ++i )
  for( std::size_t j = 0 ; j < 5 ; ++j )
   CHECK( H5::parent( H5::child( i , j ) ) == i &&
          H5::child( i , j ) == 5 * i + j + 1 );
 }

// ods ex 10.4 and ex 10.5: the d-ary heap sorts as the binary one for
// d = 2 to 8, and its depth is that of the complete d-ary tree,
// ceil( log_d( ( d - 1 ) n + 1 ) ) levels
template< unsigned D >
void dary( void )
{
 ADS::DaryHeap< int , std::less< int > , D > h;
 std::mt19937 g( D );
 std::vector< int > v( 5000 );
 for( auto & x : v )
  x = int( g() % 100000 );
 h.assign( v.begin() , v.end() );
 std::sort( v.begin() , v.end() );
 for( const int x : v )
  CHECK( h.pop() == x );
 // n = 1 + D + ... + D^( k - 1 ) items fill k levels exactly
 std::size_t n = 0 , level = 1 , k = 0;
 for( ; n + level <= 5000 ; n += level , level *= D )
  ++k;
 std::size_t last = n - 1;   // the last place, at depth k - 1
 std::size_t d = 0;
 for( ; last > 0 ; last = h.parent( last ) )
  ++d;
 CHECK( d + 1 == k );
 }

ADS_TEST( "ods.ex10.5.dary-heap" )
{
 dary< 2 >();
 dary< 3 >();
 dary< 4 >();
 dary< 5 >();
 dary< 8 >();
 }

/// a differential test of the meldable heap against a std::multiset, with
/// pushes, pops, removals by handle and melds
template< class H >
void meldableAgainstMultiset( H & h , H & o , int ops , std::uint64_t seed )
{
 std::multiset< int > s , t;
 std::vector< typename H::Handle > handles;
 std::mt19937_64 g( seed );
 for( int i = 0 ; i < ops ; ++i ) {
  const auto c = g() % 10;
  if( ( c < 4 ) || s.empty() ) {
   const int x = int( g() % 1000 );
   handles.push_back( h.push( x ) );
   s.insert( x );
   }
  else if( c < 6 ) {
   CHECK( h.pop() == *s.begin() );
   s.erase( s.begin() );
   handles.clear();   // the handles may be stale
   }
  else if( c < 8 ) {
   if( ! handles.empty() ) {
    const int x = h.erase( handles.back() );
    handles.pop_back();
    CHECK( s.count( x ) > 0 );
    s.erase( s.find( x ) );
    }
   }
  else if( c < 9 ) {
   const int x = int( g() % 1000 );
   o.push( x );
   t.insert( x );
   }
  else {
   h.meld( o );
   s.insert( t.begin() , t.end() );
   t.clear();
   CHECK( o.empty() );
   }
  CHECK( h.size() == s.size() );
  if( ! s.empty() )
   CHECK( h.top() == *s.begin() );
  if( s.size() >= 2 )
   CHECK( *h.secondTop() == *std::next( s.begin() ) );
  }
 }

// ods 10.2, ods ex 10.8 and ex 10.9: the randomized meldable heap against a
// std::multiset, with the removal by handle (exercise 10.8) and the second
// smallest item (exercise 10.9), on the default allocator and on a pool;
// the keys of figure 10.5 with the operations of exercises 10.6 and 10.7
ADS_TEST( "ods10.2.meldable-heap" )
{
 {
  ADS::MeldableHeap< int > h , o;
  meldableAgainstMultiset( h , o , 30000 , 1 );
  }
 {
  ADS::PoolResource<> R;
  using PA = ADS::PoolAllocator< int >;
  ADS::MeldableHeap< int , std::less< int > , PA > h( {} , 1 , PA( R ) ) ,
                                                    o( {} , 2 , PA( R ) );
  meldableAgainstMultiset( h , o , 30000 , 2 );
  }
 ADS::MeldableHeap< int > h;
 for( int x : { 19 , 17 , 9 , 26 , 4 , 55 , 16 , 25 , 32 , 28 , 93 , 19 , 99 ,
                89 , 20 , 50 , 8 } )
  h.push( x );
 h.push( 17 );   // exercise 10.6
 h.push( 82 );
 CHECK( h.pop() == 4 && h.pop() == 8 );   // exercise 10.7
 CHECK( h.top() == 9 && *h.secondTop() == 16 && h.size() == 17 );
 // a heap of 10^6 items in increasing order: the expected depth stays
 // logarithmic, the destructor and clear() with no recursion
 ADS::MeldableHeap< int > big;
 for( int x = 0 ; x < 1000000 ; ++x )
  big.push( x );
 CHECK( big.pop() == 0 && big.size() == 999999 );
 big.clear();
 CHECK( big.empty() );
 }

// ods ex 10.10: the k smallest items in O( k log k ), from the binary, the
// 4-ary and the meldable heap, against the sorted items
ADS_TEST( "ods.ex10.10.k-smallest" )
{
 std::mt19937 g( 9 );
 std::vector< int > v( 3000 );
 for( auto & x : v )
  x = int( g() % 500 );
 ADS::DaryHeap< int , std::less< int > , 2 > b;
 ADS::DaryHeap< int , std::less< int > , 4 > d;
 ADS::MeldableHeap< int > m;
 for( const int x : v ) {
  b.push( x );
  d.push( x );
  m.push( x );
  }
 std::vector< int > w = v;
 std::sort( w.begin() , w.end() );
 for( std::size_t k : { 0 , 1 , 2 , 7 , 100 , 2999 , 3000 , 5000 } ) {
  const std::vector< int > e( w.begin() ,
                              w.begin() + std::ptrdiff_t( std::min( k ,
                                                          w.size() ) ) );
  CHECK( b.smallest( k ) == e && d.smallest( k ) == e &&
         m.smallest( k ) == e );
  }
 CHECK( b.size() == 3000 );   // unchanged
 }

// ods ex 10.11: k sorted lists merged by a heap of their heads, the head of
// the smallest list replaced by its next item, O( n log k ), equal to the
// merge of multiwayMerge()
ADS_TEST( "ods.ex10.11.k-way-merge" )
{
 std::mt19937 g( 10 );
 const std::size_t k = 37;
 std::vector< std::vector< int > > L( k );
 std::size_t n = 0;
 for( auto & l : L ) {
  l.resize( g() % 200 );
  for( auto & x : l )
   x = int( g() % 10000 );
  std::sort( l.begin() , l.end() );
  n += l.size();
  }
 // the heap of the pairs ( head , list ), the smallest head on top
 ADS::DaryHeap< std::pair< int , std::size_t > ,
                std::less< std::pair< int , std::size_t > > , 2 > h;
 std::vector< std::size_t > next( k , 0 );
 for( std::size_t i = 0 ; i < k ; ++i )
  if( ! L[ i ].empty() )
   h.push( { L[ i ][ next[ i ]++ ] , i } );
 std::vector< int > M;
 while( ! h.empty() ) {
  const auto [ x , i ] = h.pop();
  M.push_back( x );
  if( next[ i ] < L[ i ].size() )
   h.push( { L[ i ][ next[ i ]++ ] , i } );
  }
 std::vector< std::pair< const int * , const int * > > R;
 for( const auto & l : L )
  R.emplace_back( l.data() , l.data() + l.size() );
 std::vector< int > E( n );
 ADS::multiwayMerge( R , E.data() );
 CHECK( M.size() == n && M == E );
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
