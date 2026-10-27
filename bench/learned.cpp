/*--------------------------------------------------------------------------*/
/*---------------------------- File learned.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the learned structures built on the piecewise linear
 * approximation of the PGM-index:
 *
 * - lavector: access and nextGEQ (the select and the rank of the vector of
 *   bits of the items) and the building of the LA-vector (with
 *   corrections of 7 bits, and with the width of the fewest bits) against
 *   Elias-Fano, the partitioned Elias-Fano and, if found, the sd_vector of
 *   sdsl-lite, on n distinct items: uniform in [ 0 , 128 n ), in
 *   clusters (those of the benchmark succinct), on a line of slope 64 with noise of 6 bits,
 *   and the gaps of a geometric law of mean 100 (the lists of an inverted
 *   index); the bits per item in the group space;
 * - pgm: the lower bound of n random queries (each giving the item found)
 *   and the building of the PGM-index and of its compressed form against,
 *   if found, the PGM-index and the CompressedPGMIndex of Ferragina and
 *   Vinciguerra (64 and 4 their errors), on n keys of 64 bits: uniform,
 *   lognormal (the exponential of a normal of deviation 2, times 10^9) and
 *   in clusters; the bits per key of the index (the items aside) in the
 *   group space;
 * - dynamic: n insertions of random keys of 64 bits in random order, n
 *   lookups of present keys, n of absent ones, n lower bounds, and n mixed
 *   operations (half insertions, a quarter removals, a quarter lookups) on
 *   the map of the n keys, by the dynamic PGM-index against std::map,
 *   BTreeMap and, if found, the DynamicPGMIndex of Ferragina and
 *   Vinciguerra.
 *
 * Each cell is the median of r runs, in nanoseconds per operation; the
 * queries run on a structure built once, and the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   learned [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/search/CompressedPGMIndex.h"
#include "ADS/search/DynamicPGMIndex.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/LAVector.h"
#include "ADS/succinct/PartitionedEliasFano.h"
#include "ADS/tree/BTreeMap.h"

#if __has_include( <sdsl/sd_vector.hpp> )
 #include <sdsl/sd_vector.hpp>
 #define ADS_HAS_SDSL 1
#endif

#if __has_include( <pgm/pgm_index.hpp> )
 #include <pgm/pgm_index.hpp>
 #include <pgm/pgm_index_dynamic.hpp>
 #define ADS_HAS_PGM 1
 #if defined( ADS_HAS_SDSL ) && __has_include( <pgm/pgm_index_variants.hpp> )
  #include <pgm/pgm_index_variants.hpp>
  #define ADS_HAS_CPGM 1
 #endif
#endif

#include <cmath>
#include <map>
#include <memory>
#include <string>

using namespace ADS::bench;

namespace {

/// the median time of the queries f( x ) for the x of X, summed into the
/// sink
template< class F >
double queries( const std::vector< std::uint64_t > & X , int r , F && f )
{
 return( median( [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t s = 0;
          for( const auto x : X ) s += std::uint64_t( f( x ) );
          sink = sink + s; } , r , X.size() ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- LA-VECTOR -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cells of a sequence: access( i ), from 0, and nextGEQ( x ), the
/// number of the items below x, and its bits per item
template< class B , class A , class G , class Z >
void sequence( const std::string & tag ,
               const std::vector< std::uint64_t > & X , std::uint64_t u ,
               const std::vector< std::uint64_t > & I ,
               const std::vector< std::uint64_t > & Q ,
               const std::string & who , B build , A access , G geq , Z bits , int r )
{
 if( skip( who ) )
  return;
 struct { const std::string & who; B & build; A & access; G & geq;
          Z & bits; } c{ who , build , access , geq , bits };
 const std::size_t n = X.size();
 print( "lavector" , "build " + tag , n , c.who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto s = c.build( X , u );
          sink = sink + std::uint64_t( s != nullptr ); } , r , n ) );
 auto s = c.build( X , u );
 print( "lavector" , "access " + tag , n , c.who , queries( I , r ,
         [ & ]( std::uint64_t i ) { return( c.access( *s , i ) ); } ) );
 print( "lavector" , "nextGEQ " + tag , n , c.who , queries( Q , r ,
         [ & ]( std::uint64_t x ) { return( c.geq( *s , x ) ); } ) );
 print( "space" , "lavector " + tag , n , c.who , c.bits( *s ) );
 }

void laVectors( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 for( const char * tag : { "uniform" , "clustered" , "line" , "postings" } ) {
  const std::string t = tag;
  std::vector< std::uint64_t > X;
  std::uint64_t u = 128 * std::uint64_t( n );
  if( t == "uniform" ) {                    // distinct, for sd_vector
   std::unordered_set< std::uint64_t > seen;
   while( X.size() < n )
    if( const std::uint64_t x = g() % u ; seen.insert( x ).second )
     X.push_back( x );
   std::sort( X.begin() , X.end() );
   }
  else {
   // clusters as in the benchmark succinct, a line of slope 64 with a
   // noise of 6 bits, or the gaps of a geometric law of mean 100
   std::geometric_distribution< std::uint64_t > geo( 0.01 );
   std::uint64_t x = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    if( t == "clustered" )
     x += g() % 64 == 0 ? 1 + g() % 16384 : 1 + g() % 4;
    else if( t == "line" )
     x = 64 * std::uint64_t( i ) + g() % 64;
    else
     x += 1 + geo( g );
    X.push_back( x );
    }
   u = x + 1;
   }
  std::vector< std::uint64_t > I , Q;
  for( std::size_t q = 0 ; q < 1000000 ; ++q ) {
   I.push_back( g() % n );
   Q.push_back( g() % u );
   }
  const double nn = double( n );
  using LA = ADS::LAVector;
  for( const char * who : { "ADS::LAVector" , "ADS::LAVector A/A" } )
   sequence( t , X , u , I , Q , who ,
    []( const std::vector< std::uint64_t > & X , std::uint64_t ) {
     return( std::make_unique< LA >( X , 7 ) ); } ,
    []( const LA & s , std::uint64_t i ) { return( s.access( i ) ); } ,
    []( const LA & s , std::uint64_t x ) { return( s.nextGEQ( x ) ); } ,
    [ nn ]( const LA & s ) { return( double( s.sizeInBits() ) / nn ); } ,
    r );
  sequence( t , X , u , I , Q , "ADS::LAVector tuned" ,
   []( const std::vector< std::uint64_t > & X , std::uint64_t ) {
    return( std::make_unique< LA >( LA::tuned( X ) ) ); } ,
   []( const LA & s , std::uint64_t i ) { return( s.access( i ) ); } ,
   []( const LA & s , std::uint64_t x ) { return( s.nextGEQ( x ) ); } ,
   [ nn ]( const LA & s ) { return( double( s.sizeInBits() ) / nn ); } ,
   r );
  using EF = ADS::EliasFano;
  sequence( t , X , u , I , Q , "ADS::EliasFano" ,
   []( const std::vector< std::uint64_t > & X , std::uint64_t u ) {
    return( std::make_unique< EF >( X , u ) ); } ,
   []( const EF & s , std::uint64_t i ) { return( s.access( i ) ); } ,
   []( const EF & s , std::uint64_t x ) { return( s.nextGEQ( x ) ); } ,
   [ nn ]( const EF & s ) { return( double( s.sizeInBits() ) / nn ); } ,
   r );
  using PEF = ADS::PartitionedEliasFano;
  sequence( t , X , u , I , Q , "ADS::PartitionedEliasFano" ,
   []( const std::vector< std::uint64_t > & X , std::uint64_t u ) {
    return( std::make_unique< PEF >( X , u ) ); } ,
   []( const PEF & s , std::uint64_t i ) { return( s.access( i ) ); } ,
   []( const PEF & s , std::uint64_t x ) { return( s.nextGEQ( x ) ); } ,
   [ nn ]( const PEF & s ) { return( double( s.sizeInBits() ) / nn ); } ,
   r );
#ifdef ADS_HAS_SDSL
  struct SD {
   sdsl::sd_vector<> v;
   sdsl::sd_vector<>::rank_1_type rk;
   sdsl::sd_vector<>::select_1_type sl;
   };
  sequence( t , X , u , I , Q , "sdsl::sd_vector" ,
   []( const std::vector< std::uint64_t > & X , std::uint64_t u ) {
    auto s = std::make_unique< SD >();
    sdsl::sd_vector_builder b( u , X.size() );
    for( const auto x : X ) b.set( x );
    s->v = sdsl::sd_vector<>( b );
    s->rk = sdsl::sd_vector<>::rank_1_type( &s->v );
    s->sl = sdsl::sd_vector<>::select_1_type( &s->v );
    return( s ); } ,
   []( const SD & s , std::uint64_t i ) { return( s.sl.select( i + 1 ) ); } ,
   []( const SD & s , std::uint64_t x ) { return( s.rk.rank( x ) ); } ,
   [ nn ]( const SD & s ) {
    return( 8 * double( sdsl::size_in_bytes( s.v ) ) / nn ); } , r );
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- PGM-INDEX --------------------------------*/
/*--------------------------------------------------------------------------*/

