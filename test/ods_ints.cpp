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
#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/UniversalHashing.h"
#include "ADS/dictionary/BitmapTrie.h"
#include "ADS/dictionary/XFastTrie.h"
#include "ADS/dictionary/YFastTrie.h"
#include "ADS/external/BlockBTree.h"
#include "ADS/external/BlockStore.h"
#include "ADS/graph/IncidenceGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/Visit.h"
#include "ADS/memory/PoolAllocator.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/MeldableHeap.h"
#include "ADS/priority/MultiwayMerge.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/ListSort.h"
#include "ADS/sort/CountingSort.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/QuicksortAdversary.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/tree/BPlusTree.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <span>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// the sorted sets of integers and of strings are SSets, and the tables of
// chapter 5 dictionaries [see SetInterfaces.h, where it is there]
#if __has_include( "ADS/set/SetInterfaces.h" )
 #include "ADS/set/SetInterfaces.h"
static_assert( ADS::SSet< ADS::BinaryTrie< std::uint32_t > > );
static_assert( ADS::SSet< ADS::SimpleBinaryTrie< std::uint32_t > > );
static_assert( ADS::SSet< ADS::XFastTrie< std::uint32_t > > );
static_assert( ADS::SSet< ADS::ListXFastTrie< std::uint32_t > > );
static_assert( ADS::SSet< ADS::YFastTrie< std::uint32_t > > );
static_assert( ADS::SSet< ADS::BitmapTrie<> > );
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// the item that a pointer gives, or nothing for nil
template< class T >
std::optional< T > opt( const T * p )
{
 if( ! p )
  return( std::nullopt );
 return( *p );
 }

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
 auto key = [ & ]( void ) { return( U( ( U( g() ) << shift ) & mask ) ); };
 for( int i = 0 ; i < ops ; ++i ) {
  const U x = key();
  switch( g() % 4 ) {
   case 0:
   case 1:
    CHECK( t.insert( x ) == s.insert( x ).second );
    break;
   case 2:
    CHECK( t.remove( x ) == ( s.erase( x ) == 1 ) );
    break;
   default:
    CHECK( t.contains( x ) == ( s.count( x ) == 1 ) );
   }
  const U y = U( ( key() + U( g() % 3 ) - U( 1 ) ) & mask );
  CHECK( opt( t.lowerBound( y ) ) == successorIn( s , y ) );
  CHECK( ( t.find( y ) != nullptr ) == ( s.count( y ) == 1 ) );
  CHECK( t.size() == s.size() );
  }
 // the keys in order, and all of them removed
 std::vector< U > all( s.begin() , s.end() );
 for( const U x : all )
  CHECK( t.remove( x ) );
 CHECK( t.empty() && ! t.lowerBound( 0 ) );
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
  CHECK( ! t.lowerBound( 0 ) );
  CHECK( t.insert( 0xffffffffu , 7 ) && t.insert( 0 , 3 ) );
  CHECK( ! t.insert( 0 , 4 ) );
  CHECK( t.lookup( 0 ) == 4 && t.lookup( 0xffffffffu ) == 7 );
  CHECK( ! t.lookup( 5 ) );
  CHECK( *t.lowerBound( 1 ) == 0xffffffffu && *t.lowerBound( 0 ) == 0u );
  CHECK( *t.find( 0 ) == 0u && ! t.find( 1 ) );
  CHECK( t.remove( 0xffffffffu ) && ! t.lowerBound( 1 ) );
  }
 }

/// the tries as template aliases
struct MakeBinary
{
 template< class U , class V , int W >
 using type = ADS::BinaryTrie< U , V , W >;
 };
struct MakeSimple
{
 template< class U , class V , int W >
 using type = ADS::SimpleBinaryTrie< U , V , W >;
 };
struct MakeXFast
{
 template< class U , class V , int W >
 using type = ADS::XFastTrie< U , V , W >;
 };
struct MakeListXFast
{
 template< class U , class V , int W >
 using type = ADS::ListXFastTrie< U , V , W >;
 };
struct MakeYFast
{
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
  const auto lb = t.seek( x );
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
 CHECK( u.size() == 334 && t.empty() && ! t.lowerBound( 0 ) );
 t.insert( 5 );
 CHECK( *t.lowerBound( 0 ) == 5 );
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

// ods ex 13.4: findNear() of both x-fast tries equals lowerBound() on every
// key
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
 CHECK( *x.findNear( half - 1 ) == half && *l.findNear( half - 1 ) == half );
 CHECK( *x.findNear( 0 ) == half && *l.findNear( 0 ) == half );
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
  for( std::uint64_t d :
       { 0ULL , 1ULL , 2ULL , 3ULL , 100ULL , 1ULL << 20 , 1ULL << 40 } ) {
   const std::uint64_t y = k - d;
   const auto e = opt( x.lowerBound( y ) );
   CHECK( opt( x.findNear( y ) ) == e && opt( l.findNear( y ) ) == e );
   CHECK( opt( l.lowerBound( y ) ) == e );
   }
 for( int i = 0 ; i < 20000 ; ++i ) {
  const std::uint64_t y = g();
  const auto e = opt( x.lowerBound( y ) );
  CHECK( opt( x.findNear( y ) ) == e && opt( l.findNear( y ) ) == e );
  }
 // a trie of w = 10 bits, every key searched
 ADS::ListXFastTrie< std::uint16_t , std::monostate , 10 > s;
 ADS::XFastTrie< std::uint16_t , std::monostate , 10 > t;
 for( unsigned k = 0 ; k < 1024 ; k += 37 ) {
  s.insert( std::uint16_t( k ) );
  t.insert( std::uint16_t( k ) );
  }
 for( unsigned y = 0 ; y < 1024 ; ++y ) {
  const auto e = opt( s.lowerBound( std::uint16_t( y ) ) );
  CHECK( opt( s.findNear( std::uint16_t( y ) ) ) == e );
  CHECK( opt( t.findNear( std::uint16_t( y ) ) ) == e );
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
 CHECK( u.size() == m.size() && t.empty() && ! t.lowerBound( 0 ) );
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
 CHECK( ! t.lowerBound( "" ) );
 for( int i = 0 ; i < 6000 ; ++i ) {
  const std::string w = word();
  switch( g() % 3 ) {
   case 0:
   case 1:
    CHECK( t.insert( w , i ) == ( s.count( w ) == 0 ) );
    s[ w ] = i;
    break;
   default:
    CHECK( t.remove( w ) == ( s.erase( w ) == 1 ) );
   }
  const std::string y = word();
  CHECK( opt( t.lowerBound( y ) ) == succ( y ) );
  CHECK( ( t.find( y ) != nullptr ) == ( s.count( y ) == 1 ) );
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
		     return( a == b.first );
		     } ) );
 // a long string, and all removed
 const std::string L( 100000 , 'z' );
 CHECK( t.insert( L ) && *t.lowerBound( "z" ) == L );
 CHECK( t.remove( L ) );
 for( const auto & [ w , v ] : s )
  CHECK( t.remove( w ) );
 CHECK( t.empty() && t.nodes() == 1 && ! t.lowerBound( "" ) );
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
					      19 , 69 ,
					      32 , 93 , 55 , 50 } ) );
 auto r = h;
 h.push( 7 );
 CHECK( arrayOf( h ) == ( std::vector< int >{ 4 , 9 , 6 , 17 , 26 , 8 , 7 ,
					      19 , 69 ,
					      32 , 93 , 55 , 50 , 16 } ) );
 h.push( 3 );
 CHECK( arrayOf( h ) == ( std::vector< int >{ 3 , 9 , 4 , 17 , 26 , 8 , 6 ,
					      19 , 69 ,
					      32 , 93 , 55 , 50 , 16 , 7 } ) );
 CHECK( r.pop() == 4 );
 CHECK( arrayOf( r ) == ( std::vector< int >{ 6 , 9 , 8 , 17 , 26 , 50 , 16 ,
					      19 , 69 ,
					      32 , 93 , 55 } ) );
 CHECK( r.pop() == 6 );
 CHECK( arrayOf( r ) ==
	( std::vector< int >{ 8 , 9 , 16 , 17 , 26 , 50 , 55 , 19 , 69 , 32 ,
			      93 } ) );
 CHECK( r.pop() == 8 );
 CHECK( arrayOf( r ) ==
	( std::vector< int >{ 9 , 17 , 16 , 19 , 26 , 50 , 55 , 93 , 69 ,
			      32 } ) );
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
 std::size_t last = n - 1;  // the last place, at depth k - 1
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
   handles.clear();  // the handles may be stale
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
// the keys of figure 10.4 with the operations of exercises 10.6 and 10.7
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
 for( int x :
      { 19 , 17 , 9 , 26 , 4 , 55 , 16 , 25 , 32 , 28 , 93 , 19 , 99 , 89 ,
	20 , 50 , 8 } )
  h.push( x );
 h.push( 17 );  // exercise 10.6
 h.push( 82 );
 CHECK( h.pop() == 4 && h.pop() == 8 );  // exercise 10.7
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
  const std::vector< int > e(
    w.begin() , w.begin() + std::ptrdiff_t( std::min( k , w.size() ) ) );
  CHECK( b.smallest( k ) == e && d.smallest( k ) == e &&
	 m.smallest( k ) == e );
  }
 CHECK( b.size() == 3000 );  // unchanged
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
		std::less< std::pair< int , std::size_t > > , 2 >
   h;
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


