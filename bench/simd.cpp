/*--------------------------------------------------------------------------*/
/*----------------------------- File simd.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the vector kernels, old against new: one row per
 * kernel, each a loop of calls of the public function (or of the inline
 * function of `detail`) that runs it, on data fixed by a seed, so that the
 * same program built on the headers of two revisions of ADS (the kernels
 * on the intrinsics under the flags of the compiler, and those of Highway)
 * times the same work. `bench/simd.py` builds it on both and runs it: the
 * counts of the instructions by callgrind, which the load of the machine
 * does not change, and the times, which are taken only on an idle machine,
 * with the row A/A (the new build against itself) for the noise.
 *
 *   simd <kernel> <reps>     runs the kernel reps times, prints a checksum
 *   simd --time <kernel>     the nanoseconds of one call, median of 11
 *   simd --list              the kernels
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/FastBitwise.h"
#include "ADS/compression/BinaryPacking.h"
#include "ADS/compression/StreamVByte.h"
#include "ADS/dictionary/AdaptiveRadixTree.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/set/VectorIntersection.h"
#include "ADS/sort/VectorQuickSort.h"
#include "ADS/string/LongestCommonExtension.h"
#include "ADS/string/StringSearch.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// the checksum of the results, which keeps the calls alive
std::uint64_t sink = 0;

/// n integers of the generator of seed s below m
std::vector< std::uint32_t > draws( std::size_t n , std::uint32_t m ,
				    unsigned s )
{
 std::mt19937_64 g( s );
 std::vector< std::uint32_t > v( n );
 for( auto & x : v )
  x = std::uint32_t( g() % m );
 return( v );
 }

/// the sorted distinct integers of draws()
std::vector< std::uint32_t > sortedDraws( std::size_t n , std::uint32_t m ,
					  unsigned s )
{
 auto v = draws( n , m , s );
 std::sort( v.begin() , v.end() );
 v.erase( std::unique( v.begin() , v.end() ) , v.end() );
 return( v );
 }

/// a text of n bytes of an alphabet of a letters
std::string text( std::size_t n , unsigned a , unsigned s )
{
 std::mt19937_64 g( s );
 std::string t( n , 'a' );
 for( auto & c : t )
  c = char( 'a' + g() % a );
 return( t );
 }

/// the kernels: each prepares its data once and gives one call
std::map< std::string , std::function< void( void ) > > kernels( void )
{
 std::map< std::string , std::function< void( void ) > > K;
 {
  auto w = std::make_shared< std::vector< std::uint64_t > >( 1 << 14 );
  std::mt19937_64 g( 1 );
  for( auto & x : *w )
   x = g();
  K[ "popcount" ] = [ w ] { sink += ADS::popcount( w->data() , w->size() ); };
  K[ "popcount-and" ] = [ w ] {
   sink +=
     ADS::popcountAnd( w->data() , w->data() + w->size() / 2 , w->size() / 2 );
   };
  }
 {
  auto a = std::make_shared< std::vector< std::int32_t > >( 1 << 16 );
  std::mt19937_64 g( 2 );
  for( auto & x : *a )
   x = std::int32_t( g() );
  K[ "sort-i32" ] = [ a ] {
   auto b = *a;
   ADS::vectorQuickSort( b );
   sink += std::uint64_t( b[ b.size() / 2 ] );
   };
  auto d = std::make_shared< std::vector< double > >( 1 << 16 );
  for( auto & x : *d )
   x = double( std::int64_t( g() ) ) / 3.0;
  K[ "sort-f64" ] = [ d ] {
   auto b = *d;
   ADS::vectorQuickSort( b );
   sink += std::uint64_t( b[ 0 ] != 0 );
   };
  auto f = std::make_shared< std::vector< std::uint64_t > >( 1 << 16 );
  for( auto & x : *f )
   x = g() % 16;
  K[ "sort-u64-few" ] = [ f ] {
   auto b = *f;
   ADS::vectorQuickSort( b );
   sink += b.back();
   };
  }
 {
  auto a = std::make_shared< std::vector< std::uint32_t > >(
    sortedDraws( 1 << 15 , 1 << 17 , 3 ) );
  auto b = std::make_shared< std::vector< std::uint32_t > >(
    sortedDraws( 1 << 15 , 1 << 17 , 4 ) );
  auto s = std::make_shared< std::vector< std::uint32_t > >(
    sortedDraws( 1 << 9 , 1 << 17 , 5 ) );
  auto o = std::make_shared< std::vector< std::uint32_t > >( 1 << 16 );
  K[ "intersect-blocks" ] = [ a , b , o ] {
   sink +=
     ADS::intersect( a->data() , a->size() , b->data() , b->size() ,
		     o->data() );
   };
  K[ "intersect-gallop" ] = [ a , s , o ] {
   sink +=
     ADS::intersect( a->data() , a->size() , s->data() , s->size() ,
		     o->data() );
   };
  }
 {
  auto A = std::make_shared< ADS::BoolSet >( 1 << 18 );
  auto B = std::make_shared< ADS::BoolSet >( 1 << 18 );
  for( auto x : draws( 1 << 16 , 1 << 18 , 6 ) )
   A->insert( long( x ) );
  for( auto x : draws( 1 << 16 , 1 << 18 , 7 ) )
   B->insert( long( x ) );
  K[ "boolset-and" ] = [ A ,
			 B ] { sink += setIntersection( *A , *B ).size(); };
  }
 for( const auto & [ name , n ] :
      { std::pair< std::string , std::size_t >{ "dense" , 1 << 17 } ,
	{ "sparse" , 1 << 12 } } ) {
  auto A = std::make_shared< ADS::RoaringBitmap >();
  auto B = std::make_shared< ADS::RoaringBitmap >();
  for( auto x : draws( n , 1 << 20 , 8 ) )
   A->insert( x );
  for( auto x : draws( n , 1 << 20 , 9 ) )
   B->insert( x );
  K[ "roaring-and-" + name ] = [ A , B ] {
   sink += setIntersection( *A , *B ).size();
   };
  K[ "roaring-or-" + name ] = [ A ,
				B ] { sink += setUnion( *A , *B ).size(); };
  K[ "roaring-minus-" + name ] = [ A , B ] {
   sink += setDifference( *A , *B ).size();
   };
  }
 {
  auto T =
    std::make_shared< ADS::FlatHashTable< std::uint64_t , std::uint64_t > >();
  auto keys = draws( 1 << 16 , ~0u , 10 );
  for( auto k : keys )
   T->insert( k , k );
  auto Q = std::make_shared< std::vector< std::uint32_t > >(
    draws( 1 << 12 , ~0u , 11 ) );
  for( std::size_t i = 0 ; i < Q->size() ; i += 2 )
   ( *Q )[ i ] = keys[ i ];
  K[ "flat-find" ] = [ T , Q ] {
   for( auto q : *Q )
    sink += T->find( q ) != nullptr;
   };
  }
 {
  auto T = std::make_shared< ADS::AdaptiveRadixTree< int > >();
  std::vector< std::string > S;
  for( auto x : draws( 1 << 14 , ~0u , 12 ) )
   S.push_back( std::to_string( x ) );
  for( const auto & s : S )
   T->insert( s , 1 );
  auto Q = std::make_shared< std::vector< std::string > >( S );
  K[ "art-find" ] = [ T , Q ] {
   for( const auto & q : *Q )
    sink += T->find( q ) != nullptr;
   };
  }
 {
  auto keys = draws( 1 << 16 , ~0u , 13 );
  auto Q = std::make_shared< std::vector< std::uint32_t > >(
    draws( 1 << 12 , ~0u , 14 ) );
  auto F = std::make_shared< ADS::BlockedBloomFilter< std::uint32_t > >(
    keys.size() );
  auto P =
    std::make_shared< ADS::PrefixFilter< std::uint32_t > >( keys.size() );
  for( auto k : keys ) {
   F->insert( k );
   P->insert( k );
   }
  auto R = std::make_shared< ADS::RibbonFilter< std::uint32_t > >(
    std::vector< std::uint32_t >( keys.begin() , keys.end() ) );
  K[ "bloom-maybe" ] = [ F , Q ] {
   for( auto q : *Q )
    sink += F->maybe( q );
   };
  K[ "prefix-maybe" ] = [ P , Q ] {
   for( auto q : *Q )
    sink += P->maybe( q );
   };
  K[ "ribbon-maybe" ] = [ R , Q ] {
   for( auto q : *Q )
    sink += R->maybe( q );
   };
  }
 {
  auto t = std::make_shared< std::string >( text( 1 << 18 , 4 , 15 ) );
  auto L = std::make_shared< ADS::LceIndex< std::uint32_t > >( *t );
  auto Q = std::make_shared< std::vector< std::uint32_t > >(
    draws( 1 << 12 , std::uint32_t( t->size() ) , 16 ) );
  K[ "lce" ] = [ L , Q ] {
   for( std::size_t i = 0 ; i + 1 < Q->size() ; ++i )
    sink += L->lce( ( *Q )[ i ] , ( *Q )[ i + 1 ] );
   };
  auto S = std::make_shared< ADS::StringSearcher >( t->substr( 1000 , 12 ) );
  K[ "search-count" ] = [ S , t ] { sink += S->count( *t ); };
  K[ "hash-bytes" ] = [ t ] {
   sink += ADS::hashBytes( t->data() , t->size() );
   };
  }
 {
  auto x = std::make_shared< std::vector< std::uint32_t > >(
    draws( 1 << 16 , 1 << 20 , 17 ) );
  auto c = std::make_shared< std::vector< std::uint8_t > >(
    ADS::streamVByteEncode( *x ) );
  K[ "vbyte-encode" ] = [ x ] { sink += ADS::streamVByteEncode( *x ).size(); };
  K[ "vbyte-decode" ] = [ c , x ] {
   sink += ADS::streamVByteDecode( *c , x->size() )[ 7 ];
   };
  auto p =
    std::make_shared< std::vector< std::uint8_t > >( ADS::binaryPack( *x ) );
  K[ "bp-pack" ] = [ x ] { sink += ADS::binaryPack( *x ).size(); };
  K[ "bp-unpack" ] = [ p , x ] {
   sink += ADS::binaryUnpack( *p , x->size() )[ 7 ];
   };
  }
 {
  auto w = std::make_shared< std::vector< ADS::Word > >( 1 << 12 );
  std::mt19937_64 g( 18 );
  for( auto & x : *w )
   x = g();
  K[ "line-rank" ] = [ w ] {
   for( std::size_t i = 0 ; i + 8 <= w->size() ; i += 8 )
    sink += ADS::detail::lineRank< false >( w->data() + i ,
					    unsigned( i * 37 % 512 ) );
   };
  }
 return( K );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const auto K = kernels();
 if( ( argc == 2 ) && ( std::string( argv[ 1 ] ) == "--list" ) ) {
  for( const auto & [ k , f ] : K )
   std::cout << k << '\n';
  return( 0 );
  }
 if( ( argc == 3 ) && ( std::string( argv[ 1 ] ) == "--time" ) ) {
  const auto & f = K.at( argv[ 2 ] );
  std::vector< double > t;
  for( int r = 0 ; r < 11 ; ++r ) {
   const auto a = std::chrono::steady_clock::now();
   for( int i = 0 ; i < 10 ; ++i )
    f();
   const auto b = std::chrono::steady_clock::now();
   t.push_back( std::chrono::duration< double , std::nano >( b - a ).count() /
		10 );
   }
  std::sort( t.begin() , t.end() );
  std::cout << argv[ 2 ] << ' ' << t[ 5 ] << ' ' << ( sink & 1 ) << '\n';
  return( 0 );
  }
 if( argc != 3 ) {
  std::cerr << "simd <kernel> <reps> | --time <kernel> | --list\n";
  return( 1 );
  }
 const auto & f = K.at( argv[ 1 ] );
 for( long r = std::atol( argv[ 2 ] ) ; r > 0 ; --r )
  f();
 std::cout << sink << '\n';
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File simd.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
