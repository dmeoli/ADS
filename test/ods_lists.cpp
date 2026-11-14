/*--------------------------------------------------------------------------*/
/*--------------------------- File ods_lists.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the interfaces, the lists on arrays and the linked lists of
 * chapters 1, 2 and 3 of Morin's Open Data Structures, with their
 * exercises: each test is named after the section or the exercise it
 * covers ("ods-2.4", "ods-ex2.10"), checks the structure against the
 * standard library on random sequences of operations (at the ends, in the
 * middle and anywhere) and checks numerically the bounds that the
 * exercises claim (the places wasted, the balance, the uniformity of a
 * draw); at the level 3 of the contracts.
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

#include "ADSTest.h"

#include "ADS/ADT.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/queue/MinDeque.h"
#include "ADS/queue/MinQueue.h"
#include "ADS/queue/RandomQueue.h"
#include "ADS/queue/RingDeque.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/ContiguousDeque.h"
#include "ADS/sequence/CubishArrayStack.h"
#include "ADS/sequence/DualArrayDeque.h"
#include "ADS/sequence/IndexedList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/RootishArrayStack.h"
#include "ADS/sequence/SEList.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sequence/TieredVector.h"
#include "ADS/sequence/Treque.h"
#include "ADS/sequence/XorList.h"
#include "ADS/set/Bag.h"
#include "ADS/set/SetInterfaces.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/MinStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/stack/StackAlgorithms.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <random>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an item that is not trivially copyable, so that the paths of the moves
/// that build and destroy run
std::string item( std::uint64_t k )
{
 return( std::to_string( k ) + std::string( k % 7 , 'x' ) );
 }

/// the items of \f$ l \f$ equal to those of \f$ r \f$, by rank and by the
/// iterators
template< class L , class R >
bool same( L & l , const R & r )
{
 if( l.size() != r.size() )
  return( false );
 std::size_t k = 0;
 for( const auto & x : l )
  if( !( x == r[ k++ ] ) )
   return( false );
 for( std::size_t i = 0 ; i < r.size() ; i += 1 + r.size() / 17 )
  if( !( l[ i ] == r[ i ] ) )
   return( false );
 return( true );
 }

/// \f$ m \f$ random insertions and removals at rank on the LIST `L`
/// against `std::deque`: at the front, at the back, in the middle and
/// anywhere, insertions with probability \f$ p / 10 \f$
template< class L >
void listRandom( L & l , std::size_t m , int p , std::uint64_t seed ,
                 bool front = true )
{
 std::mt19937_64 g( seed );
 std::deque< std::string > r( l.begin() , l.end() );   // what l holds
 for( std::size_t t = 0 ; t < m ; ++t ) {
  const std::size_t n = r.size();
  const int w = int( g() % 4 );
  if( ( n == 0 ) || ( int( g() % 10 ) < p ) ) {
   std::size_t i = w == 0 ? 0 : w == 1 ? n : w == 2 ? n / 2 : g() % ( n + 1 );
   if( ! front && ( w == 0 ) ) i = n;
   const std::string x = item( g() % 100000 );
   l.insert( i , x );
   r.insert( r.begin() + std::ptrdiff_t( i ) , x );
   }
  else {
   std::size_t i = w == 0 ? 0 : w == 1 ? n - 1 : w == 2 ? n / 2 : g() % n;
   if( ! front && ( w == 0 ) ) i = n - 1;
   l.erase( i );
   r.erase( r.begin() + std::ptrdiff_t( i ) );
   }
  if( ( t % 997 == 0 ) || ( t + 1 == m ) )
   CHECK( same( l , r ) );
  }
 }

/// random operations at the two ends of the double ended queue `D` against
/// `std::deque`, insertions with probability \f$ p / 10 \f$
template< class D >
void dequeRandom( D & d , std::size_t m , int p , std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 std::deque< std::string > r( d.begin() , d.end() );
 for( std::size_t t = 0 ; t < m ; ++t ) {
  const bool atFront = g() % 2;
  if( r.empty() || ( int( g() % 10 ) < p ) ) {
   const std::string x = item( g() % 100000 );
   if( atFront ) { d.push_front( x ); r.push_front( x ); }
   else { d.push_back( x ); r.push_back( x ); }
   }
  else if( atFront ) { d.pop_front(); r.pop_front(); }
  else { d.pop_back(); r.pop_back(); }
  CHECK( d.size() == r.size() );
  if( ! r.empty() )
   CHECK( ( d.front() == r.front() ) && ( d.back() == r.back() ) );
  }
 CHECK( std::equal( d.begin() , d.end() , r.begin() , r.end() ) );
 }

/// a record equal to another by its key alone, for the bag
struct Record {
 int key;          ///< the key, which the equality reads
 int payload;      ///< the rest, which it ignores
 bool operator==( const Record & o ) const { return( key == o.key ); }
 };

/// the hash of a record, its key
struct RecordHash {
 std::uint64_t operator()( const Record & r ) const {
  return( std::uint64_t( r.key ) * 0x9E3779B97F4A7C15ULL );
  }
 };

/*--------------------------------------------------------------------------*/
/*--------------------------- CHAPTER 1 ------------------------------------*/
/*--------------------------------------------------------------------------*/

// the interfaces of section 1.2, as concepts, met by the realizations
static_assert( IndexedList< RingDeque< int > > );
static_assert( IndexedList< DualArrayDeque< int > > );
static_assert(
 IndexedList< DualArrayDeque< int , RootishArrayStack< int > > > );
