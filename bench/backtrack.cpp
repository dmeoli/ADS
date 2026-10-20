/*--------------------------------------------------------------------------*/
/*--------------------------- File backtrack.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the dancing links: the exact cover of the pentominoes
 * on 6 x 10 and 3 x 20 squares, of the pairs of Langford and of the
 * queens as covers, against ExactCover of the book and, if KNUTH names the
 * directory of their programs, DLX1, DLX2 and SSXCC of Knuth, run as
 * processes on a file of the instance (their counts of memory accesses
 * compiled out, so that they run at their best).
 *
 * Each cell is the median of r runs of the time to count all the
 * solutions, in nanoseconds per run; the exact covers of ADS include the
 * building of the structure from the options, and those of Knuth the
 * reading of the file and the start of the process. The size of a cell is
 * the number of solutions. The contracts are off (ADS_CONTRACTS=0).
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

#include <array>
#include <cstdlib>
#include <fstream>
#include <set>
#include <string>
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

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , {} );
 const int r = o.runs;
 covers( pentominoes( 3 , 20 ) , r );
 covers( pentominoes( 6 , 10 ) , r );
 covers( langford( 11 ) , r );
 covers( queens( 12 ) , r );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File backtrack.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
