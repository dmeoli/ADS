/*--------------------------------------------------------------------------*/
/*-------------------------- File lib_strings.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the classic problems on strings of the library: the palindromes
 * (the longest substring and subsequence, the fewest palindromic pieces),
 * the interleaving, the shortest common supersequence, the occurrences of a
 * subsequence and the LCS of three strings, the counts of segmentations and
 * of decodings, the longest balanced parentheses, the classes of anagrams
 * and the breaking of a string. Each one against an independent oracle (a
 * brute force on small random inputs with a fixed seed, a slower dynamic
 * programming, a closed formula) and on the edge cases, at the level 3 of
 * the contracts.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#include "ADS/string/Anagrams.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Palindromes.h"
#include "ADS/string/Parentheses.h"
#include "ADS/string/Segmentations.h"
#include "ADS/string/StringBreaking.h"
#include "ADS/string/SubsequenceProblems.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
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

/// a random string of n letters among the first k
std::string word( std::mt19937 & g , std::size_t n , int k )
{
 std::string s( n , 'a' );
 for( auto & c : s )
  c = char( 'a' + int( g() % unsigned( k ) ) );
 return( s );
 }

/// whether s reads the same backwards
template< class S >
bool palindrome( const S & s )
{
 return( std::equal( s.begin() , s.end() , s.rbegin() ) );
 }

/// whether a is a subsequence of b
template< class S >
bool subsequenceOf( const S & a , const S & b )
{
 std::size_t i = 0;
 for( std::size_t j = 0 ; ( i < a.size() ) && ( j < b.size() ) ; ++j )
  if( a[ i ] == b[ j ] ) ++i;
 return( i == a.size() );
 }

/// the subsequence of s chosen by the bits of mask
std::string pick( const std::string & s , std::uint32_t mask )
{
 std::string r;
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  if( mask >> i & 1 ) r += s[ i ];
 return( r );
 }

/// the Fibonacci number F_n, F_1 = F_2 = 1
std::uint64_t fibonacci( int n )
{
 std::uint64_t a = 0 , b = 1;
 for( int i = 0 ; i < n ; ++i ) {
  const std::uint64_t c = a + b;
  a = b;
  b = c;
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// Manacher against every substring on 2000 random strings of up to 14
// letters over 2 or 3 symbols, the leftmost of the longest; the examples,
// the empty string, one symbol repeated, a sequence of integers
ADS_TEST( "palindromic-substring" )
{
 CHECK( longestPalindromicSubstring( "casacca" ) == "casac" );
 CHECK( longestPalindromicSubstring( "ovattavo con l'ovatta" ) ==
        "ovattavo" );
 CHECK( longestPalindromicSubstring( "abcd" ) == "a" );
 CHECK( longestPalindromicSubstring( "abba" ) == "abba" );
 CHECK( longestPalindromicSubstring( "" ).empty() );
 CHECK( longestPalindrome( std::string() ) == std::pair( std::size_t( 0 ) ,
                                                         std::size_t( 0 ) ) );
 CHECK( longestPalindromicSubstring( std::string( 5000 , 'z' ) ).size() ==
        5000 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  const std::string s = word( g , g() % 15 , 2 + int( g() % 2 ) );
  std::size_t best = 0 , start = 0;
  for( std::size_t len = s.size() ; ( len > 0 ) && ( best == 0 ) ; --len )
   for( std::size_t i = 0 ; i + len <= s.size() ; ++i )
    if( palindrome( s.substr( i , len ) ) ) {
     best = len;
     start = i;
     break;
     }
  CHECK( longestPalindrome( s ) == std::pair( start , best ) );
  CHECK( longestPalindromicSubstring( s ) == s.substr( start , best ) );
  }
 const std::vector< int > v = { 1 , 2 , 3 , 2 , 1 , 1 , 2 };
 CHECK( longestPalindromicSubstring( v ) ==
        std::vector< int >( { 1 , 2 , 3 , 2 , 1 } ) );
 }

// against the longest palindrome among all the subsequences of up to 12
// letters, and against the LCS of the string and its reverse up to 40;
// the example BBABCBCAB (7), the edge cases, a sequence of integers
ADS_TEST( "palindromic-subsequence" )
{
 const auto p = longestPalindromicSubsequence( "BBABCBCAB" );
 CHECK( ( p.size() == 7 ) && palindrome( p ) &&
        subsequenceOf( p , std::string( "BBABCBCAB" ) ) );
 CHECK( longestPalindromicSubsequence( "" ).empty() );
 CHECK( longestPalindromicSubsequence( "q" ) == "q" );
 CHECK( longestPalindromicSubsequence( "abc" ).size() == 1 );
 CHECK( longestPalindromicSubsequence( std::string( 600 , 'k' ) ).size() ==
        600 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const std::string s = word( g , g() % 13 , 2 + int( g() % 2 ) );
  std::size_t best = 0;
  for( std::uint32_t m = 0 ; m < ( 1u << s.size() ) ; ++m ) {
   const auto q = pick( s , m );
   if( palindrome( q ) ) best = std::max( best , q.size() );
   }
  const auto r = longestPalindromicSubsequence( s );
  CHECK( ( r.size() == best ) && palindrome( r ) && subsequenceOf( r , s ) );
  }
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::string s = word( g , g() % 41 , 4 );
  CHECK( longestPalindromicSubsequence( s ).size() ==
         lcs( s , std::string( s.rbegin() , s.rend() ) ).size() );
  }
 const std::vector< int > v = { 5 , 1 , 7 , 2 , 1 , 5 };
 CHECK( longestPalindromicSubsequence( v ).size() == 5 );
 }

// against all the 2^( n - 1 ) cuts of 400 random strings of up to 12
// letters, and against the cubic interval recursion on strings of 80; the
// examples BUB.BASEESAB.ANANA (3) and AFISICACISIFA (1), the edge cases
ADS_TEST( "palindromic-partition" )
{
 const auto P = palindromicPartition( "BUBBASEESABANANA" );
 CHECK( P == std::vector< std::string >( { "BUB" , "BASEESAB" , "ANANA" } ) );
 CHECK( palindromicPartition( "AFISICACISIFA" ).size() == 1 );
 CHECK( palindromicPartition( "" ).empty() );
 CHECK( palindromicPartition( "x" ).size() == 1 );
 CHECK( palindromicPartition( "abcdefghij" ).size() == 10 );
 CHECK( palindromicPartition( std::string( 3000 , 'a' ) ).size() == 1 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const std::string s = word( g , 1 + g() % 12 , 2 + int( g() % 2 ) );
  const std::size_t n = s.size();
  std::size_t best = n;
  for( std::uint32_t m = 0 ; m < ( 1u << ( n - 1 ) ) ; ++m ) {
   std::size_t parts = 0 , start = 0;
   bool ok = true;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( ( i == n - 1 ) || ( m >> i & 1 ) ) {
     ok = ok && palindrome( s.substr( start , i + 1 - start ) );
     ++parts;
     start = i + 1;
     }
   if( ok ) best = std::min( best , parts );
   }
  const auto pieces = palindromicPartition( s );
  CHECK( pieces.size() == best );
  CHECK( std::accumulate( pieces.begin() , pieces.end() , std::string() ) ==
         s );
  }
 for( int t = 0 ; t < 20 ; ++t ) {
  const std::string s = word( g , 80 , 2 );
  const int n = int( s.size() );
  std::vector< std::vector< int > > D( n , std::vector< int >( n , 0 ) );
  for( int len = 1 ; len <= n ; ++len )
   for( int i = 0 ; i + len <= n ; ++i ) {
    const int j = i + len - 1;
    if( palindrome( s.substr( i , len ) ) ) {
     D[ i ][ j ] = 1;
     continue;
     }
    D[ i ][ j ] = n;
    for( int k = i ; k < j ; ++k )
     D[ i ][ j ] = std::min( D[ i ][ j ] , D[ i ][ k ] + D[ k + 1 ][ j ] );
    }
  CHECK( int( palindromicPartition( s ).size() ) == D[ 0 ][ n - 1 ] );
  }
 }

// against the plain recursion on 600 random triples, also with the
// interleavings built at random; the examples, the lengths that do not
// add up, the empty strings, a sequence of integers
ADS_TEST( "interleaving" )
{
 std::function< bool( const std::string & , const std::string & ,
                      const std::string & , std::size_t , std::size_t ) >
  rec = [ & ]( const std::string & X , const std::string & Y ,
               const std::string & Z , std::size_t i , std::size_t j ) {
   if( i + j == Z.size() ) return( ( i == X.size() ) && ( j == Y.size() ) );
   return( ( ( i < X.size() ) && ( X[ i ] == Z[ i + j ] ) &&
             rec( X , Y , Z , i + 1 , j ) ) ||
           ( ( j < Y.size() ) && ( Y[ j ] == Z[ i + j ] ) &&
             rec( X , Y , Z , i , j + 1 ) ) );
   };
 CHECK( isInterleaving( "AB" , "CD" , "ACBD" ) );
 CHECK( ! isInterleaving( "AB" , "CD" , "BACD" ) );
 CHECK( isInterleaving( "123" , "345" , "123435" ) );
 CHECK( ! isInterleaving( "123" , "345" , "143235" ) );
 CHECK( ! isInterleaving( "ABC" , "ABC" , "ABCDEF" ) );
 CHECK( ! isInterleaving( "AB" , "C" , "ABCD" ) );
 CHECK( isInterleaving( "" , "" , "" ) && ! isInterleaving( "" , "" , "a" ) );
 CHECK( isInterleaving( "ab" , "" , "ab" ) &&
        ! isInterleaving( "" , "ab" , "ba" ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::string X = word( g , g() % 7 , 2 ) , Y = word( g , g() % 7 , 2 );
  std::string Z;
  if( t % 2 ) {
   std::size_t i = 0 , j = 0;
   while( ( i < X.size() ) || ( j < Y.size() ) )
    if( ( j == Y.size() ) || ( ( i < X.size() ) && ( g() % 2 ) ) )
     Z += X[ i++ ];
    else
     Z += Y[ j++ ];
   CHECK( isInterleaving( X , Y , Z ) );
   }
  else
   Z = word( g , X.size() + Y.size() , 2 );
  CHECK( isInterleaving( X , Y , Z ) == rec( X , Y , Z , 0 , 0 ) );
  }
 const std::vector< int > a = { 1 , 2 } , b = { 3 } , c = { 1 , 3 , 2 } ,
                          d = { 2 , 3 , 1 };
 CHECK( isInterleaving( a , b , c ) && ! isInterleaving( a , b , d ) );
 }

// scsLength() against the shortest string over the alphabet holding both,
// on 300 random pairs of up to 4 letters over 2, and against n + m - | LCS |
// on longer ones; the string of shortestCommonSupersequence() holds both
// and is that long; the examples AB, BC and DAB, DCB, the edge cases
ADS_TEST( "supersequence" )
{
 CHECK( scsLength( "AB" , "BC" ) == 3 && scsLength( "DAB" , "DCB" ) == 4 );
 CHECK( shortestCommonSupersequence( "AB" , "BC" ) == "ABC" );
 CHECK( shortestCommonSupersequence( "DAB" , "DCB" ).size() == 4 );
 CHECK( scsLength( "" , "" ) == 0 && scsLength( "abc" , "" ) == 3 );
 CHECK( shortestCommonSupersequence( "" , "abc" ) == "abc" );
 CHECK( shortestCommonSupersequence( "abc" , "abc" ) == "abc" );
 CHECK( scsLength( std::string( 500 , 'a' ) , std::string( 500 , 'b' ) ) ==
        1000 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string P = word( g , g() % 5 , 2 ) , T = word( g , g() % 5 , 2 );
  std::size_t best = 0;
  for( bool found = false ; ! found ; ++best )
   for( std::uint32_t w = 0 ; ( w < ( 1u << best ) ) && ! found ; ++w ) {
    std::string s( best , 'a' );
    for( std::size_t i = 0 ; i < best ; ++i )
     if( w >> i & 1 ) s[ i ] = 'b';
    found = subsequenceOf( P , s ) && subsequenceOf( T , s );
    }
  --best;
  CHECK( scsLength( P , T ) == best );
  }
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string P = word( g , g() % 30 , 3 ) ,
                    T = word( g , g() % 30 , 3 );
  const auto S = shortestCommonSupersequence( P , T );
  CHECK( S.size() == P.size() + T.size() - lcs( P , T ).size() );
  CHECK( scsLength( P , T ) == S.size() );
  CHECK( subsequenceOf( P , S ) && subsequenceOf( T , S ) );
  }
 const std::vector< int > a = { 1 , 2 , 3 } , b = { 2 , 4 };
 CHECK( shortestCommonSupersequence( a , b ).size() == 4 );
 }

// against every set of places of T on 500 random pairs; the example dog in
// "did you go" (2); a^m in a^n, C( n , m ), also beyond 2^64 against the
// triangle of Tartaglia computed modulo 2^64; the edge cases
ADS_TEST( "subsequence-occurrences" )
{
 CHECK( subsequenceOccurrences( "dog" , "did you go" ) == 2 );
 CHECK( subsequenceOccurrences( "abc" , "aabdcc" ) == 4 );
 CHECK( subsequenceOccurrences( "" , "" ) == 1 );
 CHECK( subsequenceOccurrences( "" , "abc" ) == 1 );
 CHECK( subsequenceOccurrences( "abcd" , "abc" ) == 0 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::string T = word( g , g() % 13 , 2 + int( g() % 2 ) ) ,
                    P = word( g , g() % 5 , 2 + int( g() % 2 ) );
  std::uint64_t brute = 0;
  for( std::uint32_t m = 0 ; m < ( 1u << T.size() ) ; ++m )
   if( pick( T , m ) == P ) ++brute;
  CHECK( subsequenceOccurrences( P , T ) == brute );
  }
 std::vector< std::vector< std::uint64_t > > C( 101 );
 for( std::size_t n = 0 ; n <= 100 ; ++n ) {
  C[ n ].assign( n + 1 , 1 );
  for( std::size_t k = 1 ; k < n ; ++k )
   C[ n ][ k ] = C[ n - 1 ][ k - 1 ] + C[ n - 1 ][ k ];
  }
 for( std::size_t n : { 10 , 60 , 100 } )
  for( std::size_t m = 0 ; m <= n ; m += 7 )
   CHECK( subsequenceOccurrences( std::string( m , 'a' ) ,
                                  std::string( n , 'a' ) ) == C[ n ][ m ] );
 const std::vector< int > p = { 1 , 2 } , q = { 1 , 1 , 2 , 2 };
 CHECK( subsequenceOccurrences( p , q ) == 4 );
 }

// against every subsequence of the first string on 300 random triples of
// up to 10 letters over 3; the examples, where the pairwise LCS fails; the
// edge cases, a sequence of integers
ADS_TEST( "lcs3" )
{
 CHECK( lcs3( "ccabbacc" , "ccccabba" , "cccc" ) == "cccc" );
 CHECK( lcs3( "ABC" , "BCD" , "CDE" ) == "C" );
 CHECK( lcs3( "" , "abc" , "abc" ).empty() );
 CHECK( lcs3( "aaaa" , "aaaa" , "aaaa" ) == "aaaa" );
 CHECK( lcs3( "abc" , "def" , "abc" ).empty() );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string X = word( g , g() % 11 , 3 ) ,
                    Y = word( g , g() % 11 , 3 ) ,
                    Z = word( g , g() % 11 , 3 );
  std::size_t best = 0;
  for( std::uint32_t m = 0 ; m < ( 1u << X.size() ) ; ++m ) {
   const auto P = pick( X , m );
   if( subsequenceOf( P , Y ) && subsequenceOf( P , Z ) )
    best = std::max( best , P.size() );
   }
  const auto r = lcs3( X , Y , Z );
  CHECK( ( r.size() == best ) && subsequenceOf( r , X ) &&
         subsequenceOf( r , Y ) && subsequenceOf( r , Z ) );
  }
 const std::vector< int > a = { 1 , 2 , 3 } , b = { 2 , 3 , 1 } ,
                          c = { 3 , 2 , 3 };
 CHECK( lcs3( a , b , c ) == std::vector< int >( { 2 , 3 } ) );
 }

// against the plain recursion on 400 random binary strings and lists of
// words; the examples, a^90 over { a , aa } (F_91), a repeated word, the
// empty string, no words, an empty word
ADS_TEST( "segmentations" )
{
 const std::vector< std::string > S = { "01" , "10" , "011" , "101" };
 CHECK( countSegmentations( "0111010101" , S ) == 3 );
 CHECK( countSegmentations( "0110001" , S ) == 0 );
 CHECK( countSegmentations( "" , S ) == 1 );
 CHECK( countSegmentations( "01" , {} ) == 0 );
 CHECK( countSegmentations( "aa" , { "a" , "a" } ) == 4 );
 CHECK( countSegmentations( std::string( 90 , 'a' ) , { "a" , "aa" } ) ==
        fibonacci( 91 ) );
 std::function< std::uint64_t( const std::string & ,
                               const std::vector< std::string > & ,
                               std::size_t ) > brute =
  [ & ]( const std::string & X , const std::vector< std::string > & W ,
         std::size_t i ) -> std::uint64_t {
   if( i == X.size() ) return( 1 );
   std::uint64_t c = 0;
   for( const auto & w : W )
    if( X.compare( i , w.size() , w ) == 0 )
     c += brute( X , W , i + w.size() );
   return( c );
   };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  std::vector< std::string > W( 1 + g() % 5 );
  for( auto & w : W ) w = word( g , 1 + g() % 3 , 2 );
  const std::string X = word( g , g() % 19 , 2 );
  CHECK( countSegmentations( X , W ) == brute( X , W , 0 ) );
  }
 const std::vector< int > x = { 1 , 2 , 1 , 2 };
 const std::vector< std::vector< int > > w = { { 1 } , { 2 } , { 1 , 2 } };
 CHECK( countSegmentations( x , w ) == 4 );
 CHECK_THROWS( countSegmentations( "0" , { "" } ) , contract_violation );
 }

// against the recursion on the pieces 1 ... 9 and 10 ... 26 on 1000 random
// strings rich in 0, 1, 2; the example 13135 (4), no digits, the zeros,
// 27, 90 ones (F_91); a digit out of 0 .. 9
ADS_TEST( "decodings" )
{
 CHECK( countDecodings( { 1 , 3 , 1 , 3 , 5 } ) == 4 );
 CHECK( countDecodings( {} ) == 1 && countDecodings( { 0 } ) == 0 );
 CHECK( countDecodings( { 1 , 0 } ) == 1 && countDecodings( { 3 , 0 } ) == 0 );
 CHECK( countDecodings( { 2 , 6 } ) == 2 && countDecodings( { 2 , 7 } ) == 1 );
 CHECK( countDecodings( { 2 , 0 , 2 , 0 } ) == 1 );
 CHECK( countDecodings( { 0 , 1 , 1 } ) == 0 );
 CHECK( countDecodings( std::vector< int >( 90 , 1 ) ) == fibonacci( 91 ) );
 std::function< std::uint64_t( const std::vector< int > & ,
                               std::size_t ) > brute =
  [ & ]( const std::vector< int > & T , std::size_t i ) -> std::uint64_t {
   if( i == T.size() ) return( 1 );
   if( T[ i ] == 0 ) return( 0 );
   std::uint64_t c = brute( T , i + 1 );
   if( ( i + 1 < T.size() ) && ( 10 * T[ i ] + T[ i + 1 ] <= 26 ) )
    c += brute( T , i + 2 );
   return( c );
   };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  std::vector< int > T( g() % 19 );
  for( auto & d : T ) d = g() % 4 ? int( g() % 3 ) : int( g() % 10 );
  CHECK( countDecodings( T ) == brute( T , 0 ) );
  }
 CHECK_THROWS( countDecodings( { 1 , 10 } ) , contract_violation );
 CHECK_THROWS( countDecodings( { -1 } ) , contract_violation );
 }

// against every subsequence of 500 random strings over ( ) and another
// character; the example of 10, ()() , other parentheses; open == close
ADS_TEST( "balanced-subsequence" )
{
 const std::string E = "che)esercizio)(interess)()ante((())questo()qui";
 CHECK( longestBalancedSubsequence( E ) == 10 );
 CHECK( longestBalancedSubsequence( "()()" ) == 4 );
 CHECK( longestBalancedSubsequence( "" ) == 0 );
 CHECK( longestBalancedSubsequence( ")(" ) == 0 );
 CHECK( longestBalancedSubsequence( "[x]][" , '[' , ']' ) == 2 );
 CHECK( longestBalancedSubsequence( std::string( 500 , '(' ) +
                                    std::string( 500 , ')' ) ) == 1000 );
 auto balanced = []( const std::string & s ) {
  int d = 0;
  for( char c : s ) {
   d += c == '(' ? 1 : -1;
   if( d < 0 ) return( false );
   }
  return( d == 0 );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::string S;
  for( std::size_t i = g() % 15 ; i > 0 ; --i ) S += "()x"[ g() % 3 ];
  std::string p;
  for( char c : S ) if( c != 'x' ) p += c;
  std::size_t best = 0;
  for( std::uint32_t m = 0 ; m < ( 1u << p.size() ) ; ++m ) {
   const auto s = pick( p , m );
   if( balanced( s ) ) best = std::max( best , s.size() );
   }
  CHECK( longestBalancedSubsequence( S ) == best );
  }
 CHECK_THROWS( longestBalancedSubsequence( "aa" , 'a' , 'a' ) ,
               contract_violation );
 }

// against every substring of 1000 random strings over ( ) and another
// character, the leftmost of the longest; the classic examples, the edge
// cases, other parentheses; open == close
ADS_TEST( "balanced-substring" )
{
 using P = std::pair< std::size_t , std::size_t >;
 CHECK( longestBalancedSubstring( ")()())" ) == P( 1 , 4 ) );
 CHECK( longestBalancedSubstring( "(()" ) == P( 1 , 2 ) );
 CHECK( longestBalancedSubstring( "()(()" ) == P( 0 , 2 ) );
 CHECK( longestBalancedSubstring( "()x()" ) == P( 0 , 2 ) );
 CHECK( longestBalancedSubstring( "" ) == P( 0 , 0 ) );
 CHECK( longestBalancedSubstring( ")(" ) == P( 0 , 0 ) );
 CHECK( longestBalancedSubstring( "a<<>><>" , '<' , '>' ) == P( 1 , 6 ) );
 auto balanced = []( const std::string & s ) {
  int d = 0;
  for( char c : s ) {
   if( ( c != '(' ) && ( c != ')' ) ) return( false );
   d += c == '(' ? 1 : -1;
   if( d < 0 ) return( false );
   }
  return( d == 0 );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  std::string S;
  for( std::size_t i = g() % 20 ; i > 0 ; --i )
   S += "(()))x"[ g() % ( t % 2 ? 5 : 6 ) ];
  P best( 0 , 0 );
  for( std::size_t len = S.size() ; ( len > 0 ) && ( best.second == 0 ) ;
       --len )
   for( std::size_t i = 0 ; i + len <= S.size() ; ++i )
    if( balanced( S.substr( i , len ) ) ) {
     best = P( i , len );
     break;
     }
  CHECK( longestBalancedSubstring( S ) == best );
  }
 CHECK_THROWS( longestBalancedSubstring( "aa" , 'a' , 'a' ) ,
               contract_violation );
 }

/// a hash of a vector of integers
struct VectorHash {
 std::size_t operator()( const std::vector< int > & v ) const
 {
  std::size_t h = 0;
  for( int x : v ) h = h * 1000003u + std::hash< int >()( x );
  return( h );
  }
 };

// against the classes by std::is_permutation on 300 random lists of short
// words, the groups and their words in order; the example, the empty
// list, repeated words, integers with their own hash
ADS_TEST( "anagrams" )
{
 using G = std::vector< std::vector< std::string > >;
 CHECK( anagramGroups( { "rosa" , "pippo" , "poppi" , "raso" , "orsa" ,
                         "lupo" } ) ==
        G( { { "rosa" , "raso" , "orsa" } , { "pippo" , "poppi" } ,
             { "lupo" } } ) );
 CHECK( anagramGroups( {} ).empty() );
 CHECK( anagramGroups( { "ab" , "ab" , "ba" } ) ==
        G( { { "ab" , "ab" , "ba" } } ) );
 CHECK( anagramGroups( { "" , "a" , "" } ) == G( { { "" , "" } , { "a" } } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::vector< std::string > W( g() % 20 );
  for( auto & w : W ) w = word( g , g() % 4 , 3 );
  G expected;
  for( const auto & w : W ) {
   auto it = std::find_if( expected.begin() , expected.end() ,
                           [ & ]( const std::vector< std::string > & e ) {
                            return( std::is_permutation( w.begin() , w.end() ,
                                                         e[ 0 ].begin() ,
                                                         e[ 0 ].end() ) ); } );
   if( it == expected.end() ) expected.push_back( { w } );
   else it->push_back( w );
   }
  CHECK( anagramGroups( W ) == expected );
  }
 const std::vector< std::vector< int > > V = { { 1 , 2 } , { 3 } ,
                                               { 2 , 1 } };
 const auto H = anagramGroups< std::vector< int > , VectorHash >( V );
 CHECK( ( H.size() == 2 ) && ( H[ 0 ].size() == 2 ) &&
        ( H[ 1 ].size() == 1 ) );
 }

/// the cost of the cuts in the given order on a string of length L
long long cutCost( const std::vector< long long > & order , long long L )
{
 std::set< long long > done = { 0 , L };
 long long c = 0;
 for( long long p : order ) {
  const auto hi = done.upper_bound( p );
  c += *hi - *std::prev( hi );
  done.insert( p );
  }
 return( c );
 }

// against every order of the cuts on 300 random instances of up to 6
// cuts, and against the cubic recursion without the speedup of Knuth on
// instances of up to 60; the example of three cuts (46, 37, 36), the edge
// cases, the order found and its cost; places out of order or out of range
ADS_TEST( "string-breaking" )
{
 using V = std::vector< long long >;
 CHECK( cutCost( { 3 , 8 , 10 } , 19 ) == 46 );
 CHECK( cutCost( { 10 , 8 , 3 } , 19 ) == 37 );
 CHECK( cutCost( { 10 , 3 , 8 } , 19 ) == 36 );
 CHECK( stringBreakingCost( { 3 , 8 , 10 } , 19 ) == 36 );
 CHECK( stringBreakingOrder( { 3 , 8 , 10 } , 19 ) ==
        std::vector< std::int64_t >( { 10 , 3 , 8 } ) );
 CHECK( stringBreakingCost( {} , 7 ) == 0 );
 CHECK( stringBreakingOrder( {} , 7 ).empty() );
 CHECK( stringBreakingCost( { 4 } , 7 ) == 7 );
 CHECK( stringBreakingCost( { 1 , 2 , 3 } , 4 ) == 8 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const long long L = 2 + g() % 30;
  std::set< long long > s;
  const std::size_t n = g() % 7;
  while( ( s.size() < n ) && ( s.size() + 1 < std::size_t( L ) ) )
   s.insert( 1 + g() % ( L - 1 ) );
  V P( s.begin() , s.end() );
  long long best = std::numeric_limits< long long >::max();
  do best = std::min( best , cutCost( P , L ) );
  while( std::next_permutation( P.begin() , P.end() ) );
  const V cuts( s.begin() , s.end() );
  CHECK( stringBreakingCost( cuts , L ) == best );
  CHECK( cutCost( stringBreakingOrder( cuts , L ) , L ) == best );
  }
 for( int t = 0 ; t < 40 ; ++t ) {
  const long long L = 70 + g() % 1000;
  std::set< long long > s;
  const std::size_t n = g() % 61;
  while( s.size() < n ) s.insert( 1 + g() % ( L - 1 ) );
  V w = { 0 };
  w.insert( w.end() , s.begin() , s.end() );
  w.push_back( L );
  const std::size_t m = w.size();
  std::vector< V > D( m , V( m , 0 ) );
  for( std::size_t d = 2 ; d < m ; ++d )
   for( std::size_t i = 0 ; i + d < m ; ++i ) {
    const std::size_t j = i + d;
    D[ i ][ j ] = std::numeric_limits< long long >::max();
    for( std::size_t k = i + 1 ; k < j ; ++k )
     D[ i ][ j ] = std::min( D[ i ][ j ] , D[ i ][ k ] + D[ k ][ j ] );
    D[ i ][ j ] += w[ j ] - w[ i ];
    }
  const V cuts( s.begin() , s.end() );
  CHECK( stringBreakingCost( cuts , L ) == D[ 0 ][ m - 1 ] );
  CHECK( cutCost( stringBreakingOrder( cuts , L ) , L ) == D[ 0 ][ m - 1 ] );
  }
 const std::vector< int > small = { 2 , 5 };
 CHECK( stringBreakingCost( small , 9 ) == 14 );
 CHECK_THROWS( stringBreakingCost( { 3 , 3 } , 7 ) , contract_violation );
 CHECK_THROWS( stringBreakingCost( { 7 } , 7 ) , contract_violation );
 CHECK_THROWS( stringBreakingCost( { 0 , 2 } , 7 ) , contract_violation );
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
/*------------------------ End File lib_strings.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
