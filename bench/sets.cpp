/*--------------------------------------------------------------------------*/
/*----------------------------- File sets.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sets: the intersection of sorted lists
 * ("intersection"), the union of disjoint sets ("mfset"), the boolean
 * vector ("boolset") and the compressed bitmap ("bitmap").
 *
 * - intersection: intersect() and the intersections of the notes [see
 *   SortedIntersection.h] against std::set_intersection and, where they
 *   are found, SIMDintersection of the library of Lemire, Boytsov and Kurz
 *   and the and of two Roaring bitmaps (built before the clock); the
 *   longer list has n random distinct integers of 32 bits out of 4 n, the
 *   shorter one n / r out of the same range, for r = 1, 10, 100 and 1000,
 *   and the time is per item of the shorter list.
 * - mfset: RemUnionFind, UnionFind (union by size and halving) and MFSet
 *   against boost::disjoint_sets (union by rank and compression of the
 *   paths): 2 n merges of random pairs of n
 *   elements, then n queries of random pairs, and the n - 1 merges of a
 *   chain ( i , i + 1 ) in random order; the time is per operation.
 * - boolset: BoolSet against boost::dynamic_bitset and, where it is found,
 *   Roaring, on two sets of half of the integers in [ 0 , n ) at random:
 *   the union, the intersection, the difference and the symmetric
 *   difference of the two (their results built in the clock), n random
 *   membership tests and the visit of the items; the time is per word of
 *   64 integers of the universe for the operations, per test for the
 *   tests and per integer for the visit.
 *
 * - bitmap: RoaringBitmap against the bitmaps of CRoaring, if found, on two
 *   sets of n random integers of 32 bits out of 4 n ("dense", bitmaps),
 *   32 n ("medium", arrays of about 2048 items) and 1000 n ("sparse",
 *   short arrays): the building from the sorted items, the four
 *   operations of two sets, n random membership tests and the visit by
 *   the iterators; the time is per item of a set.
 *
 * Each cell is the median of r runs, in nanoseconds; the contracts are off
 * (ADS_CONTRACTS=0). ONLY=a|b|... runs only the structures whose name has
 * a or b ... in it, PART=intersection, mfset, boolset or bitmap only that
 * part.
 *
 *   sets [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/mfset/MFSet.h"
#include "ADS/mfset/RemUnionFind.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/set/SortedIntersection.h"
#include "ADS/set/VectorIntersection.h"

#if __has_include( <boost/pending/disjoint_sets.hpp> )
 #include <boost/dynamic_bitset.hpp>
 #include <boost/pending/disjoint_sets.hpp>
 #define ADS_HAS_BOOST 1
#endif

#if __has_include( <SIMDCompressionAndIntersection/intersection.h> )
 #include <SIMDCompressionAndIntersection/intersection.h>
 #define ADS_HAS_LEMIRE 1
#endif

#if __has_include( <roaring/roaring.h> )
 #include <roaring/roaring.h>
 #define ADS_HAS_ROARING 1
#endif

#include <cstdint>
#include <set>

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

/// whether to measure the structure who: wanted() and not left out by
/// skip()
bool measured( const std::string & who )
{
 return( wanted( who ) && ! skip( who ) );
 }

/// k distinct random integers below u, sorted
std::vector< std::uint32_t > sample( std::size_t k , std::uint64_t u ,
                                     std::mt19937_64 & g )
{
 std::vector< std::uint32_t > V;
 V.reserve( k + k / 4 );
 while( V.size() < k ) {
  for( std::size_t t = V.size() ; t < k + k / 8 + 8 ; ++t )
   V.push_back( std::uint32_t( g() % u ) );
  std::sort( V.begin() , V.end() );
  V.erase( std::unique( V.begin() , V.end() ) , V.end() );
  }
 std::shuffle( V.begin() , V.end() , g );   // k of them at random
 V.resize( k );
 std::sort( V.begin() , V.end() );
 return( V );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- INTERSECTION -------------------------------*/
/*--------------------------------------------------------------------------*/

