/*--------------------------------------------------------------------------*/
/*-------------------- File engineered_modification.cpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the typed modifications of Modification.h, beyond the book: the
 * events, their effects, directions and inverses; every optional part of
 * an observer, found at compile time and used by the static and by the
 * dynamic subjects (spans of one kind and of the variant, group hooks with
 * and without the versions, prepare(), audit(), mask(), Keep, the events
 * required, the dependencies); the policies NoObservers, Static, Dynamic
 * and Both; nested batches and every rule of the coalescing (cancellation,
 * an erasure and an insertion into changes or not, composition and
 * reductions, the touches, the nuclear events, the shifts, the
 * replacements, the relocations as boundaries, the copies of the bulk
 * events, the handles with no hash); the strong guarantee by the inverse
 * events, the refusals, the batches that exit by an exception, UndoLog;
 * the mutes; the revisions; the audits and the counts; Shadow; and the
 * graph wrapper with the observers of the new form. At the level 3 of the
 * contracts.
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
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"

#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ins = mod::Inserted< int , int , int >;
using Era = mod::Erased< int , int , int >;
using KC = mod::KeyChanged< int , int >;
using As = mod::Assigned< int , int >;
using CC = mod::Changed< mod::Cost , int , int >;
using Sh = mod::Shifted< mod::Key , int >;
using Events = mod::list< Ins , Era , KC , As , CC , Sh , mod::Cleared ,
                          mod::Rebuilt >;
using Ev = mod::variant_t< Events >;

/*--------------------------------------------------------------------------*/
/*---------------------------- A KEYED STRUCTURE ---------------------------*/
/*--------------------------------------------------------------------------*/

/// n places of items with a key, a value and a cost, each with a stable
/// handle, its changes told through the subject policy `Policy`; with
/// `Undo`, checkpoints of its whole state
template< class Policy , bool Undo = false >
class Keyed {
 public:

 using modifications = Events;
 using Subject = typename Policy::template bind< modifications , Keyed >;

 /// an item
 struct Item {
  bool live = false;
  int key = 0 , value = 0 , cost = 0;
  /// the same item, or both places empty
  bool operator==( const Item & y ) const {
   return( ( live == y.live ) &&
           ( ! live || ( ( key == y.key ) && ( value == y.value ) &&
                         ( cost == y.cost ) ) ) );
   }
  };

 template< class ... O >
 explicit Keyed( int n , O & ... o )
  : f_i( std::size_t( n ) ) , f_sub( o... ) {}

 void insert( int h , int k , int v ) {
  ADS_PRE( ! at( h ).live );
  const Ins e{ h , k , v };
  f_sub.prepare( e );
  at( h ) = Item{ true , k , v , 0 };
  f_sub.emit( e , [ & ]() noexcept { at( h ).live = false; } );
  f_sub.audit( *this );
  }

 void erase( int h ) {
  ADS_PRE( at( h ).live );
  f_sub.emit( Era{ h , at( h ).key , at( h ).value } );   // before
  at( h ).live = false;
  f_sub.audit( *this );
  }

 void setKey( int h , int k ) {
  const KC e{ h , at( h ).key , k };
  f_sub.prepare( e );
  at( h ).key = k;
  f_sub.emit( e , [ & , old = e.from ]() noexcept { at( h ).key = old; } );
  f_sub.audit( *this );
  }

 void assign( int h , int v ) {
  const As e{ h , at( h ).value , v };
  at( h ).value = v;
  f_sub.emit( e , [ & , old = e.from ]() noexcept { at( h ).value = old; } );
  f_sub.audit( *this );
  }

 void setCost( int h , int c ) {
  const CC e{ h , at( h ).cost , c };
  at( h ).cost = c;
  f_sub.emit( e );
  f_sub.audit( *this );
  }

 void shift( int d ) {
  for( auto & x : f_i )
   if( x.live )
    x.key += d;
  f_sub.emit( Sh{ d } );
  f_sub.audit( *this );
  }

 void clear( void ) {
  for( auto & x : f_i )
   x.live = false;
  f_sub.emit( mod::Cleared{} );
  }

 void rebuild( void ) { f_sub.emit( mod::Rebuilt{} ); }

 /// the change of an event, for UndoLog
 void apply( const Ins & e ) { insert( e.h , e.key , e.value ); }
 void apply( const Era & e ) { erase( e.h ); }
 void apply( const KC & e ) { setKey( e.h , e.to ); }
 void apply( const As & e ) { assign( e.h , e.to ); }
 void apply( const CC & e ) { setCost( e.h , e.to ); }
 void apply( const Sh & e ) { shift( e.delta ); }

 /// a checkpoint of the whole state
 std::size_t checkpoint( void ) requires Undo {
  f_snap.push_back( f_i );
  return( f_snap.size() - 1 );
  }

 /// the state of the checkpoint t
 void rollback( std::size_t t ) requires Undo {
  f_i = f_snap[ t ];
  f_snap.resize( t );
  }

 [[nodiscard]] const Item & item( int h ) const {
  return( f_i[ std::size_t( h ) ] );
  }
 [[nodiscard]] const std::vector< Item > & items( void ) const {
  return( f_i );
  }

 Subject & subject( void ) { return( f_sub ); }

 private:

 Item & at( int h ) { return( f_i[ std::size_t( h ) ] ); }

