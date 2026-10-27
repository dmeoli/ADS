/*--------------------------------------------------------------------------*/
/*------------------------ File contract_string.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the algorithms on strings: each
 * operation against its specification and the violations of its preconditions,
 * which must throw ADS::contract_violation. The tests are compiled at the
 * level 3, so that each operation also checks its postconditions and audits,
 * on random instances against a brute force or a second algorithm.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 13 and
 *      14, https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#include "ADS/string/Alignment.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/FrontCoding.h"
#include "ADS/string/Huffman.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Parentheses.h"
#include "ADS/string/PrefixSearch.h"
#include "ADS/string/Segmentations.h"
#include "ADS/string/StringBreaking.h"
#include "ADS/string/StringMatching.h"
#include "ADS/string/SuffixApplications.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixTree.h"
#include "ADS/string/WordBreak.h"

#include <cmath>
#include <cstdint>
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
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a random string of length up to n on the first k letters
std::string word( std::mt19937 & g , int n , int k )
{
 std::string s( g() % ( n + 1 ) , 'a' );
 for( auto & c : s ) c = char( 'a' + g() % k );
 return( s );
 }

// huffman(), encode() and decode(): the preconditions violated, and the
// code of random texts prefix-free and decoded back; one symbol (a code
// of one bit, the other bit refused), frequencies 0, the empty text
ADS_TEST( "huffman" )
{
 CHECK_THROWS( huffman( std::map< char , int >() ) , contract_violation );
 CHECK_THROWS( huffman( std::map< char , int >( { { 'a' , 1 } ,
                                                  { 'b' , -1 } } ) ) ,
               contract_violation );
 const auto code = huffman( std::map< char , int >( { { 'a' , 5 } ,
                                                      { 'b' , 2 } ,
                                                      { 'c' , 1 } } ) );
 CHECK_THROWS( encode( std::string( "abd" ) , code ) , contract_violation );
 CHECK_THROWS( decode( code.at( 'a' ) + code.at( 'b' ).substr( 0 , 1 ) ,
                       code ) , contract_violation );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::string s = word( g , 40 , 1 + int( g() % 8 ) );
  if( s.empty() ) continue;
  std::map< char , long > freq;
  for( char c : s ) ++freq[ c ];
  const auto C = huffman( freq );
  CHECK( decode( encode( s , C ) , C ) == s );
  }
 // one symbol (a code of one bit), a frequency 0 allowed, the empty text
 const auto one = huffman( std::map< char , int >( { { 'z' , 3 } } ) );
 CHECK( one.at( 'z' ).size() == 1 &&
        decode( encode( std::string( "zzz" ) , one ) , one ) == "zzz" );
 const auto zero = huffman( std::map< char , int >( { { 'a' , 0 } ,
                                                      { 'b' , 0 } } ) );
 CHECK( zero.size() == 2 && encode( std::string() , zero ).empty() &&
        decode( std::string() , zero ).empty() );
 const std::string other( 1 , one.at( 'z' ) == "0" ? '1' : '0' );
 CHECK_THROWS( decode( other , one ) , contract_violation );   // no code
 }

// occurrence() with a table of other strings refused; editDistance(),
// stringMatching() and kmp() within their bounds on random strings; j = n
// just inside, a table of another text refused; the empty strings, the
// pattern longer than the text and equal to it, all characters equal
ADS_TEST( "matching" )
{
 const std::string P = "ab" , T = "xaby";
 const auto DP = matchingTable( P , T );
 CHECK_THROWS( occurrence( DP , std::string( "abc" ) , T , 2 ) ,
               contract_violation );
 CHECK_THROWS( occurrence( DP , P , T , 5 ) , contract_violation );
 CHECK( occurrence( DP , P , T , 3 ) == 1 );
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string a = word( g , 8 , 3 ) , b = word( g , 12 , 3 );
  const int d = editDistance( a , b );
  CHECK( d == editDistance( b , a ) );
  CHECK( stringMatchingSingleRow( a , b ) == stringMatching( a , b ).second );
  CHECK( kmp( b , a ).size() == naiveMatching( b , a ).size() );
  }
 // the bounds of occurrence(): j = n just inside, a table of another text
 CHECK( occurrence( DP , P , T , 4 ) <= 4 );
 CHECK_THROWS( occurrence( DP , P , std::string( "xabyz" ) , 2 ) ,
               contract_violation );
 // the empty strings, the pattern longer than the text and equal to it,
 // all the characters equal
 const std::string E , A( 6 , 'a' );
 CHECK( editDistance( E , E ) == 0 && editDistance( E , T ) == 4 &&
        editDistance( T , E ) == 4 && editDistance( T , T ) == 0 );
 CHECK( kmp( T , T ).size() == 1 && kmp( P , T ).empty() &&
        kmp( E , P ).empty() && kmp( P , E ).size() <= 3 );
 CHECK( kmp( A , std::string( "aa" ) ).size() == 5 &&
        naiveMatching( A , std::string( "aa" ) ).size() == 5 );
 CHECK( stringMatching( T , T ).second == 0 &&
        stringMatching( std::string( "abc" ) , E ).second == 3 );
 CHECK( lcs( E , T ).empty() && lcs( A , std::string( "a" ) ) == "a" );
 }

// lcs() a common subsequence of the length of the table, and the longest
// common substring shared by both strings
ADS_TEST( "lcs" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string a = word( g , 10 , 3 ) , b = word( g , 10 , 3 );
  CHECK( int( lcs( a , b ).size() ) == lcsMemo( a , b ) );
  const auto [ best , ei , ej ] = longestCommonSubstring( a , b );
  CHECK( a.substr( ei - best , best ) == b.substr( ej - best , best ) );
  }
 }

// lcsMemo() on a string of 10^6 items against one of 1, 2 and 3, a path
// of the subproblems of about 10^6 levels, which the recursion of the
// book takes on the stack: the length, and the number of the subproblems
// solved against the ones that it reaches; then the empty strings
ADS_TEST( "lcs-long" )
{
 const std::string A( 1000000 , 'a' );
 for( const std::string b : { "b" , "ba" , "bab" } ) {
  long solved = 0;
  const int l = int( b.size() / 2 );
  CHECK( lcsMemo( A , b , &solved ) == l && lcsMemo( b , A ) == l );
  CHECK( solved >= long( A.size() ) && solved < long( 3 * A.size() ) );
  }
 const std::string E;
 long solved = 0;
 CHECK( lcsMemo( E , E , &solved ) == 0 && lcsMemo( A , E ) == 0 &&
        solved == 0 );
 }

// globalAlignment() of strings of equal length, and the words of
// wordBreak() joined back; the empty strings, a dictionary with no word
ADS_TEST( "alignment-wordbreak" )
{
 std::mt19937 g( 4 );
 const std::set< std::string > D = { "a" , "ab" , "ba" , "b" , "aab" };
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string a = word( g , 8 , 3 ) , b = word( g , 8 , 3 );
  const auto [ x , y ] = globalAlignment( a , b , '-' );
  CHECK( x.size() == y.size() );
  const std::string s = word( g , 10 , 2 );
  const auto W = wordBreak( s , [ & ]( const std::string & w ) {
   return( D.count( w ) > 0 ); } );
  CHECK( W.has_value() == ( s.find_first_not_of( "ab" ) == s.npos ) );
  }
 const std::string E;
 const auto [ ex , ey ] = globalAlignment( E , E , '-' );
 CHECK( ex.empty() && ey.empty() );
 const auto [ fx , fy ] = globalAlignment( E , std::string( "abc" ) , '-' );
 CHECK( fx == "---" && fy == "abc" );
 const auto none = []( const std::string & ) { return( false ); };
 CHECK( wordBreak( E , none ) && wordBreak( E , none )->empty() &&
        ! wordBreak( std::string( "a" ) , none ) );
 }

// the other headers of strings: each precondition just inside and just
// outside, i.e., front coding (a first pair with l > 0, an l beyond the
// previous string, a length of 2^30 - 1 and 2^30, a code cut short, b = 0,
// c = 0 and NaN, the indices and the anchors at n), parentheses with
// open = close, the sorted inputs of the prefix searches and the bins
// audited, an empty word and a digit out of 0 .. 9, cuts at 0, at the
// length and repeated, C = 0 and k at 0 and m + 1, copies from too far or
// of distance 0, symbols at K, m = 0, a wrong suffix array, the queries of
// the suffix array and of the suffix tree at n + 1; the empty text
ADS_TEST( "other-strings" )
{
 using F = std::vector< FrontPair >;
 CHECK( frontDecode( F{} ).empty() );
 CHECK( frontDecode( F{ { 0 , "ab" } , { 2 , "c" } } ) ==
        std::vector< std::string >( { "ab" , "abc" } ) );
 CHECK_THROWS( (void) frontDecode( F{ { 1 , "a" } } ) , contract_violation );
 CHECK_THROWS( (void) frontDecode( F{ { 0 , "ab" } , { 3 , "" } } ) ,
               contract_violation );
 std::string out;
 encodeLength( 0 , out );
 encodeLength( ( std::uint32_t( 1 ) << 30 ) - 1 , out );
 std::size_t pos = 0;
 CHECK( decodeLength( out , pos ) == 0 &&
        decodeLength( out , pos ) == ( std::uint32_t( 1 ) << 30 ) - 1 &&
        pos == out.size() );
 CHECK_THROWS( encodeLength( std::uint32_t( 1 ) << 30 , out ) ,
               contract_violation );
 CHECK_THROWS( (void) decodeLength( out , pos ) , contract_violation );
 std::size_t cut = 0;
 CHECK_THROWS( (void) decodeLength( std::string_view( out.data() + 1 , 2 ) ,
                                    cut ) , contract_violation );
 const std::vector< std::string > D = { "a" , "ab" , "abc" , "b" } ,
                                  U = { "b" , "a" } , N;
 CHECK_THROWS( (void) FrontCodedDictionary::bucketed( D , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) FrontCodedDictionary::bucketed( U , 2 ) ,
               contract_violation );
 CHECK_THROWS( (void) FrontCodedDictionary::localityPreserving( D , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) FrontCodedDictionary::localityPreserving(
                D , std::nan( "" ) ) , contract_violation );
 const auto B1 = FrontCodedDictionary::bucketed( D , 1 ) ,
            BN = FrontCodedDictionary::bucketed( D , 99 ) ,
            L3 = FrontCodedDictionary::localityPreserving( D , 3 ) ,
            E0 = FrontCodedDictionary::bucketed( N , 2 );
 CHECK( B1.access( 3 ) == "b" && BN.access( 3 ) == "b" &&
        L3.access( 0 ) == "a" && E0.size() == 0 );
 CHECK( B1.anchors() == 4 && BN.anchors() == 1 && E0.anchors() == 0 );
 CHECK_THROWS( (void) B1.access( 4 ) , contract_violation );
 CHECK_THROWS( (void) E0.access( 0 ) , contract_violation );
 CHECK_THROWS( (void) BN.decodeCost( 4 ) , contract_violation );
 CHECK( BN.decodeCost( 3 ) >= 1 );
 CHECK( BN.anchor( 0 ) == 0 && BN.anchorString( 0 ) == "a" );
 CHECK_THROWS( (void) BN.anchor( 1 ) , contract_violation );
 CHECK_THROWS( (void) BN.anchorString( 1 ) , contract_violation );
 CHECK( BN.rankInBucket( 0 , "ab" , false ) == 0 &&
        BN.rankInBucket( 1 , "ab" , false ) == 1 );
 CHECK_THROWS( (void) BN.rankInBucket( 2 , "ab" , false ) ,
               contract_violation );
 CHECK( E0.lexicographicPosition( "x" ) == 0 && E0.prefixEnd( "" ) == 0 );
 CHECK( B1.prefixEnd( "" ) == 4 && B1.lexicographicPosition( "" ) == 0 );

 CHECK_THROWS( (void) longestBalancedSubsequence( "()" , 'x' , 'x' ) ,
               contract_violation );
 CHECK_THROWS( (void) longestBalancedSubstring( "()" , '(' , '(' ) ,
               contract_violation );
 CHECK( longestBalancedSubsequence( "" ) == 0 &&
        longestBalancedSubstring( "" ).second == 0 &&
        longestBalancedSubsequence( ")(" ) == 0 &&
        longestBalancedSubstring( "(()" ).second == 2 );

 CHECK_THROWS( (void) lexicographicPosition( U , "a" ) , contract_violation );
 CHECK_THROWS( (void) prefixEnd( U , "a" ) , contract_violation );
 CHECK_THROWS( (void) prefixRange( U , "a" ) , contract_violation );
 CHECK( prefixRange( N , "a" ) == std::pair( std::size_t( 0 ) ,
                                             std::size_t( 0 ) ) );
 CHECK( prefixRange( D , "a" ) == std::pair( std::size_t( 0 ) ,
                                             std::size_t( 3 ) ) );
 using W = std::vector< std::uint64_t >;
 CHECK_THROWS( BinnedSearch( W{ 2 , 1 } ) , contract_violation );
 const BinnedSearch S( W{ 0 , 5 , ~std::uint64_t( 0 ) } ) , S0( W{} );
 CHECK( S.contains( ~std::uint64_t( 0 ) ) && S.contains( 0 ) &&
        ! S.contains( 1 ) && ! S0.contains( 0 ) && S0.position( 7 ) == 0 );
 (void) S.bin( S.bins() - 1 );
 CHECK_THROWS( (void) S.bin( S.bins() ) , contract_violation );
 CHECK_THROWS( BinnedStringSearch( U ) , contract_violation );
 const BinnedStringSearch BS( D );
 CHECK( BS.prefixEnd( "ab" ) == 3 && BS.lexicographicPosition( "zz" ) == 4 );

 CHECK_THROWS( (void) countSegmentations( std::string( "ab" ) ,
                                         { "a" , "" } ) ,
               contract_violation );
 CHECK( countSegmentations( std::string() , { "a" } ) == 1 &&
        countSegmentations( std::string( "aa" ) , { "a" , "aa" } ) == 2 &&
        countSegmentations( std::string( "b" ) , {} ) == 0 );
 CHECK_THROWS( (void) countDecodings( { 1 , 10 } ) , contract_violation );
 CHECK_THROWS( (void) countDecodings( { -1 } ) , contract_violation );
 CHECK( countDecodings( {} ) == 1 && countDecodings( { 0 } ) == 0 &&
        countDecodings( { 9 } ) == 1 && countDecodings( { 2 , 6 } ) == 2 &&
        countDecodings( { 2 , 7 } ) == 1 && countDecodings( { 1 , 0 } ) == 1 );

 using I = std::vector< std::int64_t >;
 CHECK( stringBreakingCost( I{} , 10 ) == 0 &&
        stringBreakingOrder( I{} , 10 ).empty() );
 CHECK( stringBreakingCost( I{ 1 , 9 } , 10 ) == 19 );     // just inside
 for( const I & bad : { I{ 0 } , I{ 10 } , I{ 3 , 3 } , I{ 5 , 2 } } ) {
  CHECK_THROWS( (void) stringBreakingCost( bad , 10 ) , contract_violation );
  CHECK_THROWS( (void) stringBreakingOrder( bad , 10 ) ,
                contract_violation );
  }

 const SuffixArray A( std::string_view( "banana" ) ) , A0( "" );
 CHECK_THROWS( (void) repeatedSubstring( A , 2 , 0 ) , contract_violation );
 CHECK( repeatedSubstring( A , 3 , 2 ) && ! repeatedSubstring( A , 3 , 3 ) &&
        repeatedSubstring( A , 0 , 7 ) && ! repeatedSubstring( A , 0 , 8 ) &&
        ! repeatedSubstring( A , 7 , 1 ) && repeatedSubstring( A , 6 , 1 ) );
 CHECK( ! repeatedSubstring( A0 , 1 , 1 ) && distinctSubstrings( A0 ) == 0 &&
        distinctSubstrings( A ) == 15 );
 const std::vector< std::string > M = { "xabc" , "abcy" , "zab" };
 CHECK_THROWS( (void) longestCommonSubstring( M , 0 ) , contract_violation );
 CHECK_THROWS( (void) longestCommonSubstring( M , 4 ) , contract_violation );
 CHECK( longestCommonSubstring( M , 3 ) == "ab" &&
        longestCommonSubstring( M , 1 ).size() == 4 &&
        longestCommonSubstring( M , 2 ) == "abc" );
 CHECK_THROWS( (void) longestCommonSubstring( std::vector< std::string >() ,
                                              1 ) , contract_violation );
 using Z = std::vector< CopyPhrase >;
 CHECK( decodeCopyPhrases( Z{} ).empty() );
 CHECK( decodeCopyPhrases( Z{ { 0 , 0 , 'a' } , { 1 , 3 , 'b' } } ) ==
        "aaaab" );                                  // overlapping copy
 CHECK_THROWS( (void) decodeCopyPhrases( Z{ { 1 , 1 , 'a' } } ) ,
               contract_violation );                // farther than decoded
 CHECK_THROWS( (void) decodeCopyPhrases( Z{ { 0 , 0 , 'a' } ,
                                             { 0 , 2 , 'b' } } ) ,
               contract_violation );                // a copy of distance 0
 CHECK( decodeCopyPhrases( lz77ByTree( "" ) ).empty() &&
        decodeCopyPhrases( lz77ByTree( "aaaa" ) ) == "aaaa" );

 const Symbols T = { 1 , 0 , 2 , 0 };
 CHECK( suffixArraySkew( T , 3 ) == suffixArraySais( T , 3 ) &&
        suffixArrayScan( T , 1 ) == suffixArraySais( T , 3 ) &&
        suffixArrayScan( T , 99 ) == suffixArrayDoubling( T ) );
 CHECK_THROWS( (void) suffixArraySkew( T , 2 ) , contract_violation );
 CHECK_THROWS( (void) suffixArraySais( T , 2 ) , contract_violation );
 CHECK_THROWS( (void) suffixArrayScan( T , 0 ) , contract_violation );
 CHECK_THROWS( SuffixArray( T , 2 ) , contract_violation );
 CHECK_THROWS( (void) lcpArray( T , std::vector< std::size_t >(
                                    { 0 , 1 , 2 , 3 } ) ) ,
               contract_violation );
 CHECK( suffixArraySais( Symbols{} , 1 ).empty() &&
        suffixArraySkew( Symbols{ 0 } , 1 ) ==
        std::vector< std::size_t >( { 0 } ) );
 CHECK( A.lce( 6 , 0 ) == 0 && A.lce( 1 , 3 ) == 3 && A.rank( 5 ) < 6 &&
        A[ 5 ] < 6 );
 CHECK_THROWS( (void) A.lce( 7 , 0 ) , contract_violation );
 CHECK_THROWS( (void) A.lce( 0 , 7 ) , contract_violation );
 CHECK_THROWS( (void) A.rank( 7 ) , contract_violation );
 CHECK_THROWS( (void) A[ 6 ] , contract_violation );
 CHECK_THROWS( (void) A0[ 0 ] , contract_violation );
 CHECK( A0.count( "" ) <= 1 && A0.lce( 0 , 0 ) == 0 );

 CHECK_THROWS( SuffixTree( T , 2 ) , contract_violation );
 const SuffixTree ST( std::string_view( "banana" ) ) , ST0( "" );
 const std::size_t n = ST.textSize();
 (void) ST.node( ST.size() - 1 );
 (void) ST.leaf( n );
 (void) ST.symbol( n );
 CHECK( ST.lce( n , 0 ) == 0 && ST.lce( 1 , 3 ) == 3 &&
        ST.lca( 0 , ST.size() - 1 ) == 0 );
 CHECK_THROWS( (void) ST.node( ST.size() ) , contract_violation );
 CHECK_THROWS( (void) ST.leaf( n + 1 ) , contract_violation );
 CHECK_THROWS( (void) ST.symbol( n + 1 ) , contract_violation );
 CHECK_THROWS( (void) ST.lca( ST.size() , 0 ) , contract_violation );
 CHECK_THROWS( (void) ST.lca( 0 , ST.size() ) , contract_violation );
 CHECK_THROWS( (void) ST.lce( n + 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) ST.lce( 0 , n + 1 ) , contract_violation );
 CHECK( ST0.textSize() == 0 && ST0.count( "a" ) == 0 );
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
/*---------------------- End File contract_string.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
