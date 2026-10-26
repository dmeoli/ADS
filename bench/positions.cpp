/*--------------------------------------------------------------------------*/
/*--------------------------- File positions.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the indices of the positions of objects in a vector,
 * from their addresses to positions of 32 bits: PositionIndex of ADS
 * against std::unordered_map, ADS::FlatHashTable and, if Boost is found,
 * boost::unordered_flat_map, each with its default hash, on a vector of n
 * objects of 32 bytes:
 *
 * - build: the index of the vector, from empty, with no reserve;
 * - lookup: the positions of n objects drawn at random;
 * - renumber: the erasure from the vector of a random tenth of the
 *   positions, given sorted: the keys of the positions erased go away and
 *   the others move to their new positions; ADS by renumber(), the others
 *   by erasing those keys and rewriting each position by a table from the
 *   old positions to the new ones (built in the clock);
 * - renumber-one: the same for the erasure of one position in the middle.
 *
 * Each cell is the median of r runs, in nanoseconds per object (per lookup
 * for lookup); the contracts are off (ADS_CONTRACTS=0).
 *
 *   positions [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/PositionIndex.h"

#if __has_include( <boost/unordered/unordered_flat_map.hpp> )
 #include <boost/unordered/unordered_flat_map.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <unordered_map>

using namespace ADS::bench;

namespace {

/// an object of a model
struct Row {
 std::uint64_t w[ 4 ];
 };

using Pos = std::uint32_t;

std::vector< Row > rows;                ///< the objects
std::vector< const Row * > v;           ///< their vector
std::vector< std::uint32_t > probe;     ///< the positions looked up
std::vector< Pos > erased , one;        ///< the positions erased

/// the cases of a map of the standard interface, made by make()
template< class M , class Make >
void standard( const std::string & who , std::size_t n , int r ,
               Make && make )
{
 if( skip( who ) )
  return;
 print( "positions" , "build" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          M m = make();
          for( std::size_t i = 0 ; i < n ; ++i )
           m.emplace( v[ i ] , Pos( i ) );
          sink = sink + m.size(); } , r , n ) );
 M full = make();
 for( std::size_t i = 0 ; i < n ; ++i )
  full.emplace( v[ i ] , Pos( i ) );
 print( "positions" , "lookup" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          std::uint64_t t = 0;
          for( const std::uint32_t i : probe )
           t += full.find( v[ i ] )->second;
          sink = sink + t; } , r , n ) );
 for( const auto * E : { &erased , &one } )
  print( "positions" , E == &erased ? "renumber" : "renumber-one" , n , who ,
         median( [ & ](){ return( full ); } , [ & ]( M & m ) {
          std::vector< Pos > to( n );       // the old positions to the new
          std::size_t k = 0 , d = 0;
          for( std::size_t i = 0 ; i < n ; ++i )
           if( ( k < E->size() ) && ( ( *E )[ k ] == i ) ) {
            to[ i ] = Pos( -1 );
            ++k;
            }
           else
            to[ i ] = Pos( d++ );
          for( const Pos e : *E )
           m.erase( v[ e ] );
          for( auto & [ p , i ] : m )
           i = to[ i ];
          sink = sink + m.size(); } , r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  std::mt19937_64 g( o.seed );
  rows.assign( n , Row{} );
  v.resize( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   v[ i ] = &rows[ i ];
  probe.resize( n );
  for( auto & p : probe )
   p = std::uint32_t( g() % n );
  erased.clear();
  for( std::size_t i = 0 ; i < n ; ++i )
   if( g() % 10 == 0 )
    erased.push_back( Pos( i ) );
  one = { Pos( n / 2 ) };

  using U = std::unordered_map< const Row * , Pos >;
  for( const char * who : { "std::unordered_map" ,
                            "std::unordered_map A/A" } )
   standard< U >( who , n , r , [](){ return( U() ); } );
#ifdef ADS_HAS_BOOST
  using B = boost::unordered_flat_map< const Row * , Pos >;
  standard< B >( "boost::unordered_flat_map" , n , r ,
                 [](){ return( B() ); } );
#endif

  // FlatHashTable: build and lookup, no update of the values in a pass
  using F = ADS::FlatHashTable< const Row * , Pos >;
  if( ! skip( "ADS::FlatHashTable" ) ) {
   print( "positions" , "build" , n , "ADS::FlatHashTable" ,
          median( [](){ return( 0 ); } , [ & ]( int ) {
           F m;
           for( std::size_t i = 0 ; i < n ; ++i )
            m.insert( v[ i ] , Pos( i ) );
           sink = sink + m.size(); } , r , n ) );
   {
    F m;
    for( std::size_t i = 0 ; i < n ; ++i )
     m.insert( v[ i ] , Pos( i ) );
    print( "positions" , "lookup" , n , "ADS::FlatHashTable" ,
           median( [](){ return( 0 ); } , [ & ]( int ) {
            std::uint64_t t = 0;
            for( const std::uint32_t i : probe )
             t += *m.find( v[ i ] );
            sink = sink + t; } , r , n ) );
    }
   }

  using P = ADS::PositionIndex< Row >;
  if( ! skip( "ADS::PositionIndex" ) ) {
   print( "positions" , "build" , n , "ADS::PositionIndex" ,
          median( [](){ return( 0 ); } , [ & ]( int ) {
           P m;
           for( std::size_t i = 0 ; i < n ; ++i )
            m.insert( v[ i ] , Pos( i ) );
           sink = sink + m.size(); } , r , n ) );
   P full;
   full.assign( v );
   print( "positions" , "lookup" , n , "ADS::PositionIndex" ,
          median( [](){ return( 0 ); } , [ & ]( int ) {
           std::uint64_t t = 0;
           for( const std::uint32_t i : probe )
            t += full.find( v[ i ] );
           sink = sink + t; } , r , n ) );
   for( const auto * E : { &erased , &one } )
    print( "positions" , E == &erased ? "renumber" : "renumber-one" , n ,
           "ADS::PositionIndex" , median( [ & ](){ return( full ); } ,
            [ & ]( P & m ) {
             m.renumber( *E );
             sink = sink + m.size(); } , r , n ) );
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File positions.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
