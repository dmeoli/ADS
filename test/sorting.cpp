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
#include <array>
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

/// the inputs of the battery: every permutation of up to 7 items (the
/// empty vector, one item, two), and vectors of the given sizes in the
/// classical patterns, all equal and with the extremes of int
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
  Ints r( n ) , s( n ) , v( n ) , w( n ) , o( n ) , f( n ) , e( n , 5 ) ,
       x( n );
  const Ints ext = { std::numeric_limits< int >::min() , -1 , 0 , 1 ,
                     std::numeric_limits< int >::max() };
  for( std::size_t i = 0 ; i < n ; ++i ) {
   r[ i ] = int( g() % ( 2 * n ) );          // random
   s[ i ] = int( i );                        // sorted
   v[ i ] = int( n - i );                    // reversed
   w[ i ] = int( i % 17 );                   // sawtooth
   o[ i ] = int( std::min( i , n - 1 - i ) );   // organ pipe
   f[ i ] = int( g() % 4 );                  // few distinct keys
   x[ i ] = ( i % 2 ) ? ext[ g() % 5 ] : int( g() );   // the extremes
   }
  for( auto * y : { &r , &s , &v , &w , &o , &f , &e , &x } )
   I.push_back( *y );
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
// the odd-even transposition at most n, one on a sorted, empty or single
// vector
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
 oddEvenSort( A , std::less<>() , &r );          // sorted: one round
 CHECK( r == 1 );
 Ints E , O = { 4 };
 oddEvenSort( E , std::less<>() , &r );
 CHECK( r == 1 && E.empty() );
 oddEvenSort( O , std::less<>() , &r );
 CHECK( r == 1 && O == Ints( { 4 } ) );
 Ints D = { 1 , 0 };                             // n = 2: one swap
 oddEvenSort( D , std::less<>() , &r );
 CHECK( r == 2 && D == Ints( { 0 , 1 } ) );
 }

// the cycle sort with at most n writes, n - ( fixed points ) on distinct
// keys and the items out of place with duplicates, and the pancake sort
// with at most 2 n - 3 flips; no write and no flip on an empty, single,
// all equal vector
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
 Ints E , O = { 7 } , Q( 9 , 3 );                // nothing to write or flip
 CHECK( cycleSort( E ) == 0 && pancakeSort( E ) == 0 );
 CHECK( cycleSort( O ) == 0 && pancakeSort( O ) == 0 );
 CHECK( cycleSort( Q ) == 0 && pancakeSort( Q ) == 0 );
 Ints T = { 0 , 1 , 1 , 2 , 2 };                 // sorted, with equals
 CHECK( cycleSort( T ) == 0 && pancakeSort( T ) == 0 );
 Ints U = { 1 , 1 , 0 };                         // 2 items out of place
 CHECK( cycleSort( U ) == 2 && U == Ints( { 0 , 1 , 1 } ) );
 Ints V = { 1 , 0 };                             // 2 n - 3 = 1 flip
 CHECK( pancakeSort( V ) == 1 && V == Ints( { 0 , 1 } ) );
 }

// the tree sort on a Red-Black tree and on an unbalanced tree, the repeated
// items kept, empty, all equal, and the unbalanced tree as a chain of 20000
// nodes on a reversed input; the patience sort with as many piles as a
// longest strictly increasing subsequence: 0 on an empty vector, 1 on equal
// or decreasing items, n on sorted ones
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
 Ints E , Q( 50 , 2 );                           // equals on one pile: the
 patienceSort( E , std::less<>() , &piles );     // increase is strict
 CHECK( piles == 0 && E.empty() );
 patienceSort( Q , std::less<>() , &piles );
 CHECK( piles == 1 && Q == Ints( 50 , 2 ) );
 std::iota( D.begin() , D.end() , 0 );           // sorted: n piles
 patienceSort( D , std::less<>() , &piles );
 CHECK( piles == 100 );
 treeSort( E );
 treeSort( Q , false );
 CHECK( E.empty() && Q == Ints( 50 , 2 ) );
 Ints C( 20000 );                                // a chain of 20000 nodes,
 for( int i = 0 ; i < 20000 ; ++i )              // with duplicates
  C[ i ] = i / 2;
 Ints R( C.rbegin() , C.rend() );
 treeSort( R , false );
 CHECK( R == C );
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

