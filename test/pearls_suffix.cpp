/*--------------------------------------------------------------------------*/
/*------------------------- File pearls_suffix.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the suffix arrays, the suffix trees, the range minima and the
 * problems on strings of chapter 10 of Ferragina's notes: each one against
 * an independent oracle (the suffixes sorted as strings by the multikey
 * quicksort, std::search, the compact trie of Trie.h, the dynamic
 * programming of LongestCommonSubsequence.h, brute force), on the examples
 * of the notes with their numbers, on the edge cases and on random texts
 * with a fixed seed, at the level 3 of the contracts.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10
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

#include "ADS/dictionary/Trie.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/SuffixApplications.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixTree.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- ORACLES ----------------------------------*/
/*--------------------------------------------------------------------------*/

using Places = std::vector< std::size_t >;

/// the suffix array as the suffixes sorted as strings by the multikey
/// quicksort, each identified by its length
Places naiveSuffixArray( const std::string & T )
{
 std::vector< std::string > S;
 for( std::size_t i = 0 ; i < T.size() ; ++i ) S.push_back( T.substr( i ) );
 multikeyQuickSort( S );
 Places SA;
 for( const auto & s : S ) SA.push_back( T.size() - s.size() );
 return( SA );
 }

/// the lcp of the suffixes i and j, by comparing them
std::size_t naiveLcp( const std::string & T , std::size_t i , std::size_t j )
{
 std::size_t h = 0;
 while( ( i + h < T.size() ) && ( j + h < T.size() ) &&
        ( T[ i + h ] == T[ j + h ] ) )
  ++h;
 return( h );
 }

Places naiveLcpArray( const std::string & T , const Places & SA )
{
 Places L;
 for( std::size_t r = 0 ; r + 1 < SA.size() ; ++r )
  L.push_back( naiveLcp( T , SA[ r ] , SA[ r + 1 ] ) );
 return( L );
 }

/// the occurrences of P in T by std::search, increasing
Places occurrences( const std::string & T , const std::string & P )
{
 Places out;
 if( P.empty() ) {
  for( std::size_t i = 0 ; i < T.size() ; ++i ) out.push_back( i );
  return( out );
  }
 for( auto it = T.begin() ;
      ( it = std::search( it , T.end() , P.begin() , P.end() ) ) != T.end() ;
      ++it )
  out.push_back( std::size_t( it - T.begin() ) );
 return( out );
 }

Places sorted( Places v )
{
 std::sort( v.begin() , v.end() );
 return( v );
 }

/// a random text of n symbols among the first sigma letters
std::string randomText( std::mt19937 & g , std::size_t n , int sigma )
{
 std::string s( n , 'a' );
 for( auto & c : s ) c = char( 'a' + int( g() % unsigned( sigma ) ) );
 return( s );
 }

/// the Fibonacci word of at least n letters, cut to n
std::string fibonacciWord( std::size_t n )
{
 std::string a = "b" , b = "a";
 while( b.size() < n ) {
  std::string c = b + a;
  a = b;
  b = c;
  }
 return( b.substr( 0 , n ) );
 }

/// the texts of the tests: every binary text up to 10 letters, the
/// degenerate ones and random ones on several alphabets
std::vector< std::string > texts( void )
{
 std::vector< std::string > out;
 for( std::size_t n = 0 ; n <= 10 ; ++n )
  for( std::size_t mask = 0 ; mask < ( std::size_t( 1 ) << n ) ; ++mask ) {
   std::string s( n , 'a' );
   for( std::size_t i = 0 ; i < n ; ++i )
    if( ( mask >> i ) & 1 ) s[ i ] = 'b';
   out.push_back( s );
   }
 for( std::size_t n : { 50 , 257 , 1000 } ) {
  out.push_back( std::string( n , 'a' ) );
  std::string ab;
  while( ab.size() < n ) ab += "ab";
  out.push_back( ab );
  out.push_back( fibonacciWord( n ) );
  }
 out.push_back( "mississippi" );
 out.push_back( "abracadabra" );
 out.push_back( "CCGTACGATCAGTA" );
 out.push_back( "abxabyabz" );
 std::mt19937 g( 1 );
 for( int sigma : { 1 , 2 , 3 , 4 , 26 } )
  for( int t = 0 ; t < 12 ; ++t )
   out.push_back( randomText( g , g() % 400 , sigma ) );
 std::string bytes;                             // every byte, 0 included
 for( int c = 255 ; c >= 0 ; --c ) bytes.push_back( char( c ) );
 out.push_back( bytes + bytes );
 return( out );
 }

