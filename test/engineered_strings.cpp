/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_strings.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered algorithms on strings, beyond the book: the
 * exact search with its filter and its Two-Way fallback, against the naive
 * search on random texts and on the adversarial ones; the suffix array by
 * the engineered SA-IS and the LCP array by the permuted LCP, against the
 * constructions of the textbook; the bit-parallel edit distance, LCS and
 * approximate search against the dynamic programming of the textbook; the
 * classic exact matchers, the automaton of Aho and Corasick, the Lyndon
 * factorization, the suffix automaton and the eertree against naive
 * searches and enumerations; all at the level 3 of the contracts.
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

#include "ADS/string/AhoCorasick.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/EditDistance.h"
#include "ADS/string/LongestCommonExtension.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Lyndon.h"
#include "ADS/string/MultiSearch.h"
#include "ADS/string/Palindromes.h"
#include "ADS/string/StringMatching.h"
#include "ADS/string/StringSearch.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixAutomaton.h"
#include "ADS/string/SuffixSorting.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>
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
// pattern longer than the text, of one, two and three bytes, at the two ends
// of the text, overlapping occurrences, the bytes 0 and 255, texts shorter and
// longer than the vectors of the filter, random texts over 2, 4, 26 and 256
// bytes with patterns taken from them or not, the Fibonacci word, and the
// adversarial texts: a^n with a^m (an occurrence at every place), a^{m-1} b
// and b a^{m-1}, and a periodic text that passes every probe of its pattern
// and fails in the middle, which sends the search to the Two-Way algorithm;
// the interface of std::search(); patterns of 64 to 363 bytes, searched by the
// shifts of their q-grams, found, repeated and off by one byte, and on texts
// of 10^5 and 2 10^5 bytes, whose two halves are searched at once
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
 {                                 // the four segments at once
  const std::string T = randomString( g , 100000 , 256 , 0 );
  for( std::size_t at : { std::size_t( 0 ) , std::size_t( 1000 ) ,
                          std::size_t( 24850 ) , std::size_t( 24925 ) ,
                          std::size_t( 49800 ) , std::size_t( 50000 ) ,
                          std::size_t( 74800 ) , std::size_t( 99700 ) } ) {
   const std::string P = T.substr( at , 300 );
   checkSearch( T , P );
   checkSearch( T + T , P );
   }
  std::string W = T;                   // the same pattern in both halves
  const std::string P = T.substr( 7000 , 200 );
  W.replace( 60000 , 200 , P );
  W.replace( 30000 , 200 , P );
  checkSearch( W , P );
  for( std::size_t at : { 24900 , 49900 , 74900 , 90000 } )
   W.replace( at , 200 , P );          // in all, and across the borders
  checkSearch( W , P );
  checkSearch( W.substr( 0 , 70000 ) , P );
  }
 const std::string F = fibonacciWord( 3000 );
 for( std::size_t m : { 2 , 5 , 13 , 34 , 100 , 377 } )
  checkSearch( F , F.substr( 500 , m ) );
 for( std::size_t m : { 2 , 20 , 100 , 300 } )    // an occurrence at every
  checkSearch( std::string( 20000 , 'a' ) , std::string( m , 'a' ) );
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
// and two bytes, one repeated byte, the bytes 0 and 255, random texts over 1,
// 2, 4, 26 and 256 bytes, of 60000 bytes over 64, 128 and 256 bytes (whose few
// repeated names are sorted by doubling), the Fibonacci word, a periodic text
// whose reduced strings recurse several times, a text with all its
// LMS-substrings distinct, texts that never rise, never fall, or rise and then
// fall; the texts sorted directly by the windows of their packed codes
// (1 to 8 bits, few and many LMS places, ties at the end, short and long
// repeated blocks); integer texts over large alphabets; the LCP array of
// the permuted LCP against lcpArray() of Kasai et al., on the same texts,
// and on large texts by the bytes of the permuted LCP, with few and many
// values beyond a byte
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
 check( "zyyxxxw" );                      // never rises: all of type L
 check( "abbcd" );                        // never falls: no LMS place
 check( "aabbzzyx" );                     // rises, then falls
 check( std::string( "\0\xff\0\xff\xff\0" , 6 ) );
 std::mt19937_64 g( 11 );
 for( unsigned sigma : { 1u , 2u , 4u , 26u , 256u } )
  for( int r = 0 ; r < 100 ; ++r )
   check( randomString( g , g() % 2000 , sigma , sigma == 256 ? 0 : 'a' ) );
 check( fibonacciWord( 10000 ) );
 for( unsigned sigma : { 64u , 128u , 256u } )   // few names repeat: the
  check( randomString( g , 60000 , sigma , 0 ) );  // doubling of the groups
 // equal LMS-substrings sorted by the text after them: in small groups
 // and in groups large enough for the radix sort, and with long repeats,
 // where the first round gives up for the names
 check( randomString( g , 300000 , 26 , 'a' ) );
 {
  std::string W , R = randomString( g , 20000 , 26 , 'a' );
  for( int i = 0 ; W.size() < 120000 ; ++i ) W += i % 2 ? "the " : "then ";
  for( int i = 0 ; i < 5 ; ++i ) W += R;
  check( W );
  std::string E;                          // words: ends inside the keys
  const char * word[] = { "a " , "an " , "and " , "ant " , "antenna " };
  while( E.size() < 100000 ) E += word[ g() % 5 ];
  check( E );
  }
 // the LMS-suffixes sorted directly by the windows of the packed text:
 // codes of 1 to 6 bits and the bytes themselves, on few places (one
 // radix sort) and on many (the buckets of the top bits first); a text
 // that ends in the smallest symbol (windows padded with zeros), a short
 // block repeated (ties settled by the next windows), a long one (the
 // ties beyond the budget, and the induction instead), all the symbols
 // spread over the bytes (the ranks, not the bytes)
 for( unsigned sigma : { 2u , 3u , 5u , 16u , 17u , 33u , 64u , 65u , 200u } )
  for( std::size_t n : { std::size_t( 5000 ) , std::size_t( 250000 ) } ) {
   std::string T = randomString( g , n , sigma , sigma > 64 ? 0 : 'a' );
   check( T );
   T.append( 40 , sigma > 64 ? '\0' : 'a' );
   check( T );
   }
 {
  std::string T = randomString( g , 100000 , 256 , 0 );
  std::copy( T.begin() + 1000 , T.begin() + 1100 , T.begin() + 50000 );
  check( T );
  std::copy( T.begin() + 1000 , T.begin() + 21000 , T.begin() + 60000 );
  check( T );
  std::string U = randomString( g , 70000 , 4 , 0 );
  for( auto & c : U ) c = "\x01\x40\x7f\xfe"[ unsigned( c ) ];
  check( U );
  }
 // the LCP array of a large text read from the bytes of the permuted LCP:
 // a few values of 255 or more (read from the integers), and so many
 // that the integers are read instead
 {
  std::string T = randomString( g , 300000 , 4 , 'a' );
  for( int r = 0 ; r < 5 ; ++r )       // about 1% of the values
   std::copy( T.begin() + 1000 * r , T.begin() + 1000 * r + 600 ,
              T.begin() + 100000 + 5000 * r );
  check( T );
  std::string B = randomString( g , 3000 , 26 , 'a' ) , U;
  while( U.size() < 300000 ) U += B;
  check( U );
  }
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

