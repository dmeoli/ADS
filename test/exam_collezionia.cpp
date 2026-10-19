/*--------------------------------------------------------------------------*/
/*----------------------- File exam_collezionia.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the exercises of the collections of chapters 2, 5, 6, 7, 9 and
 * 10 of the book (analysis of algorithms, trees, binary search trees,
 * hashing, graphs, heaps): each solution is implemented in the namespace
 * collezioni, at the level 3 of the contracts, or taken from the library
 * when it already has it, and checked on the examples of its text, against
 * an independent oracle on random small instances, and on its edge cases.
 * The exercises on the structures of the library (the search trees, the
 * hash tables, the graphs, the heaps) are run on its classes: the
 * insertions of the text are simulated and the shape or the table that
 * results is compared with the one of the solution. The numeric exercises
 * on recurrences evaluate the recurrence and check the order claimed by
 * the ratios to it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 2, 5, 6,
 *      7, 9 and 10, https://algoritmicamente.it/materiali/libro/capitoli/
 *      cap02.pdf, ..., cap10.pdf
 * \see A. Montresor, Algoritmicamente, Esercizi per capitolo, collezione
 *      completa
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

#include "ADS/Contract.h"
#include "ADS/algorithm/AkraBazzi.h"
#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/HeapByInsertion.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using I64 = std::int64_t;
using U64 = std::uint64_t;
using Ints = std::vector< int >;
using BT = BinaryTree< int >;
using BTPtr = std::unique_ptr< BT >;

constexpr int INFI = std::numeric_limits< int >::max();

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a random integer in [ lo , hi ]
int randInt( std::mt19937 & g , int lo , int hi )
{
 return( lo + int( g() % unsigned( hi - lo + 1 ) ) );
 }

/// n random integers in [ lo , hi ]
Ints randInts( std::mt19937 & g , int n , int lo , int hi )
{
 Ints A( n );
 for( auto & a : A )
  a = randInt( g , lo , hi );
 return( A );
 }

/// a binary tree of n nodes of random shape, the values from val(); nil if
/// n is 0
template< class F , class T = std::invoke_result_t< F > >
BinaryTree< T > * randomTree( std::mt19937 & g , int n , F && val )
{
 if( n == 0 )
  return( nullptr );
 const int k = randInt( g , 0 , n - 1 );
 auto t = new BinaryTree< T >( val() );
 if( auto l = randomTree( g , k , val ) ) t->insertLeft( l );
 if( auto r = randomTree( g , n - 1 - k , val ) ) t->insertRight( r );
 return( t );
 }

/// a binary tree of n nodes of random shape and random values in
/// [ lo , hi ]
BT * randomTree( std::mt19937 & g , int n , int lo = 0 , int hi = 9 )
{
 return( randomTree( g , n , [ & ]() { return( randInt( g , lo , hi ) ); } ) );
 }

/// the perfect binary tree of height h, the values 1 , 2 , ... by levels
BT * perfectTree( int h , int v = 1 )
{
 BT * t = new BT( v );
 if( h > 0 ) {
  t->insertLeft( perfectTree( h - 1 , 2 * v ) );
  t->insertRight( perfectTree( h - 1 , 2 * v + 1 ) );
  }
 return( t );
 }

/// the values of a binary tree in previsit, invisit or postvisit
template< class T >
std::vector< T > visit( BinaryTree< T > * t , int which )
{
 std::vector< T > out;
 auto f = [ & ]( BinaryTree< T > * u ) { out.push_back( u->read() ); };
 if( which == 0 ) pre_visit( t , f );
 else if( which == 1 ) in_visit( t , f );
 else post_visit( t , f );
 return( out );
 }

/// the number of nodes of a binary tree
int countNodes( const BT * t )
{
 return( t ? 1 + countNodes( t->left() ) + countNodes( t->right() ) : 0 );
 }

/// the depth of each node, in previsit, by a visit with a stack
std::vector< std::pair< BT * , int > > depths( BT * t )
{
 std::vector< std::pair< BT * , int > > out , S;
 if( t ) S.push_back( std::pair( t , 0 ) );
 while( ! S.empty() ) {
  const auto [ u , d ] = S.back();
  S.pop_back();
  out.push_back( std::pair( u , d ) );
  if( u->right() ) S.push_back( std::pair( u->right() , d + 1 ) );
  if( u->left() ) S.push_back( std::pair( u->left() , d + 1 ) );
  }
 return( out );
 }

/// whether two doubles agree within the relative tolerance eps
bool near( double a , double b , double eps = 1e-9 )
{
 return( std::abs( a - b ) <= eps * std::max( 1.0 , std::abs( b ) ) );
 }

/// the ratios T( n ) / g( n ) at the given points stay within a factor
/// spread of each other (T = Theta( g ) on them), while the ratios to the
/// orders g * n^{eps} and g / n^{eps} drift by more than that factor
template< class F , class Gf >
bool order( F && T , Gf && g , const std::vector< double > & at ,
            double spread , double eps )
{
 double lo = 1e300 , hi = 0;
 for( double n : at ) {
  const double r = T( n ) / g( n );
  lo = std::min( lo , r );
  hi = std::max( hi , r );
  }
 const double a = at.front() , b = at.back();
 const double up = ( T( b ) / ( g( b ) * std::pow( b , eps ) ) ) /
                   ( T( a ) / ( g( a ) * std::pow( a , eps ) ) );
 const double down = ( T( b ) * std::pow( b , eps ) / g( b ) ) /
                     ( T( a ) * std::pow( a , eps ) / g( a ) );
 return( ( hi <= spread * lo ) && ( up * spread < 1 ) &&
         ( down > spread ) );
 }

}  // end( namespace )

namespace collezioni {

/*--------------------------------------------------------------------------*/
/*------------------- 02-analisi, 1.1, Trova la costante -------------------*/
/*--------------------------------------------------------------------------*/

/// the smallest constant c, as a fraction f( 2 ) / g( 2 ), with f( n ) <=
/// c g( n ) for each n > 1, when f / g decreases for n >= 2, as for f =
/// n^2 + n + 1 and g = 2 n^3 (1.1/1): the ratio is its value at n = 2
/// \pre g( 2 ) > 0
template< class F , class G >
std::pair< I64 , I64 > constantAtTwo( F && f , G && g )
{
 ADS_PRE( g( 2 ) > 0 );
 const I64 a = f( 2 ) , b = g( 2 );
 const I64 d = std::gcd( a , b );
 return( std::pair( a / d , b / d ) );
 }

// f = n^2 + n + 1 and g = 2 n^3: c = 7 / 16, reached at n = 2, f <= c g
// exactly up to 10^5 and the ratio decreasing; the fraction reduced, a
// zero f, and g( 2 ) = 0 refused
ADS_TEST( "collezioni-02-analisi-1-1-1" )
{
 auto f = []( I64 n ) { return( n * n + n + 1 ); };
 auto g = []( I64 n ) { return( 2 * n * n * n ); };
 const auto [ a , b ] = constantAtTwo( f , g );
 CHECK( ( a == 7 ) && ( b == 16 ) );
 CHECK( b * f( 2 ) == a * g( 2 ) );           // no smaller c
 for( I64 n = 2 ; n <= 100000 ; ++n )
  CHECK( b * f( n ) <= a * g( n ) );
 for( I64 n = 2 ; n <= 1000 ; ++n )
  CHECK( f( n + 1 ) * g( n ) < f( n ) * g( n + 1 ) );
 // the fraction in lowest terms when f( 2 ) and g( 2 ) share a factor, a
 // zero f, and g( 2 ) = 0 refused
 CHECK( constantAtTwo( []( I64 n ) { return( 3 * n ); } ,
                       []( I64 n ) { return( 3 * n * n ); } ) ==
        std::pair( I64( 1 ) , I64( 2 ) ) );
 CHECK( constantAtTwo( []( I64 ) { return( I64( 0 ) ); } , g ) ==
        std::pair( I64( 0 ) , I64( 1 ) ) );
 CHECK_THROWS( constantAtTwo( f , []( I64 n ) { return( n - 2 ); } ) ,
               contract_violation );
 }

// f = n sqrt( n ) + n^2 and g = n^2: f / g = 1 + 1 / sqrt( n ) decreases,
// c = 1 + 1 / sqrt( 2 ) = ( 2 + sqrt( 2 ) ) / 2
ADS_TEST( "collezioni-02-analisi-1-1-2" )
{
 auto r = []( double n ) { return( ( n * std::sqrt( n ) + n * n ) /
                                   ( n * n ) ); };
 const double c = r( 2 );
 CHECK( near( c , 1 + 1 / std::sqrt( 2.0 ) ) &&
        near( c , ( 2 + std::sqrt( 2.0 ) ) / 2 ) );
 for( int n = 2 ; n <= 1000000 ; ++n ) {
  CHECK( r( n ) <= c * ( 1 + 1e-15 ) );
  if( n > 2 ) CHECK( r( n ) < r( n - 1 ) );
  }
 }