/// the leftmost minimum of A[ i .. j ]
template< class T >
std::size_t naiveMin( const std::vector< T > & A , std::size_t i ,
                      std::size_t j )
{
 std::size_t r = i;
 for( std::size_t k = i + 1 ; k <= j ; ++k )
  if( A[ k ] < A[ r ] ) r = k;
 return( r );
 }

/// the tree as the labels, depths and suffixes of its nodes in preorder
std::vector< std::tuple< std::string , std::size_t , std::size_t > >
                                 canonical( const SuffixTree & ST )
{
 std::vector< std::tuple< std::string , std::size_t , std::size_t > > out;
 for( std::size_t u : ST.preorder() ) {
  const auto & x = ST.node( u );
  std::string label;
  const std::size_t d = u == ST.root() ? 0 : ST.node( x.parent ).depth;
  for( std::size_t k = x.pos + d ; k < x.pos + x.depth ; ++k )
   label.push_back( char( ST.symbol( k ) ) );
  out.emplace_back( label , x.depth , x.suffix );
  }
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// Figure 10.1 (mississippi), Figure 10.2 (the range of ssi), Figure 10.9
// (na in banana at 3 and 5 of the notes, which count from 1) and Figure
// 10.12 (the lcp array of CCGTACGATCAGTA and lcp( X[ 2 , 14 ] , X[ 10 , 14 ]
// ) = 1); the notes add the suffix $, first in SA with lcp 0
ADS_TEST( "sec10.2.notes-examples" )
{
 const SuffixArray A( "mississippi" );
 CHECK( Places( A.begin() , A.end() ) ==
        Places( { 10 , 7 , 4 , 1 , 0 , 9 , 8 , 6 , 3 , 5 , 2 } ) );
 CHECK( A.lcp() == Places( { 1 , 1 , 4 , 0 , 0 , 1 , 0 , 2 , 1 , 3 } ) );
 const auto ssi = std::pair( std::size_t( 9 ) , std::size_t( 11 ) );
 CHECK( A.range( "ssi" ) == ssi );
 CHECK( A.rangeLcp( "ssi" ) == A.range( "ssi" ) );
 CHECK( sorted( A.locate( "siss" ) ) == Places( { 3 } ) );
 CHECK( sorted( SuffixArray( "banana" ).locate( "na" ) ) ==
        Places( { 2 , 4 } ) );
 CHECK( SuffixArray( "banana" ).count( "anas" ) == 0 );
 const SuffixArray X( "CCGTACGATCAGTA" );
 CHECK( X.lcp() ==
        Places( { 1 , 1 , 1 , 0 , 1 , 1 , 2 , 0 , 1 , 3 , 0 , 2 , 1 } ) );
 CHECK( X.lce( 1 , 9 ) == 1 );
 const SuffixArray E( "" );
 CHECK( E.size() == 0 && E.count( "a" ) == 0 && E.lcp().empty() );
 }

// the six constructions against the suffixes sorted by the multikey
// quicksort on all the texts, the scan-based one for several blocks, and
// Skew and SA-IS on integer alphabets as large as the text
ADS_TEST( "sec10.2.3.constructions" )
{
 for( const auto & T : texts() ) {
  const Places R = naiveSuffixArray( T );
  CHECK( suffixArraySorting( T ) == R );
  CHECK( suffixArrayDoubling( T ) == R );
  CHECK( suffixArrayLarssonSadakane( T ) == R );
  CHECK( suffixArraySkew( T ) == R );
  CHECK( suffixArraySais( T ) == R );
  if( T.size() <= 300 )
   for( std::size_t m : { std::size_t( 1 ) , std::size_t( 3 ) ,
                          std::size_t( 7 ) , T.size() + 1 } )
    CHECK( suffixArrayScan( T , m ) == R );
  }
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 40 ; ++t ) {
  const std::size_t n = g() % 600 , K = 1 + g() % 1000;
  Symbols S( n );
  for( auto & c : S ) c = g() % K;
  const Places R = suffixArraySorting( S );
  CHECK( suffixArraySkew( S , K ) == R );
  CHECK( suffixArraySais( S , K ) == R );
  CHECK( suffixArrayDoubling( S ) == R );
  CHECK( suffixArrayLarssonSadakane( S ) == R );
  }
 CHECK_THROWS( suffixArraySkew( Symbols( { 0 , 3 } ) , 3 ) ,
               contract_violation );
 CHECK_THROWS( suffixArraySais( Symbols( { 5 } ) , 2 ) , contract_violation );
 CHECK_THROWS( suffixArrayScan( "abc" , 0 ) , contract_violation );
 }

