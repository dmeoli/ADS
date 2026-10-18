/*--------------------------------------------------------------------------*/
/*-------------------------- File lib_search.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the searches, the rearrangements and the questions on sequences
 * and intervals of LocalOptima.h, MonotoneSearch.h, SortedSearches.h,
 * Rearrangements.h, SequenceQueries.h and ClosestPair.h: each function
 * against an independent oracle (a brute force on small random inputs with
 * a fixed seed, the standard library) and on the edge cases, at the level 3
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

#include "ADS/algorithm/SequenceQueries.h"
#include "ADS/geometry/ClosestPair.h"
#include "ADS/search/LocalOptima.h"
#include "ADS/search/MonotoneSearch.h"
#include "ADS/search/SortedSearches.h"
#include "ADS/sequence/Rearrangements.h"

#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using BT = BinaryTree< int >;

/// a random integer in [ a , b ]
int rnd( std::mt19937 & g , int a , int b )
{
 return( std::uniform_int_distribution< int >( a , b )( g ) );
 }

/// n random integers in [ a , b ]
Ints rndInts( std::mt19937 & g , std::size_t n , int a , int b )
{
 Ints A( n );
 for( int & x : A )
  x = rnd( g , a , b );
 return( A );
 }

/// a random binary tree of n nodes of random values in [ 0 , m ]
BT * rndTree( std::mt19937 & g , int n , int m )
{
 if( n == 0 )
  return( nullptr );
 auto * t = new BT( rnd( g , 0 , m ) );
 const int l = rnd( g , 0 , n - 1 );
 if( BT * u = rndTree( g , l , m ) )
  t->insertLeft( u );
 if( BT * u = rndTree( g , n - 1 - l , m ) )
  t->insertRight( u );
 return( t );
 }

/// the nodes of t
void nodes( BT * t , std::vector< BT * > & N )
{
 if( ! t )
  return;
 N.push_back( t );
 nodes( t->left() , N );
 nodes( t->right() , N );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// findPeak() and localMinimum() on every permutation of up to 7 items and
// on random vectors with repeated items, against their neighbours, with at
// most ceil( log2 n ) comparisons; the strict peak of distinct items; the
// local minimum of random trees against their nodes, the first smaller
// child taken; the empty vector and the nil tree
ADS_TEST( "local-optima" )
{
 auto isPeak = []( const Ints & A , std::size_t r ) {
  return( ( r < A.size() ) && ( ( r == 0 ) || ( A[ r - 1 ] <= A[ r ] ) ) &&
          ( ( r + 1 == A.size() ) || ( A[ r + 1 ] <= A[ r ] ) ) );
  };
 auto isMin = []( const Ints & A , std::size_t r ) {
  return( ( r < A.size() ) && ( ( r == 0 ) || ( A[ r - 1 ] >= A[ r ] ) ) &&
          ( ( r + 1 == A.size() ) || ( A[ r + 1 ] >= A[ r ] ) ) );
  };
 for( std::size_t n = 1 ; n <= 7 ; ++n ) {
  Ints A( n );
  std::iota( A.begin() , A.end() , 0 );
  do {
   long cmp = 0;
   const auto count = [ & ]( int a , int b ) {
    cmp += ! checkingContract();
    return( a < b ); };
   const std::size_t r = findPeak( A , count );
   CHECK( isPeak( A , r ) && ( ( r == 0 ) || ( A[ r - 1 ] < A[ r ] ) ) );
   CHECK( cmp <= long( std::bit_width( n - 1 ) ) );
   CHECK( isMin( A , localMinimum( A ) ) );
   } while( std::next_permutation( A.begin() , A.end() ) );
  }
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints A = rndInts( g , std::size_t( rnd( g , 1 , 60 ) ) , 0 ,
                          rnd( g , 0 , 5 ) );
  CHECK( isPeak( A , findPeak( A ) ) && isMin( A , localMinimum( A ) ) );
  }
 Ints B( 1000 );
 std::iota( B.begin() , B.end() , 0 );
 CHECK( ( findPeak( B ) == 999 ) && ( localMinimum( B ) == 0 ) );
 CHECK( findPeak( B , std::greater<>() ) == 0 );
 CHECK( ( findPeak( Ints( 9 , 4 ) ) < 9 ) && ( findPeak( Ints{ 7 } ) == 0 ) );
 CHECK_THROWS( findPeak( Ints() ) , contract_violation );
 CHECK_THROWS( localMinimum( Ints() ) , contract_violation );
 for( int t = 0 ; t < 2000 ; ++t ) {
  std::unique_ptr< BT > T( rndTree( g , rnd( g , 1 , 40 ) , 20 ) );
  BT * u = localMinimum( T.get() );
  CHECK( ( ! u->left() || ( u->left()->read() >= u->read() ) ) &&
         ( ! u->right() || ( u->right()->read() >= u->read() ) ) &&
         ( ( u == T.get() ) || ( u->parent()->read() > u->read() ) ) );
  BT * v = T.get();                         // the first smaller child
  for( ; ; ) {
   if( v->left() && ( v->left()->read() < v->read() ) )
    v = v->left();
   else if( v->right() && ( v->right()->read() < v->read() ) )
    v = v->right();
   else
    break;
   }
  CHECK( u == v );
  std::vector< BT * > N;
  nodes( T.get() , N );
  CHECK( std::find( N.begin() , N.end() , u ) != N.end() );
  }
 std::unique_ptr< BT > S( new BT( 5 ) );
 S->insertLeft( new BT( 4 ) );
 CHECK( localMinimum( S.get() )->read() == 4 );
 CHECK( localMinimum( S.get() , std::greater<>() )->read() == 5 );
 CHECK_THROWS( localMinimum( static_cast< BT * >( nullptr ) ) ,
               contract_violation );
 }

// exponentialSearch() against the threshold of random predicates on int,
// std::int64_t and std::int8_t, from negative and positive starts, up to
// the largest integer, with at most 2 ceil( log2( h - lo + 1 ) ) + 1
// evaluations out of the contracts; no true value; unboundedSearch() on
// sorted vectors of unknown length against std::lower_bound
ADS_TEST( "exponential-search" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 5000 ; ++t ) {
  const std::int64_t lo = rnd( g , -1000 , 1000 ) ,
                     h = lo + rnd( g , 0 , t < 4000 ? 100 : 1000000 );
  long calls = 0;
  const auto p = [ & ]( std::int64_t i ) {
   calls += ! checkingContract();
   return( i >= h ); };
  CHECK( exponentialSearch( p , lo ) == h );
  const double d = double( h - lo + 1 );
  CHECK( calls <= 2 * long( std::ceil( std::log2( d ) ) ) + 1 );
  }
 constexpr std::int64_t M = std::numeric_limits< std::int64_t >::max();
 for( std::int64_t h : { M , M - 1 , M / 2 , M / 2 + 1 , std::int64_t( 0 ) } )
  for( std::int64_t lo : { std::numeric_limits< std::int64_t >::min() ,
                           std::int64_t( -5 ) , std::int64_t( 0 ) } )
   CHECK( exponentialSearch( [ & ]( std::int64_t i ) { return( i >= h ); } ,
                             lo ) == h );
 for( int h = -128 ; h <= 127 ; ++h )
  for( int lo = -128 ; lo <= h ; lo += 7 )
   CHECK( exponentialSearch( [ & ]( std::int8_t i ) { return( i >= h ); } ,
                             std::int8_t( lo ) ) == h );
 CHECK( exponentialSearch< int >( []( int i ) { return( i >= 0 ); } ) == 0 );
 CHECK( exponentialSearch( []( unsigned i ) { return( i > 40 ); } , 3u )
        == 41u );
 CHECK_THROWS( exponentialSearch( []( std::int8_t ) { return( false ); } ,
                                  std::int8_t( 0 ) ) , contract_violation );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints A = rndInts( g , std::size_t( rnd( g , 0 , t < 2000 ? 40 : 5000 ) ) ,
                    -100 , 100 );
  std::sort( A.begin() , A.end() );
  const int v = rnd( g , -110 , 110 );
  long reads = 0;
  const auto get = [ & ]( std::size_t i ) {
   reads += ! checkingContract();
   return( i < A.size() ? std::optional< int >( A[ i ] ) : std::nullopt ); };
  const std::size_t r = unboundedSearch( get , [ & ]( int x ) {
   return( x >= v ); } );
  CHECK( r == std::size_t( std::lower_bound( A.begin() , A.end() , v ) -
                           A.begin() ) );
  CHECK( reads <= 2 * long( std::bit_width( A.size() ) ) + 1 );
  }
 }

// integerSqrt() against the floor of the square root for n < 10^5 and about
// the squares of random roots of 32 bits, the largest integers of 8 to 64
// bits; isPerfectSquare() against the squares
ADS_TEST( "integer-sqrt" )
{
 std::uint64_t r = 0;
 for( std::uint64_t n = 0 ; n < 100000 ; ++n ) {
  while( ( r + 1 ) * ( r + 1 ) <= n )
   ++r;
  CHECK( integerSqrt( n ) == r );
  CHECK( isPerfectSquare( n ) == ( r * r == n ) );
  }
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  const std::uint64_t k = g();
  const std::uint64_t s = k * k;
  CHECK( integerSqrt( s ) == k && isPerfectSquare( s ) );
  if( k > 1 ) {
   CHECK( integerSqrt( s - 1 ) == k - 1 && ! isPerfectSquare( s - 1 ) );
   CHECK( integerSqrt( s + 1 ) == k && ! isPerfectSquare( s + 1 ) );
   }
  }
 CHECK( integerSqrt( std::numeric_limits< std::uint64_t >::max() ) ==
        0xFFFFFFFFull );
 CHECK( integerSqrt( std::numeric_limits< std::uint32_t >::max() ) ==
        0xFFFFu );
 CHECK( integerSqrt( std::uint16_t( 65535 ) ) == 255 );
 CHECK( integerSqrt( std::uint8_t( 255 ) ) == 15 );
 for( unsigned n = 0 ; n < 256 ; ++n )
  CHECK( integerSqrt( std::uint8_t( n ) ) ==
         unsigned( std::sqrt( double( n ) ) ) );
 CHECK( isPerfectSquare( 0u ) && isPerfectSquare( 1u ) &&
        ! isPerfectSquare( 2u ) );
 CHECK( ! isPerfectSquare( std::numeric_limits< std::uint64_t >::max() ) );
 CHECK( isPerfectSquare( 0xFFFFFFFE00000001ull ) );
 }

// rotatedMinimum() on every rotation of random strictly increasing vectors
// against std::min_element, with at most ceil( log2 n ) comparisons;
// closestValue() against a scan, first place and smaller item on ties, on
// vectors with runs of equal items, at the extremes of int and on doubles;
// pairWithSum() against all the pairs; kthOfTwoSorted() against the merge
ADS_TEST( "sorted-searches" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::set< int > V;
  const std::size_t n = std::size_t( rnd( g , 1 , 40 ) );
  while( V.size() < n )
   V.insert( rnd( g , -100 , 100 ) );
  const Ints A( V.begin() , V.end() );
  for( std::size_t k = 0 ; k < n ; ++k ) {
   Ints R( A );
   std::rotate( R.begin() , R.end() - std::ptrdiff_t( k ) , R.end() );
   long cmp = 0;
   const auto count = [ & ]( int a , int b ) {
    cmp += ! checkingContract();
    return( a < b ); };
   CHECK( rotatedMinimum( R , count ) == k );
   CHECK( cmp <= long( std::bit_width( n - 1 ) ) );
   }
  }
 CHECK( rotatedMinimum( Ints{ 5 } ) == 0 );
 CHECK( rotatedMinimum( Ints{ INT_MAX , INT_MIN } ) == 1 );
 CHECK( rotatedMinimum( Ints{ 2 , 1 , 3 } , std::greater<>() ) == 2 );
 CHECK_THROWS( rotatedMinimum( Ints() ) , contract_violation );
 CHECK_THROWS( rotatedMinimum( Ints{ 3 , 3 } ) , contract_violation );
 CHECK_THROWS( rotatedMinimum( Ints{ 2 , 1 , 3 } ) , contract_violation );
 for( int t = 0 ; t < 5000 ; ++t ) {
  Ints A = rndInts( g , std::size_t( rnd( g , 1 , 20 ) ) , -30 , 30 );
  std::sort( A.begin() , A.end() );
  const int x = rnd( g , -40 , 40 );
  std::size_t best = 0;
  for( std::size_t i = 1 ; i < A.size() ; ++i )
   if( std::abs( A[ i ] - x ) < std::abs( A[ best ] - x ) )
    best = i;
  CHECK( closestValue( A , x ) == best );
  }
 CHECK( closestValue( Ints{ 0 , 3 , 3 } , 1 ) == 0 );
 CHECK( closestValue( Ints{ 0 , 2 , 2 , 9 } , 1 ) == 0 );
 CHECK( closestValue( Ints{ 0 , 3 , 3 , 9 } , 2 ) == 1 );
 CHECK( closestValue( Ints( 7 , 3 ) , 100 ) == 0 );
 CHECK( closestValue( Ints{ INT_MIN , INT_MAX } , INT_MIN ) == 0 );
 CHECK( closestValue( Ints{ INT_MIN , INT_MAX } , INT_MAX ) == 1 );
 CHECK( closestValue( Ints{ INT_MIN , INT_MAX } , 0 ) == 1 );
 CHECK( closestValue( Ints{ INT_MIN , INT_MAX } , -1 ) == 0 );
 CHECK( closestValue( Ints{ INT_MIN , 0 , INT_MAX } , 1 ) == 1 );
 CHECK( closestValue( std::vector< double >{ 0.5 , 1.25 , 2.0 } , 1.7 ) == 2 );
 CHECK( closestValue( std::vector< unsigned >{ 0u , 4000000000u } ,
                      2000000001u ) == 1 );
 CHECK_THROWS( closestValue( Ints() , 0 ) , contract_violation );
 CHECK_THROWS( closestValue( Ints{ 2 , 1 } , 0 ) , contract_violation );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints A = rndInts( g , std::size_t( rnd( g , 0 , 20 ) ) , -30 , 30 );
  std::sort( A.begin() , A.end() );
  const long x = rnd( g , -60 , 60 );
  bool brute = false;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i + 1 ; j < A.size() ; ++j )
    brute = brute || ( A[ i ] + A[ j ] == x );
  const auto r = pairWithSum( A , x );
  CHECK( r.has_value() == brute );
  CHECK( ! r || ( ( r->first < r->second ) &&
                  ( A[ r->first ] + A[ r->second ] == x ) ) );
  }
 CHECK( ! pairWithSum( Ints() , 0 ) && ! pairWithSum( Ints{ 4 } , 8 ) );
 CHECK( pairWithSum( Ints{ 4 , 4 } , 8 ) ==
        std::pair( std::size_t( 0 ) , std::size_t( 1 ) ) );
 CHECK( pairWithSum( Ints{ INT_MAX , INT_MAX } ,
                     2 * std::int64_t( INT_MAX ) ) );
 CHECK( pairWithSum( std::vector< double >{ 0.25 , 0.5 , 4.0 } , 4.5 ) );
 CHECK_THROWS( pairWithSum( Ints{ 2 , 1 } , 3 ) , contract_violation );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints X = rndInts( g , std::size_t( rnd( g , 0 , 20 ) ) , 0 ,
                    rnd( g , 1 , 50 ) );
  Ints Y = rndInts( g , std::size_t( rnd( g , X.empty() , 20 ) ) , 0 ,
                    rnd( g , 1 , 50 ) );
  std::sort( X.begin() , X.end() );
  std::sort( Y.begin() , Y.end() );
  Ints Z( X.size() + Y.size() );
  std::merge( X.begin() , X.end() , Y.begin() , Y.end() , Z.begin() );
  for( std::size_t k = 0 ; k < Z.size() ; ++k )
   CHECK( kthOfTwoSorted( X , Y , k ) == Z[ k ] );
  }
 const Ints X{ 1 , 3 , 5 } , Y{ 2 , 4 , 6 };
 CHECK( kthOfTwoSorted( X , Y , 2 ) == 3 && kthOfTwoSorted( X , Y , 3 ) == 4 );
 CHECK( kthOfTwoSorted( Ints{ 7 } , Ints() , 0 ) == 7 );
 CHECK( kthOfTwoSorted( Ints{ 5 , 3 } , Ints{ 4 } , 0 , std::greater<>() ) ==
        5 );
 CHECK_THROWS( kthOfTwoSorted( X , Y , 6 ) , contract_violation );
 CHECK_THROWS( kthOfTwoSorted( Ints{ 2 , 1 } , Y , 0 ) , contract_violation );
 }

/// the least total cost and the fewest swaps that sort A, by Dijkstra on
/// the permutations, a swap costing the sum of its two items
std::pair< std::size_t , std::int64_t > swapSortBrute( const Ints & A )
{
 Ints S( A );
 std::sort( S.begin() , S.end() );
 std::map< Ints , std::int64_t > cost;
 std::map< Ints , std::size_t > hops;
 using E = std::pair< std::int64_t , Ints >;
 std::priority_queue< E , std::vector< E > , std::greater<> > Q;
 std::queue< Ints > B;
 cost[ A ] = 0;
 hops[ A ] = 0;
 Q.push( E( 0 , A ) );
 B.push( A );
 while( ! Q.empty() ) {
  const auto [ c , u ] = Q.top();
  Q.pop();
  if( c > cost[ u ] )
   continue;
  for( std::size_t i = 0 ; i < u.size() ; ++i )
   for( std::size_t j = i + 1 ; j < u.size() ; ++j ) {
    Ints v( u );
    std::swap( v[ i ] , v[ j ] );
    const std::int64_t d = c + u[ i ] + u[ j ];
    if( ! cost.count( v ) || ( d < cost[ v ] ) ) {
     cost[ v ] = d;
     Q.push( E( d , v ) );
     }
    }
  }
 while( ! B.empty() ) {                     // the swaps by a visit
  const Ints u = B.front();
  B.pop();
  for( std::size_t i = 0 ; i < u.size() ; ++i )
   for( std::size_t j = i + 1 ; j < u.size() ; ++j ) {
    Ints v( u );
    std::swap( v[ i ] , v[ j ] );
    if( ! hops.count( v ) ) {
     hops[ v ] = hops[ u ] + 1;
     B.push( v );
     }
    }
  }
 return( std::pair( hops[ S ] , cost[ S ] ) );
 }

// dutchNationalFlag() against the counts of the colours, at most n swaps,
// the colour out of range; wiggleSort() on random vectors with repeated
// items, strict on distinct ones; sortKSorted() against std::sort on random
// k-sorted vectors, the vector not k-sorted; minimumSwapSort() against
// Dijkstra and a visit on the permutations of up to 6 items
ADS_TEST( "rearrangements" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints A = rndInts( g , std::size_t( rnd( g , 0 , 30 ) ) , 0 , 2 );
  const Ints C( A );
  std::size_t swaps = 0;
  const auto [ i , j ] = dutchNationalFlag( A , []( int x ) {
   return( x ); } , & swaps );
  Ints S( C );
  std::sort( S.begin() , S.end() );
  CHECK( ( A == S ) && ( swaps <= A.size() ) );
  CHECK( i == std::size_t( std::count( C.begin() , C.end() , 0 ) ) );
  CHECK( j == i + std::size_t( std::count( C.begin() , C.end() , 1 ) ) );
  }
 std::vector< std::string > W{ "red" , "white" , "green" , "red" , "green" };
 const auto colour = []( const std::string & s ) {
  return( s == "green" ? 0 : ( s == "white" ? 1 : 2 ) ); };
 CHECK( dutchNationalFlag( W , colour ) ==
        std::pair( std::size_t( 2 ) , std::size_t( 3 ) ) );
 CHECK( W[ 0 ] == "green" && W[ 2 ] == "white" && W[ 4 ] == "red" );
 Ints E;
 CHECK( dutchNationalFlag( E , []( int ) { return( 1 ); } ) ==
        std::pair( std::size_t( 0 ) , std::size_t( 0 ) ) );
 Ints F{ 0 , 3 };
 CHECK_THROWS( dutchNationalFlag( F , []( int x ) { return( x ); } ) ,
               contract_violation );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints A = rndInts( g , std::size_t( rnd( g , 0 , 40 ) ) , 0 ,
                    rnd( g , 0 , 100 ) );
  Ints B( A );
  wiggleSort( B );
  CHECK( std::is_permutation( A.begin() , A.end() , B.begin() , B.end() ) );
  const bool distinct = std::set< int >( A.begin() , A.end() ).size() ==
                        A.size();
  for( std::size_t i = 0 ; i + 1 < B.size() ; ++i )
   CHECK( ( i % 2 == 0 ) ? ( distinct ? B[ i ] < B[ i + 1 ]
                                      : B[ i ] <= B[ i + 1 ] )
                         : ( distinct ? B[ i ] > B[ i + 1 ]
                                      : B[ i ] >= B[ i + 1 ] ) );
  }
 Ints H{ 1 , 3 , 4 , 6 , 8 , 7 };
 wiggleSort( H , std::greater<>() );
 CHECK( H == Ints( { 3 , 1 , 6 , 4 , 8 , 7 } ) );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const std::size_t n = std::size_t( rnd( g , 0 , 60 ) ) ,
                    k = std::size_t( rnd( g , 0 , 8 ) );
  Ints A = rndInts( g , n , -50 , 50 );
  std::sort( A.begin() , A.end() );
  Ints S( A );
  for( std::size_t b = 0 ; b < n ; b += k + 1 )   // blocks of k + 1
   std::shuffle( A.begin() + std::ptrdiff_t( b ) ,
                 A.begin() + std::ptrdiff_t( std::min( n , b + k + 1 ) ) ,
                 g );
  sortKSorted( A , k );
  CHECK( A == S );
  }
 Ints K{ 3 , 2 , 1 };
 sortKSorted( K , std::size_t( -1 ) );
 CHECK( K == Ints( { 1 , 2 , 3 } ) );
 Ints D{ 1 , 2 , 3 };
 sortKSorted( D , 2 , std::greater<>() );
 CHECK( D == Ints( { 3 , 2 , 1 } ) );
 Ints N{ 3 , 2 , 1 };
 CHECK_THROWS( sortKSorted( N , 1 ) , contract_violation );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::set< int > V;
  const std::size_t n = std::size_t( rnd( g , 0 , 6 ) );
  while( V.size() < n )
   V.insert( rnd( g , 1 , 30 ) );
  Ints A( V.begin() , V.end() );
  std::shuffle( A.begin() , A.end() , g );
  CHECK( ( minimumSwapSort< int , std::int64_t >( A ) ) ==
         swapSortBrute( A ) );
  }
 CHECK( ( minimumSwapSort< int , std::int64_t >( { 8 , 4 , 5 , 3 , 2 , 7 } ) )
        == std::pair( std::size_t( 4 ) , std::int64_t( 34 ) ) );
 CHECK( ( minimumSwapSort< double , double >( { 2.5 , 1.5 } ) ) ==
        std::pair( std::size_t( 1 ) , 4.0 ) );
 CHECK_THROWS( ( minimumSwapSort< int , int >( { 1 , 1 } ) ) ,
               contract_violation );
 }

// smallerToTheRight() against the double loop with repeated items;
// longestKDistinctRun() against the extension of each run on strings and
// vectors, k from 0; largestGap() against the uncovered pairs of an end and
// a start; lineAssignment() against every permutation
ADS_TEST( "sequence-queries" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints A = rndInts( g , std::size_t( rnd( g , 0 , 40 ) ) , -20 ,
                          rnd( g , -20 , 60 ) );
  std::vector< std::size_t > c( A.size() , 0 );
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i + 1 ; j < A.size() ; ++j )
    c[ i ] += ( A[ j ] < A[ i ] );
  CHECK( smallerToTheRight( A ) == c );
  }
 CHECK( smallerToTheRight( Ints{ 1 , 7 , 2 , 3 , -1 } ) ==
        ( std::vector< std::size_t >{ 1 , 3 , 1 , 1 , 0 } ) );
 CHECK( smallerToTheRight( Ints{ 1 , 2 , 3 } , std::greater<>() ) ==
        ( std::vector< std::size_t >{ 2 , 1 , 0 } ) );
 CHECK( smallerToTheRight( Ints() ).empty() );
 Ints R( 3000 );
 std::iota( R.rbegin() , R.rend() , 0 );
 const auto r = smallerToTheRight( R );
 for( std::size_t i = 0 ; i < R.size() ; ++i )
  CHECK( r[ i ] == R.size() - 1 - i );
 for( int t = 0 ; t < 3000 ; ++t ) {
  std::string S( std::size_t( rnd( g , 0 , 17 ) ) , 'a' );
  for( char & c : S )
   c = char( 'a' + rnd( g , 0 , 3 ) );
  const std::size_t k = std::size_t( rnd( g , 0 , 4 ) );
  std::size_t best = 0 , start = 0;
  for( std::size_t i = 0 ; i < S.size() ; ++i ) {
   std::set< char > C;
   for( std::size_t j = i ; j < S.size() ; ++j ) {
    C.insert( S[ j ] );
    if( C.size() > k )
     break;
    if( j - i + 1 > best ) {
     best = j - i + 1;
     start = i;
     }
    }
   }
  CHECK( longestKDistinctRun( S , k ) == std::pair( start , best ) );
  }
 CHECK( longestKDistinctRun( std::string( "abacccbb" ) , 2 ) ==
        std::pair( std::size_t( 3 ) , std::size_t( 5 ) ) );
 CHECK( longestKDistinctRun( Ints{ 1 , 2 , 1 , 3 } , 2 ) ==
        std::pair( std::size_t( 0 ) , std::size_t( 3 ) ) );
 CHECK( longestKDistinctRun( std::string() , 2 ).second == 0 );
 CHECK( longestKDistinctRun( std::string( 1000 , 'q' ) , 1 ).second == 1000 );
 using I = std::vector< std::pair< int , int > >;
 for( int t = 0 ; t < 3000 ; ++t ) {
  I V( std::size_t( rnd( g , 0 , 8 ) ) );
  for( auto & [ a , b ] : V ) {
   a = rnd( g , 0 , 40 );
   b = a + rnd( g , 0 , 6 );
   }
  std::optional< std::pair< int , int > > best;
  for( const auto & e : V )
   for( const auto & s : V )
    if( ( e.second < s.first ) &&
        std::none_of( V.begin() , V.end() , [ & ]( const auto & p ) {
         return( ( p.second > e.second ) && ( p.first < s.first ) ); } ) &&
        ( ! best || ( s.first - e.second > best->second - best->first ) ||
          ( ( s.first - e.second == best->second - best->first ) &&
            ( e.second < best->first ) ) ) )
     best = std::pair( e.second , s.first );
  CHECK( largestGap( V ) == best );
  }
 CHECK( largestGap( I( { { 1 , 3 } , { 7 , 9 } , { 4 , 5 } } ) ) ==
        std::pair( 5 , 7 ) );
 CHECK( ! largestGap( I( { { 1 , 5 } , { 5 , 9 } } ) ) );
 CHECK( ! largestGap( I() ) );
 CHECK( largestGap( I( { { INT_MIN , INT_MIN } , { INT_MAX , INT_MAX } ,
                         { 0 , 1 } } ) ) == std::pair( INT_MIN , 0 ) );
 CHECK( largestGap( std::vector< std::pair< double , double > >(
                     { { 0.0 , 1.0 } , { 1.5 , 2.0 } } ) ) ==
        std::pair( 1.0 , 1.5 ) );
 CHECK_THROWS( largestGap( I( { { 2 , 1 } } ) ) , contract_violation );
 for( int t = 0 ; t < 400 ; ++t ) {
  const std::size_t n = std::size_t( rnd( g , 0 , 7 ) );
  const Ints H = rndInts( g , n , -50 , 50 ) , L = rndInts( g , n , -50 , 50 );
  Ints P( n );
  std::iota( P.begin() , P.end() , 0 );
  long best = std::numeric_limits< long >::max();
  do {
   long c = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    c += std::abs( H[ i ] - L[ std::size_t( P[ i ] ) ] );
   best = std::min( best , c );
   } while( std::next_permutation( P.begin() , P.end() ) );
  CHECK( ( lineAssignment< int , long >( H , L ) ) == best );
  }
 CHECK( ( lineAssignment< int , long >( { 5 , 10 } , { 9 , 14 } ) ) == 8 );
 CHECK( ( lineAssignment< int , std::int64_t >( { INT_MIN } , { INT_MAX } ) )
        == std::int64_t( UINT_MAX ) );
 CHECK_THROWS( ( lineAssignment< int , long >( { 1 } , {} ) ) ,
               contract_violation );
 }

// closestPair() against closestPairBrute() and the double loop on random
// sets of small coordinates (repeated points), large ones and collinear
// ones; two points; one point and coordinates out of range
ADS_TEST( "closest-pair" )
{
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 1500 ; ++t ) {
  const int n = rnd( g , 2 , 150 ) ,
            M = t % 3 == 0 ? 10 : ( t % 3 == 1 ? 1000000000 : 1000 );
  std::vector< Point > P( static_cast< std::size_t >( n ) );
  for( auto & p : P )
   p = Point{ rnd( g , -M , M ) , t % 7 ? rnd( g , -M , M ) : 0 };
  std::int64_t best = std::numeric_limits< std::int64_t >::max();
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   for( std::size_t j = i + 1 ; j < P.size() ; ++j ) {
    const std::int64_t dx = std::int64_t( P[ i ].x ) - P[ j ].x ,
                       dy = std::int64_t( P[ i ].y ) - P[ j ].y;
    best = std::min( best , dx * dx + dy * dy );
    }
  const auto [ ij , d ] = closestPair( P );
  CHECK( ( d == best ) && ( ij.first < ij.second ) );
  CHECK( squaredDistance( P[ ij.first ] , P[ ij.second ] ) == d );
  CHECK( closestPairBrute( P ).second == best );
  }
 CHECK( closestPair( { Point{ 0 , 0 } , Point{ 3 , 4 } } ).second == 25 );
 CHECK( closestPair( { Point{ -1000000000 , -1000000000 } ,
                       Point{ 1000000000 , 1000000000 } } ).second ==
        std::int64_t( 8000000000000000000LL ) );
 CHECK( closestPair( { Point{ 1 , 1 } , Point{ 9 , 9 } , Point{ 1 , 1 } } )
        == std::pair( std::pair( std::size_t( 0 ) , std::size_t( 2 ) ) ,
                      std::int64_t( 0 ) ) );
 CHECK_THROWS( closestPair( { Point{ 0 , 0 } } ) , contract_violation );
 CHECK_THROWS( closestPair( { Point{ 0 , 0 } , Point{ 1000000001 , 0 } } ) ,
               contract_violation );
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
/*------------------------ End File lib_search.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
