/*--------------------------------------------------------------------------*/
/*---------------------------- File search.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the searches in arrays: the lower bound in a sorted
 * array (benchmark "search"), the selection of the item of a rank
 * ("select") and the range minimum queries ("rmq").
 *
 * - search: SPlusTree, PGMIndex, branchlessLowerBound() and the binary
 *   search of the book against std::lower_bound, the layout of Eytzinger
 *   with the prefetch of Khuong and Morin (written here from their paper)
 *   and, if found, the PGM-index of Ferragina and Vinciguerra (64 and 4 its
 *   errors, its window searched by std::lower_bound, as in its README), on n
 *   sorted random keys of 32 and of 64 bits and n random queries, mostly
 *   absent; each query gives the item found, so that all of them pay for
 *   reading it.
 * - select: nthElement() and the two selections of the book against
 *   std::nth_element and, if found, the selections of miniselect (Floyd
 *   and Rivest, pdqselect, the adaptive quickselect of Alexandrescu), for
 *   the median and the rank n / 10 of integers of 32 and 64 bits and of
 *   doubles, random, sorted, reversed, with few distinct values and in an
 *   organ pipe; the time is per item, the copy of the input excluded.
 * - rmq: WordRangeMinimum, SparseTable and RangeMinimum against, if found,
 *   rmq_succinct_sct, rmq_succinct_sada and rmq_support_sparse_table of
 *   sdsl-lite, on n random integers of 32 bits: n queries on random ranges,
 *   n on ranges of at most 64 items, and the building, per item.
 *
 * Each cell is the median of r runs, in nanoseconds per operation; the
 * contracts are off (ADS_CONTRACTS=0). ONLY=a|b|... runs only the
 * structures whose name has a or b ... in it.
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

#include "ADS/search/BinarySearch.h"
#include "ADS/search/NthElement.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/search/Selection.h"
#include "ADS/search/StaticSearch.h"
#include "ADS/search/WordRangeMinimum.h"

#if __has_include( <miniselect/floyd_rivest_select.h> )
 #include <miniselect/floyd_rivest_select.h>
 #include <miniselect/median_of_ninthers.h>
 #include <miniselect/pdqselect.h>
 #define ADS_HAS_MINISELECT 1
#endif

#if __has_include( <pgm/pgm_index.hpp> )
 #include <pgm/pgm_index.hpp>
 #define ADS_HAS_PGM 1
#endif

#if __has_include( <sdsl/rmq_succinct_sct.hpp> )
 #include <sdsl/rmq_succinct_sada.hpp>
 #include <sdsl/rmq_succinct_sct.hpp>
 #include <sdsl/rmq_support_sparse_table.hpp>
 #define ADS_HAS_SDSL 1
#endif

#include <bit>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

using namespace ADS::bench;

namespace {

/// whether to run the structure who: all, or those whose name has in it
/// one of the texts separated by | in the variable ONLY
bool wanted( const std::string & who )
{
 const char * o = std::getenv( "ONLY" );
 if( ! o || ! *o )
  return( true );
 const std::string s = o;
 for( std::size_t a = 0 , b ; a <= s.size() ; a = b + 1 ) {
  b = std::min( s.find( '|' , a ) , s.size() );
  if( ( b > a ) && ( who.find( s.substr( a , b - a ) ) !=
                     std::string::npos ) )
   return( true );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- SEARCH ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the layout of Eytzinger of Khuong and Morin: the sorted items in the
/// order of a heap, searched with no branch and with the prefetch of the
/// 16 descendants 4 levels down
template< class T >
class Eytzinger {
 public:
 explicit Eytzinger( const std::vector< T > & A ) : f_n( A.size() ) ,
  f_b( static_cast< T * >( ::operator new( ( A.size() + 1 ) * sizeof( T ) ,
                                           std::align_val_t( 64 ) ) ) ) {
  std::size_t j = 0;
  fill( A , j , 1 );
  }
 ~Eytzinger() { ::operator delete( f_b , std::align_val_t( 64 ) ); }
 Eytzinger( const Eytzinger & ) = delete;
 Eytzinger & operator=( const Eytzinger & ) = delete;

 /// the first item not smaller than x, 0 if none
 T lower( T x ) const {
  std::size_t i = 1;
  while( i <= f_n ) {
   __builtin_prefetch( f_b + 16 * i );
   i = 2 * i + ( f_b[ i ] < x );
   }
  i >>= std::countr_zero( ~i ) + 1;
  return( i ? f_b[ i ] : T( 0 ) );
  }

 private:
 void fill( const std::vector< T > & A , std::size_t & j , std::size_t i ) {
  if( i > f_n ) return;
  fill( A , j , 2 * i );
  f_b[ i ] = A[ j++ ];
  fill( A , j , 2 * i + 1 );
  }
 std::size_t f_n;
 T * f_b;
 };

template< class T >
void search( const char * type , std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< T > A( n ) , Q( n );
 for( auto & a : A ) a = T( g() );
 for( auto & q : Q ) q = T( g() );
 std::sort( A.begin() , A.end() );
 auto at = [ & ]( std::size_t i ) { return( i < n ? A[ i ] : T( 0 ) ); };
 auto cell = [ & ]( const std::string & who , auto && f ) {
  if( ! wanted( who ) )
   return;
  print( "search" , type , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           T s = 0;
           for( T q : Q ) s += f( q );
           sink = sink + std::uint64_t( s ); } , r , n ) );
  };
 for( const char * who : { "std::lower_bound" , "std::lower_bound A/A" } )
  cell( who , [ & ]( T q ) {
   return( at( std::size_t( std::lower_bound( A.begin() , A.end() , q ) -
                            A.begin() ) ) ); } );
 if( wanted( "Eytzinger" ) ) {
  const Eytzinger< T > E( A );
  cell( "Eytzinger" , [ & ]( T q ) { return( E.lower( q ) ); } );
  }
 cell( "ADS::lowerBound" , [ & ]( T q ) {
  return( at( ADS::lowerBound( A , q ) ) ); } );
 cell( "ADS::branchlessLowerBound" , [ & ]( T q ) {
  return( at( ADS::branchlessLowerBound( A , q ) ) ); } );
 if( wanted( "ADS::SPlusTree" ) ) {
  const ADS::SPlusTree< T > S( A );
  cell( "ADS::SPlusTree" , [ & ]( T q ) {   // the item of its own copy
   const std::size_t r = S.lowerBound( q );
   return( r < n ? S[ r ] : T( 0 ) ); } );
  }
#ifdef ADS_HAS_PGM
 if( wanted( "pgm::PGMIndex" ) ) {          // the search of its README
  const pgm::PGMIndex< T , 64 , 4 > P( A.begin() , A.end() );
  cell( "pgm::PGMIndex" , [ & ]( T q ) {
   const auto w = P.search( q );
   return( at( std::size_t( std::lower_bound( A.begin() + long( w.lo ) ,
                                              A.begin() + long( w.hi ) ,
                                              q ) - A.begin() ) ) ); } );
  }
#endif
 if( wanted( "ADS::PGMIndex" ) ) {
  const ADS::PGMIndex< T , 64 , 4 > P( A );
  cell( "ADS::PGMIndex" , [ & ]( T q ) {    // the item of its own copy
   const std::size_t r = P.lowerBound( q );
   return( r < n ? P[ r ] : T( 0 ) ); } );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- SELECT ----------------------------------*/
