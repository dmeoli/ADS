/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_succinct.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered succinct structures and codes, beyond the book:
 * the compact rank and select and the wavelet matrix, against a scan, and
 * the coders of
 * Huffman and rANS, by their round trip and their size against the
 * entropy, and Stream VByte, by its round trip and its size, at the level 3
 * of the contracts.
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

#include "ADS/compression/DirectAccessCodes.h"
#include "ADS/compression/Entropy.h"
#include "ADS/compression/HuffmanCoder.h"
#include "ADS/compression/LZ77Linear.h"
#include "ADS/compression/Rans.h"
#include "ADS/compression/StreamVByte.h"
#include "ADS/compression/Tans.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/RRRBitVector.h"
#include "ADS/succinct/RankSelect.h"
#include "ADS/succinct/WaveletMatrix.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <thread>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the compact rank and select against a scan: every rank and every select
// of both bits on vectors of many sizes, around the blocks of 512 and the
// superblocks of 4096 bits, and densities; long runs of one bit; the empty
// vector and the bounds
ADS_TEST( "compact-rank-select" )
{
 std::mt19937_64 g( 2 );
 auto check = [ & ]( const std::vector< bool > & R ) {
  BitVector b;
  for( const bool x : R ) b.push_back( x );
  const CompactRankSelect S( b );
  std::size_t ones = 0 , zeros = 0;
  for( std::size_t i = 0 ; i <= R.size() ; ++i ) {
   CHECK( S.rank1( i ) == ones && S.rank0( i ) == zeros );
   if( i == R.size() ) break;
   if( R[ i ] ) CHECK( S.select1( ++ones ) == i );
   else CHECK( S.select0( ++zeros ) == i );
   }
  CHECK( S.ones() == ones && S.zeros() == zeros && S.size() == R.size() );
  CHECK( S.directoryBits() <= R.size() / 25 + 1024 );
  CHECK_THROWS( ( void )S.select1( ones + 1 ) , contract_violation );
  CHECK_THROWS( ( void )S.select0( zeros + 1 ) , contract_violation );
  CHECK_THROWS( ( void )S.rank1( R.size() + 1 ) , contract_violation );
  };
 for( const std::size_t n : { 0 , 1 , 511 , 512 , 513 , 4095 , 4096 , 4097 ,
                              70000 } )
  for( const double p : { 0.0 , 0.001 , 0.1 , 0.5 , 0.99 , 1.0 } ) {
   std::vector< bool > R( n );
   std::bernoulli_distribution B( p );
   for( std::size_t i = 0 ; i < n ; ++i ) R[ i ] = B( g );
   check( R );
   }
 std::vector< bool > runs;
 for( int k = 0 ; k < 8 ; ++k ) runs.insert( runs.end() , 30000 , k % 2 );
 check( runs );
 }

// the inventories of RankSelect built by the first select: 8 threads
// that select at once on a fresh directory, each every 1 and every 0, get
// the places of a scan; a copy made before the first select, one made
// after it and one assigned, each with its own inventories; the bits of
// the directory are the same before and after the inventories are built
ADS_TEST( "rank-select-lazy" )
{
 std::mt19937_64 g( 7 );
 for( const double p : { 0.003 , 0.3 , 0.97 } ) {
  BitVector b;
  std::vector< std::size_t > one , zero;
  std::bernoulli_distribution B( p );
  for( std::size_t i = 0 ; i < 200000 ; ++i ) {
   const bool x = B( g );
   b.push_back( x );
   ( x ? one : zero ).push_back( i );
   }
  const RankSelect S( b );
  const RankSelect before( S );
  const std::size_t bits = S.directoryBits();
  std::vector< int > bad( 8 , 0 );
  {
   std::vector< std::jthread > T;
   for( int t = 0 ; t < 8 ; ++t )
    T.emplace_back( [ & , t ]() {
     for( std::size_t k = std::size_t( t ) ; k < one.size() ; k += 8 )
      bad[ t ] += S.select1( k + 1 ) != one[ k ];
     for( std::size_t k = std::size_t( t ) ; k < zero.size() ; k += 8 )
      bad[ t ] += S.select0( k + 1 ) != zero[ k ];
     } );
  }
  CHECK( std::count( bad.begin() , bad.end() , 0 ) == 8 );
  CHECK( S.directoryBits() == bits );
  const RankSelect after( S );
  RankSelect assigned;
  assigned = after;
  for( const RankSelect * C : { &before , &after ,
                                ( const RankSelect * )&assigned } ) {
   bool ok = C->directoryBits() == bits;
   for( std::size_t k = 0 ; k < one.size() ; k += 97 )
    ok = ok && C->select1( k + 1 ) == one[ k ];
   for( std::size_t k = 0 ; k < zero.size() ; k += 97 )
    ok = ok && C->select0( k + 1 ) == zero[ k ];
   CHECK( ok );
   }
  }
 }