// the matchers of the textbooks against naiveMatching() (and their
// audits): Boyer-Moore, Horspool, Shift-Or, the Z-function and Two-Way, on
// the empty pattern and text, a pattern longer than the text, patterns of
// 1 to 64 bytes over 1, 2, 4 and 26 letters taken from the text or not,
// a^n with a^m and a^{m-1} b, the Fibonacci word; the Z-function against
// its definition; Shift-Or refuses a pattern of 65 bytes
ADS_TEST( "classic-matching" )
{
 auto check = [ & ]( std::string_view T , std::string_view P ) {
  const auto N = naiveMatching( T , P );
  long c = 0;
  CHECK( boyerMoore( T , P , &c ) == N );
  CHECK( horspool( T , P , &c ) == N );
  if( P.size() <= 64 ) CHECK( shiftOr( T , P ) == N );
  CHECK( zMatching( T , P ) == N );
  CHECK( twoWay( T , P ) == N );
  };
 check( "" , "" );
 check( "abc" , "" );
 check( "ab" , "abc" );
 check( "abracadabra" , "abra" );
 check( std::string( 300 , 'a' ) , std::string( 7 , 'a' ) );
 check( std::string( 300 , 'a' ) , std::string( 6 , 'a' ) + "b" );
 check( std::string( 300 , 'a' ) , "b" + std::string( 6 , 'a' ) );
 check( fibonacciWord( 2000 ) , fibonacciWord( 2000 ).substr( 100 , 34 ) );
 std::mt19937_64 g( 13 );
 for( unsigned sigma : { 1u , 2u , 4u , 26u } )
  for( int r = 0 ; r < 200 ; ++r ) {
   const std::string T = randomString( g , g() % 500 , sigma );
   const std::size_t m = 1 + g() % 64;
   std::string P = randomString( g , m , sigma );
   if( ( T.size() >= m ) && ( g() % 2 ) )
    P = T.substr( g() % ( T.size() - m + 1 ) , m );
   check( T , P );
   const auto Z = zFunction( T );
   for( std::size_t i = 0 ; i < T.size() ; ++i ) {
    std::size_t k = 0;
    while( ( i + k < T.size() ) && ( T[ k ] == T[ i + k ] ) ) ++k;
    CHECK( Z[ i ] == k );
    }
   }
 CHECK( zFunction( "" ).empty() );
 CHECK_THROWS( (void) shiftOr( "abc" , std::string( 65 , 'a' ) ) ,
               contract_violation );
 }

