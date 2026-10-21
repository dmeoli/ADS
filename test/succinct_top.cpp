/*--------------------------------------------------------------------------*/
/*------------------------- File succinct_top.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the paths of the rank and select directory that a vector of
 * practical size never takes: the top level of the counts, which starts at
 * \f$ 2^{40} \f$ bits and here at \f$ 2^{10} \f$ (ADS_RANKSELECT_TOP), and
 * the sparse groups of select, which span \f$ 2^{16} \f$ bits or more, and
 * the top level of the compact directory, at \f$ 2^{32} \f$ bits and here
 * at \f$ 2^{13} \f$ (ADS_COMPACT_TOP), and the copy of the words as
 * they are counted, from \f$ 2^{23} \f$ bits and here from 64
 * (ADS_RANKSELECT_FUSE), and the binary packing a lane at a time, as with
 * no SSE2 (ADS_BP_SCALAR); rank and select against a scan, and Elias-Fano
 * and the balanced parentheses on top of them, at the level 3 of the
 * contracts.
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

#define ADS_RANKSELECT_TOP 10
#define ADS_COMPACT_TOP 13
#define ADS_RANKSELECT_FUSE 64
#define ADS_BP_SCALAR 1

#include "ADSTest.h"

#include "ADS/compression/BinaryPacking.h"

#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// every rank and every select of both bits of R against a scan
template< class D = RankSelect >
void checkAll( const std::vector< bool > & R )
{
 BitVector b;
 for( const bool x : R ) b.push_back( x );
 const D S( b );
 std::size_t ones = 0 , zeros = 0;
 for( std::size_t i = 0 ; i <= R.size() ; ++i ) {
  CHECK( S.rank1( i ) == ones && S.rank0( i ) == zeros );
  if( i == R.size() ) break;
  if( R[ i ] ) CHECK( S.select1( ++ones ) == i );
  else CHECK( S.select0( ++zeros ) == i );
  }
 CHECK( S.ones() == ones && S.zeros() == zeros );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the binary packing a lane at a time, against the integers: the round
// trip of sequences of many sizes around the blocks of 128 and the groups
// of 16 blocks, with widths from 0 to 32 bits, of random and of sorted
// sequences (up to near 2^32); the size, 16 bytes per group and 16 b bytes
// per block of width b; the patched code (PFor) on the same sequences,
// and on blocks with a few large integers, where it takes less than a
// third; the empty sequence; the codes cut short
ADS_TEST( "binary-packing-scalar" )
{
 std::mt19937_64 g( 29 );
 for( const std::size_t n : { 0 , 1 , 4 , 127 , 128 , 129 , 2047 , 2048 ,
                              2049 , 5000 } )
  for( int t = 0 ; t < 6 ; ++t ) {
   std::vector< std::uint32_t > x( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const int b = t == 5 ? int( ( i / 128 ) % 33 ) : t * 6 + 2;
    x[ i ] = b == 0 ? 0
                    : std::uint32_t( g() ) >> ( 32 - std::min( b , 32 ) );
    }
   const auto c = binaryPack( x );
   std::size_t bytes = 16 * ( ( n + 2047 ) / 2048 );
   for( std::size_t k = 0 ; 128 * k < n ; ++k ) {
    std::uint32_t o = 0;
    for( std::size_t i = 128 * k ; i < std::min( n , 128 * k + 128 ) ; ++i )
     o |= x[ i ];
    bytes += 16 * std::size_t( std::bit_width( o ) );
    }
   CHECK( c.size() == bytes && binaryUnpack( c , n ) == x );
   CHECK( patchedUnpack( patchedPack( x ) , n ) == x );
   CHECK( patchedPack( x ).size() <= bytes + 2 * ( ( n + 127 ) / 128 ) + 8 );
   std::sort( x.begin() , x.end() );
   CHECK( binaryUnpack( binaryPack( x , true ) , n , true ) == x );
   CHECK( patchedUnpack( patchedPack( x , true ) , n , true ) == x );
   }
 // a few large integers in each block: the patched code writes them apart
 std::vector< std::uint32_t > out( 5000 );
 for( std::size_t i = 0 ; i < out.size() ; ++i )
  out[ i ] = i % 50 == 7 ? std::uint32_t( g() ) : std::uint32_t( g() % 16 );
 const auto pc = patchedPack( out );
 CHECK( patchedUnpack( pc , out.size() ) == out &&
        pc.size() < binaryPack( out ).size() / 3 );
 auto pt = pc;
 pt.pop_back();
 CHECK_THROWS( ( void )patchedUnpack( pt , out.size() ) ,
               contract_violation );
 std::vector< std::uint32_t > big( 3000 );
 for( std::size_t i = 0 ; i < big.size() ; ++i )
  big[ i ] = 0xFFFFF000u + std::uint32_t( i );
 CHECK( binaryUnpack( binaryPack( big , true ) , big.size() , true ) == big );
 CHECK( binaryPack( big , true ).size() < binaryPack( big ).size() / 4 );
 std::vector< std::uint32_t > y( 300 , 77 );
 auto c = binaryPack( y );
 c.pop_back();
 CHECK_THROWS( ( void )binaryUnpack( c , y.size() ) , contract_violation );
 }

// RankSelect and CompactRankSelect from the words, copied as they are
// counted (from 64 bits here), against those from a BitVector of the same
// words, with the top counts every 2^10 and 2^13 bits: the bits, every
// rank and select, on sizes around the words, the blocks and the top
// regions, with 1s in the words past the size
ADS_TEST( "rank-select-words-top" )
{
 std::mt19937_64 g( 13 );
 for( const std::size_t n : { 0 , 1 , 63 , 64 , 65 , 255 , 256 , 257 , 1023 ,
                              1024 , 1025 , 3000 , 4096 , 8191 , 8192 ,
                              8193 , 70001 } )
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
   const CompactRankSelect X( std::span< const Word >( w ) , n );
   const CompactRankSelect Y( BitVector( std::span< const Word >( w ) , n ) );
   ok = X.size() == n && X.ones() == Y.ones() && X.bits() == Y.bits() &&
        X.directoryBits() == Y.directoryBits() && X.ones() == A.ones();
   for( std::size_t i = 0 ; ok && i <= n ; ++i )
    ok = X.rank1( i ) == A.rank1( i );
   for( std::size_t k = 1 ; ok && k <= X.ones() ; ++k )
    ok = X.select1( k ) == A.select1( k );
   for( std::size_t k = 1 ; ok && k <= X.zeros() ; ++k )
    ok = X.select0( k ) == A.select0( k );
   CHECK( ok );
   }
 }

// rank and select with a top count every 2^10 bits, on random vectors of
// many densities and sizes around the multiples of 2^10; and the groups
// of select that span 2^16 bits or more: dense runs of 1s (and of 0s)
// separated by long empty stretches, so that the groups across them are
// sparse, and a vector whose last group is sparse; a vector built from
// its words; the directories with one select or none
ADS_TEST( "rank-select-top" )
{
 std::mt19937 g( 3 );
 for( const std::size_t n : { 1023 , 1024 , 1025 , 4096 , 20000 } )
  for( const double p : { 0.0 , 0.01 , 0.5 , 0.99 , 1.0 } ) {
   std::vector< bool > R( n );
   std::bernoulli_distribution B( p );
   for( std::size_t i = 0 ; i < n ; ++i ) R[ i ] = B( g );
   checkAll( R );
   }
 for( const std::size_t n : { 8191 , 8192 , 8193 , 40000 } ) {   // compact
  std::vector< bool > R( n );
  for( std::size_t i = 0 ; i < n ; ++i ) R[ i ] = g() % 3 == 0;
  checkAll< CompactRankSelect >( R );
  }
 std::vector< bool > C;
 for( int run = 0 ; run < 6 ; ++run ) {
  for( int i = 0 ; i < 20000 ; ++i ) C.push_back( g() % 2 );
  C.insert( C.end() , 100000 , run % 2 == 0 );   // no 0s, then no 1s
  }
 checkAll( C );
 std::vector< bool > T( 5000 , true );
 T.insert( T.end() , 200000 , false );
 T.push_back( true );
 checkAll( T );
 // a vector from its words, the bits past the size cleared
 std::vector< Word > W( 9 );
 for( auto & w : W ) w = g() | ( Word( g() ) << 32 );
 for( const std::size_t n : { 0 , 1 , 63 , 64 , 65 , 575 , 576 } ) {
  const BitVector v( W , n );
  BitVector u;
  for( std::size_t i = 0 ; i < n ; ++i )
   u.push_back( ( W[ i / 64 ] >> ( i % 64 ) ) & 1 );
  CHECK( v == u && v.size() == n );
  }
 CHECK_THROWS( BitVector( std::span< const Word >( W ).first( 1 ) , 65 ) ,
               contract_violation );
 // the directories with fewer selects: the same ranks and the selects
 // asked for, in fewer bits, and the others refused
 BitVector b;
 for( int i = 0 ; i < 30000 ; ++i ) b.push_back( g() % 3 == 0 );
 const RankSelect A( b ) , O( b , RankSelect::Selects::ONES ) ,
                  Z( b , RankSelect::Selects::ZEROS ) ,
                  N( b , RankSelect::Selects::NONE );
 for( std::size_t i = 0 ; i <= b.size() ; i += 7 )
  CHECK( N.rank1( i ) == A.rank1( i ) && O.rank0( i ) == A.rank0( i ) );
 for( std::size_t k = 1 ; k <= A.ones() ; k += 5 )
  CHECK( O.select1( k ) == A.select1( k ) );
 for( std::size_t k = 1 ; k <= A.zeros() ; k += 5 )
  CHECK( Z.select0( k ) == A.select0( k ) );
 CHECK( N.directoryBits() < O.directoryBits() &&
        O.directoryBits() < A.directoryBits() &&
        Z.directoryBits() < A.directoryBits() );
 CHECK_THROWS( ( void )N.select1( 1 ) , contract_violation );
 CHECK_THROWS( ( void )O.select0( 1 ) , contract_violation );
 CHECK_THROWS( ( void )Z.select1( 1 ) , contract_violation );
 }

// Elias-Fano and the balanced parentheses on the directory with the top
// level: their queries against the definitions
ADS_TEST( "succinct-top" )
{
 std::mt19937_64 g( 5 );
 std::vector< std::uint64_t > x( 5000 );
 for( auto & v : x ) v = g() % 300000;
 std::sort( x.begin() , x.end() );
 const EliasFano E( x , 300000 );
 for( std::size_t i = 0 ; i < x.size() ; ++i )
  CHECK( E.access( i ) == x[ i ] );
 for( int q = 0 ; q < 2000 ; ++q ) {
  const std::uint64_t v = g() % 300001;
  CHECK( E.nextGEQ( v ) == std::size_t( std::lower_bound( x.begin() ,
                                                          x.end() , v ) -
                                        x.begin() ) );
  }
 BitVector b;                               // ( ( ) ( ( ) ) ... )
 std::vector< std::size_t > S , match( 6000 );
 for( std::size_t i = 0 ; i < 6000 ; ++i ) {
  const bool open = S.empty() ||
                    ( ( g() % 2 ) && ( S.size() + i + 1 < 6000 ) );
  b.push_back( open );
  if( open ) S.push_back( i );
  else {
   match[ i ] = S.back();
   match[ S.back() ] = i;
   S.pop_back();
   }
  }
 const BalancedParentheses P( b );
 for( std::size_t i = 0 ; i < 6000 ; ++i )
  CHECK( b[ i ] ? P.findClose( i ) == match[ i ] :
                  P.findOpen( i ) == match[ i ] );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File succinct_top.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