// f = n^2 - n + 1 and g = n^2 / 2: f <= 2 g, i.e., 2 f <= 2 n^2, for each
// n > 1, and no c < 2 works, the ratio 2 - 2 / n + 2 / n^2 tending to 2
ADS_TEST( "collezioni-02-analisi-1-1-3" )
{
 auto f = []( I64 n ) { return( n * n - n + 1 ); };
 for( I64 n = 2 ; n <= 1000000 ; ++n )
  CHECK( 2 * f( n ) <= 2 * ( n * n ) );       // f <= 2 g, g = n^2 / 2
 CHECK( 2.0 * f( 1000 ) / ( 1000.0 * 1000.0 ) > 1.99 );
 for( double c : { 1.5 , 1.9 , 1.999 } ) {   // c < 2 fails for some n
  I64 n = 2;
  while( 2.0 * f( n ) <= c * double( n * n ) )
   ++n;
  CHECK( n < 100000 );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------- 02-analisi, 1.4, Esercizio 2.9 del libro ----------------*/
/*--------------------------------------------------------------------------*/

/// whether A has a value twice: A sorted by merge sort, then two equal
/// neighbours sought, O( n log n ) time (the upper bound; the lower bound
/// Omega( n ) holds since each place must be read)
bool hasDuplicate( std::vector< int > A )
{
 mergeSort( A );
 for( std::size_t i = 1 ; i < A.size() ; ++i )
  if( A[ i - 1 ] == A[ i ] )
   return( true );
 return( false );
 }

// against std::set on random vectors of few values, the edge cases (empty,
// one item, two equal, all equal, the repetition at the ends, the extremes
// of int), and the lower bound: in a vector of distinct values, each place,
// changed to another value, can make a repetition, so every place must be
// read
ADS_TEST( "collezioni-02-analisi-1-4" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const int n = randInt( g , 0 , 30 );
  const Ints A = randInts( g , n , 0 , std::max( 1 , 2 * n ) );
  CHECK( hasDuplicate( A ) ==
         ( std::set< int >( A.begin() , A.end() ).size() < A.size() ) );
  }
 CHECK( ! hasDuplicate( {} ) && ! hasDuplicate( { 5 } ) );
 CHECK( hasDuplicate( { 7 , 7 } ) && hasDuplicate( Ints( 100 , 3 ) ) );
 Ints A( 50 );
 std::iota( A.begin() , A.end() , 0 );
 std::shuffle( A.begin() , A.end() , g );
 CHECK( ! hasDuplicate( A ) );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  Ints B = A;
  B[ i ] = A[ ( i + 1 ) % A.size() ];
  CHECK( hasDuplicate( B ) );
  }
 CHECK( hasDuplicate( { std::numeric_limits< int >::max() , 0 ,
                        std::numeric_limits< int >::max() } ) );
 // the repetition at the two ends, and the two extremes of int distinct
 CHECK( hasDuplicate( { 4 , 1 , 2 , 3 , 4 } ) );
 CHECK( ! hasDuplicate( { std::numeric_limits< int >::min() ,
                          std::numeric_limits< int >::max() } ) );
 CHECK( hasDuplicate( { std::numeric_limits< int >::min() , 5 ,
                        std::numeric_limits< int >::min() } ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 02-analisi, 1.5, Ricorrenze -----------------------*/
/*--------------------------------------------------------------------------*/

/// the asymptotic order of two growths, with a tolerance on the exponents
/// that the logarithms of master() leave: -1 if a grows slower, 0 if they
/// are the same order, 1 if faster
int compareGrowth( const Growth & a , const Growth & b )
{
 for( auto [ x , y ] : { std::pair( a.c , b.c ) , std::pair( a.p , b.p ) ,
                         std::pair( a.q , b.q ) , std::pair( a.r , b.r ) } )
  if( std::abs( x - y ) > 1e-9 )
   return( x < y ? -1 : 1 );
 return( 0 );
 }

/// T( n ) = 4 T( n / 2 ) + n^beta at n = 2^k, T( 1 ) = 1; master( 4 , 2 ,
/// beta ) gives its order, the case 1, 2 or 3 for beta = 1, 2, 3
double fourHalves( double n , double beta )
{
 double t = 1;
 for( double m = 2 ; m <= n ; m *= 2 )
  t = 4 * t + std::pow( m , beta );
 return( t );
 }

// T( n ) = 4 T( n / 2 ) + n^beta for beta = 1, 2, 3: the three cases of
// the Master theorem, and the ratio of T( 2^k ) to the order flat while the
// ratio to the previous order grows
void masterCase( double beta , const Growth & e , const Growth & below )
{
 const Growth g = master( 4 , 2 , beta );
 CHECK( compareGrowth( g , e ) == 0 );
 auto T = [ & ]( double n ) { return( fourHalves( n , beta ) ); };
 auto G = [ & ]( double n ) { return( g.at( n ) ); };
 const std::vector< double > at = { std::ldexp( 1.0 , 10 ) ,
                                    std::ldexp( 1.0 , 20 ) ,
                                    std::ldexp( 1.0 , 30 ) ,
                                    std::ldexp( 1.0 , 40 ) };
 CHECK( order( T , G , at , 1.2 , 0.1 ) );
 const double a = at.front() , b = at.back();
 CHECK( ( T( b ) / below.at( b ) ) / ( T( a ) / below.at( a ) ) > 3 );
 }

ADS_TEST( "collezioni-02-analisi-1-5-1" )
{
 masterCase( 1 , Growth{ 1 , 2 , 0 , 0 } , Growth{ 1 , 1 , 0 , 0 } );
 }

ADS_TEST( "collezioni-02-analisi-1-5-2" )
{
 masterCase( 2 , Growth{ 1 , 2 , 1 , 0 } , Growth{ 1 , 2 , 0 , 0 } );
 }

ADS_TEST( "collezioni-02-analisi-1-5-3" )
{
 masterCase( 3 , Growth{ 1 , 3 , 0 , 0 } , Growth{ 1 , 2 , 1 , 0 } );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 02-analisi, 1.6 and 1.7, Esercizio -------------------*/
/*--------------------------------------------------------------------------*/

/// T( n ) = 2 T( n / 8 ) + 2 T( n / 4 ) + f( n ), T( n ) = 1 for n <= 1,
/// with the integer divisions and a table of the values met
/// \pre n >= 0
template< class F >
double eighthsQuarters( I64 n , F && f ,
                        std::unordered_map< I64 , double > & memo )
{
 ADS_PRE( n >= 0 );
 if( n <= 1 )
  return( 1 );
 if( const auto it = memo.find( n ) ; it != memo.end() )
  return( it->second );
 const double v = 2 * eighthsQuarters( n / 8 , f , memo ) +
                  2 * eighthsQuarters( n / 4 , f , memo ) + f( double( n ) );
 memo[ n ] = v;
 return( v );
 }

// T( n ) = 2 T( n / 8 ) + 2 T( n / 4 ) + n: n <= T( n ) <= 4 n, the
// constant of the substitution, on every n up to 10^5 and on random n up to
// 10^15, and the ratio to n flat; the base cases, n = 4 and 8 by hand, n <
// 0 refused
ADS_TEST( "collezioni-02-analisi-1-6" )
{
 std::unordered_map< I64 , double > memo;
 auto T = [ & ]( double n ) {
  return( eighthsQuarters( I64( n ) , []( double x ) { return( x ); } ,
                           memo ) );
  };
 for( I64 n = 1 ; n <= 100000 ; ++n )
  CHECK( ( T( n ) >= n ) && ( T( n ) <= 4.0 * n ) );
 std::mt19937_64 g( 6 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const I64 n = I64( g() % 1000000000000000 ) + 1;
  CHECK( ( T( n ) >= n ) && ( T( n ) <= 4.0 * n ) );
  }
 CHECK( order( T , []( double n ) { return( n ); } ,
               { 1e4 , 1e6 , 1e8 , 1e10 , 1e12 } , 2 , 0.1 ) );
 CHECK( T( 0 ) == 1 && T( 1 ) == 1 && T( 2 ) == 6 );
 // the first n with a nonzero n / 4 and n / 8, and n < 0 refused
 CHECK( T( 4 ) == 2 * 1 + 2 * 1 + 4 && T( 8 ) == 2 * 1 + 2 * 6 + 8 );
 CHECK_THROWS( eighthsQuarters( -1 , []( double x ) { return( x ); } ,
                                memo ) , contract_violation );
 }

// T( n ) = 2 T( n / 8 ) + 2 T( n / 4 ) + n log n: n log n <= T( n ) <= 4 n
// log n for n >= 2, and the ratio to n log n flat
ADS_TEST( "collezioni-02-analisi-1-7" )
{
 std::unordered_map< I64 , double > memo;
 auto nlog = []( double n ) { return( n * std::log2( n ) ); };
 auto T = [ & ]( double n ) {
  return( eighthsQuarters( I64( n ) , nlog , memo ) );
  };
 for( I64 n = 2 ; n <= 100000 ; ++n )
  CHECK( ( T( n ) >= nlog( n ) ) && ( T( n ) <= 4 * nlog( n ) ) );
 std::mt19937_64 g( 7 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const I64 n = I64( g() % 1000000000000000 ) + 2;
  CHECK( ( T( n ) >= nlog( n ) ) && ( T( n ) <= 4 * nlog( n ) ) );
  }
 CHECK( order( T , nlog , { 1e4 , 1e6 , 1e8 , 1e10 , 1e12 } , 2 , 0.1 ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------- 02-analisi, 1.8, Funzione misteriosa 1 -----------------*/
/*--------------------------------------------------------------------------*/

/// mystery( n ) of the text, which counts its calls: 0 for n = 0, 1 for n
/// odd, mystery( n / 2 ) + mystery( n / 2 ) + 1 otherwise; with n = 2^k q,
/// q odd, it makes 2^{k + 1} - 1 calls, Theta( n ) for a power of two and
/// Theta( 1 ) for n odd
/// \pre n >= 0
I64 mystery1( I64 n , I64 & calls )
{
 ADS_PRE( n >= 0 );
 ++calls;
 if( n == 0 )
  return( 0 );
 if( n % 2 == 1 )
  return( 1 );
 return( mystery1( n / 2 , calls ) + mystery1( n / 2 , calls ) + 1 );
 }

// with n = 2^k q, q odd, both the calls and the value are 2^{k + 1} - 1:
// linear on the powers of two, one call on the odd numbers
ADS_TEST( "collezioni-02-analisi-1-8" )
{
 for( I64 n = 1 ; n <= 5000 ; ++n ) {
  I64 calls = 0;
  const I64 v = mystery1( n , calls );
  const I64 e = ( I64( 2 ) << std::countr_zero( U64( n ) ) ) - 1;
  CHECK( ( calls == e ) && ( v == e ) );
  }
 I64 calls = 0;
 CHECK( mystery1( 0 , calls ) == 0 && calls == 1 );
 calls = 0;
 CHECK( mystery1( I64( 1 ) << 20 , calls ) == ( I64( 1 ) << 21 ) - 1 );
 CHECK( calls == 2 * ( I64( 1 ) << 20 ) - 1 );
 calls = 0;
 mystery1( ( I64( 1 ) << 40 ) + 1 , calls );
 CHECK( calls == 1 );
 CHECK_THROWS( mystery1( -2 , calls ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------- 02-analisi, 1.9, Funzione misteriosa 2 -----------------*/
/*--------------------------------------------------------------------------*/

/// mystery( A , n ) of the text, which counts the iterations of its loop:
/// k is the mean of A[ 0 .. n - 1 ], and for n > 44 the result is 3
/// mystery( A , n - 2 ) + k; one call per level, T( n ) = T( n - 2 ) +
/// Theta( n ), Theta( n^2 )
/// \pre 0 <= n <= |A|
double mystery2( const std::vector< double > & A , int n , I64 & loops )
{
 ADS_PRE( ( n >= 0 ) && ( std::size_t( n ) <= A.size() ) );
 double k = 0;
 for( int i = 0 ; i < n ; ++i ) {
  k += A[ i ] / n;
  ++loops;
  }
 if( n > 44 )
  return( 3 * mystery2( A , n - 2 , loops ) + k );
 return( k );
 }

// the iterations of the loop are n + ( n - 2 ) + ... down to the first
// size <= 44, about n^2 / 4: one recursive call, the 3 multiplies a value;
// n = 0, 44, 45, 46, 48, and n out of [ 0 , |A| ] refused
ADS_TEST( "collezioni-02-analisi-1-9" )
{
 const std::vector< double > Z( 20000 , 0.0 ) , O( 100 , 1.0 );
 for( int n = 0 ; n <= 600 ; ++n ) {
  I64 loops = 0 , e = 0;
  CHECK( mystery2( Z , n , loops ) == 0 );
  for( int m = n ; ; m -= 2 ) {
   e += m;
   if( m <= 44 ) break;
   }
  CHECK( loops == e );
  }
 I64 loops = 0;
 mystery2( Z , 20000 , loops );
 const double r = loops / ( 20000.0 * 20000.0 / 4 );
 CHECK( ( r > 0.99 ) && ( r < 1.01 ) );
 loops = 0;
 CHECK( near( mystery2( O , 44 , loops ) , 1 ) && loops == 44 );
 CHECK( near( mystery2( O , 46 , loops ) , 4 ) );    // 3 * 1 + 1
 CHECK( near( mystery2( O , 48 , loops ) , 13 ) );   // 3 * 4 + 1
 // n = 0 (no loop, the mean 0), an odd n above 44 (45 then 43), and n out
 // of [ 0 , |A| ] refused
 loops = 0;
 CHECK( mystery2( O , 0 , loops ) == 0 && loops == 0 );
 CHECK( near( mystery2( O , 45 , loops ) , 4 ) && loops == 45 + 43 );
 CHECK_THROWS( mystery2( O , 101 , loops ) , contract_violation );
 CHECK_THROWS( mystery2( O , -1 , loops ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---- 02-analisi, 1.11 and 1.12, Scegli la funzione, Quando conviene? -----*/
/*--------------------------------------------------------------------------*/

/// 1.11: -1 if the recursive algorithm T( n ) = a T( n / 4 ) + 1 is faster
/// than the iterative Theta( n^2 ), 0 if the same order, 1 if slower
/// \pre a >= 1
int recursiveVsQuadratic( int a )
{
 ADS_PRE( a >= 1 );
 return( compareGrowth( master( a , 4 , 0 ) , Growth{ 1 , 2 , 0 , 0 } ) );
 }

/// 1.12: the largest a with a T'( n / 4 ) + n^2 asymptotically faster than
/// 7 T( n / 2 ) + n^2: the exponent log_4 a below log_2 7 = log_4 49
int largestFasterA( void )
{
 const Growth A = master( 7 , 2 , 2 );
 int best = 0;
 for( int a = 1 ; a <= 1000 ; ++a )
  if( compareGrowth( master( a , 4 , 2 ) , A ) < 0 )
   best = a;
 return( best );
 }

// a T( n / 4 ) + 1 against n^2: the recursive one is better for a < 16,
// the same order for a = 16, worse beyond; T( 4^k ) has the order of
// master()
ADS_TEST( "collezioni-02-analisi-1-11" )
{
 for( int a = 1 ; a <= 40 ; ++a )
  CHECK( recursiveVsQuadratic( a ) == ( a < 16 ? -1 : ( a == 16 ? 0 : 1 ) ) );
 CHECK( compareGrowth( master( 1 , 4 , 0 ) , Growth{ 1 , 0 , 1 , 0 } ) ==
        0 );                                   // log n for a = 1
 for( int a : { 2 , 5 , 16 , 20 } ) {
  auto T = [ & ]( double n ) {
   double t = n <= 1 ? n : 1;
   for( double m = 4 ; m <= n ; m *= 4 )
    t = a * t + 1;
   return( t );
   };
  const Growth g = master( a , 4 , 0 );
  CHECK( order( T , [ & ]( double n ) { return( g.at( n ) ); } ,
                { std::ldexp( 1.0 , 20 ) , std::ldexp( 1.0 , 40 ) ,
                  std::ldexp( 1.0 , 60 ) } , 1.5 , 0.1 ) );
  }
 CHECK_THROWS( recursiveVsQuadratic( 0 ) , contract_violation );
 }

// a = 48: log_4 48 < log_2 7 = log_4 49, whose order is the same
ADS_TEST( "collezioni-02-analisi-1-12" )
{
 CHECK( largestFasterA() == 48 );
 const Growth A = master( 7 , 2 , 2 );
 CHECK( compareGrowth( master( 49 , 4 , 2 ) , A ) == 0 );
 CHECK( compareGrowth( master( 47 , 4 , 2 ) , A ) < 0 );
 CHECK( compareGrowth( master( 16 , 4 , 2 ) , Growth{ 1 , 2 , 1 , 0 } ) ==
        0 );
 CHECK( compareGrowth( master( 15 , 4 , 2 ) , Growth{ 1 , 2 , 0 , 0 } ) ==
        0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------- 02-analisi, 1.13, Limite superiore-inferiore --------------*/
/*--------------------------------------------------------------------------*/

/// the exponent p of Akra and Bazzi for T( n ) = 2 T( n / 3 ) + T( 2 n /
/// 3 ) + 1, the root of 2 ( 1 / 3 )^p + ( 2 / 3 )^p = 1, by the bisection
/// of akraBazziExponent() of the library
double akraBazziThirds( void )
{
 return( akraBazziExponent( { { 2 , 1.0 / 3 } , { 1 , 2.0 / 3 } } ) );
 }

/// T( n ) = 2 T( n / 3 ) + T( 2 n / 3 ) + 1, T( n ) = 1 for n <= 1, with
/// the integer divisions and a table of the values met
/// \pre n >= 0
double thirds( I64 n , std::unordered_map< I64 , double > & memo )
{
 ADS_PRE( n >= 0 );
 if( n <= 1 )
  return( 1 );
 if( const auto it = memo.find( n ) ; it != memo.end() )
  return( it->second );
 const double v = 2 * thirds( n / 3 , memo ) + thirds( 2 * n / 3 , memo ) +
                  1;
 memo[ n ] = v;
 return( v );
 }

// p = 1.394954..., the bounds n <= T( n ) <= 3 n^2 of the solution, and
// T( n ) / n^p within a constant factor from 10^3 to 10^12; T( 0 .. 3 ) by
// hand, n < 0 refused
ADS_TEST( "collezioni-02-analisi-1-13" )
{
 const double p = akraBazziThirds();
 CHECK( std::abs( p - 1.394954 ) < 1e-6 );
 CHECK( near( 2 * std::pow( 1.0 / 3 , p ) + std::pow( 2.0 / 3 , p ) , 1 ) );
 std::unordered_map< I64 , double > memo;
 auto T = [ & ]( double n ) { return( thirds( I64( n ) , memo ) ); };
 for( I64 n = 1 ; n <= 100000 ; ++n )
  CHECK( ( T( n ) >= n ) && ( T( n ) <= 3.0 * n * n ) );
 CHECK( order( T , [ & ]( double n ) { return( std::pow( n , p ) ); } ,
               { 1e3 , 1e5 , 1e7 , 1e9 , 1e11 , 1e12 } , 3 , 0.15 ) );
 // the base cases and the first values by hand, n < 0 refused
 CHECK( T( 0 ) == 1 && T( 1 ) == 1 && T( 2 ) == 2 * 1 + 1 + 1 );
 CHECK( T( 3 ) == 2 * 1 + T( 2 ) + 1 );
 CHECK_THROWS( thirds( -3 , memo ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------- 02-analisi, 1.19, Esercizio sostituzione di variabile ----------*/
/*--------------------------------------------------------------------------*/

/// the integer square root of n, the largest r with r^2 <= n
U64 isqrt( U64 n )
{
 U64 r = U64( std::sqrt( double( n ) ) );
 while( ( r > 0 ) && ( r > n / r ) )
  --r;
 while( ( r + 1 ) <= n / ( r + 1 ) )
  ++r;
 return( r );
 }

/// T( n ) = T( sqrt( n ) ) + 1, T( n ) = 1 for n <= 2, the square roots
/// rounded down: with n = 2^k and S( k ) = T( 2^k ) = S( k / 2 ) + 1,
/// Theta( log log n )
U64 sqrtRecurrence( U64 n )
{
 U64 t = 1;
 for( ; n > 2 ; n = isqrt( n ) )
  ++t;
 return( t );
 }

// T( 2^{2^j} ) = j + 1, and T( n ) - log log n in [ 0 , 2 ] for random n
// up to 2^64; S( k ) = S( k / 2 ) + 1 is the case 2 of the Master theorem;
// isqrt() at 0, 1, around the squares and at the largest square of 64 bits
ADS_TEST( "collezioni-02-analisi-1-19" )
{
 for( int j = 0 ; j <= 5 ; ++j )
  CHECK( sqrtRecurrence( U64( 1 ) << ( 1 << j ) ) == U64( j + 1 ) );
 CHECK( sqrtRecurrence( 0 ) == 1 && sqrtRecurrence( 2 ) == 1 );
 CHECK( sqrtRecurrence( 3 ) == 2 );
 std::mt19937_64 g( 19 );
 for( int t = 0 ; t < 10000 ; ++t ) {
  const U64 n = ( g() >> ( g() % 62 ) ) | 3;
  const double d = double( sqrtRecurrence( n ) ) -
                   std::log2( std::log2( double( n ) ) );
  CHECK( ( d >= 0 ) && ( d <= 2 ) );
  }
 const U64 top = std::numeric_limits< U64 >::max();
 CHECK( sqrtRecurrence( top ) == 7 );
 CHECK( isqrt( top ) == 4294967295u );
 CHECK( isqrt( 0 ) == 0 && isqrt( 1 ) == 1 && isqrt( 3 ) == 1 );
 for( U64 r = 2 ; r <= 100000 ; r += 997 )
  CHECK( isqrt( r * r ) == r && isqrt( r * r - 1 ) == r - 1 );
 CHECK( isqrt( U64( 4294967295u ) * 4294967295u ) == 4294967295u );
 CHECK( compareGrowth( master( 1 , 2 , 0 ) , Growth{ 1 , 0 , 1 , 0 } ) ==
        0 );
 }

/*--------------------------------------------------------------------------*/
/*------- 02-analisi, 1.20, Ricorrenze con sostituzione di variabile -------*/
/*--------------------------------------------------------------------------*/

// the three recurrences are evaluated as written, in the tests; with n =
// 2^m they become S( m ) = 2 S( m / 2 ) + m, S( k ) = S( k / 2 ) + k / 2
// and S( k ) = S( k / 2 ) + 1, of the orders log n log log n, n log n and
// n log log n

// the three recurrences evaluated at n = 2^{2^j}, whose square roots are
// exact, up to n = 2^512: the ratio to the order claimed flat, the ratios
// to an order a log smaller and a log larger drifting
template< class F , class Gf , class Lo , class Hi >
void substitution( F && T , Gf && g , Lo && lo , Hi && hi )
{
 double rmin = 1e300 , rmax = 0;
 for( int j = 3 ; j <= 9 ; ++j ) {
  const double n = std::ldexp( 1.0 , 1 << j );
  const double r = T( n ) / g( n );
  rmin = std::min( rmin , r );
  rmax = std::max( rmax , r );
  }
 CHECK( rmax <= 1.4 * rmin );
 const double a = std::ldexp( 1.0 , 8 ) , b = std::ldexp( 1.0 , 512 );
 CHECK( ( T( b ) / lo( b ) ) / ( T( a ) / lo( a ) ) > 2 );
 CHECK( ( T( b ) / hi( b ) ) / ( T( a ) / hi( a ) ) < 0.5 );
 }

// T( n ) = 2 T( sqrt( n ) ) + log n: S( m ) = 2 S( m / 2 ) + m, Theta( log n
// log log n )
ADS_TEST( "collezioni-02-analisi-1-20-1" )
{
 std::function< double( double ) > T = [ & ]( double n ) {
  return( n <= 2 ? 1 : 2 * T( std::sqrt( n ) ) + std::log2( n ) );
  };
 auto L = []( double n ) { return( std::log2( n ) ); };
 substitution( T , [ & ]( double n ) { return( L( n ) * std::log2( L( n ) ) );
                                       } ,
               L , [ & ]( double n ) { return( L( n ) * L( n ) ); } );
 CHECK( T( 16 ) == 2 * ( 2 * 1 + 2 ) + 4 );   // T( 4 ) = 4, T( 16 ) = 12
 }

// T( n ) = sqrt( n ) T( sqrt( n ) ) + n log sqrt( n ): the text prints the
// last term so, as its solution reads it, while the catalogue has sqrt( n )
// log n, whose solution would be Theta( n ); S( k ) = T( 2^k ) / 2^k =
// S( k / 2 ) + k / 2, Theta( n log n )
ADS_TEST( "collezioni-02-analisi-1-20-2" )
{
 std::function< double( double ) > T = [ & ]( double n ) {
  return( n <= 2 ? 1 : std::sqrt( n ) * T( std::sqrt( n ) ) +
                       n * std::log2( std::sqrt( n ) ) );
  };
 auto L = []( double n ) { return( std::log2( n ) ); };
 substitution( T , [ & ]( double n ) { return( n * L( n ) ); } ,
               [ & ]( double n ) { return( n * std::log2( L( n ) ) ); } ,
               [ & ]( double n ) { return( n * L( n ) * L( n ) ); } );
 for( int j = 1 ; j <= 9 ; ++j ) {            // S( 2^j ) = 2^j - 1 / 2
  const double k = std::ldexp( 1.0 , j );
  CHECK( near( T( std::ldexp( 1.0 , 1 << j ) ) / std::ldexp( 1.0 , 1 << j ) ,
               k - 0.5 ) );
  }
 }

// T( n ) = sqrt( n ) T( sqrt( n ) ) + n: S( k ) = S( k / 2 ) + 1, Theta( n
// log log n )
ADS_TEST( "collezioni-02-analisi-1-20-3" )
{
 std::function< double( double ) > T = [ & ]( double n ) {
  return( n <= 2 ? 1 : std::sqrt( n ) * T( std::sqrt( n ) ) + n );
  };
 auto LL = []( double n ) { return( std::log2( std::log2( n ) ) ); };
 substitution( T , [ & ]( double n ) { return( n * LL( n ) ); } ,
               []( double n ) { return( n ); } ,
               []( double n ) { return( n * std::log2( n ) ); } );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 02-analisi, 1.26, Bandiera olandese -------------------*/
/*--------------------------------------------------------------------------*/

/// the three colors of the flag
enum class Flag { VERDE , BIANCO , ROSSO };

/// ordinaBandiera(): the greens, then the whites, then the reds, by
/// examining and swapping only: dutchNationalFlag() of the library, with
/// B[ 0 .. i - 1 ] green, B[ i .. j - 1 ] white, B[ j .. k ] not examined,
/// B[ k + 1 .. n - 1 ] red; Theta( n ) time, O( 1 ) space; the number of
/// swaps is returned
int ordinaBandiera( std::vector< Flag > & B )
{
 std::size_t swaps = 0;
 dutchNationalFlag( B , []( Flag f ) { return( int( f ) ); } , & swaps );
 return( int( swaps ) );
 }

// every vector of up to 8 colors, and random long ones: the colors in
// order, the same multiset as std::sort and threeWayQuickSort() give, at
// most n swaps
ADS_TEST( "collezioni-02-analisi-1-26" )
{
 auto check = []( const std::vector< Flag > & A ) {
  std::vector< Flag > B = A , C = A;
  const int swaps = ordinaBandiera( B );
  std::sort( C.begin() , C.end() );
  CHECK( ( B == C ) && ( swaps <= int( A.size() ) ) );
  std::vector< int > D;
  for( Flag f : A ) D.push_back( int( f ) );
  threeWayQuickSort( D );
  for( std::size_t i = 0 ; i < D.size() ; ++i )
   CHECK( D[ i ] == int( B[ i ] ) );
  };
 for( int n = 0 ; n <= 8 ; ++n ) {
  int all = 1;
  for( int i = 0 ; i < n ; ++i ) all *= 3;
  for( int c = 0 ; c < all ; ++c ) {
   std::vector< Flag > A;
   for( int x = c , i = 0 ; i < n ; ++i , x /= 3 )
    A.push_back( Flag( x % 3 ) );
   check( A );
   }
  }
 std::mt19937 g( 26 );
 for( int t = 0 ; t < 100 ; ++t ) {
  std::vector< Flag > A( randInt( g , 100 , 2000 ) );
  for( auto & f : A ) f = Flag( randInt( g , 0 , 2 ) );
  check( A );
  }
 std::vector< Flag > R( 1000 , Flag::ROSSO );
 CHECK( ordinaBandiera( R ) == 1000 );         // each red swapped once
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 02-analisi, 1.27, Somma 17 -----------------------*/
/*--------------------------------------------------------------------------*/

/// whether two values of V, distinct and positive, sum to 17: P[ x ]
/// records the values x < 17 met, each new x looks for 17 - x; Theta( n )
/// time, O( 1 ) space
/// \pre the values are positive and distinct (audit)
bool somma17( const std::vector< int > & V )
{
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( int x ) { return( x > 0 ); } ) );
 ADS_AUDIT( std::set< int >( V.begin() , V.end() ).size() == V.size() );
 bool P[ 17 ] = {};
 for( int x : V )
  if( x < 17 ) {
   if( P[ 17 - x ] )
    return( true );
   P[ x ] = true;
   }
 return( false );
 }

// against all the pairs on random vectors of distinct positive values,
// and the edge cases
ADS_TEST( "collezioni-02-analisi-1-27" )
{
 std::mt19937 g( 27 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints all( 40 );
  std::iota( all.begin() , all.end() , 1 );
  std::shuffle( all.begin() , all.end() , g );
  const Ints V( all.begin() , all.begin() + randInt( g , 0 , 12 ) );
  bool e = false;
  for( std::size_t i = 0 ; i < V.size() ; ++i )
   for( std::size_t j = i + 1 ; j < V.size() ; ++j )
    e = e || ( V[ i ] + V[ j ] == 17 );
  CHECK( somma17( V ) == e );
  }
 CHECK( ! somma17( {} ) && ! somma17( { 17 } ) && ! somma17( { 8 } ) );
 CHECK( somma17( { 8 , 9 } ) && somma17( { 100 , 1 , 50 , 16 } ) );
 CHECK( ! somma17( { 17 , 34 , std::numeric_limits< int >::max() } ) );
 CHECK_THROWS( somma17( { 0 , 17 } ) , contract_violation );
 CHECK_THROWS( somma17( { 3 , 3 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------ 02-analisi, 1.28, Analisi di funzioni ricorsive -------------*/
/*--------------------------------------------------------------------------*/

/// pippo( n ) of the text, which counts its calls: 1 for n <= 2, pippo(
/// n / 2 ) + n ( n / 2 ) for n > 99, pippo( n - 2 ) + 5 otherwise;
/// Theta( log n )
/// \pre 0 <= n <= 2^31
I64 pippo( I64 n , I64 & calls )
{
 ADS_PRE( ( n >= 0 ) && ( n <= ( I64( 1 ) << 31 ) ) );
 ++calls;
 if( n <= 2 )
  return( 1 );
 if( n > 99 ) {
  const I64 i = n / 2;
  return( pippo( i , calls ) + n * i );
  }
 return( pippo( n - 2 , calls ) + 5 );
 }

/// follia( n ) of the text, which counts its calls, the values modulo
/// 2^64: for n > 10 a loop of 4 steps and follia( n / 2 ) * follia( n / 2
/// ) + 5 n; the branch n > 20 after it is never taken; n otherwise;
/// T( n ) = 2 T( n / 2 ) + Theta( 1 ), Theta( n )
/// \pre n >= 0
U64 follia( U64 n , I64 & calls , I64 & loops )
{
 ++calls;
 if( n > 10 ) {
  for( int i = 0 ; i < 4 ; ++i )           // i = n downto n - 3
   ++loops;
  return( follia( n / 2 , calls , loops ) * follia( n / 2 , calls , loops )
          + 5 * n );
  }
 if( n > 20 )
  return( follia( n + 1 , calls , loops ) );
 return( n );
 }

// pippo(): one call more each time n doubles above 99, at most about 50
// calls below 100, so log n + O( 1 ); the values of pippo( 99 ), pippo(
// 100 ) and pippo( 101 ) by hand, the base cases, n out of range refused
ADS_TEST( "collezioni-02-analisi-1-28-1" )
{
 I64 calls = 0;
 CHECK( pippo( 50 , calls ) == 1 + 24 * 5 );
 calls = 0;
 CHECK( pippo( 100 , calls ) == 121 + 100 * 50 && calls == 26 );
 std::mt19937 g( 28 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  const I64 n = randInt( g , 50 , 1 << 30 );
  I64 a = 0 , b = 0;
  pippo( n , a );
  pippo( 2 * n , b );
  CHECK( b == a + 1 );
  CHECK( a <= std::log2( double( n ) ) + 51 );
  }
 for( I64 n = 0 ; n <= 2 ; ++n ) {
  calls = 0;
  CHECK( pippo( n , calls ) == 1 && calls == 1 );
  }
 calls = 0;
 pippo( I64( 1 ) << 31 , calls );
 CHECK( calls <= 31 + 51 );
 // the threshold 99 / 100 by hand: 99 goes down by 2 to 1 (49 calls of
 // the second branch), 101 halves to 50; n out of [ 0 , 2^31 ] refused
 calls = 0;
 CHECK( pippo( 99 , calls ) == 1 + 49 * 5 && calls == 50 );
 calls = 0;
 CHECK( pippo( 101 , calls ) == pippo( 50 , calls ) + 101 * 50 );
 CHECK_THROWS( pippo( -1 , calls ) , contract_violation );
 CHECK_THROWS( pippo( ( I64( 1 ) << 31 ) + 1 , calls ) , contract_violation );
 }

// follia(): for n = 2^k >= 16 exactly n / 4 - 1 calls, each above 10 with a
// loop of 4 steps; the calls stay below n for every n
ADS_TEST( "collezioni-02-analisi-1-28-2" )
{
 for( int k = 4 ; k <= 22 ; ++k ) {
  I64 calls = 0 , loops = 0;
  follia( U64( 1 ) << k , calls , loops );
  CHECK( calls == ( I64( 1 ) << ( k - 2 ) ) - 1 );
  CHECK( loops == 4 * ( ( I64( 1 ) << ( k - 3 ) ) - 1 ) );
  }
 for( U64 n = 0 ; n <= 10 ; ++n ) {
  I64 calls = 0 , loops = 0;
  CHECK( follia( n , calls , loops ) == n && calls == 1 && loops == 0 );
  }
 for( U64 n = 11 ; n <= 20000 ; ++n ) {
  I64 calls = 0 , loops = 0;
  follia( n , calls , loops );
  CHECK( calls < I64( n ) );
  }
 I64 calls = 0 , loops = 0;
 CHECK( follia( 11 , calls , loops ) == 5 * 5 + 55 && calls == 3 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- 05-alberi, 1.1, Visite -------------------------*/
/*--------------------------------------------------------------------------*/

// the tree of the example, its postvisit, and random trees of distinct
// labels rebuilt from their previsit and invisit by fromPreorderInorder()
// of the library; the empty tree, one node, a left and a right chain;
// visits of different labels or lengths refused
ADS_TEST( "collezioni-05-alberi-1-1" )
{
 const std::vector< char > pre = { 'A' , 'E' , 'B' , 'F' , 'G' , 'C' , 'D' ,
                                   'I' , 'H' } ,
                           in = { 'B' , 'E' , 'G' , 'F' , 'C' , 'A' , 'D' ,
                                  'H' , 'I' } ,
                           post = { 'B' , 'G' , 'C' , 'F' , 'E' , 'H' , 'I' ,
                                    'D' , 'A' };
 std::unique_ptr< BinaryTree< char > > t( fromPreorderInorder( pre , in ) );
 CHECK( to_list( t.get() ) == "( A ( E ( B () () ) ( F ( G () () ) ( C () () "
                              ") ) ) ( D () ( I ( H () () ) () ) ) )" );
 CHECK( visit( t.get() , 2 ) == post );
 std::mt19937 g( 51 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 0 , 40 );
  Ints L( n );
  std::iota( L.begin() , L.end() , 0 );
  std::shuffle( L.begin() , L.end() , g );
  std::size_t next = 0;
  BTPtr u( randomTree( g , n , [ & ]() { return( L[ next++ ] ); } ) );
  BTPtr v( fromPreorderInorder( visit( u.get() , 0 ) ,
                                visit( u.get() , 1 ) ) );
  CHECK( to_list( u.get() ) == to_list( v.get() ) );
  }
 CHECK( fromPreorderInorder( Ints() , Ints() ) == nullptr );
 CHECK_THROWS( fromPreorderInorder( Ints( { 1 , 2 } ) , Ints( { 1 , 3 } ) ) ,
               contract_violation );
 // one node, a left chain (in order reversed), a right chain (the same
 // order), visits of different lengths refused
 BTPtr one( fromPreorderInorder( Ints( { 5 } ) , Ints( { 5 } ) ) );
 CHECK( to_list( one.get() ) == "( 5 () () )" );
 BTPtr lc( fromPreorderInorder( Ints( { 1 , 2 , 3 } ) ,
                               Ints( { 3 , 2 , 1 } ) ) );
 CHECK( to_list( lc.get() ) == "( 1 ( 2 ( 3 () () ) () ) () )" );
 BTPtr rc( fromPreorderInorder( Ints( { 1 , 2 , 3 } ) ,
                               Ints( { 1 , 2 , 3 } ) ) );
 CHECK( to_list( rc.get() ) == "( 1 () ( 2 () ( 3 () () ) ) )" );
 CHECK_THROWS( fromPreorderInorder( Ints( { 1 , 2 } ) , Ints( { 1 } ) ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 05-alberi, 1.2, Lunghezza di cammino ------------------*/
/*--------------------------------------------------------------------------*/

/// pathLength(): the sum of the depths of the nodes, plRec( t , l ) adding
/// the depth l of t; Theta( n ) time
int pathLength( const BT * t , int l = 0 )
{
 if( ! t )
  return( 0 );
 return( l + pathLength( t->left() , l + 1 ) +
         pathLength( t->right() , l + 1 ) );
 }

// the perfect trees, 10 for the height 2, and random trees against the
// depths of a visit
ADS_TEST( "collezioni-05-alberi-1-2" )
{
 for( int h = 0 ; h <= 12 ; ++h ) {
  BTPtr t( perfectTree( h ) );
  int e = 0;
  for( int d = 0 ; d <= h ; ++d ) e += d << d;
  CHECK( pathLength( t.get() ) == e );
  if( h == 2 ) CHECK( e == 10 );
  }
 std::mt19937 g( 52 );
 for( int r = 0 ; r < 500 ; ++r ) {
  BTPtr t( randomTree( g , randInt( g , 0 , 60 ) ) );
  int e = 0;
  for( auto [ u , d ] : depths( t.get() ) ) e += d;
  CHECK( pathLength( t.get() ) == e );
  }
 CHECK( pathLength( nullptr ) == 0 );
 BT * p = new BT( 0 );                        // a path of 100 nodes
 BTPtr path( p );
 for( int i = 1 ; i < 100 ; ++i ) {
  p->insertRight( new BT( i ) );
  p = p->right();
  }
 CHECK( pathLength( path.get() ) == 99 * 100 / 2 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 05-alberi, 1.3, Altezza minimale --------------------*/
/*--------------------------------------------------------------------------*/

/// minHeight(): the least distance from t to a leaf of its subtree, +inf
/// (INT_MAX) for an empty subtree, so that a node with one child takes the
/// other one; Theta( n ) time
int minHeight( const BT * t )
{
 if( ! t )
  return( INFI );
 if( ! t->left() && ! t->right() )
  return( 0 );
 return( std::min( minHeight( t->left() ) , minHeight( t->right() ) ) + 1 );
 }

// the figure of the text is not in the catalogue: a tree whose nearest leaf
// is two edges from the root, though one child has a single child; then
// every node of random trees against a visit by levels down to the first
// leaf; the empty tree, one node, a chain of 100, a near leaf beside the
// chain
ADS_TEST( "collezioni-05-alberi-1-3" )
{
 BTPtr t( from_list< int >( "( 1 ( 2 () ( 4 ( 6 () () ) () ) ) ( 3 ( 5 () () "
                            ") ( 7 ( 8 () () ) () ) ) )" ) );
 CHECK( minHeight( t.get() ) == 2 && minHeight( t->left() ) == 2 );
 std::mt19937 g( 53 );
 for( int r = 0 ; r < 300 ; ++r ) {
  BTPtr u( randomTree( g , randInt( g , 1 , 50 ) ) );
  for( auto [ v , d ] : depths( u.get() ) ) {
   int e = -1;
   std::vector< BT * > L = { v };
   for( int level = 0 ; e < 0 ; ++level ) {
    std::vector< BT * > N;
    for( BT * x : L ) {
     if( ! x->left() && ! x->right() ) e = level;
     if( x->left() ) N.push_back( x->left() );
     if( x->right() ) N.push_back( x->right() );
     }
    L = N;
    }
   CHECK( minHeight( v ) == e );
   }
  }
 CHECK( minHeight( nullptr ) == INFI );
 BTPtr one( new BT( 0 ) );
 CHECK( minHeight( one.get() ) == 0 );
 // a left chain of 100 nodes (its only leaf at the bottom), and a root
 // with a leaf on one side and a long chain on the other
 BT * p = new BT( 0 );
 BTPtr chain( p );
 for( int i = 1 ; i < 100 ; ++i ) {
  p->insertLeft( new BT( i ) );
  p = p->left();
  }
 CHECK( minHeight( chain.get() ) == 99 );
 BTPtr side( new BT( 0 ) );
 side->insertRight( new BT( 1 ) );
 side->insertLeft( chain.release() );
 CHECK( minHeight( side.get() ) == 1 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 05-alberi, 1.4, Allunga l'albero --------------------*/
/*--------------------------------------------------------------------------*/

/// addChild(): to each leaf a new left child, of value the sum of the
/// values of the path from the root to the leaf; addChildRec( t , sum )
/// with sum the values above t; the new nodes are not visited; Theta( n )
void addChild( BT * t , int sum = 0 )
{
 if( ! t )
  return;
 sum += t->read();
 if( ! t->left() && ! t->right() )
  t->insertLeft( new BT( sum ) );
 else {
  addChild( t->left() , sum );
  addChild( t->right() , sum );
  }
 }

// the figure: the leaves 2 and 1 under 3 under 1 get 6 and 5; random trees
// against the sums of the paths taken before
ADS_TEST( "collezioni-05-alberi-1-4" )
{
 BTPtr t( from_list< int >( "( 1 ( 3 ( 2 () () ) ( 1 () () ) ) () )" ) );
 addChild( t.get() );
 CHECK( to_list( t.get() ) == "( 1 ( 3 ( 2 ( 6 () () ) () ) ( 1 ( 5 () () ) "
                              "() ) ) () )" );
 std::mt19937 g( 54 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 1 , 50 );
  BTPtr u( randomTree( g , n , -9 , 9 ) );
  std::vector< std::pair< BT * , int > > leaves;
  std::function< void( BT * , int ) > rec = [ & ]( BT * v , int s ) {
   s += v->read();
   if( ! v->left() && ! v->right() ) leaves.push_back( std::pair( v , s ) );
   if( v->left() ) rec( v->left() , s );
   if( v->right() ) rec( v->right() , s );
   };
  rec( u.get() , 0 );
  addChild( u.get() );
  CHECK( countNodes( u.get() ) == n + int( leaves.size() ) );
  for( auto [ v , s ] : leaves )
   CHECK( v->left() && ! v->right() && ( v->left()->read() == s ) &&
          ! v->left()->left() && ! v->left()->right() );
  }
 addChild( nullptr );
 BTPtr one( new BT( 7 ) );
 addChild( one.get() );
 CHECK( to_list( one.get() ) == "( 7 ( 7 () () ) () )" );
 }

/*--------------------------------------------------------------------------*/
/*------- 05-alberi, 1.5 and 1.6, Nodi a profondita' e ad altezza k --------*/
/*--------------------------------------------------------------------------*/

/// countDepthK(): the nodes at depth k, never below it; O( n ) time
/// \pre k >= 0
int countDepthK( const BT * t , int k )
{
 ADS_PRE( k >= 0 );
 if( ! t )
  return( 0 );
 if( k == 0 )
  return( 1 );
 return( countDepthK( t->left() , k - 1 ) +
         countDepthK( t->right() , k - 1 ) );
 }

/// chkRec(): the height of t, -1 if empty, and its nodes of height k
std::pair< int , int > chkRec( const BT * t , int k )
{
 if( ! t )
  return( std::pair( -1 , 0 ) );
 const auto [ hl , cl ] = chkRec( t->left() , k );
 const auto [ hr , cr ] = chkRec( t->right() , k );
 const int h = std::max( hl , hr ) + 1;
 return( std::pair( h , cl + cr + ( h == k ) ) );
 }

/// countHeightK(): the nodes of height k, in a single postvisit; Theta( n )
int countHeightK( const BT * t , int k )
{
 return( chkRec( t , k ).second );
 }

// the nodes at depth k against the depths of a visit, on random trees
ADS_TEST( "collezioni-05-alberi-1-5" )
{
 std::mt19937 g( 55 );
 for( int r = 0 ; r < 500 ; ++r ) {
  BTPtr t( randomTree( g , randInt( g , 0 , 60 ) ) );
  std::map< int , int > level;
  for( auto [ u , d ] : depths( t.get() ) ) ++level[ d ];
  for( int k = 0 ; k <= 62 ; ++k )
   CHECK( countDepthK( t.get() , k ) == level[ k ] );
  }
 BTPtr p( perfectTree( 10 ) );
 for( int k = 0 ; k <= 10 ; ++k )
  CHECK( countDepthK( p.get() , k ) == ( 1 << k ) );
 CHECK( countDepthK( p.get() , 11 ) == 0 && countDepthK( nullptr , 0 ) == 0 );
 CHECK_THROWS( countDepthK( p.get() , -1 ) , contract_violation );
 }

// the nodes of height k against the heights computed node by node, on
// random trees; the perfect tree has 2^{h - k} nodes of height k
ADS_TEST( "collezioni-05-alberi-1-6" )
{
 std::function< int( const BT * ) > height = [ & ]( const BT * u ) {
  return( u ? 1 + std::max( height( u->left() ) , height( u->right() ) )
            : -1 );
  };
 std::mt19937 g( 56 );
 for( int r = 0 ; r < 500 ; ++r ) {
  BTPtr t( randomTree( g , randInt( g , 0 , 60 ) ) );
  std::map< int , int > byHeight;
  for( auto [ u , d ] : depths( t.get() ) ) ++byHeight[ height( u ) ];
  for( int k = -1 ; k <= 61 ; ++k )
   CHECK( countHeightK( t.get() , k ) == ( k < 0 ? 0 : byHeight[ k ] ) );
  }
 BTPtr p( perfectTree( 9 ) );
 for( int k = 0 ; k <= 9 ; ++k )
  CHECK( countHeightK( p.get() , k ) == ( 1 << ( 9 - k ) ) );
 CHECK( countHeightK( nullptr , 0 ) == 0 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 05-alberi, 1.7, Larghezza ------------------------*/
/*--------------------------------------------------------------------------*/

/// breadth(), by levels: count is the number of nodes of the level left
/// in the queue; when it runs out the queue holds the next level;
/// Theta( n ) time
/// \pre t is a node
template< class T >
int breadthBfs( Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 std::vector< Tree< T > * > Q = { t };
 std::size_t head = 0;
 int count = 1 , breadth = 1;
 while( head < Q.size() ) {
  Tree< T > * u = Q[ head++ ];
  for( Tree< T > * c = u->leftmostChild() ; c ; c = c->rightSibling() )
   Q.push_back( c );
  if( --count == 0 ) {
   count = int( Q.size() - head );
   breadth = std::max( breadth , count );
   }
  }
 return( breadth );
 }

/// breadth(), in depth: count[ l ] the nodes met at the level l, a new
/// place for each new level; Theta( n ) time
/// \pre t is a node
template< class T >
int breadthDfs( Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 std::vector< int > count;
 std::function< void( Tree< T > * , std::size_t ) > rec =
  [ & ]( Tree< T > * u , std::size_t level ) {
  if( count.size() == level )
   count.push_back( 0 );
  ++count[ level ];
  for( Tree< T > * c = u->leftmostChild() ; c ; c = c->rightSibling() )
   rec( c , level + 1 );
  };
 rec( t , 0 );
 return( *std::max_element( count.begin() , count.end() ) );
 }

// both versions against the levels of the nodes, met by the bfs() of the
// library and measured by their parents, on random ordered trees; a star,
// a path, a single node
ADS_TEST( "collezioni-05-alberi-1-7" )
{
 std::mt19937 g( 57 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 1 , 80 );
  std::vector< Tree< int > * > N = { new Tree< int >( 0 ) };
  std::unique_ptr< Tree< int > > root( N[ 0 ] );
  for( int i = 1 ; i < n ; ++i ) {
   N.push_back( new Tree< int >( i ) );
   N[ randInt( g , 0 , i - 1 ) ]->insertChild( N.back() );
   }
  std::map< int , int > level;
  bfs( root.get() , [ & ]( Tree< int > * u ) {
   int d = 0;
   for( ; u->parent() ; u = u->parent() ) ++d;
   ++level[ d ];
   } );
  int e = 0;
  for( auto [ d , c ] : level ) e = std::max( e , c );
  CHECK( breadthBfs( root.get() ) == e && breadthDfs( root.get() ) == e );
  }
 std::unique_ptr< Tree< int > > star( new Tree< int >( 0 ) ) ,
                                path( new Tree< int >( 0 ) );
 Tree< int > * p = path.get();
 for( int i = 1 ; i <= 100 ; ++i ) {
  star->insertChild( new Tree< int >( i ) );
  p->insertChild( new Tree< int >( i ) );
  p = p->leftmostChild();
  }
 CHECK( breadthBfs( star.get() ) == 100 && breadthDfs( star.get() ) == 100 );
 CHECK( breadthBfs( path.get() ) == 1 && breadthDfs( path.get() ) == 1 );
 CHECK_THROWS( breadthBfs< int >( nullptr ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 05-alberi, 1.8, Albero speculare --------------------*/
/*--------------------------------------------------------------------------*/

/// mirrorInPlace(): t mirrored in place as in the text, the two subtrees of
/// each node detached and inserted back swapped; Theta( n ) time
template< class T >
void mirrorInPlace( BinaryTree< T > * t )
{
 if( ! t )
  return;
 BinaryTree< T > * l = t->detachLeft() , * r = t->detachRight();
 mirrorInPlace( l );
 mirrorInPlace( r );
 if( r ) t->insertLeft( r );
 if( l ) t->insertRight( l );
 }

/// specular(): the mirror of t, with left and right swapped at each node,
/// made in place on a copy of t; Theta( n ) time; the caller owns it
template< class T >
BinaryTree< T > * specular( const BinaryTree< T > * t )
{
 BinaryTree< T > * m = copy( t );
 mirrorInPlace( m );
 return( m );
 }

// the invisit of the mirror is the reversed invisit, its previsit visits
// the root, then the right, then the left subtree, twice gives the tree;
// the empty tree, one node, a perfect tree, the original left untouched
ADS_TEST( "collezioni-05-alberi-1-8" )
{
 std::mt19937 g( 58 );
 for( int r = 0 ; r < 500 ; ++r ) {
  BTPtr t( randomTree( g , randInt( g , 0 , 60 ) , 0 , 99 ) );
  BTPtr m( specular( t.get() ) ) , mm( specular( m.get() ) );
  Ints in = visit( t.get() , 1 );
  std::reverse( in.begin() , in.end() );
  CHECK( visit( m.get() , 1 ) == in );
  Ints rl;
  std::function< void( BT * ) > rec = [ & ]( BT * u ) {
   if( ! u ) return;
   rl.push_back( u->read() );
   rec( u->right() );
   rec( u->left() );
   };
  rec( t.get() );
  CHECK( visit( m.get() , 0 ) == rl );
  CHECK( to_list( mm.get() ) == to_list( t.get() ) );
  }
 BTPtr t( from_list< int >( "( 1 ( 2 () ( 3 () () ) ) () )" ) );
 BTPtr m( specular( t.get() ) );
 CHECK( to_list( m.get() ) == "( 1 () ( 2 ( 3 () () ) () ) )" );
 CHECK( specular< int >( nullptr ) == nullptr );
 // one node (itself), a symmetric tree (the same), the original untouched
 BTPtr one( new BT( 4 ) ) , sym( perfectTree( 3 ) );
 BTPtr m1( specular( one.get() ) ) , ms( specular( sym.get() ) );
 CHECK( to_list( m1.get() ) == "( 4 () () )" );
 Ints lv = visit( sym.get() , 1 );
 std::reverse( lv.begin() , lv.end() );
 CHECK( visit( ms.get() , 1 ) == lv );
 CHECK( to_list( t.get() ) == "( 1 ( 2 () ( 3 () () ) ) () )" );
 }

/*--------------------------------------------------------------------------*/
/*--------- 05-alberi, 1.10, Alberi binari strutturalmente diversi ---------*/
/*--------------------------------------------------------------------------*/

/// P( n ), the binary trees of n nodes: P( 0 ) = 1 and P( n ) = sum_k
/// P( k ) P( n - 1 - k ), k nodes on the left; O( n^2 ) time
/// \pre 0 <= n <= 35
U64 binaryTreeShapes( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 35 ) );
 std::vector< U64 > P( n + 1 , 0 );
 P[ 0 ] = 1;
 for( int m = 1 ; m <= n ; ++m )
  for( int k = 0 ; k < m ; ++k )
   P[ m ] += P[ k ] * P[ m - 1 - k ];
 return( P[ n ] );
 }

// 1, 2, 5, 14 for 1 to 4 nodes, as many as the shapes enumerated and as the
// strings of nestedParentheses() of the library
ADS_TEST( "collezioni-05-alberi-1-10-1" )
{
 const std::vector< U64 > e = { 1 , 2 , 5 , 14 };
 std::function< std::vector< std::string >( int ) > shapes = [ & ]( int n ) {
  std::vector< std::string > out;
  if( n == 0 ) {
   out.push_back( "()" );
   return( out );
   }
  for( int k = 0 ; k < n ; ++k )
   for( const auto & l : shapes( k ) )
    for( const auto & r : shapes( n - 1 - k ) )
     out.push_back( "(" + l + r + ")" );
  return( out );
  };
 for( int n = 1 ; n <= 4 ; ++n )
  CHECK( binaryTreeShapes( n ) == e[ n - 1 ] );
 for( int n = 0 ; n <= 9 ; ++n ) {
  const auto s = shapes( n );
  CHECK( std::set< std::string >( s.begin() , s.end() ).size() ==
         binaryTreeShapes( n ) );
  CHECK( U64( nestedParentheses( n , []( const std::string & ) {
               return( false ); } ) ) == binaryTreeShapes( n ) );
  }
 }

// the recurrence against the Catalan numbers binom( 2 n , n ) / ( n + 1 ) up
// to 30, n = 0, and the largest n of the precondition; -1 and 36 refused
ADS_TEST( "collezioni-05-alberi-1-10-2" )
{
 const auto C = tartaglia( 60 );
 for( int n = 0 ; n <= 30 ; ++n )
  CHECK( binaryTreeShapes( n ) == C[ 2 * n ][ n ] / U64( n + 1 ) );
 CHECK( binaryTreeShapes( 35 ) == 3116285494907301262u );
 CHECK( binaryTreeShapes( 0 ) == 1 );           // the empty tree
 CHECK_THROWS( binaryTreeShapes( -1 ) , contract_violation );
 CHECK_THROWS( binaryTreeShapes( 36 ) , contract_violation );   // > 2^64
 }

/*--------------------------------------------------------------------------*/
/*------------------- 05-alberi, 1.11, Visite iterative --------------------*/
/*--------------------------------------------------------------------------*/

/// preorderIterative(): the root on a stack; each node popped is visited,
/// then its right and its left child pushed; Theta( n )
/// \pre t is a node
template< class T >
std::vector< T > preorderIterative( BinaryTree< T > * t )
{
 ADS_PRE( t != nullptr );
 std::vector< T > out;
 SequenceStack< List< BinaryTree< T > * > > S;
 S.push( t );
 while( ! S.empty() ) {
  BinaryTree< T > * u = S.pop();
  out.push_back( u->read() );
  if( u->right() ) S.push( u->right() );
  if( u->left() ) S.push( u->left() );
  }
 return( out );
 }

/// the postvisit (in = false) or the invisit (in = true) with a stack of
/// pairs ( node , visit now ): a node popped with false is pushed back with
/// true, after (postvisit) or between (invisit) its children; Theta( n )
/// \pre t is a node
template< class T >
std::vector< T > flaggedIterative( BinaryTree< T > * t , bool in )
{
 ADS_PRE( t != nullptr );
 using P = std::pair< BinaryTree< T > * , bool >;
 std::vector< T > out;
 SequenceStack< List< P > > S;
 S.push( P( t , false ) );
 while( ! S.empty() ) {
  const auto [ u , now ] = S.pop();
  if( now ) {
   out.push_back( u->read() );
   continue;
   }
  if( ! in ) S.push( P( u , true ) );
  if( u->right() ) S.push( P( u->right() , false ) );
  if( in ) S.push( P( u , true ) );
  if( u->left() ) S.push( P( u->left() , false ) );
  }
 return( out );
 }

/// postorderIterative()
template< class T >
std::vector< T > postorderIterative( BinaryTree< T > * t )
{
 return( flaggedIterative( t , false ) );
 }

/// inorderIterative()
template< class T >
std::vector< T > inorderIterative( BinaryTree< T > * t )
{
 return( flaggedIterative( t , true ) );
 }

// the three iterative visits against the recursive ones of the library, on
// random trees, a single node, a long left and right path; nil refused
ADS_TEST( "collezioni-05-alberi-1-11" )
{
 std::mt19937 g( 511 );
 for( int r = 0 ; r < 500 ; ++r ) {
  BTPtr t( randomTree( g , randInt( g , 1 , 80 ) , 0 , 99 ) );
  CHECK( preorderIterative( t.get() ) == visit( t.get() , 0 ) );
  CHECK( inorderIterative( t.get() ) == visit( t.get() , 1 ) );
  CHECK( postorderIterative( t.get() ) == visit( t.get() , 2 ) );
  }
 BT * p = new BT( 0 );
 BTPtr path( p );
 for( int i = 1 ; i < 5000 ; ++i ) {
  p->insertLeft( new BT( i ) );
  p = p->left();
  }
 Ints down( 5000 ) , up( 5000 );
 std::iota( down.begin() , down.end() , 0 );
 std::reverse_copy( down.begin() , down.end() , up.begin() );
 CHECK( preorderIterative( path.get() ) == down );
 CHECK( inorderIterative( path.get() ) == up );
 CHECK( postorderIterative( path.get() ) == up );
 // a right chain (in order the previsit), one node
 BT * q = new BT( 0 );
 BTPtr right( q );
 for( int i = 1 ; i < 5000 ; ++i ) {
  q->insertRight( new BT( i ) );
  q = q->right();
  }
 CHECK( preorderIterative( right.get() ) == down );
 CHECK( inorderIterative( right.get() ) == down );
 CHECK( postorderIterative( right.get() ) == up );
 BTPtr one( new BT( 9 ) );
 CHECK( preorderIterative( one.get() ) == Ints( 1 , 9 ) &&
        inorderIterative( one.get() ) == Ints( 1 , 9 ) &&
        postorderIterative( one.get() ) == Ints( 1 , 9 ) );
 CHECK_THROWS( preorderIterative< int >( nullptr ) , contract_violation );
 CHECK_THROWS( inorderIterative< int >( nullptr ) , contract_violation );
 CHECK_THROWS( postorderIterative< int >( nullptr ) , contract_violation );
 }

}  // end( namespace collezioni )

/*--------------------------------------------------------------------------*/
/*-- 06-abr, 1.1, Distribuzione di valori in un albero binario di ricerca --*/
/*--------------------------------------------------------------------------*/

/// the keys of a search tree of the library in previsit, which fix its
/// shape
template< class Tr >
std::vector< typename Tr::Key > searchPre( const Tr & t )
{
 std::vector< typename Tr::Key > out;
 std::function< void( typename Tr::Link * ) > rec = [ & ]( auto u ) {
  if( t.isNil( u ) )
   return;
  out.push_back( Tr::key( u ) );
  rec( u->left );
  rec( u->right );
  };
 rec( t.root() );
 return( out );
 }

/// the binary tree with the shape and the keys of a search tree of the
/// library; the caller owns it
template< class Tr >
BT * fromSearchTree( const Tr & t , typename Tr::Link * u )
{
 if( t.isNil( u ) )
  return( nullptr );
 BT * b = new BT( Tr::key( u ) );
 if( BT * l = fromSearchTree( t , u->left ) ) b->insertLeft( l );
 if( BT * r = fromSearchTree( t , u->right ) ) b->insertRight( r );
 return( b );
 }

/// the keys K, distinct and sorted, written on the nodes of t in invisit,
/// the only labelling that makes t a search tree; Theta( n ) time
/// \pre K is strictly increasing and has as many keys as t has nodes
void labelSearchTree( BT * t , const std::vector< int > & K )
{
 ADS_PRE( std::adjacent_find( K.begin() , K.end() ,
                              std::greater_equal<>() ) == K.end() );
 ADS_PRE( int( K.size() ) == countNodes( t ) );
 std::size_t i = 0;
 in_visit( t , [ & ]( BT * u ) { u->write( K[ i++ ] ); } );
 }

// the shape of the text labelled as in its solution; random shapes become
// search trees, and on small ones no other order of the keys does; the
// empty tree, the extremes of int; too few, repeated or decreasing keys
// refused
ADS_TEST( "collezioni-06-abr-1-1" )
{
 BTPtr t( from_list< int >( "( 0 ( 0 ( 0 ( 0 () () ) ( 0 () () ) ) ( 0 ( 0 ()"
                            " () ) () ) ) ( 0 ( 0 () () ) ( 0 () () ) ) )" ) );
 labelSearchTree( t.get() , { 7 , 12 , 15 , 17 , 18 , 21 , 23 , 32 , 33 ,
                              37 } );
 CHECK( to_list( t.get() ) == "( 23 ( 17 ( 12 ( 7 () () ) ( 15 () () ) ) ( 2"
                              "1 ( 18 () () ) () ) ) ( 33 ( 32 () () ) ( 37 "
                              "() () ) ) )" );
 CHECK( isSearchTree( t.get() ) );
 std::mt19937 g( 61 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 0 , 40 );
  BTPtr u( randomTree( g , n ) );
  std::set< int > S;
  while( int( S.size() ) < n ) S.insert( randInt( g , -1000 , 1000 ) );
  const Ints K( S.begin() , S.end() );
  labelSearchTree( u.get() , K );
  CHECK( isSearchTree( u.get() ) && ( visit( u.get() , 1 ) == K ) );
  }
 for( int n = 1 ; n <= 6 ; ++n ) {            // the labelling is unique
  BTPtr u( randomTree( g , n ) );
  Ints K( n );
  std::iota( K.begin() , K.end() , 1 );
  int good = 0;
  do {
   std::size_t i = 0;
   in_visit( u.get() , [ & ]( BT * v ) { v->write( K[ i++ ] ); } );
   good += isSearchTree( u.get() );
   } while( std::next_permutation( K.begin() , K.end() ) );
  CHECK( good == 1 );
  }
 CHECK_THROWS( labelSearchTree( t.get() , { 1 , 2 } ) , contract_violation );
 // the empty tree with no key, keys repeated or decreasing refused
 labelSearchTree( nullptr , {} );
 BTPtr two( from_list< int >( "( 0 ( 0 () () ) () )" ) );
 CHECK_THROWS( labelSearchTree( two.get() , { 3 , 3 } ) , contract_violation );
 CHECK_THROWS( labelSearchTree( two.get() , { 4 , 3 } ) , contract_violation );
 labelSearchTree( two.get() , { std::numeric_limits< int >::min() ,
                                std::numeric_limits< int >::max() } );
 CHECK( two->read() == std::numeric_limits< int >::max() );
 }

/*--------------------------------------------------------------------------*/
/*------------ 06-abr, 1.2, Versione ricorsiva di insertNode() -------------*/
/*--------------------------------------------------------------------------*/

/// a binary search tree of pairs ( key , value ) on BinaryTree
using KV = BinaryTree< std::pair< int , int > >;

/// insertNodeRec(): the key x with the value v in the search tree t,
/// whose root is returned (a new one if t is empty); the value replaced if
/// x is there; O( h ) time and stack. The text assigns the result of the
/// call to the child; here the child is linked only when it is created,
/// as BinaryTree links its children itself
KV * insertNodeRec( KV * t , int x , int v )
{
 if( ! t )
  return( new KV( std::pair( x , v ) ) );
 const int k = t->read().first;
 if( x < k ) {
  if( t->left() ) insertNodeRec( t->left() , x , v );
  else t->insertLeft( new KV( std::pair( x , v ) ) );
  }
 else if( x > k ) {
  if( t->right() ) insertNodeRec( t->right() , x , v );
  else t->insertRight( new KV( std::pair( x , v ) ) );
  }
 else
  t->write( std::pair( x , v ) );
 return( t );
 }

// random insertions, with repeated keys, give the shape of the
// BinarySearchTree of the library and the values of std::map, each child
// knowing its parent; the empty tree, a repeated root, a right chain of
// 2000 keys
ADS_TEST( "collezioni-06-abr-1-2" )
{
 std::mt19937 g( 62 );
 for( int r = 0 ; r < 300 ; ++r ) {
  std::unique_ptr< KV > t;
  BinarySearchTree< int , int > ads;
  std::map< int , int > m;
  const int n = randInt( g , 0 , 80 );
  for( int i = 0 ; i < n ; ++i ) {
   const int k = randInt( g , 0 , 50 );
   KV * root = insertNodeRec( t.get() , k , i );
   if( ! t ) t.reset( root );
   CHECK( root == t.get() );
   ads.insert( k , i );
   m[ k ] = i;
   }
  std::vector< int > pre , inK , inV;
  std::function< void( KV * ) > rec = [ & ]( KV * u ) {
   if( ! u ) return;
   pre.push_back( u->read().first );
   for( KV * c : { u->left() , u->right() } )
    CHECK( ! c || ( c->parent() == u ) );
   rec( u->left() );
   inK.push_back( u->read().first );
   inV.push_back( u->read().second );
   rec( u->right() );
   };
  rec( t.get() );
  CHECK( pre == searchPre( ads ) );
  std::vector< int > mk , mv;
  for( auto [ k , v ] : m ) { mk.push_back( k ); mv.push_back( v ); }
  CHECK( ( inK == mk ) && ( inV == mv ) );
  }
 // the first key makes the root, a repeated root only changes its value,
 // increasing keys make a right chain of 2000 (the recursion as deep)
 std::unique_ptr< KV > t( insertNodeRec( nullptr , 5 , 1 ) );
 CHECK( insertNodeRec( t.get() , 5 , 2 ) == t.get() );
 CHECK( ( t->read() == std::pair( 5 , 2 ) ) && ! t->left() && ! t->right() );
 std::unique_ptr< KV > c( insertNodeRec( nullptr , 0 , 0 ) );
 for( int k = 1 ; k < 2000 ; ++k )
  insertNodeRec( c.get() , k , k );
 int depth = 0;
 for( KV * u = c.get() ; u->right() ; u = u->right() )
  ++depth;
 CHECK( depth == 1999 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 06-abr, 1.3, Verifica ABR ------------------------*/
/*--------------------------------------------------------------------------*/

// isSearchTree() of the library, each key in the open interval that its
// ancestors leave to it: random trees against the invisit strictly
// increasing, the search trees of the library, and a violation at a
// grandchild only
ADS_TEST( "collezioni-06-abr-1-3" )
{
 std::mt19937 g( 63 );
 int yes = 0;
 for( int r = 0 ; r < 2000 ; ++r ) {
  BTPtr t( randomTree( g , randInt( g , 0 , 8 ) , 0 , 9 ) );
  const Ints in = visit( t.get() , 1 );
  const bool e = std::adjacent_find( in.begin() , in.end() ,
                                     std::greater_equal<>() ) == in.end();
  CHECK( isSearchTree( t.get() ) == e );
  yes += e;
  }
 CHECK( yes > 100 );
 for( int r = 0 ; r < 100 ; ++r ) {
  BinarySearchTree< int , int > s;
  for( int i = 0 ; i < 60 ; ++i ) s.insert( randInt( g , 0 , 99 ) , 0 );
  BTPtr t( fromSearchTree( s , s.root() ) );
  CHECK( isSearchTree( t.get() ) );
  }
 BTPtr w( from_list< int >( "( 10 ( 5 () ( 12 () () ) ) ( 15 () () ) )" ) );
 CHECK( ! isSearchTree( w.get() ) );             // each parent agrees
 w->left()->right()->write( 7 );
 CHECK( isSearchTree( w.get() ) );
 BTPtr d( from_list< int >( "( 5 ( 5 () () ) () )" ) );
 CHECK( ! isSearchTree( d.get() ) &&
        isSearchTree( static_cast< BT * >( nullptr ) ) );
 BTPtr x( from_list< int >( "( -2147483648 () ( 2147483647 () () ) )" ) );
 CHECK( isSearchTree( x.get() ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 06-abr, 1.5, Cancellazione -----------------------*/
/*--------------------------------------------------------------------------*/

/// whether the removals of x and y from the search tree of the keys K,
/// inserted in this order, give the same tree in both orders; the
/// BinarySearchTree of the library removes as removeNode() of the book
bool removalsCommute( const std::vector< int > & K , int x , int y )
{
 BinarySearchTree< int , int > a , b;
 for( int k : K ) {
  a.insert( k , k );
  b.insert( k , k );
  }
 a.remove( x );
 a.remove( y );
 b.remove( y );
 b.remove( x );
 return( searchPre( a ) == searchPre( b ) );
 }

// the counterexample of the solution on the BinarySearchTree of the
// library: 2 ( 1 , 4 ( 3 , - ) ) gives 4 ( 3 , - ) removing 1 then 2, and 3
// ( - , 4 ) removing 2 then 1; on random trees the order matters sometimes,
// never for the keys left
ADS_TEST( "collezioni-06-abr-1-5" )
{
 const Ints K = { 2 , 1 , 4 , 3 };
 CHECK( ! removalsCommute( K , 1 , 2 ) );
 BinarySearchTree< int , int > a , b;
 for( int k : K ) { a.insert( k , k ); b.insert( k , k ); }
 a.remove( 1 ); a.remove( 2 );
 b.remove( 2 ); b.remove( 1 );
 CHECK( searchPre( a ) == Ints( { 4 , 3 } ) );
 CHECK( searchPre( b ) == Ints( { 3 , 4 } ) );
 std::mt19937 g( 65 );
 int differ = 0;
 for( int r = 0 ; r < 2000 ; ++r ) {
  Ints L = randInts( g , randInt( g , 2 , 12 ) , 0 , 20 );
  const int x = L[ randInt( g , 0 , int( L.size() ) - 1 ) ] ,
            y = L[ randInt( g , 0 , int( L.size() ) - 1 ) ];
  differ += ! removalsCommute( L , x , y );
  CHECK( removalsCommute( L , x , x ) );
  std::set< int > S( L.begin() , L.end() );
  S.erase( x );
  S.erase( y );
  BinarySearchTree< int , int > c;
  for( int k : L ) c.insert( k , k );
  c.remove( x );
  c.remove( y );
  const Ints left = searchPre( c );
  CHECK( std::set< int >( left.begin() , left.end() ) == S );
  }
 CHECK( differ > 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 06-abr, 1.6, Concatenazione -----------------------*/
/*--------------------------------------------------------------------------*/

/// concatenate(): T2 hung as the right child of the maximum of T1, which
/// has none; O( h ) time; T1 owns T2 then
/// \pre T1 and T2 are search trees, not empty, and the keys of T1 are
///      smaller than those of T2 (audit)
BT * concatenate( BT * T1 , BT * T2 )
{
 ADS_PRE( T1 && T2 && ! T2->parent() );
 ADS_AUDIT( isSearchTree( T1 ) && isSearchTree( T2 ) );
 BT * t = T1;
 while( t->right() )
  t = t->right();
 const BT * m = T2;
 while( m->left() )
  m = m->left();
 ADS_PRE( t->read() < m->read() );
 t->insertRight( T2 );
 return( T1 );
 }

// random search trees split at a random key: the invisit of the result is
// the two invisits one after the other; one node each, the maximum at the
// root; keys not separated, T2 with a parent, a nil tree refused
ADS_TEST( "collezioni-06-abr-1-6" )
{
 std::mt19937 g( 66 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::set< int > S;
  const int n = randInt( g , 2 , 60 );
  while( int( S.size() ) < n ) S.insert( randInt( g , 0 , 999 ) );
  const Ints K( S.begin() , S.end() );
  const int cut = randInt( g , 1 , n - 1 );
  BinarySearchTree< int , int > a , b;
  Ints A( K.begin() , K.begin() + cut ) , B( K.begin() + cut , K.end() );
  std::shuffle( A.begin() , A.end() , g );
  std::shuffle( B.begin() , B.end() , g );
  for( int k : A ) a.insert( k , 0 );
  for( int k : B ) b.insert( k , 0 );
  BTPtr t( concatenate( fromSearchTree( a , a.root() ) ,
                        fromSearchTree( b , b.root() ) ) );
  CHECK( isSearchTree( t.get() ) && ( visit( t.get() , 1 ) == K ) );
  CHECK( countNodes( t.get() ) == n );
  }
 BTPtr x( new BT( 5 ) ) , y( new BT( 3 ) );
 CHECK_THROWS( concatenate( x.get() , y.get() ) , contract_violation );
 CHECK_THROWS( concatenate( x.get() , nullptr ) , contract_violation );
 BT * z = new BT( 9 );
 CHECK( concatenate( x.get() , z ) == x.get() && x->right() == z );
 // equal maximum and minimum, T2 already hung somewhere, T1 nil refused;
 // T1 whose maximum is its root with a left subtree
 BTPtr p( new BT( 20 ) ) , q( new BT( 20 ) );
 CHECK_THROWS( concatenate( p.get() , q.get() ) , contract_violation );
 CHECK_THROWS( concatenate( x.get() , z ) , contract_violation );
 CHECK_THROWS( concatenate( nullptr , q.get() ) , contract_violation );
 BTPtr r( from_list< int >( "( 10 ( 5 () () ) () )" ) );
 concatenate( r.get() , q.release() );
 CHECK( to_list( r.get() ) == "( 10 ( 5 () () ) ( 20 () () ) )" );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 06-abr, 1.7, Distanza minima ----------------------*/
/*--------------------------------------------------------------------------*/

/// minDistance(): two keys at the least distance, the closest pair being
/// two consecutive keys of the invisit, walked by min() and
/// successorNode(); Theta( n ) time, O( 1 ) space. For integer keys the
/// distances are computed in the unsigned type of the same width, which
/// holds the difference of two keys in order (INT_MAX - INT_MIN overflows
/// an int)
/// \pre the tree has at least two keys
template< class Tr >
std::pair< typename Tr::Key , typename Tr::Key > minDistance( const Tr & T )
{
 using K = typename Tr::Key;
 ADS_PRE( T.size() >= 2 );
 auto gap = []( const K & a , const K & b ) {     // b - a, a <= b
  if constexpr( std::is_integral_v< K > )
   return( std::make_unsigned_t< K >( b ) - std::make_unsigned_t< K >( a ) );
  else
   return( b - a );
  };
 auto u = T.min();
 auto v = T.successorNode( u );
 auto best = std::pair( Tr::key( u ) , Tr::key( v ) );
 for( ; ! T.isNil( v ) ; u = v , v = T.successorNode( v ) )
  if( gap( Tr::key( u ) , Tr::key( v ) ) < gap( best.first , best.second ) )
   best = std::pair( Tr::key( u ) , Tr::key( v ) );
 return( best );
 }

// the least distance against all the pairs, on random BinarySearchTrees of
// the library; two keys; the largest keys; the extremes of int (distances
// beyond int); a tie; fewer than two keys refused
ADS_TEST( "collezioni-06-abr-1-7" )
{
 std::mt19937 g( 67 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  BinarySearchTree< int , int > t;
  std::set< int > S;
  const int n = randInt( g , 2 , 40 );
  while( int( S.size() ) < n ) {
   const int k = randInt( g , -500 , 500 );
   S.insert( k );
   t.insert( k , 0 );
   }
  int e = INFI;
  for( int x : S )
   for( int y : S )
    if( x < y ) e = std::min( e , y - x );
  const auto [ x , y ] = minDistance( t );
  CHECK( S.count( x ) && S.count( y ) && ( x < y ) && ( y - x == e ) );
  }
 BinarySearchTree< int , int > two;
 two.insert( 1000000000 , 0 );
 two.insert( -1000000000 , 0 );
 CHECK( minDistance( two ) == std::pair( -1000000000 , 1000000000 ) );
 two.remove( 1000000000 );
 CHECK_THROWS( minDistance( two ) , contract_violation );
 // the extremes of int, whose distances overflow an int: 0 - INT_MIN is
 // 2^31, larger than INT_MAX - 0; a tie (the first pair); no key
 BinarySearchTree< int , int > ext;
 for( int k : { 0 , std::numeric_limits< int >::max() ,
                std::numeric_limits< int >::min() } )
  ext.insert( k , 0 );
 CHECK( minDistance( ext ) == std::pair( 0 , INFI ) );
 BinarySearchTree< int , int > tie;
 for( int k : { 5 , 1 , 9 , 3 } )
  tie.insert( k , 0 );
 CHECK( minDistance( tie ) == std::pair( 1 , 3 ) );
 CHECK_THROWS( minDistance( BinarySearchTree< int , int >() ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------- 06-abr, 1.8, Albero binario di ricerca di altezza minima --------*/
/*--------------------------------------------------------------------------*/

/// the height of a binary tree, -1 if empty
template< class T >
int height( const BinaryTree< T > * t )
{
 return( t ? 1 + std::max( height( t->left() ) , height( t->right() ) )
           : -1 );
 }

// minimumHeightSearchTree() of the library, the middle key at the root and
// the two halves below: the least height ceil( log2( n + 1 ) ) - 1 for n up
// to 1000, the invisit equal to A, and the previsit giving back the same
// shape when inserted in the BinarySearchTree of the library; no key;
// keys not sorted or repeated refused
ADS_TEST( "collezioni-06-abr-1-8" )
{
 for( int n = 1 ; n <= 1000 ; ++n ) {
  Ints A( n );
  for( int i = 0 ; i < n ; ++i ) A[ i ] = 3 * i - n;
  BTPtr t( minimumHeightSearchTree( A ) );
  int h = 0;
  while( ( 2 << h ) - 1 < n ) ++h;             // 2^{h + 1} - 1 >= n
  CHECK( height( t.get() ) == h );
  CHECK( h == int( std::ceil( std::log2( n + 1.0 ) ) ) - 1 );
  CHECK( isSearchTree( t.get() ) && ( visit( t.get() , 1 ) == A ) );
  BinarySearchTree< int , int > s;
  const Ints pre = visit( t.get() , 0 );
  for( int k : pre ) s.insert( k , 0 );
  CHECK( searchPre( s ) == pre );
  }
 CHECK( minimumHeightSearchTree( Ints() ) == nullptr );
 CHECK_THROWS( minimumHeightSearchTree( Ints( { 1 , 3 , 2 } ) ) ,
               contract_violation );
 CHECK_THROWS( minimumHeightSearchTree( Ints( { 1 , 1 } ) ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 06-abr, 1.9, Colorazione Red-Black -------------------*/
/*--------------------------------------------------------------------------*/

/// a shape of binary tree, its nodes named by letters
using Shape = BinaryTree< char >;

/// the red nodes of a coloring
using Reds = std::set< const Shape * >;

/// whether the reds give a Red-Black tree: the root black, no red child of
/// a red node, the same number of black nodes on each path to a leaf nil
bool isRedBlack( const Shape * t , const Reds & red )
{
 bool ok = ! red.count( t );
 std::function< int( const Shape * ) > rec = [ & ]( const Shape * u ) {
  if( ! u )
   return( 1 );                            // the black leaf nil
  const bool r = red.count( u );
  for( const Shape * c : { u->left() , u->right() } )
   if( r && c && red.count( c ) )
    ok = false;
  const int a = rec( u->left() ) , b = rec( u->right() );
  if( a != b )
   ok = false;
  return( a + ( r ? 0 : 1 ) );
  };
 rec( t );
 return( ok );
 }

/// whether the shape t admits a Red-Black coloring, and one in *red: for
/// each node and color the set of the black heights its subtree can have
/// (a red node over two black children of the same black height, a black
/// one over any two of the same), in postvisit, then the colors chosen
/// going down from a black root; O( n h^2 log h ) time
/// \pre t is a node
bool redBlackColoring( const Shape * t , Reds * red = nullptr )
{
 ADS_PRE( t != nullptr );
 using Sets = std::array< std::set< int > , 2 >;   // [ 0 ] black, [ 1 ] red
 std::map< const Shape * , Sets > B;
 auto of = [ & ]( const Shape * u ) -> Sets {
  if( ! u ) {
   Sets s;
   s[ 0 ].insert( 0 );                     // nil is black, not counted
   return( s );
   }
  return( B[ u ] );
  };
 std::function< void( const Shape * ) > up = [ & ]( const Shape * u ) {
  if( ! u )
   return;
  up( u->left() );
  up( u->right() );
  const Sets l = of( u->left() ) , r = of( u->right() );
  Sets s;
  for( int cl = 0 ; cl < 2 ; ++cl )
   for( int cr = 0 ; cr < 2 ; ++cr )
    for( int b : l[ cl ] )
     if( r[ cr ].count( b ) ) {
      s[ 0 ].insert( b + 1 );
      if( ( cl == 0 ) && ( cr == 0 ) )
       s[ 1 ].insert( b );
      }
  B[ u ] = s;
  };
 up( t );
 if( B[ t ][ 0 ].empty() )
  return( false );
 if( red ) {
  red->clear();
  std::function< void( const Shape * , int , int ) > down =
   [ & ]( const Shape * u , int c , int b ) {
   if( ! u )
    return;
   if( c == 1 ) red->insert( u );
   const int bc = b - ( c == 0 );          // the black height below
   const Sets l = of( u->left() ) , r = of( u->right() );
   for( int cl = 0 ; cl < 2 ; ++cl )
    for( int cr = 0 ; cr < 2 ; ++cr )
     if( ( ( c == 0 ) || ( ( cl == 0 ) && ( cr == 0 ) ) ) &&
         l[ cl ].count( bc ) && r[ cr ].count( bc ) ) {
      down( u->left() , cl , bc );
      down( u->right() , cr , bc );
      return;
      }
   };
  down( t , 0 , *B[ t ][ 0 ].begin() );
  ADS_POST( isRedBlack( t , *red ) );
  }
 return( true );
 }

/// the node of t of name c
const Shape * named( const Shape * t , char c )
{
 if( ! t || ( t->read() == c ) )
  return( t );
 const Shape * l = named( t->left() , c );
 return( l ? l : named( t->right() , c ) );
 }

// the four trees of the text: (a), (c) and (d) colorable, with the
// colorings of the solution valid, (b) not; random shapes against all
// their 2^n colorings; one node, chains of 2 and 3, a perfect tree; nil
// refused
ADS_TEST( "collezioni-06-abr-1-9" )
{
 std::unique_ptr< Shape >
  a( from_list< char >( "( A ( B () () ) ( C ( D () () ) ( E () () ) ) )" ) ) ,
  b( from_list< char >( "( A ( B ( D ( F () () ) ( G ( J () () ) () ) ) ( E ("
                        " H () () ) ( I () () ) ) ) ( C () () ) )" ) ) ,
  c( from_list< char >( "( A ( B ( D ( H () () ) ( I ( L () () ) () ) ) ( E ("
                        " J () () ) ( K () () ) ) ) ( C ( F () () ) ( G () ()"
                        " ) ) )" ) ) ,
  d( from_list< char >( "( A ( B ( D ( F () () ) ( G ( J () () ) ( K () () ) "
                        ") ) ( E ( L () () ) ( M () () ) ) ) ( C ( H () () ) "
                        "( I () () ) ) )" ) );
 Reds red;
 CHECK( redBlackColoring( a.get() , &red ) && isRedBlack( a.get() , red ) );
 CHECK( ! redBlackColoring( b.get() ) );
 CHECK( redBlackColoring( c.get() , &red ) && isRedBlack( c.get() , red ) );
 CHECK( redBlackColoring( d.get() , &red ) && isRedBlack( d.get() , red ) );
 CHECK( isRedBlack( a.get() , { named( a.get() , 'C' ) } ) );
 CHECK( isRedBlack( c.get() , { named( c.get() , 'B' ) ,
                                named( c.get() , 'L' ) } ) );
 CHECK( isRedBlack( d.get() , { named( d.get() , 'B' ) ,
                                named( d.get() , 'J' ) ,
                                named( d.get() , 'K' ) } ) );
 CHECK( ! isRedBlack( a.get() , {} ) );       // all black fails in (a)
 std::mt19937 g( 69 );
 int some = 0;
 for( int r = 0 ; r < 1500 ; ++r ) {
  const int n = randInt( g , 1 , 12 );
  char next = 'A';
  std::unique_ptr< Shape > t( randomTree( g , n , [ & ]() {
                                          return( next++ ); } ) );
  std::vector< const Shape * > N;
  std::function< void( const Shape * ) > rec = [ & ]( const Shape * u ) {
   if( ! u ) return;
   N.push_back( u );
   rec( u->left() );
   rec( u->right() );
   };
  rec( t.get() );
  bool e = false;
  for( int mask = 0 ; ( mask < ( 1 << n ) ) && ! e ; ++mask ) {
   Reds R;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) R.insert( N[ i ] );
   e = isRedBlack( t.get() , R );
   }
  CHECK( redBlackColoring( t.get() , &red ) == e );
  some += e;
  }
 CHECK( ( some > 100 ) && ( some < 1500 ) );
 // one node (black), a chain of 2 (a red child), a chain of 3 (no
 // coloring: the black heights of the two sides of the root differ), a
 // perfect tree (all black)
 std::unique_ptr< Shape > s1( from_list< char >( "( A () () )" ) ) ,
  s2( from_list< char >( "( A ( B () () ) () )" ) ) ,
  s3( from_list< char >( "( A ( B ( C () () ) () ) () )" ) ) ,
  s7( from_list< char >( "( A ( B ( D () () ) ( E () () ) ) ( C ( F () () )"
                         " ( G () () ) ) )" ) );
 CHECK( redBlackColoring( s1.get() , &red ) && red.empty() );
 CHECK( redBlackColoring( s2.get() , &red ) &&
        ( red == Reds( { named( s2.get() , 'B' ) } ) ) );
 CHECK( ! redBlackColoring( s3.get() ) );
 CHECK( redBlackColoring( s7.get() ) && isRedBlack( s7.get() , {} ) );
 CHECK_THROWS( redBlackColoring( nullptr ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------ 06-abr, 1.10, Realizzazione di insiemi con alberi Red-Black -------*/
/*--------------------------------------------------------------------------*/

/// the kinds of operation on two sets
enum class SetOp { UNION , INTERSECTION , DIFFERENCE };

/// union(), intersection() and difference() of two sets in Red-Black
/// trees, by a merge of the two invisits from min() along successorNode(),
/// the result inserted in a new Red-Black tree: O( n1 + n2 + nR log( nR +
/// 1 ) ) time
template< class K >
RedBlackTree< K , int > setOperation( const RedBlackTree< K , int > & A ,
                                     const RedBlackTree< K , int > & B ,
                                     SetOp op )
{
 using Tr = RedBlackTree< K , int >;
 Tr R;
 auto a = A.min();
 auto b = B.min();
 while( ! A.isNil( a ) || ! B.isNil( b ) ) {
  const bool takeA = ! A.isNil( a ) &&
                     ( B.isNil( b ) || ( Tr::key( a ) < Tr::key( b ) ) );
  const bool takeB = ! B.isNil( b ) &&
                     ( A.isNil( a ) || ( Tr::key( b ) < Tr::key( a ) ) );
  if( takeA ) {
   if( op != SetOp::INTERSECTION ) R.insert( Tr::key( a ) , 0 );
   a = A.successorNode( a );
   }
  else if( takeB ) {
   if( op == SetOp::UNION ) R.insert( Tr::key( b ) , 0 );
   b = B.successorNode( b );
   }
  else {                                   // the same key
   if( op != SetOp::DIFFERENCE ) R.insert( Tr::key( a ) , 0 );
   a = A.successorNode( a );
   b = B.successorNode( b );
   }
  }
 return( R );
 }

// the three operations against std::set_union, std::set_intersection and
// std::set_difference on random sets, the empty ones included; the results
// are Red-Black trees of the library
ADS_TEST( "collezioni-06-abr-1-10" )
{
 using RB = RedBlackTree< int , int >;
 auto keys = []( const RB & t ) {
  Ints out;
  for( auto u = t.min() ; ! t.isNil( u ) ; u = t.successorNode( u ) )
   out.push_back( RB::key( u ) );
  return( out );
  };
 std::mt19937 g( 610 );
 for( int r = 0 ; r < 500 ; ++r ) {
  RB A , B;
  std::set< int > SA , SB;
  for( int i = randInt( g , 0 , 40 ) ; i > 0 ; --i ) {
   const int k = randInt( g , 0 , 60 );
   A.insert( k , 0 );
   SA.insert( k );
   }
  for( int i = randInt( g , 0 , 40 ) ; i > 0 ; --i ) {
   const int k = randInt( g , 0 , 60 );
   B.insert( k , 0 );
   SB.insert( k );
   }
  Ints u , in , d;
  std::set_union( SA.begin() , SA.end() , SB.begin() , SB.end() ,
                  std::back_inserter( u ) );
  std::set_intersection( SA.begin() , SA.end() , SB.begin() , SB.end() ,
                         std::back_inserter( in ) );
  std::set_difference( SA.begin() , SA.end() , SB.begin() , SB.end() ,
                       std::back_inserter( d ) );
  const RB U = setOperation( A , B , SetOp::UNION ) ,
           I = setOperation( A , B , SetOp::INTERSECTION ) ,
           D = setOperation( A , B , SetOp::DIFFERENCE );
  CHECK( keys( U ) == u && keys( I ) == in && keys( D ) == d );
  CHECK( U.size() == u.size() && I.size() == in.size() &&
         D.size() == d.size() );
  }
 const RB E;
 CHECK( setOperation( E , E , SetOp::UNION ).empty() );
 }

/*--------------------------------------------------------------------------*/
/*-------------- 07-hashing, 1.1, Inserimento in tabella hash --------------*/
/*--------------------------------------------------------------------------*/

/// the hash h( k ) = k of the text
struct Identity {
 std::size_t operator()( int k ) const { return( std::size_t( k ) ); }
 };

/// the insertions of K, in order, in a table of size m with H( k , i ) = (
/// k mod m + c1 i + c2 i^2 ) mod m, simulated by hand: the table, -1 for
/// nil, the inspections of each key inserted, 0 for a key that finds no
/// place in m inspections
/// \pre m > 0, the keys not negative
std::pair< std::vector< int > , std::vector< int > >
simulateQuadratic( const std::vector< int > & K , int m , int c1 , int c2 )
{
 ADS_PRE( m > 0 );
 std::vector< int > T( m , -1 ) , probes;
 for( int k : K ) {
  ADS_PRE( k >= 0 );
  int used = 0;
  for( int i = 0 ; i < m ; ++i ) {
   const int j = ( k % m + c1 * i + c2 * i * i ) % m;
   if( T[ j ] == -1 ) {
    T[ j ] = k;
    used = i + 1;
    break;
    }
   }
  probes.push_back( used );
  }
 return( std::pair( T , probes ) );
 }

// the OpenHashTable of the library with H( k , i ) = ( k mod 11 + 3 i + i^2
// ) mod 11 gives the table of the solution, refuses 7, whose inspection
// 7, 0, 6, 3, 2, 3, 6, 0, 7, 5, 5 misses the place 8, and finds the keys in
// 12 inspections, 1.2 on average; random insertions against a simulation;
// a table of one place, no key; m = 0 and a negative key refused
ADS_TEST( "collezioni-07-hashing-1-1" )
{
 const Ints K = { 33 , 10 , 24 , 14 , 16 , 13 , 23 , 31 , 18 , 11 };
 OpenHashTable< int , int , Identity > T( 11 , Probing::quadratic( 3 , 1 ) );
 for( int k : K )
  T.insert( k , k );
 CHECK_THROWS( T.insert( 7 , 7 ) , std::overflow_error );
 const Ints e = { 33 , 23 , 24 , 14 , 11 , 16 , 13 , 18 , -1 , 31 , 10 };
 for( std::size_t i = 0 ; i < 11 ; ++i )
  CHECK( ( e[ i ] == -1 ) ? ( T.slot( i ) == decltype( T )::Slot::NIL )
                          : ( T.keyAt( i ) == e[ i ] ) );
 const Ints seq = { 7 , 0 , 6 , 3 , 2 , 3 , 6 , 0 , 7 , 5 , 5 };
 for( int i = 0 ; i < 11 ; ++i )
  CHECK( int( Probing::quadratic( 3 , 1 ).at( 7 , i , 11 ) ) == seq[ i ] );
 std::size_t probes = 0;
 for( int k : K ) {
  CHECK( T.lookup( k ) == k );
  probes += T.probes();
  }
 CHECK( probes == 12 && near( probes / 10.0 , 1.2 ) );
 Ints withSeven = K;
 withSeven.push_back( 7 );
 const auto [ S , P ] = simulateQuadratic( withSeven , 11 , 3 , 1 );
 CHECK( S == e && P.back() == 0 );
 std::mt19937 g( 71 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int m = std::vector< int >( { 5 , 7 , 11 , 13 } )[ randInt( g , 0 ,
                                                                    3 ) ];
  std::set< int > distinct;
  for( int i = randInt( g , 0 , m ) ; i > 0 ; --i )
   distinct.insert( randInt( g , 0 , 100 ) );
  Ints L( distinct.begin() , distinct.end() );
  std::shuffle( L.begin() , L.end() , g );
  const auto [ Ts , Ps ] = simulateQuadratic( L , m , 3 , 1 );
  OpenHashTable< int , int , Identity > U( m , Probing::quadratic( 3 , 1 ) );
  for( std::size_t i = 0 ; i < L.size() ; ++i ) {
   bool thrown = false;
   try { U.insert( L[ i ] , 0 ); }
   catch( const std::overflow_error & ) { thrown = true; }
   CHECK( thrown == ( Ps[ i ] == 0 ) );
   }
  for( int i = 0 ; i < m ; ++i )
   CHECK( ( Ts[ i ] == -1 ) ? ( U.slot( i ) == decltype( U )::Slot::NIL )
                            : ( U.keyAt( i ) == Ts[ i ] ) );
  }
 // a table of one place (the second key finds none), no key, and the
 // contracts of the simulation: m = 0, a negative key
 const auto [ S1 , P1 ] = simulateQuadratic( { 4 , 9 } , 1 , 3 , 1 );
 CHECK( S1 == Ints( 1 , 4 ) && P1 == Ints( { 1 , 0 } ) );
 const auto [ S0 , P0 ] = simulateQuadratic( {} , 3 , 3 , 1 );
 CHECK( S0 == Ints( 3 , -1 ) && P0.empty() );
 CHECK_THROWS( simulateQuadratic( { 1 } , 0 , 3 , 1 ) , contract_violation );
 CHECK_THROWS( simulateQuadratic( { -1 } , 5 , 3 , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------ 07-hashing, 1.2, Ispezione lineare e quadratica -------------*/
/*--------------------------------------------------------------------------*/

/// a table with open addressing of the text: the keys, nil and deleted
/// places, and the inspection linear H( k , i ) = ( H0( k ) + delta i ) mod
/// m or quadratic H( k , i ) = ( H0( k ) + delta i^2 ) mod m, H0( k ) = k
/// mod m
struct ScanTable {
 enum State { NIL , FULL , DELETED };
 std::vector< int > key;
 std::vector< State > state;
 int delta;
 bool quadratic;

 /// \pre m > 0, delta coprime with m if linear, m prime if quadratic
 ScanTable( int m , int d , bool q )
  : key( m , 0 ) , state( m , NIL ) , delta( d ) , quadratic( q ) {
  ADS_PRE( ( m > 0 ) && ( d > 0 ) );
  ADS_PRE( q || ( std::gcd( d , m ) == 1 ) );
  }

 int m( void ) const { return( int( key.size() ) ); }

 int H( int k , long i ) const {
  const long s = quadratic ? i * i : i;
  return( int( ( k % m() + delta * ( s % m() ) ) % m() ) );
  }

 /// scanLinear() or scanQuadratic(): the place where the search of k
 /// ends, at k, at a nil place or after m inspections; with insert, the
 /// first deleted place met if k is not there; O( m ) time, O( 1 ) space
 int scan( int k , bool insert ) const {
  long i = 0;
  int j = H( k , 0 ) , firstDeleted = -1;
  while( ! ( state[ j ] == FULL && key[ j ] == k ) && ( state[ j ] != NIL ) &&
         ( i < m() ) ) {
   if( ( state[ j ] == DELETED ) && ( firstDeleted == -1 ) )
    firstDeleted = j;
   j = H( k , ++i );
   }
  if( insert && ! ( state[ j ] == FULL && key[ j ] == k ) &&
      ( firstDeleted != -1 ) )
   return( firstDeleted );
  return( j );
  }

 bool lookup( int k ) const {
  const int j = scan( k , false );
  return( state[ j ] == FULL && key[ j ] == k );
  }

 /// false if no place is found
 bool insert( int k ) {
  const int j = scan( k , true );
  if( state[ j ] == FULL && key[ j ] != k )
   return( false );
  state[ j ] = FULL;
  key[ j ] = k;
  return( true );
  }

 void remove( int k ) {
  const int j = scan( k , false );
  if( state[ j ] == FULL && key[ j ] == k )
   state[ j ] = DELETED;
  }
 };

// random insertions, removals and lookups against std::set, and the places
// against the OpenHashTable of the library with the same inspection, the
// linear one with delta 1 and 3, the quadratic one with delta 1 and 2; a
// full table, a key inserted twice, a removal of an absent key, a search
// past a deleted place and an insertion into it, one place; m = 0, delta
// = 0 and delta not coprime with m refused
ADS_TEST( "collezioni-07-hashing-1-2" )
{
 std::mt19937 g( 72 );
 for( int r = 0 ; r < 200 ; ++r ) {
  const bool q = r % 2;
  const int m = q ? 13 : 16 , d = q ? 1 + r % 4 / 2 : ( r % 4 < 2 ? 1 : 3 );
  ScanTable T( m , d , q );
  OpenHashTable< int , int , Identity > U( m , q ? Probing::quadratic( 0 , d )
                                                 : Probing::linear( d ) );
  std::set< int > S;
  for( int i = 0 ; i < 300 ; ++i ) {
   const int k = randInt( g , 0 , 3 * m ) , op = randInt( g , 0 , 9 );
   if( op < 5 ) {
    const bool ok = T.insert( k );
    bool thrown = false;
    try { U.insert( k , 0 ); }
    catch( const std::overflow_error & ) { thrown = true; }
    CHECK( ok != thrown );
    if( ok ) S.insert( k );
    }
   else if( op < 8 ) {
    T.remove( k );
    U.remove( k );
    S.erase( k );
    }
   else
    CHECK( T.lookup( k ) == bool( S.count( k ) ) );
   for( int j = 0 ; j < m ; ++j ) {
    const auto s = U.slot( j );
    using Slot = decltype( U )::Slot;
    CHECK( ( T.state[ j ] == ScanTable::NIL ) == ( s == Slot::NIL ) );
    CHECK( ( T.state[ j ] == ScanTable::DELETED ) == ( s == Slot::DELETED ) );
    if( s == Slot::FULL ) CHECK( U.keyAt( j ) == T.key[ j ] );
    }
   }
  for( int k = 0 ; k <= 3 * m ; ++k )
   CHECK( T.lookup( k ) == bool( S.count( k ) ) );
  }
 ScanTable F( 5 , 1 , false );
 for( int k = 0 ; k < 5 ; ++k ) CHECK( F.insert( 5 * k ) );
 CHECK( ! F.insert( 7 ) && F.insert( 10 ) );   // full; 10 is there
 F.remove( 10 );
 CHECK( F.scan( 7 , true ) == 2 && F.insert( 7 ) );   // the deleted place
 CHECK_THROWS( ScanTable( 16 , 2 , false ) , contract_violation );
 // a key inserted twice stays once, a removal of an absent key changes
 // nothing, a key found after a deleted place, one place only; m = 0 and
 // delta = 0 refused
 ScanTable D( 7 , 1 , false );
 CHECK( D.insert( 3 ) && D.insert( 10 ) && D.insert( 3 ) );
 D.remove( 4 );
 D.remove( 3 );
 CHECK( ! D.lookup( 3 ) && D.lookup( 10 ) );   // past the deleted 3
 CHECK( D.insert( 17 ) && D.key[ 3 ] == 17 );  // into the deleted place
 ScanTable One( 1 , 1 , false );
 CHECK( One.insert( 8 ) && ! One.insert( 9 ) && One.lookup( 8 ) );
 CHECK_THROWS( ScanTable( 0 , 1 , false ) , contract_violation );
 CHECK_THROWS( ScanTable( 7 , 0 , true ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 07-hashing, 1.3, Confronto file ---------------------*/
/*--------------------------------------------------------------------------*/

/// the groups, of two files or more, of equal content: the files grouped
/// by length and digest in a hash table, then each group split by a
/// comparison byte by byte, which a collision of the digests needs; O( L )
/// time to read the files plus O( n ) expected operations on the table,
/// and the comparisons of the files of the same digest. The groups, each
/// sorted, in increasing order
template< class D >
std::vector< std::vector< int > > equalFiles(
                                   const std::vector< std::string > & F ,
                                   D && digest )
{
 std::unordered_map< std::string , std::vector< int > > bucket;
 for( int i = 0 ; i < int( F.size() ) ; ++i )
  bucket[ std::to_string( F[ i ].size() ) + ":" +
          std::to_string( digest( F[ i ] ) ) ].push_back( i );
 std::vector< std::vector< int > > out;
 for( auto & [ k , L ] : bucket ) {
  std::vector< std::vector< int > > split;  // by content, byte by byte
  for( int i : L ) {
   auto it = std::find_if( split.begin() , split.end() ,
                           [ & ]( const std::vector< int > & s ) {
                            return( F[ s[ 0 ] ] == F[ i ] ); } );
   if( it == split.end() ) split.push_back( { i } );
   else it->push_back( i );
   }
  for( auto & s : split )
   if( s.size() >= 2 )
    out.push_back( s );
  }
 std::sort( out.begin() , out.end() );
 return( out );
 }

// random short files against the classes of a comparison of all the
// pairs, with a good digest, one by the length only and one constant, so
// that the comparisons byte by byte must split the collisions; no file, one
// file, all equal, empty files, two groups in one bucket
ADS_TEST( "collezioni-07-hashing-1-3" )
{
 std::mt19937 g( 73 );
 auto good = []( const std::string & s ) {
  return( std::hash< std::string >()( s ) ); };
 auto bad = []( const std::string & ) { return( std::size_t( 42 ) ); };
 for( int r = 0 ; r < 500 ; ++r ) {
  std::vector< std::string > F( randInt( g , 0 , 25 ) );
  for( auto & f : F )
   for( int i = randInt( g , 0 , 4 ) ; i > 0 ; --i )
    f += char( 'a' + randInt( g , 0 , 1 ) );
  std::vector< std::vector< int > > e;
  std::vector< bool > used( F.size() , false );
  for( std::size_t i = 0 ; i < F.size() ; ++i ) {
   if( used[ i ] ) continue;
   std::vector< int > c = { int( i ) };
   for( std::size_t j = i + 1 ; j < F.size() ; ++j )
    if( F[ j ] == F[ i ] ) { c.push_back( int( j ) ); used[ j ] = true; }
   if( c.size() >= 2 ) e.push_back( c );
   }
  std::sort( e.begin() , e.end() );
  CHECK( equalFiles( F , good ) == e && equalFiles( F , bad ) == e );
  }
 CHECK( equalFiles( {} , good ).empty() );
 const std::vector< std::string > same( 10 , "abc" );
 CHECK( equalFiles( same , bad ) ==
        std::vector< std::vector< int > >( { { 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 ,
                                               8 , 9 } } ) );
 CHECK( equalFiles( { "ab" , "ba" , "a" , "" } , bad ).empty() );
 // one file, empty files (equal), two groups of the same length and digest
 CHECK( equalFiles( { "x" } , good ).empty() );
 CHECK( equalFiles( { "" , "a" , "" } , good ) ==
        std::vector< std::vector< int > >( 1 , { 0 , 2 } ) );
 CHECK( equalFiles( { "ab" , "ba" , "ab" , "ba" } , bad ) ==
        std::vector< std::vector< int > >( { { 0 , 2 } , { 1 , 3 } } ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 09-grafi, 1.1, DFS iterativa ----------------------*/
/*--------------------------------------------------------------------------*/

/// an event of a visit: a node ( u , -1 ) or an edge ( u , v )
using Event = std::pair< int , int >;

/// dfs(), iterative: next[ u ] is the place in adj( u ) of the next edge
/// of u to examine, so that the nodes and the edges come in the order of
/// the recursive visit; Theta( n + m ) time, Theta( n ) space
/// \pre r is a node
template< Graph G >
std::vector< Event > dfsIterative( const G & g , int r ,
                                   std::vector< bool > & visited )
{
 ADS_PRE( g.hasNode( r ) && ( int( visited.size() ) == g.bound() ) );
 std::vector< Event > out;
 std::vector< std::vector< int > > adj( g.bound() );
 std::vector< std::size_t > next( g.bound() , 0 );
 SequenceStack< List< int > > S;
 visited[ r ] = true;
 out.push_back( Event( r , -1 ) );
 adj[ r ] = g.adj( r );
 S.push( r );
 while( ! S.empty() ) {
  const int u = S.top();
  if( next[ u ] < adj[ u ].size() ) {
   const int v = adj[ u ][ next[ u ]++ ];
   out.push_back( Event( u , v ) );
   if( ! visited[ v ] ) {
    visited[ v ] = true;
    out.push_back( Event( v , -1 ) );
    adj[ v ] = g.adj( v );
    S.push( v );
    }
   }
  else
   S.pop();
  }
 return( out );
 }

/// a graph of n nodes with the edges E, inserted in this order
template< class W = std::monostate >
ListGraph< W > makeGraph( int n , const std::vector< std::pair< int , int > >
                          & E , bool directed , W w = W() )
{
 ListGraph< W > g( n , directed );
 for( auto [ u , v ] : E )
  g.insertEdge( u , v , w );
 return( g );
 }

/// a random graph of n nodes, each edge with probability p; the edges are
/// inserted by increasing ends, so that the lists are sorted
template< class W = std::monostate >
ListGraph< W > randomGraph( std::mt19937 & g , int n , double p ,
                            bool directed , W w = W() )
{
 // p as 2.0 / n is more than 1 for n = 1, which bernoulli_distribution
 // does not take (an assertion of libstdc++ with _GLIBCXX_ASSERTIONS)
 std::bernoulli_distribution coin( std::min( p , 1.0 ) );
 ListGraph< W > G( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && coin( g ) )
    G.insertEdge( u , v , w );
 return( G );
 }

/// a random tree of n nodes as an unoriented graph, each node i > 0 hung
/// to a random node before it
ListGraph<> randomFreeTree( std::mt19937 & g , int n )
{
 ListGraph<> G( n , false );
 for( int i = 1 ; i < n ; ++i )
  G.insertEdge( randInt( g , 0 , i - 1 ) , i );
 return( G );
 }

// the graph of the figure from A, with sorted lists, gives the sequence of
// the solution; on random graphs, oriented or not, the iterative visit
// gives the nodes and the edges of the dfs() of the library in its order;
// one node; a root out of the graph, marks of the wrong size refused
ADS_TEST( "collezioni-09-grafi-1-1" )
{
 enum { A , B , C , D , E , F , G , H , I };
 const ListGraph<> fig = makeGraph( 9 , { { A , B } , { A , C } , { A , D } ,
                                          { A , F } , { A , H } , { B , D } ,
                                          { B , E } , { B , G } , { B , I } ,
                                          { C , F } , { D , E } , { G , H } ,
                                          { G , I } } , false );
 std::vector< bool > visited( 9 , false );
 std::string s;
 for( auto [ u , v ] : dfsIterative( fig , A , visited ) ) {
  if( ! s.empty() ) s += ", ";
  if( v < 0 ) s += char( 'A' + u );
  else s += std::string( "(" ) + char( 'A' + u ) + "," + char( 'A' + v ) + ")";
  }
 CHECK( s == "A, (A,B), B, (B,A), (B,D), D, (D,A), (D,B), (D,E), E, (E,B), "
             "(E,D), (B,E), (B,G), G, (G,B), (G,H), H, (H,A), (H,G), (G,I), "
             "I, (I,B), (I,G), (B,I), (A,C), C, (C,A), (C,F), F, (F,A), (F,"
             "C), (A,D), (A,F), (A,H)" );
 std::mt19937 g( 91 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 20 );
  const ListGraph<> G = randomGraph( g , n , 0.2 , r % 2 );
  const int root = randInt( g , 0 , n - 1 );
  std::vector< Event > e;
  std::vector< bool > v1( n , false ) , v2( n , false );
  dfs( G , root , v1 , [ & ]( int u ) { e.push_back( Event( u , -1 ) ); } ,
       [ & ]( int u , int v ) { e.push_back( Event( u , v ) ); } ,
       []( int ) {} );
  CHECK( dfsIterative( G , root , v2 ) == e && v1 == v2 );
  }
 // one node, a root already visited marks nothing new, and the contracts:
 // a root out of the graph, a vector of marks of the wrong size
 std::vector< bool > one( 1 , false );
 CHECK( dfsIterative( ListGraph<>( 1 , false ) , 0 , one ) ==
        std::vector< Event >( 1 , Event( 0 , -1 ) ) );
 std::vector< bool > v9( 9 , false );
 CHECK_THROWS( dfsIterative( fig , 9 , v9 ) , contract_violation );
 std::vector< bool > v3( 3 , false );
 CHECK_THROWS( dfsIterative( fig , 0 , v3 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------- 09-grafi, 1.3, Larghezza albero radicato ----------------*/
/*--------------------------------------------------------------------------*/

/// graphWidth(): the largest number of nodes at the same distance from r
/// in the tree G rooted in r, by a visit by levels counting the nodes of
/// each distance; Theta( n ) time and space
/// \pre r is a node
template< Graph G >
int graphWidth( const G & g , int r )
{
 ADS_PRE( g.hasNode( r ) );
 std::vector< int > dist( g.bound() , -1 ) , count( 1 , 1 ) , Q = { r };
 dist[ r ] = 0;
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  g.forEachAdj( u , [ & ]( int v ) {
   if( dist[ v ] < 0 ) {
    dist[ v ] = dist[ u ] + 1;
    if( int( count.size() ) == dist[ v ] ) count.push_back( 0 );
    ++count[ dist[ v ] ];
    Q.push_back( v );
    }
   } );
  }
 return( *std::max_element( count.begin() , count.end() ) );
 }

// random trees from a random root against the classes of the distances of
// distance() of the library; a single node, a star, a path
ADS_TEST( "collezioni-09-grafi-1-3" )
{
 std::mt19937 g( 93 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 1 , 60 );
  const ListGraph<> T = randomFreeTree( g , n );
  const int root = randInt( g , 0 , n - 1 );
  std::map< int , int > level;
  for( int d : distance( T , root ).first ) ++level[ d ];
  int e = 0;
  for( auto [ d , c ] : level ) e = std::max( e , c );
  CHECK( graphWidth( T , root ) == e );
  }
 CHECK( graphWidth( ListGraph<>( 1 , false ) , 0 ) == 1 );
 ListGraph<> star( 50 , false ) , path( 50 , false );
 for( int i = 1 ; i < 50 ; ++i ) {
  star.insertEdge( 0 , i );
  path.insertEdge( i - 1 , i );
  }
 CHECK( graphWidth( star , 0 ) == 49 && graphWidth( star , 7 ) == 48 );
 CHECK( graphWidth( path , 0 ) == 1 && graphWidth( path , 25 ) == 2 );
 }

/*--------------------------------------------------------------------------*/
/*-------------- 09-grafi, 1.4, Albero libero, non orientato ---------------*/
/*--------------------------------------------------------------------------*/

// isTree(), whether an unoriented graph is a free tree, is isTree() of the
// library: n - 1 edges and a visit from a node reaching all the nodes;
// Theta( n + m ) time

// random sparse graphs and trees with an edge more or less against one
// component of cc() of the library and m = n - 1
ADS_TEST( "collezioni-09-grafi-1-4" )
{
 std::mt19937 g( 94 );
 int trees = 0;
 for( int r = 0 ; r < 1500 ; ++r ) {
  const int n = randInt( g , 1 , 12 );
  ListGraph<> G = r % 3 ? randomFreeTree( g , n )
                        : randomGraph( g , n , 2.0 / n , false );
  if( ( r % 3 == 1 ) && ( n >= 3 ) ) {
   const int u = randInt( g , 0 , n - 1 ) , v = randInt( g , 0 , n - 1 );
   if( u != v ) G.insertEdge( u , v );
   }
  if( ( r % 3 == 2 ) && ( n >= 2 ) ) {
   const auto E = edges( G );
   const auto e = E[ randInt( g , 0 , int( E.size() ) - 1 ) ];
   G.deleteEdge( e.u , e.v );
   }
  const auto c = cc( G );
  const bool e = ( *std::max_element( c.begin() , c.end() ) == 1 ) &&
                 ( G.m() == n - 1 );
  CHECK( isTree( G ) == e );
  trees += e;
  }
 CHECK( trees > 300 && trees < 1300 );
 CHECK( isTree( ListGraph<>( 1 , false ) ) );
 CHECK( ! isTree( ListGraph<>( 2 , false ) ) );
 CHECK_THROWS( isTree( ListGraph<>( 0 , false ) ) , contract_violation );
 CHECK_THROWS( isTree( ListGraph<>( 3 , true ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------- 09-grafi, 1.5, Quadrato di grafo orientato ---------------*/
/*--------------------------------------------------------------------------*/

/// an adjacency matrix of 0 and 1
using Adj = std::vector< std::vector< int > >;

/// square(): ( x , y ) is an edge of G^2, x != y, if a node u has ( x , u
/// ) and ( u , y ) in G, the search for u stopping at the first one;
/// Theta( n^3 ) time, Theta( n^2 ) space
Adj square( const Adj & A )
{
 const std::size_t n = A.size();
 Adj B( n , std::vector< int >( n , 0 ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j )
   for( std::size_t k = 0 ; ( k < n ) && ( i != j ) && ! B[ i ][ j ] ; ++k )
    B[ i ][ j ] = A[ i ][ k ] && A[ k ][ j ];
 return( B );
 }

/// the adjacency matrix of a graph
template< Graph G >
Adj matrixOf( const G & g )
{
 Adj A( g.bound() , std::vector< int >( g.bound() , 0 ) );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { A[ u ][ v ] = 1; } );
 return( A );
 }

// random oriented graphs against the paths of two edges walked on the
// lists; the empty graph and one node
ADS_TEST( "collezioni-09-grafi-1-5" )
{
 std::mt19937 g( 95 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 25 );
  const ListGraph<> G = randomGraph( g , n , 0.15 , true );
  Adj e( n , std::vector< int >( n , 0 ) );
  for( int x : G.V() )
   for( int u : G.adj( x ) )
    for( int y : G.adj( u ) )
     if( y != x ) e[ x ][ y ] = 1;
  CHECK( square( matrixOf( G ) ) == e );
  }
 CHECK( square( Adj() ).empty() );
 CHECK( square( Adj( 1 , std::vector< int >( 1 , 0 ) ) ) ==
        Adj( 1 , std::vector< int >( 1 , 0 ) ) );
 const Adj cyc = { { 0 , 1 } , { 1 , 0 } };    // 0 -> 1 -> 0: no loop
 CHECK( square( cyc ) == Adj( { { 0 , 0 } , { 0 , 0 } } ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 09-grafi, 1.6, Diametro -------------------------*/
/*--------------------------------------------------------------------------*/

// diameter(), the largest distance between two nodes of a connected
// unoriented graph, is diameter() of the library: a visit by levels from
// each node; O( n ( n + m ) ) time, O( n ) space

// random connected graphs against floydWarshall() of the library with unit
// weights; a path, a complete graph, one node
ADS_TEST( "collezioni-09-grafi-1-6" )
{
 std::mt19937 g( 96 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 30 );
  ListGraph< int > G( n , false );
  for( int i = 1 ; i < n ; ++i ) G.insertEdge( randInt( g , 0 , i - 1 ) , i ,
                                               1 );
  for( int k = randInt( g , 0 , n ) ; k > 0 ; --k ) {
   const int u = randInt( g , 0 , n - 1 ) , v = randInt( g , 0 , n - 1 );
   if( u != v ) G.insertEdge( u , v , 1 );
   }
  const auto R = floydWarshall( G );
  int e = 0;
  for( const auto & row : R.d )
   e = std::max( e , *std::max_element( row.begin() , row.end() ) );
  CHECK( diameter( G ) == e );
  }
 ListGraph<> path( 40 , false ) , K( 10 , false );
 for( int i = 1 ; i < 40 ; ++i ) path.insertEdge( i - 1 , i );
 for( int i = 0 ; i < 10 ; ++i )
  for( int j = i + 1 ; j < 10 ; ++j ) K.insertEdge( i , j );
 CHECK( diameter( path ) == 39 && diameter( K ) == 1 );
 CHECK( diameter( ListGraph<>( 1 , false ) ) == 0 );
 CHECK_THROWS( diameter( ListGraph<>( 2 , false ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------- 09-grafi, 1.7, Grado entrante e grado uscente --------------*/
/*--------------------------------------------------------------------------*/

/// the out-degrees, the lengths of the lists, and the in-degrees, a
/// counter per node raised by each edge; Theta( n + m ) time, Theta( n )
/// extra space for the in-degrees
template< Graph G >
std::pair< std::vector< int > , std::vector< int > > degrees( const G & g )
{
 std::vector< int > out( g.bound() , 0 ) , in( g.bound() , 0 );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ++out[ u ]; ++in[ v ]; } );
 return( std::pair( out , in ) );
 }

// random oriented graphs against the list of the edges they were built of
ADS_TEST( "collezioni-09-grafi-1-7" )
{
 std::mt19937 g( 97 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 0 , 30 );
  std::set< std::pair< int , int > > E;
  for( int k = n > 1 ? randInt( g , 0 , 3 * n ) : 0 ; k > 0 ; --k ) {
   const int u = randInt( g , 0 , n - 1 ) , v = randInt( g , 0 , n - 1 );
   if( u != v ) E.insert( std::pair( u , v ) );
   }
  const ListGraph<> G = makeGraph( n , std::vector< std::pair< int , int > >(
                                   E.begin() , E.end() ) , true );
  std::vector< int > out( n , 0 ) , in( n , 0 );
  for( auto [ u , v ] : E ) { ++out[ u ]; ++in[ v ]; }
  CHECK( degrees( G ) == std::pair( out , in ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 09-grafi, 1.8, Grafo trasposto ---------------------*/
/*--------------------------------------------------------------------------*/

/// the transposed adjacency matrix, Theta( n^2 ) time; with lists,
/// transpose() of the library takes Theta( n + m )
Adj transposeMatrix( const Adj & A )
{
 Adj T( A.size() , std::vector< int >( A.size() , 0 ) );
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = 0 ; j < A.size() ; ++j )
   T[ j ][ i ] = A[ i ][ j ];
 return( T );
 }

// transpose() of the library on random oriented graphs: ( v , u ) for each
// ( u , v ), as many edges, twice the graph itself; the transposed matrix
// is the matrix of the transposed graph
ADS_TEST( "collezioni-09-grafi-1-8" )
{
 std::mt19937 g( 98 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 0 , 25 );
  const ListGraph<> G = randomGraph( g , n , 0.2 , true );
  const ListGraph<> T = transpose( G ) , TT = transpose( T );
  CHECK( T.m() == G.m() && TT.m() == G.m() );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( u != v )
     CHECK( ( G.hasEdge( u , v ) == T.hasEdge( v , u ) ) &&
            ( G.hasEdge( u , v ) == TT.hasEdge( u , v ) ) );
  CHECK( transposeMatrix( matrixOf( G ) ) == matrixOf( T ) );
  CHECK( transposeMatrix( transposeMatrix( matrixOf( G ) ) ) ==
         matrixOf( G ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 09-grafi, 1.9, Pozzo universale ---------------------*/
/*--------------------------------------------------------------------------*/

// universalSink() of the library: the only candidate is left by
// eliminating a node per step (an edge c -> j excludes c, its absence
// excludes j), then its row must be all 0 and its column all 1 out of the
// diagonal; Theta( n ) time, NIL if there is no sink

// random matrices, some with a sink planted, against the degrees of all
// the nodes; one node is a sink
ADS_TEST( "collezioni-09-grafi-1-9" )
{
 std::mt19937 g( 99 );
 int sinks = 0;
 for( int r = 0 ; r < 3000 ; ++r ) {
  const int n = randInt( g , 1 , 8 );
  const ListGraph<> G = randomGraph( g , n , r % 2 ? 0.5 : 0.9 , true );
  Adj M = matrixOf( G );
  if( r % 3 == 0 ) {
   const int s = randInt( g , 0 , n - 1 );
   for( int j = 0 ; j < n ; ++j ) {
    M[ s ][ j ] = 0;
    if( j != s ) M[ j ][ s ] = 1;
    }
   }
  bool e = false;
  for( int c = 0 ; c < n ; ++c ) {
   int out = 0 , in = 0;
   for( int j = 0 ; j < n ; ++j ) { out += M[ c ][ j ]; in += M[ j ][ c ]; }
   e = e || ( ( out == 0 ) && ( in == n - 1 ) );
   }
  CHECK( ( universalSink( M ) != NIL ) == e );
  sinks += e;
  }
 CHECK( sinks > 1000 && sinks < 2900 );
 CHECK( universalSink( Adj( 1 , std::vector< int >( 1 , 0 ) ) ) == 0 );
 CHECK( universalSink( Adj() ) == NIL );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 09-grafi, 1.10, Massima distanza --------------------*/
/*--------------------------------------------------------------------------*/

/// maxdist(): the nodes reached from s at the largest distance, by a
/// visit by levels that keeps the largest distance met and how many nodes
/// have it; Theta( n + m ) time
/// \pre s is a node
template< Graph G >
int maxdist( const G & g , int s )
{
 ADS_PRE( g.hasNode( s ) );
 std::vector< int > dist( g.bound() , -1 ) , Q = { s };
 dist[ s ] = 0;
 int best = 0 , count = 1;
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  g.forEachAdj( u , [ & ]( int v ) {
   if( dist[ v ] < 0 ) {
    dist[ v ] = dist[ u ] + 1;
    Q.push_back( v );
    if( dist[ v ] > best ) { best = dist[ v ]; count = 1; }
    else if( dist[ v ] == best ) ++count;
    }
   } );
  }
 return( count );
 }

// random oriented graphs against the largest finite distance of distance()
// of the library and the nodes at it; s alone counts itself, an oriented
// cycle, a star from its center and from a leaf; s out of the graph
// refused
ADS_TEST( "collezioni-09-grafi-1-10" )
{
 std::mt19937 g( 910 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 1 , 30 );
  const ListGraph<> G = randomGraph( g , n , 2.0 / n , true );
  const int s = randInt( g , 0 , n - 1 );
  const auto d = distance( G , s ).first;
  int best = 0 , count = 0;
  for( int x : d )
   if( x != INF ) best = std::max( best , x );
  for( int x : d ) count += ( x == best );
  CHECK( maxdist( G , s ) == count );
  }
 CHECK( maxdist( ListGraph<>( 3 , true ) , 1 ) == 1 );
 // an oriented cycle (one node at n - 1), a star out of s (all leaves),
 // a node out of the graph refused
 const ListGraph<> cyc = makeGraph( 5 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                                          { 3 , 4 } , { 4 , 0 } } , true );
 const ListGraph<> star = makeGraph( 5 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } ,
                                           { 0 , 4 } } , true );
 CHECK( maxdist( cyc , 2 ) == 1 && maxdist( star , 0 ) == 4 );
 CHECK( maxdist( star , 3 ) == 1 );
 CHECK_THROWS( maxdist( star , 5 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 09-grafi, 1.11, Spazio di memoria --------------------*/
/*--------------------------------------------------------------------------*/

/// the most edges for which the lists, 32 bits per head and 64 per cell
/// (the node and the pointer), take less than the matrix of n^2 bits:
/// the largest m with 64 m + 32 n < n^2, -1 if there is none
/// \pre n >= 0
I64 listsBeatMatrix( I64 n )
{
 ADS_PRE( n >= 0 );
 const I64 room = n * n - 32 * n - 1;            // 64 m <= room
 return( room < 0 ? -1 : room / 64 );
 }

// m <= 15124 for n = 1000, and the largest m against the comparison of the
// two sizes made edge count by edge count; n = 0, 32 and 33 (the first n
// with room); n < 0 refused
ADS_TEST( "collezioni-09-grafi-1-11" )
{
 CHECK( listsBeatMatrix( 1000 ) == 15124 );
 CHECK( 64 * 15124 + 32 * 1000 < 1000 * 1000 &&
        64 * 15125 + 32 * 1000 >= 1000 * 1000 );
 for( I64 n = 0 ; n <= 400 ; ++n ) {
  I64 e = -1;
  for( I64 m = 0 ; m <= n * ( n - 1 ) ; ++m )
   if( 32 * n + 64 * m < n * n ) e = m;
  CHECK( listsBeatMatrix( n ) == e );
  }
 CHECK( listsBeatMatrix( 32 ) == -1 && listsBeatMatrix( 33 ) == 0 );
 CHECK( listsBeatMatrix( 0 ) == -1 );            // nothing beats 0 bits
 CHECK_THROWS( listsBeatMatrix( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 09-grafi, 1.12, Triangoli ------------------------*/
/*--------------------------------------------------------------------------*/

/// the triangles of the complete graph K_n, binom( n , 3 ) by binomial()
/// of the library, 0 for n < 3
/// \pre 0 <= n <= 67
I64 trianglesComplete( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 67 ) );
 return( n < 3 ? 0 : I64( binomial( n , 3 ) ) );
 }

/// the triangles of an unoriented graph by its matrix, the triples a < b
/// < c with the three edges; Theta( n^3 ) time
I64 trianglesMatrix( const Adj & A )
{
 const std::size_t n = A.size();
 I64 t = 0;
 for( std::size_t a = 0 ; a < n ; ++a )
  for( std::size_t b = a + 1 ; b < n ; ++b )
   if( A[ a ][ b ] )
    for( std::size_t c = b + 1 ; c < n ; ++c )
     t += A[ b ][ c ] && A[ a ][ c ];
 return( t );
 }

// the triangles of an unoriented graph by its lists: countTriangles() of
// the library, each edge oriented from the end of smaller degree, each
// triangle counted once at its first node; O( m sqrt( m ) ) time, against
// O( m n ) for the merge of the sorted lists of the ends of each edge

// the triangles of the complete graph K_n, counted on the matrix, are
// binom( n , 3 ) = n ( n - 1 ) ( n - 2 ) / 6; n = 2 and 3; n out of
// [ 0 , 67 ] refused
ADS_TEST( "collezioni-09-grafi-1-12-1" )
{
 for( int n = 0 ; n <= 40 ; ++n ) {
  Adj K( n , std::vector< int >( n , 1 ) );
  for( int i = 0 ; i < n ; ++i ) K[ i ][ i ] = 0;
  CHECK( trianglesMatrix( K ) == trianglesComplete( n ) );
  CHECK( trianglesMatrix( K ) == I64( n ) * ( n - 1 ) * ( n - 2 ) / 6 );
  }
 CHECK( trianglesComplete( 67 ) == I64( 67 ) * 66 * 65 / 6 );
 CHECK( trianglesComplete( 2 ) == 0 && trianglesComplete( 3 ) == 1 );
 CHECK_THROWS( trianglesComplete( -1 ) , contract_violation );
 CHECK_THROWS( trianglesComplete( 68 ) , contract_violation );
 }

// the matrix and the lists against hasEdge() on all the triples, on random
// graphs; K_n by lists
ADS_TEST( "collezioni-09-grafi-1-12-2" )
{
 std::mt19937 g( 912 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 0 , 25 );
  const ListGraph<> G = randomGraph( g , n , 0.3 , false );
  I64 e = 0;
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b )
    for( int c = 0 ; c < n ; ++c )
     e += ( a != b ) && ( b != c ) && ( a != c ) && G.hasEdge( a , b ) &&
          G.hasEdge( b , c ) && G.hasEdge( a , c );
  CHECK( trianglesMatrix( matrixOf( G ) ) == e / 6 );
  CHECK( countTriangles( G ) == e / 6 );
  }
 ListGraph<> K( 30 , false );
 for( int i = 0 ; i < 30 ; ++i )
  for( int j = i + 1 ; j < 30 ; ++j ) K.insertEdge( j , i );
 CHECK( countTriangles( K ) == I64( binomial( 30 , 3 ) ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 09-grafi, 1.14, BFS in matrice ---------------------*/
/*--------------------------------------------------------------------------*/

/// bfs() on the adjacency matrix M from r: the order in which the nodes
/// are examined and their distances, -1 if not reached; the row of each
/// node scanned in full, Theta( n^2 ) time, Theta( n ) space
/// \pre r is a row of M
std::pair< std::vector< int > , std::vector< int > >
bfsMatrix( const Adj & M , int r )
{
 ADS_PRE( ( r >= 0 ) && ( std::size_t( r ) < M.size() ) );
 const int n = int( M.size() );
 std::vector< int > order , dist( n , -1 ) , Q = { r };
 dist[ r ] = 0;
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  order.push_back( u );
  for( int v = 0 ; v < n ; ++v )
   if( M[ u ][ v ] && ( dist[ v ] < 0 ) ) {
    dist[ v ] = dist[ u ] + 1;
    Q.push_back( v );
    }
  }
 return( std::pair( order , dist ) );
 }

// random oriented graphs: the order and the distances of the visit of the
// matrix against bfs() and distance() of the library on the MatrixGraph
ADS_TEST( "collezioni-09-grafi-1-14" )
{
 std::mt19937 g( 914 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 25 );
  const ListGraph<> L = randomGraph( g , n , 0.15 , r % 2 );
  const Adj M = matrixOf( L );
  MatrixGraph<> G( n , r % 2 );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( M[ u ][ v ] ) G.insertEdge( u , v );
  const int s = randInt( g , 0 , n - 1 );
  std::vector< int > order;
  bfs( G , s , [ & ]( int u ) { order.push_back( u ); } ,
       []( int , int ) {} );
  const auto [ o , d ] = bfsMatrix( M , s );
  CHECK( o == order );
  const auto e = distance( G , s ).first;
  for( int v = 0 ; v < n ; ++v )
   CHECK( d[ v ] == ( e[ v ] == INF ? -1 : e[ v ] ) );
  }
 CHECK_THROWS( bfsMatrix( Adj() , 0 ) , contract_violation );
 // one node, a node with no arc out (only itself), r out of the rows
 const Adj two = { { 0 , 0 } , { 1 , 0 } };
 CHECK( bfsMatrix( Adj( 1 , std::vector< int >( 1 , 0 ) ) , 0 ) ==
        std::pair( std::vector< int >( 1 , 0 ) ,
                   std::vector< int >( 1 , 0 ) ) );
 CHECK( bfsMatrix( two , 0 ).second == std::vector< int >( { 0 , -1 } ) );
 CHECK( bfsMatrix( two , 1 ).first == std::vector< int >( { 1 , 0 } ) );
 CHECK_THROWS( bfsMatrix( two , 2 ) , contract_violation );
 CHECK_THROWS( bfsMatrix( two , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------- 09-grafi, 1.15, Quanto e' grande la foresta? --------------*/
/*--------------------------------------------------------------------------*/

/// treeCount(): the components that are trees, each visited counting its
/// nodes and the sum of their degrees, a tree having degreeSum / 2 =
/// nodes - 1; Theta( n + m ) time
/// \pre the graph is unoriented
template< Graph G >
int treeCount( const G & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< bool > seen( g.bound() , false );
 int trees = 0;
 for( int r : g.V() ) {
  if( seen[ r ] ) continue;
  long nodes = 0 , degrees = 0;
  std::vector< int > Q = { r };
  seen[ r ] = true;
  for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
   ++nodes;
   g.forEachAdj( Q[ h ] , [ & ]( int v ) {
    ++degrees;
    if( ! seen[ v ] ) { seen[ v ] = true; Q.push_back( v ); }
    } );
   }
  trees += ( degrees / 2 == nodes - 1 );
  }
 return( trees );
 }

// random forests with some edges more against the components of cc() of
// the library, each a tree if its edges are one less than its nodes; no
// node, isolated nodes, a cycle, a cycle beside a path; an oriented graph
// refused
ADS_TEST( "collezioni-09-grafi-1-15" )
{
 std::mt19937 g( 915 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 0 , 40 );
  ListGraph<> G( n , false );
  for( int i = 1 ; i < n ; ++i )
   if( randInt( g , 0 , 3 ) ) G.insertEdge( randInt( g , 0 , i - 1 ) , i );
  for( int k = randInt( g , 0 , 4 ) ; ( k > 0 ) && ( n > 1 ) ; --k ) {
   const int u = randInt( g , 0 , n - 1 ) , v = randInt( g , 0 , n - 1 );
   if( u != v ) G.insertEdge( u , v );
   }
  const auto c = cc( G );
  std::map< int , int > nodes , edgesOf;
  for( int u = 0 ; u < n ; ++u ) ++nodes[ c[ u ] ];
  for( const auto & e : edges( G ) ) ++edgesOf[ c[ e.u ] ];
  int e = 0;
  for( auto [ k , v ] : nodes ) e += ( edgesOf[ k ] == v - 1 );
  CHECK( treeCount( G ) == e );
  }
 CHECK( treeCount( ListGraph<>( 0 , false ) ) == 0 );
 CHECK( treeCount( ListGraph<>( 7 , false ) ) == 7 );   // isolated nodes
 // a cycle (no tree), a cycle beside a path (one tree), an oriented graph
 // refused
 const ListGraph<> cyc = makeGraph( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } ,
                                    false );
 CHECK( treeCount( cyc ) == 0 );
 CHECK( treeCount( makeGraph( 6 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } ,
                                    { 3 , 4 } , { 4 , 5 } } , false ) ) == 1 );
 CHECK_THROWS( treeCount( ListGraph<>( 2 , true ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 09-grafi, 1.16, Distanza media ---------------------*/
/*--------------------------------------------------------------------------*/

/// averageDistance(): the mean distance of the other nodes from s, by a
/// visit by levels summing the distances; Theta( n + m ) time
/// \pre the graph is connected (audit), with at least two nodes, s one
template< Graph G >
double averageDistance( const G & g , int s )
{
 ADS_PRE( g.hasNode( s ) && ( g.n() >= 2 ) );
 const auto [ dist , parent ] = distance( g , s );
 long tot = 0;
 for( int v : g.V() ) {
  ADS_PRE( dist[ v ] != INF );
  tot += dist[ v ];
  }
 return( double( tot ) / ( g.n() - 1 ) );
 }

// the path A - B - C: 1 from B, 1.5 from A and C; random connected graphs
// against the rows of floydWarshall() of the library; two nodes, a star
// from its center and from a leaf; one node, a graph not connected, s out
// of the graph refused
ADS_TEST( "collezioni-09-grafi-1-16" )
{
 const ListGraph<> P = makeGraph( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 CHECK( near( averageDistance( P , 1 ) , 1 ) );
 CHECK( near( averageDistance( P , 0 ) , 1.5 ) &&
        near( averageDistance( P , 2 ) , 1.5 ) );
 std::mt19937 g( 916 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 2 , 30 );
  ListGraph< int > G( n , false );
  for( int i = 1 ; i < n ; ++i ) G.insertEdge( randInt( g , 0 , i - 1 ) , i ,
                                               1 );
  for( int k = randInt( g , 0 , n ) ; k > 0 ; --k ) {
   const int u = randInt( g , 0 , n - 1 ) , v = randInt( g , 0 , n - 1 );
   if( u != v ) G.insertEdge( u , v , 1 );
   }
  const auto R = floydWarshall( G );
  const int s = randInt( g , 0 , n - 1 );
  const double e = std::accumulate( R.d[ s ].begin() , R.d[ s ].end() , 0.0 )
                   / ( n - 1 );
  CHECK( near( averageDistance( G , s ) , e ) );
  }
 CHECK_THROWS( averageDistance( ListGraph<>( 1 , false ) , 0 ) ,
               contract_violation );
 // two nodes (1), a star from its center (1) and from a leaf, and the
 // contracts: a graph not connected, s out of the graph
 CHECK( near( averageDistance( makeGraph( 2 , { { 0 , 1 } } , false ) , 1 ) ,
              1 ) );
 const ListGraph<> star = makeGraph( 5 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } ,
                                           { 0 , 4 } } , false );
 CHECK( near( averageDistance( star , 0 ) , 1 ) &&
        near( averageDistance( star , 4 ) , ( 1 + 2 * 3 ) / 4.0 ) );
 CHECK_THROWS( averageDistance( ListGraph<>( 2 , false ) , 0 ) ,
               contract_violation );
 CHECK_THROWS( averageDistance( star , 5 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------- 09-grafi, 1.17, Componenti fortemente connesse -------------*/
/*--------------------------------------------------------------------------*/

/// the trace of the algorithm of the strongly connected components: the
/// first depth-first visit, DfsSchema of the library, from the nodes of
/// start in this order (then from the others, in increasing order), gives
/// the finish times; the second one, dfs() of the library on the
/// transposed graph, starts from the nodes by decreasing finish time and
/// gives the components in the order found
template< Graph G >
std::pair< std::vector< int > , std::vector< std::vector< int > > >
sccTrace( const G & g , const std::vector< int > & start )
{
 DfsSchema< G > first( g );
 auto none = []( int , int , EdgeKind ) {};
 for( int u : start )
  if( first.dt( u ) == 0 ) first.visit( u , none );
 for( int u : g.V() )
  if( first.dt( u ) == 0 ) first.visit( u , none );
 std::vector< int > ft( g.bound() , 0 ) , S = g.V();
 for( int u : S ) ft[ u ] = first.ft( u );
 std::sort( S.begin() , S.end() ,
            [ & ]( int a , int b ) { return( ft[ a ] > ft[ b ] ); } );
 const ListGraph<> T = transpose( g );
 std::vector< bool > visited( g.bound() , false );
 std::vector< std::vector< int > > comps;
 for( int u : S )
  if( ! visited[ u ] ) {
   comps.emplace_back();
   dfs( T , u , visited , [ & ]( int v ) { comps.back().push_back( v ); } ,
        []( int , int ) {} , []( int ) {} );
   std::sort( comps.back().begin() , comps.back().end() );
   }
 return( std::pair( ft , comps ) );
 }

// the graph of the text from b, then a: the finish times and the
// components of the solution, in its order; on random graphs the
// components are those of scc() of the library and of the mutual
// reachability
ADS_TEST( "collezioni-09-grafi-1-17" )
{
 enum { a , b , c , d , e , f , g , h };
 const ListGraph<> G = makeGraph( 8 , { { a , b } , { a , e } , { b , c } ,
                                        { b , f } , { c , f } , { c , d } ,
                                        { c , h } , { c , g } , { d , h } ,
                                        { e , f } , { e , a } , { f , b } ,
                                        { g , h } , { h , d } } , true );
 const auto [ ft , comps ] = sccTrace( G , { b , a } );
 CHECK( ft == Ints( { 16 , 12 , 11 , 8 , 15 , 4 , 10 , 7 } ) );
 CHECK( comps == std::vector< Ints >( { { a , e } , { b , c , f } , { g } ,
                                         { d , h } } ) );
 std::mt19937 rng( 917 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( rng , 1 , 20 );
  const ListGraph<> R = randomGraph( rng , n , 1.5 / n , true );
  const auto C = sccTrace( R , {} ).second;
  const auto id = scc( R );
  std::vector< std::vector< int > > reach( n );
  for( int u = 0 ; u < n ; ++u ) reach[ u ] = distance( R , u ).first;
  std::vector< int > mine( n , -1 );
  for( std::size_t k = 0 ; k < C.size() ; ++k )
   for( int u : C[ k ] ) mine[ u ] = int( k );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v ) {
    const bool same = ( reach[ u ][ v ] != INF ) && ( reach[ v ][ u ] != INF );
    CHECK( ( mine[ u ] == mine[ v ] ) == same );
    CHECK( ( id[ u ] == id[ v ] ) == same );
    }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 09-grafi, 1.18, Facebook ------------------------*/
/*--------------------------------------------------------------------------*/

/// facebook3(): the largest number of nodes at distance 1 or 2 from a
/// node, count( v ) marking the neighbours and then their neighbours;
/// O( n ( n + m ) ) time, O( n ) space
template< Graph G >
int facebook3( const G & g )
{
 int best = 0;
 std::vector< int > mark( g.bound() , -1 );
 for( int v : g.V() ) {
  int c = 0;
  mark[ v ] = v;
  for( int u : g.adj( v ) )
   if( mark[ u ] != v ) { mark[ u ] = v; ++c; }
  for( int u : g.adj( v ) )
   for( int w : g.adj( u ) )
    if( mark[ w ] != v ) { mark[ w ] = v; ++c; }
  best = std::max( best , c );
  }
 return( best );
 }

// random graphs against the nodes at distance 1 or 2 of distance() of the
// library, node by node; no node, one, a star, a path, a triangle
ADS_TEST( "collezioni-09-grafi-1-18" )
{
 std::mt19937 g( 918 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 30 );
  const ListGraph<> G = randomGraph( g , n , 1.5 / n , false );
  int e = 0;
  for( int v = 0 ; v < n ; ++v ) {
   const auto d = distance( G , v ).first;
   e = std::max( e , int( std::count( d.begin() , d.end() , 1 ) +
                          std::count( d.begin() , d.end() , 2 ) ) );
   }
  CHECK( facebook3( G ) == e );
  }
 ListGraph<> star( 20 , false );
 for( int i = 1 ; i < 20 ; ++i ) star.insertEdge( 0 , i );
 CHECK( facebook3( star ) == 19 && facebook3( ListGraph<>( 1 ) ) == 0 );
 // no node, a path (the middle sees four), a triangle (no node counted
 // twice)
 CHECK( facebook3( ListGraph<>( 0 , false ) ) == 0 );
 ListGraph<> path( 7 , false );
 for( int i = 1 ; i < 7 ; ++i ) path.insertEdge( i - 1 , i );
 CHECK( facebook3( path ) == 4 );
 CHECK( facebook3( makeGraph( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } ,
                              false ) ) == 2 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 09-grafi, 1.19, Cicli verdi -----------------------*/
/*--------------------------------------------------------------------------*/

/// hasGreenCycle(): a depth-first visit of the subgraph of the green
/// nodes, 0 not visited, 1 on the stack, 2 finished; an edge to a node on
/// the stack closes a green cycle; Theta( n + m ) time
template< Graph G >
bool hasGreenCycle( const G & g , const std::vector< bool > & green )
{
 ADS_PRE( int( green.size() ) == g.bound() );
 std::vector< int > state( g.bound() , 0 );
 std::function< bool( int ) > rec = [ & ]( int u ) {
  state[ u ] = 1;
  for( int v : g.adj( u ) )
   if( green[ v ] ) {
    if( state[ v ] == 1 ) return( true );
    if( ( state[ v ] == 0 ) && rec( v ) ) return( true );
    }
  state[ u ] = 2;
  return( false );
  };
 for( int u : g.V() )
  if( green[ u ] && ( state[ u ] == 0 ) && rec( u ) )
   return( true );
 return( false );
 }

// random oriented graphs and colors against hasCycle() of the library on
// the subgraph of the green nodes; no node green, all green, a 2-cycle
// with one end not green, a green ring of 500 broken by one node; colors
// of the wrong size refused
ADS_TEST( "collezioni-09-grafi-1-19" )
{
 std::mt19937 g( 919 );
 int cycles = 0;
 for( int r = 0 ; r < 1000 ; ++r ) {
  const int n = randInt( g , 1 , 15 );
  const ListGraph<> G = randomGraph( g , n , 1.5 / n , true );
  std::vector< bool > green( n );
  for( int u = 0 ; u < n ; ++u ) green[ u ] = randInt( g , 0 , 3 ) > 0;
  ListGraph<> S( n , true );
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) )
    if( green[ u ] && green[ v ] ) S.insertEdge( u , v );
  const bool e = hasCycle( S );
  CHECK( hasGreenCycle( G , green ) == e );
  cycles += e;
  CHECK( ! hasGreenCycle( G , std::vector< bool >( n , false ) ) );
  CHECK( hasGreenCycle( G , std::vector< bool >( n , true ) ) ==
         hasCycle( G ) );
  }
 CHECK( cycles > 100 && cycles < 900 );
 // a 2-cycle green, with one end not green, a long green cycle; colors of
 // the wrong size refused
 const ListGraph<> two = makeGraph( 2 , { { 0 , 1 } , { 1 , 0 } } , true );
 CHECK( hasGreenCycle( two , { true , true } ) &&
        ! hasGreenCycle( two , { true , false } ) );
 ListGraph<> ring( 500 , true );
 for( int i = 0 ; i < 500 ; ++i ) ring.insertEdge( i , ( i + 1 ) % 500 );
 std::vector< bool > all( 500 , true );
 CHECK( hasGreenCycle( ring , all ) );
 all[ 250 ] = false;
 CHECK( ! hasGreenCycle( ring , all ) );
 CHECK_THROWS( hasGreenCycle( two , { true } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 09-grafi, 1.20, Sorgenti e pozzi --------------------*/
/*--------------------------------------------------------------------------*/

/// printSinkSources(): the sources, never the end of an edge, and the
/// sinks, of empty list, in increasing order; Theta( n + m ) time
template< Graph G >
std::pair< std::vector< int > , std::vector< int > >
sourcesSinks( const G & g )
{
 std::vector< bool > in( g.bound() , false );
 std::vector< int > sources , sinks;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { in[ v ] = true; } );
 for( int u : g.V() ) {
  if( ! in[ u ] ) sources.push_back( u );
  if( g.adj( u ).empty() ) sinks.push_back( u );
  }
 return( std::pair( sources , sinks ) );
 }

// random oriented graphs against the degrees of the list of their edges;
// a node alone (both), a 2-cycle (neither), a path (one of each)
ADS_TEST( "collezioni-09-grafi-1-20" )
{
 std::mt19937 g( 920 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 0 , 25 );
  const ListGraph<> G = randomGraph( g , n , 1.0 / std::max( n , 1 ) , true );
  std::vector< int > in( n , 0 ) , out( n , 0 ) , so , si;
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( ( u != v ) && G.hasEdge( u , v ) ) { ++out[ u ]; ++in[ v ]; }
  for( int u = 0 ; u < n ; ++u ) {
   if( in[ u ] == 0 ) so.push_back( u );
   if( out[ u ] == 0 ) si.push_back( u );
   }
  CHECK( sourcesSinks( G ) == std::pair( so , si ) );
  }
 // a node alone is both, a 2-cycle has neither, a path has one of each
 using Ns = std::vector< int >;
 CHECK( sourcesSinks( ListGraph<>( 1 , true ) ) ==
        std::pair( Ns( { 0 } ) , Ns( { 0 } ) ) );
 CHECK( sourcesSinks( makeGraph( 2 , { { 0 , 1 } , { 1 , 0 } } , true ) ) ==
        std::pair( Ns() , Ns() ) );
 CHECK( sourcesSinks( makeGraph( 3 , { { 2 , 1 } , { 1 , 0 } } , true ) ) ==
        std::pair( Ns( { 2 } ) , Ns( { 0 } ) ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 09-grafi, 1.21, Diametro di alberi -------------------*/
/*--------------------------------------------------------------------------*/

// the diameter of a binary tree with its height is treeDiameter() of the
// library: the longest path within a subtree or through the root,
// ( hl + 1 ) + ( hr + 1 ); < -1 , 0 > for the empty tree; Theta( n ) time,
// Theta( h ) space

// random binary trees against the largest distance of distance() of the
// library from each node of the tree as an unoriented graph; height and
// diameter of the empty tree, a node, a path, a perfect tree
ADS_TEST( "collezioni-09-grafi-1-21" )
{
 std::mt19937 g( 921 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 40 );
  int next = 0;
  BTPtr t( randomTree( g , n , [ & ]() { return( next++ ); } ) );
  ListGraph<> G( n , false );
  pre_visit( t.get() , [ & ]( BT * u ) {
   if( u->parent() ) G.insertEdge( u->parent()->read() , u->read() ); } );
  int e = 0;
  for( int u = 0 ; u < n ; ++u )
   for( int x : distance( G , u ).first ) e = std::max( e , x );
  const auto [ h , d ] = treeDiameter( t.get() );
  CHECK( d == e && h == height( t.get() ) );
  }
 CHECK( treeDiameter< int >( nullptr ) == std::pair( -1 , 0 ) );
 BTPtr one( new BT( 0 ) ) , p( perfectTree( 5 ) );
 CHECK( treeDiameter( one.get() ) == std::pair( 0 , 0 ) );
 CHECK( treeDiameter( p.get() ) == std::pair( 5 , 10 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 09-grafi, 1.22, Stessa distanza ---------------------*/
/*--------------------------------------------------------------------------*/

/// sameDistance(): the nodes reached from s1 and s2 at the same distance,
/// by the two visits by levels of distance() of the library; Theta( n + m )
/// \pre s1 and s2 are nodes
template< Graph G >
int sameDistance( const G & g , int s1 , int s2 )
{
 ADS_PRE( g.hasNode( s1 ) && g.hasNode( s2 ) );
 const auto d1 = distance( g , s1 ).first , d2 = distance( g , s2 ).first;
 int c = 0;
 for( int u : g.V() )
  c += ( d1[ u ] != INF ) && ( d1[ u ] == d2[ u ] );
 return( c );
 }

// random oriented graphs against floydWarshall() of the library with unit
// weights; s1 = s2, a path from its ends, nodes reached by neither; nodes
// out of the graph refused
ADS_TEST( "collezioni-09-grafi-1-22" )
{
 std::mt19937 g( 922 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = randInt( g , 1 , 25 );
  const ListGraph< int > G = randomGraph( g , n , 1.5 / n , true , 1 );
  const auto R = floydWarshall( G );
  const int s1 = randInt( g , 0 , n - 1 ) , s2 = randInt( g , 0 , n - 1 );
  int e = 0;
  for( int u = 0 ; u < n ; ++u )
   e += ( R.d[ s1 ][ u ] != INF ) && ( R.d[ s1 ][ u ] == R.d[ s2 ][ u ] );
  CHECK( sameDistance( G , s1 , s2 ) == e );
  int reach = 0;
  for( int u = 0 ; u < n ; ++u ) reach += R.d[ s1 ][ u ] != INF;
  CHECK( sameDistance( G , s1 , s1 ) == reach );
  }
 // a node unreachable from both is not counted, an undirected path from
 // its two ends (only the middle when the length is even), nodes out of
 // the graph refused
 ListGraph<> path( 5 , false );
 for( int i = 1 ; i < 5 ; ++i ) path.insertEdge( i - 1 , i );
 CHECK( sameDistance( path , 0 , 4 ) == 1 &&
        sameDistance( path , 0 , 3 ) == 0 );
 CHECK( sameDistance( ListGraph<>( 3 , true ) , 0 , 1 ) == 0 );
 CHECK_THROWS( sameDistance( path , 0 , 5 ) , contract_violation );
 CHECK_THROWS( sameDistance( path , -1 , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 10-heap, 1.2, Esecuzione heapsort() -------------------*/
/*--------------------------------------------------------------------------*/

/// the trace of heapsort() by the maxHeapRestore() of the library: the
/// array after each restore of heapBuild(), from floor( n / 2 ) - 1 down to
/// 0, then after each swap of A[ 0 ] with A[ i ] and the restore that
/// follows it
std::vector< std::string > heapsortTrace( std::string A )
{
 std::vector< char > V( A.begin() , A.end() );
 std::vector< std::string > out = { A };
 auto now = [ & ]() { out.emplace_back( V.begin() , V.end() ); };
 for( std::size_t i = V.size() / 2 ; i-- > 0 ; ) {
  maxHeapRestore( V , i , V.size() );
  now();
  }
 for( std::size_t i = V.size() ; i-- > 1 ; ) {
  std::swap( V[ 0 ] , V[ i ] );
  now();
  maxHeapRestore( V , 0 , i );
  now();
  }
 return( out );
 }

// the 25 arrays of the solution for ORDINAMENTO (the last restore, on one
// item, changes nothing and is not shown), the end equal to heapsort() and
// std::sort on random words; equal keys are allowed but heapsort() is not
// stable; the empty word, one letter, two
ADS_TEST( "collezioni-10-heap-1-2" )
{
 const std::vector< std::string > e = {
  "ORDINAMENTO" , "ORDITAMENNO" , "ORDNTAMEINO" , "ORMNTADEINO" ,
  "OTMNRADEINO" , "TRMNOADEINO" , "ORMNOADEINT" , "ROMNOADEINT" ,
  "NOMNOADEIRT" , "OOMNNADEIRT" , "IOMNNADEORT" , "ONMINADEORT" ,
  "ENMINADOORT" , "NNMIEADOORT" , "DNMIEANOORT" , "NIMDEANOORT" ,
  "AIMDENNOORT" , "MIADENNOORT" , "EIADMNNOORT" , "IEADMNNOORT" ,
  "DEAIMNNOORT" , "EDAIMNNOORT" , "ADEIMNNOORT" , "DAEIMNNOORT" ,
  "ADEIMNNOORT" };
 const auto T = heapsortTrace( "ORDINAMENTO" );
 CHECK( T.size() == e.size() + 1 && T.back() == e.back() );
 CHECK( std::equal( e.begin() , e.end() , T.begin() ) );
 std::mt19937 g( 102 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::string w( randInt( g , 0 , 30 ) , ' ' );
  for( char & ch : w ) ch = char( 'A' + randInt( g , 0 , 5 ) );
  std::vector< char > V( w.begin() , w.end() );
  heapsort( V );
  std::string s = w;
  std::sort( s.begin() , s.end() );
  CHECK( heapsortTrace( w ).back() == s &&
         std::string( V.begin() , V.end() ) == s );
  }
 std::vector< std::pair< int , char > > P = { { 1 , 'a' } , { 1 , 'b' } };
 heapsort( P , []( const auto & x , const auto & y ) {
  return( x.first < y.first ); } );
 CHECK( P[ 0 ].second == 'b' );               // not stable
 // the empty word (only itself), one letter (no restore, no swap), two
 CHECK( heapsortTrace( "" ) == std::vector< std::string >( 1 , "" ) );
 CHECK( heapsortTrace( "Q" ) == std::vector< std::string >( 1 , "Q" ) );
 CHECK( heapsortTrace( "AB" ) ==
        std::vector< std::string >( { "AB" , "BA" , "AB" , "AB" } ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 10-heap, 1.4, Versione iterativa --------------------*/
/*--------------------------------------------------------------------------*/

/// the recursive maxHeapRestore() of the book, the oracle of the iterative
/// one of the library
void maxHeapRestoreRec( std::vector< int > & A , std::size_t i ,
                        std::size_t dim )
{
 std::size_t max = i;
 const std::size_t l = 2 * i + 1 , r = 2 * i + 2;
 if( ( l < dim ) && ( A[ l ] > A[ max ] ) ) max = l;
 if( ( r < dim ) && ( A[ r ] > A[ max ] ) ) max = r;
 if( max != i ) {
  std::swap( A[ i ] , A[ max ] );
  maxHeapRestoreRec( A , max , dim );
  }
 }

// the iterative maxHeapRestore() of the library against the recursive one,
// after lowering one item of a random heap, on the whole vector and on a
// prefix; one item, equal items, the root down a whole path, a leaf, a
// prefix of one; i out of the heap refused
ADS_TEST( "collezioni-10-heap-1-4" )
{
 std::mt19937 g( 104 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const int n = randInt( g , 1 , 40 );
  Ints A = randInts( g , n , 0 , 50 );
  const std::size_t dim = randInt( g , 1 , n );
  std::vector< int > P( A.begin() , A.begin() + dim );
  heapBuild( P );
  std::copy( P.begin() , P.end() , A.begin() );
  const std::size_t i = randInt( g , 0 , int( dim ) - 1 );
  A[ i ] -= randInt( g , 0 , 60 );
  Ints B = A;
  maxHeapRestore( A , i , dim );
  maxHeapRestoreRec( B , i , dim );
  CHECK( A == B && isMaxHeap( A , dim ) );
  }
 Ints one = { 5 };
 maxHeapRestore( one , 0 , 1 );
 CHECK( one == Ints( { 5 } ) );
 CHECK_THROWS( maxHeapRestore( one , 1 , 1 ) , contract_violation );
 // equal items (nothing moves), the root down a whole path, a leaf (no
 // child), a dim smaller than the vector (the rest untouched)
 Ints eq( 15 , 4 ) , path = { 0 , 9 , 8 , 7 , 6 , 5 , 4 } ,
      cut = { 1 , 9 , 9 };
 maxHeapRestore( eq , 0 , 15 );
 CHECK( eq == Ints( 15 , 4 ) );
 Ints rp = path;
 maxHeapRestore( path , 0 , 7 );
 maxHeapRestoreRec( rp , 0 , 7 );
 CHECK( path == rp && isMaxHeap( path , 7 ) );
 Ints leaf = { 1 , 5 , 6 };
 maxHeapRestore( leaf , 2 , 3 );
 CHECK( leaf == Ints( { 1 , 5 , 6 } ) );
 maxHeapRestore( cut , 0 , 1 );
 CHECK( cut == Ints( { 1 , 9 , 9 } ) );
 }

/*--------------------------------------------------------------------------*/
/*------------ 10-heap, 1.6, heapBuild() basato su inserimenti -------------*/
/*--------------------------------------------------------------------------*/

// the heap built by insertions, each item at the end of the heap and moved
// up while larger than its parent [see heapBuildByInsertion() of the
// library], is the heap of the HeapPriorityQueue of the library, with the
// priorities negated as it keeps the minimum on top; no item, one,
// increasing and decreasing items
ADS_TEST( "collezioni-10-heap-1-6-1" )
{
 std::mt19937 g( 1061 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  const Ints A = randInts( g , randInt( g , 0 , 50 ) , -20 , 20 );
  Ints H = A;
  heapBuildByInsertion( H );
  HeapPriorityQueue< int , int > Q( std::max( int( A.size() ) , 1 ) );
  for( int x : A ) Q.insert( x , -x );
  Ints q = Q.priorities();
  for( int & x : q ) x = -x;
  CHECK( H == q && isMaxHeap( H , H.size() ) );
  Ints s = A , t = H;
  std::sort( s.begin() , s.end() );
  std::sort( t.begin() , t.end() );
  CHECK( s == t );
  }
 Ints E;
 heapBuildByInsertion( E );
 CHECK( E.empty() );
 // one item, increasing items (each one up to the root), decreasing ones
 // (none moves)
 Ints O = { 3 } , up( 7 ) , down( 7 );
 std::iota( up.begin() , up.end() , 1 );
 std::iota( down.rbegin() , down.rend() , 1 );
 const Ints dn = down;
 heapBuildByInsertion( O );
 heapBuildByInsertion( up );
 heapBuildByInsertion( down );
 CHECK( O == Ints( { 3 } ) && down == dn );
 CHECK( up == Ints( { 7 , 4 , 6 , 1 , 3 , 2 , 5 } ) && isMaxHeap( up , 7 ) );
 }

// [ 4 , 7 , 9 , 10 ]: heapBuild() of the library gives [ 10 , 7 , 9 , 4 ],
// the insertions [ 10 , 9 , 7 , 4 ]; on random vectors both are heaps and
// sometimes differ
ADS_TEST( "collezioni-10-heap-1-6-2" )
{
 Ints A = { 4 , 7 , 9 , 10 } , I = A;
 heapBuildByInsertion( I );
 CHECK( I == Ints( { 10 , 9 , 7 , 4 } ) );
 heapBuild( A );
 CHECK( A == Ints( { 10 , 7 , 9 , 4 } ) );
 std::mt19937 g( 1062 );
 int differ = 0;
 for( int r = 0 ; r < 500 ; ++r ) {
  Ints B = randInts( g , randInt( g , 0 , 20 ) , 0 , 99 );
  Ints H = B;
  heapBuildByInsertion( H );
  heapBuild( B );
  CHECK( isMaxHeap( B , B.size() ) && isMaxHeap( H , H.size() ) );
  differ += ( B != H );
  }
 CHECK( differ > 100 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 10-heap, 1.7, k-merging -------------------------*/
/*--------------------------------------------------------------------------*/

/// merge(): the k sorted vectors in one: kWayMerge() of the library, a tree
/// of losers holding the next item of each vector, where the text uses a
/// heap of k items; Theta( n log k ) time for n items, Theta( k ) extra
/// space
/// \pre each vector is sorted
std::vector< int > kMerge( const std::vector< std::vector< int > > & A )
{
 return( kWayMerge( A ) );
 }

// random sorted vectors against std::sort of their concatenation; equal
// items, empty vectors among others, no vector, only empty ones, a single
// one, the extremes of int; a vector not sorted refused
ADS_TEST( "collezioni-10-heap-1-7-2" )
{
 std::mt19937 g( 107 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  const int k = randInt( g , 2 , 12 ) , m = randInt( g , 1 , 20 );
  std::vector< Ints > A( k );
  Ints all;
  for( auto & a : A ) {
   a = randInts( g , m , -50 , 50 );
   std::sort( a.begin() , a.end() );
   all.insert( all.end() , a.begin() , a.end() );
   }
  std::sort( all.begin() , all.end() );
  CHECK( kMerge( A ) == all );
  }
 CHECK( kMerge( { { 1 , 1 } , { 1 , 1 } } ) == Ints( 4 , 1 ) );
 CHECK( kMerge( { { 3 } , {} , { 1 , 2 } } ) == Ints( { 1 , 2 , 3 } ) );
 CHECK( kMerge( {} ).empty() && kMerge( { {} , {} } ).empty() );
 CHECK( kMerge( { { 1 , 5 } } ) == Ints( { 1 , 5 } ) );
 CHECK( kMerge( { { std::numeric_limits< int >::max() } ,
                  { std::numeric_limits< int >::min() } } ) ==
        Ints( { std::numeric_limits< int >::min() ,
                std::numeric_limits< int >::max() } ) );
 CHECK_THROWS( kMerge( { { 2 , 1 } , { 0 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 10-heap, 1.8, Heap ternario -----------------------*/
/*--------------------------------------------------------------------------*/

/// the height of a ternary heap of n items: the level h begins at the place
/// ( 3^h - 1 ) / 2, so h = floor( log3( 2 n - 1 ) ), by integers
/// \pre n >= 1
int ternaryHeight( I64 n )
{
 ADS_PRE( n >= 1 );
 int h = 0;
 for( I64 p = 3 ; p <= 2 * n - 1 ; p *= 3 )
  ++h;
 return( h );
 }

/// maxHeapify3(): A[ i ] down the ternary max-heap A[ 0 .. n - 1 ],
/// swapped with the largest of its children 3 i + 1 , 3 i + 2 , 3 i + 3;
/// Theta( log3 n ) levels, 3 comparisons each
void maxHeapify3( std::vector< int > & A , std::size_t i , std::size_t n )
{
 std::size_t max = i;
 for( std::size_t c = 3 * i + 1 ; ( c <= 3 * i + 3 ) && ( c < n ) ; ++c )
  if( A[ c ] > A[ max ] ) max = c;
 if( max != i ) {
  std::swap( A[ i ] , A[ max ] );
  maxHeapify3( A , max , n );
  }
 }

/// deleteMax3(): the root taken out, the last item in its place and down;
/// Theta( log3 n )
/// \pre the heap is not empty
int deleteMax3( std::vector< int > & A )
{
 ADS_PRE( ! A.empty() );
 const int top = A[ 0 ];
 A[ 0 ] = A.back();
 A.pop_back();
 if( ! A.empty() ) maxHeapify3( A , 0 , A.size() );
 return( top );
 }

/// insert3(): x at the end, up while larger than its parent ( i - 1 ) / 3;
/// Theta( log3 n )
void insert3( std::vector< int > & A , int x )
{
 A.push_back( x );
 for( std::size_t i = A.size() - 1 ;
      ( i > 0 ) && ( A[ ( i - 1 ) / 3 ] < A[ i ] ) ; i = ( i - 1 ) / 3 )
  std::swap( A[ i ] , A[ ( i - 1 ) / 3 ] );
 }

/// whether A is a ternary max-heap
bool isTernaryMaxHeap( const std::vector< int > & A )
{
 for( std::size_t i = 1 ; i < A.size() ; ++i )
  if( A[ dary_parent( long( i ) , 3 ) ] < A[ i ] )
   return( false );
 return( true );
 }

// floor( log3( 2 n - 1 ) ) against the level of the last place, by
// dary_parent() of the library, up to 10^5
ADS_TEST( "collezioni-10-heap-1-8-2" )
{
 for( I64 n = 1 ; n <= 100000 ; ++n ) {
  int level = 0;
  for( long i = long( n - 1 ) ; i > 0 ; i = dary_parent( i , 3 ) ) ++level;
  CHECK( ternaryHeight( n ) == level );
  }
 CHECK( ternaryHeight( 1 ) == 0 && ternaryHeight( 4 ) == 1 &&
        ternaryHeight( 5 ) == 2 );
 CHECK_THROWS( ternaryHeight( 0 ) , contract_violation );
 }

// insert3() then deleteMax3() to the end: the items by decreasing value,
// the heap kept at each step; with distinct items the vector is the one of
// the DaryHeap of the library with D = 3, which pops the same way
ADS_TEST( "collezioni-10-heap-1-8-3" )
{
 std::mt19937 g( 1083 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = randInt( g , 1 , 60 );
  Ints A( n );
  std::iota( A.begin() , A.end() , 0 );
  std::shuffle( A.begin() , A.end() , g );
  Ints H;
  DaryHeap< int , std::greater< int > , 3 > D;
  for( int x : A ) { insert3( H , x ); D.push( x ); }
  Ints popped;
  while( ! H.empty() ) {
   CHECK( Ints( D.begin() , D.end() ) == H );
   popped.push_back( deleteMax3( H ) );
   CHECK( D.pop() == popped.back() && isTernaryMaxHeap( H ) );
   }
  std::sort( A.rbegin() , A.rend() );
  CHECK( popped == A );
  }
 Ints R = { 2 , 2 , 2 , 1 };
 CHECK( deleteMax3( R ) == 2 && isTernaryMaxHeap( R ) && R.size() == 3 );
 Ints E;
 CHECK_THROWS( deleteMax3( E ) , contract_violation );
 }

// insert3() against push() of the DaryHeap with D = 3, item by item, and
// the heap kept with repeated items too
ADS_TEST( "collezioni-10-heap-1-8-4" )
{
 std::mt19937 g( 1084 );
 for( int r = 0 ; r < 500 ; ++r ) {
  Ints H;
  DaryHeap< int , std::greater< int > , 3 > D;
  for( int x : randInts( g , randInt( g , 0 , 60 ) , 0 , 9 ) ) {
   insert3( H , x );
   D.push( x );
   CHECK( Ints( D.begin() , D.end() ) == H && isTernaryMaxHeap( H ) );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ 10-heap, 1.9, Massimo in un min-heap ------------------*/
/*--------------------------------------------------------------------------*/

/// the largest item of a min-heap, among the leaves only, the places
/// floor( n / 2 ) .. n - 1; Theta( n ) time
/// \pre the heap is not empty
int maxOfMinHeap( const std::vector< int > & A )
{
 ADS_PRE( ! A.empty() );
 return( *std::max_element( A.begin() + A.size() / 2 , A.end() ) );
 }

// random min-heaps, built by heapBuild() of the library with the order
// reversed: the leaves hold the largest item, never smaller than an
// internal one; one item, two, equal items, a sorted vector; the empty
// heap refused
ADS_TEST( "collezioni-10-heap-1-9" )
{
 std::mt19937 g( 109 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  Ints A = randInts( g , randInt( g , 1 , 40 ) , -100 , 100 );
  heapBuild( A , std::greater<>() );
  const int m = maxOfMinHeap( A );
  CHECK( m == *std::max_element( A.begin() , A.end() ) );
  for( std::size_t i = 0 ; i < A.size() / 2 ; ++i )
   CHECK( A[ i ] <= m );
  }
 CHECK( maxOfMinHeap( { 7 } ) == 7 );
 CHECK( maxOfMinHeap( { 1 , 9 } ) == 9 && maxOfMinHeap( { 4 , 4 , 4 } ) == 4 );
 CHECK( maxOfMinHeap( { 1 , 2 , 3 , 4 , 5 , 6 , 7 } ) == 7 );
 CHECK_THROWS( maxOfMinHeap( {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------ 10-heap, 1.10, Verifica della proprieta' di heap ------------*/
/*--------------------------------------------------------------------------*/

// checkMaxHeap() of the text is isMaxHeap() of the library: A[ ( i - 1 ) /
// 2 ] >= A[ i ] for each i, without changing A, Theta( n )

// isMaxHeap() of the library, the solution, against the check of both the
// children of each node, on every vector of up to 7 items in { 0 , 1 , 2 }
// and on the prefixes; heapBuild() gives heaps
ADS_TEST( "collezioni-10-heap-1-10" )
{
 for( int n = 0 ; n <= 7 ; ++n ) {
  int all = 1;
  for( int i = 0 ; i < n ; ++i ) all *= 3;
  for( int c = 0 ; c < all ; ++c ) {
   Ints A;
   for( int x = c , i = 0 ; i < n ; ++i , x /= 3 ) A.push_back( x % 3 );
   for( int dim = 0 ; dim <= n ; ++dim ) {
    bool e = true;
    for( int i = 0 ; i < dim ; ++i )
     for( int ch : { 2 * i + 1 , 2 * i + 2 } )
      if( ( ch < dim ) && ( A[ i ] < A[ ch ] ) ) e = false;
    CHECK( isMaxHeap( A , dim ) == e );
    }
   }
  }
 std::mt19937 g( 1010 );
 for( int r = 0 ; r < 200 ; ++r ) {
  Ints A = randInts( g , randInt( g , 0 , 100 ) , 0 , 1000 );
  heapBuild( A );
  CHECK( isMaxHeap( A , A.size() ) );
  }
 CHECK_THROWS( isMaxHeap( Ints( 2 ) , 3 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File exam_collezionia.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
