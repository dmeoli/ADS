/*--------------------------------------------------------------------------*/
/*----------------------------- File zerocost.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The variants of the check of zero cost [see zerocost.py], shared by its
 * two translation units, zerocost.cpp (counted by callgrind) and
 * zerocost_asm.cpp (disassembled): the same change of a structure written
 * on the plain structure and on the structure that tells its changes, with
 * nobody listening, with a static observer that does not want the kind of
 * the change, with a static observer that counts the decreases, and with a
 * dynamic subject to which nobody is attached. The structures are
 * ArrayGraph (`setWeight()`), ListGraph (`insertEdge()` of an edge that is
 * there, i.e., a change of weight, and `insertEdge()` followed by
 * `deleteEdge()`), an array of keys written on the subject policies of
 * Modification.h with the pattern of the mutators of the library, and
 * GridGraph (`setCost()`), whose plain variant is the grid with
 * `NoObservers`, since the grid had no change before its subject. The
 * variants on the subject policies exist only when Modification.h does.
 * The allocation paths of Arena and ObjectPool, with and without their
 * epochs, are counted by zerocost.cpp alone.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

#ifndef __ZEROCOST_H
 #define __ZEROCOST_H

/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/GridGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/ObjectPool.h"

#if __has_include( "ADS/Modification.h" )
 #include "ADS/Modification.h"
 #define ZC_POLICIES 1
#endif

#include <cstddef>
#include <vector>

namespace zc {

using AG = ADS::ArrayGraph< long long >;
using LG = ADS::ListGraph< long long >;
using Ev = ADS::GraphEvent< long long >;

/*--------------------------------------------------------------------------*/
/*--------------------- THE OBSERVERS OF Observable.h ----------------------*/
/*--------------------------------------------------------------------------*/

/// an observer of the insertions of nodes only, the kind that no change of
/// the check makes
struct LegacyExcluding {
 long long n = 0;
 [[nodiscard]] unsigned mask( void ) const {
  return( ADS::GraphEvents::NODE_INSERT );
  }
 void notify( const Ev & ) { ++n; }
 void notify( std::span< const Ev > s ) { n += (long long)( s.size() ); }
 };

#ifdef ZC_POLICIES

/*--------------------------------------------------------------------------*/
/*---------------------- THE OBSERVERS OF THE POLICIES ---------------------*/
/*--------------------------------------------------------------------------*/

/// an observer of the insertions of nodes only
struct Excluding {
 long long n = 0;
 void on( const ADS::NodeInserted & ) { ++n; }
 };

/// an observer that counts the weights that decrease
struct CountDecreases {
 long long n = 0;
 void on( const ADS::WeightChanged< long long > & e ) {
  n += ( e.to < e.from ) ? 1 : 0;
  }
 };

/// the change of a key of the array
using KeySet = ADS::mod::KeyChanged< std::size_t , int >;

/// an observer of the array that wants no change of key
struct KeyExcluding {
 long long n = 0;
 void on( const ADS::mod::Cleared & ) noexcept { ++n; }
 };

/// an observer of the array that counts the keys that decrease
struct KeyCountDecreases {
 long long n = 0;
 void on( const KeySet & e ) { n += ( e.to < e.from ) ? 1 : 0; }
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- THE ARRAY OF KEYS ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an array of keys that tells its changes through the subject policy
/// `Policy`, written as a structure of the library writes its mutators
template< class Policy >
class KeyArray {
 public:

 using modifications = ADS::mod::list< KeySet , ADS::mod::Cleared >;
 using Subject = typename Policy::template bind< modifications >;

 template< class ... O >
 explicit KeyArray( std::size_t n , O & ... o ) : f_k( n ) , f_sub( o... ) {}

 /// the key of h set to k
 void set( std::size_t h , int k ) {
  if constexpr( Subject::template listened< KeySet > ) {
   if( f_sub.template live< KeySet >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     setCold( h , k );
    else
     setEmit( h , k );
    return;
    }
   }
  f_k[ h ] = k;
  }

 /// the key of h
 [[nodiscard]] int get( std::size_t h ) const { return( f_k[ h ] ); }

 /// the subject
 Subject & subject( void ) { return( f_sub ); }

 private:

 void setEmit( std::size_t h , int k ) {
  const int old = f_k[ h ];
  f_k[ h ] = k;
  f_sub.emit( KeySet{ h , old , k } );
  }

 ADS_COLD_PATH void setCold( std::size_t h , int k ) { setEmit( h , k ); }

 std::vector< int > f_k;
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;
 };

static_assert( sizeof( KeyArray< ADS::NoObservers > ) ==
               sizeof( std::vector< int > ) );

/*--------------------------------------------------------------------------*/
/*------------------------------- THE GRID ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the change of the cost of a cell of the grid
using CellSet = ADS::mod::Changed< ADS::mod::Value , int , int >;

/// the grid with the subject policy `Policy`
template< class Policy >
using Grid = ADS::GridGraph< int , int , Policy >;

/// an observer of the grid that counts the costs that decrease
struct GridCountDecreases {
 long long n = 0;
 void on( const CellSet & e ) { n += ( e.to < e.from ) ? 1 : 0; }
 };

#endif

}  // end( namespace zc )

#endif  /* zerocost.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File zerocost.h --------------------------*/
/*--------------------------------------------------------------------------*/
