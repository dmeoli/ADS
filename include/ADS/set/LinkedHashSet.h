/*--------------------------------------------------------------------------*/
/*-------------------------- File LinkedHashSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET on a hash table whose foreach follows the order of insertion
 * (exercise 8.9): a LIST keeps the items in that order and the table maps
 * each item to its position, so insert(), remove() and contains() take
 * O( 1 ) expected time and forEach() O( n ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 8.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
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

#ifndef __ADS_LinkedHashSet
 #define __ADS_LinkedHashSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/sequence/List.h"
#include "ADS/set/SetOperations.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS LinkedHashSet ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set on a hash table whose foreach follows the order of insertion: a
/// LIST keeps the items in that order and the table maps each item to its
/// position, so insert(), remove() and contains() take O( 1 ) expected
/// time and forEach() O( n ) (exercise 8.9)
template< class T , class H = std::hash< T > >
class LinkedHashSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 LinkedHashSet( void ) : f_index( 7 , true ) {}

 /// a copy, with positions into its own list
 LinkedHashSet( const LinkedHashSet & o ) : LinkedHashSet() {
  o.forEach( [ & ]( const T & x ) { insert( x ); } );
  }

 LinkedHashSet & operator=( const LinkedHashSet & ) = delete;

 [[nodiscard]] std::size_t size( void ) const { return( f_index.size() ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_index.size() == 0 ); }

 [[nodiscard]] bool contains( const T & x ) const {
  return( f_index.contains( x ) );
  }

 /// a new item goes to the end of the order; an item already there keeps
 /// its place
 void insert( const T & x ) {
  if( ! f_index.contains( x ) )
   f_index.insert( x , f_order.insert( f_order.next( f_order.tail() ) ,
                                       x ) );
  }

 void remove( const T & x ) {
  if( const auto p = f_index.lookup( x ) ) {
   f_order.remove( *p );
   f_index.remove( x );
   }
  }

 /// f( x ) on the items in the order of insertion, O( n ) time
 template< class F >
 void forEach( F && f ) const {
  for( auto p = f_order.head() ; ! f_order.finished( p ) ;
       p = f_order.next( p ) )
   f( f_order.read( p ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 using Pos = typename List< T >::Pos;

 List< T > f_order;                                        ///< the order
 ChainedHashTable< T , Pos , Chain::FRONT , H > f_index;   ///< the places

 };  // end( class LinkedHashSet )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* LinkedHashSet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkedHashSet.h ------------------------*/
/*--------------------------------------------------------------------------*/
