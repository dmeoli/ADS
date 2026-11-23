/*--------------------------------------------------------------------------*/
/*---------------------------- File hashing.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered hashing, beyond the book: the hash functions of
 * FastHash.h, the blocked filter of Bloom, the binary fuse filter, the
 * cuckoo filter, the prefix filter, the Ribbon filter, the range filters
 * of Grafite and by buckets, the minimal perfect hash functions by pilots
 * (also of 8 bits, PtrHash), by recursive splitting (also with the seeds of
 * Consensus) and the cuckoo table with buckets of 4 places. Each one is
 * checked against a reference of the standard library or against its own
 * analysis, on random and adversarial inputs, at the level 3 of the
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

#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/ConsensusRecSplit.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FingerprintPerfectHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/MinimalPerfectHash.h"
#include "ADS/dictionary/PtrHash.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/CountingQuotientFilter.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/GrafiteFilter.h"
#include "ADS/set/MementoFilter.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"
#include "ADS/set/VectorQuotientFilter.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash that sends every key to the same group, with the same
/// fingerprint, and declares it needs no mixing
struct OneGroup
{
 using is_avalanching = void;
 std::uint64_t operator()( std::uint64_t ) const { return( 0x105 ); }
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

/// the answers of the vector quotient filters of 8 and 16 bits of
/// 9 * 10^5 random keys to 10^6 absent ones, folded, the same on every
/// machine
constexpr std::uint64_t kVectorAnswers8 = 4473595884742287525ULL;
constexpr std::uint64_t kVectorAnswers16 = 11341926651226538773ULL;

/// the answers of the Ribbon filters of 8, 16 and 32 bits of 10^5 random
/// keys to 10^5 absent ones, folded, the same on every machine
constexpr std::uint64_t kRibbonAnswers = 15295418154177606124ULL;

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
 for( std::uint64_t a :
      { 0ULL , 1ULL , 0xFFFFFFFFULL , 1ULL << 63 , ~0ULL ,
	0x9E3779B97F4A7C15ULL } )
  for( std::uint64_t b :
       { 0ULL , 3ULL , 0xFFFFFFFFULL , ~0ULL , 0x123456789ABCDEF0ULL } ) {
   const Product128 q = product128( a , b );
   CHECK( q.lo == a * b && foldedProduct( a , b ) == ( q.lo ^ q.hi ) );
   const auto w = detail::wideProductHalves( a , b );
   CHECK( w.lo == q.lo && w.hi == q.hi );
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
 const std::size_t lens[] = {
  0 ,  1 ,  3 ,  4 ,  8 ,   9 ,   16 ,   17 ,   32 ,   33 ,
  64 , 65 , 96 , 97 , 256 , 257 , 1024 , 1025 , 4096 , 4999
  };
 const std::uint64_t known[] = {
  0xBC13060E2D1AAC79ULL , 0x5D16303249F2E44CULL , 0x944CD8618E651A8FULL ,
  0x96969D554F526124ULL , 0x112A8DA1E4CBC450ULL , 0xE9379B9163EE3F38ULL ,
  0xCE5EC7274574FD66ULL , 0x1E9C7E2CEF42118EULL , 0xA58DBB58B77161EFULL ,
  0x57C6910028675731ULL , 0x9BF13D56F64BA0F7ULL , 0x67B962D0635E1889ULL ,
  0xFE3EDA134B696614ULL , 0x1D8784A207CF8E9EULL , 0x0B9DC74F0DEF81A9ULL ,
  0x97D5950D50E76F48ULL , 0xF762805411CD9B93ULL , 0x576FF01834410863ULL ,
  0xAD70599F8618F739ULL , 0xF347ADC6514B2133ULL
  };
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
    const std::vector< unsigned char > s( n ,
					  static_cast< unsigned char >( c ) );
    if( ! std::equal( s.begin() , s.end() , buf.begin() ) )  // a prefix
     CHECK( H.insert( hashBytes( s.data() , n ) ).second );
    }
  std::unordered_set< std::uint64_t > W;
  for( std::uint64_t k = 0 ; k < 1000000 ; ++k )
   CHECK( W.insert( hashWord( k ) ).second );
  }
 {  // the avalanche
  std::mt19937_64 g( 7 );
  for( std::size_t n : { 1 , 3 , 4 , 7 , 8 , 12 , 16 , 24 , 32 , 48 , 64 , 80 ,
			 128 , 200 ,
			 256 , 300 , 1100 } ) {
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
 CHECK( FastHash< int >().hash64( -1 ) == hashWord( std::uint64_t( -1 ) ) );
 CHECK( FastHash< int >()( -1 ) ==
	std::size_t( hashWord( std::uint64_t( -1 ) ) ) );
 const char * c = "hashing";
 CHECK( FastHash< std::string >()( std::string( c ) ) ==
	FastHash< std::string_view >()( c ) );
 CHECK( FastHash< const char * >().hash64( c ) == hashBytes( c , 7 ) );
 CHECK( FastHash< std::string >{ 5 }( "x" ) !=
	FastHash< std::string >()( "x" ) );
 CHECK( hashBytes( nullptr , 0 ) == hashBytes( c , 0 ) );
 CHECK_THROWS( ( void )hashBytes( nullptr , 1 ) , contract_violation );
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
 {  // consecutive, std::hash
  BlockedBloomFilter< std::uint64_t , std::hash< std::uint64_t > > G( 100000 );
  for( std::uint64_t k = 0 ; k < 100000 ; ++k )
   G.insert( k );
  std::size_t f = 0;
  for( std::uint64_t k = 0 ; k < 100000 ; ++k )
   CHECK( G.maybe( k ) );
  for( std::uint64_t k = 100000 ; k < 200000 ; ++k )
   f += G.maybe( k );
  CHECK( double( f ) / 100000 < 1.25 * G.falsePositiveRate() );
  }
 {  // strings, the union
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
 BlockedBloomFilter< int > Z( 0 );  // one block
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
  CHECK( f > 3200 && f < 4600 );  // 10^6 / 256 = 3906
  CHECK( g < 40 );                // 10^6 / 65536 = 15
  CHECK( double( F.bits() ) / 1e6 < 9.2 && double( G.bits() ) / 1e6 < 18.4 );
  }
 {  // consecutive
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
 {  // repeated, strings
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

// the binary fuse filter of keys of 32 bits and of fingerprints of 32
// bits: every key maybe there for 0, 1, 3, 1000 and 10^5 keys, the keys 0
// and 2^32 - 1 among them; no false positive (about 2^-32 each) on 10^6
// absent keys, and about 36 bits per key
ADS_TEST( "binary-fuse-32" )
{
 for( std::size_t n : { 0 , 1 , 3 , 1000 , 100000 } ) {
  const auto W = randomKeys( 2 * n , unsigned( n + 3 ) );
  std::vector< std::uint32_t > K;
  for( std::size_t i = 0 ; i < n ; ++i )
   K.push_back( std::uint32_t( W[ i ] ) );
  if( n > 2 ) {
   K[ 0 ] = 0;
   K[ 1 ] = ~std::uint32_t( 0 );
   }
  BinaryFuseFilter< std::uint32_t > F( K );
  BinaryFuseFilter< std::uint64_t , std::uint32_t > G(
    std::vector< std::uint64_t >( W.begin() ,
				  W.begin() + std::ptrdiff_t( n ) ) );
  CHECK( F.empty() == ( n == 0 ) && G.size() == n );
  bool ok = true;
  for( std::uint32_t k : K )
   ok = ok && F.maybe( k );
  for( std::size_t i = 0 ; i < n ; ++i )
   ok = ok && G.maybe( W[ i ] );
  CHECK( ok );
  if( n == 0 )
   CHECK( ! F.maybe( 0 ) && ! G.maybe( 0 ) && G.bits() == 0 );
  }
 const auto K = randomKeys( 2000000 , 15 );
 const std::vector< std::uint64_t > In( K.begin() , K.begin() + 1000000 );
 BinaryFuseFilter< std::uint64_t , std::uint32_t > G( In );
 std::size_t f = 0;
 for( std::size_t i = 1000000 ; i < K.size() ; ++i )
  f += G.maybe( K[ i ] );
 CHECK( f < 4 );  // 10^6 / 2^32
 CHECK( double( G.bits() ) / 1e6 < 37 );
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
 for( int b : { 8 , 16 } ) {  // filled until full
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
 {  // repeated, adversarial
  CuckooFilter< std::uint64_t > F( 1000 );
  for( int i = 0 ; i < 8 ; ++i )
   CHECK( F.insert( 77 ) );
  CHECK( F.size() == 8 && F.maybe( 77 ) );
  for( int i = 0 ; i < 8 ; ++i )
   CHECK( F.remove( 77 ) );
  CHECK( F.empty() && ! F.maybe( 77 ) && ! F.remove( 77 ) );
  CuckooFilter< std::uint64_t , std::uint8_t , std::hash< std::uint64_t > > C(
    100000 );
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
 CuckooFilter< int > Z( 0 );  // one bucket
 CHECK( Z.capacity() == 4 && ! Z.maybe( 3 ) && ! Z.remove( 3 ) );
 for( int i = 0 ; i < 4 ; ++i )
  CHECK( Z.insert( i ) );
 CHECK( Z.insert( 4 ) && Z.size() == 5 && ! Z.insert( 5 ) );  // the victim
 for( int i = 0 ; i < 5 ; ++i )
  CHECK( Z.maybe( i ) );
 CHECK( Z.remove( 4 ) && Z.insert( 5 ) && Z.maybe( 5 ) );
 }

/// the hash of a key whose high 32 bits are 0: every key in the bin 0
struct OneBin
{
 using is_avalanching = void;

 std::uint64_t operator()( std::uint64_t k ) const {
  return( hashWord( k ) & 0xFFFFFFFFU );
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
 {  // adversarial
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
 {  // duplicates
  PrefixFilter< std::uint64_t > D( 1000 );
  for( int i = 0 ; i < 100 ; ++i )
   CHECK( D.insert( 77 ) );
  CHECK( D.size() == 100 && D.spareSize() == 75 && D.maybe( 77 ) );
  }
 {  // one bin
  const std::size_t n = 5000;
  const auto K = randomKeys( 2 * n , 67 );
  PrefixFilter< std::uint64_t , OneBin > B( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( B.insert( K[ i ] ) );
  CHECK( B.spareSize() == n - 25 && B.size() == n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( B.maybe( K[ i ] ) );
  }
 PrefixFilter< int > Z( 0 );  // no key: one bin
 CHECK( Z.bins() == 1 && Z.capacity() == 0 && ! Z.maybe( 3 ) );
 CHECK( ! Z.insert( 3 ) && Z.empty() && ! Z.maybe( 3 ) );
 }

/// n random hashes whose high p bits, their fingerprints in a quotient
/// filter of p bits, are distinct
std::vector< std::uint64_t > distinctPrints( std::size_t n , int p ,
					     unsigned seed )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::uint64_t > seen;
 std::vector< std::uint64_t > H;
 while( H.size() < n ) {
  const std::uint64_t h = g();
  if( seen.insert( h >> ( 64 - p ) ).second )
   H.push_back( h );
  }
 return( H );
 }

// the counting quotient filter: for 0, 1, 2, 63, 64, 65, 100, 1000 and
// 10^5 random keys every key in, counted at least once, all removed and
// the filter empty with no slot in use; the counts of 20000 hashes of
// distinct fingerprints exact along 4 * 10^5 random insertions and
// removals of 1 to 2^40 copies, a removal of more copies than counted or
// of an absent hash rejected with nothing changed; the counters of the
// remainders 0, 1, 2^r - 2 and 2^r - 1, side by side in one run, counted
// up to 600 one copy at a time and down again; at 10^6 keys the false
// positives on 10^6 absent keys near the bound D / 2^p; filled until an
// insertion fails, at the load 0.95, every key still there and the
// failed insertion changing nothing; the table doubled, halved and merged
// with itself and with another, the counts kept or added; remainders of
// 16 bits, consecutive keys by std::hash and strings
ADS_TEST( "counting-quotient-filter" )
{
 for( std::size_t n : { 0 , 1 , 2 , 63 , 64 , 65 , 100 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( n + 61 ) );
  CountingQuotientFilter< std::uint64_t > F( n );
  CHECK( F.empty() && std::has_single_bit( F.capacity() ) &&
	 F.capacity() >= 64 && 0.95 * double( F.capacity() ) >= double( n ) );
  CHECK( F.remainderBits() == 8 && F.bits() >= 10 * F.capacity() );
  for( std::uint64_t k : K )
   CHECK( F.insert( k ) );
  CHECK( F.size() == n && F.distinct() <= n && F.slotsUsed() >= F.distinct() );
  for( std::uint64_t k : K )
   CHECK( F.maybe( k ) && F.count( k ) >= 1 );
  for( std::uint64_t k : K )
   CHECK( F.remove( k ) );
  CHECK( F.empty() && F.slotsUsed() == 0 && F.distinct() == 0 );
  if( n > 0 )
   CHECK( ! F.maybe( K[ 0 ] ) && ! F.remove( K[ 0 ] ) );
  }
 {  // exact counts of distinct fingerprints
  CountingQuotientFilter< std::uint64_t > F( 100000 );
  const int p = F.fingerprintBits();
  const auto H = distinctPrints( 20000 , p , 62 );
  std::vector< std::uint64_t > C( H.size() , 0 );
  std::mt19937_64 g( 63 );
  for( int it = 0 ; it < 400000 ; ++it ) {
   const std::size_t i = g() % H.size();
   const std::uint64_t c =
     1 +
     ( g() % 8 == 0 ? g() % ( std::uint64_t( 1 ) << ( 1 + g() % 40 ) ) : 0 );
   if( g() % 3 != 0 ) {
    if( F.insertHash( H[ i ] , c ) )
     C[ i ] += c;
    }
   else {
    const std::size_t u = F.slotsUsed();
    const bool r = F.removeHash( H[ i ] , c );
    CHECK( r == ( C[ i ] >= c ) );
    if( r )
     C[ i ] -= c;
    else
     CHECK( F.slotsUsed() == u );
    }
   }
  std::uint64_t sum = 0;
  for( std::size_t i = 0 ; i < H.size() ; ++i ) {
   CHECK( F.countHash( H[ i ] ) == C[ i ] );
   sum += C[ i ];
   }
  CHECK( F.size() == sum );
  for( std::size_t i = 0 ; i < H.size() ; ++i )
   if( C[ i ] > 0 )
    CHECK( ! F.removeHash( H[ i ] , C[ i ] + 1 ) &&
	   F.removeHash( H[ i ] , C[ i ] ) );
  CHECK( F.empty() && F.slotsUsed() == 0 );
  }
 {  // the special remainders, side by side in the run of the quotient 5
  CountingQuotientFilter< std::uint64_t > F( 1000 );
  const int p = F.fingerprintBits() , r = F.remainderBits();
  const std::uint64_t q = 5 , X[] = { 0 , 1 , 254 , 255 };
  auto h = [ & ]( std::uint64_t x ) {
   return( ( ( q << r ) | x ) << ( 64 - p ) );
   };
  for( std::uint64_t c = 1 ; c <= 600 ; ++c )
   for( std::uint64_t x : X ) {
    CHECK( F.insertHash( h( x ) ) );
    for( std::uint64_t z : X )
     CHECK( F.countHash( h( z ) ) == ( z <= x ? c : c - 1 ) );
    }
  CHECK( F.countHash( h( 2 ) ) == 0 && F.size() == 2400 );
  for( std::uint64_t c = 600 ; c >= 1 ; --c )
   for( std::uint64_t x : X ) {
    CHECK( F.removeHash( h( x ) ) );
    CHECK( F.countHash( h( x ) ) == c - 1 );
    }
  CHECK( F.empty() && F.slotsUsed() == 0 );
  CHECK( F.insertHash( h( 0 ) , ~std::uint64_t( 0 ) ) &&
	 F.countHash( h( 0 ) ) == ~std::uint64_t( 0 ) );
  CHECK( F.removeHash( h( 0 ) , ~std::uint64_t( 0 ) ) && F.empty() );
  }
 {  // the false positives and the bits
  const auto K = randomKeys( 2000000 , 64 );
  CountingQuotientFilter< std::uint64_t > F( 1000000 );
  for( std::size_t i = 0 ; i < 1000000 ; ++i )
   CHECK( F.insert( K[ i ] ) );
  std::size_t f = 0;
  for( std::size_t i = 1000000 ; i < K.size() ; ++i )
   f += F.maybe( K[ i ] );
  const double e = 1e6 * F.falsePositiveBound();
  CHECK( double( f ) < e + 3 * std::sqrt( e ) && double( f ) > 0.85 * e );
  CHECK( F.capacity() == ( std::size_t( 1 ) << 21 ) &&
	 F.fingerprintBits() == 29 );
  for( std::size_t i = 0 ; i < 1000000 ; ++i )
   CHECK( F.maybe( K[ i ] ) );
  }
 {  // full, then resized and merged
  const auto K = randomKeys( 1 << 16 , 65 );
  CountingQuotientFilter< std::uint64_t > F( 20000 );
  std::size_t i = 0;
  while( i < K.size() && F.insert( K[ i ] ) )
   ++i;
  CHECK( i < K.size() && F.size() == i && F.load() > 0.94 &&
	 F.load() <= 0.95 );
  for( std::size_t j = 0 ; j < i ; ++j )
   CHECK( F.maybe( K[ j ] ) );
  const std::size_t u = F.slotsUsed();
  CHECK( ! F.insert( K[ i + 1 ] ) && F.size() == i && F.slotsUsed() == u );
  auto G = F;
  CHECK( G.resize( 2 * F.capacity() ) && G.remainderBits() == 7 &&
	 G.fingerprintBits() == F.fingerprintBits() && G.size() == i );
  for( std::size_t j = 0 ; j < i ; ++j )
   CHECK( G.count( K[ j ] ) == F.count( K[ j ] ) );
  CHECK( G.insert( K[ i ] ) && G.maybe( K[ i ] ) );
  CHECK( ! G.resize( F.capacity() ) && G.size() == i + 1 );  // too many
  auto M = F;
  CHECK( M.merge( F ) && M.size() == 2 * i &&
	 M.capacity() == 2 * F.capacity() );
  for( std::size_t j = 0 ; j < i ; ++j )
   CHECK( M.count( K[ j ] ) == 2 * F.count( K[ j ] ) );
  CountingQuotientFilter< std::uint64_t > A( 1000 ) , B( 1000 );
  for( std::size_t j = 0 ; j < 50 ; ++j )
   CHECK( A.insert( K[ j ] , j + 1 ) && B.insert( K[ j + 25 ] , 3 ) );
  CHECK( A.merge( B ) && A.size() == 1275 + 150 );
  for( std::size_t j = 0 ; j < 75 ; ++j )
   CHECK( A.count( K[ j ] ) >= ( j < 50 ? j + 1 : 0 ) + ( j >= 25 ? 3 : 0 ) );
  auto Z = A;  // doubled, then halved back
  CHECK( Z.resize( 2 * A.capacity() ) && Z.remainderBits() == 7 &&
	 Z.resize( A.capacity() ) && Z.remainderBits() == 8 &&
	 Z.size() == A.size() );
  for( std::size_t j = 0 ; j < 75 ; ++j )
   CHECK( Z.count( K[ j ] ) == A.count( K[ j ] ) );
  }
 {  // remainders of 16 bits, consecutive keys, strings
  CountingQuotientFilter< std::uint64_t , std::uint16_t > L( 300000 );
  CountingQuotientFilter< std::uint64_t , std::uint8_t ,
			  std::hash< std::uint64_t > >
    C( 100000 );
  CHECK( L.remainderBits() == 16 );
  for( std::uint64_t k = 0 ; k < 90000 ; ++k )
   CHECK( L.insert( k , k % 5 + 1 ) && C.insert( k ) );
  std::size_t f = 0;
  for( std::uint64_t k = 0 ; k < 90000 ; ++k ) {
   CHECK( L.count( k ) >= k % 5 + 1 && C.maybe( k ) );
   f += C.maybe( k + 90000 );
   }
  CHECK( double( f ) < 3 * 9e4 * C.falsePositiveBound() + 10 );
  const auto S = randomStrings( 30000 , 66 );
  CountingQuotientFilter< std::string > T( 2 * S.size() );
  for( const auto & s : S )
   CHECK( T.insert( s , 2 ) );
  for( const auto & s : S )
   CHECK( T.count( s ) >= 2 );
  }
 }

// the vector quotient filter of 8 and 16 bits: 0, 1, 2, 47, 48, 49, 100,
// 1000 and 10^5 random keys, every one maybe there, all removed and the
// filter empty; at 9 * 10^5 keys the false positives on 10^6 absent keys
// within 3 sigma of the bound 2 n / ( R 2^f ), and their answers equal to
// known ones, which the paths of Highway and of any machine must give
// alike; filled until an insertion fails, at a load above 0.93 (0.88 for
// 16 bits), every key still there, the failed insertion changing nothing
// and a removal making room; a key inserted 40 times and removed 40
// times; consecutive keys by std::hash, strings; the removal of a key
// absent from an empty filter
ADS_TEST( "vector-quotient-filter" )
{
 auto all = [ & ]( auto tag , unsigned s , double full ,
		   std::uint64_t answers ) {
  using V = VectorQuotientFilter< std::uint64_t , decltype( tag ) >;
  for( std::size_t n : { 0 , 1 , 2 , 47 , 48 , 49 , 100 , 1000 , 100000 } ) {
   const auto K = randomKeys( n , unsigned( n + 71 ) );
   V Q( n );
   CHECK( Q.empty() && Q.capacity() % s == 0 && Q.capacity() >= n );
   CHECK( Q.bits() == 512 * ( Q.capacity() / s ) );
   for( std::uint64_t k : K )
    CHECK( Q.insert( k ) );
   CHECK( Q.size() == n );
   for( std::uint64_t k : K )
    CHECK( Q.maybe( k ) );
   for( std::uint64_t k : K )
    CHECK( Q.remove( k ) );
   CHECK( Q.empty() );
   if( n > 0 )
    CHECK( ! Q.maybe( K[ 0 ] ) && ! Q.remove( K[ 0 ] ) );
   }
  {
   const auto K = randomKeys( 1900000 , 72 );
   V Q( 1000000 );
   for( std::size_t i = 0 ; i < 900000 ; ++i )
    CHECK( Q.insert( K[ i ] ) );
   std::size_t f = 0;
   std::uint64_t sum = 0;
   for( std::size_t i = 900000 ; i < K.size() ; ++i ) {
    const bool y = Q.maybe( K[ i ] );
    f += y;
    sum = sum * 31 + ( y ? i : 0 );
    }
   const double e = 1e6 * Q.falsePositiveBound();
   CHECK( double( f ) < e + 3 * std::sqrt( e ) + 3 &&
	  double( f ) > e - 3 * std::sqrt( e ) - 3 );
   CHECK( sum == answers );
   for( std::size_t i = 0 ; i < 900000 ; ++i )
    CHECK( Q.maybe( K[ i ] ) );
   }
  {  // filled until full
   const auto K = randomKeys( 1 << 18 , 73 );
   V Q( 100000 );
   std::size_t i = 0;
   while( i < K.size() && Q.insert( K[ i ] ) )
    ++i;
   CHECK( i < K.size() && Q.size() == i && Q.load() > full );
   for( std::size_t j = 0 ; j < i ; ++j )
    CHECK( Q.maybe( K[ j ] ) );
   CHECK( ! Q.insert( K[ i ] ) && Q.size() == i );
   for( std::size_t j = 0 ; j < i && ! Q.insert( K[ i ] ) ; ++j )
    CHECK( Q.remove( K[ j ] ) );  // room near the key, at last
   CHECK( Q.maybe( K[ i ] ) );
   Q.clear();
   CHECK( Q.empty() && ! Q.maybe( K[ 1 ] ) && Q.insert( K[ 1 ] ) );
   }
  {  // repeated
   V Q( 1000 );
   for( int i = 0 ; i < 40 ; ++i )
    CHECK( Q.insert( 77 ) );
   CHECK( Q.size() == 40 && Q.maybe( 77 ) );
   for( int i = 0 ; i < 40 ; ++i )
    CHECK( Q.remove( 77 ) );
   CHECK( Q.empty() && ! Q.maybe( 77 ) && ! Q.remove( 77 ) );
   }
  };
 all( std::uint8_t( 0 ) , 48 , 0.93 , kVectorAnswers8 );
 all( std::uint16_t( 0 ) , 28 , 0.88 , kVectorAnswers16 );
 VectorQuotientFilter< std::uint64_t , std::uint8_t ,
		       std::hash< std::uint64_t > >
   C( 100000 );
 for( std::uint64_t k = 0 ; k < 90000 ; ++k )
  CHECK( C.insert( k ) );
 std::size_t f = 0;
 for( std::uint64_t k = 0 ; k < 90000 ; ++k ) {
  CHECK( C.maybe( k ) );
  f += C.maybe( k + 90000 );
  }
 CHECK( double( f ) < 1.3 * 9e4 * C.falsePositiveBound() + 10 );
 const auto S = randomStrings( 30000 , 74 );
 VectorQuotientFilter< std::string > T( S.size() );
 for( const auto & s : S )
  CHECK( T.insert( s ) );
 for( const auto & s : S )
  CHECK( T.maybe( s ) );
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
  CHECK( f > 3200 && f < 4600 );  // 10^6 / 256 = 3906
  CHECK( g < 40 );                // 10^6 / 65536 = 15
  CHECK( double( F.bits() ) / 1e6 < 9.2 && double( G.bits() ) / 1e6 < 18.4 );
  for( std::uint64_t k : In )
   CHECK( F.maybe( k ) && G.maybe( k ) );
  }
 {  // consecutive, low bits
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
 {  // 32 bits, every machine the same
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
  CHECK( w <= 1 );  // 10^5 / 2^32
  CHECK( sum == kRibbonAnswers );
  }
 {  // repeated, strings
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

// the false positive rate of the static filters, whose fingerprint must be
// independent of the places (Lemma 10.17): 10^6 random keys and 10^6
// random keys out of them, for the fingerprints of 8 and 16 bits of the
// binary fuse and the Ribbon filters, the false positives within 3
// standard deviations of 10^6 2^-b; then 5 10^6 consecutive keys, whose
// binary fuse filter has segments of 2^15 places, and 10^6 keys after them
ADS_TEST( "filters-fingerprint-rate" )
{
 const std::size_t n = 1000000;
 const auto R = randomKeys( 2 * n , 61 );
 const std::vector< std::uint64_t > S( R.begin() , R.begin() + long( n ) ) ,
   Q( R.begin() + long( n ) , R.end() );
 auto within = []( std::size_t fp , std::size_t q , int b ) {
  const double p = std::ldexp( 1.0 , -b ) , mu = double( q ) * p;
  const double sd = std::sqrt( mu * ( 1 - p ) );
  return( std::abs( double( fp ) - mu ) <= 3 * sd );
  };
 auto rate = [ & ]( const auto & f , const std::vector< std::uint64_t > & K ,
		    const std::vector< std::uint64_t > & A , int b ) {
  bool all = true;
  for( std::uint64_t k : K )
   all = all && f.maybe( k );
  std::size_t fp = 0;
  for( std::uint64_t x : A )
   fp += f.maybe( x );
  return( all && within( fp , A.size() , b ) );
  };
 CHECK( rate( BinaryFuseFilter< std::uint64_t >( S ) , S , Q , 8 ) );
 CHECK(
   rate( BinaryFuseFilter< std::uint64_t , std::uint16_t >( S ) , S , Q ,
	 16 ) );
 CHECK( rate( RibbonFilter< std::uint64_t >( S ) , S , Q , 8 ) );
 CHECK( rate( RibbonFilter< std::uint64_t , std::uint16_t >( S ) , S , Q ,
	      16 ) );
 std::vector< std::uint64_t > C( 5 * n ) , D( n );
 for( std::size_t i = 0 ; i < C.size() ; ++i )
  C[ i ] = i;
 for( std::size_t i = 0 ; i < D.size() ; ++i )
  D[ i ] = C.size() + i;
 CHECK( rate( BinaryFuseFilter< std::uint64_t >( C ) , C , D , 8 ) );
 CHECK( rate( RibbonFilter< std::uint64_t >( C ) , C , D , 8 ) );
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
  return( true );
  };
 for( std::size_t n : { 1 , 2 , 3 , 10 , 1000 , 100000 } ) {
  const auto K = randomKeys( n , unsigned( 2 * n + 1 ) );
  for( auto [ lambda , alpha ] :
       { std::pair( 1.0 , 0.5 ) , std::pair( 1.0 , 1.0 ) ,
	 std::pair( 5.0 , 0.5 ) ,
	 std::pair( 5.0 , 0.99 ) , std::pair( 5.0 , 1.0 ) ,
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
 CHECK_THROWS( ( void )E( 3 ) , contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 , 2 , 1 } ) ,
	       contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 } , 0.5 ) ,
	       contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 } , 5 , 0.4 ) ,
	       contract_violation );
 CHECK_THROWS( MinimalPerfectHash< int >( std::vector< int >{ 1 } , 5 , 1.1 ) ,
	       contract_violation );
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
  return( true );
  };
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
 CHECK_THROWS( ( void )E( 3 ) , contract_violation );
 CHECK_THROWS( FingerprintPerfectHash< int >( std::vector< int >{ 1 , 1 } ) ,
	       contract_violation );
 CHECK_THROWS( FingerprintPerfectHash< int >( std::vector< int >{ 1 } , 0.5 ) ,
	       contract_violation );
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
  return( true );
  };
 for( std::size_t n :
      { 1 , 2 , 7 , 8 , 9 , 99 , 100 , 101 , 199 , 200 , 201 , 1000 ,
	100000 } ) {
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
 CHECK_THROWS( ( void )E( 3 ) , contract_violation );
 CHECK_THROWS( RecSplit< int >( std::vector< int >{ 1 , 2 , 1 } ) ,
	       contract_violation );
 CHECK_THROWS( RecSplit< int >( std::vector< int >{ 1 } , 0 ) ,
	       contract_violation );
 CHECK_THROWS( RecSplit< int >( std::vector< int >{ 1 } , 2001 ) ,
	       contract_violation );
 }

// the keys of the adversarial inputs of the minimal perfect hash
// functions: 0 to n - 1, their multiples of 2^32 (only the high bits
// vary) and of 2^20 + 1, and the 64 powers of 2
std::vector< std::vector< std::uint64_t > > adversarialKeys( std::size_t n )
{
 std::vector< std::vector< std::uint64_t > > A( 4 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  A[ 0 ].push_back( i );
  A[ 1 ].push_back( std::uint64_t( i ) << 32 );
  A[ 2 ].push_back( std::uint64_t( i ) *
		    ( ( std::uint64_t( 1 ) << 20 ) + 1 ) );
  }
 for( unsigned b = 0 ; b < 64 ; ++b )
  A[ 3 ].push_back( std::uint64_t( 1 ) << b );
 return( A );
 }

/// the sizes around the powers of 2 up to 2^16, and 0, 1, 2 and 3
std::vector< std::size_t > sizesAroundPowers( void )
{
 std::vector< std::size_t > N = { 0 , 1 , 2 , 3 };
 for( std::size_t p = 8 ; p <= 65536 ; p *= 8 )
  N.insert( N.end() , { p - 1 , p , p + 1 } );
 return( N );
 }

// the minimal perfect hash function by pilots of 8 bits: 0 to 3 keys and
// the sizes around 8, 64, 512, 4096, 32768 and 65536 with the linear and
// the cubic bucket functions and the two remaps, at ( lambda , alpha )
// ( 3 , 0.99 ), ( 3.5 , 0.99 ), ( 1 , 0.5 ) and ( 3 , 1 ); 10^6 random keys
// in about 3 bits per key (linear, lambda 3) and 2.4 (cubic and
// Elias-Fano, lambda 3.5); the adversarial integers, strings and the keys
// of 32 bits with their extremes; stream() as the queries one by one, also
// on keys out of the set; the parts built on a pool of threads as on one;
// a key out of the set below n; equal hashes throw; repeated keys, the
// parameters out of range and an empty function violate the contracts
ADS_TEST( "ptrhash" )
{
 auto bijective = []( const auto & f , const auto & K ) {
  std::vector< char > seen( K.size() , 0 );
  for( const auto & k : K ) {
   const std::size_t v = f( k );
   if( v >= K.size() || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( f.size() == K.size() );
  };
 using Fast = PtrHash< std::uint64_t >;
 using Compact =
   PtrHash< std::uint64_t , PtrHashBucket::Cubic , PtrHashRemap::EliasFano >;
 for( const std::size_t n : sizesAroundPowers() ) {
  const auto K = randomKeys( n , unsigned( 7 * n + 1 ) );
  for( auto [ lambda , alpha ] :
       { std::pair( 3.0 , 0.99 ) , std::pair( 3.5 , 0.99 ) ,
	 std::pair( 1.0 , 0.5 ) ,
	 std::pair( 3.0 , 1.0 ) } ) {
   const Fast f( K , lambda , alpha );
   const Compact g( K , lambda , alpha );
   CHECK( bijective( f , K ) && bijective( g , K ) );
   CHECK( ( f.empty() == ( n == 0 ) ) && ( f.slots() >= n ) );
   if( n > 0 )
    CHECK( ( f( 0xDEADBEEF ) < n ) && ( g( 0xDEADBEEF ) < n ) );
   }
  }
 {
  const auto K = randomKeys( 1000000 , 41 );
  const Fast f( K );
  const Compact g( K , 3.5 );
  const double bf = double( f.bits() ) / 1e6 , bg = double( g.bits() ) / 1e6;
  CHECK( bijective( f , K ) && ( bf > 2.6 ) && ( bf < 3.1 ) );
  CHECK( bijective( g , K ) && ( bg > 2.2 ) && ( bg < 2.5 ) );
  CHECK( ( f.trials() >= 1 ) && ( f.parts() >= 1 ) &&
	 ( f.buckets() >= 1000000 / 3 ) );
  std::vector< std::uint64_t > Q( K.begin() , K.begin() + 1000 );
  for( std::size_t i = 0 ; i < 1000 ; ++i )
   Q.push_back( 0xABCDEF0123456789ULL * ( i + 1 ) );  // also out of S
  std::vector< std::size_t > V( Q.size() ) , W( Q.size() );
  f.stream( Q , V );
  g.stream( Q , W );
  bool same = true;
  for( std::size_t i = 0 ; i < Q.size() ; ++i )
   same = same && ( V[ i ] == f( Q[ i ] ) ) && ( W[ i ] == g( Q[ i ] ) );
  CHECK( same );
  f.prefetch( K[ 0 ] );
  f.stream( std::span< const std::uint64_t >() , std::span< std::size_t >() );
  }
 for( const auto & A : adversarialKeys( 100000 ) ) {
  CHECK( bijective( Fast( A ) , A ) );
  CHECK( bijective( Compact( A , 3.5 ) , A ) );
  }
 const auto S = randomStrings( 50000 , 42 );
 CHECK( bijective( PtrHash< std::string >( S ) , S ) );
 {
  std::vector< std::uint32_t > K32 = { 0 , ~std::uint32_t( 0 ) };
  for( std::uint32_t i = 1 ; i < 70000 ; ++i )
   K32.push_back( i * 61 );
  CHECK( bijective( PtrHash< std::uint32_t >( K32 ) , K32 ) );
  CHECK( bijective(
    PtrHash< std::uint32_t , PtrHashBucket::Cubic , PtrHashRemap::EliasFano >(
      K32 , 3.9 ) ,
    K32 ) );
  }
 {
  // 12 parts of 10^5 keys at alpha = 0.9, on 4 threads and on one
  ThreadPool pool( 4 );
  const auto K = randomKeys( 100000 , 43 );
  const Fast f( K , 3 , 0.9 ) , g( pool , K , 3 , 0.9 );
  bool same = f.parts() > 1;
  for( const std::uint64_t k : K )
   same = same && ( f( k ) == g( k ) );
  CHECK( same && bijective( g , K ) );
  }
 const Fast E( std::vector< std::uint64_t >{} );
 CHECK( E.empty() && E.size() == 0 && E.trials() == 0 );
 CHECK_THROWS( ( void )E( 3 ) , contract_violation );
 CHECK_THROWS(
   ( PtrHash< std::uint64_t , PtrHashBucket::Linear , PtrHashRemap::Array ,
	      OneGroup >( std::vector< std::uint64_t >{ 1 , 2 } ) ) ,
   std::runtime_error );
 CHECK_THROWS( PtrHash< int >( std::vector< int >{ 1 , 2 , 1 } ) ,
	       contract_violation );
 CHECK_THROWS( PtrHash< int >( std::vector< int >{ 1 } , 0.5 ) ,
	       contract_violation );
 CHECK_THROWS( PtrHash< int >( std::vector< int >{ 1 } , 3 , 0.4 ) ,
	       contract_violation );
 CHECK_THROWS( PtrHash< int >( std::vector< int >{ 1 } , 3 , 1.1 ) ,
	       contract_violation );
 }

// the minimal perfect hash function by Consensus-RecSplit: 0 to 3 keys and
// the sizes around 8, 64, 512, 4096, 32768 and 65536 with buckets of 4, 64
// and 4096 keys and the overheads 0.01, 0.1 and 1 (the sizes below K go
// all to the fallback, those above a multiple of K leave the last values
// to it); 10^6 random keys between log2 e and 1.53 bits per key (K 512,
// epsilon 0.03) and below 1.48 (K 4096, epsilon 0.01); the adversarial
// integers, strings and the keys of 32 bits with their extremes; two
// chunks searched on a pool of threads as on one; a key out of the set
// below n; equal hashes throw; repeated keys, epsilon out of range and an
// empty function violate the contracts
ADS_TEST( "consensus-recsplit" )
{
 auto bijective = []( const auto & f , const auto & K ) {
  std::vector< char > seen( K.size() , 0 );
  for( const auto & k : K ) {
   const std::size_t v = f( k );
   if( v >= K.size() || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( f.size() == K.size() );
  };
 for( const std::size_t n : sizesAroundPowers() ) {
  const auto K = randomKeys( n , unsigned( 11 * n + 3 ) );
  for( const double eps : { 0.01 , 0.1 , 1.0 } ) {
   const ConsensusRecSplit< std::uint64_t , 4 > f( K , eps );
   const ConsensusRecSplit< std::uint64_t , 64 > g( K , eps );
   CHECK( bijective( f , K ) && bijective( g , K ) );
   CHECK( ( f.buckets() == n / 4 ) && ( g.buckets() == n / 64 ) );
   if( n > 0 )
    CHECK( ( f( 0xDEADBEEF ) < n ) && ( g( 0xDEADBEEF ) < n ) );
   }
  const ConsensusRecSplit< std::uint64_t > h( K , 0.1 );
  CHECK( bijective( h , K ) && ( h.bumped() <= n ) );
  }
 {
  const auto K = randomKeys( 1000000 , 51 );
  const ConsensusRecSplit< std::uint64_t , 512 > f( K , 0.03 );
  const ConsensusRecSplit< std::uint64_t > g( K );
  const double bf = double( f.bits() ) / 1e6 , bg = double( g.bits() ) / 1e6;
  CHECK( bijective( f , K ) && ( bf > 1.4427 ) && ( bf < 1.53 ) );
  CHECK( bijective( g , K ) && ( bg > 1.4427 ) && ( bg < 1.48 ) );
  }
 for( const auto & A : adversarialKeys( 100000 ) )
  CHECK( bijective( ConsensusRecSplit< std::uint64_t , 256 >( A , 0.1 ) ,
		    A ) );
 const auto S = randomStrings( 50000 , 52 );
 CHECK( bijective( ConsensusRecSplit< std::string , 256 >( S , 0.1 ) , S ) );
 {
  std::vector< std::uint32_t > K32 = { 0 , ~std::uint32_t( 0 ) };
  for( std::uint32_t i = 1 ; i < 70000 ; ++i )
   K32.push_back( i * 61 );
  CHECK(
    bijective( ConsensusRecSplit< std::uint32_t , 1024 >( K32 , 0.1 ) ,
	       K32 ) );
  }
 {
  // two chunks of 2^22 keys, the second one partial, on 4 threads and on
  // one
  ThreadPool pool( 4 );
  const auto K = randomKeys( ( std::size_t( 1 ) << 22 ) + 5000 , 53 );
  const ConsensusRecSplit< std::uint64_t , 64 > f( K , 1.0 ) ,
    g( pool , K , 1.0 );
  bool same = true;
  for( const std::uint64_t k : K )
   same = same && ( f( k ) == g( k ) );
  CHECK( same && bijective( g , K ) );
  }
 const ConsensusRecSplit< std::uint64_t > E( std::vector< std::uint64_t >{} );
 CHECK( E.empty() && E.size() == 0 && E.buckets() == 0 );
 CHECK_THROWS( ( void )E( 3 ) , contract_violation );
 CHECK_THROWS( ( ConsensusRecSplit< std::uint64_t , 64 , OneGroup >(
		 std::vector< std::uint64_t >{ 1 , 2 } ) ) ,
	       std::runtime_error );
 CHECK_THROWS( ConsensusRecSplit< int >( std::vector< int >{ 1 , 2 , 1 } ) ,
	       contract_violation );
 CHECK_THROWS( ConsensusRecSplit< int >( std::vector< int >{ 1 } , 0.0 ) ,
	       contract_violation );
 CHECK_THROWS( ConsensusRecSplit< int >( std::vector< int >{ 1 } , 1.5 ) ,
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
    case( 0 ):
    case( 1 ):
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
 {  // strings
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
 {  // a high load, no growth
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
  for( std::size_t i = 0 ; i < 8 ; ++i )  // 2 buckets, full
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

// the cuckoo table with buckets on a hash of one value for every key: 8
// keys fill the two buckets and the others go to the stash, with at most
// 2 doublings per insertion and at most 4 places per key; insertions,
// lookups, removals and visits against std::unordered_map, the keys 0 and
// 2^64 - 1 among them; then 8 keys on each of 64 hashes, whose groups
// that share a bucket leave keys in the stash through the doublings
ADS_TEST( "bucket-cuckoo-stash" )
{
 constexpr std::uint64_t top = std::numeric_limits< std::uint64_t >::max();
 BucketCuckooTable< std::uint64_t , std::uint64_t , OneGroup > T;
 std::unordered_map< std::uint64_t , std::uint64_t > R;
 const std::size_t P0 = T.capacity();
 std::size_t most = 0 , growths = T.rehashes();
 auto bounded = [ & ]( void ) {  // the doublings of one insertion
  const bool ok = ( T.rehashes() - growths <= 2 ) &&
		  ( T.capacity() <= std::max( P0 , 4 * most ) );
  growths = T.rehashes();
  return( ok );
  };
 for( std::uint64_t k = 0 ; k < 1000 ; ++k ) {
  const std::uint64_t key = k == 999 ? top : k;
  CHECK( T.insert( key , 3 * k ) && R.emplace( key , 3 * k ).second );
  most = std::max( most , T.size() );
  CHECK( bounded() );
  }
 CHECK( T.size() == 1000 && T.stashed() == 1000 - 8 );
 CHECK( T.rehashes() <= 10 && T.capacity() <= 4000 );
 CHECK( ! T.insert( 0 , 7 ) && *T.find( 0 ) == 7 && ! T.insert( top , 8 ) );
 R[ 0 ] = 7;
 R[ top ] = 8;
 *T.find( 500 ) = 1;
 R[ 500 ] = 1;
 auto same = [ & ]( void ) {
  std::size_t n = 0;
  bool ok = T.size() == R.size();
  for( const auto & [ k , v ] : T ) {
   const auto it = R.find( k );
   ok = ok && ( it != R.end() ) && ( it->second == v );
   ++n;
   }
  for( const auto & [ k , v ] : R )
   ok = ok && ( T.lookup( k ) == v ) && T.contains( k );
  return( ok && ( n == R.size() ) );
  };
 CHECK( same() );
 CHECK( ! T.contains( 1000 ) && ! T.find( 1000 ) && ! T.remove( 1000 ) );
 std::mt19937_64 g( 35 );
 for( int s = 0 ; s < 20000 ; ++s ) {
  const std::uint64_t r = g() % 1200;
  const std::uint64_t k = r == 1100 ? top : r , v = g();
  switch( g() % 3 ) {
   case( 0 ):
    CHECK( T.insert( k , v ) == R.insert_or_assign( k , v ).second );
    most = std::max( most , T.size() );
    CHECK( bounded() );
    break;
   case( 1 ):
    CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) );
    break;
   default: {
    const std::uint64_t * p = T.find( k );
    const auto it = R.find( k );
    CHECK( ( p == nullptr ) == ( it == R.end() ) );
    if( p )
     CHECK( *p == it->second );
    }
   }
  CHECK( T.size() == R.size() );
  }
 CHECK( same() );
 for( auto it = R.begin() ; it != R.end() ; )  // all removed
  if( T.remove( it->first ) )
   it = R.erase( it );
  else
   ++it;
 CHECK( R.empty() && T.empty() && T.stashed() == 0 && T.begin() == T.end() );
 CHECK( ! T.contains( 0 ) && ! T.contains( top ) );
 {  // 8 keys on each of 64 hashes, groups that may share a bucket
  struct Few
  {
   using is_avalanching [[maybe_unused]] = void;
   std::uint64_t operator()( std::uint64_t k ) const {
    return( hashWord( k % 64 ) );
    }
   };
  BucketCuckooTable< std::uint64_t , int , Few > F;
  for( std::uint64_t k = 0 ; k < 512 ; ++k )
   CHECK( F.insert( k , int( k ) ) );
  for( std::uint64_t k = 0 ; k < 512 ; ++k )
   CHECK( F.lookup( k ) == int( k ) );
  CHECK( F.size() == 512 && F.capacity() <= 4 * 512 );
  }
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
 {  // random operations
  DenseHashMap< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 41 );
  for( int s = 0 ; s < 200000 ; ++s ) {
   const std::uint64_t k = g() % 30000 , v = g();
   switch( g() % 4 ) {
    case( 0 ):
    case( 1 ):
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
  auto C = T;  // copy, move, swap
  sameAs( C , R );
  auto M = std::move( C );
  CHECK( C.empty() && C.capacity() == 0 && ! C.contains( 1 ) );
  sameAs( M , R );
  C.insert( 1 , 2 );
  C.swap( M );
  sameAs( C , R );
  CHECK( M.size() == 1 && M.lookup( 1 ) == 2 );
  for( const auto & [ k , v ] : R )  // all out, in random order
   CHECK( T.remove( k ) );
  CHECK( T.empty() && T.begin() == T.end() && T.capacity() == m );
  }
 {  // strings
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
 {  // one group, one tag
  DenseHashMap< std::uint64_t , int , OneGroup > T;
  std::unordered_map< std::uint64_t , int > R;
  for( std::uint64_t k = 0 ; k < 300 ; ++k ) {
   CHECK( T.insert( k , int( k ) ) );
   R[ k ] = int( k );
   }
  sameAs( T , R );
  CHECK( T.deleted() == 0 );
  for( std::uint64_t k = 0 ; k < 300 ; k += 3 ) {  // the first group too
   CHECK( T.remove( k ) );
   R.erase( k );
   }
  // the places counted: those of the 100 removals but the ones of the
  // last group, which no inspection went past (checked against a scan
  // of the groups by the invariant, on fewer than 1024 places)
  CHECK( T.deleted() >= 100 - 12 && T.deleted() <= 100 && ! T.remove( 0 ) );
  sameAs( T , R );
  for( std::uint64_t k = 1000 ; k < 1100 ; ++k ) {  // the places reused
   CHECK( T.insert( k , 1 ) );
   R[ k ] = 1;
   }
  sameAs( T , R );
  }
 {  // churn at a steady size: rebuilds of the same size, no drift
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
 {  // sizes and reserve
  DenseHashMap< int , int > E;
  CHECK( E.empty() && E.capacity() == 0 && E.load() == 0.0 );
  CHECK( ! E.lookup( 1 ) && ! E.remove( 1 ) && E.find( 1 ) == nullptr );
  CHECK( E.deleted() == 0 && E.begin() == E.end() );
  CHECK( E.insert( 1 , 10 ) && E.capacity() == 24 );
  *E.find( 1 ) = 11;
  CHECK( ! E.insert( 1 , 12 ) && E.lookup( 1 ) == 12 && E.size() == 1 );
  for( int k = 0 ; k < 21 ; ++k )
   E.insert( k , k );  // 7 / 8 of 24
  CHECK( E.capacity() == 24 && E.size() == 21 );
  E.insert( 21 , 21 );
  CHECK( E.capacity() == 48 && E.size() == 22 );
  CHECK( E.remove( 1 ) && ! E.contains( 1 ) && E.size() == 21 );
  DenseHashMap< int , int > F( 1000 );
  const std::size_t m = F.capacity();
  CHECK( m >= 1000 * 8 / 7 );
  for( int k = 0 ; k < 1000 ; ++k )
   F.insert( k , -k );
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


/*--------------------------------------------------------------------------*/