void intersection( std::size_t n , int r , unsigned seed )
{
 using V = std::vector< std::uint32_t >;
 for( std::size_t ratio : { 1 , 10 , 100 , 1000 } ) {
  std::mt19937_64 g( seed + ratio );
  const V A = sample( n , 4 * std::uint64_t( n ) , g );
  const V B = sample( std::max< std::size_t >( n / ratio , 1 ) ,
                      4 * std::uint64_t( n ) , g );
  V out( B.size() + 8 );
  const std::string what = "1:" + std::to_string( ratio );
  const std::size_t items = B.size();
  auto cell = [ & ]( const std::string & who , auto && f ) {
   if( ! measured( who ) )
    return;
   print( "intersection" , what , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) { sink = sink + std::uint64_t( f() ); } , r ,
           items ) );
   };
  for( const char * who : { "std::set_intersection" ,
                            "std::set_intersection A/A" } )
   cell( who , [ & ](){
    return( std::size_t( std::set_intersection( A.begin() , A.end() ,
                                                B.begin() , B.end() ,
                                                out.begin() ) -
                         out.begin() ) ); } );
#ifdef ADS_HAS_LEMIRE
  cell( "Lemire SIMDintersection" , [ & ](){
   return( SIMDCompressionLib::SIMDintersection( B.data() , B.size() ,
                                                 A.data() , A.size() ,
                                                 out.data() ) ); } );
#endif
#ifdef ADS_HAS_ROARING
  if( wanted( "Roaring" ) && ! skip( "Roaring and" ) ) {
   roaring_bitmap_t * a = roaring_bitmap_of_ptr( A.size() , A.data() );
   roaring_bitmap_t * b = roaring_bitmap_of_ptr( B.size() , B.data() );
   roaring_bitmap_run_optimize( a );
   roaring_bitmap_run_optimize( b );
   cell( "Roaring and" , [ & ](){
    roaring_bitmap_t * c = roaring_bitmap_and( a , b );
    const std::size_t k = std::size_t( roaring_bitmap_get_cardinality( c ) );
    roaring_bitmap_free( c );
    return( k ); } );
   roaring_bitmap_free( a );
   roaring_bitmap_free( b );
   }
#endif
  cell( "ADS::intersect" , [ & ](){
   return( ADS::intersect( A.data() , A.size() , B.data() , B.size() ,
                           out.data() ) ); } );
  cell( "ADS::intersectMerge" , [ & ](){
   return( ADS::intersectMerge( A , B ).size() ); } );
  cell( "ADS::intersectDoubling" , [ & ](){
   return( ADS::intersectDoubling( A , B ).size() ); } );
  cell( "ADS::intersectMutualPartition" , [ & ](){
   return( ADS::intersectMutualPartition( A , B ).size() ); } );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MFSET -----------------------------------*/
/*--------------------------------------------------------------------------*/

