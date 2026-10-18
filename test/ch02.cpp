/*--------------------------------------------------------------------------*/
/*----------------------------- File ch02.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 2, Analisi di algoritmi: the sum and the products of
 * binary integers of section 2.6, Selection Sort and Insertion Sort on the
 * figures, Merge Sort and Merge() on the figures and against the standard
 * library, the lower bound of the sorts by comparisons on the tree of
 * decision, Counting Sort, the hybrid Merge Sort, the solutions of the
 * recurrences of section 2.9 checked on their values, and the factorial
 * with the uniform cost.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/numeric/BinaryInteger.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/CountingSort.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/SelectionSort.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

/// the bits of a string written from the most significant one
Bits bits( const std::string & s )
{
 Bits b;
 for( auto c = s.rbegin() ; c != s.rend() ; ++c )
  b.push_back( *c - '0' );
 while( ( b.size() > 1 ) && ( b.back() == 0 ) ) b.pop_back();
 return( b );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// Figura 2.3: 01100110 + 01110011 = 11011001; the edges: empty operands
// (the value 0), 0 + 0, operands of different lengths, the carry through
// all the bits (1...1 + 1, 2^64 - 1 + 2^64 - 1, one bit more than 64), and
// an item that is not a bit (the audit of the precondition)
ADS_TEST( "sec2.6.binary-add" )
{
 CHECK( binaryAdd( bits( "01100110" ) , bits( "01110011" ) ) ==
        bits( "11011001" ) );
 std::mt19937_64 g( 3 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::uint64_t x = g() >> 2 , y = g() >> 2;
  CHECK( fromBits( binaryAdd( toBits( x ) , toBits( y ) ) ) == x + y );
  }
 CHECK( binaryAdd( Bits() , Bits() ).empty() );
 CHECK( binaryAdd( Bits() , bits( "101" ) ) == bits( "101" ) );
 CHECK( binaryAdd( bits( "101" ) , Bits() ) == bits( "101" ) );
 CHECK( fromBits( binaryAdd( { 0 } , { 0 } ) ) == 0 );
 CHECK( binaryAdd( bits( "1111111" ) , bits( "1" ) ) == bits( "10000000" ) );
 CHECK( binaryAdd( bits( "1" ) , bits( "1111111" ) ) == bits( "10000000" ) );
 const Bits M( 64 , 1 ) , S = binaryAdd( M , M );   // 2^65 - 2
 CHECK( S.size() == 65 && S[ 0 ] == 0 &&
        std::count( S.begin() , S.end() , 1 ) == 64 );
 CHECK_THROWS( (void) binaryAdd( { 0 , 2 } , { 1 } ) , contract_violation );
 CHECK_THROWS( (void) binaryAdd( { 1 } , { -1 } ) , contract_violation );
 }

// Figura 2.4: 01100110 x 01110011 = 010110111010010, by the school, by
// four products and by Karatsuba, which on n = 2^k bits make n^2, n^2 and
// 3^k products of bits; the edges: an empty operand (the product 0), the
// factors 0 and 1, operands of different lengths, not powers of 2, the
// all-ones operands (the most carries), and an item that is not a bit
ADS_TEST( "sec2.6.multiply" )
{
 const Bits X = bits( "01100110" ) , Y = bits( "01110011" ) ,
            P = bits( "010110111010010" );
 CHECK( multiplyBinarySchool( X , Y ) == P );
 CHECK( multiplyBinaryFour( X , Y ) == P );
 CHECK( multiplyBinaryKaratsuba( X , Y ) == P );
 std::mt19937 g( 4 );
 for( int k = 0 ; k <= 6 ; ++k ) {
  const std::size_t n = std::size_t( 1 ) << k;
  Bits A( n ) , B( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   A[ i ] = g() % 2;
   B[ i ] = g() % 2;
   }
  long s = 0 , f = 0 , c = 0;
  const auto r = multiplyBinarySchool( A , B , &s );
  CHECK( multiplyBinaryFour( A , B , &f ) == r );
  CHECK( multiplyBinaryKaratsuba( A , B , &c ) == r );
  CHECK( s == long( n * n ) && f == long( n * n ) );
  CHECK( c == long( std::pow( 3.0 , k ) ) );
  if( n <= 32 )
   CHECK( fromBits( r ) == fromBits( A ) * fromBits( B ) );
  }
 using F = Bits ( * )( const Bits & , const Bits & , long * );
 for( F mul : { F( multiplyBinarySchool ) , F( multiplyBinaryFour ) ,
                F( multiplyBinaryKaratsuba ) } ) {
  auto v = [ & ]( const Bits & a , const Bits & b ) {
   const Bits r = mul( a , b , nullptr );
   return( r.empty() ? std::uint64_t( 0 ) : fromBits( r ) ); };
  CHECK( v( Bits() , X ) == 0 && v( X , Bits() ) == 0 );
  CHECK( v( Bits() , Bits() ) == 0 );
  CHECK( v( { 0 } , X ) == 0 && v( X , { 0 } ) == 0 );
  CHECK( mul( { 1 } , X , nullptr ) == X && mul( X , { 1 } , nullptr ) == X );
  for( int t = 0 ; t < 200 ; ++t ) {        // lengths 1 .. 31, unequal
   const std::uint64_t a = g() >> ( 1 + g() % 31 ) ,
                       b = g() >> ( 1 + g() % 31 );
   CHECK( v( toBits( a ) , toBits( b ) ) == a * b );
   CHECK( v( toBits( a , 30 ) , toBits( b ) ) == a * b );   // leading 0s
   }
  for( std::size_t n : { 1 , 2 , 3 , 5 , 31 , 32 } ) {      // all ones
   const std::uint64_t a = ( std::uint64_t( 1 ) << n ) - 1;
   CHECK( v( Bits( n , 1 ) , Bits( n , 1 ) ) == a * a );
   }
  CHECK_THROWS( (void) mul( { 2 } , { 1 } , nullptr ) , contract_violation );
  CHECK_THROWS( (void) mul( { 1 } , { 0 , -1 } , nullptr ) ,
                contract_violation );
  }
 }

// Figura 2.8: Selection Sort on 7 4 2 1 8 3 5, the vector after each
// iteration; n ( n - 1 ) / 2 comparisons whatever the order; the edges:
// empty, one item, two items, all equal (still n ( n - 1 ) / 2), the
// extremes of int, against std::sort on random vectors with duplicates
ADS_TEST( "sec2.7.1.selection" )
{
 Ints A = { 7 , 4 , 2 , 1 , 8 , 3 , 5 };
 const std::vector< Ints > rows = { { 1 , 4 , 2 , 7 , 8 , 3 , 5 } ,
  { 1 , 2 , 4 , 7 , 8 , 3 , 5 } , { 1 , 2 , 3 , 7 , 8 , 4 , 5 } ,
  { 1 , 2 , 3 , 4 , 8 , 7 , 5 } , { 1 , 2 , 3 , 4 , 5 , 7 , 8 } ,
  { 1 , 2 , 3 , 4 , 5 , 7 , 8 } };
 for( std::size_t i = 0 ; i + 1 < A.size() ; ++i ) {
  std::swap( A[ i ] , A[ argmin( A , i , A.size() - 1 ) ] );
  CHECK( A == rows[ i ] );
  }
 for( Ints B : { Ints( { 1 , 2 , 3 , 4 , 5 } ) ,
                 Ints( { 5 , 4 , 3 , 2 , 1 } ) } ) {
  long c = 0;
  selectionSort( B , [ & ]( int a , int b ) {
   c += ! checkingContract(); return( a < b ); } );
  CHECK( std::is_sorted( B.begin() , B.end() ) && c == 10 );
  }
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract(); return( a < b ); };
 Ints E;
 selectionSort( E , count );
 CHECK( E.empty() && c == 0 );
 Ints O = { 4 };
 selectionSort( O , count );
 CHECK( O == Ints( { 4 } ) && c == 0 );
 Ints T = { 2 , 1 };
 selectionSort( T , count );
 CHECK( T == Ints( { 1 , 2 } ) && c == 1 );
 Ints Q( 6 , 3 );
 c = 0;
 selectionSort( Q , count );
 CHECK( Q == Ints( 6 , 3 ) && c == 15 );
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 Ints X = { hi , 0 , lo , hi , -1 , lo };
 selectionSort( X );
 CHECK( X == Ints( { lo , lo , -1 , 0 , hi , hi } ) );
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints R( g() % 40 );
  for( auto & r : R ) r = int( g() % 10 ) - 5;
  Ints S = R;
  selectionSort( R );
  std::sort( S.begin() , S.end() );
  CHECK( R == S );
  }
 }

// Figura 2.9: Insertion Sort on 7 4 2 1 8 3 5; n - 1 comparisons on a
// sorted vector, n ( n - 1 ) / 2 on a reversed one; stable; the edges:
// empty, one item, all equal (n - 1 comparisons, nothing moves), the
// extremes of int, the range version on an empty range, on the whole
// vector and on a part (the rest untouched), with the sentinel too, and
// its precondition lo <= hi <= n
ADS_TEST( "sec2.7.2.insertion" )
{
 Ints A = { 7 , 4 , 2 , 1 , 8 , 3 , 5 };
 insertionSort( A );
 CHECK( A == Ints( { 1 , 2 , 3 , 4 , 5 , 7 , 8 } ) );
 for( bool sorted : { true , false } ) {
  Ints B( 100 );
  std::iota( B.begin() , B.end() , 0 );
  if( ! sorted ) std::reverse( B.begin() , B.end() );
  long c = 0;
  insertionSort( B , [ & ]( int a , int b ) {
   c += ! checkingContract(); return( a < b ); } );
  CHECK( c == ( sorted ? 99 : 99 * 100 / 2 ) );
  }
 std::vector< std::pair< int , int > > P = { { 2 , 0 } , { 1 , 1 } ,
                                             { 2 , 2 } , { 1 , 3 } };
 insertionSort( P , []( auto & a , auto & b ) {
  return( a.first < b.first ); } );
 const std::vector< std::pair< int , int > > stable = {
  { 1 , 1 } , { 1 , 3 } , { 2 , 0 } , { 2 , 2 } };
 CHECK( P == stable );
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract(); return( a < b ); };
 Ints E;
 insertionSort( E , count );
 insertionSortSentinel( E );
 CHECK( E.empty() && c == 0 );
 Ints O = { 4 };
 insertionSort( O , count );
 insertionSortSentinel( O );
 CHECK( O == Ints( { 4 } ) && c == 0 );
 Ints Q( 8 , 3 );
 insertionSort( Q , count );
 CHECK( Q == Ints( 8 , 3 ) && c == 7 );
 std::vector< std::pair< int , int > > Z = { { 0 , 0 } , { 0 , 1 } ,
                                             { 0 , 2 } };
 insertionSort( Z , []( auto & a , auto & b ) {
  return( a.first < b.first ); } );
 CHECK( Z[ 0 ].second == 0 && Z[ 1 ].second == 1 && Z[ 2 ].second == 2 );
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 for( bool sentinel : { false , true } ) {
  Ints X = { hi , 0 , lo , hi , -1 , lo };
  if( sentinel ) insertionSortSentinel( X ); else insertionSort( X );
  CHECK( X == Ints( { lo , lo , -1 , 0 , hi , hi } ) );
  }
 Ints W = { 5 , 4 , 3 , 2 , 1 };
 insertionSort( W , 2 , 2 , std::less<>() );               // empty range
 CHECK( W == Ints( { 5 , 4 , 3 , 2 , 1 } ) );
 insertionSort( W , 1 , 4 , std::less<>() );               // a part
 CHECK( W == Ints( { 5 , 2 , 3 , 4 , 1 } ) );
 insertionSort( W , 0 , 5 , std::less<>() );               // the whole
 CHECK( W == Ints( { 1 , 2 , 3 , 4 , 5 } ) );
 insertionSort( W , 5 , 5 , std::less<>() );
 CHECK_THROWS( insertionSort( W , 3 , 2 , std::less<>() ) ,
               contract_violation );
 CHECK_THROWS( insertionSort( W , 0 , 6 , std::less<>() ) ,
               contract_violation );
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints R( g() % 40 );
  for( auto & r : R ) r = int( g() % 10 ) - 5;
  Ints S = R , U = R;
  insertionSortSentinel( R );
  std::sort( S.begin() , S.end() );
  insertionSort( U );
  CHECK( R == S && U == S );
  }
 }

// Figura 2.10 and 2.11; against std::stable_sort, with every cutoff, the
// bottom-up version and the version on lists; the edges: n = 0, 1, 2, 3,
// powers of 2 and their neighbours, all equal (stable), sorted (n - 1
// comparisons with a cutoff > 1) and reverse sorted, a cutoff of 0 (taken
// as 1) and larger than n, the extremes of int, mergeParts() on parts of
// one item and its preconditions; countInversions() on empty, sorted,
// all-equal and reverse sorted vectors
ADS_TEST( "sec2.7.3.merge-sort" )
{
 Ints A = { 33 , 21 , 7 , 48 , 28 , 13 , 65 , 17 };
 mergeSort( A );
 CHECK( A == Ints( { 7 , 13 , 17 , 21 , 28 , 33 , 48 , 65 } ) );
 Ints M = { 1 , 3 , 7 , 8 , 2 , 4 , 6 , 9 } , B( 8 );
 mergeParts( M , B , 0 , 3 , 7 );
 CHECK( M == Ints( { 1 , 2 , 3 , 4 , 6 , 7 , 8 , 9 } ) );
 std::mt19937 g( 5 );
 using P = std::pair< int , int >;
 auto byKey = []( const P & a , const P & b ) { return( a.first < b.first ); };
 for( int n : { 0 , 1 , 2 , 3 , 15 , 16 , 17 , 100 , 1000 , 1024 } )
  for( int keys : { 1 , 20 , 1 << 30 } ) {    // all equal to distinct
   std::vector< P > V( n );
   for( int i = 0 ; i < n ; ++i ) V[ i ] = { int( g() % keys ) , i };
   auto R = V;
   std::stable_sort( R.begin() , R.end() , byKey );
   for( std::size_t cutoff : { 0 , 1 , 4 , 64 , 5000 } ) {
    auto W = V;
    mergeSort( W , cutoff , byKey );
    CHECK( W == R );
    }
   auto W = V;
   mergeSortBottomUp( W , byKey );
   CHECK( W == R );
   List< P > L;
   for( const auto & v : V ) L.insert( L.next( L.tail() ) , v );
   mergeSort( L , byKey );
   std::vector< P > out;
   for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
    out.push_back( L.read( p ) );
   CHECK( out == R );
   }
 long c = 0;
 auto count = [ & ]( int a , int b ) {
  c += ! checkingContract(); return( a < b ); };
 Ints S( 100 );
 std::iota( S.begin() , S.end() , 0 );
 mergeSort( S , 4 , count );                  // sorted: n - 1 comparisons
 CHECK( c == 99 && std::is_sorted( S.begin() , S.end() ) );
 for( std::size_t cutoff : { 1 , 4 } ) {
  Ints D( 100 );
  std::iota( D.rbegin() , D.rend() , 0 );
  mergeSort( D , cutoff );
  CHECK( D == S );
  D.assign( S.rbegin() , S.rend() );
  mergeSortBottomUp( D );
  CHECK( D == S );
  }
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 Ints X = { hi , 0 , lo , hi , -1 , lo , 1 };
 Ints Y = X;
 mergeSort( X );
 mergeSortBottomUp( Y );
 CHECK( X == Ints( { lo , lo , -1 , 0 , 1 , hi , hi } ) && Y == X );
 Ints M2 = { 2 , 1 } , B2( 2 );
 mergeParts( M2 , B2 , 0 , 0 , 1 );           // parts of one item
 CHECK( M2 == Ints( { 1 , 2 } ) );
 Ints M3 = { 9 , 3 , 5 , 1 } , B3( 4 );
 mergeParts( M3 , B3 , 1 , 1 , 2 );           // a middle part only
 CHECK( M3 == Ints( { 9 , 3 , 5 , 1 } ) );
 CHECK_THROWS( mergeParts( M3 , B3 , 1 , 2 , 2 ) , contract_violation );
 CHECK_THROWS( mergeParts( M3 , B3 , 2 , 1 , 3 ) , contract_violation );
 CHECK_THROWS( mergeParts( M3 , B3 , 0 , 1 , 4 ) , contract_violation );
 Ints B1( 3 );
 CHECK_THROWS( mergeParts( M3 , B1 , 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( mergeParts( M3 , B3 , 0 , 1 , 2 ) , contract_violation );
 Ints I;
 CHECK( countInversions( I ) == 0 );
 I = { 7 };
 CHECK( countInversions( I ) == 0 );
 I = Ints( 10 , 2 );
 CHECK( countInversions( I ) == 0 );
 I = S;
 CHECK( countInversions( I ) == 0 );
 I.assign( S.rbegin() , S.rend() );
 CHECK( countInversions( I ) == 99 * 100 / 2 && I == S );
 for( int t = 0 ; t < 50 ; ++t ) {            // against the definition
  Ints J( g() % 30 );
  for( auto & j : J ) j = int( g() % 6 );
  long inv = 0;
  for( std::size_t a = 0 ; a < J.size() ; ++a )
   for( std::size_t b = a + 1 ; b < J.size() ; ++b )
    inv += J[ b ] < J[ a ];
  CHECK( countInversions( J ) == inv );
  }
 }

// Figura 2.14: the tree of decision of three items, at most 3 comparisons;
// any sort by comparisons needs ceil( log2 n! ) of them on some input:
// Merge Sort on all the permutations of n <= 8, from n = 1 (no comparison),
// whose worst case is exactly n ceil( log2 n ) - 2^ceil( log2 n ) + 1
ADS_TEST( "sec2.7.4.lower-bound" )
{
 auto sort3 = []( int a , int b , int c , int & k ) {
  ++k;
  if( a < b ) {
   ++k;
   if( b < c ) return( Ints( { a , b , c } ) );
   ++k;
   if( a < c ) return( Ints( { a , c , b } ) );
   return( Ints( { c , a , b } ) );
   }
  ++k;
  if( a < c ) return( Ints( { b , a , c } ) );
  ++k;
  if( b < c ) return( Ints( { b , c , a } ) );
  return( Ints( { c , b , a } ) );
  };
 Ints p = { 1 , 2 , 3 };
 do {
  int k = 0;
  CHECK( sort3( p[ 0 ] , p[ 1 ] , p[ 2 ] , k ) == Ints( { 1 , 2 , 3 } ) );
  CHECK( k <= 3 );
  } while( std::next_permutation( p.begin() , p.end() ) );
 for( int n = 1 ; n <= 8 ; ++n ) {
  Ints q( n );
  std::iota( q.begin() , q.end() , 0 );
  long worst = 0;
  do {
   Ints r = q;
   long c = 0;
   mergeSort( r , 1 , [ & ]( int a , int b ) {
   c += ! checkingContract(); return( a < b ); } );
   worst = std::max( worst , c );
   } while( std::next_permutation( q.begin() , q.end() ) );
  CHECK( worst >= long( std::ceil( std::lgamma( n + 1.0 ) / std::log( 2.0 )
                                   - 1e-9 ) ) );
  long l = 0;                                // ceil( log2 n )
  while( ( 1L << l ) < n ) ++l;
  CHECK( worst == n * l - ( 1L << l ) + 1 );  // the worst of Merge Sort
  }
 }

// Counting Sort, Theta( n + k ), against std::sort; the edges: empty (also
// with k = 0), one item, all equal, the keys 0 and k - 1, sorted and
// reverse sorted, unsigned keys, and the precondition (a key k or more,
// a negative key, any key with k = 0)
ADS_TEST( "sec2.7.5.counting-sort" )
{
 std::mt19937 g( 6 );
 for( int k : { 1 , 5 , 1000 } ) {
  Ints A( 500 );
  for( auto & a : A ) a = g() % k;
  Ints B = A;
  countingSort( A , k );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 Ints C = { 3 , 7 };
 CHECK_THROWS( countingSort( C , 5 ) , contract_violation );
 Ints E;
 countingSort( E , 0 );
 countingSort( E , 7 );
 CHECK( E.empty() );
 Ints O = { 4 };
 countingSort( O , 5 );
 CHECK( O == Ints( { 4 } ) );
 Ints Q( 9 , 2 );
 countingSort( Q , 3 );
 CHECK( Q == Ints( 9 , 2 ) );
 Ints K = { 9 , 0 , 9 , 0 , 5 };                  // the keys 0 and k - 1
 countingSort( K , 10 );
 CHECK( K == Ints( { 0 , 0 , 5 , 9 , 9 } ) );
 Ints D = { 4 , 3 , 2 , 1 , 0 };
 countingSort( D , 5 );
 CHECK( D == Ints( { 0 , 1 , 2 , 3 , 4 } ) );
 countingSort( D , 5 );
 CHECK( D == Ints( { 0 , 1 , 2 , 3 , 4 } ) );
 std::vector< unsigned > U = { 3u , 0u , 3u , 1u };
 countingSort( U , 4 );
 CHECK( U == std::vector< unsigned >( { 0u , 1u , 3u , 3u } ) );
 Ints N = { 1 , -1 };
 CHECK_THROWS( countingSort( N , 5 ) , contract_violation );
 Ints B = { 5 };
 CHECK_THROWS( countingSort( B , 5 ) , contract_violation );
 Ints Z = { 0 };
 CHECK_THROWS( countingSort( Z , 0 ) , contract_violation );
 }

// the recurrences of section 2.9 and their orders; the values of each
// recurrence at n = 2^k, divided by its order, settle to a constant; the
// edges: a = 1 (alpha = 0), beta = 0, k below -1 (no logarithm), a
// non-integer b, linear() with a = 1 and a = 2 at the boundary, and the
// preconditions a >= 1, b > 1
ADS_TEST( "sec2.9.recurrences" )
{
 auto same = []( Growth g , double c , double p , double q , double r ,
                 bool upper = false ) {
  return( std::abs( g.c - c ) < 1e-9 && std::abs( g.p - p ) < 1e-9 &&
          std::abs( g.q - q ) < 1e-9 && std::abs( g.r - r ) < 1e-9 &&
          g.upper == upper );
  };
 CHECK( same( master( 1 , 2 , 0 ) , 1 , 0 , 1 , 0 ) );        // binary search
 CHECK( same( master( 2 , 2 , 1 ) , 1 , 1 , 1 , 0 ) );        // Merge Sort
 CHECK( same( master( 3 , 2 , 1 ) , 1 , std::log2( 3.0 ) , 0 , 0 ) );
 CHECK( same( master( 1 , 2 , 1 ) , 1 , 1 , 0 , 0 ) );        // T(n/2) + n
 CHECK( same( master( 4 , 2 , 1 ) , 1 , 2 , 0 , 0 ) );
 CHECK( same( master( 4 , 2 , 2 ) , 1 , 2 , 1 , 0 ) );
 CHECK( same( master( 4 , 2 , 3 ) , 1 , 3 , 0 , 0 ) );
 CHECK( same( master( 7 , 2 , 2 ) , 1 , std::log2( 7.0 ) , 0 , 0 ) );
 CHECK( same( master( 2 , 2 , 1 , 1 ) , 1 , 1 , 2 , 0 ) );    // n log n
 CHECK( same( master( 2 , 2 , 1 , -1 ) , 1 , 1 , 0 , 1 ) );   // n / log n
 CHECK( same( linear( 1 , 1 ) , 1 , 2 , 0 , 0 ) );           // Quicksort
 CHECK( same( linear( 2 , 0 ) , 2 , 0 , 0 , 0 , true ) );    // Hanoi
 CHECK_THROWS( (void) master( 0.5 , 2 , 1 ) , contract_violation );
 CHECK( same( master( 1 , 2 , 0 , -2 ) , 1 , 0 , 0 , 0 ) );   // T = Theta(1)
 CHECK( same( master( 2 , 2 , 1 , -2 ) , 1 , 1 , 0 , 0 ) );   // n
 CHECK( same( master( 2 , 2 , 0 ) , 1 , 1 , 0 , 0 ) );        // tree visit
 CHECK( same( master( 1 , 2 , 1 , 3 ) , 1 , 1 , 3 , 0 ) );    // n log^3 n
 CHECK( same( master( 9 , 3 , 2 ) , 1 , 2 , 1 , 0 ) );
 CHECK( same( master( 2 , 1.5 , 1 ) , 1 , std::log( 2.0 ) /
                                         std::log( 1.5 ) , 0 , 0 ) );
 CHECK( same( master( 1 , 1.0001 , 0 ) , 1 , 0 , 1 , 0 ) );
 CHECK( same( linear( 1 , 0 ) , 1 , 1 , 0 , 0 ) );           // n
 CHECK( same( linear( 2 , 1 ) , 2 , 1 , 0 , 0 , true ) );
 CHECK_THROWS( (void) master( 2 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 2 , 0.5 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 0 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) linear( 0.5 , 1 ) , contract_violation );

 struct R { double a , b , beta , k; };
 for( const R & r : { R{ 2 , 2 , 1 , 0 } , R{ 3 , 2 , 1 , 0 } ,
                      R{ 4 , 2 , 2 , 0 } , R{ 1 , 2 , 1 , 0 } ,
                      R{ 2 , 2 , 1 , 1 } , R{ 4 , 2 , 1 , 0 } ,
                      R{ 2 , 2 , 1 , -1 } } ) {
  const Growth G = master( r.a , r.b , r.beta , r.k );
  std::vector< double > T = { 1 };                 // T( 2^0 ) = 1
  std::vector< double > ratio;
  for( int k = 1 ; k <= 40 ; ++k ) {
   const double n = std::pow( 2.0 , k );
   T.push_back( r.a * T.back() + std::pow( n , r.beta ) *
                std::pow( std::log( n ) , r.k ) );
   if( k >= 4 ) ratio.push_back( T.back() / G.at( n ) );
   }
  // the ratio of the last values changes by less than 5%
  const double last = ratio.back() , before = ratio[ ratio.size() - 6 ];
  CHECK( std::abs( last / before - 1 ) < 0.05 );
  }
 }

// the factorial with integers of 64 bits: 20! fits, 21! does not, so that
// n multiplications cost O( 1 ) each only up to n = 20; the product by 21
// wraps modulo 2^64
ADS_TEST( "sec2.10.factorial" )
{
 std::uint64_t f = 1;
 int n = 1;
 for( ; n < 30 ; ++n ) {
  if( f > std::numeric_limits< std::uint64_t >::max() /
          std::uint64_t( n + 1 ) )
   break;
  f *= std::uint64_t( n + 1 );
  }
 CHECK( n == 20 && f == 2432902008176640000ull );
 const std::uint64_t w = f * 21;               // 21! wraps modulo 2^64
 CHECK( w / 21 != f && w == 14197454024290336768ull );
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
/*---------------------------- End File ch02.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
