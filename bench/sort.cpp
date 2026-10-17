/*--------------------------------------------------------------------------*/
/*----------------------------- File sort.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sorts: each sort of ADS against std::sort,
 * std::stable_sort and, if Boost.Sort is found, boost::sort::pdqsort,
 * spreadsort, spinsort and flat_stable_sort, on integers of 32 and 64 bits,
 * doubles, strings and records with an order of their own, each in the
 * patterns random, sorted, reversed, few distinct keys, organ pipe and 16
 * sorted runs, for the sizes given on the command line.
 *
 * Each cell is the median of r runs of the time to sort a fresh copy of the
 * same input, in nanoseconds per item, the copy excluded; the contracts are
 * off (ADS_CONTRACTS=0). The output is one line per ( type , pattern , n ,
 * sort ), tab separated, so that a script can tabulate it.
 *
 *   sort [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/PatienceSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/ShellSort.h"
#include "ADS/sort/Smoothsort.h"
#include "ADS/sort/Sort.h"

#if __has_include( <boost/sort/sort.hpp> )
 #include <boost/sort/sort.hpp>
 #define ADS_HAS_BOOST_SORT 1
#endif

#include <algorithm>
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

/// the median of r times, in nanoseconds per item, of sort on copies of A
template< class T , class F >
double timeIt( const std::vector< T > & A , F && sort , int r )
{
 std::vector< double > t;
 for( int i = 0 ; i < r ; ++i ) {
  std::vector< T > B = A;
  const auto a = std::chrono::steady_clock::now();
  sort( B );
  const auto b = std::chrono::steady_clock::now();
  t.push_back( std::chrono::duration< double , std::nano >( b - a ).count()
               / double( std::max< std::size_t >( A.size() , 1 ) ) );
  }
 std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
 return( t[ t.size() / 2 ] );
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
  { "std::stable_sort" , [ & ]( V & A ) {
   std::stable_sort( A.begin() , A.end() , less ); } } ,
  { "ADS::sort" , [ & ]( V & A ) { ADS::sort( A , less ); } } ,
  { "ADS::stableSort" , [ & ]( V & A ) { ADS::stableSort( A , less ); } } ,
  { "pdqSort" , [ & ]( V & A ) { ADS::pdqSort( A , less ); } } ,
  { "introSort" , [ & ]( V & A ) { ADS::introSort( A , less ); } } ,
  { "dualPivotQuickSort" , [ & ]( V & A ) {
   ADS::dualPivotQuickSort( A , less ); } } ,
  { "threeWayQuickSort" , [ & ]( V & A ) {
   ADS::threeWayQuickSort( A , less ); } } ,
  { "quickSortTuned" , [ & ]( V & A ) {
   ADS::quickSortTuned( A , 16 , less ); } } ,
  { "powerSort" , [ & ]( V & A ) { ADS::powerSort( A , less ); } } ,
  { "timSort" , [ & ]( V & A ) { ADS::timSort( A , less ); } } ,
  { "mergeSort" , [ & ]( V & A ) { ADS::mergeSort( A , 16 , less ); } } ,
  { "heapsort" , [ & ]( V & A ) { ADS::heapsort( A , less ); } } ,
  { "smoothsort" , [ & ]( V & A ) { ADS::smoothsort( A , less ); } } ,
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
 if constexpr( ADS::RadixKey< T > ) {
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
    std::cout << type << '\t' << patterns[ p ] << '\t' << n << '\t'
              << name << '\t' << timeIt( A , sort , r ) << std::endl;
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
 std::cout << "type\tpattern\tn\tsort\tns_per_item" << std::endl;
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
