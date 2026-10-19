/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_strings.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered algorithms on strings, beyond the book: the
 * exact search with its filter and its Two-Way fallback, against the naive
 * search on random texts and on the adversarial ones; the suffix array by
 * the engineered SA-IS and the LCP array by the permuted LCP, against the
 * constructions of the textbook; the bit-parallel edit distance, LCS and
 * approximate search against the dynamic programming of the textbook; all
 * at the level 3 of the contracts.
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

#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/EditDistance.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/StringMatching.h"
#include "ADS/string/StringSearch.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixSorting.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// a random string of n bytes over the first sigma bytes from base
std::string randomString( std::mt19937_64 & g , std::size_t n ,
                          unsigned sigma , unsigned base = 'a' )
{
 std::string s( n , ' ' );
 for( auto & c : s ) c = char( base + g() % sigma );
 return( s );
 }

/// the Fibonacci word of at least n bytes over { a , b }, cut to n
std::string fibonacciWord( std::size_t n )
{
 std::string a = "a" , b = "ab";
 while( b.size() < n ) {
  std::string c = b + a;
  a = std::move( b );
  b = std::move( c );
  }
 b.resize( n );
 return( b );
 }

/// b from a by e random edits (substitutions, insertions, deletions)
std::string mutate( std::mt19937_64 & g , std::string a , std::size_t e ,
                    unsigned sigma )
{
 for( std::size_t k = 0 ; k < e ; ++k ) {
  const std::size_t i = a.empty() ? 0 : g() % ( a.size() + 1 );
  const char c = char( 'a' + g() % sigma );
  switch( a.empty() ? 1 : g() % 3 ) {
   case( 0 ): if( i < a.size() ) a[ i ] = c; break;
   case( 1 ): a.insert( a.begin() + std::ptrdiff_t( i ) , c ); break;
   default: if( i < a.size() ) a.erase( a.begin() + std::ptrdiff_t( i ) );
   }
  }
 return( a );
 }