/*--------------------------------------------------------------------------*/

const char * patterns[] = { "random" , "sorted" , "reversed" , "few" ,
                            "organ" };

template< class T >
void select( const char * type , std::size_t n , int r , unsigned seed )
{
 for( int p = 0 ; p < 5 ; ++p )
  for( int q = 0 ; q < 2 ; ++q ) {
   std::mt19937_64 g( seed );
   std::vector< T > A( n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t k = p == 0 ? g() : p == 1 ? i : p == 2 ? n - i :
                            p == 3 ? g() % 16 : std::min( i , n - 1 - i );
    if constexpr( std::is_floating_point_v< T > )
     A[ i ] = T( std::int64_t( k ) ) / 1e3;
    else
     A[ i ] = T( k );
    }
   const std::size_t k = q == 0 ? n / 2 : n / 10;
   const std::string what = std::string( type ) + " " + patterns[ p ] +
                            ( q == 0 ? " median" : " n/10" );
   auto cell = [ & ]( const std::string & who , auto && f ) {
    if( ! wanted( who ) )
     return;
    print( "select" , what , n , who , median( [ & ](){ return( A ); } ,
            [ & ]( std::vector< T > & B ) {
             f( B );
             sink = sink + std::uint64_t( B[ k ] ); } , r , n ) );
    };
   using V = std::vector< T >;
   const auto at = std::ptrdiff_t( k );
   for( const char * who : { "std::nth_element" , "std::nth_element A/A" } )
    cell( who , [ & ]( V & B ) {
     std::nth_element( B.begin() , B.begin() + at , B.end() ); } );
#ifdef ADS_HAS_MINISELECT
   cell( "miniselect::floyd_rivest_select" , [ & ]( V & B ) {
    miniselect::floyd_rivest_select( B.begin() , B.begin() + at ,
                                     B.end() ); } );
   cell( "miniselect::pdqselect" , [ & ]( V & B ) {
    miniselect::pdqselect( B.begin() , B.begin() + at , B.end() ); } );
   cell( "miniselect::median_of_ninthers" , [ & ]( V & B ) {
    miniselect::median_of_ninthers_select( B.begin() , B.begin() + at ,
                                           B.end() ); } );
#endif
   cell( "ADS::nthElement" , [ & ]( V & B ) {
    ADS::nthElement( B.begin() , B.begin() + at , B.end() ); } );
   if( p != 3 ) {                           // quadratic on equal items
    cell( "ADS::selection" , [ & ]( V & B ) {
     std::mt19937 rng( 7 );
     ADS::selection( B , k + 1 , &rng ); } );
    cell( "ADS::medianOfMedians" , [ & ]( V & B ) {
     ADS::medianOfMedians( B , k + 1 ); } );
    }
   }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- RMQ ------------------------------------*/
/*--------------------------------------------------------------------------*/

void rmq( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::int32_t > A( n );
 for( auto & a : A ) a = std::int32_t( g() );
 std::vector< std::pair< std::size_t , std::size_t > > L( n ) , S( n );
 for( std::size_t q = 0 ; q < n ; ++q ) {
  std::size_t i = g() % n , j = g() % n;
  if( i > j ) std::swap( i , j );
  L[ q ] = { i , j };
  i = g() % n;
  S[ q ] = { i , std::min( n - 1 , i + g() % 64 ) };
  }
 auto cells = [ & ]( const std::string & who , auto && build , auto && ask ) {
  if( ! wanted( who ) )                     // the tables of n log n words
   return;                                  // are left out past 4 10^6
  if( ( n > 4000000 ) && ( ( who.find( "parse" ) != std::string::npos ) ||
                           ( who.find( "sparse_table" ) !=
                             std::string::npos ) ) )
   return;
  print( "rmq" , "build" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) { auto s = build(); sink = sink + ask( *s , 0 ,
                                                              n - 1 ); } ,
          r , n ) );
  const auto s = build();
  for( int c = 0 ; c < 2 ; ++c )
   print( "rmq" , c == 0 ? "random ranges" : "short ranges" , n , who ,
          median( [](){ return( 0 ); } , [ & ]( int ) {
           std::uint64_t t = 0;
           for( const auto & [ i , j ] : c == 0 ? L : S )
            t += std::uint64_t( A[ ask( *s , i , j ) ] );
           sink = sink + t; } , r , n ) );
  };
 using V = std::vector< std::int32_t >;
 auto ads = []( const auto & s , std::size_t i , std::size_t j ) {
  return( s.query( i , j ) ); };
