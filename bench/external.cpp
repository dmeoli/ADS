/*--------------------------------------------------------------------------*/
/*--------------------------- File external.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the merges and of the samplings of the notes of
 * Ferragina:
 *
 * - merge: multiwayMerge() of ADS and FlatLoserTree alone against the
 *   merge through std::priority_queue, the kWayMerge() of the notes [see
 *   LoserTree.h], __gnu_parallel::multiway_merge() of libstdc++ run
 *   sequentially and, if tlx is given, tlx::multiway_merge() and
 *   tlx::stable_multiway_merge(), on k = 4 , 16 , 64 , 256 , 1024 sorted
 *   runs of random keys of 64 bits and of records of 32 bytes by a key of
 *   64 bits, n items in all; FlatLoserTree is the tree of losers of
 *   libstdc++ and of tlx, the same algorithm, and the comparisons per item
 *   of each, counted in a run apart, are the cells of "multiway merge
 *   counts";
 * - book mergesort: multiwayMergeSort() of the notes [see
 *   ExternalSorting.h] in the simulated two-level memory, blocks of B =
 *   512 items and fan-in k = 16 , 64 , 256, the same algorithm with the
 *   least head of the runs chosen by the tree of losers of the notes
 *   (ADS::LoserTree), by FlatLoserTree, by std::priority_queue and, if
 *   Boost is found, by boost::heap::d_ary_heap of arity 2 and 4; the I/Os
 *   and the comparisons per item of each, counted in a run apart, are the
 *   cells of "book mergesort counts", the I/Os the same for all;
 * - sampling: n of N = 10^8 positions by sampleBySkips() (Vitter's
 *   Algorithm D), sampleBySorting() and sampleByDictionary() against
 *   std::ranges::sample() on std::views::iota (selection sampling, a pass
 *   over the N positions); nanoseconds per item sampled.
 *
 * Each cell is the median of r runs, in nanoseconds per item; the input is
 * made before the clock starts. The contracts are off (ADS_CONTRACTS=0).
 *
 *   external merge [ -r runs ] [ -s seed ] n1 n2 ...
 *   external sampling [ -r runs ] [ -s seed ] n1 n2 ...
 *   external book [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/external/ExternalSorting.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/RandomSampling.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/priority/MultiwayMerge.h"

#include <parallel/multiway_merge.h>
#include <parallel/merge.h>

#if defined( ADS_HAS_TLX )
 #include <tlx/algorithm/multiway_merge.hpp>
#endif

#if __has_include( <boost/heap/d_ary_heap.hpp> )
 #include <boost/heap/d_ary_heap.hpp>
 #define ADS_HAS_BOOST_HEAP 1
#endif

#include <cstdlib>
#include <functional>
#include <optional>
#include <queue>
#include <ranges>
#include <random>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- INPUTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

using namespace ADS::bench;

/// a record of 32 bytes sorted by its key
struct Record {
 std::uint64_t key;
 std::uint64_t pad[ 3 ];
 };

struct ByKey {
 bool operator()( const Record & a , const Record & b ) const {
  return( a.key < b.key );
  }
 };

std::uint64_t keyOf( std::uint64_t x ) { return( x ); }
std::uint64_t keyOf( const Record & x ) { return( x.key ); }

template< class T > T make( std::uint64_t k );
template<> std::uint64_t make( std::uint64_t k ) { return( k ); }
template<> Record make( std::uint64_t k ) {
 return( Record{ k , { k , k , k } } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- MERGES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the merges of k runs of n / k items in all of type T
template< class T , class C >
void merges( const char * type , std::size_t n , std::size_t k , int r ,
             unsigned seed , C less )
{
 std::mt19937_64 g( seed + k );
 std::vector< std::vector< T > > R( k );
 for( std::size_t i = 0 ; i < n ; ++i )
  R[ i % k ].push_back( make< T >( g() ) );
 for( auto & v : R ) std::sort( v.begin() , v.end() , less );
 using It = typename std::vector< T >::iterator;
 std::vector< std::pair< It , It > > ranges;
 for( auto & v : R ) ranges.emplace_back( v.begin() , v.end() );
 std::vector< T > out( n );
 const std::string what = std::string( type ) + " k=" +
                          std::to_string( k );
 auto check = [ & ]( void ) {
  sink = sink + keyOf( out[ n / 2 ] ) + keyOf( out[ n - 1 ] ); };
 auto none = [ & ]( void ) { return( 0 ); };
 for( const char * who : { "std::priority_queue" ,
                           "std::priority_queue A/A" } )
  if( ! skip( who ) )
   print( "multiway merge" , what , n , who , median( none , [ & ]( int ) {
    using P = std::pair< T , std::size_t >;
    auto greater = [ & ]( const P & a , const P & b ) {
     return( less( b.first , a.first ) ); };
    std::priority_queue< P , std::vector< P > , decltype( greater ) >
     Q( greater );
    std::vector< std::size_t > next( k , 1 );
    for( std::size_t s = 0 ; s < k ; ++s )
     if( ! R[ s ].empty() ) Q.emplace( R[ s ][ 0 ] , s );
    T * o = out.data();
    while( ! Q.empty() ) {
     const std::size_t s = Q.top().second;
     *o++ = Q.top().first;
     Q.pop();
     if( next[ s ] < R[ s ].size() ) Q.emplace( R[ s ][ next[ s ]++ ] , s );
     }
    check(); } , r , n ) );
 if( ! skip( "ADS::kWayMerge" ) )
  print( "multiway merge" , what , n , "ADS::kWayMerge" , median( none ,
   [ & ]( int ) { out = ADS::kWayMerge( R , less ); check(); } , r , n ) );
 if( ! skip( "ADS::multiwayMerge" ) )
  print( "multiway merge" , what , n , "ADS::multiwayMerge" , median( none ,
   [ & ]( int ) {
    ADS::multiwayMerge( ranges , out.begin() , less ); check(); } , r ,
   n ) );
 if( ! skip( "libstdc++ multiway_merge" ) )
  print( "multiway merge" , what , n , "libstdc++ multiway_merge" ,
  median( none , [ & ]( int ) {
   auto S = ranges;
   __gnu_parallel::multiway_merge( S.begin() , S.end() , out.begin() ,
                                   std::ptrdiff_t( n ) , less ,
                                   __gnu_parallel::sequential_tag() );
   check(); } , r , n ) );
 // the tree of losers alone, the algorithm of tlx and libstdc++
 auto tree = [ & ]( auto cmp ) {
  ADS::FlatLoserTree< T , decltype( cmp ) > L( k , cmp );
  std::vector< const T * > p( k ) , e( k );
  for( std::size_t s = 0 ; s < k ; ++s ) {
   p[ s ] = R[ s ].data();
   e[ s ] = p[ s ] + R[ s ].size();
   if( p[ s ] != e[ s ] ) L.start( s , *p[ s ] );
   }
  L.build();
  T * o = out.data();
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::size_t s = L.winner();
   *o++ = L.top();
   if( ++p[ s ] != e[ s ] ) L.replaceTop( *p[ s ] );
   else L.exhaustTop();
   }
  };
 if( ! skip( "ADS::FlatLoserTree" ) )
  print( "multiway merge" , what , n , "ADS::FlatLoserTree" , median( none ,
   [ & ]( int ) { tree( less ); check(); } , r , n ) );
 {                                          // the comparisons per item
  std::uint64_t count = 0;
  auto counted = [ & ]( const T & a , const T & b ) {
   ++count;
   return( less( a , b ) ); };
  if( ! skip( "ADS::FlatLoserTree comparisons" ) ) {
   count = 0;
   tree( counted );
   print( "multiway merge counts" , what , n ,
          "ADS::FlatLoserTree comparisons" , double( count ) / double( n ) );
   }
  if( ! skip( "ADS::multiwayMerge comparisons" ) ) {
   count = 0;
   ADS::multiwayMerge( ranges , out.begin() , counted );
   print( "multiway merge counts" , what , n ,
          "ADS::multiwayMerge comparisons" , double( count ) / double( n ) );
   }
#if defined( ADS_HAS_TLX )
  if( ! skip( "tlx::multiway_merge comparisons" ) ) {
   count = 0;
   auto S = ranges;
   tlx::multiway_merge( S.begin() , S.end() , out.begin() ,
                        std::ptrdiff_t( n ) , counted );
   print( "multiway merge counts" , what , n ,
          "tlx::multiway_merge comparisons" , double( count ) / double( n ) );
   }
  if( ! skip( "tlx::stable_multiway_merge comparisons" ) ) {
   count = 0;
   auto S = ranges;
   tlx::stable_multiway_merge( S.begin() , S.end() , out.begin() ,
                               std::ptrdiff_t( n ) , counted );
   print( "multiway merge counts" , what , n ,
          "tlx::stable_multiway_merge comparisons" ,
          double( count ) / double( n ) );
   }
#endif
 }
#if defined( ADS_HAS_TLX )
 if( ! skip( "tlx::multiway_merge" ) )
  print( "multiway merge" , what , n , "tlx::multiway_merge" , median( none ,
   [ & ]( int ) {
    auto S = ranges;
    tlx::multiway_merge( S.begin() , S.end() , out.begin() ,
                         std::ptrdiff_t( n ) , less );
    check(); } , r , n ) );
 if( ! skip( "tlx::stable_multiway_merge" ) )
  print( "multiway merge" , what , n , "tlx::stable_multiway_merge" ,
  median( none , [ & ]( int ) {
   auto S = ranges;
   tlx::stable_multiway_merge( S.begin() , S.end() , out.begin() ,
                               std::ptrdiff_t( n ) , less );
   check(); } , r , n ) );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ SAMPLING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the samplings of n items
void samplings( std::size_t n , int r , unsigned seed )
{
 const std::size_t N = 100000000;
 std::mt19937_64 g( seed );
 auto none = [ & ]( void ) { return( 0 ); };
 const std::string what = "positions of 1e8";
 for( const char * who : { "std::sample" , "std::sample A/A" } )
  if( ! skip( who ) )
   print( "sampling" , what , n , who , median( none , [ & ]( int ) {
    std::vector< int > out( n );          // on 32 bits: the difference
    std::ranges::sample( std::views::iota( 0 , int( N ) ) , // of 64 is
                         out.begin() , std::ptrdiff_t( n ) , g ); // wider
    sink = sink + std::size_t( out[ n / 2 ] ); } , r , n ) );
 if( ! skip( "ADS::sampleBySkips" ) )
  print( "sampling" , what , n , "ADS::sampleBySkips" , median( none ,
   [ & ]( int ) {
    sink = sink + ADS::sampleBySkips( n , N , g )[ n / 2 ]; } , r , n ) );
 if( ! skip( "ADS::sampleBySorting" ) )
  print( "sampling" , what , n , "ADS::sampleBySorting" , median( none ,
   [ & ]( int ) {
    sink = sink + ADS::sampleBySorting( n , N , g )[ n / 2 ]; } , r , n ) );
 if( ! skip( "ADS::sampleByDictionary" ) )
  print( "sampling" , what , n , "ADS::sampleByDictionary" , median( none ,
   [ & ]( int ) {
    sink = sink + ADS::sampleByDictionary( n , N , g )[ n / 2 ]; } , r ,
   n ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- BOOK MERGESORT -------------------------------*/