// RankSelect from the words, copied as they are counted, against the one
// from a BitVector of the same words: the bits, every rank and select of
// both values, on sizes around the words and the blocks of 256 bits, with
// 1s in the words past the size (which the copy cuts) and at many
// densities
ADS_TEST( "rank-select-words" )
{
 std::mt19937_64 g( 11 );
 for( const std::size_t n : { 0 , 1 , 63 , 64 , 65 , 255 , 256 , 257 , 511 ,
                              1000 , 1024 , 4099 , 70001 } )
  for( const double p : { 0.0 , 0.05 , 0.5 , 1.0 } ) {
   std::vector< Word > w( ( n + 63 ) / 64 + 3 );
   std::bernoulli_distribution B( p );
   for( auto & x : w )
    for( int i = 0 ; i < 64 ; ++i ) x |= Word( B( g ) ) << i;
   w.back() = ~Word( 0 );                  // past the size
   const RankSelect A( std::span< const Word >( w ) , n );
   const RankSelect C( BitVector( std::span< const Word >( w ) , n ) );
   bool ok = A.size() == n && A.ones() == C.ones() &&
             A.bits() == C.bits() && A.directoryBits() == C.directoryBits();
   for( std::size_t i = 0 ; ok && i <= n ; ++i )
    ok = A.rank1( i ) == C.rank1( i );
   for( std::size_t k = 1 ; ok && k <= A.ones() ; ++k )
    ok = A.select1( k ) == C.select1( k );
   for( std::size_t k = 1 ; ok && k <= A.zeros() ; ++k )
    ok = A.select0( k ) == C.select0( k );
   CHECK( ok );
   }
 }

// the compressed vector of bits of RRR against a scan: every bit, rank
// and select of both bits, on vectors of many sizes around the blocks of 63
// and the superblocks of 2016 bits and of many densities (all 0s, all 1s
// in blocks of class 63); and the size within the entropy and 0.18 bits
// more per bit
ADS_TEST( "rrr-bit-vector" )
{
 std::mt19937_64 g( 23 );
 for( const std::size_t n : { 0 , 1 , 62 , 63 , 64 , 2016 , 2017 , 30000 } )
  for( const double p : { 0.0 , 0.01 , 0.2 , 0.5 , 0.97 , 1.0 } ) {
   BitVector b;
   std::bernoulli_distribution B( p );
   for( std::size_t i = 0 ; i < n ; ++i ) b.push_back( B( g ) );
   const RRRBitVector R( b );
   std::size_t ones = 0 , zeros = 0;
   for( std::size_t i = 0 ; i <= n ; ++i ) {
    CHECK( R.rank1( i ) == ones && R.rank0( i ) == zeros );
    if( i == n ) break;
    CHECK( R[ i ] == b[ i ] );
    if( b[ i ] ) CHECK( R.select1( ++ones ) == i );
    else CHECK( R.select0( ++zeros ) == i );
    }
   CHECK( R.ones() == ones && R.zeros() == zeros && R.size() == n );
   const double q = n ? double( ones ) / double( n ) : 0.0;
   const double h = ( q <= 0 || q >= 1 ) ? 0 :
                    -q * std::log2( q ) - ( 1 - q ) * std::log2( 1 - q );
   CHECK( double( R.sizeInBits() ) <= double( n ) * ( h + 0.18 ) + 512 );
   CHECK_THROWS( ( void )R.select1( ones + 1 ) , contract_violation );
   CHECK_THROWS( ( void )R.rank1( n + 1 ) , contract_violation );
   }
 }

