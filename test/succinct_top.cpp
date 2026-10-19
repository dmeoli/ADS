/*--------------------------------------------------------------------------*/
/*------------------------- File succinct_top.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the paths of the rank and select directory that a vector of
 * practical size never takes: the top level of the counts, which starts at
 * \f$ 2^{40} \f$ bits and here at \f$ 2^{10} \f$ (ADS_RANKSELECT_TOP), and
 * the sparse groups of select, which span \f$ 2^{16} \f$ bits or more; rank
 * and select against a scan, and Elias-Fano and the balanced parentheses on
 * top of them, at the level 3 of the contracts.
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

#include "ADSTest.h"

#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// every rank and every select of both bits of R against a scan
void checkAll( const std::vector< bool > & R )
{
 BitVector b;
 for( const bool x : R ) b.push_back( x );
 const RankSelect S( b );
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

// rank and select with a top count every 2^10 bits, on random vectors of
// many densities and sizes around the multiples of 2^10; and the groups
// of select that span 2^16 bits or more: dense runs of 1s (and of 0s)
// separated by long empty stretches, so that the groups across them are
// sparse, and a vector whose last group is sparse; the directories with
// one select or none
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
