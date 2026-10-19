/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_succinct.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered succinct structures and codes, beyond the book:
 * the wavelet matrix, against a scan of the sequence, at the level 3 of the
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

#include "ADSTest.h"

#include "ADS/succinct/WaveletMatrix.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

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