// the wavelet matrix against a scan: access, rank and select of every item
// and of symbols absent or beyond the width, quantile and countLess on
// random ranges, the iterators; alphabets of 1, 2, 3, 8 and 64 bits,
// a sequence of one symbol, the empty sequence, items near 2^64, and the
// bounds of the queries
ADS_TEST( "wavelet-matrix" )
{
 std::mt19937_64 g( 1 );
 auto check = [ & ]( const std::vector< std::uint64_t > & x , int w ) {
  const WaveletMatrix M( x , w );
  const std::size_t n = x.size();
  CHECK( M.size() == n && M.empty() == ( n == 0 ) );
  CHECK( std::equal( x.begin() , x.end() , M.begin() , M.end() ) );
  std::vector< std::uint64_t > C = x;       // the symbols, and two absent
  C.push_back( 0 );
  C.push_back( w < 64 ? ( std::uint64_t( 1 ) << w ) - 1 : ~0ULL );
  if( w < 64 ) C.push_back( std::uint64_t( 1 ) << w );   // too wide
  std::sort( C.begin() , C.end() );
  C.erase( std::unique( C.begin() , C.end() ) , C.end() );
  if( C.size() > 40 ) C.resize( 40 );
  for( const auto c : C ) {
   std::size_t r = 0;
   for( std::size_t i = 0 ; i <= n ; ++i ) {
    CHECK( M.rank( c , i ) == r );
    if( i == n ) break;
    if( x[ i ] == c ) {
     ++r;
     CHECK( M.select( c , r ) == i );
     }
    }
   CHECK_THROWS( ( void )M.select( c , r + 1 ) , contract_violation );
   }
  for( int q = 0 ; ( q < 200 ) && ( n > 0 ) ; ++q ) {
   std::size_t i = g() % ( n + 1 ) , j = g() % ( n + 1 );
   if( i > j ) std::swap( i , j );
   std::vector< std::uint64_t > R( x.begin() + std::ptrdiff_t( i ) ,
                                   x.begin() + std::ptrdiff_t( j ) );
   std::sort( R.begin() , R.end() );
   for( std::size_t k = 0 ; k < R.size() ; k += 1 + R.size() / 7 )
    CHECK( M.quantile( i , j , k ) == R[ k ] );
   CHECK_THROWS( ( void )M.quantile( i , j , R.size() ) ,
                 contract_violation );
   const std::uint64_t c = n > 0 ? x[ g() % n ] + ( g() % 3 ) - 1 : 0;
   CHECK( M.countLess( i , j , c ) == std::size_t(
           std::lower_bound( R.begin() , R.end() , c ) - R.begin() ) );
   }
  CHECK_THROWS( ( void )M.access( n ) , contract_violation );
  CHECK_THROWS( ( void )M.rank( 0 , n + 1 ) , contract_violation );
  CHECK_THROWS( ( void )M.countLess( 1 , 0 , 0 ) , contract_violation );
  };
 for( const int w : { 1 , 2 , 3 , 8 , 64 } )
  for( const std::size_t n : { 0 , 1 , 2 , 7 , 513 , 3000 } ) {
   std::vector< std::uint64_t > x( n );
   for( auto & v : x )
    v = w == 64 ? g() : g() % ( std::uint64_t( 1 ) << w );
   check( x , w );
   }
 check( std::vector< std::uint64_t >( 1000 , 5 ) , 3 );   // one symbol
 std::vector< std::uint64_t > big = { ~0ULL , 0 , ~0ULL - 1 , 1ULL << 63 };
 check( big , 64 );
 const WaveletMatrix A( big );              // the width of the largest
 CHECK( A.width() == 64 && A[ 0 ] == ~0ULL );
 const WaveletMatrix B( std::vector< std::uint64_t >{ 4 , 1 } );
 CHECK( B.width() == 3 && B.quantile( 0 , 2 , 0 ) == 1 &&
        B.countLess( 0 , 2 , 100 ) == 2 && B.rank( 9 , 2 ) == 0 );
 const WaveletMatrix E;
 CHECK( E.empty() && E.sizeInBits() == 0 );
 CHECK_THROWS( WaveletMatrix( std::vector< std::uint64_t >{ 8 } , 3 ) ,
               contract_violation );
 CHECK_THROWS( WaveletMatrix( std::vector< std::uint64_t >{ 1 } , 65 ) ,
               contract_violation );
 }