void pgms( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 using T = std::uint64_t;
 for( const char * tag : { "uniform" , "lognormal" , "clustered" } ) {
  const std::string t = tag;
  std::vector< T > A( n ) , Q( n );
  std::normal_distribution< double > N( 0 , 2 );
  T x = 0;
  for( auto & a : A )
   if( t == "uniform" ) a = g();
   else if( t == "lognormal" ) a = T( std::exp( N( g ) ) * 1e9 );
   else a = x += g() % 64 == 0 ? 1 + g() % ( T( 1 ) << 30 ) : 1 + g() % 4;
  std::sort( A.begin() , A.end() );
  for( auto & q : Q ) q = A[ g() % n ] + g() % 3;
  const double nn = double( n );
  auto cell = [ & ]( const std::string & who , auto && make , auto && f ,
                     auto && bytes ) {
   if( skip( who ) )
    return;
   print( "pgm" , "build " + t , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) { auto P = make(); sink = sink + P->size(); } ,
           r , n ) );
   const auto P = make();
   print( "pgm" , "lowerBound " + t , n , who , queries( Q , r ,
           [ & ]( T q ) { return( f( *P , q ) ); } ) );
   print( "space" , "pgm " + t , n , who , 8 * double( bytes( *P ) ) / nn );
   };
  using P = ADS::PGMIndex< T , 64 , 4 >;
  for( const char * who : { "ADS::PGMIndex" , "ADS::PGMIndex A/A" } )
   cell( who , [ & ](){ return( std::make_unique< P >( A ) ); } ,
         [ & ]( const P & p , T q ) {
          const std::size_t i = p.lowerBound( q );
          return( i < n ? p[ i ] : T( 0 ) ); } ,
         []( const P & p ) { return( p.bytes() ); } );
  using C = ADS::CompressedPGMIndex< T , 64 >;
  cell( "ADS::CompressedPGMIndex" ,
        [ & ](){ return( std::make_unique< C >( A ) ); } ,
        [ & ]( const C & p , T q ) {
         const std::size_t i = p.lowerBound( q );
         return( i < n ? p[ i ] : T( 0 ) ); } ,
        []( const C & p ) { return( p.bytes() ); } );