// the LCP array of Kasai et al. against the comparison of each adjacent
// pair, the lcp of any two suffixes against lce() (0 at the end of the
// text, n - i for a suffix with itself), rank() as the inverse of SA, and
// the bounds of lce(), rank() and operator[]
ADS_TEST( "sec10.2.2.kasai" )
{
 for( const auto & T : texts() ) {
  const Places SA = naiveSuffixArray( T );
  CHECK( lcpArray( T , SA ) == naiveLcpArray( T , SA ) );
  if( T.size() > 120 ) continue;
  const SuffixArray A( T );
  for( std::size_t r = 0 ; r < T.size() ; ++r )
   CHECK( A[ r ] == SA[ r ] && A.rank( SA[ r ] ) == r );
  for( std::size_t i = 0 ; i <= T.size() ; ++i )
   for( std::size_t j = 0 ; j <= T.size() ; ++j )
    CHECK( A.lce( i , j ) == naiveLcp( T , i , j ) );
  }
 const SuffixArray A( "aaaa" ) , One( "z" );
 CHECK( A.lce( 0 , 0 ) == 4 && A.lce( 0 , 1 ) == 3 && A.lce( 4 , 4 ) == 0 );
 CHECK( One.size() == 1 && One[ 0 ] == 0 && One.lcp().empty() &&
        One.lce( 0 , 0 ) == 1 && One.lce( 0 , 1 ) == 0 );
 CHECK( lcpArray( "" , Places() ).empty() );
 CHECK_THROWS( lcpArray( "ab" , Places( { 0 , 1 , 2 } ) ) ,
               contract_violation );
 CHECK_THROWS( A.lce( 5 , 0 ) , contract_violation );
 CHECK_THROWS( A.lce( 0 , 5 ) , contract_violation );
 CHECK_THROWS( A.rank( 4 ) , contract_violation );
 CHECK_THROWS( A[ 4 ] , contract_violation );
 CHECK_THROWS( SuffixArray( "" )[ 0 ] , contract_violation );
 }

// the two binary searches against std::search and against the keys with
// the prefix in the compact trie of the suffixes; Algorithm 10.1 within
// 2 ( p + 1 ) ( log2 n + 2 ) comparisons, Manber and Myers within
// 2 ( p + log2 n + 2 )
ADS_TEST( "sec10.2.1.search" )
{
 std::mt19937 g( 1 );
 for( const auto & T : texts() ) {
  if( T.size() > 1000 ) continue;
  const SuffixArray A( T );
  RadixTree< std::size_t > R;
  if( T.size() <= 300 )
   for( std::size_t i = 0 ; i < T.size() ; ++i ) R.insert( T.substr( i ) , i );
  const double lg = std::log2( double( T.size() ) + 1 );
  for( int q = 0 ; q < 30 ; ++q ) {
   std::string P;
   if( ( q % 2 == 0 ) && ! T.empty() ) {
    const std::size_t i = g() % T.size();
    P = T.substr( i , 1 + g() % 12 );
    }
   else
    P = randomText( g , g() % 6 , 3 );
   long c1 = 0 , c2 = 0;
   const auto r1 = A.range( P , &c1 ) , r2 = A.rangeLcp( P , &c2 );
   CHECK( r1 == r2 );
   const Places occ = occurrences( T , P );
   CHECK( r1.second - r1.first == occ.size() );
   CHECK( sorted( A.locate( P ) ) == occ );
   CHECK( double( c1 ) <= 2 * ( double( P.size() ) + 1 ) * ( lg + 2 ) );
   CHECK( double( c2 ) <= 2 * ( double( P.size() ) + lg + 2 ) );
   if( ( T.size() <= 300 ) && ! P.empty() ) {
    Places t;
    for( const auto & k : R.keysWithPrefix( P ) )
     t.push_back( T.size() - k.size() );
    CHECK( sorted( t ) == occ );
    }
   }
  }
 }

