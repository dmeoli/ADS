/*--------------------------------------------------------------------------*/
/*---------------------------- File joins.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the bulk operations of the trees based on join, on t
 * threads (-t, 1 by default): the union, the intersection and the
 * difference of a tree of n distinct random keys of 64 bits with one of m
 * keys (m = n and m = n / 100, half of the small one in the large one),
 * the insertion of a sorted batch of m pairs into a tree of n, and the
 * construction of a tree of n pairs from a sorted batch, on the AVL tree,
 * the Red-Black tree and the treap of ADS and on the four schemes of
 * JoinTree, sequential (no executor) and on the pool of ADS of t threads
 * ("pool"), against std::map, whose operations go one key at a time
 * (merge() for the union, the hinted insertions of the batch, the lookups
 * of the keys of the smaller map in the larger one for the intersection
 * and the difference). The maps of JoinTree augmented by the sum of the
 * values ("aug") run two cells more: the union of the two trees, which
 * keeps the sums, and m sums of the values in ranges of keys by augRange()
 * (by parallelFor() on the pool). If PAM is compiled in (-DADS_HAS_PAM
 * with the headers of PAM and of ParlayLib), pam_map and aug_map on its
 * four schemes (AVL, RB, WB, treap) run the same cells on the threads of
 * ParlayLib (PARLAY_NUM_THREADS = t), their operations taking their
 * arguments by move, as those of ADS do, and the sorted batches inserted
 * by the sorted insertion of PAM, as multi_insert() would sort them first.
 *
 * Each cell is the median of r runs, in nanoseconds per key of the two
 * trees (per pair of the batch for the insertion and the construction),
 * of the operation alone: the trees are built before each run, and checked
 * and freed after it, out of the time.
 * The benchmark of a cell is "joins <operation> t<t>"; its first structure
 * runs twice, the second time as "A/A", and ADS_BENCH_WHO and
 * ADS_BENCH_LIST pick the structures [see Bench.h], so that run.sh runs
 * each in a process of its own. A result that differs from the one of
 * std::map, or from the sums by brute force, gets -1.
 *
 *   joins [ -r runs ] [ -s seed ] [ -t threads ] n1 n2 ...
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

#include "ADS/parallel/ThreadPool.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/JoinTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Treap.h"

#if defined( ADS_HAS_PAM )
 #include <pam/pam.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;
using namespace ADS::bench;

namespace {

using Key = std::uint64_t;
using Batch = std::vector< std::pair< Key , Key > >;

/// the inputs of a cell: the large and the small sorted batches
struct Input {
 Batch a , b;
 };

/// a = n keys, b = m keys of which half in a, both sorted
Input input( std::size_t n , std::size_t m , unsigned seed )
{
 const auto K = keys( n + m , seed );
 Input in;
 for( std::size_t i = 0 ; i < n ; ++i )
  in.a.emplace_back( K[ i ] , i );
 for( std::size_t i = 0 ; i < m ; ++i )
  in.b.emplace_back( i % 2 ? K[ i ] : K[ n + i ] , i );
 std::sort( in.a.begin() , in.a.end() );
 std::sort( in.b.begin() , in.b.end() );
 return( in );
 }

/// the checksum of the keys of a tree of ADS (or of a pam_map)
template< class T >
std::uint64_t sum( const T & t )
{
 std::uint64_t s = 0;
 if constexpr( requires { t.forEach( []( const Key & , const Key & ) {} ); } )
  t.forEach( [ & ]( const Key & k , const Key & ) { s += k; } );
 else                                       // a map of PAM, by a copy
  for( const auto & k : T::keys( T( t ) ) ) s += k;
 return( s + t.size() );
 }

/// the checksum of the keys of a std::map
std::uint64_t sum( const std::map< Key , Key > & m )
{
 std::uint64_t s = 0;
 for( const auto & kv : m ) s += kv.first;
 return( s + m.size() );
 }

/// the two trees of a run, whose checksum goes to *got when the run is
/// over: the destructor runs after the clock of median() has stopped, so
/// that the visit of the whole result is not timed
template< class T >
struct Run {
 std::unique_ptr< T > first , second;   ///< the two trees
 std::uint64_t * got;                   ///< where the checksum goes
 ~Run() { if( first ) *got = sum( *first ); }
 };

/// the operations of the cells, by name
const char * const kOps[] = { "union" , "intersection" , "difference" ,
                              "multi-insert" , "build" , "aug-union" ,
                              "aug-range" };

/// the ranges of the cell aug-range: m pairs of keys of b, lo <= hi
std::vector< std::pair< Key , Key > > queries( const Input & in )
{
 const std::size_t m = in.b.size();
 std::vector< std::pair< Key , Key > > Q( m );
 for( std::size_t i = 0 ; i < m ; ++i ) {
  Key lo = in.b[ i ].first , hi = in.b[ ( i * 7919 + 13 ) % m ].first;
  if( hi < lo ) std::swap( lo , hi );
  Q[ i ] = { lo , hi };
  }
 return( Q );
 }

/// the checksums of aug-union (the sum of the values of the union, those of
/// b on the keys of both, plus its size) and of aug-range (the sum of the
/// sums of the values of a in the ranges of Q), by brute force
std::pair< std::uint64_t , std::uint64_t > augWant( const Input & in ,
                 const std::vector< std::pair< Key , Key > > & Q )
{
 std::map< Key , Key > u( in.a.begin() , in.a.end() );
 for( const auto & kv : in.b ) u[ kv.first ] = kv.second;
 std::uint64_t su = u.size();
 for( const auto & kv : u ) su += kv.second;
 std::vector< std::uint64_t > P( in.a.size() + 1 , 0 );   // prefix sums
 for( std::size_t i = 0 ; i < in.a.size() ; ++i )
  P[ i + 1 ] = P[ i ] + in.a[ i ].second;
 auto below = [ & ]( Key k ) {                 // the pairs of a with key < k
  return( std::size_t( std::lower_bound( in.a.begin() , in.a.end() ,
           std::make_pair( k , Key( 0 ) ) ) - in.a.begin() ) );
  };
 std::uint64_t sr = 0;
 for( const auto & [ lo , hi ] : Q ) {
  const std::size_t i = below( lo ) ,
   j = hi == ~Key( 0 ) ? in.a.size() : below( hi + 1 );
  sr += P[ j ] - P[ i ];
  }
 return( std::make_pair( su , sr ) );
 }

/// the cells aug-union and aug-range of the map of ADS of scheme B,
/// augmented by the sum of the values, sequential and on the pool (the
/// queries of aug-range by parallelFor())
template< class B >
void adsAugCells( const std::string & who , ThreadPool & pool , int t ,
                  const Input & in , std::size_t n , std::size_t m ,
                  const std::vector< std::pair< Key , Key > > & Q ,
                  std::pair< std::uint64_t , std::uint64_t > want ,
                  int runs )
{
 using T = JoinTree< Key , Key , B , SumAugmentation< Key > >;
 for( int par = 0 ; par < 2 ; ++par ) {
  const std::string name = who + ( par ? " pool" : "" );
  if( skip( name ) )
   continue;
  for( int op = 5 ; op < 7 ; ++op ) {
   std::uint64_t got = 0 , aug = 0;
   auto prep = [ & ]() {
    auto x = std::make_unique< T >() , y = std::make_unique< T >();
    x->multiInsert( in.a );
    if( op == 5 ) y->multiInsert( in.b );
    return( Run< T >{ std::move( x ) , std::move( y ) , &got } );
    };
   auto work = [ & ]( auto & s ) {
    T & x = *s.first;
    if( op == 5 ) {
     if( par ) x.unionWith( pool , std::move( *s.second ) );
     else x.unionWith( std::move( *s.second ) );
     aug = x.augValue() + x.size();
     return;
     }
    std::vector< Key > R( Q.size() );
    auto q = [ & ]( std::size_t i ) {
     R[ i ] = x.augRange( Q[ i ].first , Q[ i ].second ); };
    if( par ) parallelFor( pool , 0 , Q.size() , q );
    else for( std::size_t i = 0 ; i < Q.size() ; ++i ) q( i );
    aug = 0;
    for( const Key r : R ) aug += r;
    };
   const std::size_t ops = op == 5 ? n + m : m;
   const double ns = median( prep , work , runs , ops );
   print( ( std::string( "joins " ) + kOps[ op ] + " t" +
            std::to_string( t ) ).c_str() ,
          "m=" + std::to_string( m ) , n , name ,
          aug == ( op == 5 ? want.first : want.second ) ? ns : -1 );
   }
  }
 }

#if defined( ADS_HAS_PAM )

/// an entry of pam_map: keys and values of 64 bits; the priority of a key
/// in a treap of PAM is its hash
struct PamEntry {
 using key_t = Key;
 using val_t = Key;
 static bool comp( const key_t & a , const key_t & b ) { return( a < b ); }
 static std::size_t hash( const std::pair< key_t , val_t > & e ) {
  return( parlay::hash64( e.first ) );
  }
 };

/// an entry of aug_map: PamEntry augmented by the sum of the values
struct PamSumEntry : PamEntry {
 using aug_t = Key;
 static aug_t get_empty( void ) { return( 0 ); }
 static aug_t from_entry( const key_t & , const val_t & v ) { return( v ); }
 static aug_t combine( aug_t a , aug_t b ) { return( a + b ); }
 };

/// the batch B inserted in m by the sorted insertion of PAM, as the batch
/// is sorted (multi_insert() of PAM sorts it first), the value of B on a
/// key of both
template< class M >
M pamInsert( M m , const parlay::sequence< std::pair< Key , Key > > & B )
{
 auto replace = []( const Key & , const Key & b ) { return( b ); };
 auto * A = const_cast< std::pair< Key , Key > * >( B.data() );
 return( M( M::Tree::multi_insert_sorted( m.get_root() , A , B.size() ,
                                          replace ) ) );
 }

/// the cells of pam_map and of aug_map of the schemes BM and BA (the same
/// but for the treap, whose scheme names its entry), on the threads of
/// ParlayLib
/// (PARLAY_NUM_THREADS), whose operations take their arguments by move as
/// those of ADS do; the construction by from_sorted() and the insertion of
/// a batch by the sorted insertion, as for ADS
template< class BM , class BA >
void pamCells( const std::string & who , int t , const Input & in ,
               std::size_t n , std::size_t m ,
               const std::vector< std::uint64_t > & want ,
               const std::vector< std::pair< Key , Key > > & Q ,
               std::pair< std::uint64_t , std::uint64_t > augw , int runs )
{
 using M = pam_map< PamEntry , BM >;
 using A = aug_map< PamSumEntry , BA >;
 using S = parlay::sequence< std::pair< Key , Key > >;
 const S a( in.a.begin() , in.a.end() ) , b( in.b.begin() , in.b.end() );
 if( ! skip( who ) )
  for( int op = 0 ; op < 5 ; ++op ) {
   std::uint64_t got = 0;
   auto prep = [ & ]() {
    auto x = std::make_unique< M >() , y = std::make_unique< M >();
    if( op != 4 ) *x = pamInsert( M() , a );
    if( op < 3 ) *y = pamInsert( M() , b );
    return( Run< M >{ std::move( x ) , std::move( y ) , &got } );
    };
   auto work = [ & ]( auto & s ) {
    M & x = *s.first;
    M & y = *s.second;
    switch( op ) {
     case 0: x = M::map_union( std::move( x ) , std::move( y ) ); break;
     case 1: x = M::map_intersect( std::move( x ) , std::move( y ) );
             break;
     case 2: x = M::map_difference( std::move( x ) , std::move( y ) );
             break;
     case 3: x = pamInsert( std::move( x ) , b ); break;
     default: {
      S c = a;                       // from_sorted() takes it by reference
      x = M::from_sorted( c );
      break;
      }
     }
    };
   const std::size_t ops = op == 3 ? m : ( op == 4 ? n : n + m );
   const double ns = median( prep , work , runs , ops );
   print( ( std::string( "joins " ) + kOps[ op ] + " t" +
            std::to_string( t ) ).c_str() ,
          "m=" + std::to_string( m ) , n , who ,
          got == want[ op ] ? ns : -1 );
   }
 if( skip( who + " aug" ) )
  return;
 for( int op = 5 ; op < 7 ; ++op ) {
  std::uint64_t got = 0 , aug = 0;
  auto prep = [ & ]() {
   auto x = std::make_unique< A >() , y = std::make_unique< A >();
   *x = pamInsert( A() , a );
   if( op == 5 ) *y = pamInsert( A() , b );
   return( Run< A >{ std::move( x ) , std::move( y ) , &got } );
   };
  auto work = [ & ]( auto & s ) {
   A & x = *s.first;
   if( op == 5 ) {
    x = A::map_union( std::move( x ) , std::move( *s.second ) );
    aug = x.aug_val() + x.size();
    return;
    }
   parlay::sequence< Key > R( Q.size() );
   parlay::parallel_for( 0 , Q.size() , [ & ]( std::size_t i ) {
    R[ i ] = x.aug_range( Q[ i ].first , Q[ i ].second ); } );
   aug = 0;
   for( const Key r : R ) aug += r;
   };
  const std::size_t ops = op == 5 ? n + m : m;
  const double ns = median( prep , work , runs , ops );
  print( ( std::string( "joins " ) + kOps[ op ] + " t" +
           std::to_string( t ) ).c_str() ,
         "m=" + std::to_string( m ) , n , who + " aug" ,
         aug == ( op == 5 ? augw.first : augw.second ) ? ns : -1 );
  }
 }

#endif

/// the cells of a tree T of ADS, sequential and on the pool
template< class T >
void adsCells( const std::string & who , ThreadPool & pool , int t ,
               const Input & in , std::size_t n , std::size_t m ,
               const std::vector< std::uint64_t > & want , int runs )
{
 for( int par = 0 ; par < 2 ; ++par ) {
  const std::string w = who + ( par ? " pool" : "" );
  {
   const std::string name = w;
   if( skip( name ) )
    continue;
   for( int op = 0 ; op < 5 ; ++op ) {
    std::uint64_t got = 0;
    auto prep = [ & ]() {
     auto x = std::make_unique< T >() , y = std::make_unique< T >();
     if( op != 4 ) x->multiInsert( in.a );
     if( op < 3 ) y->multiInsert( in.b );
     return( Run< T >{ std::move( x ) , std::move( y ) , &got } );
     };
    auto work = [ & ]( auto & s ) {
     T & x = *s.first;
     T & y = *s.second;
     auto go = [ & ]( auto & ex ) {
      switch( op ) {
       case 0: x.unionWith( ex , std::move( y ) ); break;
       case 1: x.intersectWith( ex , std::move( y ) ); break;
       case 2: x.differenceWith( ex , std::move( y ) ); break;
       case 3: x.multiInsert( ex , in.b ); break;
       default: x.multiInsert( ex , in.a ); break;
       }
      };
     if( par ) go( pool );
     else {
      Sequential seq;
      go( seq );
      }
     };
    const std::size_t ops = op == 3 ? m : ( op == 4 ? n : n + m );
    const double ns = median( prep , work , runs , ops );
    print( ( std::string( "joins " ) + kOps[ op ] + " t" +
             std::to_string( t ) ).c_str() ,
           "m=" + std::to_string( m ) , n , name ,
           got == want[ op ] ? ns : -1 );
    }
   }
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 int t = 1;
 std::vector< char * > rest = { argv[ 0 ] };
 for( int i = 1 ; i < argc ; ++i )
  if( ( std::string( argv[ i ] ) == "-t" ) && ( i + 1 < argc ) )
   t = std::atoi( argv[ ++i ] );
  else
   rest.push_back( argv[ i ] );
 const Options o( int( rest.size() ) , rest.data() , { 1000000 } );
 ThreadPool pool( std::size_t( std::max( t , 1 ) ) );

 for( const std::size_t n : o.sizes )
  for( const std::size_t m : { n , n / 100 } ) {
   const Input in = input( n , m , o.seed );
   // the results of std::map, which the others must give
   std::vector< std::uint64_t > want( 5 );
   {
    std::map< Key , Key > a( in.a.begin() , in.a.end() ) ,
                          b( in.b.begin() , in.b.end() );
    std::map< Key , Key > u = a , i , d;
    u.insert( b.begin() , b.end() );
    for( const auto & kv : a )
     ( b.contains( kv.first ) ? i : d ).insert( kv );
    want = { sum( u ) , sum( i ) , sum( d ) , sum( u ) , sum( a ) };
    }
   // std::map, one key at a time, run twice (the second as "A/A")
   for( const std::string who : { "std::map" , "A/A" } )
    if( ! skip( who ) )
    for( int op = 0 ; op < 5 ; ++op ) {
     std::uint64_t got = 0;
     auto prep = [ & ]() {
      auto x = std::make_unique< std::map< Key , Key > >() ,
           y = std::make_unique< std::map< Key , Key > >();
      if( op != 4 ) x->insert( in.a.begin() , in.a.end() );
      if( op < 3 ) y->insert( in.b.begin() , in.b.end() );
      return( Run< std::map< Key , Key > >{ std::move( x ) , std::move( y ) ,
                                            &got } );
      };
     auto work = [ & ]( auto & s ) {
      auto & x = *s.first;
      auto & y = *s.second;
      switch( op ) {
       case 0: x.merge( y ); break;
       case 1:
       case 2: {
        auto & big = x.size() >= y.size() ? x : y;
        auto & small = x.size() >= y.size() ? y : x;
        std::map< Key , Key > r;
        if( op == 1 ) {
         for( const auto & kv : small )
          if( big.contains( kv.first ) )
           r.emplace_hint( r.end() , kv.first , x.at( kv.first ) );
         x.swap( r );
         }
        else
         for( const auto & kv : y )
          x.erase( kv.first );
        break;
        }
       case 3:
        for( const auto & kv : in.b )
         x.insert_or_assign( kv.first , kv.second );
        break;
       default:
        x.insert( in.a.begin() , in.a.end() );
        break;
       }
      };
     const std::size_t ops = op == 3 ? m : ( op == 4 ? n : n + m );
     const double ns = median( prep , work , o.runs , ops );
     print( ( std::string( "joins " ) + kOps[ op ] + " t" +
              std::to_string( t ) ).c_str() ,
            "m=" + std::to_string( m ) , n , who ,
            got == want[ op ] ? ns : -1 );
     }
   adsCells< AVLTree< Key , Key > >( "AVLTree" , pool , t , in , n , m ,
                                     want , o.runs );
   adsCells< RedBlackTree< Key , Key > >( "RedBlackTree" , pool , t , in ,
                                          n , m , want , o.runs );
   adsCells< Treap< Key , Key > >( "Treap" , pool , t , in , n , m , want ,
                                   o.runs );
   adsCells< JoinAVLTree< Key , Key > >( "JoinAVLTree" , pool , t , in , n ,
                                         m , want , o.runs );
   adsCells< JoinRedBlackTree< Key , Key > >( "JoinRedBlackTree" , pool , t ,
                                              in , n , m , want , o.runs );
   adsCells< WeightBalancedTree< Key , Key > >( "WeightBalancedTree" , pool ,
                                                t , in , n , m , want ,
                                                o.runs );
   adsCells< JoinTreap< Key , Key > >( "JoinTreap" , pool , t , in , n , m ,
                                       want , o.runs );
   const auto Q = queries( in );
   const auto augw = augWant( in , Q );
   adsAugCells< AVLBalance >( "JoinAVLTree aug" , pool , t , in , n , m , Q ,
                              augw , o.runs );
   adsAugCells< RedBlackBalance >( "JoinRedBlackTree aug" , pool , t , in ,
                                   n , m , Q , augw , o.runs );
   adsAugCells< WeightBalance >( "WeightBalancedTree aug" , pool , t , in ,
                                 n , m , Q , augw , o.runs );
   adsAugCells< TreapBalance >( "JoinTreap aug" , pool , t , in , n , m , Q ,
                                augw , o.runs );
#if defined( ADS_HAS_PAM )
   pamCells< avl_tree , avl_tree >( "PAM AVL" , t , in , n , m , want , Q ,
                                    augw , o.runs );
   pamCells< red_black_tree , red_black_tree >( "PAM RB" , t , in , n , m ,
                                                want , Q , augw , o.runs );
   pamCells< weight_balanced_tree , weight_balanced_tree >( "PAM WB" , t ,
                                    in , n , m , want , Q , augw , o.runs );
   pamCells< treap< PamEntry > , treap< PamSumEntry > >( "PAM treap" , t ,
                                    in , n , m , want , Q , augw , o.runs );
#endif
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File joins.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
