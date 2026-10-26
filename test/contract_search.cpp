/*--------------------------------------------------------------------------*/
/*------------------------ File contract_search.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the searches, the minimum and the
 * selection: the results as the specifications of the book state them, and the
 * violations of the preconditions, which must throw ADS::contract_violation.
 * The tests are compiled at the level 3, so that each call also checks its
 * postconditions and its audits, on random instances against a reference of
 * the standard library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.2 and
 *      12.1, https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
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

#include "ADS/search/BinarySearch.h"
#include "ADS/search/InterpolationSearch.h"
#include "ADS/search/KnuthSearches.h"
#include "ADS/search/LocalOptima.h"
#include "ADS/search/Minimum.h"
#include "ADS/search/MonotoneSearch.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/search/Selection.h"
#include "ADS/search/SortedSearches.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;

/// a random vector of n items in [ 0 , k )
Ints random_ints( std::size_t n , int k , std::mt19937 & g )
{
 Ints A( n );
 for( auto & a : A ) a = int( g() % k );
 return( A );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the binary searches on sorted vectors against std::find,
// std::lower_bound and std::upper_bound; an unsorted vector and a range
// out of [ 0 , n ] violate their contracts, lo = hi = n is just inside;
// the empty vector, one item, all items equal, the extremes of int; the
// searches of Knuth and the decision tree refuse an unsorted vector
ADS_TEST( "binary-search" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Ints A = random_ints( g() % 40 , 30 , g );
  std::sort( A.begin() , A.end() );
  const int v = int( g() % 32 ) - 1;
  const long r = binarySearch( A , v );
  CHECK( ( r < 0 ) == ( std::find( A.begin() , A.end() , v ) == A.end() ) );
  if( r >= 0 ) CHECK( A[ r ] == v );
  const auto lb = std::size_t( std::lower_bound( A.begin() , A.end() , v ) -
                               A.begin() );
  CHECK( lowerBound( A , v ) == lb && gallopSearch( A , v ) == lb );
  CHECK( upperBound( A , v ) == std::size_t(
         std::upper_bound( A.begin() , A.end() , v ) - A.begin() ) );
  const std::size_t lo = A.empty() ? 0 : g() % A.size();
  const std::size_t hi = lo + g() % ( A.size() - lo + 1 );
  const auto lr = std::lower_bound( A.begin() + lo , A.begin() + hi , v );
  CHECK( lowerBound( A , v , lo , hi ) == std::size_t( lr - A.begin() ) );
  }
 const Ints U = { 3 , 1 , 2 };
 CHECK_THROWS( (void) binarySearch( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) lowerBound( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) upperBound( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) gallopSearch( U , 2 ) , contract_violation );
 const Ints S = { 1 , 2 , 3 };
 CHECK_THROWS( (void) lowerBound( S , 2 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) lowerBound( S , 2 , 0 , 4 ) , contract_violation );
 CHECK( lowerBound( S , 2 , 1 , 1 ) == 1 );
 CHECK( lowerBound( S , 2 , 3 , 3 ) == 3 );     // lo = hi = n: just inside
 CHECK( lowerBound( S , 9 , 0 , 3 ) == 3 && lowerBound( S , 0 ) == 0 );
 CHECK_THROWS( (void) lowerBound( S , 2 , 4 , 4 ) , contract_violation );
 CHECK_THROWS( (void) lowerBound( U , 2 , 0 , 2 ) , contract_violation );
 CHECK( lowerBound( U , 9 , 1 , 3 ) == 3 );    // only A[ lo .. hi ) sorted
 const Ints E;                                 // empty, one item, all equal
 CHECK( binarySearch( E , 1 ) == -1 && lowerBound( E , 1 ) == 0 &&
        upperBound( E , 1 ) == 0 && gallopSearch( E , 1 ) == 0 );
 CHECK( binarySearch( Ints( { 4 } ) , 4 ) == 0 &&
        binarySearch( Ints( { 4 } ) , 5 ) == -1 );
 const Ints Q( 9 , 7 );
 CHECK( lowerBound( Q , 7 ) == 0 && upperBound( Q , 7 ) == 9 &&
        gallopSearch( Q , 7 ) == 0 && Q[ binarySearch( Q , 7 ) ] == 7 );
 // the extreme values of the type
 const Ints X = { std::numeric_limits< int >::min() , 0 ,
                  std::numeric_limits< int >::max() };
 CHECK( binarySearch( X , std::numeric_limits< int >::max() ) == 2 &&
        binarySearch( X , std::numeric_limits< int >::min() ) == 0 &&
        interpolationSearch( X , std::numeric_limits< int >::max() ) == 2 &&
        interpolationSearch( X , 1 ) == -1 );
 // Knuth's searches and the decision tree: the audit of the order
 long cmp = 0;
 CHECK( fibonacciSearch( E , 1 ) == -1 && uniformBinarySearch( E , 1 ) == -1 );
 CHECK( fibonacciSearch( S , 3 , &cmp ) == 2 && cmp > 0 &&
        uniformBinarySearch( S , 1 ) == 0 && fibonacciSearch( S , 4 ) == -1 );
 CHECK_THROWS( (void) fibonacciSearch( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) uniformBinarySearch( U , 2 ) , contract_violation );
 CHECK( decisionTree( E ) == nullptr );
 delete decisionTree( S );
 CHECK_THROWS( delete decisionTree( U ) , contract_violation );
 }

// the search by interpolation against std::find; an unsorted vector
// violates its contract
ADS_TEST( "interpolation-search" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Ints A = random_ints( g() % 40 , 50 , g );
  std::sort( A.begin() , A.end() );
  const int v = int( g() % 52 ) - 1;
  const long r = interpolationSearch( A , v );
  CHECK( ( r < 0 ) == ( std::find( A.begin() , A.end() , v ) == A.end() ) );
  if( r >= 0 ) CHECK( A[ r ] == v );
  }
 CHECK_THROWS( (void) interpolationSearch( Ints( { 5 , 1 , 9 } ) , 9 ) ,
               contract_violation );
 }

// argmin(), min() and the two minMax() against std::min_element; a range
// out of the vector and an empty vector violate their contracts, a range
// of one item and one item are just inside; the first of equal minima,
// the extremes of int
ADS_TEST( "minimum" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Ints A = random_ints( 1 + g() % 30 , 20 , g );
  const std::size_t s = g() % A.size() , e = s + g() % ( A.size() - s );
  const auto m = argmin( A , s , e );
  CHECK( m >= s && m <= e &&
         A[ m ] == *std::min_element( A.begin() + s , A.begin() + e + 1 ) );
  CHECK( min( A ) == *std::min_element( A.begin() , A.end() ) );
  }
 const Ints A = { 4 , 2 , 7 };
 CHECK_THROWS( (void) argmin( A , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) argmin( A , 0 , 3 ) , contract_violation );
 CHECK_THROWS( (void) min( Ints() ) , contract_violation );
 CHECK( argmin( A , 2 , 2 ) == 2 && argmin( A , 0 , 2 ) == 1 );  // inside
 CHECK_THROWS( (void) argmin( Ints() , 0 , 0 ) , contract_violation );
 CHECK( argmin( Ints( { 1 , 0 , 0 } ) , 0 , 2 ) == 1 );   // the first tie
 CHECK_THROWS( (void) minMax( Ints() ) , contract_violation );
 CHECK_THROWS( (void) minMaxDivide( Ints() ) , contract_violation );
 CHECK( minMax( Ints( { 5 } ) ) == std::pair( 5 , 5 ) &&
        minMaxDivide( Ints( { 5 } ) ) == std::pair( 5 , 5 ) );
 CHECK( minMax( Ints( { 5 , 3 } ) ) == std::pair( 3 , 5 ) &&
        minMaxDivide( Ints( { 3 , 5 } ) ) == std::pair( 3 , 5 ) );
 const Ints X = { std::numeric_limits< int >::max() , 0 ,
                  std::numeric_limits< int >::min() };
 CHECK( minMax( X ) == std::pair( X[ 2 ] , X[ 0 ] ) &&
        minMaxDivide( X ) == minMax( X ) && min( X ) == X[ 2 ] );
 }

// the local optima, the monotone searches, the range minima and the
// searches in sorted vectors: every precondition just inside and just
// outside, an empty vector, one item, all items equal, the extremes of the
// type, and a comparator that is not an order, which the postcondition of
// findPeak() catches
ADS_TEST( "local-and-sorted" )
{
 CHECK_THROWS( (void) findPeak( Ints() ) , contract_violation );
 CHECK_THROWS( (void) localMinimum( Ints() ) , contract_violation );
 CHECK( findPeak( Ints( { 3 } ) ) == 0 && localMinimum( Ints( { 3 } ) ) == 0 );
 CHECK( findPeak( Ints( { 1 , 2 } ) ) == 1 &&
        localMinimum( Ints( { 1 , 2 } ) ) == 0 );
 const Ints Q( 6 , 4 );
 CHECK( findPeak( Q ) < 6 && localMinimum( Q ) < 6 );
 const auto always = []( int , int ) { return( true ); };
 CHECK_THROWS( (void) findPeak( Ints( { 1 , 2 } ) , always ) ,
               contract_violation );
 CHECK_THROWS( (void) localMinimum( (BinaryTree< int > *) nullptr ) ,
               contract_violation );
 {
  auto * t = new BinaryTree< int >( 5 );
  CHECK( localMinimum( t ) == t );            // one node
  t->insertLeft( new BinaryTree< int >( 7 ) );
  t->insertRight( new BinaryTree< int >( 2 ) );
  CHECK( localMinimum( t ) == t->right() );
  delete t;
  }

 // exponentialSearch(): p true somewhere in [ lo , max ]
 using SC = signed char;
 constexpr SC top = std::numeric_limits< SC >::max();
 CHECK( exponentialSearch( []( SC i ) { return( i == top ); } , SC( 0 ) ) ==
        top );
 CHECK( exponentialSearch( []( SC ) { return( true ); } , top ) == top );
 CHECK( exponentialSearch( []( SC ) { return( true ); } ,
                           std::numeric_limits< SC >::min() ) ==
        std::numeric_limits< SC >::min() );
 CHECK_THROWS( exponentialSearch( []( SC ) { return( false ); } , SC( 0 ) ) ,
               contract_violation );
 CHECK_THROWS( exponentialSearch( []( SC ) { return( false ); } , top ) ,
               contract_violation );
 CHECK( integerSqrt( 0u ) == 0 && integerSqrt( 1u ) == 1 &&
        integerSqrt( std::numeric_limits< unsigned >::max() ) == 65535u &&
        integerSqrt( std::numeric_limits< unsigned long long >::max() ) ==
        4294967295ull );

 // the range minima: i <= j < n, just inside and just outside
 const Ints A = { 3 , 1 , 4 , 1 , 5 };
 SparseTable< int > T( A );
 RangeMinimum< int > R( A );
 CHECK( T.query( 0 , 4 ) == 1 && R.query( 0 , 4 ) == 1 );   // leftmost
 CHECK( T.query( 4 , 4 ) == 4 && R.query( 4 , 4 ) == 4 );
 CHECK( T[ 4 ] == 5 && R[ 4 ] == 5 && T.query( 2 , 3 ) == 3 );
 CHECK_THROWS( (void) T.query( 3 , 2 ) , contract_violation );
 CHECK_THROWS( (void) T.query( 0 , 5 ) , contract_violation );
 CHECK_THROWS( (void) T[ 5 ] , contract_violation );
 CHECK_THROWS( (void) R.query( 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) R.query( 5 , 5 ) , contract_violation );
 CHECK_THROWS( (void) R[ 5 ] , contract_violation );
 SparseTable< int > T0( Ints{} );
 RangeMinimum< int > R0( Ints{} );
 CHECK_THROWS( (void) T0.query( 0 , 0 ) , contract_violation );
 CHECK_THROWS( (void) R0.query( 0 , 0 ) , contract_violation );
 CHECK_THROWS( (void) T0[ 0 ] , contract_violation );
 const PlusMinusOneRangeMinimum P( Longs{ 0 , 1 , 0 , -1 , 0 , -1 } );
 CHECK( P.query( 0 , 5 ) == 3 && P.query( 5 , 5 ) == 5 &&
        P.query( 0 , 1 ) == 0 );
 CHECK_THROWS( (void) P.query( 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) P.query( 0 , 6 ) , contract_violation );
 CHECK_THROWS( PlusMinusOneRangeMinimum( Longs{ 0 , 1 , 3 } ) ,
               contract_violation );
 CHECK_THROWS( PlusMinusOneRangeMinimum( Longs{ 0 , 0 } ) ,
               contract_violation );
 CHECK( PlusMinusOneRangeMinimum( Longs{} ).size() == 0 );
 CHECK( PlusMinusOneRangeMinimum( Longs{ 7 } ).query( 0 , 0 ) == 0 );

 // the searches in sorted vectors
 CHECK_THROWS( (void) rotatedMinimum( Ints() ) , contract_violation );
 CHECK_THROWS( (void) rotatedMinimum( Ints( { 2 , 1 , 3 } ) ) ,
               contract_violation );
 CHECK_THROWS( (void) rotatedMinimum( Ints( { 1 , 1 } ) ) ,
               contract_violation );                 // not strictly
 CHECK( rotatedMinimum( Ints( { 9 } ) ) == 0 &&
        rotatedMinimum( Ints( { 3 , 4 , 1 , 2 } ) ) == 2 &&
        rotatedMinimum( Ints( { 1 , 2 , 3 } ) ) == 0 &&
        rotatedMinimum( Ints( { 2 , 3 , 1 } ) ) == 2 );
 CHECK_THROWS( (void) closestValue( Ints() , 1 ) , contract_violation );
 CHECK_THROWS( (void) closestValue( Ints( { 3 , 1 } ) , 1 ) ,
               contract_violation );
 CHECK( closestValue( Ints( { 1 , 3 } ) , 2 ) == 0 &&    // the smaller
        closestValue( Ints( { 1 , 3 } ) , 9 ) == 1 &&
        closestValue( Ints( { 5 } ) , -9 ) == 0 );
 CHECK( closestValue( Ints( { std::numeric_limits< int >::min() ,
                              std::numeric_limits< int >::max() } ) ,
                      1 ) == 1 );
 CHECK_THROWS( (void) pairWithSum( Ints( { 3 , 1 } ) , 4 ) ,
               contract_violation );
 CHECK( ! pairWithSum( Ints() , 0 ) && ! pairWithSum( Ints( { 2 } ) , 4 ) );
 CHECK( pairWithSum( Ints( { 2 , 2 } ) , 4 ) == std::pair( std::size_t( 0 ) ,
                                                         std::size_t( 1 ) ) );
 CHECK( pairWithSum( Ints( { std::numeric_limits< int >::max() ,
                             std::numeric_limits< int >::max() } ) ,
                     2LL * std::numeric_limits< int >::max() ) );
 const Ints X = { 1 , 3 } , Y = { 2 };
 CHECK( kthOfTwoSorted( X , Y , 0 ) == 1 && kthOfTwoSorted( X , Y , 2 ) == 3 );
 CHECK( kthOfTwoSorted( Ints() , Y , 0 ) == 2 &&
        kthOfTwoSorted( X , Ints() , 1 ) == 3 );
 CHECK_THROWS( (void) kthOfTwoSorted( X , Y , 3 ) , contract_violation );
 CHECK_THROWS( (void) kthOfTwoSorted( Ints() , Ints() , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) kthOfTwoSorted( Ints( { 3 , 1 } ) , Y , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) kthOfTwoSorted( X , Ints( { 2 , 1 } ) , 0 ) ,
               contract_violation );
 }

// selection() and medianOfMedians() against std::nth_element, for every
// rank; the ranks 0 and n + 1 violate their contracts, 1 and n are just
// inside; an empty vector, one item, all items equal
ADS_TEST( "selection" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const Ints A = random_ints( 1 + g() % 60 , 25 , g );
  Ints S = A;
  std::sort( S.begin() , S.end() );
  const std::size_t k = 1 + g() % A.size();
  Ints B = A , C = A , D = A;
  CHECK( selection( B , k ) == S[ k - 1 ] );
  CHECK( selection( C , k , &g ) == S[ k - 1 ] );
  CHECK( medianOfMedians( D , k ) == S[ k - 1 ] );
  }
 Ints E = { 3 , 1 , 2 };
 CHECK_THROWS( (void) selection< int >( E , 0 ) , contract_violation );
 CHECK_THROWS( (void) selection< int >( E , 4 ) , contract_violation );
 CHECK_THROWS( (void) medianOfMedians( E , 0 ) , contract_violation );
 CHECK_THROWS( (void) medianOfMedians( E , 4 ) , contract_violation );
 Ints Z;                                       // no rank at all
 CHECK_THROWS( (void) selection< int >( Z , 0 ) , contract_violation );
 CHECK_THROWS( (void) selection< int >( Z , 1 ) , contract_violation );
 CHECK_THROWS( (void) medianOfMedians( Z , 1 ) , contract_violation );
 Ints O = { 8 } , Q( 50 , 6 );                 // one item, all equal
 CHECK( selection< int >( O , 1 ) == 8 && medianOfMedians( O , 1 ) == 8 );
 CHECK( selection< int >( Q , 1 ) == 6 && medianOfMedians( Q , 50 ) == 6 );
 Ints F = { 3 , 1 , 2 } , G = F;               // the ranks 1 and n
 CHECK( selection< int >( F , 1 ) == 1 && selection< int >( F , 3 ) == 3 &&
        medianOfMedians( G , 3 ) == 3 && medianOfMedians( G , 1 ) == 1 );
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
/*---------------------- End File contract_search.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