// the sparse table, the +-1 structure and Bender and Farach-Colton against
// the leftmost minimum by a scan on every range of small arrays with many
// ties, and on random ranges of large ones; all equal items (the leftmost
// wins), the extremes of int, one item, the bounds of the queries and of
// operator[], and an empty array; the Cartesian tree of Figure
// 10.13, and the query RMQ( 6 , 8 ) of Figure 10.14 on the lcp array of
// Figure 10.12, whose minima 1 are at 6 and 7 (from 1)
ADS_TEST( "sec10.4.1.range-minimum" )
{
 CHECK( cartesianTree( std::vector< int >( { 2 , 5 , 1 , 7 , 3 } ) ) ==
        Places( { 2 , 0 , 5 , 4 , 2 } ) );
 const std::vector< std::size_t > L = { 0 , 1 , 1 , 1 , 0 , 1 , 1 , 2 , 0 ,
                                        1 , 3 , 0 , 2 , 1 };
 const RangeMinimum< std::size_t > F( L );
 CHECK( F.query( 5 , 7 ) == 5 && F[ 5 ] == 1 );
 CHECK( F.eulerTour().size() == 2 * L.size() - 1 );
 CHECK( SparseTable< std::size_t >( L ).query( 5 , 7 ) == 5 );
 std::mt19937 g( 1 );
 for( std::size_t n = 1 ; n <= 70 ; ++n ) {
  std::vector< int > A( n );
  for( auto & x : A ) x = int( g() % ( n % 3 == 0 ? 3 : 50 ) );
  const SparseTable< int > S( A );
  const RangeMinimum< int > M( A );
  std::vector< long > D( n );                  // a +-1 walk
  for( std::size_t i = 1 ; i < n ; ++i )
   D[ i ] = D[ i - 1 ] + ( g() % 2 ? 1 : -1 );
  const PlusMinusOneRangeMinimum P( D );
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = i ; j < n ; ++j ) {
    CHECK( S.query( i , j ) == naiveMin( A , i , j ) );
    CHECK( M.query( i , j ) == naiveMin( A , i , j ) );
    CHECK( P.query( i , j ) == naiveMin( D , i , j ) );
    }
  }
 for( std::size_t n : { 1000 , 4096 , 20000 } ) {
  std::vector< double > A( n );
  for( auto & x : A ) x = double( g() % 1000 ) / 7;
  const SparseTable< double > S( A );
  const RangeMinimum< double > M( A );
  for( int q = 0 ; q < 300 ; ++q ) {
   std::size_t i = g() % n , j = g() % n;
   if( i > j ) std::swap( i , j );
   CHECK( S.query( i , j ) == naiveMin( A , i , j ) );
   CHECK( M.query( i , j ) == naiveMin( A , i , j ) );
   }
  }
 const std::vector< int > G = { 5 , 4 , 3 , 2 , 1 };   // a decreasing chain
 const RangeMinimum< int , std::greater< int > > Mx( G );
 CHECK( Mx.query( 1 , 4 ) == 1 );
 CHECK( RangeMinimum< int >().size() == 0 );
 for( std::size_t n : { 1 , 2 , 7 , 300 } ) {
  const std::vector< int > Q( n , 7 );          // all equal: the leftmost
  const SparseTable< int > S( Q );
  const RangeMinimum< int > M( Q );
  const PlusMinusOneRangeMinimum P( [ n ] {
   std::vector< long > D( n );                  // a zigzag with ties
   for( std::size_t i = 0 ; i < n ; ++i ) D[ i ] = long( i % 2 );
   return( D ); }() );
  for( std::size_t i = 0 ; i < n ; i += 1 + n / 10 )
   for( std::size_t j = i ; j < n ; j += 1 + n / 10 ) {
    CHECK( S.query( i , j ) == i && M.query( i , j ) == i );
    CHECK( P.query( i , j ) == i + i % 2 - ( i == j && i % 2 ? 1 : 0 ) );
    }
  }
 const std::vector< int > X = { std::numeric_limits< int >::max() ,
                                std::numeric_limits< int >::min() ,
                                0 , std::numeric_limits< int >::min() };
 const RangeMinimum< int > MX( X );
 CHECK( MX.query( 0 , 3 ) == 1 && MX.query( 2 , 3 ) == 3 &&
        MX.query( 0 , 0 ) == 0 && SparseTable< int >( X ).query( 0 , 3 ) ==
        1 );
 CHECK_THROWS( MX.query( 2 , 1 ) , contract_violation );
 CHECK_THROWS( MX.query( 0 , 4 ) , contract_violation );
 CHECK_THROWS( MX[ 4 ] , contract_violation );
 CHECK_THROWS( SparseTable< int >( X ).query( 3 , 4 ) , contract_violation );
 CHECK_THROWS( SparseTable< int >( X )[ 4 ] , contract_violation );
 CHECK_THROWS( PlusMinusOneRangeMinimum( std::vector< long >( { 0 , 1 } ) )
               .query( 0 , 2 ) , contract_violation );
 CHECK_THROWS( RangeMinimum< int >().query( 0 , 0 ) , contract_violation );
 CHECK_THROWS( SparseTable< int >( std::vector< int >( { 1 , 2 } ) ).query(
               1 , 0 ) , contract_violation );
 CHECK_THROWS( PlusMinusOneRangeMinimum( std::vector< long >( { 0 , 2 } ) ) ,
               contract_violation );
 }

