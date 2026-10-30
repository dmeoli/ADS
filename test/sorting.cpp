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
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/sequence/List.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/ExchangeSorts.h"
#include "ADS/sort/LearnedSort.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/SortingNetworks.h"
#include "ADS/sort/StrandSort.h"
#include "ADS/sort/StringSort.h"
#include "ADS/sort/TreeSort.h"
#include "ADS/sort/VectorQuickSort.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
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

/// the stable reference: std::stable_sort with a comparator of its own,
/// since with std::less a library may sort the floating point by radix,
/// -0 before +0, which is not stable on them
template< class T >
void stdStable( std::vector< T > & V )
{
 std::stable_sort( V.begin() , V.end() ,
                   []( const T & a , const T & b ) { return( a < b ); } );
 }

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

// the tree sort on a Red-Black tree and on an unbalanced tree, the repeated
// items kept, empty, all equal, and the unbalanced tree as a chain of 20000
// nodes on a reversed input
ADS_TEST( "tree-sort" )
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
 Ints E , Q( 50 , 2 );
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
 CHECK( k == 0 && E.empty() );
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

// the engineered quicksorts on the battery, and on inputs against the
// classical quicksort: a killer of the median of three would make them
// quadratic, but the depth bound of pdqsort keeps it O( n log n ), also
// against the adversary of McIlroy that makes every pivot a smallest item;
// pdqsort takes O( n ) comparisons on a sorted vector and on a reversed
// one, whose first partition by blocks swaps the items in pairs and leaves
// both sides sorted
ADS_TEST( "engineered-quicksorts" )
{
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
 Ints R( 100000 );
 std::iota( R.rbegin() , R.rend() , 0 );
 c = 0;
 pdqSort( R , count );
 CHECK( std::is_sorted( R.begin() , R.end() ) && c < 4 * 100000 );
 Ints K( 100000 );                               // few distinct keys
 std::mt19937 g( 11 );
 for( auto & x : K ) x = int( g() % 3 );
 for( auto sort : { pdqSort< int , decltype( count ) > ,
                    dualPivotQuickSort< int , decltype( count ) > ,
                    threeWayQuickSort< int , decltype( count ) > } ) {
  Ints A = K;
  c = 0;
  sort( A , count );
  CHECK( std::is_sorted( A.begin() , A.end() ) && c < 40 * 100000 );
  }
 const int n = 20000;                 // the adversary of McIlroy: the keys
 {                                    // fixed only when compared, so that
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
  pdqSort( A , adv );
  CHECK( std::is_sorted( A.begin() , A.end() , [ & ]( int x , int y ) {
          return( val[ x ] < val[ y ] ); } ) );
  CHECK( cmp < 6L * n * 15 );         // n log n, not n^2 / 4
  }
 }

// Powersort on the battery, stable; the runs found on vectors
// made of r sorted runs, and O( n ) comparisons on a sorted vector and on
// two runs one after the other; no run on an empty vector, one on a
// strictly decreasing vector (reversed) and on all equal items
ADS_TEST( "run-sorts" )
{
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
 {
  Ints A = T;
  c = 0;
  powerSort( A , count , &runs );
  CHECK( std::is_sorted( A.begin() , A.end() ) && runs == 2 &&
         c <= 2 * 100000 + 64 );   // n - 1 to find the runs, n - 1 to merge
  }
 Ints U( 100000 );                               // two runs, one after
 for( int i = 0 ; i < 100000 ; ++i ) U[ i ] = ( i + 50000 ) % 100000;
 c = 0;
 powerSort( U , count );
 CHECK( std::is_sorted( U.begin() , U.end() ) && c < 100000 + 200 );
 Ints E , D( 100000 ) , Q( 100000 , 3 );
 std::iota( D.rbegin() , D.rend() , 0 );        // strictly decreasing
 powerSort( E , count , &runs );
 CHECK( runs == 0 && E.empty() );
 Ints A = D , B = Q;
 c = 0;
 powerSort( A , count , &runs );                 // one run, reversed
 CHECK( runs == 1 && std::is_sorted( A.begin() , A.end() ) && c < 100000 );
 c = 0;
 powerSort( B , count , &runs );                 // equals: one run
 CHECK( runs == 1 && B == Q && c < 100000 );
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

// the American flag sort against std::sort by radixKey(), bit by bit:
// sizes around kFlagSmall, where the groups leave the radix passes, and a
// group of exactly kFlagSmall items; a vector sorted, non-increasing with
// equal keys (reversed whole) and strictly decreasing; keys that differ in
// one byte alone (counted), in the top byte alone, in two bytes far apart
// (a single bucket skipped between them) and only below a shared top byte;
// all equal; and for the floating point numbers -0 and +0, the infinities,
// the denormals and the NaN of both signs spread over a large vector, whose
// groups the vector quicksort or pdqsort must put in the order of the keys
template< class T >
void flag_of( unsigned seed )
{
 using U = decltype( radixKey( T() ) );
 constexpr int W = int( sizeof( U ) );
 std::mt19937_64 g( seed );
 auto bits = []( const std::vector< T > & A ) {
  std::vector< U > K( A.size() );
  for( std::size_t i = 0 ; i < A.size() ; ++i ) K[ i ] = radixKey( A[ i ] );
  return( K ); };
 auto check = [ & ]( std::vector< T > A ) {
  auto R = bits( A );
  std::sort( R.begin() , R.end() );
  americanFlagSort( A );
  CHECK( bits( A ) == R );
  };
 auto of = [ & ]( U k ) {                   // the number of key k
  if constexpr( std::is_floating_point_v< T > ) {
   constexpr U sign = U( U( 1 ) << ( 8 * W - 1 ) );
   return( std::bit_cast< T >( U( ( k & sign ) ? ( k ^ sign ) : ~k ) ) );
   }
  else
   return( T( k ^ ( std::is_signed_v< T > ? U( U( 1 ) << ( 8 * W - 1 ) )
                                          : U( 0 ) ) ) );
  };
 const std::size_t f = kFlagSmall;
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) , f - 1 , f ,
                        f + 1 , 3 * f + 5 } ) {
  std::vector< std::vector< U > > K( 9 , std::vector< U >( n ) );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const U r = U( g() );
   K[ 0 ][ i ] = r;                                    // random
   K[ 1 ][ i ] = U( i );                               // sorted
   K[ 2 ][ i ] = U( ( n - i ) / 3 );                   // non-increasing
   K[ 3 ][ i ] = W > 2 ? U( ( U( n - i ) << ( 8 * W - 16 ) ) | ( n - i ) )
                       : U( n - i );           // strictly decreasing
   K[ 4 ][ i ] = U( U( r & 0xFF ) << ( 8 * ( W - 1 ) ) );   // one byte
   K[ 5 ][ i ] = U( r & 0x0F );                        // the low byte
   K[ 6 ][ i ] = W > 1 ? U( ( r & 0x3 ) | ( U( r >> 8 & 0x3 ) <<
                                            ( 8 * ( W - 1 ) ) ) )
                       : U( r );                       // bytes far apart
   K[ 7 ][ i ] = W > 1 ? U( ( U( 0x5A ) << ( 8 * ( W - 1 ) ) ) |
                            ( r >> 8 ) ) : U( 7 );     // the top shared
   K[ 8 ][ i ] = U( 42 );                              // all equal
   }
  if( f + 1 == n ) {                         // a bucket of kFlagSmall
   for( std::size_t i = 0 ; i < n ; ++i )
    K[ 0 ][ i ] = W > 1 ? U( ( U( i == 0 ) << ( 8 * ( W - 1 ) ) ) |
                             ( U( g() ) & U( 0xFFFF ) ) ) : U( g() );
   }
  for( const auto & k : K ) {
   std::vector< T > A( n );
   for( std::size_t i = 0 ; i < n ; ++i ) A[ i ] = of( k[ i ] );
   check( A );
   }
  }
 if constexpr( std::is_floating_point_v< T > ) {
  constexpr T inf = std::numeric_limits< T >::infinity() ,
              nan = std::numeric_limits< T >::quiet_NaN() ,
              den = std::numeric_limits< T >::denorm_min();
  const T odd[] = { -T( 0 ) , T( 0 ) , inf , -inf , den , -den , nan ,
                    std::copysign( nan , T( -1 ) ) ,
                    std::numeric_limits< T >::max() ,
                    std::numeric_limits< T >::lowest() };
  for( std::size_t n : { f / 2 , 3 * f } ) {
   std::vector< T > A( n );
   std::uniform_real_distribution< T > u( -2 , 2 );
   for( auto & x : A ) x = u( g );
   for( std::size_t i = 0 ; i < n ; i += 7 ) A[ i ] = odd[ g() % 10 ];
   check( A );
   std::vector< T > B( n );                  // a shared top byte, and
   for( auto & x : B ) x = odd[ g() % 3 ];   // the zeros and inf alone
   check( B );
   }
  }
 }

