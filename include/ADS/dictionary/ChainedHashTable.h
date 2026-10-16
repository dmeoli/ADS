/*--------------------------------------------------------------------------*/
/*------------------------ File ChainedHashTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash table with lists of overflow of section 7.2.1, a realization of
 * the DICTIONARY [see ADT.h]: the place i of a vector of size m holds a
 * LIST with the pairs whose keys have H( k ) = i, insert() puts a new key
 * at the head of its list, lookup() and remove() go along it. A list may
 * also bring to its head the key just found, so that the keys looked up
 * often stay close to it (exercise 7.5), or be kept sorted, so that an
 * unsuccessful search stops at the first larger key (exercise 7.10) [see
 * Chain].
 *
 * With the load factor \f$ \alpha = n / m \f$, an unsuccessful search
 * accesses \f$ 1 + \alpha \f$ cells on average and a successful one
 * \f$ 1 + \alpha / 2 \f$; probes() gives the cells that the last
 * operation examined, the nil that ends an unsuccessful search included.
 * The table may restructure itself (section 7.4), doubling m when
 * \f$ \alpha > 2 \f$ and halving it, down to the initial size, when
 * \f$ \alpha < 1 / 2 \f$: the two thresholds are 4 times apart, so that
 * between two restructurings there are at least m / 4 operations and the
 * cost stays O( 1 ) amortized [see TableSize.h].
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.2-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see W. W. Peterson, Addressing for random-access storage, IBM Journal
 *      of Research and Development 1(2), 130-146, 1957,
 *      doi:10.1147/rd.12.0130
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ChainedHashTable
 #define __ADS_ChainedHashTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.h"
#include "ADS/dictionary/TableSize.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ChainedHashTable -------------------------*/
/*--------------------------------------------------------------------------*/

/// how a ChainedHashTable keeps its lists
enum class Chain {
 FRONT ,          ///< a new key at the head, nothing moves
 MOVE_TO_FRONT ,  ///< also the key found by lookup() goes to the head
 SORTED           ///< the keys of a list in increasing order
 };

/// a hash table with lists of overflow
template< class K , class V , Chain C = Chain::FRONT ,
          class H = std::hash< K > >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
class ChainedHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty table of m > 0 lists, which restructures itself or not
 explicit ChainedHashTable( std::size_t m , bool restructure = false ,
                            H h = H() )
  : f_T( m ) , f_m0( m ) , f_restructure( restructure ) , f_h( h ) {
  if( m == 0 )
   throw( std::invalid_argument( "ADS::ChainedHashTable: m = 0" ) );
  }

/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const {
  auto & L = f_T[ place( k ) ];
  auto p = find( L , k );
  if( L.finished( p ) || ! ( L.read( p ).first == k ) )
   return( std::nullopt );
  const V v = L.read( p ).second;
  if constexpr( C == Chain::MOVE_TO_FRONT )
   if( p != L.head() ) {
    auto kv = L.read( p );
    L.remove( p );
    L.insert( L.head() , kv );
    }
  return( v );
  }

 /// associates the value v to the key k
 void insert( const K & k , const V & v ) {
  auto & L = f_T[ place( k ) ];
  auto p = find( L , k );
  if( ! L.finished( p ) && ( L.read( p ).first == k ) ) {
   L.write( p , { k , v } );
   return;
   }
  if constexpr( C == Chain::SORTED )
   L.insert( p , { k , v } );           // before the first larger key
  else
   L.insert( L.head() , { k , v } );
  ++f_size;
  if( f_restructure && ( f_size > 2 * f_T.size() ) )
   rebuild( resized( f_T.size() , true , f_m0 ) );
  }

 /// removes the key k, if it is there
 void remove( const K & k ) {
  auto & L = f_T[ place( k ) ];
  auto p = find( L , k );
  if( L.finished( p ) || ! ( L.read( p ).first == k ) )
   return;
  L.remove( p );
  --f_size;
  if( f_restructure && ( f_T.size() > f_m0 ) &&
      ( 2 * f_size < f_T.size() ) )
   rebuild( resized( f_T.size() , false , f_m0 ) );
  }

 [[nodiscard]] bool contains( const K & k ) const {
  auto & L = f_T[ place( k ) ];
  auto p = find( L , k );
  return( ! L.finished( p ) && ( L.read( p ).first == k ) );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// the number m of lists
 [[nodiscard]] std::size_t capacity( void ) const { return( f_T.size() ); }

 /// the load factor n / m
 [[nodiscard]] double load( void ) const {
  return( double( f_size ) / f_T.size() );
  }

 /// the foreach of the book: f( k , v ) on the pairs, list after list,
 /// O( m + n ) time
 template< class F >
 void forEach( F && f ) const {
  for( const auto & L : f_T )
   for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
    f( L.read( p ).first , L.read( p ).second );
  }

 /// the keys of the list i, from its head
 [[nodiscard]] std::vector< K > chain( std::size_t i ) const {
  std::vector< K > out;
  const auto & L = f_T.at( i );
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   out.push_back( L.read( p ).first );
  return( out );
  }

 /// the cells of the list examined by the last operation, plus one for
 /// the nil that ends an unsuccessful search
 [[nodiscard]] std::size_t probes( void ) const { return( f_probes ); }

 /// the number of restructurings so far
 [[nodiscard]] std::size_t restructures( void ) const { return( f_rebuilds ); }

 /// the number of pairs moved by the restructurings so far
 [[nodiscard]] std::size_t moved( void ) const { return( f_moved ); }

/*--------------------------------------------------------------------------*/
 private:

 using Pair = std::pair< K , V >;
 using Pos = typename List< Pair >::Pos;

 std::size_t place( const K & k ) const { return( f_h( k ) % f_T.size() ); }

 /// the cell of k, or where the search stopped: the end of the list, or for
 /// a sorted list the first larger key
 Pos find( const List< Pair > & L , const K & k ) const {
  f_probes = 1;
  auto p = L.head();
  for( ; ! L.finished( p ) ; p = L.next( p ) , ++f_probes ) {
   if( L.read( p ).first == k )
    break;
   if constexpr( C == Chain::SORTED )
    if( k < L.read( p ).first )
     break;
   }
  return( p );
  }

 void rebuild( std::size_t m ) {
  std::vector< List< Pair > > T( m );
  std::swap( f_T , T );
  for( auto & L : T )
   for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) ) {
    const auto & kv = L.read( p );
    auto & M = f_T[ place( kv.first ) ];
    if constexpr( C == Chain::SORTED )
     M.insert( find( M , kv.first ) , kv );
    else
     M.insert( M.head() , kv );
    }
  ++f_rebuilds;
  f_moved += f_size;
  }

 mutable std::vector< List< Pair > > f_T;  ///< the lists
 std::size_t f_m0;                         ///< the initial size
 bool f_restructure;                       ///< whether it restructures
 H f_h;                                    ///< the hash of the keys
 std::size_t f_size = 0;                   ///< the number of keys
 mutable std::size_t f_probes = 0;         ///< see probes()
 std::size_t f_rebuilds = 0;               ///< see restructures()
 std::size_t f_moved = 0;                  ///< see moved()

 };  // end( class ChainedHashTable )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ChainedHashTable.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ChainedHashTable.h -----------------------*/
/*--------------------------------------------------------------------------*/
