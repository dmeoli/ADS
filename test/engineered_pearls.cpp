/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_pearls.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tests of the engineered structures of the notes of Ferragina, beyond
 * the book: the tree of losers with the keys in its nodes and the
 * multiway merge, each against the structure of the notes or the standard
 * library on random and adversarial inputs, and on the limits (empty, one
 * item, duplicates, the largest values of the type).
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

#include "ADS/external/ExternalSorting.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/priority/MultiwayMerge.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// a record with a key and the place it comes from, ordered by the key
struct Rec {
 std::int64_t key;
 std::size_t from;
 std::uint64_t pad[ 3 ];
 };

struct RecLess {
 bool operator()( const Rec & a , const Rec & b ) const {
  return( a.key < b.key );
  }
 };

/// the ranges of the vectors of V
template< class T >
std::vector< std::pair< typename std::vector< T >::const_iterator ,
                        typename std::vector< T >::const_iterator > >
rangesOf( const std::vector< std::vector< T > > & V )
{
 std::vector< std::pair< typename std::vector< T >::const_iterator ,
                         typename std::vector< T >::const_iterator > > R;
 for( const auto & v : V ) R.emplace_back( v.begin() , v.end() );
 return( R );
 }

/// the merge of V through multiwayMerge() equals the stable sort of the
/// concatenation, item by item
template< class T , class C , class E >
void checkMerge( const std::vector< std::vector< T > > & V , C less ,
                 E same )
{
 std::vector< T > all , out;
 for( const auto & v : V ) all.insert( all.end() , v.begin() , v.end() );
 std::stable_sort( all.begin() , all.end() , less );
 multiwayMerge( rangesOf( V ) , std::back_inserter( out ) , less );
 CHECK( out.size() == all.size() );
 for( std::size_t i = 0 ; i < all.size() ; ++i )
  CHECK( same( out[ i ] , all[ i ] ) );
 }

/// k sorted vectors of random lengths up to len and keys below range
template< class T , class C , class G >
std::vector< std::vector< T > > sortedRuns( std::size_t k , std::size_t len ,
                                            std::uint64_t range , C less ,
                                            G && make , std::mt19937_64 & g )
{
 std::vector< std::vector< T > > V( k );
 for( std::size_t s = 0 ; s < k ; ++s ) {
  const std::size_t l = len == 0 ? 0 : g() % ( len + 1 );
  for( std::size_t i = 0 ; i < l ; ++i )
   V[ s ].push_back( make( g() % range , s ) );
  std::stable_sort( V[ s ].begin() , V[ s ].end() , less );
  }
 return( V );
 }