static_assert( IndexedList< Treque< int > > );
static_assert( IndexedList< ContiguousDeque< int > > );
static_assert( IndexedList< RootishArrayStack< int > > );
static_assert( IndexedList< TieredVector< int > > );
static_assert( IndexedList< CubishArrayStack< int > > );
static_assert( IndexedList< SEList< int > > );
static_assert( IndexedDeque< RingDeque< int > > );
static_assert( IndexedDeque< DualArrayDeque< int > > );
static_assert( IndexedDeque< Treque< int > > );
static_assert( IndexedDeque< ContiguousDeque< int > > );
static_assert( IndexedDeque< TieredVector< int > > );
static_assert( IndexedDeque< SEList< int > > );
static_assert( IndexedDeque< XorList< int > > );
static_assert( USet< ArrayUSet< int > > );
static_assert( SSet< ArraySSet< int > > );
static_assert( USet< Bag< int > > );
static_assert( Stack< MinStack< int > > );
static_assert( Queue< MinQueue< int > > );

ADS_TEST( "ods-1.2.interfaces" )
{
 // the same LIST through the concept, on every realization
 auto run = []< IndexedList L >( L l ) {
  for( int i = 0 ; i < 50 ; ++i ) l.insert( l.size() / 2 , i );
  std::vector< int > r;
  for( int i = 0 ; i < 50 ; ++i ) r.insert( r.begin() + r.size() / 2 , i );
  for( std::size_t i = 0 ; i < r.size() ; ++i ) CHECK( l[ i ] == r[ i ] );
  };
 run( RingDeque< int >() );
 run( DualArrayDeque< int >() );
 run( Treque< int >() );
 run( ContiguousDeque< int >() );
 run( RootishArrayStack< int >() );
 run( TieredVector< int >() );
 run( CubishArrayStack< int >() );
 run( SEList< int >( 3 ) );
 }

ADS_TEST( "ods-ex1.1.lines" )
{
 // the nine problems of exercise 1.1, each with the structure it calls
 // for, against a direct solution
 std::mt19937_64 g( 11 );
 std::vector< std::string > in;
 for( int k = 0 ; k < 1000 ; ++k )
  in.push_back( g() % 9 == 0 ? std::string() : item( g() % 300 ) );
 // 1. the lines in the reverse order: a stack
 {
  SinglyList< std::string > s;
  for( const auto & x : in ) s.push_front( x );
  CHECK( std::equal( s.begin() , s.end() , in.rbegin() , in.rend() ) );
  }
 // 2. blocks of 50 reversed, no more than 50 lines kept: a stack of 50
 {
  std::vector< std::string > out , ref;
  ArrayStack< std::string > s( 50 );
  for( const auto & x : in ) {
   s.push( x );
   if( s.size() == 50 ) while( ! s.empty() ) out.push_back( s.pop() );
   }
  while( ! s.empty() ) out.push_back( s.pop() );
  for( std::size_t b = 0 ; b < in.size() ; b += 50 )
   for( std::size_t k = std::min( in.size() , b + 50 ) ; k > b ; --k )
    ref.push_back( in[ k - 1 ] );
  CHECK( out == ref );
  }
 // 3. a blank line gives the line 42 before it, 43 lines kept: a queue
 {
  std::vector< std::string > out , ref;
  RingDeque< std::string > q;
  for( const auto & x : in ) {
   q.push_back( x );
   if( q.size() > 43 ) q.pop_front();
   if( x.empty() && ( q.size() == 43 ) ) out.push_back( q.front() );
   }
  for( std::size_t k = 42 ; k < in.size() ; ++k )
   if( in[ k ].empty() ) ref.push_back( in[ k - 42 ] );
  CHECK( out == ref && q.capacity() <= 64 );
  }
 // 4. and 5. the first and the later occurrences: a USet, a bag
 {
  std::vector< std::string > first , later , rf , rl;
  Bag< std::string > b;
  for( const auto & x : in ) {
   ( b.count( x ) ? later : first ).push_back( x );
   b.insert( x );
   }
  std::set< std::string > seen;
  for( const auto & x : in )
   ( seen.insert( x ).second ? rf : rl ).push_back( x );
  CHECK( first == rf && later == rl && b.distinct() == seen.size() );
  }
 // 6. and 7. sorted by length, then in order, once and with repetitions:
 // an SSet and a bag
 {
  auto shorter = []( const std::string & a , const std::string & b ) {
   return( a.size() != b.size() ? a.size() < b.size() : a < b );
   };
  ArraySSet< std::string , decltype( shorter ) > s( shorter );
  Bag< std::string > b;
  for( const auto & x : in ) { s.insert( x ); b.insert( x ); }
  std::vector< std::string > once( s.items() ) , all , ref( in );
  for( const auto & x : s.items() )
   for( std::size_t k = b.count( x ) ; k > 0 ; --k ) all.push_back( x );
  std::sort( ref.begin() , ref.end() , shorter );
  CHECK( all == ref );
  ref.erase( std::unique( ref.begin() , ref.end() ) , ref.end() );
  CHECK( once == ref );
  }
 // 8. the even lines, then the odd ones: two queues
 {
  RingDeque< std::string > even , odd;
  for( std::size_t k = 0 ; k < in.size() ; ++k )
   ( k % 2 ? odd : even ).push_back( in[ k ] );
  std::vector< std::string > out( even.begin() , even.end() ) , ref;
  out.insert( out.end() , odd.begin() , odd.end() );
  for( std::size_t k = 0 ; k < in.size() ; k += 2 ) ref.push_back( in[ k ] );
  for( std::size_t k = 1 ; k < in.size() ; k += 2 ) ref.push_back( in[ k ] );
  CHECK( out == ref );
  }
 // 9. a random permutation: a random queue
 {
  RandomQueue< std::string > q( 5 );
  for( const auto & x : in ) q.push( x );
  std::vector< std::string > out;
  while( ! q.empty() ) out.push_back( q.pop() );
  CHECK( out != in && std::is_permutation( out.begin() , out.end() ,
                                           in.begin() , in.end() ) );
  }
 }

