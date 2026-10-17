/*--------------------------------------------------------------------------*/
/*-------------------------- File HashMultiset.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multiset on a hash table (exercise 8.10): a counter of occurrences
 * per item, which the insertion of a new item sets to 1 and of an item
 * already there increases by 1, and the removal decreases, removing the
 * item at 0.
 *
 * The contracts [see Contract.h]: insert() adds one occurrence and
 * remove() takes one away, if any; the invariant is that the size is the
 * sum of the counters, none of which is 0. The operations are defined in
 * HashMultiset.tpp.
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

#include "ADS/Contract.h"
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

 HashMultiset( void );

 /// the number of items, each counted as many times as it occurs
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of distinct items
 [[nodiscard]] std::size_t distinct( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] bool contains( const T & x ) const;

 /// the occurrences of x
 [[nodiscard]] std::size_t count( const T & x ) const;

 /// \post one occurrence of x more
 void insert( const T & x );

 /// removes one occurrence of x, if there is one
 /// \post one occurrence of x less, or none
 void remove( const T & x );

 /// f( x , c ) on the distinct items and their occurrences
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the size is the sum of the counters, none of which is 0, O( n )
 [[nodiscard]] bool counted( void ) const;

 ChainedHashTable< T , std::size_t , Chain::FRONT , H > f_count;
 std::size_t f_size = 0;

 };  // end( class HashMultiset )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/HashMultiset.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HashMultiset.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashMultiset.h -------------------------*/
/*--------------------------------------------------------------------------*/