// the automaton of Aho and Corasick against naiveMatching() on each pattern
// (and its audit): no pattern, one pattern, equal patterns, patterns that
// are prefixes, suffixes and substrings of one another, of 1 to 30 bytes
// over 2 and 4 letters, random texts; the order of the occurrences (by end,
// the longer first); a pattern added after a search, which builds again;
// the empty pattern refused
ADS_TEST( "aho-corasick" )
{
 AhoCorasick E;
 CHECK( E.findAll( "abc" ).empty() && E.size() == 0 && E.states() == 1 );
 AhoCorasick A( { "he" , "she" , "his" , "hers" , "he" } );
 const auto R = A.findAll( "ushers" );
 const std::vector< std::pair< std::size_t , std::size_t > > X = {
  { 1 , 1 } , { 2 , 0 } , { 2 , 4 } , { 2 , 3 } };
 CHECK( R == X && A.count( "ushers" ) == 4 );
 CHECK( A.add( "us" ) == 5 );
 A.build();
 CHECK( A.count( "ushers" ) == 5 );
 CHECK_THROWS( A.add( "" ) , contract_violation );
 std::mt19937_64 g( 17 );
 for( unsigned sigma : { 2u , 4u } )
  for( int r = 0 ; r < 100 ; ++r ) {
   std::vector< std::string > P;
   const std::string T = randomString( g , g() % 800 , sigma );
   for( std::size_t k = 1 + g() % 12 ; k-- > 0 ; )
    P.push_back( ( T.size() > 30 ) && ( g() % 2 ) ?
                 T.substr( g() % ( T.size() - 30 ) , 1 + g() % 30 ) :
                 randomString( g , 1 + g() % 8 , sigma ) );
   const AhoCorasick B( P );
   const auto S = B.findAll( T );                     // an audit
   std::size_t total = 0;
   for( const auto & p : P ) total += naiveMatching( T , p ).size();
   CHECK( S.size() == total && B.count( T ) == total );
   for( std::size_t k = 1 ; k < S.size() ; ++k ) {
    const std::size_t e0 = S[ k - 1 ].first + P[ S[ k - 1 ].second ].size() ,
                      e1 = S[ k ].first + P[ S[ k ].second ].size();
    CHECK( ( e0 < e1 ) || ( ( e0 == e1 ) && ( S[ k - 1 ].first <=
                                                S[ k ].first ) ) );
    }
   }
 }

