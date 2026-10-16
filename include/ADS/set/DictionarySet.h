/*--------------------------------------------------------------------------*/
/*-------------------------- File DictionarySet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET on any DICTIONARY with a foreach (sections 8.4 and 8.5):
 * ChainedHashSet and OpenHashSet on the hash tables, contains(), insert()
 * and remove() in O( 1 ) expected time and forEach() in O( m + n ) and
 * O( m ), m the size of the table; TreeSet on a Red-Black tree, the three
 * in O( log n ) and forEach() in O( n ), in order. Their operations are
 * the generic ones [see SetOperations.h].
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 8.4 and 8.5,
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

#ifndef __ADS_DictionarySet
 #define __ADS_DictionarySet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/tree/SearchTree.h"
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
/*-------------------------- CLASS DictionarySet ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set on a DICTIONARY D with values bool and a forEach( f( k , v ) )
template< class D >
class DictionarySet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = typename D::Key;   ///< the type of the items

 /// the empty set on the dictionary D( a... )
 template< class... A >
  requires std::constructible_from< D , A... >
 explicit DictionarySet( A &&... a ) : f_d( std::forward< A >( a )... ) {}

 [[nodiscard]] std::size_t size( void ) const { return( f_d.size() ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_d.size() == 0 ); }

 [[nodiscard]] bool contains( const Item & x ) const {
  return( f_d.contains( x ) );
  }

 void insert( const Item & x ) { f_d.insert( x , true ); }

 void remove( const Item & x ) { f_d.remove( x ); }

 /// f( x ) on the items, in the order of the dictionary
 template< class F >
 void forEach( F && f ) const {
  f_d.forEach( [ & ]( const Item & x , bool ) { f( x ); } );
  }

 /// the dictionary
 [[nodiscard]] const D & dictionary( void ) const { return( f_d ); }

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

#endif  /* DictionarySet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DictionarySet.h ------------------------*/
/*--------------------------------------------------------------------------*/
