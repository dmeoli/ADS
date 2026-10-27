/*--------------------------------------------------------------------------*/
/*------------------------------ File Bench.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * What the benchmarks share: the median of r runs of a timed piece of work
 * after an untimed preparation, in nanoseconds per operation, a generator
 * of distinct random keys, and the output, one tab-separated line per
 * ( benchmark , case , n , structure , ns/op ), so that a script can
 * tabulate it. Each benchmark also runs its first structure twice, the
 * second time under the name "A/A", which measures the noise of the
 * machine: two runs of the same code differ by that much.
 *
 * Two variables of the environment pick the cells to measure, so that
 * each structure can run in a process of its own: with ADS_BENCH_WHO set
 * to a label only the cells of the structure of that label are measured,
 * with ADS_BENCH_LIST set none is, and the labels of the structures are
 * printed instead, once each, in the order of their first cell. *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

#ifndef __BENCH_H
 #define __BENCH_H

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace ADS::bench {

/// the median of r runs of work( prepare() ), in nanoseconds per one of the
/// ops operations of a run; prepare() is not timed
template< class P , class W >
double median( P && prepare , W && work , int r , std::size_t ops )
{
 std::vector< double > t;
 for( int i = 0 ; i < r ; ++i ) {
  auto s = prepare();
  const auto a = std::chrono::steady_clock::now();
  work( s );
  const auto b = std::chrono::steady_clock::now();
  t.push_back( std::chrono::duration< double , std::nano >( b - a ).count()
               / double( std::max< std::size_t >( ops , 1 ) ) );
  }
 std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
 return( t[ t.size() / 2 ] );
 }

/// n distinct random keys of 64 bits, from the seed
inline std::vector< std::uint64_t > keys( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::uint64_t > seen;
 std::vector< std::uint64_t > K;
 while( K.size() < n ) {
  const std::uint64_t k = g() >> 1;         // fits a signed key too
  if( seen.insert( k ).second ) K.push_back( k );
  }
 return( K );
 }

/// whether ADS_BENCH_LIST is set: the labels are printed, no cell is
/// measured
inline bool listing( void )
{
 static const bool list = [](){
  const char * l = std::getenv( "ADS_BENCH_LIST" );
  return( l && *l && std::strcmp( l , "0" ) ); }();
 return( list );
 }

/// whether the cells of the structure who are not to be measured: those
/// of a label other than that of ADS_BENCH_WHO, and all of them under
/// ADS_BENCH_LIST, which prints who the first time; a benchmark calls it
/// before any work of who, its preparation too
inline bool skip( const std::string & who )
{
 static const char * const only = [](){
  const char * w = std::getenv( "ADS_BENCH_WHO" );
  return( w && *w ? w : nullptr ); }();
 if( listing() ) {
  static std::unordered_set< std::string > seen;
  if( seen.insert( who ).second ) {
   std::printf( "%s\n" , who.c_str() );
   std::fflush( stdout );
   }
  return( true );
  }
 return( only && ( who != only ) );
 }

/// a line of the output, none for a structure that skip() leaves out
inline void print( const char * bench , const std::string & what ,
                   std::size_t n , const std::string & who , double ns )
{
 if( skip( who ) )
  return;
 // 4 decimals below 1 ns, where 2 would leave one or two digits
 std::printf( ns < 1 ? "%s\t%s\t%zu\t%s\t%.4f\n" : "%s\t%s\t%zu\t%s\t%.2f\n" ,
              bench , what.c_str() , n , who.c_str() , ns );
 std::fflush( stdout );
 }

/// the options -r runs and -s seed, and the sizes after them
struct Options {
 int runs = 5;
 unsigned seed = 1;
 std::vector< std::size_t > sizes;

 Options( int argc , char ** argv , std::vector< std::size_t > dflt ) {
  for( int i = 1 ; i < argc ; ++i ) {
   const std::string a = argv[ i ];
   if( ( a == "-r" ) && ( i + 1 < argc ) ) runs = std::atoi( argv[ ++i ] );
   else if( ( a == "-s" ) && ( i + 1 < argc ) )
    seed = unsigned( std::atoi( argv[ ++i ] ) );
   else sizes.push_back( std::size_t( std::atoll( a.c_str() ) ) );
   }
  if( sizes.empty() ) sizes = dflt;
  }
 };

/// keeps the compiler from dropping a result
inline volatile std::uint64_t sink = 0;

}  // end( namespace ADS::bench )

#endif  /* Bench.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Bench.h ----------------------------*/
/*--------------------------------------------------------------------------*/
