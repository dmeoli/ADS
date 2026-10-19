/*--------------------------------------------------------------------------*/
/*------------------------- File pearls_succinct.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the integer codes of chapter 11 and of the compressed data
 * structures of chapter 15 of the notes: each code against a round trip,
 * decode( encode( x ) ) == x, on random and extreme integers, its lengths
 * against the formulae of the notes and its codewords against the figures;
 * rank and select against a scan, Elias-Fano against the standard library,
 * the succinct trees against the children lists they come from and the
 * matching of the parentheses against a stack, the compressed graphs
 * against their adjacency lists and matrices, all at the level 3 of the
 * contracts, and each structure on the examples of the notes.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 11 and 15
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

#include "ADS/compression/IntegerCodes.h"
#include "ADS/succinct/BPTree.h"
#include "ADS/succinct/BVGraph.h"
#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/BitStream.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/EliasFanoGraph.h"
#include "ADS/succinct/JacobsonTree.h"
#include "ADS/succinct/K2Tree.h"
#include "ADS/succinct/LoudsTree.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
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

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

using Lists = std::vector< std::vector< std::size_t > >;

/// n random bits, each one 1 with probability p
std::vector< bool > randomBits( std::mt19937 & g , std::size_t n , double p )
{
 std::bernoulli_distribution b( p );
 std::vector< bool > v( n );
 for( std::size_t i = 0 ; i < n ; ++i ) v[ i ] = b( g );
 return( v );
 }

BitVector toBits( const std::vector< bool > & v )
{
 BitVector b;
 for( const bool x : v ) b.push_back( x );
 return( b );
 }

/// a random 64-bit integer whose length is uniform in 1 , ... , 64
std::uint64_t randomWide( std::mt19937 & g )
{
 const int len = 1 + int( g() % 64 );
 std::uint64_t x = ( std::uint64_t( g() ) << 32 ) | g();
 x = len == 64 ? x : x & ( ( std::uint64_t( 1 ) << len ) - 1 );
 return( x | ( std::uint64_t( 1 ) << ( len - 1 ) ) );
 }

/// a random tree of n nodes rooted at 0, its children in a random order
Lists randomTree( std::mt19937 & g , std::size_t n , std::size_t spread )
{
 Lists c( n );
 for( std::size_t u = 1 ; u < n ; ++u ) {
  const std::size_t lo = u > spread ? u - spread : 0;
  c[ lo + g() % ( u - lo ) ].push_back( u );
  }
 for( auto & l : c ) std::shuffle( l.begin() , l.end() , g );
 return( c );
 }

/// the nodes of the tree c by levels
std::vector< std::size_t > byLevels( const Lists & c )
{
 std::vector< std::size_t > Q;
 if( ! c.empty() ) Q.push_back( 0 );
 for( std::size_t h = 0 ; h < Q.size() ; ++h )
  for( const auto u : c[ Q[ h ] ] ) Q.push_back( u );
 return( Q );
 }

/// the nodes of the tree c in preorder
std::vector< std::size_t > inPreorder( const Lists & c )
{
 std::vector< std::size_t > P , S;
 if( ! c.empty() ) S.push_back( 0 );
 while( ! S.empty() ) {
  const std::size_t u = S.back();
  S.pop_back();
  P.push_back( u );
  for( auto it = c[ u ].rbegin() ; it != c[ u ].rend() ; ++it )
   S.push_back( *it );
  }
 return( P );
 }

/// the inverse of the permutation o
std::vector< std::size_t > inverse( const std::vector< std::size_t > & o )
{
 std::vector< std::size_t > r( o.size() );
 for( std::size_t i = 0 ; i < o.size() ; ++i ) r[ o[ i ] ] = i;
 return( r );
 }

/// the parent of each node of the tree c, nil for the root
std::vector< std::size_t > parents( const Lists & c , std::size_t nil )
{
 std::vector< std::size_t > p( c.size() , nil );
 for( std::size_t u = 0 ; u < c.size() ; ++u )
  for( const auto v : c[ u ] ) p[ v ] = u;
 return( p );
 }

/// a random graph of n nodes, each list strictly increasing; with
/// locality, the successors near the node and often copied from the list
/// before
Lists randomGraph( std::mt19937 & g , std::size_t n , std::size_t deg ,
                   bool locality )
{
 Lists a( n );
 for( std::size_t u = 0 ; u < n ; ++u ) {
  std::set< std::size_t > s;
  const std::size_t d = n ? g() % ( deg + 1 ) : 0;
  if( locality && u > 0 && g() % 2 )
   for( const auto v : a[ u - 1 ] )
    if( g() % 4 ) s.insert( v );
  for( std::size_t k = 0 ; k < d ; ++k )
   if( locality ) {
    const std::size_t run = 1 + g() % 6 , at = u + g() % 32;
    for( std::size_t j = 0 ; j < run ; ++j ) s.insert( ( at + j ) % n );
    }
   else s.insert( g() % n );
  a[ u ].assign( s.begin() , s.end() );
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// BitVector against std::vector< bool >: push_back(), append() and get() of
// fields of random lengths across the words, put(), set(), count(), the
// iterators and the strings of '0' and '1'; fields of 0 and of 64 bits,
// at the end and across two words, and the bounds of every access
ADS_TEST( "bit-vector" )
{
 std::mt19937 g( 1 );
 BitVector B;
 std::vector< bool > R;
 for( int op = 0 ; op < 20000 ; ++op ) {
  if( g() % 3 ) {
   B.push_back( g() % 2 );
   R.push_back( B[ B.size() - 1 ] );
   }
  else {
   const int len = int( g() % 65 );
   const std::uint64_t v = len ? randomWide( g ) >> ( 64 - len ) : 0;
   B.append( v , len );
   for( int i = 0 ; i < len ; ++i ) R.push_back( ( v >> i ) & 1 );
   }
  }
 CHECK( B.size() == R.size() );
 CHECK( std::equal( B.begin() , B.end() , R.begin() , R.end() ) );
 CHECK( B.count() == std::size_t( std::count( R.begin() , R.end() , true ) ) );
 for( int q = 0 ; q < 5000 ; ++q ) {
  const int len = int( g() % 65 );
  const std::size_t i = g() % ( B.size() - std::size_t( len ) + 1 );
  std::uint64_t r = 0;
  for( int k = 0 ; k < len ; ++k ) r |= std::uint64_t( R[ i + k ] ) << k;
  CHECK( B.get( i , len ) == r );
  const std::uint64_t v = len ? randomWide( g ) >> ( 64 - len ) : 0;
  B.put( i , v , len );
  for( int k = 0 ; k < len ; ++k ) R[ i + k ] = ( v >> k ) & 1;
  const std::size_t j = g() % B.size();
  B.set( j , ! B[ j ] );
  R[ j ] = ! R[ j ];
  }
 CHECK( std::equal( B.begin() , B.end() , R.begin() , R.end() ) );
 const auto it = B.begin() + 100;           // random access
 CHECK( ( it[ 5 ] == R[ 105 ] ) && ( B.end() - B.begin() ==
                                      std::ptrdiff_t( R.size() ) ) );
 CHECK( std::random_access_iterator< BitVector::const_iterator > );
 CHECK( BitVector( "0010111" ).toString() == "0010111" );
 CHECK( BitVector( "0010111" ).count() == 4 );
 CHECK( BitVector( 130 , true ).count() == 130 );
 CHECK( BitVector().empty() && BitVector( "" ).size() == 0 );
 CHECK( BitVector( "0101" ) == BitVector( "0101" ) );
 BitVector A;                               // fields of 0 and 64 bits
 A.append( 0 , 0 );
 CHECK( A.empty() && A.get( 0 , 0 ) == 0 );
 A.append( ~Word( 0 ) , 64 );
 A.append( 1 , 1 );
 A.append( 0x8000000000000001ULL , 64 );    // across two words
 CHECK( A.size() == 129 && A.count() == 67 );
 CHECK( A.get( 0 , 64 ) == ~Word( 0 ) && A.get( 65 , 64 ) ==
        0x8000000000000001ULL && A.get( 64 , 64 ) == 0x0000000000000003ULL );
 A.put( 1 , 0 , 64 );
 CHECK( A.get( 0 , 1 ) == 1 && A.get( 1 , 64 ) == 0 && A.count() == 3 );
 A.put( 129 , 0 , 0 );                      // nothing at the end
 CHECK( A.size() == 129 && A.get( 129 , 0 ) == 0 );
 CHECK_THROWS( BitVector( "012" ) , contract_violation );
 CHECK_THROWS( B.get( B.size() - 3 , 4 ) , contract_violation );
 CHECK_THROWS( B.append( 4 , 2 ) , contract_violation );
 CHECK_THROWS( B.append( 0 , 65 ) , contract_violation );
 CHECK_THROWS( B.append( 0 , -1 ) , contract_violation );
 CHECK_THROWS( A.get( 0 , 65 ) , contract_violation );
 CHECK_THROWS( A.put( 0 , 4 , 2 ) , contract_violation );
 CHECK_THROWS( A.put( 100 , 0 , 30 ) , contract_violation );
 CHECK_THROWS( ( void )A[ 129 ] , contract_violation );
 CHECK_THROWS( A.set( 129 , true ) , contract_violation );
 CHECK_THROWS( ( void )BitVector()[ 0 ] , contract_violation );
 }

// selectInWord() against a scan of the bits, on random words of every
// density, on the single bits and on the word of all 1s, the two extreme
// bits; k past the 1s, negative, and the word 0
ADS_TEST( "select-in-word" )
{
 std::mt19937 g( 1 );
 auto scan = []( Word x , int k ) {
  for( int i = 0 ; i < 64 ; ++i )
   if( ( ( x >> i ) & 1 ) && ( k-- == 0 ) ) return( i );
  return( -1 );
  };
 for( int t = 0 ; t < 20000 ; ++t ) {
  Word x = ( Word( g() ) << 32 ) | g();
  for( int d = int( g() % 4 ) ; d > 0 ; --d )   // sparser words
   x &= ( Word( g() ) << 32 ) | g();
  for( int k = 0 ; k < sidewaysSum( x ) ; ++k )
   CHECK( selectInWord( x , k ) == scan( x , k ) );
  }
 for( int i = 0 ; i < 64 ; ++i ) {
  CHECK( selectInWord( Word( 1 ) << i , 0 ) == i );
  CHECK( selectInWord( ~Word( 0 ) , i ) == i );
  }
 CHECK( selectInWord( ( Word( 1 ) << 63 ) | 1 , 1 ) == 63 );
 CHECK_THROWS( ( void )selectInWord( 6 , 2 ) , contract_violation );
 CHECK_THROWS( ( void )selectInWord( 6 , -1 ) , contract_violation );
 CHECK_THROWS( ( void )selectInWord( 0 , 0 ) , contract_violation );
 }

// BitWriter and BitReader: fields of random lengths in both orders, and
// unary codes longer than a word, read back as written; fields of 0 and
// of 64 bits, a unary code without its 1, and the reads past the end
ADS_TEST( "bit-stream" )
{
 std::mt19937 g( 1 );
 BitWriter w;
 struct Op { int kind , len; std::uint64_t v; };
 std::vector< Op > ops;
 for( int t = 0 ; t < 20000 ; ++t ) {
  const int kind = int( g() % 4 );
  const int len = int( g() % 65 );
  std::uint64_t v = len ? randomWide( g ) >> ( 64 - len ) : 0;
  if( kind == 2 ) v = g() % 300;
  ops.push_back( { kind , len , v } );
  if( kind == 0 ) w.write( v , len );
  else if( kind == 1 ) w.writeBinary( v , len );
  else if( kind == 2 ) w.writeUnary( v );
  else w.writeBit( v & 1 );
  }
 const BitVector B = w.release();
 CHECK( w.size() == 0 );
 BitReader r( B );
 for( const Op & o : ops )
  if( o.kind == 0 ) CHECK( r.read( o.len ) == o.v );
  else if( o.kind == 1 ) CHECK( r.readBinary( o.len ) == o.v );
  else if( o.kind == 2 ) CHECK( r.readUnary() == o.v );
  else CHECK( r.readBit() == bool( o.v & 1 ) );
 CHECK( r.atEnd() && ( r.position() == B.size() ) );
 BitWriter m;
 m.writeBinary( 9 , 4 );                    // most significant bit first
 m.write( 9 , 4 );
 m.writeUnary( 2 );
 CHECK( m.bits().toString() == "10011001001" );
 BitReader e( m.bits() );
 e.seek( 8 );
 CHECK( e.readUnary() == 2 );
 CHECK_THROWS( ( void )e.readBit() , contract_violation );
 CHECK_THROWS( ( void )e.readUnary() , contract_violation );
 CHECK( e.read( 0 ) == 0 && e.readBinary( 0 ) == 0 );   // none at the end
 CHECK_THROWS( ( void )e.read( 1 ) , contract_violation );
 CHECK_THROWS( e.seek( m.bits().size() + 1 ) , contract_violation );
 CHECK_THROWS( BitReader( m.bits() , m.bits().size() + 1 ) ,
               contract_violation );
 BitWriter u;                               // a unary code of 3 words
 u.writeUnary( 200 );
 u.write( ~Word( 0 ) , 64 );
 u.writeBinary( ~Word( 0 ) , 64 );
 BitReader ru( u.bits() );
 CHECK( ru.readUnary() == 200 && ru.read( 64 ) == ~Word( 0 ) &&
        ru.readBinary( 64 ) == ~Word( 0 ) && ru.atEnd() );
 const BitVector Z( 10 , false );           // zeros up to the end
 BitReader rz( Z );
 CHECK_THROWS( ( void )rz.readUnary() , contract_violation );
 CHECK_THROWS( u.write( 4 , 2 ) , contract_violation );
 CHECK_THROWS( u.writeBinary( 0 , 65 ) , contract_violation );
 }

// unary, gamma and delta: the codewords of figures 11.1 and 11.2, their
// lengths against Facts 11.1, 11.3 and 11.4 for 1 to 5000 and for random
// integers up to 2^64 - 1, and a mixed stream decoded back; the codewords
// of 2^64 - 1, a gamma and a delta whose length is past 64 bits, and 0
ADS_TEST( "elias-codes" )
{
 auto code = []( auto enc , std::uint64_t x ) {
  BitWriter w;
  enc( w , x );
  return( w.bits().toString() );
  };
 CHECK( code( encodeGamma , 9 ) == "0001001" );   // figure 11.1
 // figure 11.2 prints delta( 14 ) with one 0 more than the 9 bits of
 // gamma( 4 ) = 00100 and B( 14 ) = 1110, which the formula gives
 CHECK( code( encodeDelta , 14 ) == "001001110" );
 CHECK( code( encodeUnary , 4 ) == "0001" );
 CHECK( code( encodeGamma , 1 ) == "1" && code( encodeDelta , 1 ) == "11" );
 std::mt19937 g( 1 );
 std::vector< std::uint64_t > X;
 for( std::uint64_t x = 1 ; x <= 5000 ; ++x ) X.push_back( x );
 for( int t = 0 ; t < 5000 ; ++t ) X.push_back( randomWide( g ) );
 X.push_back( ~std::uint64_t( 0 ) );
 X.push_back( std::uint64_t( 1 ) << 63 );
 BitWriter w;
 for( const auto x : X ) {
  const int L = 1 + int( std::floor( std::log2( double( x ) ) ) );
  const int B = binaryLength( x );          // the double rounds near 2^k
  CHECK( ( B == L ) || ( x > ( std::uint64_t( 1 ) << 50 ) ) );
  CHECK( gammaLength( x ) == 2 * ( B - 1 ) + 1 );
  CHECK( deltaLength( x ) == 1 + 2 * ( binaryLength( std::uint64_t( B ) ) -
                                       1 ) + B );
  CHECK( code( encodeGamma , x ).size() == std::size_t( gammaLength( x ) ) );
  encodeGamma( w , x );
  encodeDelta( w , x );
  if( x <= 300 ) encodeUnary( w , x );
  }
 BitReader r( w.bits() );
 for( const auto x : X ) {
  CHECK( decodeGamma( r ) == x );
  CHECK( decodeDelta( r ) == x );
  if( x <= 300 ) CHECK( decodeUnary( r ) == x );
  }
 CHECK( r.atEnd() );
 CHECK( code( encodeGamma , ~std::uint64_t( 0 ) ).size() == 127 &&
        deltaLength( ~std::uint64_t( 0 ) ) == 1 + 2 * 6 + 64 );
 CHECK( binaryLength( 1 ) == 1 && binaryLength( ~std::uint64_t( 0 ) ) == 64 );
 BitWriter c;                               // a gamma of 64 zeros: past
 c.write( 0 , 64 );                         // 2^64 - 1
 c.writeBit( true );
 c.write( 0 , 64 );
 BitReader rc( c.bits() );
 CHECK_THROWS( ( void )decodeGamma( rc ) , contract_violation );
 BitWriter d;                               // a delta of length 65
 encodeGamma( d , 65 );
 d.write( 0 , 64 );
 d.write( 0 , 1 );
 BitReader rd( d.bits() );
 CHECK_THROWS( ( void )decodeDelta( rd ) , contract_violation );
 CHECK_THROWS( code( encodeGamma , 0 ) , contract_violation );
 CHECK_THROWS( code( encodeDelta , 0 ) , contract_violation );
 CHECK_THROWS( code( encodeUnary , 0 ) , contract_violation );
 CHECK_THROWS( ( void )binaryLength( 0 ) , contract_violation );
 }

// Rice and Golomb: the codeword of figure 11.3, the length of Fact 11.5,
// round trips for k up to 12 and every modulus up to 70, Golomb of 2^k
// equal to Rice of k bit by bit, and the truncated binary remainders; the
// largest modulus 2^63 and k = 63 on 1, 2^63, 2^63 + 1 and 2^64 - 1, the
// lengths of 1, and x = 0, k outside [ 0 , 63 ], b = 0 and b > 2^63
ADS_TEST( "rice-golomb" )
{
 BitWriter f;
 encodeRice( f , 83 , 4 );
 CHECK( f.bits().toString() == "0000010010" );   // figure 11.3
 BitWriter t;                    // b = 5: the remainders 00 01 10 110 111
 for( std::uint64_t x = 1 ; x <= 5 ; ++x ) encodeGolomb( t , x , 5 );
 CHECK( t.bits().toString() ==
        "1" "00" "1" "01" "1" "10" "1" "110" "1" "111" );
 std::mt19937 g( 1 );
 for( int k = 0 ; k <= 12 ; ++k ) {
  BitWriter w , v;
  std::vector< std::uint64_t > X;
  for( int i = 0 ; i < 500 ; ++i ) X.push_back( 1 + g() % ( 40u << k ) );
  for( const auto x : X ) {
   CHECK( riceLength( x , k ) ==
          ( x - 1 ) / ( std::uint64_t( 1 ) << k ) + 1 + std::uint64_t( k ) );
   encodeRice( w , x , k );
   encodeGolomb( v , x , std::uint64_t( 1 ) << k );
   }
  CHECK( w.bits() == v.bits() );
  BitReader r( w.bits() );
  for( const auto x : X ) CHECK( decodeRice( r , k ) == x );
  }
 for( std::uint64_t b = 1 ; b <= 70 ; ++b ) {
  BitWriter w;
  for( std::uint64_t x = 1 ; x <= 400 ; ++x ) {
   const std::size_t before = w.size();
   encodeGolomb( w , x , b );
   CHECK( w.size() - before == golombLength( x , b ) );
   }
  BitReader r( w.bits() );
  for( std::uint64_t x = 1 ; x <= 400 ; ++x )
   CHECK( decodeGolomb( r , b ) == x );
  }
 BitWriter e;
 encodeRice( e , ~std::uint64_t( 0 ) , 60 );
 BitReader re( e.bits() );
 CHECK( decodeRice( re , 60 ) == ~std::uint64_t( 0 ) );
 BitWriter h;                               // the largest modulus, 2^63
 const std::uint64_t b63 = std::uint64_t( 1 ) << 63;
 for( const std::uint64_t x : { std::uint64_t( 1 ) , b63 , b63 + 1 ,
                                ~std::uint64_t( 0 ) } ) {
  const std::size_t before = h.size();
  encodeGolomb( h , x , b63 );
  CHECK( h.size() - before == golombLength( x , b63 ) );
  encodeRice( h , x , 63 );
  }
 BitReader rh( h.bits() );
 for( const std::uint64_t x : { std::uint64_t( 1 ) , b63 , b63 + 1 ,
                                ~std::uint64_t( 0 ) } )
  CHECK( decodeGolomb( rh , b63 ) == x && decodeRice( rh , 63 ) == x );
 CHECK( rh.atEnd() );
 CHECK( riceLength( 1 , 0 ) == 1 && golombLength( 1 , 1 ) == 1 &&
        golombLength( 1000 , 1 ) == 1000 );
 CHECK_THROWS( encodeRice( e , 1 , 64 ) , contract_violation );
 CHECK_THROWS( encodeRice( e , 1 , -1 ) , contract_violation );
 CHECK_THROWS( encodeRice( e , 0 , 3 ) , contract_violation );
 CHECK_THROWS( ( void )decodeRice( re , 64 ) , contract_violation );
 CHECK_THROWS( encodeGolomb( e , 0 , 3 ) , contract_violation );
 CHECK_THROWS( encodeGolomb( e , 1 , 0 ) , contract_violation );
 CHECK_THROWS( encodeGolomb( e , 1 , b63 + 1 ) , contract_violation );
 CHECK_THROWS( ( void )decodeGolomb( re , 0 ) , contract_violation );
 }

// PForDelta: the example of figure 11.4, with b = 3 and the exceptions 10,
// 16 and 22, the width of 90% of the values, and round trips on values
// around a base with rare large exceptions, the bits n b + 64 e; the
// widths 1 and 63, values below the base and 2^64 - 1 as exceptions, the
// empty block, and b outside [ 1 , 63 ] and f outside ( 0 , 1 ]
ADS_TEST( "pfordelta" )
{
 const std::vector< std::uint64_t > S{ 1 , 2 , 10 , 16 , 2 , 6 , 4 , 5 ,
                                       22 , 2 };
 const PForDeltaBlock p = encodePForDelta( S , 3 , 0 );
 CHECK( p.exceptions == std::vector< std::uint64_t >( { 10 , 16 , 22 } ) );
 CHECK( p.slots.get( 2 * 3 , 3 ) == 7 && p.slots.get( 5 * 3 , 3 ) == 6 );
 CHECK( decodePForDelta( p ) == S );
 CHECK( p.codeBits() == 10 * 3 + 3 * 64 && p.size() == 10 );
 CHECK( pforDeltaWidth( S , 0 , 0.7 ) == 3 );    // 7 of the 10 in [ 0 , 6 ]
 CHECK( pforDeltaWidth( S , 0 , 1.0 ) == 5 );    // 22 <= 30
 std::mt19937 g( 1 );
 std::normal_distribution< double > N( 1000 , 20 );
 for( int t = 0 ; t < 50 ; ++t ) {
  std::vector< std::uint64_t > X( g() % 300 );
  for( auto & x : X )
   x = g() % 50 ? std::uint64_t( std::max( 0.0 , N( g ) ) ) : randomWide( g );
  const std::uint64_t base = 900;
  const int b = pforDeltaWidth( X , base );
  std::size_t in = 0;
  for( const auto x : X )
   if( ( x >= base ) && ( x - base < ( std::uint64_t( 1 ) << b ) - 1 ) ) ++in;
  CHECK( double( in ) >= 0.9 * double( X.size() ) );
  const PForDeltaBlock q = encodePForDelta( X , b , base );
  CHECK( decodePForDelta( q ) == X );
  CHECK( q.codeBits() == X.size() * std::size_t( b ) +
                         64 * q.exceptions.size() );
  }
 CHECK( decodePForDelta( encodePForDelta( {} , 5 ) ).empty() );
 const std::vector< std::uint64_t > T{ 0 , 5 , ~std::uint64_t( 0 ) , 7 , 6 };
 const PForDeltaBlock a = encodePForDelta( T , 1 , 6 );   // 1 bit, base 6
 CHECK( a.exceptions == std::vector< std::uint64_t >( { 0 , 5 ,
                                       ~std::uint64_t( 0 ) , 7 } ) );
 CHECK( decodePForDelta( a ) == T );        // all but one exceptions
 const PForDeltaBlock w = encodePForDelta( T , 63 , 0 );   // 63 bits
 CHECK( w.exceptions == std::vector< std::uint64_t >(
                                       { ~std::uint64_t( 0 ) } ) );
 CHECK( decodePForDelta( w ) == T );
 CHECK( pforDeltaWidth( T , 0 , 1.0 ) == 63 );   // 2^64 - 1 never fits
 CHECK( pforDeltaWidth( {} , 0 ) == 1 );
 CHECK( pforDeltaWidth( T , ~std::uint64_t( 0 ) , 0.2 ) == 1 );
 CHECK_THROWS( ( void )encodePForDelta( S , 0 ) , contract_violation );
 CHECK_THROWS( ( void )encodePForDelta( S , 64 ) , contract_violation );
 CHECK_THROWS( ( void )pforDeltaWidth( S , 0 , 0 ) , contract_violation );
 CHECK_THROWS( ( void )pforDeltaWidth( S , 0 , 1.5 ) , contract_violation );
 }

// the variable-byte code and the ( s , c )-dense codes: the bytes of
// figure 11.5, Fact 11.7, the words of figure 11.6 for s = c = 4 and for
// s = 6, c = 2 with words of 3 bits, s c^(k-1) integers in k words, round
// trips, and the optimal s against the lengths of the codes written; the
// dense code of 2^64 - 1, the optimal s of no frequencies and of 1 bit, a
// stream cut before its stopper, a digit past s + c, and s, c, bits at 0
ADS_TEST( "vbyte-dense" )
{
 std::vector< std::uint8_t > v;
 encodeVByte( v , 1 << 16 );                     // figure 11.5
 CHECK( v == std::vector< std::uint8_t >( { 0x84 , 0x80 , 0x00 } ) );
 std::mt19937 g( 1 );
 std::vector< std::uint64_t > X{ 0 , 1 , 127 , 128 , 16383 , 16384 ,
                                 ~std::uint64_t( 0 ) };
 for( int t = 0 ; t < 3000 ; ++t ) X.push_back( randomWide( g ) );
 v.clear();
 for( const auto x : X ) {
  const std::size_t before = v.size();
  encodeVByte( v , x );
  const int B = x ? binaryLength( x ) : 1;
  CHECK( int( v.size() - before ) == ( B + 6 ) / 7 );
  CHECK( vbyteLength( x ) == ( B + 6 ) / 7 );
  }
 std::size_t p = 0;
 for( const auto x : X ) CHECK( decodeVByte( v , p ) == x );
 CHECK( p == v.size() );
 auto words = []( std::uint64_t x , std::uint32_t s , std::uint32_t c ) {
  std::vector< std::uint32_t > o;
  encodeDense( o , x , s , c );
  std::string r;
  for( const auto d : o )
   r += std::string( 1 , char( '0' + ( d >> 2 ) ) ) +
        char( '0' + ( ( d >> 1 ) & 1 ) ) + char( '0' + ( d & 1 ) );
  return( r );
  };
 CHECK( words( 3 , 4 , 4 ) == "011" && words( 4 , 4 , 4 ) == "100000" );
 CHECK( words( 19 , 4 , 4 ) == "111011" && words( 9 , 4 , 4 ) == "101001" );
 CHECK( words( 5 , 6 , 2 ) == "101" && words( 6 , 6 , 2 ) == "110000" );
 CHECK( words( 11 , 6 , 2 ) == "110101" && words( 12 , 6 , 2 ) == "111000" );
 CHECK( words( 17 , 6 , 2 ) == "111101" );
 const std::vector< std::pair< std::uint32_t , std::uint32_t > > SC{
                       { 4 , 4 } , { 6 , 2 } , { 128 , 128 } , { 200 , 56 } ,
                       { 1 , 7 } };
 for( const auto & [ s , c ] : SC ) {
  std::vector< std::uint64_t > count( 6 , 0 );
  std::uint64_t x = 0;
  for( ; x < 20000 ; ++x ) {
   const int k = denseLength( x , s , c );
   if( k < 6 ) ++count[ std::size_t( k ) ];
   }
  for( int k = 1 ; k < 6 ; ++k ) {          // s c^( k - 1 ) in k words
   std::uint64_t e = s;
   for( int i = 1 ; i < k ; ++i ) e *= c;
   std::uint64_t below = 0;
   for( int i = 1 ; i < k ; ++i ) below += count[ std::size_t( i ) ];
   if( below + e <= 20000 ) CHECK( count[ std::size_t( k ) ] == e );
   }
  std::vector< std::uint32_t > o;
  std::vector< std::uint64_t > Y{ 0 , 1 , s , s + 1 , s * ( c + 1 ) };
  for( int t = 0 ; t < 500 ; ++t ) Y.push_back( randomWide( g ) >> 8 );
  for( const auto y : Y ) {
   const std::size_t before = o.size();
   encodeDense( o , y , s , c );
   CHECK( int( o.size() - before ) == denseLength( y , s , c ) );
   }
  std::size_t q = 0;
  for( const auto y : Y ) CHECK( decodeDense( o , q , s , c ) == y );
  if( s == 128 )                            // never longer than vbyte
   for( const auto y : Y ) CHECK( denseLength( y , s , c ) <=
                                  vbyteLength( y ) );
  }
 std::vector< std::uint64_t > F( 3000 );    // Zipf, the best s by the codes
 for( std::size_t i = 0 ; i < F.size() ; ++i ) F[ i ] = 1000000 / ( i + 1 );
 const std::uint32_t s = denseOptimalS( F , 8 );
 auto cost = [ & ]( std::uint32_t t ) {
  std::uint64_t z = 0;
  for( std::size_t i = 0 ; i < F.size() ; ++i ) {
   std::vector< std::uint32_t > o;
   encodeDense( o , i , t , 256 - t );
   z += F[ i ] * o.size();
   }
  return( z );
  };
 for( std::uint32_t t = 1 ; t < 256 ; t += 5 ) CHECK( cost( s ) <= cost( t ) );
 CHECK( ( s > 128 ) && ( cost( s ) < cost( 128 ) ) );
 std::vector< std::uint32_t > o;
 encodeDense( o , ~std::uint64_t( 0 ) , 128 , 128 );   // the largest
 std::size_t at = 0;
 CHECK( decodeDense( o , at , 128 , 128 ) == ~std::uint64_t( 0 ) &&
        at == o.size() && int( o.size() ) <= vbyteLength( ~std::uint64_t(
                                                         0 ) ) );
 CHECK( denseOptimalS( {} , 4 ) == 1 && denseOptimalS( { 5 } , 1 ) == 1 );
 const std::vector< std::uint8_t > cut{ 0x81 };   // no stopper
 std::size_t pc = 0;
 CHECK_THROWS( ( void )decodeVByte( cut , pc ) , contract_violation );
 const std::vector< std::uint32_t > bad{ 8 } , none;
 std::size_t pd = 0;
 CHECK_THROWS( ( void )decodeDense( bad , pd , 4 , 4 ) , contract_violation );
 pd = 0;
 CHECK_THROWS( ( void )decodeDense( none , pd , 4 , 4 ) ,
               contract_violation );
 CHECK_THROWS( encodeDense( o , 1 , 0 , 4 ) , contract_violation );
 CHECK_THROWS( encodeDense( o , 1 , 4 , 0 ) , contract_violation );
 CHECK_THROWS( ( void )denseOptimalS( F , 0 ) , contract_violation );
 CHECK_THROWS( ( void )denseOptimalS( F , 17 ) , contract_violation );
 }

// the interpolative code: the example of figure 11.8 in 19 bits, the items
// coded in the order 9, 3, 5, 7, 18, 11, 15 (the figure gives 4 for 15,
// whose bounds are 12 and 17, which give 3), no bit for runs, and round
// trips on random sets, sparse and clustered; the universe of all 64-bit
// integers with its two ends, the empty set, one item with low = hi (no
// bit), and items repeated or outside [ low , hi ]
ADS_TEST( "interpolative" )
{
 const std::vector< std::uint64_t > S{ 1 , 2 , 3 , 5 , 7 , 9 , 11 , 15 , 18 ,
                                       19 , 20 , 21 };
 BitWriter w;
 encodeInterpolative( w , S , 1 , 21 );
 CHECK( w.bits().toString() == "0011" "00" "01" "01" "110" "001" "011" );
 BitReader r( w.bits() );
 CHECK( decodeInterpolative( r , S.size() , 1 , 21 ) == S );
 BitWriter z;                                    // a run: no bit
 std::vector< std::uint64_t > run( 100 );
 std::iota( run.begin() , run.end() , 50 );
 encodeInterpolative( z , run , 50 , 149 );
 CHECK( z.size() == 0 );
 BitReader rz( z.bits() );
 CHECK( decodeInterpolative( rz , 100 , 50 , 149 ) == run );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  std::set< std::uint64_t > s;
  const std::uint64_t u = 1 + g() % ( t % 2 ? 100000 : 300 );
  const std::size_t n = g() % 200;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::uint64_t at = g() % u;
   for( std::uint64_t j = 0 ; j < ( t % 3 ? 1 : 8 ) ; ++j )
    if( at + j < u ) s.insert( at + j );
   }
  const std::vector< std::uint64_t > X( s.begin() , s.end() );
  BitWriter e;
  encodeInterpolative( e , X , 0 , u - 1 );
  BitReader d( e.bits() );
  CHECK( decodeInterpolative( d , X.size() , 0 , u - 1 ) == X );
  CHECK( d.atEnd() );
  }
 const std::uint64_t top = ~std::uint64_t( 0 );  // the whole universe
 const std::vector< std::uint64_t > U{ 0 , 1 , top / 2 , top - 1 , top };
 BitWriter wu;
 encodeInterpolative( wu , U , 0 , top );
 BitReader ru( wu.bits() );
 CHECK( decodeInterpolative( ru , U.size() , 0 , top ) == U && ru.atEnd() );
 BitWriter wo;                                   // empty, one, low = hi
 encodeInterpolative( wo , {} , 0 , 9 );
 encodeInterpolative( wo , { 7 } , 7 , 7 );
 CHECK( wo.size() == 0 );
 encodeInterpolative( wo , { 4 } , 0 , 9 );
 CHECK( wo.size() == 4 );
 BitReader ro( wo.bits() );
 CHECK( decodeInterpolative( ro , 0 , 0 , 9 ).empty() &&
        decodeInterpolative( ro , 1 , 7 , 7 ) ==
        std::vector< std::uint64_t >( { 7 } ) &&
        decodeInterpolative( ro , 1 , 0 , 9 ) ==
        std::vector< std::uint64_t >( { 4 } ) );
 CHECK_THROWS( encodeInterpolative( w , { 3 , 3 } , 0 , 9 ) ,
               contract_violation );
 CHECK_THROWS( encodeInterpolative( w , { 3 , 4 } , 4 , 9 ) ,
               contract_violation );
 CHECK_THROWS( encodeInterpolative( w , { 3 , 10 } , 0 , 9 ) ,
               contract_violation );
 CHECK_THROWS( encodeBounded( w , 10 , 0 , 9 ) , contract_violation );
 CHECK_THROWS( ( void )decodeBounded( r , 5 , 4 ) , contract_violation );
 }

// the mapping of the integers of any sign and the d-gaps: -2 to 5 as in
// Table 15.7, the gaps of node 15 of Table 15.4 and their prefix sums;
// the extremes +-( 2^62 - 1 ) and 2^64 - 1, repeated items (gap 0) up to
// 2^64 - 1, the empty sequence, and the values past the bounds, unsorted
ADS_TEST( "signed-gaps" )
{
 CHECK( signedToNatural( -2 ) == 5 && signedToNatural( 3 ) == 6 );
 CHECK( signedToNatural( 0 ) == 0 );
 for( std::int64_t x = -1000 ; x <= 1000 ; ++x )
  CHECK( naturalToSigned( signedToNatural( x ) ) == x );
 const std::vector< std::uint64_t > S{ 13 , 15 , 16 , 17 , 18 , 19 , 23 ,
                                       24 , 203 , 315 , 1034 };
 const std::vector< std::uint64_t > G{ 13 , 2 , 1 , 1 , 1 , 1 , 4 , 1 ,
                                       179 , 112 , 719 };
 CHECK( dGaps( S ) == G && prefixSums( G ) == S );
 CHECK( dGaps( {} ).empty() && prefixSums( {} ).empty() );
 const std::int64_t M = ( std::int64_t( 1 ) << 62 ) - 1;   // the extremes
 CHECK( signedToNatural( M ) == ( std::uint64_t( 1 ) << 63 ) - 2 &&
        signedToNatural( -M ) == ( std::uint64_t( 1 ) << 63 ) - 1 );
 CHECK( naturalToSigned( signedToNatural( M ) ) == M &&
        naturalToSigned( signedToNatural( -M ) ) == -M );
 CHECK( naturalToSigned( ~std::uint64_t( 0 ) ) ==
        -std::int64_t( ~std::uint64_t( 0 ) / 2 ) );
 const std::vector< std::uint64_t > D{ 0 , 0 , 5 , 5 , ~std::uint64_t( 0 ) };
 CHECK( dGaps( D ) == std::vector< std::uint64_t >( { 0 , 0 , 5 , 0 ,
                                        ~std::uint64_t( 0 ) - 5 } ) &&
        prefixSums( dGaps( D ) ) == D );         // repeated items, gap 0
 CHECK_THROWS( ( void )signedToNatural( M + 1 ) , contract_violation );
 CHECK_THROWS( ( void )signedToNatural( -M - 1 ) , contract_violation );
 CHECK_THROWS( ( void )dGaps( { 2 , 1 } ) , contract_violation );
 }

// rank and select against a scan: Example 15.1, then vectors of many sizes
// and densities, around the words and the superblocks, every rank and
// every select of both bits; the empty vector, all zeros, and the bounds
// of rank (i <= n) and select (1 <= k <= the bits of that value)
ADS_TEST( "rank-select" )
{
 const RankSelect E( BitVector( "00101001010" ) );   // Example 15.1
 CHECK( E.rank1( 6 ) == 2 && E.select1( 3 ) == 7 );
 std::mt19937 g( 1 );
 for( const std::size_t n : { 0 , 1 , 63 , 64 , 65 , 511 , 512 , 513 , 4097 ,
                              70000 , 300000 } )
  for( const double p : { 0.0 , 0.001 , 0.1 , 0.5 , 0.97 , 1.0 } ) {
   const std::vector< bool > R = randomBits( g , n , p );
   const RankSelect S( toBits( R ) );
   CHECK( S.size() == n );
   std::size_t ones = 0;
   std::vector< std::size_t > P1 , P0;
   for( std::size_t i = 0 ; i <= n ; ++i ) {
    CHECK( S.rank1( i ) == ones && S.rank0( i ) == i - ones );
    if( i == n ) break;
    if( R[ i ] ) { ++ones; P1.push_back( i ); }
    else P0.push_back( i );
    }
   CHECK( S.ones() == P1.size() && S.zeros() == P0.size() );
   for( std::size_t k = 0 ; k < P1.size() ; ++k )
    CHECK( S.select1( k + 1 ) == P1[ k ] );
   for( std::size_t k = 0 ; k < P0.size() ; ++k )
    CHECK( S.select0( k + 1 ) == P0[ k ] );
   CHECK( S.directoryBits() <= n / 4 + n / 5 + 4096 );
   }
 CHECK_THROWS( ( void )E.select1( 5 ) , contract_violation );
 CHECK_THROWS( ( void )E.select0( 0 ) , contract_violation );
 CHECK_THROWS( ( void )E.select1( 0 ) , contract_violation );
 CHECK_THROWS( ( void )E.select0( 8 ) , contract_violation );
 CHECK_THROWS( ( void )E.rank1( 12 ) , contract_violation );
 CHECK_THROWS( ( void )E.rank0( 12 ) , contract_violation );
 const RankSelect Z( BitVector( 100 , false ) ) , N;
 CHECK( Z.rank1( 100 ) == 0 && Z.select0( 100 ) == 99 && N.rank1( 0 ) == 0 );
 CHECK_THROWS( ( void )Z.select1( 1 ) , contract_violation );
 CHECK_THROWS( ( void )N.select0( 1 ) , contract_violation );
 }

// Elias-Fano: the code of figure 11.9 (L, H, access( 4 ) = 24 and
// nextGEQ( 25 ) = 26), the bound of Theorem 11.1, the rank and select of
// Example 15.3, and random sequences with repeated items against
// std::lower_bound, from the empty one to items near 2^63; one item in the
// universe 2^64 - 1 (64 low bits), all equal items, the universe { 0 },
// the empty sequence in a universe, the bounds of access(), an unsorted
// sequence, an item not below u, and 2^64 - 1 without its universe
ADS_TEST( "elias-fano" )
{
 const std::vector< std::uint64_t > S{ 1 , 4 , 7 , 18 , 24 , 26 , 30 , 31 };
 const EliasFano F( S , 32 );
 CHECK( F.lowWidth() == 2 );
 const std::vector< std::uint64_t > low{ 1 , 0 , 3 , 2 , 0 , 2 , 2 , 3 };
 for( std::size_t i = 0 ; i < 8 ; ++i ) CHECK( F.low().get( 2 * i , 2 ) ==
                                               low[ i ] );
 CHECK( F.high().bits().toString() == "1011000100110110" );
 CHECK( F.access( 4 ) == 24 && F[ F.nextGEQ( 25 ) ] == 26 );
 CHECK( F.codeBits() <= 2 * 8 + 8 * 2 );         // Theorem 11.1
 const EliasFano X( { 2 , 4 , 7 , 9 } , 11 );   // Example 15.3, from 0
 CHECK( X[ 3 ] == 9 );                           // Select_1( B , 4 ) = 10
 CHECK( X.nextGEQ( 8 ) == 3 && X.nextGEQ( 7 ) == 2 && X.nextGEQ( 9 ) == 3 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t n = t < 3 ? std::size_t( t ) : g() % 2000;
  std::uint64_t u = 1 + ( t % 5 == 0 ? randomWide( g ) >> 1
                                     : g() % ( 1 + 50 * ( n + 1 ) ) );
  if( t % 7 == 0 ) u = 1 + g() % 4;              // many repetitions
  std::vector< std::uint64_t > A( n );
  for( auto & a : A ) a = ( ( std::uint64_t( g() ) << 32 ) | g() ) % u;
  std::sort( A.begin() , A.end() );
  const EliasFano E( A , u );
  CHECK( E.size() == n && E.universe() == u );
  CHECK( std::equal( E.begin() , E.end() , A.begin() , A.end() ) );
  CHECK( E.decode() == A );
  const double ell = n ? std::ceil( std::log2( double( u ) / double( n ) ) )
                       : 0;
  CHECK( double( E.codeBits() ) <= 2.0 * double( n ) + double( n ) *
                                   std::max( ell , 0.0 ) + 1 );
  for( int q = 0 ; q < 300 ; ++q ) {
   const std::uint64_t x = q % 3 || A.empty() ? ( ( std::uint64_t( g() ) <<
                                                    32 ) | g() ) % ( u + 2 )
                                              : A[ g() % A.size() ];
   const std::size_t r = std::size_t( std::lower_bound( A.begin() ,
                                                        A.end() , x ) -
                                      A.begin() );
   CHECK( E.nextGEQ( x ) == r );
   CHECK( E.contains( x ) == std::binary_search( A.begin() , A.end() , x ) );
   }
  }
 const EliasFano Z;
 CHECK( Z.empty() && Z.nextGEQ( 5 ) == 0 );
 const EliasFano W( { 0 , ~std::uint64_t( 0 ) - 1 } );
 CHECK( W[ 1 ] == ~std::uint64_t( 0 ) - 1 && W.nextGEQ( 1 ) == 1 );
 const std::uint64_t top = ~std::uint64_t( 0 );
 const EliasFano O( { 5 } , top );               // 64 low bits per item
 CHECK( O.lowWidth() == 64 && O[ 0 ] == 5 && O.nextGEQ( 5 ) == 0 &&
        O.nextGEQ( 6 ) == 1 && O.nextGEQ( top ) == 1 && ! O.contains( 4 ) );
 const EliasFano R( { 7 , 7 , 7 } );             // all equal, u = 8
 CHECK( R.decode() == std::vector< std::uint64_t >( { 7 , 7 , 7 } ) &&
        R.nextGEQ( 7 ) == 0 && R.nextGEQ( 0 ) == 0 && R.nextGEQ( 8 ) == 3 );
 const EliasFano U( { 0 , 0 } , 1 );             // the universe { 0 }
 CHECK( U[ 1 ] == 0 && U.nextGEQ( 0 ) == 0 && U.nextGEQ( 1 ) == 2 );
 const EliasFano V( {} , 100 );
 CHECK( V.empty() && V.universe() == 100 && V.nextGEQ( 0 ) == 0 &&
        ! V.contains( 0 ) && V.begin() == V.end() );
 CHECK_THROWS( ( void )O.access( 1 ) , contract_violation );
 CHECK_THROWS( ( void )V[ 0 ] , contract_violation );
 CHECK_THROWS( EliasFano( { 3 , 1 } ) , contract_violation );
 CHECK_THROWS( EliasFano( { 1 , 5 } , 5 ) , contract_violation );
 CHECK_THROWS( EliasFano( { 1 , top } ) , contract_violation );
 CHECK_THROWS( EliasFano( { 0 } , 0 ) , contract_violation );
 }

// Jacobson's binary tree: the queries of Example 15.4 on the code of
// figure 15.7, and random binary trees against their children; the empty
// tree, one node, a right chain of 10^5 nodes, the bounds of the queries,
// and children lists with a self-loop, a cycle or a node twice (with as
// many nodes reached as there are, hence the marks of the nodes reached)
ADS_TEST( "jacobson-tree" )
{
 const JacobsonTree E( BitVector( "11110110100100000" ) );
 CHECK( E.size() == 8 );
 CHECK( E.left( 2 ) == 4 && E.right( 3 ) == 6 && E.parent( 7 ) == 5 );
 constexpr std::size_t nil = JacobsonTree::nil;
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t n = t < 2 ? std::size_t( t ) : 1 + g() % 600;
  std::vector< std::pair< std::size_t , std::size_t > > c( n , { nil , nil } );
  std::vector< std::uint64_t > key( n );   // a random search tree
  for( std::size_t u = 0 ; u < n ; ++u ) {
   key[ u ] = g();
   if( u == 0 ) continue;
   for( std::size_t x = 0 ; ; ) {
    auto & next = key[ u ] < key[ x ] ? c[ x ].first : c[ x ].second;
    if( next == nil ) { next = u; break; }
    x = next;
    }
   }
  const JacobsonTree J( c );
  CHECK( J.size() == n && J.bits().size() == 2 * n + 1 );
  Lists ch( n );                             // the levels, from the left
  for( std::size_t u = 0 ; u < n ; ++u )
   for( const auto v : { c[ u ].first , c[ u ].second } )
    if( v != nil ) ch[ u ].push_back( v );
  const auto L = byLevels( ch );
  const auto at = inverse( L );
  const auto P = parents( ch , nil );
  auto map = [ & ]( std::size_t v ) { return( v == nil ? nil : at[ v ] ); };
  for( std::size_t v = 0 ; v < n ; ++v ) {
   const std::size_t u = L[ v ];
   CHECK( J.left( v ) == map( c[ u ].first ) );
   CHECK( J.right( v ) == map( c[ u ].second ) );
   CHECK( J.parent( v ) == map( P[ u ] ) );
   CHECK( J.isLeaf( v ) == ch[ u ].empty() );
   }
  }
 CHECK( JacobsonTree().size() == 0 && JacobsonTree().root() == nil );
 const JacobsonTree O( BitVector( "100" ) );    // one node
 CHECK( O.size() == 1 && O.isLeaf( 0 ) && O.left( 0 ) == nil &&
        O.right( 0 ) == nil && O.parent( 0 ) == nil );
 std::vector< std::pair< std::size_t , std::size_t > > chain( 100000 ,
                                                              { nil , nil } );
 for( std::size_t u = 0 ; u + 1 < chain.size() ; ++u )
  chain[ u ].second = u + 1;                     // a right chain, deep
 const JacobsonTree C( chain );
 CHECK( C.right( 99998 ) == 99999 && C.parent( 99999 ) == 99998 &&
        C.left( 500 ) == nil && C.isLeaf( 99999 ) );
 CHECK_THROWS( O.left( 1 ) , contract_violation );
 CHECK_THROWS( O.right( 1 ) , contract_violation );
 CHECK_THROWS( O.parent( 1 ) , contract_violation );
 using Kids = std::vector< std::pair< std::size_t , std::size_t > >;
 CHECK_THROWS( JacobsonTree( Kids{ { 0 , nil } } ) , contract_violation );
 CHECK_THROWS( JacobsonTree( Kids{ { 1 , nil } , { nil , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( JacobsonTree( Kids{ { 1 , 1 } , { nil , nil } ,
                                   { nil , nil } } ) ,
               contract_violation );            // 1 twice, 2 never
 CHECK_THROWS( JacobsonTree( BitVector( "10100" ) ).left( 3 ) ,
               contract_violation );
 CHECK_THROWS( JacobsonTree( BitVector( "10010" ) ) , contract_violation );
 }

// LOUDS: the code and the queries of figure 15.8 and Example 15.5, and
// random trees against their children; the empty tree, one node, a star
// and a chain of 10^5 nodes, the bounds of the queries, and children lists
// with a self-loop, a cycle, a node twice or a node unreached
ADS_TEST( "louds-tree" )
{
 const std::vector< std::size_t > deg{ 3 , 2 , 0 , 3 , 0 , 1 , 0 , 2 , 0 , 0 ,
                                       0 , 0 };   // figure 15.8, by levels
 Lists c( deg.size() );
 for( std::size_t u = 0 , next = 1 ; u < deg.size() ; ++u )
  for( std::size_t k = 0 ; k < deg[ u ] ; ++k ) c[ u ].push_back( next++ );
 const LoudsTree E( c );
 CHECK( E.bits().toString() == "0100010011000110110011111" );
 constexpr std::size_t nil = LoudsTree::nil;
 CHECK( E.degree( 3 ) == 3 && E.parent( 4 ) == 1 );
 CHECK( E.leftmostChild( 8 ) == nil && E.leftmostChild( 7 ) == 10 );
 CHECK( E.rightSibling( 11 ) == nil && E.rightSibling( 10 ) == 11 );
 CHECK( LoudsTree( E.bits() ).degree( 0 ) == 3 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t n = t < 2 ? std::size_t( t ) : 1 + g() % 800;
  const Lists T = randomTree( g , n , t % 2 ? 3 : n );
  const LoudsTree D( T );
  CHECK( D.size() == n && D.bits().size() == 2 * n + 1 );
  const auto L = byLevels( T );
  const auto at = inverse( L );
  const auto P = parents( T , nil );
  for( std::size_t v = 0 ; v < n ; ++v ) {
   const auto & k = T[ L[ v ] ];
   CHECK( D.degree( v ) == k.size() );
   CHECK( D.parent( v ) == ( v ? at[ P[ L[ v ] ] ] : nil ) );
   CHECK( D.leftmostChild( v ) == ( k.empty() ? nil : at[ k[ 0 ] ] ) );
   for( std::size_t i = 0 ; i < k.size() ; ++i ) {
    CHECK( D.child( v , i ) == at[ k[ i ] ] );
    CHECK( D.rightSibling( at[ k[ i ] ] ) ==
           ( i + 1 < k.size() ? at[ k[ i + 1 ] ] : nil ) );
    }
   }
  }
 CHECK( LoudsTree().size() == 0 && LoudsTree().root() == nil );
 const LoudsTree O( Lists( 1 ) );               // one node
 CHECK( O.size() == 1 && O.isLeaf( 0 ) && O.parent( 0 ) == nil &&
        O.leftmostChild( 0 ) == nil && O.rightSibling( 0 ) == nil );
 Lists star( 100000 ) , chain( 100000 );        // wide and deep
 for( std::size_t u = 1 ; u < star.size() ; ++u ) star[ 0 ].push_back( u );
 for( std::size_t u = 0 ; u + 1 < chain.size() ; ++u )
  chain[ u ].push_back( u + 1 );
 const LoudsTree S( star ) , C( chain );
 CHECK( S.degree( 0 ) == 99999 && S.child( 0 , 99998 ) == 99999 &&
        S.parent( 99999 ) == 0 && S.rightSibling( 99999 ) == nil );
 CHECK( C.degree( 99998 ) == 1 && C.parent( 99999 ) == 99998 &&
        C.isLeaf( 99999 ) );
 CHECK_THROWS( O.degree( 1 ) , contract_violation );
 CHECK_THROWS( O.parent( 1 ) , contract_violation );
 CHECK_THROWS( O.leftmostChild( 1 ) , contract_violation );
 CHECK_THROWS( O.rightSibling( 1 ) , contract_violation );
 CHECK_THROWS( O.child( 0 , 0 ) , contract_violation );
 CHECK_THROWS( LoudsTree( Lists{ { 0 } } ) , contract_violation );
 CHECK_THROWS( LoudsTree( Lists{ { 1 } , { 1 } } ) , contract_violation );
 CHECK_THROWS( LoudsTree( Lists{ { 1 , 1 } , {} , {} } ) ,
               contract_violation );            // 1 twice, 2 never
 CHECK_THROWS( LoudsTree( Lists{ { 1 } , {} , {} } ) , contract_violation );
 CHECK_THROWS( LoudsTree( BitVector( "00111" ) ) , contract_violation );
 }

// the parentheses: findClose(), findOpen() and enclose() against a stack on
// random sequences of many sizes, on a deep nesting and on a flat one,
// across many blocks of the range min-max tree; the empty sequence, the
// outermost pair (no enclosing one), unbalanced sequences of one symbol,
// and every query on the wrong symbol or past the end
ADS_TEST( "balanced-parentheses" )
{
 std::mt19937 g( 1 );
 auto check = [ & ]( const std::vector< bool > & R ) {
  const BalancedParentheses B( toBits( R ) );
  const std::size_t m = R.size() , nil = BalancedParentheses::nil;
  std::vector< std::size_t > match( m ) , up( m , nil ) , S;
  std::int64_t e = 0;
  for( std::size_t i = 0 ; i < m ; ++i ) {
   e += R[ i ] ? 1 : -1;
   CHECK( B.excess( i ) == e );
   if( R[ i ] ) {
    up[ i ] = S.empty() ? nil : S.back();
    S.push_back( i );
    }
   else {
    match[ i ] = S.back();
    match[ S.back() ] = i;
    S.pop_back();
    }
   }
  for( std::size_t i = 0 ; i < m ; ++i )
   if( R[ i ] ) {
    CHECK( B.findClose( i ) == match[ i ] );
    CHECK( B.enclose( i ) == up[ i ] );
    }
   else CHECK( B.findOpen( i ) == match[ i ] );
  };
 for( int t = 0 ; t < 120 ; ++t ) {        // random walks kept balanced
  const std::size_t n = t < 2 ? std::size_t( t ) : 1 + g() % 5000;
  std::vector< bool > R;
  std::size_t open = 0 , left = n;
  while( left > 0 || open > 0 ) {
   const bool o = left > 0 &&
                  ( open == 0 || g() % 100 < 50u + unsigned( t % 3 ) * 10 );
   R.push_back( o );
   if( o ) { --left; ++open; } else --open;
   }
  check( R );
  }
 std::vector< bool > deep( 6000 , true ) , flat;
 deep.insert( deep.end() , 6000 , false );
 check( deep );
 for( int i = 0 ; i < 3000 ; ++i ) { flat.push_back( true );
                                     flat.push_back( false ); }
 check( flat );
 CHECK_THROWS( BalancedParentheses( BitVector( "1001" ) ) ,
               contract_violation );
 CHECK_THROWS( BalancedParentheses( BitVector( "1" ) ) , contract_violation );
 CHECK_THROWS( BalancedParentheses( BitVector( "0" ) ) , contract_violation );
 const BalancedParentheses B( BitVector( "1100" ) ) , E( BitVector( "" ) );
 CHECK( E.size() == 0 && B.enclose( 0 ) == BalancedParentheses::nil &&
        B.enclose( 1 ) == 0 && B.excess( 3 ) == 0 );
 CHECK_THROWS( ( void )B.findClose( 2 ) , contract_violation );
 CHECK_THROWS( ( void )B.findClose( 4 ) , contract_violation );
 CHECK_THROWS( ( void )B.findOpen( 0 ) , contract_violation );
 CHECK_THROWS( ( void )B.findOpen( 4 ) , contract_violation );
 CHECK_THROWS( ( void )B.enclose( 3 ) , contract_violation );
 CHECK_THROWS( ( void )B.excess( 4 ) , contract_violation );
 CHECK_THROWS( ( void )E.excess( 0 ) , contract_violation );
 }

// the tree by parentheses against the children of random trees, numbered
// in preorder: parent, children, siblings, degree, subtree size, depth,
// ancestors; and against LOUDS on the same tree; the empty tree, one node,
// a chain of 10^5 nodes, the bounds of the queries, children lists with a
// self-loop, a cycle, a node twice or a node unreached, and two trees
ADS_TEST( "bp-tree" )
{
 std::mt19937 g( 1 );
 constexpr std::size_t nil = BPTree::nil;
 for( int t = 0 ; t < 150 ; ++t ) {
  const std::size_t n = t < 2 ? std::size_t( t ) : 1 + g() % 1500;
  const Lists T = randomTree( g , n , t % 3 == 0 ? 2 : n );
  const BPTree B( T );
  CHECK( B.size() == n && B.parentheses().size() == 2 * n );
  const auto P = inPreorder( T );
  const auto at = inverse( P );
  const auto par = parents( T , nil );
  std::vector< std::size_t > size( n , 1 ) , depth( n , 0 );
  for( std::size_t i = n ; i-- > 0 ; )      // children after their parent
   if( par[ P[ i ] ] != nil ) size[ par[ P[ i ] ] ] += size[ P[ i ] ];
  for( std::size_t i = 0 ; i < n ; ++i )
   if( par[ P[ i ] ] != nil ) depth[ P[ i ] ] = depth[ par[ P[ i ] ] ] + 1;
  for( std::size_t v = 0 ; v < n ; ++v ) {
   const std::size_t u = P[ v ];
   const auto & k = T[ u ];
   CHECK( B.parent( v ) == ( par[ u ] == nil ? nil : at[ par[ u ] ] ) );
   CHECK( B.leftmostChild( v ) == ( k.empty() ? nil : at[ k[ 0 ] ] ) );
   CHECK( B.degree( v ) == k.size() && B.isLeaf( v ) == k.empty() );
   CHECK( B.subtreeSize( v ) == size[ u ] && B.depth( v ) == depth[ u ] );
   for( std::size_t i = 0 ; i < k.size() ; ++i )
    CHECK( B.rightSibling( at[ k[ i ] ] ) ==
           ( i + 1 < k.size() ? at[ k[ i + 1 ] ] : nil ) );
   const std::size_t w = g() % n;           // an ancestor by the parents
   bool anc = false;
   for( std::size_t x = P[ w ] ; x != nil ; x = par[ x ] ) anc |= x == u;
   CHECK( B.isAncestor( v , w ) == anc );
   }
  const LoudsTree L( T );
  const auto lv = inverse( byLevels( T ) );
  for( std::size_t v = 0 ; v < n ; ++v )
   CHECK( L.degree( lv[ P[ v ] ] ) == B.degree( v ) );
  if( n > 0 ) CHECK( B.subtreeSize( 0 ) == n && B.parent( 0 ) == nil );
  }
 CHECK( BPTree( BitVector( "110100" ) ).degree( 0 ) == 2 );
 const BPTree E( Lists{} ) , O( Lists( 1 ) );   // empty, one node
 CHECK( E.size() == 0 && E.root() == nil && BPTree( BitVector( "" ) ).size()
        == 0 );
 CHECK( O.size() == 1 && O.isLeaf( 0 ) && O.parent( 0 ) == nil &&
        O.depth( 0 ) == 0 && O.subtreeSize( 0 ) == 1 &&
        O.rightSibling( 0 ) == nil && O.isAncestor( 0 , 0 ) );
 Lists chain( 100000 );                          // deep: no recursion
 for( std::size_t u = 0 ; u + 1 < chain.size() ; ++u )
  chain[ u ].push_back( u + 1 );
 const BPTree C( chain );
 CHECK( C.depth( 99999 ) == 99999 && C.subtreeSize( 1 ) == 99999 &&
        C.parent( 99999 ) == 99998 && C.isAncestor( 0 , 99999 ) &&
        ! C.isAncestor( 99999 , 0 ) );
 CHECK_THROWS( O.parent( 1 ) , contract_violation );
 CHECK_THROWS( O.leftmostChild( 1 ) , contract_violation );
 CHECK_THROWS( O.subtreeSize( 1 ) , contract_violation );
 CHECK_THROWS( O.isAncestor( 0 , 1 ) , contract_violation );
 CHECK_THROWS( O.isAncestor( 1 , 0 ) , contract_violation );
 CHECK_THROWS( BPTree( Lists{ { 0 } } ) , contract_violation );
 CHECK_THROWS( BPTree( Lists{ { 1 } , { 1 } } ) , contract_violation );
 CHECK_THROWS( BPTree( Lists{ { 1 , 1 } , {} , {} } ) ,
               contract_violation );            // 1 twice, 2 never
 CHECK_THROWS( BPTree( Lists{ { 1 } , {} , {} } ) , contract_violation );
 CHECK_THROWS( BPTree( BitVector( "1010" ) ) , contract_violation );
 }

// the graph by Elias-Fano against its adjacency lists: outdegrees, the i-th
// successors, the lists and every edge of small graphs; the empty graph, a
// single self-loop, nodes without edges, the bounds of the queries, a
// target past the nodes and a list out of order
ADS_TEST( "elias-fano-graph" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const std::size_t n = t < 2 ? std::size_t( t ) : 1 + g() % 300;
  const Lists A = randomGraph( g , n , t % 2 ? 4 : 40 , t % 3 == 0 );
  const EliasFanoGraph G( A );
  std::size_t m = 0;
  for( std::size_t u = 0 ; u < n ; ++u ) {
   m += A[ u ].size();
   CHECK( G.outdegree( u ) == A[ u ].size() && G.successors( u ) == A[ u ] );
   for( std::size_t i = 0 ; i < A[ u ].size() ; ++i )
    CHECK( G.successor( u , i ) == A[ u ][ i ] );
   for( std::size_t v = 0 ; v < n ; ++v )
    CHECK( G.hasEdge( u , v ) ==
           std::binary_search( A[ u ].begin() , A[ u ].end() , v ) );
   }
  CHECK( G.order() == n && G.edges() == m );
  }
 const EliasFanoGraph L( Lists{ { 0 } } ) , E( Lists{} );   // a self-loop
 CHECK( L.order() == 1 && L.edges() == 1 && L.hasEdge( 0 , 0 ) &&
        L.successor( 0 , 0 ) == 0 && E.order() == 0 && E.edges() == 0 );
 const EliasFanoGraph N( Lists( 5 ) );           // no edge
 CHECK( N.edges() == 0 && N.outdegree( 4 ) == 0 && ! N.hasEdge( 4 , 4 ) &&
        N.successors( 2 ).empty() );
 CHECK_THROWS( L.outdegree( 1 ) , contract_violation );
 CHECK_THROWS( L.successor( 0 , 1 ) , contract_violation );
 CHECK_THROWS( L.hasEdge( 0 , 1 ) , contract_violation );
 CHECK_THROWS( L.hasEdge( 1 , 0 ) , contract_violation );
 CHECK_THROWS( EliasFanoGraph( Lists{ { 1 } } ) , contract_violation );
 CHECK_THROWS( EliasFanoGraph( Lists{ { 1 , 0 } , {} } ) ,
               contract_violation );
 }

// the k^2-tree: T and L of figure 15.9, then random graphs of sides that are
// and are not powers of k, for k = 2, 3, 4, against the adjacency matrix:
// successors, predecessors and every edge; a self-loop given twice, no
// edge, k larger than the side, the complete graph, the bounds of the
// queries and of the edges, and k = 1
ADS_TEST( "k2-tree" )
{
 const K2Tree E( 4 , { { 0 , 0 } , { 0 , 1 } , { 1 , 1 } , { 2 , 2 } ,
                       { 2 , 3 } , { 3 , 2 } , { 0 , 1 } } );
 CHECK( E.levels().toString() == "1001" );
 CHECK( E.leaves().toString() == "11011110" );
 CHECK( E.edges() == 6 );
 CHECK( E.neighbors( 2 ) == std::vector< std::size_t >( { 2 , 3 } ) );
 CHECK( E.reverseNeighbors( 1 ) == std::vector< std::size_t >( { 0 , 1 } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 90 ; ++t ) {
  const std::size_t k = 2 + std::size_t( t % 3 );
  const std::size_t n = t < 3 ? std::size_t( t ) : 1 + g() % 200;
  const Lists A = randomGraph( g , n , t % 2 ? 3 : 30 , t % 4 == 0 );
  std::vector< K2Tree::Edge > e;
  std::vector< std::vector< bool > > M( n , std::vector< bool >( n ) );
  for( std::size_t u = 0 ; u < n ; ++u )
   for( const auto v : A[ u ] ) {
    e.emplace_back( u , v );
    M[ u ][ v ] = true;
    }
  std::shuffle( e.begin() , e.end() , g );
  const K2Tree K( n , e , k );
  CHECK( K.order() == n && K.edges() == e.size() );
  for( std::size_t u = 0 ; u < n ; ++u ) {
   CHECK( K.neighbors( u ) == A[ u ] );
   std::vector< std::size_t > in;
   for( std::size_t v = 0 ; v < n ; ++v ) {
    if( M[ v ][ u ] ) in.push_back( v );
    CHECK( K.hasEdge( u , v ) == M[ u ][ v ] );
    }
   CHECK( K.reverseNeighbors( u ) == in );
   }
  }
 const K2Tree O( 1 , { { 0 , 0 } , { 0 , 0 } } );   // a self-loop, twice
 CHECK( O.edges() == 1 && O.hasEdge( 0 , 0 ) &&
        O.neighbors( 0 ) == std::vector< std::size_t >( { 0 } ) );
 const K2Tree N( 5 , {} ) , W( 3 , { { 2 , 0 } } , 16 );   // k > n
 CHECK( N.edges() == 0 && N.neighbors( 4 ).empty() && ! N.hasEdge( 4 , 4 ) &&
        N.reverseNeighbors( 0 ).empty() );
 CHECK( W.hasEdge( 2 , 0 ) && ! W.hasEdge( 0 , 2 ) &&
        W.reverseNeighbors( 0 ) == std::vector< std::size_t >( { 2 } ) );
 const K2Tree F( 70 , [] {                       // the complete graph
  std::vector< K2Tree::Edge > c;
  for( std::size_t u = 0 ; u < 70 ; ++u )
   for( std::size_t v = 0 ; v < 70 ; ++v ) c.emplace_back( u , v );
  return( c ); }() );
 CHECK( F.edges() == 4900 && F.neighbors( 69 ).size() == 70 &&
        F.reverseNeighbors( 0 ).size() == 70 && F.hasEdge( 69 , 0 ) );
 CHECK_THROWS( O.neighbors( 1 ) , contract_violation );
 CHECK_THROWS( O.reverseNeighbors( 1 ) , contract_violation );
 CHECK_THROWS( O.hasEdge( 0 , 1 ) , contract_violation );
 CHECK_THROWS( K2Tree( 3 , { { 0 , 3 } } ) , contract_violation );
 CHECK_THROWS( K2Tree( 3 , { { 3 , 0 } } ) , contract_violation );
 CHECK_THROWS( K2Tree( 3 , {} , 1 ) , contract_violation );
 }

// the lists of Boldi and Vigna: the copy lists and the copy blocks of nodes
// 16 and 18 of Tables 15.5 and 15.6, the intervals and the residuals of
// node 15 of Table 15.7 (whose last gap, 1034 - 315 - 1 = 718, the table
// prints as 178), the references chosen on those lists, and random graphs,
// local and not, decoded back; the empty graph, self-loops with a window
// and a chain of 1, a node pointing to all, the bounds of the queries, and
// a target past the nodes, w = 0, r = 0, lmin < 2
ADS_TEST( "bv-graph" )
{
 const BVGraph::List S15{ 13 , 15 , 16 , 17 , 18 , 19 , 23 , 24 , 203 ,
                          315 , 1034 };
 const BVGraph::List S16{ 15 , 16 , 17 , 22 , 23 , 24 , 315 , 316 , 317 ,
                          3041 };
 const BVGraph::List S18{ 13 , 15 , 16 , 17 , 50 };
 CHECK( BVGraph::copyList( S16 , S15 ).toString() == "01110011010" );
 CHECK( BVGraph::copyList( S18 , S15 ).toString() == "11110000000" );
 const auto b16 = BVGraph::copyBlocks( BVGraph::copyList( S16 , S15 ) );
 CHECK( ! b16.first && b16.second ==
        std::vector< std::size_t >( { 0 , 2 , 1 , 1 , 0 , 0 } ) );
 const auto b18 = BVGraph::copyBlocks( BVGraph::copyList( S18 , S15 ) );
 CHECK( b18.first && b18.second == std::vector< std::size_t >( { 3 } ) );
 const auto [ iv , res ] = BVGraph::intervals( S15 , 2 );
 CHECK( iv == std::vector< BVGraph::Interval >( { { 15 , 5 } ,
                                                  { 23 , 2 } } ) );
 CHECK( res == BVGraph::List( { 13 , 203 , 315 , 1034 } ) );
 CHECK( BVGraph::residualGaps( res , 15 ) ==
        std::vector< std::uint64_t >( { 5 , 189 , 111 , 718 } ) );
 std::vector< BVGraph::List > A( 3042 );
 A[ 15 ] = S15;
 A[ 16 ] = S16;
 A[ 18 ] = S18;
 const BVGraph T( A , 7 , 3 , 2 );
 CHECK( T.reference( 15 ) == 0 && T.reference( 16 ) == 1 );
 CHECK( T.reference( 18 ) == 3 && T.successors( 18 ) == S18 );
 CHECK( T.outdegree( 16 ) == 10 && T.outdegree( 17 ) == 0 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const std::size_t n = t < 2 ? std::size_t( t ) : 1 + g() % 1500;
  const Lists L = randomGraph( g , n , t % 2 ? 5 : 20 , t % 3 != 0 );
  const std::size_t w = 1 + g() % 8 , r = 1 + g() % 4 , lmin = 2 + g() % 4;
  const BVGraph G( L , w , r , lmin );
  std::size_t m = 0;
  for( std::size_t x = 0 ; x < n ; ++x ) {
   m += L[ x ].size();
   CHECK( G.successors( x ) == L[ x ] && G.outdegree( x ) == L[ x ].size() );
   std::size_t depth = 0;                   // the chain of references
   for( std::size_t y = x ; G.reference( y ) ; y -= G.reference( y ) )
    ++depth;
   CHECK( depth <= r && G.reference( x ) <= w );
   }
  CHECK( G.order() == n && G.edges() == m );
  if( ( t % 3 != 0 ) && ( m > 1000 ) )      // locality pays
   CHECK( double( G.codeBits() ) < 0.75 * double( m ) *
                                   std::log2( double( n ) ) );
  }
 const BVGraph E( std::vector< BVGraph::List >{} );   // empty, self-loops
 CHECK( E.order() == 0 && E.edges() == 0 );
 std::vector< BVGraph::List > Y( 50 );
 for( std::size_t x = 0 ; x < 50 ; ++x ) Y[ x ] = { x };
 Y[ 49 ] = BVGraph::List( 50 );
 std::iota( Y[ 49 ].begin() , Y[ 49 ].end() , std::size_t( 0 ) );
 const BVGraph G( Y , 1 , 1 , 2 );
 for( std::size_t x = 0 ; x < 50 ; ++x ) CHECK( G.successors( x ) == Y[ x ] );
 CHECK( G.edges() == 99 && G.outdegree( 49 ) == 50 );
 CHECK_THROWS( G.successors( 50 ) , contract_violation );
 CHECK_THROWS( G.outdegree( 50 ) , contract_violation );
 CHECK_THROWS( G.reference( 50 ) , contract_violation );
 CHECK_THROWS( BVGraph( { { 2 } , {} } ) , contract_violation );
 CHECK_THROWS( BVGraph( { {} } , 0 ) , contract_violation );
 CHECK_THROWS( BVGraph( { {} } , 1 , 0 ) , contract_violation );
 CHECK_THROWS( BVGraph( { {} } , 1 , 1 , 1 ) , contract_violation );
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
/*----------------------- End File pearls_succinct.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