/// the strings of the round trips of the coders: empty, one byte, one
/// distinct byte, two, all 256, a skewed source whose codes of Huffman
/// reach the limit of 11 bits, uniform bytes, a text, and sizes around the
/// four streams and the groups of 4
std::vector< std::vector< std::uint8_t > > coderInputs( void )
{
 std::mt19937_64 g( 7 );
 std::vector< std::vector< std::uint8_t > > S = { {} , { 42 } ,
  std::vector< std::uint8_t >( 1000 , 9 ) , { 0 , 255 } };
 std::vector< std::uint8_t > all( 256 );
 for( int i = 0 ; i < 256 ; ++i ) all[ std::size_t( i ) ] = std::uint8_t( i );
 S.push_back( all );
 for( const std::size_t n : { 2 , 3 , 5 , 7 , 15 , 16 , 17 , 100003 } ) {
  std::vector< std::uint8_t > u( n ) , z( n );
  for( auto & c : u ) c = std::uint8_t( g() );
  // Fibonacci-like frequencies: byte k with probability about 2^-k
  for( auto & c : z ) c = std::uint8_t( std::min( 40 , std::countr_zero(
                                          g() | ( 1ULL << 40 ) ) ) );
  S.push_back( u );
  S.push_back( z );
  }
 const std::string t = "it was the best of times, it was the worst of "
                       "times, it was the age of wisdom, it was the age of "
                       "foolishness";
 std::vector< std::uint8_t > text;
 for( int r = 0 ; r < 2000 ; ++r )
  text.insert( text.end() , t.begin() , t.end() );
 S.push_back( text );
 return( S );
 }

// the code of Huffman of bytes: the round trip, the lengths within 11 bits
// and their Kraft sum, the size within the entropy and one bit per byte
// (and the header), package-merge against the code of Huffman when it fits
// and on a skewed source that it cuts, and the malformed inputs
ADS_TEST( "huffman-coder" )
{
 for( const auto & s : coderInputs() ) {
  const auto c = huffmanCompress( s );
  CHECK( huffmanDecompress( c ) == s );
  if( s.size() > 1000 ) {
   const double h = entropy( std::string_view(
                     reinterpret_cast< const char * >( s.data() ) ,
                     s.size() ) );
   CHECK( 8.0 * double( c.size() ) <= double( s.size() ) * ( h + 1 ) +
                                       8 * 200 );
   }
  }
 std::array< std::uint64_t , 256 > f{};
 for( int k = 0 ; k < 40 ; ++k ) f[ std::size_t( k ) ] = 1ULL << ( 40 - k );
 const auto len = limitedHuffmanLengths( f , 11 );
 std::uint64_t kraft = 0;
 for( int k = 0 ; k < 256 ; ++k ) {
  CHECK( ( len[ std::size_t( k ) ] > 0 ) == ( k < 40 ) );
  CHECK( len[ std::size_t( k ) ] <= 11 );
  if( len[ std::size_t( k ) ] > 0 )
   kraft += 1ULL << ( 11 - len[ std::size_t( k ) ] );
  }
 CHECK( kraft == 2048 );                    // complete
 const auto free = limitedHuffmanLengths( f , 40 );   // Huffman: 1, 2, ...
 CHECK( free[ 0 ] == 1 && free[ 1 ] == 2 && free[ 38 ] == 39 &&
        free[ 39 ] == 39 );
 std::array< std::uint64_t , 256 > one{};
 one[ 5 ] = 3;
 CHECK( limitedHuffmanLengths( one , 11 )[ 5 ] == 1 );
 CHECK_THROWS( ( void )huffmanDecompress( std::vector< std::uint8_t >( 5 ) ) ,
               contract_violation );
 auto c = huffmanCompress( coderInputs().back() );
 c[ 9 ] = 0x11;                            // two codes of 1 bit too many
 CHECK_THROWS( ( void )huffmanDecompress( c ) , contract_violation );
 }

