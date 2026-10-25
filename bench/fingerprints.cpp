/*--------------------------------------------------------------------------*/
/*-------------------------- File fingerprints.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the fingerprints of the rows of doubles, the cuts of a
 * decomposition: rowFingerprint() of ADS, +0 equal to -0 and every NaN to
 * every NaN, against the hash of each value by std::hash< double >
 * combined as boost::hash_combine does, and, if Boost is found, against
 * boost::hash_range (neither of which makes the NaNs equal), on n values
 * in rows of L of them:
 *
 * - dense8, dense64, dense512: dense rows of 8, 64 and 512 values;
 * - sparse8, sparse64: sparse rows of 8 and 64 entries, an index of 32
 *   bits and a value each, the indices hashed with the values;
 * - equal64: the comparison of each dense row of 64 values with an equal
 *   copy of it, by sameRow() against std::equal (which tells -0 from +0
 *   equal but no NaN from itself).
 *
 * Each cell is the median of r runs, in nanoseconds per value; the
 * contracts are off (ADS_CONTRACTS=0).
 *
 *   fingerprints [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/dictionary/RowFingerprint.h"

#if __has_include( <boost/container_hash/hash.hpp> )
 #include <boost/container_hash/hash.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <functional>

using namespace ADS::bench;

namespace {

/// the combination of boost::hash_combine
inline std::size_t combine( std::size_t h , std::size_t x )
{
 return( h ^ ( x + 0x9E3779B9 + ( h << 6 ) + ( h >> 2 ) ) );
 }

/// the cell of the case what for who: the median time of f(), per value
/// of the n
template< class F >
void rows( const char * what , const char * who , std::size_t n , int r ,
           F && f )
{
 if( skip( who ) )
  return;
 print( "fingerprints" , what , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { sink = sink + f(); } , r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  std::mt19937_64 g( o.seed );
  std::vector< double > x( n );
  for( auto & v : x )
   v = double( std::int64_t( g() % 2001 ) - 1000 ) / 64;
  std::vector< std::int32_t > J( n );
  for( auto & j : J )
   j = std::int32_t( g() % 100000 );

  for( const std::size_t L : { 8 , 64 , 512 } ) {
   const std::string what = "dense" + std::to_string( L );
   const std::size_t R = n / L;
   auto ads = [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i )
     t += ADS::rowFingerprint( std::span< const double >( &x[ i * L ] ,
                                                          L ) );
    return( t ); };
   for( const char * who : { "ADS::rowFingerprint" ,
                             "ADS::rowFingerprint A/A" } )
    rows( what.c_str() , who , n , r , ads );
   rows( what.c_str() , "std::hash" , n , r , [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i ) {
     std::size_t h = 0;
     for( std::size_t k = 0 ; k < L ; ++k )
      h = combine( h , std::hash< double >()( x[ i * L + k ] ) );
     t += h;
     }
    return( t ); } );
#ifdef ADS_HAS_BOOST
   rows( what.c_str() , "boost::hash_range" , n , r , [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i )
     t += boost::hash_range( x.begin() + i * L , x.begin() + ( i + 1 ) * L );
    return( t ); } );
#endif
   }

  for( const std::size_t L : { 8 , 64 } ) {
   const std::string what = "sparse" + std::to_string( L );
   const std::size_t R = n / L;
   auto ads = [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i )
     t += ADS::rowFingerprint( L , &J[ i * L ] , &x[ i * L ] );
    return( t ); };
   for( const char * who : { "ADS::rowFingerprint" ,
                             "ADS::rowFingerprint A/A" } )
    rows( what.c_str() , who , n , r , ads );
   rows( what.c_str() , "std::hash" , n , r , [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i ) {
     std::size_t h = 0;
     for( std::size_t k = 0 ; k < L ; ++k ) {
      h = combine( h , std::hash< std::int32_t >()( J[ i * L + k ] ) );
      h = combine( h , std::hash< double >()( x[ i * L + k ] ) );
      }
     t += h;
     }
    return( t ); } );
#ifdef ADS_HAS_BOOST
   rows( what.c_str() , "boost::hash_range" , n , r , [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i ) {
     std::size_t h = boost::hash_range( J.begin() + i * L ,
                                        J.begin() + ( i + 1 ) * L );
     boost::hash_combine( h , boost::hash_range( x.begin() + i * L ,
                                                 x.begin() + ( i + 1 ) * L ) );
     t += h;
     }
    return( t ); } );
#endif
   }

  {
   constexpr std::size_t L = 64;
   const std::size_t R = n / L;
   const std::vector< double > y = x;
   auto ads = [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i )
     t += ADS::sameRow( std::span< const double >( &x[ i * L ] , L ) ,
                        std::span< const double >( &y[ i * L ] , L ) );
    return( t ); };
   for( const char * who : { "ADS::sameRow" , "ADS::sameRow A/A" } )
    rows( "equal64" , who , n , r , ads );
   rows( "equal64" , "std::equal" , n , r , [ & ]() {
    std::uint64_t t = 0;
    for( std::size_t i = 0 ; i < R ; ++i )
     t += std::equal( x.begin() + i * L , x.begin() + ( i + 1 ) * L ,
                      y.begin() + i * L );
    return( t ); } );
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File fingerprints.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