ADS_TEST( "american-flag" )
{
 flag_of< std::uint8_t >( 1 );
 flag_of< std::int16_t >( 2 );
 flag_of< std::uint32_t >( 3 );
 flag_of< int >( 4 );
 flag_of< long long >( 5 );
 flag_of< std::uint64_t >( 6 );
 flag_of< float >( 7 );
 flag_of< double >( 8 );
 }

// the bucket sort on both sides of the cache (1 MiB of items, one group of
// buckets below and groups of 1024 above), with n not a multiple of 1024:
// uniform numbers, numbers crowded into a few buckets (more than 32 items
// each, sorted by pdqsort) and into the last group alone, all equal, -0
// with +0, the largest number below 1, and floats past 2^24 items, whose
// products by n round; all against std::sort
template< class T >
void bucket_of( unsigned seed )
{
 std::mt19937_64 g( seed );
 std::uniform_real_distribution< T > u( 0 , 1 );
 const T below = std::nextafter( T( 1 ) , T( 0 ) );
 const std::size_t c = ( std::size_t( 1 ) << 20 ) / sizeof( T );
 for( std::size_t n : { c - 1 , c , c + 1 , 3 * c + 777 } ) {
  std::vector< std::vector< T > > X( 5 , std::vector< T >( n ) );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   X[ 0 ][ i ] = u( g );                                  // uniform
   X[ 1 ][ i ] = T( g() % 7 ) / T( 7 );                   // 7 crowds
   X[ 2 ][ i ] = below - T( g() % 1000 ) * T( 1e-7 );     // the top
   X[ 3 ][ i ] = T( 0.5 );                                // all equal
   X[ 4 ][ i ] = i % 3 ? u( g ) : ( i % 2 ? T( 0 ) : -T( 0 ) );
   }
  for( auto & A : X ) {
   auto E = A;
   std::sort( E.begin() , E.end() );
   bucketSort( A );
   CHECK( A == E );
   }
  }
 }

ADS_TEST( "bucket-sort" )
{
 bucket_of< double >( 1 );
 bucket_of< float >( 2 );
 std::vector< float > F( ( std::size_t( 1 ) << 24 ) + 3 );
 std::mt19937 g( 3 );
 std::uniform_real_distribution< float > u( 0 , 1 );
 for( auto & x : F ) x = u( g );
 auto E = F;
 std::sort( E.begin() , E.end() );
 bucketSort( F );
 CHECK( F == E );
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
 stdStable( Zs );
 CHECK( Z.back() == 1.0 );
 for( std::size_t i = 0 ; i < Z.size() ; ++i )
  CHECK( std::signbit( Z[ i ] ) == std::signbit( Zs[ i ] ) );
 }