/// all the checks of a search of P in T against the naive search
void checkSearch( std::string_view T , std::string_view P )
{
 const auto N = naiveMatching( T , P );
 const StringSearcher S( P );
 CHECK( S.findAll( T ) == N );            // an audit too
 CHECK( S.count( T ) == N.size() );
 CHECK( S.find( T ) == ( N.empty() ? StringSearcher::npos : N[ 0 ] ) );
 CHECK( findAll( T , P ) == N );
 if( N.size() > 1 ) {                    // from the place after the first
  CHECK( S.find( T , N[ 0 ] + 1 ) == N[ 1 ] );
  CHECK( findFirst( T , P , N[ 0 ] + 1 ) == N[ 1 ] );
  }
 CHECK( S.find( T , T.size() + 1 ) == StringSearcher::npos );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the exact search against naiveMatching(): the empty pattern and text, a
// pattern longer than the text, of one, two and three bytes, at the two
// ends of the text, overlapping occurrences, the bytes 0 and 255, texts
// shorter and longer than the vectors of the filter, random texts over 2,
// 4, 26 and 256 bytes with patterns taken from them or not, the Fibonacci
// word, and the adversarial texts: a^n with a^{m-1} b and b a^{m-1}, and a
// periodic text that passes every probe of its pattern and fails in the
// middle, which sends the search to the Two-Way algorithm; the interface
// of std::search(); patterns of 64 to 363 bytes, searched by the shifts of
// their q-grams, found, repeated and off by one byte
ADS_TEST( "string-search" )
{
 checkSearch( "" , "" );
 checkSearch( "abc" , "" );
 checkSearch( "" , "a" );
 checkSearch( "ab" , "abc" );
 checkSearch( "abracadabra" , "a" );
 checkSearch( "abracadabra" , "abra" );
 checkSearch( "abracadabra" , "ra" );
 checkSearch( "abracadabra" , "cad" );
 checkSearch( "abracadabra" , "abracadabra" );
 checkSearch( "aaaaaaaaaa" , "aaa" );
 checkSearch( std::string( 200 , 'a' ) , std::string( 7 , 'a' ) );
 std::string z( 300 , '\0' );
 z[ 150 ] = char( 255 );
 checkSearch( z , std::string( "\0\xff\0" , 3 ) );
 checkSearch( z , std::string( 40 , '\0' ) );
 std::mt19937_64 g( 7 );
 for( unsigned sigma : { 2u , 4u , 26u , 256u } )
  for( int r = 0 ; r < 300 ; ++r ) {
   const std::size_t n = g() % 600 , m = 1 + g() % 40;
   const std::string T = randomString( g , n , sigma ,
                                       sigma == 256 ? 0 : 'a' );
   std::string P = randomString( g , m , sigma , sigma == 256 ? 0 : 'a' );
   if( ( n >= m ) && ( g() % 2 ) )
    P = T.substr( g() % ( n - m + 1 ) , m );
   checkSearch( T , P );
   checkSearch( T , T.substr( n / 2 ) );    // a suffix
   checkSearch( T , T.substr( 0 , n / 3 ) ); // a prefix
   }
 for( unsigned sigma : { 2u , 4u , 26u , 256u } )   // the q-gram shifts
  for( int r = 0 ; r < 60 ; ++r ) {
   const std::size_t n = 64 + g() % 4000 , m = 64 + g() % 300;
   const std::string T = randomString( g , n , sigma ,
                                       sigma == 256 ? 0 : 'a' );
   if( m > n ) continue;
   const std::string P = T.substr( g() % ( n - m + 1 ) , m );
   checkSearch( T , P );
   checkSearch( T + P + T , P );
   std::string Q = P;
   Q[ g() % m ] ^= 1;                    // one byte off
   checkSearch( T + P + T , Q );
   }
 const std::string F = fibonacciWord( 3000 );
 for( std::size_t m : { 2 , 5 , 13 , 34 , 100 , 377 } )
  checkSearch( F , F.substr( 500 , m ) );
 for( std::size_t m : { 2 , 3 , 9 , 64 , 65 , 200 } ) {
  const std::string A( 5000 , 'a' );
  checkSearch( A , std::string( m - 1 , 'a' ) + "b" );
  checkSearch( A , "b" + std::string( m - 1 , 'a' ) );
  checkSearch( A + "b" , std::string( m - 1 , 'a' ) + "b" );
  }
 std::string ab;                          // (ab)^n
 for( int i = 0 ; i < 20000 ; ++i ) ab += "ab";
 for( std::size_t k : { 4 , 30 , 200 } ) {
  std::string half;
  for( std::size_t i = 0 ; i < k ; ++i ) half += "ab";
  checkSearch( ab , half + "ba" + half );
  checkSearch( ab , half + "a" + half );
  checkSearch( ab + "a" + half , half + "a" + half );
  }
 const std::string T = "the quick brown fox jumps over the lazy dog";
 const StringSearcher S( "lazy" );
 const auto [ a , b ] = S( T.begin() , T.end() );
 CHECK( std::string( a , b ) == "lazy" && a - T.begin() == 35 );
 CHECK( std::search( T.begin() , T.end() , S ) == a );
 const StringSearcher U( "cat" );
 CHECK( std::search( T.begin() , T.end() , U ) == T.end() );
 const StringSearcher E;
 CHECK( E.find( T ) == 0 && E.count( T ) == T.size() + 1 );
 CHECK( E.pattern().empty() && S.pattern() == "lazy" );
 }

// the engineered SA-IS on 32 and 64 bits against suffixArraySais() of the
// textbook (and the audit of the suffix array in both): the empty text, one
// and two bytes, one repeated byte, the bytes 0 and 255, random texts over
// 1, 2, 4, 26 and 256 bytes, the Fibonacci word, a periodic text whose
// reduced strings recurse several times, a text with all its LMS-substrings
// distinct; integer texts over large alphabets; the LCP array of the
// permuted LCP against lcpArray() of Kasai et al., on the same texts
ADS_TEST( "suffix-sorting" )
{
 auto check = [ & ]( const std::string & T ) {
  const auto R = suffixArraySais( T );
  const auto A = suffixArray( T );
  const auto B = suffixArray< std::uint64_t >( T );
  CHECK( std::equal( R.begin() , R.end() , A.begin() , A.end() ) );
  CHECK( std::equal( R.begin() , R.end() , B.begin() , B.end() ) );
  const auto L = lcpArray( T , R );
  const auto P = lcpArrayPhi( T , A );
  CHECK( std::equal( L.begin() , L.end() , P.begin() , P.end() ) );
  const auto Q = plcpArray( T , A );
  for( std::size_t r = 1 ; r < A.size() ; ++r )
   CHECK( Q[ A[ r ] ] == L[ r - 1 ] );
  if( ! A.empty() ) CHECK( Q[ A[ 0 ] ] == 0 );
  };
 check( "" );
 check( "a" );
 check( "ab" );
 check( "ba" );
 check( "aa" );
 check( "banana" );
 check( "mississippi" );
 check( std::string( 1000 , 'x' ) );
 check( std::string( "\0\xff\0\xff\xff\0" , 6 ) );
 std::mt19937_64 g( 11 );
 for( unsigned sigma : { 1u , 2u , 4u , 26u , 256u } )
  for( int r = 0 ; r < 100 ; ++r )
   check( randomString( g , g() % 2000 , sigma , sigma == 256 ? 0 : 'a' ) );
 check( fibonacciWord( 10000 ) );
 std::string P;
 for( int i = 0 ; i < 3000 ; ++i ) P += "abaabab";
 check( P );
 std::string D;                           // every LMS-substring distinct
 for( int i = 0 ; i < 200 ; ++i ) D += char( 'a' + i % 26 );
 check( D );
 for( std::size_t K : { 1 , 3 , 1000 , 100000 } )
  for( int r = 0 ; r < 20 ; ++r ) {
   const std::size_t n = g() % 3000;
   std::vector< std::uint32_t > T( n );
   Symbols U( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    U[ i ] = T[ i ] = std::uint32_t( g() % K );
   const auto R = suffixArraySais( U , K );
   const auto A = suffixArray( T , K );
   CHECK( std::equal( R.begin() , R.end() , A.begin() , A.end() ) );
   std::vector< std::uint64_t > W( T.begin() , T.end() );
   const auto B = suffixArray( W , K );
   CHECK( std::equal( R.begin() , R.end() , B.begin() , B.end() ) );
   }
 CHECK_THROWS( (void) suffixArray( std::vector< std::uint32_t >{ 0 , 5 } ,
                                   5 ) , contract_violation );
 }

// the bit-parallel edit distance against editDistance() of the textbook
// (and the audits): empty strings, one empty, equal strings, strings of 1,
// 63, 64, 65, 128 and 129 bytes (one word, the edge of the blocks), a
// common prefix and suffix only, unrelated strings and strings a few edits
// apart over 2, 4 and 26 bytes up to 700 bytes, of 5000 bytes a few edits
// apart (the diagonal transitions, which give up on a slow start), of 3000
// bytes 150 to 900 edits apart (the band doubled, then all the blocks), the
// bound k below, at and above the distance; the LCS length against lcs();
// the approximate search against the last row of matchingTable(), with k
// = 0, k >= m, the empty pattern, patterns of 1 to 200 bytes
ADS_TEST( "edit-distance" )
{
 auto check = [ & ]( const std::string & a , const std::string & b ) {
  const std::size_t d = std::size_t( editDistance( a , b ) );
  CHECK( levenshtein( a , b ) == d );
  CHECK( levenshtein( b , a ) == d );
  for( std::size_t k : { std::size_t( 0 ) , d / 2 , d , d + 1 , 2 * d } )
   CHECK( levenshtein( a , b , k ) == std::min( d , k + 1 ) );
  CHECK( lcsLength( a , b ) == lcs( a , b ).size() );
  };
 check( "" , "" );
 check( "" , "abc" );
 check( "kitten" , "sitting" );
 check( "abc" , "abc" );
 check( "a" , "b" );
 std::mt19937_64 g( 3 );
 for( std::size_t m : { 1 , 63 , 64 , 65 , 128 , 129 } ) {
  const std::string a = randomString( g , m , 4 );
  check( a , a );
  check( a , randomString( g , m , 4 ) );
  check( a , mutate( g , a , 3 , 4 ) );
  check( std::string( m , 'x' ) + a , std::string( m , 'x' ) + "y" + a );
  check( a + std::string( 70 , 'z' ) , "q" + a + std::string( 70 , 'z' ) );
  }
 {                                       // a slow start, then close
  const std::string a = randomString( g , 5000 , 4 );
  check( a , randomString( g , 40 , 4 ) + mutate( g , a , 3 , 4 ) );
  check( a , mutate( g , a , 30 , 4 ) );           // the diagonals
  }
 for( std::size_t e : { 150 , 400 , 900 } ) {     // the band doubled
  const std::string a = randomString( g , 3000 , 4 );
  check( a , mutate( g , a , e , 4 ) );
  check( a , mutate( g , a , e , 4 ) + randomString( g , e , 4 ) );
  check( randomString( g , e , 4 ) + a , mutate( g , a , e / 4 , 4 ) );
  }
 for( unsigned sigma : { 2u , 4u , 26u } )
  for( int r = 0 ; r < 60 ; ++r ) {
   const std::string a = randomString( g , g() % 700 , sigma );
   check( a , mutate( g , a , g() % 40 , sigma ) );
   check( a , randomString( g , g() % 700 , sigma ) );
   }
 for( unsigned sigma : { 2u , 4u , 26u } )
  for( int r = 0 ; r < 60 ; ++r ) {
   const std::string T = randomString( g , g() % 600 , sigma );
   std::string P = randomString( g , g() % 200 , sigma );
   if( ( T.size() > 50 ) && ( g() % 2 ) )
    P = mutate( g , T.substr( g() % ( T.size() - 50 ) , 50 ) , 4 , sigma );
   for( std::size_t k : { std::size_t( 0 ) , std::size_t( 1 ) ,
                          P.size() / 4 , P.size() / 2 , P.size() ,
                          P.size() + 3 } ) {
    const auto R = approximateMatches( T , P , k );   // an audit
    for( const auto & [ j , d ] : R ) CHECK( ( j >= 1 ) && ( d <= k ) );
    }
   }
 const auto E = approximateMatches( "abc" , "" , 0 );
 CHECK( E.size() == 3 && E[ 0 ].first == 1 && E[ 2 ].second == 0 );
 CHECK( approximateMatches( "" , "abc" , 5 ).empty() );
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
/*-------------------- End File engineered_strings.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