/// whether the sorted keys K hold a key in [ a , b ]
bool holds( const std::vector< std::uint64_t > & K , std::uint64_t a ,
	    std::uint64_t b )
{
 const auto it = std::lower_bound( K.begin() , K.end() , a );
 return( ( it != K.end() ) && ( *it <= b ) );
 }

/// the empty ranges [ k + 1 , k + l ] right after each key k of the sorted
/// K, the queries correlated with the keys, and how many of them F passes
template< class F >
std::pair< std::size_t , std::size_t >
correlated( const F & f , const std::vector< std::uint64_t > & K ,
	    std::uint64_t l )
{
 std::size_t q = 0 , fp = 0;
 for( std::size_t i = 0 ; i + 1 < K.size() ; ++i )
  if( K[ i + 1 ] - K[ i ] > l ) {
   ++q;
   fp += f.mayContain( K[ i ] + 1 , K[ i ] + l );
   }
 return( std::pair( q , fp ) );
 }

/// the false positives fp of q queries within the bound p of each, with a
/// margin of 6 standard deviations of the binomial, and 8 more, so that
/// the test fails only if the bound does not hold
bool withinBound( std::size_t fp , std::size_t q , double p )
{
 const double m = double( q ) * p;
 return( double( fp ) <= m + 6 * std::sqrt( m ) + 8 );
 }

