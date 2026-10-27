/*--------------------------------------------------------------------------*/
/*-------------------------- File stringdicts.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the static dictionaries of strings, which map each
 * string to its rank and back and find the strings with a prefix: the
 * Patricia trie in DFUDS and the rear coding with buckets of 16 strings of
 * ADS against the front coding with buckets of 16 strings and the Patricia
 * trie with pointers of ADS, the sorted vector of std::string with
 * std::lower_bound and, where found, the HAT-trie of tsl::htrie_map (which
 * maps a string to a value, kept here as its rank, and has no access by
 * rank), on the two sets of n distinct keys of the benchmark tries: "urls",
 * whose keys share long prefixes, and "words", which share short ones. The
 * cases are the building from the sorted keys, n lookups of present keys
 * in random order (the rank), n lookups of absent keys, n accesses of
 * random ranks (the string), n / 100 counts of the keys that begin with
 * half of a present key (by the range of their ranks), and the bytes per
 * key of the structure in the group space, the raw bytes of the keys
 * being those of "raw".
 *
 * Each cell is the median of r runs, in nanoseconds per operation; the
 * queries run on a structure built once, and the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   stringdicts [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/dictionary/PatriciaTrie.h"
#include "ADS/dictionary/SuccinctPatriciaTrie.h"
#include "ADS/string/FrontCoding.h"
#include "ADS/string/RearCoding.h"

#if __has_include( <tsl/htrie_map.h> )
 #include <tsl/htrie_map.h>
 #define ADS_HAS_HTRIE 1
#endif

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

using namespace ADS::bench;

namespace {

/// a random lower-case word of a to b letters
std::string word( std::mt19937_64 & g , int a , int b )
{
 std::string w( std::size_t( a + int( g() % std::uint64_t( b - a + 1 ) ) ) ,
                'a' );
 for( auto & c : w ) c = char( 'a' + g() % 26 );
 return( w );
 }

/// 2 n distinct keys of the set s, as in the benchmark tries: the first n
/// in the dictionary and the others absent
std::vector< std::string > strings( int s , std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed + unsigned( s ) );
 std::vector< std::string > host , vocab;
 for( int i = 0 ; i < 1000 ; ++i )
  host.push_back( word( g , 3 , 10 ) + "." + ( i % 3 ? "com" : "org" ) );
 for( int i = 0 ; i < 5000 ; ++i ) vocab.push_back( word( g , 2 , 9 ) );
 std::unordered_set< std::string > seen;
 std::vector< std::string > K;
 while( K.size() < 2 * n ) {
  std::string k;
  if( s == 0 ) {
   k = "https://www." + host[ g() % host.size() ];
   for( int d = int( 1 + g() % 4 ) ; d > 0 ; --d )
    k += "/" + vocab[ g() % vocab.size() ];
   k += "/" + std::to_string( g() % 1000 );
   }
  else
   k = word( g , 4 , 16 );
  if( seen.insert( k ).second ) K.push_back( std::move( k ) );
  }
 return( K );
 }

/// a missing operation of a structure
struct None {};

/// the cells of a dictionary built by make() from the sorted keys D, with
/// look( d , q ) (the rank, or n), get( d , i ) (the string of rank i, or
/// None), pre( d , p ) (the number of keys with prefix p) and bytes( d )
template< class M , class L , class G , class P , class B >
void run( const std::string & who , const char * set ,
          const std::vector< std::string > & D ,
          const std::vector< std::string > & H ,
          const std::vector< std::string > & Q ,
          const std::vector< std::uint64_t > & I ,
          const std::vector< std::string > & R , int r , M && make ,
          L && look , G && get , P && pre , B && bytes )
{
 if( skip( who ) )
  return;
 const std::size_t n = D.size();
 const std::string s = set;
 print( "stringdicts" , s + " build" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto d = make();
          sink = sink + std::uint64_t( d != nullptr ); } , r , n ) );
 const auto d = make();
 print( "stringdicts" , s + " lookup hit" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t t = 0;
          for( const auto & k : H ) t += look( *d , k );
          sink = sink + t; } , r , n ) );
 print( "stringdicts" , s + " lookup miss" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t t = 0;
          for( const auto & k : Q ) t += look( *d , k );
          sink = sink + t; } , r , n ) );
 if constexpr( ! std::is_same_v< std::decay_t< G > , None > )
  print( "stringdicts" , s + " access" , n , who , median(
          [](){ return( 0 ); } , [ & ]( int ) {
           std::uint64_t t = 0;
           for( const auto i : I ) t += get( *d , i ).size();
           sink = sink + t; } , r , I.size() ) );
 print( "stringdicts" , s + " prefix" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t t = 0;
          for( const auto & p : R ) t += pre( *d , p );
          sink = sink + t; } , r , R.size() ) );
 print( "space" , s + " stringdicts" , n , who ,
        double( bytes( *d ) ) / double( n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes )
  for( int set = 0 ; set < 2 ; ++set ) {
   const char * name = set == 0 ? "urls" : "words";
   const std::vector< std::string > K = strings( set , n , o.seed );
   std::vector< std::string > D( K.begin() , K.begin() + long( n ) ) , H ,
                              Q( K.begin() + long( n ) , K.end() ) , R;
   std::sort( D.begin() , D.end() );
   std::mt19937_64 g( o.seed );
   H = D;
   std::shuffle( H.begin() , H.end() , g );
   std::vector< std::uint64_t > I( n );
   for( auto & i : I ) i = g() % n;
   for( std::size_t i = 0 ; i < n / 100 ; ++i ) {
    const std::string & k = D[ g() % n ];
    R.push_back( k.substr( 0 , k.size() / 2 ) );
    }
   std::size_t raw = 0;
   for( const auto & k : D ) raw += k.size();
   print( "space" , std::string( name ) + " stringdicts" , n , "raw" ,
          double( raw ) / double( n ) );
   const int r = o.runs;

   using ST = ADS::SuccinctPatriciaTrie;
   for( const char * who : { "ADS::SuccinctPatriciaTrie" ,
                             "ADS::SuccinctPatriciaTrie A/A" } )
    run( who , name , D , H , Q , I , R , r ,
     [ & ](){ return( std::make_unique< ST >( D ) ); } ,
     []( const ST & d , const std::string & q ) { return( d.lookup( q ) ); } ,
     []( const ST & d , std::uint64_t i ) { return( d.access( i ) ); } ,
     []( const ST & d , const std::string & p ) {
      const auto [ a , b ] = d.prefixRange( p ); return( b - a ); } ,
     []( const ST & d ) { return( d.bytes() ); } );

   using RC = ADS::RearCodedDictionary;
   run( "ADS::RearCodedDictionary" , name , D , H , Q , I , R , r ,
    [ & ](){ return( std::make_unique< RC >( D , 16 ) ); } ,
    []( const RC & d , const std::string & q ) { return( d.lookup( q ) ); } ,
    []( const RC & d , std::uint64_t i ) { return( d.access( i ) ); } ,
    []( const RC & d , const std::string & p ) {
     const auto [ a , b ] = d.prefixRange( p ); return( b - a ); } ,
    []( const RC & d ) { return( d.bytes() ); } );

   using FC = ADS::FrontCodedDictionary;
   run( "ADS::FrontCodedDictionary" , name , D , H , Q , I , R , r ,
    [ & ](){ return( std::make_unique< FC >( FC::bucketed( D , 16 ) ) ); } ,
    []( const FC & d , const std::string & q ) {
     const std::size_t i = d.lexicographicPosition( q );
     return( ( i < d.size() ) && ( d.access( i ) == q ) ? i : d.size() ); } ,
    []( const FC & d , std::uint64_t i ) { return( d.access( i ) ); } ,
    []( const FC & d , const std::string & p ) {
     const auto [ a , b ] = d.prefixRange( p ); return( b - a ); } ,
    []( const FC & d ) {
     // its memory: the characters, a length and a place per string and a
     // place per anchor (bytes() is the size of the coding alone)
     return( d.characters() + d.size() * 2 * sizeof( std::size_t ) +
             d.anchors() * sizeof( std::size_t ) ); } );

   using PT = ADS::PatriciaTrie;
   run( "ADS::PatriciaTrie" , name , D , H , Q , I , R , r ,
    [ & ](){ return( std::make_unique< PT >( D ) ); } ,
    []( const PT & d , const std::string & q ) {
     const std::size_t i = d.lexicographicPosition( q );
     return( ( i < d.size() ) && ( d.string( i ) == q ) ? i : d.size() ); } ,
    []( const PT & d , std::uint64_t i ) { return( d.string( i ) ); } ,
    []( const PT & d , const std::string & p ) {
     const auto [ a , b ] = d.prefixRange( p ); return( b - a ); } ,
    []( const PT & d ) {
     // its nodes (three words, a flag and a vector of children) and its
     // edges (a character and a word), with its copy of the strings
     std::size_t b = d.nodes() * ( 4 * sizeof( std::size_t ) +
                                   sizeof( std::vector< int > ) ) +
                     ( d.nodes() - 1 ) * 2 * sizeof( std::size_t );
     for( std::size_t i = 0 ; i < d.size() ; ++i )
      b += sizeof( std::string ) + d.string( i ).capacity();
     return( b ); } );

   using SV = std::vector< std::string >;
   run( "std::lower_bound" , name , D , H , Q , I , R , r ,
    [ & ](){ return( std::make_unique< SV >( D ) ); } ,
    []( const SV & d , const std::string & q ) {
     const auto it = std::lower_bound( d.begin() , d.end() , q );
     return( ( it != d.end() ) && ( *it == q ) ? std::size_t( it - d.begin() )
                                               : d.size() ); } ,
    []( const SV & d , std::uint64_t i ) { return( d[ i ] ); } ,
    []( const SV & d , const std::string & p ) {
     const auto a = std::lower_bound( d.begin() , d.end() , p );
     auto b = a;
     while( ( b != d.end() ) && ( b->compare( 0 , p.size() , p ) == 0 ) ) ++b;
     return( std::size_t( b - a ) ); } ,
    []( const SV & d ) {
     std::size_t b = 0;
     for( const auto & s : d ) b += sizeof( std::string ) + s.capacity();
     return( b ); } );

#ifdef ADS_HAS_HTRIE
   using HT = tsl::htrie_map< char , std::uint64_t >;
   run( "tsl::htrie_map" , name , D , H , Q , I , R , r ,
    [ & ](){
     auto d = std::make_unique< HT >();
     for( std::size_t i = 0 ; i < D.size() ; ++i ) d->insert( D[ i ] , i );
     return( d ); } ,
    [ n ]( const HT & d , const std::string & q ) {
     const auto it = d.find( q );
     return( it == d.end() ? n : it.value() ); } ,
    None() ,
    []( const HT & d , const std::string & p ) {
     const auto [ a , b ] = d.equal_prefix_range( p );
     return( std::size_t( std::distance( a , b ) ) ); } ,
    []( const HT & d ) {
     // the HAT-trie does not report its size: its keys and values, a
     // lower bound
     std::size_t b = 0;
     for( auto it = d.begin() ; it != d.end() ; ++it )
      b += it.key().size() + sizeof( std::uint64_t );
     return( b ); } );
#endif
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File stringdicts.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