// the strand sort of a list, stable on pairs compared by their keys, one
// strand on a sorted list or on equal items, n on a decreasing one, none
// on an empty list
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
 List< int > E , O = { 4 } , Q = { 2 , 2 , 2 };  // equals: one strand
 strandSort( E , std::less<>() , &k );
 CHECK( k == 0 && E.isEmpty() );
 strandSort( O , std::less<>() , &k );
 CHECK( k == 1 && O.read( O.head() ) == 4 );
 strandSort( Q , std::less<>() , &k );
 CHECK( k == 1 );
 using P = std::pair< int , int >;               // stable on equal keys
 std::mt19937 g( 14 );
 List< P > L;
 std::vector< P > V;
 for( int i = 0 ; i < 300 ; ++i ) {
  V.emplace_back( int( g() % 5 ) , i );
  L.insert( L.next( L.tail() ) , V.back() );
  }
 auto byKey = []( const P & a , const P & b ) { return( a.first < b.first ); };
 strandSort( L , byKey );
 std::stable_sort( V.begin() , V.end() , byKey );
 std::vector< P > W;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  W.push_back( L.read( p ) );
 CHECK( W == V );
 }

// the sorting networks: by the 0-1 principle, every vector of 0 and 1 of
// n items sorted, for n up to 12 (merge exchange) and n = 16 (bitonic);
// random vectors of other sizes; the comparators within n; no comparator
// for n = 0 and 1, one for n = 2, and the bitonic sorter refused for n not
// a power of 2
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
 CHECK( mergeExchangeNetwork( 0 ).empty() &&
        mergeExchangeNetwork( 1 ).empty() );
 CHECK( bitonicNetwork( 0 ).empty() && bitonicNetwork( 1 ).empty() &&
        bitonicNetwork( 2 ).size() == 1 &&
        mergeExchangeNetwork( 2 ).size() == 1 );
 CHECK_THROWS( bitonicNetwork( 3 ) , contract_violation );
 Ints E;
 applyNetwork( E , mergeExchangeNetwork( 0 ) );
 CHECK( E.empty() );
 CHECK_THROWS( applyNetwork( E , { Comparator( 0 , 1 ) } ) ,
               contract_violation );
 applyNetwork( Z , mergeExchangeNetwork( 2 ) );
 CHECK( Z == Ints( { 0 , 1 } ) );
 }

// the slow sorts on small vectors, the bogo sort only up to 10 items, with
// no shuffle on a sorted, empty or all equal vector
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
 Ints E , T( 10 ) , Q( 10 , 4 );                 // sorted: no shuffle, also
 std::iota( T.begin() , T.end() , 0 );           // at the bound n = 10
 CHECK( bogoSort( E , g ) == 0 && bogoSort( T , g ) == 0 &&
        bogoSort( Q , g ) == 0 );
 Ints D = { 1 , 0 };
 CHECK( bogoSort( D , g ) >= 1 && D == Ints( { 0 , 1 } ) );
 }

// the engineered quicksorts on the battery, and on inputs against the
// classical quicksort: a killer of the median of three would make them
// quadratic, but the depth bound of introsort and pdqsort keeps them
// O( n log n ), also against the adversary of McIlroy that makes every
// pivot a smallest item; pdqsort takes O( n ) comparisons on a sorted
// vector
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
 const int n = 20000;                 // the adversary of McIlroy: the keys
 for( int k = 0 ; k < 2 ; ++k ) {     // fixed only when compared, so that
  std::vector< int > val( n , n );    // each pivot is a smallest item
  int solid = 0 , cand = -1;
  long cmp = 0;
  auto adv = [ & ]( int x , int y ) {
   if( ! checkingContract() ) {
    ++cmp;
    if( ( val[ x ] == n ) && ( val[ y ] == n ) )
     val[ x == cand ? x : y ] = solid++;
    if( val[ x ] == n ) cand = x;
    else if( val[ y ] == n ) cand = y;
    }
   return( val[ x ] < val[ y ] ); };
  Ints A( n );
  std::iota( A.begin() , A.end() , 0 );
  if( k == 0 ) introSort( A , adv ); else pdqSort( A , adv );
  CHECK( std::is_sorted( A.begin() , A.end() , [ & ]( int x , int y ) {
          return( val[ x ] < val[ y ] ); } ) );
  CHECK( cmp < 6L * n * 15 );         // n log n, not n^2 / 4
  }
 }

