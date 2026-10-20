/*--------------------------------------------------------------------------*/
/*---------------------------- File memory.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the allocators of objects: the Arena and the ObjectPool
 * of ADS against new and delete (glibc's malloc), the memory resources of
 * std::pmr (monotonic_buffer_resource, unsynchronized_pool_resource) and,
 * if Boost is found, boost::object_pool and boost::pool, and against
 * mimalloc and jemalloc when ADS_HAS_MIMALLOC and ADS_HAS_JEMALLOC are
 * defined (MIMALLOC and JEMALLOC in the makefile, jemalloc built with the
 * prefix je_ and mimalloc without the override of malloc, so that the
 * rest of the program keeps glibc's), on objects of 32 bytes:
 *
 * - arena: n objects made one after another and written, then all freed
 *   at once (by the arena, the monotonic resource and the object pool of
 *   Boost) or one by one (by the allocators);
 * - churn: n objects alive, then n steps that free one at random and make
 *   another, then all freed one by one: the nodes of a linked structure
 *   that changes (not boost::object_pool, whose free() keeps its list of
 *   free chunks ordered, in time linear in the chunks).
 *
 * Each cell is the median of r runs, in nanoseconds per object (per step
 * of the churn, its filling and emptying included); the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   memory [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/memory/Arena.h"
#include "ADS/memory/ObjectPool.h"

#if __has_include( <boost/pool/object_pool.hpp> )
 #include <boost/pool/object_pool.hpp>
 #include <boost/pool/pool.hpp>
 #define ADS_HAS_BOOST 1
#endif
#ifdef ADS_HAS_MIMALLOC
 #include <mimalloc.h>
#endif
#ifdef ADS_HAS_JEMALLOC
 #include <jemalloc/jemalloc.h>
#endif

#include <memory_resource>
#include <new>

using namespace ADS::bench;

namespace {

/// the object: four words
struct Node {
 std::uint64_t a , b , c , d;
 };

/// the indices of the objects freed by the steps of the churn
std::vector< std::uint32_t > victims;

/// the case arena: n objects by make( i ), then free( all )
template< class M , class Make , class Done >
void arena( const std::string & who , std::size_t n , int r , M && start ,
            Make && make , Done && done )
{
 print( "memory" , "arena" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto s = start();
          std::uint64_t t = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) {
           Node * x = make( s );
           x->a = i;
           x->d = i ^ 7;
           t += x->a;
           }
          done( s );
          sink = sink + t; } , r , n ) );
 }

/// the case churn: n objects by make( s ), n steps of free( s , x ) and
/// make( s ), then all freed by free( s , x )
template< class M , class Make , class Free >
void churn( const std::string & who , std::size_t n , int r , M && start ,
            Make && make , Free && free )
{
 print( "memory" , "churn" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto s = start();
          std::vector< Node * > L( n );
          for( std::size_t i = 0 ; i < n ; ++i ) {
           L[ i ] = make( s );
           L[ i ]->a = i;
           }
          std::uint64_t t = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) {
           Node * & y = L[ victims[ i ] ];
           t += y->a;
           free( s , y );
           y = make( s );
           y->a = i;
           }
          for( Node * x : L )
           free( s , x );
          sink = sink + t; } , r , n ) );
 }

/// the allocators of single objects, by allocate() and deallocate()
template< class A , class D >
void allocators( const std::string & who , std::size_t n , int r ,
                 A && allocate , D && deallocate )
{
 struct None {};
 std::vector< Node * > all;
 arena( who , n , r , [ & ](){
         all.clear();
         all.reserve( n );
         return( None() ); } ,
        [ & ]( None & ) {
         Node * x = static_cast< Node * >( allocate( sizeof( Node ) ) );
         all.push_back( x );
         return( x ); } ,
        [ & ]( None & ) { for( Node * x : all ) deallocate( x ); } );
 churn( who , n , r , [](){ return( None() ); } , [ & ]( None & ) {
         return( static_cast< Node * >( allocate( sizeof( Node ) ) ) ); } ,
        [ & ]( None & , Node * x ) { deallocate( x ); } );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  victims.resize( n );
  for( auto & v : victims ) v = std::uint32_t( g() % n );
  const int r = o.runs;
  for( const char * who : { "new/delete" , "new/delete A/A" } )
   allocators( who , n , r , []( std::size_t b ) {
                return( ::operator new( b ) ); } ,
               []( void * p ) { ::operator delete( p ); } );
#ifdef ADS_HAS_MIMALLOC
  allocators( "mimalloc" , n , r , []( std::size_t b ) {
               return( mi_malloc( b ) ); } ,
              []( void * p ) { mi_free( p ); } );
#endif
#ifdef ADS_HAS_JEMALLOC
  allocators( "jemalloc" , n , r , []( std::size_t b ) {
               return( je_malloc( b ) ); } ,
              []( void * p ) { je_free( p ); } );
#endif

  // all freed at once

  arena( "std::pmr::monotonic_buffer_resource" , n , r , [](){
         return( std::make_unique< std::pmr::monotonic_buffer_resource >() );
         } , []( auto & m ) {
          return( static_cast< Node * >(
                   m->allocate( sizeof( Node ) , alignof( Node ) ) ) ); } ,
         []( auto & m ) { m.reset(); } );
#ifdef ADS_HAS_BOOST
  arena( "boost::object_pool" , n , r , [](){
         return( std::make_unique< boost::object_pool< Node > >() ); } ,
         []( auto & p ) { return( p->malloc() ); } ,
         []( auto & p ) { p.reset(); } );
#endif
  arena( "ADS::Arena" , n , r , [](){
         return( std::make_unique< ADS::Arena< Node > >() ); } ,
         []( auto & a ) { return( a->make() ); } ,
         []( auto & a ) { a.reset(); } );
  arena( "ADS::ObjectPool" , n , r , [](){
         return( std::make_unique< ADS::ObjectPool< Node > >() ); } ,
         []( auto & a ) { return( a->make() ); } ,
         []( auto & a ) { a.reset(); } );

  // freed one by one

  churn( "std::pmr::unsynchronized_pool_resource" , n , r , [](){
         return( std::make_unique< std::pmr::unsynchronized_pool_resource >()
                 ); } , []( auto & m ) {
          return( static_cast< Node * >(
                   m->allocate( sizeof( Node ) , alignof( Node ) ) ) ); } ,
         []( auto & m , Node * x ) {
          m->deallocate( x , sizeof( Node ) , alignof( Node ) ); } );
#ifdef ADS_HAS_BOOST
  churn( "boost::pool" , n , r , [](){
         return( std::make_unique< boost::pool<> >( sizeof( Node ) ) ); } ,
         []( auto & p ) { return( static_cast< Node * >( p->malloc() ) ); } ,
         []( auto & p , Node * x ) { p->free( x ); } );
#endif
  churn( "ADS::ObjectPool" , n , r , [](){
         return( std::make_unique< ADS::ObjectPool< Node > >() ); } ,
         []( auto & a ) { return( a->make() ); } ,
         []( auto & a , Node * x ) { a->free( x ); } );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File memory.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