#ifdef ADS_HAS_SDSL
 for( const char * who : { "sdsl::rmq_succinct_sct" ,
                           "sdsl::rmq_succinct_sct A/A" } )
  cells( who , [ & ](){
          return( std::make_unique< sdsl::rmq_succinct_sct<> >( &A ) ); } ,
         []( const auto & s , std::size_t i , std::size_t j ) {
          return( std::size_t( s( i , j ) ) ); } );
 cells( "sdsl::rmq_succinct_sada" , [ & ](){
         return( std::make_unique< sdsl::rmq_succinct_sada<> >( &A ) ); } ,
        []( const auto & s , std::size_t i , std::size_t j ) {
         return( std::size_t( s( i , j ) ) ); } );
 cells( "sdsl::rmq_support_sparse_table" , [ & ](){
         return( std::make_unique<
                  sdsl::rmq_support_sparse_table< V , true > >( &A ) ); } ,
        []( const auto & s , std::size_t i , std::size_t j ) {
         return( std::size_t( s( i , j ) ) ); } );
#endif
 cells( "ADS::WordRangeMinimum" , [ & ](){
         return( std::make_unique< ADS::WordRangeMinimum< std::int32_t > >(
                  A ) ); } , ads );
 cells( "ADS::SparseTable" , [ & ](){
         return( std::make_unique< ADS::SparseTable< std::int32_t > >(
                  A ) ); } , ads );
 cells( "ADS::RangeMinimum" , [ & ](){
         return( std::make_unique< ADS::RangeMinimum< std::int32_t > >(
                  A ) ); } , ads );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 , 10000000 } );
 const char * part = std::getenv( "PART" );   // search, select or rmq
 auto doing = [ & ]( const char * p ) {
  return( ! part || ! *part || ( std::string( part ) == p ) ); };
 for( std::size_t n : o.sizes ) {
  if( doing( "search" ) ) {
   search< std::uint32_t >( "uint32" , n , o.runs , o.seed );
   search< std::uint64_t >( "uint64" , n , o.runs , o.seed );
   }
  if( doing( "select" ) ) {
   select< std::uint32_t >( "uint32" , n , o.runs , o.seed );
   select< std::int64_t >( "int64" , n , o.runs , o.seed );
   select< double >( "double" , n , o.runs , o.seed );
   }
  if( doing( "rmq" ) )
   rmq( n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File search.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