// Timsort and Powersort on the battery, stable; the runs found on vectors
// made of r sorted runs, and O( n ) comparisons on a sorted vector and on
// two runs one after the other; no run on an empty vector, one on a
// strictly decreasing vector (reversed) and on all equal items
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
 Ints E , D( 100000 ) , Q( 100000 , 3 );
 std::iota( D.rbegin() , D.rend() , 0 );        // strictly decreasing
 for( auto sort : { timSort< int , decltype( count ) > ,
                    powerSort< int , decltype( count ) > } ) {
  sort( E , count , &runs );
  CHECK( runs == 0 && E.empty() );
  Ints A = D , B = Q;
  c = 0;
  sort( A , count , &runs );                     // one run, reversed
  CHECK( runs == 1 && std::is_sorted( A.begin() , A.end() ) &&
         c < 100000 );
  c = 0;
  sort( B , count , &runs );                     // equals: one run
  CHECK( runs == 1 && B == Q && c < 100000 );
  }
 }

// the radix sorts: integers of every width and sign, of 0, 1, 2 items,
// around the insertion threshold of the American flag sort, with the
// extremes of the type and all equal to them; doubles with negatives,
// -0 before +0, the infinities and the denormals in their places, the NaN
// at the ends by their sign; the stable sort by a key, with the largest
// key; the MSD sort of strings against std::sort, with prefixes, '\0'
// inside, equal long strings; the bucket sort of numbers in [ 0 , 1 ), the
// largest double below 1 included, NaN and negatives refused
template< class T >
void radix_of( unsigned seed )
{
 std::mt19937_64 g( seed );
 constexpr T lo = std::numeric_limits< T >::min() ,
             hi = std::numeric_limits< T >::max();
 for( std::size_t n : { 0u , 1u , 2u , 7u , 33u , 100u , 5000u } ) {
  std::vector< T > A( n );
  for( auto & x : A ) x = T( g() );
  if( n > 5 ) { A[ 3 ] = lo; A[ 4 ] = hi; A[ 5 ] = T( 0 ); }
  if( n == 2 ) { A[ 0 ] = hi; A[ 1 ] = lo; }
  for( const auto & X : { A , std::vector< T >( n , lo ) ,
                          std::vector< T >( n , hi ) } ) {
   auto B = X , C = X , D = X;                 // also all equal
   radixSort( B );
   americanFlagSort( C );
   std::sort( D.begin() , D.end() );
   CHECK( B == D && C == D );
   }
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
 const double inf = std::numeric_limits< double >::infinity() ,
              nan = std::numeric_limits< double >::quiet_NaN() ,
              den = std::numeric_limits< double >::denorm_min();
 const std::vector< double > G = { 1.0 , nan , -0.0 , inf , -inf , 0.0 ,
                                   -den , den , std::copysign( nan , -1.0 ) ,
                                   -1.0 };
 const std::vector< double > order = { -inf , -1.0 , -den , -0.0 , 0.0 ,
                                       den , 1.0 , inf };
 for( int k = 0 ; k < 2 ; ++k ) {
  auto H = G;
  if( k == 0 ) radixSort( H ); else americanFlagSort( H );
  CHECK( std::isnan( H[ 0 ] ) && std::signbit( H[ 0 ] ) &&
         std::isnan( H[ 9 ] ) && ! std::signbit( H[ 9 ] ) );
  CHECK( std::equal( order.begin() , order.end() , H.begin() + 1 ) &&
         std::signbit( H[ 4 ] ) && ! std::signbit( H[ 5 ] ) );
  }
 using P = std::pair< unsigned , int >;
 std::vector< P > R;
 for( int i = 0 ; i < 3000 ; ++i ) R.emplace_back( unsigned( g() % 50 ) , i );
 auto Q = R;
 radixSortBy( R , []( const P & p ) { return( p.first ); } );
 std::stable_sort( Q.begin() , Q.end() , []( const P & a , const P & b ) {
  return( a.first < b.first ); } );
 CHECK( R == Q );
 std::vector< P > none;
 radixSortBy( none , []( const P & p ) { return( p.first ); } );
 CHECK( none.empty() );
 constexpr auto top = std::numeric_limits< std::uint64_t >::max();
 std::vector< std::uint64_t > K = { top , 0 , top - 1 , 1 , top };
 radixSortBy( K , []( std::uint64_t x ) { return( x ); } );
 CHECK( K == std::vector< std::uint64_t >( { 0 , 1 , top - 1 , top ,
                                             top } ) );
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
 std::vector< std::string > X;
 const std::string big( 300 , 'q' );
 for( std::size_t l = 0 ; l <= 300 ; l += 7 )   // the prefixes of a string
  X.push_back( big.substr( 0 , l ) );
 for( int i = 0 ; i < 40 ; ++i )                 // equal long strings
  X.push_back( std::string( 2000 , 'z' ) );
 for( const char * s : { "a\0b" , "a\0\0" , "\0\0" } )   // '\0' inside
  X.push_back( std::string( s , 3 ) );
 X.push_back( std::string( "a\0" , 2 ) );
 X.push_back( std::string( 1 , '\0' ) );
 X.push_back( "a" );
 X.push_back( "" );
 std::shuffle( X.begin() , X.end() , g );
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) , X.size() } ) {
  std::vector< std::string > Y( X.begin() , X.begin() + n ) , Y2 = Y ,
                             Y3 = Y;
  msdRadixSort( Y );
  multikeyQuickSort( Y2 );
  std::sort( Y3.begin() , Y3.end() );
  CHECK( Y == Y3 && Y2 == Y3 );
  }
 std::vector< double > U( 4000 );
 std::uniform_real_distribution< double > z( 0 , 1 );
 for( auto & x : U ) x = z( g );
 auto Z = U;
 bucketSort( U );
 std::sort( Z.begin() , Z.end() );
 CHECK( U == Z );
 std::vector< double > bad = { 0.5 , 1.0 };
 CHECK_THROWS( bucketSort( bad ) , contract_violation );
 const double below = std::nextafter( 1.0 , 0.0 );
 for( std::size_t n : { 1u , 2u , 3u , 1000u , 4097u } ) {
  std::vector< double > V( n , below );          // the last bucket
  V[ 0 ] = 0.0;
  if( n > 2 ) V[ 1 ] = -0.0;
  bucketSort( V );
  CHECK( std::is_sorted( V.begin() , V.end() ) && V[ 0 ] == 0.0 &&
         ( n == 1 || V.back() == below ) );
  }
 std::vector< float > Fb( 5 , std::nextafter( 1.0f , 0.0f ) ) , Ne;
 bucketSort( Fb );
 bucketSort( Ne );
 CHECK( Fb == std::vector< float >( 5 , std::nextafter( 1.0f , 0.0f ) ) &&
        Ne.empty() );
 for( double x : { -1e-300 , nan , inf , -0.5 } ) {
  std::vector< double > V = { 0.25 , x };
  CHECK_THROWS( bucketSort( V ) , contract_violation );
  }
 }

