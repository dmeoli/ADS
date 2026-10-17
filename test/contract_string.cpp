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
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/string/Alignment.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/Huffman.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/StringMatching.h"
#include "ADS/string/WordBreak.h"

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
// code of random texts prefix-free and decoded back
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
 }

// occurrence() with a table of other strings refused; editDistance(),
// stringMatching() and kmp() within their bounds on random strings
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

// globalAlignment() of strings of equal length, and the words of
// wordBreak() joined back
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
