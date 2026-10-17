/*--------------------------------------------------------------------------*/
/*--------------------------- File sequences.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sequences through the standard interface: the
 * sequences of ADS against the containers of std with the same structure
 * (ArraySequence against std::vector, List against std::list, SinglyList
 * against std::forward_list), on n integers. The cases are n insertions at
 * the end (at the front for the singly linked lists), a visit that sums
 * the items, std::sort or the reversal in place by the iterators, and a
 * visit by the positions of the book where it applies.
 *
 * Each cell is the median of r runs, in nanoseconds per item; the contracts
 * are off (ADS_CONTRACTS=0).
 *
 *   sequences [ -r runs ] [ -s seed ] n1 n2 ... *
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

#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/SinglyList.h"

#include <forward_list>
#include <list>
#include <numeric>

using namespace ADS::bench;

namespace {

/// the cases on the sequence that make() builds and add( s , x ) fills;
/// reorder( s ) sorts or reverses it in place
template< class M , class A , class R >
void run( const std::string & who , const std::vector< int > & X , int r ,
          M && make , A && add , const char * what , R && reorder )
{
 const std::size_t n = X.size();
 auto fill = [ & ]( void ) {
  auto s = make();
  for( int x : X ) add( s , x );
  return( s );
  };
 print( "sequences" , "fill" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { auto s = fill(); sink = sink + *s.begin(); } , r ,
         n ) );
 const auto s = fill();
 print( "sequences" , "visit" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          sink = sink + std::uint64_t( std::accumulate( s.begin() , s.end() ,
                                                        0L ) ); } , r , n ) );
 print( "sequences" , what , n , who , median( fill , [ & ]( auto & t ) {
         reorder( t );
         sink = sink + *t.begin(); } , r , n ) );
 }

/// the visit by the positions of the book
template< class S >
void positions( const std::string & who , const S & s , std::size_t n ,
                int r )
{
 print( "sequences" , "visit by positions" , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          long t = 0;
          for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
           t += s.read( p );
          sink = sink + std::uint64_t( t ); } , r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 10000000 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  std::vector< int > X( n );
  for( auto & x : X ) x = int( g() % 1000000 );
  const int r = o.runs;
  auto back = []( auto & s , int x ) { s.push_back( x ); };
  auto front = []( auto & s , int x ) { s.push_front( x ); };
  auto sort = []( auto & s ) { std::sort( s.begin() , s.end() ); };
  auto reverse = []( auto & s ) { std::reverse( s.begin() , s.end() ); };
  for( const char * who : { "std::vector" , "std::vector A/A" } )
   run( who , X , r , [](){ return( std::vector< int >() ); } , back ,
        "std::sort" , sort );
  run( "ADS::ArraySequence" , X , r , [](){
       return( ADS::ArraySequence< int >() ); } , back , "std::sort" , sort );
  run( "std::list" , X , r , [](){ return( std::list< int >() ); } , back ,
       "reverse" , reverse );
  run( "ADS::List" , X , r , [](){ return( ADS::List< int >() ); } , back ,
       "reverse" , reverse );
  run( "std::forward_list" , X , r , [](){
       return( std::forward_list< int >() ); } , front , "visit again" ,
       []( auto & s ) { sink = sink + std::uint64_t( std::accumulate(
                         s.begin() , s.end() , 0L ) ); } );
  run( "ADS::SinglyList" , X , r , [](){
       return( ADS::SinglyList< int >() ); } , front , "visit again" ,
       []( auto & s ) { sink = sink + std::uint64_t( std::accumulate(
                         s.begin() , s.end() , 0L ) ); } );
  ADS::ArraySequence< int > A( X.begin() , X.end() );
  ADS::List< int > L( X.begin() , X.end() );
  positions( "ADS::ArraySequence" , A , n , r );
  positions( "ADS::List" , L , n , r );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File sequences.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