/// try_emplace() of a flat map against that of std::unordered_map, with
/// growth, removals and values changed through the pointer
template< class M >
void tryEmplaceOf( void )
{
 M T;
 std::unordered_map< std::uint64_t , std::uint64_t > R;
 std::mt19937_64 g( 43 );
 for( int s = 0 ; s < 100000 ; ++s ) {
  const std::uint64_t k = g() % 5000 , v = g();
  if( g() % 4 == 0 ) {
   CHECK( T.remove( k ) == ( R.erase( k ) == 1 ) );
   continue;
   }
  const auto [ p , fresh ] = T.try_emplace( k , v );
  const auto [ it , rfresh ] = R.try_emplace( k , v );
  CHECK( ( fresh == rfresh ) && ( *p == it->second ) );
  if( g() % 2 ) {
   *p += 1;
   it->second += 1;
   }
  CHECK( T.size() == R.size() );
  }
 for( const auto & [ k , v ] : R )
  CHECK( T.find( k ) && ( *T.find( k ) == v ) );
 }

// try_emplace() of both flat maps, and a value built from its arguments
// only when its key is new
ADS_TEST( "flat-try-emplace" )
{
 tryEmplaceOf< DenseHashMap< std::uint64_t , std::uint64_t > >();
 tryEmplaceOf< FlatHashTable< std::uint64_t , std::uint64_t > >();
 auto moved = []< class M >( M & T ) {
  const auto a = T.try_emplace( std::string( "a" ) , 3 , 'x' );
  CHECK( a.second && ( *a.first == "xxx" ) );
  const auto b = T.try_emplace( std::string( "a" ) , 5 , 'y' );
  CHECK( ! b.second && ( *b.first == "xxx" ) );  // not touched
  for( int i = 0 ; i < 1000 ; ++i )                // through the growths
   CHECK(
     T.try_emplace( std::to_string( i ) , std::size_t( i % 7 ) ,
		    'z' ).second );
  CHECK( ( T.size() == 1001 ) && ( *T.find( "a" ) == "xxx" ) &&
	 ( T.find( "13" )->size() == 6 ) );
  };
 DenseHashMap< std::string , std::string > D;
 FlatHashTable< std::string , std::string > F;
 moved( D );
 moved( F );
 }