#ifdef ADS_HAS_PGM
  // the search of their README: the window by the index, then
  // std::lower_bound in the array, which they keep apart
  using Q4 = pgm::PGMIndex< T , 64 , 4 >;
  struct PG {
   const std::vector< T > & A;
   Q4 p;
   std::size_t size( void ) const { return( A.size() ); }
   };
  cell( "pgm::PGMIndex" ,
        [ & ](){ return( std::make_unique< PG >( PG{ A ,
                          Q4( A.begin() , A.end() ) } ) ); } ,
        [ & ]( const PG & p , T q ) {
         const auto w = p.p.search( q );
         const auto it = std::lower_bound( A.begin() + long( w.lo ) ,
                                           A.begin() + long( w.hi ) , q );
         return( it == A.end() ? T( 0 ) : *it ); } ,
        []( const PG & p ) { return( p.p.size_in_bytes() ); } );
#endif
#ifdef ADS_HAS_CPGM
  using CQ = pgm::CompressedPGMIndex< T , 64 , 4 >;
  struct CPG {
   const std::vector< T > & A;
   CQ p;
   std::size_t size( void ) const { return( A.size() ); }
   };
  cell( "pgm::CompressedPGMIndex" ,
        [ & ](){ return( std::make_unique< CPG >( CPG{ A , CQ( A ) } ) ); } ,
        [ & ]( const CPG & p , T q ) {
         const auto w = p.p.search( q );
         const auto it = std::lower_bound( A.begin() + long( w.lo ) ,
                                           A.begin() + long( w.hi ) , q );
         return( it == A.end() ? T( 0 ) : *it ); } ,
        []( const CPG & p ) { return( p.p.size_in_bytes() ); } );
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- DYNAMIC --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cells of a map, by make() (an empty one), ins( m , k , v ),
/// era( m , k ), find( m , k ) (the value, or 0) and lb( m , k ) (the value
/// of the least key not below k, or 0)
template< class M , class I , class E , class F , class L >
void dynamicMap( const std::string & who ,
                 const std::vector< std::uint64_t > & K ,
                 const std::vector< std::uint64_t > & H ,
                 const std::vector< std::uint64_t > & Q ,
                 const std::vector< std::uint64_t > & O , int r , M make ,
                 I ins , E era , F find , L lb )
{
 if( skip( who ) )
  return;
 const std::size_t n = K.size();
 print( "dynamic" , "insert" , n , who , median( make , [ & ]( auto & m ) {
         for( std::size_t i = 0 ; i < n ; ++i ) ins( *m , K[ i ] , i + 1 ); } ,
         r , n ) );
 auto m = make();
 for( std::size_t i = 0 ; i < n ; ++i ) ins( *m , K[ i ] , i + 1 );
 print( "dynamic" , "lookup hit" , n , who , queries( H , r ,
         [ & ]( std::uint64_t k ) { return( find( *m , k ) ); } ) );
 print( "dynamic" , "lookup miss" , n , who , queries( Q , r ,
         [ & ]( std::uint64_t k ) { return( find( *m , k ) ); } ) );
 print( "dynamic" , "lowerBound" , n , who , queries( Q , r ,
         [ & ]( std::uint64_t k ) { return( lb( *m , k ) ); } ) );
 // the mixed operations on a copy of the map of the n keys, built again
 // before each run
 print( "dynamic" , "mixed" , n , who , median( [ & ](){
         auto c = make();
         for( std::size_t i = 0 ; i < n ; ++i ) ins( *c , K[ i ] , i + 1 );
         return( c ); } , [ & ]( auto & c ) {
          std::uint64_t s = 0;
          for( std::size_t i = 0 ; i < O.size() ; ++i )
           switch( i % 4 ) {
            case 0: case 1: ins( *c , O[ i ] , i ); break;
            case 2: era( *c , K[ O[ i ] % n ] ); break;
            default: s += find( *c , O[ i ] ); break;
            }
          sink = sink + s; } , r , O.size() ) );
 }