ADS_TEST( "ods-ex1.2.dyck" )
{
 std::mt19937_64 g( 2 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  std::vector< int > w( g() % 20 );
  for( int & c : w ) c = g() % 5 < 3 ? 1 : -1;
  long s = 0;
  bool prefix = true , stack = true;
  for( int c : w ) prefix = prefix && ( ( s += c ) >= 0 );
  ArrayStack< int > S( w.size() + 1 );           // the stack itself
  for( int c : w )
   if( c == 1 ) S.push( 0 );
   else if( S.empty() ) { stack = false; break; }
   else ( void )S.pop();
  CHECK( isDyckWord( w ) == prefix && prefix == stack );
  }
 const std::vector< int > yes{ 1 , -1 , 1 , -1 } , no{ 1 , -1 , -1 , 1 };
 CHECK( isDyckWord( yes ) && ! isDyckWord( no ) );
 }

ADS_TEST( "ods-ex1.3.matched" )
{
 CHECK( isMatched( "{{()[]}}" ) && ! isMatched( "{{()]}" ) );
 CHECK( isMatched( "" ) && ! isMatched( "(" ) && ! isMatched( ")(" ) );
 // against the removal of the innermost pairs until nothing changes
 std::mt19937_64 g( 3 );
 const std::string B = "()[]{}";
 for( int t = 0 ; t < 3000 ; ++t ) {
  std::string s;
  for( std::size_t k = g() % 12 ; k > 0 ; --k ) s += B[ g() % 6 ];
  std::string r = s;
  for( bool changed = true ; changed ; ) {
   changed = false;
   for( const char * p : { "()" , "[]" , "{}" } )
    for( std::size_t f ; ( f = r.find( p ) ) != std::string::npos ; ) {
     r.erase( f , 2 );
     changed = true;
     }
   }
  CHECK( isMatched( s ) == r.empty() );
  }
 }

ADS_TEST( "ods-ex1.4.reverse-stack" )
{
 ArrayStack< int > s( 100 );
 for( int i = 0 ; i < 100 ; ++i ) s.push( i );
 reverseWithQueue( s );
 for( int i = 0 ; i < 100 ; ++i ) CHECK( s.pop() == i );
 SequenceStack< List< std::string > > t;
 for( int i = 0 ; i < 30 ; ++i ) t.push( item( i ) );
 reverseWithQueue( t );
 for( int i = 0 ; i < 30 ; ++i ) CHECK( t.pop() == item( i ) );
 }

ADS_TEST( "ods-ex1.5.bag" )
{
 Bag< Record , RecordHash > b;
 std::multimap< int , int > r;
 std::mt19937_64 g( 4 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  const Record x{ int( g() % 200 ) , t };
  if( g() % 3 ) {
   CHECK( b.insert( x ) );
   r.emplace( x.key , x.payload );
   }
  else {
   const auto range = r.equal_range( x.key );
   const bool there = range.first != range.second;
   CHECK( b.remove( x ) == there );
   if( there ) r.erase( std::prev( range.second ) );   // the last added
   }
  const auto range = r.equal_range( x.key );
  const std::vector< Record > all = b.findAll( x );
  CHECK( all.size() == std::size_t( std::distance( range.first ,
                                                   range.second ) ) );
  auto it = range.first;
  for( const Record & y : all ) CHECK( y.payload == ( it++ )->second );
  const Record * f = b.find( x );
  CHECK( ( f == nullptr ) == all.empty() );
  if( f ) CHECK( f->payload == range.first->second );
  }
 CHECK( b.size() == r.size() );
 }