// the sort that chooses: numbers short and long, strings, records with an
// order of their own, stable or not, all against the standard library;
// around the thresholds kRadix and kStrings, the extremes of long long,
// the inputs already sorted or decreasing (strictly or not), and -0 and
// +0 kept in their order by the stable sort
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
 for( std::size_t n : { kRadix - 1 , kRadix , kRadix + 1 } ) {
  using L = long long;                           // around the threshold
  std::vector< L > A( n );
  for( auto & x : A ) x = L( g() ) - L( g() );
  A[ 0 ] = std::numeric_limits< L >::max();
  A[ 1 ] = std::numeric_limits< L >::min();
  auto B = A , C = A;
  sort( A );
  stableSort( C );
  std::sort( B.begin() , B.end() );
  CHECK( A == B && C == B );
  Ints S( n ) , D( n ) , Q( n );                 // the presorted ones
  for( std::size_t i = 0 ; i < n ; ++i ) {
   S[ i ] = int( i / 3 );
   D[ i ] = int( n - i );
   Q[ i ] = int( ( n - i ) / 3 );                // decreasing, not strictly
   }
  for( const auto & X : { S , D , Q } ) {
   Ints Y = X , Z = X , W = X;
   sort( Y );
   stableSort( Z );
   std::sort( W.begin() , W.end() );
   CHECK( Y == W && Z == W );
   }
  }
 for( std::size_t n : { kStrings - 1 , kStrings } ) {
  std::vector< std::string > S( n );
  for( auto & s : S ) s = std::string( g() % 3 , char( 'a' + g() % 2 ) );
  auto T = S , U = S;
  sort( S );
  stableSort( U );
  std::sort( T.begin() , T.end() );
  CHECK( S == T && U == T );
  }
 std::vector< double > Z( 1000 );                // -0 and +0 are equal:
 for( std::size_t i = 1 ; i < Z.size() ; ++i )   // stableSort keeps their
  Z[ i ] = ( i % 3 ) ? 0.0 : -0.0;               // order, also by radix
 Z[ 0 ] = 1.0;
 auto Zs = Z;
 stableSort( Z );
 std::stable_sort( Zs.begin() , Zs.end() );
 CHECK( Z.back() == 1.0 );
 for( std::size_t i = 0 ; i < Z.size() ; ++i )
  CHECK( std::signbit( Z[ i ] ) == std::signbit( Zs[ i ] ) );
 }

