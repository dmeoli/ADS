/*--------------------------------------------------------------------------*/
/*---------------------------- File portable.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations whose code changed when the branches on the processor,
 * the compiler and the system were replaced by portable code: the program
 * that portable.py builds on two revisions of the headers, so that each
 * operation is compared with itself before and after.
 *
 *   portable <case> <n>     n operations of the case, under callgrind
 *   portable time           the timings of every case, two runs of each
 *
 * The cases are select (`selectInWord()`, the select in a word), bselect
 * (`select()` of FastBitwise.h), prefix (`maybe()` of PrefixFilter), recsplit
 * (a query of RecSplit), hash (`hashBytes()` of 32 bytes), mulmod
 * (`mulMod()` with a modulus of 62 bits), montgomery (`powMod()` with an
 * exponent of 64 bits), bigmul and bigdiv (a product and a quotient of
 * BigInteger of 16 words by 8), merge (`replaceTop()` of a FlatLoserTree of
 * 64 sources of long long), cas16 (`insert()` of a new pair of 64-bit words
 * into a ConcurrentFlatTable), notify (`notify()` of an EventCount with no
 * waiter), wait (`prepareWait()` and `cancelWait()`), suffix (the suffix
 * array of a text of 4096 bytes, one byte being an operation), rank1
 * (`rank1()` of RankSelect, the cut of the last word), linerank (the
 * scalar `lineRank()` of a line of 512 bits), tans (`tansDecompress()` of
 * a code of 1000 bytes, a byte an operation), philox (a draw of
 * Philox4x64, its 10 rounds unrolled) and sell (the product of a
 * SellMatrix of 1000 rows of 8 entries by a vector, a row an operation).
 * With
 * `time`, each case prints its line of Bench.h under the label
 * `PORTABLE_TAG` (old or new, set by portable.py), twice, the second time
 * as "A/A"; no timing is taken by the counts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

#include "Bench.h"

#include "ADS/combinatorial/FastBitwise.h"
#include "ADS/compression/Tans.h"
#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/concurrent/EventCount.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/numeric/SellMatrix.h"
#include "ADS/priority/MultiwayMerge.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/string/SuffixSorting.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <utility>
#include <vector>

#ifndef PORTABLE_TAG
 #define PORTABLE_TAG "new"
#endif

namespace
{

using U64 = std::uint64_t;

constexpr std::size_t M = 4096;  ///< the inputs, used in a cycle

/// a sink, so that no operation is optimized away
volatile U64 sink = 0;

/// the random words of the inputs, and the ranks of their 1s
struct Words
{
 std::vector< U64 > w;
 std::vector< int > r;
 Words( void ) {
  std::mt19937_64 g( 7 );
  while( w.size() < M ) {
   const U64 x = g() & g();  // a density of 1/4
   if( x == 0 )
    continue;
   w.push_back( x );
   r.push_back( int( g() % U64( std::popcount( x ) ) ) );
   }
  }
 };

/// one run of a case, n operations; the checksum of their results
U64 run( const std::string & c , std::size_t n )
{
 static const Words W;
 U64 s = 0;
 if( c == "select" )
  for( std::size_t i = 0 ; i < n ; ++i )
   s += U64( ADS::selectInWord( W.w[ i % M ] , W.r[ i % M ] ) );
 else if( c == "bselect" )
  for( std::size_t i = 0 ; i < n ; ++i )
   s += U64( ADS::select( W.w[ i % M ] , W.r[ i % M ] ) );
 else if( c == "prefix" ) {
  static ADS::PrefixFilter< U64 > F = [] {
   ADS::PrefixFilter< U64 > f( 1 << 16 );
   for( U64 k = 0 ; k < ( 1 << 15 ) ; ++k )
    f.insert( 2 * k );
   return( f );
   }();
  for( std::size_t i = 0 ; i < n ; ++i )
   s += F.maybe( W.w[ i % M ] % ( 1 << 16 ) ) ? 1 : 0;
  }
 else if( c == "recsplit" ) {
  static const std::vector< U64 > K = ADS::bench::keys( M , 7 );  // distinct
  static const ADS::RecSplit< U64 > R( K );
  for( std::size_t i = 0 ; i < n ; ++i )
   s += U64( R( K[ i % M ] ) );
  }
 else if( c == "hash" ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   s += ADS::hashBytes( W.w.data() + ( i % ( M - 4 ) ) , 32 , s );
  }
 else if( c == "mulmod" ) {
  const U64 m = ( U64( 1 ) << 62 ) + 57;
  for( std::size_t i = 0 ; i < n ; ++i )
   s += ADS::mulMod( W.w[ i % M ] % m , W.w[ ( i + 1 ) % M ] % m , m );
  }
 else if( c == "montgomery" ) {
  const U64 m = ( U64( 1 ) << 62 ) + 57;
  for( std::size_t i = 0 ; i < n ; ++i )
   s += ADS::powMod( W.w[ i % M ] , W.w[ ( i + 1 ) % M ] , m );
  }
 else if( ( c == "bigmul" ) || ( c == "bigdiv" ) ) {
  std::vector< ADS::BigInteger > A , B;
  for( std::size_t i = 0 ; i < 64 ; ++i ) {
   ADS::BigInteger a( 1 ) , b( 1 );
   auto digit = [ & ]( std::size_t j ) {
    return( ADS::BigInteger( ( long long )( W.w[ j % M ] >> 2 ) ) );
    };
   for( std::size_t j = 0 ; j < 16 ; ++j )
    a = a * digit( 16 * i + j );
   for( std::size_t j = 0 ; j < 8 ; ++j )
    b = b * digit( 8 * i + j + 7 );
   A.push_back( a );
   B.push_back( b );
   }
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const ADS::BigInteger &a = A[ i % 64 ] , &b = B[ ( i + 1 ) % 64 ];
   const ADS::BigInteger r = ( c == "bigmul" ) ? a * b : a / b;
   s += U64( r.sign() != 0 );
   }
  }
 else if( c == "merge" ) {
  constexpr std::size_t k = 64;
  ADS::FlatLoserTree< long long > T( k );
  for( std::size_t j = 0 ; j < k ; ++j )
   T.start( j , ( long long )( W.w[ j ] >> 40 ) );
  T.build();
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const long long x = T.top();
   s += U64( T.winner() );
   T.replaceTop( x + ( long long )( W.w[ i % M ] >> 56 ) );
   }
  }
 else if( c == "cas16" ) {
  ADS::ConcurrentFlatTable< U64 , U64 > T( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   s += T.insert( W.w[ i % M ] + U64( i / M ) * 0x9E3779B97F4A7C15ULL , i ) ? 1
									   : 0;
  }
 else if( c == "notify" ) {
  static ADS::EventCount E;
  for( std::size_t i = 0 ; i < n ; ++i )
   E.notify();
  s += E.waiters();
  }
 else if( c == "wait" ) {
  static ADS::EventCount E;
  for( std::size_t i = 0 ; i < n ; ++i )
   E.cancelWait( E.prepareWait() );
  s += E.waiters();
  }
 else if( c == "suffix" ) {
  std::string T( n , 'a' );
  std::mt19937_64 g( 11 );
  for( auto & x : T )
   x = char( 'a' + g() % 4 );
  const auto SA = ADS::suffixArray< std::uint32_t >( T );
  s += SA.empty() ? 0 : SA[ SA.size() / 2 ];
  }
 else if( c == "rank1" ) {
  static const ADS::RankSelect R = [] {
   ADS::BitVector b( 64 * M );
   for( std::size_t i = 0 ; i < M ; ++i )
    for( int j = 0 ; j < 64 ; ++j )
     if( ( W.w[ i ] >> j ) & 1 )
      b.set( 64 * i + std::size_t( j ) );
   return( ADS::RankSelect( std::move( b ) ) );
   }();
  for( std::size_t i = 0 ; i < n ; ++i )
   s += U64( R.rank1( std::size_t( W.w[ i % M ] % ( 64 * M ) ) ) );
  }
 else if( c == "linerank" ) {
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const U64 * w = W.w.data() + 8 * ( i % ( M / 8 ) );
   s += U64(
     ADS::detail::lineRank< true >( w , unsigned( W.w[ i % M ] % 512 ) ) );
   }
  }
 else if( c == "tans" ) {  // a byte decoded, of a code of 1000
  static const std::vector< std::uint8_t > code = [] {
   std::vector< std::uint8_t > in( 1000 );
   for( std::size_t i = 0 ; i < in.size() ; ++i )
    in[ i ] = std::uint8_t( 'a' + std::countr_zero( W.w[ i % M ] | 256 ) );
   return( ADS::tansCompress( in ) );
   }();
  for( std::size_t i = 0 ; i < n ; i += 1000 )
   s += ADS::tansDecompress( code )[ i % 997 ];
  }
 else if( c == "philox" ) {
  ADS::Philox4x64 g( 7 );
  for( std::size_t i = 0 ; i < n ; ++i )
   s += g();
  }
 else if( c == "sell" ) {  // a row of 8 entries, of 1000 rows
  static const ADS::SellMatrix<> A = [] {
   std::vector< ADS::CsrMatrix::Triplet > E;
   for( std::size_t i = 0 ; i < 1000 ; ++i )
    for( std::size_t k = 0 ; k < 8 ; ++k )
     E.emplace_back( i , std::size_t( W.w[ ( 8 * i + k ) % M ] % M ) ,
		     double( k + 1 ) );
   return( ADS::SellMatrix<>( ADS::CsrMatrix( 1000 , M , std::move( E ) ) ) );
   }();
  static std::vector< double > x( M , 1.0 ) , y( 1000 );
  for( std::size_t i = 0 ; i < n ; i += 1000 ) {
   A.multiply( x.data() , y.data() );
   s += U64( y[ i % 997 ] );
   }
  }
 else {
  std::fprintf( stderr , "unknown case %s\n" , c.c_str() );
  std::exit( 2 );
  }
 return( s );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 if( ( argc == 3 ) && ( std::strcmp( argv[ 1 ] , "time" ) != 0 ) ) {
  sink =
    run( argv[ 1 ] ,
	 std::size_t( std::strtoull( argv[ 2 ] , nullptr , 10 ) ) );
  std::printf( "%llu\n" , ( unsigned long long )sink );
  return( 0 );
  }
 if( ( argc != 2 ) || ( std::strcmp( argv[ 1 ] , "time" ) != 0 ) ) {
  std::fprintf( stderr , "portable <case> <n> | portable time\n" );
  return( 2 );
  }
 const std::size_t n = 1 << 20;
 for( const char * c :
      { "select" , "bselect" , "prefix" , "recsplit" , "hash" , "mulmod" ,
	"montgomery" , "bigmul" , "bigdiv" , "merge" , "cas16" , "notify" ,
	"wait" ,
	"suffix" , "rank1" , "linerank" , "tans" , "philox" , "sell" } )
  for( const std::string who :
       { std::string( PORTABLE_TAG ) , std::string( PORTABLE_TAG ) + " A/A" } )
   if( ! ADS::bench::skip( who ) ) {
    const double t = ADS::bench::median(
      [] { return( 0 ); } , [ c , n ]( int ) { sink = run( c , n ); } , 5 ,
      n );
    ADS::bench::print( "portable" , c , n , who , t );
    }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File portable.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
