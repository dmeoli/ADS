/*--------------------------------------------------------------------------*/
/*---------------------- File pearls_compression.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the statistical codes, of the compressors of Lempel and Ziv and
 * of the transform of Burrows and Wheeler of chapters 12, 13 and 14 of the
 * notes: each one on the examples of the notes with their numbers, its
 * round trip decode( encode( x ) ) == x on random, repetitive, empty and
 * one-symbol texts, its lengths against the bounds of the entropy, and the
 * transform and the FM-index against the sorted rotations and a naive
 * search, at the level 3 of the contracts.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 12, 13 and 14
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

#include "ADS/compression/ArithmeticCoding.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/compression/CanonicalHuffman.h"
#include "ADS/compression/Entropy.h"
#include "ADS/compression/FMIndex.h"
#include "ADS/compression/LempelZiv.h"
#include "ADS/compression/PPM.h"
#include "ADS/string/Huffman.h"
#include "ADS/string/StringMatching.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <span>
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

/// n random symbols among the first sigma bytes from first
std::string randomText( std::mt19937 & g , std::size_t n , unsigned sigma ,
                        unsigned first = 'a' )
{
 std::string s( n , '\0' );
 for( auto & c : s ) c = char( ( first + g() % sigma ) & 0xFF );
 return( s );
 }

/// the texts of the round trips: empty, one symbol, all equal, random on
/// small and large alphabets (all the bytes too), repetitive
std::vector< std::string > texts( void )
{
 std::mt19937 g( 1 );
 std::vector< std::string > T = { "" , "a" , "b" , std::string( 300 , 'z' ) ,
                                  "abracadabra" , "mississippi" };
 for( unsigned sigma : { 2u , 4u , 26u } )
  for( std::size_t n : { 2u , 17u , 300u , 2000u } )
   T.push_back( randomText( g , n , sigma ) );
 T.push_back( randomText( g , 3000 , 256 , 0 ) );
 std::string all;
 for( int c = 0 ; c < 256 ; ++c ) all += char( c );
 T.push_back( all + all );
 std::string rep;
 for( int i = 0 ; i < 120 ; ++i ) rep += "mississippi ";
 T.push_back( rep );
 std::string block = randomText( g , 50 , 8 ) , mut;
 for( int i = 0 ; i < 40 ; ++i ) {
  mut += block;
  block[ g() % block.size() ] = char( 'a' + g() % 8 );
  }
 T.push_back( mut );
 std::string a = "a" , b = "ab";                 // a word of Fibonacci
 while( b.size() < 2000 ) {
  const std::string c = b + a;
  a = b;
  b = c;
  }
 T.push_back( b );
 return( T );
 }

/// a range of rows, and runs
using Range = std::pair< std::size_t , std::size_t >;
using Runs = std::vector< std::pair< std::size_t , char > >;

/// the frequencies of the symbols of s
std::map< char , std::uint64_t > frequencies( const std::string & s )
{
 std::map< char , std::uint64_t > f;
 for( char c : s ) ++f[ c ];
 return( f );
 }

/// the bytes of s as symbols of a text of type char
std::span< const char > chars( const std::string & s )
{
 return( std::span< const char >( s.data() , s.size() ) );
 }

/// the transform by sorting the rotations of s$ explicitly
BWT naiveBwt( const std::string & s )
{
 const std::size_t n1 = s.size() + 1;
 std::vector< int > t( n1 , 0 );                 // the $ is 0
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  t[ i ] = int( static_cast< unsigned char >( s[ i ] ) ) + 1;
 std::vector< std::size_t > rot( n1 );
 std::iota( rot.begin() , rot.end() , std::size_t( 0 ) );
 std::sort( rot.begin() , rot.end() , [ & ]( std::size_t a , std::size_t b ) {
  for( std::size_t k = 0 ; k < n1 ; ++k ) {
   const int x = t[ ( a + k ) % n1 ] , y = t[ ( b + k ) % n1 ];
   if( x != y ) return( x < y );
   }
  return( false ); } );
 BWT r{ std::string() , 0 };
 for( std::size_t i = 0 ; i < n1 ; ++i ) {
  const std::size_t j = ( rot[ i ] + n1 - 1 ) % n1;   // the last symbol
  if( j == s.size() ) r.primary = i;
  else r.last += s[ j ];
  }
 return( r );
 }

/// the number of the occurrences of P in T by std::search
std::size_t searchCount( const std::string & T , const std::string & P )
{
 std::size_t k = 0;
 for( auto it = T.begin() ; ; ++it ) {
  it = std::search( it , T.end() , P.begin() , P.end() );
  if( it == T.end() ) return( k );
  ++k;
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the entropies: the source of the notes with P[ a ] = 99 / 100 has
// H0 = 0.0808 (0.08056 in the notes, from rounded logarithms); H0( s ) is
// log2 sigma on a text of equal frequencies, 0 on one symbol; Hk( s ) does
// not increase with k; a frequency 0 among others, frequencies near the
// largest double, k at and past the length, and a frequency negative,
// all 0 or none
ADS_TEST( "entropy" )
{
 const double h = entropy( std::map< char , int >{ { 'a' , 99 } ,
                                                    { 'b' , 1 } } );
 CHECK( std::abs( h - 0.08079 ) < 1e-4 );
 CHECK( entropy( std::string( "" ) ) == 0 );
 CHECK( entropy( std::string( "aaaa" ) ) == 0 );
 CHECK( std::abs( entropy( std::string( "abcdabcd" ) ) - 2 ) < 1e-12 );
 CHECK( std::abs( entropy( std::string( "abcdabcd" ) , 1 ) ) < 1e-12 );
 CHECK_THROWS( entropy( std::map< char , int >{ { 'a' , 0 } } ) ,
               contract_violation );
 CHECK_THROWS( entropy( std::map< char , int >{ { 'a' , 2 } ,
                                               { 'b' , -1 } } ) ,
               contract_violation );
 CHECK_THROWS( entropy( std::map< char , int >() ) , contract_violation );
 CHECK( entropy( std::map< char , int >{ { 'a' , 3 } , { 'b' , 0 } } ) == 0 );
 CHECK( std::abs( entropy( std::map< int , double >{ { 1 , 1e300 } ,
                                                     { 2 , 1e300 } } ) -
                  1 ) < 1e-12 );         // no overflow of the total
 CHECK( entropy( std::string( "ab" ) , 1 ) == 0 &&       // k >= n: 0
        entropy( std::string( "ab" ) , 2 ) == 0 &&
        entropy( std::string( "abcab" ) , 9 ) == 0 );
 for( const auto & s : texts() ) {
  double prev = entropy( s );
  for( std::size_t k = 1 ; k <= 4 ; ++k ) {
   const double hk = entropy( s , k );
   CHECK( hk <= prev + 1e-9 );
   prev = hk;
   }
  }
 }

// the canonical codes of figures 12.5 and 12.9: the arrays fc, the codes,
// and the decoding of 01 in d by the two steps of figures 12.6 and 12.7;
// one symbol, a code of 63 bits, and the bounds of firstCode(), code(),
// encode() and of the lengths (0, 64, Kraft above 1, none)
ADS_TEST( "canonical-huffman-notes" )
{
 const CanonicalHuffman< char > A( std::map< char , int >{
  { 'a' , 4 } , { 'b' , 4 } , { 'c' , 3 } , { 'd' , 2 } , { 'e' , 2 } ,
  { 'f' , 2 } } );
 CHECK( A.maxLength() == 4 );
 CHECK( A.firstCode( 1 ) == 2 && A.firstCode( 2 ) == 1 &&
        A.firstCode( 3 ) == 1 && A.firstCode( 4 ) == 0 );
 CHECK( A.code( 'a' ) == "0000" && A.code( 'b' ) == "0001" &&
        A.code( 'c' ) == "001" && A.code( 'd' ) == "01" &&
        A.code( 'e' ) == "10" && A.code( 'f' ) == "11" );
 const std::vector< std::uint8_t > bits = { 0x40 };   // 01
 CHECK( A.decode( bits , 1 ) == std::vector< char >( { 'd' } ) );
 const CanonicalHuffman< char > B( std::map< char , int >{
  { 'a' , 4 } , { 'b' , 4 } , { 'c' , 3 } , { 'd' , 2 } , { 'e' , 2 } ,
  { 'f' , 3 } , { 'g' , 3 } } );
 CHECK( B.firstCode( 1 ) == 2 && B.firstCode( 2 ) == 2 &&
        B.firstCode( 3 ) == 1 && B.firstCode( 4 ) == 0 );
 CHECK( B.code( 'c' ) == "001" && B.code( 'f' ) == "010" &&
        B.code( 'g' ) == "011" && B.code( 'd' ) == "10" &&
        B.code( 'e' ) == "11" );
 // the lengths of figure 12.9 are those of Huffman on its probabilities
 const auto L = huffmanLengths( std::map< char , double >{
  { 'a' , 0.05 } , { 'b' , 0.05 } , { 'c' , 0.1 } , { 'd' , 0.2 } ,
  { 'e' , 0.3 } , { 'f' , 0.2 } , { 'g' , 0.1 } } );
 double avg = 0;
 for( const auto & [ c , l ] : L ) avg += l * std::map< char , double >{
  { 'a' , 0.05 } , { 'b' , 0.05 } , { 'c' , 0.1 } , { 'd' , 0.2 } ,
  { 'e' , 0.3 } , { 'f' , 0.2 } , { 'g' , 0.1 } }.at( c );
 CHECK( std::abs( avg - 2.6 ) < 1e-12 );
 CHECK_THROWS( CanonicalHuffman< char >( std::map< char , int >{
  { 'a' , 1 } , { 'b' , 1 } , { 'c' , 1 } } ) , contract_violation );
 CHECK_THROWS( CanonicalHuffman< char >( std::map< char , int >() ) ,
               contract_violation );
 const CanonicalHuffman< char > O( std::map< char , int >{ { 'z' , 1 } } );
 CHECK( O.code( 'z' ) == "0" && O.maxLength() == 1 );     // one symbol
 const std::vector< char > zz( 20 , 'z' );
 CHECK( O.decode( O.encode( zz ) , zz.size() ) == zz );
 const CanonicalHuffman< char > D( std::map< char , int >{ { 'a' , 63 } ,
                                                           { 'b' , 1 } } );
 CHECK( D.code( 'b' ) == "1" && D.code( 'a' ) == std::string( 63 , '0' ) );
 const std::vector< char > ab = { 'a' , 'b' , 'a' };      // the longest
 CHECK( D.decode( D.encode( ab ) , 3 ) == ab );
 CHECK_THROWS( A.firstCode( 0 ) , contract_violation );
 CHECK_THROWS( A.firstCode( 5 ) , contract_violation );
 CHECK_THROWS( A.code( 'z' ) , contract_violation );
 CHECK_THROWS( A.encode( std::vector< char >( { 'z' } ) ) ,
               contract_violation );
 CHECK_THROWS( CanonicalHuffman< char >( std::map< char , int >{
  { 'a' , 0 } } ) , contract_violation );
 CHECK_THROWS( CanonicalHuffman< char >( std::map< char , int >{
  { 'a' , 64 } } ) , contract_violation );
 }

// the canonical code of each text: the lengths of Huffman.h, a total as
// long as the code of Huffman.h, H0 <= L < H0 + 1 per symbol, and the
// round trip; also a code that is not complete
ADS_TEST( "canonical-huffman" )
{
 for( const auto & s : texts() ) {
  if( s.empty() ) continue;
  const auto f = frequencies( s );
  const auto H = CanonicalHuffman< char >::fromFrequencies( f );
  const auto code = H.encode( chars( s ) );
  const auto back = H.decode( code , s.size() );
  CHECK( std::string( back.begin() , back.end() ) == s );
  const auto ref = huffman( f );
  std::size_t bits = 0;
  for( const auto & [ c , k ] : f ) {
   CHECK( H.lengths().at( c ) == int( ref.at( c ).size() ) );
   bits += std::size_t( H.lengths().at( c ) ) * k;
   }
  CHECK( bits == encode( s , ref ).size() );
  CHECK( code.size() == ( bits + 7 ) / 8 );
  const double n = double( s.size() ) , h = entropy( s );
  CHECK( double( bits ) >= n * h - 1e-6 );
  CHECK( double( bits ) < n * ( h + 1 ) + 1e-6 );
  // the prefix-free property, checked on the strings
  CHECK( std::all_of( f.begin() , f.end() , [ & ]( const auto & x ) {
   return( std::all_of( f.begin() , f.end() , [ & ]( const auto & y ) {
    return( ( x.first == y.first ) ||
            ( H.code( y.first ).rfind( H.code( x.first ) , 0 ) != 0 ) );
    } ) ); } ) );
  }
 const CanonicalHuffman< int > I( std::map< int , int >{ { 7 , 2 } ,
                                                         { 9 , 3 } } );
 const std::vector< int > x = { 9 , 7 , 7 , 9 };
 CHECK( I.decode( I.encode( x ) , 4 ) == x );
 CHECK( I.code( 7 ) == "01" && I.code( 9 ) == "000" );
 CHECK_THROWS( I.decode( std::vector< std::uint8_t >{ 0xFF } , 1 ) ,
               contract_violation );
 }

// the codes of bounded length by iterative scaling: the frequencies of
// Fibonacci of section 12.1.2 give a code of Huffman as long as the
// alphabet; with the bound L, no length above L, the inequality of Kraft,
// the round trip, and an average no shorter than that of Huffman (equal
// when L does not cut); exactly 2^L symbols (all of length L) with skewed
// frequencies up to 2^49, one symbol, L = 1 and L = 63, and L below
// log2 of the symbols, L outside [ 1 , 63 ], no symbol, a frequency 0
ADS_TEST( "limited-lengths" )
{
 std::map< int , std::uint64_t > F;
 std::uint64_t a = 1 , b = 1;
 for( int i = 0 ; i < 30 ; ++i ) {       // 1 , 1 , 2 , 3 , 5 , 8 , ...
  F[ i ] = a;
  const std::uint64_t c = a + b;
  a = b;
  b = c;
  }
 int max = 0;
 for( const auto & e : huffmanLengths( F ) ) max = std::max( max , e.second );
 CHECK( max >= 20 );
 auto avg = [ & ]( const std::map< int , int > & len ) {
  double s = 0;
  for( const auto & [ x , l ] : len ) s += double( F.at( x ) ) * l;
  return( s );
  };
 const double opt = avg( huffmanLengths( F ) );
 std::mt19937 g( 1 );
 for( int L : { 5 , 6 , 8 , 12 , 20 , 40 } ) {
  const auto len = limitedLengths( F , L );
  double kraft = 0;
  int m = 0;
  for( const auto & e : len ) {
   kraft += std::ldexp( 1.0 , -e.second );
   m = std::max( m , e.second );
   }
  CHECK( m <= L && kraft <= 1 + 1e-12 );
  CHECK( avg( len ) >= opt - 1e-9 );
  if( L >= max ) CHECK( std::abs( avg( len ) - opt ) < 1e-9 );
  const CanonicalHuffman< int > H( len );
  std::vector< int > x( 500 );
  for( auto & v : x ) v = int( g() % 30 );
  CHECK( H.decode( H.encode( x ) , x.size() ) == x );
  }
 const auto L5 = limitedLengths( F , 5 );       // 30 symbols, 2^5 = 32
 CHECK( std::all_of( L5.begin() , L5.end() , []( const auto & e ) {
  return( e.second <= 5 ); } ) );
 std::map< int , std::uint64_t > T;              // 2^L symbols exactly
 for( int i = 0 ; i < 8 ; ++i ) T[ i ] = std::uint64_t( 1 ) << ( 7 * i );
 const auto L3 = limitedLengths( T , 3 );
 CHECK( std::all_of( L3.begin() , L3.end() , []( const auto & e ) {
  return( e.second == 3 ); } ) );
 CHECK( limitedLengths( std::map< char , int >{ { 'a' , 7 } } , 1 ) ==
        ( std::map< char , int >{ { 'a' , 1 } } ) );      // one symbol
 CHECK( limitedLengths( std::map< char , int >{ { 'a' , 1 } , { 'b' , 9 } } ,
                        1 ) ==
        ( std::map< char , int >{ { 'a' , 1 } , { 'b' , 1 } } ) );
 CHECK( limitedLengths( F , 63 ) == huffmanLengths( F ) );
 CHECK_THROWS( limitedLengths( F , 4 ) , contract_violation );
 CHECK_THROWS( limitedLengths( T , 2 ) , contract_violation );
 CHECK_THROWS( limitedLengths( F , 0 ) , contract_violation );
 CHECK_THROWS( limitedLengths( F , 64 ) , contract_violation );
 CHECK_THROWS( limitedLengths( std::map< int , int >() , 3 ) ,
               contract_violation );
 CHECK_THROWS( limitedLengths( std::map< int , int >{ { 1 , 0 } } , 3 ) ,
               contract_violation );
 }

// the exact arithmetic code of section 12.2: abac with P[ a ] = 1 / 2,
// P[ b ] = P[ c ] = 1 / 4 is [ 19 / 64 , 20 / 64 ) and 0100111, which
// decodes back; on random texts the code has at most 2 + n H0 bits, and
// the empty text is the interval [ 0 , 1 ); a symbol of probability 1, a
// probability 0, probabilities not adding up to 1, a symbol without one,
// and a code with a symbol other than 0 and 1
ADS_TEST( "arithmetic-exact" )
{
 const std::map< char , Rational > P = { { 'a' , Rational( 1 , 2 ) } ,
                                         { 'b' , Rational( 1 , 4 ) } ,
                                         { 'c' , Rational( 1 , 4 ) } };
 const std::string s = "abac";
 const auto [ l , w ] = arithmeticInterval( chars( s ) , P );
 CHECK( l == Rational( 19 , 64 ) && w == Rational( 1 , 64 ) );
 const std::string bits = arithmeticCodeExact( chars( s ) , P );
 CHECK( bits == "0100111" );
 const auto back = arithmeticDecodeExact( bits , 4 , P );
 CHECK( std::string( back.begin() , back.end() ) == s );
 const std::string e;
 CHECK( arithmeticInterval( chars( e ) , P ).second == Rational( 1 ) );
 CHECK( arithmeticDecodeExact( arithmeticCodeExact( chars( e ) , P ) , 0 ,
                               P ).empty() );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const std::string x = randomText( g , 1 + g() % 40 , 1 + g() % 4 );
  const auto f = frequencies( x );
  std::map< char , Rational > Q;
  for( const auto & [ c , k ] : f )
   Q[ c ] = Rational( static_cast< long long >( k ) ,
                     static_cast< long long >( x.size() ) );
  const std::string code = arithmeticCodeExact( chars( x ) , Q );
  const auto y = arithmeticDecodeExact( code , x.size() , Q );
  CHECK( std::string( y.begin() , y.end() ) == x );
  CHECK( double( code.size() ) <=
         2 + double( x.size() ) * entropy( x ) + 1e-9 );
  }
 const std::map< char , Rational > One = { { 'a' , Rational( 1 ) } };
 const std::string aaa( 9 , 'a' );               // a certain symbol
 CHECK( arithmeticInterval( chars( aaa ) , One ).second == Rational( 1 ) );
 const auto ya = arithmeticDecodeExact( arithmeticCodeExact( chars( aaa ) ,
                                                             One ) , 9 , One );
 CHECK( std::string( ya.begin() , ya.end() ) == aaa );
 CHECK_THROWS( arithmeticInterval( chars( s ) ,
               std::map< char , Rational >{ { 'a' , Rational( 1 , 2 ) } } ) ,
               contract_violation );
 CHECK_THROWS( arithmeticInterval( chars( s ) ,
               std::map< char , Rational >{ { 'a' , Rational( 1 ) } ,
                                            { 'b' , Rational( 0 ) } } ) ,
               contract_violation );
 CHECK_THROWS( arithmeticInterval( chars( std::string( "ad" ) ) , P ) ,
               contract_violation );
 CHECK_THROWS( arithmeticDecodeExact( std::string( "012" ) , 1 , P ) ,
               contract_violation );
 }

// the integer coder of section 12.2.6 with M = 16 on abac emits 010011 as
// in the notes, then the two bits of the end, and decodes back; it rejects
// a total above M / 4; the smallest coder (w = 2) on a certain symbol, the
// largest total 2^30 of w = 32 with a symbol of count 1, an empty or
// reversed range, w outside [ 2 , 32 ], a symbol without count, a count 0
ADS_TEST( "range-coder-notes" )
{
 const std::map< char , std::uint64_t > counts = { { 'a' , 2 } ,
                                                   { 'b' , 1 } ,
                                                   { 'c' , 1 } };
 const std::string s = "abac";
 RangeEncoder E( 4 );
 E.encode( 0 , 2 , 4 );
 E.encode( 2 , 3 , 4 );
 E.encode( 0 , 2 , 4 );
 CHECK( E.bits() == 4 );                 // 0100
 E.encode( 3 , 4 , 4 );
 CHECK( E.bits() == 6 );                 // 010011
 const auto code = E.finish();
 CHECK( code.size() == 1 && ( code[ 0 ] >> 2 ) == 0x13 );
 CHECK( arithmeticEncode( chars( s ) , counts , 4 ) == code );
 const auto back = arithmeticDecode( std::span< const std::uint8_t >( code ) ,
                                     4 , counts , 4 );
 CHECK( std::string( back.begin() , back.end() ) == s );
 RangeEncoder F( 4 );
 CHECK_THROWS( F.encode( 0 , 1 , 5 ) , contract_violation );
 CHECK_THROWS( F.encode( 1 , 1 , 4 ) , contract_violation );
 CHECK_THROWS( F.encode( 0 , 5 , 4 ) , contract_violation );
 CHECK_THROWS( RangeEncoder( 33 ) , contract_violation );
 CHECK_THROWS( RangeEncoder( 1 ) , contract_violation );
 const RangeDecoder G( std::span< const std::uint8_t >( code ) , 4 );
 CHECK_THROWS( G.target( 0 ) , contract_violation );
 CHECK_THROWS( G.target( 5 ) , contract_violation );
 const std::map< char , std::uint64_t > w2 = { { 'a' , 1 } };   // M = 4
 const auto c2 = arithmeticEncode( chars( std::string( 7 , 'a' ) ) , w2 , 2 );
 CHECK( arithmeticDecode( std::span< const std::uint8_t >( c2 ) , 7 , w2 ,
                          2 ) == std::vector< char >( 7 , 'a' ) );
 const std::map< char , std::uint64_t > big = { { 'a' , 1 } ,  // 2^30
  { 'b' , ( std::uint64_t( 1 ) << 30 ) - 1 } };
 const std::string sb = "abbbab";
 const auto cb = arithmeticEncode( chars( sb ) , big );
 const auto bb = arithmeticDecode( std::span< const std::uint8_t >( cb ) ,
                                   sb.size() , big );
 CHECK( std::string( bb.begin() , bb.end() ) == sb );
 CHECK_THROWS( arithmeticEncode( chars( s ) , counts , 3 ) ,
               contract_violation );            // total 4 above 2
 CHECK_THROWS( arithmeticEncode( chars( std::string( "ad" ) ) , counts , 4 ) ,
               contract_violation );
 CHECK_THROWS( arithmeticEncode( chars( s ) ,
               std::map< char , std::uint64_t >{ { 'a' , 2 } , { 'b' , 1 } ,
                                                 { 'c' , 0 } } , 4 ) ,
               contract_violation );
 CHECK_THROWS( arithmeticEncode( chars( s ) , big ,
               31 ) , contract_violation );     // 2^30 above 2^29
 }

// the static integer arithmetic code: the round trip on every text with the
// counts of the text, and at most n H0 + 2 bits plus 10^-3 per symbol and
// the bits of the end, against the exact bound
ADS_TEST( "arithmetic-static" )
{
 for( const auto & s : texts() ) {
  if( s.empty() ) continue;
  const auto f = frequencies( s );
  const auto code = arithmeticEncode( chars( s ) , f );
  const auto back = arithmeticDecode( std::span< const std::uint8_t >( code ) ,
                                      s.size() , f );
  CHECK( std::string( back.begin() , back.end() ) == s );
  const double n = double( s.size() );
  CHECK( double( 8 * code.size() ) <= n * entropy( s ) + 1e-3 * n + 2 + 10 );
  }
 const std::string e;
 const auto f = std::map< char , std::uint64_t >{ { 'x' , 3 } };
 const auto code = arithmeticEncode( chars( e ) , f );
 CHECK( arithmeticDecode( std::span< const std::uint8_t >( code ) , 0 ,
                          f ).empty() );
 }

// the adaptive arithmetic code: the round trip on every text; the
// estimate ( l_s + 1 ) / ( l + 256 ) costs at most about 256 log2 n bits
// more than n H0; a skewed text goes well below one bit per symbol, which
// no prefix code can do; an alphabet of one symbol (a few bytes for 500),
// the largest alphabet 2^20 with its last symbol, no symbol, a symbol
// past sigma, and sigma 0 or above 2^20
ADS_TEST( "arithmetic-adaptive" )
{
 for( const auto & s : texts() ) {
  const auto code = adaptiveArithmeticEncode( s );
  CHECK( adaptiveArithmeticDecode( code , s.size() ) == s );
  const double n = double( s.size() );
  CHECK( double( 8 * code.size() ) <=
         n * entropy( s ) + 256 * std::log2( n + 256 ) + 64 );
  }
 std::mt19937 g( 1 );
 std::string skew( 50000 , 'a' );
 for( auto & c : skew ) if( g() % 100 == 0 ) c = 'b';
 const auto code = adaptiveArithmeticEncode( skew );
 CHECK( double( 8 * code.size() ) < 0.15 * double( skew.size() ) );
 CHECK( adaptiveArithmeticDecode( code , skew.size() ) == skew );
 std::vector< std::uint32_t > x( 3000 );
 for( auto & v : x ) v = g() % 1000;
 const auto cx = adaptiveArithmeticEncode( std::span< const std::uint32_t >(
                                           x ) , 1000 );
 CHECK( adaptiveArithmeticDecode( cx , x.size() , 1000 ) == x );
 using Syms = std::vector< std::uint32_t >;
 const Syms z( 500 , 0 );                        // one symbol
 const auto cz = adaptiveArithmeticEncode( std::span< const std::uint32_t >(
                                           z ) , 1 );
 CHECK( adaptiveArithmeticDecode( cz , z.size() , 1 ) == z && cz.size() <= 8 );
 const std::uint32_t S = std::uint32_t( 1 ) << 20;   // the largest alphabet
 const Syms e = { 0 , S - 1 , S - 1 , 12345 , 0 };
 const auto ce = adaptiveArithmeticEncode( std::span< const std::uint32_t >(
                                           e ) , S );
 CHECK( adaptiveArithmeticDecode( ce , e.size() , S ) == e );
 const Syms none;
 CHECK( adaptiveArithmeticDecode( adaptiveArithmeticEncode(
         std::span< const std::uint32_t >( none ) , 7 ) , 0 , 7 ).empty() );
 CHECK_THROWS( adaptiveArithmeticEncode( std::span< const std::uint32_t >(
               x ) , 999 ) , contract_violation );
 CHECK_THROWS( adaptiveArithmeticEncode( std::span< const std::uint32_t >(
               none ) , 0 ) , contract_violation );
 CHECK_THROWS( adaptiveArithmeticEncode( std::span< const std::uint32_t >(
               none ) , S + 1 ) , contract_violation );
 CHECK_THROWS( adaptiveArithmeticDecode( cz , 1 , 0 ) , contract_violation );
 CHECK_THROWS( adaptiveArithmeticDecode( cz , 1 , S + 1 ) ,
               contract_violation );
 }

// the models of table 12.1 after abracadabra with K = 2 (a b c d r e as
// 0 1 2 3 4 5), and the probabilities of section 12.3.2 with exclusion: c
// costs 1 / 2 in ra; d an escape of 1 / 2 and 1 / 6 in a; the novel e
// escapes to the order -1 with 1 / ( 6 - 5 )
ADS_TEST( "ppm-notes" )
{
 const std::map< char , std::uint32_t > id = { { 'a' , 0 } , { 'b' , 1 } ,
  { 'c' , 2 } , { 'd' , 3 } , { 'r' , 4 } , { 'e' , 5 } };
 PPMModel M( 2 , 6 );
 CHECK( M.steps( 0 ) == std::vector< PPMModel::Step >( { { 0 , 1 , 6 } } ) );
 M.update( 0 );
 // b: an escape from the order 0, then 1 / 5 with a excluded
 CHECK( M.steps( 1 ) == std::vector< PPMModel::Step >( { { 1 , 2 , 2 } ,
                                                         { 0 , 1 , 5 } } ) );
 for( char c : std::string( "bracadabra" ) ) M.update( id.at( c ) );
 using Counts = std::map< std::uint32_t , std::uint64_t >;
 CHECK( M.counts( {} ) == Counts( { { 0 , 5 } , { 1 , 2 } , { 2 , 1 } ,
                                    { 3 , 1 } , { 4 , 2 } } ) );
 CHECK( M.counts( { 0 } ) == Counts( { { 1 , 2 } , { 2 , 1 } ,
                                       { 3 , 1 } } ) );
 CHECK( M.counts( { 1 } ) == Counts( { { 4 , 2 } } ) );
 CHECK( M.counts( { 4 } ) == Counts( { { 0 , 2 } } ) );
 CHECK( M.counts( { 0 , 1 } ) == Counts( { { 4 , 2 } } ) );
 CHECK( M.counts( { 1 , 4 } ) == Counts( { { 0 , 2 } } ) );
 CHECK( M.counts( { 4 , 0 } ) == Counts( { { 2 , 1 } } ) );
 CHECK( M.counts( { 3 , 0 } ) == Counts( { { 1 , 1 } } ) );
 CHECK( M.counts( { 2 , 2 } ).empty() );
 CHECK( M.steps( 2 ) == std::vector< PPMModel::Step >( { { 0 , 1 , 2 } } ) );
 CHECK( M.steps( 3 ) == std::vector< PPMModel::Step >( { { 1 , 2 , 2 } ,
                                                         { 2 , 3 , 6 } } ) );
 CHECK( M.steps( 5 ) == std::vector< PPMModel::Step >( { { 1 , 2 , 2 } ,
                                                         { 3 , 6 , 6 } ,
                                                         { 7 , 12 , 12 } ,
                                                         { 0 , 1 , 1 } } ) );
 PPMModel U( 3 , 1 );                            // one symbol
 CHECK( U.steps( 0 ) == std::vector< PPMModel::Step >( { { 0 , 1 , 1 } } ) );
 U.update( 0 );
 CHECK( U.steps( 0 ).front().low == 0 );
 CHECK_THROWS( M.steps( 6 ) , contract_violation );
 CHECK_THROWS( M.update( 6 ) , contract_violation );
 CHECK_THROWS( PPMModel( -1 ) , contract_violation );
 CHECK_THROWS( PPMModel( 2 , 0 ) , contract_violation );
 CHECK_THROWS( PPMModel( 2 , ( std::uint32_t( 1 ) << 16 ) + 1 ) ,
               contract_violation );
 }

// PPM of order 0 to 5: the round trip on every text; on a text of a
// source of order 2 the orders 2 and 3 beat the order 0 and approach the
// entropy of order 2
ADS_TEST( "ppm" )
{
 for( int K : { 0 , 1 , 2 , 3 , 5 } )
  for( const auto & s : texts() )
   CHECK( ppmDecode( ppmEncode( s , K ) , s.size() , K ) == s );
 std::mt19937 g( 1 );
 std::string s = "ab";                     // the next symbol of each pair
 std::map< std::string , char > next;
 for( char x = 'a' ; x < 'a' + 6 ; ++x )
  for( char y = 'a' ; y < 'a' + 6 ; ++y )
   next[ std::string( { x , y } ) ] = char( 'a' + g() % 6 );
 while( s.size() < 20000 ) {
  const char c = g() % 10 ? next[ s.substr( s.size() - 2 ) ]
                          : char( 'a' + g() % 6 );
  s += c;
  }
 const double n = double( s.size() );
 const double b0 = 8.0 * double( ppmEncode( s , 0 ).size() ) ,
              b2 = 8.0 * double( ppmEncode( s , 2 ).size() ) ,
              b3 = 8.0 * double( ppmEncode( s , 3 ).size() );
 CHECK( b2 < 0.6 * b0 && b3 < 0.6 * b0 );
 CHECK( b2 < n * entropy( s , 2 ) * 1.1 + 2000 );
 CHECK( b0 < n * entropy( s ) * 1.05 + 4000 );
 }

// the parses of section 13.1 of aabbabab: LZ77 gives <0,0,a> <1,1,b>
// <1,1,a> <2,3,EOF>, the last copy overlapping itself, and LZSS <0,a>
// <1,1> <0,b> <1,1> <3,2> <2,2>; a window of 1 finds only the copies of the
// previous symbol; the empty text, one symbol, copies shorter than
// minLength, a run copied from a window of 1, a copy from before the text,
// and the bounds of w, minLength and maxChain
ADS_TEST( "lz77-notes" )
{
 const std::string s = "aabbabab";
 const std::vector< LZ77Phrase > P = { { 0 , 0 , 'a' } , { 1 , 1 , 'b' } ,
                                       { 1 , 1 , 'a' } ,
                                       { 2 , 3 , std::nullopt } };
 CHECK( lz77Parse( s ) == P );
 CHECK( lz77Decode( P ) == s );
 const std::vector< LZSSPhrase > Q = { { 0 , 1 , 'a' } , { 1 , 1 , '\0' } ,
                                       { 0 , 1 , 'b' } , { 1 , 1 , '\0' } ,
                                       { 3 , 2 , '\0' } , { 2 , 2 , '\0' } };
 CHECK( lzssParse( s ) == Q );
 CHECK( lzssDecode( Q ) == s );
 for( const auto & f : lz77Parse( s , 1 ) ) CHECK( f.distance <= 1 );
 CHECK( lz77Decode( lz77Parse( s , 1 ) ) == s );
 CHECK( lz77Parse( std::string( 10 , 'x' ) ) ==
        std::vector< LZ77Phrase >( { { 0 , 0 , 'x' } ,
                                     { 1 , 9 , std::nullopt } } ) );
 CHECK( lz77Parse( "" ).empty() && lzssParse( "" ).empty() &&
        lzssParseHashed( "" , 8 ).empty() && lz77Decode( {} ).empty() &&
        lzssDecode( {} ).empty() );
 CHECK( lz77Parse( "q" ) == std::vector< LZ77Phrase >( { { 0 , 0 , 'q' } } ) );
 CHECK( lzssParse( "ab" , 1 , 100 ) ==          // minLength past the text
        std::vector< LZSSPhrase >( { { 0 , 1 , 'a' } , { 0 , 1 , 'b' } } ) );
 const std::string z( 100 , 'z' );               // a window of 1, a run
 CHECK( lzssParse( z , 1 ) == std::vector< LZSSPhrase >( { { 0 , 1 , 'z' } ,
                                                    { 1 , 99 , '\0' } } ) );
 CHECK( lzssDecode( lzssParseHashed( z , 1 , 3 , 1 ) ) == z );
 CHECK_THROWS( lz77Decode( { { 2 , 1 , 'a' } } ) , contract_violation );
 CHECK_THROWS( lzssDecode( { { 1 , 1 , '\0' } } ) , contract_violation );
 CHECK_THROWS( lz77Parse( s , 0 ) , contract_violation );
 CHECK_THROWS( lzssParse( s , 0 ) , contract_violation );
 CHECK_THROWS( lzssParse( s , 4 , 0 ) , contract_violation );
 CHECK_THROWS( lzssParseHashed( s , 0 ) , contract_violation );
 CHECK_THROWS( lzssParseHashed( s , 4 , 2 ) , contract_violation );
 CHECK_THROWS( lzssParseHashed( s , 4 , 3 , 0 ) , contract_violation );
 }

// LZ77 and LZSS with windows of 1 to unbounded: the round trip, the copies
// within the window, and no copy that could be longer; the parse by the
// chains of gzip with no bound equals the search of the whole window, and
// with a bound it still decodes
ADS_TEST( "lz77" )
{
 for( const auto & s : texts() ) {
  if( s.size() > 2100 ) continue;
  for( std::size_t w : { std::size_t( 1 ) , std::size_t( 4 ) ,
                         std::size_t( 64 ) , std::size_t( 100000 ) } ) {
   const auto P = lz77Parse( s , w );
   CHECK( lz77Decode( P ) == s );
   std::size_t p = 0;
   for( const auto & f : P ) {
    CHECK( f.distance <= w );
    if( f.next ) {                         // no longer copy in the window
     const std::size_t lo = p - std::min( p , w );
     for( std::size_t j = lo ; j < p ; ++j )
      CHECK( s.compare( j , f.length + 1 , s , p , f.length + 1 ) != 0 );
     }
    p += f.length + 1;
    }
   for( std::size_t m : { 1u , 3u , 5u } ) {
    const auto Q = lzssParse( s , w , m );
    CHECK( lzssDecode( Q ) == s );
    if( m >= 3 ) {
     CHECK( lzssParseHashed( s , w , m ) == Q );
     CHECK( lzssDecode( lzssParseHashed( s , w , m , 2 ) ) == s );
     }
    }
   }
  }
 std::string rep;
 for( int i = 0 ; i < 500 ; ++i ) rep += "abcdefgh";
 CHECK( lzssParseHashed( rep , 32768 ).size() == 9 );
 }

// the parse of LZ78 of table 13.1 and its dictionary; a text that ends
// inside a phrase; a dictionary that restarts every 3 phrases; the round
// trip on every text; the empty text, a dictionary of one phrase (every
// phrase a symbol), a phrase not or no longer there, and maxPhrases 0
ADS_TEST( "lz78" )
{
 const std::string s = "aabbababbbaababaa";
 const std::vector< LZ78Phrase > P = { { 0 , 'a' } , { 1 , 'b' } ,
  { 0 , 'b' } , { 2 , 'a' } , { 3 , 'b' } , { 3 , 'a' } , { 4 , 'b' } ,
  { 1 , 'a' } };
 CHECK( lz78Parse( s ) == P );
 CHECK( lz78Decode( P ) == s );
 CHECK( lz78Parse( "abab" ) == std::vector< LZ78Phrase >( { { 0 , 'a' } ,
  { 0 , 'b' } , { 1 , 'b' } } ) );
 CHECK( lz78Parse( "aaa" ) == std::vector< LZ78Phrase >( { { 0 , 'a' } ,
  { 1 , 'a' } } ) );
 CHECK( lz78Parse( "aba" ) == std::vector< LZ78Phrase >( { { 0 , 'a' } ,
  { 0 , 'b' } , { 0 , 'a' } } ) );
 const auto R = lz78Parse( s , 3 );
 CHECK( R[ 3 ] == ( LZ78Phrase{ 0 , 'a' } ) );   // after the restart
 CHECK( lz78Decode( R , 3 ) == s );
 for( const auto & t : texts() )
  for( std::size_t m : { std::size_t( 1 ) , std::size_t( 2 ) ,
                         std::size_t( 50 ) ,
                         std::numeric_limits< std::size_t >::max() } )
   CHECK( lz78Decode( lz78Parse( t , m ) , m ) == t );
 CHECK( lz78Parse( "" ).empty() && lz78Decode( {} ).empty() );
 CHECK( lz78Parse( "aaaa" , 1 ) == std::vector< LZ78Phrase >( {
  { 0 , 'a' } , { 0 , 'a' } , { 0 , 'a' } , { 0 , 'a' } } ) );   // m = 1
 CHECK_THROWS( lz78Decode( { { 1 , 'a' } } ) , contract_violation );
 CHECK_THROWS( lz78Decode( { { 0 , 'a' } , { 1 , 'b' } } , 1 ) ,
               contract_violation );            // phrase 1 forgotten
 CHECK_THROWS( lz78Parse( s , 0 ) , contract_violation );
 CHECK_THROWS( lz78Decode( P , 0 ) , contract_violation );
 }

// the code of LZW of table 13.2, 97 97 98 98 257 260, whose 260 is the
// phrase not yet built when the decoder reads it (table 13.3); the round
// trip on every text, the bytes above 127 included; the empty text, the
// byte 255, the late phrase of the byte 0, and an identifier past the one
// being built or a first one that is not a byte
ADS_TEST( "lzw" )
{
 const std::string s = "aabbababa";
 const std::vector< std::uint32_t > I = { 97 , 97 , 98 , 98 , 257 , 260 };
 CHECK( lzwEncode( s ) == I );
 CHECK( lzwDecode( I ) == s );
 CHECK( lzwEncode( "aaaa" ) == std::vector< std::uint32_t >( { 97 , 256 ,
                                                               97 } ) );
 for( const auto & t : texts() ) CHECK( lzwDecode( lzwEncode( t ) ) == t );
 std::string rep( 10000 , 'q' );
 CHECK( lzwEncode( rep ).size() < 150 );
 CHECK( lzwEncode( "" ).empty() && lzwDecode( {} ).empty() );
 CHECK( lzwEncode( std::string( 1 , '\xff' ) ) ==
        std::vector< std::uint32_t >( { 255 } ) );
 CHECK( lzwDecode( { 0 , 256 } ) == std::string( 3 , '\0' ) );
 CHECK_THROWS( lzwDecode( { 97 , 300 } ) , contract_violation );
 CHECK_THROWS( lzwDecode( { 97 , 258 } ) , contract_violation );
 CHECK_THROWS( lzwDecode( { 256 } ) , contract_violation );
 }

// the transform of figures 14.1 and 14.7: abracadabra gives ardrcaaaabb
// with r = 3, LF[ 0 ] = 1 and LF[ 1 ] = 10 as in section 14.1.2; the
// mississippi of section 14.3 three times gives r = 15 and the L of the
// notes; the move-to-front of bananacocco of figure 14.2 is 11311134101,
// and that of that L on the list imps the string of the notes; the runs of
// section 14.2.2; the run 00000 is 10 in the code of Wheeler, and the RLE0
// of that string follows from its move-to-front (the notes misprint the
// 1 and the 2 of it, grown by 1, as 4); the empty transform, the shortest
// one, the empty inputs of every step, the largest place 255, a primary
// row out of range, a string that is no transform, places past the list,
// a list with a repeated symbol, a run of 0 and an RLE0 integer past 256
ADS_TEST( "bwt-notes" )
{
 const BWT b = bwt( "abracadabra" );
 CHECK( b.last == "ardrcaaaabb" && b.primary == 3 );
 const auto LF = lfMapping( b );
 CHECK( LF[ 0 ] == 1 && LF[ 1 ] == 10 && LF[ 10 ] == 6 && LF[ 3 ] == 0 );
 CHECK( inverseBwt( b ) == "abracadabra" );
 const BWT m = bwt( "mississippimississippimississippi" );
 CHECK( m.primary == 15 );
 CHECK( m.last == "ipppssssssmmmiipppiiissssssiiiiii" );
 const auto x = moveToFront( "bananacocco" , "abcno" );
 CHECK( x == std::vector< std::uint8_t >( { 1 , 1 , 3 , 1 , 1 , 1 , 3 , 4 ,
                                            1 , 0 , 1 } ) );
 CHECK( inverseMoveToFront( x , "abcno" ) == "bananacocco" );
 const auto y = moveToFront( m.last , "imps" );
 const std::string ys = "020030000030030300100300000100000";
 CHECK( y.size() == ys.size() );
 for( std::size_t i = 0 ; i < ys.size() ; ++i )
  CHECK( y[ i ] == std::uint8_t( ys[ i ] - '0' ) );
 const auto R = runLengthEncode( "WWWWWWWWWWWBWWWWWWWWWWWWBBBBBWWWWWW" );
 CHECK( R == Runs( { { 11 , 'W' } , { 1 , 'B' } , { 12 , 'W' } ,
                     { 5 , 'B' } , { 6 , 'W' } } ) );
 CHECK( zeroRunEncode( { 0 , 0 , 0 , 0 , 0 , 3 } ) ==
        std::vector< std::uint32_t >( { 1 , 0 , 4 } ) );
 CHECK( zeroRunEncode( { 0 , 2 , 0 , 0 } ) ==
        std::vector< std::uint32_t >( { 0 , 3 , 1 } ) );
 CHECK( zeroRunEncode( y ) == std::vector< std::uint32_t >( {
  0 , 3 , 1 , 4 , 1 , 0 , 4 , 1 , 4 , 0 , 4 , 1 , 2 , 1 , 4 , 1 , 0 , 2 ,
  1 , 0 } ) );
 CHECK( inverseBwt( BWT{ "" , 0 } ).empty() && lfMapping( BWT{ "" , 0 } ) ==
        std::vector< std::size_t >( { 0 } ) );
 CHECK( inverseBwt( BWT{ "ba" , 1 } ) == "ab" );
 CHECK( moveToFront( "" , "ab" ).empty() && inverseMoveToFront( {} , "" )
        .empty() && runLengthEncode( "" ).empty() &&
        zeroRunEncode( {} ).empty() && zeroRunDecode( {} ).empty() );
 CHECK( zeroRunDecode( { 256 } ) == std::vector< std::uint8_t >( { 255 } ) );
 CHECK_THROWS( inverseBwt( BWT{ "ab" , 0 } ) , contract_violation );
 CHECK_THROWS( inverseBwt( BWT{ "ab" , 3 } ) , contract_violation );
 CHECK_THROWS( inverseBwt( BWT{ "" , 1 } ) , contract_violation );
 CHECK_THROWS( inverseBwt( BWT{ "ab" , 1 } ) , contract_violation );   // no s
 CHECK_THROWS( moveToFront( "abc" , "ab" ) , contract_violation );
 CHECK_THROWS( moveToFront( "a" , "aa" ) , contract_violation );
 CHECK_THROWS( inverseMoveToFront( { 0 , 2 } , "ab" ) , contract_violation );
 CHECK_THROWS( inverseMoveToFront( { 0 } , "aa" ) , contract_violation );
 CHECK_THROWS( runLengthDecode( { { 2 , 'a' } , { 0 , 'b' } } ) ,
               contract_violation );
 CHECK_THROWS( zeroRunDecode( { 257 } ) , contract_violation );
 }

// the transform against the sorted rotations of s$ on random texts of
// 0 to 120 symbols on 1 to 4 symbols and on all the bytes ('\0' included),
// and its inverse; on every text the inverse, and the first column F,
// sorted, mapped by LF to the last
ADS_TEST( "bwt" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const std::string s = t % 5 ? randomText( g , g() % 121 , 1 + g() % 4 )
                              : randomText( g , g() % 121 , 256 , 0 );
  CHECK( bwt( s ) == naiveBwt( s ) );
  }
 for( const auto & s : texts() ) {
  const BWT b = bwt( s );
  CHECK( inverseBwt( b ) == s );
  const auto LF = lfMapping( b );
  std::vector< std::size_t > sorted( LF );
  std::sort( sorted.begin() , sorted.end() );
  for( std::size_t i = 0 ; i < sorted.size() ; ++i )
   CHECK( sorted[ i ] == i );                // a permutation
  }
 CHECK( bwt( "" ) == ( BWT{ "" , 0 } ) );
 CHECK( bwt( "z" ) == ( BWT{ "z" , 1 } ) );
 }

// the move-to-front, the runs and RLE0 on every text: the round trips; the
// move-to-front of the transform of a repetitive text is mostly 0, and the
// bound 2 H0 + 1 of Theorem 14.5 on the gamma-code of the places
ADS_TEST( "mtf-rle" )
{
 for( const auto & s : texts() ) {
  const auto x = moveToFront( s );
  CHECK( inverseMoveToFront( x ) == s );
  CHECK( runLengthDecode( runLengthEncode( s ) ) == s );
  CHECK( zeroRunDecode( zeroRunEncode( x ) ) == x );
  if( s.empty() ) continue;
  double gamma = 0;                        // gamma( i + 1 ) bits for i
  for( std::uint8_t i : x )
   gamma += 2 * std::floor( std::log2( double( i ) + 1 ) ) + 1;
  // the first occurrences of the symbols cost the places in the list
  const double first = 256 * ( 2 * 8 + 1 );
  CHECK( gamma <= double( s.size() ) * ( 2 * entropy( s ) + 1 ) + first );
  }
 std::string rep;
 for( int i = 0 ; i < 200 ; ++i ) rep += "abracadabra ";
 const auto x = moveToFront( bwt( rep ).last );
 const auto zeros = std::count( x.begin() , x.end() , std::uint8_t( 0 ) );
 CHECK( double( zeros ) > 0.9 * double( x.size() ) );
 CHECK( zeroRunEncode( x ).size() < x.size() / 10 );
 }

// the bzip of every text with blocks of 1, 7, 100 and 900000 symbols: the
// round trip; a repetitive text takes much less than n H0 bits, the
// entropy of order 0 that a code of Huffman of its symbols would reach; the
// empty text, a long run of the byte 0 in several blocks, and blocks of 0
ADS_TEST( "bzip" )
{
 for( const auto & s : texts() )
  for( std::size_t b : { std::size_t( 1 ) , std::size_t( 7 ) ,
                         std::size_t( 100 ) , std::size_t( 900000 ) } ) {
   if( ( b == 1 ) && ( s.size() > 300 ) ) continue;
   CHECK( bzipDecompress( bzipCompress( s , b ) ) == s );
   }
 std::string rep;
 for( int i = 0 ; i < 1000 ; ++i ) rep += "mississippi";
 const auto code = bzipCompress( rep );
 CHECK( double( 8 * code.size() ) <
        0.1 * double( rep.size() ) * entropy( rep ) );
 CHECK( bzipDecompress( bzipCompress( "" ) ).empty() );
 const std::string zero( 5000 , '\0' );          // a run of the byte 0
 CHECK( bzipDecompress( bzipCompress( zero , 1000 ) ) == zero );
 CHECK_THROWS( bzipCompress( "abc" , 0 ) , contract_violation );
 }

// the FM-index of figure 14.7: ab prefixes the rows 2 and 3, found by the
// backward search through the rows [ 6 , 8 ) of b and the ranks
// Rank( a , 5 ) = 1 and Rank( a , 7 ) = 3 of the notes (here of the
// prefixes L[ 0 , 6 ) and L[ 0 , 8 )); it occurs at 0 and 7; the ranks at
// 0 and at n + 1, an absent symbol, the empty extract at the end, a text
// of one symbol and a run with overlapping occurrences, the empty pattern,
// the bounds of extract() and rank(), and mu = 0
ADS_TEST( "fm-index-notes" )
{
 const FMIndex I( "abracadabra" , 4 , 3 );
 CHECK( I.size() == 11 );
 CHECK( I.transform() == ( BWT{ "ardrcaaaabb" , 3 } ) );
 CHECK( I.range( "b" ) == Range( 6 , 8 ) );
 CHECK( I.rank( 'a' , 6 ) == 1 && I.rank( 'a' , 8 ) == 3 );
 CHECK( I.range( "ab" ) == Range( 2 , 4 ) );
 CHECK( I.count( "ab" ) == 2 && I.count( "abra" ) == 2 &&
        I.count( "a" ) == 5 && I.count( "abracadabra" ) == 1 );
 CHECK( I.count( "abrac" ) == 1 && I.count( "x" ) == 0 &&
        I.count( "rab" ) == 0 && I.count( "abracadabrab" ) == 0 );
 CHECK( I.locate( "ab" ) == std::vector< std::size_t >( { 0 , 7 } ) );
 CHECK( I.locate( "a" ) == std::vector< std::size_t >( { 0 , 3 , 5 , 7 ,
                                                         10 } ) );
 CHECK( I.extract( 0 , 11 ) == "abracadabra" );
 CHECK( I.extract( 4 , 7 ) == "cad" && I.extract( 5 , 5 ).empty() );
 CHECK( I.rank( 'a' , 0 ) == 0 && I.rank( 'a' , 12 ) == 5 &&
        I.rank( 'z' , 12 ) == 0 && I.extract( 11 , 11 ).empty() );
 const FMIndex O( "x" , 1 , 1 ) , Z( std::string( 50 , 'z' ) , 8 , 7 );
 CHECK( O.count( "x" ) == 1 && O.count( "xx" ) == 0 &&
        O.locate( "x" ) == std::vector< std::size_t >( { 0 } ) );
 CHECK( Z.count( std::string( 50 , 'z' ) ) == 1 &&
        Z.count( std::string( 49 , 'z' ) ) == 2 && Z.count( "z" ) == 50 &&
        Z.locate( std::string( 48 , 'z' ) ) ==
        std::vector< std::size_t >( { 0 , 1 , 2 } ) );
 CHECK_THROWS( I.count( "" ) , contract_violation );
 CHECK_THROWS( I.locate( "" ) , contract_violation );
 CHECK_THROWS( I.extract( 3 , 12 ) , contract_violation );
 CHECK_THROWS( I.extract( 4 , 3 ) , contract_violation );
 CHECK_THROWS( I.rank( 'a' , 13 ) , contract_violation );
 CHECK_THROWS( FMIndex( "abc" , 4 , 0 ) , contract_violation );
 }

// the FM-index of every text with b and mu from 1 to 64: the ranks against
// a count of L, the counts against the naive search, std::search and KMP
// (overlapping occurrences), the places against the naive search, and the
// extracts against substr, on substrings of the text and on random
// patterns
ADS_TEST( "fm-index" )
{
 std::mt19937 g( 1 );
 for( const auto & s : texts() ) {
  if( s.size() > 2100 ) continue;
  for( auto [ b , mu ] : { std::pair< std::size_t , std::size_t >( 1 , 1 ) ,
                           std::pair< std::size_t , std::size_t >( 3 , 5 ) ,
                           std::pair< std::size_t , std::size_t >( 64 , 32 ) ,
                           std::pair< std::size_t , std::size_t >( 7 ,
                                                                   64 ) } ) {
   const FMIndex I( s , b , mu );
   const BWT t = I.transform();
   CHECK( t == bwt( s ) );
   for( int q = 0 ; q < 20 ; ++q ) {      // the ranks
    const char c = s.empty() ? 'a' : s[ g() % s.size() ];
    const std::size_t k = g() % ( s.size() + 2 );
    std::size_t r = 0;
    for( std::size_t i = 0 ; i < std::min( k , t.last.size() + 1 ) ; ++i )
     if( ( i != t.primary ) &&
         ( t.last[ i < t.primary ? i : i - 1 ] == c ) ) ++r;
    CHECK( I.rank( c , k ) == r );
    }
   for( int q = 0 ; q < 30 ; ++q ) {
    std::string P;
    if( ( q % 3 ) && ! s.empty() ) {      // a substring of s
     const std::size_t i = g() % s.size();
     P = s.substr( i , 1 + g() % 8 );
     }
    else P = randomText( g , 1 + g() % 4 , 3 );
    const auto occ = naiveMatching( s , P );
    CHECK( I.count( P ) == occ.size() );
    CHECK( I.count( P ) == searchCount( s , P ) );
    CHECK( I.count( P ) == kmp( s , P ).size() );
    CHECK( I.locate( P ) == occ );
    const std::size_t i = g() % ( s.size() + 1 );
    const std::size_t j = i + g() % ( s.size() - i + 1 );
    CHECK( I.extract( i , j ) == s.substr( i , j - i ) );
    }
   CHECK( I.extract( 0 , s.size() ) == s );
   }
  }
 const FMIndex E( "" );
 CHECK( E.count( "a" ) == 0 && E.locate( "a" ).empty() &&
        E.extract( 0 , 0 ).empty() );
 CHECK_THROWS( FMIndex( "abc" , 0 ) , contract_violation );
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
/*-------------------- End File pearls_compression.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