/*--------------------------------------------------------------------------*/
/*-------------------------- CHAPTER 5, HASHING ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a differential test of a dictionary against a std::map, with the load
/// of the table checked after each operation by `check( t )`
template< class T , class F >
void tableAgainstMap( T & t , int ops , std::uint64_t universe ,
		      std::uint64_t seed , F && check )
{
 std::map< std::uint64_t , int > m;
 std::mt19937_64 g( seed );
 for( int i = 0 ; i < ops ; ++i ) {
  const std::uint64_t k = g() % universe;
  // a growing phase, a shrinking one, and a mixed one
  const int phase = 3 * i / ops;
  const auto c = g() % 10;
  if( ( phase == 0 && c < 8 ) || ( phase == 2 && c < 5 ) ) {
   t.insert( k , i );
   m[ k ] = i;
   }
  else {
   t.remove( k );
   m.erase( k );
   }
  CHECK( t.size() == m.size() );
  CHECK( t.contains( k ) == ( m.count( k ) == 1 ) );
  check( t );
  }
 for( const auto & [ k , v ] : m )
  CHECK( t.lookup( k ) == v );
 }

// ods 5.1: the table of the lists with the multiplicative hashing on
// 2^d lists and the rules of Morin's book, a load at most 1 and, above the
// initial size, at least 1/3, against a std::map; the places taken from the
// high bits, the same for any table size
ADS_TEST( "ods5.1.chained-hash-table" )
{
 using H = ADS::MultiplicativeHash< std::uint64_t >;
 ADS::ChainedHashTable< std::uint64_t , int , ADS::Chain::FRONT , H > t(
   4 , 1.0 , 1.0 / 3 , H( 7 ) );
 tableAgainstMap( t , 60000 , 1u << 14 , 1 , [ & ]( const auto & T ) {
  CHECK( ADS::is_pow2( T.capacity() ) && T.size() <= T.capacity() );
  CHECK( T.capacity() == 4 || 3 * T.size() >= T.capacity() - 3 );
  } );
 CHECK( t.restructures() > 10 );
 // the keys that differ in their low bits only spread over the table,
 // while h( k ) mod m would put them in one list
 ADS::ChainedHashTable< std::uint64_t , int , ADS::Chain::FRONT , H > u(
   1024 , false , H( 3 ) );
 for( std::uint64_t k = 0 ; k < 1024 ; ++k )
  u.insert( k << 20 , 0 );
 std::size_t longest = 0;
 for( std::size_t i = 0 ; i < 1024 ; ++i )
  longest = std::max( longest , u.chain( i ).size() );
 CHECK( longest <= 8 );
 const H h( 5 );
 for( std::uint64_t k = 1 ; k < 1000 ; k += 7 )
  for( std::size_t m = 1 ; m <= ( std::size_t( 1 ) << 20 ) ; m <<= 1 )
   CHECK( h.place( k , m ) ==
	  ( m == 1
	      ? 0
	      : h( k ) >> ( 64 - std::countr_zero( std::uint64_t( m ) ) ) ) );
 }

/// the modular inverse of an odd \f$ z \f$ modulo \f$ 2^{64} \f$, by the
/// iteration of Newton, each step doubling the bits that are right
std::uint64_t inverse64( std::uint64_t z )
{
 std::uint64_t y = z;  // right on 3 bits, z z = 1 mod 8
 for( int i = 0 ; i < 5 ; ++i )
  y *= 2 - z * y;
 return( y );
 }

// ods 5.1.1, ods ex 5.2 and ex 5.3: two distinct keys collide under the
// multiplicative hashing with probability at most 2 / 2^d (Lemma 5.1), and
// exactly 2 / 2^d for x = 2^( w - d - 2 ) and y = 3 x (exercise 5.3), over
// 2^17 random multipliers; for a given z, the n = 2^d keys z^-1 j, j < n,
// all have the hash 0 when d <= w / 2 (exercise 5.2); the hash code is the
// key itself, as in the book (std::hash need not be the identity)
ADS_TEST( "ods5.1.multiplicative-hash" )
{
 using MH = ADS::MultiplicativeHash< std::uint64_t , std::identity >;
 const int d = 4;
 const std::size_t m = std::size_t( 1 ) << d;
 const int N = 1 << 17;
 const std::uint64_t x = std::uint64_t( 1 ) << ( 64 - d - 2 ) , y = 3 * x;
 int same = 0 , random = 0;
 std::mt19937_64 g( 11 );
 for( int i = 0 ; i < N ; ++i ) {
  const auto h = MH::ofMultiplier( g() | 1 );
  same += h.place( x , m ) == h.place( y , m );
  const std::uint64_t a = g() , b = g();
  random += h.place( a , m ) == h.place( b , m );
  }
 // the binomial of mean N / 8 and of mean at most N / 8, within 5 sigma
 const double mean = N * 2.0 / m , sigma = std::sqrt( mean );
 CHECK( std::abs( same - mean ) < 5 * sigma );
 CHECK( random < mean + 5 * sigma );
 for( std::uint64_t z : { 1ULL , 3ULL , 0x9e3779b97f4a7c15ULL , ~0ULL } ) {
  const auto h = MH::ofMultiplier( z );
  const std::uint64_t zi = inverse64( z );
  CHECK( z * zi == 1 );
  const std::size_t n = std::size_t( 1 ) << 20;  // d = 20 <= w / 2
  for( std::uint64_t j = 0 ; j < n ; j += 997 )
   CHECK( h.place( zi * j , n ) == 0 );
  }
 }

// ods 5.2: the table of the linear inspection with the tabulation hashing
// on 2^d places and the rules of Morin's book, against a std::map: the
// full and deleted places at most half, the keys above an eighth past the
// initial size; a table emptied by removals and refilled with other keys
// is rebuilt at its size instead of doubling for ever
ADS_TEST( "ods5.2.linear-hash-table" )
{
 ADS::OpenHashTable< std::uint64_t , int , ADS::TabulationHash > t(
   8 , ADS::Probing::linear( 1 ) , true , ADS::TabulationHash( 3 ) );
 tableAgainstMap( t , 60000 , 1u << 13 , 2 , [ & ]( const auto & T ) {
  CHECK( ADS::is_pow2( T.capacity() ) );
  CHECK( 2 * T.size() <= T.capacity() );
  } );
 ADS::OpenHashTable< std::uint64_t , int , ADS::TabulationHash > u(
   8 , ADS::Probing::linear( 1 ) , true , ADS::TabulationHash( 4 ) );
 std::uint64_t next = 0;
 for( int round = 0 ; round < 200 ; ++round ) {
  for( int i = 0 ; i < 100 ; ++i )
   u.insert( next + std::uint64_t( i ) , i );
  for( int i = 0 ; i < 100 ; ++i )
   u.remove( next + std::uint64_t( i ) );
  next += 100;
  }
 CHECK( u.empty() && u.capacity() <= 512 );
 const ADS::TabulationHash h( 9 );
 for( std::uint64_t k = 0 ; k < 1000 ; k += 13 )
  CHECK( h.place( k , 1 ) == 0 && h.place( k , 1024 ) == ( h( k ) >> 54 ) );
 }

/// the hash code of exercise 5.6, the early String.hashCode() of Java on
/// the characters of even place only, s_0 31^( k - 1 ) + s_2 31^( k - 2 )
/// + ... modulo 2^32
std::uint32_t evenCharacters( const std::string & s )
{
 std::uint32_t h = 0;
 for( std::size_t i = 0 ; i < s.size() ; i += 2 )
  h = 31 * h + static_cast< unsigned char >( s[ i ] );
 return( h );
 }

// ods 5.3 and ods ex 5.6 to ex 5.10: the hash codes of the compound objects
// and of the sequences tell apart the large sets of objects on which the XOR
// of two words (all ( x , x ) give 0, exercise 5.7), their sum (all
// ( x , c - x ) give c, exercise 5.8) and the hash of the even characters
// (all the strings that differ in the odd places, exercise 5.6) collide;
// any function of two words into one has a value of at least 2^w pairs
// (exercise 5.9, on words of 8 bits); the prefixes of an array and the
// arrays of zeros of different lengths differ; and x mod ( 2^w - 1 ) by the
// sums of the halves (exercise 5.10)
ADS_TEST( "ods5.3.hash-codes" )
{
 const ADS::CompoundHash c( 2 , 5 );
 std::set< std::uint64_t > H;
 for( std::uint64_t x = 0 ; x < 20000 ; ++x ) {
  CHECK( ( x ^ x ) == 0 && ( x + ( 12345 - x ) ) == 12345 );
  const std::uint64_t a[ 2 ] = { x , x } , b[ 2 ] = { x , 12345 - x };
  H.insert( c( a ) );
  H.insert( c( b ) );
  }
 CHECK( H.size() >= 39999 );  // ( 6172 , 6173 ) is in both sets
 const ADS::CompoundHash c3( 3 , 6 );
 const std::uint64_t p[ 3 ] = { 1 , 2 , 3 } , q[ 3 ] = { 1 , 2 , 4 };
 CHECK( c3( p ) != c3( q ) );

 // exercise 5.6: 2^10 strings of 20 characters with one even-place hash
 const ADS::SequenceHash s( 7 );
 std::set< std::uint32_t > E;
 std::set< std::uint64_t > S;
 for( unsigned v = 0 ; v < 1024 ; ++v ) {
  std::string w( 20 , 'a' );
  for( int i = 0 ; i < 10 ; ++i )
   w[ 2 * std::size_t( i ) + 1 ] = ( v >> i ) & 1 ? 'b' : 'a';
  E.insert( evenCharacters( w ) );
  S.insert( s( w ) );
  }
 CHECK( E.size() == 1 && S.size() == 1024 );

 // the arrays of zeros of lengths 0 to 999, and the prefixes of an array
 std::set< std::uint64_t > Z , P;
 std::vector< std::uint64_t > zeros , pre;
 std::mt19937_64 g( 12 );
 for( int i = 0 ; i < 1000 ; ++i ) {
  Z.insert( s( std::span< const std::uint64_t >( zeros ) ) );
  P.insert( s( std::span< const std::uint64_t >( pre ) ) );
  zeros.push_back( 0 );
  pre.push_back( g() );
  }
 CHECK( Z.size() == 1000 && P.size() == 1000 );
 std::set< std::uint64_t > T;
 for( int i = 0 ; i < 300 ; ++i )
  T.insert( s( std::string( std::size_t( i ) , '\0' ) ) );
 CHECK( T.size() == 300 );

 // exercise 5.9: a function of two bytes into one byte, here the folded
 // multiplicative hash, has a value taken by at least 2^8 pairs
 std::vector< int > count( 256 , 0 );
 for( unsigned x = 0 ; x < 256 ; ++x )
  for( unsigned y = 0 ; y < 256 ; ++y )
   ++count[ ( ( x * 0x9Du + y * 0x3Bu ) >> 3 ) & 0xFF ];
 CHECK( *std::max_element( count.begin() , count.end() ) >= 256 );

 // exercise 5.10: ( x mod 2^w ) + ( x div 2^w ) = x mod ( 2^w - 1 )
 for( int w : { 7 , 31 , 32 , 61 } ) {
  const std::uint64_t M = ( std::uint64_t( 1 ) << w ) - 1;
  for( int i = 0 ; i < 10000 ; ++i ) {
   std::uint64_t x = g() >> 1;  // so that the sums do not overflow
   const std::uint64_t r = x % M;
   while( x > M )
    x = ( x & M ) + ( x >> w );
   CHECK( ( x == M ? 0 : x ) == r );
   }
  }
 }


/*--------------------------------------------------------------------------*/
/*-------------------------- CHAPTER 11, SORTING ---------------------------*/
/*--------------------------------------------------------------------------*/

