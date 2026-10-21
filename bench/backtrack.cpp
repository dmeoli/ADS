/*--------------------------------------------------------------------------*/
/*--------------------------- File backtrack.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the dancing links: the exact cover of the pentominoes
 * on 6 x 10 and 3 x 20 squares, of the pairs of Langford and of the
 * queens as covers, against ExactCover of the book and, if KNUTH names the
 * directory of their programs, DLX1, DLX2 and SSXCC of Knuth, run as
 * processes on a file of the instance (their counts of memory accesses
 * compiled out, so that they run at their best). Then the algorithms of
 * the book on the structures of std, Boost and ADS, the same algorithm
 * with the same counts (checked, a difference printed on the standard
 * error): the clause learning of SatSolvers.h on its heap (std::
 * priority_queue, the d-ary and pairing heaps of Boost, DaryHeap of ADS)
 * on 10 random formulae of n variables at the threshold, the A* of
 * FifteenPuzzle.h on its map of the levels (std::map, std::unordered_map,
 * boost::unordered_flat_map, FlatHashTable of ADS) on walks of random
 * moves from the goal, and the subset sum of Pseudopolynomial.h on its
 * bits (std::vector< bool >, boost::dynamic_bitset, BitVector of ADS).
 *
 * Each cell is the median of r runs of the time to count all the
 * solutions, in nanoseconds per run; the exact covers of ADS include the
 * building of the structure from the options, and those of Knuth the
 * reading of the file and the start of the process. The size of a cell is
 * the number of solutions for the covers, n for the clause learning, the
 * instances for the A* and k for the subset sum, timed per formula, per
 * instance and per cell of the table. The contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   backtrack [ -r runs ] [ -s seed ]
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/backtrack/DancingLinks.h"
#include "ADS/backtrack/FastExactCover.h"
#include "ADS/backtrack/SatSolvers.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/intractable/FifteenPuzzle.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/succinct/BitVector.h"

#if __has_include( <boost/heap/d_ary_heap.hpp> )
 #include <boost/dynamic_bitset.hpp>
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #include <boost/unordered/unordered_flat_map.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <unistd.h>

using namespace ADS::bench;

namespace {

/// the time of work() alone, r runs, in ns per run
template< class W >
double timed( W && work , int r )
{
 return( median( [](){ return( 0 ); } , [ & ]( int ) { work(); } , r , 1 ) );
 }

/// an instance of exact cover: the items and the options
struct Cover {
 std::string name;
 int primary = 0 , secondary = 0;
 std::vector< std::vector< int > > options;
 };

/// the pentominoes on h x w squares, h w = 60; on 6 x 10 the centre of X
/// in the upper left quarter, one solution of each class of 4
Cover pentominoes( int h , int w )
{
 const std::vector< std::vector< std::array< int , 2 > > > P = {
  { { 0 , 1 } , { 0 , 2 } , { 1 , 0 } , { 1 , 1 } , { 2 , 1 } } ,   // F
  { { 0 , 0 } , { 0 , 1 } , { 0 , 2 } , { 0 , 3 } , { 0 , 4 } } ,   // I
  { { 0 , 0 } , { 1 , 0 } , { 2 , 0 } , { 3 , 0 } , { 3 , 1 } } ,   // L
  { { 0 , 1 } , { 1 , 1 } , { 2 , 1 } , { 2 , 0 } , { 3 , 0 } } ,   // N
  { { 0 , 0 } , { 0 , 1 } , { 1 , 0 } , { 1 , 1 } , { 2 , 0 } } ,   // P
  { { 0 , 0 } , { 0 , 1 } , { 0 , 2 } , { 1 , 1 } , { 2 , 1 } } ,   // T
  { { 0 , 0 } , { 0 , 2 } , { 1 , 0 } , { 1 , 1 } , { 1 , 2 } } ,   // U
  { { 0 , 0 } , { 1 , 0 } , { 2 , 0 } , { 2 , 1 } , { 2 , 2 } } ,   // V
  { { 0 , 0 } , { 1 , 0 } , { 1 , 1 } , { 2 , 1 } , { 2 , 2 } } ,   // W
  { { 0 , 1 } , { 1 , 0 } , { 1 , 1 } , { 1 , 2 } , { 2 , 1 } } ,   // X
  { { 0 , 1 } , { 1 , 0 } , { 1 , 1 } , { 2 , 1 } , { 3 , 1 } } ,   // Y
  { { 0 , 0 } , { 0 , 1 } , { 1 , 1 } , { 2 , 1 } , { 2 , 2 } } };  // Z
 Cover C;
 C.name = "pentominoes " + std::to_string( h ) + "x" + std::to_string( w );
 C.primary = 12 + h * w;
 for( int p = 0 ; p < 12 ; ++p ) {
  std::set< std::vector< std::array< int , 2 > > > shapes;
  for( int t = 0 ; t < 8 ; ++t ) {          // the 8 symmetries
   std::vector< std::array< int , 2 > > s;
   for( auto [ a , b ] : P[ p ] ) {
    int x = ( t & 1 ) ? b : a , y = ( t & 1 ) ? a : b;
    if( t & 2 ) x = -x;
    if( t & 4 ) y = -y;
    s.push_back( { x , y } );
    }
   int mx = 99 , my = 99;
   for( auto [ x , y ] : s ) {
    mx = std::min( mx , x );
    my = std::min( my , y );
    }
   for( auto & c : s ) c = { c[ 0 ] - mx , c[ 1 ] - my };
   std::sort( s.begin() , s.end() );
   shapes.insert( s );
   }
  for( const auto & s : shapes )
   for( int i = 0 ; i < h ; ++i )
    for( int j = 0 ; j < w ; ++j ) {
     std::vector< int > o = { p };
     bool fits = true;
     for( auto [ x , y ] : s ) {
      if( ( i + x >= h ) || ( j + y >= w ) ) fits = false;
      o.push_back( 12 + ( i + x ) * w + j + y );
      }
     if( ( p == 9 ) && ( h * 2 > w ) &&     // X in a quarter: no mirror
         ( ( 2 * ( i + 1 ) >= h ) || ( 2 * ( j + 1 ) >= w ) ) )
      fits = false;
     if( fits ) C.options.push_back( o );
     }
  }
 return( C );
 }

/// the pairs of Langford of 1 , ... , n
Cover langford( int n )
{
 Cover C;
 C.name = "langford " + std::to_string( n );
 C.primary = 3 * n;
 for( int i = 1 ; i <= n ; ++i )
  for( int j = 0 ; j + i + 1 < 2 * n ; ++j )
   C.options.push_back( { i - 1 , n + j , n + j + i + 1 } );
 return( C );
 }

/// the n queens: rows and columns primary, the diagonals secondary, the
/// rows and the columns from the middle out (organ pipe)
Cover queens( int n )
{
 Cover C;
 C.name = "queens " + std::to_string( n );
 C.primary = 2 * n;
 C.secondary = 4 * n - 2;
 std::vector< int > order;
 for( int k = 0 ; k < n ; ++k )
  order.push_back( n / 2 + ( k & 1 ? -( k + 1 ) / 2 : k / 2 ) );
 for( int r : order )
  for( int c : order )
   C.options.push_back( { r , n + c , 2 * n + r + c ,
                          5 * n - 2 + r - c } );
 return( C );
 }

/// the file of the instance for the programs of Knuth
std::string knuthFile( const Cover & C )
{
 const std::string f = "/tmp/ads-bench-xc-" + std::to_string( ::getpid() ) +
                       ".dlx";
 std::ofstream o( f );
 for( int i = 0 ; i < C.primary + C.secondary ; ++i ) {
  if( i == C.primary ) o << "| ";
  o << "i" << i << " ";
  }
 o << "\n";
 for( const auto & op : C.options ) {
  for( int i : op ) o << "i" << i << " ";
  o << "\n";
  }
 return( f );
 }

void covers( const Cover & C , int r )
{
 long count = 0;
 {
  ADS::FastExactCover X( C.primary , C.secondary );
  for( const auto & o : C.options ) X.addOption( o );
  count = X.solve( []( const std::vector< int > & ) {} );
  }
 auto book = [ & ]( void ) {
  ADS::ExactCover X( C.primary , C.secondary );
  for( const auto & o : C.options ) X.addOption( o );
  sink = sink + std::uint64_t( X.solve( []( const std::vector< int > & ) {
   return( false ); } ) );
  };
 print( "exact cover" , C.name , count , "ADS::ExactCover" ,
        timed( book , r ) );
 print( "exact cover" , C.name , count , "ADS::ExactCover A/A" ,
        timed( book , r ) );
#ifdef ADS_KNUTH
 const std::string f = knuthFile( C );
 for( const char * p : { "dlx1" , "dlx2" , "ssxcc" } ) {
  const std::string cmd = std::string( ADS_KNUTH ) + "/" + p + " < " + f +
                          " > /dev/null 2>&1";
  print( "exact cover" , C.name , count , std::string( "knuth::" ) + p ,
         timed( [ & ](){ sink = sink + std::uint64_t(
                         std::system( cmd.c_str() ) ); } , r ) );
  }
 std::remove( f.c_str() );
#endif
 print( "exact cover" , C.name , count , "ADS::FastExactCover" ,
        timed( [ & ](){
         ADS::FastExactCover X( C.primary , C.secondary );
         for( const auto & o : C.options ) X.addOption( o );
         sink = sink + std::uint64_t( X.solve(
          []( const std::vector< int > & ) {} ) ); } , r ) );
 }

/// the clause learning of the book on random formulae of 3 literals at
/// the threshold, on the heaps of std, Boost and ADS
void bookCdcl( int n , int r , unsigned seed )
{
 using Pair = std::pair< double , int >;
 std::mt19937_64 g( seed );
 std::vector< ADS::Cnf > S( 10 );
 for( auto & F : S )
  for( int i = 0 ; i < int( 4.26 * n ) ; ++i ) {
   ADS::Clause c;
   for( int j = 0 ; j < 3 ; ++j )
    c.push_back( ( 1 + int( g() % n ) ) * ( g() % 2 ? 1 : -1 ) );
   F.push_back( c );
   }
 long want = -1;
 auto cell = [ & ]( const char * who , auto make ) {
  long conflicts = 0;
  print( "book cdcl" , "random 3-sat" , std::size_t( n ) , who ,
         median( [](){ return( 0 ); } , [ & ]( int ) {
          conflicts = 0;
          for( const auto & F : S ) {
           auto X = make( F );
           X.solve();
           conflicts += X.conflicts();
           } } , r , S.size() ) );
  if( want < 0 ) want = conflicts;
  else if( conflicts != want )
   std::fprintf( stderr , "book cdcl: %s has %ld conflicts, not %ld\n" ,
                 who , conflicts , want );
  };
 auto std_ = [ n ]( const ADS::Cnf & F ) { return( ADS::Cdcl( F , n ) ); };
 cell( "std::priority_queue" , std_ );
 cell( "std::priority_queue A/A" , std_ );
#ifdef ADS_HAS_BOOST
 cell( "boost::heap::d_ary_heap d=4" , [ n ]( const ADS::Cnf & F ) {
  return( ADS::BasicCdcl< boost::heap::d_ary_heap< Pair ,
                          boost::heap::arity< 4 > > >( F , n ) ); } );
 cell( "boost::heap::pairing_heap" , [ n ]( const ADS::Cnf & F ) {
  return( ADS::BasicCdcl< boost::heap::pairing_heap< Pair > >( F , n ) ); } );
#endif
 cell( "ADS::DaryHeap d=4" , [ n ]( const ADS::Cnf & F ) {
  return( ADS::BasicCdcl< ADS::DaryHeap< Pair , std::greater< Pair > , 4 > >(
          F , n ) ); } );
 cell( "ADS::DaryHeap d=2" , [ n ]( const ADS::Cnf & F ) {
  return( ADS::BasicCdcl< ADS::DaryHeap< Pair , std::greater< Pair > , 2 > >(
          F , n ) ); } );
 }

/// the hash of a configuration, its bytes
struct BoardHash {
 using is_avalanching = void;
 std::size_t operator()( const ADS::Board & b ) const {
  return( std::size_t( ADS::hashBytes( b.data() , b.size() *
                                       sizeof( int ) ) ) );
  }
 };

/// the A* of the book on walks of m random moves from the goal of the
/// n x n puzzle, on the maps of std, Boost and ADS
void bookAStar( int n , int m , int count , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< ADS::Board > S;
 while( int( S.size() ) < count ) {
  ADS::Board B = ADS::puzzleGoal( n );
  for( int i = 0 ; i < m ; ++i ) {
   const auto M = ADS::puzzleMoves( B , n );
   B = M[ g() % M.size() ];
   }
  S.push_back( B );
  }
 const std::string what = std::to_string( n ) + "x" + std::to_string( n ) +
                          " walk " + std::to_string( m );
 long want = -1;
 auto cell = [ & ]( const char * who , auto solve ) {
  long visited = 0;
  print( "book a-star" , what , S.size() , who , median( [](){
          return( 0 ); } , [ & ]( int ) {
           visited = 0;
           for( const auto & B : S ) {
            long v = 0;
            sink = sink + solve( B , &v );
            visited += v;
            } } , r , S.size() ) );
  if( want < 0 ) want = visited;
  else if( visited != want )
   std::fprintf( stderr , "book a-star: %s visits %ld, not %ld\n" , who ,
                 visited , want );
  };
 auto std_ = [ n ]( const ADS::Board & B , long * v ) {
  return( std::uint64_t( ADS::solvePuzzle( B , n , v )->size() ) ); };
 cell( "std::map" , std_ );
 cell( "std::map A/A" , std_ );
 cell( "std::unordered_map" , [ n ]( const ADS::Board & B , long * v ) {
  return( std::uint64_t( ADS::solvePuzzle< std::unordered_map< ADS::Board ,
                         int , BoardHash > >( B , n , v )->size() ) ); } );
#ifdef ADS_HAS_BOOST
 cell( "boost::unordered_flat_map" , [ n ]( const ADS::Board & B , long * v ) {
  return( std::uint64_t( ADS::solvePuzzle< boost::unordered_flat_map<
                         ADS::Board , int , BoardHash > >( B , n , v )->size()
                         ) ); } );
#endif
 cell( "ADS::FlatHashTable" , [ n ]( const ADS::Board & B , long * v ) {
  return( std::uint64_t( ADS::solvePuzzle< ADS::FlatHashTable< ADS::Board ,
                         int , BoardHash > >( B , n , v )->size() ) ); } );
 }

/// the subset sum of the book, 1000 even numbers to an odd k (all the
/// sums computed), on the bits of std, Boost and ADS
void bookSubsetSum( std::size_t k , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< long > A( 1000 );
 for( auto & a : A ) a = 2 * ( 1 + long( g() % ( k / 100 ) ) );
 const long t = long( k ) | 1;
 const std::size_t ops = A.size() * k;
 auto cell = [ & ]( const char * who , auto solve ) {
  print( "book subset sum" , "even" , k , who , median( [](){
          return( 0 ); } , [ & ]( int ) {
           sink = sink + std::uint64_t( solve() ); } , r , ops ) );
  };
 auto std_ = [ & ](){ return( ADS::subsetSumDP( A , t ) ); };
 cell( "std::vector<bool>" , std_ );
 cell( "std::vector<bool> A/A" , std_ );
#ifdef ADS_HAS_BOOST
 cell( "boost::dynamic_bitset" , [ & ](){
  return( ADS::subsetSumDP< boost::dynamic_bitset<> >( A , t ) ); } );
#endif
 cell( "ADS::BitVector" , [ & ](){
  return( ADS::subsetSumDP< ADS::BitVector >( A , t ) ); } );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , {} );
 const int r = o.runs;
 covers( pentominoes( 3 , 20 ) , r );
 covers( pentominoes( 6 , 10 ) , r );
 covers( langford( 11 ) , r );
 covers( queens( 12 ) , r );
 bookCdcl( 150 , r , o.seed );
 bookCdcl( 200 , r , o.seed );
 bookAStar( 3 , 30 , 200 , r , o.seed );
 bookAStar( 4 , 40 , 30 , r , o.seed );
 bookSubsetSum( 100000 , r , o.seed );
 bookSubsetSum( 1000000 , r , o.seed );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File backtrack.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