// the three constructions of the suffix tree give the same tree, whose SA
// and LCP read back are those of the suffix array; Figure 10.8 has 7
// internal nodes on mississippi$, z = ssi is the extended locus of ss and
// x = issi occurs at 2 and 5 (from 1)
ADS_TEST( "sec10.3.constructions" )
{
 const SuffixTree M( "mississippi" );
 CHECK( M.size() == 19 );
 CHECK( M.node( M.locus( "ss" ) ).depth == 3 );
 CHECK( M.node( M.locus( "issi" ) ).leaves == 2 );
 CHECK( sorted( M.locate( "issi" ) ) == Places( { 1 , 4 } ) );
 for( const auto & T : texts() ) {
  if( T.size() > 1000 ) continue;
  const SuffixTree a( T , SuffixTree::Build::naive ) ,
                   b( T , SuffixTree::Build::mcCreight ) ,
                   c( T , SuffixTree::Build::suffixArray );
  CHECK( a.size() <= std::max< std::size_t >( 2 , 2 * T.size() + 1 ) );
  CHECK( canonical( a ) == canonical( b ) );
  CHECK( canonical( a ) == canonical( c ) );
  const Places SA = naiveSuffixArray( T );
  CHECK( b.toSuffixArray() == std::pair( SA , naiveLcpArray( T , SA ) ) );
  }
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const std::size_t n = g() % 300 , K = 1 + g() % 500;
  Symbols S( n );
  for( auto & x : S ) x = g() % K;
  const SuffixTree b( S , K ) , c( S , K , SuffixTree::Build::suffixArray );
  CHECK( b.size() == c.size() );
  CHECK( b.toSuffixArray().first == suffixArraySorting( S ) );
  CHECK( c.toSuffixArray().first == suffixArraySorting( S ) );
  }
 CHECK_THROWS( SuffixTree( Symbols( { 4 } ) , 4 ) , contract_violation );
 }