void mfset( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::pair< std::uint32_t , std::uint32_t > > E( 2 * n ) ,
                                                          Q( n ) , C;
 for( auto & [ x , y ] : E ) {
  x = std::uint32_t( g() % n );
  y = std::uint32_t( g() % n );
  }
 for( auto & [ x , y ] : Q ) {
  x = std::uint32_t( g() % n );
  y = std::uint32_t( g() % n );
  }
 for( std::uint32_t i = 0 ; i + 1 < n ; ++i ) C.emplace_back( i , i + 1 );
 std::shuffle( C.begin() , C.end() , g );
 // the three cases on the structure that make() builds, by merge( s , x ,
 // y ) and same( s , x , y )
 auto run = [ & ]( const std::string & who , auto && make , auto && merge ,
                   auto && same ) {
  if( ! measured( who ) )
   return;
  print( "mfset" , "random merges" , n , who , median( make ,
          [ & ]( auto & s ) {
           std::size_t k = 0;
           for( const auto & [ x , y ] : E ) k += merge( *s , x , y );
           sink = sink + k; } , r , E.size() ) );
  auto s = make();
  for( const auto & [ x , y ] : E ) merge( *s , x , y );
  print( "mfset" , "random queries" , n , who , median(
          [](){ return( 0 ); } , [ & ]( int ) {
           std::size_t k = 0;
           for( const auto & [ x , y ] : Q ) k += same( *s , x , y );
           sink = sink + k; } , r , Q.size() ) );
  print( "mfset" , "chain merges" , n , who , median( make ,
          [ & ]( auto & t ) {
           std::size_t k = 0;
           for( const auto & [ x , y ] : C ) k += merge( *t , x , y );
           sink = sink + k; } , r , C.size() ) );
  };
#ifdef ADS_HAS_BOOST
 struct Boost {                             // the forest and its sets
  std::vector< std::uint32_t > rank , parent;
  boost::disjoint_sets< std::uint32_t * , std::uint32_t * > s;
  explicit Boost( std::size_t n ) : rank( n , 0 ) , parent( n ) ,
   s( rank.data() , parent.data() ) {
   for( std::uint32_t i = 0 ; i < n ; ++i ) s.make_set( i );
   }
  };
 for( const char * who : { "boost::disjoint_sets" ,
                           "boost::disjoint_sets A/A" } )
  run( who , [ & ](){ return( std::make_unique< Boost >( n ) ); } ,
       []( Boost & b , std::uint32_t x , std::uint32_t y ) {
        const auto a = b.s.find_set( x ) , c = b.s.find_set( y );
        if( a == c ) return( false );
        b.s.link( a , c );
        return( true ); } ,
       []( Boost & b , std::uint32_t x , std::uint32_t y ) {
        return( b.s.find_set( x ) == b.s.find_set( y ) ); } );
#endif
 run( "ADS::RemUnionFind" , [ & ](){
       return( std::make_unique< ADS::RemUnionFind >( n ) ); } ,
      []( ADS::RemUnionFind & u , std::uint32_t x , std::uint32_t y ) {
       return( u.merge( x , y ) ); } ,
      []( ADS::RemUnionFind & u , std::uint32_t x , std::uint32_t y ) {
       return( u.same( x , y ) ); } );
 run( "ADS::UnionFind" , [ & ](){
       return( std::make_unique< ADS::UnionFind >( int( n ) ) ); } ,
      []( ADS::UnionFind & u , std::uint32_t x , std::uint32_t y ) {
       return( u.merge( int( x ) , int( y ) ) ); } ,
      []( ADS::UnionFind & u , std::uint32_t x , std::uint32_t y ) {
       return( u.same( int( x ) , int( y ) ) ); } );
 run( "ADS::MFSet" , [ & ](){
       return( std::make_unique< ADS::MFSet >( int( n ) ) ); } ,
      []( ADS::MFSet & u , std::uint32_t x , std::uint32_t y ) {
       return( u.merge( int( x ) , int( y ) ) ); } ,
      []( ADS::MFSet & u , std::uint32_t x , std::uint32_t y ) {
       return( u.same( int( x ) , int( y ) ) ); } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BOOLSET ----------------------------------*/
/*--------------------------------------------------------------------------*/

void boolset( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::uint32_t > X , Y , T( n );
 for( std::uint32_t i = 0 ; i < n ; ++i ) {
  if( g() % 2 ) X.push_back( i );
  if( g() % 2 ) Y.push_back( i );
  }
 for( auto & t : T ) t = std::uint32_t( g() % n );
 const char * ops[] = { "union" , "intersection" , "difference" ,
                        "symmetric difference" };
 // the cases on two sets a and b, by op( a , b , k ) (the size of the
 // result of the operation k), has( a , x ) and visit( a ) (the sum of
 // the items)
 auto run = [ & ]( const std::string & who , const auto & a ,
                   const auto & b , auto && op , auto && has ,
                   auto && visit ) {
  if( ! measured( who ) )
   return;
  for( int k = 0 ; k < 4 ; ++k )
   print( "boolset" , ops[ k ] , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) { sink = sink + op( a , b , k ); } , r ,
           ( n + 63 ) / 64 ) );
  print( "boolset" , "contains" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::size_t c = 0;
           for( std::uint32_t t : T ) c += has( a , t );
           sink = sink + c; } , r , n ) );
  print( "boolset" , "visit" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) { sink = sink + visit( a ); } , r , n ) );
  };
