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
 * and the difference) and, if PAM is compiled in (-DADS_HAS_PAM with the
 * headers of PAM and of ParlayLib), against pam_map on the threads of
 * ParlayLib (PARLAY_NUM_THREADS = t), whose operations take their
 * arguments by move, as those of ADS do.
 *
 * Each cell is the median of r runs, in nanoseconds per key of the two
 * trees (per pair of the batch for the insertion and the construction),
 * of the operation alone: the trees are built before each run, not timed.
 * The benchmark of a cell is "joins <operation> t<t>"; its first structure
 * runs twice, the second time as "A/A", and ADS_BENCH_WHO and
 * ADS_BENCH_LIST pick the structures [see Bench.h], so that run.sh runs
 * each in a process of its own. A result that differs from the one of
 * std::map gets -1.
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

/// the checksum of the keys of a tree of ADS
template< class T >
std::uint64_t sum( const T & t )
{
 std::uint64_t s = 0;
 t.forEach( [ & ]( const Key & k , const Key & ) { s += k; } );
 return( s + t.size() );
 }

/// the checksum of the keys of a std::map
std::uint64_t sum( const std::map< Key , Key > & m )
{
 std::uint64_t s = 0;
 for( const auto & kv : m ) s += kv.first;
 return( s + m.size() );
 }

/// the operations of the cells, by name
const char * const kOps[] = { "union" , "intersection" , "difference" ,
                              "multi-insert" , "build" };

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
     return( std::make_pair( std::move( x ) , std::move( y ) ) );
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
     got = sum( x );
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
      return( std::make_pair( std::move( x ) , std::move( y ) ) );
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
      got = sum( x );
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
#if defined( ADS_HAS_PAM )
   if( ! skip( "PAM" ) ) {
    struct Entry {
     using key_t = Key;
     using val_t = Key;
     static bool comp( const key_t & a , const key_t & b ) {
      return( a < b );
      }
     };
    using M = pam_map< Entry >;
    for( int op = 0 ; op < 5 ; ++op ) {
     std::uint64_t got = 0;
     auto prep = [ & ]() {
      auto x = std::make_unique< M >() , y = std::make_unique< M >();
      if( op != 4 ) *x = M( in.a.begin() , in.a.end() );
      if( op < 3 ) *y = M( in.b.begin() , in.b.end() );
      return( std::make_pair( std::move( x ) , std::move( y ) ) );
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
       case 3: x = M::multi_insert( std::move( x ) , in.b ); break;
       default: x = M( in.a.begin() , in.a.end() ); break;
       }
      std::uint64_t r = x.size();
      M::foreach_seq( x , [ & ]( const auto & e ) { r += e.first; } );
      got = r;
      };
     const std::size_t ops = op == 3 ? m : ( op == 4 ? n : n + m );
     const double ns = median( prep , work , o.runs , ops );
     print( ( std::string( "joins " ) + kOps[ op ] + " t" +
              std::to_string( t ) ).c_str() ,
            "m=" + std::to_string( m ) , n , "PAM" ,
            got == want[ op ] ? ns : -1 );
     }
    }
#endif
   }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File joins.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