// the search by the suffix tree against std::search, the lowest common
// ancestors of the leaves against the lcp of the suffixes (Property 10.9),
// and of any two nodes against the walk up to the root; the empty pattern
// (the root, n places), the pattern equal to the text and longer than it,
// the empty text, and the bounds of node(), leaf(), symbol(), lca(), lce()
ADS_TEST( "sec10.3.1.search-lca" )
{
 const SuffixTree B( "banana" );
 CHECK( sorted( B.locate( "na" ) ) == Places( { 2 , 4 } ) );
 CHECK( B.locus( "anas" ) == SuffixTree::npos && B.count( "anas" ) == 0 );
 CHECK( B.locus( "" ) == B.root() && B.count( "" ) == 6 );
 CHECK( sorted( B.locate( "" ) ) == Places( { 0 , 1 , 2 , 3 , 4 , 5 } ) );
 CHECK( B.count( "banana" ) == 1 && B.locate( "banana" ) == Places( { 0 } ) );
 CHECK( B.count( "bananas" ) == 0 && B.locate( "bananab" ).empty() );
 CHECK( B.lca( B.root() , B.leaf( 0 ) ) == B.root() );
 CHECK( B.lca( B.leaf( 3 ) , B.leaf( 3 ) ) == B.leaf( 3 ) );
 CHECK( B.lce( 6 , 6 ) == 0 && B.lce( 2 , 2 ) == 4 );
 const SuffixTree E( "" );
 CHECK( E.textSize() == 0 && E.count( "" ) == 0 && E.count( "a" ) == 0 &&
        E.locate( "" ).empty() && E.lce( 0 , 0 ) == 0 );
 CHECK_THROWS( B.node( B.size() ) , contract_violation );
 CHECK_THROWS( B.leaf( 7 ) , contract_violation );
 CHECK_THROWS( B.symbol( 7 ) , contract_violation );
 CHECK_THROWS( B.lca( 0 , B.size() ) , contract_violation );
 CHECK_THROWS( B.lce( 7 , 0 ) , contract_violation );
 CHECK_THROWS( B.lce( 0 , 7 ) , contract_violation );
 std::mt19937 g( 1 );
 for( const auto & T : texts() ) {
  if( T.size() > 400 ) continue;
  const SuffixTree ST( T );
  for( int q = 0 ; q < 20 ; ++q ) {
   std::string P;
   if( ( q % 2 == 0 ) && ! T.empty() )
    P = T.substr( g() % T.size() , 1 + g() % 10 );
   else
    P = randomText( g , g() % 5 , 3 );
   const Places occ = occurrences( T , P );
   CHECK( ST.count( P ) == occ.size() );
   CHECK( sorted( ST.locate( P ) ) == occ );
   }
  if( T.size() > 60 ) continue;
  for( std::size_t i = 0 ; i <= T.size() ; ++i )
   for( std::size_t j = 0 ; j <= T.size() ; ++j )
    CHECK( ST.lce( i , j ) == naiveLcp( T , i , j ) );
  for( int q = 0 ; q < 50 ; ++q ) {
   const std::size_t u = g() % ST.size() , v = g() % ST.size();
   std::set< std::size_t > up;
   for( std::size_t x = u ; x != SuffixTree::npos ; x = ST.node( x ).parent )
    up.insert( x );
   std::size_t w = v;
   while( ! up.count( w ) ) w = ST.node( w ).parent;
   CHECK( ST.lca( u , v ) == w );
   }
  }
 }

// the parsing of mississippi of section 10.4.2, and on all the texts the
// parsing that copies from the leftmost of the longest earlier matches,
// found by brute force, and the decoding; the empty text, one symbol, a
// phrase that copies nothing, and the two preconditions of the decoding
ADS_TEST( "sec10.4.2.lz77" )
{
 auto ph = []( std::size_t d , std::size_t l , char c ) {
  CopyPhrase z;
  z.distance = d;
  z.length = l;
  z.next = c;
  return( z );
  };
 CHECK( lz77ByTree( "mississippi" ) ==
        std::vector< CopyPhrase >( { ph( 0 , 0 , 'm' ) , ph( 0 , 0 , 'i' ) ,
                                     ph( 0 , 0 , 's' ) , ph( 1 , 1 , 'i' ) ,
                                     ph( 3 , 3 , 'p' ) ,
                                     ph( 1 , 1 , 'i' ) } ) );
 const auto Z = lz77ByTree( "aaaaa" );          // a copy that overlaps
 CHECK( Z.size() == 2 && Z[ 1 ].distance == 1 && Z[ 1 ].length == 4 &&
        ! Z[ 1 ].next );
 for( const auto & T : texts() ) {
  if( T.size() > 400 ) continue;
  std::vector< CopyPhrase > R;
  for( std::size_t i = 0 ; i < T.size() ; ) {
   std::size_t best = 0 , at = 0;
   for( std::size_t j = 0 ; j < i ; ++j ) {
    const std::size_t l = naiveLcp( T , j , i );
    if( l > best ) {
     best = l;
     at = j;
     }
    }
   CopyPhrase z;
   z.length = best;
   z.distance = best ? i - at : 0;
   if( i + best < T.size() ) z.next = T[ i + best ];
   R.push_back( z );
   i += best + 1;
   }
  CHECK( lz77ByTree( T ) == R );
  CHECK( decodeCopyPhrases( R ) == T );
  }
 CHECK( lz77ByTree( "" ).empty() && decodeCopyPhrases( {} ).empty() );
 CHECK( lz77ByTree( "x" ) ==
        std::vector< CopyPhrase >( { ph( 0 , 0 , 'x' ) } ) );
 CHECK( decodeCopyPhrases( { ph( 0 , 0 , 'a' ) , ph( 1 , 0 , 'b' ) } ) ==
        "ab" );                                 // a copy of nothing
 CHECK_THROWS( decodeCopyPhrases( { ph( 1 , 1 , 'a' ) } ) ,
               contract_violation );
 CHECK_THROWS( decodeCopyPhrases( { ph( 0 , 0 , 'a' ) , ph( 0 , 1 , 'b' ) } ) ,
               contract_violation );
 }

