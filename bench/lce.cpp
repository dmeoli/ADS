/*--------------------------------------------------------------------------*/
/*------------------------------ File lce.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the longest common extensions: LceIndex of ADS against
 * lce() of the SuffixArray of the textbook (the range minimum of Bender
 * and Farach-Colton), the direct comparison of the two suffixes (8 bytes at
 * a time) and, if sdsl-lite is found (ADS_HAS_SDSL), its succinct range
 * minimum rmq_succinct_sct on the same LCP array and inverse suffix array.
 * The texts are those of Texts.h, of n bytes; the queries are 10^6 pairs
 * of suffixes, "random" (two places at random, mostly short extensions)
 * or "near" (two suffixes at most 8 ranks apart in SA, the long ones);
 * the direct comparison answers the first 10^4 only on the texts of long
 * repeats, and the textbook is left out above 10^6.
 *
 * Each cell is the median of r runs of the queries, in nanoseconds per
 * query, the construction excluded; the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   lce [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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

#include "ADS/string/LongestCommonExtension.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixSorting.h"

#ifdef ADS_HAS_SDSL
 #include <sdsl/rmq_succinct_sct.hpp>
#endif

#include <bit>
#include <cstring>
#include <optional>
#include <random>

using namespace ADS::bench;

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 , 10000000 } );
 const std::size_t Q = 1000000;
 for( const char * kind : textKinds )
  for( std::size_t n : o.sizes ) {
   const std::string T = makeText( kind , n , o.seed );
   const auto SA = ADS::suffixArray( T );
   std::vector< std::uint32_t > rank( n );
   for( std::size_t r = 0 ; r < n ; ++r ) rank[ SA[ r ] ] = std::uint32_t( r );
   const auto LCP = ADS::lcpArrayPhi( T , SA );
   std::optional< ADS::LceIndex<> > X;     // built only to be measured
   if( ! skip( "ADS::LceIndex" ) || ! skip( "ADS::LceIndex A/A" ) )
    X.emplace( T );
   const std::string k( kind );
   const bool longRepeats = ( k == "one-byte" ) || ( k == "fibonacci" ) ||
                            ( k == "repeats" );
   // the direct comparison, 8 bytes at a time
   auto direct = [ & ]( std::size_t i , std::size_t j ) {
    const std::size_t far = n - std::max( i , j );
    std::size_t h = 0;
    for( ; h + 8 <= far ; h += 8 ) {
     std::uint64_t a , b;
     std::memcpy( &a , T.data() + i + h , 8 );
     std::memcpy( &b , T.data() + j + h , 8 );
     if( a != b ) return( h + std::size_t( std::countr_zero( a ^ b ) ) / 8 );
     }
    while( ( h < far ) && ( T[ i + h ] == T[ j + h ] ) ) ++h;
    return( h );
    };
   for( const char * set : { "random" , "near" } ) {
    std::mt19937_64 g( o.seed + n );
    std::vector< std::pair< std::size_t , std::size_t > > P( Q );
    for( auto & [ i , j ] : P ) {
     if( std::string( set ) == "random" ) {
      i = g() % n;
      j = g() % n;
      }
     else {
      const std::size_t r = g() % n , d = 1 + g() % 8;
      i = SA[ r ];
      j = SA[ std::min( r + d , n - 1 ) ];
      }
     }
    const std::string what = k + " " + set;
    std::uint64_t expected = 0;
    auto run = [ & ]( const char * who , auto && lce , std::size_t q ) {
     if( skip( who ) )
      return;
     std::uint64_t c = 0;
     const double t = median( [](){ return( 0 ); } , [ & ]( int ) {
      c = 0;
      for( std::size_t z = 0 ; z < q ; ++z )
       c += lce( P[ z ].first , P[ z ].second );
      sink = sink + c; } , o.runs , q );
     if( q == Q ) {
      if( expected == 0 ) expected = c;
      else if( c != expected ) {
       std::fprintf( stderr , "%s: a different sum of the lce\n" , who );
       std::exit( 1 );
       }
      }
     print( "lce" , what , n , who , t );
     };
    run( "ADS::LceIndex" , [ & ]( std::size_t i , std::size_t j ) {
     return( X->lce( i , j ) ); } , Q );
    run( "ADS::LceIndex A/A" , [ & ]( std::size_t i , std::size_t j ) {
     return( X->lce( i , j ) ); } , Q );
    if( ( n <= 1000000 ) && ! skip( "ADS::SuffixArray::lce" ) ) {
     static ADS::SuffixArray B;
     static std::string built;
     if( built != T ) {
      B = ADS::SuffixArray( T );
      built = T;
      }
     run( "ADS::SuffixArray::lce" , [ & ]( std::size_t i , std::size_t j ) {
      return( B.lce( i , j ) ); } , Q );
     }
    run( "direct comparison" , direct , longRepeats ? 10000 : Q );
#ifdef ADS_HAS_SDSL
    if( ! skip( "sdsl rmq_succinct_sct" ) ) {
     static sdsl::int_vector<> L;
     static sdsl::rmq_succinct_sct<> R;
     L = sdsl::int_vector<>( LCP.size() );
     for( std::size_t r = 0 ; r < LCP.size() ; ++r ) L[ r ] = LCP[ r ];
     R = sdsl::rmq_succinct_sct<>( &L );
     run( "sdsl rmq_succinct_sct" , [ & ]( std::size_t i , std::size_t j ) {
      if( i == j ) return( std::size_t( n - i ) );
      std::size_t a = rank[ i ] , b = rank[ j ];
      if( a > b ) std::swap( a , b );
      return( std::size_t( L[ R( a , b - 1 ) ] ) ); } , Q );
     }
#endif
    }
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File lce.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
