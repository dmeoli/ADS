/*--------------------------------------------------------------------------*/
/*----------------------------- File multi.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the search of many patterns at once: MultiSearcher and
 * AhoCorasick of ADS against k searches of std::string_view::find() and,
 * if found (ADS_HAS_HYPERSCAN), Vectorscan, the portable Hyperscan, on
 * the patterns written as expressions of escaped bytes, which reports
 * every occurrence of every pattern as the others do. Each counts all the
 * occurrences, overlapping ones included, of k patterns of 8 to 16 bytes,
 * half taken from random places of the text and half random over its
 * alphabet, in a text of n bytes [see Texts.h], for k = 1, 8, 32, 64, 256
 * and 1024; find() is left out above 64 patterns.
 *
 * Each cell is the median of r runs of one count, the preparation of the
 * patterns not included, in nanoseconds per byte of text; the contracts
 * are off (ADS_CONTRACTS=0).
 *
 *   multi [ -r runs ] [ -s seed ] n1 n2 ...
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
#include "Texts.h"

#include "ADS/string/AhoCorasick.h"
#include "ADS/string/MultiSearch.h"

#ifdef ADS_HAS_HYPERSCAN
 #include <hs/hs.h>
#endif

#include <random>
#include <set>

using namespace ADS::bench;

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 1000000 , 10000000 } );
 for( const char * kind : textKinds )
  for( std::size_t n : o.sizes ) {
   const std::string T = makeText( kind , n , o.seed );
   const std::set< char > A( T.begin() , T.end() );
   const std::string alphabet( A.begin() , A.end() );
   for( std::size_t k : { 1 , 8 , 32 , 64 , 256 , 1024 } ) {
    std::mt19937_64 g( o.seed + k );
    std::vector< std::string > P;
    for( std::size_t z = 0 ; z < k ; ++z ) {
     const std::size_t m = 8 + g() % 9;
     if( z % 2 == 0 ) P.push_back( T.substr( g() % ( n - m + 1 ) , m ) );
     else {
      std::string p( m , ' ' );
      for( auto & c : p ) c = alphabet[ g() % alphabet.size() ];
      P.push_back( p );
      }
     }
    const std::string what = std::string( kind ) + " k=" +
                             std::to_string( k );
    std::size_t expected = 0;
    bool first = true;
    auto run = [ & ]( const char * who , auto && count ) {
     if( skip( who ) )
      return;
     std::size_t c = 0;
     const double t = median( [](){ return( 0 ); } , [ & ]( int ) {
      c = count();
      sink = sink + c; } , o.runs , n );
     if( first ) expected = c;
     else if( c != expected ) {
      std::fprintf( stderr , "%s: %zu occurrences, not %zu\n" , who , c ,
                    expected );
      std::exit( 1 );
      }
     first = false;
     print( "multi" , what , n , who , t );
     };
    if( ! skip( "ADS::MultiSearcher" ) ||
        ! skip( "ADS::MultiSearcher A/A" ) ) {
     const ADS::MultiSearcher M( P );
     run( "ADS::MultiSearcher" , [ & ]() { return( M.count( T ) ); } );
     run( "ADS::MultiSearcher A/A" , [ & ]() { return( M.count( T ) ); } );
     }
    if( ! skip( "ADS::AhoCorasick" ) ) {
     const ADS::AhoCorasick C( P );
     run( "ADS::AhoCorasick" , [ & ]() { return( C.count( T ) ); } );
     }
    if( k <= 64 )
     run( "std::string_view::find" , [ & ]() {
      std::size_t c = 0;
      const std::string_view V( T );
      for( const auto & p : P )
       for( std::size_t i = V.find( p ) ; i != V.npos ;
            i = V.find( p , i + 1 ) )
        ++c;
      return( c ); } );
#ifdef ADS_HAS_HYPERSCAN
    if( ! skip( "Vectorscan" ) ) {
     // each pattern as an expression of escaped bytes, \xHH (the literal
     // API refuses the patterns with a byte 0)
     std::vector< std::string > X( k );
     std::vector< const char * > E;
     std::vector< unsigned > F , I;
     for( std::size_t z = 0 ; z < k ; ++z ) {
      for( const char c : P[ z ] ) {
       char h[ 5 ];
       std::snprintf( h , sizeof( h ) , "\\x%02x" ,
                      unsigned( static_cast< unsigned char >( c ) ) );
       X[ z ] += h;
       }
      E.push_back( X[ z ].c_str() );
      F.push_back( 0 );
      I.push_back( unsigned( z ) );
      }
     hs_database_t * db = nullptr;
     hs_compile_error_t * err = nullptr;
     if( hs_compile_multi( E.data() , F.data() , I.data() , unsigned( k ) ,
                           HS_MODE_BLOCK , nullptr , &db , &err ) !=
         HS_SUCCESS ) {
      std::fprintf( stderr , "hyperscan: %s\n" , err->message );
      hs_free_compile_error( err );
      return( 1 );
      }
     hs_scratch_t * scratch = nullptr;
     hs_alloc_scratch( db , &scratch );
     run( "Vectorscan" , [ & ]() {
      std::size_t c = 0;
      hs_scan( db , T.data() , unsigned( n ) , 0 , scratch ,
               []( unsigned , unsigned long long , unsigned long long ,
                   unsigned , void * ctx ) {
                ++*static_cast< std::size_t * >( ctx );
                return( 0 ); } , &c );
      return( c ); } );
     hs_free_scratch( scratch );
     hs_free_database( db );
     }
#endif
    }
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File multi.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