// the longest repeated substring, the repeats of length L at least C
// times, the distinct substrings and the proximity search against brute
// force; the proximity of a pattern with itself, at distance 0 and of an
// absent pattern, the repeats longer than the text and of the empty string
ADS_TEST( "sec10.4.3.text-mining" )
{
 std::mt19937 g( 1 );
 for( const auto & T : texts() ) {
  if( T.size() > 120 ) continue;
  const SuffixArray A( T );
  const std::size_t n = T.size();
  std::map< std::string , std::size_t > cnt;
  std::set< std::string > all;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t l = 1 ; i + l <= n ; ++l ) {
    ++cnt[ T.substr( i , l ) ];
    all.insert( T.substr( i , l ) );
    }
  CHECK( distinctSubstrings( A ) == all.size() );
  std::size_t lrs = 0;
  for( const auto & [ s , c ] : cnt )
   if( c >= 2 ) lrs = std::max( lrs , s.size() );
  const auto [ pos , len ] = longestRepeatedSubstring( A );
  CHECK( len == lrs );
  CHECK( occurrences( T , T.substr( pos , len ) ).size() >= ( len ? 2 : 0 ) );
  for( std::size_t L = 0 ; L <= std::min< std::size_t >( n + 1 , 6 ) ; ++L )
   for( std::size_t C = 1 ; C <= 5 ; ++C ) {
    bool exists = false;
    if( L == 0 ) exists = C <= n + 1;
    else
     for( const auto & [ s , c ] : cnt )
      if( ( s.size() == L ) && ( c >= C ) ) exists = true;
    const auto r = repeatedSubstring( A , L , C );
    CHECK( r.has_value() == exists );
    if( r && ( L > 0 ) )
     CHECK( occurrences( T , T.substr( *r , L ) ).size() >= C );
    }
  for( int q = 0 ; q < 10 ; ++q ) {
   const std::string P = randomText( g , 1 + g() % 2 , 3 ) ,
                     Q = randomText( g , 1 + g() % 2 , 3 );
   const std::size_t k = g() % 6;
   bool near = false;
   for( std::size_t i : occurrences( T , P ) )
    for( std::size_t j : occurrences( T , Q ) )
     if( ( i > j ? i - j : j - i ) <= k ) near = true;
   CHECK( proximity( A , P , Q , k ) == near );
   }
  }
 const SuffixArray A( "abab" );               // P = Q, k = 0, too long
 CHECK( proximity( A , "ab" , "ab" , 0 ) && ! proximity( A , "a" , "b" , 0 ) );
 CHECK( proximity( A , "a" , "b" , 1 ) && ! proximity( A , "abc" , "a" , 9 ) );
 CHECK( ! repeatedSubstring( A , 5 , 1 ) && repeatedSubstring( A , 4 , 1 ) );
 CHECK( ! repeatedSubstring( A , 0 , 6 ) && repeatedSubstring( A , 0 , 5 ) );
 CHECK_THROWS( repeatedSubstring( SuffixArray( "ab" ) , 1 , 0 ) ,
               contract_violation );
 }

