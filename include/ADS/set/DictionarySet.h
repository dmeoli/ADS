/*--------------------------------------------------------------------------*/
/*-------------------------- File DictionarySet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET on any DICTIONARY with a foreach (sections 8.4 and 8.5):
 * `ChainedHashSet` and `OpenHashSet` on the hash tables, `contains()`,
 * `insert()` and `remove()` in \f$ O( 1 ) \f$ expected time and `forEach()`
 * in \f$ O( m + n ) \f$ and \f$ O( m ) \f$, \f$ m \f$ the size of the table;
 * `TreeSet` on a Red-Black tree, the three in \f$ O( \log n ) \f$ and
 * `forEach()` in \f$ O( n ) \f$, in order. Their operations of two sets are
 * the generic ones [see `SetOperations.h`].
 *
 * The contracts [see `Contract.h`]: after `insert()` the item is there and
 * after `remove()` it is not; the dictionary keeps its own invariant. The
 * operations are defined in `DictionarySet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 8.4 and 8.5,
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

#ifndef __ADS_DictionarySet
 #define __ADS_DictionarySet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/set/SetOperations.h"
#include "ADS/tree/SearchTree.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DictionarySet ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set on a DICTIONARY `D` with the values `bool` and a
/// `forEach( f( k , v ) )`, an item being a key
template< class D >
class DictionarySet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = typename D::Key;   ///< the type of the items

 /// the empty set on the dictionary `D( a... )`
 template< class... A >
  requires std::constructible_from< D , A... >
 explicit DictionarySet( A &&... a );

 /// the number of items, the keys of the dictionary
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the set has no item
 [[nodiscard]] bool empty( void ) const;

 /// whether \f$ x \f$ is a key of the dictionary
 [[nodiscard]] bool contains( const Item & x ) const;

 /// \f$ x \f$ into the set, as the key of the value true
 ///
 /// In pseudocode (section 8.4), \f$ D \f$ the dictionary:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad D.\mathrm{insert}( x , \textbf{true} )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is in the set
 void insert( const Item & x );

 /// \f$ x \f$ out of the set, the key removed from the dictionary
 ///
 /// In pseudocode (section 8.4):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad D.\mathrm{remove}( x )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not in the set
 void remove( const Item & x );

 /// `f( x )` on the items, in the order of the dictionary
 template< class F >
 void forEach( F && f ) const;

 /// the dictionary
 [[nodiscard]] const D & dictionary( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items, the keys of the dictionary in its order,
 // when the dictionary has iterators

 /// the iterator on the first key of the dictionary
 [[nodiscard]] auto begin( void ) const
  requires std::ranges::forward_range< const D > {
  return( KeyIterator( std::ranges::begin( f_d ) ) );
  }

 /// the iterator past the last key of the dictionary
 [[nodiscard]] auto end( void ) const
  requires std::ranges::forward_range< const D > {
  return( KeyIterator( std::ranges::end( f_d ) ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 D f_d;   ///< the dictionary of the items

 };  // end( class DictionarySet )

/// the set on a hash table with lists of overflow
template< class T , class H = std::hash< T > >
using ChainedHashSet = DictionarySet< ChainedHashTable< T , bool ,
                                                        Chain::FRONT , H > >;

/// the set on a hash table with open addressing
template< class T , class H = std::hash< T > >
using OpenHashSet = DictionarySet< OpenHashTable< T , bool , H > >;

/// the set on a Red-Black tree
template< std::totally_ordered T >
using TreeSet = DictionarySet< RedBlackTree< T , bool > >;

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/DictionarySet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DictionarySet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DictionarySet.h ------------------------*/
/*--------------------------------------------------------------------------*/
