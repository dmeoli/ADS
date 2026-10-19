/*--------------------------------------------------------------------------*/
/*----------------------------- File edit.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the edit distance, of the longest common subsequence
 * and of the approximate search: levenshtein(), lcsLength() and
 * approximateMatches() of ADS, and the dynamic programming of the textbook
 * (editDistance(), lcsTable() and stringMatchingSingleRow(), up to 1000
 * bytes and 10^5 bytes of text), against edlib (ADS_HAS_EDLIB: the global
 * mode NW for the distance and the infix mode HW for the search) and
 * rapidfuzz-cpp (ADS_HAS_RAPIDFUZZ: levenshtein_distance() and
 * lcs_seq_similarity()), when found.
 *
 * The distances are taken between pairs of strings of n bytes over 4 and
 * 26 letters, the second string the first one with 1% or 10% of random
 * edits, or an unrelated string; 2 10^5 / n pairs per run. The search
 * finds the ends of the substrings at distance at most k = m / 8 from a
 * pattern of m = 16, 64 and 256 bytes, a substring of the text with m / 16
 * edits, in a random text of 10^5 and 10^6 bytes over 4 letters; the time
 * of edlib is that of the best distance and its ends, which costs the same
 * scan of the text.
 *
 * Each cell is the median of r runs, in nanoseconds per byte of the
 * longer string (of the text); the contracts are off (ADS_CONTRACTS=0).
 *
 *   edit [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/EditDistance.h"
#include "ADS/string/LongestCommonSubsequence.h"

#ifdef ADS_HAS_EDLIB
 #include <edlib.h>
#endif
#ifdef ADS_HAS_RAPIDFUZZ
 #include <rapidfuzz/distance.hpp>
#endif

#include <functional>
#include <map>
#include <string_view>

using namespace ADS::bench;

namespace {

/// a random string of n letters of an alphabet of sigma
std::string letters( std::mt19937_64 & g , std::size_t n , unsigned sigma )
{
 std::string s( n , ' ' );
 for( auto & c : s ) c = "ACGTBDEFHIJKLMNOPQRSUVWXYZ"[ g() % sigma ];
 return( s );
 }

/// a with e random edits
std::string mutate( std::mt19937_64 & g , std::string a , std::size_t e ,
                    unsigned sigma )
{
 for( std::size_t k = 0 ; k < e ; ++k ) {
  const std::size_t i = g() % ( a.size() + 1 );
  const char c = "ACGTBDEFHIJKLMNOPQRSUVWXYZ"[ g() % sigma ];
  const auto r = g() % 3;
  if( ( r == 0 ) && ( i < a.size() ) ) a[ i ] = c;
  else if( r == 1 ) a.insert( a.begin() + std::ptrdiff_t( i ) , c );
  else if( i < a.size() ) a.erase( a.begin() + std::ptrdiff_t( i ) );
  }
 return( a );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100 , 1000 , 10000 , 100000 } );
 using Pair = std::pair< std::string , std::string >;
 using Of = std::function< std::size_t( const std::string & ,
                                        const std::string & ) >;
 // the distances and the LCS, with the largest length they are run at
 const std::vector< std::tuple< const char * , const char * , Of ,
                                std::size_t > > D = {
  { "edit distance" , "ADS::levenshtein" , []( const auto & a ,
                                               const auto & b ) {
    return( ADS::levenshtein( a , b ) ); } , 1u << 30 } ,
  { "edit distance" , "ADS::levenshtein A/A" , []( const auto & a ,
                                                   const auto & b ) {
    return( ADS::levenshtein( a , b ) ); } , 1u << 30 } ,
  { "edit distance" , "ADS::editDistance" , []( const auto & a ,
                                                const auto & b ) {
    return( std::size_t( ADS::editDistance( a , b ) ) ); } , 1000 } ,
#ifdef ADS_HAS_EDLIB
  { "edit distance" , "edlib" , []( const auto & a , const auto & b ) {
    EdlibAlignResult r = edlibAlign( a.data() , int( a.size() ) ,
                                     b.data() , int( b.size() ) ,
                                     edlibDefaultAlignConfig() );
    const auto d = std::size_t( r.editDistance );
    edlibFreeAlignResult( r );
    return( d ); } , 1u << 30 } ,
#endif
#ifdef ADS_HAS_RAPIDFUZZ
  { "edit distance" , "rapidfuzz" , []( const auto & a , const auto & b ) {
    return( std::size_t( rapidfuzz::levenshtein_distance( a , b ) ) ); } ,
    1u << 30 } ,
#endif
  { "lcs" , "ADS::lcsLength" , []( const auto & a , const auto & b ) {
    return( ADS::lcsLength( a , b ) ); } , 1u << 30 } ,
  { "lcs" , "ADS::lcsLength A/A" , []( const auto & a , const auto & b ) {
    return( ADS::lcsLength( a , b ) ); } , 1u << 30 } ,
  { "lcs" , "ADS::lcsTable" , []( const auto & a , const auto & b ) {
    return( std::size_t( ADS::lcsTable( a , b ).back().back() ) ); } ,
    1000 } ,
#ifdef ADS_HAS_RAPIDFUZZ
  { "lcs" , "rapidfuzz" , []( const auto & a , const auto & b ) {
    return( std::size_t( rapidfuzz::lcs_seq_similarity( a , b ) ) ); } ,
    1u << 30 } ,
#endif
  };
 for( unsigned sigma : { 4u , 26u } )
  for( const char * what : { "1%" , "10%" , "unrelated" } )
   for( std::size_t n : o.sizes ) {
    std::mt19937_64 g( o.seed + n );
    const std::size_t pairs = std::max< std::size_t >( 1 , 200000 / n );
    std::vector< Pair > P;
    for( std::size_t k = 0 ; k < pairs ; ++k ) {
     std::string a = letters( g , n , sigma );
     std::string b = what[ 0 ] == 'u' ? letters( g , n , sigma ) :
                     mutate( g , a , n / ( what[ 1 ] == '%' ? 100 : 10 ) ,
                             sigma );
     P.emplace_back( std::move( a ) , std::move( b ) );
     }
    const std::string c = "sigma" + std::to_string( sigma ) + " " + what;
    std::map< std::string , std::size_t > first;    // the result of each
    for( const auto & [ bench , who , of , upto ] : D ) {
     if( n > upto ) continue;
     std::size_t sum = 0;
     const double t = median( [](){ return( 0 ); } , [ & ]( int ) {
      sum = 0;
      for( const auto & [ a , b ] : P ) sum += of( a , b );
      sink = sink + sum; } , o.runs , pairs * n );
     if( ! first.count( bench ) ) first[ bench ] = sum;
     else if( first[ bench ] != sum ) {
      std::fprintf( stderr , "%s: %zu, not %zu\n" , who , sum ,
                    first[ bench ] );
      return( 1 );
      }
     print( bench , c , n , who , t );
     }
    }

 // the approximate search
 for( std::size_t n : { 100000 , 1000000 } )
  for( std::size_t m : { 16 , 64 , 256 } ) {
   std::mt19937_64 g( o.seed + n + m );
   const std::string T = letters( g , n , 4 );
   const std::string P = mutate( g , T.substr( g() % ( n - m ) , m ) ,
                                 m / 16 , 4 );
   const std::size_t k = m / 8;
   const std::string c = "sigma4 m=" + std::to_string( m );
   std::size_t best = 0;
   for( const char * who : { "ADS::approximateMatches" ,
                             "ADS::approximateMatches A/A" } )
    print( "approximate search" , c , n , who , median(
            [](){ return( 0 ); } , [ & ]( int ) {
             const auto R = ADS::approximateMatches( T , P , k );
             best = k + 1;
             for( const auto & e : R ) best = std::min( best , e.second );
             sink = sink + best; } , o.runs , n ) );
   if( ( n <= 100000 ) && ( m <= 64 ) )
    print( "approximate search" , c , n , "ADS::stringMatchingSingleRow" ,
           median( [](){ return( 0 ); } , [ & ]( int ) {
            sink = sink + std::uint64_t( ADS::stringMatchingSingleRow(
                                          P , T ) ); } , o.runs , n ) );
#ifdef ADS_HAS_EDLIB
   std::size_t e = 0;
   print( "approximate search" , c , n , "edlib" , median(
           [](){ return( 0 ); } , [ & ]( int ) {
            EdlibAlignResult r = edlibAlign( P.data() , int( P.size() ) ,
                                  T.data() , int( n ) ,
                                  edlibNewAlignConfig( int( k ) ,
                                  EDLIB_MODE_HW , EDLIB_TASK_DISTANCE ,
                                  nullptr , 0 ) );
            e = r.editDistance < 0 ? k + 1 : std::size_t( r.editDistance );
            edlibFreeAlignResult( r );
            sink = sink + e; } , o.runs , n ) );
   if( e != best ) {
    std::fprintf( stderr , "edlib: %zu, not %zu\n" , e , best );
    return( 1 );
    }
#endif
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File edit.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