// the edges of the mechanisms of the fast sorts: the cutoffs of pdqSort()
// (insertion below 24, ninther above 128, blocks of 64) on organ pipe,
// sawtooth, two values, all equal, sorted, and all equal but for one
// item at an end, the scans of the partitions bounded only by the samples
// of the pivot, with doubles (the partition by blocks) and records; few
// distinct keys at random sizes, whose runs of equals go to the partition
// with the equal keys on the left, in int64 and strings too; the killer of
// the median of 3 of Musser
ADS_TEST( "pdq-edges" )
{
 using R = std::pair< int , std::size_t >;
 auto byKey = []( const R & a , const R & b ) {
  return( a.first < b.first ); };
 std::mt19937 g( 31 );
 for( std::size_t n : { 0u , 1u , 2u , 23u , 24u , 25u , 127u , 128u ,
                        129u , 191u , 192u , 193u , 257u } )
  for( int p = 0 ; p < 9 ; ++p ) {
   std::vector< double > D( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    switch( p ) {
     case( 0 ): D[ i ] = double( std::min( i , n - 1 - i ) ); break;
     case( 1 ): D[ i ] = double( i % 7 ); break;
     case( 2 ): D[ i ] = ( g() % 2 ) ? 1e300 : -1e300; break;
     case( 3 ): D[ i ] = 0.5; break;
     case( 4 ): D[ i ] = double( n - i ); break;
     case( 6 ): D[ i ] = double( i ); break;
     case( 7 ): D[ i ] = i + 1 == n ? 1.0 : 5.0; break;   // one smaller
     case( 8 ): D[ i ] = i == 0 ? 9.0 : 5.0; break;       // one larger
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
 for( int t = 0 ; t < 300 ; ++t ) {              // few distinct keys
  const std::size_t n = 24 + g() % 3000;
  const unsigned k = std::array< unsigned , 4 >{ 1 , 2 , 3 , 16 }[ t % 4 ];
  std::vector< std::int64_t > I( n );
  std::vector< std::string > S( n );
  std::vector< R > Q( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   I[ i ] = std::int64_t( g() % k ) - 1;
   S[ i ] = std::string( 20 , char( 'a' + I[ i ] + 1 ) );
   Q[ i ] = { int( I[ i ] ) , i };
   }
  auto IE = I;
  auto SE = S;
  auto Q2 = Q;
  pdqSort( I );
  pdqSort( S );
  pdqSort( Q , byKey );
  std::sort( IE.begin() , IE.end() );
  std::sort( SE.begin() , SE.end() );
  std::stable_sort( Q2.begin() , Q2.end() , byKey );
  CHECK( I == IE && S == SE );
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
// galloping, and in blocks of 64 and then of 1, so that the galloping pays
// down to a threshold of 1 and then fails; all against std::stable_sort
ADS_TEST( "merge-paths" )
{
 std::mt19937 g( 21 );
 using D = std::vector< double >;
 auto stableAs = []( const D & A , D B ) {      // the signs too
  stdStable( B );
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
   stdStable( run );
   if( r % 2 )                                   // strictly decreasing
    std::reverse( run.begin() , run.end() );     // only if no equals
   L.insert( L.end() , run.begin() , run.end() );
   }
  for( const D & Y : { X , L } ) {
   D B = Y , C = Y;
   powerSort( B );
   stableSort( C );
   CHECK( stableAs( B , Y ) && stableAs( C , Y ) );
   }
  }
 using R = std::pair< int , std::size_t >;
 auto byKey = []( const R & a , const R & b ) {
  return( a.first < b.first ); };
 for( std::size_t n : { 0u , 1u , 2u , 23u , 24u , 25u , 100u , 511u ,
                        512u , 513u , 5000u } )
  for( int p = 0 ; p < 11 ; ++p ) {
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
     case( 10 ): {                                  // 64 , then 1
      const std::size_t j = i < h ? i : i - h , o = i < h ? 0 : 1;
      k = j < h / 2 ? int( ( j / 64 ) * 128 + 64 * o + j % 64 )
                    : int( ( 1 << 20 ) + 2 * j + o );
      }
      break;
     default: {                                     // blocks of L
      const std::size_t j = i < h ? i : i - h;
      k = int( ( j / L ) * 2 * L + ( i < h ? 0 : L ) + j % L );
      }
     }
    Q[ i ] = { k , i };
    }
   auto B = Q , C = Q , E = Q;
   powerSort( B , byKey );
   stableSort( C , byKey );
   std::stable_sort( E.begin() , E.end() , byKey );
   CHECK( B == E && C == E );
   }
 for( int t = 0 ; t < 200 ; ++t ) {             // randomized
  const std::size_t n = g() % 3000;
  std::vector< R > Q( n );
  const int keys = 1 + int( g() % 50 );
  for( std::size_t i = 0 ; i < n ; ++i ) Q[ i ] = { int( g() % keys ) , i };
  if( t % 3 == 1 )                              // partly presorted
   std::sort( Q.begin() , Q.begin() + n / 2 , byKey );
  auto A = Q , E = Q;
  powerSort( A , byKey );
  std::stable_sort( E.begin() , E.end() , byKey );
  CHECK( A == E );
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
// or the sign, all equal, two values, and its counting sort of one byte on
// all 256 values, signed, on -0 and +0, and not used on a key that does
// not tell the item; the MSD sort of strings around the
// insertion cutoff of 16, groups of equal strings, prefixes of each other,
// the shortest first, empty strings and long shared prefixes, cut by the
// last string at their last place or in their middle, and the bytes 0 and
// 255 inside
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
  sort( V2 ); stableSort( V3 ); stdStable( Vs );
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
 stdStable( Zs );
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
 stdStable( Ys );
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
  stdStable( V3 );
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
 {                                             // the counting sort of
  std::vector< std::uint8_t > B( 3000 );        // one byte: all 256
  std::vector< std::int8_t > S( 3000 );         // values, signed, -0
  std::vector< double > Z( 3000 );              // and +0, and a key that
  std::vector< std::pair< std::uint32_t , std::size_t > > P( 3000 );
  for( std::size_t i = 0 ; i < B.size() ; ++i ) {   // does not tell the
   B[ i ] = std::uint8_t( i < 256 ? i : g() );  // item, which stays
   S[ i ] = std::int8_t( B[ i ] );              // stable
   Z[ i ] = g() % 2 ? -0.0 : 0.0;
   P[ i ] = { std::uint32_t( g() % 256 ) << 16 , i };
   }
  auto B2 = B;
  auto S2 = S;
  auto P2 = P;
  radixSort( B );
  radixSort( S );
  radixSort( Z );
  radixSortBy( P , []( const std::pair< std::uint32_t , std::size_t > & x ) {
   return( x.first ); } );
  std::sort( B2.begin() , B2.end() );
  std::sort( S2.begin() , S2.end() );
  std::stable_sort( P2.begin() , P2.end() , []( const auto & x ,
                                                const auto & y ) {
   return( x.first < y.first ); } );
  CHECK( B == B2 && S == S2 && P == P2 );
  CHECK( std::is_partitioned( Z.begin() , Z.end() , []( double x ) {
   return( std::signbit( x ) ); } ) && Z.front() == 0.0 );
  }
 const std::string pre( 1000 , 'p' );
 std::string cut = pre;                        // the prefix cut in the
 cut[ 500 ] = 'q';                              // middle, by the last
 for( std::size_t c : { 15u , 16u , 17u , 18u , 40u } ) {   // groups
  std::vector< std::vector< std::string > > T( 10 );
  for( std::size_t i = 0 ; i < c ; ++i ) {
   T[ 0 ].push_back( "abc" );                   // all equal
   T[ 1 ].push_back( "abc" );                   // and a longer one
   T[ 2 ].push_back( "abc" );                   // and a shorter one
   T[ 3 ].push_back( "" );                      // empty strings
   T[ 4 ].push_back( pre + char( 'a' + g() % 3 ) );   // shared prefix
   T[ 5 ].push_back( std::string( g() % 5 , 'x' ) );  // prefixes
   T[ 6 ].push_back( i % 2 ? "ab" : "abc" );
   T[ 7 ].push_back( pre + "b" );
   T[ 8 ].push_back( pre );
   T[ 9 ].push_back( std::string( "a\0\xff" , 2 + i % 2 ) );   // bytes 0
   }                                            // and 255
  T[ 7 ].push_back( pre + "a" );                // a cut at the last place
  T[ 8 ].push_back( cut );
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

// the fast paths of sort() and stableSort(): the stable merge sort of
// items with an order of their own, around its insertion cutoff of 16 and
// the parity of the halves, on few keys, sorted, reversed, all equal,
// organ pipe and sawtooth, with halves already in order or in reverse,
// against std::stable_sort; the sort by few keys around kRadix, with 1,
// 2, 16 and 17 keys, and with a key out of the sample, at the end or the
// largest, which falls back to the merge sort; the runs counted in blocks
// of 32, ending around the multiples of 32, against a count item by item;
// the merges of the runs with the shorter one on either side, of numbers
// and of pairs with ties; the radix sort of vectors around the size of the
// cache, the first pass by the top byte, the parts of at most 32 items, a
// byte that does not split a part, keys of 3 bytes and -0 with +0 among
// few doubles; the strings of few distinct values, around the cutoff of
// 16, all equal, with the empty ones and those that end inside a bucket
/// the bits of x as an unsigned integer, so that -0, +0 and the NaN of
/// different signs are told apart
template< class T >
auto bitsOf( T x )
{
 std::conditional_t< sizeof( T ) == 4 , std::uint32_t , std::uint64_t > b;
 std::memcpy( &b , &x , sizeof( b ) );
 return( b );
 }

/// the order of vectorQuickSort(): the natural one, extended for floating
/// point numbers to the total order of IEEE 754 (-NaN, -inf, ..., -0, +0,
/// ..., +inf, +NaN)
template< class T >
bool totalLess( T a , T b )
{
 if constexpr( std::is_floating_point_v< T > ) {
  using S = std::conditional_t< sizeof( T ) == 4 , std::int32_t ,
                                std::int64_t >;
  using U = std::make_unsigned_t< S >;
  auto key = []( T x ) {
   const U u = U( bitsOf( x ) );
   return( S( u ^ ( U( S( u ) >> ( 8 * sizeof( S ) - 1 ) ) >> 1 ) ) ); };
  return( key( a ) < key( b ) );
  }
 else
  return( a < b );
 }

/// vectorQuickSort() and its portable registers on A, against std::sort
/// by the total order, bit by bit
template< class T >
bool vectorSortsLike( const std::vector< T > & A )
{
 using K = detail::vqs::KeyOf< T >;
 auto B = A , C = A , R = A;
 vectorQuickSort( B );
 detail::vqs::sortWith< detail::vqs::Lanes< K > >( C.data() , C.size() );
 std::sort( R.begin() , R.end() , totalLess< T > );
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  if( ( bitsOf( B[ i ] ) != bitsOf( R[ i ] ) ) ||
      ( bitsOf( C[ i ] ) != bitsOf( R[ i ] ) ) )
   return( false );
 return( true );
 }

/// the battery of vectorQuickSort() on the numbers of type T: every size
/// up to 600 (the networks of 1 to 16 registers and the first partitions),
/// then larger ones, in the patterns random, few keys, two keys, all
/// equal, sorted, reversed, organ pipe, the extremes of T and, for the
/// floating point numbers, the zeros, the infinities and the NaN, a few
/// keys at either end of T, a few keys with one far from them, inside or
/// among the last keys (the counting sorts that give up), and 16 keys of
/// any bits
template< class T >
void vectorBattery()
{
 std::mt19937_64 g( 91 );
 using L = std::numeric_limits< T >;
 std::vector< T > ext = { L::lowest() , L::max() , T( 0 ) , T( 1 ) };
 if constexpr( std::is_signed_v< T > )
  ext.push_back( T( -1 ) );
 if constexpr( std::is_floating_point_v< T > )
  for( T x : { T( -0.0 ) , L::infinity() , -L::infinity() ,
               L::quiet_NaN() , -L::quiet_NaN() , L::denorm_min() ,
               -L::denorm_min() } )
   ext.push_back( x );
 std::vector< std::size_t > sizes;
 for( std::size_t n = 0 ; n <= 600 ; ++n ) sizes.push_back( n );
 for( std::size_t n : { 1023u , 1024u , 1025u , 4095u , 4097u , 20000u ,
                        100000u } )
  sizes.push_back( n );
 std::array< T , 16 > pool;                 // 16 keys of any bits
 for( auto & x : pool ) {
  const std::uint64_t r = g();
  std::memcpy( &x , &r , sizeof( T ) );
  }
 for( std::size_t n : sizes )
  for( int p = 0 ; p < 12 ; ++p ) {
   if( ( n <= 600 ) && ( n % 7 != 0 ) && ( p > 1 ) )
    continue;                               // all sizes on random keys
   std::vector< T > A( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t r = g();
    switch( p ) {
     case( 0 ): std::memcpy( &A[ i ] , &r , sizeof( T ) );   // any bits
                if constexpr( std::is_floating_point_v< T > )
                 if( A[ i ] != A[ i ] ) A[ i ] = T( r % 1000 );
                break;
     case( 1 ): A[ i ] = T( r % 16 ); break;
     case( 2 ): A[ i ] = r % 2 ? L::max() : T( 3 ); break;
     case( 3 ): A[ i ] = L::max(); break;
     case( 4 ): A[ i ] = T( i ); break;
     case( 5 ): A[ i ] = T( n - i ); break;
     case( 6 ): A[ i ] = T( std::min( i , n - 1 - i ) ); break;
     case( 7 ): A[ i ] = ext[ r % ext.size() ]; break;
     case( 8 ): A[ i ] = n % 2 ? T( L::max() - T( r % 20 ) ) :   // a few
                         T( L::lowest() + T( r % 20 ) );   // keys at an end
                break;
     case( 11 ): A[ i ] = pool[ r % 16 ]; break;
     default:   A[ i ] = T( r % 16 );       // a few keys and one apart
     }
    }
   if( ( p >= 9 ) && ( p <= 10 ) && ( n > 0 ) )   // inside, or last
    A[ p == 9 ? n * 7 / 8 : n - 1 ] = T( 100 );
   CHECK( vectorSortsLike( A ) );
   }
 }

ADS_TEST( "vector-quicksort" )
{
 vectorBattery< std::int32_t >();
 vectorBattery< std::uint32_t >();
 vectorBattery< std::int64_t >();
 vectorBattery< std::uint64_t >();
 vectorBattery< float >();
 vectorBattery< double >();
 vectorBattery< long long >();
 vectorBattery< unsigned long long >();
 std::mt19937_64 g( 92 );                   // the last resort, heapsort,
 for( std::size_t n : { 0u , 1u , 2u , 3u , 100u , 1001u } ) {   // alone
  std::vector< std::int64_t > A( n );
  for( auto & x : A ) x = std::int64_t( g() );
  auto B = A;
  detail::vqs::heapKeys( A.data() , n );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 for( std::size_t n : { 600u , 5000u } ) {  // the recursion with no depth
  std::vector< std::int32_t > A( n );       // left, at once to heapsort
  for( auto & x : A ) x = std::int32_t( g() );
  auto B = A;
  std::uint64_t s = 1;
  detail::vqs::quick< detail::vqs::Best< std::int32_t > >( A.data() , n , 0 ,
                                                          s );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 std::vector< double > A = { 2.0 , -0.0 , 0.0 , -0.0 , 1.0 };
 vectorQuickSort( A );                      // -0 before +0
 CHECK( std::signbit( A[ 0 ] ) && std::signbit( A[ 1 ] ) &&
        ! std::signbit( A[ 2 ] ) && ( A[ 4 ] == 2.0 ) );
 }

/// detail::vqs::mergeRuns() of V forward and backward, with the run that
/// stays in place at the end of the output (at its start backward), on
/// runs of every length up to 70 and some longer, of random keys and of
/// few, against std::merge
template< class V >
void mergeKeys()
{
 using K = typename V::K;
 std::mt19937_64 g( 96 );
 std::vector< std::size_t > lens;
 for( std::size_t l = 0 ; l <= 70 ; ++l ) lens.push_back( l );
 for( std::size_t l : { 127u , 128u , 129u , 1000u , 4099u } )
  lens.push_back( l );
 for( std::size_t na : lens )
  for( std::size_t nb : lens ) {
   if( ( na > 70 ) && ( nb > 70 ) && ( na != nb ) )
    continue;
   for( int p = 0 ; p < 2 ; ++p ) {
    std::vector< K > A( na ) , B( nb ) , R( na + nb );
    for( auto & x : A ) x = p ? K( g() % 5 ) : K( g() );
    for( auto & x : B ) x = p ? K( g() % 5 ) : K( g() );
    std::sort( A.begin() , A.end() );
    std::sort( B.begin() , B.end() );
    std::merge( A.begin() , A.end() , B.begin() , B.end() , R.begin() );
    std::vector< K > F( na + nb ) , D( na + nb );
    std::copy( B.begin() , B.end() , F.begin() + std::ptrdiff_t( na ) );
    detail::vqs::mergeRuns< V , false >( A.data() , na , F.data() + na ,
                                         nb , F.data() );
    std::copy( B.begin() , B.end() , D.begin() );
    detail::vqs::mergeRuns< V , true >( A.data() , na , D.data() , nb ,
                                        D.data() );
    CHECK( ( F == R ) && ( D == R ) );
    }
   }
 }

/// mergeKeys() on the registers of K, where K is its own key
template< class K >
void mergeBattery()
{
 using V = detail::vqs::Best< detail::vqs::KeyOf< K > >;
 if constexpr( std::is_same_v< typename V::K , K > )   // its own key
  mergeKeys< V >();
 }

ADS_TEST( "vector-merge" )
{
 mergeBattery< std::int32_t >();
 mergeBattery< std::uint32_t >();
 mergeBattery< std::int64_t >();
 mergeBattery< std::uint64_t >();
 std::mt19937_64 g( 97 );                   // sort() on 2 to 17 runs,
 for( std::size_t n : { 600u , 5000u , 70001u } )   // some decreasing
  for( std::size_t r = 2 ; r <= 17 ; ++r ) {
   std::vector< std::uint32_t > U( n );
   std::vector< std::int64_t > S( n );
   std::size_t at = 0;
   for( std::size_t k = 0 ; k < r ; ++k ) {
    const std::size_t e = k + 1 == r ? n : at + ( n / r ) +
                                           ( g() % ( n / r / 2 + 1 ) );
    std::vector< std::uint32_t > run;
    for( ; at < std::min( e , n ) ; ++at ) run.push_back(
     std::uint32_t( g() % ( k % 3 == 0 ? 7 : 1000000 ) ) );
    std::sort( run.begin() , run.end() );
    if( ( k % 2 == 1 ) && ( std::adjacent_find( run.begin() ,
                                                run.end() ) == run.end() ) )
     std::reverse( run.begin() , run.end() );   // strictly decreasing
    std::copy( run.begin() , run.end() ,
               U.begin() + std::ptrdiff_t( at - run.size() ) );
    }
   std::vector< double > F( n ) , Z( n );   // negative, and with zeros
   for( std::size_t i = 0 ; i < n ; ++i ) {   // of either sign
    S[ i ] = std::int64_t( U[ i ] ) - 500000;
    F[ i ] = double( S[ i ] ) / 7.0;
    Z[ i ] = U[ i ] % 7 == 0 ? ( g() % 2 ? -0.0 : 0.0 ) : F[ i ];
    }
   auto RU = U;
   auto RS = S;
   auto RF = F , RZ = Z;
   std::sort( RU.begin() , RU.end() );
   std::sort( RS.begin() , RS.end() );
   std::sort( RF.begin() , RF.end() );
   std::sort( RZ.begin() , RZ.end() );
   sort( U );
   stableSort( S );
   stableSort( F );
   sort( Z );
   CHECK( ( U == RU ) && ( S == RS ) && ( F == RF ) && ( Z == RZ ) );
   }
 }

ADS_TEST( "prefix-key-sort" )
{
 std::mt19937_64 g( 95 );
 auto check = [ & ]( std::vector< std::string > A ) {
  auto B = A , C = A;
  prefixKeySort( A );
  std::sort( B.begin() , B.end() );
  sort( C );
  CHECK( ( A == B ) && ( C == B ) );
  };
 for( std::size_t n : { 0u , 1u , 2u , 3u , 8u , 9u , 100u , 1000u ,
                        20000u } )
  for( int p = 0 ; p < 9 ; ++p ) {
   std::vector< std::string > A( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    std::string & x = A[ i ];
    const std::size_t len = g() % 24;
    switch( p ) {
     case( 0 ):                             // random, any length
      for( std::size_t c = 0 ; c < len ; ++c ) x += char( 'a' + g() % 26 );
      break;
     case( 1 ):                             // a long common prefix
      x = std::string( 40 , 'p' );
      for( std::size_t c = 0 ; c < len ; ++c ) x += char( 'a' + g() % 3 );
      break;
     case( 2 ): x = "same string"; break;   // all equal
     case( 3 ):                             // the character 0, any byte
      for( std::size_t c = 0 ; c < len % 6 ; ++c )
       x += char( g() % 3 == 0 ? 0 : g() % 256 );
      break;
     case( 4 ): x = std::string( i % 200 , 'a' ) + 'b'; break;   // a ladder
     case( 5 ): x = std::string( g() % 12 , '\0' ); break;   // zeros alone
     case( 6 ):                             // few distinct
      x = "key" + std::to_string( g() % 7 );
      break;
     case( 7 ):                             // sorted, numbered
      x = std::to_string( 1000000 + i );
      break;
     default:                               // empty or one character
      if( g() % 2 ) x = char( g() % 256 );
     }
    }
   check( A );
   }
 std::vector< std::string > L( 3000 );      // long strings that differ
 for( std::size_t i = 0 ; i < L.size() ; ++i )   // only at the end
  L[ i ] = std::string( 5000 , 'z' ) + std::to_string( g() % 100000 );
 check( L );
 }

ADS_TEST( "vector-dispatch" )
{
 for( std::size_t n : { 512u , 513u , 640u , 1000u , 4096u , 4099u ,
                        16384u , 16385u , 50001u , 140001u } ) {
  const std::size_t q = n / 4;
  std::vector< std::size_t > at = { 1 , 2 , 31 , 32 , 33 , 34 , 64 , 65 ,
                                    n - 2 , n - 1 , n - 31 , n - 32 ,
                                    n - 33 , n - 64 , n - 65 };
  for( std::size_t d = 0 ; d < 5 ; ++d )   // the middle
   at.push_back( n / 2 + d - 2 );
  for( std::size_t s = 1 ; s < 4 ; ++s )   // the borders of the quarters
   for( std::size_t d = 0 ; d < 5 ; ++d )
    at.push_back( s * q + d - 2 );
  for( std::size_t j : at ) {
   std::vector< std::int64_t > U( n ) , D( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    U[ i ] = std::int64_t( i );
    D[ i ] = std::int64_t( n - i );
    }
   std::swap( U[ j ] , U[ j - 1 ] );        // one place out of order
   std::swap( D[ j ] , D[ j - 1 ] );
   for( auto * V : { &U , &D } ) {
    auto W = *V , R = *V;
    sort( *V );
    stableSort( W );
    std::sort( R.begin() , R.end() );
    CHECK( ( *V == R ) && ( W == R ) );
    }
   }
  std::vector< std::int64_t > E( n , 3 );   // all equal: sorted, not
  sort( E );                                // strictly decreasing
  CHECK( E == std::vector< std::int64_t >( n , 3 ) );
  }
 std::mt19937_64 g( 94 );                   // -0 and +0 keep their order
 for( std::size_t n : { 10u , 600u , 5000u } )   // in stableSort()
  for( int p = 0 ; p < 3 ; ++p ) {
   std::vector< double > A( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    A[ i ] = p == 0 ? ( g() % 2 ? -0.0 : 0.0 ) :
             p == 1 ? double( std::int64_t( g() % 5 ) - 2 ) * ( g() % 2 ) :
             double( n - i ) * ( i % 7 == 0 ? 0.0 : 1.0 );
   if( p == 1 )
    for( auto & x : A ) if( x == 0.0 && ( g() % 2 ) ) x = -0.0;
   auto B = A;
   stableSort( A );
   std::stable_sort( B.begin() , B.end() , []( double x , double y ) {
    return( x < y ); } );
   for( std::size_t i = 0 ; i < n ; ++i )
    CHECK( bitsOf( A[ i ] ) == bitsOf( B[ i ] ) );
   }
 }

ADS_TEST( "sample-sort" )
{
 struct Rec { std::uint64_t key; std::uint64_t pad[ 3 ]; };
 auto byKey = []( const Rec & a , const Rec & b ) {
  return( a.key < b.key ); };
 std::mt19937_64 g( 93 );
 for( std::size_t n : { 0u , 1u , 2u , 64u , 65u , 100u , 1000u , 2185u ,
                        4096u , 4097u , 30000u , 300000u } )
  for( int p = 0 ; p < 8 ; ++p ) {
   std::vector< Rec > A( n );
   std::vector< std::uint64_t > U( n );
   std::vector< std::string > S( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t k[] = { g() , g() % 16 , g() % 2 , 7 , i , n - i ,
                                std::min( i , n - 1 - i ) , g() % 1000 };
    A[ i ] = Rec{ k[ p ] , { i , 0 , 0 } };
    U[ i ] = k[ p ];
    S[ i ] = "k" + std::to_string( k[ p ] % 100003 );
    }
   auto B = A;
   auto V = U;
   auto T = S;
   sampleSort( A , byKey );
   std::stable_sort( B.begin() , B.end() , byKey );
   for( std::size_t i = 0 ; i < n ; ++i )
    CHECK( A[ i ].key == B[ i ].key );
   sampleSort( U , std::greater<>() );      // another order
   std::sort( V.begin() , V.end() , std::greater<>() );
   CHECK( U == V );
   if( n <= 30000 ) {                       // items that are not bytes
    sampleSort( S );
    std::sort( T.begin() , T.end() );
    CHECK( S == T );
    }
   }
 battery( []( auto & A , auto less ) { sampleSort( A , less ); } , 4096 ,
          false );
 }

ADS_TEST( "fast-paths" )
{
 std::mt19937_64 g( 57 );
 struct Rec { std::uint64_t key; std::size_t id; };
 auto byKey = []( const Rec & a , const Rec & b ) {
  return( a.key < b.key ); };
 auto same = []( const std::vector< Rec > & A ,
                 const std::vector< Rec > & B ) {
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   if( ( A[ i ].key != B[ i ].key ) || ( A[ i ].id != B[ i ].id ) )
    return( false );
  return( A.size() == B.size() ); };
 for( std::size_t n : { 0u , 1u , 2u , 3u , 15u , 16u , 17u , 31u , 32u ,
                        33u , 34u , 63u , 64u , 65u , 1000u , 4097u } )
  for( int p = 0 ; p < 9 ; ++p ) {
   std::vector< Rec > A( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t k[] = { g() , g() % 3 , i , n - i , 7 ,
                                std::min( i , n - 1 - i ) , i % 17 ,
                                i / 8 * 8 + ( ( i % 8 ) ^ 5 ) ,
                                ( n - i ) / 8 * 8 + ( ( i % 8 ) ^ 5 ) };
    A[ i ] = { k[ p ] , i };                     // random, few, sorted,
    }                                            // reversed, equal, organ,
   auto B = A;                                   // sawtooth, blocks in
   stableSort( A , byKey );                      // order and in reverse
   std::stable_sort( B.begin() , B.end() , byKey );
   CHECK( same( A , B ) );
   }
 for( std::size_t n : { 511u , 512u , 513u , 5000u } )   // few keys: 16
  for( std::uint64_t k : { 1u , 2u , 16u , 17u , 1000u } )   // and 17,
   for( int rare = 0 ; rare < 3 ; ++rare ) {    // one out of the sample
    std::vector< Rec > A( n );
    for( std::size_t i = 0 ; i < n ; ++i )
     A[ i ] = { ( g() % k ) * 1000003 , i };
    if( rare == 1 )
     A[ n - 1 ].key = 7;                         // at the end
    if( rare == 2 )
     A[ n / 2 ].key = 999999999999;              // the largest
    auto B = A , C = A;
    stableSort( A , byKey );
    sort( C , byKey );
    std::stable_sort( B.begin() , B.end() , byKey );
    CHECK( same( A , B ) );
    for( std::size_t i = 0 ; i < n ; ++i )
     CHECK( C[ i ].key == B[ i ].key );
    }
 for( std::size_t n : { 31u , 32u , 33u , 63u , 64u , 65u , 97u , 128u ,
                        1000u } )
  for( int t = 0 ; t < 40 ; ++t ) {              // runs of every length
   std::vector< double > V( n );
   std::size_t i = 0;
   while( i < n ) {
    const std::size_t l = std::min< std::size_t >( n - i ,
                                                  t < 20 ? 30 + g() % 6 :
                                                  1 + g() % 70 );
    const bool down = g() % 2;
    for( std::size_t j = 0 ; j < l ; ++j )
     V[ i + j ] = down ? double( l - j ) : double( j / 2 );
    if( ( ! down ) && ( l > 3 ) && ( g() % 2 ) )
     V[ i + 1 ] = -0.0;                          // a tie of -0 and +0
    i += l;
    }
   auto less = std::less<>();
   std::array< std::size_t , kRuns + 1 > E , F;
   const std::size_t r = detail::runs( V , less , kRuns , E.data() );
   std::size_t q = 0;                            // item by item
   for( std::size_t k = 0 ; ( k < n ) && ( q <= kRuns ) ; ++q ) {
    std::size_t j = k + 1;
    if( ( j < n ) && ( V[ j ] < V[ k ] ) )
     while( ( j < n ) && ( V[ j ] < V[ j - 1 ] ) ) ++j;
    else
     while( ( j < n ) && ! ( V[ j ] < V[ j - 1 ] ) ) ++j;
    F[ q ] = k = j;
    }
   CHECK( r == q && std::equal( E.begin() , E.begin() + r , F.begin() ) );
   auto W = V , Ws = V;
   stableSort( W );
   stdStable( Ws );
   for( std::size_t k = 0 ; k < n ; ++k )
    CHECK( W[ k ] == Ws[ k ] &&
           std::signbit( W[ k ] ) == std::signbit( Ws[ k ] ) );
   }
 for( int t = 0 ; t < 60 ; ++t ) {               // the shorter run on
  const std::size_t a = 1 + g() % 3000 , b = 1 + g() % 3000;   // either
  std::vector< std::pair< int , std::size_t > > P( a + b );    // side
  Ints I( a + b );
  for( std::size_t i = 0 ; i < a + b ; ++i ) {
   const int k = int( g() % ( t % 2 ? 7 : 100000 ) );
   P[ i ] = { k , i };
   I[ i ] = k;
   }
  auto first = []( const auto & x , const auto & y ) {
   return( x.first < y.first ); };
  std::stable_sort( P.begin() , P.begin() + a , first );
  std::stable_sort( P.begin() + a , P.end() , first );
  std::sort( I.begin() , I.begin() + a );
  std::sort( I.begin() + a , I.end() );
  auto P2 = P;
  auto I2 = I;
  powerSort( P2 , first );
  powerSort( I2 );
  std::stable_sort( P.begin() , P.end() , first );
  std::sort( I.begin() , I.end() );
  CHECK( P2 == P && I2 == I );
  }
 const std::size_t c64 = ( std::size_t( 1 ) << 20 ) / 8;   // the cache
 for( std::size_t n : { c64 - 1 , c64 , c64 + 1 , 3 * c64 } )
  for( int p = 0 ; p < 5 ; ++p ) {
   std::vector< long long > L( n );
   std::vector< double > Z( n );
   std::vector< std::uint32_t > U( 2 * n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t x = g();
    const std::uint64_t k[] = { x , x & 0xFFFFFF ,    // all, 3 bytes,
                                ( x % 1000 ? 0 : ( x % 7 + 1 ) << 56 ) |
                                ( x & 0xFFFFFF ) ,    // parts of <= 32,
                                ( x % 2 ) << 56 | ( x % 2 ) << 49 |
                                ( x & 0xFFFF00 ) ,    // no split in a
                                x % 16 };             // part, few
    L[ i ] = ( long long )( k[ p ] );
    Z[ i ] = p == 4 ? double( int( x % 5 ) - 2 ) * ( x % 3 ? 1.0 : 0.0 ) *
                      ( x % 7 < 3 ? -1.0 : 1.0 ) : double( L[ i ] ) / 1e3;
    U[ 2 * i ] = std::uint32_t( k[ p ] );
    U[ 2 * i + 1 ] = std::uint32_t( k[ p ] >> 8 );
    }
   auto L2 = L , L3 = L;
   auto U2 = U;
   auto Z2 = Z , Z3 = Z;
   sort( L2 );
   stableSort( L3 );
   sort( U2 );
   stableSort( Z2 );
   std::sort( L.begin() , L.end() );
   std::sort( U.begin() , U.end() );
   stdStable( Z3 );
   CHECK( L2 == L && L3 == L && U2 == U );
   for( std::size_t i = 0 ; i < n ; ++i )
    CHECK( Z2[ i ] == Z3[ i ] &&
           std::signbit( Z2[ i ] ) == std::signbit( Z3[ i ] ) );
   std::vector< std::pair< std::uint64_t , std::size_t > > P( n );
   for( std::size_t i = 0 ; i < n ; ++i ) P[ i ] = { L[ ( i * 7919 ) % n ] ,
                                                    i };
   auto P2 = P;
   radixSortBy( P , []( const auto & x ) { return( x.first ); } );
   std::stable_sort( P2.begin() , P2.end() , []( const auto & x ,
                                                 const auto & y ) {
    return( x.first < y.first ); } );
   CHECK( P == P2 );
   }
 for( std::size_t n : { 15u , 16u , 17u , 18u , 33u , 5000u } )
  for( int p = 0 ; p < 4 ; ++p ) {
   std::vector< std::string > S( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::size_t k = g() % 4;
    S[ i ] = p == 0 ? "item" + std::string( 12 , char( 'a' + k ) ) :
             p == 1 ? std::string( "same" ) :
             p == 2 ? std::string( k , 'z' ) :      // empty, and ended
                      "ab" + std::string( 1 , char( 'a' + k ) ) +
                      std::string( g() % 2 , 'q' );
    }
   auto S2 = S , S3 = S , S4 = S;
   sort( S2 );
   stableSort( S3 );
   msdRadixSort( S4 );
   std::sort( S.begin() , S.end() );
   CHECK( S2 == S && S3 == S && S4 == S );
   }
 }

/// the key of rank i of n in the pattern p of the learned sort, from g:
/// random, all equal, few distinct, sorted, reversed, organ pipe, half of
/// them equal, Zipf, exponential, clustered, the ends of the type
std::uint64_t learnedKey( int p , std::size_t i , std::size_t n ,
                          std::mt19937_64 & g )
{
 const double u = double( g() >> 11 ) * 0x1p-53;
 switch( p ) {
  case( 0 ): return( g() );
  case( 1 ): return( 42 );
  case( 2 ): return( g() % 16 );
  case( 3 ): return( i << 20 );
  case( 4 ): return( ( n - i ) << 20 );
  case( 5 ): return( std::min( i , n - 1 - i ) << 20 );
  case( 6 ): return( g() % 2 ? std::uint64_t( 7 ) << 40 : g() >> 1 );
  case( 7 ): return( std::uint64_t( 1e6 / std::pow( 1 - u , 1 / 1.2 ) ) );
  case( 8 ): return( std::uint64_t( -std::log( 1 - u ) * 1e12 ) );
  case( 9 ): return( ( ( g() % 8 ) << 52 ) + g() % 1000 );
  default: {
   const std::uint64_t r = g() % 4;
   return( r == 0 ? std::uint64_t( 1 ) << 63 :
           r == 1 ? ( std::uint64_t( 1 ) << 63 ) - 1 : g() );
   }
  }
 }

/// the item of T of the key k: the integer, or a floating point number
/// whose order follows k for the patterns of smooth laws
template< class T >
T learnedItem( std::uint64_t k , int p )
{
 if constexpr( std::is_integral_v< T > )
  return( T( k ) );
 else if( p == 0 || p == 10 )
  return( T( double( std::int64_t( k ) ) / 1e3 ) );
 else
  return( T( double( k ) ) );
 }

/// learnedSort() of A against the total order of radixKey() bit for bit,
/// and against sort() when A holds no NaN
template< class T >
void learnedCheck( std::vector< T > A )
{
 auto R = A , S = A;
 learnedSort( A );
 std::sort( R.begin() , R.end() , []( T x , T y ) {
  return( radixKey( x ) < radixKey( y ) ); } );
 bool same = A.size() == R.size();
 for( std::size_t i = 0 ; same && ( i < A.size() ) ; ++i )
  same = bitsOf( A[ i ] ) == bitsOf( R[ i ] );
 CHECK( same );
 if( std::none_of( S.begin() , S.end() , []( T x ) { return( x != x ); } ) ) {
  sort( S );
  CHECK( S == A );
  }
 }

// the learned sort: the sizes around the least that the model takes, the
// patterns of learnedKey() on integers and floating point numbers of 32
// and 64 bits, the specials of IEEE 754 (-0, +0, the infinities, the NaN
// of both signs, the subnormals); the model taken on smooth laws, left to
// sampleSort() on few keys, with buckets of equality on half of them equal
ADS_TEST( "learned-sort" )
{
 const std::size_t L = 4 * detail::kLearnedBucket;
 std::mt19937_64 g( 95 );
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                        std::size_t( 2 ) , std::size_t( 100 ) , L - 1 , L ,
                        L + 1 , std::size_t( 100000 ) ,
                        std::size_t( 300001 ) } )
  for( int p = 0 ; p < 11 ; ++p ) {
   std::vector< std::uint64_t > k( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    k[ i ] = learnedKey( p , i , n , g );
   std::vector< std::int64_t > I( n );
   std::vector< std::uint64_t > U( n );
   std::vector< std::int32_t > I32( n );
   std::vector< double > D( n );
   std::vector< float > F( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    I[ i ] = learnedItem< std::int64_t >( k[ i ] , p );
    U[ i ] = k[ i ];
    I32[ i ] = std::int32_t( k[ i ] >> 32 );
    D[ i ] = learnedItem< double >( k[ i ] , p );
    F[ i ] = learnedItem< float >( k[ i ] , p );
    }
   learnedCheck( I );
   learnedCheck( U );
   learnedCheck( I32 );
   learnedCheck( D );
   learnedCheck( F );
   }
 {                                         // the specials of IEEE 754
  constexpr double inf = std::numeric_limits< double >::infinity() ,
                   nan = std::numeric_limits< double >::quiet_NaN() ,
                   sub = std::numeric_limits< double >::denorm_min();
  for( std::size_t n : { std::size_t( 50 ) , std::size_t( 100000 ) } ) {
   std::vector< double > D( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t r = g() % 16;
    D[ i ] = r == 0 ? -0.0 : r == 1 ? 0.0 : r == 2 ? inf : r == 3 ? -inf :
             r == 4 ? nan : r == 5 ? -nan : r == 6 ? sub * double( g() % 9 ) :
             double( std::int64_t( g() ) ) / 1e9;
    }
   learnedCheck( D );
   std::vector< float > F( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    F[ i ] = float( D[ i ] );
   learnedCheck( F );
   std::vector< double > Z( n );           // -0 and +0 only
   for( std::size_t i = 0 ; i < n ; ++i )
    Z[ i ] = g() % 2 ? -0.0 : 0.0;
   learnedCheck( Z );
   }
  }
 {                                         // which way the sort went
  std::vector< std::uint64_t > R( 100000 ) , E( 100000 , 5 ) , H( 100000 );
  for( std::size_t i = 0 ; i < R.size() ; ++i ) {
   R[ i ] = g();
   H[ i ] = i % 2 ? 0 : g();
   }
  CHECK( detail::learnedSortIf( R ) && std::is_sorted( R.begin() ,
                                                        R.end() ) );
  CHECK( ! detail::learnedSortIf( E ) );   // few keys: left as they are
  CHECK( E == std::vector< std::uint64_t >( 100000 , 5 ) );
  auto S = detail::learnedSample( H , 16 * 64 );
  const detail::CdfModel< std::uint64_t > M( S.data() , S.size() , 64 );
  CHECK( M.equality() && M.fits() && M.equal( 1 ) && ! M.equal( 2 ) );
  CHECK( detail::learnedSortIf( H ) && std::is_sorted( H.begin() ,
                                                        H.end() ) );
  std::vector< std::uint64_t > Z( 100000 );   // a heavy tail: no fit
  for( auto & x : Z )
   x = ( std::uint64_t( 1 ) << ( g() % 63 ) ) + g() % 1000;
  CHECK( ! detail::learnedSortIf( Z ) );
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
