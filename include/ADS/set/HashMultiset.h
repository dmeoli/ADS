/*--------------------------------------------------------------------------*/
/*-------------------------- File HashMultiset.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multiset on a hash table (exercise 8.10): a counter of occurrences
 * per item, which the insertion of a new item sets to 1 and of an item
 * already there increases by 1, and the removal decreases, removing the
 * item at 0.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 8.10,
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

#ifndef __ADS_HashMultiset
 #define __ADS_HashMultiset
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/ChainedHashTable.h"

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
/*--------------------------- CLASS HashMultiset ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a multiset on a hash table, a counter of occurrences per item: the
/// insertion of a new item sets it to 1, of an item already there adds 1,
/// the removal subtracts 1 and removes the item at 0 (exercise 8.10)
template< class T , class H = std::hash< T > >
class HashMultiset {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 HashMultiset( void ) : f_count( 7 , true ) {}

 /// the number of items, each counted as many times as it occurs
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the number of distinct items
 [[nodiscard]] std::size_t distinct( void ) const {
  return( f_count.size() );
  }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 [[nodiscard]] bool contains( const T & x ) const {
  return( f_count.contains( x ) );
  }

 /// the occurrences of x
 [[nodiscard]] std::size_t count( const T & x ) const {
  return( f_count.lookup( x ).value_or( 0 ) );
  }

 void insert( const T & x ) {
  f_count.insert( x , count( x ) + 1 );
  ++f_size;
  }

 /// removes one occurrence of x, if there is one
 void remove( const T & x ) {
  const auto c = count( x );
  if( c == 0 )
   return;
  if( c == 1 )
   f_count.remove( x );
  else
   f_count.insert( x , c - 1 );
  --f_size;
  }

 /// f( x , c ) on the distinct items and their occurrences
 template< class F >
 void forEach( F && f ) const { f_count.forEach( f ); }

/*--------------------------------------------------------------------------*/
 private:

 ChainedHashTable< T , std::size_t , Chain::FRONT , H > f_count;
 std::size_t f_size = 0;

 };  // end( class HashMultiset )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* HashMultiset.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashMultiset.h -------------------------*/
/*--------------------------------------------------------------------------*/
