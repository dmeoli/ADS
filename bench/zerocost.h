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
 * JoinTree (`insert()` of a new key and its `remove()`, `insert()` of a key
 * that is there, `assign()`). The variants on the subject policies exist
 * only when Modification.h does; with ZC_BASE (the headers of an older
 * revision, see the option --base of zerocost.py), only the tree with no
 * observer, of the default policy, and only its operations of that
 * revision.
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
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/tree/JoinTree.h"

#if __has_include( "ADS/Modification.h" )
 #include "ADS/Modification.h"
 #define ZC_POLICIES 1
#endif

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace zc {

using AG = ADS::ArrayGraph< long long >;
using LG = ADS::ListGraph< long long >;
using Ev = ADS::GraphEvent< long long >;

/// the tree of the workloads of JoinTree, of the default policy
using JT = ADS::JoinTree< int , long long , ADS::AVLBalance ,
                          ADS::SumAugmentation< long long > >;

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

#ifndef ZC_BASE

/*--------------------------------------------------------------------------*/
/*------------------------------ THE JoinTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// an observer of the tree that wants none of the events of the workloads
struct TreeExcluding {
 long long n = 0;
 void on( const ADS::mod::Cleared & ) noexcept { ++n; }
 };

/// an observer of the tree that counts the values that decrease
struct TreeCountDecreases {
 long long n = 0;
 void on( const JT::Assigned & e ) { n += ( e.to < e.from ) ? 1 : 0; }
 };

/// the tree of the policy P
template< class P >
using JTP = ADS::JoinTree< int , long long , ADS::AVLBalance ,
                           ADS::SumAugmentation< long long > , P >;

/// with no observer, the root and the generator of the priorities alone
static_assert( sizeof( JT ) == sizeof( void * ) + sizeof( std::uint64_t ) );

#endif

#endif

}  // end( namespace zc )

#endif  /* zerocost.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File zerocost.h --------------------------*/
/*--------------------------------------------------------------------------*/