void dynamics( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 const std::vector< std::uint64_t > K = keys( 2 * n , seed );
 const std::vector< std::uint64_t > In( K.begin() , K.begin() + long( n ) );
 const std::vector< std::uint64_t > Out( K.begin() + long( n ) , K.end() );
 std::vector< std::uint64_t > H( In ) , O( n );
 std::shuffle( H.begin() , H.end() , g );
 for( auto & o : O ) o = g() >> 1;
 using U = std::uint64_t;
 using D = ADS::DynamicPGMIndex< U , U >;
 for( const char * who : { "ADS::DynamicPGMIndex" ,
                           "ADS::DynamicPGMIndex A/A" } )
  dynamicMap( who , In , H , Out , O , r ,
   [](){ return( std::make_unique< D >() ); } ,
   []( D & m , U k , U v ) { m.insert( k , v ); } ,
   []( D & m , U k ) { m.erase( k ); } ,
   []( const D & m , U k ) { const U * v = m.find( k );
                             return( v ? *v : 0 ); } ,
   []( const D & m , U k ) { const auto p = m.lowerBound( k );
                             return( p ? p->second : 0 ); } );
 using S = std::map< U , U >;
 dynamicMap( "std::map" , In , H , Out , O , r ,
  [](){ return( std::make_unique< S >() ); } ,
  []( S & m , U k , U v ) { m.insert_or_assign( k , v ); } ,
  []( S & m , U k ) { m.erase( k ); } ,
  []( const S & m , U k ) { const auto it = m.find( k );
                            return( it == m.end() ? 0 : it->second ); } ,
  []( const S & m , U k ) { const auto it = m.lower_bound( k );
                            return( it == m.end() ? 0 : it->second ); } );
 using B = ADS::BTreeMap< U , U >;
 dynamicMap( "ADS::BTreeMap" , In , H , Out , O , r ,
  [](){ return( std::make_unique< B >() ); } ,
  []( B & m , U k , U v ) {
   if( U * p = m.find( k ) ) *p = v;
   else m.insert( k , v ); } ,
  []( B & m , U k ) { m.remove( k ); } ,
  []( const B & m , U k ) { const U * v = m.find( k );
                            return( v ? *v : 0 ); } ,
  []( const B & m , U k ) { const auto it = m.lowerBound( k );
                            return( it == m.end() ? 0 : ( *it ).second ); } );
#ifdef ADS_HAS_PGM
 using P = pgm::DynamicPGMIndex< U , U >;
 dynamicMap( "pgm::DynamicPGMIndex" , In , H , Out , O , r ,
  [](){ return( std::make_unique< P >() ); } ,
  []( P & m , U k , U v ) { m.insert_or_assign( k , v ); } ,
  []( P & m , U k ) { m.erase( k ); } ,
  []( const P & m , U k ) { const auto it = m.find( k );
                            return( it == m.end() ? 0 : it->second ); } ,
  []( const P & m , U k ) { const auto it = m.lower_bound( k );
                            return( it == m.end() ? 0 : it->second ); } );
#endif
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  laVectors( n , o.runs , o.seed );
  pgms( n , o.runs , o.seed );
  dynamics( n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File learned.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