/*--------------------------------------------------------------------------*/

/// FlatLoserTree with the interface of LoserTree: the keys it holds kept
/// here, so that one held by address stays valid
template< class T , class C >
class FlatSelector {
 public:
 FlatSelector( std::vector< std::optional< T > > keys , C less )
  : f_L( keys.size() , less ) , f_cur( keys.size() ) {
  for( std::size_t s = 0 ; s < keys.size() ; ++s )
   if( keys[ s ] ) {
    f_cur[ s ] = *keys[ s ];
    f_L.start( s , f_cur[ s ] );
    }
  f_L.build();
  }
 bool empty( void ) const { return( f_L.empty() ); }
 std::size_t winner( void ) const { return( f_L.winner() ); }
 const T & top( void ) const { return( f_L.top() ); }
 void replace( std::optional< T > x ) {
  if( ! x ) {
   f_L.exhaustTop();
   return;
   }
  const std::size_t s = f_L.winner();
  f_cur[ s ] = *x;
  f_L.replaceTop( f_cur[ s ] );
  }
 private:
 ADS::FlatLoserTree< T , C > f_L;
 std::vector< T > f_cur;
 };

/// the heads in a heap H of pairs ( key , source ), the lower source first
/// on equal keys, with the interface of LoserTree
template< class T , class C , class H >
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
 using P = std::pair< T , std::size_t >;
 struct Worse {                             // a max-heap of the worse
  C less;
  bool operator()( const P & a , const P & b ) const {
   if( less( b.first , a.first ) ) return( true );
   if( less( a.first , b.first ) ) return( false );
   return( a.second > b.second );
   }
  };
 private:
 typename H::template rebind< P , Worse >::type f_H;
 };

