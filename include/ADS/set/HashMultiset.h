/*--------------------------------------------------------------------------*/
/*-------------------------- File HashMultiset.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multiset on a hash table (exercise 8.10): a counter of occurrences
 * per item, which the insertion of a new item sets to 1 and of an item
 * already there increases by 1, and the removal decreases, removing the
 * item at 0; each operation costs \f$ O( 1 ) \f$ expected time.
 *
 * The contracts [see `Contract.h`]: `insert()` adds one occurrence and
 * `remove()` takes one away, if any; the invariant is that the size is the
 * sum of the counters, none of which is 0. The operations are defined in
 * `HashMultiset.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 8.10,
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

 /// the empty multiset
 HashMultiset( void );

 /// the number of items, each counted as many times as it occurs
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of distinct items
 [[nodiscard]] std::size_t distinct( void ) const;

 /// whether the multiset has no item
 [[nodiscard]] bool empty( void ) const;

 /// whether \f$ x \f$ occurs at least once
 [[nodiscard]] bool contains( const T & x ) const;

 /// the occurrences of \f$ x \f$, 0 if none
 [[nodiscard]] std::size_t count( const T & x ) const;

 /// one occurrence of \f$ x \f$ more
 ///
 /// In pseudocode (exercise 8.10), \f$ D \f$ the table of the counters:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad c \gets D.\mathrm{lookup}( x ),\ 0 \text{ if nil} \cr
 /// 3 & \quad D.\mathrm{insert}( x , c + 1 )
 /// \end{array}}
 /// \f]
 /// \post one occurrence of \f$ x \f$ more
 void insert( const T & x );

 /// one occurrence of \f$ x \f$ less, if there is one
 ///
 /// In pseudocode (exercise 8.10):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad c \gets D.\mathrm{lookup}( x ),\ 0 \text{ if nil} \cr
 /// 3 & \quad \textbf{if}\ c = 1\ \textbf{then}\ D.\mathrm{remove}( x ) \cr
 /// 4 & \quad \textbf{else if}\ c > 1\ \textbf{then}\ D.\mathrm{insert}( x ,
 ///     c - 1 )
 /// \end{array}}
 /// \f]
 /// \post one occurrence of \f$ x \f$ less, or none
 void remove( const T & x );

 /// `f( x , c )` on the distinct items and their occurrences
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( item , count ) of the distinct items,
 // forward

 /// a pair of an item and its occurrences
 using value_type = std::pair< T , std::size_t >;
 /// the iterator of the table of the counters, constant
 using const_iterator =
  typename ChainedHashTable< T , std::size_t , Chain::FRONT , H >::
   const_iterator;
 using iterator = const_iterator;   ///< the same, constant

 /// the iterator on the first pair
 [[nodiscard]] const_iterator begin( void ) const {
  return( f_count.begin() );
  }
 /// the iterator past the last pair
 [[nodiscard]] const_iterator end( void ) const { return( f_count.end() ); }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the size is the sum of the counters, none of which is 0, \f$ O( n ) \f$
 [[nodiscard]] bool counted( void ) const;

 /// the counters of the items
 ChainedHashTable< T , std::size_t , Chain::FRONT , H > f_count;
 std::size_t f_size = 0;   ///< the sum of the counters

 };  // end( class HashMultiset )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/HashMultiset.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HashMultiset.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashMultiset.h -------------------------*/
/*--------------------------------------------------------------------------*/