// the Lyndon factorization of Duval against its definition (the audit) and
// against the factorization by brute force (the longest Lyndon prefix
// first); the least rotation against all the rotations: the empty string,
// one byte, one repeated byte, a Lyndon word, a decreasing string, random
// strings over 1, 2 and 3 letters
ADS_TEST( "lyndon" )
{
 CHECK( lyndonFactorization( "" ).empty() && leastRotation( "" ) == 0 );
 CHECK( lyndonFactorization( "a" ) == std::vector< std::size_t >{ 0 } );
 CHECK( lyndonFactorization( "aaa" ) ==
        ( std::vector< std::size_t >{ 0 , 1 , 2 } ) );
 CHECK( lyndonFactorization( "aab" ) == std::vector< std::size_t >{ 0 } );
 CHECK( lyndonFactorization( "cba" ) ==
        ( std::vector< std::size_t >{ 0 , 1 , 2 } ) );
 CHECK( isLyndon( "aabab" ) && ! isLyndon( "abab" ) && ! isLyndon( "" ) );
 std::mt19937_64 g( 19 );
 for( unsigned sigma : { 1u , 2u , 3u } )
  for( int r = 0 ; r < 300 ; ++r ) {
   const std::string S = randomString( g , g() % 60 , sigma );
   std::vector< std::size_t > F;      // the longest Lyndon prefix first
   for( std::size_t i = 0 ; i < S.size() ; ) {
    F.push_back( i );
    std::size_t l = 1;
    for( std::size_t k = S.size() - i ; k > 0 ; --k )
     if( isLyndon( std::string_view( S ).substr( i , k ) ) ) {
      l = k;
      break;
      }
    i += l;
    }
   CHECK( lyndonFactorization( S ) == F );
   const std::size_t q = leastRotation( S );         // an audit
   CHECK( q < std::max< std::size_t >( S.size() , 1 ) );
   for( std::size_t i = 0 ; i < q ; ++i )            // the first least
    CHECK( S.substr( i ) + S.substr( 0 , i ) >
           S.substr( q ) + S.substr( 0 , q ) );
   }
 }

// the suffix automaton against the substrings enumerated: the empty
// string, one byte, a^n (n + 1 states), random strings over 1, 2, 4 and 26
// letters: at most 2 n - 1 states, contains() and occurrences() on every
// substring and on strings that do not occur, the number of the distinct
// substrings, the longest common substring with another string; built in
// one go and a byte at a time
ADS_TEST( "suffix-automaton" )
{
 const SuffixAutomaton E;
 CHECK( E.states() == 1 && E.distinctSubstrings() == 0 &&
        E.contains( "" ) && ! E.contains( "a" ) &&
        E.occurrences( "" ) == 1 );
 const SuffixAutomaton A( std::string( 50 , 'a' ) );
 CHECK( A.states() == 51 && A.distinctSubstrings() == 50 &&
        A.occurrences( "aaa" ) == 48 );
 std::mt19937_64 g( 23 );
 for( unsigned sigma : { 1u , 2u , 4u , 26u } )
  for( int r = 0 ; r < 40 ; ++r ) {
   const std::string S = randomString( g , g() % 120 , sigma );
   SuffixAutomaton B;
   for( char c : S ) B.extend( static_cast< unsigned char >( c ) );
   const SuffixAutomaton M( S );
   CHECK( M.states() == B.states() && M.size() == S.size() );
   CHECK( M.states() <= ( S.size() < 2 ? S.size() + 1 : 2 * S.size() - 1 ) );
   std::set< std::string > sub;
   for( std::size_t i = 0 ; i < S.size() ; ++i )
    for( std::size_t l = 1 ; i + l <= S.size() ; ++l )
     sub.insert( S.substr( i , l ) );
   CHECK( M.distinctSubstrings() == sub.size() );
   for( const auto & u : sub )
    CHECK( M.contains( u ) && M.occurrences( u ) ==
           naiveMatching( S , u ).size() );
   for( int k = 0 ; k < 20 ; ++k ) {
    const std::string u = randomString( g , 1 + g() % 6 , sigma + 1 );
    CHECK( M.contains( u ) == sub.count( u ) );
    CHECK( M.occurrences( u ) == naiveMatching( S , u ).size() );
    }
   const std::string T = randomString( g , g() % 100 , sigma );
   std::size_t best = 0;
   for( std::size_t j = 0 ; j < T.size() ; ++j )
    for( std::size_t l = 1 ; j + l <= T.size() ; ++l )
     if( sub.count( T.substr( j , l ) ) ) best = std::max( best , l );
   const auto [ len , end ] = M.longestCommonSubstring( T );
   CHECK( len == best );
   if( len > 0 ) CHECK( sub.count( T.substr( end - len , len ) ) );
   }
 }

