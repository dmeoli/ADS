/*--------------------------------------------------------------------------*/
/*----------------------------- File ch12.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 12, Divide et impera: the search by interpolation on the
 * example of the book, perno() on Figura 12.1, Quicksort on Figure 12.2 and
 * 12.3 and on average, the tuned Quicksort and its stack, the products of
 * polynomials and the algorithm of Strassen, and the exercises 12.1 to
 * 12.13, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 12,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
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

#include "ADS/numeric/Matrix.h"
#include "ADS/numeric/Polynomial.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/search/InterpolationSearch.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/QuickSort.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <numeric>
#include <random>
#include <set>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;

/// the items of a list
template< class T >
std::vector< T > items( const List< T > & L )
{
 std::vector< T > out;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  out.push_back( L.read( p ) );
 return( out );
 }

/// a counting comparison
struct Counting {
 long * c;
 bool operator()( int a , int b ) const {   // not those of the contracts
  *c += ! checkingContract();
  return( a < b );
  }
 };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the example of the book, v = 13: 2 probes, ( 0 , 11 , 3 ) and
// ( 4 , 11 , 4 ), where the binary search makes 4; on uniform keys a few
// probes, O( log log n ), against about log2 n; the first and the last,
// absent below, between and above, empty, one item, all equal, the
// extremes of long, distinct keys equal as doubles and infinite keys (an
// undefined fraction), duplicates, and the audit on an unsorted vector
ADS_TEST( "sec12.1.interpolation" )
{
 const Ints A = { 1 , 5 , 7 , 8 , 13 , 17 , 21 , 27 , 33 , 37 , 40 , 42 };
 std::vector< std::tuple< long , long , long > > probes;
 CHECK( interpolationSearch( A , 13 , [ & ]( long i , long j , long m ) {
         probes.emplace_back( i , j , m ); } ) == 4 );
 const std::vector< std::tuple< long , long , long > > book = {
  { 0 , 11 , 3 } , { 4 , 11 , 4 } };
 CHECK( probes == book );
 CHECK( interpolationSearch( A , 14 ) == -1 );
 CHECK( interpolationSearch( A , 0 ) == -1 );
 CHECK( interpolationSearch( A , 43 ) == -1 );            // past the last
 CHECK( interpolationSearch( A , 1 ) == 0 );              // the first
 CHECK( interpolationSearch( A , 42 ) == 11 );            // the last
 CHECK( interpolationSearch( Ints( { 5 , 5 , 5 } ) , 5 ) >= 0 );
 CHECK( interpolationSearch( Ints( { 5 , 5 , 5 } ) , 4 ) == -1 );
 CHECK( interpolationSearch( Ints( { 5 , 5 , 5 } ) , 6 ) == -1 );
 CHECK( interpolationSearch( Ints() , 5 ) == -1 );         // empty
 CHECK( interpolationSearch( Ints( { 5 } ) , 5 ) == 0 );   // one item
 CHECK( interpolationSearch( Ints( { 5 } ) , 3 ) == -1 );
 // the extremes of long long: v - A[ i ] would overflow, the probe stays
 // in A
 using LLs = std::vector< long long >;
 const long long LO = std::numeric_limits< long long >::min() ,
                 HI = std::numeric_limits< long long >::max();
 const LLs X = { LO , LO + 1 , -1 , 0 , 1 , HI - 1 , HI };
 for( std::size_t i = 0 ; i < X.size() ; ++i )
  CHECK( interpolationSearch( X , X[ i ] ) == long( i ) );
 CHECK( interpolationSearch( X , 2LL ) == -1 );
 CHECK( interpolationSearch( X , HI - 2 ) == -1 );
 // distinct keys equal as doubles (past 2^53): the interpolation 0 / 0
 // falls back on the middle (it was a cast of a NaN to long)
 const LLs Y = { HI - 4 , HI - 3 , HI - 2 , HI - 1 , HI };
 for( std::size_t i = 0 ; i < Y.size() ; ++i )
  CHECK( interpolationSearch( Y , Y[ i ] ) == long( i ) );
 CHECK( interpolationSearch( Y , HI - 5 ) == -1 );
 // infinite keys give inf / inf, also the middle
 const double INF = std::numeric_limits< double >::infinity();
 const std::vector< double > Z = { -INF , -1.5 , 0.0 , 2.5 , INF };
 for( std::size_t i = 0 ; i < Z.size() ; ++i )
  CHECK( interpolationSearch( Z , Z[ i ] ) == long( i ) );
 CHECK( interpolationSearch( Z , 1.0 ) == -1 );
 // runs of duplicates: a place of v, whichever
 const Ints R = { 1 , 1 , 1 , 2 , 2 , 9 , 9 , 9 , 9 };
 for( int v : { 1 , 2 , 9 } )
  CHECK( R[ interpolationSearch( R , v ) ] == v );
 CHECK_THROWS( interpolationSearch( Ints( { 3 , 1 , 2 } ) , 7 ) ,
               std::logic_error );                    // not sorted, audit
 std::mt19937 g( 1 );
 const int n = 1 << 20;
 Longs U( n );
 for( auto & u : U ) u = long( g() );
 std::sort( U.begin() , U.end() );
 long total = 0;
 for( int t = 0 ; t < 1000 ; ++t ) {
  const long v = U[ g() % n ];
  long c = 0;
  CHECK( U[ interpolationSearch( U , v , [ & ]( long , long , long ) {
          ++c; } ) ] == v );
  total += c;
  }
 CHECK( total / 1000.0 < 6 );                   // log2 log2 n = 4.3
 }

// Figura 12.1: perno() on 9 12 8 18 6 13 11 3 5 10 puts the pivot in j = 4;
// one item, the pivot the smallest or the largest, all equal, an inner
// range, and the contracts on the range
ADS_TEST( "sec12.2.perno" )
{
 Ints A = { 9 , 12 , 8 , 18 , 6 , 13 , 11 , 3 , 5 , 10 };
 CHECK( partition( A , 0 , 9 ) == 4 );
 CHECK( A == Ints( { 5 , 8 , 6 , 3 , 9 , 13 , 11 , 18 , 12 , 10 } ) );
 // a range of one item, the pivot the smallest, the largest, all equal,
 // and a range inside the vector, the rest untouched
 Ints B = { 4 , 2 , 7 };
 CHECK( partition( B , 1 , 1 ) == 1 && B == Ints( { 4 , 2 , 7 } ) );
 Ints C = { 1 , 5 , 3 , 4 };
 CHECK( partition( C , 0 , 3 ) == 0 && C[ 0 ] == 1 );
 Ints D = { 9 , 5 , 3 , 4 };
 CHECK( partition( D , 0 , 3 ) == 3 && D[ 3 ] == 9 );
 Ints E( 6 , 2 );
 CHECK( partition( E , 0 , 5 ) == 0 && E == Ints( 6 , 2 ) );
 Ints F = { 8 , 6 , 9 , 1 , 7 , 0 };
 CHECK( partition( F , 1 , 4 ) == 2 && F[ 0 ] == 8 && F[ 5 ] == 0 &&
        F[ 1 ] == 1 && F[ 2 ] == 6 );
 // start > end, end out of the vector, an empty vector
 CHECK_THROWS( partition( B , 2 , 1 ) , std::logic_error );
 CHECK_THROWS( partition( B , 0 , 3 ) , std::logic_error );
 Ints none;
 CHECK_THROWS( partition( none , 0 , 0 ) , std::logic_error );
 }

// Figura 12.2 and 12.3: the comparisons are n ( n - 1 ) / 2 on a sorted
// vector and about 2 n ln n on average, below 2 ( n + 1 ) H_{n+1}; also
// n ( n - 1 ) / 2 on a reversed one; empty, one, two, the extremes of int,
// the empty range past the end and the contract on a range past it
ADS_TEST( "sec12.2.quicksort" )
{
 Ints A = { 9 , 12 , 8 , 18 , 6 , 13 , 11 , 3 , 5 , 10 };
 quickSort( A );
 CHECK( A == Ints( { 3 , 5 , 6 , 8 , 9 , 10 , 11 , 12 , 13 , 18 } ) );
 const int n = 1000;
 Ints S( n );
 std::iota( S.begin() , S.end() , 0 );
 long c = 0;
 quickSort( S , Counting{ &c } );
 CHECK( c == long( n ) * ( n - 1 ) / 2 );
 std::mt19937 g( 2 );
 double H = 0;
 for( int k = 3 ; k <= n + 1 ; ++k ) H += 1.0 / k;
 double avg = 0;
 for( int t = 0 ; t < 50 ; ++t ) {
  Ints R = S;
  std::shuffle( R.begin() , R.end() , g );
  long d = 0;
  quickSort( R , Counting{ &d } );
  CHECK( std::is_sorted( R.begin() , R.end() ) );
  avg += d / 50.0;
  }
 CHECK( avg <= 2.0 * ( n + 1 ) * H );
 CHECK( avg >= 1.5 * n * std::log( double( n ) ) );

 // reversed: also n ( n - 1 ) / 2, the pivot the largest at each level
 Ints V( S.rbegin() , S.rend() );
 long e = 0;
 quickSort( V , Counting{ &e } );
 CHECK( V == S && e == long( n ) * ( n - 1 ) / 2 );
 // empty, one, two in both orders, the extremes of int, no comparison
 // on fewer than two items
 for( Ints X : { Ints() , Ints( { 4 } ) , Ints( { 1 , 2 } ) ,
                 Ints( { 2 , 1 } ) ,
                 Ints( { 0 , std::numeric_limits< int >::max() , -1 ,
                         std::numeric_limits< int >::min() , 0 } ) } ) {
  Ints Y = X;
  long f = 0;
  quickSort( X , Counting{ &f } );
  std::sort( Y.begin() , Y.end() );
  CHECK( X == Y && ( Y.size() > 1 || f == 0 ) );
  }
 // a range: the empty one ( start > end ) does nothing even past the end,
 // one past the end of the vector breaks the contract
 Ints W = { 3 , 2 , 1 , 0 };
 quickSort( W , 3 , 2 , std::less<>() );
 quickSort( W , 9 , 0 , std::less<>() );
 CHECK( W == Ints( { 3 , 2 , 1 , 0 } ) );
 quickSort( W , 1 , 2 , std::less<>() );
 CHECK( W == Ints( { 3 , 1 , 2 , 0 } ) );
 CHECK_THROWS( quickSort( W , 1 , 4 , std::less<>() ) , std::logic_error );
 }

// the improvements: the median of three, the partition of Hoare, the
// cutoff, the explicit stack of at most log2 n parts; sorted, reversed,
// equal, random inputs against std::sort; the cutoffs below 3, around the
// size and past it, on organ pipes, two values and the extremes of int
ADS_TEST( "sec12.2.tuned" )
{
 std::mt19937 g( 3 );
 for( int n : { 0 , 1 , 2 , 3 , 10 , 100 , 10000 , 100000 } ) {
  std::vector< Ints > inputs( 5 , Ints( n ) );
  std::iota( inputs[ 0 ].begin() , inputs[ 0 ].end() , 0 );
  inputs[ 1 ] = inputs[ 0 ];
  std::reverse( inputs[ 1 ].begin() , inputs[ 1 ].end() );
  for( auto & x : inputs[ 2 ] ) x = 7;
  for( auto & x : inputs[ 3 ] ) x = g() % 10;
  for( auto & x : inputs[ 4 ] ) x = int( g() );
  for( auto A : inputs ) {
   Ints B = A;
   std::size_t depth = 0;
   long c = 0;
   quickSortTuned( A , 16 , Counting{ &c } , &depth );
   std::sort( B.begin() , B.end() );
   CHECK( A == B );
   CHECK( depth <= std::size_t( std::log2( n + 1.0 ) ) + 1 );
   if( n >= 10000 )
    CHECK( c < 3 * n * std::log2( double( n ) ) );
   }
  }
 // the cutoffs 0 , 1 , 2 (taken as 3), at the size, past it (insertion
 // only), and one below; organ pipes, two values, the extremes of int
 const int MAXI = std::numeric_limits< int >::max() ,
           MINI = std::numeric_limits< int >::min();
 for( int n : { 2 , 3 , 4 , 16 , 17 , 33 , 1000 } ) {
  std::vector< Ints > inputs( 3 , Ints( n ) );
  for( int i = 0 ; i < n ; ++i ) {
   inputs[ 0 ][ i ] = std::min( i , n - 1 - i );
   inputs[ 1 ][ i ] = int( g() % 2 );
   inputs[ 2 ][ i ] = ( g() % 3 == 0 ) ? MAXI : ( g() % 2 ) ? MINI :
                                          int( g() );
   }
  for( std::size_t cut : { std::size_t( 0 ) , std::size_t( 1 ) ,
                           std::size_t( 2 ) , std::size_t( n - 1 ) ,
                           std::size_t( n ) , std::size_t( n + 1 ) } )
   for( auto A : inputs ) {
    Ints B = A;
    quickSortTuned( A , cut );
    std::sort( B.begin() , B.end() );
    CHECK( A == B );
    }
  }
 }

// the products of polynomials: the naive one, four products, Karatsuba;
// on n = 2^k coefficients, n^2, n^2 and 3^k products; an empty factor (no
// product), constants, a constant times a long one, the zero polynomial,
// lengths one off a power of two
ADS_TEST( "sec12.3.polynomials" )
{
 const Longs p = { 1 , 2 , 3 } , q = { 4 , 5 };      // 1 + 2x + 3x^2 ...
 CHECK( multiplyNaive( p , q ) == Longs( { 4 , 13 , 22 , 15 } ) );
 CHECK( multiplyKaratsuba( p , q ) == Longs( { 4 , 13 , 22 , 15 } ) );
 CHECK( multiplyFour( p , q ) == Longs( { 4 , 13 , 22 , 15 } ) );
 CHECK( multiplyKaratsuba( Longs() , q ).empty() );
 // an empty factor gives the empty product, with no multiplication
 for( auto [ a , b ] : { std::pair( Longs() , q ) , std::pair( p , Longs() ) ,
                         std::pair( Longs() , Longs() ) } ) {
  long m1 = 0 , m2 = 0 , m3 = 0;
  CHECK( multiplyNaive( a , b , &m1 ).empty() &&
         multiplyFour( a , b , &m2 ).empty() &&
         multiplyKaratsuba( a , b , &m3 ).empty() );
  CHECK( m1 == 0 && m2 == 0 && m3 == 0 );
  }
 // constants, a constant times a long one, the zero polynomial, and
 // lengths one off a power of two
 CHECK( multiplyKaratsuba( Longs( { -3 } ) , Longs( { 5 } ) ) ==
        Longs( { -15 } ) );
 CHECK( multiplyFour( Longs( { 2 } ) , Longs( { 1 , 0 , -1 , 4 , 0 } ) ) ==
        Longs( { 2 , 0 , -2 , 8 , 0 } ) );
 CHECK( multiplyKaratsuba( Longs( { 1 , 0 , -1 , 4 , 0 } ) , Longs( { 2 } ) )
        == Longs( { 2 , 0 , -2 , 8 , 0 } ) );
 CHECK( multiplyKaratsuba( Longs( 7 , 0 ) , Longs( 9 , 0 ) ) ==
        Longs( 15 , 0 ) );
 for( std::size_t n : { 3 , 5 , 7 , 9 , 15 , 17 , 31 , 33 } ) {
  Longs a( n ) , b( n );
  std::iota( a.begin() , a.end() , -long( n / 2 ) );
  std::iota( b.rbegin() , b.rend() , 1L );
  CHECK( multiplyKaratsuba( a , b ) == multiplyNaive( a , b ) );
  CHECK( multiplyFour( a , b ) == multiplyNaive( a , b ) );
  }
 std::mt19937 g( 4 );
 for( int k = 0 ; k <= 8 ; ++k ) {
  const std::size_t n = std::size_t( 1 ) << k;
  Longs a( n ) , b( n );
  for( auto & x : a ) x = long( g() % 21 ) - 10;
  for( auto & x : b ) x = long( g() % 21 ) - 10;
  long m1 = 0 , m2 = 0 , m3 = 0;
  const auto r = multiplyNaive( a , b , &m1 );
  CHECK( multiplyFour( a , b , &m2 ) == r );
  CHECK( multiplyKaratsuba( a , b , &m3 ) == r );
  CHECK( m1 == long( n * n ) && m2 == long( n * n ) );
  CHECK( m3 == long( std::pow( 3.0 , k ) ) );
  }
 for( int t = 0 ; t < 50 ; ++t ) {                 // lengths of any kind
  Longs a( 1 + g() % 40 ) , b( 1 + g() % 40 );
  for( auto & x : a ) x = long( g() % 7 );
  for( auto & x : b ) x = long( g() % 7 );
  CHECK( multiplyKaratsuba( a , b ) == multiplyNaive( a , b ) );
  CHECK( multiplyFour( a , b ) == multiplyNaive( a , b ) );
  }
 }

// Strassen against the naive product, 7^k products on n = 2^k, any n by
// bordering with zeros, and the cutoff; n = 0, the cutoffs 0 and past n,
// the identity and the zero matrix, rectangular and ragged factors
ADS_TEST( "sec12.4.strassen" )
{
 std::mt19937 g( 5 );
 for( std::size_t n : { 1 , 2 , 3 , 4 , 7 , 8 , 16 , 33 } ) {
  Matrix< long > A( n , Longs( n ) ) , B = A;
  for( auto & r : A ) for( auto & x : r ) x = long( g() % 11 ) - 5;
  for( auto & r : B ) for( auto & x : r ) x = long( g() % 11 ) - 5;
  long mn = 0 , ms = 0;
  const auto C = multiplyNaive( A , B , &mn );
  CHECK( strassen( A , B , 1 , &ms ) == C );
  CHECK( strassen( A , B , 4 ) == C );
  if( ( n & ( n - 1 ) ) == 0 )
   CHECK( ms == long( std::pow( 7.0 , std::log2( double( n ) ) ) + 0.5 ) );
  }
 CHECK_THROWS( strassen( Matrix< long >( 2 , Longs( 2 ) ) ,
                         Matrix< long >( 3 , Longs( 3 ) ) ) ,
               contract_violation );
 // n = 0, the cutoff 0 (taken as 1) and past n (one naive product of the
 // matrices bordered to 8 x 8), the identity, the zero matrix
 CHECK( strassen( Matrix< long >() , Matrix< long >() ).empty() );
 Matrix< long > I( 5 , Longs( 5 , 0 ) ) , A( 5 , Longs( 5 ) ) ,
                Z( 5 , Longs( 5 , 0 ) );
 for( std::size_t i = 0 ; i < 5 ; ++i ) {
  I[ i ][ i ] = 1;
  for( auto & x : A[ i ] ) x = long( g() % 11 ) - 5;
  }
 CHECK( strassen( A , I , 0 ) == A && strassen( I , A , 1 ) == A );
 long m0 = 0;
 CHECK( strassen( A , Z , 100 , &m0 ) == Z && strassen( Z , A ) == Z );
 CHECK( m0 == 8 * 8 * 8 );                   // bordered to 8, then naive
 // rectangular and ragged factors break the contract
 CHECK_THROWS( strassen( Matrix< long >( 2 , Longs( 3 ) ) ,
                         Matrix< long >( 2 , Longs( 3 ) ) ) ,
               contract_violation );
 Matrix< long > R( 3 , Longs( 3 ) );
 R[ 1 ].pop_back();
 CHECK_THROWS( strassen( R , Matrix< long >( 3 , Longs( 3 ) ) ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// Merge Sort and Quicksort on lists with pointers: empty, one, two, three
// items, random, sorted, reversed, all equal
ADS_TEST( "ex12.1" )
{
 std::mt19937 g( 6 );
 // random, sorted, reversed, all equal; the last three, the worst case of
 // quickSort( L ), O( n^2 ) moves each audited in O( n ), only up to 300
 for( int n : { 0 , 1 , 2 , 3 , 50 , 300 , 5000 } )
  for( int kind = 0 ; kind < ( n > 300 ? 1 : 4 ) ; ++kind ) {
   List< int > L , M;
   Ints ref;
   for( int i = 0 ; i < n ; ++i ) {
    const int x = kind == 0 ? int( g() % 100 ) : kind == 1 ? i :
                  kind == 2 ? n - i : 7;
    L.insert( L.next( L.tail() ) , x );
    M.insert( M.next( M.tail() ) , x );
    ref.push_back( x );
    }
   std::sort( ref.begin() , ref.end() );
   mergeSort( L );
   quickSort( M );
   CHECK( items( L ) == ref && items( M ) == ref );
   }
 }

// the subvector of largest sum: O( n^3 ), O( n^2 ), divide et impera
// O( n log n ), and the scan of Kadane O( n ); the empty one has sum 0
long maxsum3( const Longs & A )
{
 long max = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = i ; j < A.size() ; ++j ) {
   long sum = 0;
   for( std::size_t k = i ; k <= j ; ++k ) sum += A[ k ];
   max = std::max( max , sum );
   }
 return( max );
 }

long maxsum2( const Longs & A )
{
 long max = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  long sum = 0;
  for( std::size_t k = i ; k < A.size() ; ++k ) {
   sum += A[ k ];
   max = std::max( max , sum );
   }
  }
 return( max );
 }

long maxsumRec( const Longs & A , long i , long j )
{
 if( i > j ) return( 0 );
 if( i == j ) return( std::max( 0L , A[ i ] ) );
 const long m = i + ( j - i ) / 2;
 const long ms = maxsumRec( A , i , m ) , md = maxsumRec( A , m + 1 , j );
 long sum = 0 , ls = 0 , rs = 0;
 for( long k = m ; k >= i ; --k ) {
  sum += A[ k ];
  ls = std::max( ls , sum );
  }
 sum = 0;
 for( long k = m + 1 ; k <= j ; ++k ) {
  sum += A[ k ];
  rs = std::max( rs , sum );
  }
 return( std::max( { ms , md , ls + rs } ) );
 }

long maxsumKadane( const Longs & A )
{
 long best = 0 , here = 0;
 for( long a : A ) {
  here = std::max( 0L , here + a );
  best = std::max( best , here );
  }
 return( best );
 }

// the four against each other on random vectors, and on empty, one item,
// all negative, all positive, the best at either end, huge items
ADS_TEST( "ex12.2" )
{
 std::mt19937 g( 7 );
 CHECK( maxsumKadane( Longs( { -2 , -3 } ) ) == 0 );
 CHECK( maxsumRec( Longs( { 2 , -1 , 3 , -5 , 4 } ) , 0 , 4 ) == 4 );
 // empty, one item of each sign, all negative, all positive (the whole
 // vector), the best at the first or the last place, huge items
 const long B = std::numeric_limits< long >::max() / 4;
 for( const auto & [ A , r ] : std::vector< std::pair< Longs , long > >( {
       { {} , 0 } , { { 5 } , 5 } , { { -5 } , 0 } , { { 0 } , 0 } ,
       { { -1 , -7 , -3 } , 0 } , { { 1 , 2 , 3 } , 6 } ,
       { { 9 , -10 , 1 , 1 } , 9 } , { { 1 , 1 , -10 , 9 } , 9 } ,
       { { B , -1 , B , -3 * B , B } , 2 * B - 1 } } ) ) {
  CHECK( maxsum3( A ) == r && maxsum2( A ) == r );
  CHECK( maxsumRec( A , 0 , long( A.size() ) - 1 ) == r );
  CHECK( maxsumKadane( A ) == r );
  }
 for( int t = 0 ; t < 100 ; ++t ) {
  Longs A( g() % 60 );
  for( auto & a : A ) a = long( g() % 21 ) - 10;
  const long r = maxsum3( A );
  CHECK( maxsum2( A ) == r );
  CHECK( maxsumRec( A , 0 , long( A.size() ) - 1 ) == r );
  CHECK( maxsumKadane( A ) == r );
  }
 }

// the inversions by Merge Sort against the pairs counted one by one; empty,
// one, sorted, all equal, and reversed with a count just below 2^31
ADS_TEST( "ex12.3" )
{
 Ints A = { 3 , 1 , 2 };
 CHECK( countInversions( A ) == 2 );
 // empty, one, sorted (0), reversed (n ( n - 1 ) / 2, just below 2^31 for
 // n = 2^16, the most a long of 32 bits holds), all equal (0: equal items
 // are no inversion)
 Ints E , O = { 4 } , S( 100 ) , Q( 100 , 3 );
 std::iota( S.begin() , S.end() , 0 );
 CHECK( countInversions( E ) == 0 && countInversions( O ) == 0 );
 CHECK( countInversions( S ) == 0 && countInversions( Q ) == 0 );
 Ints Rv( 65536 );
 std::iota( Rv.rbegin() , Rv.rend() , 0 );
 CHECK( (long long)( countInversions( Rv ) ) == 65536LL * 65535 / 2 );
 CHECK( std::is_sorted( Rv.begin() , Rv.end() ) );
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 50 ; ++t ) {
  Ints B( g() % 300 );
  for( auto & b : B ) b = int( g() % 50 );
  long ref = 0;
  for( std::size_t i = 0 ; i < B.size() ; ++i )
   for( std::size_t j = i + 1 ; j < B.size() ; ++j )
    ref += B[ i ] > B[ j ];
  CHECK( countInversions( B ) == ref );
  CHECK( std::is_sorted( B.begin() , B.end() ) );
  }
 }

// all the items equal: perno() of the book moves nothing, j = start, and
// Quicksort makes n ( n - 1 ) / 2 comparisons; the partition of Hoare
// splits the equal items evenly, O( n log n ), with a stack of at most
// log2 n parts also for 10^5 equal items and the smallest cutoff
ADS_TEST( "ex12.4" )
{
 const int n = 2000;
 Ints A( n , 5 ) , B( n , 5 );
 long c = 0 , d = 0;
 quickSort( A , Counting{ &c } );
 quickSortTuned( B , 16 , Counting{ &d } );
 CHECK( c == long( n ) * ( n - 1 ) / 2 );
 CHECK( d < 3 * n * std::log2( double( n ) ) );
 CHECK( A == Ints( n , 5 ) && B == Ints( n , 5 ) );
 // the depth of the stack on equal items stays logarithmic, also with the
 // smallest cutoff
 Ints C( 100000 , 5 );
 std::size_t depth = 0;
 quickSortTuned( C , 0 , std::less<>() , &depth );
 CHECK( C == Ints( 100000 , 5 ) && depth <= 17 );
 }

// the iterative Merge Sort, no stack, against std::stable_sort, on sizes
// around the powers of two; random keys, all one key (the order kept),
// reversed
ADS_TEST( "ex12.5" )
{
 std::mt19937 g( 9 );
 for( int n : { 0 , 1 , 2 , 3 , 5 , 64 , 100 , 1023 , 1025 } ) {
  std::vector< std::pair< int , int > > A( n );
  for( int i = 0 ; i < n ; ++i ) A[ i ] = { int( g() % 10 ) , i };
  auto byKey = []( auto & a , auto & b ) { return( a.first < b.first ); };
  for( int kind = 0 ; kind < 3 ; ++kind ) {    // random, one key, reversed
   if( kind == 1 )
    for( auto & a : A ) a.first = 4;
   if( kind == 2 )
    for( int i = 0 ; i < n ; ++i ) A[ i ].first = n - i;
   auto B = A;
   mergeSortBottomUp( A , byKey );
   std::stable_sort( B.begin() , B.end() , byKey );
   CHECK( A == B );
   }
  }
 }

// a matrix sorted by rows and columns: the search from the top right
// corner drops a row or a column at each step, at most 2 n - 1 steps;
// against the scan of all the entries; no entry, one entry, all equal
ADS_TEST( "ex12.6" )
{
 auto search = []( const Matrix< int > & M , int k , int & steps ) {
  const int n = int( M.size() );
  int i = 0 , j = n - 1;
  steps = 0;
  while( ( i < n ) && ( j >= 0 ) ) {
   ++steps;
   if( M[ i ][ j ] == k ) return( true );
   if( M[ i ][ j ] < k ) ++i; else --j;
   }
  return( false );
  };
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 1 + int( g() % 30 );
  Matrix< int > M( n , Ints( n ) );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    M[ i ][ j ] = std::max( i ? M[ i - 1 ][ j ] : 0 , j ? M[ i ][ j - 1 ] : 0 )
                  + int( g() % 3 );
  for( int k = -1 ; k < 4 * n ; ++k ) {
   bool ref = false;
   for( auto & r : M ) ref = ref || std::count( r.begin() , r.end() , k );
   int steps;
   CHECK( search( M , k , steps ) == ref && steps <= 2 * n - 1 );
   }
  }
 // no entry, one, all equal: a hit at the first step, a miss below in n
 // steps left along the first row, one above in n steps down the last
 // column
 int steps;
 CHECK( ! search( Matrix< int >() , 3 , steps ) && steps == 0 );
 CHECK( search( Matrix< int >( 1 , Ints( 1 , 3 ) ) , 3 , steps ) &&
        steps == 1 );
 CHECK( ! search( Matrix< int >( 1 , Ints( 1 , 3 ) ) , 4 , steps ) );
 const Matrix< int > E( 6 , Ints( 6 , 2 ) );
 CHECK( search( E , 2 , steps ) && steps == 1 );
 CHECK( ! search( E , 1 , steps ) && steps == 6 );
 CHECK( ! search( E , 3 , steps ) && steps == 6 );
 }

// the tennis tournament of n = 2^k players: in the day j each player i
// meets M[ i ][ j ], who meets i; each pair meets once, in n - 1 days
void torneo( Matrix< int > & M , int imin , int imax , int jmin , int jmax ,
             int gmin , int gmax )
{
 if( gmin == gmax - 1 ) {
  M[ imin ][ jmin ] = gmin;
  M[ imin ][ jmax ] = gmax;
  M[ imax ][ jmin ] = gmax;
  M[ imax ][ jmax ] = gmin;
  return;
  }
 const int imed = ( imin + imax ) / 2 , jmed = ( jmin + jmax ) / 2 ,
           gmed = ( gmin + gmax ) / 2;
 torneo( M , imin , imed , jmin , jmed , gmin , gmed );
 torneo( M , imed + 1 , imax , jmed + 1 , jmax , gmin , gmed );
 torneo( M , imed + 1 , imax , jmin , jmed , gmed + 1 , gmax );
 torneo( M , imin , imed , jmed + 1 , jmax , gmed + 1 , gmax );
 }

ADS_TEST( "ex12.7" )
{
 for( int n : { 2 , 4 , 8 , 32 } ) {
  Matrix< int > M( n , Ints( n ) );
  torneo( M , 0 , n - 1 , 0 , n - 1 , 0 , n - 1 );
  std::set< std::pair< int , int > > met;
  for( int i = 0 ; i < n ; ++i ) {
   CHECK( M[ i ][ 0 ] == i );
   for( int j = 1 ; j < n ; ++j ) {
    const int o = M[ i ][ j ];
    CHECK( o != i && M[ o ][ j ] == i );            // a match of the day
    met.insert( { std::min( i , o ) , std::max( i , o ) } );
    }
   }
  CHECK( int( met.size() ) == n * ( n - 1 ) / 2 );
  }
 if( true ) {
  Matrix< int > M( 4 , Ints( 4 ) );
  torneo( M , 0 , 3 , 0 , 3 , 0 , 3 );
  CHECK( M == Matrix< int >( { { 0 , 1 , 2 , 3 } , { 1 , 0 , 3 , 2 } ,
                               { 2 , 3 , 0 , 1 } , { 3 , 2 , 1 , 0 } } ) );
  }
 }

// ( a + b i ) ( c + d i ) with three multiplications: k1 = c ( a + b ),
// k2 = a ( d - c ), k3 = b ( c + d ), real k1 - k3, imaginary k1 + k2; a
// zero, a real, an imaginary factor, and factors near 2^31
ADS_TEST( "ex12.8" )
{
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const long a = long( g() % 201 ) - 100 , b = long( g() % 201 ) - 100 ,
             c = long( g() % 201 ) - 100 , d = long( g() % 201 ) - 100;
  int mults = 0;
  auto mul = [ & ]( long x , long y ) { ++mults; return( x * y ); };
  const long k1 = mul( c , a + b ) , k2 = mul( a , d - c ) ,
             k3 = mul( b , c + d );
  const std::complex< long > ref = std::complex< long >( a , b ) *
                                   std::complex< long >( c , d );
  CHECK( k1 - k3 == ref.real() && k1 + k2 == ref.imag() && mults == 3 );
  }
 // zero, real and imaginary factors, i^2 = -1, and factors near 2^31
 // whose products need 64 bits
 using LL = long long;
 const LL H = 1LL << 31;
 for( auto [ a , b , c , d ] : { std::tuple( LL( 0 ) , LL( 0 ) , LL( 7 ) ,
                                             LL( -3 ) ) ,
                                 std::tuple( LL( 5 ) , LL( 0 ) , LL( -2 ) ,
                                             LL( 0 ) ) ,
                                 std::tuple( LL( 0 ) , LL( 1 ) , LL( 0 ) ,
                                             LL( 1 ) ) ,
                                 std::tuple( H , -H , H - 1 , H ) } ) {
  const LL k1 = c * ( a + b ) , k2 = a * ( d - c ) , k3 = b * ( c + d );
  CHECK( k1 - k3 == a * c - b * d && k1 + k2 == a * d + b * c );
  }
 }

// the zeros before the ones: a binary search for the first 1 in O( log n )
// and a galloping search in O( log h ), for h = 0 (all ones) to n (all
// zeros); the empty vector, empty and partial ranges, the contracts
ADS_TEST( "ex12.9" )
{
 for( int n : { 1 , 10 , 1000 , 100000 } )
  for( int h : { 0 , 1 , 2 , 7 , n / 2 , n } ) {
   if( h > n ) continue;
   Ints V( n , 0 );
   for( int i = h ; i < n ; ++i ) V[ i ] = 1;
   CHECK( int( lowerBound( V , 1 ) ) == h );
   long c = 0;
   CHECK( int( gallopSearch( V , 1 , [ & ]( int a , int b ) {
           c += ! checkingContract(); return( a < b ); } ) ) == h );
   CHECK( c <= 2 * long( std::log2( h + 1.0 ) ) + 3 );
   }
 // the empty vector: 0 for both, and the range [ lo , hi ) of lowerBound()
 // empty, or past the end, which breaks its contract
 const Ints E , V = { 0 , 0 , 1 , 1 };
 CHECK( lowerBound( E , 1 ) == 0 && gallopSearch( E , 1 ) == 0 );
 CHECK( lowerBound( V , 1 , 3 , 3 , std::less<>() ) == 3 );
 CHECK( lowerBound( V , 1 , 0 , 2 , std::less<>() ) == 2 );
 CHECK_THROWS( lowerBound( V , 1 , 3 , 2 , std::less<>() ) ,
               std::logic_error );
 CHECK_THROWS( lowerBound( V , 1 , 0 , 5 , std::less<>() ) ,
               std::logic_error );
 CHECK_THROWS( gallopSearch( Ints( { 1 , 0 } ) , 1 ) , std::logic_error );
 }

// the minimum of a matrix n x n, n a power of two, by its four quadrants:
// n^2 - 1 comparisons, as the scan; n = 1, the minimum in the last place,
// all equal
ADS_TEST( "ex12.10" )
{
 std::function< int( const Matrix< int > & , int , int , int , long & ) > mn =
  [ & ]( const Matrix< int > & M , int r , int c , int s , long & cmp ) {
   if( s == 1 ) return( M[ r ][ c ] );
   const int h = s / 2;
   int m = mn( M , r , c , h , cmp );
   for( auto [ dr , dc ] : { std::pair( 0 , h ) , std::pair( h , 0 ) ,
                             std::pair( h , h ) } ) {
    const int x = mn( M , r + dr , c + dc , h , cmp );
    ++cmp;
    m = std::min( m , x );
    }
   return( m );
   };
 std::mt19937 g( 12 );
 for( int n : { 1 , 2 , 8 , 64 } ) {
  Matrix< int > M( n , Ints( n ) );
  int ref = 1 << 30;
  for( auto & r : M )
   for( auto & x : r ) {
    x = int( g() % 1000 );
    ref = std::min( ref , x );
    }
  long cmp = 0;
  CHECK( mn( M , 0 , 0 , n , cmp ) == ref && cmp == long( n ) * n - 1 );
  // the minimum alone in the last place, then all equal
  M[ n - 1 ][ n - 1 ] = -1;
  cmp = 0;
  CHECK( mn( M , 0 , 0 , n , cmp ) == -1 && cmp == long( n ) * n - 1 );
  for( auto & r : M ) for( auto & x : r ) x = 3;
  CHECK( mn( M , 0 , 0 , n , cmp ) == 3 );
  }
 }

// the missing one of 0 .. n in a sorted V of n distinct items: the first
// place i with V[ i ] != i, by a binary search; n = 0 (V empty, 0
// missing), the first and the last missing
ADS_TEST( "ex12.11" )
{
 auto missing = []( const Ints & V ) {
  std::size_t lo = 0 , hi = V.size();
  while( lo < hi ) {
   const std::size_t m = lo + ( hi - lo ) / 2;
   if( V[ m ] == int( m ) ) lo = m + 1; else hi = m;
   }
  return( int( lo ) );
  };
 for( int n : { 0 , 1 , 2 , 9 , 1000 } ) {
  Ints xs = { n };                               // the last one missing
  for( int x = 0 ; x < n ; x += std::max( 1 , n / 7 ) ) xs.push_back( x );
  for( int x : xs ) {
   Ints V;
   for( int v = 0 ; v <= n ; ++v ) if( v != x ) V.push_back( v );
   CHECK( missing( V ) == x );
   }
  }
 }

// the maximum of a unimodal sequence: the first i with a_i > a_{i+1}, by a
// binary search on the sign of the step; one item, two, the peak at either
// end
ADS_TEST( "ex12.12" )
{
 auto peak = []( const Ints & a ) {
  std::size_t lo = 0 , hi = a.size() - 1;
  while( lo < hi ) {
   const std::size_t m = lo + ( hi - lo ) / 2;
   if( a[ m ] < a[ m + 1 ] ) lo = m + 1; else hi = m;
   }
  return( lo );
  };
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 50 ) , top = int( g() % n );
  Ints a( n );
  for( int i = 0 ; i < n ; ++i )         // increasing, then decreasing
   a[ i ] = i <= top ? i : -i;
  CHECK( int( peak( a ) ) == top );
  }
 // one item, two in both orders, only increasing, only decreasing
 CHECK( peak( Ints( { 4 } ) ) == 0 );
 CHECK( peak( Ints( { 1 , 2 } ) ) == 1 && peak( Ints( { 2 , 1 } ) ) == 0 );
 Ints up( 1000 ) , down( 1000 );
 std::iota( up.begin() , up.end() , -500 );
 std::iota( down.rbegin() , down.rend() , -500 );
 CHECK( peak( up ) == 999 && peak( down ) == 0 );
 }

// the occurrences of k in a sorted vector, upperBound() - lowerBound(), for
// k below, inside and above; empty, all equal, the audit on the order
ADS_TEST( "ex12.13" )
{
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints A( g() % 200 );
  for( auto & a : A ) a = int( g() % 20 );
  std::sort( A.begin() , A.end() );
  for( int k = -1 ; k <= 20 ; ++k )
   CHECK( long( upperBound( A , k ) - lowerBound( A , k ) ) ==
          std::count( A.begin() , A.end() , k ) );
  }
 // empty, all equal (all of them, none of a neighbour), and the audit on
 // an unsorted vector
 const Ints E , Q( 50 , 7 );
 CHECK( upperBound( E , 3 ) == 0 && lowerBound( E , 3 ) == 0 );
 CHECK( lowerBound( Q , 7 ) == 0 && upperBound( Q , 7 ) == 50 );
 CHECK( upperBound( Q , 6 ) == 0 && lowerBound( Q , 8 ) == 50 );
 CHECK_THROWS( upperBound( Ints( { 2 , 1 } ) , 1 ) , std::logic_error );
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
/*---------------------------- End File ch12.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