// ods 11.1 and ods ex 11.1: merge sort, quicksort (the first item as the
// pivot, and the random pivot with the partition in three of Morin's
// book), heapsort, counting sort and radix sort on the array of exercise
// 11.1 and on random arrays, against std::sort; the list merge sort that
// relinks the cells, with no auxiliary array (exercise 11.2)
ADS_TEST( "ods11.1.sorts" )
{
 const std::vector< int > a{ 1 , 7 , 4 , 6 , 2 , 8 , 3 , 5 };
 const std::vector< int > e{ 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 };
 std::mt19937 g( 13 );
 for( int round = 0 ; round < 50 ; ++round ) {
  std::vector< int > v = a;
  if( round > 0 ) {
   v.resize( g() % 2000 );
   for( auto & x : v )
    x = int( g() % 1000 );
   }
  std::vector< int > w = v;
  std::sort( w.begin() , w.end() );
  auto m = v , q = v , t = v , h = v , c = v , r = v;
  ADS::mergeSort( m );
  ADS::quickSort( q );
  ADS::threeWayQuickSort( t );
  ADS::heapsort( h );
  ADS::countingSort( c , 1000 );
  ADS::radixSort( r );
  CHECK( m == w && q == w && t == w && h == w && c == w && r == w );
  if( round == 0 )
   CHECK( w == e );
  ADS::List< int > L( v.begin() , v.end() );
  ADS::listSort( L );
  CHECK( std::equal( L.begin() , L.end() , w.begin() , w.end() ) );
  }
 }

// ods ex 11.2: the list merge sort keeps the cells, only relinked: the
// address of each item is the same before and after, and it is stable
ADS_TEST( "ods.ex11.2.list-merge-sort" )
{
 ADS::List< std::pair< int , int > > L;
 std::mt19937 g( 14 );
 for( int i = 0 ; i < 5000 ; ++i )
  L.push_back( { int( g() % 100 ) , i } );
 std::set< const std::pair< int , int > * > before;
 for( const auto & x : L )
  before.insert( &x );
 ADS::listSort(
   L ,
   []( const auto & a , const auto & b ) { return( a.first < b.first ); } );
 std::set< const std::pair< int , int > * > after;
 const std::pair< int , int > * prev = nullptr;
 for( const auto & x : L ) {
  after.insert( &x );
  if( prev )
   CHECK( prev->first < x.first ||
	  ( prev->first == x.first && prev->second < x.second ) );
  prev = &x;
  }
 CHECK( before == after );
 }

/// a quicksort of \f$ A[ i .. i + n - 1 ] \f$ whose pivot is the middle
/// item, \f$ A[ i + \lfloor n / 2 \rfloor ] \f$, moved to the front and
/// compared with the other \f$ n - 1 \f$ items, the rule of exercise 11.4
template< class T , class C >
void middlePivotQuickSort( std::vector< T > & A , std::size_t i ,
			   std::size_t n ,
			   C less )
{
 while( n > 1 ) {
  std::swap( A[ i ] , A[ i + n / 2 ] );
  std::size_t p = i;  // A[ i + 1 .. p ] < pivot
  for( std::size_t j = i + 1 ; j < i + n ; ++j )
   if( less( A[ j ] , A[ i ] ) )
    std::swap( A[ ++p ] , A[ j ] );
  std::swap( A[ i ] , A[ p ] );
  middlePivotQuickSort( A , i , p - i , less );
  n -= p - i + 1;
  i = p + 1;
  }
 }