// rANS of order 0: the round trip, the size within a percent of the
// entropy (and the header) on the large strings, the scaled frequencies
// (sum, positive ones kept positive, a single byte), and the malformed
// inputs
ADS_TEST( "rans-coder" )
{
 for( const auto & s : coderInputs() ) {
  const auto c = ransCompress( s );
  CHECK( ransDecompress( c ) == s );
  if( s.size() > 10000 ) {
   const double h = entropy( std::string_view(
                     reinterpret_cast< const char * >( s.data() ) ,
                     s.size() ) );
   CHECK( 8.0 * double( c.size() ) <= double( s.size() ) * h * 1.01 +
                                       8 * 600 );
   }
  }
 std::array< std::uint64_t , 256 > f{};
 f[ 0 ] = 1000000;
 for( int k = 1 ; k < 256 ; ++k ) f[ std::size_t( k ) ] = 1;
 const auto g = scaleFrequencies( f , 12 );
 std::uint64_t sum = 0;
 for( const auto x : g ) {
  CHECK( x >= 1 );
  sum += x;
  }
 CHECK( sum == 4096 && g[ 0 ] == 4096 - 255 );
 std::array< std::uint64_t , 256 > one{};
 one[ 200 ] = 1;
 CHECK( scaleFrequencies( one , 12 )[ 200 ] == 4096 );
 CHECK_THROWS( ( void )scaleFrequencies( {} , 12 ) , contract_violation );
 CHECK_THROWS( ( void )ransDecompress( std::vector< std::uint8_t >( 20 ) ) ,
               contract_violation );
 auto c = ransCompress( coderInputs().back() );
 c[ 8 ] ^= 1;                              // the frequencies sum wrong
 CHECK_THROWS( ( void )ransDecompress( c ) , contract_violation );
 }

// tANS of order 0: the round trip, the size within a percent of the
// entropy (and the header) on the large strings, and the malformed inputs
ADS_TEST( "tans-coder" )
{
 for( const auto & s : coderInputs() ) {
  const auto c = tansCompress( s );
  CHECK( tansDecompress( c ) == s );
  if( s.size() > 10000 ) {
   const double h = entropy( std::string_view(
                     reinterpret_cast< const char * >( s.data() ) ,
                     s.size() ) );
   CHECK( 8.0 * double( c.size() ) <= double( s.size() ) * h * 1.01 +
                                       8 * 600 );
   }
  }
 CHECK_THROWS( ( void )tansDecompress( std::vector< std::uint8_t >( 20 ) ) ,
               contract_violation );
 auto c = tansCompress( coderInputs().back() );
 c[ 8 ] ^= 1;                              // the frequencies sum wrong
 CHECK_THROWS( ( void )tansDecompress( c ) , contract_violation );
 auto d = tansCompress( coderInputs().back() );
 d[ 8 + 512 ] ^= 0x40;                     // a wrong length of the stream
 CHECK_THROWS( ( void )tansDecompress( d ) , contract_violation );
 }

// the directly addressable codes: every item of sequences of geometric,
// uniform and skewed integers, of all zeros, of one item, of the largest
// integers; the widths cover the bits of the largest item, and the bits
// stay within those of a plain array of fixed width (and its ranks) and
// below those of a split in bytes on a skewed sequence; the levels of a
// fixed width
ADS_TEST( "direct-access-codes" )
{
 std::mt19937_64 g( 13 );
 auto check = [ & ]( const std::vector< std::uint64_t > & x ) {
  const DirectAccessCodes D( x );
  CHECK( D.size() == x.size() && D.empty() == x.empty() );
  CHECK( std::equal( x.begin() , x.end() , D.begin() , D.end() ) );
  std::uint64_t m = 0;
  for( const auto v : x ) m |= v;
  int sum = 0;
  for( const int b : D.widths() ) {
   CHECK( b >= 1 );
   sum += b;
   }
  if( ! x.empty() ) CHECK( sum == std::max( 1 , int( std::bit_width( m ) ) ) );
  CHECK( D.sizeInBits() <= std::size_t( 1.05 * double( x.size() ) *
                                        std::max( 1 , sum ) ) + 2048 );
  CHECK_THROWS( ( void )D.access( x.size() ) , contract_violation );
  return( D.sizeInBits() );
  };
 std::geometric_distribution< std::uint64_t > G( 0.01 );
 for( const std::size_t n : { 1 , 2 , 100 , 5000 } ) {
  std::vector< std::uint64_t > a( n ) , b( n ) , c( n );
  for( auto & v : a ) v = G( g );
  for( auto & v : b ) v = g() >> ( g() % 64 );
  for( auto & v : c ) v = g() % 4;
  check( a );
  check( b );
  check( c );
  }
 check( std::vector< std::uint64_t >( 300 , 0 ) );
 check( std::vector< std::uint64_t >( 300 , ~0ULL ) );
 check( std::vector< std::uint64_t >{ 1ULL << 63 , 0 , 5 } );
 check( {} );
 for( const int b : { 1 , 4 , 8 , 64 } ) {   // the levels of fixed width
  std::vector< std::uint64_t > a( 3000 );
  for( auto & v : a ) v = G( g ) << ( g() % 20 );
  const DirectAccessCodes D( a , b );
  CHECK( std::equal( a.begin() , a.end() , D.begin() , D.end() ) );
  const auto W = D.widths();
  for( std::size_t k = 0 ; k + 1 < W.size() ; ++k ) CHECK( W[ k ] == b );
  CHECK( W.back() <= b );
  }
 CHECK_THROWS( DirectAccessCodes( std::vector< std::uint64_t >{ 1 } , 0 ) ,
               contract_violation );
 std::vector< std::uint64_t > z( 20000 );      // 3 bits but a few of 40
 for( std::size_t i = 0 ; i < z.size() ; ++i )
  z[ i ] = i % 1000 == 0 ? ( 1ULL << 40 ) + i : i % 8;
 CHECK( check( z ) < 20000 * 5 );
 }

