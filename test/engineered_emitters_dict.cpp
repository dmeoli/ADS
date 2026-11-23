/*--------------------------------------------------------------------------*/
/*-------------------- File engineered_emitters_dict.cpp -------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tests of the dictionaries that tell their changes, beyond the book:
 * the events of BTree, DenseHashMap and PointerTable for a sequence of
 * operations, against the records expected; an observer that keeps a copy
 * of the content from the events alone, checked against the structure
 * after each operation and by its audit; the batches, whose coalesced
 * records rebuild the same content; the mutes, the refusals and the strong
 * guarantee; and the plain structure and the observed one fed the same
 * random operations, with the same content.
 *
 *   engineered_emitters_dict <test>
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

#include "ADS/Modification.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/tree/BTree.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE MIRRORS -------------------------------*/
/*--------------------------------------------------------------------------*/

using Ins = mod::Inserted< int , int , int >;
using Era = mod::Erased< int , int , int >;
using Asg = mod::Assigned< int , int >;
using Rec = std::variant< Ins , Era , Asg , mod::Cleared , mod::Rebuilt >;

/// an observer of a map of int to int that records the events and keeps a
/// copy of the content from them alone, the events checked against it
struct MapMirror
{
 std::map< int , int > m;  ///< the content, from the events
 std::vector< Rec > ev;   ///< the events received
 int groups = 0;          ///< the batches received
 bool stale = false;      ///< a Rebuilt received
 bool wrong = false;      ///< an event that does not fit the copy
 void on( const Ins & e ) {
  wrong = wrong || m.count( e.h ) || ( e.h != e.key );
  m[ e.h ] = e.value;
  ev.emplace_back( e );
  }
 void on( const Era & e ) noexcept {
  const auto it = m.find( e.h );
  wrong = wrong || ( it == m.end() ) || ( it->second != e.value );
  m.erase( e.h );
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void on( const Asg & e ) {
  const auto it = m.find( e.h );
  wrong = wrong || ( it == m.end() ) || ( it->second != e.from );
  m[ e.h ] = e.to;
  ev.emplace_back( e );
  }
 void on( const mod::Cleared & e ) noexcept {
  m.clear();
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void on( const mod::Rebuilt & e ) noexcept {
  stale = true;
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void group_begin( void ) { ++groups; }
 void group_end( void ) { }
 /// whether the copy is the content of \f$ s \f$ (unless stale)
 template< class S >
 [[nodiscard]] bool audit( const S & s ) const {
  if( stale )
   return( true );
  if( wrong || ( m.size() != s.size() ) )
   return( false );
  for( const auto & [ k , v ] : m )
   if( s.lookup( k ) != std::optional< int >( v ) )
    return( false );
  return( true );
  }
 };

/// the content of a map by forEach(), sorted
template< class S >
std::map< int , int > content( const S & s )
{
 std::map< int , int > c;
 s.forEach( [ & ]( const int & k , const auto & v ) { c[ k ] = int( v ); } );
 return( c );
 }

/// random insertions, assignments and removals on two maps, the second
/// checked after each by its mirror
template< class S1 , class S2 >
void mapRun( S1 & a , S2 & b , MapMirror & mm , unsigned seed , int ops )
{
 std::mt19937 rng( seed );
 for( int t = 0 ; t < ops ; ++t ) {
  const int k = int( rng() % 97 ) , v = int( rng() % 1000 );
  if( rng() % 3 == 0 ) {
   a.remove( k );
   b.remove( k );
   }
  else {
   a.insert( k , v );
   b.insert( k , v );
   }
  CHECK( mm.audit( b ) );
  }
 CHECK( content( a ) == content( b ) );
 CHECK( content( b ) == mm.m );
 }

/// the sequence of the records test: the events expected of a map
template< class S >
void mapSequence( S & s , MapMirror & mm )
{
 s.insert( 1 , 10 );
 s.insert( 2 , 20 );
 s.insert( 1 , 11 );  // an assignment
 s.remove( 2 );
 s.remove( 5 );  // not there: nothing
 const std::vector< Rec > E1{ Ins{ 1 , 1 , 10 } , Ins{ 2 , 2 , 20 } ,
			      Asg{ 1 , 10 , 11 } , Era{ 2 , 2 , 20 } };
 CHECK( mm.ev == E1 );
 CHECK( mm.audit( s ) && ( mm.m == std::map< int , int >{ { 1 , 11 } } ) );
 // a batch: coalesced per key into its net effect
 mm.ev.clear();
 {
  auto b = batch( s );
  s.insert( 3 , 30 );
  s.insert( 3 , 31 );  // folded into the insertion
  s.insert( 4 , 40 );
  s.remove( 4 );  // cancels the insertion
  s.remove( 1 );
  s.insert( 6 , 60 );
  CHECK( mm.ev.empty() );
  }
 const std::vector< Rec > E2{ Ins{ 3 , 3 , 31 } , Era{ 1 , 1 , 11 } ,
			      Ins{ 6 , 6 , 60 } };
 CHECK( ( mm.groups == 1 ) && ( mm.ev == E2 ) );
 CHECK( mm.audit( s ) && ( mm.m == content( s ) ) );
 // a mute: Rebuilt alone
 mm.ev.clear();
 {
  auto q = quiet( s );
  s.insert( 7 , 70 );
  }
 CHECK( ( mm.ev == std::vector< Rec >{ mod::Rebuilt{} } ) && mm.stale );
 mm.m = content( s );
 mm.stale = false;
 mm.ev.clear();
 s.clear();
 CHECK( ( mm.ev == std::vector< Rec >{ mod::Cleared{} } ) && mm.m.empty() &&
	( s.size() == 0 ) );
 }

/// an observer that refuses the insertions of negative keys, and throws
/// from `on()` of a key 13
struct Picky
{
 int seen = 0;
 [[nodiscard]] bool prepare( const Ins & e ) const { return( e.h >= 0 ); }
 void on( const Ins & e ) {
  ++seen;
  if( e.h == 13 )
   throw( std::runtime_error( "13" ) );
  }
 void on( const Asg & e ) {
  ++seen;
  if( e.to == 13 )
   throw( std::runtime_error( "13" ) );
  }
 };

/// the refusals and the strong guarantee on a map with `p` attached, for
/// the insertions too if `strong`; `p` stays attached, and must outlive
/// the map or be detached before it dies
template< class S >
void mapPicky( S & s , Picky & p , bool strong )
{
 s.attach( p );
 s.insert( 1 , 1 );
 CHECK_THROWS( s.insert( -1 , 1 ) , Refused );
 CHECK( ! s.contains( -1 ) && ( p.seen == 1 ) );
 CHECK_THROWS( s.insert( 13 , 1 ) , std::runtime_error );
 CHECK( ( p.seen == 2 ) && ( s.contains( 13 ) != strong ) );
 CHECK_THROWS( s.insert( 1 , 13 ) , std::runtime_error );
 CHECK( s.lookup( 1 ) == std::optional< int >( 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE TABLE OF POINTERS -------------------------*/
/*--------------------------------------------------------------------------*/

/// an object of a search: its state
struct Node
{
 int state;
 };

using PIns = mod::Inserted< Node * , std::uint64_t >;

/// an observer of a table of nodes that records its events and keeps the
/// set of the objects from them alone
struct TableMirror
{
 std::set< Node * > in;  ///< the objects, from the events
 std::vector< std::variant< PIns , mod::Cleared , mod::Rebuilt > > ev;
 bool wrong = false;  ///< an event that does not fit the copy
 bool stale = false;  ///< a Rebuilt received
 void on( const PIns & e ) {
  wrong = wrong || ! in.insert( e.h ).second ||
	  ( e.key != std::uint64_t( e.h->state ) * 0x9E3779B97F4A7C15ULL );
  ev.emplace_back( e );
  }
 void on( const mod::Cleared & e ) noexcept {
  in.clear();
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void on( const mod::Rebuilt & e ) noexcept {
  stale = true;
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 /// whether the copy is the set of the objects of \f$ t \f$ (unless
 /// stale)
 template< class T >
 [[nodiscard]] bool audit( const T & t ) const {
  if( stale )
   return( ! wrong );
  if( wrong || ( in.size() != t.size() ) )
   return( false );
  for( Node * p : in )
   if( t.find( p->state ,
	       std::uint64_t( p->state ) * 0x9E3779B97F4A7C15ULL ) !=
       p )
    return( false );
  return( true );
  }
 };

/// the hash of a state
std::uint64_t hashOf( int s )
{
 return( std::uint64_t( s ) * 0x9E3779B97F4A7C15ULL );
 }

/// find_or_insert() of the state \f$ s \f$, its node made in `pool`
template< class T >
std::pair< Node * , bool >
findOrMake( T & t , std::vector< std::unique_ptr< Node > > & pool , int s )
{
 return( t.find_or_insert( s , hashOf( s ) , [ & ]() {
  pool.push_back( std::make_unique< Node >( Node{ s } ) );
  return( pool.back().get() );
  } ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE B-TREE ------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-btree" )
{
 // nobody listens: the plain tree, no storage for the subject
 static_assert(
   std::is_same_v< BTree< int , int > , BTree< int , int , NoObservers > > );
 struct Old
 {
  int t;
  std::unique_ptr< int > root;
  std::size_t size;
  };
 static_assert( sizeof( BTree< int , int > ) == sizeof( Old ) );
 static_assert( ! BTree< int , int >::Subject::template listened< Ins > );
 // the records of a sequence, then a batch, a mute and a clear
 MapMirror mm;
 BTree< int , int , Static< MapMirror > > t( 2 , mm );
 mapSequence( t , mm );
 // random operations, the copy checked after each, against the plain tree
 MapMirror m2;
 BTree< int , int , Static< MapMirror > > s( 2 , m2 );
 BTree< int , int > p( 2 );
 mapRun( p , s , m2 , 7 , 5000 );
 // attached at run time
 BTree< int , int , Dynamic<> > d( 3 );
 MapMirror m3;
 const int id = d.attach( m3 );
 BTree< int , int > p3( 3 );
 mapRun( p3 , d , m3 , 11 , 3000 );
 d.detach( id );
 d.insert( 1000 , 1 );
 CHECK( ! m3.m.count( 1000 ) );
 }

ADS_TEST( "emit-btree-guarantee" )
{
 Picky p;  // before the map, which outlives it
 BTree< int , int , Dynamic<> > d( 2 );
 mapPicky( d , p , false );  // a removal may allocate
 // the erasure told before the removal
 struct Before
 {
  const BTree< int , int , Dynamic<> > * t = nullptr;
  int present = 0;
  void on( const Era & e ) noexcept { present += t->contains( e.h ) ? 1 : 0; }
  };
 Before b;
 b.t = &d;
 d.attach( b );
 d.remove( 1 );
 CHECK( ( b.present == 1 ) && ! d.contains( 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE DENSE MAP -----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-densemap" )
{
 using Plain = DenseHashMap< int , int >;
 static_assert(
   std::is_same_v<
     Plain ,
       DenseHashMap< int , int , DefaultHash< int > , std::equal_to< int > ,
			  std::allocator< std::pair< const int , int > > ,
			  NoObservers > > );
 static_assert( ! Plain::Subject::template listened< Ins > );
 using SM = DenseHashMap< int , int , DefaultHash< int > ,
			  std::equal_to< int > ,
			  std::allocator< std::pair< const int , int > > ,
			  Static< MapMirror > >;
 using DM =
   DenseHashMap< int , int , DefaultHash< int > , std::equal_to< int > ,
		 std::allocator< std::pair< const int , int > > , Dynamic<> >;
 MapMirror mm;
 SM t( 0 , DefaultHash< int >() , std::equal_to< int >() ,
       std::allocator< std::pair< const int , int > >() , mm );
 mapSequence( t , mm );
 MapMirror m2;
 SM s( 0 , DefaultHash< int >() , std::equal_to< int >() ,
       std::allocator< std::pair< const int , int > >() , m2 );
 Plain p;
 mapRun( p , s , m2 , 5 , 5000 );
 // try_emplace(): Inserted with the value built, nothing if there
 DM d;
 MapMirror m3;
 d.attach( m3 );
 d.try_emplace( 4 , 44 );
 d.try_emplace( 4 , 45 );
 int k = 5 , v = 55;
 d.insert( std::move( k ) , std::move( v ) );
 CHECK( m3.ev == ( std::vector< Rec >{ Ins{ 4 , 4 , 44 } ,
				       Ins{ 5 , 5 , 55 } } ) );
 // a swap: Rebuilt to both
 DM e;
 MapMirror m4;
 e.attach( m4 );
 e.insert( 9 , 9 );
 d.swap( e );
 CHECK( m3.stale && m4.stale &&
	( content( d ) == std::map< int , int >{ { 9 , 9 } } ) );
 // a move from the map: Cleared to its observers, none to the new one
 MapMirror m5;
 DM f;
 f.attach( m5 );
 f.insert( 1 , 1 );
 DM g( std::move( f ) );
 CHECK( m5.m.empty() && ( m5.ev.back() == Rec( mod::Cleared{} ) ) );
 g.insert( 2 , 2 );
 CHECK( m5.m.empty() && ( g.size() == 2 ) );
 // the refusals and the strong guarantee
 Picky picky;  // before the map, which outlives it
 DM h;
 mapPicky( h , picky , true );
 h.try_emplace( 7 , 7 );
 CHECK_THROWS( h.try_emplace( -3 , 1 ) , Refused );
 CHECK( ! h.contains( -3 ) && h.contains( 7 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE TABLE OF POINTERS -------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-pointertable" )
{
 using Plain = PointerTable< Node >;
 static_assert(
   std::is_same_v< Plain ,
		   PointerTable< Node , StateEqual ,
				 std::allocator< std::byte > ,
				 NoObservers > > );
 static_assert( ! Plain::Subject::template listened< PIns > );
 using ST = PointerTable< Node , StateEqual , std::allocator< std::byte > ,
			  Static< TableMirror > >;
 using DT =
   PointerTable< Node , StateEqual , std::allocator< std::byte > , Dynamic<> >;
 std::vector< std::unique_ptr< Node > > pool;
 TableMirror tm;
 ST t( 16 , StateEqual() , std::allocator< std::byte >() , tm );
 const auto a = findOrMake( t , pool , 1 );
 const auto b = findOrMake( t , pool , 2 );
 const auto c = findOrMake( t , pool , 1 );  // found: nothing
 CHECK( a.second && b.second && ! c.second && ( c.first == a.first ) );
 CHECK( tm.ev.size() == 2 );
 CHECK( tm.ev[ 0 ] ==
	decltype( tm.ev )::value_type( PIns{ a.first , hashOf( 1 ) } ) );
 CHECK( tm.ev[ 1 ] ==
	decltype( tm.ev )::value_type( PIns{ b.first , hashOf( 2 ) } ) );
 // past the growths, the copy checked after each insertion
 Plain p;
 std::vector< std::unique_ptr< Node > > pool2;
 findOrMake( p , pool2 , 1 );
 findOrMake( p , pool2 , 2 );
 std::mt19937 rng( 3 );
 for( int i = 0 ; i < 4000 ; ++i ) {
  const int s = int( rng() % 1500 );
  CHECK( findOrMake( t , pool , s ).second == findOrMake( p , pool2 ,
							  s ).second );
  CHECK( tm.audit( t ) );
  }
 CHECK( t.size() == p.size() );
 t.clear();
 CHECK( tm.in.empty() &&
	( tm.ev.back() == decltype( tm.ev )::value_type( mod::Cleared{} ) ) );
 // a batch with a duplicate insertion found, a mute
 DT d;
 TableMirror m2;
 d.attach( m2 );
 {
  auto g = batch( d );
  findOrMake( d , pool , 5 );
  findOrMake( d , pool , 5 );
  findOrMake( d , pool , 6 );
  CHECK( m2.ev.empty() );
  }
 CHECK( ( m2.ev.size() == 2 ) && m2.audit( d ) );
 {
  auto q = quiet( d );
  findOrMake( d , pool , 8 );
  }
 CHECK( m2.ev.back() == decltype( m2.ev )::value_type( mod::Rebuilt{} ) );
 // a move: Cleared to the table moved from, Rebuilt to the one moved into
 DT e;
 TableMirror m3;
 e.attach( m3 );
 e = std::move( d );
 CHECK( ( m2.ev.back() == decltype( m2.ev )::value_type( mod::Cleared{} ) ) &&
	( m3.ev.back() == decltype( m3.ev )::value_type( mod::Rebuilt{} ) ) &&
	( e.size() == 3 ) && ( d.size() == 0 ) );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File engineered_emitters_dict.cpp -----------------*/
/*--------------------------------------------------------------------------*/