ADS_TEST( "grafite-filter" )
{
 constexpr std::uint64_t kMax = ~std::uint64_t( 0 );
 {  // empty
  const GrafiteFilter<> F( std::vector< std::uint64_t >{} );
  CHECK( F.empty() && F.size() == 0 && F.codes() == 0 );
  CHECK( ! F.maybe( 0 ) && ! F.maybe( kMax ) );
  CHECK( ! F.mayContain( 0 , kMax ) && ! F.mayContain( 5 , 5 ) );
  }
 for( const std::uint64_t k : { std::uint64_t( 0 ) , std::uint64_t( 1 ) ,
				std::uint64_t( 1 ) << 63 , kMax - 1 ,
				kMax } ) {
  // one key, at the ends of the universe too, and all keys equal
  const GrafiteFilter<> F( std::vector< std::uint64_t >( 7 , k ) , 10 );
  CHECK( F.size() == 1 && F.codes() == 1 && F.reduced() == 256 );
  CHECK( F.maybe( k ) && F.mayContain( k , k ) );
  CHECK( F.mayContain( 0 , kMax ) && F.mayContain( 0 , k ) &&
	 F.mayContain( k , kMax ) );
  if( k > 0 )
   CHECK( F.mayContain( k - 1 , k ) );
  if( k < kMax )
   CHECK( F.mayContain( k , k + 1 ) );
  // a range of r keys or more is maybe there; a shorter one, with no key
  // and inside one block, is not, the block of the key mapped one to one
  CHECK( F.mayContain( k < 1000 ? 1000 : 0 , k < 1000 ? 1255 : 255 ) );
  const std::uint64_t base = k - k % 256;
  if( k > base )
   CHECK( ! F.mayContain( base , k - 1 ) );
  if( k % 256 != 255 )
   CHECK( ! F.mayContain( k + 1 , base + 255 ) );
  }
 {  // the bound and the bits
  const auto K0 = randomKeys( 20000 , 21 );
  std::vector< std::uint64_t > K = K0;
  std::sort( K.begin() , K.end() );
  for( int B : { 8 , 12 , 16 } ) {
   const GrafiteFilter<> F( K0 , B , std::uint64_t( B ) );
   CHECK( F.size() == K.size() && F.codes() <= K.size() );
   CHECK( F.reduced() == std::uint64_t( K.size() ) << ( B - 2 ) );
   CHECK( double( F.sizeInBits() ) < double( K.size() ) * ( B + 1.5 ) );
   for( std::uint64_t k : K )
    CHECK( F.maybe( k ) && F.mayContain( k , k ) );
   std::mt19937_64 g( static_cast< std::uint64_t >( B ) );
   std::size_t q = 0 , fp = 0;
   for( int t = 0 ; t < 200000 ; ++t ) {     // ranges of 1 to 64 keys,
    const std::uint64_t l = 1 + g() % 64;  // at random, across the
    std::uint64_t a = g();                 // ends of the blocks too
    if( t % 4 == 0 )
     a = ( a / F.reduced() ) * F.reduced() - l / 2;
    const std::uint64_t b = a > kMax - l ? kMax : a + l - 1;
    const bool in = holds( K , a , b ) , ans = F.mayContain( a , b );
    CHECK( ans || ! in );  // no false negative
    if( ! in ) {
     ++q;
     fp += ans;
     }
    }
   CHECK( withinBound( fp , q , 64.0 / double( 1 << ( B - 2 ) ) ) );
   for( std::uint64_t l : { 1 , 2 , 16 , 256 } ) {  // correlated queries
    const auto [ cq , cfp ] = correlated( F , K , l );
    CHECK( withinBound( cfp , cq , F.falsePositiveBound( double( l ) ) ) );
    }
   }
  CHECK( GrafiteFilter<>( K , 12 ).falsePositiveBound( 1e9 ) == 1 );
  }
 {                                 // adversarial: dense keys,
  std::vector< std::uint64_t > K;  // the queries in their gaps
  for( std::uint64_t i = 0 ; i < 50000 ; ++i )
   K.push_back( kMax - 3 * i );     // up to the max of the
  std::sort( K.begin() , K.end() );  // universe
  const GrafiteFilter<> F( K , 14 , 7 );
  std::size_t fp = 0 , q = 0;
  for( std::size_t i = 0 ; i + 1 < K.size() ; ++i ) {
   CHECK( F.mayContain( K[ i ] , K[ i ] + 1 ) );
   ++q;
   fp += F.mayContain( K[ i ] + 1 , K[ i ] + 2 );  // the empty gap
   }
  CHECK( withinBound( fp , q , 2.0 / double( 1 << 12 ) ) );
  CHECK( F.mayContain( kMax - 1 , kMax ) && F.mayContain( kMax , kMax ) );
  }
 {  // signed and floating keys
  const std::vector< std::int64_t > I = {
   std::numeric_limits< std::int64_t >::min() , -5 , 0 , 7 ,
   std::numeric_limits< std::int64_t >::max()
   };
  const GrafiteFilter< std::int64_t > F( I , 12 );
  for( auto x : I )
   CHECK( F.maybe( x ) );
  CHECK( F.mayContain( -6 , -5 ) && F.mayContain( -1 , 1 ) );
  CHECK( F.mayContain( std::numeric_limits< std::int64_t >::min() ,
		       std::numeric_limits< std::int64_t >::max() ) );
  const std::vector< double > D = {
   -2.5 , -0.0 , 1e-300 , 3.0 , std::numeric_limits< double >::infinity()
   };
  const GrafiteFilter< double > G( D , 12 );
  for( double x : D )
   CHECK( G.maybe( x ) );
  CHECK( G.mayContain( -3.0 , -2.0 ) && G.mayContain( 2.0 , 4.0 ) &&
	 G.mayContain( -0.0 , 0.0 ) &&
	 G.mayContain( 1e300 , std::numeric_limits< double >::infinity() ) );
  }
 }

