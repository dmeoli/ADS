/*--------------------------------------------------------------------------*/
/*---------------------------- File search.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the exact search: StringSearcher of ADS and the kmp()
 * of the textbook against std::string_view::find(), std::search() with
 * std::default_searcher, std::boyer_moore_searcher and
 * std::boyer_moore_horspool_searcher, memmem() of the C library and, if
 * Boost is found, the searchers boyer_moore, boyer_moore_horspool and
 * knuth_morris_pratt of Boost.Algorithm. Each counts all the occurrences,
 * overlapping ones included, of 4 patterns of m bytes taken from random
 * places of a text of n bytes [see Texts.h], for m = 4, 16, 64 and 256; on
 * the text of one repeated byte the pattern is a^{m-1} b, which never
 * occurs, and the searches of O( n m ) time in the worst case (find(),
 * std::search() with the default searcher and the two searchers of
 * Horspool) are left out there where n m exceeds 10^8.
 *
 * Each cell is the median of r runs of the 4 searches, the preparation of
 * the pattern included, in nanoseconds per byte of text and pattern; the
 * contracts are off (ADS_CONTRACTS=0).
 *
 *   search [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/string/StringMatching.h"
#include "ADS/string/StringSearch.h"

#if __has_include( <boost/algorithm/searching/boyer_moore.hpp> )
 #include <boost/algorithm/searching/boyer_moore.hpp>
 #include <boost/algorithm/searching/boyer_moore_horspool.hpp>
 #include <boost/algorithm/searching/knuth_morris_pratt.hpp>
 #define ADS_HAS_BOOST_SEARCH 1
#endif

#include <cstring>
#include <functional>
#include <string_view>

using namespace ADS::bench;

namespace {

/// the occurrences of P in T by a searcher of std::search()
template< class S >
std::size_t countBy( std::string_view T , const S & s )
{
 std::size_t c = 0;
 for( auto i = T.begin() ; ; ++i ) {
  i = std::search( i , T.end() , s );
  if( i == T.end() ) break;
  ++c;
  }
 return( c );
 }

/// the occurrences of P in T by a searcher of Boost.Algorithm
template< class S >
std::size_t countBoost( std::string_view T , const S & s )
{
 std::size_t c = 0;
 for( auto i = T.begin() ; ; ++i ) {
  i = s( i , T.end() ).first;
  if( i == T.end() ) break;
  ++c;
  }
 return( c );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 using Count = std::function< std::size_t( std::string_view ,
                                           std::string_view ) >;
 // the searches, and whether their time is O( n m ) in the worst case
 const std::vector< std::tuple< const char * , Count , bool > > S = {
  { "ADS::StringSearcher" , []( std::string_view T , std::string_view P ) {
    return( ADS::StringSearcher( P ).count( T ) ); } , false } ,
  { "ADS::StringSearcher A/A" , []( std::string_view T ,
                                     std::string_view P ) {
    return( ADS::StringSearcher( P ).count( T ) ); } , false } ,
  { "ADS::kmp" , []( std::string_view T , std::string_view P ) {
    return( ADS::kmp( T , P ).size() ); } , false } ,
  { "std::string_view::find" , []( std::string_view T ,
                                    std::string_view P ) {
    std::size_t c = 0;
    for( std::size_t i = T.find( P ) ; i != T.npos ; i = T.find( P , i + 1 ) )
     ++c;
    return( c ); } , true } ,
  { "std::search" , []( std::string_view T , std::string_view P ) {
    return( countBy( T , std::default_searcher( P.begin() , P.end() ) ) );
    } , true } ,
  { "std::boyer_moore_searcher" , []( std::string_view T ,
                                       std::string_view P ) {
    return( countBy( T , std::boyer_moore_searcher( P.begin() ,
                                                    P.end() ) ) ); } ,
    false } ,
  { "std::boyer_moore_horspool_searcher" , []( std::string_view T ,
                                                std::string_view P ) {
    return( countBy( T , std::boyer_moore_horspool_searcher( P.begin() ,
                                                             P.end() ) ) );
    } , true } ,
  { "memmem" , []( std::string_view T , std::string_view P ) {
    std::size_t c = 0;
    for( const char * p = T.data() , * e = T.data() + T.size() ; ; ++p ) {
     p = static_cast< const char * >( memmem( p , std::size_t( e - p ) ,
                                              P.data() , P.size() ) );
     if( ! p ) break;
     ++c;
     }
    return( c ); } , false } ,
#ifdef ADS_HAS_BOOST_SEARCH
  { "boost::boyer_moore" , []( std::string_view T , std::string_view P ) {
    return( countBoost( T , boost::algorithm::boyer_moore<
                             std::string_view::const_iterator >(
                             P.begin() , P.end() ) ) ); } , false } ,
  { "boost::boyer_moore_horspool" , []( std::string_view T ,
                                         std::string_view P ) {
    return( countBoost( T , boost::algorithm::boyer_moore_horspool<
                             std::string_view::const_iterator >(
                             P.begin() , P.end() ) ) ); } , true } ,
  { "boost::knuth_morris_pratt" , []( std::string_view T ,
                                       std::string_view P ) {
    return( countBoost( T , boost::algorithm::knuth_morris_pratt<
                             std::string_view::const_iterator >(
                             P.begin() , P.end() ) ) ); } , false } ,
#endif
  };
 for( const char * kind : textKinds )
  for( std::size_t n : o.sizes ) {
   const std::string T = makeText( kind , n , o.seed );
   for( std::size_t m : { 4 , 16 , 64 , 256 } ) {
    if( m > n ) continue;
    std::vector< std::string > P;
    std::mt19937_64 g( o.seed + m );
    for( int k = 0 ; k < 4 ; ++k )
     P.push_back( std::string( kind ) == "one-byte" ?
                  std::string( m - 1 , 'a' ) + "b" :
                  T.substr( g() % ( n - m + 1 ) , m ) );
    const bool hard = std::string( kind ) == "one-byte";
    std::size_t expected = 0;
    bool first = true;
    for( const auto & [ who , count , quadratic ] : S ) {
     if( ( hard && quadratic && ( double( n ) * double( m ) > 1e8 ) ) ||
         skip( who ) )
      continue;
     std::size_t c = 0;
     const double t = median( [](){ return( 0 ); } , [ & ]( int ) {
      c = 0;
      for( const auto & p : P ) c += count( T , p );
      sink = sink + c; } , o.runs , n * P.size() );
     if( first ) expected = c;
     else if( c != expected ) {
      std::fprintf( stderr , "%s: %zu occurrences, not %zu\n" , who , c ,
                    expected );
      return( 1 );
      }
     first = false;
     print( "search" , std::string( kind ) + " m=" + std::to_string( m ) ,
            n , who , t );
     }
    }
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File search.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
