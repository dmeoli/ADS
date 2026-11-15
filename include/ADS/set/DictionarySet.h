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
 * It is also the one adaptor that makes every dictionary of the library a
 * set of Morin [see `SetInterfaces.h`], beyond the book: on a
 * `KeyedDictionary`, whose `findKey( k )` gives the address of the key
 * \f$ k \f$ it holds, it is a `USet`, `find( x )` being that address, and
 * on an `OrderedDictionary`, whose `lowerBoundKey( k )` gives the address
 * of the smallest key not smaller than \f$ k \f$, it is an `SSet`, with
 * `lowerBound( x )`; the hash tables are of the first kind, and the search
 * trees, the skip lists, the B-trees and the sorted array of the second.
 * `insert()` and `remove()` say whether they changed the set, by the
 * answer of the dictionary when its own `insert()` and `remove()` give
 * one, and otherwise by one search more. The dictionary is `mutable`,
 * since the searches of a splay tree move its nodes while the set they
 * hold stays the same, and `find()` of a `USet` is constant.
 *
 * The contracts [see `Contract.h`]: after `insert()` the item is there and
 * after `remove()` it is not; the dictionary keeps its own invariant. The
 * operations are defined in `DictionarySet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 8.4 and 8.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, sections 1.2.3 and 1.2.4
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
#include "ADS/set/SetInterfaces.h"
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

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DictionarySet ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set on a DICTIONARY `D`, an item being a key: a `USet` of Morin if
/// `D` is a `KeyedDictionary`, an `SSet` if it is an `OrderedDictionary`
template< class D >
class DictionarySet
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = typename D::Key;  ///< the type of the items

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

 /// \f$ x \f$ into the set, as the key of the value true (or of the value
 /// made by default, for values that are not `bool`); true if it was not
 /// there
 ///
 /// In pseudocode (section 8.4), \f$ D \f$ the dictionary:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x )\quad \triangleright
 ///     \ \text{true if } x \text{ is new} \cr
 /// 2 & \quad \textbf{if}\ \text{the insertion of } D
 ///     \text{ tells whether the key was new}\ \textbf{then return}
 ///     \ D.\mathrm{insert}( x , \textbf{true} ) \cr
 /// 3 & \quad \mathit{fresh} \gets \textbf{not}\ \mathrm{contains}( x );
 ///     \ \textbf{if}\ \mathit{fresh}\ \textbf{then}\ D.\mathrm{insert}( x ,
 ///     \textbf{true} ) \cr
 /// 4 & \quad \textbf{return}\ \mathit{fresh}
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is in the set
 bool insert( const Item & x );

 /// \f$ x \f$ out of the set, the key removed from the dictionary
 ///
 /// In pseudocode (section 8.4):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x )\quad \triangleright
 ///     \ \text{true if } x \text{ was there} \cr
 /// 2 & \quad \textbf{if}\ \text{the removal of } D
 ///     \text{ tells whether the key was there}\ \textbf{then return}
 ///     \ D.\mathrm{remove}( x ) \cr
 /// 3 & \quad \mathit{found} \gets \mathrm{contains}( x );\ \textbf{if}
 ///     \ \mathit{found}\ \textbf{then}\ D.\mathrm{remove}( x ) \cr
 /// 4 & \quad \textbf{return}\ \mathit{found}
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not in the set
 bool remove( const Item & x );

 /// the address of the item equal to \f$ x \f$, nil if none, by
 /// `findKey()` of the dictionary: the `find( x )` of a USet of Morin
 [[nodiscard]] const Item * find( const Item & x ) const
  requires KeyedDictionary< D >;

 /// the address of the smallest item not smaller than \f$ x \f$, nil if
 /// none, by `lowerBoundKey()` of the dictionary: the `find( x )` of an
 /// SSet of Morin
 [[nodiscard]] const Item * lowerBound( const Item & x ) const
  requires OrderedDictionary< D >;

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
  requires std::ranges::forward_range< const D >
 {
  return( KeyIterator( std::ranges::begin( std::as_const( f_d ) ) ) );
  }

 /// the iterator past the last key of the dictionary
 [[nodiscard]] auto end( void ) const
  requires std::ranges::forward_range< const D >
 {
  return( KeyIterator( std::ranges::end( std::as_const( f_d ) ) ) );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the value of a new key: true, or made by default
 [[nodiscard]] static typename D::Value mark( void );

 mutable D f_d;  ///< the dictionary of the items, which a search may move

 };  // end( class DictionarySet )

/// the set on a hash table with lists of overflow
template< class T , class H = std::hash< T > >
using ChainedHashSet =
  DictionarySet< ChainedHashTable< T , bool , Chain::FRONT , H > >;

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

#endif /* DictionarySet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DictionarySet.h ------------------------*/
/*--------------------------------------------------------------------------*/
