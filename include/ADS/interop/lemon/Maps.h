/*--------------------------------------------------------------------------*/
/*------------------------------ File Maps.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The maps of LEMON on the structures of ADS, beyond the book: models of
 * the concepts `ReadMap`, `ReadWriteMap` and `ReferenceMap` of
 * `lemon/concepts/maps.h`, which the algorithms of LEMON take for their
 * input and output maps by named parameters (`SetPredMap`, `SetDistMap`,
 * `SetHeap< H , CR >`, `lengthMap()`) [see \ref theory_graph_lemon]:
 *
 * - `DenseMap< GR , T , Item >`, a `ReferenceMap` from the nodes, the arcs
 *   or the edges of a graph of LEMON to an array of `maxId( Item() ) + 1`
 *   values of `T` (never a `std::vector< bool >`, so that `Reference` is a
 *   true `bool &`) that does not attach to the notifier of the graph: a map
 *   of LEMON attaches at its construction under the lock of the notifier,
 *   so that \f$ k \f$ algorithms started at once on one graph serialize on
 *   that lock and change one shared list, while with `DenseMap` they share
 *   no mutable state and run on the threads of an executor of ADS [see
 *   `Parallel.h`]; the price is that it does not follow the graph, and is
 *   valid while the graph keeps its items;
 * - `IndexMap< GR , S , Item >`, a `ReadWriteMap` over a structure of ADS
 *   indexed by the ids of the items, `ObservableArray` [see
 *   `ObservableArray.h`], whose `set()` tells `Assigned` to its observers,
 *   `Fenwick` [see `Fenwick.h`], whose `set()` keeps the prefix aggregates,
 *   read by `prefix()`, or any structure with `size()`, `set( i , v )` and
 *   a read by `operator[]( i )` or `get( i )`; it does not own the
 *   structure, which must have room for `maxId( Item() ) + 1` values;
 * - `ObservedMap< GR , Map , A , Policy >`, a `ReadWriteMap` over a map of
 *   LEMON whose `set()` tells `Changed< A , int , V >` (the id of the item,
 *   the old and the new value) through the subject policy `Policy` [see
 *   `Modification.h`], since the maps of LEMON cannot be observed; a read
 *   costs that of the map below, and a write one more read of the old value
 *   when someone listens. A change of a cost or of a capacity must go
 *   through it to reach the observers; a direct write to the map below is
 *   not seen.
 *
 * The contracts [see `Contract.h`]: the ids of the items within the room of
 * the structure (for `IndexMap`) or of the array (for `DenseMap`). The maps
 * are defined in this file.
 *
 * \see B. Dezső, A. Jüttner, P. Kovács, LEMON - an open source C++ graph
 *      template library, Electronic Notes in Theoretical Computer Science
 *      264(5), 23-45, 2011, doi:10.1016/j.entcs.2011.06.003
 * \see P. M. Fenwick, A new data structure for cumulative frequency tables,
 *      Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_interop_lemon_Maps
 #define __ADS_interop_lemon_Maps
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <lemon/core.h> )
 #error "the interoperation with LEMON needs the headers of LEMON"
#endif

#include "ADS/Contract.h"
#include "ADS/Modification.h"

#include <lemon/core.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::lemon_interop
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DenseMap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a `ReferenceMap` of LEMON by the ids of the items of a graph, on an
/// array of its own, not attached to the notifier of the graph
template< class GR , class T , class Item = typename GR::Node >
class DenseMap
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = Item;                       ///< the items
 using Value = T;                        ///< the values
 using Reference = T &;                  ///< a reference to a value
 using ConstReference = const T &;       ///< a reference to a value read
 using ReferenceMapTag = ::lemon::True;  ///< a `ReferenceMap`

 /// the map of the items of \f$ g \f$, all \f$ t \f$; \f$ g \f$ must stay
 /// alive and keep its items
 explicit DenseMap( const GR & g , const T & t = T() )
     : f_g( &g ) , f_n( std::size_t( g.maxId( Item() ) + 1 ) ) ,
       f_v( new T[ f_n ] ) {
  std::fill_n( f_v.get() , f_n , t );
  }

 /// the value of \f$ k \f$
 [[nodiscard]] Reference operator[]( const Key & k ) {
  return( f_v[ at( k ) ] );
  }
 /// the value of \f$ k \f$, read
 [[nodiscard]] ConstReference operator[]( const Key & k ) const {
  return( f_v[ at( k ) ] );
  }

 /// the value of \f$ k \f$ set to \f$ t \f$
 void set( const Key & k , const T & t ) { f_v[ at( k ) ] = t; }

 /// the number of values, `maxId( Item() ) + 1` at the construction
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

/*--------------------------------------------------------------------------*/
 private:
 /// the place of \f$ k \f$
 [[nodiscard]] std::size_t at( const Key & k ) const {
  const auto i = std::size_t( f_g->id( k ) );
  ADS_PRE( i < f_n );
  return( i );
  }

 const GR * f_g;              ///< the graph
 std::size_t f_n;             ///< the number of values
 std::unique_ptr< T[] > f_v;  ///< the values

 };  // end( class DenseMap )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS IndexMap ------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