ADS_TEST( "bucketing-filter" )
{
 constexpr std::uint64_t kMax = ~std::uint64_t( 0 );
 {
  const BucketingFilter<> F( std::vector< std::uint64_t >{} );
  CHECK( F.buckets() == 0 && ! F.maybe( 0 ) && ! F.mayContain( 0 , kMax ) );
  }
 for( const std::uint64_t k : { std::uint64_t( 0 ) , kMax } ) {
  const BucketingFilter<> F( std::vector< std::uint64_t >( 3 , k ) , 10 );
  CHECK( F.buckets() == 1 && F.maybe( k ) && F.mayContain( 0 , kMax ) );
  CHECK( F.mayContain( k , k ) );
  }
 const auto K0 = randomKeys( 20000 , 23 );
 std::vector< std::uint64_t > K = K0;
 std::sort( K.begin() , K.end() );
 const BucketingFilter<> F( K0 , 12 );
 CHECK( double( F.sizeInBits() ) < double( K.size() ) * 13.5 );
 for( std::uint64_t k : K )
  CHECK( F.maybe( k ) );
 std::mt19937_64 g( 5 );
 std::size_t q = 0 , fp = 0;
 for( int t = 0 ; t < 200000 ; ++t ) {  // uncorrelated: exact but
  const std::uint64_t a = g() >> 1 , b = a + g() % 64;  // for the
  const bool in = holds( K , a , b ) , ans = F.mayContain( a , b );
  CHECK( ans || ! in );  // buckets of the keys
  if( ! in ) {
   ++q;
   fp += ans;
   }
  }
 CHECK( withinBound( fp , q , 2.0 / 1024 ) );
 // correlated: a gap shorter than a bucket is always a false positive
 const auto [ cq , cfp ] = correlated( F , K , 1 );
 CHECK( cfp == cq );
 }

