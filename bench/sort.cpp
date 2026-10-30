/*--------------------------------------------------------------------------*/
/*----------------------------- File sort.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sorts: each sort of ADS against std::sort,
 * std::stable_sort and, if Boost.Sort is found, boost::sort::pdqsort,
 * spreadsort, spinsort and flat_stable_sort; if their headers are found,
 * the sequential IPS4o and IPS2Ra, x86-simd-sort, ska_sort, fluxsort,
 * crumsort and quadsort, and vqsort of Highway if ADS_HAS_VQSORT is
 * defined (it needs its library); on integers of 32 and 64 bits,
 * doubles, strings and records with an order of their own, each in the
 * patterns random, sorted, reversed, few distinct keys, organ pipe and 16
 * sorted runs, for the sizes given on the command line.
 *
 * Each cell is the median of r runs of the time to sort a fresh copy of the
 * same input, in nanoseconds per item, the copy excluded; the contracts are
 * off (ADS_CONTRACTS=0). The output is one line per ( type , pattern , n ,
 * sort ), tab separated, so that a script can tabulate it; the row
 * "std::sort A/A" is std::sort run a second time, the noise of the machine,
 * and a sort that leaves a copy out of order gets -1. The records are also
 * sorted by their key, by IPS2Ra, ska_sort and radixSortBy, marked "(key)",
 * which do not compare with the order of the records.
 *
 *   sort [ -r runs ] [ -s seed ] n1 n2 ...
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

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/LearnedSort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/ShellSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/StringSort.h"
#include "ADS/sort/VectorQuickSort.h"

#if __has_include( <boost/sort/sort.hpp> )
 #include <boost/sort/sort.hpp>
 #define ADS_HAS_BOOST_SORT 1
#endif

#if __has_include( <ips4o.hpp> )
 #include <ips4o.hpp>
 #define ADS_HAS_IPS4O 1
#endif

#if __has_include( <ips2ra.hpp> )
 #include <ips2ra.hpp>
 #define ADS_HAS_IPS2RA 1
#endif

#if __has_include( <x86simdsort-static-incl.h> ) && defined( __AVX2__ )
 #include <x86simdsort-static-incl.h>
 #define ADS_HAS_XSS 1
#endif

#if __has_include( <ska_sort.hpp> )
 #include <ska_sort.hpp>
 #define ADS_HAS_SKA 1
#endif

#if __has_include( <fluxsort.h> ) && __has_include( <crumsort.h> )
 #include <cassert>
 #include <cerrno>
 #include <cfloat>
 #include <cstdio>
 #include <cstring>
 #include <stdalign.h>
 #define restrict __restrict
 // their generic entries read the items of 8 bytes as long long and pass
 // them to the comparison as doubles, which the strict aliasing of g++ at
 // -O3 reorders: a vector of doubles left out of order
 #if defined( __GNUC__ ) && ! defined( __clang__ )
  #pragma GCC push_options
  #pragma GCC optimize( "no-strict-aliasing" )
 #endif
namespace scandum {
 #include <fluxsort.h>
 #include <crumsort.h>
}
 #if defined( __GNUC__ ) && ! defined( __clang__ )
  #pragma GCC pop_options
 #endif
 #undef restrict
 #define ADS_HAS_SCANDUM 1
#endif

#ifdef ADS_HAS_VQSORT
 #include <hwy/contrib/sort/vqsort.h>
#endif

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- INPUTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// a record of 32 bytes sorted by its key
struct Record {
 std::uint64_t key;
 std::uint64_t pad[ 3 ];
 };

struct ByKey {
 bool operator()( const Record & a , const Record & b ) const {
  return( a.key < b.key );
  }
 };

const char * patterns[] = { "random" , "sorted" , "reversed" , "few" ,
                            "organ" , "runs16" };

/// the key of rank i of n in the pattern p, from the generator g
std::uint64_t rank( int p , std::size_t i , std::size_t n ,
                    std::mt19937_64 & g )
{
 switch( p ) {
  case( 0 ): return( g() );
  case( 1 ): return( i );
  case( 2 ): return( n - i );
  case( 3 ): return( g() % 16 );
  case( 4 ): return( std::min( i , n - 1 - i ) );
  default: return( ( i % ( n / 16 + 1 ) ) * 16 + i / ( n / 16 + 1 ) );
  }
 }

template< class T >
T make( std::uint64_t k );

template<> std::uint32_t make( std::uint64_t k ) {
 return( std::uint32_t( k ) );
 }
template<> std::int64_t make( std::uint64_t k ) {
 return( std::int64_t( k ) );
 }
template<> double make( std::uint64_t k ) {
 return( double( std::int64_t( k ) ) / 1e3 );
 }
template<> std::string make( std::uint64_t k ) {
 std::string s = "item";                    // a common prefix of 4
 for( int d = 0 ; d < 12 ; ++d , k /= 26 ) s += char( 'a' + k % 26 );
 return( s );
 }
template<> Record make( std::uint64_t k ) {
 return( Record{ k , { 0 , 0 , 0 } } );
 }

/// the order of the bits of T as an unsigned key, for the radix sorts
/// that take one
template< class T >
auto bitsKey( const T & x )
{
 if constexpr( std::is_same_v< T , Record > )
  return( x.key );
 else if constexpr( std::is_unsigned_v< T > )
  return( x );
 else if constexpr( std::is_integral_v< T > )
  return( std::uint64_t( x ) ^ ( std::uint64_t( 1 ) << 63 ) );
 else {
  const auto b = std::bit_cast< std::uint64_t >( x );
  return( b ^ ( ( b >> 63 ) ? ~std::uint64_t( 0 ) :
                              std::uint64_t( 1 ) << 63 ) );
  }
 }

#ifdef ADS_HAS_SCANDUM
/// the comparison of the generic entries of fluxsort and crumsort
template< class T , class C >
int scandumCmp( const void * a , const void * b )
{
 return( C()( *static_cast< const T * >( b ) ,
              *static_cast< const T * >( a ) ) );
 }

/// the same through the pointers to the items, for the entries _size,
/// which sort an array of pointers
template< class T , class C >
int scandumCmpRef( const void * a , const void * b )
{
 return( C()( **static_cast< const T * const * >( b ) ,
              **static_cast< const T * const * >( a ) ) );
 }
#endif

/// the median of r times, in nanoseconds per item, of sort on copies of A,
/// or -1 if the sort left a copy out of order by less (checked untimed)
template< class T , class F , class C >
double timeIt( const std::vector< T > & A , F && sort , int r , C less )
{
 std::vector< double > t;
 for( int i = 0 ; i < r ; ++i ) {
  std::vector< T > B = A;
  const auto a = std::chrono::steady_clock::now();
  sort( B );
  const auto b = std::chrono::steady_clock::now();
  if( ! std::is_sorted( B.begin() , B.end() , less ) )
   return( -1 );
  t.push_back( std::chrono::duration< double , std::nano >( b - a ).count()
               / double( std::max< std::size_t >( A.size() , 1 ) ) );
  }
 std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
 return( t[ t.size() / 2 ] );
 }

/// the header of the table, before its first row, so that a run that
/// measures no sort prints none
void header( void )
{
 static bool done = false;
 if( ! std::exchange( done , true ) )
  std::cout << "type\tpattern\tn\tsort\tns_per_item" << std::endl;
 }

/// the sorts of the battery for T, with their names
template< class T , class C >
void run( const char * type , const std::vector< std::size_t > & sizes ,
          int r , unsigned seed , C less )
{
 using V = std::vector< T >;
 std::vector< std::pair< std::string , std::function< void( V & ) > > > S = {
  { "std::sort" , [ & ]( V & A ) {
   std::sort( A.begin() , A.end() , less ); } } ,
  { "std::sort A/A" , [ & ]( V & A ) {
   std::sort( A.begin() , A.end() , less ); } } ,
  { "std::stable_sort" , [ & ]( V & A ) {
   std::stable_sort( A.begin() , A.end() , less ); } } ,
  { "ADS::sort" , [ & ]( V & A ) { ADS::sort( A , less ); } } ,
  { "ADS::stableSort" , [ & ]( V & A ) { ADS::stableSort( A , less ); } } ,
  { "pdqSort" , [ & ]( V & A ) { ADS::pdqSort( A , less ); } } ,
  { "sampleSort" , [ & ]( V & A ) { ADS::sampleSort( A , less ); } } ,
  { "dualPivotQuickSort" , [ & ]( V & A ) {
   ADS::dualPivotQuickSort( A , less ); } } ,
  { "threeWayQuickSort" , [ & ]( V & A ) {
   ADS::threeWayQuickSort( A , less ); } } ,
  { "quickSortTuned" , [ & ]( V & A ) {
   ADS::quickSortTuned( A , 16 , less ); } } ,
  { "powerSort" , [ & ]( V & A ) { ADS::powerSort( A , less ); } } ,
  { "mergeSort" , [ & ]( V & A ) { ADS::mergeSort( A , 16 , less ); } } ,
  { "heapsort" , [ & ]( V & A ) { ADS::heapsort( A , less ); } } ,
  { "shellSort" , [ & ]( V & A ) {
   ADS::shellSort( A , ADS::ciuraGaps( A.size() ) , less ); } } ,
#ifdef ADS_HAS_BOOST_SORT
  { "boost::pdqsort" , [ & ]( V & A ) {
   boost::sort::pdqsort( A.begin() , A.end() , less ); } } ,
  { "boost::spinsort" , [ & ]( V & A ) {
   boost::sort::spinsort( A.begin() , A.end() , less ); } } ,
  { "boost::flat_stable_sort" , [ & ]( V & A ) {
   boost::sort::flat_stable_sort( A.begin() , A.end() , less ); } } ,
#endif
  };
#ifdef ADS_HAS_IPS4O
 S.emplace_back( "ips4o" , [ & ]( V & A ) {
  ips4o::sort( A.begin() , A.end() , less ); } );
#endif
#ifdef ADS_HAS_IPS2RA
 if constexpr( ! std::is_same_v< T , std::string > )        // the records
  S.emplace_back( std::is_same_v< T , Record > ? "ips2ra (key)" : "ips2ra" ,
                  []( V & A ) {                 // by a key, as radixSortBy
   ips2ra::sort( A.begin() , A.end() , []( const T & x ) {
    return( bitsKey( x ) ); } ); } );
#endif
 if constexpr( std::is_same_v< T , Record > )
  S.emplace_back( "ADS::radixSortBy (key)" , []( V & A ) {
   ADS::radixSortBy( A , []( const Record & x ) { return( x.key ); } ); } );
#ifdef ADS_HAS_SKA
 if constexpr( std::is_same_v< T , Record > )
  S.emplace_back( "ska_sort (key)" , []( V & A ) {
   ska_sort( A.begin() , A.end() , []( const Record & x ) {
    return( x.key ); } ); } );
 else
  S.emplace_back( "ska_sort" , []( V & A ) {
   ska_sort( A.begin() , A.end() ); } );
#endif
#ifdef ADS_HAS_SCANDUM
 if constexpr( std::is_integral_v< T > ) {
  constexpr std::size_t z = sizeof( T ) + std::is_unsigned_v< T >;
  S.emplace_back( "crumsort" , []( V & A ) {
   scandum::crumsort_prim( A.data() , A.size() , z ); } );
  S.emplace_back( "fluxsort" , []( V & A ) {
   scandum::fluxsort_prim( A.data() , A.size() , z ); } );
  S.emplace_back( "quadsort" , []( V & A ) {
   scandum::quadsort_prim( A.data() , A.size() , z ); } );
  }
 else if constexpr( std::is_floating_point_v< T > ) {
  S.emplace_back( "crumsort" , []( V & A ) {
   scandum::crumsort( A.data() , A.size() , sizeof( T ) ,
                      scandumCmp< T , C > ); } );
  S.emplace_back( "fluxsort" , []( V & A ) {
   scandum::fluxsort( A.data() , A.size() , sizeof( T ) ,
                      scandumCmp< T , C > ); } );
  S.emplace_back( "quadsort" , []( V & A ) {
   scandum::quadsort( A.data() , A.size() , sizeof( T ) ,
                      scandumCmp< T , C > ); } );
  }
 else if constexpr( std::is_same_v< T , Record > ) {
  S.emplace_back( "fluxsort" , []( V & A ) {
   scandum::fluxsort_size( A.data() , A.size() , sizeof( T ) ,
                           scandumCmpRef< T , C > ); } );
  S.emplace_back( "quadsort" , []( V & A ) {
   scandum::quadsort_size( A.data() , A.size() , sizeof( T ) ,
                           scandumCmpRef< T , C > ); } );
  }
#endif
 if constexpr( ADS::RadixKey< T > ) {
#ifdef ADS_HAS_XSS
  S.emplace_back( "x86-simd-sort" , []( V & A ) {
   x86simdsortStatic::qsort( A.data() , A.size() ); } );
#endif
#ifdef ADS_HAS_VQSORT
  S.emplace_back( "vqsort" , []( V & A ) {
   hwy::VQSort( A.data() , A.size() , hwy::SortAscending() ); } );
#endif
  if constexpr( ADS::VectorKey< T > ) {
   S.emplace_back( "vectorQuickSort" , []( V & A ) {
    ADS::vectorQuickSort( A ); } );
   S.emplace_back( "learnedSort" , []( V & A ) { ADS::learnedSort( A ); } );
   }
  S.emplace_back( "radixSort" , []( V & A ) { ADS::radixSort( A ); } );
  S.emplace_back( "americanFlagSort" , []( V & A ) {
   ADS::americanFlagSort( A ); } );
#ifdef ADS_HAS_BOOST_SORT
  if constexpr( std::is_integral_v< T > )
   S.emplace_back( "boost::spreadsort" , []( V & A ) {
    boost::sort::spreadsort::integer_sort( A.begin() , A.end() ); } );
  else
   S.emplace_back( "boost::spreadsort" , []( V & A ) {
    boost::sort::spreadsort::float_sort( A.begin() , A.end() ); } );
#endif
  }
 if constexpr( std::is_same_v< T , std::string > ) {
  S.emplace_back( "multikeyQuickSort" , []( V & A ) {
   ADS::multikeyQuickSort( A ); } );
  S.emplace_back( "msdRadixSort" , []( V & A ) { ADS::msdRadixSort( A ); } );
  S.emplace_back( "prefixKeySort" , []( V & A ) {
   ADS::prefixKeySort( A ); } );
#ifdef ADS_HAS_BOOST_SORT
  S.emplace_back( "boost::spreadsort" , []( V & A ) {
   boost::sort::spreadsort::string_sort( A.begin() , A.end() ); } );
#endif
  }
 for( std::size_t n : sizes )
  for( int p = 0 ; p < 6 ; ++p ) {
   std::mt19937_64 g( seed );
   V A( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    A[ i ] = make< T >( rank( p , i , n , g ) );
   for( auto & [ name , sort ] : S ) {
    if( ( n > 200000 ) && ( name == "shellSort" ) )
     continue;                              // too slow to matter
    if( ADS::bench::skip( name ) )
     continue;
    header();
    std::cout << type << '\t' << patterns[ p ] << '\t' << n << '\t'
              << name << '\t' << timeIt( A , sort , r , less ) << std::endl;
    }
   }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 int r = 5;
 unsigned seed = 1;
 std::vector< std::size_t > sizes;
 for( int i = 1 ; i < argc ; ++i ) {
  const std::string a = argv[ i ];
  if( ( a == "-r" ) && ( i + 1 < argc ) ) r = std::atoi( argv[ ++i ] );
  else if( ( a == "-s" ) && ( i + 1 < argc ) )
   seed = unsigned( std::atoi( argv[ ++i ] ) );
  else sizes.push_back( std::size_t( std::atoll( a.c_str() ) ) );
  }
 if( sizes.empty() )
  sizes = { 1000 , 100000 };
 run< std::uint32_t >( "uint32" , sizes , r , seed , std::less<>() );
 run< std::int64_t >( "int64" , sizes , r , seed , std::less<>() );
 run< double >( "double" , sizes , r , seed , std::less<>() );
 run< std::string >( "string" , sizes , r , seed , std::less<>() );
 run< Record >( "record" , sizes , r , seed , ByKey() );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File sort.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
