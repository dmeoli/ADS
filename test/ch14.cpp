/*--------------------------------------------------------------------------*/
/*----------------------------- File ch14.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 14, Greedy: the change with coins, the real knapsack
 * and the counterexamples of the text, the independent set of intervals
 * against the weighted one, Kruskal and Prim on Figure 14.1-14.3 and
 * against all the spanning trees, the Teorema 14.1, the code of Huffman
 * on the example of the text, and the exercises 14.1 to 14.10, each a test
 * named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 14,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
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

#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/optimization/CoinChange.h"
#include "ADS/optimization/FractionalKnapsack.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/Knapsack.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/string/Huffman.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;
using Edge = WeightedEdge< long >;

const Frontier fronts[] = { Frontier::LIST , Frontier::HEAP ,
                            Frontier::FIBONACCI };

/// Figura 14.1, the nodes from 1 (node 0 alone): the edges in the order of
/// the example of Kruskal, by increasing weight
const std::vector< Edge > fig141 = { { 2 , 4 , 1 } , { 3 , 4 , 2 } ,
 { 5 , 7 , 3 } , { 1 , 3 , 6 } , { 1 , 2 , 9 } , { 2 , 3 , 11 } ,
 { 4 , 5 , 13 } , { 4 , 7 , 15 } , { 3 , 6 , 18 } , { 2 , 5 , 20 } ,
 { 6 , 7 , 25 } , { 4 , 6 , 28 } };

ListGraph< long > graph( int n , const std::vector< Edge > & E )
{
 ListGraph< long > g( n , false );
 for( const auto & e : E )
  g.insertEdge( e.u , e.v , e.w );
 return( g );
 }

long weight( const std::vector< Edge > & T )
{
 long w = 0;
 for( const auto & e : T ) w += e.w;
 return( w );
 }

/// the weight of a minimum spanning tree by trying all the sets of n - 1
/// edges, infinity if not connected
long brute_mst( int n , const std::vector< Edge > & E )
{
 long best = infinity< long >();
 const int m = int( E.size() );
 for( int mask = 0 ; mask < ( 1 << m ) ; ++mask ) {
  if( std::popcount( unsigned( mask ) ) != n - 1 ) continue;
  MFSet M( n );
  long w = 0;
  bool ok = true;
  for( int i = 0 ; i < m && ok ; ++i )
   if( mask >> i & 1 ) {
    ok = M.merge( E[ i ].u , E[ i ].v );
    w += E[ i ].w;
    }
  if( ok ) best = std::min( best , w );
  }
 return( best );
 }

/// a random connected graph of n nodes: a random tree plus more edges
std::vector< Edge > random_edges( int n , int extra , int maxw ,
                                  std::mt19937 & g )
{
 std::vector< Edge > E;
 std::map< std::pair< int , int > , bool > seen;
 auto add = [ & ]( int u , int v ) {
  if( u == v || seen[ { std::min( u , v ) , std::max( u , v ) } ] ) return;
  seen[ { std::min( u , v ) , std::max( u , v ) } ] = true;
  E.push_back( { u , v , long( g() % maxw ) } );
  };
 for( int v = 1 ; v < n ; ++v ) add( int( g() % v ) , v );
 for( int k = 0 ; k < extra ; ++k ) add( int( g() % n ) , int( g() % n ) );
 return( E );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the change of 72 cents with coins of 50, 10, 5, 1: five coins, optimal
// for every amount; with coins of 11, 5, 1 the greedy choice pays 15 with
// five coins, the optimum is three; the edges c = 0, no coin, coins
// repeated or larger than c, and the preconditions
ADS_TEST( "sec14.change" )
{
 CHECK( *greedyChange( { 50 , 10 , 5 , 1 } , 72 ) ==
        Longs( { 50 , 10 , 10 , 1 , 1 } ) );
 for( long c = 0 ; c <= 300 ; ++c )
  CHECK( long( greedyChange( { 1 , 5 , 10 , 50 } , c )->size() ) ==
         *optimalChange( { 1 , 5 , 10 , 50 } , c ) );
 CHECK( greedyChange( { 11 , 5 , 1 } , 15 )->size() == 5 );
 CHECK( *optimalChange( { 11 , 5 , 1 } , 15 ) == 3 );
 CHECK( ! greedyChange( { 5 , 3 } , 7 ) && ! optimalChange( { 5 , 3 } , 7 ) );
 // the edges: c = 0 is paid by no coin, even with no coin at all; with no
 // coin nothing else is paid; the coins in any order and repeated; a coin
 // larger than c is skipped; the coin 1 alone pays c with c coins
 CHECK( greedyChange( { 50 , 10 } , 0 )->empty() &&
        *optimalChange( { 50 , 10 } , 0 ) == 0 );
 CHECK( greedyChange( {} , 0 )->empty() && *optimalChange( {} , 0 ) == 0 );
 CHECK( ! greedyChange( {} , 3 ) && ! optimalChange( {} , 3 ) );
 CHECK( *greedyChange( { 1 , 10 , 5 , 10 } , 27 ) ==
        Longs( { 10 , 10 , 5 , 1 , 1 } ) );
 CHECK( *optimalChange( { 1 , 10 , 5 , 10 } , 27 ) == 5 );
 CHECK( *greedyChange( { 100 , 3 } , 9 ) == Longs( { 3 , 3 , 3 } ) );
 CHECK( ! greedyChange( { 100 } , 99 ) && ! optimalChange( { 100 } , 99 ) );
 CHECK( greedyChange( { 1 } , 1000 )->size() == 1000 &&
        *optimalChange( { 1 } , 1000 ) == 1000 );
 // the greedy choice may fail where the optimum pays: 6 with 4 , 3
 CHECK( ! greedyChange( { 4 , 3 } , 6 ) && *optimalChange( { 4 , 3 } , 6 ) ==
        2 );
 CHECK_THROWS( greedyChange( { 5 , 1 } , -1 ) , contract_violation );
 CHECK_THROWS( greedyChange( { 5 , 0 } , 3 ) , contract_violation );
 CHECK_THROWS( optimalChange( { 5 , 1 } , -1 ) , contract_violation );
 CHECK_THROWS( optimalChange( { -5 , 1 } , 3 ) , contract_violation );
 }

// the real knapsack: by profit, 3 against the optimum 4; by volume, 2
// against 3; by specific profit, the optimum, against the bound of the
// linear relaxation found by trying every last object in part; the edges
// of no object, of capacity 0 or above the total, of ties, and the
// preconditions
ADS_TEST( "sec14.1.real-knapsack" )
{
 auto profit = []( const std::vector< double > & p ,
                   const std::vector< double > & x ) {
  return( std::inner_product( p.begin() , p.end() , x.begin() , 0.0 ) );
  };
 const std::vector< double > p1 = { 3 , 2 , 2 } , v1 = { 2 , 1 , 1 };
 const auto x1 = fractionalKnapsack( p1 , v1 , 2 );
 CHECK( std::abs( profit( p1 , x1 ) - 4 ) < 1e-9 );
 const std::vector< double > p2 = { 1 , 1 , 3 } , v2 = { 1 , 1 , 2 };
 const auto x2 = fractionalKnapsack( p2 , v2 , 2 );
 CHECK( std::abs( profit( p2 , x2 ) - 3 ) < 1e-9 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 8 );
  std::vector< double > p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + g() % 20;
   v[ i ] = 1 + g() % 10;
   }
  const double C = double( g() % 30 );
  const auto x = fractionalKnapsack( p , v , C );
  double vol = 0;
  for( int i = 0 ; i < n ; ++i ) {
   CHECK( x[ i ] >= -1e-12 && x[ i ] <= 1 + 1e-12 );
   vol += v[ i ] * x[ i ];
   }
  CHECK( vol <= C + 1e-9 );
  // the optimum: some set whole and one more object in part
  double best = 0;
  for( int m = 0 ; m < ( 1 << n ) ; ++m ) {
   double pp = 0 , vv = 0;
   for( int i = 0 ; i < n ; ++i )
    if( m >> i & 1 ) {
     pp += p[ i ];
     vv += v[ i ];
     }
   if( vv > C ) continue;
   best = std::max( best , pp );
   for( int j = 0 ; j < n ; ++j )
    if( ! ( m >> j & 1 ) )
     best = std::max( best ,
                      pp + p[ j ] * std::min( 1.0 , ( C - vv ) / v[ j ] ) );
   }
  CHECK( std::abs( profit( p , x ) - best ) < 1e-6 );
  }
 // the edges: no object; capacity 0 takes nothing; a capacity above the
 // total volume takes everything; an exact fit takes the last one whole;
 // equal specific profits fill the first object first; an object of
 // profit 0 is taken last
 CHECK( fractionalKnapsack( {} , {} , 5 ).empty() );
 CHECK( fractionalKnapsack( { 3 , 2 } , { 1 , 1 } , 0 ) ==
        std::vector< double >( { 0 , 0 } ) );
 CHECK( fractionalKnapsack( { 3 , 2 , 1 } , { 1 , 2 , 3 } , 1e9 ) ==
        std::vector< double >( { 1 , 1 , 1 } ) );
 CHECK( fractionalKnapsack( { 4 , 3 } , { 2 , 3 } , 5 ) ==
        std::vector< double >( { 1 , 1 } ) );
 CHECK( fractionalKnapsack( { 2 , 4 , 6 } , { 1 , 2 , 3 } , 2 ) ==
        std::vector< double >( { 1 , 0.5 , 0 } ) );
 CHECK( fractionalKnapsack( { 0 , 1 } , { 1 , 1 } , 1.5 ) ==
        std::vector< double >( { 0.5 , 1 } ) );
 CHECK_THROWS( fractionalKnapsack( { 1 } , { 1 , 2 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( fractionalKnapsack( { 1 } , { 0 } , 1 ) , contract_violation );
 CHECK_THROWS( fractionalKnapsack( { -1 } , { 1 } , 1 ) , contract_violation );
 CHECK_THROWS( fractionalKnapsack( { 1 } , { 1 } , -1 ) , contract_violation );
 }

// the greedy independent set has as many intervals as the weighted one
// with all the weights 1; the edges of no interval, equal, touching and
// nested ones, the extremes of long, and the precondition
ADS_TEST( "sec14.2.intervals" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = int( g() % 30 );
  std::vector< std::pair< long , long > > I( n );
  std::vector< WeightedInterval< long > > J( n );
  for( int i = 0 ; i < n ; ++i ) {
   I[ i ].first = long( g() % 50 );
   I[ i ].second = I[ i ].first + 1 + long( g() % 10 );
   J[ i ] = { I[ i ].first , I[ i ].second , 1 };
   }
  const auto S = independentSet( I );
  CHECK( long( S.size() ) == maxinterval( J ).second );
  for( std::size_t x = 0 ; x < S.size() ; ++x )
   for( std::size_t y = x + 1 ; y < S.size() ; ++y )
    CHECK( I[ S[ x ] ].second <= I[ S[ y ] ].first ||
           I[ S[ y ] ].second <= I[ S[ x ] ].first );
  }
 // the edges: no interval; one; all equal, one of them, the first; the
 // intervals [ a , b ) that touch are disjoint; nested ones, the inner;
 // the extremes of long
 using I2 = std::vector< std::pair< long , long > >;
 using Places = std::vector< std::size_t >;
 CHECK( independentSet( I2() ).empty() );
 CHECK( independentSet( I2( { { 3 , 4 } } ) ) == Places( { 0 } ) );
 CHECK( independentSet( I2( 5 , { 1 , 9 } ) ) == Places( { 0 } ) );
 CHECK( independentSet( I2( { { 2 , 3 } , { 0 , 1 } , { 1 , 2 } } ) ) ==
        Places( { 0 , 1 , 2 } ) );
 CHECK( independentSet( I2( { { 0 , 10 } , { 4 , 5 } } ) ) ==
        Places( { 1 } ) );
 const long lo = std::numeric_limits< long >::min() ,
            hi = std::numeric_limits< long >::max();
 CHECK( independentSet( I2( { { lo , 0 } , { 0 , hi } , { lo , hi } } ) ) ==
        Places( { 0 , 1 } ) );
 CHECK_THROWS( independentSet( I2( { { 1 , 2 } , { 3 , 3 } } ) ) ,
               contract_violation );
 CHECK_THROWS( independentSet( I2( { { 4 , 3 } } ) ) , contract_violation );
 }

// Figura 14.2: Kruskal keeps [ 2 , 4 ], [ 3 , 4 ], [ 5 , 7 ], [ 1 , 3 ],
// [ 4 , 5 ], [ 3 , 6 ], and leaves [ 1 , 2 ], [ 2 , 3 ], [ 4 , 7 ]: weight
// 43; Figura 14.3: Prim from 1 gives the same tree; against all the sets
// of n - 1 edges on random graphs; the edges of 0 and 1 node, self-loops,
// parallel edges, forests, negative and equal weights, and the
// preconditions
ADS_TEST( "sec14.3.mst" )
{
 const std::vector< Edge > T = kruskal( 8 , fig141 );
 CHECK( T == std::vector< Edge >( { { 2 , 4 , 1 } , { 3 , 4 , 2 } ,
                                    { 5 , 7 , 3 } , { 1 , 3 , 6 } ,
                                    { 4 , 5 , 13 } , { 3 , 6 , 18 } } ) );
 CHECK( weight( T ) == 43 );
 const auto g = graph( 8 , fig141 );
 for( Frontier f : fronts ) {
  const auto p = prim( g , 1 , f );
  CHECK( p == Ints( { NIL , NIL , 4 , 1 , 3 , 4 , 3 , 5 } ) );
  }
 std::mt19937 gen( 3 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 2 + int( gen() % 6 );
  const auto E = random_edges( n , int( gen() % 8 ) , 20 , gen );
  const long best = brute_mst( n , E );
  CHECK( weight( kruskal( n , E ) ) == best );
  const auto h = graph( n , E );
  for( Frontier f : fronts ) {
   const auto p = prim( h , 0 , f );
   long w = 0;
   for( int v = 1 ; v < n ; ++v ) w += *h.weight( p[ v ] , v );
   CHECK( w == best );
   }
  }
 // the edges: no node and one node give no edge; the self-loops are never
 // taken and of two parallel edges the lighter is; a disconnected graph
 // gives a forest of n minus the components edges, and prim() leaves NIL
 // out of the component of r; negative and equal weights
 using Es = std::vector< Edge >;
 CHECK( kruskal( 0 , Es() ).empty() && kruskal( 1 , Es() ).empty() );
 CHECK( kruskal( 1 , Es( { { 0 , 0 , -5 } } ) ).empty() );
 CHECK( kruskal( 2 , Es( { { 0 , 0 , -9 } , { 0 , 1 , 7 } , { 1 , 0 , 3 } ,
                           { 1 , 1 , -9 } } ) ) == Es( { { 1 , 0 , 3 } } ) );
 const Es forest = { { 0 , 1 , 4 } , { 1 , 2 , -3 } , { 0 , 2 , 1 } ,
                     { 3 , 4 , 2 } , { 3 , 4 , 2 } };
 const auto F = kruskal( 6 , forest );                // 5 isolated
 CHECK( F.size() == 3 && weight( F ) == 0 &&
        isMinimumSpanningForest( 6 , forest , F ) );
 CHECK( ! isMinimumSpanningForest( 6 , forest , Es( { { 0 , 1 , 4 } ,
                                   { 1 , 2 , -3 } , { 3 , 4 , 2 } } ) ) );
 const Es equal = { { 0 , 1 , 5 } , { 1 , 2 , 5 } , { 2 , 0 , 5 } ,
                    { 2 , 3 , 5 } };
 CHECK( kruskal( 4 , equal ) == Es( { { 0 , 1 , 5 } , { 1 , 2 , 5 } ,
                                      { 2 , 3 , 5 } } ) );
 const auto gf = graph( 6 , forest );
 for( Frontier f : fronts ) {
  CHECK( prim( graph( 1 , Es() ) , 0 , f ) == Ints( { NIL } ) );
  CHECK( prim( gf , 0 , f ) == Ints( { NIL , 2 , 0 , NIL , NIL , NIL } ) );
  CHECK( prim( gf , 4 , f ) == Ints( { NIL , NIL , NIL , 4 , NIL , NIL } ) );
  CHECK( prim( gf , 5 , f ) == Ints( 6 , NIL ) );
  }
 CHECK_THROWS( kruskal( -1 , Es() ) , contract_violation );
 CHECK_THROWS( kruskal( 2 , Es( { { 0 , 2 , 1 } } ) ) , contract_violation );
 CHECK_THROWS( kruskal( 2 , Es( { { -1 , 0 , 1 } } ) ) , contract_violation );
 ListGraph< long > d( 2 , true );
 d.insertEdge( 0 , 1 , 1 );
 CHECK_THROWS( prim( d , 0 ) , contract_violation );
 CHECK_THROWS( kruskal( d ) , contract_violation );
 CHECK_THROWS( prim( gf , 6 ) , contract_violation );
 CHECK_THROWS( prim( gf , 0 , Frontier::QUEUE ) , contract_violation );
 }

// the Teorema 14.1: for a random cut, the lightest edge across it is in a
// minimum spanning tree, i.e., the best tree with that edge weighs the same
ADS_TEST( "sec14.3.3.cut" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 5 );
  auto E = random_edges( n , int( g() % 8 ) , 20 , g );
  const unsigned S = 1 + g() % ( ( 1u << n ) - 2 );         // a cut
  int best = -1;
  for( int i = 0 ; i < int( E.size() ) ; ++i )
   if( ( S >> E[ i ].u & 1 ) != ( S >> E[ i ].v & 1 ) &&
       ( best < 0 || E[ i ].w < E[ best ].w ) )
    best = i;
  if( best < 0 ) continue;
  auto F = E;
  F[ best ].w = -1000;                    // forced into every best tree
  CHECK( weight( kruskal( n , F ) ) + 1000 + E[ best ].w ==
         weight( kruskal( n , E ) ) );
  }
 }

// the example of Huffman: frequencies 45, 13, 12, 16, 9, 5 give codes of
// lengths 1, 3, 3, 3, 4, 4, and 2.24 n bits; a prefix code that decodes;
// the edges of one and two characters, equal, zero and Fibonacci
// frequencies, the empty text, and the preconditions
ADS_TEST( "sec14.4.huffman" )
{
 const std::map< char , int > f = { { 'a' , 45 } , { 'b' , 13 } ,
  { 'c' , 12 } , { 'd' , 16 } , { 'e' , 9 } , { 'f' , 5 } };
 const auto code = huffman( f );
 int bits = 0;
 for( const auto & [ c , s ] : code ) bits += f.at( c ) * int( s.size() );
 CHECK( bits == 224 );
 CHECK( code.at( 'a' ).size() == 1 && code.at( 'e' ).size() == 4 &&
        code.at( 'f' ).size() == 4 );
 for( const auto & [ c , s ] : code )                 // a prefix code
  for( const auto & [ d , t ] : code )
   if( c != d ) CHECK( t.compare( 0 , s.size() , s ) != 0 );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 50 ; ++t ) {
  std::string text;
  for( int i = 0 ; i < 200 ; ++i )                   // skewed
   text += char( 'a' + std::min( g() % 30 , g() % 12 ) );
  std::map< char , int > fr;
  for( char c : text ) ++fr[ c ];
  const auto C = huffman( fr );
  const auto enc = encode( text , C );
  CHECK( decode( enc , C ) == text );
  const int fixed = int( std::ceil( std::log2( double( fr.size() ) ) ) );
  CHECK( enc.size() <= text.size() * std::max( fixed , 1 ) );
  }
 const std::map< char , int > one = { { 'z' , 3 } };
 CHECK( huffman( one ).at( 'z' ) == "0" );
 // the edges: one character of frequency 0 and its text; two characters
 // get one bit each; 2^k equal frequencies give codes of k bits; the
 // frequencies of Fibonacci give the deepest tree, a chain with codes of
 // 1 , 2 , ... , n - 1 , n - 1 bits; frequencies 0 still get a code; the
 // empty text encodes and decodes to nothing
 const std::map< char , int > zero = { { 'q' , 0 } };
 CHECK( huffman( zero ).at( 'q' ) == "0" );
 CHECK( decode( encode( std::string( "qqq" ) , huffman( zero ) ) ,
                huffman( zero ) ) == "qqq" );
 const auto two = huffman( std::map< char , int >( { { 'a' , 1 } ,
                                                     { 'b' , 100 } } ) );
 CHECK( two.at( 'a' ).size() == 1 && two.at( 'b' ).size() == 1 &&
        two.at( 'a' ) != two.at( 'b' ) );
 std::map< char , int > flat;
 for( char c = 'a' ; c < 'a' + 16 ; ++c ) flat[ c ] = 7;
 for( const auto & [ c , s ] : huffman( flat ) ) CHECK( s.size() == 4 );
 std::map< int , long > fib;
 long f0 = 1 , f1 = 1;
 for( int i = 0 ; i < 40 ; ++i ) {
  fib[ i ] = f0;
  f1 = f0 + f1;
  f0 = f1 - f0;
  }
 const auto deep = huffman( fib );
 std::vector< std::size_t > lengths;
 for( const auto & [ c , s ] : deep ) lengths.push_back( s.size() );
 std::sort( lengths.begin() , lengths.end() );
 for( std::size_t i = 0 ; i + 1 < lengths.size() ; ++i )
  CHECK( lengths[ i ] == i + 1 );
 CHECK( lengths.back() == 39 );
 const auto withzero = huffman( std::map< char , int >( { { 'a' , 0 } ,
                                { 'b' , 0 } , { 'c' , 5 } } ) );
 CHECK( withzero.size() == 3 && withzero.at( 'c' ).size() == 1 );
 CHECK( encode( std::string() , code ).empty() &&
        decode( std::string() , code ).empty() );
 CHECK_THROWS( huffman( std::map< char , int >() ) , contract_violation );
 CHECK_THROWS( huffman( std::map< char , int >( { { 'a' , -1 } } ) ) ,
               contract_violation );
 CHECK_THROWS( encode( std::string( "ax" ) , code ) , contract_violation );
 CHECK_THROWS( decode( code.at( 'e' ).substr( 0 , 3 ) , code ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the 0-1 knapsack is not greedy: capacity 6, volumes 3 , 3 , 4, profits
// 3 , 3 , 5: by specific profit 5, the optimum 6; no object, capacity 0
// with volumes 0, and the preconditions
ADS_TEST( "ex14.1" )
{
 const Longs p = { 3 , 3 , 5 } , v = { 3 , 3 , 4 };
 std::vector< std::size_t > by = { 0 , 1 , 2 };
 std::stable_sort( by.begin() , by.end() , [ & ]( auto a , auto b ) {
  return( p[ a ] * v[ b ] > p[ b ] * v[ a ] ); } );
 long c = 6 , greedy = 0;
 for( auto i : by ) if( v[ i ] <= c ) { c -= v[ i ]; greedy += p[ i ]; }
 CHECK( greedy == 5 && knapsack( p , v , 6 ).first == 6 );
 // the edges: no object; capacity 0 still takes the objects of volume 0;
 // every object fitting takes all of them
 CHECK( knapsack( {} , {} , 9 ).first == 0 );
 const auto z = knapsack( { 4 , 7 , 2 } , { 0 , 1 , 0 } , 0 );
 CHECK( z.first == 6 && z.second == std::vector< std::size_t >( { 0 , 2 } ) );
 CHECK( knapsack( p , v , 100 ).first == 11 );
 CHECK_THROWS( knapsack( { 1 } , { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( knapsack( { 1 } , { -1 } , 1 ) , contract_violation );
 CHECK_THROWS( knapsack( { 1 , 2 } , { 1 } , 1 ) , contract_violation );
 }

// Kruskal with integer weights in [ 0 , n ): Counting Sort, O( m + n );
// k = 1, ties kept in order, and the weights out of [ 0 , k )
ADS_TEST( "ex14.2" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 2 + int( g() % 40 );
  const auto E = random_edges( n , int( g() % 100 ) , n , g );
  CHECK( weight( kruskal( n , E , n ) ) == weight( kruskal( n , E ) ) );
  }
 // the edges: k = 1, all the weights 0; the weight k - 1; the order of
 // equal weights kept as by the stable sort; a weight k, or negative, out
 // of the range
 const std::vector< Edge > E = { { 0 , 1 , 0 } , { 1 , 2 , 0 } ,
                                 { 0 , 2 , 0 } };
 CHECK( kruskal( 3 , E , 1 ) == kruskal( 3 , E ) && kruskal( 3 , E , 1 ) ==
        std::vector< Edge >( { { 0 , 1 , 0 } , { 1 , 2 , 0 } } ) );
 const std::vector< Edge > H = { { 0 , 1 , 4 } , { 1 , 2 , 2 } ,
                                 { 0 , 2 , 2 } , { 2 , 3 , 4 } };
 CHECK( kruskal( 4 , H , 5 ) == kruskal( 4 , H ) );
 CHECK( kruskal( 0 , std::vector< Edge >() , 3 ).empty() );
 CHECK_THROWS( kruskal( 4 , H , 4 ) , contract_violation );
 CHECK_THROWS( kruskal( 2 , std::vector< Edge >( { { 0 , 1 , -1 } } ) , 3 ) ,
               contract_violation );
 }

// the spanning tree with the fewest red edges: weight 1 to the red ones, 0
// to the blue ones
ADS_TEST( "ex14.3" )
{
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 2 + int( g() % 6 );
  const auto E = random_edges( n , int( g() % 8 ) , 2 , g );   // 1 = red
  long fewest = n;
  for( int mask = 0 ; mask < ( 1 << E.size() ) ; ++mask ) {
   if( std::popcount( unsigned( mask ) ) != n - 1 ) continue;
   MFSet M( n );
   long red = 0;
   bool ok = true;
   for( std::size_t i = 0 ; i < E.size() && ok ; ++i )
    if( mask >> i & 1 ) {
     ok = M.merge( E[ i ].u , E[ i ].v );
     red += E[ i ].w;
     }
   if( ok ) fewest = std::min( fewest , red );
   }
  CHECK( weight( kruskal( n , E ) ) == fewest );
  }
 }

// Prim with a Fibonacci heap, O( m + n log n ): the same trees on a dense
// graph, and on a chain of 2000 nodes from an end and from the middle
ADS_TEST( "ex14.4" )
{
 std::mt19937 g( 8 );
 const int n = 200;
 std::vector< Edge > E;
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   E.push_back( { u , v , long( g() % 1000000 ) } );       // distinct, a.s.
 const auto h = graph( n , E );
 const auto a = prim( h , 0 , Frontier::FIBONACCI ) ,
            b = prim( h , 0 , Frontier::HEAP ) ,
            c = prim( h , 0 , Frontier::LIST );
 CHECK( a == b && b == c );
 long w = 0;
 for( int v = 1 ; v < n ; ++v ) w += *h.weight( a[ v ] , v );
 CHECK( w == weight( kruskal( n , E ) ) );
 // the edges: a chain of 2000 nodes, the deepest tree, from an end and
 // from the middle; two nodes and one edge
 const int L = 2000;
 std::vector< Edge > P;
 for( int v = 1 ; v < L ; ++v ) P.push_back( { v - 1 , v , long( v % 7 ) } );
 const auto chain = graph( L , P );
 for( Frontier f : fronts ) {
  const auto q = prim( chain , 0 , f ) , m = prim( chain , L / 2 , f );
  for( int v = 1 ; v < L ; ++v ) CHECK( q[ v ] == v - 1 );
  CHECK( m[ L / 2 ] == NIL && m[ 0 ] == 1 && m[ L - 1 ] == L - 2 );
  CHECK( prim( graph( 2 , { { 0 , 1 , 3 } } ) , 1 , f ) ==
         Ints( { 1 , NIL } ) );
  }
 CHECK( kruskal( L , P ).size() == std::size_t( L - 1 ) );
 }

// Shortest Job First against all the orders; no program, one, ties, times
// 0, and the preconditions
ADS_TEST( "ex14.5" )
{
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 50 ; ++t ) {
  Longs d( 1 + g() % 7 );
  for( auto & x : d ) x = 1 + long( g() % 20 );
  std::vector< std::size_t > A( d.size() );
  std::iota( A.begin() , A.end() , 0 );
  long best = infinity< long >();
  do best = std::min( best , totalCompletion( d , A ) );
  while( std::next_permutation( A.begin() , A.end() ) );
  CHECK( totalCompletion( d , shortestJobFirst( d ) ) == best );
  }
 // the edges: no program; one; equal times keep their order; times 0 go
 // first and cost nothing; an order of some of the programs only; a time
 // negative, a place out of range
 using Places = std::vector< std::size_t >;
 CHECK( shortestJobFirst( Longs() ).empty() &&
        totalCompletion( Longs() , Places() ) == 0 );
 CHECK( shortestJobFirst( Longs( { 9 } ) ) == Places( { 0 } ) &&
        totalCompletion( Longs( { 9 } ) , Places( { 0 } ) ) == 9 );
 CHECK( shortestJobFirst( Longs( { 3 , 1 , 3 , 1 } ) ) ==
        Places( { 1 , 3 , 0 , 2 } ) );
 const Longs z = { 5 , 0 , 2 , 0 };
 CHECK( shortestJobFirst( z ) == Places( { 1 , 3 , 2 , 0 } ) &&
        totalCompletion( z , shortestJobFirst( z ) ) == 9 );
 CHECK( totalCompletion( z , Places( { 2 } ) ) == 2 );
 CHECK_THROWS( shortestJobFirst( Longs( { 1 , -1 } ) ) , contract_violation );
 CHECK_THROWS( totalCompletion( z , Places( { 4 } ) ) , contract_violation );
 }

// the coloring of intervals: as many colors as the largest overlap, and
// meeting intervals of different colors; no interval, touching, equal and
// nested ones, and the precondition
ADS_TEST( "ex14.6" )
{
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = int( g() % 25 );
  std::vector< std::pair< long , long > > I( n );
  for( auto & [ a , b ] : I ) {
   a = long( g() % 40 );
   b = a + 1 + long( g() % 8 );
   }
  const auto c = intervalColoring( I );
  int colors = 0 , depth = 0;
  for( int x : c ) colors = std::max( colors , x + 1 );
  for( long x = 0 ; x < 50 ; ++x ) {
   int k = 0;
   for( auto [ a , b ] : I ) k += a <= x && x < b;
   depth = std::max( depth , k );
   }
  CHECK( colors == depth );
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 1 ; j < n ; ++j )
    if( I[ i ].first < I[ j ].second && I[ j ].first < I[ i ].second )
     CHECK( c[ i ] != c[ j ] );
  }
 // the edges: no interval; a chain of intervals that touch takes one
 // color; n equal intervals take n; nested ones; an empty interval
 using I2 = std::vector< std::pair< long , long > >;
 CHECK( intervalColoring( I2() ).empty() );
 CHECK( intervalColoring( I2( { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } } ) ) ==
        Ints( { 0 , 0 , 0 } ) );
 auto same = intervalColoring( I2( 6 , { -4 , 4 } ) );
 std::sort( same.begin() , same.end() );
 CHECK( same == Ints( { 0 , 1 , 2 , 3 , 4 , 5 } ) );
 const auto nest = intervalColoring( I2( { { 0 , 9 } , { 1 , 8 } ,
                                          { 2 , 3 } , { 4 , 5 } } ) );
 CHECK( nest[ 0 ] != nest[ 1 ] && nest[ 2 ] == nest[ 3 ] &&
        *std::max_element( nest.begin() , nest.end() ) == 2 );
 CHECK_THROWS( intervalColoring( I2( { { 2 , 2 } } ) ) , contract_violation );
 }

// the towers along a road: each house within d of a tower, as few towers
// as there are houses pairwise farther than 2 d apart in the greedy scan;
// no house, houses in one place, d = 0, 2 d apart, and the precondition
ADS_TEST( "ex14.7" )
{
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 100 ; ++t ) {
  std::vector< double > k( 1 + g() % 20 );
  for( auto & x : k ) x = double( g() % 100 );
  const double d = 1 + double( g() % 10 );
  const auto T = towers( k , d );
  for( double x : k ) {
   bool near = false;
   for( double y : T ) near = near || std::abs( x - y ) <= d;
   CHECK( near );
   }
  // a lower bound: houses more than 2 d apart need different towers
  auto s = k;
  std::sort( s.begin() , s.end() );
  int apart = 0;
  double last = -1e18;
  for( double x : s ) if( x > last + 2 * d ) { ++apart; last = x; }
  CHECK( int( T.size() ) == apart );
  }
 // the edges: no house, no tower; all the houses in one place; d = 0, a
 // tower on each distinct house; houses exactly 2 d apart share a tower;
 // d negative
 CHECK( towers( {} , 3 ).empty() );
 CHECK( towers( { 5 , 5 , 5 } , 0 ) == std::vector< double >( { 5 } ) );
 CHECK( towers( { 4 , 1 , 4 , 9 } , 0 ) ==
        std::vector< double >( { 1 , 4 , 9 } ) );
 CHECK( towers( { 0 , 10 } , 5 ) == std::vector< double >( { 5 } ) );
 CHECK( towers( { 0 , 10.5 } , 5 ).size() == 2 );
 CHECK_THROWS( towers( { 1 } , -1 ) , contract_violation );
 }

// skiers and skis: the greedy pair of least difference first is not
// optimal (heights 2 , 6, skis 5 , 9: 6 with 5, then 2 with 9, total 8,
// against 6 by sorting both, which is optimal); no skier, one, all equal
ADS_TEST( "ex14.8" )
{
 auto greedy = []( Longs p , Longs s ) {
  long total = 0;
  while( ! p.empty() ) {
   std::size_t bi = 0 , bj = 0;
   for( std::size_t i = 0 ; i < p.size() ; ++i )
    for( std::size_t j = 0 ; j < s.size() ; ++j )
     if( std::abs( p[ i ] - s[ j ] ) < std::abs( p[ bi ] - s[ bj ] ) ) {
      bi = i;
      bj = j;
      }
   total += std::abs( p[ bi ] - s[ bj ] );
   p.erase( p.begin() + bi );
   s.erase( s.begin() + bj );
   }
  return( total );
  };
 auto sorted = []( Longs p , Longs s ) {
  std::sort( p.begin() , p.end() );
  std::sort( s.begin() , s.end() );
  long total = 0;
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   total += std::abs( p[ i ] - s[ i ] );
  return( total );
  };
 CHECK( greedy( { 2 , 6 } , { 5 , 9 } ) == 8 );
 CHECK( sorted( { 2 , 6 } , { 5 , 9 } ) == 6 );
 // the edges: no skier; one; equal heights and skis cost nothing
 CHECK( greedy( {} , {} ) == 0 && sorted( {} , {} ) == 0 );
 CHECK( greedy( { 7 } , { 3 } ) == 4 && sorted( { 7 } , { 3 } ) == 4 );
 CHECK( sorted( { 4 , 4 , 4 } , { 4 , 4 , 4 } ) == 0 );
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 50 ; ++t ) {               // sorting is optimal
  Longs p( 1 + g() % 6 ) , s( p.size() );
  for( auto & x : p ) x = long( g() % 30 );
  for( auto & x : s ) x = long( g() % 30 );
  std::vector< std::size_t > h( p.size() );
  std::iota( h.begin() , h.end() , 0 );
  long best = infinity< long >();
  do {
   long c = 0;
   for( std::size_t i = 0 ; i < p.size() ; ++i )
    c += std::abs( p[ i ] - s[ h[ i ] ] );
   best = std::min( best , c );
   } while( std::next_permutation( h.begin() , h.end() ) );
  CHECK( sorted( p , s ) == best );
  }
 }

// distinct weights: the lightest and the second lightest edge are in every
// minimum spanning tree (it is unique), the third is not when it closes a
// triangle with them
ADS_TEST( "ex14.9" )
{
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 6 );
  auto E = random_edges( n , int( g() % 10 ) , 1 , g );
  std::vector< long > w( E.size() );
  std::iota( w.begin() , w.end() , 1 );
  std::shuffle( w.begin() , w.end() , g );
  for( std::size_t i = 0 ; i < E.size() ; ++i ) E[ i ].w = w[ i ];
  auto T = kruskal( n , E );
  auto has = [ & ]( long x ) {
   return( std::any_of( T.begin() , T.end() ,
                        [ & ]( auto & e ) { return( e.w == x ); } ) );
   };
  CHECK( has( 1 ) && ( E.size() < 2 || has( 2 ) ) );
  }
 const auto T = kruskal( 3 , std::vector< Edge >( { { 0 , 1 , 1 } ,
                                                    { 1 , 2 , 2 } ,
                                                    { 0 , 2 , 3 } } ) );
 CHECK( weight( T ) == 3 && T.size() == 2 );
 }

// the maximum spanning tree is the minimum one with the weights negated;
// the longest path is not the shortest one with the costs negated, which
// may have negative cycles: on a cycle of positive costs Bellman-Ford-Moore
// finds one
ADS_TEST( "ex14.10" )
{
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 2 + int( g() % 6 );
  auto E = random_edges( n , int( g() % 8 ) , 20 , g );
  long best = -infinity< long >();
  for( int mask = 0 ; mask < ( 1 << E.size() ) ; ++mask ) {
   if( std::popcount( unsigned( mask ) ) != n - 1 ) continue;
   MFSet M( n );
   long w = 0;
   bool ok = true;
   for( std::size_t i = 0 ; i < E.size() && ok ; ++i )
    if( mask >> i & 1 ) { ok = M.merge( E[ i ].u , E[ i ].v ); w += E[ i ].w; }
   if( ok ) best = std::max( best , w );
   }
  for( auto & e : E ) e.w = -e.w;
  CHECK( -weight( kruskal( n , E ) ) == best );
  }
 ListGraph< long > c( 3 , true );
 c.insertEdge( 0 , 1 , -1 );
 c.insertEdge( 1 , 2 , -1 );
 c.insertEdge( 2 , 0 , -1 );
 CHECK( bellmanFordMoore( c , 0 ).negativeCycle );
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
/*---------------------------- End File ch14.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
