/*--------------------------------------------------------------------------*/
/*------------------------- File pearls_hashing.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapters 8 and 9 of the notes of Ferragina, the dictionary
 * problem and the search of strings by prefix: the universal classes by
 * the enumeration of their functions, the hash tables against the standard
 * library on random operations, the examples of the notes with their
 * numbers, the bounds of the analysis (lists, space, false positives,
 * decoding) on random instances, and each search by prefix against the
 * binary search, at the level 3 of the contracts.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 8 and 9
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

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/CuckooHashing.h"
#include "ADS/dictionary/PatriciaTrie.h"
#include "ADS/dictionary/PerfectHashing.h"
#include "ADS/dictionary/TwoLevelDictionary.h"
#include "ADS/dictionary/UniversalHashing.h"
#include "ADS/set/BloomFilter.h"
#include "ADS/set/BloomVariants.h"
#include "ADS/string/FrontCoding.h"
#include "ADS/string/PrefixSearch.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

// a range of positions
using Range = std::pair< std::size_t , std::size_t >;

// n random strings of 0 to maxLen characters from the first sigma letters,
// sorted and distinct
std::vector< std::string > randomDictionary( std::mt19937 & g , int n ,
                                             int sigma , int maxLen )
{
 std::set< std::string > S;
 for( int i = 0 ; i < n ; ++i ) {
  std::string s( g() % unsigned( maxLen + 1 ) , 'a' );
  for( auto & c : s ) c = char( 'a' + g() % unsigned( sigma ) );
  S.insert( s );
  }
 return( std::vector< std::string >( S.begin() , S.end() ) );
 }

// every string of 0 to len characters from the first sigma letters
std::vector< std::string > allStrings( int sigma , int len )
{
 std::vector< std::string > A( 1 , "" );
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  if( int( A[ i ].size() ) < len )
   for( int c = 0 ; c < sigma ; ++c ) A.push_back( A[ i ] + char( 'a' + c ) );
 return( A );
 }

// the positions of P and P# in D by brute force
Range bruteRange( const std::vector< std::string > & D ,
                  const std::string & P )
{
 std::size_t l = 0 , r = 0;
 for( const auto & s : D ) {
  l += s < P;
  r += ( s < P ) || s.starts_with( P );
  }
 return( std::pair( l , r ) );
 }

// a dictionary S of prefix searches against the brute force on all the
// patterns of up to len characters from the first sigma + 1 letters
template< class S >
void againstBrute( const S & T , const std::vector< std::string > & D ,
                   int sigma , int len )
{
 for( const auto & P : allStrings( sigma + 1 , len ) ) {
  const auto R = bruteRange( D , P );
  CHECK( T.lexicographicPosition( P ) == R.first );
  CHECK( T.prefixEnd( P ) == R.second );
  CHECK( T.prefixRange( P ) == R );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the products mod p against values computed apart; the three universal
// classes by the enumeration of all their functions: two distinct keys
// collide under exactly m^( r - 1 ) = |H| / m dot products (Theorem 8.4),
// under at most |H| / m functions of H_{p,m} and under at most 2 |H| / m
// multiplications and shifts (Theorem 8.5); StringHash on the character 0;
// the extremes: p = 1, the factors 0 and p - 1, m = 1, a universe past
// 2^64, the multiply-shift on 64 bits, and every precondition
ADS_TEST( "universal-classes" )
{
 const std::uint64_t P = kMersenne61 , q = 4611686018427388039ULL;
 CHECK( mulMod61( P - 1 , P - 1 ) == 1 );
 CHECK( mulMod61( 123456789012345678ULL , 987654321098765432ULL ) ==
        1974130249480659620ULL );
 CHECK( mulMod( 1152921504606859321ULL , 2305843009213693949ULL , P ) ==
        2305843009213669260ULL );
 CHECK( mulMod( q - 1 , q - 2 , q ) == 2 );
 CHECK( mulMod( 1350851717672992089ULL , 1490116119384765625ULL , q ) ==
        1766865651719879378ULL );
 CHECK( mulMod( 4294967295ULL , 4294967294ULL , 4294967296ULL ) ==
        4294967295ULL * 4294967294ULL % 4294967296ULL );
 CHECK_THROWS( mulMod61( P , 1 ) , contract_violation );

 // the dot products, m = 5 and r = 3: 125 functions on 125 keys
 std::vector< DotProductHash > D;
 for( std::uint64_t a = 0 ; a < 125 ; ++a )
  D.emplace_back( 5 , std::vector< std::uint64_t >( { a % 5 , a / 5 % 5 ,
                                                      a / 25 } ) );
 for( std::uint64_t x = 0 ; x < 125 ; ++x )
  for( std::uint64_t y = x + 1 ; y < 125 ; ++y ) {
   int c = 0;
   for( const auto & h : D ) c += h( x ) == h( y );
   CHECK( c == 25 );
   }
 CHECK( D[ 7 ].universe() == 125 );
 CHECK_THROWS( D[ 7 ]( 125 ) , contract_violation );

 // H_{p,m}, p = 13 and m = 4: 12 * 13 functions on 13 keys
 for( std::uint64_t x = 0 ; x < 13 ; ++x )
  for( std::uint64_t y = x + 1 ; y < 13 ; ++y ) {
   int c = 0;
   for( std::uint64_t a = 1 ; a < 13 ; ++a )
    for( std::uint64_t b = 0 ; b < 13 ; ++b ) {
     const CarterWegmanHash h( 13 , a , b , 4 );
     c += h( x ) == h( y );
     }
   CHECK( 4 * c <= 12 * 13 );
   }

 // multiply and shift, h = 8 and l = 3: 128 odd multipliers on 256 keys
 for( std::uint64_t x = 0 ; x < 256 ; ++x )
  for( std::uint64_t y = x + 1 ; y < 256 ; ++y ) {
   int c = 0;
   for( std::uint64_t a = 1 ; a < 256 ; a += 2 ) {
    const auto h = MultiplyShiftHash::ofMultiplier( 8 , 3 , a );
    c += h( x ) == h( y );
    }
   CHECK( c <= 128 * 2 / 8 );
   }
 CHECK_THROWS( MultiplyShiftHash::ofMultiplier( 8 , 3 , 2 ) ,
               contract_violation );

 const StringHash s( 3 );
 const std::string z( 1 , '\0' ) , az = std::string( "a" ) + z;
 CHECK( s( "" ) != s( z ) && s( "a" ) != s( az ) && s( "ab" ) == s( "ab" ) );
 CHECK( s( z ) == 1 && s( z + z ) != s( z ) && s( z + z ) != s( z + z + z ) );
 CHECK( s( "" ) == 0 && s( std::string( 1000 , '\xff' ) ) < P );

 CHECK( mulMod( 0 , 0 , 1 ) == 0 && mulMod61( 0 , P - 1 ) == 0 &&
        mulMod( P - 1 , 1 , P ) == P - 1 && mulMod( q - 1 , 0 , q ) == 0 );
 CHECK( mulMod( q - 1 , q - 1 , q ) == 1 );     // ( -1 )^2
 CHECK( mulMod( 5 , 1 , 5 ) == 0 && mulMod( 6 , 7 , 5 ) == 2 );
 CHECK( mulMod( P + 2 , P + 3 , P ) == 6 );      // reduced first
 CHECK_THROWS( mulMod( 0 , 0 , 0 ) , contract_violation );
 CHECK( mulMod( 3 , 5 , std::uint64_t( 1 ) << 63 ) == 15 );
 CHECK_THROWS( mulMod61( 1 , P ) , contract_violation );
 const CarterWegmanHash c1( 13 , 1 , 12 , 1 );  // m = 1: a single list
 CHECK( c1( 0 ) == 0 && c1( 12 ) == 0 );
 const CarterWegmanHash cw( P , P - 1 , P - 1 , ~std::uint64_t( 0 ) );
 CHECK( cw( 0 ) == P - 1 && cw( 1 ) == P - 2 && cw( P - 1 ) == 0 );
 CHECK_THROWS( c1( 13 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 13 , 0 , 0 , 4 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 13 , 13 , 0 , 4 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 13 , 1 , 13 , 4 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 13 , 1 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 1 , 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( CarterWegmanHash( 0 ) , contract_violation );
 const DotProductHash W( std::uint64_t( 1 ) << 32 , 3 );   // past 2^64
 CHECK( W.universe() == 0 && W( ~std::uint64_t( 0 ) ) < ( 1ULL << 32 ) );
 CHECK( DotProductHash( 2 , 64 ).universe() == 0 &&
        DotProductHash( 2 , 63 ).universe() == ( 1ULL << 63 ) );
 CHECK_THROWS( DotProductHash( 1 , 1 ) , contract_violation );
 CHECK_THROWS( DotProductHash( 5 , 0 ) , contract_violation );
 CHECK_THROWS( DotProductHash( ( 1ULL << 32 ) + 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( DotProductHash( 5 , std::vector< std::uint64_t >() ) ,
               contract_violation );
 CHECK_THROWS( DotProductHash( 5 , std::vector< std::uint64_t >( { 5 } ) ) ,
               contract_violation );
 const auto F = MultiplyShiftHash::ofMultiplier( 64 , 63 ,
                                                 ~std::uint64_t( 0 ) );
 CHECK( F( 0 ) == 0 && F( 1 ) == ( 1ULL << 63 ) - 1 &&
        F( ~std::uint64_t( 0 ) ) == 0 );       // -1 * -1 = 1, shifted out
 CHECK( MultiplyShiftHash( 2 , 1 )( 3 ) <= 1 );
 CHECK_THROWS( MultiplyShiftHash( 8 , 8 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash( 8 , 0 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash( 65 , 3 ) , contract_violation );
 CHECK_THROWS( MultiplyShiftHash::ofMultiplier( 8 , 3 , 257 ) ,
               contract_violation );
 CHECK_THROWS( MultiplyShiftHash::ofMultiplier( 8 , 3 , 1 )( 256 ) ,
               contract_violation );
 }

// chaining with 200 functions of H_{p,m} on the multiples of m, all in one
// list for h( k ) = k mod m: the list of a key holds on average at most
// 1 + alpha keys (Theorem 8.2), the longest at most about
// 3 ln n / ln ln n (Theorem 8.3) on average over the functions, and each
// longest list within sqrt( 2 n ), as a list of L keys makes L ( L - 1 ) / 2
// of the pairs that collide, n ( n - 1 ) / ( 2 m ) expected in a universal
// class (Theorem 8.3 is about uniform hashing, and a few functions of
// H_{p,m} are far from it on these keys, whence the many functions); a
// table of one list, a key inserted again, an absent key removed, and the
// bounds of m and of chain()
ADS_TEST( "universal-chaining" )
{
 const std::uint64_t m = 1024 , n = 1024;
 const std::uint64_t F = 200;
 double sum = 0 , longest = 0;
 for( std::uint64_t seed = 1 ; seed <= F ; ++seed ) {
  const CarterWegmanHash h( m , seed );
  ChainedHashTable< std::uint64_t , int , Chain::FRONT , CarterWegmanHash >
   T( m , false , h );
  std::vector< std::uint64_t > len( m , 0 );
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   T.insert( i * m , int( i ) );
   ++len[ h( i * m ) ];
   }
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   CHECK( T.lookup( i * m ) == int( i ) );
   sum += double( len[ h( i * m ) ] );
   }
  const double L = double( *std::max_element( len.begin() , len.end() ) );
  CHECK( L <= std::sqrt( 2.0 * double( n ) ) );
  longest += L;
  }
 const double ln = std::log( double( n ) );
 CHECK( longest / double( F ) <= 3 * ln / std::log( ln ) );
 CHECK( sum / ( double( F ) * double( n ) ) <= 2.1 );   // 1 + alpha = 2
 std::set< std::uint64_t > lists;                // the division
 for( std::uint64_t i = 0 ; i < n ; ++i ) lists.insert( i * m % m );
 CHECK( lists.size() == 1 );
 const CarterWegmanHash h1( 1 );                 // m = 1: one list of all
 ChainedHashTable< std::uint64_t , int , Chain::FRONT , CarterWegmanHash >
  O( 1 , false , h1 );
 for( std::uint64_t i = 0 ; i < 50 ; ++i ) O.insert( i , int( i ) );
 O.insert( 7 , -7 );                             // a key again: replaced
 O.remove( 1000 );                               // absent: nothing
 CHECK( O.size() == 50 && O.chain( 0 ).size() == 50 &&
        O.lookup( 7 ) == -7 && ! O.lookup( 50 ) );
 CHECK_THROWS( O.chain( O.capacity() ) , contract_violation );
 CHECK_THROWS( ( ChainedHashTable< int , int >( 0 ) ) , contract_violation );
 }

// the d-left hashing against std::map on random operations for d = 1, 2
// and 4; with n = m keys the longest list drops from about ln n / ln ln n
// to about ln ln n / ln 2, the power of two choices; more tables than
// places (one list each), the empty table, a key inserted again, an absent
// key removed, and m = 0 or d = 0
ADS_TEST( "d-left" )
{
 for( int d : { 1 , 2 , 4 } ) {
  DLeftHashTable< int , int > T( 64 , d , unsigned( d ) );
  std::map< int , int > R;
  std::mt19937 g( 4 );
  for( int op = 0 ; op < 4000 ; ++op ) {
   const int k = int( g() % 300 );
   if( g() % 3 ) {
    T.insert( k , op );
    R[ k ] = op;
    }
   else {
    T.remove( k );
    R.erase( k );
    }
   const int q = int( g() % 300 );
   CHECK( T.lookup( q ) == ( R.count( q ) ? std::optional< int >( R[ q ] )
                                          : std::nullopt ) );
   }
  CHECK( T.size() == R.size() && T.tables() == d );
  std::map< int , int > I;
  for( const auto [ k , v ] : T ) I[ k ] = v;
  CHECK( I == R );
  }
 std::size_t L[ 5 ] = { 0 , 0 , 0 , 0 , 0 };
 for( int d : { 1 , 2 , 4 } ) {
  const std::size_t n = std::size_t( 1 ) << 16;
  DLeftHashTable< std::uint64_t , int > T( n , d , 7 );
  for( std::uint64_t i = 0 ; i < n ; ++i ) T.insert( i , 0 );
  L[ d ] = T.maxLoad();
  CHECK( T.size() == n );
  }
 CHECK( L[ 1 ] >= 6 && L[ 2 ] <= 5 && L[ 4 ] <= L[ 2 ] && L[ 2 ] < L[ 1 ] );
 DLeftHashTable< int , int > S( 1 , 4 );        // d > m: a list per table
 CHECK( S.size() == 0 && ! S.lookup( 0 ) && S.begin() == S.end() );
 S.remove( 3 );                                  // absent: nothing
 for( int k = 0 ; k < 20 ; ++k ) S.insert( k , k );
 S.insert( 5 , -5 );                             // a key again: replaced
 CHECK( S.size() == 20 && S.lookup( 5 ) == -5 && S.tables() == 4 &&
        S.maxLoad() >= 5 );
 CHECK_THROWS( ( DLeftHashTable< int , int >( 8 , 0 ) ) , contract_violation );
 CHECK_THROWS( ( DLeftHashTable< int , int >( 0 , 2 ) ) , contract_violation );
 }

// Figure 8.6, h( k ) = ( ( 2 k + 42 ) mod 101 ) mod 11 on its nine keys:
// 79 goes to the list 0 (( 2 * 79 + 42 ) mod 101 = 99), not to the list 9
// of the figure, so that L = 4 + 1 + 4 + 4 + 4 = 17 < 18; the searches of
// 98 and of 8 with h_3( k ) = ( ( 4 k + 42 ) mod 101 ) mod 4
ADS_TEST( "fks-example" )
{
 const std::vector< std::uint64_t > S = { 98 , 19 , 14 , 50 , 1 , 72 , 79 ,
                                          3 , 69 };
 const CarterWegmanHash h( 101 , 2 , 42 , 11 ) , h3( 101 , 4 , 42 , 4 ) ,
                        h4( 101 , 3 , 42 , 4 ) , h8( 101 , 4 , 42 , 4 );
 std::vector< std::vector< std::uint64_t > > T( 11 );
 for( const auto k : S ) T[ h( k ) ].push_back( k );
 std::size_t L = 0;
 for( const auto & t : T ) L += t.size() * t.size();
 CHECK( L == 17 && L < 2 * S.size() );
 CHECK( T[ 0 ] == std::vector< std::uint64_t >( { 1 , 79 } ) );
 CHECK( T[ 3 ] == std::vector< std::uint64_t >( { 98 , 19 } ) );
 CHECK( T[ 4 ] == std::vector< std::uint64_t >( { 14 , 3 } ) );
 CHECK( T[ 8 ] == std::vector< std::uint64_t >( { 50 , 72 } ) );
 CHECK( T[ 2 ] == std::vector< std::uint64_t >( { 69 } ) );
 CHECK( h( 98 ) == 3 && h3( 98 ) == 2 && h3( 19 ) == 1 );   // no collision
 CHECK( h4( 14 ) != h4( 3 ) && h8( 50 ) != h8( 72 ) );
 CHECK( h( 8 ) == 3 && h3( 8 ) == 2 );          // finds 98, so 8 is not in S
 const PerfectHashTable F( S );
 for( const auto k : S ) CHECK( F.contains( k ) );
 CHECK( ! F.contains( 8 ) && F.size() == 9 && F.space() < 36 );
 }

// the two-level perfect hashing against std::set on random sets, with the
// extremes of the universe and consecutive keys; its keys by the
// iterators, and fewer than 4 n places in the second level
ADS_TEST( "fks-table" )
{
 std::mt19937_64 g( 5 );
 for( std::size_t n : { 0u , 1u , 2u , 10u , 1000u , 20000u } ) {
  std::vector< std::uint64_t > S;
  for( std::size_t i = 0 ; i < n ; ++i ) S.push_back( g() % kMersenne61 );
  if( n >= 2 ) {
   S[ 0 ] = 0;
   S[ 1 ] = kMersenne61 - 1;
   }
  const std::set< std::uint64_t > R( S.begin() , S.end() );
  const PerfectHashTable T( S , n + 3 );
  CHECK( T.size() == R.size() );
  CHECK( ( n == 0 ) || ( T.space() < 4 * n ) );
  CHECK( T.trials() <= 3 * n + 20 );
  for( const auto k : S ) CHECK( T.contains( k ) );
  for( int i = 0 ; i < 10000 ; ++i ) {
   const std::uint64_t k = g() % kMersenne61;
   CHECK( T.contains( k ) == ( R.count( k ) > 0 ) );
   }
  CHECK( std::set< std::uint64_t >( T.begin() , T.end() ) == R );
  CHECK( ! T.contains( kMersenne61 ) && ! T.contains( ~std::uint64_t( 0 ) ) );
  }
 std::vector< std::uint64_t > C( 5000 );        // consecutive
 for( std::size_t i = 0 ; i < C.size() ; ++i ) C[ i ] = i;
 const PerfectHashTable T( C );
 CHECK( T.size() == 5000 && T.contains( 4999 ) && ! T.contains( 5000 ) );
 CHECK_THROWS( PerfectHashTable( { kMersenne61 } ) , contract_violation );
 CHECK_THROWS( PerfectHashTable( { 1 , 1 } ) , contract_violation );
 }

// Table 8.1 and Figure 8.5: the labelling of the graph of h1 and h2 gives
// the g of the table, so that ( g( h1( t ) ) + g( h2( t ) ) ) mod 9 is the
// rank of t; the labelling by peeling of the same forest also satisfies
// every edge; a cycle whose values add up is labelled, a double edge with
// two values and a triangle (no vertex of degree 1) are not
ADS_TEST( "opmphf-example" )
{
 using F = OrderedMinimalPerfectHash< std::string >;
 // body cat dog flower house mouse sun tree zoo
 const std::vector< std::size_t > E = { 1 , 6 , 7 , 2 , 5 , 7 , 4 , 6 , 1 ,
                                        10 , 0 , 1 , 8 , 11 , 11 , 9 , 5 , 3 };
 std::vector< std::size_t > h( 9 );
 for( std::size_t i = 0 ; i < 9 ; ++i ) h[ i ] = i;
 const auto g = F::labelAcyclicGraph( 12 , E , h , 9 );
 CHECK( g && *g == std::vector< std::uint32_t >( { 0 , 5 , 0 , 7 , 8 , 1 ,
                                                   4 , 1 , 0 , 1 , 8 ,
                                                   6 } ) );
 const auto p = F::labelByPeeling( 12 , E , 2 , h , 9 );
 CHECK( p.has_value() );
 for( std::size_t e = 0 ; e < 9 ; ++e )
  CHECK( ( ( *p )[ E[ 2 * e ] ] + ( *p )[ E[ 2 * e + 1 ] ] ) % 9 == e );
 CHECK( F::labelAcyclicGraph( 4 , { 0 , 1 , 1 , 2 , 2 , 3 , 3 , 0 } ,
                              { 1 , 1 , 1 , 1 } , 5 ).has_value() );
 CHECK( ! F::labelAcyclicGraph( 2 , { 0 , 1 , 1 , 0 } , { 0 , 1 } , 3 ) );
 CHECK( ! F::labelByPeeling( 3 , { 0 , 1 , 1 , 2 , 2 , 0 } , 2 , { 0 , 1 ,
                                                                 2 } , 3 ) );
 CHECK_THROWS( F::labelByPeeling( 3 , { 0 , 0 } , 2 , { 0 } , 1 ) ,
               contract_violation );
 }

// the ordered minimal perfect hash on random sorted words, r = 2 with
// c = 3 (about two graphs, sqrt( 3 ) on average) and r = 3 with c = 1.25,
// on integers, and on 0, 1 and 2 keys; repeated keys break the audit
ADS_TEST( "opmphf" )
{
 std::mt19937 g( 6 );
 const auto W = randomDictionary( g , 3000 , 26 , 12 );
 double trials = 0;
 for( std::uint64_t seed = 1 ; seed <= 30 ; ++seed ) {
  const OrderedMinimalPerfectHash< std::string > H( W , 2 , 3 , seed );
  trials += double( H.trials() );
  if( seed <= 3 )
   for( std::size_t i = 0 ; i < W.size() ; ++i ) CHECK( H( W[ i ] ) == i );
  CHECK( H.vertices() == 3 * W.size() && H.size() == W.size() );
  }
 CHECK( trials / 30 <= 3 );
 const OrderedMinimalPerfectHash< std::string > H3( W , 3 );
 for( std::size_t i = 0 ; i < W.size() ; ++i ) CHECK( H3( W[ i ] ) == i );
 CHECK( H3.vertices() <= std::size_t( 1.25 * double( W.size() ) ) + 3 &&
        H3.trials() <= 5 );
 std::vector< long > K;
 for( long i = 0 ; i < 2000 ; ++i ) K.push_back( i * i * ( i % 2 ? -1 : 1 ) );
 for( int r : { 2 , 3 } ) {
  const OrderedMinimalPerfectHash< long > I( K , r );
  for( std::size_t i = 0 ; i < K.size() ; ++i ) CHECK( I( K[ i ] ) == i );
  }
 for( std::size_t n : { 1u , 2u } )
  for( int r : { 2 , 3 } ) {
   const std::vector< std::string > S( W.begin() ,
                                       W.begin() + std::ptrdiff_t( n ) );
   const OrderedMinimalPerfectHash< std::string > T( S , r );
   for( std::size_t i = 0 ; i < n ; ++i ) CHECK( T( S[ i ] ) == i );
   }
 const OrderedMinimalPerfectHash< std::string > E(
                                            std::vector< std::string >{} );
 CHECK( E.size() == 0 );
 CHECK_THROWS( E( "a" ) , contract_violation );
 CHECK_THROWS( OrderedMinimalPerfectHash< int >( { 1 , 2 } , 2 , 2 ) ,
               contract_violation );
 CHECK_THROWS( OrderedMinimalPerfectHash< int >( { 1 , 2 , 1 } ) ,
               contract_violation );
 }

// the cuckoo hashing against std::map on random operations, with and
// without a stash, the load never past maxLoad, the pairs by the iterators;
// the smallest table (m = 2) grown from empty, a key inserted again, an
// absent key removed, and the bounds of m and maxLoad
ADS_TEST( "cuckoo" )
{
 for( std::size_t s : { 0u , 4u } ) {
  CuckooHashTable< int , int > T( 4 , 0.4 , s , 9 );
  std::map< int , int > R;
  std::mt19937 g( 10 );
  for( int op = 0 ; op < 20000 ; ++op ) {
   const int k = int( g() % ( op < 10000 ? 3000u : 600u ) );
   if( g() % 3 ) {
    T.insert( k , op );
    R[ k ] = op;
    }
   else {
    T.remove( k );
    R.erase( k );
    }
   const int q = int( g() % 3000 );
   CHECK( T.lookup( q ) == ( R.count( q ) ? std::optional< int >( R[ q ] )
                                          : std::nullopt ) );
   CHECK( double( T.size() ) <= 0.4 * double( T.capacity() ) );
   }
  CHECK( T.size() == R.size() && T.stashed() <= s );
  std::map< int , int > I;
  for( const auto [ k , v ] : T ) I[ k ] = v;
  CHECK( I == R );
  for( const auto & [ k , v ] : R ) T.remove( k );
  CHECK( T.size() == 0 && T.capacity() == 4 );
  }
 CuckooHashTable< int , int > S( 2 , 0.49 );
 CHECK( S.size() == 0 && ! S.lookup( 0 ) && S.begin() == S.end() );
 S.remove( 0 );                                  // absent: nothing
 for( int k = 0 ; k < 100 ; ++k ) S.insert( k , k );
 S.insert( 42 , -42 );                           // a key again: replaced
 CHECK( S.size() == 100 && S.lookup( 42 ) == -42 && ! S.lookup( 100 ) );
 CHECK( double( S.size() ) <= 0.49 * double( S.capacity() ) );
 CHECK_THROWS( ( CuckooHashTable< int , int >( 16 , 0.5 ) ) ,
               contract_violation );
 CHECK_THROWS( ( CuckooHashTable< int , int >( 16 , 0 ) ) ,
               contract_violation );
 CHECK_THROWS( ( CuckooHashTable< int , int >( 1 , 0.4 ) ) ,
               contract_violation );
 }

// Corollary 8.3: a table of m >= 6 n ( 1 + epsilon ) places, epsilon = 0.1,
// needs no rehash in at least half of the builds (here in almost all of
// them); near the load 1 / 2 a stash of 4 places saves rehashes
ADS_TEST( "cuckoo-analysis" )
{
 const std::size_t n = 1000 , m = 6600;
 int clean = 0;
 double evictions = 0;
 for( std::uint64_t seed = 1 ; seed <= 100 ; ++seed ) {
  CuckooHashTable< std::uint64_t , int > T( m , 0.45 , 0 , seed );
  for( std::uint64_t i = 0 ; i < n ; ++i ) T.insert( i * 7919 , 0 );
  clean += T.rehashes() == 0;
  evictions += double( T.evictions() ) / double( n );
  CHECK( T.capacity() == m );
  }
 CHECK( clean >= 50 && evictions / 100 < 1 );
 std::size_t plain = 0 , stash = 0;
 for( std::uint64_t seed = 1 ; seed <= 40 ; ++seed ) {
  CuckooHashTable< std::uint64_t , int > A( 2048 , 0.49 , 0 , seed ) ,
                                         B( 2048 , 0.49 , 4 , seed );
  for( std::uint64_t i = 0 ; i < 1000 ; ++i ) {
   A.insert( i , 0 );
   B.insert( i , 0 );
   }
  plain += A.rehashes();
  stash += B.rehashes();
  CHECK( A.capacity() == 2048 && B.capacity() == 2048 );
  }
 CHECK( stash <= plain );
 }

// the analysis of section 8.7: 22 hashes for 32 bits per key, with false
// positives below 1e-6; the factor 1 / ln 2 = 1.44 over the lower bound;
// the compressed filters of Table 8.3; the formula against the false
// positives of BloomFilter; one bit (m = 1) and m / n past an int (the
// best number of hashes cut to an int), no keys, an empty filter, and
// every precondition
ADS_TEST( "bloom-analysis" )
{
 CHECK( bloomBestHashes( 32000 , 1000 ) == 22 );
 CHECK( bloomFalsePositive( 32000 , 1000 , 22 ) < 1e-6 );
 for( int r = 1 ; r <= 40 ; ++r )
  CHECK( bloomFalsePositive( 32000 , 1000 , r ) >=
         bloomFalsePositive( 32000 , 1000 , 22 ) );
 for( double mn : { 1.0 , 2.0 , 5.0 , 10.0 } ) {     // ( 0.6185 )^( m / n )
  const double r = mn * std::log( 2.0 );
  CHECK( std::abs( std::pow( 1 - std::exp( -r / mn ) , r ) -
                   std::pow( 0.6185 , mn ) ) < 1e-3 );
  }
 CHECK( std::abs( bloomBits( 1000 , 0.01 ) / bloomLowerBound( 1000 , 0.01 ) -
                  1.4427 ) < 1e-4 );
 CHECK( std::abs( bloomLowerBound( 1 , 0.25 ) - 2 ) < 1e-12 );
 const auto [ f16 , z16 ] = compressedBloom( 16 , 11 );
 const auto [ f28 , z28 ] = compressedBloom( 28 , 4 );
 const auto [ f48 , z48 ] = compressedBloom( 48 , 3 );
 CHECK( std::abs( f16 - 0.000459 ) < 5e-7 && std::abs( z16 - 16 ) < 1e-2 );
 CHECK( std::abs( f28 - 0.000314 ) < 5e-7 && std::abs( z28 - 15.846 ) < 1e-3 );
 CHECK( std::abs( f48 - 0.000222 ) < 5e-7 && std::abs( z48 - 15.829 ) < 1e-3 );
 CHECK( std::abs( bloomFalsePositive( 1e6 , 1e5 , 7 ) -
                  std::pow( 1 - std::exp( -0.7 ) , 7 ) ) < 1e-6 );
 CHECK( bloomFalsePositive( 100 , 0 , 3 ) == 0 &&
        bloomFalsePositive( 1 , 5 , 3 ) == 1 );
 BloomFilter< int > B( 80000 , 6 );
 for( int i = 0 ; i < 10000 ; ++i ) B.insert( 2 * i );
 int fp = 0;
 for( int i = 0 ; i < 200000 ; ++i ) fp += B.maybe( 2 * i + 1 );
 const double rate = fp / 200000.0 , exact = bloomFalsePositive( 80000 ,
                                                               10000 , 6 );
 CHECK( std::abs( rate - exact ) < 0.2 * exact );
 CHECK( bloomBestHashes( 1 , 1000 ) == 1 && bloomBestHashes( 1 , 1 ) == 1 );
 const int big = bloomBestHashes( 1e12 , 1 );   // the floor past an int
 CHECK( big >= std::numeric_limits< int >::max() - 1 );
 CHECK( bloomLowerBound( 0 , 0.5 ) == 0 && bloomBits( 0 , 0.5 ) == 0 );
 const BloomFilter< int > Z( 64 , 3 );           // empty: all negative
 for( int i = 0 ; i < 100 ; ++i ) CHECK( ! Z.maybe( i ) );
 BloomFilter< int > One( 1 , 1 );                // one bit: all positive
 CHECK( ! One.maybe( 5 ) );
 One.insert( 0 );
 for( int i = 0 ; i < 100 ; ++i ) CHECK( One.maybe( i ) );
 CHECK_THROWS( bloomLowerBound( 10 , 1 ) , contract_violation );
 CHECK_THROWS( bloomLowerBound( 10 , 0 ) , contract_violation );
 CHECK_THROWS( bloomLowerBound( -1 , 0.5 ) , contract_violation );
 CHECK_THROWS( bloomBits( 10 , 0 ) , contract_violation );
 CHECK_THROWS( bloomFalsePositive( 0 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( bloomFalsePositive( 10 , -1 , 1 ) , contract_violation );
 CHECK_THROWS( bloomFalsePositive( 10 , 1 , 0 ) , contract_violation );
 CHECK_THROWS( bloomBestHashes( 10 , 0 ) , contract_violation );
 CHECK_THROWS( bloomBestHashes( 0 , 10 ) , contract_violation );
 CHECK_THROWS( compressedBloom( 0 , 1 ) , contract_violation );
 CHECK_THROWS( compressedBloom( 8 , 0 ) , contract_violation );
 CHECK_THROWS( ( BloomFilter< int >( 0 , 1 ) ) , contract_violation );
 CHECK_THROWS( ( BloomFilter< int >( 8 , 0 ) ) , contract_violation );
 CHECK_THROWS( BloomFilter< int >::forItems( 0 , 0.1 ) , contract_violation );
 CHECK_THROWS( BloomFilter< int >::forItems( 10 , 1 ) , contract_violation );
 }

// the spectral filter on a multiset: an estimate never below the
// multiplicity, above it about as often as a false positive, also after
// removals; the counting filter of counters of 8 bits, whose counters
// stick at 255; the partitioned filter, r distinct bits per key and the
// false positives of the formula; parts of one bit (m = r), an empty
// spectral filter, zero copies, and the bounds of m, r and remove()
ADS_TEST( "spectral-bloom" )
{
 std::mt19937 g( 11 );
 const std::size_t n = 5000 , m = 50000;
 const int r = 7;
 SpectralBloomFilter< int > S( m , r );
 std::map< int , unsigned > f;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const int x = int( g() % 1000000 );
  const unsigned c = 1 + g() % 20;
  S.insert( x , c );
  f[ x ] += c;
  }
 auto wrong = [ & ]( void ) {
  std::size_t w = 0;
  for( const auto & [ x , c ] : f ) {
   CHECK( S.count( x ) >= c );
   w += S.count( x ) > c;
   }
  return( double( w ) / double( f.size() ) );
  };
 const double eps = bloomFalsePositive( double( m ) , double( f.size() ) , r );
 CHECK( wrong() <= 3 * eps + 0.005 );
 for( auto & [ x , c ] : f ) {                  // half of each one goes
  S.remove( x , c / 2 );
  c -= c / 2;
  }
 CHECK( wrong() <= 3 * eps + 0.005 );
 CHECK( S.counters() == m && S.hashes() == r && S.stuck() == 0 );
 const int x = f.begin()->first;
 CHECK_THROWS( S.remove( x , S.count( x ) + 1 ) , contract_violation );

 SpectralBloomFilter< int , std::uint8_t > C( 1000 , 4 );
 for( int i = 0 ; i < 300 ; ++i ) C.insert( 42 );
 CHECK( C.count( 42 ) == 255 && C.stuck() >= 1 && C.stuck() <= 4 );
 C.remove( 42 , 255 );
 CHECK( C.maybe( 42 ) );                        // stuck, no false negative
 C.insert( 7 );
 C.remove( 7 );
 CHECK( C.count( 7 ) == 0 || C.count( 7 ) == 255 );

 PartitionedBloomFilter< int > P( 70000 , 7 );
 P.insert( 1 );
 CHECK( P.ones() == 7 && P.bits() == 70000 );
 for( int i = 2 ; i <= 10000 ; ++i ) P.insert( 3 * i );
 for( int i = 2 ; i <= 10000 ; ++i ) CHECK( P.maybe( 3 * i ) );
 int fp = 0;
 for( int i = 0 ; i < 200000 ; ++i ) fp += P.maybe( 3 * i + 1 );
 const double rate = fp / 200000.0 ,
              q = std::pow( 1 - std::exp( -7.0 * 10000 / 70000 ) , 7 );
 CHECK( std::abs( rate - q ) < 0.25 * q );
 PartitionedBloomFilter< int > Q( 4 , 4 );      // m = r: parts of one bit
 CHECK( ! Q.maybe( 9 ) );
 Q.insert( 9 );
 CHECK( Q.ones() == 4 && Q.maybe( 0 ) && Q.maybe( 12345 ) );
 const SpectralBloomFilter< int > E( 10 , 2 );  // empty: all zero
 CHECK( E.count( 1 ) == 0 && ! E.maybe( 1 ) && E.stuck() == 0 );
 SpectralBloomFilter< int > S0( 10 , 2 );
 S0.insert( 3 , 0 );                             // zero copies: nothing
 S0.remove( 4 , 0 );
 CHECK( S0.count( 3 ) == 0 );
 CHECK_THROWS( S0.remove( 3 , 1 ) , contract_violation );
 CHECK_THROWS( ( SpectralBloomFilter< int >( 0 , 1 ) ) , contract_violation );
 CHECK_THROWS( ( SpectralBloomFilter< int >( 10 , 0 ) ) ,
               contract_violation );
 CHECK_THROWS( ( PartitionedBloomFilter< int >( 3 , 4 ) ) ,
               contract_violation );
 CHECK_THROWS( ( PartitionedBloomFilter< int >( 3 , 0 ) ) ,
               contract_violation );
 }

// the intersection of section 8.7.4: exact, and with about
// |A cap B| + 0.6185^( mA / |A| ) |B| keys sent back; B empty, B = A and
// two disjoint sets with a filter of one bit, A empty or mA = 0
ADS_TEST( "bloom-intersection" )
{
 std::vector< int > A , B;
 for( int i = 0 ; i < 4000 ; ++i ) A.push_back( 3 * i );
 for( int i = 0 ; i < 6000 ; ++i ) B.push_back( 2 * i + 1 );
 std::vector< int > I0;
 for( const int b : B ) if( b % 3 == 0 && b < 12000 ) I0.push_back( b );
 const auto [ Q , I ] = bloomIntersection( A , B , 8 * A.size() );
 CHECK( I == I0 );
 CHECK( std::includes( Q.begin() , Q.end() , I.begin() , I.end() ) );
 const double extra = double( Q.size() - I.size() ) ,
              eps = bloomFalsePositive( 8.0 * 4000 , 4000 ,
                                        bloomBestHashes( 32000 , 4000 ) );
 CHECK( extra <= 2 * eps * double( B.size() ) + 10 );
 CHECK( std::pow( 0.6185 , 8 ) > eps * 0.8 );
 const auto [ Q0 , I1 ] = bloomIntersection( A , std::vector< int >() , 64 );
 CHECK( Q0.empty() && I1.empty() );              // B empty
 const auto [ QA , IA ] = bloomIntersection( A , A , 1 );   // A = B, 1 bit
 CHECK( QA == A && IA == A );
 const std::vector< int > O = { 1 } , N = { 2 };            // disjoint
 const auto [ QN , IN ] = bloomIntersection( O , N , 1 );
 CHECK( QN == N && IN.empty() );                 // one bit: all positive
 CHECK_THROWS( bloomIntersection( std::vector< int >() , B , 8 ) ,
               contract_violation );
 CHECK_THROWS( bloomIntersection( A , B , 0 ) , contract_violation );
 }

// the array of string pointers of Figure 9.1: the range of "te" and the
// lexicographic positions; on random dictionaries, every pattern against
// the brute force, also by the binned search on strings
ADS_TEST( "prefix-search" )
{
 const std::vector< std::string > A = { "i" , "in" , "inn" , "tea" , "ted" ,
                                        "ten" , "to" };
 CHECK( prefixRange( A , "te" ) == Range( 3 , 6 ) );
 CHECK( prefixRange( A , "i" ) == Range( 0 , 3 ) );
 CHECK( prefixRange( A , "" ) == Range( 0 , 7 ) );
 CHECK( prefixRange( A , "z" ) == Range( 7 , 7 ) );
 CHECK( lexicographicPosition( A , "tea" ) == 3 &&
        lexicographicPosition( A , "teb" ) == 4 );
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const auto D = randomDictionary( g , 1 + t * 3 , 3 , 5 );
  struct Array {
   const std::vector< std::string > & D;
   std::size_t lexicographicPosition( const std::string & P ) const {
    return( ADS::lexicographicPosition( D , P ) );
    }
   std::size_t prefixEnd( const std::string & P ) const {
    return( ADS::prefixEnd( D , P ) );
    }
   Range prefixRange( const std::string & P ) const {
    return( ADS::prefixRange( D , P ) );
    }
   };
  againstBrute( Array{ D } , D , 3 , 4 );
  againstBrute( BinnedStringSearch( D ) , D , 3 , 4 );
  }
 std::vector< std::string > L;                  // keys beyond 8 characters
 for( int i = 0 ; i < 50 ; ++i )
  L.push_back( "commonprefix" + std::string( 1 , char( 'a' + i % 7 ) ) +
               std::to_string( i ) );
 std::sort( L.begin() , L.end() );
 const BinnedStringSearch BL( L );
 for( const std::string P : { "common" , "commonprefixb" , "commonprefixb1" ,
                              "commonprefixz" , "d" , "" } )
  CHECK( BL.prefixRange( P ) == bruteRange( L , P ) );
 CHECK( BinnedStringSearch( {} ).prefixRange( "a" ) == Range( 0 , 0 ) );
 }

// Figure 9.3: twelve integers in [ 1 , 36 ], bins of width 3, B4 and B8
// empty; the positions against std::lower_bound; on uniform and on skewed
// integers, a bin holds at most ceil( Delta ) items (Theorem 9.4); no
// item, a single item, repeated items, the items 0 and 2^64 - 1, and the
// bounds of bin()
ADS_TEST( "binned-search" )
{
 const BinnedSearch F( { 1 , 2 , 3 , 8 , 9 , 17 , 19 , 20 , 28 , 30 , 32 ,
                         36 } );
 CHECK( F.width() == 3 && F.bins() == 12 );
 const std::vector< std::size_t > count = { 3 , 0 , 2 , 0 , 0 , 1 , 2 , 0 , 0 ,
                                            2 , 1 , 1 };
 for( std::size_t j = 0 ; j < 12 ; ++j )
  CHECK( F.bin( j ).second - F.bin( j ).first == count[ j ] );
 for( std::uint64_t y = 0 ; y <= 40 ; ++y ) {
  const auto & X = F.items();
  CHECK( F.position( y ) == std::size_t( std::lower_bound( X.begin() ,
                                             X.end() , y ) - X.begin() ) );
  CHECK( F.contains( y ) == std::binary_search( X.begin() , X.end() , y ) );
  }
 CHECK( F.binOf( 0 ) == std::nullopt && F.binOf( 37 ) == std::nullopt &&
        F.binOf( 10 ) == 3u );
 std::mt19937_64 g( 13 );
 for( int kind = 0 ; kind < 2 ; ++kind ) {
  std::set< std::uint64_t > S;
  std::uint64_t x = 0;
  while( S.size() < 20000 ) {
   if( kind == 0 ) S.insert( g() );
   else S.insert( x += 1 + ( g() % 64 ? g() % 4 : g() % 100000 ) );
   }
  const std::vector< std::uint64_t > X( S.begin() , S.end() );
  const BinnedSearch B( X );
  std::uint64_t lo = ~std::uint64_t( 0 ) , hi = 0;
  for( std::size_t i = 1 ; i < X.size() ; ++i ) {
   lo = std::min( lo , X[ i ] - X[ i - 1 ] );
   hi = std::max( hi , X[ i ] - X[ i - 1 ] );
   }
  CHECK( double( B.largestBin() ) <=
         std::ceil( double( hi ) / double( lo ) ) );
  if( kind == 0 ) CHECK( B.largestBin() <= 12 );
  for( int i = 0 ; i < 20000 ; ++i ) {
   const std::uint64_t y = i % 2 ? X[ g() % X.size() ] : g();
   CHECK( B.position( y ) == std::size_t( std::lower_bound( X.begin() ,
                                             X.end() , y ) - X.begin() ) );
   }
  }
 const BinnedSearch E( {} ) , O( { 5 } ) , R( { 4 , 4 , 4 , 9 } );
 CHECK( E.position( 3 ) == 0 && E.bins() == 0 );
 CHECK( O.position( 5 ) == 0 && O.position( 6 ) == 1 && O.contains( 5 ) );
 CHECK( R.position( 4 ) == 0 && R.position( 5 ) == 3 &&
        R.position( 10 ) == 4 );
 const std::uint64_t top = ~std::uint64_t( 0 );  // the extremes of the type
 const BinnedSearch X( { 0 , 1 , top - 1 , top } );
 CHECK( X.position( 0 ) == 0 && X.position( 2 ) == 2 &&
        X.position( top ) == 3 && X.contains( top ) && ! X.contains( 2 ) );
 const BinnedSearch A( { top , top } );
 CHECK( A.position( top ) == 0 && A.position( 0 ) == 0 && A.contains( top ) );
 CHECK_THROWS( F.bin( F.bins() ) , contract_violation );
 CHECK_THROWS( BinnedSearch( { 3 , 1 } ) , contract_violation );
 }

// the front coding of the dictionary of section 9.1.2 and its pairs; the
// byte-aligned code of the lengths; the bucketing of Figure 9.2, two
// strings per bucket, and the searches of the buckets against the brute
// force on random dictionaries, with the extremes; a code of a length cut
// short, the dictionary of the empty string alone, and the bounds of
// access(), decodeCost(), anchor(), anchorString() and rankInBucket()
ADS_TEST( "front-coding" )
{
 const std::vector< std::string > D = { "alcatraz" , "alcool" , "alcyone" ,
                                        "anacleto" , "ananas" , "aster" ,
                                        "astral" , "astronomy" };
 const std::vector< FrontPair > F0 = { { 0 , "alcatraz" } , { 3 , "ool" } ,
                                       { 3 , "yone" } , { 1 , "nacleto" } ,
                                       { 3 , "nas" } , { 1 , "ster" } ,
                                       { 3 , "ral" } , { 4 , "onomy" } };
 CHECK( frontCode( D ) == F0 && frontDecode( F0 ) == D );
 CHECK_THROWS( frontDecode( { { 1 , "a" } } ) , contract_violation );
 for( const std::uint32_t l : { 0u , 63u , 64u , 16383u , 16384u , 4194303u ,
                                4194304u , ( 1u << 30 ) - 1 } ) {
  std::string c;
  encodeLength( l , c );
  std::size_t pos = 0;
  CHECK( decodeLength( c , pos ) == l && pos == c.size() );
  CHECK( c.size() == ( l < 64 ? 1u : l < 16384 ? 2u : l < 4194304 ? 3u
                                                                  : 4u ) );
  }
 std::string c0;
 CHECK_THROWS( encodeLength( 1u << 30 , c0 ) , contract_violation );
 const auto B = FrontCodedDictionary::bucketed( D , 2 );
 CHECK( B.anchors() == 4 && B.anchorString( 1 ) == "alcyone" &&
        B.anchorString( 2 ) == "ananas" && B.anchorString( 3 ) == "astral" );
 CHECK( B.characters() == 46 );        // 8 + 3 + 7 + 7 + 6 + 4 + 6 + 5
 CHECK( FrontCodedDictionary::bucketed( D , 8 ).characters() == 37 );
 CHECK( std::equal( B.begin() , B.end() , D.begin() , D.end() ) );
 for( std::size_t i = 0 ; i < D.size() ; ++i )
  CHECK( B.access( i ) == D[ i ] );
 CHECK( B.prefixRange( "ast" ) == Range( 5 , 8 ) );
 std::mt19937 g( 14 );
 for( std::size_t b : { 1u , 2u , 3u , 8u , 100u } )
  for( int t = 0 ; t < 6 ; ++t ) {
   const auto R = randomDictionary( g , 1 + 15 * t , 3 , 6 );
   const auto T = FrontCodedDictionary::bucketed( R , b );
   CHECK( T.size() == R.size() && T.anchors() == ( R.size() + b - 1 ) / b );
   CHECK( std::equal( T.begin() , T.end() , R.begin() , R.end() ) );
   againstBrute( T , R , 3 , 4 );
   }
 const auto E = FrontCodedDictionary::bucketed( {} , 4 );
 CHECK( E.size() == 0 && E.prefixRange( "a" ) == Range( 0 , 0 ) );
 const std::vector< std::string > W = { "" , "a" , "a" , "ab" };
 const auto T = FrontCodedDictionary::bucketed( W , 3 );
 CHECK( std::equal( T.begin() , T.end() , W.begin() , W.end() ) &&
        T.prefixRange( "a" ) == Range( 1 , 4 ) );
 std::size_t at = 0;                             // a code cut short
 CHECK_THROWS( decodeLength( "" , at ) , contract_violation );
 CHECK_THROWS( decodeLength( std::string( 1 , '\x40' ) , at ) ,
               contract_violation );
 at = 1;
 CHECK_THROWS( decodeLength( std::string( 1 , '\x00' ) , at ) ,
               contract_violation );
 const auto S = FrontCodedDictionary::bucketed( { "" } , 1 );
 CHECK( S.size() == 1 && S.access( 0 ).empty() &&
        S.prefixRange( "" ) == Range( 0 , 1 ) &&
        S.prefixRange( "a" ) == Range( 1 , 1 ) );
 CHECK_THROWS( B.access( D.size() ) , contract_violation );
 CHECK_THROWS( B.decodeCost( D.size() ) , contract_violation );
 CHECK_THROWS( B.anchorString( B.anchors() ) , contract_violation );
 CHECK_THROWS( B.anchor( B.anchors() ) , contract_violation );
 CHECK_THROWS( B.rankInBucket( B.anchors() + 1 , "a" , false ) ,
               contract_violation );
 CHECK_THROWS( E.access( 0 ) , contract_violation );
 CHECK_THROWS( FrontCodedDictionary::bucketed( D , 0 ) , contract_violation );
 }

// the locality-preserving front coding: at most ( 1 + 2 / ( c - 2 ) )
// FC( D ) characters and at most ( c + 1 ) | s_i | characters read to
// decode s_i (Theorem 9.5), where the plain coding may read the whole
// dictionary; its searches against the brute force; c <= 2 (no bound on
// the space, the strings still right), the empty dictionary, and c <= 0
ADS_TEST( "lpfc" )
{
 std::mt19937 g( 15 );
 std::vector< std::string > U;                  // shared prefixes, short ends
 for( int i = 0 ; i < 3000 ; ++i ) {
  std::string s = "http://www.site" + std::to_string( g() % 4 ) + ".org/";
  for( int d = 0 ; d < 1 + int( g() % 5 ) ; ++d )
   s += "dir" + std::to_string( g() % 3 ) + "/";
  U.push_back( s );
  }
 std::sort( U.begin() , U.end() );
 U.erase( std::unique( U.begin() , U.end() ) , U.end() );
 std::vector< std::string > Z;                  // a , aa , aaa , ...
 for( int i = 1 ; i <= 300 ; ++i ) Z.push_back( std::string( i , 'a' ) );
 for( const auto * Dp : { &U , &Z } ) {
  const auto & D = *Dp;
  double fc = 0;
  for( const auto & p : frontCode( D ) ) fc += double( p.second.size() );
  const auto P = FrontCodedDictionary::bucketed( D , D.size() );
  CHECK( double( P.characters() ) == fc );
  for( double c : { 2.5 , 3.0 , 4.0 , 8.0 } ) {
   const auto L = FrontCodedDictionary::localityPreserving( D , c );
   CHECK( double( L.characters() ) <= ( 1 + 2 / ( c - 2 ) ) * fc );
   for( std::size_t i = 0 ; i < D.size() ; ++i ) {
    CHECK( double( L.decodeCost( i ) ) <=
           ( c + 1 ) * double( D[ i ].size() ) );
    if( i % 97 == 0 ) CHECK( L.access( i ) == D[ i ] );
    }
   CHECK( std::equal( L.begin() , L.end() , D.begin() , D.end() ) );
   }
  }
 const std::size_t last = U.size() - 1;
 CHECK( FrontCodedDictionary::bucketed( U , U.size() ).decodeCost( last ) >
        20 * U[ last ].size() );
 for( int t = 0 ; t < 10 ; ++t ) {
  const auto R = randomDictionary( g , 1 + 10 * t , 3 , 7 );
  againstBrute( FrontCodedDictionary::localityPreserving( R , 3 ) , R , 3 ,
                4 );
  }
 for( double c : { 0.5 , 1.0 , 2.0 } ) {        // c <= 2: no bound, but right
  const auto L = FrontCodedDictionary::localityPreserving( Z , c );
  CHECK( std::equal( L.begin() , L.end() , Z.begin() , Z.end() ) );
  CHECK( L.access( Z.size() - 1 ) == Z.back() );
  }
 const auto E = FrontCodedDictionary::localityPreserving( {} , 3 );
 CHECK( E.size() == 0 && E.anchors() == 0 && E.prefixEnd( "" ) == 0 );
 CHECK_THROWS( FrontCodedDictionary::localityPreserving( U , 0 ) ,
               contract_violation );
 CHECK_THROWS( FrontCodedDictionary::localityPreserving( U , -1 ) ,
               contract_violation );
 }

// Figures 9.7 and 9.8: the Patricia trie of seven strings, its 13 nodes
// with their lengths (a leaf one more than its string, for the end of the
// string) and branching characters; the blind search of bcbabcba reads
// bcbcbbba and puts the pattern after the three strings of a; on random
// dictionaries, with strings prefixes of others, every pattern against the
// brute force; at most 2 n nodes; the empty trie, the empty string alone,
// one string, a chain of prefixes, and the bounds of blindLeaf(), string()
ADS_TEST( "patricia" )
{
 const std::vector< std::string > D = { "abaaba" , "abaabb" , "abac" ,
                                        "bcbcaba" , "bcbcabb" , "bcbcbba" ,
                                        "bcbcbbba" };
 const PatriciaTrie T( D );
 CHECK( T.nodes() == 13 );
 const std::vector< std::pair< std::size_t , int > > shape = {
  { 0 , -1 } , { 3 , 'a' } , { 5 , 'a' } , { 7 , 'a' } , { 7 , 'b' } ,
  { 5 , 'c' } , { 4 , 'b' } , { 6 , 'a' } , { 8 , 'a' } , { 8 , 'b' } ,
  { 6 , 'b' } , { 8 , 'a' } , { 9 , 'b' } };
 CHECK( T.shape() == shape );
 CHECK( T.blindLeaf( "bcbabcba" ) == 6 );
 CHECK( T.lexicographicPosition( "bcbabcba" ) == 3 );
 CHECK( T.prefixRange( "bcbc" ) == Range( 3 , 7 ) );
 CHECK( T.contains( "abac" ) && ! T.contains( "aba" ) );
 CHECK( std::equal( T.begin() , T.end() , D.begin() , D.end() ) );
 std::mt19937 g( 16 );
 for( int sigma : { 2 , 3 } )
  for( int t = 0 ; t < 25 ; ++t ) {
   const auto R = randomDictionary( g , 1 + 4 * t , sigma , 6 );
   const PatriciaTrie P( R );
   CHECK( P.nodes() <= 2 * R.size() + 1 );
   againstBrute( P , R , sigma , 5 );
   }
 const PatriciaTrie E( {} ) , O( { "" } ) , A( { "abc" } );
 CHECK( E.prefixRange( "x" ) == Range( 0 , 0 ) );
 CHECK( O.lexicographicPosition( "" ) == 0 && O.prefixEnd( "" ) == 1 &&
        O.lexicographicPosition( "a" ) == 1 );
 CHECK( A.prefixRange( "ab" ) == Range( 0 , 1 ) &&
        A.lexicographicPosition( "abd" ) == 1 );
 CHECK( ! E.contains( "" ) && O.contains( "" ) && ! O.contains( "a" ) );
 CHECK( A.blindLeaf( "zzz" ) == 0 && A.string( 0 ) == "abc" );
 const PatriciaTrie C( { "a" , "aa" , "aaa" , "aaaa" } );   // a chain
 CHECK( C.prefixRange( "aa" ) == Range( 1 , 4 ) && C.contains( "aaaa" ) &&
        C.lexicographicPosition( "aaaaa" ) == 4 && C.nodes() <= 9 );
 CHECK_THROWS( E.blindLeaf( "a" ) , contract_violation );
 CHECK_THROWS( A.string( 1 ) , contract_violation );
 CHECK_THROWS( PatriciaTrie( { "b" , "a" } ) , contract_violation );
 CHECK_THROWS( PatriciaTrie( { "a" , "a" } ) , contract_violation );
 }

// the two-level dictionary, a Patricia trie on the anchors of the bucketed
// and of the locality-preserving coding, against the brute force; the
// empty dictionary, every string an anchor and a single bucket, with the
// empty string, and the bounds of access()
ADS_TEST( "two-level" )
{
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 12 ; ++t ) {
  const auto R = randomDictionary( g , 1 + 12 * t , 3 , 6 );
  for( std::size_t b : { 1u , 3u , 16u } ) {
   const TwoLevelDictionary T( FrontCodedDictionary::bucketed( R , b ) );
   CHECK( T.upper().size() == T.lower().anchors() && T.size() == R.size() );
   againstBrute( T , R , 3 , 4 );
   }
  const TwoLevelDictionary L( FrontCodedDictionary::localityPreserving( R ,
                                                                        3 ) );
  againstBrute( L , R , 3 , 4 );
  CHECK( std::equal( L.begin() , L.end() , R.begin() , R.end() ) );
  for( std::size_t i = 0 ; i < R.size() ; ++i )
   CHECK( L.access( i ) == R[ i ] );
  }
 const TwoLevelDictionary E( FrontCodedDictionary::bucketed( {} , 2 ) );
 CHECK( E.size() == 0 && E.prefixEnd( "a" ) == 0 );
 CHECK( E.prefixRange( "" ) == Range( 0 , 0 ) );
 const std::vector< std::string > W = { "" , "a" , "ab" , "b" };
 const TwoLevelDictionary O( FrontCodedDictionary::bucketed( W , 1 ) ) ,
                          M( FrontCodedDictionary::bucketed( W , 99 ) );
 againstBrute( O , W , 2 , 3 );                  // all anchors, one bucket
 againstBrute( M , W , 2 , 3 );
 CHECK_THROWS( O.access( 4 ) , contract_violation );
 CHECK_THROWS( E.access( 0 ) , contract_violation );
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
/*----------------------- End File pearls_hashing.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