 std::vector< Item > f_i;
 std::vector< std::vector< Item > > f_snap;
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ THE OBSERVERS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// records every event, one by one
struct Rec {
 std::vector< Ev > one;
 template< class E >
  requires mod::contains_v< E , Events >
 void on( const E & e ) noexcept { one.push_back( Ev( e ) ); }
 };

/// records the events one by one and the batches as spans of the variant
struct SpanRec {
 std::vector< Ev > one;
 std::vector< std::vector< Ev > > batches;
 template< class E >
  requires mod::contains_v< E , Events >
 void on( const E & e ) noexcept { one.push_back( Ev( e ) ); }
 void on( std::span< const Ev > s ) {
  batches.emplace_back( s.begin() , s.end() );
  }
 };

/// wants the insertions only, as spans of their kind, and the erasures
/// one by one
struct KindSpan {
 std::vector< std::vector< Ins > > runs;
 std::vector< Era > erased;
 void on( const Ins & e ) { runs.push_back( { e } ); }
 void on( std::span< const Ins > s ) { runs.emplace_back( s.begin() , s.end() ); }
 void on( const Era & e ) noexcept { erased.push_back( e ); }
 };

/// counts the groups, with the versions
struct Grouped {
 int begins = 0 , ends = 0 , events = 0;
 mod::Group last;
 void on( const KC & ) { ++events; }
 void group_begin( const mod::Group & g ) { ++begins; last = g; }
 void group_end( void ) { ++ends; }
 };

/// counts the groups, without the versions
struct PlainGrouped {
 int begins = 0 , ends = 0;
 void on( const As & ) {}
 void group_begin( void ) { ++begins; }
 void group_end( void ) { ++ends; }
 };

/// refuses the key 13
struct Refuser {
 int told = 0;
 bool prepare( const KC & e ) const { return( e.to != 13 ); }
 bool prepare( const Ins & e ) const { return( e.key != 13 ); }
 void on( const KC & ) { ++told; }
 void on( const Ins & ) { ++told; }
 };

/// throws at the insertions and at the changes of key when armed
struct Thrower {
 bool armed = false;
 int told = 0;
 void on( const Ins & ) {
  if( armed )
   throw( std::runtime_error( "thrower" ) );
  ++told;
  }
 void on( const KC & ) {
  if( armed )
   throw( std::runtime_error( "thrower" ) );
  ++told;
  }
 };

/// leaves at its first event
struct Leaver {
 int told = 0;
 Keep on( const As & ) { ++told; return( Keep::Detach ); }
 };

/// a filter of the kinds at run time
struct Masked {
 std::uint64_t m = 0;
 int told = 0;
 [[nodiscard]] std::uint64_t mask( void ) const { return( m ); }
 void on( const Ins & ) { ++told; }
 void on( const KC & ) { ++told; }
 };

/// audits itself by a flag
struct Auditor {
 bool ok = true;
 mutable int audits = 0;
 void on( const As & ) {}
 template< class S >
 bool audit( const S & ) const {
  ++audits;
  return( ok );
  }
 };

/// the sum of the values of the live items, kept by the events
struct SumIncremental {
 long long sum = 0;
 void on( const Ins & e ) { sum += e.value; }
 void on( const Era & e ) noexcept { sum -= e.value; }
 void on( const As & e ) { sum += e.to - e.from; }
 void on( const mod::Cleared & ) noexcept { sum = 0; }
 };

/// the sum of the values of the live items, from the last values seen
struct SumScratch {
 std::vector< std::optional< int > > v = std::vector< std::optional< int > >( 8 );
 bool broken = false;
 void on( const Ins & e ) { v[ std::size_t( e.h ) ] = e.value; }
 void on( const Era & e ) noexcept { v[ std::size_t( e.h ) ].reset(); }
 void on( const As & e ) {
  v[ std::size_t( e.h ) ] = broken ? e.from : e.to;
  }
 void on( const mod::Cleared & ) noexcept {
  for( auto & x : v )
   x.reset();
  }
 [[nodiscard]] long long sum( void ) const {
  long long s = 0;
  for( const auto & x : v )
   s += x.value_or( 0 );
  return( s );
  }
 void flush( void ) {}
 };

/// an observer of the insertions that its successor depends on
struct First {
 void on( const Ins & ) {}
 };

/// depends on First
struct Second {
 using depends_on = mod::list< First >;
 using requires_modifications = mod::list< Ins >;
 void on( const Ins & ) {}
 };

using KN = Keyed< NoObservers >;
using KD = Keyed< Dynamic<> >;

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------------------- THE EVENTS ----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-events" )
{
 static_assert( Ins::effects == mod::effect::grows );
 static_assert( Era::effects == mod::effect::shrinks );
 static_assert( KC::effects == ( mod::effect::up | mod::effect::down ) );
 static_assert( mod::Cleared::effects == mod::effect::all );
 static_assert( mod::effects_of< int > == mod::effect::all );
 static_assert( mod::role_of< Ins > == mod::Role::Insert );
 static_assert( mod::role_of< int > == mod::Role::Other );
 static_assert( mod::must_not_throw< Era > && mod::must_not_throw< mod::Rebuilt > );
 static_assert( mod::must_not_throw< mod::Replaced< mod::Cost > > );
 static_assert( mod::must_not_throw< mod::BulkErased< int > > );
 static_assert( ! mod::must_not_throw< Ins > && ! mod::must_not_throw< KC > );
 static_assert( sizeof( mod::Inserted< int , int > ) == 2 * sizeof( int ) );
 static_assert( mod::index_v< KC , Events > == 2 );
 static_assert( mod::index_v< double , Events > == Events::size );
 static_assert( mod::contains_v< Sh , Events > && ! mod::contains_v< int , Events > );
 static_assert( std::is_same_v< mod::variant_t< mod::list< int , char > > ,
                                std::variant< int , char > > );
 static_assert( mod::invertible< Ins > && mod::invertible< Sh > );
 static_assert( ! mod::invertible< mod::Cleared > );
 static_assert( std::is_same_v< mod::reduction_of< mod::Key >::type ,
                                mod::reduce::last > );
 // the directions
 CHECK( ( KC{ 0 , 5 , 3 } ).direction() == mod::effect::down );
 CHECK( ( KC{ 0 , 3 , 5 } ).direction() == mod::effect::up );
 CHECK( ( KC{ 0 , 3 , 3 } ).direction() == 0 );
 struct NoOrder { int x; bool operator==( const NoOrder & ) const = default; };
 CHECK( ( mod::Assigned< int , NoOrder >{ 0 , { 1 } , { 2 } } ).direction() ==
        ( mod::effect::up | mod::effect::down ) );
 CHECK( ( Sh{ -2 } ).direction() == mod::effect::down );
 CHECK( ( Sh{ 2 } ).direction() == mod::effect::up );
 CHECK( ( Sh{ 0 } ).direction() == 0 );
 // the inverses
 CHECK( mod::inverse( Ins{ 1 , 2 , 3 } ) == ( Era{ 1 , 2 , 3 } ) );
 CHECK( mod::inverse( Era{ 1 , 2 , 3 } ) == ( Ins{ 1 , 2 , 3 } ) );
 CHECK( mod::inverse( mod::Inserted< int , int >{ 1 , 2 } ) ==
        ( mod::Erased< int , int >{ 1 , 2 } ) );
 CHECK( mod::inverse( mod::Erased< int , int >{ 4 , 2 } ) ==
        ( mod::Inserted< int , int >{ 4 , 2 } ) );
 CHECK( mod::inverse( KC{ 1 , 2 , 3 } ) == ( KC{ 1 , 3 , 2 } ) );
 CHECK( mod::inverse( mod::Relocated< int >{ 1 , 2 } ) ==
        ( mod::Relocated< int >{ 2 , 1 } ) );
 CHECK( mod::inverse( Sh{ 5 } ) == ( Sh{ -5 } ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE OPTIONAL PARTS --------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-observer-parts" )
{
 // found at compile time
 static_assert( mod::observes< KindSpan , Ins > &&
                ! mod::observes< KindSpan , KC > );
 static_assert( mod::observes_span< KindSpan , Ins > &&
                ! mod::observes_span< KindSpan , Era > );
 static_assert( mod::observes_span< SpanRec , Ev > );
 static_assert( mod::groups< Grouped > && mod::groups< PlainGrouped > &&
                ! mod::groups< Rec > );
 static_assert( mod::refuses< Refuser , KC > && ! mod::refuses< Rec , KC > );
 static_assert( mod::auditable< Auditor , KN > && ! mod::auditable< Rec , KN > );
 static_assert( mod::masked< Masked > && ! mod::masked< Rec > );
 static_assert( mod::lazy< SumScratch > && ! mod::lazy< Rec > );
 static_assert( mod::observer_of< Rec , Events > &&
                ! mod::observer_of< Rec , mod::list< int > > );
 static_assert( mod::counted< Shadow< SumIncremental , SumScratch > > );

 // a static set of every form, then the same attached dynamically
 for( int dyn = 0 ; dyn < 2 ; ++dyn ) {
  Rec r;
  SpanRec sr;
  KindSpan ks;
  Grouped gr;
  PlainGrouped pg;
  Leaver lv;
  Masked mk;
  mk.m = std::uint64_t( 1 ) << mod::index_v< KC , Events >;
  auto run = [ & ]( auto & s ) {
   s.insert( 0 , 10 , 100 );
   s.insert( 1 , 11 , 101 );
   s.setKey( 0 , 12 );
   s.assign( 0 , 7 );                    // Leaver leaves here
   s.assign( 0 , 8 );
   {
    auto b = ADS::batch( s );
    s.insert( 2 , 20 , 200 );
    s.insert( 3 , 30 , 300 );
    s.erase( 1 );
    s.setKey( 0 , 13 );
    s.assign( 0 , 9 );
    }
   CHECK( r.one.size() == 10 );
   CHECK( sr.one.size() == 5 && sr.batches.size() == 1 );
   CHECK( sr.batches[ 0 ].size() == 5 );
   // the runs of the insertions: one by one outside, one span inside
   CHECK( ks.runs.size() == 3 && ks.runs[ 2 ].size() == 2 );
   CHECK( ks.erased.size() == 1 && ( ks.erased[ 0 ] == Era{ 1 , 11 , 101 } ) );
   CHECK( gr.events == 2 && gr.begins == 1 && gr.ends == 1 );
   CHECK( gr.last.size == 5 && gr.last.to > gr.last.from );
   CHECK( pg.begins == 1 && pg.ends == 1 );
   CHECK( lv.told == 1 );
   CHECK( mk.told == 2 );                 // the changes of key only
   };
  if( dyn == 0 ) {
   Keyed< Static< Rec , SpanRec , KindSpan , Grouped , PlainGrouped ,
                  Leaver , Masked > > s( 8 , r , sr , ks , gr , pg , lv , mk );
   run( s );
   CHECK( s.subject().mask() != 0 );
   }
  else {
   KD s( 8 );
   for( auto * o : { & r } )
    s.subject().attach( *o );
   s.subject().attach( sr );
   s.subject().attach( ks );
   s.subject().attach( gr );
   s.subject().attach( pg );
   const int l = s.subject().attach( lv );
   s.subject().attach( mk );
   run( s );
   CHECK( ! s.subject().attached( l ) );     // it left
   }
  }

 // the dependencies and the events required, in order
 First f;
 Second g;
 Keyed< Static< First , Second > > s( 2 , f , g );
 s.insert( 0 , 1 , 1 );
 // a static observer through an adapter
 struct Counter { int n = 0; void bump( void ) { ++n; } };
 struct Bump {
  static void one( Counter & c , const Ins & ) { c.bump(); }
  };
 Counter c;
 Keyed< Static< mod::Via< Counter , Bump > > > t( 2 , c );
 t.insert( 0 , 1 , 1 );
 t.setKey( 0 , 2 );
 CHECK( c.n == 1 );
 static_assert( ! decltype( t )::Subject::listened< KC > );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE POLICIES ------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-policies" )
{
 // nobody: no member, every test false at compile time
 static_assert( sizeof( KN::Subject ) == 1 );
 static_assert( ! KN::Subject::listened< Ins > && KN::Subject::none );
 static_assert( ! KN::Subject::dynamic && KD::Subject::dynamic );
 static_assert( Keyed< Static< Rec > >::Subject::listened< KC > );
 static_assert( ! Keyed< Static< KindSpan > >::Subject::listened< KC > );
 static_assert( KD::Subject::listened< KC > );
 KN n( 4 );
 n.insert( 0 , 1 , 2 );
 {
  auto b = ADS::batch( n );
  n.setKey( 0 , 3 );
  auto q = ADS::quiet( n );
  }
 CHECK( n.item( 0 ).key == 3 && n.subject().built() == 0 );
 CHECK( n.subject().version() == 0 && n.subject().depth() == 0 );

 // dynamic: attach, the mask, detach, the contracts
 KD d( 4 );
 CHECK( ! d.subject().live< Ins >() && d.subject().mask() == 0 );
 d.insert( 0 , 1 , 1 );                   // nobody: nothing built
 CHECK( d.subject().version() == 0 );
 KindSpan ks;
 Rec r;
 const int a = d.subject().attach( ks );
 CHECK( d.subject().live< Ins >() && ! d.subject().live< KC >() );
 const int b = d.subject().attach( r );
 CHECK( d.subject().live< KC >() );
 d.insert( 1 , 2 , 2 );
 CHECK( ks.runs.size() == 1 && r.one.size() == 1 );
 d.subject().detach( b );
 CHECK( ! d.subject().live< KC >() );
 CHECK_THROWS( d.subject().detach( b ) , contract_violation );
 CHECK_THROWS( d.subject().detach( 7 ) , contract_violation );
 {
  auto bt = ADS::batch( d );
  CHECK_THROWS( d.subject().attach( r ) , contract_violation );
  CHECK_THROWS( d.subject().detach( a ) , contract_violation );
  }
 // an observer that attaches from within on(): refused
 struct Attacher {
  KD * s = nullptr;
  Rec other;
  bool refused = false;
  void on( const As & ) {
   try { s->subject().attach( other ); }
   catch( const contract_violation & ) { refused = true; }
   }
  };
 Attacher at;
 at.s = & d;
 d.subject().attach( at );
 d.assign( 0 , 5 );
 CHECK( at.refused );

 // both: the static ones first, then the dynamic ones
 std::vector< int > order;
 struct Tag {
  std::vector< int > * o;
  int id;
  void on( const Ins & ) { o->push_back( id ); }
  };
 Tag t1{ & order , 1 } , t2{ & order , 2 } , t3{ & order , 3 };
 Keyed< Both< Static< Tag , Tag > > > bo( 4 , t1 , t2 );
 bo.subject().attach( t3 );
 bo.insert( 0 , 0 , 0 );
 CHECK( ( order == std::vector< int >{ 1 , 2 , 3 } ) );
 // Dynamic< L > with the events of the structure
 Keyed< Dynamic< Events > > de( 2 );
 de.insert( 0 , 1 , 1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE BATCHES -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-batch" )
{
 SpanRec sr;
 Keyed< Static< SpanRec > > s( 8 , sr );
 s.insert( 0 , 1 , 1 );
 s.insert( 1 , 2 , 2 );
 sr.one.clear();
 {
  auto b = ADS::batch( s );
  s.setKey( 0 , 5 );
  {
   auto inner = ADS::batch( s );         // nested: the outer one sends
   CHECK( s.subject().depth() == 2 );
   s.setKey( 0 , 6 );
   s.insert( 2 , 3 , 3 );
   }
  CHECK( sr.batches.empty() && sr.one.empty() );
  s.erase( 1 );
  b.close();
  b.close();                              // idempotent
  CHECK( s.subject().depth() == 0 );
  }
 CHECK( sr.batches.size() == 1 );
 const auto & B = sr.batches[ 0 ];
 CHECK( B.size() == 3 );
 CHECK( B[ 0 ] == Ev( KC{ 0 , 1 , 6 } ) );
 CHECK( B[ 1 ] == Ev( Ins{ 2 , 3 , 3 } ) );
 CHECK( B[ 2 ] == Ev( Era{ 1 , 2 , 2 } ) );
 // a batch that cancels out sends nothing, an empty one too
 {
  auto b = ADS::batch( s );
  s.setKey( 0 , 9 );
  s.setKey( 0 , 6 );
  }
 { auto b = ADS::batch( s ); }
 CHECK( sr.batches.size() == 1 );
 // a batch moved keeps its channel
 {
  auto b = ADS::batch( s );
  auto c = std::move( b );
  b.close();                              // moved from: nothing
  CHECK( s.subject().depth() == 1 );
  s.assign( 0 , 4 );
  }
 CHECK( sr.batches.size() == 2 && sr.batches[ 1 ].size() == 1 );
 // the bulk events are copied: the span of the caller may die
 using BI = mod::BulkInserted< int >;
 using PE = mod::Permuted< int >;
 using BL = mod::list< BI , PE >;
 struct BulkRec {
  std::vector< std::vector< int > > seen;
  void on( const BI & e ) { seen.emplace_back( e.hs.begin() , e.hs.end() ); }
  void on( const PE & e ) {
   seen.emplace_back( e.old_of_new.begin() , e.old_of_new.end() );
   }
  };
 BulkRec br;
 Static< BulkRec >::bind< BL > sub( br );
 sub.open();
 {
  std::vector< int > hs = { 4 , 5 , 6 };
  sub.emit( BI{ std::span< const int >( hs ) } );
  std::vector< int > p = { 2 , 0 , 1 };
  sub.emit( PE{ std::span< const int >( p ) } );
  hs.assign( 3 , -1 );                    // the caller changes them
  p.assign( 3 , -1 );
  }
 sub.close( 0 );
 CHECK( br.seen.size() == 2 );
 CHECK( ( br.seen[ 0 ] == std::vector< int >{ 4 , 5 , 6 } ) );
 CHECK( ( br.seen[ 1 ] == std::vector< int >{ 2 , 0 , 1 } ) );
 CHECK( sub.buffered() == 0 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE COALESCING -----------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// the net effect of B by the default rule of Events
std::vector< Ev > net( const std::vector< Ev > & B )
{
 return( mod::coalesce_traits< Events >::coalesce(
                                          std::span< const Ev >( B ) ) );
 }

/// an attribute that keeps the least value of a batch
struct Low {
 using reduction = mod::reduce::min;
 };
/// an attribute that keeps the largest
struct High {
 using reduction = mod::reduce::max;
 };
/// an attribute whose differences add up
struct Total {
 using reduction = mod::reduce::sum;
 };

}  // end( namespace )

ADS_TEST( "mod-coalesce" )
{
 // an insertion and an erasure cancel, with the changes between them
 CHECK( net( { Ins{ 0 , 1 , 1 } , KC{ 0 , 1 , 2 } , CC{ 0 , 0 , 5 } ,
               As{ 0 , 1 , 3 } , Era{ 0 , 2 , 3 } } ).empty() );
 // the changes of key and value fold into the insertion; the cost stays
 CHECK( ( net( { Ins{ 0 , 1 , 1 } , KC{ 0 , 1 , 2 } , CC{ 0 , 0 , 5 } ,
                 As{ 0 , 1 , 3 } } ) ==
          std::vector< Ev >{ Ins{ 0 , 2 , 3 } , CC{ 0 , 0 , 5 } } ) );
 // changes then an erasure: the erasure of what there was before
 CHECK( ( net( { KC{ 0 , 1 , 2 } , As{ 0 , 5 , 6 } , KC{ 0 , 2 , 3 } ,
                 Era{ 0 , 3 , 6 } } ) ==
          std::vector< Ev >{ Era{ 0 , 1 , 5 } } ) );
 // an erasure and an insertion: the changes, or nothing if equal
 CHECK( ( net( { Era{ 0 , 1 , 5 } , Ins{ 0 , 2 , 6 } } ) ==
          std::vector< Ev >{ KC{ 0 , 1 , 2 } , As{ 0 , 5 , 6 } } ) );
 CHECK( ( net( { Era{ 0 , 1 , 5 } , Ins{ 0 , 1 , 7 } } ) ==
          std::vector< Ev >{ As{ 0 , 5 , 7 } } ) );
 CHECK( net( { Era{ 0 , 1 , 5 } , Ins{ 0 , 1 , 5 } } ).empty() );
 // ... kept as they are when the structure has no change of that kind
 {
  using I2 = mod::Inserted< int , int >;
  using E2 = mod::Erased< int , int >;
  using L2 = mod::list< I2 , E2 >;
  using V2 = mod::variant_t< L2 >;
  const std::vector< V2 > B = { E2{ 0 , 1 } , I2{ 0 , 2 } , E2{ 1 , 4 } ,
                                I2{ 1 , 4 } };
  const auto C = mod::coalesce_traits< L2 >::coalesce(
                                             std::span< const V2 >( B ) );
  CHECK( ( C == std::vector< V2 >{ E2{ 0 , 1 } , I2{ 0 , 2 } } ) );
  }
 // consecutive changes compose; a touch vanishes; other attributes commute
 CHECK( ( net( { KC{ 0 , 1 , 2 } , CC{ 0 , 0 , 4 } , KC{ 0 , 2 , 3 } } ) ==
          std::vector< Ev >{ KC{ 0 , 1 , 3 } , CC{ 0 , 0 , 4 } } ) );
 CHECK( net( { KC{ 0 , 1 , 2 } , KC{ 0 , 2 , 1 } } ).empty() );
 CHECK( net( { As{ 3 , 4 , 4 } } ).empty() );
 // handles apart, in the order of their first event
 CHECK( ( net( { KC{ 1 , 1 , 2 } , KC{ 0 , 5 , 6 } , KC{ 1 , 2 , 4 } } ) ==
          std::vector< Ev >{ KC{ 1 , 1 , 4 } , KC{ 0 , 5 , 6 } } ) );
 // the reductions of the attributes
 {
  using L3 = mod::list< mod::Changed< Low , int , int > ,
                        mod::Changed< High , int , int > ,
                        mod::Changed< Total , int , int > ,
                        mod::Changed< mod::Value , int , int > >;
  using V3 = mod::variant_t< L3 >;
  using CL = mod::Changed< Low , int , int >;
  using CH = mod::Changed< High , int , int >;
  using CT = mod::Changed< Total , int , int >;
  using CV = mod::Changed< mod::Value , int , int >;
  const std::vector< V3 > B = { CL{ 0 , 9 , 5 } , CL{ 0 , 9 , 7 } ,
                                CH{ 1 , 0 , 5 } , CH{ 1 , 0 , 3 } ,
                                CT{ 2 , 0 , 5 } , CT{ 2 , 0 , 3 } ,
                                CV{ 3 , 0 , 5 } , CV{ 3 , 0 , 3 } };
  const auto C = mod::coalesce_traits< L3 >::coalesce(
                                             std::span< const V3 >( B ) );
  CHECK( ( C == std::vector< V3 >{ CL{ 0 , 9 , 5 } , CH{ 1 , 0 , 5 } ,
                                   CT{ 2 , 0 , 8 } , CV{ 3 , 0 , 3 } } ) );
  }
 // everything before the last Cleared or Rebuilt is dropped
 CHECK( ( net( { Ins{ 0 , 1 , 1 } , mod::Cleared{} , Ins{ 1 , 2 , 2 } ,
                 mod::Rebuilt{} , KC{ 1 , 2 , 3 } } ) ==
          std::vector< Ev >{ mod::Rebuilt{} , KC{ 1 , 2 , 3 } } ) );
 CHECK( ( net( { mod::Rebuilt{} , mod::Cleared{} } ) ==
          std::vector< Ev >{ mod::Cleared{} } ) );
 // the shifts add up, to nothing if they cancel; they bound the runs
 CHECK( ( net( { KC{ 0 , 1 , 2 } , Sh{ 2 } , Sh{ 3 } , KC{ 0 , 7 , 8 } } ) ==
          std::vector< Ev >{ KC{ 0 , 1 , 2 } , Sh{ 5 } , KC{ 0 , 7 , 8 } } ) );
 CHECK( ( net( { Sh{ 2 } , Sh{ -2 } , KC{ 0 , 1 , 2 } } ) ==
          std::vector< Ev >{ KC{ 0 , 1 , 2 } } ) );
 // replacements of one attribute are one; relocations bound the runs
 {
  using R = mod::Replaced< mod::Cost >;
  using RL = mod::Relocated< int >;
  using L4 = mod::list< KC , R , RL >;
  using V4 = mod::variant_t< L4 >;
  const std::vector< V4 > B = { KC{ 0 , 1 , 2 } , R{} , R{} , RL{ 0 , 5 } ,
                                KC{ 0 , 2 , 3 } , KC{ 0 , 3 , 4 } };
  const auto C = mod::coalesce_traits< L4 >::coalesce(
                                             std::span< const V4 >( B ) );
  CHECK( ( C == std::vector< V4 >{ KC{ 0 , 1 , 2 } , R{} , RL{ 0 , 5 } ,
                                   KC{ 0 , 2 , 4 } } ) );
  }
 // handles with no hash are not coalesced
 {
  struct NH {
   int x;
   bool operator==( const NH & ) const = default;
   };
  using KN2 = mod::KeyChanged< NH , int >;
  using L5 = mod::list< KN2 >;
  using V5 = mod::variant_t< L5 >;
  const std::vector< V5 > B = { KN2{ { 0 } , 1 , 2 } , KN2{ { 0 } , 2 , 3 } };
  CHECK( mod::coalesce_traits< L5 >::coalesce(
                                std::span< const V5 >( B ) ).size() == 2 );
  }
 // values with no == never vanish
 {
  struct NoEq { int x; };
  using AN = mod::Assigned< int , NoEq >;
  using L6 = mod::list< AN >;
  using V6 = mod::variant_t< L6 >;
  const std::vector< V6 > B = { AN{ 0 , { 1 } , { 2 } } ,
                                AN{ 0 , { 2 } , { 1 } } };
  CHECK( mod::coalesce_traits< L6 >::coalesce(
                                std::span< const V6 >( B ) ).size() == 1 );
  }
 CHECK( net( {} ).empty() );
 // a long random sequence: replaying the net effect gives the same state
 std::vector< std::optional< std::pair< int , int > > > s0( 16 ) , s1 , s2;
 unsigned x = 7;
 auto rnd = [ & ]( unsigned m ) {
  x = x * 1103515245u + 12345u;
  return( int( ( x >> 16 ) % m ) );
  };
 for( int round = 0 ; round < 200 ; ++round ) {
  s1 = s0;
  std::vector< Ev > B;
  for( int k = 0 ; k < 30 ; ++k ) {
   const int h = rnd( 16 );
   auto & it = s1[ std::size_t( h ) ];
   if( ! it ) {
    it = std::pair< int , int >( rnd( 5 ) , rnd( 5 ) );
    B.push_back( Ins{ h , it->first , it->second } );
    }
   else if( rnd( 3 ) == 0 ) {
    B.push_back( Era{ h , it->first , it->second } );
    it.reset();
    }
   else if( rnd( 2 ) == 0 ) {
    const int k2 = rnd( 5 );
    B.push_back( KC{ h , it->first , k2 } );
    it->first = k2;
    }
   else {
    const int v2 = rnd( 5 );
    B.push_back( As{ h , it->second , v2 } );
    it->second = v2;
    }
   }
  const auto C = net( B );
  CHECK( C.size() <= B.size() );
  s2 = s0;
  for( const auto & e : C )
   std::visit( [ & ]( const auto & y ) {
    using Y = std::decay_t< decltype( y ) >;
    if constexpr( ! mod::detail::element< Y > )
     CHECK( false );
    else {
    auto & it = s2[ std::size_t( y.h ) ];
    if constexpr( std::is_same_v< Y , Ins > ) {
     CHECK( ! it );
     it = std::pair< int , int >( y.key , y.value );
     }
    else if constexpr( std::is_same_v< Y , Era > ) {
     CHECK( it && ( it->first == y.key ) && ( it->second == y.value ) );
     it.reset();
     }
    else if constexpr( std::is_same_v< Y , KC > ) {
     CHECK( it && ( it->first == y.from ) );
     it->first = y.to;
     }
    else if constexpr( std::is_same_v< Y , As > ) {
     CHECK( it && ( it->second == y.from ) );
     it->second = y.to;
     }
    else
     CHECK( false );
    } } , e );
  CHECK( s2 == s1 );
  s0 = s1;
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE ROLLBACK -----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-rollback" )
{
 // an observer throws: those told before get the inverse, in reverse
 // order, the structure undoes its change, the exception goes on
 Rec r1 , r3;
 Thrower th;
 Keyed< Both< Static< Rec , Thrower > > > s( 4 , r1 , th );
 s.subject().attach( r3 );
 s.insert( 0 , 1 , 1 );
 th.armed = true;
 CHECK_THROWS( s.insert( 1 , 2 , 2 ) , std::runtime_error );
 CHECK( ! s.item( 1 ).live );
 CHECK( r1.one.size() == 3 && r3.one.size() == 1 );   // r3 never told
 CHECK( r1.one[ 1 ] == Ev( Ins{ 1 , 2 , 2 } ) );
 CHECK( r1.one[ 2 ] == Ev( Era{ 1 , 2 , 2 } ) );
 CHECK_THROWS( s.setKey( 0 , 5 ) , std::runtime_error );
 CHECK( s.item( 0 ).key == 1 );
 CHECK( r1.one.back() == Ev( KC{ 0 , 5 , 1 } ) );
 // a dynamic observer told before the one that throws
 {
  Rec a;
  Thrower t;
  KD d( 4 );
  d.subject().attach( a );
  d.subject().attach( t );
  t.armed = true;
  CHECK_THROWS( d.insert( 0 , 1 , 1 ) , std::runtime_error );
  CHECK( a.one.size() == 2 && ( a.one[ 1 ] == Ev( Era{ 0 , 1 , 1 } ) ) );
  CHECK( ! d.item( 0 ).live );
  }
 // a Keep::Detach returned by the inverse is honoured after the rollback
 {
  struct LeaveOnUndo {
   int told = 0;
   void on( const KC & ) { ++told; }
   Keep on( const As & ) { return( Keep::Detach ); }
   };
  struct ThrowAs {
   void on( const As & ) { throw( std::runtime_error( "as" ) ); }
   };
  LeaveOnUndo lu;
  ThrowAs ta;
  KD d( 2 );
  const int id = d.subject().attach( lu );
  d.subject().attach( ta );
  d.insert( 0 , 1 , 1 );
  CHECK_THROWS( d.assign( 0 , 9 ) , std::runtime_error );
  CHECK( d.item( 0 ).value == 1 && ! d.subject().attached( id ) );
  }
 // a refusal: nothing changes and nobody is told
 Refuser rf;
 Keyed< Static< Refuser > > p( 4 , rf );
 p.insert( 0 , 1 , 1 );
 CHECK_THROWS( p.setKey( 0 , 13 ) , Refused );
 CHECK_THROWS( p.insert( 1 , 13 , 1 ) , Refused );
 CHECK( p.item( 0 ).key == 1 && ! p.item( 1 ).live && rf.told == 1 );
 {
  KD d( 2 );
  Refuser rd;
  d.subject().attach( rd );
  CHECK_THROWS( d.insert( 0 , 13 , 0 ) , Refused );
  CHECK( ! d.item( 0 ).live );
  }
 // a batch that exits by an exception, on an undoable structure: back to
 // its checkpoint, and the observers see nothing
 {
  SpanRec sr;
  Keyed< Static< SpanRec > , true > u( 4 , sr );
  u.insert( 0 , 1 , 1 );
  const auto before = u.items();
  try {
   auto b = ADS::batch( u );
   u.setKey( 0 , 4 );
   u.insert( 1 , 2 , 2 );
   throw( std::runtime_error( "abort" ) );
   }
  catch( const std::runtime_error & ) {}
  CHECK( u.items() == before && sr.batches.empty() );
  CHECK( u.subject().depth() == 0 );
  // Batch::rollback(), also nested: the outer batch keeps its events
  {
   auto outer = ADS::batch( u );
   u.assign( 0 , 7 );
   {
    auto inner = ADS::batch( u );
    u.setKey( 0 , 9 );
    inner.rollback();
    }
   CHECK( u.item( 0 ).key == 1 );
   }
  CHECK( sr.batches.size() == 1 && sr.batches[ 0 ].size() == 1 );
  CHECK( sr.batches[ 0 ][ 0 ] == Ev( As{ 0 , 1 , 7 } ) );
  }
 // ... on one that is not: Rebuilt for the observers
 {
  SpanRec sr;
  Keyed< Static< SpanRec > > u( 4 , sr );
  try {
   auto b = ADS::batch( u );
   u.insert( 0 , 1 , 1 );
   throw( std::runtime_error( "abort" ) );
   }
  catch( const std::runtime_error & ) {}
  CHECK( u.item( 0 ).live && u.subject().depth() == 0 );
  CHECK( sr.batches.size() == 1 &&
         ( sr.batches[ 0 ] == std::vector< Ev >{ mod::Rebuilt{} } ) );
  }
 // UndoLog: any structure with apply() becomes undoable
 {
  UndoLog< Events > log;
  SpanRec other;
  Keyed< Static< UndoLog< Events > , SpanRec > > u( 4 , log , other );
  u.insert( 0 , 1 , 10 );
  const std::size_t t = log.checkpoint();
  const auto before = u.items();
  u.setKey( 0 , 2 );
  u.assign( 0 , 20 );
  u.insert( 1 , 3 , 30 );
  u.erase( 0 );
  u.shift( 4 );
  CHECK( log.size() == 6 );
  other.one.clear();
  log.rollback( u , t );
  CHECK( u.items() == before );
  CHECK( log.checkpoint() == t && log.size() == 1 );
  // the other observer gets the net effect of the undo, in one batch
  CHECK( other.one.empty() && other.batches.size() == 1 );
  CHECK( log.size() == 1 );               // the undo is not recorded
  CHECK_THROWS( log.rollback( u , t + 5 ) , contract_violation );
  u.clear();                              // no rollback before a Cleared
  CHECK( log.size() == 0 && log.checkpoint() == t );
  CHECK_THROWS( log.rollback( u , t - 1 ) , contract_violation );
  log.rollback( u , t );                  // nothing to undo
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE MUTES -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-quiet" )
{
 SpanRec sr;
 Auditor au;
 Keyed< Static< SpanRec , Auditor > > s( 4 , sr , au );
 // a mute: no event, Rebuilt at its end
 {
  auto q = ADS::quiet( s );
  s.insert( 0 , 1 , 1 );
  s.setKey( 0 , 2 );
  CHECK( sr.one.empty() && s.subject().muted() );
  }
 CHECK( ( sr.one == std::vector< Ev >{ mod::Rebuilt{} } ) );
 CHECK( ! s.subject().muted() );
 // no_rebuild: nothing, and the audits at its end
 const int audits = au.audits;
 {
  auto q = ADS::quiet( s , no_rebuild );
  s.setKey( 0 , 3 );
  q.end();
  q.end();                                // idempotent
  }
 CHECK( sr.one.size() == 1 && au.audits == audits + 1 );
 // nested mutes: one Rebuilt at the outermost end, if any asked for it
 {
  auto q1 = ADS::quiet( s , no_rebuild );
  {
   auto q2 = ADS::quiet( s );
   s.setKey( 0 , 4 );
   }
  CHECK( sr.one.size() == 1 );
  }
 CHECK( sr.one.size() == 2 && ( sr.one.back() == Ev( mod::Rebuilt{} ) ) );
 // a mute in a batch: the events collected dropped, Rebuilt at the close
 {
  auto b = ADS::batch( s );
  s.setKey( 0 , 5 );
  {
   auto q = ADS::quiet( s );
   s.setKey( 0 , 6 );
   }
  s.setKey( 0 , 7 );                      // after the mute: kept
  }
 CHECK( sr.batches.size() == 1 &&
        ( sr.batches[ 0 ] == std::vector< Ev >{ mod::Rebuilt{} ,
                                                KC{ 0 , 6 , 7 } } ) );
 // a mute with no_rebuild in a batch keeps what came before it
 {
  auto b = ADS::batch( s );
  s.setKey( 0 , 8 );
  {
   auto q = ADS::quiet( s , no_rebuild );
   s.setKey( 0 , 9 );
   }
  }
 CHECK( sr.batches.size() == 2 &&
        ( sr.batches[ 1 ] == std::vector< Ev >{ KC{ 0 , 7 , 8 } } ) );
 // the built events count the muted ones too
 CHECK( s.subject().built() == 9 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE REVISIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-revisions" )
{
 KindSpan ks;
 Grouped gr;
 Keyed< Both< Static< KindSpan , Grouped > > > s( 8 , ks , gr );
 CHECK( s.subject().version() == 0 );
 s.insert( 0 , 1 , 1 );
 s.insert( 1 , 2 , 2 );
 s.assign( 0 , 5 );                       // nobody wants it: no revision
 CHECK( s.subject().revision< Ins >() == 2 );
 CHECK( s.subject().revision< As >() == 0 );
 CHECK( s.subject().version() == 2 );
 s.setKey( 0 , 3 );
 CHECK( s.subject().revision< KC >() == 1 && s.subject().version() == 3 );
 CHECK( s.subject().revision( mod::index_v< KC , Events > ) == 1 );
 // a batch: the revisions of its net events, the range in the group
 {
  auto b = ADS::batch( s );
  s.setKey( 0 , 4 );
  s.setKey( 0 , 5 );
  s.insert( 2 , 3 , 3 );
  s.insert( 3 , 3 , 3 );
  }
 CHECK( s.subject().revision< KC >() == 2 );
 CHECK( s.subject().revision< Ins >() == 4 );
 CHECK( gr.last.from == 3 && gr.last.to == 6 && gr.last.size == 3 );
 // a dynamic observer attached late reads the version
 Rec r;
 s.subject().attach( r );
 const auto v = s.subject().version();
 s.assign( 0 , 6 );
 CHECK( s.subject().version() == v + 1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE AUDITS AND THE COUNTS ----------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-audit" )
{
 static_assert( ADS_CONTRACTS >= 3 );
 Auditor au;
 Rec r;
 Keyed< Both< Static< Auditor , Rec > > > s( 4 , au , r );
 Auditor ad;
 const int id = s.subject().attach( ad );
 s.insert( 0 , 1 , 1 );
 CHECK( au.audits == 1 && ad.audits == 1 );   // after each event
 {
  auto b = ADS::batch( s );
  s.assign( 0 , 2 );
  s.assign( 0 , 3 );
  CHECK( au.audits == 1 );                // not inside a batch
  }
 CHECK( au.audits == 2 );                 // after the batch
 au.ok = false;
 CHECK_THROWS( s.assign( 0 , 4 ) , contract_violation );
 au.ok = true;
 ad.ok = false;
 CHECK_THROWS( s.assign( 0 , 5 ) , contract_violation );
 ad.ok = true;
 // the counts of each observer, at the level of the audits
 const mod::Counts c = s.subject().staticCounts( 1 );
 CHECK( c.events == 3 && c.groups == 1 && c.rebuilds == 0 );
 CHECK( s.subject().staticCounts( 0 ).events == 2 );
 CHECK( s.subject().slotCounts( id ).events == 2 );
 CHECK( s.subject().slotCounts( id ).groups == 1 );
 s.rebuild();
 CHECK( s.subject().staticCounts( 1 ).rebuilds == 1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASS Shadow -----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-shadow" )
{
 SumIncremental inc;
 SumScratch scr;
 using Sd = Shadow< SumIncremental , SumScratch >;
 Sd sd( inc , scr );
 Keyed< Static< Sd > > s( 8 , sd );
 s.insert( 0 , 1 , 10 );
 s.insert( 1 , 1 , 20 );
 s.assign( 0 , 15 );
 {
  auto b = ADS::batch( s );
  s.erase( 1 );
  s.insert( 2 , 1 , 7 );
  }
 auto incSum = []( SumIncremental & x ) { return( x.sum ); };
 auto both = [ & ]( auto & o ) {
  if constexpr( std::is_same_v< std::decay_t< decltype( o ) > ,
                                SumIncremental > )
   return( o.sum );
  else
   return( o.sum() );
  };
 CHECK( sd.query( both ) == 22 && sd.agree( both ) );
 (void)( incSum );
 sd.flush();
 CHECK( sd.counts().events == 5 && sd.counts().queries == 2 );
 // a broken observer from scratch: the mismatch is found
 scr.broken = true;
 s.assign( 0 , 30 );
 CHECK( ! sd.agree( both ) );
 CHECK_THROWS( sd.query( both ) , contract_violation );
 CHECK( sd.counts().mismatches == 2 );
 CHECK( sd.audit( s ) );
 s.clear();
 CHECK( & sd.incremental() == & inc && & sd.scratch() == & scr );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE GRAPH OF THE NEW FORM ----------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "mod-observed-graph" )
{
 using LG = ListGraph< long long >;
 using W = WeightChanged< long long >;
 struct Lower {
  int n = 0;
  void on( const W & e ) { n += ( e.to < e.from ) ? 1 : 0; }
  };
 struct Nodes {
  int n = 0;
  void on( const NodeInserted & ) { ++n; }
  };
 // static, of the new form: only the kinds it wants are built
 LG g( 3 );
 Lower lo;
 ObservedGraph< LG , Static< Lower > > og( g , lo );
 og.insertEdge( 0 , 1 , 5 );
 og.insertEdge( 0 , 1 , 3 );              // a lower weight
 og.setWeight( 0 , 1 , 4 );
 og.insertNode();
 CHECK( lo.n == 1 && og.built() == 2 );
 // nobody: the plain graph, and batch() and quiet() still compile
 ObservedGraph< LG > none( g );
 none.setWeight( 0 , 1 , 1 );
 {
  auto b = ADS::batch( none );
  auto q = ADS::quiet( none , no_rebuild );
  none.insertEdge( 1 , 2 , 1 );
  }
 CHECK( none.built() == 0 && none.mask() == 0 && ! none.listening() );
 CHECK( g.weight( 0 , 1 ) == 1 );
 // dynamic, both forms attached
 struct Old {
  int n = 0;
  [[nodiscard]] unsigned mask( void ) const { return( GraphEvents::ALL ); }
  void notify( const GraphEvent< long long > & ) { ++n; }
  void notify( std::span< const GraphEvent< long long > > s ) {
   n += int( s.size() );
   }
  };
 Old old;
 Nodes nd;
 ObservedGraph< LG , Dynamic<> > dg( g );
 dg.attach( old );
 dg.attach( nd );
 dg.insertNode();
 {
  auto b = dg.batch();
  dg.insertNode();
  dg.deleteEdge( 1 , 2 );
  }
 CHECK( old.n == 3 && nd.n == 2 );
 // an observer of the new form attached to the old alias
 Observable< LG > ob( g );
 Nodes n2;
 ob.attach( n2 );
 ob.insertNode();
 CHECK( n2.n == 1 );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File engineered_modification.cpp ------------------*/
/*--------------------------------------------------------------------------*/
