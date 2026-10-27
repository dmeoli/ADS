/*--------------------------------------------------------------------------*/
/*----------------------------- File ch18.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 18, Teoria dell'NP-completezza: the examples of the
 * problems, the certificates, the nondeterministic algorithms simulated by
 * enumeration, the linear tiling in polynomial time, the reduction of the
 * sorting to the convex hull, the reductions of the square tiling to
 * CRICCA, SODDISFATTIBILITÀ and PROGRAMMAZIONE LINEARE 0/1 on figure 18.8
 * and on random tiles, the other reductions on random instances, and the
 * exercises 18.1 to 18.16, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 18,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
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

#include "ADS/complexity/Domino.h"
#include "ADS/complexity/Nondeterminism.h"
#include "ADS/complexity/Problems.h"
#include "ADS/complexity/Reductions.h"
#include "ADS/geometry/ConvexHull.h"
#include "ADS/optimization/Knapsack.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Longs = std::vector< long >;

/// a random graph of n nodes, unoriented or not, each edge with
/// probability p percent
Adjacency random_graph( int n , int p , bool oriented , std::mt19937 & g )
{
 Adjacency G( n , std::vector< bool >( n , false ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = oriented ? 0 : u + 1 ; v < n ; ++v )
   if( u != v && int( g() % 100 ) < p ) {
    G[ u ][ v ] = true;
    if( ! oriented ) G[ v ][ u ] = true;
    }
 return( G );
 }

/// the tiles of figure 18.8: two of a kind never side by side, d0 and d1
/// fit in both orders, as the covering d0 d1 / d1 d0 of the figure needs
const std::vector< Tile > fig188 = { { 1 , 1 , 0 , 0 } , { 0 , 0 , 1 , 1 } };

/// random tiles of c colors
std::vector< Tile > random_tiles( int m , int c , std::mt19937 & g )
{
 std::vector< Tile > D( m );
 for( auto & t : D )
  t = { int( g() % c ) , int( g() % c ) , int( g() % c ) , int( g() % c ) };
 return( D );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the examples of the text: the system of 0/1 programming with
// x = ( 1 , 1 , 0 , 0 ), the formula with u1 false and u2, u3 true, the
// provinces of Tuscany colored with 3 colors and not 2, with a clique of 3
// and none of 4, and the seven places of the salesperson; then the
// degenerate instances: the empty formula, the empty clause, x and not x,
// no variables, the empty system, the empty graph, k <= 0 and k > n, a
// loop (ignored), the tour of no city (0 long, an empty P read past its
// end before the fix) and of one, and the preconditions
ADS_TEST( "sec18.examples" )
{
 // A x <= b: the >= rows negated
 const std::vector< Longs > A = { { -1 , -1 , -1 , -1 } , { -1 , 1 , 1 , -1 } ,
                                  { 1 , 0 , 1 , 1 } };
 const Longs b = { -2 , 0 , 1 };
 const std::vector< int > book = { 1 , 1 , 0 , 0 };
 for( auto x : { book , *lp01( A , b , 4 ) } )
  for( std::size_t r = 0 ; r < A.size() ; ++r ) {
   long s = 0;
   for( int j = 0 ; j < 4 ; ++j ) s += A[ r ][ j ] * x[ j ];
   CHECK( s <= b[ r ] );
   }
 const Cnf F = { { -1 , -2 , -3 } , { 2 , -3 } , { 1 , -2 , 3 } };
 CHECK( satCertificate( F , { false , true , true } ) );
 CHECK( satisfiable( F , 3 ) );
 const std::vector< std::string > P = { "MS" , "LU" , "PT" , "PO" , "FI" ,
                                        "AR" , "SI" , "GR" , "LI" , "PI" };
 auto I = [ & ]( const char * p ) {
  return( int( std::find( P.begin() , P.end() , p ) - P.begin() ) ); };
 Adjacency T( 10 , std::vector< bool >( 10 , false ) );
 for( auto [ a , c ] : std::vector< std::pair< const char * , const char * > >(
       { { "MS" , "LU" } , { "LU" , "PT" } , { "LU" , "PI" } ,
         { "PT" , "PO" } , { "PT" , "FI" } , { "PT" , "PI" } ,
         { "PO" , "FI" } , { "FI" , "AR" } , { "FI" , "SI" } ,
         { "FI" , "PI" } , { "AR" , "SI" } , { "SI" , "GR" } ,
         { "SI" , "PI" } , { "GR" , "PI" } , { "GR" , "LI" } ,
         { "LI" , "PI" } } ) )
  T[ I( a ) ][ I( c ) ] = T[ I( c ) ][ I( a ) ] = true;
 CHECK( coloring( T , 3 ) && ! coloring( T , 2 ) );
 CHECK( cliqueCertificate( T , { I( "FI" ) , I( "PT" ) , I( "PO" ) } , 3 ) );
 CHECK( cliqueCertificate( T , { I( "PI" ) , I( "GR" ) , I( "SI" ) } , 3 ) );
 CHECK( clique( T , 3 ) && ! clique( T , 4 ) );
 const std::vector< Longs > Dist = { { 0 , 68 , 42 , 89 , 44 , 37 , 85 } ,
  { 68 , 0 , 29 , 30 , 75 , 28 , 19 } , { 42 , 29 , 0 , 42 , 41 , 14 , 54 } ,
  { 89 , 30 , 42 , 0 , 80 , 52 , 36 } , { 44 , 75 , 41 , 80 , 0 , 50 , 94 } ,
  { 37 , 28 , 14 , 52 , 50 , 0 , 49 } , { 85 , 19 , 54 , 36 , 94 , 49 , 0 } };
 long best = 0;
 while( ! tsp( Dist , best ) ) ++best;     // the least k with a YES
 const auto tour = tsp( Dist , best );
 CHECK( tspCertificate( Dist , best , *tour ) &&
        ! tspCertificate( Dist , best - 1 , *tour ) );
 CHECK( best == 247 );          // Pisa, Castelnuovo, Serravalle, ...
 CHECK( *tour == Nodes( { 0 , 4 , 2 , 3 , 6 , 1 , 5 } ) );
 // the formulas and the systems
 CHECK( satCertificate( {} , {} ) && ! satCertificate( { {} } , { true } ) );
 CHECK( satisfiable( {} , 0 ) && ! satisfiable( { {} } , 2 ) );
 CHECK( ! satisfiable( { { 1 } , { -1 } } , 1 ) );
 CHECK( *satisfiable( { { -1 } , { 2 } } , 2 ) ==
        std::vector< bool >( { false , true } ) );
 CHECK( lp01( {} , {} , 0 ) && ( *lp01( {} , {} , 3 ) ==
                                 std::vector< int >( 3 , 0 ) ) );
 CHECK( lp01( { {} } , { 0 } , 0 ) && ! lp01( { {} } , { -1 } , 0 ) );
 CHECK( ! lp01( { { 1 , 1 } , { -1 , -1 } } , { 1 , -2 } , 2 ) );
 CHECK_THROWS( satisfiable( { { 0 } } , 1 ) , contract_violation );
 CHECK_THROWS( satisfiable( { { 2 } } , 1 ) , contract_violation );
 CHECK_THROWS( satisfiable( {} , -1 ) , contract_violation );
 CHECK_THROWS( satCertificate( { { -3 } } , { true , true } ) ,
               contract_violation );
 CHECK_THROWS( lp01( { { 1 } } , {} , 1 ) , contract_violation );
 CHECK_THROWS( lp01( { { 1 , 2 } } , { 0 } , 1 ) , contract_violation );
 // the graphs
 const Adjacency E0 , L1 = { { true } } ,
                 K2 = { { false , true } , { true , false } };
 CHECK( clique( E0 , 0 ) && ! clique( E0 , 1 ) && clique( K2 , -1 )->empty() );
 CHECK( *clique( K2 , 2 ) == Nodes( { 0 , 1 } ) && ! clique( K2 , 3 ) );
 CHECK( *clique( L1 , 1 ) == Nodes( { 0 } ) );
 CHECK( cliqueCertificate( K2 , {} , 0 ) &&
        ! cliqueCertificate( K2 , { 0 } , 2 ) );
 CHECK( ! cliqueCertificate( K2 , { 0 , 0 } , 2 ) );
 CHECK( coloring( E0 , 0 ) && ! coloring( L1 , 0 ) && coloring( L1 , 1 ) );
 CHECK( ! coloring( K2 , 1 ) &&
        *coloring( K2 , 2 ) == std::vector< int >( { 0 , 1 } ) );
 CHECK_THROWS( coloring( K2 , -1 ) , contract_violation );
 CHECK_THROWS( clique( { { false , true } , { false , false } } , 1 ) ,
               contract_violation );
 CHECK_THROWS( cliqueCertificate( { { false , true } } , {} , 0 ) ,
               contract_violation );
 // the tours
 using LM = std::vector< Longs >;
 CHECK( tsp( LM() , 0L ) && tsp( LM() , 0L )->empty() && ! tsp( LM() , -1L ) );
 CHECK( tspCertificate( LM() , 0L , {} ) &&
        ! tspCertificate( LM() , -1L , {} ) );
 CHECK( *tsp( LM( { { 5 } } ) , 5L ) == Nodes( { 0 } ) );
 CHECK( ! tsp( LM( { { 5 } } ) , 4L ) );
 const LM D2 = { { 0 , 3 } , { 4 , 0 } };
 CHECK( tsp( D2 , 7L ) && ! tsp( D2 , 6L ) );
 for( const Nodes & P : { Nodes( { 0 } ) , Nodes( { 0 , 0 } ) ,
                          Nodes( { 0 , 2 } ) , Nodes( { 0 , -1 } ) } )
  CHECK( ! tspCertificate( D2 , 9L , P ) );
 CHECK_THROWS( tsp( LM( { { 0 , 1 } } ) , 0L ) , contract_violation );
 CHECK_THROWS( tspCertificate( LM( { { 0 , 1 } } ) , 0L , {} ) ,
               contract_violation );
 }

// the nondeterministic algorithms ndCricca() and ndTsp(), simulated by the
// enumeration of their choices, against the enumerations; an algorithm
// with no choice runs once, a choice of one is always 0, the sequences
// come in lexicographic order and the last one is kept after a YES, and a
// choice among none violates the precondition
ADS_TEST( "sec18.2.nondeterminism" )
{
 Chooser c0;
 long r0 = 0;
 CHECK( c0.run( []( Chooser & ) { return( true ); } , &r0 ) && ( r0 == 1 ) );
 CHECK( c0.choices().empty() );
 r0 = 0;
 CHECK( ! c0.run( []( Chooser & ) { return( false ); } , &r0 ) && r0 == 1 );
 r0 = 0;
 CHECK( ! c0.run( []( Chooser & ch ) { return( ch.choice( 1 ) != 0 ); } ,
                  &r0 ) && ( r0 == 1 ) );
 r0 = 0;
 std::vector< std::vector< int > > seen;
 CHECK( c0.run( [ & ]( Chooser & ch ) {
         const int a = ch.choice( 3 ) , b = ch.choice( 2 );
         seen.push_back( { a , b } );
         return( ( a == 2 ) && ( b == 1 ) ); } , &r0 ) && ( r0 == 6 ) );
 CHECK( c0.choices() == std::vector< int >( { 2 , 1 } ) );
 CHECK( std::is_sorted( seen.begin() , seen.end() ) && seen.size() == 6 );
 CHECK_THROWS( c0.run( []( Chooser & ch ) {
                return( ch.choice( 0 ) == 0 ); } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 2 + int( g() % 7 ) , k = 1 + int( g() % 4 );
  const auto G = random_graph( n , 50 , false , g );
  Chooser c;
  long runs = 0;
  const bool yes = c.run( [ & ]( Chooser & ch ) {
   Nodes S;
   for( int i = 0 ; i < n ; ++i ) if( ch.choice( 2 ) ) S.push_back( i );
   return( cliqueCertificate( G , S , k ) ); } , &runs );
  CHECK( yes == clique( G , k ).has_value() );
  CHECK( runs <= ( 1L << n ) );
  // ndTsp(): S[ i ] = choice( C ), C the cities not chosen yet
  std::vector< Longs > Dist( n , Longs( n , 0 ) );
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 1 ; j < n ; ++j )
    Dist[ i ][ j ] = Dist[ j ][ i ] = 1 + g() % 9;
  const long kt = long( g() % ( 5 * n ) );
  Chooser d;
  const bool tyes = d.run( [ & ]( Chooser & ch ) {
   Nodes C( n ) , S;
   std::iota( C.begin() , C.end() , 0 );
   for( int i = 0 ; i < n ; ++i ) {
    const int j = ch.choice( int( C.size() ) );
    S.push_back( C[ j ] );
    C.erase( C.begin() + j );
    }
   return( tspCertificate( Dist , kt , S ) ); } );
  CHECK( tyes == tsp( Dist , kt ).has_value() );
  }
 }

// the linear tiling in polynomial time, against the enumeration of the
// strips; n < 1 and no tiles give false, n = 1 is d alone, a tile that
// fits itself or a cycle of two give any n up to the largest long, a chain
// of 2000 tiles gives n <= 2000 only (and no deep recursion trouble), a
// cycle not reachable from d does not help
ADS_TEST( "sec18.5.linear-tiling" )
{
 const std::vector< Tile > self = { { 0 , 1 , 0 , 1 } } ,
                           alone = { { 0 , 1 , 0 , 2 } } ,
                           two = { { 0 , 1 , 0 , 2 } , { 0 , 2 , 0 , 1 } };
 CHECK( ! linearTiling( self , 0 ) && ! linearTiling( self , -5 ) );
 CHECK( ! linearTiling( {} , 3 ) );
 CHECK( linearTiling( alone , 1 ) && ! linearTiling( alone , 2 ) );
 CHECK( linearTiling( self , std::numeric_limits< long >::max() ) );
 CHECK( linearTiling( two , std::numeric_limits< long >::max() ) );
 std::vector< Tile > chain;                     // left i , right i + 1
 for( int i = 0 ; i < 2000 ; ++i )
  chain.push_back( Tile{ 0 , i + 1 , 0 , i } );
 CHECK( linearTiling( chain , 2000 ) && ! linearTiling( chain , 2001 ) );
 chain.push_back( Tile{ 0 , 5000 , 0 , 5000 } );
 CHECK( ! linearTiling( chain , 2001 ) );
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto D = random_tiles( 1 + int( g() % 4 ) , 3 , g );
  const int n = 1 + int( g() % 6 );
  // the strips d , x_1 , ... , x_{n-1}, enumerated
  bool ref = false;
  std::vector< int > s( n , 0 );
  for( ; ! ref ; ) {
   bool ok = s[ 0 ] == 0;
   for( int i = 0 ; i + 1 < n && ok ; ++i )
    ok = D[ s[ i ] ].right == D[ s[ i + 1 ] ].left;
   ref = ok;
   int i = 1;
   while( i < n && s[ i ] == int( D.size() ) - 1 ) s[ i++ ] = 0;
   if( i >= n ) break;
   ++s[ i ];
   }
  CHECK( linearTiling( D , n ) == ref );
  }
 }

// sorting to the convex hull: the hull of the points on the parabola
// lists all of them in increasing order of a from the leftmost point,
// negative numbers and one number included; no numbers give no points,
// the largest a whose square fits in a long is accepted and the next one,
// on either side, is not
ADS_TEST( "sec18.5.hull-sort" )
{
 CHECK( sortToHull( {} ).empty() );
 const long R = detail::kLongRoot;
 const auto E = sortToHull( { -R , 0 , R } );
 CHECK( E[ 0 ].x == -R && E[ 0 ].y == R * R && E[ 1 ].y == 0 &&
        E[ 2 ].y == R * R );
 CHECK( R * R > std::numeric_limits< long >::max() - 2 * R - 1 );
 CHECK_THROWS( sortToHull( { R + 1 } ) , contract_violation );
 CHECK_THROWS( sortToHull( { -R - 1 } ) , contract_violation );
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 100 ; ++t ) {
  std::set< long > s;
  while( s.size() < 1 + g() % 30 ) s.insert( long( g() % 1000 ) - 500 );
  Longs a( s.begin() , s.end() );
  std::shuffle( a.begin() , a.end() , g );
  const auto H = graham( sortToHull( a ) );
  Longs x;
  for( const auto & p : H ) x.push_back( p.x );
  // the hull starts at its lowest point, a = min | a |: read it from the
  // leftmost one
  std::rotate( x.begin() , std::min_element( x.begin() , x.end() ) ,
               x.end() );
  CHECK( x == Longs( s.begin() , s.end() ) );
  }
 }

// Figure 18.8: the covering d0 d1 / d1 d0, the formula that the assignment
// of the text satisfies, the system that its x satisfies, the clique of
// k = 4; on random tiles the four answers agree. The example of the text
// lists 19 clauses, two of which, ( not u101 or not u011 ) and ( not u100
// or not u010 ), forbid d1 left of d0, which its own covering has in the
// upper row and which its assignment violates (the same for x101 + x011
// <= 1 in the system): with them the formula has no solution. The tiles of
// the covering give 17 clauses. The degenerate coverings: n < 1, T not
// n x n (ragged rows, read past their end before the fix), tiles out of
// D, d not in ( 0 , 0 ); n = 1, always covered by d, and one tile with
// n = 3, with the four answers agreeing; the preconditions of the
// reductions and of tileVar()
ADS_TEST( "sec18.7.domino" )
{
 CHECK( ! legalTiling( fig188 , 0 , {} ) );
 CHECK( ! legalTiling( fig188 , 2 , { { 0 , 1 } , { 1 } } ) );
 CHECK( ! legalTiling( fig188 , 2 , { { 0 } , { 1 , 0 } } ) );
 CHECK( ! legalTiling( fig188 , 1 , { {} } ) );
 CHECK( ! legalTiling( fig188 , 2 , { { 0 , 1 } , { 1 , 2 } } ) );
 CHECK( ! legalTiling( fig188 , 2 , { { 0 , 1 } , { -1 , 0 } } ) );
 CHECK( ! legalTiling( fig188 , 2 , { { 1 , 0 } , { 0 , 1 } } ) );
 CHECK( ! squareTiling( fig188 , 0 ) && ! squareTiling( {} , 2 ) );
 CHECK( *squareTiling( fig188 , 1 ) == Tiling( { { 0 } } ) );
 CHECK_THROWS( dominoToClique( fig188 , 0 ) , contract_violation );
 CHECK_THROWS( dominoToSat( {} , 1 ) , contract_violation );
 CHECK_THROWS( dominoToLp01( {} , 1 ) , contract_violation );
 CHECK_THROWS( tileVar( -1 , 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( tileVar( 0 , 0 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( tileVar( 0 , 0 , 0 , 0 ) , contract_violation );
 const int n = 2;
 const Tiling T = { { 0 , 1 } , { 1 , 0 } };      // T[ i ][ j ]
 CHECK( legalTiling( fig188 , n , T ) );
 CHECK( squareTiling( fig188 , n ) );
 const Cnf F = dominoToSat( fig188 , n );
 CHECK( F.size() == 17 );
 std::vector< bool > u( 8 , false );
 for( auto [ h , i , j ] : { std::array< int , 3 >{ 0 , 0 , 0 } ,
                             std::array< int , 3 >{ 0 , 1 , 1 } ,
                             std::array< int , 3 >{ 1 , 0 , 1 } ,
                             std::array< int , 3 >{ 1 , 1 , 0 } } )
  u[ tileVar( h , i , j , n ) ] = true;
 CHECK( satCertificate( F , u ) );
 const auto [ A , b ] = dominoToLp01( fig188 , n );
 std::vector< int > x( 8 );
 for( int v = 0 ; v < 8 ; ++v ) x[ v ] = u[ v ];
 for( std::size_t r = 0 ; r < A.size() ; ++r ) {
  long s = 0;
  for( int v = 0 ; v < 8 ; ++v ) s += A[ r ][ v ] * x[ v ];
  CHECK( s <= b[ r ] );
  }
 const auto [ G , k ] = dominoToClique( fig188 , n );
 Nodes S;
 for( int v = 0 ; v < 8 ; ++v ) if( u[ v ] ) S.push_back( v );
 CHECK( k == 4 && cliqueCertificate( G , S , k ) );
 std::mt19937 g( 4 );
 int yes = 0;
 for( int t = 0 ; t < 60 ; ++t ) {
  const auto D = random_tiles( 1 + int( g() % 3 ) , 2 , g );
  const bool tiling = squareTiling( D , 2 ).has_value();
  const auto [ GG , kk ] = dominoToClique( D , 2 );
  const auto [ AA , bb ] = dominoToLp01( D , 2 );
  const int N = int( D.size() ) * 4;
  CHECK( clique( GG , kk ).has_value() == tiling );
  CHECK( satisfiable( dominoToSat( D , 2 ) , N ).has_value() == tiling );
  CHECK( lp01( AA , bb , N ).has_value() == tiling );
  yes += tiling;
  }
 CHECK( yes > 5 && yes < 55 );
 for( int t = 0 ; t < 20 ; ++t )
  for( auto [ m , n1 ] : { std::pair( 1 + int( g() % 3 ) , 1 ) ,
                           std::pair( 1 , 3 ) } ) {
   const auto D = random_tiles( m , 2 , g );
   const bool tiling = squareTiling( D , n1 ).has_value();
   CHECK( tiling || ( n1 > 1 ) );
   const auto [ GG , kk ] = dominoToClique( D , n1 );
   const auto [ AA , bb ] = dominoToLp01( D , n1 );
   const int N = m * n1 * n1;
   CHECK( clique( GG , kk ).has_value() == tiling );
   CHECK( satisfiable( dominoToSat( D , n1 ) , N ).has_value() == tiling );
   CHECK( lp01( AA , bb , N ).has_value() == tiling );
   }
 }

// the other reductions of section 18.7 on random instances, k from 0 to
// n + 1, the sequences empty or with zeros; the degenerate instances: the
// empty graph, a loop (dropped by complement()), the empty sequence and
// the odd sum, whose bound -1 has no YES, and the preconditions
ADS_TEST( "sec18.7.reductions" )
{
 CHECK( complement( {} ).empty() && hamiltonToTsp( {} ).empty() );
 CHECK( complement( { { true } } ) == Adjacency( { { false } } ) );
 CHECK( halfSum( {} ) == 0 && halfSum( { 0 } ) == 0 );
 CHECK( halfSum( { 1 } ) == -1 && halfSum( { 3 , 5 } ) == 4 );
 CHECK( ! subsetSumExists( { 1 } , -1 ) );
 CHECK( ! multiprocessorScheduling( { 1 } , 2 , -1 ) );
 CHECK( ! multiprocessorScheduling( {} , 2 , -1 ) );     // threw before
 CHECK( multiprocessorScheduling( {} , 2 , 0 )->empty() );
 CHECK( ! knapsackDecision( { 1 } , { 1 } , -1 , -1 ) );
 CHECK_THROWS( halfSum( { -1 } ) , contract_violation );
 CHECK_THROWS( complement( { { false , true } } ) , contract_violation );
 CHECK_THROWS( hamiltonToTsp( { { false , true } , { false , false } } ) ,
               contract_violation );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 7 ) , k = int( g() % ( n + 1 ) );
  const auto G = random_graph( n , int( g() % 100 ) , false , g );
  CHECK( clique( G , k ).has_value() ==
         independentSet( complement( G ) , k ).has_value() );   // 18.5
  CHECK( ! clique( G , n + 1 ) &&
         ! independentSet( complement( G ) , n + 1 ) );
  if( n >= 3 )
   CHECK( hamiltonianCircuit( G ).has_value() ==               // 18.6
          tsp( hamiltonToTsp( G ) , long( n ) ).has_value() );
  Longs a( g() % 8 );
  for( auto & x : a ) x = long( g() % 11 );
  const bool part = partition( a ).has_value();
  const long h = halfSum( a );
  CHECK( multiprocessorScheduling( a , 2 , h ).has_value() == part );  // 18.7
  CHECK( subsetSumExists( a , h ).has_value() == part );              // 18.8
  CHECK( knapsackDecision( a , a , h , h ).has_value() == part );     // 18.9
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// mazzette(): the permutation of the bribes chosen one place at a time,
// against the enumeration of the permutations (exercise 18.4), no
// politicians included
ADS_TEST( "ex18.1" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = int( g() % 6 );
  Longs M( n );
  for( auto & m : M ) m = 1 + long( g() % 6 );
  std::vector< bool > ps( n );
  for( int i = 0 ; i < n ; ++i ) ps[ i ] = g() % 2;
  Chooser c;
  const bool yes = c.run( [ & ]( Chooser & ch ) {
   Longs S = M;
   for( int i = 0 ; i < n ; ++i )
    std::swap( S[ i ] , S[ i + ch.choice( n - i ) ] );   // j in { i .. n-1 }
   long a = 0 , b = 0;
   for( int i = 0 ; i < n ; ++i ) ( ps[ i ] ? a : b ) += S[ i ];
   return( a == 2 * b ); } );
  CHECK( yes == bribes( M , ps ).has_value() );
  }
 }

// ndZaino(): n choices in or out, O( n ) per run, against the dynamic
// programming of chapter 13; n = 0 (one run, no choice), C = 0 and k = 0
ADS_TEST( "ex18.2" )
{
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = int( g() % 8 );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + g() % 9;
   v[ i ] = 1 + g() % 9;
   }
  const long C = long( g() % 25 ) , k = long( g() % 30 );
  Chooser c;
  const bool yes = c.run( [ & ]( Chooser & ch ) {
   long tp = 0 , tv = 0;
   for( int i = 0 ; i < n ; ++i )
    if( ch.choice( 2 ) ) {
     tp += p[ i ];
     tv += v[ i ];
     }
   return( tp >= k && tv <= C ); } );
  CHECK( yes == ( knapsack( p , v , C ).first >= k ) );
  }
 }

// the coloring chosen node by node, k colors each, the empty graph
// included
ADS_TEST( "ex18.3" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = int( g() % 6 ) , k = 1 + int( g() % 3 );
  const auto G = random_graph( n , 50 , false , g );
  Chooser c;
  const bool yes = c.run( [ & ]( Chooser & ch ) {
   std::vector< int > col( n );
   for( auto & x : col ) x = ch.choice( k );
   for( int u = 0 ; u < n ; ++u )
    for( int v = 0 ; v < n ; ++v )
     if( G[ u ][ v ] && col[ u ] == col[ v ] ) return( false );
   return( true ); } );
  CHECK( yes == coloring( G , k ).has_value() );
  }
 }

// the enumeration of the bribes: a case with and one without an answer,
// the bribes returned being a permutation that gives the PS twice the
// PDF; no politicians (0 = 2 0), one, all the bribes equal, and sizes
// that differ
ADS_TEST( "ex18.4" )
{
 auto fair = []( const std::vector< long > & M ,
                 const std::vector< bool > & ps ) {
  long a = 0 , b = 0;
  for( std::size_t i = 0 ; i < M.size() ; ++i ) ( ps[ i ] ? a : b ) += M[ i ];
  return( a == 2 * b ); };
 const std::vector< bool > ps = { true , true , false };
 const auto B = bribes( { 1 , 2 , 3 } , ps );                 // 1 + 3 = 2 2
 CHECK( B && fair( *B , ps ) &&
        std::is_permutation( B->begin() , B->end() , Longs( { 1 , 2 , 3 } )
                             .begin() ) );
 CHECK( ! bribes( { 1 , 1 } , { true , false } ) );
 CHECK( bribes( {} , {} ) && bribes( {} , {} )->empty() );
 CHECK( bribes( { 0 } , { true } ) && ! bribes( { 1 } , { true } ) );
 CHECK( ! bribes( { 1 } , { false } ) );
 CHECK( bribes( { 5 , 5 , 5 } , ps ) );
 CHECK( ! bribes( { 5 , 5 , 5 } , { true , false , false } ) );
 CHECK_THROWS( bribes( { 1 } , {} ) , contract_violation );
 }

// enZaino() against the dynamic programming; no objects, C < 0 (not even
// the empty set fits), k <= 0 (the empty set), an object of volume 0 in
// C = 0, a negative profit, and the preconditions
ADS_TEST( "ex18.5" )
{
 CHECK( ! knapsackDecision( {} , {} , -1 , 0 ) );
 CHECK( knapsackDecision( {} , {} , 0 , -5 )->empty() );
 CHECK( ! knapsackDecision( { 5 } , { 0 } , -1 , 0 ) );
 CHECK( *knapsackDecision( { 5 } , { 0 } , 0 , 5 ) == Nodes( { 0 } ) );
 CHECK( *knapsackDecision( { -3 , 4 } , { 1 , 1 } , 2 , 4 ) ==
        Nodes( { 1 } ) );
 CHECK( ! knapsackDecision( { -3 , 4 } , { 1 , 1 } , 2 , 5 ) );
 CHECK_THROWS( knapsackDecision( { 1 } , {} , 1 , 1 ) , contract_violation );
 CHECK_THROWS( knapsackDecision( { 1 } , { -1 } , 1 , 1 ) ,
               contract_violation );
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = int( g() % 10 );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + g() % 9;
   v[ i ] = 1 + g() % 9;
   }
  const long C = long( g() % 30 ) , k = long( g() % 35 );
  const auto S = knapsackDecision( p , v , C , k );
  CHECK( S.has_value() == ( knapsack( p , v , C ).first >= k ) );
  if( S ) {
   long tp = 0 , tv = 0;
   for( int i : *S ) { tp += p[ i ]; tv += v[ i ]; }
   CHECK( tp >= k && tv <= C );
   }
  }
 }

// the coloring by enumeration, against all the colorings
ADS_TEST( "ex18.6" )
{
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 7 ) , k = 1 + int( g() % 3 );
  const auto G = random_graph( n , 40 , false , g );
  bool ref = false;
  std::vector< int > col( n , 0 );
  for( ; ! ref ; ) {
   bool ok = true;
   for( int u = 0 ; u < n && ok ; ++u )
    for( int v = u + 1 ; v < n && ok ; ++v )
     ok = ! ( G[ u ][ v ] && col[ u ] == col[ v ] );
   ref = ok;
   int i = 0;
   while( i < n && col[ i ] == k - 1 ) col[ i++ ] = 0;
   if( i == n ) break;
   ++col[ i ];
   }
  const auto c = coloring( G , k );
  CHECK( c.has_value() == ref );
  }
 }

// enCopertura(), against all the subfamilies; m = 0 (the empty
// subfamily), m not a multiple of 3, no sets, the same set twice, sets
// that meet, and the preconditions
ADS_TEST( "ex18.7" )
{
 using Y3 = std::vector< std::array< int , 3 > >;
 CHECK( exactCover3( {} , 0 ) && exactCover3( {} , 0 )->empty() );
 CHECK( ! exactCover3( {} , 3 ) );
 CHECK( ! exactCover3( Y3( { { 0 , 1 , 2 } , { 2 , 3 , 1 } } ) , 4 ) );
 CHECK( *exactCover3( Y3( { { 0 , 1 , 2 } , { 0 , 1 , 2 } } ) , 3 ) ==
        Nodes( { 0 } ) );
 CHECK( ! exactCover3( Y3( { { 0 , 1 , 2 } , { 2 , 3 , 4 } } ) , 6 ) );
 CHECK_THROWS( exactCover3( Y3( { { 0 , 0 , 1 } } ) , 3 ) ,
               contract_violation );
 CHECK_THROWS( exactCover3( Y3( { { 0 , 1 , 3 } } ) , 3 ) ,
               contract_violation );
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int m = 3 * ( 1 + int( g() % 3 ) ) , n = 1 + int( g() % 9 );
  std::vector< std::array< int , 3 > > Y( n );
  for( auto & y : Y ) {
   std::vector< int > e( m );
   std::iota( e.begin() , e.end() , 0 );
   std::shuffle( e.begin() , e.end() , g );
   y = { e[ 0 ] , e[ 1 ] , e[ 2 ] };
   }
  bool ref = false;
  for( unsigned S = 0 ; S < ( 1u << n ) && ! ref ; ++S ) {
   std::vector< int > c( m , 0 );
   for( int j = 0 ; j < n ; ++j )
    if( S >> j & 1 ) for( int e : Y[ j ] ) ++c[ e ];
   ref = std::all_of( c.begin() , c.end() ,
                      []( int x ) { return( x == 1 ); } );
   }
  CHECK( exactCover3( Y , m ).has_value() == ref );
  }
 }

// the dimension of an integer n is d = log n bits: a loop of log n steps is
// polynomial in d, one of n steps exponential (exercises 18.8 to 18.10:
// Counting Sort is O( n + k ) = O( n + 2^d ), tartaglia() Theta( n^2 ) =
// Theta( 4^d ), both pseudopolynomial); from n = 1 (d = 0), and on the
// largest n of d + 1 bits, 2^( d + 1 ) - 1, the halving takes d steps too
ADS_TEST( "ex18.8" )
{
 for( int d = 0 ; d < 30 ; ++d ) {
  const long n = 1L << d;
  long halving = 0 , counting = 0 , top = 0;
  for( long x = n ; x > 1 ; x /= 2 ) ++halving;             // Pippo()
  for( long x = 0 ; x < n ; ++x ) ++counting;               // Follia()
  for( long x = 2 * n - 1 ; x > 1 ; x /= 2 ) ++top;
  CHECK( halving == d && counting == ( 1L << d ) && top == d );
  }
 }

ADS_TEST( "ex18.9" )
{
 // the steps of Counting Sort on one item of value k - 1: Theta( k ),
 // from k = 1
 for( int d = 0 ; d < 20 ; ++d ) {
  const long k = 1L << d;
  std::vector< long > B( k , 0 );
  B[ k - 1 ] = 1;
  long steps = 0;
  for( long i = 0 ; i < k ; ++i ) {
   ++steps;
   for( long c = B[ i ] ; c > 0 ; --c ) ++steps;
   }
  CHECK( steps == k + 1 );
  }
 }

ADS_TEST( "ex18.10" )
{
 // the entries of the triangle of Tartaglia of C( n , k ): ( n + 1 )
 // ( n + 2 ) / 2, quadratic in n and so exponential in d = log2 n
 for( int d = 1 ; d < 12 ; ++d ) {
  const long n = 1L << d;
  const long entries = ( n + 1 ) * ( n + 2 ) / 2;
  CHECK( entries > ( 1L << ( 2 * d - 1 ) ) );
  }
 }

// INSIEME INDIPENDENTE to COPERTURA CON NODI: S independent if and only if
// V - S covers the edges; k = 0 and k = n, k > n and a negative cover, the
// empty graph, and the loops, which both problems ignore (a node with a
// loop made independentSet() violate its postcondition before the fix)
ADS_TEST( "ex18.11" )
{
 CHECK( *independentSet( { { true } } , 1 ) == Nodes( { 0 } ) );
 CHECK( vertexCover( { { true } } , 0 )->empty() );
 CHECK( independentSet( {} , 0 ) && ! independentSet( {} , 1 ) );
 CHECK( vertexCover( {} , 0 ) && ! vertexCover( {} , -1 ) );
 CHECK_THROWS( independentSet( { { false , true } , { false , false } } , 1 ) ,
               contract_violation );
 CHECK_THROWS( vertexCover( { { false , true } , { false , false } } , 1 ) ,
               contract_violation );
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 8 ) , k = int( g() % ( n + 1 ) );
  const auto G = random_graph( n , int( g() % 80 ) , false , g );
  CHECK( independentSet( G , k ).has_value() ==
         vertexCover( G , n - k ).has_value() );
  auto L = G;                                   // loops, ignored by both
  for( int u = 0 ; u < n ; ++u ) L[ u ][ u ] = g() % 2;
  CHECK( independentSet( L , k ).has_value() ==
         independentSet( G , k ).has_value() );
  CHECK( vertexCover( L , n - k ).has_value() ==
         vertexCover( G , n - k ).has_value() );
  CHECK( ! independentSet( G , n + 1 ) && ! vertexCover( G , -1 ) );
  CHECK( vertexCover( G , n ) && independentSet( G , 0 )->empty() );
  }
 }

// CIRCUITO HAMILTONIANO to CICLO HAMILTONIANO: both orientations; a
// circuit needs 3 nodes and a cycle 2, so that the two differ on one
// edge, and the empty graph or one node with a loop has neither; the
// complete graph has both, and the preconditions
ADS_TEST( "ex18.12" )
{
 const Adjacency K2 = { { false , true } , { true , false } };
 CHECK( ! hamiltonianCircuit( K2 ) && hamiltonianCycle( K2 ) );
 CHECK( ! hamiltonianCircuit( {} ) && ! hamiltonianCycle( {} ) );
 CHECK( ! hamiltonianCycle( { { true } } ) );
 const Adjacency K7( 7 , std::vector< bool >( 7 , true ) );
 CHECK( hamiltonianCircuit( K7 ) && hamiltonianCycle( K7 ) );
 CHECK_THROWS( hamiltonianCircuit( { { false , true } , { false , false } } ) ,
               contract_violation );
 CHECK_THROWS( hamiltonianCycle( { { false , true } } ) , contract_violation );
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 5 );
  const auto G = random_graph( n , 50 , false , g );
  CHECK( hamiltonianCircuit( G ).has_value() ==
         hamiltonianCycle( G ).has_value() );          // G is symmetric
  }
 }

// CICLO HAMILTONIANO to CAMMINO HAMILTONIANO from the new node n to 0, on
// graphs with loops too (a loop at 0 of a single node gave the path n , 0
// before the fix) and from one node; the paths of no node and of one,
// from r back to r, and the preconditions
ADS_TEST( "ex18.13" )
{
 CHECK( ! hamiltonianPath( cycleToPath( { { true } } ) , 1 , 0 ) );
 CHECK( ! hamiltonianPath( cycleToPath( { { false } } ) , 1 , 0 ) );
 CHECK_THROWS( cycleToPath( {} ) , contract_violation );
 CHECK_THROWS( cycleToPath( { { false } , {} } ) , contract_violation );
 const Adjacency K2 = { { false , true } , { true , false } };
 CHECK( hamiltonianPath( {} ) && hamiltonianPath( {} )->empty() );
 CHECK( *hamiltonianPath( { { false } } , 0 , 0 ) == Nodes( { 0 } ) );
 CHECK( ! hamiltonianPath( K2 , 0 , 0 ) && hamiltonianPath( K2 , 1 , 0 ) );
 CHECK_THROWS( hamiltonianPath( { { false } } , 1 ) , contract_violation );
 CHECK_THROWS( hamiltonianPath( { { false } } , -2 ) , contract_violation );
 CHECK_THROWS( hamiltonianPath( { { false } } , 0 , 1 ) , contract_violation );
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 7 );
  auto G = random_graph( n , 45 , true , g );
  if( t % 2 )
   for( int u = 0 ; u < n ; ++u ) G[ u ][ u ] = g() % 2;
  const auto H = cycleToPath( G );
  CHECK( hamiltonianCycle( G ).has_value() ==
         hamiltonianPath( H , n , 0 ).has_value() );
  }
 }

// CICLO HAMILTONIANO to CATENA HAMILTONIANA: chains of three nodes and the
// pendant ends, on graphs with loops too and from one node (a single node
// with a loop had a chain before the fix of cycleToPath()); the chains of
// no node, one node and two nodes apart
ADS_TEST( "ex18.14" )
{
 CHECK( ! hamiltonianChain( cycleToChain( { { true } } ) ) );
 CHECK( ! hamiltonianChain( cycleToChain( { { false } } ) ) );
 CHECK( hamiltonianChain( {} ) && hamiltonianChain( {} )->empty() );
 CHECK( *hamiltonianChain( { { true } } ) == Nodes( { 0 } ) );
 CHECK( ! hamiltonianChain( { { false , false } , { false , false } } ) );
 CHECK_THROWS( cycleToChain( {} ) , contract_violation );
 CHECK_THROWS( hamiltonianChain( { { false , true } , { false , false } } ) ,
               contract_violation );
 std::mt19937 g( 15 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 1 + int( g() % 5 );
  auto G = random_graph( n , 50 , true , g );
  if( t % 2 )
   for( int u = 0 ; u < n ; ++u ) G[ u ][ u ] = g() % 2;
  CHECK( hamiltonianCycle( G ).has_value() ==
         hamiltonianChain( cycleToChain( G ) ).has_value() );
  }
 }

// CAMMINO MASSIMO: a Hamiltonian path from r to t is a path of n - 1 edges;
// on an acyclic graph the longest path is polynomial; from r back to r
// (no edge), k <= 0, t not reachable, a chain of 3000 nodes (no deep
// recursion trouble), and the preconditions, a cycle among them
ADS_TEST( "ex18.15" )
{
 const Adjacency P3 = { { false , true , false } , { false , false , true } ,
                        { false , false , false } };
 CHECK( *longestPath( P3 , 1 , 1 , 0 ) == Nodes( { 1 } ) );
 CHECK( ! longestPath( P3 , 1 , 1 , 1 ) && longestPath( P3 , 0 , 2 , -3 ) );
 CHECK( ! longestPath( P3 , 0 , 2 , 3 ) && ! longestPath( P3 , 2 , 0 , 0 ) );
 CHECK( longestPathDag( P3 , 0 , 2 ) == 2 &&
        longestPathDag( P3 , 1 , 1 ) == 0 );
 CHECK( longestPathDag( P3 , 2 , 0 ) == -1 );
 Adjacency Ch( 3000 , std::vector< bool >( 3000 , false ) );
 for( int u = 0 ; u + 1 < 3000 ; ++u ) Ch[ u ][ u + 1 ] = true;
 CHECK( longestPathDag( Ch , 0 , 2999 ) == 2999 );
 CHECK_THROWS( longestPath( P3 , 3 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( longestPath( P3 , 0 , -1 , 0 ) , contract_violation );
 CHECK_THROWS( longestPathDag( { { false , true } , { true , false } } , 0 ,
                               1 ) , contract_violation );
 CHECK_THROWS( longestPathDag( { { true } } , 0 , 0 ) , contract_violation );
 CHECK_THROWS( longestPathDag( P3 , 0 , 3 ) , contract_violation );
 std::mt19937 g( 16 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 2 + int( g() % 6 );
  const auto G = random_graph( n , 45 , true , g );
  CHECK( hamiltonianPath( G , 0 , n - 1 ).has_value() ==
         longestPath( G , 0 , n - 1 , n - 1 ).has_value() );
  Adjacency A( n , std::vector< bool >( n , false ) );       // acyclic
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v ) A[ u ][ v ] = g() % 2;
  int best = -1;
  for( int k = n - 1 ; k >= 0 && best < 0 ; --k )
   if( longestPath( A , 0 , n - 1 , k ) ) best = k;
  CHECK( longestPathDag( A , 0 , n - 1 ) == best );
  }
 }

// ALBERO DI COPERTURA LIMITATO with k = 2 is the Hamiltonian chain; no
// node and one node (no edges, any k), k = 0 and 1 on two nodes, a
// disconnected graph, a star of five nodes with k = 4 and k = 3, and the
// precondition
ADS_TEST( "ex18.16" )
{
 const Adjacency K2 = { { false , true } , { true , false } };
 CHECK( boundedSpanningTree( {} , 0 )->empty() );
 CHECK( boundedSpanningTree( { { true } } , 0 )->empty() );
 CHECK( ! boundedSpanningTree( K2 , 0 ) );
 CHECK( boundedSpanningTree( K2 , 1 )->size() == 1 );
 CHECK( ! boundedSpanningTree( { { false , false } , { false , false } } ,
                               5 ) );
 Adjacency S5( 5 , std::vector< bool >( 5 , false ) );
 for( int v = 1 ; v < 5 ; ++v ) S5[ 0 ][ v ] = S5[ v ][ 0 ] = true;
 CHECK( boundedSpanningTree( S5 , 4 ) && ! boundedSpanningTree( S5 , 3 ) );
 CHECK_THROWS( boundedSpanningTree( { { false , true } , { false , false } } ,
                                    2 ) , contract_violation );
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 7 );
  const auto G = random_graph( n , 50 , false , g );
  CHECK( hamiltonianChain( G ).has_value() ==
         boundedSpanningTree( G , 2 ).has_value() );
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
/*---------------------------- End File ch18.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
