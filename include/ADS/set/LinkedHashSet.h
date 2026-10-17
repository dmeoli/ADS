/*--------------------------------------------------------------------------*/
/*-------------------------- File LinkedHashSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET on a hash table whose foreach follows the order of insertion
 * (exercise 8.9): a LIST keeps the items in that order and the table maps
 * each item to its position, so insert(), remove() and contains() take
 * O( 1 ) expected time and forEach() O( n ).
 *
 * The contracts [see Contract.h]: after insert() the item is there and
 * after remove() it is not; the invariant is that each item of the list
 * is indexed at its own position and the table has no other item. The
 * operations are defined in LinkedHashSet.tpp.
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

#include "ADS/Contract.h"
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

 LinkedHashSet( void );

 /// a copy, with positions into its own list
 LinkedHashSet( const LinkedHashSet & o );

 LinkedHashSet & operator=( const LinkedHashSet & ) = delete;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] bool contains( const T & x ) const;

 /// a new item goes to the end of the order; an item already there keeps
 /// its place
 /// \post x is in the set
 void insert( const T & x );

 /// \post x is not in the set
 void remove( const T & x );

 /// f( x ) on the items in the order of insertion, O( n ) time
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of their insertion,
 // bidirectional

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using const_iterator = typename List< T >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( f_order.begin() );
  }
 [[nodiscard]] const_iterator end( void ) const { return( f_order.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 using Pos = typename List< T >::Pos;

 /// each item of the order is indexed at its own position, and the table
 /// has no other item, O( n )
 [[nodiscard]] bool indexed( void ) const;

 List< T > f_order;                                        ///< the order
 ChainedHashTable< T , Pos , Chain::FRONT , H > f_index;   ///< the places

 };  // end( class LinkedHashSet )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/LinkedHashSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinkedHashSet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkedHashSet.h ------------------------*/
/*--------------------------------------------------------------------------*/