/// the heads of the runs in a std::priority_queue, with the interface of
/// LoserTree, the lower source first on equal keys
template< class T , class C >
class HeapSelector {
 public:
 HeapSelector( std::vector< std::optional< T > > keys , C less )
  : f_H( Worse{ less } ) {
  for( std::size_t s = 0 ; s < keys.size() ; ++s )
   if( keys[ s ] ) f_H.push( P{ *keys[ s ] , s } );
  }
 bool empty( void ) const { return( f_H.empty() ); }
 std::size_t winner( void ) const { return( f_H.top().second ); }
 const T & top( void ) const { return( f_H.top().first ); }
 void replace( std::optional< T > x ) {
  const std::size_t s = f_H.top().second;
  f_H.pop();
  if( x ) f_H.push( P{ *x , s } );
  }
 private:
 using P = std::pair< T , std::size_t >;
 struct Worse {
  C less;
  bool operator()( const P & a , const P & b ) const {
   if( less( b.first , a.first ) ) return( true );
   if( less( a.first , b.first ) ) return( false );
   return( a.second > b.second );
   }
  };
 std::priority_queue< P , std::vector< P > , Worse > f_H;
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// FlatLoserTree against the LoserTree of the notes: the same winners on
// random keys with duplicates, for 1 to 70 sources, the exhausted sources,
// the empty tree and its contracts
ADS_TEST( "flat-loser-tree" )
{
 std::mt19937_64 g( 7 );
 for( std::size_t k = 1 ; k <= 70 ; ++k ) {
  std::vector< std::optional< int > > first( k );
  FlatLoserTree< int > F( k );
  for( std::size_t s = 0 ; s < k ; ++s )
   if( g() % 5 != 0 ) {
    first[ s ] = int( g() % 8 );
    F.start( s , *first[ s ] );
    }
  F.build();
  LoserTree< int > L( first );
  CHECK( F.sources() == k );
  for( int step = 0 ; step < 400 ; ++step ) {
   CHECK( F.empty() == L.empty() );
   if( L.empty() ) break;
   CHECK( F.winner() == L.winner() );
   CHECK( F.top() == L.top() );
   if( g() % 6 == 0 ) {
    F.exhaustTop();
    L.replace( std::nullopt );
    }
   else {
    const int x = L.top() + int( g() % 3 );
    F.replaceTop( x );
    L.replace( x );
    }
   }
  }
 FlatLoserTree< std::string > S( 3 );       // by address, all exhausted
 CHECK( S.empty() );
 CHECK_THROWS( S.top() , contract_violation );
 CHECK_THROWS( S.exhaustTop() , contract_violation );
 CHECK_THROWS( FlatLoserTree< int >( 0 ) , contract_violation );
 }

// multiwayMerge() against the stable sort: integers with the largest and
// the smallest values of the type (the key of an exhausted source), the
// order std::greater, doubles with infinities, strings (keys by address)
// and records of 40 bytes whose equal keys must keep the order of the
// sources; 0 to 300 ranges, empty ones among them
ADS_TEST( "multiway-merge" )
{
 std::mt19937_64 g( 11 );
 const std::int64_t big = std::numeric_limits< std::int64_t >::max() ,
                    low = std::numeric_limits< std::int64_t >::lowest();
 auto eq = []( const auto & a , const auto & b ) { return( a == b ); };
 for( const std::size_t k : { 0 , 1 , 2 , 3 , 4 , 5 , 16 , 17 , 300 } )
  for( const std::size_t len : { 0 , 1 , 7 , 100 } ) {
   checkMerge( sortedRuns< std::int64_t >( k , len , 5 , std::less<>() ,
                [ & ]( std::uint64_t x , std::size_t ) {
                 return( x == 0 ? low : x == 4 ? big : std::int64_t( x ) );
                 } , g ) , std::less<>() , eq );
   checkMerge( sortedRuns< std::uint32_t >( k , len , 1000 ,
                std::greater<>() , [ & ]( std::uint64_t x , std::size_t ) {
                 return( std::uint32_t( x ) ); } , g ) , std::greater<>() ,
               eq );
   checkMerge( sortedRuns< double >( k , len , 6 , std::less<>() ,
                [ & ]( std::uint64_t x , std::size_t ) {
                 return( x == 5 ? std::numeric_limits< double >::infinity()
                         : double( x ) - 2.5 ); } , g ) , std::less<>() ,
               eq );
   checkMerge( sortedRuns< std::string >( k , len , 30 , std::less<>() ,
                [ & ]( std::uint64_t x , std::size_t ) {
                 return( std::string( x % 4 , 'a' ) + char( 'a' + x ) ); } ,
                g ) , std::less<>() , eq );
   checkMerge( sortedRuns< Rec >( k , len , 4 , RecLess() ,
                [ & ]( std::uint64_t x , std::size_t s ) {
                 return( Rec{ std::int64_t( x ) , s , { 0 , 0 , 0 } } ); } ,
                g ) , RecLess() , []( const Rec & a , const Rec & b ) {
                 return( ( a.key == b.key ) && ( a.from == b.from ) ); } );
   }
 std::vector< std::vector< int > > bad = { { 1 , 2 } , { 3 , 1 } ,
                                           { 0 } };
 std::vector< int > out;
 CHECK_THROWS( multiwayMerge( rangesOf( bad ) ,
                             std::back_inserter( out ) ) ,
               contract_violation );
 }

// multiwayMerge() into a contiguous output, by the funnel of binary
// mergers beyond 4 sources: items of 8 KiB, more than its buffer of 4 KiB
// holds, which must hold one at least, and items of 40 bytes, on 5 to 40
// sources of random lengths, against the stable sort
ADS_TEST( "multiway-merge-large" )
{
 struct Big {
  std::int64_t key;
  std::size_t from;
  char pad[ 8192 - 16 ];
  };
 auto less = []( const Big & a , const Big & b ) { return( a.key < b.key ); };
 std::mt19937_64 g( 13 );
 for( const std::size_t k : { 5 , 6 , 17 , 40 } ) {
  std::vector< std::vector< Big > > V( k );
  for( std::size_t s = 0 ; s < k ; ++s ) {
   V[ s ].resize( 1 + g() % 9 );
   for( auto & b : V[ s ] ) b = Big{ std::int64_t( g() % 7 ) , s , {} };
   std::stable_sort( V[ s ].begin() , V[ s ].end() , less );
   }
  std::vector< Big > all;
  for( const auto & v : V ) all.insert( all.end() , v.begin() , v.end() );
  std::stable_sort( all.begin() , all.end() , less );
  std::vector< Big > out( all.size() , Big{ -1 , 0 , {} } );
  const auto end = multiwayMerge( rangesOf( V ) , out.begin() , less );
  CHECK( end == out.end() );
  for( std::size_t i = 0 ; i < all.size() ; ++i )
   CHECK( ( out[ i ].key == all[ i ].key ) &&
          ( out[ i ].from == all[ i ].from ) );
  std::vector< std::vector< Rec > > W( k );
  for( std::size_t s = 0 ; s < k ; ++s ) {
   W[ s ].resize( g() % 300 );
   for( auto & r : W[ s ] ) r = Rec{ std::int64_t( g() % 50 ) , s , {} };
   std::stable_sort( W[ s ].begin() , W[ s ].end() , RecLess() );
   }
  std::vector< Rec > rall;
  for( const auto & w : W ) rall.insert( rall.end() , w.begin() , w.end() );
  std::stable_sort( rall.begin() , rall.end() , RecLess() );
  std::vector< Rec > rout( rall.size() );
  CHECK( multiwayMerge( rangesOf( W ) , rout.begin() , RecLess() ) ==
         rout.end() );
  for( std::size_t i = 0 ; i < rall.size() ; ++i )
   CHECK( ( rout[ i ].key == rall[ i ].key ) &&
          ( rout[ i ].from == rall[ i ].from ) );
  }
 }

// multiwayMergeSort() of the notes with the selector of its merge given:
// the tree of losers of the notes and a heap of the standard library give
// the same output and the same I/Os, on random keys with duplicates and
// several passes of merge, and on the empty file
ADS_TEST( "book-mergesort-selectors" )
{
 std::mt19937_64 g( 41 );
 for( const std::size_t n : { 0 , 1 , 100 , 5000 , 40000 } ) {
  std::vector< long > A( n );
  for( auto & x : A ) x = long( g() % 500 );
  TwoLevelMemory mem( 16 , 64 );            // fan-in 3, several passes
  const DiskFile< long > in( mem , A );
  DiskFile< long > a( mem ) , b( mem );
  mem.resetCounters();
  multiwayMergeSort( in , a );
  const std::uint64_t ios = mem.ios();
  mem.resetCounters();
  multiwayMergeSort< long , std::less<> ,
                     HeapSelector< long , std::less<> > >( in , b );
  CHECK( mem.ios() == ios );
  std::sort( A.begin() , A.end() );
  CHECK( a.contents() == A );
  CHECK( b.contents() == A );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File engineered_pearls.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