/// std::priority_queue as H of HeapSelector
struct StdHeap {
 template< class P , class W >
 struct rebind { using type = std::priority_queue< P , std::vector< P > ,
                                                   W >; };
 };

#if defined( ADS_HAS_BOOST_HEAP )
/// boost::heap::d_ary_heap of arity D as H of HeapSelector
template< unsigned D >
struct BoostHeap {
 template< class P , class W >
 struct rebind {
  using type = boost::heap::d_ary_heap< P , boost::heap::arity< D > ,
                                        boost::heap::compare< W > >;
  };
 };
#endif

/// the book mergesort of n random keys of 64 bits by each structure
void books( std::size_t n , int r , unsigned seed )
{
 using T = std::uint64_t;
 const std::size_t B = 512;
 std::mt19937_64 g( seed );
 std::vector< T > A( n );
 for( auto & x : A ) x = g();
 for( const std::size_t k : { 16 , 64 , 256 } ) {
  ADS::TwoLevelMemory mem( B , ( k + 1 ) * B );
  const ADS::DiskFile< T > in( mem , A );
  const std::string what = "u64 k=" + std::to_string( k );
  std::uint64_t count = 0;
  auto run = [ & ]( const char * who , auto sel , bool timed ) {
   using S = typename decltype( sel )::type;
   using CC = typename decltype( sel )::cmp;
   auto none = [ & ]( void ) { return( 0 ); };
   // the counts measured if one of their two labels is, both listed
   const bool ios = skip( std::string( who ) + ( timed ? "" : " I/Os" ) );
   if( ( timed || skip( std::string( who ) + " comparisons" ) ) && ios )
    return;
   if( timed )
    print( "book mergesort" , what , n , who , median( none , [ & ]( int ) {
     ADS::DiskFile< T > out( mem );
     ADS::multiwayMergeSort< T , std::less<> , S >( in , out );
     sink = sink + out.size(); } , r , n ) );
   else {                                   // the I/Os and comparisons
    mem.resetCounters();
    count = 0;
    ADS::DiskFile< T > out( mem );
    ADS::multiwayMergeSort< T , CC , typename decltype( sel )::ctype >(
     in , out , CC{ &count } );
    print( "book mergesort counts" , what , n ,
           std::string( who ) + " I/Os" , double( mem.ios() ) );
    print( "book mergesort counts" , what , n ,
           std::string( who ) + " comparisons" ,
           double( count ) / double( n ) );
    }
   };
  struct Counting {                         // a less that counts
   std::uint64_t * c;
   bool operator()( const T & a , const T & b ) const {
    ++*c;
    return( a < b );
    }
   };
  using L = std::less<>;
  struct Book { using type = ADS::LoserTree< T , L >; using cmp = Counting;
                using ctype = ADS::LoserTree< T , Counting >; };
  struct Flat { using type = FlatSelector< T , L >; using cmp = Counting;
                using ctype = FlatSelector< T , Counting >; };
  struct Std { using type = HeapSelector< T , L , StdHeap >;
               using cmp = Counting;
               using ctype = HeapSelector< T , Counting , StdHeap >; };
  for( const bool timed : { true , false } ) {
   run( "std::priority_queue" , Std() , timed );
   if( timed ) run( "std::priority_queue A/A" , Std() , timed );
   run( "ADS::LoserTree" , Book() , timed );
   run( "ADS::FlatLoserTree" , Flat() , timed );
#if defined( ADS_HAS_BOOST_HEAP )
   struct B2 { using type = HeapSelector< T , L , BoostHeap< 2 > >;
               using cmp = Counting;
               using ctype = HeapSelector< T , Counting , BoostHeap< 2 > >; };
   struct B4 { using type = HeapSelector< T , L , BoostHeap< 4 > >;
               using cmp = Counting;
               using ctype = HeapSelector< T , Counting , BoostHeap< 4 > >; };
   run( "boost::heap::d_ary_heap d=2" , B2() , timed );
   run( "boost::heap::d_ary_heap d=4" , B4() , timed );
#endif
   }
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 std::string mode = "merge";                // the word may be anywhere
 std::vector< char * > rest;
 for( int i = 0 ; i < argc ; ++i ) {
  const std::string a = argv[ i ];
  if( ( i > 0 ) && ( ( a == "merge" ) || ( a == "sampling" ) ||
                     ( a == "book" ) ) ) mode = a;
  else rest.push_back( argv[ i ] );
  }
 Options o( int( rest.size() ) , rest.data() ,
            std::vector< std::size_t >{ 1000000 , 10000000 } );
 if( mode == "book" ) {
  for( const std::size_t n : o.sizes ) books( n , o.runs , o.seed );
  return( 0 );
  }
 if( mode == "sampling" ) {
  for( const std::size_t n : o.sizes ) samplings( n , o.runs , o.seed );
  return( 0 );
  }
 for( const std::size_t n : o.sizes )
  for( const std::size_t k : { 4 , 16 , 64 , 256 , 1024 } ) {
   merges< std::uint64_t >( "u64" , n , k , o.runs , o.seed ,
                            std::less<>() );
   merges< Record >( "record" , n , k , o.runs , o.seed , ByKey() );
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File external.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