// the eertree and the palindromic factorization in O( n log n ) against
// palindromicPartition() of the textbook (and the audit): the empty string,
// one byte, a^n (n distinct palindromes, one piece), a string with no
// palindrome longer than 1, random strings over 1, 2, 3 and 26 letters;
// the longest common extensions against the direct comparison (and an
// audit each): the empty text and one symbol, the ends of the text, i = j,
// extensions shorter than 16 and longer, within a block of the range
// minimum and across blocks, on random texts, the Fibonacci word, a^n and
// texts with bytes 0 (which the padding of the text must not extend)
ADS_TEST( "lce" )
{
 auto check = []( const std::string & T , std::mt19937_64 & g ,
                  int queries ) {
  const LceIndex<> X( T );
  const LceIndex< std::uint64_t > Y( T );
  const std::size_t n = T.size();
  auto naive = [ & ]( std::size_t i , std::size_t j ) {
   std::size_t h = 0;
   while( ( i + h < n ) && ( j + h < n ) && ( T[ i + h ] == T[ j + h ] ) )
    ++h;
   return( h );
   };
  for( std::size_t i : { std::size_t( 0 ) , n / 2 , n } )
   for( std::size_t j : { std::size_t( 0 ) , n / 3 , n ? n - 1 : 0 , n } )
    CHECK( X.lce( i , j ) == naive( i , j ) );
  for( int q = 0 ; q < queries ; ++q ) {
   const std::size_t i = g() % ( n + 1 ) , j = g() % ( n + 1 );
   CHECK( X.lce( i , j ) == naive( i , j ) );
   CHECK( Y.lce( i , j ) == naive( i , j ) );
   }
  };
 std::mt19937_64 g( 31 );
 check( "" , g , 4 );
 check( "a" , g , 4 );
 check( std::string( 1 , '\0' ) , g , 4 );
 check( std::string( 300 , '\0' ) , g , 200 );
 check( std::string( 5000 , 'a' ) , g , 500 );
 check( fibonacciWord( 3000 ) , g , 2000 );
 for( unsigned sigma : { 2u , 4u , 26u , 256u } )
  for( std::size_t n : { 10 , 100 , 1000 , 20000 } )
   check( randomString( g , n , sigma , sigma == 256 ? 0 : 'a' ) , g , 500 );
 std::string R = randomString( g , 3000 , 4 , 'a' );
 check( R + R + R , g , 2000 );          // long extensions, many blocks
 // suffixes near in SA, up to 5000 ranks apart: every shape of the range
 // minimum (one block, two, within a group of blocks, across groups),
 // against the minimum of LCP by a scan
 for( const std::string & T : { fibonacciWord( 100000 ) ,
                                randomString( g , 100000 , 2 , 'a' ) } ) {
  const auto SA = suffixArray( T );
  const auto LCP = lcpArrayPhi( T , SA );
  std::vector< std::size_t > rank( T.size() );
  for( std::size_t r = 0 ; r < T.size() ; ++r ) rank[ SA[ r ] ] = r;
  const LceIndex<> X( T );
  for( int q = 0 ; q < 3000 ; ++q ) {
   const std::size_t r = g() % T.size() ,
                     d = 1 + g() % ( q % 2 ? 40 : 5000 ) ,
                     i = SA[ r ] , j = SA[ std::min( r + d , T.size() - 1 ) ];
   if( i == j ) continue;
   const std::size_t a = std::min( rank[ i ] , rank[ j ] ) ,
                     b = std::max( rank[ i ] , rank[ j ] );
   CHECK( X.lce( i , j ) ==
          *std::min_element( LCP.begin() + std::ptrdiff_t( a ) ,
                             LCP.begin() + std::ptrdiff_t( b ) ) );
   }
  }
 }

