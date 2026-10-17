/*--------------------------------------------------------------------------*/
/*--------------------------- File sorting.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the sorts beyond the book: each one sorts every permutation of
 * up to 7 items, then random vectors, sorted, reversed, sawtooth and organ
 * pipe ones, vectors of few distinct keys and of all equal keys, of many
 * sizes, against std::sort; a stable sort is also checked on pairs ( key ,
 * place ), whose places must stay increasing among equal keys. The tests
 * are compiled at the level 3 of the contracts, so that each sort also
 * audits its result.
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

#include "ADS/sequence/List.h"
#include "ADS/sort/CycleSort.h"
#include "ADS/sort/DidacticSorts.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/ExchangeSorts.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/PatienceSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/Smoothsort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/SortingNetworks.h"
#include "ADS/sort/StrandSort.h"
#include "ADS/sort/TreeSort.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <cstddef>
#include <functional>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the inputs of the battery: every permutation of up to 7 items, and
/// vectors of the given sizes in the classical patterns
std::vector< Ints > inputs( std::size_t maxN )
{
 std::vector< Ints > I;
 for( int n = 0 ; n <= 7 ; ++n ) {
  Ints p( n );
  std::iota( p.begin() , p.end() , 0 );
  do I.push_back( p ); while( std::next_permutation( p.begin() , p.end() ) );
  }
 std::mt19937 g( 7 );
 for( std::size_t n : { 8u , 31u , 100u , 257u , 1000u , 4096u } ) {
  if( n > maxN )
   break;
  Ints r( n ) , s( n ) , v( n ) , w( n ) , o( n ) , f( n ) , e( n , 5 );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   r[ i ] = int( g() % ( 2 * n ) );          // random
   s[ i ] = int( i );                        // sorted
   v[ i ] = int( n - i );                    // reversed
   w[ i ] = int( i % 17 );                   // sawtooth
   o[ i ] = int( std::min( i , n - 1 - i ) );   // organ pipe
   f[ i ] = int( g() % 4 );                  // few distinct keys
   }
  for( auto * x : { &r , &s , &v , &w , &o , &f , &e } )
   I.push_back( *x );
  }
 return( I );
 }

/// the battery on sort( A ) up to the size maxN; with stable, also the
/// order of the equal keys, on pairs compared by their keys only
template< class S >
void battery( S sort , std::size_t maxN , bool stable )
{
 for( const auto & x : inputs( maxN ) ) {
  Ints A = x , B = x;
  sort( A , std::less<>() );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  if( ! stable )
   continue;
  using P = std::pair< int , std::size_t >;
  std::vector< P > Q;
  for( std::size_t i = 0 ; i < x.size() ; ++i )
   Q.emplace_back( x[ i ] % 5 , i );
  std::vector< P > R = Q;
  sort( Q , []( const P & a , const P & b ) { return( a.first < b.first ); } );
  std::stable_sort( R.begin() , R.end() , []( const P & a , const P & b ) {
   return( a.first < b.first ); } );
  CHECK( Q == R );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the sorts by exchanges, all stable but the comb sort, and the rounds of
// the odd-even transposition at most n
ADS_TEST( "exchange-sorts" )
{
 battery( []( auto & A , auto less ) { bubbleSort( A , less ); } , 1000 ,
          true );
 battery( []( auto & A , auto less ) { cocktailShakerSort( A , less ); } ,
          1000 , true );
 battery( []( auto & A , auto less ) { oddEvenSort( A , less ); } , 1000 ,
          true );
 battery( []( auto & A , auto less ) { combSort( A , less ); } , 4096 ,
          false );
 battery( []( auto & A , auto less ) { gnomeSort( A , less ); } , 1000 ,
          true );
 Ints A( 50 );
 std::iota( A.rbegin() , A.rend() , 0 );
 std::size_t r = 0;
 oddEvenSort( A , std::less<>() , &r );
 CHECK( r <= 51 );
 }

// the cycle sort with at most n writes, n - ( fixed points ) on distinct
// keys, and the pancake sort with at most 2 n - 3 flips
ADS_TEST( "cycle-pancake" )
{
 battery( []( auto & A , auto less ) { cycleSort( A , less ); } , 1000 ,
          false );
 battery( []( auto & A , auto less ) { pancakeSort( A , less ); } , 1000 ,
          false );
 Ints A = { 3 , 0 , 1 , 2 , 4 };                 // one cycle of 4 , 4 fixed
 CHECK( cycleSort( A ) == 4 && A == Ints( { 0 , 1 , 2 , 3 , 4 } ) );
 Ints S = { 0 , 1 , 2 };
 CHECK( cycleSort( S ) == 0 && pancakeSort( S ) == 0 );
 Ints P = { 2 , 0 , 3 , 1 };
 CHECK( pancakeSort( P ) <= 5 && P == Ints( { 0 , 1 , 2 , 3 } ) );
 }

// the tree sort on a Red-Black tree and on an unbalanced tree, the repeated
// items kept; the patience sort with as many piles as a longest increasing
// subsequence
ADS_TEST( "tree-patience" )
{
 battery( []( auto & A , auto ) {
  if constexpr( std::is_same_v< typename std::decay_t< decltype( A ) >::
                                value_type , int > )
   treeSort( A );
  else
   std::stable_sort( A.begin() , A.end() , []( const auto & a ,
                                               const auto & b ) {
    return( a.first < b.first ); } );
  } , 4096 , false );
 Ints U = { 5 , 1 , 5 , 3 , 1 };
 treeSort( U , false );
 CHECK( U == Ints( { 1 , 1 , 3 , 5 , 5 } ) );
 battery( []( auto & A , auto less ) { patienceSort( A , less ); } , 4096 ,
          false );
 Ints L = { 3 , 1 , 4 , 1 , 5 , 9 , 2 , 6 };     // 1 4 5 9 or 1 4 5 6
 std::size_t piles = 0;
 patienceSort( L , std::less<>() , &piles );
 CHECK( piles == 4 );
 Ints D( 100 );
 std::iota( D.rbegin() , D.rend() , 0 );
 patienceSort( D , std::less<>() , &piles );
 CHECK( piles == 1 );
 }

// smoothsort, and its O( n ) comparisons on a sorted vector against the
// n log n of a random one
ADS_TEST( "smoothsort" )
{
 battery( []( auto & A , auto less ) { smoothsort( A , less ); } , 4096 ,
          false );
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract();
  return( a < b ); };
 Ints S( 4096 );
 std::iota( S.begin() , S.end() , 0 );
 smoothsort( S , count );
 const long sorted = c;
 std::shuffle( S.begin() , S.end() , std::mt19937( 8 ) );
 c = 0;
 smoothsort( S , count );
 CHECK( sorted < 4 * 4096 && c > 5 * sorted );
 }

// the strand sort of a list, stable, one strand on a sorted list and n on
// a decreasing one
ADS_TEST( "strand-sort" )
{
 for( const auto & x : inputs( 1000 ) ) {
  List< int > L;
  for( int v : x ) L.insert( L.next( L.tail() ) , v );
  strandSort( L );
  Ints a , b = x;
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   a.push_back( L.read( p ) );
  std::sort( b.begin() , b.end() );
  CHECK( a == b );
  }
 List< int > S = { 1 , 2 , 3 } , D = { 3 , 2 , 1 };
 std::size_t k = 0;
 strandSort( S , std::less<>() , &k );
 CHECK( k == 1 );
 strandSort( D , std::less<>() , &k );
 CHECK( k == 3 );
 }

// the sorting networks: by the 0-1 principle, every vector of 0 and 1 of
// n items sorted, for n up to 12 (merge exchange) and n = 16 (bitonic);
// random vectors of other sizes; the comparators within n
ADS_TEST( "sorting-networks" )
{
 for( std::size_t n = 0 ; n <= 12 ; ++n ) {
  const auto N = mergeExchangeNetwork( n );
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   Ints A( n );
   for( std::size_t i = 0 ; i < n ; ++i ) A[ i ] = ( m >> i ) & 1;
   applyNetwork( A , N );
   CHECK( std::is_sorted( A.begin() , A.end() ) );
   }
  }
 const auto B = bitonicNetwork( 16 );
 for( unsigned m = 0 ; m < ( 1u << 16 ) ; ++m ) {
  Ints A( 16 );
  for( std::size_t i = 0 ; i < 16 ; ++i ) A[ i ] = ( m >> i ) & 1;
  applyNetwork( A , B );
  CHECK( std::is_sorted( A.begin() , A.end() ) );
  }
 CHECK( B.size() == 80 );           // ( n / 4 ) log n ( log n + 1 )
 CHECK_THROWS( bitonicNetwork( 12 ) , contract_violation );
 std::mt19937 g( 9 );
 for( std::size_t n : { 13u , 100u , 1000u } ) {
  Ints A( n );
  for( auto & x : A ) x = int( g() % 50 );
  Ints C = A;
  applyNetwork( A , mergeExchangeNetwork( n ) );
  std::sort( C.begin() , C.end() );
  CHECK( A == C );
  }
 Ints Z = { 1 , 0 };
 CHECK_THROWS( applyNetwork( Z , { Comparator( 0 , 2 ) } ) ,
               contract_violation );
 }

// the slow sorts on small vectors, the bogo sort only up to 10 items
ADS_TEST( "didactic-sorts" )
{
 battery( []( auto & A , auto less ) { stoogeSort( A , less ); } , 100 ,
          false );
 battery( []( auto & A , auto less ) {
  if( A.size() <= 30 ) slowSort( A , less );
  else std::sort( A.begin() , A.end() , less ); } , 100 , false );
 std::mt19937 g( 10 );
 Ints A = { 3 , 1 , 2 , 0 , 4 };
 CHECK( bogoSort( A , g ) >= 0 && A == Ints( { 0 , 1 , 2 , 3 , 4 } ) );
 Ints L( 11 );
 CHECK_THROWS( bogoSort( L , g ) , contract_violation );
 }

// the engineered quicksorts on the battery, and on inputs against the
// classical quicksort: a killer of the median of three would make them
// quadratic, but the depth bound of introsort and pdqsort keeps them
// O( n log n ); pdqsort takes O( n ) comparisons on a sorted vector
ADS_TEST( "engineered-quicksorts" )
{
 battery( []( auto & A , auto less ) { introSort( A , less ); } , 4096 ,
          false );
 battery( []( auto & A , auto less ) { pdqSort( A , less ); } , 4096 ,
          false );
 battery( []( auto & A , auto less ) { dualPivotQuickSort( A , less ); } ,
          4096 , false );
 battery( []( auto & A , auto less ) { threeWayQuickSort( A , less ); } ,
          4096 , false );
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract();
  return( a < b ); };
 Ints S( 100000 );
 std::iota( S.begin() , S.end() , 0 );
 pdqSort( S , count );
 CHECK( c < 3 * 100000 );
 Ints K( 100000 );                               // few distinct keys
 std::mt19937 g( 11 );
 for( auto & x : K ) x = int( g() % 3 );
 for( auto sort : { introSort< int , decltype( count ) > ,
                    pdqSort< int , decltype( count ) > ,
                    dualPivotQuickSort< int , decltype( count ) > ,
                    threeWayQuickSort< int , decltype( count ) > } ) {
  Ints A = K;
  c = 0;
  sort( A , count );
  CHECK( std::is_sorted( A.begin() , A.end() ) && c < 40 * 100000 );
  }
 }

// Timsort and Powersort on the battery, stable; the runs found on vectors
// made of r sorted runs, and O( n ) comparisons on a sorted vector and on
// two runs one after the other
ADS_TEST( "run-sorts" )
{
 battery( []( auto & A , auto less ) { timSort( A , less ); } , 4096 ,
          true );
 battery( []( auto & A , auto less ) { powerSort( A , less ); } , 4096 ,
          true );
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract();
  return( a < b ); };
 Ints S( 100000 );
 std::iota( S.begin() , S.end() , 0 );
 std::size_t runs = 0;
 powerSort( S , count , &runs );
 CHECK( runs == 1 && c < 100000 );
 Ints T( 100000 );                               // two runs interleaved
 for( int i = 0 ; i < 50000 ; ++i ) {
  T[ i ] = 2 * i;
  T[ 50000 + i ] = 2 * i + 1;
  }
 for( auto sort : { timSort< int , decltype( count ) > ,
                    powerSort< int , decltype( count ) > } ) {
  Ints A = T;
  c = 0;
  sort( A , count , &runs );
  CHECK( std::is_sorted( A.begin() , A.end() ) && runs == 2 &&
         c <= 2 * 100000 + 64 );   // n - 1 to find the runs, n - 1 to merge
  }
 Ints U( 100000 );                               // two runs, one after
 for( int i = 0 ; i < 100000 ; ++i ) U[ i ] = ( i + 50000 ) % 100000;
 c = 0;
 timSort( U , count );
 CHECK( std::is_sorted( U.begin() , U.end() ) && c < 100000 + 200 );
 }

// the radix sorts: integers of every width and sign, doubles with
// negatives, the stable sort by a key, the MSD sort of strings against
// std::sort, the bucket sort of numbers in [ 0 , 1 )
template< class T >
void radix_of( unsigned seed )
{
 std::mt19937_64 g( seed );
 for( std::size_t n : { 0u , 1u , 7u , 100u , 5000u } ) {
  std::vector< T > A( n );
  for( auto & x : A ) x = T( g() );
  if( n > 50 ) { A[ 3 ] = std::numeric_limits< T >::min();
                 A[ 4 ] = std::numeric_limits< T >::max(); A[ 5 ] = T( 0 ); }
  auto B = A , C = A;
  radixSort( A );
  americanFlagSort( C );
  std::sort( B.begin() , B.end() );
  CHECK( A == B && C == B );
  }
 }

ADS_TEST( "radix-sorts" )
{
 radix_of< std::uint8_t >( 1 );
 radix_of< std::int8_t >( 2 );
 radix_of< std::int16_t >( 3 );
 radix_of< std::uint32_t >( 4 );
 radix_of< int >( 5 );
 radix_of< long long >( 6 );
 radix_of< std::uint64_t >( 7 );
 std::mt19937 g( 12 );
 std::uniform_real_distribution< double > u( -1e6 , 1e6 );
 std::vector< double > D( 5000 );
 for( auto & x : D ) x = u( g );
 D[ 0 ] = -0.0; D[ 1 ] = 0.0; D[ 2 ] = -1e300; D[ 3 ] = 1e-300;
 auto E = D;
 radixSort( D );
 std::sort( E.begin() , E.end() );
 CHECK( std::is_sorted( D.begin() , D.end() ) && D.size() == E.size() );
 std::vector< float > F = { 3.5f , -2.0f , 0.0f , -0.5f , 7.0f };
 radixSort( F );
 CHECK( F == std::vector< float >( { -2.0f , -0.5f , 0.0f , 3.5f , 7.0f } ) );
 using P = std::pair< unsigned , int >;
 std::vector< P > R;
 for( int i = 0 ; i < 3000 ; ++i ) R.emplace_back( unsigned( g() % 50 ) , i );
 auto Q = R;
 radixSortBy( R , []( const P & p ) { return( p.first ); } );
 std::stable_sort( Q.begin() , Q.end() , []( const P & a , const P & b ) {
  return( a.first < b.first ); } );
 CHECK( R == Q );
 std::vector< std::string > W;
 for( int i = 0 ; i < 3000 ; ++i ) {
  std::string s;
  for( int k = int( g() % 12 ) ; k > 0 ; --k ) s += char( 'a' + g() % 3 );
  W.push_back( s );
  }
 W.push_back( "" );
 W.push_back( std::string( "\xff\x01" ) );
 auto V = W , M = W;
 msdRadixSort( W );
 multikeyQuickSort( M );
 std::sort( V.begin() , V.end() );
 CHECK( W == V && M == V );
 std::vector< double > U( 4000 );
 std::uniform_real_distribution< double > z( 0 , 1 );
 for( auto & x : U ) x = z( g );
 auto Z = U;
 bucketSort( U );
 std::sort( Z.begin() , Z.end() );
 CHECK( U == Z );
 std::vector< double > bad = { 0.5 , 1.0 };
 CHECK_THROWS( bucketSort( bad ) , contract_violation );
 }

// the sort that chooses: numbers short and long, strings, records with an
// order of their own, stable or not, all against the standard library
ADS_TEST( "dispatch" )
{
 battery( []( auto & A , auto less ) { sort( A , less ); } , 4096 , false );
 battery( []( auto & A , auto less ) { stableSort( A , less ); } , 4096 ,
          true );
 std::mt19937 g( 13 );
 for( std::size_t n : { 10u , 600u , 20000u } ) {
  Ints A( n );
  for( auto & x : A ) x = int( g() % 2000001 ) - 1000000;
  Ints B = A , C = A;
  sort( A );
  stableSort( C );
  std::sort( B.begin() , B.end() );
  CHECK( A == B && C == B );
  std::vector< std::string > S( n );
  for( auto & s : S ) s = std::to_string( g() % 1000 );
  auto T = S , U = S;
  sort( S );
  stableSort( U );
  std::sort( T.begin() , T.end() );
  CHECK( S == T && U == T );
  }
 Ints R = { 3 , 1 , 2 };
 sort( R , std::greater<>() );                   // not the natural order
 CHECK( R == Ints( { 3 , 2 , 1 } ) );
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
/*------------------------- End File sorting.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
