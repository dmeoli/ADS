/*--------------------------------------------------------------------------*/
/*--------------------------- File hashing.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the hashing, in four parts, each with its competitors
 * when their headers are found (and Abseil when ADS_HAS_ABSL is defined,
 * since it is linked):
 *
 * - "hash functions": the hashes of n random words of 64 bits, each
 *   depending on the hash before it (the latency of a hash), and of n
 *   strings of fixed lengths from 3 to 4096 bytes, and of random lengths
 *   from 1 to 32, at random places of a pool of 256 KB, by FastHash of ADS
 *   against std::hash, boost::hash, absl::Hash, the hash of
 *   ankerl::unordered_dense, XXH3, wyhash and rapidhash;
 * - "hash maps": the n insertions into an empty map, n lookups of present
 *   keys in random order, n lookups of absent keys, the n removals in
 *   random order and a visit of the pairs, on keys of 64 bits and on
 *   strings of 8 to 24 characters, with values of 64 bits, by the hash
 *   tables of ADS (DenseHashMap, with its pairs in a dense vector, against
 *   the same layout of unordered_dense) against std::unordered_map,
 *   boost::unordered_map,
 *   unordered_node_map and unordered_flat_map, absl::flat_hash_map,
 *   ankerl::unordered_dense::map, tsl::robin_map and the cuckoo table of
 *   libcuckoo; each table with the default hash of its library, and those
 *   of ADS with FastHash;
 * - "filters": the build of a filter of n random keys of 64 bits (by n
 *   insertions for the dynamic ones), n queries of present and of absent
 *   keys and, for the cuckoo filters, the n removals, by the filters of
 *   ADS against the blocked filter of Bloom, the cuckoo filter and the
 *   standard Ribbon filter (of RocksDB) of fastfilter_cpp and the xor and
 *   binary fuse filters of xor_singleheader;
 * - "perfect hashing": the build of a minimal perfect hash function of n
 *   random keys of 64 bits and n queries, by those of ADS (by pilots and
 *   by recursive splitting) against PTHash, PHOBIC, BBHash and RecSplit.
 *
 * Each cell is the median of r runs, in nanoseconds per operation (per key
 * for the builds); the queries run on one structure built once, and the
 * contracts are off (ADS_CONTRACTS=0). The bits per key and the false
 * positives of the filters and of the functions go to the standard error.
 *
 *   hashing [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * The environment variable PARTS, e.g. PARTS=filters,perfect, runs only
 * the parts named in it, by the first word of their names.
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

#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/CuckooHashing.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/MinimalPerfectHash.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/PerfectHashing.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/BloomFilter.h"
#include "ADS/set/BloomVariants.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/RibbonFilter.h"

#if __has_include( <boost/unordered/unordered_flat_map.hpp> )
 #include <boost/container_hash/hash.hpp>
 #include <boost/unordered/unordered_flat_map.hpp>
 #include <boost/unordered/unordered_map.hpp>
 #include <boost/unordered/unordered_node_map.hpp>
 #define ADS_HAS_BOOST 1
#endif
#ifdef ADS_HAS_ABSL
 #include <absl/container/flat_hash_map.h>
 #include <absl/hash/hash.h>
#endif
#if __has_include( <ankerl/unordered_dense.h> )
 #include <ankerl/unordered_dense.h>
 #define ADS_HAS_ANKERL 1
#endif
#if __has_include( <tsl/robin_map.h> )
 #include <tsl/robin_map.h>
 #define ADS_HAS_TSL 1
#endif
#if __has_include( <libcuckoo/cuckoohash_map.hh> )
 #include <libcuckoo/cuckoohash_map.hh>
 #define ADS_HAS_LIBCUCKOO 1
#endif
#if __has_include( <xxhash.h> )
 #define XXH_INLINE_ALL
 #include <xxhash.h>
 #define ADS_HAS_XXH 1
#endif
#if __has_include( <wyhash.h> )
 #include <wyhash.h>
 #define ADS_HAS_WYHASH 1
#endif
#if __has_include( <rapidhash.h> )
 #include <rapidhash.h>
 #define ADS_HAS_RAPIDHASH 1
#endif
#if __has_include( <bloom/simd-block.h> ) && defined( __AVX2__ )
 #include <bloom/simd-block.h>
 #define ADS_HAS_FASTFILTER 1
#endif
#if __has_include( <cuckoo/cuckoofilter.h> )
 #include <cuckoo/cuckoofilter.h>
 #define ADS_HAS_CUCKOOFILTER 1
#endif
#if __has_include( <ribbon/ribbon_impl.h> )
 #include <ribbon/ribbon_impl.h>
 #define ADS_HAS_RIBBON 1
#endif
#if __has_include( <binaryfusefilter.h> )
 #include <binaryfusefilter.h>
 #include <xorfilter.h>
 #define ADS_HAS_XORFILTER 1
#endif
#if __has_include( <pthash.hpp> )
 #include <pthash.hpp>
 #define ADS_HAS_PTHASH 1
#endif
#if __has_include( <BooPHF.h> )
 #include <BooPHF.h>
 #define ADS_HAS_BBHASH 1
#endif
#if __has_include( <sux/function/RecSplit.hpp> )
 #include <sux/function/RecSplit.hpp>
 #define ADS_HAS_RECSPLIT 1
#endif

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace ADS::bench;
using Key = std::uint64_t;

namespace {

/*--------------------------------------------------------------------------*/
/*---------------------------- HASH FUNCTIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the strings of the hashes: n places of a pool of 256 KB, which stays in
/// the cache of level 2, each with its length, all of length len or, for
/// len = 0, of random lengths 1 to 32
struct Strings {
 std::vector< unsigned char > pool;
 std::vector< std::pair< std::size_t , std::size_t > > at;

 Strings( std::size_t n , std::size_t len , unsigned seed ) {
  std::mt19937_64 g( seed );
  pool.resize( ( std::size_t( 1 ) << 18 ) + 4096 );
  for( auto & c : pool ) c = static_cast< unsigned char >( g() );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::size_t l = len ? len : 1 + g() % 32;
   at.emplace_back( g() % ( std::size_t( 1 ) << 18 ) , l );
   }
  }
 };

/// the hashes of the strings of S by h( p , l ), in nanoseconds per string
template< class F >
void hashes( const char * what , const std::string & who , const Strings & S ,
             int r , F && h )
{
 print( "hash functions" , what , S.at.size() , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t s = 0;
          for( const auto & [ p , l ] : S.at )
           s += h( S.pool.data() + p , l );
          sink = sink + s; } , r , S.at.size() ) );
 }

/// the hashes of the words of K by h( k ), each XORed with the hash before
/// it, so that a hash waits for the one before (the latency of a hash, as
/// in the search of a table, not the throughput of a loop that the
/// compiler vectorizes), in nanoseconds per word
template< class F >
void words( const std::string & who , const std::vector< Key > & K , int r ,
            F && h )
{
 print( "hash functions" , "word" , K.size() , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t s = 0;
          for( Key k : K ) s = h( k ^ s );
          sink = sink + s; } , r , K.size() ) );
 }

void hashFunctions( std::size_t n , const std::vector< Key > & K , int r ,
                    unsigned seed )
{
 for( const char * who : { "ADS::FastHash" , "ADS::FastHash A/A" } )
  words( who , K , r , []( Key k ) { return( ADS::hashWord( k ) ); } );
 words( "std::hash identity" , K , r , []( Key k ) {
        return( std::hash< Key >()( k ) ); } );
 words( "ADS::splitmix64" , K , r , []( Key k ) {
        return( ADS::splitmix64( k ) ); } );
#ifdef ADS_HAS_BOOST
 words( "boost::hash identity" , K , r , []( Key k ) {
        return( boost::hash< Key >()( k ) ); } );
#endif
#ifdef ADS_HAS_ABSL
 words( "absl::Hash" , K , r , []( Key k ) {
        return( absl::Hash< Key >()( k ) ); } );
#endif
#ifdef ADS_HAS_ANKERL
 words( "ankerl::hash" , K , r , []( Key k ) {
        return( ankerl::unordered_dense::hash< Key >()( k ) ); } );
#endif
#ifdef ADS_HAS_WYHASH
 words( "wyhash" , K , r , []( Key k ) { return( wyhash64( k , 0 ) ); } );
#endif
#ifdef ADS_HAS_XXH
 words( "XXH3" , K , r , []( Key k ) {
        return( XXH3_64bits( &k , sizeof( k ) ) ); } );
#endif
#ifdef ADS_HAS_RAPIDHASH
 words( "rapidhash" , K , r , []( Key k ) {
        return( rapidhash( &k , sizeof( k ) ) ); } );
#endif

 using U = const unsigned char *;
 auto view = []( U p , std::size_t l ) {
  return( std::string_view( reinterpret_cast< const char * >( p ) , l ) ); };
 for( std::size_t len : { 3 , 8 , 16 , 24 , 64 , 256 , 4096 , 0 } ) {
  const Strings S( len > 256 ? n / 16 : n , len , seed );
  const std::string what = len ? "bytes " + std::to_string( len ) :
                                  std::string( "bytes 1-32" );
  for( const char * who : { "ADS::FastHash" , "ADS::FastHash A/A" } )
   hashes( what.c_str() , who , S , r , []( U p , std::size_t l ) {
           return( ADS::hashBytes( p , l ) ); } );
  hashes( what.c_str() , "std::hash" , S , r , [ & ]( U p , std::size_t l ) {
          return( std::hash< std::string_view >()( view( p , l ) ) ); } );
#ifdef ADS_HAS_BOOST
  hashes( what.c_str() , "boost::hash" , S , r , [ & ]( U p ,
                                                       std::size_t l ) {
          return( boost::hash< std::string_view >()( view( p , l ) ) ); } );
#endif
#ifdef ADS_HAS_ABSL
  hashes( what.c_str() , "absl::Hash" , S , r , [ & ]( U p ,
                                                      std::size_t l ) {
          return( absl::Hash< std::string_view >()( view( p , l ) ) ); } );
#endif
#ifdef ADS_HAS_ANKERL
  hashes( what.c_str() , "ankerl::hash" , S , r , [ & ]( U p ,
                                                        std::size_t l ) {
          return( ankerl::unordered_dense::hash< std::string_view >()(
                   view( p , l ) ) ); } );
#endif
#ifdef ADS_HAS_XXH
  hashes( what.c_str() , "XXH3" , S , r , []( U p , std::size_t l ) {
          return( XXH3_64bits( p , l ) ); } );
#endif
#ifdef ADS_HAS_WYHASH
  hashes( what.c_str() , "wyhash" , S , r , []( U p , std::size_t l ) {
          return( wyhash( p , l , 0 , _wyp ) ); } );
#endif
#ifdef ADS_HAS_RAPIDHASH
  hashes( what.c_str() , "rapidhash" , S , r , []( U p , std::size_t l ) {
          return( rapidhash( p , l ) ); } );
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- HASH MAPS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// n distinct random strings of 8 to 24 letters and digits
std::vector< std::string > words( std::size_t n , unsigned seed )
{
 static const char a[] = "abcdefghijklmnopqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
 std::mt19937_64 g( seed );
 std::unordered_set< std::string > seen;
 std::vector< std::string > W;
 while( W.size() < n ) {
  std::string s( 8 + g() % 17 , ' ' );
  for( auto & c : s ) c = a[ g() % 62 ];
  if( seen.insert( s ).second ) W.push_back( std::move( s ) );
  }
 return( W );
 }

/// the five cases on the map that make() builds, on the keys K (inserted),
/// H (K in random order) and Q (absent), by ins( d , k ) , find( d , k )
/// (the value, or 0), erase( d , k ) and sum( d ) (the sum of the values
/// by the iterators)
template< class T , class M , class I , class F , class E , class S >
void maps( const std::string & type , const std::string & who ,
           const std::vector< T > & K , const std::vector< T > & H ,
           const std::vector< T > & Q , int r , M && make , I && ins ,
           F && find , E && erase , S && sum )
{
 const std::size_t n = K.size();
 print( "hash maps" , "insert " + type , n , who , median(
         make , [ & ]( auto & e ) {
          for( std::size_t i = 0 ; i < n ; ++i ) ins( e , K[ i ] , i + 1 );
          } , r , n ) );
 auto d = make();
 for( std::size_t i = 0 ; i < n ; ++i ) ins( d , K[ i ] , i + 1 );
 print( "hash maps" , "lookup hit " + type , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          Key s = 0;
          for( const T & k : H ) s += find( d , k );
          sink = sink + s; } , r , n ) );
 print( "hash maps" , "lookup miss " + type , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          Key s = 0;
          for( const T & k : Q ) s += find( d , k );
          sink = sink + s; } , r , n ) );
 print( "hash maps" , "visit " + type , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) { sink = sink + sum( d ); } ,
         r , n ) );
 print( "hash maps" , "erase " + type , n , who , median(
         [ & ](){
          auto e = make();
          for( std::size_t i = 0 ; i < n ; ++i ) ins( e , K[ i ] , i + 1 );
          return( e ); } ,
         [ & ]( auto & e ) { for( const T & k : H ) erase( e , k ); } , r ,
         n ) );
 }

/// the sum of the values of a range of pairs
template< class D >
Key total( const D & d )
{
 Key s = 0;
 for( const auto & [ k , v ] : d ) s += v;
 return( s );
 }

/// the smallest prime >= x
std::size_t prime( std::size_t x )
{
 for( ; ; ++x ) {
  bool p = x > 1;
  for( std::size_t d = 2 ; p && ( d * d <= x ) ; ++d ) p = x % d;
  if( p ) return( x );
  }
 }

/// the battery of the maps on the keys of type T, named type
template< class T >
void hashMaps( const std::string & type , const std::vector< T > & K ,
               const std::vector< T > & H , const std::vector< T > & Q ,
               int r )
{
 const std::size_t n = K.size();
 using FH = ADS::FastHash< T >;
 auto stdIns = []( auto & d , const T & k , Key v ) { d.emplace( k , v ); };
 auto stdFind = []( const auto & d , const T & k ) {
  const auto it = d.find( k );
  return( it == d.end() ? Key( 0 ) : it->second ); };
 auto stdErase = []( auto & d , const T & k ) { d.erase( k ); };
 auto adsIns = []( auto & d , const T & k , Key v ) { d.insert( k , v ); };
 auto adsLook = []( const auto & d , const T & k ) {
  const auto o = d.lookup( k );
  return( o ? *o : Key( 0 ) ); };
 auto adsErase = []( auto & d , const T & k ) { d.remove( k ); };
 auto adsFind = []( const auto & d , const T & k ) {
  const Key * v = d.find( k );
  return( v ? *v : Key( 0 ) ); };
 auto sum = []( const auto & d ) { return( total( d ) ); };

 for( const char * who : { "ADS::FlatHashTable" , "ADS::FlatHashTable A/A" } )
  maps( type , who , K , H , Q , r ,
        [](){ return( ADS::FlatHashTable< T , Key , FH >() ); } , adsIns ,
        adsFind , adsErase , sum );
 for( const char * who : { "ADS::DenseHashMap" , "ADS::DenseHashMap A/A" } )
  maps( type , who , K , H , Q , r ,
        [](){ return( ADS::DenseHashMap< T , Key , FH >() ); } , adsIns ,
        adsFind , adsErase , sum );
 maps( type , "ADS::FlatHashTable std::hash" , K , H , Q , r ,
       [](){ return( ADS::FlatHashTable< T , Key , std::hash< T > >() ); } ,
       adsIns , adsFind , adsErase , sum );
 maps( type , "std::unordered_map" , K , H , Q , r ,
       [](){ return( std::unordered_map< T , Key >() ); } , stdIns , stdFind ,
       stdErase , sum );
#ifdef ADS_HAS_BOOST
 maps( type , "boost::unordered_map" , K , H , Q , r ,
       [](){ return( boost::unordered_map< T , Key >() ); } , stdIns ,
       stdFind , stdErase , sum );
 maps( type , "boost::unordered_node_map" , K , H , Q , r ,
       [](){ return( boost::unordered_node_map< T , Key >() ); } , stdIns ,
       stdFind , stdErase , sum );
 maps( type , "boost::unordered_flat_map" , K , H , Q , r ,
       [](){ return( boost::unordered_flat_map< T , Key >() ); } , stdIns ,
       stdFind , stdErase , sum );
#endif
#ifdef ADS_HAS_ABSL
 maps( type , "absl::flat_hash_map" , K , H , Q , r ,
       [](){ return( absl::flat_hash_map< T , Key >() ); } , stdIns ,
       stdFind , stdErase , sum );
#endif
#ifdef ADS_HAS_ANKERL
 maps( type , "ankerl::unordered_dense::map" , K , H , Q , r ,
       [](){ return( ankerl::unordered_dense::map< T , Key >() ); } , stdIns ,
       stdFind , stdErase , sum );
#endif
#ifdef ADS_HAS_TSL
 maps( type , "tsl::robin_map" , K , H , Q , r ,
       [](){ return( tsl::robin_map< T , Key >() ); } , stdIns , stdFind ,
       stdErase , sum );
#endif
#ifdef ADS_HAS_LIBCUCKOO
 maps( type , "libcuckoo::cuckoohash_map" , K , H , Q , r ,
       [](){ return( libcuckoo::cuckoohash_map< T , Key >() ); } ,
       []( auto & d , const T & k , Key v ) { d.insert( k , v ); } ,
       []( const auto & d , const T & k ) {
        Key v = 0;
        d.find( k , v );
        return( v ); } ,
       []( auto & d , const T & k ) { d.erase( k ); } ,
       []( auto & d ) {
        Key s = 0;
        auto t = const_cast< std::remove_cvref_t< decltype( d ) > & >(
                  d ).lock_table();
        for( const auto & [ k , v ] : t ) s += v;
        return( s ); } );
#endif
 maps( type , "ADS::BucketCuckooTable" , K , H , Q , r ,
       [](){ return( ADS::BucketCuckooTable< T , Key >() ); } , adsIns ,
       adsFind , adsErase , sum );
 maps( type , "ADS::ChainedHashTable" , K , H , Q , r ,
       [ n ](){ return( ADS::ChainedHashTable< T , Key , ADS::Chain::FRONT ,
                                               FH >( n ) ); } ,
       adsIns , adsLook , adsErase , sum );
 maps( type , "ADS::OpenHashTable linear" , K , H , Q , r , [ n ](){
       return( ADS::OpenHashTable< T , Key , FH >( prime( 2 * n ) ,
                                                   ADS::Probing::linear() ) );
       } , adsIns , adsLook , adsErase , sum );
 maps( type , "ADS::CuckooHashTable" , K , H , Q , r ,
       [](){ return( ADS::CuckooHashTable< T , Key , FH >() ); } , adsIns ,
       adsLook , adsErase , sum );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- FILTERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the build of the filter by build( K ) and the queries of the hits H and
/// of the misses Q by has( f , k ); the bits per key by bits( f ), and the
/// false positives, to the standard error
template< class B , class F , class Z >
void filters( const std::string & who , const std::vector< Key > & K ,
              const std::vector< Key > & H , const std::vector< Key > & Q ,
              int r , B && build , F && has , Z && bits )
{
 const std::size_t n = K.size();
 print( "filters" , "build" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto f = build( K );
          sink = sink + bits( f ); } , r , n ) );
 auto f = build( K );
 std::size_t yes = 0;
 for( Key k : Q ) yes += has( f , k ) ? 1 : 0;
 std::cerr << "filters\t" << n << '\t' << who << "\tbits/key "
           << double( bits( f ) ) / double( n ) << "\tfalse positives "
           << double( yes ) / double( Q.size() ) << '\n';
 print( "filters" , "query hit" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::size_t s = 0;
          for( Key k : H ) s += has( f , k );
          sink = sink + s; } , r , n ) );
 print( "filters" , "query miss" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::size_t s = 0;
          for( Key k : Q ) s += has( f , k );
          sink = sink + s; } , r , n ) );
 }

/// the filter that make() builds, filled by ins( f , k ) as the build of
/// filters(), then the removals of the keys in the order H by
/// rem( f , k )
template< class M , class I , class F , class R , class Z >
void dynamic( const std::string & who , const std::vector< Key > & K ,
              const std::vector< Key > & H , const std::vector< Key > & Q ,
              int r , M && make , I && ins , F && has , R && rem ,
              Z && bits )
{
 auto fill = [ & ]( const std::vector< Key > & S ) {
  auto f = make( S.size() );
  for( Key k : S ) ins( f , k );
  return( f ); };
 filters( who , K , H , Q , r , fill , has , bits );
 print( "filters" , "remove" , K.size() , who , median(
         [ & ](){ return( fill( K ) ); } , [ & ]( auto & f ) {
          std::size_t s = 0;
          for( Key k : H ) s += rem( f , k );
          sink = sink + s; } , r , K.size() ) );
 }

#ifdef ADS_HAS_RIBBON
/// the settings of the standard Ribbon filter of RocksDB, with
/// coefficients of 64 bits and results of C bits
template< std::uint32_t C >
struct RibbonSettings {
 static constexpr bool kIsFilter = true;
 static constexpr bool kHomogeneous = false;
 static constexpr bool kFirstCoeffAlwaysOne = true;
 static constexpr bool kUseSmash = false;
 static constexpr bool kAllowZeroStarts = false;
 static constexpr std::uint32_t kFixedNumColumns = C;
 using CoeffRow = std::uint64_t;
 using Hash = std::uint64_t;
 using Key = std::uint64_t;
 using Seed = std::uint32_t;
 using Index = std::size_t;
 using ResultRow = std::uint32_t;

 static Hash HashFn( const Hash & k , Seed seed ) {
  return( ADS::hashWord( k , seed ) );
  }
 };

/// the standard Ribbon filter of RocksDB with the rows of RibbonFilter
template< std::uint32_t C >
class StandardRibbon {
 IMPORT_RIBBON_IMPL_TYPES( RibbonSettings< C > );

 /// the rows of RibbonFilter for n keys
 static std::size_t rows( std::size_t n ) {
  const double eps = std::max( 0.01 , 0.0083 * std::log2( double( n ) ) -
                                      0.025 );
  return( InterleavedSoln::RoundUpNumSlots( std::size_t( std::ceil(
           double( n ) * ( 1 + eps ) ) ) ) );
  }

 std::size_t f_m;
 std::size_t f_bytes;
 std::unique_ptr< char[] > f_data;
 InterleavedSoln f_soln;
 Hasher f_hasher;

 public:
 explicit StandardRibbon( const std::vector< Key > & S )
  : f_m( rows( S.size() ) ) , f_bytes( ( f_m * C + 7 ) / 8 ) ,
    f_data( new char[ f_bytes ] ) , f_soln( f_data.get() , f_bytes ) {
  Banding b( f_m );
  if( ! b.ResetAndFindSeedToSolve( f_m , S.begin() , S.end() ) )
   throw std::runtime_error( "StandardRibbon: no seed" );
  f_hasher.SetOrdinalSeed( b.GetOrdinalSeed() );
  f_soln.BackSubstFrom( b );
  }

 bool contain( Key k ) const { return( f_soln.FilterQuery( k , f_hasher ) ); }

 std::size_t bits( void ) const { return( 8 * f_bytes ); }
 };
#endif

/// the identity on the keys, which are random already
struct Identity {
 std::size_t operator()( Key k ) const { return( std::size_t( k ) ); }
 };

void allFilters( const std::vector< Key > & K , const std::vector< Key > & H ,
                 const std::vector< Key > & Q , int r )
{
 const std::size_t n = K.size();
 for( const char * who : { "ADS::BloomFilter" , "ADS::BloomFilter A/A" } )
  filters( who , K , H , Q , r , [ n ]( const auto & S ) {
           ADS::BloomFilter< Key , Identity > f( 10 * n , 7 );
           for( Key k : S ) f.insert( k );
           return( f ); } ,
           []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
           []( const auto & f ) { return( f.bits() ); } );
 filters( "ADS::PartitionedBloomFilter" , K , H , Q , r ,
          [ n ]( const auto & S ) {
           ADS::PartitionedBloomFilter< Key , Identity > f( 10 * n , 7 );
           for( Key k : S ) f.insert( k );
           return( f ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
 {
  // the blocked filters of Bloom of 2^b bytes, about 10 bits per key, the
  // size of the one of fastfilter, whose blocks are a power of 2
  const int b = std::max( 5 , int( std::lround( std::log2(
                                   10.0 * double( n ) / 8 ) ) ) );
  const double bits = double( std::size_t( 8 ) << b ) / double( n );
  filters( "ADS::BlockedBloomFilter" , K , H , Q , r ,
           [ & ]( const auto & S ) {
           ADS::BlockedBloomFilter< Key > f( n , bits );
           for( Key k : S ) f.insert( k );
           return( f ); } ,
           []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
           []( const auto & f ) { return( f.bits() ); } );
#ifdef ADS_HAS_FASTFILTER
  filters( "fastfilter::SimdBlockFilter" , K , H , Q , r ,
           [ b ]( const auto & S ) {
            auto f = std::make_unique< SimdBlockFilter<> >( b );
            for( Key k : S ) f->Add( k );
            return( f ); } ,
           []( const auto & f , Key k ) { return( f->Find( k ) ); } ,
           [ b ]( const auto & ) { return( std::size_t( 8 ) << b ); } );
#endif
 }
 filters( "ADS::BinaryFuseFilter 8" , K , H , Q , r , []( const auto & S ) {
          return( ADS::BinaryFuseFilter< Key , std::uint8_t >( S ) ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
 filters( "ADS::BinaryFuseFilter 16" , K , H , Q , r , []( const auto & S ) {
          return( ADS::BinaryFuseFilter< Key , std::uint16_t >( S ) ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
#ifdef ADS_HAS_XORFILTER
 {
  struct Fuse8 {
   binary_fuse8_t f;
   explicit Fuse8( std::vector< Key > S ) {
    binary_fuse8_allocate( std::uint32_t( S.size() ) , &f );
    binary_fuse8_populate( S.data() , std::uint32_t( S.size() ) , &f );
    }
   ~Fuse8() { binary_fuse8_free( &f ); }
   };
  struct Fuse16 {
   binary_fuse16_t f;
   explicit Fuse16( std::vector< Key > S ) {
    binary_fuse16_allocate( std::uint32_t( S.size() ) , &f );
    binary_fuse16_populate( S.data() , std::uint32_t( S.size() ) , &f );
    }
   ~Fuse16() { binary_fuse16_free( &f ); }
   };
  struct Xor8 {
   xor8_t f;
   explicit Xor8( std::vector< Key > S ) {
    xor8_allocate( std::uint32_t( S.size() ) , &f );
    xor8_buffered_populate( S.data() , std::uint32_t( S.size() ) , &f );
    }
   ~Xor8() { xor8_free( &f ); }
   };
  filters( "xorfilter::binary_fuse8" , K , H , Q , r ,
           []( const auto & S ) { return( std::make_unique< Fuse8 >( S ) ); } ,
           []( const auto & f , Key k ) {
            return( binary_fuse8_contain( k , &f->f ) ); } ,
           []( const auto & f ) {
            return( 8 * binary_fuse8_size_in_bytes( &f->f ) ); } );
  filters( "xorfilter::binary_fuse16" , K , H , Q , r ,
           []( const auto & S ) {
            return( std::make_unique< Fuse16 >( S ) ); } ,
           []( const auto & f , Key k ) {
            return( binary_fuse16_contain( k , &f->f ) ); } ,
           []( const auto & f ) {
            return( 8 * binary_fuse16_size_in_bytes( &f->f ) ); } );
  filters( "xorfilter::xor8" , K , H , Q , r ,
           []( const auto & S ) { return( std::make_unique< Xor8 >( S ) ); } ,
           []( const auto & f , Key k ) {
            return( xor8_contain( k , &f->f ) ); } ,
           []( const auto & f ) {
            return( 8 * xor8_size_in_bytes( &f->f ) ); } );
 }
#endif
 filters( "ADS::RibbonFilter 8" , K , H , Q , r , []( const auto & S ) {
          return( ADS::RibbonFilter< Key , std::uint8_t >( S ) ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
 filters( "ADS::RibbonFilter 16" , K , H , Q , r , []( const auto & S ) {
          return( ADS::RibbonFilter< Key , std::uint16_t >( S ) ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
#ifdef ADS_HAS_RIBBON
 filters( "fastfilter::StandardRibbon 8" , K , H , Q , r ,
          []( const auto & S ) {
           return( std::make_unique< StandardRibbon< 8 > >( S ) ); } ,
          []( const auto & f , Key k ) { return( f->contain( k ) ); } ,
          []( const auto & f ) { return( f->bits() ); } );
 filters( "fastfilter::StandardRibbon 16" , K , H , Q , r ,
          []( const auto & S ) {
           return( std::make_unique< StandardRibbon< 16 > >( S ) ); } ,
          []( const auto & f , Key k ) { return( f->contain( k ) ); } ,
          []( const auto & f ) { return( f->bits() ); } );
#endif
 dynamic( "ADS::CuckooFilter 8" , K , H , Q , r , []( std::size_t m ) {
          return( ADS::CuckooFilter< Key , std::uint8_t >( m ) ); } ,
          []( auto & f , Key k ) { f.insert( k ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( auto & f , Key k ) { return( f.remove( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
 dynamic( "ADS::CuckooFilter 16" , K , H , Q , r , []( std::size_t m ) {
          return( ADS::CuckooFilter< Key , std::uint16_t >( m ) ); } ,
          []( auto & f , Key k ) { f.insert( k ); } ,
          []( const auto & f , Key k ) { return( f.maybe( k ) ); } ,
          []( auto & f , Key k ) { return( f.remove( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
#ifdef ADS_HAS_CUCKOOFILTER
 dynamic( "fastfilter::CuckooFilter 8" , K , H , Q , r ,
          []( std::size_t m ) {
           return( std::make_unique< cuckoofilter::CuckooFilter< Key , 8 > >(
                    m ) ); } ,
          []( auto & f , Key k ) { f->Add( k ); } ,
          []( const auto & f , Key k ) { return( f->Contain( k ) == 0 ); } ,
          []( auto & f , Key k ) { return( f->Delete( k ) == 0 ); } ,
          []( const auto & f ) { return( 8 * f->SizeInBytes() ); } );
 dynamic( "fastfilter::CuckooFilter 16" , K , H , Q , r ,
          []( std::size_t m ) {
           return( std::make_unique< cuckoofilter::CuckooFilter< Key , 16 > >(
                    m ) ); } ,
          []( auto & f , Key k ) { f->Add( k ); } ,
          []( const auto & f , Key k ) { return( f->Contain( k ) == 0 ); } ,
          []( auto & f , Key k ) { return( f->Delete( k ) == 0 ); } ,
          []( const auto & f ) { return( 8 * f->SizeInBytes() ); } );
#endif
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- PERFECT HASHING -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the build of the function by build( K ) and the n queries of H by
/// at( f , k ); the bits per key by bits( f ), to the standard error
template< class B , class F , class Z >
void perfect( const std::string & who , const std::vector< Key > & K ,
              const std::vector< Key > & H , int r , B && build , F && at ,
              Z && bits )
{
 const std::size_t n = K.size();
 print( "perfect hashing" , "build" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto f = build( K );
          sink = sink + bits( f ); } , std::min( r , 5 ) , n ) );
 auto f = build( K );
 std::vector< char > seen( n , 0 );
 bool ok = true;
 for( Key k : K ) {
  const std::size_t i = at( f , k );
  ok = ok && ( i < n ) && ! seen[ i ];
  if( i < n ) seen[ i ] = 1;
  }
 std::cerr << "perfect hashing\t" << n << '\t' << who << "\tbits/key "
           << double( bits( f ) ) / double( n )
           << ( ok ? "\tminimal perfect" : "\tNOT MINIMAL PERFECT" ) << '\n';
 print( "perfect hashing" , "query" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::size_t s = 0;
          for( Key k : H ) s += at( f , k );
          sink = sink + s; } , r , n ) );
 }

void perfectHashing( const std::vector< Key > & K ,
                     const std::vector< Key > & H , int r )
{
 for( const char * who : { "ADS::OrderedMinimalPerfectHash r=3" ,
                           "ADS::OrderedMinimalPerfectHash r=3 A/A" } )
  perfect( who , K , H , r , []( const auto & S ) {
           return( ADS::OrderedMinimalPerfectHash< Key >( S , 3 ) ); } ,
           []( const auto & f , Key k ) { return( f( k ) ); } ,
           []( const auto & f ) { return( 32 * f.vertices() ); } );
 perfect( "ADS::MinimalPerfectHash" , K , H , r , []( const auto & S ) {
          return( ADS::MinimalPerfectHash< Key >( S , 5 , 0.99 ) ); } ,
          []( const auto & f , Key k ) { return( f( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
 perfect( "ADS::RecSplit l=8 b=100" , K , H , r , []( const auto & S ) {
          return( ADS::RecSplit< Key , 8 >( S , 100 ) ); } ,
          []( const auto & f , Key k ) { return( f( k ) ); } ,
          []( const auto & f ) { return( f.bits() ); } );
#ifdef ADS_HAS_PTHASH
 {
  using C = pthash::build_configuration;
  auto config = []( double lambda , double alpha , std::size_t part ) {
   C c;
   c.lambda = lambda;
   c.alpha = alpha;
   c.minimal = true;
   c.verbose = false;
   c.seed = 1;
   c.num_threads = 1;
   if( part ) {
    c.avg_partition_size = part;
    c.dense_partitioning = true;
    }
   return( c ); };
  using PT = pthash::single_phf< pthash::xxhash_64 , pthash::skew_bucketer ,
                                 pthash::compact , true >;
  perfect( "PTHash C" , K , H , r , [ & ]( const auto & S ) {
           auto f = std::make_unique< PT >();
           f->build_in_internal_memory( S.begin() , S.size() ,
                                        config( 5 , 0.99 , 0 ) );
           return( f ); } ,
           []( const auto & f , Key k ) { return( ( *f )( k ) ); } ,
           []( const auto & f ) { return( f->num_bits() ); } );
  using PH = pthash::dense_partitioned_phf< pthash::xxhash_128 ,
                                            pthash::opt_bucketer ,
                                            pthash::R_int , true >;
  perfect( "PHOBIC R-int" , K , H , r , [ & ]( const auto & S ) {
           auto f = std::make_unique< PH >();
           f->build_in_internal_memory( S.begin() , S.size() ,
                                        config( 7 , 1 , 3000 ) );
           return( f ); } ,
           []( const auto & f , Key k ) { return( ( *f )( k ) ); } ,
           []( const auto & f ) { return( f->num_bits() ); } );
 }
#endif
#ifdef ADS_HAS_BBHASH
 {
  using BB = boomphf::mphf< Key , boomphf::SingleHashFunctor< Key > >;
  perfect( "BBHash g=2" , K , H , r , []( const auto & S ) {
           return( std::make_unique< BB >( S.size() , S , 1 , 2.0 , false ,
                                           false ) ); } ,
           []( const auto & f , Key k ) {
            return( std::size_t( f->lookup( k ) ) ); } ,
           []( const auto & f ) { return( f->totalBitSize() ); } );
 }
#endif
#ifdef ADS_HAS_RECSPLIT
 {
  using RS = sux::function::RecSplit< 8 >;
  perfect( "RecSplit l=8 b=100" , K , H , r , []( const auto & S ) {
           std::vector< sux::function::hash128_t > h;
           for( Key k : S )
            h.push_back( sux::function::hash128_t( ADS::splitmix64( k ) ,
                         ADS::splitmix64( k ^ 0x9E3779B97F4A7C15ULL ) ) );
           return( std::make_unique< RS >( std::span( h ) , 100 ) ); } ,
           []( const auto & f , Key k ) {
            return( ( *f )( sux::function::hash128_t( ADS::splitmix64( k ) ,
                     ADS::splitmix64( k ^ 0x9E3779B97F4A7C15ULL ) ) ) ); } ,
           []( const auto & f ) { return( f->bitCount() ); } );
 }
#endif
 }


/// true if the part is to run
bool part( const char * name )
{
 const char * p = std::getenv( "PARTS" );
 return( ( p == nullptr ) || std::strstr( p , name ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  const auto all = keys( 2 * n , o.seed );
  const std::vector< Key > K( all.begin() , all.begin() + long( n ) ) ,
                           Q( all.begin() + long( n ) , all.end() );
  std::vector< Key > H = K;                 // the hits in random order
  std::shuffle( H.begin() , H.end() , std::mt19937_64( o.seed + 1 ) );
  if( part( "hash" ) ) hashFunctions( n , K , r , o.seed );
  if( part( "maps" ) ) hashMaps( "u64" , K , H , Q , r );
  if( part( "maps" ) ) {
   const auto W = words( 2 * n , o.seed );
   const std::vector< std::string > S( W.begin() , W.begin() + long( n ) ) ,
                                    T( W.begin() + long( n ) , W.end() );
   std::vector< std::string > G = S;
   std::shuffle( G.begin() , G.end() , std::mt19937_64( o.seed + 1 ) );
   hashMaps( "str" , S , G , T , r );
  }
  if( part( "filters" ) ) allFilters( K , H , Q , r );
  if( part( "perfect" ) ) perfectHashing( K , H , r );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File hashing.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