ADS_TEST( "ods-ex1.6.reference-sets" )
{
 ArrayUSet< int > u;
 ArraySSet< int > s;
 std::set< int > r;
 std::mt19937_64 g( 6 );
 for( int t = 0 ; t < 5000 ; ++t ) {
  const int x = int( g() % 300 );
  if( g() % 2 ) {
   const bool fresh = r.insert( x ).second;
   CHECK( u.insert( x ) == fresh && s.insert( x ) == fresh );
   }
  else {
   const bool there = r.erase( x ) > 0;
   CHECK( u.remove( x ) == there && s.remove( x ) == there );
   }
  const int y = int( g() % 310 );
  const auto lb = r.lower_bound( y );
  const int * p = s.lowerBound( y );
  CHECK( ( p == nullptr ) == ( lb == r.end() ) );
  if( p ) CHECK( *p == *lb );
  CHECK( ( u.find( y ) != nullptr ) == r.contains( y ) );
  CHECK( ( s.find( y ) != nullptr ) == r.contains( y ) );
  }
 CHECK( u.size() == r.size() && s.size() == r.size() );
 CHECK( std::equal( s.items().begin() , s.items().end() , r.begin() ,
                    r.end() ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CHAPTER 2 ------------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "ods-2.1.array-stack" )
{
 // the LIST of the ArrayStack and FastArrayStack (sections 2.1 and 2.2)
 ArraySequence< std::string > a;
 std::vector< std::string > r;
 std::mt19937_64 g( 21 );
 for( int t = 0 ; t < 4000 ; ++t ) {
  if( r.empty() || ( g() % 10 < 6 ) ) {
   const std::size_t i = g() % ( r.size() + 1 );
   const std::string x = item( g() % 1000 );
   a.insert( long( i ) , x );
   r.insert( r.begin() + std::ptrdiff_t( i ) , x );
   }
  else {
   const std::size_t i = g() % r.size();
   a.remove( long( i ) );
   r.erase( r.begin() + std::ptrdiff_t( i ) );
   }
  CHECK( a.size() == long( r.size() ) &&
         ( a.capacity() == 0 || 4 * a.size() > a.capacity() ||
           a.size() == 0 ) );
  }
 CHECK( std::equal( a.begin() , a.end() , r.begin() , r.end() ) );
 }

ADS_TEST( "ods-ex2.2.add-all" )
{
 ArraySequence< int > a{ 1 , 2 , 3 };
 const std::vector< int > c{ 7 , 8 , 9 , 10 };
 a.insert( 1 , c.begin() , c.end() );
 const std::vector< int > r{ 1 , 7 , 8 , 9 , 10 , 2 , 3 };
 CHECK( std::equal( a.begin() , a.end() , r.begin() , r.end() ) );
 // the items of the sequence itself, inserted into it
 std::vector< int > own( a.begin() , a.end() );
 a.insert( 3 , a.begin() , a.end() );
 own.insert( own.begin() + 3 , own.begin() , own.end() );
 CHECK( std::equal( a.begin() , a.end() , own.begin() , own.end() ) );
 a.insert( a.size() , c.begin() , c.begin() );     // nothing
 CHECK( a.size() == long( own.size() ) );
 }

ADS_TEST( "ods-2.3.array-queue" )
{
 // a FIFO on the circular vector: the vector stops growing at the largest
 // size, and the items never move once there
 RingDeque< std::string > q;
 std::deque< std::string > r;
 std::mt19937_64 g( 23 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  if( r.empty() || ( g() % 2 ) ) {
   const std::string x = item( t );
   q.push_back( x );
   r.push_back( x );
   }
  else {
   CHECK( q.front() == r.front() );
   q.pop_front();
   r.pop_front();
   }
  }
 CHECK( std::equal( q.begin() , q.end() , r.begin() , r.end() ) );
 }

ADS_TEST( "ods-2.4.array-deque" )
{
 RingDeque< std::string > d;
 listRandom( d , 20000 , 6 , 24 );
 listRandom( d , 20000 , 4 , 25 );
 // the shifts across the end of the vector, by runs (exercise 2.7)
 for( std::size_t h = 0 ; h < 16 ; ++h ) {
  RingDeque< int > e;
  std::deque< int > r;
  for( int k = 0 ; k < 16 ; ++k ) { e.push_back( k ); r.push_back( k ); }
  for( std::size_t k = 0 ; k < h ; ++k ) {       // the head moved by h
   e.push_back( e.front() ); e.pop_front();
   r.push_back( r.front() ); r.pop_front();
   }
  e.pop_back(); r.pop_back();
  for( std::size_t i = 0 ; i <= r.size() ; ++i ) {
   RingDeque< int > f = e;
   std::deque< int > s = r;
   f.insert( i , 99 ); s.insert( s.begin() + std::ptrdiff_t( i ) , 99 );
   CHECK( std::equal( f.begin() , f.end() , s.begin() , s.end() ) );
   f.erase( ( i * 7 ) % f.size() );
   s.erase( s.begin() + std::ptrdiff_t( ( i * 7 ) % s.size() ) );
   CHECK( std::equal( f.begin() , f.end() , s.begin() , s.end() ) );
   }
  }
 for( int k = 0 ; k < 3000 ; ++k ) d.push_back( item( k ) );
 while( d.size() > 100 ) d.pop_back();
 d.shrink_to_fit();
 CHECK( d.size() == 100 && d.capacity() == 128 );
 }

ADS_TEST( "ods-ex2.8.power-of-two" )
{
 RingDeque< int > d;
 for( int k = 0 ; k < 5000 ; ++k ) {
  d.push_front( k );
  CHECK( std::has_single_bit( d.capacity() ) && d.capacity() >= d.size() );
  }
 }

ADS_TEST( "ods-2.5.dual-array-deque" )
{
 DualArrayDeque< std::string > d;
 listRandom( d , 20000 , 6 , 25 );
 listRandom( d , 20000 , 4 , 26 );
 for( int k = 0 ; k < 3000 ; ++k ) d.push_back( item( k ) );
 const std::size_t n = d.size() , f = d.frontSize();
 CHECK( 3 * f >= n - f && 3 * ( n - f ) >= f );    // the balance
 dequeRandom( d , 20000 , 5 , 27 );
 }

ADS_TEST( "ods-2.6.rootish-array-stack" )
{
 // the block of (1) against the definition, near the squares and far
 for( std::size_t i = 0 ; i < 200000 ; ++i ) {
  const std::size_t b = RootishArrayStack< int >::block( i );
  CHECK( b * ( b + 1 ) / 2 <= i && i < ( b + 1 ) * ( b + 2 ) / 2 );
  }
 for( std::size_t b : { std::size_t( 1 ) << 20 , std::size_t( 3000000 ) ,
                        std::size_t( 1 ) << 26 } ) {
  const std::size_t s = b * ( b + 1 ) / 2;
  CHECK( RootishArrayStack< int >::block( s ) == b );
  CHECK( RootishArrayStack< int >::block( s - 1 ) == b - 1 );
  CHECK( RootishArrayStack< int >::block( s + b ) == b );
  CHECK( RootishArrayStack< int >::block( s + b + 1 ) == b + 1 );
  }
 RootishArrayStack< std::string > r;
 listRandom( r , 20000 , 6 , 26 );
 listRandom( r , 20000 , 4 , 27 );
 // the places wasted: at most 2 r + 1 (Lemma 2.5 of Morin's book)
 RootishArrayStack< int > s;
 for( int k = 0 ; k < 100000 ; ++k ) {
  s.push_back( k );
  CHECK( s.capacity() - s.size() <= 2 * s.blocks() + 1 );
  }
 for( int k = 0 ; k < 100000 ; ++k ) {
  s.pop_back();
  CHECK( s.capacity() - s.size() <= 2 * s.blocks() + 1 );
  }
 CHECK( s.blocks() == 0 );
 }

ADS_TEST( "ods-ex2.3.random-queue" )
{
 // each of 8 items equally likely to come out first: the counts of 80000
 // draws within 5 standard deviations of 10000
 int c[ 8 ] = {};
 RandomQueue< int > q( 9 );
 for( int t = 0 ; t < 80000 ; ++t ) {
  for( int k = 0 ; k < 8 ; ++k ) q.push( k );
  ++c[ q.pop() ];
  q.clear();
  }
 for( int k = 0 ; k < 8 ; ++k ) CHECK( std::abs( c[ k ] - 10000 ) < 470 );
 // the draw of a place, uniform also for a range not a power of 2
 RandomQueue< int > u( 3 );
 std::vector< int > h( 3 );
 for( int t = 0 ; t < 30000 ; ++t ) ++h[ u.uniform( 3 ) ];
 for( int k : h ) CHECK( std::abs( k - 10000 ) < 500 );
 // the same items out as in
 std::multiset< int > in , out;
 for( int t = 0 ; t < 5000 ; ++t ) {
  if( q.empty() || ( t % 3 ) ) { q.push( t % 97 ); in.insert( t % 97 ); }
  else out.insert( q.pop() );
  }
 while( ! q.empty() ) out.insert( q.pop() );
 CHECK( in == out );
 }

ADS_TEST( "ods-ex2.4.treque" )
{
 Treque< std::string > t;
 listRandom( t , 20000 , 6 , 28 );
 listRandom( t , 20000 , 4 , 29 );
 dequeRandom( t , 10000 , 5 , 30 );
 }

ADS_TEST( "ods-ex2.5.rotate-array" )
{
 for( std::size_t n = 0 ; n < 40 ; ++n )
  for( std::size_t r = 0 ; r < 2 * n + 2 ; ++r ) {
   std::vector< std::string > a , b;
   for( std::size_t k = 0 ; k < n ; ++k ) a.push_back( item( k ) );
   b = a;
   rotateArray( std::span< std::string >( a ) , r );
   if( n ) std::rotate( b.rbegin() , b.rbegin() + std::ptrdiff_t( r % n ) ,
                        b.rend() );
   CHECK( a == b );
   }
 }

ADS_TEST( "ods-ex2.6.rotate-list" )
{
 // the rotate() of the lists that have one, and the cycles on the others
 auto run = []< IndexedList L >( L l , std::size_t n ) {
  std::vector< int > r;
  for( std::size_t k = 0 ; k < n ; ++k ) {
   l.insert( l.size() , int( k ) );
   r.push_back( int( k ) );
   }
  for( std::size_t s : { std::size_t( 0 ) , std::size_t( 1 ) , n / 3 ,
                         n / 2 , n - 1 , n , 3 * n + 2 } ) {
   rotate( l , s );
   if( n ) std::rotate( r.rbegin() , r.rbegin() + std::ptrdiff_t( s % n ) ,
                        r.rend() );
   for( std::size_t k = 0 ; k < n ; ++k ) CHECK( l[ k ] == r[ k ] );
   }
  };
 for( std::size_t n : { 1 , 2 , 8 , 9 , 100 , 257 } ) {
  run( RingDeque< int >() , n );
  run( DualArrayDeque< int >() , n );
  run( Treque< int >() , n );
  run( TieredVector< int >() , n );
  run( RootishArrayStack< int >() , n );
  run( SEList< int >( 4 ) , n );
  }
 RingDeque< int > full;                          // the head alone moves
 for( int k = 0 ; k < 8 ; ++k ) full.push_back( k );
 CHECK( full.capacity() == 8 );
 full.rotate( 3 );
 CHECK( full[ 0 ] == 5 && full[ 3 ] == 0 && full[ 7 ] == 4 );
 }

ADS_TEST( "ods-ex2.9.contiguous-deque" )
{
 ContiguousDeque< std::string > d;
 listRandom( d , 20000 , 6 , 31 );
 listRandom( d , 20000 , 4 , 32 );
 dequeRandom( d , 20000 , 5 , 33 );
 CHECK( d.data() + d.size() == &*d.end() || d.empty() );
 // a FIFO running along the vector: the space stays linear
 ContiguousDeque< int > q;
 for( int k = 0 ; k < 200000 ; ++k ) {
  q.push_back( k );
  if( k % 2 ) q.pop_front();
  CHECK( q.capacity() <= 9 * q.size() + 24 );
  }
 for( std::size_t i = 0 ; i < q.size() ; ++i )   // consecutive integers
  CHECK( q[ i ] == q.front() + int( i ) );
 }

ADS_TEST( "ods-ex2.10.rootish-deque" )
{
 DualArrayDeque< std::string , RootishArrayStack< std::string > > d;
 listRandom( d , 20000 , 6 , 34 );
 listRandom( d , 20000 , 4 , 35 );
 dequeRandom( d , 20000 , 5 , 36 );
 }

ADS_TEST( "ods-ex2.11.tiered-deque" )
{
 // the shifts toward the back of the tiered vector, the bound of
 // exercise 2.11; the places wasted below 2 b plus the gap
 TieredVector< std::string > v;
 listRandom( v , test::scaled< std::size_t >( 60000 , 6000 ) , 7 , 37 ,
             false );
 CHECK( v.capacity() - v.size() < 2 * v.block() );
 }

ADS_TEST( "ods-ex2.12.tiered-deque" )
{
 // the shifts toward the front too, the bound of exercise 2.12, past a
 // rebuild of the blocks (4 b^2 items for b = 64)
 TieredVector< std::string > v;
 listRandom( v , test::scaled< std::size_t >( 60000 , 6000 ) , 9 , 38 );
 listRandom( v , test::scaled< std::size_t >( 30000 , 3000 ) , 4 , 39 );
 CHECK( v.capacity() - v.size() < 2 * v.block() );
 dequeRandom( v , 20000 , 5 , 40 );
 TieredVector< int > w;
 for( int k = 0 ; k < 20000 ; ++k ) w.push_front( k );
 for( int k = 0 ; k < 20000 ; ++k ) CHECK( w[ std::size_t( k ) ] ==
                                          19999 - k );
 CHECK( w.block() * w.block() >= w.size() &&
        w.capacity() - w.size() < 2 * w.block() );
 while( ! w.empty() ) w.pop_front();
 CHECK( w.capacity() == 0 );
 }

ADS_TEST( "ods-ex2.13.cubish" )
{
 CubishArrayStack< std::string > c;
 listRandom( c , test::scaled< std::size_t >( 80000 , 8000 ) , 9 , 41 );
 listRandom( c , test::scaled< std::size_t >( 40000 , 4000 ) , 3 , 42 );
 // the places wasted, O( n^{2/3} ): 2 b per middle block and 4 kept
 CubishArrayStack< int > w;
 for( int k = 0 ; k < test::scaled( 300000 , 40000 ) ; ++k ) {
  w.insert( std::size_t( k ) / 2 , k );
  if( k % 4096 == 0 ) {
   const double n = double( w.size() );
   CHECK( double( w.capacity() - w.size() ) <=
          2.0 * double( w.block() ) * ( n / double( w.block() *
                                                    w.block() ) + 1 ) +
          4.0 * double( w.block() ) );
   CHECK( double( w.block() ) <= 2.0 * std::cbrt( n ) + 16 );
   }
  }
 std::vector< int > r;
 for( int k = 0 ; k < test::scaled( 300000 , 40000 ) ; ++k )
  r.insert( r.begin() + k / 2 , k );
 for( std::size_t i = 0 ; i < r.size() ; i += 101 ) CHECK( w[ i ] == r[ i ] );
 while( w.size() > 10 ) w.erase( w.size() / 3 );
 CHECK( w.block() == 16 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CHAPTER 3 ------------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "ods-3.1.sllist" )
{
 SinglyList< std::string > s;
 std::deque< std::string > r;
 std::mt19937_64 g( 51 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  const int w = int( g() % 3 );
  if( r.empty() || ( g() % 10 < 6 ) ) {
   const std::string x = item( t );
   if( w ) { s.push_front( x ); r.push_front( x ); }  // push(x)
   else { s.push_back( x ); r.push_back( x ); }       // add(x)
   }
  else {
   CHECK( s.pop_front() == r.front() );               // pop(), remove()
   r.pop_front();
   }
  CHECK( s.size() == r.size() );
  if( ! r.empty() ) CHECK( s.front() == r.front() && s.back() == r.back() );
  }
 CHECK( std::equal( s.begin() , s.end() , r.begin() , r.end() ) );
 }

ADS_TEST( "ods-ex3.2-3.5.sllist" )
{
 SinglyList< int > s;
 std::vector< int > r;
 std::mt19937_64 g( 52 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  if( r.empty() || ( g() % 10 < 6 ) ) {
   const std::size_t i = g() % ( r.size() + 1 );
   s.add( i , t );                                    // exercise 3.3
   r.insert( r.begin() + std::ptrdiff_t( i ) , t );
   }
  else if( g() % 2 ) {
   const std::size_t i = g() % r.size();
   CHECK( s.erase( i ) == r[ i ] );
   r.erase( r.begin() + std::ptrdiff_t( i ) );
   }
  else {
   const std::size_t i = g() % r.size();
   CHECK( s.set( i , -t ) == r[ i ] );
   r[ i ] = -t;
   }
  if( r.size() >= 2 ) CHECK( s.secondLast() == r[ r.size() - 2 ] );   // 3.2
  if( ! r.empty() ) CHECK( s.get( r.size() / 2 ) == r[ r.size() / 2 ] &&
                           s.back() == r.back() );
  }
 s.checkSize();                                       // exercise 3.5
 s.reverse();                                         // exercise 3.4
 CHECK( std::equal( s.begin() , s.end() , r.rbegin() , r.rend() ) );
 s.push_back( 1 );
 CHECK( s.back() == 1 );
 s.checkSize();
 }

ADS_TEST( "ods-3.2.dllist" )
{
 List< std::string > l;
 std::vector< std::string > r;
 std::mt19937_64 g( 53 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  if( r.empty() || ( g() % 10 < 6 ) ) {
   const std::size_t i = g() % ( r.size() + 1 );
   const std::string x = item( t );
   l.add( i , x );
   r.insert( r.begin() + std::ptrdiff_t( i ) , x );
   }
  else if( g() % 2 ) {
   const std::size_t i = g() % r.size();
   CHECK( l.erase( i ) == r[ i ] );
   r.erase( r.begin() + std::ptrdiff_t( i ) );
   }
  else {
   const std::size_t i = g() % r.size();
   CHECK( l.set( i , item( t ) ) == r[ i ] );
   r[ i ] = item( t );
   }
  CHECK( l.size() == r.size() );
  if( ! r.empty() ) {
   const std::size_t i = g() % r.size();
   CHECK( l.get( i ) == r[ i ] );
   }
  }
 CHECK( std::equal( l.begin() , l.end() , r.begin() , r.end() ) );
 l.checkSize();
 }

/// the exercises 3.5 to 3.12 on a list of cells pooled or not
template< bool P >
void dllistExercises( void )
{
 using L = List< int , P >;
 // 3.6: insert() is addBefore()
 L a{ 1 , 3 };
 a.insert( a.next( a.head() ) , 2 );
 CHECK( a.get( 0 ) == 1 && a.get( 1 ) == 2 && a.get( 2 ) == 3 );
 // 3.7
 CHECK( ( L{ 1 , 2 , 1 } ).isPalindrome() && ( L{} ).isPalindrome() &&
        ( L{ 4 , 4 } ).isPalindrome() && ! ( L{ 1 , 2 } ).isPalindrome() );
 // 3.8
 for( std::size_t n : { 1 , 2 , 7 , 50 } )
  for( std::size_t r : { std::size_t( 0 ) , std::size_t( 1 ) , n / 2 ,
                         n - 1 , n , 2 * n + 1 } ) {
   L l;
   std::vector< int > v;
   for( std::size_t k = 0 ; k < n ; ++k ) {
    l.push_back( int( k ) );
    v.push_back( int( k ) );
    }
   l.rotate( r );
   std::rotate( v.rbegin() , v.rbegin() + std::ptrdiff_t( r % n ) ,
                v.rend() );
   CHECK( std::equal( l.begin() , l.end() , v.begin() , v.end() ) );
   l.checkSize();
   }
 // 3.9 and 3.10
 for( std::size_t n : { 0 , 1 , 2 , 9 , 40 } )
  for( std::size_t i = 0 ; i <= n ; ++i ) {
   L l;
   for( std::size_t k = 0 ; k < n ; ++k ) l.push_back( int( k ) );
   L t = l.truncate( i );
   CHECK( l.size() == i && t.size() == n - i );
   for( std::size_t k = 0 ; k < i ; ++k ) CHECK( l.get( k ) == int( k ) );
   for( std::size_t k = i ; k < n ; ++k ) CHECK( t.get( k - i ) ==
                                                int( k ) );
   l.absorb( t );
   CHECK( t.empty() && t.size() == 0 && l.size() == n );
   for( std::size_t k = 0 ; k < n ; ++k ) CHECK( l.get( k ) == int( k ) );
   l.checkSize();
   t.checkSize();
   t.push_back( 5 );                              // the emptied list works
   CHECK( t.size() == 1 && t.get( 0 ) == 5 );
   }
 // 3.11
 for( std::size_t n : { 0 , 1 , 2 , 5 , 6 } ) {
  L l;
  for( std::size_t k = 0 ; k < n ; ++k ) l.push_back( int( k ) );
  L o = l.deal();
  CHECK( l.size() == ( n + 1 ) / 2 && o.size() == n / 2 );
  for( std::size_t k = 0 ; k < l.size() ; ++k ) CHECK( l.get( k ) ==
                                                      int( 2 * k ) );
  for( std::size_t k = 0 ; k < o.size() ; ++k ) CHECK( o.get( k ) ==
                                                      int( 2 * k + 1 ) );
  }
 // 3.12
 L l{ 1 , 2 , 3 , 4 };
 l.reverse();
 CHECK( l == ( L{ 4 , 3 , 2 , 1 } ) && l.get( 0 ) == 4 );
 l.push_back( 0 );
 CHECK( l.get( 4 ) == 0 && l.size() == 5 );
 l.checkSize();                                  // 3.5
 }

ADS_TEST( "ods-ex3.5-3.12.dllist" )
{
 dllistExercises< true >();
 dllistExercises< false >();
 }

/// exercise 3.13 on a list pooled or not: the merge sort stable, by the
/// pairs ( key , rank )
template< bool P >
void dllistSort( void )
{
 using I = std::pair< int , int >;
 auto byKey = []( const I & a , const I & b ) { return( a.first < b.first ); };
 std::mt19937_64 g( P ? 54 : 55 );
 for( std::size_t n : { 0 , 1 , 2 , 3 , 17 , 100 , 1000 } ) {
  List< I , P > l;
  std::vector< I > v;
  for( std::size_t k = 0 ; k < n ; ++k ) {
   const I x{ int( g() % 10 ) , int( k ) };
   l.push_back( x );
   v.push_back( x );
   }
  l.mergeSort( byKey );
  std::stable_sort( v.begin() , v.end() , byKey );
  CHECK( std::equal( l.begin() , l.end() , v.begin() , v.end() ) );
  l.checkSize();
  }
 List< I , P > a , b , c;
 for( int k = 0 ; k < 50 ; ++k ) {
  a.push_back( { 2 * k , 0 } );
  b.push_back( { 3 * k , 1 } );
  }
 c.takeFirst( a );                                 // no cell made if not P
 CHECK( c.size() == 1 && a.size() == 49 && c.get( 0 ).first == 0 );
 a.absorb( c );                                    // back at the end
 a.mergeSort( byKey );
 List< I , P > m = List< I , P >::merge( a , b , byKey );
 CHECK( a.empty() && b.empty() && m.size() == 100 );
 for( std::size_t k = 1 ; k < m.size() ; ++k ) {
  const I & x = m.get( k - 1 ) , & y = m.get( k );
  CHECK( x.first < y.first || ( x.first == y.first && x.second <= y.second ) );
  }
 m.checkSize();
 }

ADS_TEST( "ods-ex3.13.merge-sort" )
{
 dllistSort< true >();
 dllistSort< false >();
 }

ADS_TEST( "ods-ex3.14.min-stack" )
{
 MinStack< int > s;
 std::vector< int > r;
 std::mt19937_64 g( 56 );
 for( int t = 0 ; t < 4000 ; ++t ) {
  if( r.empty() || ( g() % 10 < 6 ) ) {
   const int x = int( g() % 50 );
   s.push( x );
   r.push_back( x );
   }
  else {
   CHECK( s.pop() == r.back() );
   r.pop_back();
   }
  CHECK( s.size() == r.size() );
  if( ! r.empty() ) CHECK( s.min() == *std::min_element( r.begin() ,
                                                         r.end() ) &&
                           s.top() == r.back() );
  }
 MinStack< int > inc;                              // few minima kept
 for( int k = 0 ; k < 100 ; ++k ) inc.push( k );
 CHECK( inc.minima() == 1 && inc.min() == 0 );
 }

ADS_TEST( "ods-ex3.15.min-queue" )
{
 MinQueue< int > q;
 std::deque< int > r;
 std::mt19937_64 g( 57 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  if( r.empty() || ( g() % 10 < 6 ) ) {
   const int x = int( g() % 1000 );
   q.enqueue( x );
   r.push_back( x );
   }
  else {
   CHECK( q.top() == r.front() );
   CHECK( q.dequeue() == r.front() );
   r.pop_front();
   }
  CHECK( q.size() == r.size() );
  if( ! r.empty() ) CHECK( q.min() == *std::min_element( r.begin() ,
                                                         r.end() ) );
  }
 }

ADS_TEST( "ods-ex3.16.min-deque" )
{
 MinDeque< int > d;
 std::deque< int > r;
 std::mt19937_64 g( 58 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  const bool atFront = g() % 2;
  if( r.empty() || ( g() % 10 < 5 ) ) {
   const int x = int( g() % 1000 );
   if( atFront ) { d.push_front( x ); r.push_front( x ); }
   else { d.push_back( x ); r.push_back( x ); }
   }
  else if( atFront ) { CHECK( d.pop_front() == r.front() ); r.pop_front(); }
  else { CHECK( d.pop_back() == r.back() ); r.pop_back(); }
  CHECK( d.size() == r.size() );
  if( ! r.empty() )
   CHECK( d.min() == *std::min_element( r.begin() , r.end() ) &&
          d.front() == r.front() && d.back() == r.back() );
  }
 }

ADS_TEST( "ods-3.3.selist" )
{
 for( std::size_t b : { 3 , 4 , 5 , 8 , 63 } ) {
  SEList< std::string > s( b );
  listRandom( s , 20000 , 6 , 59 + b );
  listRandom( s , 20000 , 4 , 60 + b );
  // the space: n / ( b - 1 ) + 2 blocks at most
  CHECK( s.blocks() <= s.size() / ( b - 1 ) + 2 );
  }
 SEList< int > c( 4 ) , d = c;
 for( int k = 0 ; k < 1000 ; ++k ) c.insert( c.size() / 2 , k );
 d = c;
 CHECK( d == c && d.size() == 1000 );
 SEList< int > e( std::move( d ) );
 CHECK( e == c && d.empty() );
 }

ADS_TEST( "ods-ex3.17-3.18.selist-ends" )
{
 // the operations at the ends, as a stack and as a double ended queue:
 // the blocks are those of a list built by them, all full but the ends
 for( std::size_t b : { 3 , 16 } ) {
  SEList< std::string > s( b );
  dequeRandom( s , 30000 , 6 , 61 + b );
  dequeRandom( s , 30000 , 4 , 62 + b );
  SEList< int > t( b );
  for( int k = 0 ; k < 10000 ; ++k ) t.push_back( k );
  CHECK( t.blocks() == ( 10000 + b ) / ( b + 1 ) );
  for( int k = 0 ; k < 9990 ; ++k ) t.pop_back();
  CHECK( t.size() == 10 && t.blocks() <= 10 / ( b + 1 ) + 1 );
  }
 }

ADS_TEST( "ods-3.4.xor-list" )
{
 XorList< std::string > x;
 dequeRandom( x , 5000 , 6 , 63 );
 std::vector< std::string > r( x.begin() , x.end() );
 x.reverse();
 CHECK( std::equal( x.begin() , x.end() , r.rbegin() , r.rend() ) );
 std::vector< std::string > back;
 for( auto it = x.end() ; it != x.begin() ; ) back.push_back( *--it );
 CHECK( back == r );
 XorList< std::string > y = x;
 CHECK( y == x );
 y.push_back( "z" );
 CHECK( !( y == x ) && y.back() == "z" );
 }

ADS_TEST( "ods-ex3.19.xor-swap" )
{
 std::uint64_t a = 12345 , b = 0xFFFF0000FFFF0000ULL;
 xorSwap( a , b );
 CHECK( a == 0xFFFF0000FFFF0000ULL && b == 12345 );
 unsigned c = 7 , d = 7;
 xorSwap( c , d );
 CHECK( c == 7 && d == 7 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File ods_lists.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