// the range filters on keys of 32 bits, unsigned, signed and floating:
// every key maybe there, the ends of the universe among them, no false
// negative on random ranges of 1 to 64 keys and, for the integers, the false
// positives within the bound of Grafite and within that of the buckets,
// whose width is of the universe of 2^32 words of the keys
template< class T >
void rangeFilters32( void )
{
 std::mt19937_64 g( sizeof( T ) + std::is_signed_v< T > );
 const auto W = randomKeys( 5000 , 29 );
 std::vector< T > K;
 for( auto w : W )
  if constexpr( std::is_floating_point_v< T > )
   K.push_back( T( std::int32_t( w ) ) / T( 1000 ) );
  else
   K.push_back( T( w ) );
 K.push_back( std::numeric_limits< T >::lowest() );
 K.push_back( std::numeric_limits< T >::max() );
 const GrafiteFilter< T > F( K , 12 );
 const BucketingFilter< T > Bk( K , 12 );
 bool ok = true;
 for( T k : K )
  ok = ok && F.maybe( k ) && Bk.maybe( k ) && F.mayContain( k , k );
 CHECK( ok );
 CHECK( F.mayContain( std::numeric_limits< T >::lowest() ,
		      std::numeric_limits< T >::max() ) &&
	Bk.mayContain( std::numeric_limits< T >::lowest() ,
		       std::numeric_limits< T >::max() ) );
 if constexpr( ! std::is_floating_point_v< T > ) {
  std::vector< std::uint64_t > S;  // the words of the keys, sorted
  for( T k : K )
   S.push_back( std::uint64_t( radixKey( k ) ) );
  std::sort( S.begin() , S.end() );
  using U = std::make_unsigned_t< T >;
  std::size_t q = 0 , fpF = 0 , fpB = 0;
  for( int t = 0 ; t < 200000 ; ++t ) {
   const U a = U( g() ) , l = U( 1 + g() % 64 );
   const U b = a > U( ~U( 0 ) - l ) ? U( ~U( 0 ) ) : U( a + l - 1 );
   const T x = std::bit_cast< T >( U( a ^ ( U( radixKey( T() ) ) ) ) ) ,
	   y = std::bit_cast< T >( U( b ^ ( U( radixKey( T() ) ) ) ) );
   const bool in = holds( S , std::uint64_t( radixKey( x ) ) ,
			  std::uint64_t( radixKey( y ) ) );
   const bool f = F.mayContain( x , y ) , c = Bk.mayContain( x , y );
   ok = ok && ( f || ! in ) && ( c || ! in );
   if( ! in ) {
    ++q;
    fpF += f;
    fpB += c;
    }
   }
  CHECK( ok );
  CHECK( withinBound( fpF , q , 64.0 / 1024 ) );
  CHECK( withinBound( fpB , q , 66.0 / 1024 ) );
  CHECK( Bk.buckets() > K.size() / 2 );
  }
 }