// the parse of LZ77 in linear time: the same lengths as the parse of the
// notes with no window, and the round trip, on random texts of small and
// large alphabets, on runs (copies that overlap their source), on texts of
// one symbol, on the empty text and on one symbol
ADS_TEST( "lz77-linear" )
{
 std::mt19937 g( 17 );
 auto check = [ & ]( const std::string & s ) {
  const auto L = lz77ParseLinear( s ) , B = lz77Parse( s );
  CHECK( lz77Decode( L ) == s );
  CHECK( L.size() == B.size() );
  for( std::size_t k = 0 ; k < std::min( L.size() , B.size() ) ; ++k )
   CHECK( L[ k ].length == B[ k ].length && L[ k ].next == B[ k ].next );
  };
 for( const int sigma : { 1 , 2 , 4 , 26 } )
  for( const std::size_t n : { 1 , 2 , 3 , 10 , 200 , 3000 } ) {
   std::string s;
   for( std::size_t i = 0 ; i < n ; ++i ) s += char( 'a' + g() % sigma );
   check( s );
   }
 check( "" );
 check( "abababababababababab" );
 check( "aaaaaaaaaaaaaaaaaaaaaaaaab" );
 check( "abracadabra abracadabra abracadabra" );
 const auto P = lz77ParseLinear( "aaaa" );
 CHECK( P.size() == 2 && P[ 1 ].distance == 1 && P[ 1 ].length == 3 &&
        ! P[ 1 ].next );
 }

// Stream VByte: the round trip and the size of every count of integers
// from 0 to 41 (the groups of 4 and the integers one at a time at the end)
// on the bounds of the lengths 1 to 4, and of 10^5 random ones of random
// lengths, against the variable-byte code; and the malformed inputs
ADS_TEST( "stream-vbyte" )
{
 std::mt19937_64 g( 11 );
 const std::uint32_t edge[] = { 0 , 1 , 255 , 256 , 65535 , 65536 ,
                                16777215 , 16777216 , 4294967295u };
 auto bytes = []( std::uint32_t v ) {
  return( std::size_t( 1 + ( v > 0xFF ) + ( v > 0xFFFF ) +
                       ( v > 0xFFFFFF ) ) );
  };
 auto check = [ & ]( const std::vector< std::uint32_t > & x ) {
  const auto c = streamVByteEncode( x );
  std::size_t size = ( x.size() + 3 ) / 4;
  for( const auto v : x ) size += bytes( v );
  CHECK( c.size() == size );
  CHECK( streamVByteDecode( c , x.size() ) == x );
  };
 for( std::size_t n = 0 ; n < 42 ; ++n ) {
  std::vector< std::uint32_t > x( n );
  for( auto & v : x ) v = edge[ g() % 9 ];
  check( x );
  }
 std::vector< std::uint32_t > x( 100000 );
 for( auto & v : x ) v = std::uint32_t( g() ) >> ( 8 * ( g() % 4 ) );
 check( x );
 const auto c = streamVByteEncode( x );
 CHECK_THROWS( ( void )streamVByteDecode( std::span( c ).first( c.size() -
                                                                1 ) ,
                                          x.size() ) ,
               contract_violation );
 CHECK_THROWS( ( void )streamVByteDecode( c , x.size() - 1 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File engineered_succinct.cpp --------------------*/
/*--------------------------------------------------------------------------*/