#ifdef ADS_HAS_BOOST
 if( measured( "boost::dynamic_bitset" ) ||
     measured( "boost::dynamic_bitset A/A" ) ) {
  using D = boost::dynamic_bitset< std::uint64_t >;
  D a( n ) , b( n );
  for( auto x : X ) a.set( x );
  for( auto y : Y ) b.set( y );
  for( const char * who : { "boost::dynamic_bitset" ,
                            "boost::dynamic_bitset A/A" } )
   run( who , a , b , []( const D & p , const D & q , int k ) {
         const D c = k == 0 ? ( p | q ) : k == 1 ? ( p & q ) :
                     k == 2 ? ( p - q ) : ( p ^ q );
         return( c.count() ); } ,
        []( const D & p , std::uint32_t x ) { return( p.test( x ) ); } ,
        []( const D & p ) {
         std::uint64_t s = 0;
         for( auto i = p.find_first() ; i != D::npos ; i = p.find_next( i ) )
          s += i;
         return( s ); } );
  }
#endif
#ifdef ADS_HAS_ROARING
 if( measured( "Roaring" ) ) {
  roaring_bitmap_t * a = roaring_bitmap_of_ptr( X.size() , X.data() );
  roaring_bitmap_t * b = roaring_bitmap_of_ptr( Y.size() , Y.data() );
  run( "Roaring" , a , b , []( const roaring_bitmap_t * p ,
                               const roaring_bitmap_t * q , int k ) {
        roaring_bitmap_t * c = k == 0 ? roaring_bitmap_or( p , q ) :
                               k == 1 ? roaring_bitmap_and( p , q ) :
                               k == 2 ? roaring_bitmap_andnot( p , q ) :
                                        roaring_bitmap_xor( p , q );
        const auto s = roaring_bitmap_get_cardinality( c );
        roaring_bitmap_free( c );
        return( std::size_t( s ) ); } ,
       []( const roaring_bitmap_t * p , std::uint32_t x ) {
        return( roaring_bitmap_contains( p , x ) ); } ,
       []( const roaring_bitmap_t * p ) {
        std::uint64_t s = 0;
        roaring_iterate( p , []( std::uint32_t v , void * q ) {
                          *static_cast< std::uint64_t * >( q ) += v;
                          return( true ); } , &s );
        return( s ); } );
  roaring_bitmap_free( a );
  roaring_bitmap_free( b );
  }
#endif
 if( measured( "ADS::BoolSet" ) ) {
  ADS::BoolSet a{ long( n ) } , b{ long( n ) };
  for( auto x : X ) a.insert( long( x ) );
  for( auto y : Y ) b.insert( long( y ) );
  run( "ADS::BoolSet" , a , b , []( const ADS::BoolSet & p ,
                                     const ADS::BoolSet & q , int k ) {
        const ADS::BoolSet c = k == 0 ? setUnion( p , q ) :
                               k == 1 ? setIntersection( p , q ) :
                               k == 2 ? setDifference( p , q ) :
                                        setSymmetricDifference( p , q );
        return( c.size() ); } ,
       []( const ADS::BoolSet & p , std::uint32_t x ) {
        return( p.contains( long( x ) ) ); } ,
       []( const ADS::BoolSet & p ) {
        std::uint64_t s = 0;
        p.forEach( [ & ]( long x ) { s += std::uint64_t( x ); } );
        return( s ); } );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- BITMAP ----------------------------------*/
/*--------------------------------------------------------------------------*/

void bitmap( std::size_t n , int r , unsigned seed )
{
 const char * ops[] = { "union" , "intersection" , "difference" ,
                        "symmetric difference" };
 for( int d = 0 ; d < 3 ; ++d ) {
  const std::uint64_t u = d == 0 ? 4 * std::uint64_t( n ) :
                          d == 1 ? 32 * std::uint64_t( n ) :
                                   std::min< std::uint64_t >(
                                    1000 * std::uint64_t( n ) ,
                                    0xFFFFFFFFull );
  const char * density = d == 0 ? "dense" : d == 1 ? "medium" : "sparse";
  std::mt19937_64 g( seed + unsigned( d ) );
  const auto X = sample( n , u , g ) , Y = sample( n , u , g );
  std::vector< std::uint32_t > T( n );
  for( auto & t : T ) t = std::uint32_t( g() % u );
  // the cases on two sets a and b built by make( items ), by op( a , b ,
  // k ) (the size of the result of the operation k), has( a , x ) and
  // visit( a ) (the sum of the items)
  auto run = [ & ]( const std::string & who , auto && make , auto && op ,
                    auto && has , auto && visit ) {
   if( ! measured( who ) )
    return;
   const std::string w = std::string( density ) + " ";
   print( "bitmap" , w + "build" , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) { auto a = make( X ); sink = sink + has( a , 0 ); } ,
           r , n ) );
   auto a = make( X ) , b = make( Y );
   for( int k = 0 ; k < 4 ; ++k )
    print( "bitmap" , w + ops[ k ] , n , who , median(
            [](){ return( 0 ); } , [ & ]( int ) {
             sink = sink + op( a , b , k ); } , r , n ) );
   print( "bitmap" , w + "contains" , n , who , median(
           [](){ return( 0 ); } , [ & ]( int ) {
            std::size_t c = 0;
            for( std::uint32_t t : T ) c += has( a , t );
            sink = sink + c; } , r , n ) );
   print( "bitmap" , w + "visit" , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) { sink = sink + visit( a ); } , r , n ) );
   };