// the longest substring common to k of m strings: for two strings as long
// as the dynamic programming of LongestCommonSubsequence.h, for more as
// long as the longest substring of the first one in k of them; an empty
// string, two equal ones, two disjoint ones, and k = 0, k > m, m = 0
ADS_TEST( "sec10.4.common-substring" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t m = 2 + g() % 3;
  std::vector< std::string > S( m );
  for( auto & s : S ) s = randomText( g , g() % 30 , 1 + int( g() % 3 ) );
  if( m == 2 ) {
   const auto [ len , ei , ej ] = longestCommonSubstring( S[ 0 ] , S[ 1 ] );
   CHECK( longestCommonSubstring( S , 2 ).size() == len );
   }
  for( std::size_t k = 1 ; k <= m ; ++k ) {
   std::size_t best = 0;
   for( std::size_t a = 0 ; a < m ; ++a )      // every substring of all
    for( std::size_t i = 0 ; i < S[ a ].size() ; ++i )
     for( std::size_t l = best + 1 ; i + l <= S[ a ].size() ; ++l ) {
      const std::string x = S[ a ].substr( i , l );
      std::size_t in = 0;
      for( const auto & s : S ) in += s.find( x ) != std::string::npos;
      if( in >= k ) best = l;
      }
   CHECK( longestCommonSubstring( S , k ).size() == best );
   }
  }
 CHECK( longestCommonSubstring( { "xabcy" , "zabcw" , "abq" } , 3 ) == "ab" );
 CHECK( longestCommonSubstring( { "xabcy" , "zabcw" , "abq" } , 2 ) ==
        "abc" );
 CHECK( longestCommonSubstring( { "" , "abc" } , 2 ).empty() );
 CHECK( longestCommonSubstring( { "" , "abc" } , 1 ) == "abc" );
 CHECK( longestCommonSubstring( { "abc" , "abc" } , 2 ) == "abc" );
 CHECK( longestCommonSubstring( { "aaa" , "bbb" } , 2 ).empty() );
 CHECK_THROWS( longestCommonSubstring( { "a" } , 2 ) , contract_violation );
 CHECK_THROWS( longestCommonSubstring( { "a" , "b" } , 0 ) ,
               contract_violation );
 CHECK_THROWS( longestCommonSubstring( std::vector< std::string >() , 1 ) ,
               contract_violation );
 }

// the maximal repeats against brute force: the substrings that occur
// twice, with two occurrences followed by distinct symbols (or the end)
// and two preceded by distinct symbols (or the start)
ADS_TEST( "sec10.4.maximal-repeats" )
{
 for( const auto & T : texts() ) {
  if( T.size() > 60 ) continue;
  std::map< std::string , Places > occ;
  for( std::size_t i = 0 ; i < T.size() ; ++i )
   for( std::size_t l = 1 ; i + l <= T.size() ; ++l )
    occ[ T.substr( i , l ) ].push_back( i );
  std::set< std::string > R;
  for( const auto & [ s , O ] : occ ) {
   if( O.size() < 2 ) continue;
   std::set< int > before , after;
   for( std::size_t i : O ) {
    before.insert( i == 0 ? -1 : int( ( unsigned char ) T[ i - 1 ] ) );
    after.insert( i + s.size() == T.size() ? -1 :
                  int( ( unsigned char ) T[ i + s.size() ] ) );
    }
   if( ( before.size() > 1 ) && ( after.size() > 1 ) ) R.insert( s );
   }
  std::set< std::string > got;
  for( const auto & [ p , l ] : maximalRepeats( SuffixTree( T ) ) )
   got.insert( T.substr( p , l ) );
  CHECK( got == R );
  }
 }

// the k mismatches of Algorithm 10.4 against the count of the
// substitutions at every place; Figure 10.11 has P at 1 (from 1) with 2
// mismatches; P longer than T, equal to T, k = | P |, P and T empty
ADS_TEST( "sec10.4.1.k-mismatches" )
{
 const Places F = kMismatches( "CCGTACGATCAGTA" , "CCGAACT" , 2 );
 CHECK( std::find( F.begin() , F.end() , 0 ) != F.end() );
 CHECK( kMismatches( "CCGTACGATCAGTA" , "CCGAACT" , 1 ).empty() );
 CHECK( kMismatches( "ab" , "abc" , 5 ).empty() );     // P longer than T
 CHECK( kMismatches( "abc" , "abc" , 0 ) == Places( { 0 } ) );
 CHECK( kMismatches( "abc" , "xyz" , 3 ) == Places( { 0 } ) );
 CHECK( kMismatches( "abc" , "" , 0 ) == Places( { 0 , 1 , 2 , 3 } ) );
 CHECK( kMismatches( "" , "" , 0 ) == Places( { 0 } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int sigma = 1 + int( g() % 4 );
  const std::string T = randomText( g , g() % 60 , sigma ) ,
                    P = randomText( g , g() % 8 , sigma );
  const std::size_t k = g() % 4;
  Places R;
  for( std::size_t i = 0 ; i + P.size() <= T.size() ; ++i ) {
   std::size_t m = 0;
   for( std::size_t j = 0 ; j < P.size() ; ++j ) m += T[ i + j ] != P[ j ];
   if( m <= k ) R.push_back( i );
   }
  CHECK( kMismatches( T , P , k ) == R );
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
/*----------------------- End File pearls_suffix.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
