/*--------------------------------------------------------------------------*/
/*------------------------- File contract_sort.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the sorts: the results as the
 * specifications of the book state them, and the violations of the
 * preconditions, which must throw ADS::contract_violation. The tests are
 * compiled at the level 3, so that each call also checks its postconditions
 * and its audits, on random instances against a reference of the standard
 * library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 2.7 and
 *      12.2, https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
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

#include "ADS/sequence/List.h"
#include "ADS/sort/CountingSort.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/ExchangeSorts.h"
#include "ADS/sort/ExternalSort.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/KnuthSorts.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/PigeonholeSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/SelectionSort.h"
#include "ADS/sort/ShellSort.h"
#include "ADS/sort/Sorted.h"
#include "ADS/sort/SortingNetworks.h"
#include "ADS/sort/StrandSort.h"
#include "ADS/sort/TreeSort.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <functional>
#include <limits>
#include <random>
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

/// a random vector of n items in [ 0 , k )
Ints random_ints( std::size_t n , int k , std::mt19937 & g )
{
 Ints A( n );
 for( auto & a : A ) a = int( g() % k );
 return( A );
 }

/// the items of a List
Ints items( const List< int > & L )
{
 Ints v;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  v.push_back( L.read( p ) );
 return( v );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// every sort, in increasing and in decreasing order, against std::sort,
// each checking that its output is sorted and holds the same items; the
// inversions against the enumeration of the pairs
ADS_TEST( "sorts" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const Ints A = random_ints( g() % 120 , 1 + int( g() % 40 ) , g );
  for( bool up : { true , false } ) {
   const std::function< bool( int , int ) > less = up ?
    std::function< bool( int , int ) >( std::less<>() ) :
    std::function< bool( int , int ) >( std::greater<>() );
   Ints S = A;
   std::sort( S.begin() , S.end() , less );
   auto check = [ & ]( auto && sort ) {
    Ints B = A;
    sort( B );
    CHECK( B == S );
    };
   check( [ & ]( Ints & B ) { selectionSort( B , less ); } );
   check( [ & ]( Ints & B ) { insertionSort( B , less ); } );
   check( [ & ]( Ints & B ) { mergeSort( B , 1 , less ); } );
   check( [ & ]( Ints & B ) { mergeSort( B , 8 , less ); } );
   check( [ & ]( Ints & B ) { mergeSortBottomUp( B , less ); } );
   check( [ & ]( Ints & B ) { heapsort( B , less ); } );
   check( [ & ]( Ints & B ) { quickSort( B , less ); } );
   check( [ & ]( Ints & B ) { quickSortTuned( B , 16 , less ); } );
   check( [ & ]( Ints & B ) { shellSort( B , less ); } );
   check( [ & ]( Ints & B ) {
    shellSort( B , ciuraGaps( B.size() ) , less ); } );
   check( [ & ]( Ints & B ) {
    shellSort( B , prattGaps( B.size() ) , less ); } );
   long inv = 0;
   for( std::size_t i = 0 ; i < A.size() ; ++i )
    for( std::size_t j = i + 1 ; j < A.size() ; ++j )
     inv += less( A[ j ] , A[ i ] );
   Ints B = A;
   CHECK( countInversions( B , less ) == inv && B == S );
   for( bool merge : { true , false } ) {
    List< int > L;
    for( int a : A ) L.insert( L.next( L.tail() ) , a );
    if( merge ) mergeSort( L , less ); else quickSort( L , less );
    CHECK( items( L ) == S );
    }
   }
  }
 }

// the ranges of insertionSort(), partition(), quickSort(),
// maxHeapRestore(), isMaxHeap(), mergeParts() and mergeSortRec() out of
// their contracts, and two unsorted halves given to mergeParts()
ADS_TEST( "ranges" )
{
 Ints A = { 5 , 3 , 8 , 1 };
 Ints B( 4 );
 CHECK_THROWS( insertionSort( A , 3 , 2 ) , contract_violation );
 CHECK_THROWS( insertionSort( A , 0 , 5 ) , contract_violation );
 CHECK_THROWS( (void) partition( A , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) partition( A , 0 , 4 ) , contract_violation );
 CHECK_THROWS( quickSort( A , 0 , 4 ) , contract_violation );
 quickSort( A , 2 , 1 );                      // an empty range
 CHECK_THROWS( maxHeapRestore( A , 2 , 2 ) , contract_violation );
 CHECK_THROWS( maxHeapRestore( A , 0 , 5 ) , contract_violation );
 CHECK_THROWS( (void) isMaxHeap( A , 5 ) , contract_violation );
 CHECK_THROWS( mergeParts( A , B , 0 , 3 , 3 ) , contract_violation );
 CHECK_THROWS( mergeParts( A , B , 0 , 1 , 3 ) , contract_violation );
 Ints small( 2 );
 CHECK_THROWS( mergeParts( A , small , 0 , 1 , 3 ) , contract_violation );
 CHECK_THROWS( mergeSortRec( A , B , 0 , 3 , 0 ) , contract_violation );
 CHECK_THROWS( mergeSortRec( A , B , 0 , 4 , 1 ) , contract_violation );
 Ints H = { 1 , 3 , 2 , 4 };                  // sorted halves
 mergeParts( H , B , 0 , 1 , 3 );
 CHECK( H == Ints( { 1 , 2 , 3 , 4 } ) );
 Ints P = { 9 , 4 , 7 , 1 , 8 };
 heapBuild( P );
 CHECK( isMaxHeap( P , P.size() ) );
 }

// countingSort() and pigeonholeSort() against std::sort and
// std::stable_sort; a key out of [ 0 , k ) violates their contracts
ADS_TEST( "counting-pigeonhole" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int k = 1 + int( g() % 20 );
  Ints A = random_ints( g() % 80 , k , g ) , S = A;
  std::sort( S.begin() , S.end() );
  countingSort( A , std::size_t( k ) );
  CHECK( A == S );
  std::vector< std::pair< int , int > > P;
  for( int i = 0 ; i < int( g() % 50 ) ; ++i )
   P.emplace_back( int( g() % k ) , i );
  auto Q = P;
  std::stable_sort( Q.begin() , Q.end() ,
                    []( const auto & a , const auto & b ) {
                     return( a.first < b.first ); } );
  CHECK( pigeonholeSort( P , k ) == Q );
  }
 Ints C = { 3 , 7 } , N = { 1 , -1 };
 CHECK_THROWS( countingSort( C , 5 ) , contract_violation );
 CHECK_THROWS( countingSort( N , 5 ) , contract_violation );
 const std::vector< std::pair< int , char > > R = { { 0 , 'a' } ,
                                                    { 4 , 'b' } };
 CHECK_THROWS( (void) pigeonholeSort( R , 4 ) , contract_violation );
 CHECK_THROWS( (void) pigeonholeSort(
  std::vector< std::pair< int , char > >( { { -1 , 'a' } } ) , 4 ) ,
               contract_violation );
 }

// the four sequences of gaps start from 1 and increase for every n; the
// gaps given to shellSort() must be positive and increasing, and a single
// gap h only h-sorts the vector, keeping its items
ADS_TEST( "shell-gaps" )
{
 for( std::size_t n = 0 ; n < 3000 ; n += 1 + n / 7 ) {
  for( const auto & h : { knuthGaps( n ) , shellGaps( n ) , prattGaps( n ) ,
                          ciuraGaps( n ) } ) {
   CHECK( ! h.empty() && h[ 0 ] == 1 );
   CHECK( std::adjacent_find( h.begin() , h.end() ,
                              std::greater_equal<>() ) == h.end() );
   }
  }
 Ints A = { 5 , 4 , 3 , 2 , 1 };
 using Gaps = std::vector< std::size_t >;
 CHECK_THROWS( shellSort( A , Gaps() ) , contract_violation );
 CHECK_THROWS( shellSort( A , Gaps( { 0 , 1 } ) ) , contract_violation );
 CHECK_THROWS( shellSort( A , Gaps( { 1 , 4 , 2 } ) ) , contract_violation );
 Ints B = { 4 , 3 , 2 , 1 };
 shellSort( B , Gaps( { 2 } ) );                 // 2-sorted only
 CHECK( B == Ints( { 2 , 1 , 4 , 3 } ) );
 shellSort( B , Gaps( { 1 , 2 } ) );
 CHECK( B == Ints( { 1 , 2 , 3 , 4 } ) );
 }

// the sorts of the other headers on the degenerate vectors (empty, one
// item, two, all equal, sorted, reversed, the extremes of the type), each
// checking its audit, against std::sort; their preconditions just inside
// and just outside: replacementSelection() with M = 1 and 0,
// polyphaseMerge() with 3 and 2 tapes and an unsorted
// run, partitionByValue() on a range of one item and out of the vector,
// bitonicNetwork() on 0 , 1 , 2 and 3, applyNetwork() with a comparator
// on n - 1 and on n, countingSort() with a key just inside and just
// outside [ 0 , k )
ADS_TEST( "other-sorts" )
{
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 std::vector< Ints > cases = { {} , { 7 } , { 2 , 1 } , Ints( 9 , 4 ) ,
                               { 1 , 2 , 3 , 4 , 5 , 6 } ,
                               { 6 , 5 , 4 , 3 , 2 , 1 } ,
                               { hi , lo , 0 , -1 , hi , lo , 1 } };
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 20 ; ++t )
  cases.push_back( random_ints( g() % 70 , 1 + int( g() % 30 ) , g ) );
 for( const Ints & A : cases ) {
  Ints S = A;
  std::sort( S.begin() , S.end() );
  auto check = [ & ]( auto && sort ) {
   Ints B = A;
   sort( B );
   CHECK( B == S );
   };
  check( []( Ints & B ) { pdqSort( B ); } );
  check( []( Ints & B ) { dualPivotQuickSort( B ); } );
  check( []( Ints & B ) { threeWayQuickSort( B ); } );
  check( []( Ints & B ) { bubbleSort( B ); } );
  check( []( Ints & B ) { cocktailShakerSort( B ); } );
  check( []( Ints & B ) { oddEvenSort( B ); } );
  check( []( Ints & B ) { combSort( B ); } );
  check( []( Ints & B ) { gnomeSort( B ); } );
  check( []( Ints & B ) { binaryInsertionSort( B ); } );
  check( []( Ints & B ) { listInsertionSort( B ); } );
  check( []( Ints & B ) { addressCalculationSort( B ); } );
  check( []( Ints & B ) { addressCalculationSort( B , 1 ); } );
  check( []( Ints & B ) { radixExchangeSort( B ); } );
  check( []( Ints & B ) { tournamentSort( B ); } );
  check( []( Ints & B ) { mergeInsertionSort( B ); } );
  check( []( Ints & B ) { radixSort( B ); } );
  check( []( Ints & B ) { americanFlagSort( B ); } );
  check( []( Ints & B ) { powerSort( B ); } );
  check( []( Ints & B ) { treeSort( B ); } );
  check( []( Ints & B ) { treeSort( B , false ); } );
  check( []( Ints & B ) { heapsort( B ); } );
  check( []( Ints & B ) { quickSortTuned( B , 1 ); } );
  check( [ & ]( Ints & B ) {
   B = externalSort( B , 1 , 3 ); } );
  check( [ & ]( Ints & B ) {
   B = externalSort( B , 4 , 5 ); } );
  check( []( Ints & B ) {
   std::size_t n = 1;
   while( n < B.size() ) n *= 2;
   std::vector< int > P = B;
   P.resize( n , std::numeric_limits< int >::max() );
   applyNetwork( P , bitonicNetwork( n ) );
   P.resize( B.size() );
   B = P; } );
  check( []( Ints & B ) { applyNetwork( B , mergeExchangeNetwork( B.size() ) );
   } );
  List< int > L;
  for( int a : A ) L.insert( L.next( L.tail() ) , a );
  strandSort( L );
  CHECK( items( L ) == S );
  }
 // floating point keys: -0.0, the infinities, the extremes of double
 const double inf = std::numeric_limits< double >::infinity() ,
              big = std::numeric_limits< double >::max();
 const std::vector< double > F = { 1.5 , -inf , big , -0.0 , -big , inf ,
                                   0.0 , -2.5 , std::nextafter( 0.0 , 1.0 ) };
 auto FS = F;
 std::sort( FS.begin() , FS.end() );
 for( int s = 0 ; s < 3 ; ++s ) {
  auto B = F;
  if( s == 0 ) radixSort( B );
  else if( s == 1 ) americanFlagSort( B );
  else addressCalculationSort( B );
  CHECK( std::is_sorted( B.begin() , B.end() ) );
  CHECK( std::is_permutation( B.begin() , B.end() , FS.begin() ) );
  }
 std::vector< std::string > W = { "b" , "" , "ab" , "a" , "" , "abc" ,
                                  "b" } , WS = W;
 std::sort( WS.begin() , WS.end() );
 auto W1 = W , W2 = W;
 multikeyQuickSort( W1 );
 msdRadixSort( W2 );
 CHECK( W1 == WS && W2 == WS );
 std::vector< std::string > W0;
 multikeyQuickSort( W0 );
 msdRadixSort( W0 );
 CHECK( W0.empty() );

 // the preconditions
 const Ints In = { 5 , 1 , 4 , 2 , 3 };
 CHECK( replacementSelection( In , 1 ).size() == 3 );
 CHECK( replacementSelection( Ints() , 1 ).empty() );
 CHECK_THROWS( (void) replacementSelection( In , 0 ) , contract_violation );
 CHECK_THROWS( (void) externalSort( In , 0 , 3 ) , contract_violation );
 CHECK_THROWS( (void) externalSort( In , 2 , 2 ) , contract_violation );
 const std::vector< Ints > runs = { { 1 , 5 } , { 2 } , { 0 , 3 , 9 } };
 CHECK( polyphaseMerge( runs , 3 ) == Ints( { 0 , 1 , 2 , 3 , 5 , 9 } ) );
 CHECK( polyphaseMerge( std::vector< Ints >() , 3 ).empty() );
 CHECK( polyphaseMerge( std::vector< Ints >{ {} , { 4 } } , 3 ) ==
        Ints( { 4 } ) );
 CHECK_THROWS( (void) polyphaseMerge( runs , 2 ) , contract_violation );
 CHECK_THROWS( (void) polyphaseMerge( std::vector< Ints >{ { 2 , 1 } } , 3 ) ,
               contract_violation );
 Ints V = { 4 , 1 , 3 };
 CHECK( partitionByValue( V , 1 , 1 , 2 ) == 1 &&
        partitionByValue( V , 2 , 2 , 5 ) == 1 );
 CHECK( partitionByValue( V , 0 , 2 , 3 ) == 2 && V[ 2 ] == 4 );
 CHECK_THROWS( (void) partitionByValue( V , 2 , 1 , 3 ) , contract_violation );
 CHECK_THROWS( (void) partitionByValue( V , 0 , 3 , 3 ) , contract_violation );
 CHECK( bitonicNetwork( 0 ).empty() && bitonicNetwork( 1 ).empty() &&
        bitonicNetwork( 2 ).size() == 1 );
 CHECK_THROWS( (void) bitonicNetwork( 3 ) , contract_violation );
 CHECK_THROWS( (void) bitonicNetwork( 6 ) , contract_violation );
 CHECK( mergeExchangeNetwork( 0 ).empty() &&
        mergeExchangeNetwork( 1 ).empty() );
 Ints N3 = { 3 , 2 , 1 };
 applyNetwork( N3 , { { 0 , 2 } } );
 CHECK( N3 == Ints( { 1 , 2 , 3 } ) );
 CHECK_THROWS( applyNetwork( N3 , { { 0 , 3 } } ) , contract_violation );
 CHECK_THROWS( applyNetwork( N3 , { { 3 , 0 } } ) , contract_violation );
 Ints E0;
 applyNetwork( E0 , {} );
 CHECK_THROWS( applyNetwork( E0 , { { 0 , 0 } } ) , contract_violation );
 Ints K = { 4 , 0 , 4 };
 countingSort( K , 5 );                       // the key k - 1 just inside
 CHECK( K == Ints( { 0 , 4 , 4 } ) );
 Ints K0;
 countingSort( K0 , 0 );                      // k = 0 and no key
 Ints K1 = { 0 };
 CHECK_THROWS( countingSort( K1 , 0 ) , contract_violation );
 CHECK( pigeonholeSort( std::vector< std::pair< int , char > >() , 0 )
        .empty() );
 CHECK_THROWS( (void) pigeonholeSort(
  std::vector< std::pair< int , char > >() , -1 ) , contract_violation );
 CHECK( pigeonholeSort( std::vector< std::pair< int , char > >(
                         { { 3 , 'a' } } ) , 4 ).size() == 1 );
 }

// a comparator that counts its calls skips those of the contracts, which
// are many at the level 3: Selection Sort still makes n ( n - 1 ) / 2
ADS_TEST( "checking-contract" )
{
 CHECK( ! checkingContract() );
 for( std::size_t n : { 0u , 1u , 5u , 40u } ) {
  Ints A( n );
  for( std::size_t i = 0 ; i < n ; ++i ) A[ i ] = int( n - i );
  long c = 0 , all = 0;
  selectionSort( A , [ & ]( int a , int b ) {
   ++all;
   c += ! checkingContract();
   return( a < b ); } );
  CHECK( c == long( n * ( n ? n - 1 : 0 ) / 2 ) );
  CHECK( all >= c && ( n < 2 || all > c ) );
  }
 CHECK( ! checkingContract() );
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
/*----------------------- End File contract_sort.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
