/*--------------------------------------------------------------------------*/
/*---------------------------- File hashing.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered hashing, beyond the book: the hash functions of
 * FastHash.h, the blocked filter of Bloom, the binary fuse filter, the
 * cuckoo filter, the prefix filter, the Ribbon filter, the minimal perfect
 * hash functions by pilots and by recursive splitting and the cuckoo table
 * with buckets of 4 places. Each one is checked against a reference of
 * the standard library or against its own analysis, on random and
 * adversarial inputs, at the level 3 of the contracts.
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

#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FingerprintPerfectHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/MinimalPerfectHash.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash that sends every key to the same group, with the same
/// fingerprint, and declares it needs no mixing
struct OneGroup {
 using is_avalanching = void;
 std::size_t operator()( std::uint64_t ) const { return( 0x105 ); }
 };

/// n distinct random words from the seed
std::vector< std::uint64_t > randomKeys( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::uint64_t > seen;
 std::vector< std::uint64_t > K;
 while( K.size() < n ) {
  const std::uint64_t k = g();
  if( seen.insert( k ).second )
   K.push_back( k );
  }
 return( K );
 }

/// n distinct strings of 1 to 40 letters from the seed
std::vector< std::string > randomStrings( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::string > seen;
 std::vector< std::string > S;
 while( S.size() < n ) {
  std::string s( 1 + g() % 40 , ' ' );
  for( auto & c : s )
   c = char( 'a' + g() % 26 );
  if( seen.insert( s ).second )
   S.push_back( std::move( s ) );
  }
 return( S );
 }

/// the answers of the prefix filter of 10^6 random keys to 10^6 absent
/// ones, folded, the same on every machine
constexpr std::uint64_t kPrefixAnswers = 6080367913530849277ULL;

/// the answers of the Ribbon filters of 8, 16 and 32 bits of 10^5 random
/// keys to 10^5 absent ones, folded, the same on every machine
constexpr std::uint64_t kRibbonAnswers = 914496213114749030ULL;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the product on 128 bits against its portable form; the known hashes of
// strings of every path (0 to 3, 4 to 8, 9 to 16, 17 to 32, 33 to 64, 65
// to 256, the stripes past 256 with and without whole blocks of 1 KB),
// which a machine with or without AVX2 and AVX-512 must give alike; the
// seed changes every hash; the prefixes of a string of 5000 bytes, the
// strings of 1 to 3 equal bytes and of zeros of every length up to 300,
// and 10^6 consecutive words, all with distinct hashes; a flip of any bit
// of a key of each path flips about half of the bits of its hash; the
// hash of the integers, of the floating point numbers (+0 as -0) and of
// the strings by FastHash, a nil pointer of a non empty string
ADS_TEST( "fast-hash" )
{
 for( std::uint64_t a : { 0ULL , 1ULL , 0xFFFFFFFFULL , 1ULL << 63 , ~0ULL ,
                          0x9E3779B97F4A7C15ULL } )
  for( std::uint64_t b : { 0ULL , 3ULL , 0xFFFFFFFFULL , ~0ULL ,
                           0x123456789ABCDEF0ULL } ) {
   const Product128 q = product128( a , b );
   CHECK( q.lo == a * b && foldedProduct( a , b ) == ( q.lo ^ q.hi ) );
#if defined( __SIZEOF_INT128__ )
   const unsigned __int128 p = static_cast< unsigned __int128 >( a ) * b;
   CHECK( q.hi == std::uint64_t( p >> 64 ) );
#endif
   }
 static_assert( product128( ~0ULL , ~0ULL ).hi == ~0ULL - 1 );
 static_assert( product128( 1ULL << 63 , 4 ).hi == 2 );
 std::vector< unsigned char > buf( 5000 );
 {
  std::uint64_t x = 1;
  for( auto & c : buf ) {
   x = x * 6364136223846793005ULL + 1442695040888963407ULL;
   c = static_cast< unsigned char >( x >> 56 );
   }
  }
 // the known answers, from a build with no vector instructions
 const std::size_t lens[] = { 0 , 1 , 3 , 4 , 8 , 9 , 16 , 17 , 32 , 33 ,
                              64 , 65 , 96 , 97 , 256 , 257 , 1024 , 1025 ,
                              4096 , 4999 };
 const std::uint64_t known[] = {
  0xBC13060E2D1AAC79ULL , 0x5D16303249F2E44CULL , 0x944CD8618E651A8FULL ,
  0x96969D554F526124ULL , 0x112A8DA1E4CBC450ULL , 0xE9379B9163EE3F38ULL ,
  0xCE5EC7274574FD66ULL , 0x1E9C7E2CEF42118EULL , 0xA58DBB58B77161EFULL ,
  0x57C6910028675731ULL , 0x9BF13D56F64BA0F7ULL , 0x67B962D0635E1889ULL ,
  0xFE3EDA134B696614ULL , 0x1D8784A207CF8E9EULL , 0x0B9DC74F0DEF81A9ULL ,
  0x97D5950D50E76F48ULL , 0xF762805411CD9B93ULL , 0x576FF01834410863ULL ,
  0xAD70599F8618F739ULL , 0xF347ADC6514B2133ULL };
 for( std::size_t i = 0 ; i < std::size( lens ) ; ++i ) {
  CHECK( hashBytes( buf.data() , lens[ i ] ) == known[ i ] );
  CHECK( hashBytes( buf.data() , lens[ i ] , 1 ) != known[ i ] );
  }
 {
  std::unordered_set< std::uint64_t > H;
  for( std::size_t n = 0 ; n <= buf.size() ; ++n )
   CHECK( H.insert( hashBytes( buf.data() , n ) ).second );
  const std::vector< unsigned char > zeros( 300 , 0 );
  for( std::size_t n = 1 ; n <= 300 ; ++n )
   CHECK( H.insert( hashBytes( zeros.data() , n ) ).second );
  for( int c = 1 ; c < 256 ; ++c )
   for( std::size_t n = 1 ; n <= 3 ; ++n ) {
    const std::vector< unsigned char > s( n , static_cast< unsigned char >(
                                                c ) );
    if( ! std::equal( s.begin() , s.end() , buf.begin() ) )   // a prefix
     CHECK( H.insert( hashBytes( s.data() , n ) ).second );
    }
  std::unordered_set< std::uint64_t > W;
  for( std::uint64_t k = 0 ; k < 1000000 ; ++k )
   CHECK( W.insert( hashWord( k ) ).second );
  }
 {                                             // the avalanche
  std::mt19937_64 g( 7 );
  for( std::size_t n : { 1 , 3 , 4 , 7 , 8 , 12 , 16 , 24 , 32 , 48 , 64 ,
                         80 , 128 , 200 , 256 , 300 , 1100 } ) {
   double flips = 0 , trials = 0;
   for( int t = 0 ; t < 40 ; ++t ) {
    std::vector< unsigned char > s( n );
    for( auto & c : s )
     c = static_cast< unsigned char >( g() );
    const std::uint64_t h = hashBytes( s.data() , n );
    for( std::size_t b = 0 ; b < 8 * n ; b += ( n > 64 ? 7 : 1 ) ) {
     s[ b / 8 ] ^= static_cast< unsigned char >( 1 << ( b % 8 ) );
     flips += std::popcount( h ^ hashBytes( s.data() , n ) );
     trials += 64;
     s[ b / 8 ] ^= static_cast< unsigned char >( 1 << ( b % 8 ) );
     }
    }
   CHECK( flips / trials > 0.45 && flips / trials < 0.55 );
   }
  double flips = 0;
  for( int t = 0 ; t < 1000 ; ++t ) {
   const std::uint64_t x = g();
   for( int b = 0 ; b < 64 ; ++b )
    flips += std::popcount( hashWord( x ) ^
                            hashWord( x ^ ( std::uint64_t( 1 ) << b ) ) );
   }
  CHECK( flips / ( 1000 * 64 * 64 ) > 0.45 &&
         flips / ( 1000 * 64 * 64 ) < 0.55 );
  }
 CHECK( FastHash< double >()( 0.0 ) == FastHash< double >()( -0.0 ) );
 CHECK( FastHash< float >()( 1.5f ) != FastHash< float >()( -1.5f ) );
 CHECK( FastHash< int >()( -1 ) == hashWord( std::uint64_t( -1 ) ) );
 const char * c = "hashing";
 CHECK( FastHash< std::string >()( std::string( c ) ) ==
        FastHash< std::string_view >()( c ) );
 CHECK( FastHash< const char * >()( c ) == hashBytes( c , 7 ) );
 CHECK( FastHash< std::string >{ 5 }( "x" ) != FastHash< std::string >()(
                                                 "x" ) );
 CHECK( hashBytes( nullptr , 0 ) == hashBytes( c , 0 ) );
 CHECK_THROWS( (void) hashBytes( nullptr , 1 ) , contract_violation );
 }

// the blocked filter: no false negative on 10^5 random words, on 10^5
// consecutive words by the identity of std::hash (mixed by the filter) and
// on strings; the false positives on 10^5 absent words near the estimate
// of falsePositiveRate(), about 1.26 % at 10 bits per item; the union of two
// filters, clear(), a filter of no item (one block, nothing maybe there),
// a filter of one block filled by 2000 items (false positives near 1),
// and the bits per item that are not positive
ADS_TEST( "blocked-bloom" )
{
 const auto K = randomKeys( 200000 , 3 );
 const std::vector< std::uint64_t > In( K.begin() , K.begin() + 100000 ) ,
                                    Out( K.begin() + 100000 , K.end() );
 BlockedBloomFilter< std::uint64_t > F( In.size() , 10 );
 CHECK( F.empty() && F.falsePositiveRate() == 0 && ! F.maybe( In[ 0 ] ) );
 for( std::uint64_t k : In )
  F.insert( k );
 CHECK( F.size() == In.size() && ! F.empty() );
 CHECK( F.bits() == 256 * F.blocks() && F.bits() >= 10 * In.size() );
 for( std::uint64_t k : In )
  CHECK( F.maybe( k ) );
 std::size_t fp = 0;
 for( std::uint64_t k : Out )
  fp += F.maybe( k );
 const double rate = double( fp ) / double( Out.size() ) ,
              est = F.falsePositiveRate();
 CHECK( est > 0.011 && est < 0.014 );
 CHECK( rate > 0.8 * est && rate < 1.25 * est );
 {                                             // consecutive, std::hash
  BlockedBloomFilter< std::uint64_t , std::hash< std::uint64_t > > G(
   100000 );
  for( std::uint64_t k = 0 ; k < 100000 ; ++k )
   G.insert( k );
  std::size_t f = 0;
  for( std::uint64_t k = 0 ; k < 100000 ; ++k )
   CHECK( G.maybe( k ) );
  for( std::uint64_t k = 100000 ; k < 200000 ; ++k )
   f += G.maybe( k );
  CHECK( double( f ) / 100000 < 1.25 * G.falsePositiveRate() );
  }
 {                                             // strings, the union
  const auto S = randomStrings( 20000 , 4 );
  BlockedBloomFilter< std::string > A( S.size() ) , B( S.size() );
  for( std::size_t i = 0 ; i < S.size() ; ++i )
   ( i % 2 ? A : B ).insert( S[ i ] );
  A.unite( B );
  CHECK( A.size() == S.size() );
  for( const auto & s : S )
   CHECK( A.maybe( s ) );
  A.clear();
  CHECK( A.empty() && ! A.maybe( S[ 0 ] ) );
  BlockedBloomFilter< std::string > C( 1 );
  CHECK_THROWS( A.unite( C ) , contract_violation );
  }
 BlockedBloomFilter< int > Z( 0 );             // one block
 CHECK( Z.blocks() == 1 && ! Z.maybe( 3 ) );
 for( int i = 0 ; i < 2000 ; ++i )
  Z.insert( i );
 CHECK( Z.falsePositiveRate() > 0.99 && Z.maybe( -5 ) );
 CHECK_THROWS( BlockedBloomFilter< int >( 10 , 0 ) , contract_violation );
 CHECK_THROWS( BlockedBloomFilter< int >( 10 , -1 ) , contract_violation );
 }

// the binary fuse filter of 8 and 16 bits: every key maybe there for 0, 1,
// 2, 3, 10, 100, 1000 and 10^5 random keys, 10^5 consecutive keys and
// strings; the false positives near 2^-8 and 2^-16 on 10^6 absent keys;
// about 9.1 and 18.1 bits per key at 10^6 keys; repeated keys counted once;
// no key of an empty filter
ADS_TEST( "binary-fuse" )
{
 for( std::size_t n : { 0 , 1 , 2 , 3 , 10 , 100 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( n + 1 ) );
  BinaryFuseFilter< std::uint64_t > F( K );
  BinaryFuseFilter< std::uint64_t , std::uint16_t > G( K , 9 );
  CHECK( F.size() == n && G.size() == n && F.empty() == ( n == 0 ) );
  CHECK( F.trials() >= 1 && F.trials() < 10 );
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) && G.maybe( k ) );
  if( n == 0 )
   CHECK( ! F.maybe( 0 ) && ! G.maybe( 1 ) && F.bits() == 0 );
  }
 {
  const auto K = randomKeys( 2000000 , 11 );
  const std::vector< std::uint64_t > In( K.begin() , K.begin() + 1000000 );
  BinaryFuseFilter< std::uint64_t > F( In );
  BinaryFuseFilter< std::uint64_t , std::uint16_t > G( In );
  std::size_t f = 0 , g = 0;
  for( std::size_t i = 1000000 ; i < K.size() ; ++i ) {
   f += F.maybe( K[ i ] );
   g += G.maybe( K[ i ] );
   }
  CHECK( f > 3200 && f < 4600 );               // 10^6 / 256 = 3906
  CHECK( g < 40 );                             // 10^6 / 65536 = 15
  CHECK( double( F.bits() ) / 1e6 < 9.2 && double( G.bits() ) / 1e6 < 18.4 );
  }
 {                                             // consecutive
  std::vector< std::uint64_t > K( 100000 );
  for( std::size_t i = 0 ; i < K.size() ; ++i )
   K[ i ] = i;
  BinaryFuseFilter< std::uint64_t > F( K );
  std::size_t f = 0;
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) );
  for( std::uint64_t k = 100000 ; k < 1100000 ; ++k )
   f += F.maybe( k );
  CHECK( f > 3200 && f < 4600 );
  }
 {                                             // repeated, strings
  auto K = randomKeys( 5000 , 13 );
  K.insert( K.end() , K.begin() , K.begin() + 2500 );
  K.push_back( K[ 0 ] );
  BinaryFuseFilter< std::uint64_t > F( K );
  CHECK( F.size() == 5000 && F.trials() >= 2 );
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) );
  const auto S = randomStrings( 30000 , 14 );
  BinaryFuseFilter< std::string > T( S );
  for( const auto & s : S )
   CHECK( T.maybe( s ) );
  }
 }

// the cuckoo filter of 8 and 16 bits: 0, 1, 2, 3, 10, 100, 1000 and 10^5
// random keys inserted, every one maybe there, all removed and the filter
// empty; at 10^6 keys the false positives on 10^6 absent keys near
// 8 alpha / 255 and 8 alpha / 65535 at the load alpha; filled until an
// insertion fails, at a load above 0.9, with every key in still there,
// the failed insertion changing nothing and a removal making room; a key
// inserted 8 times and removed 8 times; consecutive keys by std::hash,
// keys equal in their low 32 bits, strings; the removal of a key absent
// from an empty filter, and a filter for no key (one bucket)
ADS_TEST( "cuckoo-filter" )
{
 for( std::size_t n : { 0 , 1 , 2 , 3 , 10 , 100 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( n + 21 ) );
  CuckooFilter< std::uint64_t > F( n );
  CuckooFilter< std::uint64_t , std::uint16_t > G( n );
  CHECK( F.empty() && F.capacity() >= n && F.capacity() * 94 >= n * 100 );
  CHECK( F.bits() == 8 * F.capacity() && G.bits() == 16 * G.capacity() );
  for( std::uint64_t k : K )
   CHECK( F.insert( k ) && G.insert( k ) );
  CHECK( F.size() == n && G.size() == n && F.empty() == ( n == 0 ) );
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) && G.maybe( k ) );
  for( std::uint64_t k : K )
   CHECK( F.remove( k ) && G.remove( k ) );
  CHECK( F.empty() && G.empty() && F.load() == 0 );
  if( n > 0 )
   CHECK( ! F.maybe( K[ 0 ] ) && ! G.maybe( K[ 0 ] ) );
  }
 {
  const auto K = randomKeys( 2000000 , 31 );
  CuckooFilter< std::uint64_t > F( 1000000 );
  CuckooFilter< std::uint64_t , std::uint16_t > G( 1000000 );
  for( std::size_t i = 0 ; i < 1000000 ; ++i )
   CHECK( F.insert( K[ i ] ) && G.insert( K[ i ] ) );
  std::size_t f = 0 , g = 0;
  for( std::size_t i = 1000000 ; i < K.size() ; ++i ) {
   f += F.maybe( K[ i ] );
   g += G.maybe( K[ i ] );
   }
  const double a = F.load() , ef = 1e6 * 8 * a / 255 ,
               eg = 1e6 * 8 * a / 65535;
  CHECK( a > 0.47 && a <= 0.94 );
  CHECK( double( f ) > 0.85 * ef && double( f ) < 1.15 * ef );
  CHECK( double( g ) < 2 * eg + 10 );
  for( std::size_t i = 0 ; i < 1000000 ; ++i )
   CHECK( F.maybe( K[ i ] ) && G.maybe( K[ i ] ) );
  }
 for( int b : { 8 , 16 } ) {                   // filled until full
  const auto K = randomKeys( 1 << 18 , unsigned( 41 + b ) );
  auto fill = [ & ]( auto & F ) {
   std::size_t i = 0;
   while( i < K.size() && F.insert( K[ i ] ) )
    ++i;
   CHECK( i < K.size() && F.size() == i && F.load() > 0.9 );
   for( std::size_t j = 0 ; j < i ; ++j )
    CHECK( F.maybe( K[ j ] ) );
   CHECK( ! F.insert( K[ i + 1 ] ) && F.size() == i );
   CHECK( F.remove( K[ 0 ] ) && F.insert( K[ i ] ) && F.maybe( K[ i ] ) );
   for( std::size_t j = 1 ; j <= i ; ++j )
    CHECK( F.maybe( K[ j ] ) );
   F.clear();
   CHECK( F.empty() && ! F.maybe( K[ 1 ] ) && F.insert( K[ 1 ] ) );
   };
  if( b == 8 ) {
   CuckooFilter< std::uint64_t > F( 200000 );
   fill( F );
   }
  else {
   CuckooFilter< std::uint64_t , std::uint16_t > F( 200000 );
   fill( F );
   }
  }
 {                                             // repeated, adversarial
  CuckooFilter< std::uint64_t > F( 1000 );
  for( int i = 0 ; i < 8 ; ++i )
   CHECK( F.insert( 77 ) );
  CHECK( F.size() == 8 && F.maybe( 77 ) );
  for( int i = 0 ; i < 8 ; ++i )
   CHECK( F.remove( 77 ) );
  CHECK( F.empty() && ! F.maybe( 77 ) && ! F.remove( 77 ) );
  CuckooFilter< std::uint64_t , std::uint8_t ,
                std::hash< std::uint64_t > > C( 100000 );
  CuckooFilter< std::uint64_t , std::uint16_t > L( 100000 );
  for( std::uint64_t k = 0 ; k < 100000 ; ++k )
   CHECK( C.insert( k ) && L.insert( k << 32 ) );
  std::size_t f = 0;
  for( std::uint64_t k = 0 ; k < 100000 ; ++k ) {
   CHECK( C.maybe( k ) && L.maybe( k << 32 ) );
   f += C.maybe( k + 100000 );
   }
  CHECK( double( f ) < 1.15 * 1e5 * 8 * C.load() / 255 );
  const auto S = randomStrings( 30000 , 43 );
  CuckooFilter< std::string > T( S.size() );
  for( const auto & s : S )
   CHECK( T.insert( s ) );
  for( const auto & s : S )
   CHECK( T.maybe( s ) );
  }
 CuckooFilter< int > Z( 0 );                   // one bucket
 CHECK( Z.capacity() == 4 && ! Z.maybe( 3 ) && ! Z.remove( 3 ) );
 for( int i = 0 ; i < 4 ; ++i )
  CHECK( Z.insert( i ) );
 CHECK( Z.insert( 4 ) && Z.size() == 5 && ! Z.insert( 5 ) );  // the victim
 for( int i = 0 ; i < 5 ; ++i )
  CHECK( Z.maybe( i ) );
 CHECK( Z.remove( 4 ) && Z.insert( 5 ) && Z.maybe( 5 ) );
 }

/// the hash of a key whose high 32 bits are 0: every key in the bin 0
struct OneBin {
 using is_avalanching = void;

 std::size_t operator()( std::uint64_t k ) const {
  return( std::size_t( hashWord( k ) & 0xFFFFFFFFU ) );
  }
 };

// the prefix filter: for 0, 1, 2, 24, 25, 26, 100, 1000 and 10^5 random
// keys every key in, an insertion past the capacity failing with nothing
// changed, the clear; at 10^6 keys the fingerprints in the spare near
// their expectation, about 5.9 % of the keys, the bits per key near 11.6,
// the false positives on 10^6 absent keys near the analysis and below the
// bound 0.95 / 256 + eps' / sqrt( 50 pi ) of the paper, and their answers
// equal to known ones, which the paths of AVX-512, AVX2 and of any
// machine must give alike; consecutive keys by std::hash, keys equal in
// their low 32 bits and strings, against the analysis too; a key
// inserted 100 times, the copies past 25 in the spare; all the keys in one
// bin by a hash, the keys past 25 in the spare and every one found; a
// filter for no key
ADS_TEST( "prefix-filter" )
{
 for( std::size_t n : { 0 , 1 , 2 , 24 , 25 , 26 , 100 , 1000 , 100000 } ) {
  const auto K = randomKeys( n + 1 , unsigned( n + 51 ) );
  PrefixFilter< std::uint64_t > F( n );
  CHECK( F.empty() && F.capacity() == n && F.size() == 0 );
  CHECK( F.bins() * 25 * 95 >= n * 100 && F.bins() >= 1 );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( F.insert( K[ i ] ) );
  CHECK( F.size() == n && F.empty() == ( n == 0 ) && F.load() <= 0.96 );
  CHECK( ! F.insert( K[ n ] ) && F.size() == n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( F.maybe( K[ i ] ) );
  F.clear();
  CHECK( F.empty() && F.spareSize() == 0 && F.load() == 0 );
  if( n > 0 )
   CHECK( ! F.maybe( K[ 0 ] ) && F.insert( K[ 0 ] ) && F.maybe( K[ 0 ] ) );
  }
 {
  const std::size_t n = 1000000;
  const auto K = randomKeys( 2 * n , 61 );
  PrefixFilter< std::uint64_t > F( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( F.insert( K[ i ] ) );
  CHECK( ! F.insert( K[ n ] ) );
  const double es = F.expectedSpareSize();
  CHECK( es > 0.055 * double( n ) && es < 0.062 * double( n ) );
  CHECK( double( F.spareSize() ) > 0.97 * es &&
         double( F.spareSize() ) < 1.03 * es );
  const double bpk = double( F.bits() ) / double( n );
  CHECK( bpk > 11.3 && bpk < 11.8 );
  std::size_t f = 0;
  std::uint64_t sum = 0;
  for( std::size_t i = n ; i < K.size() ; ++i ) {
   const bool y = F.maybe( K[ i ] );
   f += y;
   sum = sum * 31 + ( y ? i : 0 );
   }
  const double e = F.falsePositiveRate();
  CHECK( double( f ) > 0.9 * e * double( n ) &&
         double( f ) < 1.1 * e * double( n ) );
  CHECK( e < 0.95 / 256 + 0.01 / std::sqrt( 50 * 3.14159265358979 ) );
  CHECK( sum == kPrefixAnswers );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( F.maybe( K[ i ] ) );
  }
 {                                             // adversarial
  const std::size_t n = 200000;
  PrefixFilter< std::uint64_t , std::hash< std::uint64_t > > C( n );
  PrefixFilter< std::uint64_t > L( n );
  for( std::uint64_t k = 0 ; k < n ; ++k )
   CHECK( C.insert( k ) && L.insert( k << 32 ) );
  std::size_t f = 0 , g = 0;
  for( std::uint64_t k = 0 ; k < n ; ++k ) {
   CHECK( C.maybe( k ) && L.maybe( k << 32 ) );
   f += C.maybe( k + n );
   g += L.maybe( ( k + n ) << 32 );
   }
  CHECK( double( f ) < 1.15 * C.falsePositiveRate() * double( n ) );
  CHECK( double( g ) < 1.15 * L.falsePositiveRate() * double( n ) );
  const auto S = randomStrings( 30000 , 63 );
  PrefixFilter< std::string > T( S.size() );
  for( const auto & s : S )
   CHECK( T.insert( s ) );
  for( const auto & s : S )
   CHECK( T.maybe( s ) );
  }
 {                                             // duplicates
  PrefixFilter< std::uint64_t > D( 1000 );
  for( int i = 0 ; i < 100 ; ++i )
   CHECK( D.insert( 77 ) );
  CHECK( D.size() == 100 && D.spareSize() == 75 && D.maybe( 77 ) );
  }
 {                                             // one bin
  const std::size_t n = 5000;
  const auto K = randomKeys( 2 * n , 67 );
  PrefixFilter< std::uint64_t , OneBin > B( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( B.insert( K[ i ] ) );
  CHECK( B.spareSize() == n - 25 && B.size() == n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( B.maybe( K[ i ] ) );
  }
 PrefixFilter< int > Z( 0 );                   // no key: one bin
 CHECK( Z.bins() == 1 && Z.capacity() == 0 && ! Z.maybe( 3 ) );
 CHECK( ! Z.insert( 3 ) && Z.empty() && ! Z.maybe( 3 ) );
 }

// the Ribbon filter of 8 and 16 bits: every key maybe there for 0, 1, 2,
// 3, 10, 63, 64, 65, 100, 1000 and 10^5 random keys, 10^5 consecutive
// keys, keys equal in their low 32 bits and strings; the false positives
// near 2^-8 and 2^-16 on 10^6 absent keys; about 9.1 and 18.3 bits per key
// at 10^6 keys; results of 32 bits; the answers of the three to absent
// keys the same on every machine, whichever parities and back
// substitution the processor takes; repeated keys counted with their
// repetitions; no key of an empty filter
ADS_TEST( "ribbon-filter" )
{
 for( std::size_t n : { 0 , 1 , 2 , 3 , 10 , 63 , 64 , 65 , 100 , 1000 ,
                        100000 } ) {
  const auto K = randomKeys( n , unsigned( n + 51 ) );
  RibbonFilter< std::uint64_t > F( K );
  RibbonFilter< std::uint64_t , std::uint16_t > G( K , 9 );
  CHECK( F.size() == n && G.size() == n && F.empty() == ( n == 0 ) );
  CHECK( F.trials() < 10 && G.trials() < 10 );
  CHECK( F.rows() % 64 == 0 && F.rows() >= n );
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) && G.maybe( k ) );
  if( n == 0 )
   CHECK( ! F.maybe( 0 ) && ! G.maybe( 1 ) && F.bits() == 0 );
  }
 {
  const auto K = randomKeys( 2000000 , 61 );
  const std::vector< std::uint64_t > In( K.begin() , K.begin() + 1000000 );
  RibbonFilter< std::uint64_t > F( In );
  RibbonFilter< std::uint64_t , std::uint16_t > G( In );
  CHECK( F.trials() < 10 && G.trials() < 10 );
  std::size_t f = 0 , g = 0;
  for( std::size_t i = 1000000 ; i < K.size() ; ++i ) {
   f += F.maybe( K[ i ] );
   g += G.maybe( K[ i ] );
   }
  CHECK( f > 3200 && f < 4600 );               // 10^6 / 256 = 3906
  CHECK( g < 40 );                             // 10^6 / 65536 = 15
  CHECK( double( F.bits() ) / 1e6 < 9.2 && double( G.bits() ) / 1e6 < 18.4 );
  for( std::uint64_t k : In )
   CHECK( F.maybe( k ) && G.maybe( k ) );
  }
 {                                             // consecutive, low bits
  std::vector< std::uint64_t > K( 100000 ) , L( 100000 );
  for( std::size_t i = 0 ; i < K.size() ; ++i ) {
   K[ i ] = i;
   L[ i ] = std::uint64_t( i ) << 32;
   }
  RibbonFilter< std::uint64_t > F( K );
  RibbonFilter< std::uint64_t , std::uint16_t > G( L );
  std::size_t f = 0;
  for( std::size_t i = 0 ; i < K.size() ; ++i )
   CHECK( F.maybe( K[ i ] ) && G.maybe( L[ i ] ) );
  for( std::uint64_t k = 100000 ; k < 1100000 ; ++k )
   f += F.maybe( k );
  CHECK( f > 3200 && f < 4600 );
  }
 {                                   // 32 bits, every machine the same
  const auto K = randomKeys( 200000 , 65 );
  const std::vector< std::uint64_t > In( K.begin() , K.begin() + 100000 );
  RibbonFilter< std::uint64_t > F( In );
  RibbonFilter< std::uint64_t , std::uint16_t > G( In );
  RibbonFilter< std::uint64_t , std::uint32_t > W( In );
  CHECK( W.bits() == 2 * G.bits() && G.bits() == 2 * F.bits() );
  std::uint64_t sum = 0;
  std::size_t w = 0;
  for( std::size_t i = 100000 ; i < K.size() ; ++i ) {
   sum = sum * 7 + 4 * F.maybe( K[ i ] ) + 2 * G.maybe( K[ i ] ) +
         W.maybe( K[ i ] );
   w += W.maybe( K[ i ] );
   }
  for( std::uint64_t k : In )
   CHECK( W.maybe( k ) );
  CHECK( w <= 1 );                             // 10^5 / 2^32
  CHECK( sum == kRibbonAnswers );
  }
 {                                             // repeated, strings
  auto K = randomKeys( 5000 , 63 );
  K.insert( K.end() , K.begin() , K.begin() + 2500 );
  K.push_back( K[ 0 ] );
  RibbonFilter< std::uint64_t > F( K );
  CHECK( F.size() == 7501 && F.trials() < 10 );
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) );
  const auto S = randomStrings( 30000 , 64 );
  RibbonFilter< std::string > T( S );
  for( const auto & s : S )
   CHECK( T.maybe( s ) );
  }
 }

// the minimal perfect hash function: a bijection onto { 0 , ... , n - 1 }
// for 1, 2, 3, 10, 1000 and 10^5 random keys, 10^5 consecutive keys and
// strings, with the parameters ( lambda , alpha ) ( 1 , 0.5 ), ( 1 , 1 ),
// ( 5 , 0.5 ), ( 5 , 0.99 ), ( 5 , 1 ) and ( 10 , 0.9 ); about 3 bits per
// key; a key out of the set below n; repeated keys, the parameters out of
// range and an empty function violate the contracts
ADS_TEST( "minimal-perfect-hash" )
{
 auto bijective = []( const auto & f , const auto & K ) {
  std::vector< char > seen( K.size() , 0 );
  for( const auto & k : K ) {
   const std::size_t v = f( k );
   if( v >= K.size() || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( true ); };
 for( std::size_t n : { 1 , 2 , 3 , 10 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( 2 * n + 1 ) );
  for( auto [ lambda , alpha ] : { std::pair( 1.0 , 0.5 ) ,
                                    std::pair( 1.0 , 1.0 ) ,
                                    std::pair( 5.0 , 0.5 ) ,
                                    std::pair( 5.0 , 0.99 ) ,
                                    std::pair( 5.0 , 1.0 ) ,
                                    std::pair( 10.0 , 0.9 ) } ) {
    const MinimalPerfectHash< std::uint64_t > f( K , lambda , alpha );
    CHECK( f.size() == n && ! f.empty() && f.trials() >= 1 );
    CHECK( bijective( f , K ) );
    CHECK( f( 0xDEADBEEF ) < n );
    }
  }
 {
  const auto K = randomKeys( 1000000 , 21 );
  const MinimalPerfectHash< std::uint64_t > f( K );
  CHECK( bijective( f , K ) && double( f.bits() ) / 1e6 < 3.5 );
  }
 std::vector< std::uint64_t > C( 100000 );
 for( std::size_t i = 0 ; i < C.size() ; ++i )
  C[ i ] = i;
 CHECK( bijective( MinimalPerfectHash< std::uint64_t >( C ) , C ) );
 const auto S = randomStrings( 50000 , 22 );
 CHECK( bijective( MinimalPerfectHash< std::string >( S ) , S ) );
 const MinimalPerfectHash< int > E( std::vector< int >{} );
 CHECK( E.empty() && E.size() == 0 );
 CHECK_THROWS( (void) E( 3 ) , contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 , 2 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 } , 0.5 ) ,
               contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 } , 5 ,
                                          0.4 ) , contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 } , 5 ,
                                          1.1 ) , contract_violation );
 }

// the minimal perfect hash function by fingerprints: 1, 2, 3, 10, 1000 and
// 10^5 random keys with gamma 1, 1.5, 2 and 5; 10^6 keys in fewer than
// 3.7 bits per key at gamma 2; consecutive keys and strings; a key out of
// the set below n; equal hashes throw; repeated keys, gamma below 1 and an
// empty function violate the contracts
ADS_TEST( "fingerprint-perfect-hash" )
{
 auto bijective = []( const auto & f , const auto & K ) {
  std::vector< char > seen( K.size() , 0 );
  for( const auto & k : K ) {
   const std::size_t v = f( k );
   if( v >= K.size() || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( true ); };
 for( std::size_t n : { 1 , 2 , 3 , 10 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( 3 * n + 1 ) );
  for( double gamma : { 1.0 , 1.5 , 2.0 , 5.0 } ) {
   const FingerprintPerfectHash< std::uint64_t > f( K , gamma );
   CHECK( f.size() == n && ! f.empty() && f.levels() >= 1 );
   CHECK( bijective( f , K ) && f( 0xDEADBEEF ) < n );
   }
  }
 {
  const auto K = randomKeys( 1000000 , 23 );
  const FingerprintPerfectHash< std::uint64_t > f( K );
  const double b = double( f.bits() ) / 1e6;  // 2 e^{1/2} = 3.30 + 1/16
  CHECK( bijective( f , K ) && b > 3.2 && b < 3.7 && f.levels() < 40 );
  const FingerprintPerfectHash< std::uint64_t > g( K , 1.0 , 7 );
  CHECK( bijective( g , K ) && double( g.bits() ) / 1e6 < 3.1 );
  }
 std::vector< std::uint64_t > C( 100000 );
 for( std::size_t i = 0 ; i < C.size() ; ++i )
  C[ i ] = i;
 CHECK( bijective( FingerprintPerfectHash< std::uint64_t >( C ) , C ) );
 const auto S = randomStrings( 50000 , 24 );
 CHECK( bijective( FingerprintPerfectHash< std::string >( S ) , S ) );
 const FingerprintPerfectHash< int > E( std::vector< int >{} );
 CHECK( E.empty() && E.size() == 0 && E.levels() == 0 );
 CHECK_THROWS( (void) E( 3 ) , contract_violation );
 CHECK_THROWS( FingerprintPerfectHash< int >( std::vector< int >{ 1 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( FingerprintPerfectHash< int >( std::vector< int >{ 1 } ,
                                              0.5 ) , contract_violation );
 CHECK_THROWS( ( FingerprintPerfectHash< std::uint64_t , OneGroup >(
                  std::vector< std::uint64_t >{ 1 , 2 } ) ) ,
               std::runtime_error );
 }

// the minimal perfect hash function by recursive splitting, leaves of 8
// keys: 1, 2, 7, 8, 9 keys (below, at and above a leaf), 99, 100, 101 and
// 199, 200, 201 (a bucket and two, at b = 100), 1000 and 10^5 random keys
// with the buckets of 1, 5, 100 and 2000 keys; 10^5 consecutive keys and
// strings; between log2 e and 2.2 bits per key at b = 100 for 10^5 keys;
// the leaves of 1, 2, 4, 12 and 16 keys; a key out of the set below n;
// repeated keys, b out of range and an empty function violate the
// contracts
ADS_TEST( "recsplit" )
{
 auto bijective = []( const auto & f , const auto & K ) {
  std::vector< char > seen( K.size() , 0 );
  for( const auto & k : K ) {
   const std::size_t v = f( k );
   if( v >= K.size() || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( true ); };
 for( std::size_t n : { 1 , 2 , 7 , 8 , 9 , 99 , 100 , 101 , 199 , 200 ,
                        201 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( 3 * n + 1 ) );
  for( std::size_t b : { 1 , 5 , 100 , 2000 } ) {
   const RecSplit< std::uint64_t > f( K , b );
   CHECK( f.size() == n && ! f.empty() && f.trials() >= 1 );
   CHECK( f.buckets() == ( n + b - 1 ) / b );
   CHECK( bijective( f , K ) );
   CHECK( f( 0xDEADBEEF ) < n );
   }
  }
 {
  const auto K = randomKeys( 100000 , 31 );
  const RecSplit< std::uint64_t > f( K );
  const double bpk = double( f.bits() ) / 1e5;
  CHECK( bijective( f , K ) && bpk > 1.4427 && bpk < 2.2 );
  }
 std::vector< std::uint64_t > C( 100000 );
 for( std::size_t i = 0 ; i < C.size() ; ++i )
  C[ i ] = i;
 CHECK( bijective( RecSplit< std::uint64_t >( C ) , C ) );
 const auto S = randomStrings( 100000 , 32 );
 CHECK( bijective( RecSplit< std::string >( S ) , S ) );
 for( std::size_t n : { 1 , 3 , 50 , 10000 } ) {
  const auto K = randomKeys( n , unsigned( 5 * n + 3 ) );
  CHECK( bijective( RecSplit< std::uint64_t , 1 >( K ) , K ) );
  CHECK( bijective( RecSplit< std::uint64_t , 2 >( K , 7 ) , K ) );
  CHECK( bijective( RecSplit< std::uint64_t , 4 >( K ) , K ) );
  CHECK( bijective( RecSplit< std::uint64_t , 12 >( K , 500 ) , K ) );
  }
 {
  const auto K = randomKeys( 2000 , 33 );
  const RecSplit< std::uint64_t , 16 > f( K , 200 );
  CHECK( bijective( f , K ) && double( f.bits() ) / 2000 < 4 );
  }
 const RecSplit< int > E( std::vector< int >{} );
 CHECK( E.empty() && E.size() == 0 && E.trials() == 0 );
 CHECK_THROWS( (void) E( 3 ) , contract_violation );
 CHECK_THROWS( RecSplit< int >( std::vector< int >{ 1 , 2 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( RecSplit< int >( std::vector< int >{ 1 } , 0 ) ,
               contract_violation );
 CHECK_THROWS( RecSplit< int >( std::vector< int >{ 1 } , 2001 ) ,
               contract_violation );
 }

// the cuckoo table with buckets of 4 places against std::unordered_map on
// random insertions, updates, lookups and removals of words and of
// strings, through the growths of the table; 10^5 keys in a table made for
// them reach a load above 0.9 with no growth, the moves of the search in
// breadth; the iterators visit each pair once; an empty table, the
// removal of an absent key and a value changed by find()
ADS_TEST( "bucket-cuckoo" )
{
 {
  BucketCuckooTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 31 );
  for( int s = 0 ; s < 200000 ; ++s ) {
   const std::uint64_t k = g() % 50000 , v = g();
   switch( g() % 4 ) {
    case( 0 ): case( 1 ):
     CHECK( T.insert( k , v ) == R.insert_or_assign( k , v ).second );
     break;
    case( 2 ):
     CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) );
     break;
    default: {
     const auto it = R.find( k );
     const std::uint64_t * p = T.find( k );
     CHECK( ( p == nullptr ) == ( it == R.end() ) );
     if( p )
      CHECK( *p == it->second );
     }
    }
   CHECK( T.size() == R.size() );
   }
  std::size_t n = 0;
  for( const auto & [ k , v ] : T ) {
   CHECK( R.at( k ) == v );
   ++n;
   }
  CHECK( n == R.size() );
  }
 {                                             // strings
  BucketCuckooTable< std::string , int > T;
  std::unordered_map< std::string , int > R;
  const auto S = randomStrings( 5000 , 32 );
  std::mt19937 g( 33 );
  for( int s = 0 ; s < 40000 ; ++s ) {
   const std::string & k = S[ g() % S.size() ];
   if( g() % 3 )
    CHECK( T.insert( k , s ) == R.insert_or_assign( k , s ).second );
   else
    CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) );
   CHECK( T.size() == R.size() );
   }
  for( const auto & [ k , v ] : R )
   CHECK( T.lookup( k ) == v );
  }
 {                                             // a high load, no growth
  const auto K = randomKeys( 100000 , 34 );
  BucketCuckooTable< std::uint64_t , std::uint64_t > T( 0 );
  BucketCuckooTable< std::uint64_t , std::uint64_t > U( 117000 );
  const std::size_t cap = U.capacity();
  for( std::uint64_t k : K ) {
   CHECK( U.insert( k , k ) );
   T.insert( k , k );
   }
  CHECK( U.capacity() == cap && U.load() > 0.75 && U.rehashes() == 0 );
  CHECK( T.rehashes() > 10 && T.size() == K.size() );
  for( std::uint64_t k : K )
   CHECK( U.contains( k ) && *T.find( k ) == k );
  BucketCuckooTable< std::uint64_t , std::uint64_t > F( 0 );
  for( std::size_t i = 0 ; i < 8 ; ++i )      // 2 buckets, full
   CHECK( F.insert( K[ i ] , i ) );
  CHECK( F.capacity() == 8 && F.rehashes() == 0 && F.load() == 1.0 );
  CHECK( F.insert( K[ 8 ] , 8 ) && F.rehashes() >= 1 );
  for( std::size_t i = 0 ; i < 9 ; ++i )
   CHECK( F.lookup( K[ i ] ) == i );
  CHECK( U.moves() > 0 );
  }
 BucketCuckooTable< int , int > E;
 CHECK( E.empty() && E.begin() == E.end() && ! E.remove( 1 ) );
 CHECK( ! E.lookup( 1 ) && ! E.contains( 1 ) && E.find( 1 ) == nullptr );
 E.insert( 1 , 10 );
 *E.find( 1 ) = 11;
 CHECK( E.lookup( 1 ) == 11 && ! E.insert( 1 , 12 ) && E.lookup( 1 ) == 12 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ DENSE MAP ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the pairs of a DenseHashMap the same as those of a reference, the
/// vector dense
template< class M , class R >
void sameAs( const M & T , const R & Ref )
{
 CHECK( T.size() == Ref.size() &&
        std::size_t( T.end() - T.begin() ) == Ref.size() );
 for( const auto & [ k , v ] : T )
  CHECK( Ref.at( k ) == v );
 for( const auto & [ k , v ] : Ref )
  CHECK( T.lookup( k ) == v );
 }

ADS_TEST( "dense-hash-map" )
{
 {                                             // random operations
  DenseHashMap< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 41 );
  for( int s = 0 ; s < 200000 ; ++s ) {
   const std::uint64_t k = g() % 30000 , v = g();
   switch( g() % 4 ) {
    case( 0 ): case( 1 ):
     CHECK( T.insert( k , v ) == R.insert_or_assign( k , v ).second );
     break;
    case( 2 ):
     CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) );
     break;
    default: {
     const auto it = R.find( k );
     const std::uint64_t * p = T.find( k );
     CHECK( ( p == nullptr ) == ( it == R.end() ) );
     if( p )
      CHECK( *p == it->second );
     }
    }
   }
  sameAs( T , R );
  const std::size_t m = T.capacity();
  CHECK( T.load() <= 0.875 && m > 0 );
  auto C = T;                                  // copy, move, swap
  sameAs( C , R );
  auto M = std::move( C );
  CHECK( C.empty() && C.capacity() == 0 && ! C.contains( 1 ) );
  sameAs( M , R );
  C.insert( 1 , 2 );
  C.swap( M );
  sameAs( C , R );
  CHECK( M.size() == 1 && M.lookup( 1 ) == 2 );
  for( const auto & [ k , v ] : R )          // all out, in random order
   CHECK( T.remove( k ) );
  CHECK( T.empty() && T.begin() == T.end() && T.capacity() == m );
  }
 {                                             // strings
  DenseHashMap< std::string , int > T;
  std::unordered_map< std::string , int > R;
  const auto S = randomStrings( 5000 , 42 );
  std::mt19937 g( 43 );
  for( int s = 0 ; s < 40000 ; ++s ) {
   const std::string & k = S[ g() % S.size() ];
   if( g() % 3 )
    CHECK( T.insert( k , s ) == R.insert_or_assign( k , s ).second );
   else
    CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) );
   }
  sameAs( T , R );
  std::string a = "moved";
  CHECK( T.insert( std::move( a ) , 7 ) && T.lookup( "moved" ) == 7 );
  }
 {                                             // one group, one tag
  DenseHashMap< std::uint64_t , int , OneGroup > T;
  std::unordered_map< std::uint64_t , int > R;
  for( std::uint64_t k = 0 ; k < 300 ; ++k ) {
   CHECK( T.insert( k , int( k ) ) );
   R[ k ] = int( k );
   }
  sameAs( T , R );
  CHECK( T.deleted() == 0 );
  for( std::uint64_t k = 0 ; k < 300 ; k += 3 ) {   // the first group too
   CHECK( T.remove( k ) );
   R.erase( k );
   }
  CHECK( T.deleted() > 0 && ! T.remove( 0 ) );
  sameAs( T , R );
  for( std::uint64_t k = 1000 ; k < 1100 ; ++k ) {  // the places reused
   CHECK( T.insert( k , 1 ) );
   R[ k ] = 1;
   }
  sameAs( T , R );
  }
 {                          // churn at a steady size: recounts, no drift
  DenseHashMap< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 44 );
  std::vector< std::uint64_t > in;
  for( std::uint64_t k = 0 ; k < 1500 ; ++k ) {
   T.insert( k , k );
   R[ k ] = k;
   in.push_back( k );
   }
  const std::size_t m = T.capacity();
  for( std::uint64_t t = 0 ; t < 60000 ; ++t ) {
   const std::size_t i = g() % in.size();
   CHECK( T.remove( in[ i ] ) && R.erase( in[ i ] ) == 1 );
   in[ i ] = 1500 + t;
   CHECK( T.insert( in[ i ] , t ) );
   R[ in[ i ] ] = t;
   const std::size_t c = T.capacity();
   CHECK( T.size() + T.deleted() <= c - c / 8 );
   }
  CHECK( T.capacity() <= 2 * m );
  sameAs( T , R );
  }
 {                                             // sizes and reserve
  DenseHashMap< int , int > E;
  CHECK( E.empty() && E.capacity() == 0 && E.load() == 0.0 );
  CHECK( ! E.lookup( 1 ) && ! E.remove( 1 ) && E.find( 1 ) == nullptr );
  CHECK( E.deleted() == 0 && E.begin() == E.end() );
  CHECK( E.insert( 1 , 10 ) && E.capacity() == 24 );
  *E.find( 1 ) = 11;
  CHECK( ! E.insert( 1 , 12 ) && E.lookup( 1 ) == 12 && E.size() == 1 );
  for( int k = 0 ; k < 21 ; ++k ) E.insert( k , k );   // 7 / 8 of 24
  CHECK( E.capacity() == 24 && E.size() == 21 );
  E.insert( 21 , 21 );
  CHECK( E.capacity() == 48 && E.size() == 22 );
  CHECK( E.remove( 1 ) && ! E.contains( 1 ) && E.size() == 21 );
  DenseHashMap< int , int > F( 1000 );
  const std::size_t m = F.capacity();
  CHECK( m >= 1000 * 8 / 7 );
  for( int k = 0 ; k < 1000 ; ++k ) F.insert( k , -k );
  CHECK( F.capacity() == m );
  long s = 0;
  F.forEach( [ & ]( int , int v ) { s += v; } );
  CHECK( s == -499500 && F.data()[ 999 ].first == 999 );
  F.clear();
  CHECK( F.empty() && F.capacity() == m && ! F.contains( 5 ) );
  F.insert( 5 , 5 );
  CHECK( F.lookup( 5 ) == 5 && F.size() == 1 );
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
/*------------------------- End File hashing.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
