/*--------------------------------------------------------------------------*/
/*--------------------------- File sequences.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sequences and of the stacks through the standard
 * interface: the sequences of ADS against the containers of std and, if
 * Boost is found, of Boost.Container with the same structure
 * (ArraySequence against std::vector, boost::container::vector, devector
 * and small_vector; List against std::list; SinglyList and CursorList
 * against std::forward_list), on n integers. The cases are n insertions
 * at the end (at the front for the singly linked lists), a visit that sums
 * the items, std::sort or the reversal in place by the iterators, and a
 * visit by the positions of the book where it applies. The vectors with
 * room inside themselves (SmallVector against boost::container::small_vector
 * and std::vector): n / 8 vectors of 8 items each made and visited. The
 * stacks
 * (ArrayStack, and SequenceStack on a SinglyList) against std::stack on a
 * std::vector and on a std::deque: n pushes and then n pops. The
 * rearrangements and the sorts of the sequences against the standard
 * library: the Dutch national flag of n random colours against two passes
 * of std::partition, the sort of a vector k-sorted (each item at most
 * k = 16 places from its place) against std::sort, and the natural merge
 * sort of a List against std::list::sort.
 *
 * Each cell is the median of r runs, in nanoseconds per item; the contracts
 * are off (ADS_CONTRACTS=0).
 *
 *   sequences [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sequence/SequenceAlgorithms.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sequence/SmallVector.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/SequenceStack.h"

#if __has_include( <boost/container/devector.hpp> )
 #include <boost/container/devector.hpp>
 #include <boost/container/small_vector.hpp>
 #include <boost/container/vector.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <algorithm>
#include <deque>
#include <forward_list>
#include <list>
#include <numeric>
#include <stack>

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

/// n / 8 vectors V of 8 items, made one after another and then visited
template< class V >
void many( const std::string & who , const std::vector< int > & X , int r )
{
 const std::size_t n = X.size() / 8 * 8;
 print( "sequences" , "8-item vectors" , X.size() , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          std::vector< V > W( n / 8 );
          for( std::size_t i = 0 ; i < n ; ++i )
           W[ i / 8 ].push_back( X[ i ] );
          long t = 0;
          for( const V & v : W )
           for( int x : v ) t += x;
          sink = sink + std::uint64_t( t ); } , r , X.size() ) );
 }

/// n pushes and n pops on the stack that make() builds, by push( s , x )
/// and pop( s ), which gives the top removed
template< class M , class Push , class Pop >
void stack( const std::string & who , const std::vector< int > & X , int r ,
            M && make , Push && push , Pop && pop )
{
 const std::size_t n = X.size();
 print( "sequences" , "stack" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto s = make();
          for( int x : X ) push( s , x );
          long t = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) t += pop( s );
          sink = sink + std::uint64_t( t ); } , r , n ) );
 }

/// a case on a copy of A, of n items, by f( copy ), the copy not timed
template< class V , class F >
void onCopy( const char * what , const std::string & who , const V & A ,
             std::size_t n , int r , F && f )
{
 print( "sequences" , what , n , who , median( [ & ](){
         return( A ); } , [ & ]( V & B ) {
          f( B );
          sink = sink + std::uint64_t( *B.begin() ); } , r , n ) );
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
  auto again = []( auto & s ) {
   sink = sink + std::uint64_t( std::accumulate( s.begin() , s.end() ,
                                                 0L ) ); };

  // the sequences

  for( const char * who : { "std::vector" , "std::vector A/A" } )
   run( who , X , r , [](){ return( std::vector< int >() ); } , back ,
        "std::sort" , sort );
#ifdef ADS_HAS_BOOST
  run( "boost::container::vector" , X , r , [](){
       return( boost::container::vector< int >() ); } , back , "std::sort" ,
       sort );
  run( "boost::container::devector" , X , r , [](){
       return( boost::container::devector< int >() ); } , back ,
       "std::sort" , sort );
  run( "boost::container::small_vector" , X , r , [](){
       return( boost::container::small_vector< int , 16 >() ); } , back ,
       "std::sort" , sort );
#endif
  run( "ADS::ArraySequence" , X , r , [](){
       return( ADS::ArraySequence< int >() ); } , back , "std::sort" , sort );
  run( "std::list" , X , r , [](){ return( std::list< int >() ); } , back ,
       "reverse" , reverse );
  run( "ADS::List" , X , r , [](){ return( ADS::List< int >() ); } , back ,
       "reverse" , reverse );
  run( "std::forward_list" , X , r , [](){
       return( std::forward_list< int >() ); } , front , "visit again" ,
       again );
  run( "ADS::SinglyList" , X , r , [](){
       return( ADS::SinglyList< int >() ); } , front , "visit again" ,
       again );
  run( "ADS::CursorList" , X , r , [](){
       return( ADS::CursorList< int >() ); } , front , "visit again" ,
       again );
  ADS::ArraySequence< int > A( X.begin() , X.end() );
  ADS::List< int > L( X.begin() , X.end() );
  positions( "ADS::ArraySequence" , A , n , r );
  positions( "ADS::List" , L , n , r );

  // the vectors with room inside themselves

  for( const char * who : { "std::vector" , "std::vector A/A" } )
   many< std::vector< int > >( who , X , r );
#ifdef ADS_HAS_BOOST
  many< boost::container::small_vector< int , 8 > >(
   "boost::container::small_vector" , X , r );
#endif
  many< ADS::SmallVector< int , 8 > >( "ADS::SmallVector" , X , r );

  // the stacks

  auto stdPop = []( auto & s ) {
   const int x = s.top();
   s.pop();
   return( x ); };
  auto stdPush = []( auto & s , int x ) { s.push( x ); };
  for( const char * who : { "std::stack<vector>" ,
                            "std::stack<vector> A/A" } )
   stack( who , X , r , [](){
          return( std::stack< int , std::vector< int > >() ); } , stdPush ,
          stdPop );
  stack( "std::stack<deque>" , X , r , [](){
         return( std::stack< int , std::deque< int > >() ); } , stdPush ,
         stdPop );
  stack( "ADS::ArrayStack" , X , r , [ n ](){
         return( ADS::ArrayStack< int >( n ) ); } , stdPush ,
         []( auto & s ) { return( s.pop() ); } );
  stack( "ADS::SequenceStack" , X , r , [](){
         return( ADS::SequenceStack< ADS::SinglyList< int > >() ); } ,
         stdPush , []( auto & s ) { return( s.pop() ); } );

  // the rearrangements and the sorts

  std::vector< int > C( n );                // the colours
  for( auto & c : C ) c = int( g() % 3 );
  auto colour = []( int c ) { return( c ); };
  onCopy( "dutch flag" , "std::partition x2" , C , n , r ,
          [ & ]( std::vector< int > & B ) {
           auto m = std::partition( B.begin() , B.end() , []( int c ) {
                                     return( c == 0 ); } );
           std::partition( m , B.end() , []( int c ) {
                            return( c == 1 ); } ); } );
  onCopy( "dutch flag" , "ADS::dutchNationalFlag" , C , n , r ,
          [ & ]( std::vector< int > & B ) {
           ADS::dutchNationalFlag( B , colour ); } );
  std::vector< int > K( X );                // 16-sorted
  std::sort( K.begin() , K.end() );
  for( std::size_t i = 0 ; i + 16 < n ; i += 16 )
   std::shuffle( K.begin() + std::ptrdiff_t( i ) ,
                 K.begin() + std::ptrdiff_t( i + 16 ) , g );
  onCopy( "k-sorted k=16" , "std::sort" , K , n , r ,
          [ & ]( std::vector< int > & B ) {
           std::sort( B.begin() , B.end() ); } );
  onCopy( "k-sorted k=16" , "ADS::sortKSorted" , K , n , r ,
          [ & ]( std::vector< int > & B ) {
           ADS::sortKSorted( B , 16 ); } );
  const std::list< int > SL( X.begin() , X.end() );
  onCopy( "list sort" , "std::list::sort" , SL , n , r ,
          []( std::list< int > & B ) { B.sort(); } );
  onCopy( "list sort" , "ADS::naturalMergeSort" , L , n , r ,
          []( ADS::List< int > & B ) { ADS::naturalMergeSort( B ); } );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File sequences.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