/// the value of the index \f$ i \f$ of \f$ s \f$, by `get()` or by `[]`
template< class S >
decltype( auto ) readAt( const S & s , std::size_t i )
{
 if constexpr( requires { s.get( i ); } )
  return( s.get( i ) );
 else
  return( s[ i ] );
 }

}  // end( namespace detail )

/// a `ReadWriteMap` of LEMON over a structure of ADS indexed by the ids of
/// the items of a graph, `ObservableArray` or `Fenwick` among them
template< class GR , class S , class Item = typename GR::Node >
class IndexMap
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = Item;  ///< the items
 /// the values, those that the structure reads
 using Value = std::remove_cvref_t< decltype( detail::readAt(
   std::declval< const S & >() , std::size_t() ) ) >;

 /// the map of the items of \f$ g \f$ on \f$ s \f$; both must stay alive
 /// \pre \f$ s \f$ has room for `maxId( Item() ) + 1` values
 IndexMap( const GR & g , S & s ) : f_g( &g ) , f_s( &s ) {
  ADS_PRE( std::size_t( g.maxId( Item() ) + 1 ) <= s.size() );
  }

 /// the value of \f$ k \f$
 [[nodiscard]] Value operator[]( const Key & k ) const {
  return( detail::readAt( *f_s , at( k ) ) );
  }

 /// the value of \f$ k \f$ set to \f$ v \f$, by `set()` of the structure
 void set( const Key & k , const Value & v ) { f_s->set( at( k ) , v ); }

 /// the aggregate of the values of the ids up to that of \f$ k \f$, by
 /// `prefix()` of the structure (e.g., `Fenwick`)
 [[nodiscard]] Value prefix( const Key & k ) const
  requires requires( const S & s ) { s.prefix( std::size_t() ); }
 {
  return( f_s->prefix( at( k ) + 1 ) );
  }

 /// the structure
 [[nodiscard]] S & structure( void ) const { return( *f_s ); }

/*--------------------------------------------------------------------------*/
 private:
 /// the index of \f$ k \f$
 [[nodiscard]] std::size_t at( const Key & k ) const {
  const auto i = std::size_t( f_g->id( k ) );
  ADS_PRE( i < f_s->size() );
  return( i );
  }

 const GR * f_g;  ///< the graph
 S * f_s;         ///< the structure

 };  // end( class IndexMap )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS ObservedMap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a map of LEMON whose writes are told as `Changed< A , int , V >`, the
/// id of the item its handle
template< class GR , class Map , class A = mod::Cost ,
	  class Policy = Dynamic<> >
class ObservedMap
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = typename Map::Key;                ///< the items
 using Value = typename Map::Value;            ///< the values
 using Event = mod::Changed< A , int , Value >;  ///< a write
 using modifications = mod::list< Event >;     ///< the events
 /// the subject
 using Subject = typename Policy::template bind< modifications , ObservedMap >;

 /// the map \f$ m \f$ of the items of \f$ g \f$, the static observers
 /// `o...`; both must stay alive
 template< class... O >
 ObservedMap( const GR & g , Map & m , O &... o )
     : f_g( g ) , f_m( m ) , f_sub( o... ) { }

 /// the value of \f$ k \f$
 [[nodiscard]] Value operator[]( const Key & k ) const { return( f_m[ k ] ); }

 /// the value of \f$ k \f$ set to \f$ v \f$, and `Changed`
 void set( const Key & k , const Value & v ) {
  if constexpr( Subject::template listened< Event > )
   if( f_sub.template live< Event >() ) [[unlikely]] {
    const Value old = f_m[ k ];
    f_m.set( k , v );
    f_sub.emit( Event{ f_g.id( k ) , old , v } );
    return;
    }
  f_m.set( k , v );
  }

 /// the map below
 [[nodiscard]] const Map & map( void ) const { return( f_m ); }

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class O >
 int attach( O & o )
  requires( Subject::dynamic )
 {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open
 void detach( int id )
  requires( Subject::dynamic )
 {
  f_sub.detach( id );
  }

/*--------------------------------------------------------------------------*/
 private:
 const GR & f_g;                       ///< the graph
 Map & f_m;                            ///< the map
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class ObservedMap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::lemon_interop )

/*--------------------------------------------------------------------------*/

#endif /* Maps.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Maps.h -----------------------------*/
/*--------------------------------------------------------------------------*/
