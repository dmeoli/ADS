/*--------------------------------------------------------------------------*/
/*----------------------------- File tries.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the dictionaries of strings: the tries of ADS against
 * std::map and std::unordered_map of std::string and, where they are
 * found, absl::btree_map, the HAT-trie of tsl::htrie_map and the adaptive
 * radix tree of libart, on two sets of n distinct keys: "urls", the
 * addresses of pages (a scheme, one of 1000 hosts, 1 to 4 words of a
 * vocabulary of 5000 and a number), whose keys share long prefixes, and
 * "words", random lower-case words of 4 to 16 letters, which share short
 * ones. The cases are the n insertions in random order, n lookups of
 * present keys in random order, n lookups of absent keys, the visit of all
 * the pairs (in the order of the keys, but for the hash table and the
 * HAT-trie), and n / 100 enumerations of the keys that begin with half of
 * a present key (the hash table has none).
 *
 * Each cell is the median of r runs, in nanoseconds per operation (per key
 * found for the enumerations); the lookups run on one structure built
 * once, and the contracts are off (ADS_CONTRACTS=0). ONLY=a|b|... runs
 * only the structures whose name has a or b ... in it.
 *
 *   tries [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/dictionary/AdaptiveRadixTree.h"
#include "ADS/dictionary/Trie.h"

#if __has_include( <absl/container/btree_map.h> )
 #include <absl/container/btree_map.h>
 #define ADS_HAS_ABSL 1
#endif

#if __has_include( <tsl/htrie_map.h> )
 #include <tsl/htrie_map.h>
 #define ADS_HAS_TSL 1
#endif

#if __has_include( <libart/art.h> )
extern "C" {
 #include <libart/art.h>
}
 #define ADS_HAS_LIBART 1
#endif

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

using namespace ADS::bench;
using Value = std::uint64_t;

namespace {

/// whether to run the structure who: all, or those whose name has in it
/// one of the texts separated by | in the variable ONLY, and of these
/// those that skip() keeps
bool wanted( const std::string & who )
{
 const char * o = std::getenv( "ONLY" );
 if( ! o || ! *o )
  return( ! skip( who ) );
 const std::string s = o;
 for( std::size_t a = 0 , b ; a <= s.size() ; a = b + 1 ) {
  b = std::min( s.find( '|' , a ) , s.size() );
  if( ( b > a ) && ( who.find( s.substr( a , b - a ) ) !=
                     std::string::npos ) )
   return( ! skip( who ) );
  }
 return( false );
 }

/// a missing operation of a structure
struct None {};

/// a random lower-case word of a to b letters
std::string word( std::mt19937_64 & g , int a , int b )
{
 std::string w( std::size_t( a + int( g() % std::uint64_t( b - a + 1 ) ) ) ,
                'a' );
 for( auto & c : w ) c = char( 'a' + g() % 26 );
 return( w );
 }

/// 2 n distinct keys of the set s, the first n to insert and the others
/// absent
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

/// the cases on the structure that make() builds empty, by ins( d , k , v
/// ), find( d , k ) (the value, or 0), sum( d ) (the sum of the values of
/// all the pairs) and pre( d , p ) (the number of the keys with prefix p)
template< class M , class I , class F , class S , class P >
void run( const std::string & who , const char * set ,
          const std::vector< std::string > & K ,
          const std::vector< std::string > & H ,
          const std::vector< std::string > & Q ,
          const std::vector< std::string > & R , int r , M && make ,
          I && ins , F && find , S && sum , P && pre )
{
 if( ! wanted( who ) )
  return;
 const std::size_t n = K.size();
 const std::string s = set;
 print( "tries" , s + " insert" , n , who , median( make ,
         [ & ]( auto & d ) {
          for( std::size_t i = 0 ; i < n ; ++i ) ins( d , K[ i ] , i + 1 ); } ,
         r , n ) );
 auto d = make();
 for( std::size_t i = 0 ; i < n ; ++i ) ins( d , K[ i ] , i + 1 );
 print( "tries" , s + " lookup hit" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          Value t = 0;
          for( const auto & k : H ) t += find( d , k );
          sink = sink + t; } , r , n ) );
 print( "tries" , s + " lookup miss" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          Value t = 0;
          for( const auto & k : Q ) t += find( d , k );
          sink = sink + t; } , r , n ) );
 print( "tries" , s + " visit" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { sink = sink + sum( d ); } , r , n ) );
 if constexpr( ! std::is_same_v< std::decay_t< P > , None > ) {
  std::size_t found = 0;
  for( const auto & p : R ) found += pre( d , p );
  print( "tries" , s + " prefix" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::size_t t = 0;
           for( const auto & p : R ) t += pre( d , p );
           sink = sink + t; } , r , std::max< std::size_t >( found , 1 ) ) );
  }
 }

/// the sum of the values of a map
template< class D >
Value total( const D & d )
{
 Value t = 0;
 for( const auto & [ k , v ] : d ) t += v;
 return( t );
 }

/// the keys of an ordered map with prefix p
template< class D >
std::size_t prefixed( const D & d , const std::string & p )
{
 std::size_t c = 0;
 for( auto it = d.lower_bound( p ) ; ( it != d.end() ) &&
      ( std::string_view( it->first ).substr( 0 , p.size() ) == p ) ; ++it )
  ++c;
 return( c );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes )
  for( int set = 0 ; set < 2 ; ++set ) {
   const char * name = set == 0 ? "urls" : "words";
   const auto all = strings( set , n , o.seed );
   const std::vector< std::string > K( all.begin() , all.begin() + long( n ) ) ,
                                    Q( all.begin() + long( n ) , all.end() );
   std::vector< std::string > H = K , R;
   std::shuffle( H.begin() , H.end() , std::mt19937_64( o.seed + 1 ) );
   for( std::size_t i = 0 ; i < n / 100 ; ++i )
    R.push_back( H[ i ].substr( 0 , H[ i ].size() / 2 ) );
   const int r = o.runs;
   auto stdIns = []( auto & d , const std::string & k , Value v ) {
    d.emplace( k , v ); };
   auto stdFind = []( const auto & d , const std::string & k ) {
    const auto it = d.find( k );
    return( it == d.end() ? Value( 0 ) : Value( it->second ) ); };
   auto stdSum = []( const auto & d ) { return( total( d ) ); };
   auto stdPre = []( const auto & d , const std::string & p ) {
    return( prefixed( d , p ) ); };
   auto adsIns = []( auto & d , const std::string & k , Value v ) {
    d.insert( k , v ); };
   auto adsFind = []( const auto & d , const std::string & k ) {
    const auto v = d.lookup( k );
    return( v ? *v : Value( 0 ) ); };
   auto adsSum = []( const auto & d ) {
    Value t = 0;
    for( const auto & [ k , v ] : d ) t += v;
    return( t ); };
   auto adsPre = []( const auto & d , const std::string & p ) {
    return( d.keysWithPrefix( p ).size() ); };

   for( const char * who : { "std::map" , "std::map A/A" } )
    run( who , name , K , H , Q , R , r ,
         [](){ return( std::map< std::string , Value >() ); } , stdIns ,
         stdFind , stdSum , stdPre );
   run( "std::unordered_map" , name , K , H , Q , R , r ,
        [](){ return( std::unordered_map< std::string , Value >() ); } ,
        stdIns , stdFind , stdSum , None() );
#ifdef ADS_HAS_ABSL
   run( "absl::btree_map" , name , K , H , Q , R , r ,
        [](){ return( absl::btree_map< std::string , Value >() ); } ,
        stdIns , stdFind , stdSum , stdPre );
#endif
#ifdef ADS_HAS_TSL
   run( "tsl::htrie_map" , name , K , H , Q , R , r ,
        [](){ return( tsl::htrie_map< char , Value >() ); } ,
        []( auto & d , const std::string & k , Value v ) {
         d.insert( k , v ); } ,
        []( const auto & d , const std::string & k ) {
         const auto it = d.find( k );
         return( it == d.end() ? Value( 0 ) : Value( it.value() ) ); } ,
        []( const auto & d ) {
         Value t = 0;
         for( auto it = d.begin() ; it != d.end() ; ++it ) t += it.value();
         return( t ); } ,
        []( const auto & d , const std::string & p ) {
         const auto [ a , b ] = d.equal_prefix_range( p );
         std::size_t c = 0;
         for( auto it = a ; it != b ; ++it ) ++c;
         return( c ); } );
#endif
#ifdef ADS_HAS_LIBART
   {
    struct Art {                            // the tree and its release
     art_tree t;
     Art( void ) { art_tree_init( &t ); }
     ~Art() { art_tree_destroy( &t ); }
     };
    // the keys with their final 0, so that none is a prefix of another
    auto bytes = []( const std::string & k ) {
     return( reinterpret_cast< const unsigned char * >( k.c_str() ) ); };
    run( "libart" , name , K , H , Q , R , r ,
         [](){ return( std::make_unique< Art >() ); } ,
         [ & ]( auto & d , const std::string & k , Value v ) {
          art_insert( &d->t , bytes( k ) , int( k.size() + 1 ) ,
                      reinterpret_cast< void * >( v ) ); } ,
         [ & ]( const auto & d , const std::string & k ) {
          return( Value( reinterpret_cast< std::uintptr_t >(
                   art_search( &d->t , bytes( k ) ,
                               int( k.size() + 1 ) ) ) ) ); } ,
         []( const auto & d ) {
          Value t = 0;
          art_iter( &d->t , []( void * s , const unsigned char * ,
                                std::uint32_t , void * v ) {
                     *static_cast< Value * >( s ) +=
                      Value( reinterpret_cast< std::uintptr_t >( v ) );
                     return( 0 ); } , &t );
          return( t ); } ,
         [ & ]( const auto & d , const std::string & p ) {
          std::size_t c = 0;
          art_iter_prefix( &d->t , bytes( p ) , int( p.size() ) ,
                           []( void * s , const unsigned char * ,
                               std::uint32_t , void * ) {
                            ++*static_cast< std::size_t * >( s );
                            return( 0 ); } , &c );
          return( c ); } );
    }
#endif
   run( "ADS::AdaptiveRadixTree" , name , K , H , Q , R , r ,
        [](){ return( ADS::AdaptiveRadixTree< Value >() ); } , adsIns ,
        adsFind , adsSum , []( const auto & d , const std::string & p ) {
         std::size_t c = 0;
         d.forPrefix( p , [ & ]( std::string_view , const Value & ) {
                       ++c; } );
         return( c ); } );
   run( "ADS::Trie" , name , K , H , Q , R , r ,
        [](){ return( ADS::Trie< Value >() ); } , adsIns , adsFind , adsSum ,
        adsPre );
   run( "ADS::RadixTree" , name , K , H , Q , R , r ,
        [](){ return( ADS::RadixTree< Value >() ); } , adsIns , adsFind ,
        adsSum , adsPre );
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File tries.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