ADS_TEST( "range-filters-32" )
{
 rangeFilters32< std::uint32_t >();
 rangeFilters32< std::int32_t >();
 rangeFilters32< float >();
 }

/// the ranges [ a , a + l - 1 ] of S, an ordered set, that hold no key,
/// from random points and from the keys plus one (the correlated ones)
template< class M >
void mementoRates( const M & F , const std::set< std::uint64_t > & S ,
		   std::uint64_t l , std::mt19937_64 & g )
{
 const std::vector< std::uint64_t > V( S.begin() , S.end() );
 for( int c = 0 ; c < 2 ; ++c ) {
  std::size_t q = 0 , f = 0;
  while( q < 300000 ) {
   const std::uint64_t a = c ? V[ g() % V.size() ] + 1 : g() , b = a + l - 1;
   if( b < a )
    continue;
   const auto it = S.lower_bound( a );
   if( it != S.end() && *it <= b )
    continue;
   ++q;
   f += F.mayContain( a , b );
   }
  const double e = double( q ) * F.falsePositiveBound();
  CHECK( double( f ) < e + 3 * std::sqrt( e ) );
  }
 }

// the Memento filter, mementos of 2, 3, 5 and 8 bits: 1.2 * 10^5 random
// insertions and removals of random keys, of keys dense in few partitions
// (whose boxes hold long lists, and lengths of more than one chunk for
// r = 2) and of keys in clusters, each range of at most 2^r keys holding
// a key answered true, every key maybe there, a removal of an absent key
// rejected unless the filter says maybe, the table doubled with every key
// still there, all removed and the filter empty; at 10^6 random keys the
// false positives of the empty ranges of 1 and 2^r keys, at random and
// right after a key, within 3 sigma of the bound 2 n rho / m; the keys
// 0 and 2^64 - 1, a range wider than the table, the empty filter, and
// keys of 32 bits, signed and floating
ADS_TEST( "memento-filter" )
{
 std::mt19937_64 g( 81 );
 for( int r : { 2 , 3 , 5 , 8 } )
  for( int kind : { 0 , 1 , 2 } ) {
   MementoFilter<> F( 50000 , r );
   CHECK( F.mementoBits() == r && F.fingerprintBits() == 16 - r );
   std::multiset< std::uint64_t > S;
   auto key = [ & ]( void ) {
    return( kind == 0   ? g()
	     : kind == 1 ? g() % 20000
			 : ( g() % 3000 ) * 1000003 + g() % 64 );
    };
   for( int it = 0 ; it < 120000 ; ++it ) {
    const std::uint64_t k = key();
    if( ( g() % 3 == 0 ) && ! S.empty() ) {
     auto i = S.lower_bound( k );
     if( i == S.end() )
      i = S.begin();
     CHECK( F.remove( *i ) );
     S.erase( i );
     }
    else if( g() % 7 == 0 ) {
     if( S.count( k + 1 ) ) {
      CHECK( F.remove( k + 1 ) );
      S.erase( S.find( k + 1 ) );
      }
     else if( ! F.maybe( k + 1 ) )
      CHECK( ! F.remove( k + 1 ) );
     }
    else if( F.insert( k ) )
     S.insert( k );
    }
   CHECK( F.size() == S.size() && F.boxes() <= S.size() );
   const std::uint64_t R = std::uint64_t( 1 ) << r;
   auto ranges = [ & ]( const MementoFilter<> & G ) {
    for( int it = 0 ; it < 20000 ; ++it ) {
     std::uint64_t a = g();
     if( ( it % 2 ) && ! S.empty() ) {
      auto i = S.lower_bound( g() );
      a = ( i == S.end() ? *S.begin() : *i ) - g() % ( 2 * R );
      }
     const std::uint64_t b = a + g() % R;
     if( b < a )
      continue;
     const auto i = S.lower_bound( a );
     if( i != S.end() && *i <= b )
      CHECK( G.mayContain( a , b ) );
     }
    for( std::uint64_t k : S )
     CHECK( G.maybe( k ) );
    };
   ranges( F );
   auto D = F;
   CHECK( D.resize( 2 * F.capacity() ) && D.size() == S.size() &&
	  D.fingerprintBits() == F.fingerprintBits() - 1 );
   ranges( D );
   for( std::uint64_t k : S )
    CHECK( F.remove( k ) );
   CHECK( F.empty() && F.boxes() == 0 && F.load() == 0 );
   }
 for( int r : { 5 , 8 } ) {  // the false positives
  const auto K = randomKeys( 1000000 , unsigned( 82 + r ) );
  MementoFilter<> F( K.size() , r );
  for( std::uint64_t k : K )
   CHECK( F.insert( k ) );
  const std::set< std::uint64_t > S( K.begin() , K.end() );
  mementoRates( F , S , 1 , g );
  mementoRates( F , S , std::uint64_t( 1 ) << r , g );
  CHECK( F.falsePositiveBound() < ( r == 5 ? 5e-4 : 4e-3 ) );
  }
 {  // the ends of the universe, the wide ranges, the empty filter
  constexpr std::uint64_t kMax = ~std::uint64_t( 0 );
  MementoFilter<> F( 100 , 4 );
  CHECK( ! F.mayContain( 0 , kMax ) && ! F.maybe( 0 ) && ! F.remove( 0 ) );
  CHECK( F.insert( 0 ) && F.insert( kMax ) && F.insert( kMax ) );
  CHECK( F.maybe( 0 ) && F.maybe( kMax ) && F.mayContain( 0 , kMax ) );
  CHECK( F.mayContain( kMax - 15 , kMax ) && F.mayContain( 0 , 15 ) );
  CHECK( F.remove( kMax ) && F.maybe( kMax ) && F.remove( kMax ) );
  CHECK( F.size() == 1 && F.mayContain( 1000 , kMax ) );  // wide
  }
 {  // keys of 32 bits
  MementoFilter< std::uint32_t > U( 1000 , 4 );
  MementoFilter< std::int32_t > I( 1000 , 4 );
  MementoFilter< float > R( 1000 , 4 , 6 );
  for( std::uint32_t k : { 0U , 1U , 0xFFFFFFF0U , 0xFFFFFFFFU } )
   CHECK( U.insert( k ) );
  CHECK( U.mayContain( 0xFFFFFFF1U , 0xFFFFFFFFU ) && U.maybe( 1 ) );
  for( std::int32_t k : { -5 , 0 , 7 } )
   CHECK( I.insert( k ) );
  CHECK( I.mayContain( -7 , -3 ) && I.mayContain( 1 , 8 ) && I.maybe( 0 ) );
  for( float k : { -1.5F , 0.0F , 2.25F } )
   CHECK( R.insert( k ) );
  CHECK( R.maybe( -1.5F ) && R.maybe( 2.25F ) && R.mayContain( 2.0F , 2.5F ) &&
	 R.fingerprintBits() == 6 );
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