// the edges of the mechanisms of the fast sorts: the cutoffs of pdqSort()
// (insertion below 24, ninther above 128, blocks of 64) on organ pipe,
// sawtooth, two values and all equal, with doubles (the partition by
// blocks) and records; the killer of the median of 3 of Musser
ADS_TEST( "pdq-edges" )
{
 using R = std::pair< int , std::size_t >;
 auto byKey = []( const R & a , const R & b ) {
  return( a.first < b.first ); };
 std::mt19937 g( 31 );
 for( std::size_t n : { 0u , 1u , 2u , 23u , 24u , 25u , 127u , 128u ,
                        129u , 191u , 192u , 193u , 257u } )
  for( int p = 0 ; p < 6 ; ++p ) {
   std::vector< double > D( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    switch( p ) {
     case( 0 ): D[ i ] = double( std::min( i , n - 1 - i ) ); break;
     case( 1 ): D[ i ] = double( i % 7 ); break;
     case( 2 ): D[ i ] = ( g() % 2 ) ? 1e300 : -1e300; break;
     case( 3 ): D[ i ] = 0.5; break;
     case( 4 ): D[ i ] = double( n - i ); break;
     default: D[ i ] = double( g() % 1000 ) - 500;
     }
   std::vector< R > Q( n );
   for( std::size_t i = 0 ; i < n ; ++i )        // 1e300 as 1
    Q[ i ] = { int( std::max( -1e3 , std::min( 1e3 , D[ i ] ) ) ) , i };
   auto E = D , F = D;
   auto Q2 = Q;
   pdqSort( D );
   sort( F );
   std::sort( E.begin() , E.end() );
   pdqSort( Q , byKey );
   std::stable_sort( Q2.begin() , Q2.end() , byKey );
   CHECK( D == E && F == E );
   CHECK( std::is_sorted( Q.begin() , Q.end() , byKey ) &&
          detail::samePermutation( Q , Q2 , std::less<>() ) );
   }
 for( std::size_t n : { 64u , 1000u , 4096u } ) {   // the median-of-3
  std::vector< double > K( n );                     // killer
  const std::size_t k = n / 2;
  for( std::size_t i = 0 ; i < k ; ++i ) {
   K[ 2 * i ] = double( i + 1 );
   K[ 2 * i + 1 ] = double( k + i + 1 );
   }
  auto E = K;
  pdqSort( K );
  std::sort( E.begin() , E.end() );
  CHECK( K == E );
  }
 }

// the two merges of the run sorts, with no branch on the numbers in
// disorder and by galloping otherwise: stability checked on -0 and +0,
// which std::less finds equal, and on records ( key , place ) in the
// patterns that matter: one descending run with equal keys, organ pipe,
// sawtooth, runs of length 1 and 2, all equal, two values, and two runs
// that interleave in blocks of 6, 7 and 8, around the start of the
// galloping; all against std::stable_sort
ADS_TEST( "merge-paths" )
{
 std::mt19937 g( 21 );
 using D = std::vector< double >;
 auto stableAs = []( const D & A , D B ) {      // the signs too
  std::stable_sort( B.begin() , B.end() );
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   if( ( A[ i ] != B[ i ] ) ||
       ( std::signbit( A[ i ] ) != std::signbit( B[ i ] ) ) )
    return( false );
  return( A.size() == B.size() ); };
 const double v[] = { -1.0 , -0.0 , 0.0 , 1.0 };
 for( std::size_t n : { 0u , 1u , 2u , 23u , 24u , 25u , 48u , 49u ,
                        511u , 512u , 513u , 20000u } ) {
  D X( n ) , L;                                  // in disorder, in runs
  for( auto & x : X ) x = v[ g() % 4 ];
  for( std::size_t r = 0 ; L.size() < n ; ++r ) {
   D run( std::min< std::size_t >( n - L.size() , 1 + n / 5 ) );
   for( auto & x : run ) x = v[ g() % 4 ];
   std::stable_sort( run.begin() , run.end() );
   if( r % 2 )                                   // strictly decreasing
    std::reverse( run.begin() , run.end() );     // only if no equals
   L.insert( L.end() , run.begin() , run.end() );
   }
  for( const D & Y : { X , L } ) {
   D A = Y , B = Y , C = Y;
   timSort( A );
   powerSort( B );
   stableSort( C );
   CHECK( stableAs( A , Y ) && stableAs( B , Y ) && stableAs( C , Y ) );
   }
  }
 using R = std::pair< int , std::size_t >;
 auto byKey = []( const R & a , const R & b ) {
  return( a.first < b.first ); };
 for( std::size_t n : { 0u , 1u , 2u , 23u , 24u , 25u , 100u , 511u ,
                        512u , 513u , 5000u } )
  for( int p = 0 ; p < 10 ; ++p ) {
   std::vector< R > Q( n );
   const std::size_t h = n / 2 , L = 6 + std::size_t( p % 3 );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    int k = 0;
    switch( p ) {
     case( 0 ): k = int( ( n - i ) / 2 ); break;   // descending, equals
     case( 1 ): k = int( std::min( i , n - 1 - i ) ); break;
     case( 2 ): k = int( i % 17 ); break;
     case( 3 ): k = int( i ^ 1 ); break;            // runs of length 2
     case( 4 ): k = int( g() % 3 ); break;          // runs of 1 and 2
     case( 5 ): k = 7; break;
     case( 6 ): k = int( g() % 2 ); break;
     default: {                                     // blocks of L
      const std::size_t j = i < h ? i : i - h;
      k = int( ( j / L ) * 2 * L + ( i < h ? 0 : L ) + j % L );
      }
     }
    Q[ i ] = { k , i };
    }
   auto A = Q , B = Q , C = Q , E = Q;
   timSort( A , byKey );
   powerSort( B , byKey );
   stableSort( C , byKey );
   std::stable_sort( E.begin() , E.end() , byKey );
   CHECK( A == E && B == E && C == E );
   }
 for( int t = 0 ; t < 200 ; ++t ) {             // randomized
  const std::size_t n = g() % 3000;
  std::vector< R > Q( n );
  const int keys = 1 + int( g() % 50 );
  for( std::size_t i = 0 ; i < n ; ++i ) Q[ i ] = { int( g() % keys ) , i };
  if( t % 3 == 1 )                              // partly presorted
   std::sort( Q.begin() , Q.begin() + n / 2 , byKey );
  auto A = Q , B = Q , E = Q;
  powerSort( A , byKey );
  timSort( B , byKey );
  std::stable_sort( E.begin() , E.end() , byKey );
  CHECK( A == E && B == E );
  }
 }