// ods ex 11.3 to ex 11.6: the adversary of McIlroy, a comparator that does
// not look at the values, forces binom( n , 2 ) comparisons or more out of
// the quicksort with the first item as the pivot (exercise 11.3), with the
// middle one (exercise 11.4), and with the random pivot and the partition
// in three of Morin's book (exercises 11.5 and 11.6); the input it leaves
// makes the deterministic sorts do the same comparisons again, the sorted
// array is the killer input of the first rule, and a random input costs
// O( n log n )
ADS_TEST( "ods.ex11.6.quicksort-adversary" )
{
 const std::size_t n = 2000 , all = n * ( n - 1 ) / 2;
 struct Counting
 {
  std::size_t * c;
  bool operator()( std::size_t a , std::size_t b ) const {
   ++*c;
   return( a < b );
   }
  };
 {
  ADS::QuicksortAdversary adv( n );
  auto I = adv.items();
  ADS::quickSort( I , adv.less() );
  CHECK( adv.comparisons() >= all );
  // the killer input, replayed with a plain comparator
  auto K = adv.input();
  std::size_t c = 0;
  ADS::quickSort( K , Counting{ &c } );
  CHECK( c == adv.comparisons() );
  // exercise 11.3: the sorted array, binom( n , 2 ) comparisons by the
  // partitions (and those of the audits of the partitions, which the
  // contracts of the tests check, on top)
  std::vector< std::size_t > S( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   S[ i ] = i;
  c = 0;
  ADS::quickSort( S , Counting{ &c } );
  CHECK( c >= all );
  }
 {
  ADS::QuicksortAdversary adv( n );
  auto I = adv.items();
  middlePivotQuickSort( I , 0 , n , adv.less() );
  CHECK( adv.comparisons() == all );
  auto K = adv.input();
  std::size_t c = 0;
  middlePivotQuickSort( K , 0 , n , Counting{ &c } );
  CHECK( c == all && std::is_sorted( K.begin() , K.end() ) );
  }
 {
  ADS::QuicksortAdversary adv( n );
  auto I = adv.items();
  ADS::threeWayQuickSort( I , adv.less() );
  CHECK( adv.comparisons() >= all / 2 );
  // the values given are a total order, and the sort agrees with it
  const auto V = adv.input();
  for( std::size_t i = 1 ; i < n ; ++i )
   CHECK( V[ I[ i - 1 ] ] < V[ I[ i ] ] );
  // a random input: O( n log n ) comparisons (the audits included), an
  // order of magnitude fewer
  std::vector< std::size_t > R = adv.items();
  std::shuffle( R.begin() , R.end() , std::mt19937( 15 ) );
  std::size_t c = 0;
  ADS::threeWayQuickSort( R , Counting{ &c } );
  CHECK( c < all / 8 );
  }
 }

// ods ex 11.7: the expected comparisons of the quicksort of Morin's book,
// which compares each of the n items of a part with the pivot (itself
// included) and stops on the parts of one item, satisfy
// T( n ) = n + ( 2 / n ) sum_{k<n} T( k ), whose solution is
// 2 ( n + 1 ) H_n - 4 n + ( 2 n - 1 ) / 3 for n >= 2, and not the
// 2 n H_n - n + H_n of the exercise; checked exactly up to 2000 and against
// the average of a run of that quicksort
ADS_TEST( "ods.ex11.7.quicksort-expectation" )
{
 std::vector< double > T( 2001 , 0.0 );
 double sum = 0 , H = 1;  // sum of T( 0 .. n - 1 ), H_n
 for( std::size_t n = 2 ; n <= 2000 ; ++n ) {
  sum += T[ n - 1 ];
  H += 1.0 / double( n );
  T[ n ] = double( n ) + 2.0 * sum / double( n );
  const double closed = 2.0 * double( n + 1 ) * H - 4.0 * double( n ) +
			( 2.0 * double( n ) - 1.0 ) / 3.0;
  CHECK( std::abs( T[ n ] - closed ) < 1e-9 * T[ n ] );
  const double exercise = 2.0 * double( n ) * H - double( n ) + H;
  CHECK( std::abs( T[ n ] - exercise ) > 1.0 );
  }
 // the quicksort of the book on distinct keys, its comparisons averaged
 std::mt19937 g( 16 );
 const std::size_t n = 200 , runs = 4000;
 double total = 0;
 std::vector< int > A( n );
 for( std::size_t r = 0 ; r < runs ; ++r ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   A[ i ] = int( i );
  std::shuffle( A.begin() , A.end() , g );
  std::size_t c = 0;
  std::vector< std::pair< std::size_t , std::size_t > > S{ { 0 , n } };
  while( ! S.empty() ) {
   const auto [ i , m ] = S.back();
   S.pop_back();
   if( m <= 1 )
    continue;
   const int x = A[ i + g() % m ];
   std::size_t p = i , j = i , q = i + m;
   while( j < q ) {
    ++c;
    if( A[ j ] < x )
     std::swap( A[ j++ ] , A[ p++ ] );
    else if( A[ j ] > x )
     std::swap( A[ j ] , A[ --q ] );
    else
     ++j;
    }
   S.push_back( { i , p - i } );
   S.push_back( { q , i + m - q } );
   }
  CHECK( std::is_sorted( A.begin() , A.end() ) );
  total += double( c );
  }
 CHECK( std::abs( total / runs - T[ n ] ) < 0.01 * T[ n ] );
 }

/// the comparisons of the heap sort of section 11.1.3 of Morin's book on
/// \f$ A \f$: a heap of the minimum built by Floyd, then the root swapped
/// with the last item n times and the new root moved down, the smaller
/// child compared with the item, two comparisons per level (its
/// trickleDown)
std::size_t bookHeapSort( std::vector< std::size_t > & A )
{
 std::size_t c = 0;
 auto down = [ & ]( std::size_t i , std::size_t n ) {
  for( ; ; ) {
   const std::size_t l = 2 * i + 1 , r = l + 1;
   std::size_t j = n;
   if( r < n ) {
    ++c;
    if( A[ r ] < A[ i ] ) {
     ++c;
     j = A[ l ] < A[ r ] ? l : r;
     }
    else if( ( ++c , A[ l ] < A[ i ] ) )
     j = l;
    }
   else if( l < n && ( ++c , A[ l ] < A[ i ] ) )
    j = l;
   if( j == n )
    return;
   std::swap( A[ i ] , A[ j ] );
   i = j;
   }
  };
 const std::size_t n = A.size();
 for( std::size_t i = n / 2 ; i-- > 0 ; )
  down( i , n );
 for( std::size_t m = n ; m > 1 ; ) {
  std::swap( A[ 0 ] , A[ --m ] );
  down( 0 , m );
  }
 std::reverse( A.begin() , A.end() );
 return( c );
 }

// ods ex 11.8: the heap sort of Morin's book on an array in increasing
// order, already a heap of the minimum, makes at least
// 2 n log2 n - 4 n comparisons: the last leaf moved to the root is larger
// than all the items of the upper levels and sinks to the bottom, with two
// comparisons per level; the heapsort of ADS, on the decreasing array,
// makes no fewer
ADS_TEST( "ods.ex11.8.heapsort-worst" )
{
 for( std::size_t n : { std::size_t( 1 ) << 10 , std::size_t( 1 ) << 14 ,
			std::size_t( 100000 ) } ) {
  const double bound =
    2.0 * double( n ) * std::log2( double( n ) ) - 4.0 * double( n );
  std::vector< std::size_t > A( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   A[ i ] = i;
  const std::size_t c = bookHeapSort( A );
  CHECK( std::is_sorted( A.begin() , A.end() ) );
  CHECK( double( c ) >= bound );
  for( std::size_t i = 0 ; i < n ; ++i )
   A[ i ] = n - i;
  std::size_t d = 0;
  ADS::heapsort( A , [ & ]( std::size_t a , std::size_t b ) {
   ++d;
   return( a < b );
   } );
  CHECK( std::is_sorted( A.begin() , A.end() ) && double( d ) >= bound );
  }
 }

// ods ex 11.9: the comparison tree of figure 11.6, with 4 leaves for the
// 6 permutations of 1, 2, 3, sorts all of them but 2, 3, 1 and 3, 2, 1,
// which reach the leaf of 3, 1, 2; the binary tree of k leaves has height
// at least log2 k (exercises 11.10 to 11.12), so that 3 comparisons are
// needed
ADS_TEST( "ods.ex11.9.comparison-tree" )
{
 // the tree: a[ 0 ] vs a[ 2 ]; if smaller, a[ 0 ] vs a[ 1 ]; if that is
 // smaller, a[ 1 ] vs a[ 2 ]
 auto order = []( const std::array< int , 3 > & a ) {
  if( a[ 0 ] > a[ 2 ] )
   return( std::array< int , 3 >{ 1 , 2 , 0 } );
  if( a[ 0 ] > a[ 1 ] )
   return( std::array< int , 3 >{ 1 , 0 , 2 } );
  if( a[ 1 ] < a[ 2 ] )
   return( std::array< int , 3 >{ 0 , 1 , 2 } );
  return( std::array< int , 3 >{ 0 , 2 , 1 } );
  };
 std::array< int , 3 > p{ 1 , 2 , 3 };
 std::vector< std::array< int , 3 > > wrong;
 do {
  const auto o = order( p );
  if( ! ( p[ o[ 0 ] ] < p[ o[ 1 ] ] && p[ o[ 1 ] ] < p[ o[ 2 ] ] ) )
   wrong.push_back( p );
  } while( std::next_permutation( p.begin() , p.end() ) );
 CHECK( ( wrong ==
	  std::vector< std::array< int , 3 > >{ { 2 , 3 , 1 } ,
						{ 3 , 2 , 1 } } ) );
 CHECK( std::ceil( std::log2( 6.0 ) ) == 3.0 );
 }

// ods ex 11.13: the radix sort of signed integers, the sign bit flipped in
// the key, against std::sort, with the extremes of each width
template< class T >
void signedRadix( std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 std::vector< T > v( 20000 );
 for( auto & x : v )
  x = T( g() );
 v.push_back( std::numeric_limits< T >::min() );
 v.push_back( std::numeric_limits< T >::max() );
 v.push_back( 0 );
 v.push_back( -1 );
 auto w = v;
 std::sort( w.begin() , w.end() );
 ADS::radixSort( v );
 CHECK( v == w );
 }

ADS_TEST( "ods.ex11.13.signed-radix-sort" )
{
 signedRadix< std::int8_t >( 1 );
 signedRadix< std::int16_t >( 2 );
 signedRadix< std::int32_t >( 3 );
 signedRadix< std::int64_t >( 4 );
 }


/*--------------------------------------------------------------------------*/
/*-------------------------- CHAPTER 12, GRAPHS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a differential test of an oriented graph of n nodes against a set of
/// pairs: insertions and removals of edges, hasEdge(), the nodes out of
/// (adj) and into (inAdj) a node
template< class G >
void graphAgainstSet( G & g , int n , int ops , std::uint64_t seed )
{
 std::set< std::pair< int , int > > E;
 std::mt19937_64 rng( seed );
 for( int k = 0 ; k < ops ; ++k ) {
  const int i = int( rng() % unsigned( n ) );
  const int j = int( rng() % unsigned( n ) );
  if( i == j )
   continue;
  if( rng() % 3 ) {
   if( ! E.count( { i , j } ) ) {
    g.insertEdge( i , j );
    E.insert( { i , j } );
    }
   }
  else {
   g.deleteEdge( i , j );
   E.erase( { i , j } );
   }
  CHECK( g.hasEdge( i , j ) == ( E.count( { i , j } ) == 1 ) );
  CHECK( g.m() == int( E.size() ) );
  }
 for( int i = 0 ; i < n ; ++i ) {
  std::vector< int > out , in;
  for( const auto & [ a , b ] : E ) {
   if( a == i )
    out.push_back( b );
   if( b == i )
    in.push_back( a );
   }
  auto A = g.adj( i ) , I = g.inAdj( i );
  std::sort( A.begin() , A.end() );
  std::sort( I.begin() , I.end() );
  CHECK( A == out && I == in );
  }
 }

// ods 12.1 and 12.2: the adjacency matrix and the adjacency lists, with
// the operations of Morin's interface, inEdges() included, against a set
// of pairs
ADS_TEST( "ods12.1.adjacency" )
{
 ADS::MatrixGraph<> M( 40 );
 ADS::ListGraph<> L( 40 );
 graphAgainstSet( M , 40 , 20000 , 1 );
 graphAgainstSet( L , 40 , 20000 , 2 );
 ADS::ListGraph<> U( 5 , false );
 U.insertEdge( 0 , 3 );
 U.insertEdge( 4 , 0 );
 CHECK( ( U.inAdj( 0 ) == std::vector< int >{ 3 , 4 } ) );
 }

// ods ex 12.2: the matrix of incidence, against a set of pairs; n m bytes,
// one -1 and one +1 per column; the visit in breadth on it equals the one on
// the lists of the same edges
ADS_TEST( "ods.ex12.2.incidence-matrix" )
{
 ADS::IncidenceGraph g( 30 );
 graphAgainstSet( g , 30 , 6000 , 3 );
 CHECK( g.bytes() == std::size_t( 30 ) * std::size_t( g.m() ) );
 for( int e = 0 ; e < g.m() ; ++e ) {
  int sum = 0 , nonzero = 0;
  for( int i = 0 ; i < 30 ; ++i ) {
   sum += g.entry( i , e );
   nonzero += g.entry( i , e ) != 0;
   }
  CHECK( sum == 0 && nonzero == 2 );
  }
 ADS::ListGraph<> L( 30 );
 for( int i = 0 ; i < 30 ; ++i )
  for( int j : g.adj( i ) )
   L.insertEdge( i , j );
 CHECK( ADS::distance( g , 0 ).first == ADS::distance( L , 0 ).first );
 }

// the matrix of incidence on its edge cases: the graph of 0 nodes (no node,
// no column, every edge a violation), a loop, a node out of range and a
// negative one for each operation, an entry out of the columns; two
// parallel edges, two equal columns, removed one at a time, and the removal
// of an edge that is not there, which changes nothing
ADS_TEST( "ods.ex12.2.incidence-contracts" )
{
 CHECK_THROWS( ADS::IncidenceGraph( -1 ) , ADS::contract_violation );
 ADS::IncidenceGraph z;
 CHECK( z.n() == 0 && z.m() == 0 && z.V().empty() && z.bytes() == 0 &&
	! z.hasNode( 0 ) );
 CHECK_THROWS( z.insertEdge( 0 , 1 ) , ADS::contract_violation );
 CHECK_THROWS( ( void )z.adj( 0 ) , ADS::contract_violation );
 ADS::IncidenceGraph g( 4 );
 for( const auto [ i , j ] : { std::pair( 2 , 2 ) , std::pair( -1 , 0 ) ,
			      std::pair( 0 , 4 ) , std::pair( 4 , 0 ) } ) {
  CHECK_THROWS( g.insertEdge( i , j ) , ADS::contract_violation );
  CHECK_THROWS( g.deleteEdge( i , j ) , ADS::contract_violation );
  }
 CHECK( ! g.hasEdge( 2 , 2 ) );  // a loop is never an edge
 CHECK_THROWS( ( void )g.hasEdge( 0 , 4 ) , ADS::contract_violation );
 CHECK_THROWS( ( void )g.hasEdge( -1 , 0 ) , ADS::contract_violation );
 CHECK_THROWS( ( void )g.adj( 4 ) , ADS::contract_violation );
 CHECK_THROWS( ( void )g.inAdj( -1 ) , ADS::contract_violation );
 CHECK_THROWS( g.forEachAdj( 4 , []( int ) { } ) , ADS::contract_violation );
 CHECK_THROWS( ( void )g.entry( 0 , 0 ) , ADS::contract_violation );
 g.insertEdge( 0 , 3 );
 g.insertEdge( 1 , 3 );
 g.insertEdge( 0 , 3 );  // parallel
 CHECK( g.m() == 3 && g.bytes() == 12 && g.hasEdge( 0 , 3 ) &&
	! g.hasEdge( 3 , 0 ) );
 CHECK( ( g.adj( 0 ) == std::vector< int >{ 3 , 3 } ) &&
	( g.inAdj( 3 ) == std::vector< int >{ 0 , 1 , 0 } ) );
 for( int i = 0 ; i < 4 ; ++i )
  CHECK( g.entry( i , 0 ) == g.entry( i , 2 ) );
 CHECK_THROWS( ( void )g.entry( 4 , 0 ) , ADS::contract_violation );
 CHECK_THROWS( ( void )g.entry( 0 , 3 ) , ADS::contract_violation );
 CHECK_THROWS( ( void )g.entry( 0 , -1 ) , ADS::contract_violation );
 g.deleteEdge( 3 , 0 );  // not there
 CHECK( g.m() == 3 );
 g.deleteEdge( 0 , 3 );
 CHECK( g.m() == 2 && g.hasEdge( 0 , 3 ) && g.hasEdge( 1 , 3 ) );
 g.deleteEdge( 0 , 3 );
 CHECK( g.m() == 1 && ! g.hasEdge( 0 , 3 ) && g.hasEdge( 1 , 3 ) &&
	g.adj( 0 ).empty() && g.bytes() == 4 );
 g.deleteEdge( 1 , 3 );
 CHECK( g.m() == 0 && g.bytes() == 0 && g.inAdj( 3 ).empty() );
 ADS::IncidenceGraph one( 1 );  // one node: no edge at all
 CHECK( one.V() == std::vector< int >{ 0 } && one.adj( 0 ).empty() &&
	! one.hasEdge( 0 , 0 ) );
 CHECK_THROWS( one.insertEdge( 0 , 0 ) , ADS::contract_violation );
 }

/// the order of the recursive visit in depth of Morin's book, dfs( g , r )
template< class G >
void recursiveDfs( const G & g , int u , std::vector< bool > & seen ,
		   std::vector< int > & order )
{
 seen[ std::size_t( u ) ] = true;
 order.push_back( u );
 for( int v : g.adj( u ) )
  if( ! seen[ std::size_t( v ) ] )
   recursiveDfs( g , v , seen , order );
 }

/// the order of the dfs2( g , r ) of Morin's book: a stack of nodes, each
/// node marked when it is popped and all its neighbours pushed
template< class G >
std::vector< int > stackDfs( const G & g , int r )
{
 std::vector< bool > seen( std::size_t( g.bound() ) , false );
 std::vector< int > S{ r } , order;
 while( ! S.empty() ) {
  const int u = S.back();
  S.pop_back();
  if( seen[ std::size_t( u ) ] )
   continue;
  seen[ std::size_t( u ) ] = true;
  order.push_back( u );
  for( int v : g.adj( u ) )
   S.push_back( v );
  }
 return( order );
 }

// ods 12.3 and ods ex 12.3, ex 12.9: the visit in depth of ADS, with its
// stack of iterators, gives the order of the recursive dfs of the book, on
// random graphs; the dfs2 of the book does not, on the graph 0 -> 1,
// 0 -> 2, 1 -> 2 (exercise 12.9); the visit in breadth gives the
// distances of the brute force
ADS_TEST( "ods12.3.visits" )
{
 ADS::ListGraph<> t( 3 );
 t.insertEdge( 0 , 1 );
 t.insertEdge( 0 , 2 );
 t.insertEdge( 1 , 2 );
 std::vector< bool > seen( 3 , false );
 std::vector< int > rec , ads;
 recursiveDfs( t , 0 , seen , rec );
 CHECK( ( rec == std::vector< int >{ 0 , 1 , 2 } ) );
 CHECK( ( stackDfs( t , 0 ) == std::vector< int >{ 0 , 2 , 1 } ) );
 std::mt19937 rng( 17 );
 for( int round = 0 ; round < 30 ; ++round ) {
  const int n = 60;
  ADS::ListGraph<> g( n );
  for( int k = 0 ; k < 150 ; ++k ) {
   const int i = int( rng() % n ) , j = int( rng() % n );
   if( i != j && ! g.hasEdge( i , j ) )
    g.insertEdge( i , j );
   }
  std::vector< bool > s1( n , false ) , s2( n , false );
  std::vector< int > r1 , r2;
  recursiveDfs( g , 0 , s1 , r1 );
  ADS::dfs(
    g , 0 , s2 , [ & ]( int u ) { r2.push_back( u ); } , []( int , int ) { } ,
    []( int ) { } );
  CHECK( r1 == r2 );
  // the distances of bfs against the iterated relaxations of Bellman
  std::vector< int > d( n , ADS::INF );
  d[ 0 ] = 0;
  for( int it = 0 ; it < n ; ++it )
   for( int u = 0 ; u < n ; ++u )
    if( d[ std::size_t( u ) ] != ADS::INF )
     for( int v : g.adj( u ) )
      d[ std::size_t( v ) ] =
	std::min( d[ std::size_t( v ) ] , d[ std::size_t( u ) ] + 1 );
  CHECK( ADS::distance( g , 0 ).first == d );  // exercise 12.8
  }
 }

// ods ex 12.4 to ex 12.7: connectivity, the labels of the components, a
// spanning forest (the trees of the visits in breadth, n - c edges of G
// for c components) and the strong connectivity, each in O( n + m ),
// against the closure of the reachability
ADS_TEST( "ods.ex12.4.components" )
{
 std::mt19937 rng( 18 );
 for( int round = 0 ; round < 40 ; ++round ) {
  const int n = 50;
  ADS::ListGraph<> u( n , false ) , o( n );
  const int edges = round * 3;
  for( int k = 0 ; k < edges ; ++k ) {
   const int i = int( rng() % n ) , j = int( rng() % n );
   if( i != j && ! o.hasEdge( i , j ) ) {
    o.insertEdge( i , j );
    if( ! u.hasEdge( i , j ) )
     u.insertEdge( i , j );
    }
   }
  // the reachability, by Warshall
  auto closure = [ & ]( const auto & g ) {
   std::vector< std::vector< bool > > R( n , std::vector< bool >( n ) );
   for( int i = 0 ; i < n ; ++i ) {
    R[ i ][ i ] = true;
    for( int j : g.adj( i ) )
     R[ i ][ j ] = true;
    }
   for( int k = 0 ; k < n ; ++k )
    for( int i = 0 ; i < n ; ++i )
     if( R[ i ][ k ] )
      for( int j = 0 ; j < n ; ++j )
       if( R[ k ][ j ] )
	R[ i ][ j ] = true;
   return( R );
   };
  const auto RU = closure( u ) , RO = closure( o );
  const auto id = ADS::cc( u );  // exercise 12.5
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    CHECK( ( id[ i ] == id[ j ] ) == RU[ i ][ j ] );
  const int c = *std::max_element( id.begin() , id.end() );
  bool connected = true;  // exercise 12.4
  for( int j = 0 ; j < n ; ++j )
   connected = connected && RU[ 0 ][ j ];
  CHECK( connected == ( c == 1 ) );
  // exercise 12.6: a forest of the trees of the visits from one node of
  // each component
  std::vector< int > parent( n , ADS::NIL );
  std::vector< bool > done( n , false );
  int trees = 0;
  for( int r = 0 ; r < n ; ++r )
   if( ! done[ r ] ) {
    ++trees;
    const auto [ d , p ] = ADS::distance( u , r );
    for( int v = 0 ; v < n ; ++v )
     if( d[ v ] != ADS::INF ) {
      done[ v ] = true;
      parent[ v ] = p[ v ];
      }
    }
  int forest = 0;
  for( int v = 0 ; v < n ; ++v )
   if( parent[ v ] != ADS::NIL ) {
    ++forest;
    CHECK( u.hasEdge( parent[ v ] , v ) && id[ parent[ v ] ] == id[ v ] );
    }
  CHECK( trees == c && forest == n - c );
  // exercise 12.7
  const auto sid = ADS::scc( o );
  bool strong = true;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) {
    strong = strong && RO[ i ][ j ];
    CHECK( ( sid[ i ] == sid[ j ] ) == ( RO[ i ][ j ] && RO[ j ][ i ] ) );
    }
  CHECK( strong == ( *std::max_element( sid.begin() , sid.end() ) == 1 ) );
  }
 }

// ods ex 12.10: the universal sink on the adjacency matrix in O( n ), by
// at most 3 ( n - 1 ) calls of hasEdge(), against the in- and
// out-degrees, on graphs with a sink, with a node that misses one edge in
// to be one, and random ones
ADS_TEST( "ods.ex12.10.universal-sink" )
{
 std::mt19937 rng( 19 );
 for( int round = 0 ; round < 300 ; ++round ) {
  const int n = 1 + int( rng() % 40 );
  ADS::MatrixGraph<> g( n );
  const int s = int( rng() % unsigned( n ) );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( i != j &&
	( ( round % 3 < 2 && j == s ) || ( i != s && rng() % 4 == 0 ) ) )
     g.insertEdge( i , j );
  if( round % 3 == 1 && n > 1 )
   g.deleteEdge( ( s + 1 ) % n , s );
  int expected = ADS::NIL;
  for( int v = 0 ; v < n ; ++v )
   if( g.adj( v ).empty() && int( g.inAdj( v ).size() ) == n - 1 )
    expected = v;
  CHECK( ADS::universalSink( g ) == expected );
  if( round % 3 == 0 )
   CHECK( expected == s );
  }
 }


/*--------------------------------------------------------------------------*/
/*-------------------- CHAPTER 14, EXTERNAL MEMORY -------------------------*/
/*--------------------------------------------------------------------------*/

// ods 14.1: the store of blocks, one I/O per read and per write, a freed
// block given again by the next placement, and the counters of a
// TwoLevelMemory shared
ADS_TEST( "ods14.1.block-store" )
{
 ADS::TwoLevelMemory mem( 4 , 64 );
 ADS::BlockStore< std::vector< int > > S( &mem );
 const auto a = S.placeBlock( { 1 , 2 } );
 const auto b = S.placeBlock( { 3 } );
 CHECK( a != b && S.blocks() == 2 && S.writes() == 2 && S.reads() == 0 );
 auto x = S.readBlock( a );
 x.push_back( 9 );  // a copy: the disk changes only by a write
 CHECK( S.peek( a ).size() == 2 && S.reads() == 1 );
 S.writeBlock( a , x );
 CHECK( S.peek( a ).size() == 3 && S.writes() == 3 );
 S.freeBlock( b );
 CHECK( S.blocks() == 1 && S.placeBlock( { 7 } ) == b );
 CHECK( mem.reads() == 1 && mem.writes() == 4 );
 CHECK_THROWS( ( void )S.readBlock( 5 ) , ADS::contract_violation );
 }

/// a differential test of the B-tree of a policy against a std::set, the
/// invariant checked at each operation while there are at most 1024 keys,
/// and the I/Os of each operation within a constant times the height
void btreeAgainstSet( int B , ADS::BTreePolicy pol , int ops ,
		      std::uint64_t seed )
{
 ADS::BlockBTree< int > t( B , pol );
 std::set< int > s;
 std::mt19937_64 g( seed );
 for( int i = 0 ; i < ops ; ++i ) {
  const int k = int( g() % 3000 );
  const std::uint64_t io = t.reads() + t.writes();
  if( g() % 5 < 3 )
   CHECK( t.insert( k ) == s.insert( k ).second );
  else
   CHECK( t.remove( k ) == ( s.erase( k ) == 1 ) );
  CHECK( t.size() == s.size() );
  // a walk down and back up, four blocks per level at most
  CHECK( t.reads() + t.writes() - io <= std::uint64_t( 8 * t.height() + 8 ) );
  const int y = int( g() % 3100 );
  const auto it = s.lower_bound( y );
  const auto lb = t.lowerBound( y );
  CHECK( lb.has_value() == ( it != s.end() ) );
  CHECK( ! lb || ( *lb == *it ) );
  }
 std::vector< int > K;
 t.forEach( [ & ]( int k ) { K.push_back( k ); } );
 CHECK( K == std::vector< int >( s.begin() , s.end() ) );
 for( const int k : K )
  CHECK( t.remove( k ) );
 CHECK( t.empty() && t.height() == 1 && t.nodes() == 1 );
 }

// ods 14.2: the B-tree on the blocks against a std::set, for B = 2, 3, 8
// and 64 and the three policies
ADS_TEST( "ods14.2.b-tree" )
{
 for( int B : { 2 , 3 , 8 , 64 } )
  for( auto pol : { ADS::BTreePolicy::STANDARD , ADS::BTreePolicy::WIDE ,
		    ADS::BTreePolicy::THREE } )
   btreeAgainstSet( B , pol , 6000 , std::uint64_t( B ) * 7 + int( pol ) );
 }

/// the B-tree of figure 14.2 of Morin's book, B = 2, its keys doubled so
/// that the 1.5 and the 7.5 of exercise 14.1 are integers
ADS::BlockBTree< int > figureBTree( void )
{
 ADS::BlockBTree< int > t( 2 );
 t.assign( { { { 20 } } ,
	     { { 6 , 12 } , { 28 , 34 , 42 } } ,
	     { { 0 , 2 , 4 } ,
	       { 8 , 10 } ,
	       { 14 , 16 , 18 } ,
	       { 22 , 24 , 26 } ,
	       { 30 , 32 } ,
	       { 36 , 38 , 40 } ,
	       { 44 , 46 } } } );
 return( t );
 }

// ods ex 14.1 and ex 14.2: the insertions of 1.5 and 7.5 into the tree of
// figure 14.2 (two splits of leaves and one of their parent, which gives
// the root a second key), and the removals of 3 and 4 (the key of an
// internal node replaced by its successor, then a borrow from the leaf on
// the left), as the book's rules give them
ADS_TEST( "ods.ex14.1.figure" )
{
 using L = std::vector< std::vector< std::vector< int > > >;
 auto t = figureBTree();
 CHECK( t.size() == 24 && t.height() == 3 );
 CHECK( t.insert( 3 ) && t.insert( 15 ) );
 CHECK( ( t.levels() == L{ { { 12 , 20 } } ,
			   { { 3 , 6 } , { 16 } , { 28 , 34 , 42 } } ,
			   { { 0 , 2 } ,
			     { 4 } ,
			     { 8 , 10 } ,
			     { 14 , 15 } ,
			     { 18 } ,
			     { 22 , 24 , 26 } ,
			     { 30 , 32 } ,
			     { 36 , 38 , 40 } ,
			     { 44 , 46 } } } ) );
 CHECK( t.splits() == 3 );
 auto u = figureBTree();
 CHECK( u.remove( 6 ) );
 CHECK( ( u.levels() == L{ { { 20 } } ,
			   { { 8 , 12 } , { 28 , 34 , 42 } } ,
			   { { 0 , 2 , 4 } ,
			     { 10 } ,
			     { 14 , 16 , 18 } ,
			     { 22 , 24 , 26 } ,
			     { 30 , 32 } ,
			     { 36 , 38 , 40 } ,
			     { 44 , 46 } } } ) );
 CHECK( u.remove( 8 ) );
 CHECK( ( u.levels() == L{ { { 20 } } ,
			   { { 4 , 12 } , { 28 , 34 , 42 } } ,
			   { { 0 , 2 } ,
			     { 10 } ,
			     { 14 , 16 , 18 } ,
			     { 22 , 24 , 26 } ,
			     { 30 , 32 } ,
			     { 36 , 38 , 40 } ,
			     { 44 , 46 } } } ) );
 CHECK( u.borrows() == 1 && u.merges() == 0 );
 }

// ods ex 14.3: a B-tree of n keys has at most ( ( n + 1 ) / B + B - 3 ) /
// ( B - 1 ) internal nodes, since its L leaves hold at least B - 1 keys
// each, n >= B L - 1, and its internal nodes but the root at least B
// children; checked on random trees and on the trees of the smallest
// nodes, built by the removals that follow the insertions
ADS_TEST( "ods.ex14.3.internal-nodes" )
{
 std::mt19937 g( 21 );
 for( int B : { 2 , 3 , 5 , 9 } ) {
  ADS::BlockBTree< int > t( B );
  for( int i = 0 ; i < 4000 ; ++i ) {
   if( g() % 3 )
    t.insert( int( g() % 20000 ) );
   else
    t.remove( int( g() % 20000 ) );
   // the bound for a root that is not a leaf
   const double n = double( t.size() );
   CHECK( ( t.height() == 1 ) ||
	  ( double( t.internalNodes() ) <=
	    ( ( n + 1 ) / B + B - 3 ) / ( B - 1 ) + 1e-9 ) );
   }
  }
 }

// ods ex 14.4: the internal memory of the operations, at most 5 nodes at
// once and the path of the indices, on trees of 5 levels and more, where the
// recursion of the book holds one node per level
ADS_TEST( "ods.ex14.4.memory" )
{
 for( auto pol : { ADS::BTreePolicy::STANDARD , ADS::BTreePolicy::WIDE ,
		   ADS::BTreePolicy::THREE } ) {
  ADS::BlockBTree< int > t( 2 , pol );
  std::mt19937 g( 22 );
  for( int i = 0 ; i < 30000 ; ++i )
   t.insert( int( g() ) );
  for( int i = 0 ; i < 30000 ; ++i )
   t.remove( int( g() ) );
  CHECK( t.height() >= 5 );
  CHECK( t.peakNodes() <= 5 );
  CHECK( t.peakPath() < std::size_t( t.height() + 2 ) );
  }
 }

// ods ex 14.6 and ex 14.7: the cycle of two insertions and two removals
// at a leaf of 2 B - 2 keys costs the book's rules a split, a borrow and a
// merge per cycle, Theta( m ) in all, for any B, while the wide nodes of
// exercise 14.6 and the three-node moves of exercise 14.7 make
// O( m / B ) restructurings
ADS_TEST( "ods.ex14.6.restructures" )
{
 const int B = 16 , cycles = 1000;
 for( auto pol : { ADS::BTreePolicy::STANDARD , ADS::BTreePolicy::WIDE ,
		   ADS::BTreePolicy::THREE } ) {
  ADS::BlockBTree< int > t( B , pol );
  std::vector< int > left , right;
  for( int k = 0 ; k < 2 * B - 2 ; ++k )
   left.push_back( k );
  for( int k = 0 ; k < B ; ++k )
   right.push_back( 1001 + k );
  t.assign( { { { 1000 } } , { left , right } } );
  const std::size_t r = t.restructures();
  for( int c = 0 ; c < cycles ; ++c ) {
   CHECK( t.insert( 500 ) && t.insert( 501 ) );
   CHECK( t.remove( 501 ) && t.remove( 500 ) );
   }
  const std::size_t done = t.restructures() - r;
  if( pol == ADS::BTreePolicy::STANDARD )
   CHECK( done == 3 * std::size_t( cycles ) );
  else
   CHECK( done <= 4 * std::size_t( cycles ) / std::size_t( B ) );
  }
 }

// ods ex 14.7 on the trees of at most two leaves, where the moves of three
// nodes lack the siblings: the cycle of two insertions and two removals
// from a root leaf of 2 B - 2 keys costs THREE a single split, where the
// rules of the book cost three restructurings a cycle; removals down to a
// leaf of floor( B / 2 ) keys cost nothing, one more merges the two
// leaves into a root leaf B / 4 below the top, and a fuller sibling shares
// its keys; random operations on 0 .. 3 B keys against a std::set, the
// invariant checked at each one
ADS_TEST( "ods.ex14.7.small-trees" )
{
 const int B = 16 , cycles = 1000;
 std::vector< int > leaf;
 for( int k = 0 ; k < 2 * B - 2 ; ++k )
  leaf.push_back( k );
 for( auto pol : { ADS::BTreePolicy::STANDARD , ADS::BTreePolicy::THREE } ) {
  ADS::BlockBTree< int > t( B , pol );
  t.assign( { { leaf } } );
  for( int c = 0 ; c < cycles ; ++c ) {
   CHECK( t.insert( 500 ) && t.insert( 501 ) );
   CHECK( t.remove( 501 ) && t.remove( 500 ) );
   }
  if( pol == ADS::BTreePolicy::STANDARD )
   CHECK( t.restructures() == 3 * std::size_t( cycles ) );
  else
   CHECK( t.restructures() == 1 && t.height() == 2 );
  }
 {
  ADS::BlockBTree< int > t( B , ADS::BTreePolicy::THREE );
  std::vector< int > left , right;
  for( int k = 0 ; k < B - 1 ; ++k ) {
   left.push_back( k );
   right.push_back( 100 + k );
   }
  t.assign( { { { 50 } } , { left , right } } );
  for( int k = 0 ; k < B - 1 - B / 2 ; ++k )  // down to floor( B / 2 )
   CHECK( t.remove( k ) );
  CHECK( t.restructures() == 0 && t.height() == 2 );
  CHECK( t.remove( B - 1 - B / 2 ) );  // merged into a root leaf
  CHECK( t.merges() == 1 && t.height() == 1 &&
	 t.size() == std::size_t( B / 2 - 1 + 1 + B - 1 ) );
  ADS::BlockBTree< int > u( B , ADS::BTreePolicy::THREE );
  std::vector< int > full;
  for( int k = 0 ; k < 2 * B - 1 ; ++k )
   full.push_back( 100 + k );
  u.assign( { { { 50 } } , { left , full } } );
  for( int k = 0 ; k <= B - 1 - B / 2 ; ++k )
   CHECK( u.remove( k ) );
  CHECK( u.borrows() == 1 && u.height() == 2 );  // the keys shared
  const auto L = u.levels();
  CHECK( L[ 1 ].size() == 2 && L[ 1 ][ 0 ].size() + 1 >= std::size_t( B ) &&
	 L[ 1 ][ 1 ].size() + 1 >= std::size_t( B ) );
  }
 for( int b : { 2 , 3 , 4 , 8 , 16 } ) {
  ADS::BlockBTree< int > t( b , ADS::BTreePolicy::THREE );
  std::set< int > s;
  std::mt19937 g( static_cast< std::uint32_t >( b ) );
  for( int i = 0 ; i < 20000 ; ++i ) {
   const int x = int( g() % std::uint32_t( 3 * b ) );
   if( g() % 2 )
    CHECK( t.insert( x ) == s.insert( x ).second );
   else
    CHECK( t.remove( x ) == ( s.erase( x ) == 1 ) );
   }
  std::vector< int > K;
  t.forEach( [ & ]( int k ) { K.push_back( k ); } );
  CHECK( K == std::vector< int >( s.begin() , s.end() ) );
  }
 }

// ods ex 14.8: the B+-tree with the removal by borrows and merges,
// lowerBound() the smallest key >= x and findRange( x , y ) the keys in
// ( x , y ], against a std::map, the shape checked at each operation
ADS_TEST( "ods.ex14.8.bplus-tree" )
{
 for( int m : { 3 , 4 , 5 , 8 } ) {
  ADS::BPlusTree< int , int > t( m );
  std::map< int , int > s;
  std::mt19937 g( static_cast< std::uint32_t >( m ) );
  for( int i = 0 ; i < 8000 ; ++i ) {
   const int k = int( g() % 600 );
   if( g() % 2 ) {
    t.insert( k , i );
    s[ k ] = i;
    }
   else {
    t.remove( k );
    s.erase( k );
    }
   CHECK( t.size() == s.size() );
   t.check();
   const int x = int( g() % 620 ) , y = x + int( g() % 50 );
   const auto it = s.lower_bound( x );
   const int * lb = t.lowerBound( x );
   CHECK( ( lb == nullptr ) == ( it == s.end() ) );
   CHECK( ! lb || ( *lb == it->first ) );
   std::vector< std::pair< int , int > > R;
   for( auto j = s.upper_bound( x ) ; j != s.end() && j->first <= y ; ++j )
    R.push_back( *j );
   CHECK( t.findRange( x , y ) == R );
   }
  for( auto [ k , v ] : std::map< int , int >( s ) ) {
   t.remove( k );
   CHECK( ! t.contains( k ) );
   }
  CHECK( t.size() == 0 && t.levels().size() == 1 );
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
/*------------------------- End File ods_ints.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
