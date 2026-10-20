/*--------------------------------------------------------------------------*/
/*------------------------- File combinatorial.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the operations on words and of the combinatorial
 * generation: the population count of an array of n words and of the
 * intersection of two, against the loop of std::popcount, the count of
 * boost::dynamic_bitset and, if found, libpopcnt; the Morton codes of n
 * pairs against interleave() of the book and, if found, libmorton; the n!
 * permutations of n distinct items and those of a multiset against
 * std::next_permutation, the algorithms of Generation.h and, if found,
 * for_each_permutation of Hinnant's combinations; the t-combinations of
 * [ 0 , n ) against std::prev_permutation on a mask of booleans, the
 * algorithms of Generation.h and for_each_combination.
 *
 * Each cell is the median of r runs, in nanoseconds per word, per pair or
 * per object visited; a visit reads two items of the object, the same for
 * every generator, so that the generators are compared by their own work.
 * The size of a cell of the generators is the number of objects. The
 * contracts are off (ADS_CONTRACTS=0).
 *
 *   combinatorial [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/combinatorial/Bitwise.h"
#include "ADS/combinatorial/FastBitwise.h"
#include "ADS/combinatorial/FastGeneration.h"
#include "ADS/combinatorial/Generation.h"

#if __has_include( <boost/dynamic_bitset.hpp> )
 #include <boost/dynamic_bitset.hpp>
 #define ADS_HAS_BOOST 1
#endif
#if __has_include( <libpopcnt.h> )
 #include <libpopcnt.h>
 #define ADS_HAS_LIBPOPCNT 1
#endif
#if __has_include( <libmorton/morton.h> )
 #include <libmorton/morton.h>
 #define ADS_HAS_LIBMORTON 1
#endif
#if __has_include( <combinations.h> )
 #include <combinations.h>
 #define ADS_HAS_HINNANT 1
#endif

#include <algorithm>
#include <bit>
#include <string>
#include <vector>

using namespace ADS::bench;

namespace {

/// the time of work() alone, r runs, in ns per one of ops
template< class W >
double timed( W && work , int r , std::size_t ops )
{
 return( median( [](){ return( 0 ); } , [ & ]( int ) { work(); } , r ,
                 ops ) );
 }

void popcounts( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< ADS::Word > A( n ) , B( n );
 for( auto & w : A ) w = g();
 for( auto & w : B ) w = g();
 auto loop = [ & ]( void ) {
  std::uint64_t s = 0;
  for( std::size_t i = 0 ; i < n ; ++i )
   s += std::uint64_t( std::popcount( A[ i ] ) );
  sink = sink + s;
  };
 for( const char * who : { "std::popcount" , "std::popcount A/A" } )
  print( "popcount" , "array" , n , who , timed( loop , r , n ) );
 print( "popcount" , "array" , n , "ADS::sidewaysSum" , timed( [ & ](){
         std::uint64_t s = 0;
         for( std::size_t i = 0 ; i < n ; ++i )
          s += std::uint64_t( ADS::sidewaysSum( A[ i ] ) );
         sink = sink + s; } , r , n ) );
#ifdef ADS_HAS_BOOST
 boost::dynamic_bitset< ADS::Word > D( A.begin() , A.end() );
 print( "popcount" , "array" , n , "boost::dynamic_bitset" , timed( [ & ](){
         sink = sink + D.count(); } , r , n ) );
#endif
#ifdef ADS_HAS_LIBPOPCNT
 print( "popcount" , "array" , n , "libpopcnt" , timed( [ & ](){
         sink = sink + popcnt( A.data() , n * 8 ); } , r , n ) );
#endif
 print( "popcount" , "array" , n , "ADS::popcount" , timed( [ & ](){
         sink = sink + ADS::popcount( A.data() , n ); } , r , n ) );
 print( "popcount" , "and" , n , "std::popcount" , timed( [ & ](){
         std::uint64_t s = 0;
         for( std::size_t i = 0 ; i < n ; ++i )
          s += std::uint64_t( std::popcount( A[ i ] & B[ i ] ) );
         sink = sink + s; } , r , n ) );
 print( "popcount" , "and" , n , "ADS::popcountAnd" , timed( [ & ](){
         sink = sink + ADS::popcountAnd( A.data() , B.data() , n ); } , r ,
         n ) );
 }

void mortons( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::uint32_t > X( n ) , Y( n );
 std::vector< ADS::Word > Z( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  X[ i ] = std::uint32_t( g() );
  Y[ i ] = std::uint32_t( g() );
  Z[ i ] = g();
  }
 auto encode = [ & ]( const char * who , auto && f ) {
  print( "morton" , "encode" , n , who , timed( [ & ](){
          ADS::Word s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s ^= f( X[ i ] , Y[ i ] );
          sink = sink + s; } , r , n ) );
  };
 auto decode = [ & ]( const char * who , auto && f ) {
  print( "morton" , "decode" , n , who , timed( [ & ](){
          ADS::Word s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s ^= f( Z[ i ] );
          sink = sink + s; } , r , n ) );
  };
 auto book = []( std::uint32_t x , std::uint32_t y ) {
  return( ADS::interleave( x , y ) ); };
 encode( "ADS::interleave" , book );
 encode( "ADS::interleave A/A" , book );
 encode( "ADS::mortonEncode" , []( std::uint32_t x , std::uint32_t y ) {
  return( ADS::mortonEncode( x , y ) ); } );
 decode( "ADS::deinterleave" , []( ADS::Word z ) {
  const auto p = ADS::deinterleave( z );
  return( ADS::Word( p.first ) ^ ( ADS::Word( p.second ) << 32 ) ); } );
 decode( "ADS::mortonDecode" , []( ADS::Word z ) {
  const auto p = ADS::mortonDecode( z );
  return( ADS::Word( p.first ) ^ ( ADS::Word( p.second ) << 32 ) ); } );
#ifdef ADS_HAS_LIBMORTON
 encode( "libmorton" , []( std::uint32_t x , std::uint32_t y ) {
  return( ADS::Word( libmorton::morton2D_64_encode( x , y ) ) ); } );
 decode( "libmorton" , []( ADS::Word z ) {
  uint_fast32_t x , y;
  libmorton::morton2D_64_decode( z , x , y );
  return( ADS::Word( x ) ^ ( ADS::Word( y ) << 32 ) ); } );
#endif
 }

/// the permutations of a, as the case named what
void permutations( const std::string & what , const std::vector< int > & a ,
                   int r , bool distinct )
{
 const int n = int( a.size() );
 std::vector< int > b = a;
 std::sort( b.begin() , b.end() );
 std::size_t count = 0;
 do ++count;
 while( std::next_permutation( b.begin() , b.end() ) );
 auto cell = [ & ]( const char * who , auto && gen ) {
  print( "permutations" , what , count , who , timed( [ & ](){
          std::vector< int > c = a;
          int s = 0;
          gen( c , s );
          sink = sink + std::uint64_t( s ); } , r , count ) );
  };
 auto stdGen = [ & ]( std::vector< int > & c , int & s ) {
  std::sort( c.begin() , c.end() );
  do s += c[ 0 ] ^ c[ n - 1 ];
  while( std::next_permutation( c.begin() , c.end() ) );
  };
 cell( "std::next_permutation" , stdGen );
 cell( "std::next_permutation A/A" , stdGen );
 cell( "ADS::permutationsLex" , [ & ]( std::vector< int > & c , int & s ) {
  ADS::permutationsLex( c , [ & ]( const ADS::Tuple & p ) {
   s += p[ 0 ] ^ p[ n - 1 ]; return( false ); } ); } );
 cell( "ADS::forEachPermutationLex" , [ & ]( std::vector< int > & c ,
                                             int & s ) {
  ADS::forEachPermutationLex( c , [ & ]( const std::vector< int > & p ) {
   s += p[ 0 ] ^ p[ n - 1 ]; } ); } );
 if( ! distinct ) return;
 cell( "ADS::heapPermutations" , [ & ]( std::vector< int > & , int & s ) {
  ADS::heapPermutations( n , [ & ]( const ADS::Tuple & p ) {
   s += p[ 0 ] ^ p[ n - 1 ]; return( false ); } ); } );
 cell( "ADS::plainChanges" , [ & ]( std::vector< int > & , int & s ) {
  ADS::plainChanges( n , [ & ]( const ADS::Tuple & p ) {
   s += p[ 0 ] ^ p[ n - 1 ]; return( false ); } ); } );
#ifdef ADS_HAS_HINNANT
 cell( "hinnant::for_each_permutation" , [ & ]( std::vector< int > & c ,
                                                int & s ) {
  for_each_permutation( c.begin() , c.end() , c.end() ,
                        [ & ]( auto f , auto l ) {
                         s += *f ^ *( l - 1 );
                         return( false ); } ); } );
#endif
 cell( "ADS::forEachPermutation" , [ & ]( std::vector< int > & c ,
                                          int & s ) {
  ADS::forEachPermutation( c , [ & ]( const std::vector< int > & p ) {
   s += p[ 0 ] ^ p[ n - 1 ]; } ); } );
 }

/// the t-combinations of [ 0 , n )
void combinations( int n , int t , int r )
{
 std::size_t count = 0;
 ADS::forEachCombinationMask( n , t , [ & ]( ADS::Word ) { ++count; } );
 const std::string what = "C(" + std::to_string( n ) + "," +
                          std::to_string( t ) + ")";
 auto cell = [ & ]( const char * who , auto && gen ) {
  print( "combinations" , what , count , who , timed( [ & ](){
          int s = 0;
          gen( s );
          sink = sink + std::uint64_t( s ); } , r , count ) );
  };
 auto stdGen = [ & ]( int & s ) {
  std::vector< bool > m( n , false );
  std::fill( m.begin() , m.begin() + t , true );
  do s += int( m[ 0 ] ) ^ int( m[ n - 1 ] );
  while( std::prev_permutation( m.begin() , m.end() ) );
  };
 cell( "std::prev_permutation" , stdGen );
 cell( "std::prev_permutation A/A" , stdGen );
 cell( "ADS::combinationsLex" , [ & ]( int & s ) {
  ADS::combinationsLex( n , t , [ & ]( const ADS::Tuple & c ) {
   s += c[ 0 ] ^ c[ t - 1 ]; return( false ); } ); } );
 cell( "ADS::revolvingDoor" , [ & ]( int & s ) {
  ADS::revolvingDoor( n , t , [ & ]( const ADS::Tuple & c ) {
   s += c[ 0 ] ^ c[ t - 1 ]; return( false ); } ); } );
#ifdef ADS_HAS_HINNANT
 cell( "hinnant::for_each_combination" , [ & ]( int & s ) {
  std::vector< int > v( n );
  for( int i = 0 ; i < n ; ++i ) v[ i ] = i;
  for_each_combination( v.begin() , v.begin() + t , v.end() ,
                        [ & ]( auto f , auto l ) {
                         s += *f ^ *( l - 1 );
                         return( false ); } ); } );
#endif
 cell( "ADS::forEachCombination" , [ & ]( int & s ) {
  ADS::forEachCombination( n , t , [ & ]( const std::vector< int > & c ) {
   s += c[ 0 ] ^ c[ t - 1 ]; } ); } );
 cell( "ADS::forEachCombinationMask" , [ & ]( int & s ) {
  ADS::forEachCombinationMask( n , t , [ & ]( ADS::Word x ) {
   s += int( x & 1 ) ^ int( x >> ( n - 1 ) ); } ); } );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  popcounts( n , o.runs , o.seed );
  mortons( n , o.runs , o.seed );
  }
 for( int n : { 10 , 11 } ) {
  std::vector< int > a( n );
  for( int i = 0 ; i < n ; ++i ) a[ i ] = i;
  permutations( "distinct " + std::to_string( n ) , a , o.runs , true );
  }
 permutations( "multiset 12" , { 0 , 0 , 0 , 1 , 1 , 1 , 2 , 2 , 3 , 3 ,
                                  4 , 5 } , o.runs , false );
 combinations( 40 , 5 , o.runs );
 combinations( 25 , 10 , o.runs );
 combinations( 28 , 14 , o.runs );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File combinatorial.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