// the choices of sort() and stableSort() at their edges: 0, 1, 2 items;
// the runs counted around kRuns on vectors above kRadix and kStrings, and
// where they end; random runs of either direction merged on the ends that
// the count found, the runs of 1 and 2 items, the last of 1, and a tie of
// -0 and +0 at a boundary, against std::stable_sort;
// strictly decreasing runs with -0 and +0 inside, one run (sorted with
// ties, or strictly decreasing and reversed) at kStrings and kRadix, a
// tie of -0 and +0 that splits a decreasing run; few wide keys (16
// distinct doubles, 17, two values far apart) against few narrow ones;
// the radix sort on keys that differ in one byte only, the top, a middle
// or the sign, all equal, two values; the MSD sort of strings around the
// insertion cutoff of 16, groups of equal strings, prefixes of each other,
// the shortest first, empty strings and long shared prefixes
ADS_TEST( "dispatch-edges" )
{
 std::mt19937 g( 41 );
 auto less = std::less<>();
 for( std::size_t n : { 0u , 1u , 2u } ) {
  std::vector< double > D( n );
  std::vector< std::string > S( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   D[ i ] = double( n - i );
   S[ i ] = std::string( n - i , 'a' );
   }
  auto D2 = D , D3 = D;
  auto S2 = S , S3 = S;
  sort( D2 );
  stableSort( D3 );
  sort( S2 );
  stableSort( S3 );
  std::sort( D.begin() , D.end() );
  std::sort( S.begin() , S.end() );
  CHECK( D2 == D && D3 == D && S2 == S && S3 == S );
  }
 for( std::size_t n : { kRadix , std::size_t( 4096 ) } )
  for( std::size_t r : { std::size_t( 1 ) , kRuns - 1 , kRuns , kRuns + 1 ,
                         2 * kRuns } ) {
   Ints A( n );                                  // exactly r runs
   for( std::size_t k = 0 ; k < r ; ++k )
    for( std::size_t i = k * n / r ; i < ( k + 1 ) * n / r ; ++i )
     A[ i ] = int( i - k * n / r );
   std::array< std::size_t , kRuns + 1 > ends;
   const std::size_t found = detail::runs( A , less , kRuns , ends.data() );
   CHECK( found == std::min( r , kRuns + 1 ) );
   for( std::size_t k = 0 ; k < found ; ++k )   // where they end
    CHECK( ends[ k ] == ( k + 1 ) * n / r );
   Ints B = A , C = A , E = A;
   sort( B );
   stableSort( C );
   std::sort( E.begin() , E.end() );
   CHECK( B == E && C == E );
   std::vector< std::string > S( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    S[ i ] = std::to_string( 100000 + A[ i ] );
   auto S2 = S , S3 = S , S4 = S;
   sort( S2 );
   stableSort( S3 );
   std::sort( S4.begin() , S4.end() );
   CHECK( S2 == S4 && S3 == S4 );
   }
 for( int trial = 0 ; trial < 300 ; ++trial ) {  // random runs, each
  const std::size_t n = kRadix + g() % 3000;    // ascending with ties or
  const std::size_t r = 2 + g() % kRuns;        // strictly descending
  std::vector< std::size_t > cut;               // 2 .. kRuns + 1 runs
  for( std::size_t k = 1 ; k < r ; ++k )
   cut.push_back( trial % 3 == 0 ? n - r + k     // runs of 1 at the end
                  : trial % 3 == 1 ? 2 * k       // runs of 2 at the start
                  : 1 + g() % ( n - 1 ) );
  cut.push_back( n );
  std::sort( cut.begin() , cut.end() );
  cut.erase( std::unique( cut.begin() , cut.end() ) , cut.end() );
  std::vector< double > V( n );
  std::size_t lo = 0;
  for( std::size_t c : cut ) {
   const bool down = g() % 2;
   for( std::size_t i = lo ; i < c ; ++i )
    V[ i ] = down ? double( c - i ) * 3 : double( ( i - lo ) / 2 ) - 1;
   if( ! down && ( c - lo > 3 ) )            // a tie of -0 and +0
    V[ lo + 2 ] = -0.0;
   lo = c;
   }
  Ints A( n );
  std::vector< std::string > S( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   A[ i ] = int( V[ i ] );
   S[ i ] = std::to_string( 100000 + A[ i ] );
   }
  auto V2 = V , V3 = V , Vs = V;
  auto A2 = A , A3 = A , As = A;
  auto S2 = S , S3 = S , Ss = S;
  sort( V2 ); stableSort( V3 ); std::stable_sort( Vs.begin() , Vs.end() );
  sort( A2 ); stableSort( A3 ); std::sort( As.begin() , As.end() );
  sort( S2 ); stableSort( S3 ); std::sort( Ss.begin() , Ss.end() );
  CHECK( V2 == Vs && V3 == Vs && A2 == As && A3 == As && S2 == Ss &&
         S3 == Ss );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( std::signbit( V3[ i ] ) == std::signbit( Vs[ i ] ) );
  }
 std::vector< double > Z( 1000 );               // strictly decreasing
 for( std::size_t i = 0 ; i < Z.size() ; ++i )  // runs, then -0 and +0
  Z[ i ] = double( 500 - int( i ) );
 for( std::size_t i = 500 ; i < 520 ; ++i ) Z[ i ] = ( i % 2 ) ? -0.0 : 0.0;
 auto Zs = Z;
 stableSort( Z );
 std::stable_sort( Zs.begin() , Zs.end() );
 CHECK( Z == Zs );
 for( std::size_t i = 0 ; i < Z.size() ; ++i )
  CHECK( std::signbit( Z[ i ] ) == std::signbit( Zs[ i ] ) );
 for( std::size_t n : { kStrings , kRadix , kRadix + 1 } ) {   // one run,
  Ints U( n ) , Dn( n );                        // non-decreasing with
  std::vector< std::string > SU( n ) , SD( n ); // ties, or strictly
  for( std::size_t i = 0 ; i < n ; ++i ) {      // decreasing: one pass
   U[ i ] = int( i / 3 );
   Dn[ i ] = int( n - i );
   SU[ i ] = std::to_string( 100000 + i / 3 );
   SD[ i ] = std::to_string( 100000 + n - i );
   }
  std::array< std::size_t , kRuns + 1 > ends;
  CHECK( detail::runs( U , less , kRuns , ends.data() ) == 1 &&
         ends[ 0 ] == n &&
         detail::runs( Dn , less , kRuns , ends.data() ) == 1 &&
         ends[ 0 ] == n );
  for( int s = 0 ; s < 2 ; ++s ) {
   Ints U2 = U , D2 = Dn , DE = Dn;
   auto SU2 = SU , SD2 = SD , SDE = SD;
   if( s == 0 ) {
    sort( U2 ); sort( D2 ); sort( SU2 ); sort( SD2 );
    }
   else {
    stableSort( U2 ); stableSort( D2 ); stableSort( SU2 ); stableSort( SD2 );
    }
   std::sort( DE.begin() , DE.end() );
   std::sort( SDE.begin() , SDE.end() );
   CHECK( U2 == U && D2 == DE && SU2 == SU && SD2 == SDE );
   }
  }
 std::vector< double > Y( kRadix );             // decreasing but for a
 for( std::size_t i = 0 ; i < Y.size() ; ++i )  // tie of -0 and +0: two
  Y[ i ] = double( 300 - int( i ) );            // runs, not one reversed
 Y[ 300 ] = 0.0;
 Y[ 301 ] = -0.0;
 for( std::size_t i = 302 ; i < Y.size() ; ++i ) Y[ i ] = -double( i );
 auto Ys = Y;
 stableSort( Y );
 std::stable_sort( Ys.begin() , Ys.end() );
 CHECK( Y == Ys );
 for( std::size_t i = 0 ; i < Y.size() ; ++i )
  CHECK( std::signbit( Y[ i ] ) == std::signbit( Ys[ i ] ) );
 for( std::size_t k : { 1u , 2u , 16u , 17u , 64u } ) {   // few keys,
  std::vector< double > V( 4096 ) , W( k );
  for( auto & w : W ) w = ( double( g() ) - 2e9 ) / 7.0;
  W[ 0 ] = -0.0;
  if( k > 1 ) W[ 1 ] = 0.0;
  for( auto & x : V ) x = W[ g() % k ];
  if( k != 17 )                                 // -0 and +0 are wide
   CHECK( detail::fewWideKeys( V ) == ( ( k > 1 ) && ( k <= 16 ) ) );
  auto V2 = V , V3 = V;
  sort( V );
  stableSort( V2 );
  std::stable_sort( V3.begin() , V3.end() );
  CHECK( V == V3 && V2 == V3 );
  for( std::size_t i = 0 ; i < V2.size() ; ++i )
   CHECK( std::signbit( V2[ i ] ) == std::signbit( V3[ i ] ) );
  std::vector< std::uint32_t > U( 4096 );       // narrow: one byte
  for( auto & x : U ) x = std::uint32_t( g() % k );
  CHECK( ! detail::fewWideKeys( U ) );
  }
 std::vector< double > Far( 4096 );             // two values, far apart
 for( auto & x : Far ) x = ( g() % 2 ) ? 1e300 : -1e300;
 auto Far2 = Far;
 sort( Far );
 std::sort( Far2.begin() , Far2.end() );
 CHECK( Far == Far2 );
 for( int b = 0 ; b < 6 ; ++b )                 // one byte that differs
  for( std::size_t n : { 0u , 1u , 2u , 1000u } ) {
   std::vector< std::uint32_t > U( n );
   std::vector< long long > L( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint32_t x = std::uint32_t( g() % 256 );
    U[ i ] = b < 4 ? ( x << ( 8 * b ) ) : ( b == 4 ? 7u : x % 2 );
    L[ i ] = b < 4 ? ( ( long long )( x ) << ( 8 * b ) ) :
             ( b == 4 ? ( x % 2 ? -1 : 0 ) : -( long long )( x % 2 ) - 5 );
    }
   auto U2 = U;
   auto L2 = L;
   radixSort( U );
   radixSort( L );
   std::sort( U2.begin() , U2.end() );
   std::sort( L2.begin() , L2.end() );
   CHECK( U == U2 && L == L2 );
   }
 std::vector< std::uint8_t > Small = { 3 , 3 , 3 };
 radixSort( Small );
 CHECK( Small == std::vector< std::uint8_t >( { 3 , 3 , 3 } ) );
 const std::string pre( 1000 , 'p' );
 for( std::size_t c : { 15u , 16u , 17u , 18u , 40u } ) {   // groups
  std::vector< std::vector< std::string > > T( 7 );
  for( std::size_t i = 0 ; i < c ; ++i ) {
   T[ 0 ].push_back( "abc" );                   // all equal
   T[ 1 ].push_back( "abc" );                   // and a longer one
   T[ 2 ].push_back( "abc" );                   // and a shorter one
   T[ 3 ].push_back( "" );                      // empty strings
   T[ 4 ].push_back( pre + char( 'a' + g() % 3 ) );   // shared prefix
   T[ 5 ].push_back( std::string( g() % 5 , 'x' ) );  // prefixes
   T[ 6 ].push_back( i % 2 ? "ab" : "abc" );
   }
  T[ 1 ].push_back( "abcd" );
  T[ 2 ].insert( T[ 2 ].begin() , "ab" );       // the shortest first
  T[ 4 ].push_back( pre );
  for( auto & X : T ) {
   auto Y = X , W = X , V = X;
   msdRadixSort( Y );
   sort( W );
   multikeyQuickSort( V );
   std::sort( X.begin() , X.end() );
   CHECK( Y == X && W == X && V == X );
   }
  }
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
