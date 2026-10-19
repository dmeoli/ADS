/*--------------------------------------------------------------------------*/
/*---------------------------- File suffix.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the suffix arrays: suffixArray() of ADS, on 32 bits,
 * and suffixArraySais() of the textbook against libsais (ADS_HAS_LIBSAIS)
 * and libdivsufsort (ADS_HAS_DIVSUFSORT), when found; then the LCP array
 * of the suffix array, lcpArrayPhi() of ADS and lcpArray() of the
 * textbook (Kasai et al.) against libsais_plcp() followed by libsais_lcp(),
 * the same method of the permuted LCP. The texts are those of Texts.h, of
 * n bytes; the constructions of the textbook, on integers of 64 bits, are
 * left out above 10^6.
 *
 * Each cell is the median of r runs of one construction, in nanoseconds
 * per byte of text, the allocation of its arrays included and the suffix
 * array of the LCP not; the contracts are off (ADS_CONTRACTS=0).
 *
 *   suffix [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixSorting.h"

#ifdef ADS_HAS_LIBSAIS
 #include <libsais.h>
#endif
#ifdef ADS_HAS_DIVSUFSORT
 #include <divsufsort.h>
#endif

#include <cstring>

using namespace ADS::bench;

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 , 10000000 } );
 for( const char * kind : textKinds )
  for( std::size_t n : o.sizes ) {
   const std::string T = makeText( kind , n , o.seed );
#if defined( ADS_HAS_LIBSAIS ) || defined( ADS_HAS_DIVSUFSORT )
   const auto * t = reinterpret_cast< const std::uint8_t * >( T.data() );
#endif
   const auto ref = ADS::suffixArray( T );
   const int r = o.runs;
   auto out = [ & ]( const char * bench , const char * who , double ns ) {
    print( bench , kind , n , who , ns );
    };
   auto same = [ & ]( const auto & A , const char * who ) {
    if( ! std::equal( A.begin() , A.end() , ref.begin() , ref.end() ,
                      []( auto a , auto b ) {
                       return( std::uint64_t( a ) == std::uint64_t( b ) );
                       } ) ) {
     std::fprintf( stderr , "%s: a different suffix array\n" , who );
     std::exit( 1 );
     }
    };

   // the suffix arrays
   for( const char * who : { "ADS::suffixArray" , "ADS::suffixArray A/A" } )
    out( "suffix array" , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           const auto A = ADS::suffixArray( T );
           sink = sink + A[ n / 2 ]; } , r , n ) );
   if( n <= 1000000 ) {
    std::vector< std::size_t > A;
    out( "suffix array" , "ADS::suffixArraySais" , median(
          [](){ return( 0 ); } , [ & ]( int ) {
           A = ADS::suffixArraySais( T );
           sink = sink + A[ n / 2 ]; } , r , n ) );
    same( A , "ADS::suffixArraySais" );
    }
#ifdef ADS_HAS_LIBSAIS
   {
    std::vector< std::int32_t > A;
    out( "suffix array" , "libsais" , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           A = std::vector< std::int32_t >( n );
           libsais( t , A.data() , std::int32_t( n ) , 0 , nullptr );
           sink = sink + std::uint64_t( A[ n / 2 ] ); } , r , n ) );
    same( A , "libsais" );
    }
#endif
#ifdef ADS_HAS_DIVSUFSORT
   {
    std::vector< saidx_t > A;
    out( "suffix array" , "divsufsort" , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           A = std::vector< saidx_t >( n );
           divsufsort( t , A.data() , saidx_t( n ) );
           sink = sink + std::uint64_t( A[ n / 2 ] ); } , r , n ) );
    same( A , "divsufsort" );
    }
#endif

   // the LCP arrays of the same suffix array
   const auto L = ADS::lcpArrayPhi( T , ref );
   for( const char * who : { "ADS::lcpArrayPhi" , "ADS::lcpArrayPhi A/A" } )
    out( "lcp" , who , median( [](){ return( 0 ); } , [ & ]( int ) {
          const auto P = ADS::lcpArrayPhi( T , ref );
          sink = sink + P[ n / 2 ]; } , r , n ) );
   if( n <= 1000000 ) {
    const std::vector< std::size_t > S( ref.begin() , ref.end() );
    std::vector< std::size_t > K;
    out( "lcp" , "ADS::lcpArray (Kasai)" , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           K = ADS::lcpArray( T , S );
           sink = sink + K[ n / 2 ]; } , r , n ) );
    if( ! std::equal( K.begin() , K.end() , L.begin() , L.end() ) )
     return( 1 );
    }
#ifdef ADS_HAS_LIBSAIS
   {
    const std::vector< std::int32_t > S( ref.begin() , ref.end() );
    std::vector< std::int32_t > Q;
    out( "lcp" , "libsais plcp+lcp" , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::vector< std::int32_t > P( n );
           Q = std::vector< std::int32_t >( n );
           libsais_plcp( t , S.data() , P.data() , std::int32_t( n ) );
           libsais_lcp( P.data() , S.data() , Q.data() , std::int32_t( n ) );
           sink = sink + std::uint64_t( Q[ n / 2 ] ); } , r , n ) );
    for( std::size_t i = 0 ; i + 1 < n ; ++i )
     if( std::uint32_t( Q[ i + 1 ] ) != L[ i ] ) {
      std::fprintf( stderr , "libsais: a different LCP array\n" );
      return( 1 );
      }
    }
#endif
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File suffix.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
