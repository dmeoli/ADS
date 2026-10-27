/*--------------------------------------------------------------------------*/
/*-------------------------- File LinkedHashSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET on a hash table whose foreach follows the order of insertion
 * (exercise 8.9): a LIST keeps the items in that order and the table maps
 * each item to its position, so `insert()`, `remove()` and `contains()` take
 * \f$ O( 1 ) \f$ expected time and `forEach()` \f$ O( n ) \f$.
 *
 * The contracts [see `Contract.h`]: after `insert()` the item is there and
 * after `remove()` it is not; the invariant is that each item of the list
 * is indexed at its own position and the table has no other item. The
 * operations are defined in `LinkedHashSet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 8.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
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
/// position, so `insert()`, `remove()` and `contains()` take \f$ O( 1 ) \f$
/// expected time and `forEach()` \f$ O( n ) \f$ (exercise 8.9)
template< class T , class H = std::hash< T > >
class LinkedHashSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty set
 LinkedHashSet( void );

 /// a copy, with positions into its own list
 LinkedHashSet( const LinkedHashSet & o );

 /// no assignment, which would copy the positions into the list of another
 /// set
 LinkedHashSet & operator=( const LinkedHashSet & ) = delete;

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the set has no item
 [[nodiscard]] bool empty( void ) const;

 /// whether \f$ x \f$ is in the set, by a search in the table
 [[nodiscard]] bool contains( const T & x ) const;

 /// \f$ x \f$ into the set: a new item goes to the end of the order, and
 /// an item already there keeps its place
 ///
 /// In pseudocode (exercise 8.9), \f$ L \f$ the list of the order and
 /// \f$ D \f$ the table of the positions:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad \textbf{if}\ D.\mathrm{lookup}( x ) = \textbf{nil}
 ///     \ \textbf{then} \cr
 /// 3 & \qquad p \gets L.\mathrm{insert}( L.\mathrm{next}( L.\mathrm{tail}() )
 ///     , x );\ D.\mathrm{insert}( x , p )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is in the set
 void insert( const T & x );

 /// \f$ x \f$ out of the set, and out of the order
 ///
 /// In pseudocode (exercise 8.9):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad p \gets D.\mathrm{lookup}( x ) \cr
 /// 3 & \quad \textbf{if}\ p \ne \textbf{nil}\ \textbf{then}
 ///     \ L.\mathrm{remove}( p );\ D.\mathrm{remove}( x )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not in the set
 void remove( const T & x );

 /// `f( x )` on the items in the order of insertion, \f$ O( n ) \f$ time
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of their insertion,
 // bidirectional

 using value_type = T;                    ///< the items
 using size_type = std::size_t;           ///< the sizes
 using difference_type = std::ptrdiff_t;  ///< the distances
 using reference = const T &;             ///< a constant reference
 using const_reference = const T &;       ///< a constant reference
 /// the iterator of the list of the order, constant
 using const_iterator = typename List< T >::const_iterator;
 using iterator = const_iterator;         ///< the same, constant

 /// the iterator on the first item inserted
 [[nodiscard]] const_iterator begin( void ) const {
  return( f_order.begin() );
  }
 /// the iterator past the last item inserted
 [[nodiscard]] const_iterator end( void ) const { return( f_order.end() ); }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 using Pos = typename List< T >::Pos;

 /// each item of the order is indexed at its own position, and the table
 /// has no other item, \f$ O( n ) \f$
 [[nodiscard]] bool indexed( void ) const;

 List< T > f_order;                                        ///< the order
 ChainedHashTable< T , Pos , Chain::FRONT , H > f_index;   ///< the places

 };  // end( class LinkedHashSet )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/LinkedHashSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinkedHashSet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkedHashSet.h ------------------------*/
/*--------------------------------------------------------------------------*/