#ifdef ADS_HAS_ROARING
  struct Roar {                             // a bitmap and its release
   roaring_bitmap_t * p;
   explicit Roar( roaring_bitmap_t * q ) : p( q ) {}
   Roar( Roar && o ) noexcept : p( std::exchange( o.p , nullptr ) ) {}
   ~Roar() { if( p ) roaring_bitmap_free( p ); }
   };
  for( const char * who : { "Roaring" , "Roaring A/A" } )
   run( who , []( const std::vector< std::uint32_t > & V ) {
         return( Roar( roaring_bitmap_of_ptr( V.size() , V.data() ) ) ); } ,
        []( const Roar & p , const Roar & q , int k ) {
         roaring_bitmap_t * c = k == 0 ? roaring_bitmap_or( p.p , q.p ) :
                                k == 1 ? roaring_bitmap_and( p.p , q.p ) :
                                k == 2 ? roaring_bitmap_andnot( p.p , q.p ) :
                                         roaring_bitmap_xor( p.p , q.p );
         const auto s = roaring_bitmap_get_cardinality( c );
         roaring_bitmap_free( c );
         return( std::size_t( s ) ); } ,
        []( const Roar & p , std::uint32_t x ) {
         return( std::size_t( roaring_bitmap_contains( p.p , x ) ) ); } ,
        []( const Roar & p ) {
         std::uint64_t s = 0;
         roaring_iterate( p.p , []( std::uint32_t v , void * q ) {
                           *static_cast< std::uint64_t * >( q ) += v;
                           return( true ); } , &s );
         return( s ); } );
#endif
  run( "ADS::RoaringBitmap" , []( const std::vector< std::uint32_t > & V ) {
        return( ADS::RoaringBitmap( V.data() , V.size() ) ); } ,
       []( const ADS::RoaringBitmap & p , const ADS::RoaringBitmap & q ,
           int k ) {
        return( ( k == 0 ? setUnion( p , q ) :
                  k == 1 ? setIntersection( p , q ) :
                  k == 2 ? setDifference( p , q ) :
                           setSymmetricDifference( p , q ) ).size() ); } ,
       []( const ADS::RoaringBitmap & p , std::uint32_t x ) {
        return( std::size_t( p.contains( x ) ) ); } ,
       []( const ADS::RoaringBitmap & p ) {
        std::uint64_t s = 0;
        for( const std::uint32_t x : p ) s += x;
        return( s ); } );
  }
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 const char * part = std::getenv( "PART" );
 auto doing = [ & ]( const char * p ) {
  return( ! part || ! *part || ( std::string( part ) == p ) ); };
 for( std::size_t n : o.sizes ) {
  if( doing( "intersection" ) )
   intersection( n , o.runs , o.seed );
  if( doing( "mfset" ) )
   mfset( n , o.runs , o.seed );
  if( doing( "boolset" ) )
   boolset( n , o.runs , o.seed );
  if( doing( "bitmap" ) )
   bitmap( n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File sets.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