// the search of many patterns against naiveMatching() of each (and an
// audit): no pattern, patterns of 1 to 3 bytes (filters of fewer bytes),
// equal and overlapping patterns, prefixes of one another, at the two ends
// of the text, bytes of the high half (above 127), texts shorter than a
// block, and more patterns than the filter takes (the automaton)
ADS_TEST( "multi-search" )
{
 auto check = []( std::string_view T , const std::vector< std::string > & P ) {
  const MultiSearcher S( P );
  std::vector< std::pair< std::size_t , std::size_t > > N;
  for( std::size_t j = 0 ; j < P.size() ; ++j )
   for( const std::size_t i : naiveMatching( T , P[ j ] ) )
    N.emplace_back( i , j );
  std::sort( N.begin() , N.end() );
  CHECK( S.findAll( T ) == N );           // an audit too
  CHECK( S.count( T ) == N.size() );
  CHECK( S.filtered() == ( ! P.empty() &&
                           ( P.size() <= MultiSearcher::kFilter ) ) );
  };
 check( "abc" , {} );
 check( "" , { "a" } );
 check( "aaaa" , { "a" , "aa" , "aa" , "aaa" } );
 check( "abcabcab" , { "ab" , "abc" , "bca" , "cab" , "b" } );
 check( std::string( "\xff\x80\x7f\xff\x80" , 5 ) ,
        { std::string( "\xff\x80" , 2 ) , std::string( "\x80\x7f" , 2 ) } );
 std::mt19937_64 g( 37 );
 for( unsigned sigma : { 2u , 4u , 26u , 256u } )
  for( std::size_t k : { 1 , 2 , 7 , 8 , 9 , 30 , 64 , 65 , 200 } )
   for( int r = 0 ; r < 3 ; ++r ) {
    const std::string T = randomString( g , g() % 3000 , sigma ,
                                        sigma == 256 ? 0 : 'a' );
    std::vector< std::string > P;
    for( std::size_t z = 0 ; z < k ; ++z ) {
     const std::size_t m = 1 + g() % 12;
     if( ( T.size() >= m ) && ( g() % 4 ) )
      P.push_back( T.substr( g() % ( T.size() - m + 1 ) , m ) );
     else
      P.push_back( randomString( g , m , sigma , sigma == 256 ? 0 : 'a' ) );
     }
    check( T , P );
    check( T.substr( 0 , std::min< std::size_t >( T.size() , 40 ) ) , P );
    }
 // the filter that lets too much through, the automaton from where it
 // gives up: patterns everywhere, and a small alphabet
 check( std::string( 20000 , 'a' ) , { "a" , "aaa" , "ab" , "aaaaaaaaa" } );
 for( unsigned sigma : { 2u , 3u } )
  for( int k : { 16 , 100 } ) {
   const std::string T = randomString( g , 50000 , sigma , 'a' );
   std::vector< std::string > P;
   for( int z = 0 ; z < k ; ++z ) P.push_back( T.substr( g() % 40000 ,
                                                          4 + g() % 5 ) );
   check( T , P );
   }
 // the shift-or filter on many patterns, of 1 to 20 bytes, and with
 // prefixes shared
 for( std::size_t least : { 1 , 2 , 5 , 8 } ) {
  const std::string T = randomString( g , 60000 , 26 , 'a' );
  std::vector< std::string > P;
  for( int z = 0 ; z < 300 ; ++z ) {
   const std::size_t m = least + g() % 12;
   P.push_back( z % 3 ? T.substr( g() % ( T.size() - m ) , m )
                      : randomString( g , m , 26 , 'a' ) );
   }
  P.push_back( P[ 0 ] + "xyz" );
  check( T , P );
  check( T.substr( 0 , 100 ) , P );
  }
 }

// the number of distinct palindromes against their enumeration
ADS_TEST( "eertree" )
{
 CHECK( palindromicFactorization( "" ).empty() );
 CHECK( palindromicFactorization( "a" ).size() == 1 );
 CHECK( palindromicFactorization( std::string( 40 , 'a' ) ).size() == 1 );
 CHECK( palindromicFactorization( "abcd" ).size() == 4 );
 Eertree A( std::string( 40 , 'a' ) );
 CHECK( A.distinct() == 40 && A.suffixLength() == 40 && A.size() == 40 );
 CHECK( ! A.add( 'a' ) == false );
 std::mt19937_64 g( 29 );
 for( unsigned sigma : { 1u , 2u , 3u , 26u } )
  for( int r = 0 ; r < 150 ; ++r ) {
   const std::string S = randomString( g , g() % 200 , sigma );
   const auto P = palindromicFactorization( S );     // an audit
   CHECK( P.size() == palindromicPartition( S ).size() );
   std::set< std::string > pal;
   for( std::size_t i = 0 ; i < S.size() ; ++i )
    for( std::size_t l = 1 ; i + l <= S.size() ; ++l ) {
     const std::string u = S.substr( i , l );
     if( std::equal( u.begin() , u.end() , u.rbegin() ) ) pal.insert( u );
     }
   CHECK( Eertree( S ).distinct() == pal.size() );
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
/*-------------------- End File engineered_strings.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
