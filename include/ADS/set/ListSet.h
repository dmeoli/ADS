/*--------------------------------------------------------------------------*/
/*----------------------------- File ListSet.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET as an unsorted LIST of section 8.2: contains(), insert() and
 * remove() in O( n ), insertNew() in O( 1 ) when the item is known to be
 * absent, the operations in O( n m ), which is optimal when the items can
 * only be compared for equality (exercises 8.1 and 8.2).
 *
 * The contracts [see Contract.h]: insertNew() needs an item that is not
 * there, after insert() the item is there and after remove() it is not,
 * and each operation of two sets gives what its name says; the invariant
 * is that no item is there twice. The operations are defined in
 * ListSet.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 8.2,
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

#ifndef __ADS_ListSet
 #define __ADS_ListSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
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
/*----------------------------- CLASS ListSet ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a set as an unsorted list
template< std::equality_comparable T >
class ListSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 ListSet( void ) = default;

 ListSet( std::initializer_list< T > items );

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] bool contains( const T & x ) const;

 /// O( n ) time, for the check that x is not there
 /// \post x is in the set
 void insert( const T & x );

 /// inserts x, which must not be there, at the head, O( 1 ) time
 /// \pre x is not in the set (an audit, O( n ))
 void insertNew( const T & x );

 /// \post x is not in the set
 void remove( const T & x );

 /// the smallest item, O( n ) time
 [[nodiscard]] std::optional< T > min( void ) const
  requires std::totally_ordered< T >;

 /// f( x ) on the items, from the last inserted, O( n ) time
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of the list, bidirectional

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using const_iterator = typename List< T >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 typename List< T >::Pos find( const T & x ) const;

 /// no item twice, and the size is the length of the list, O( n^2 )
 [[nodiscard]] bool distinct( void ) const;

 List< T > f_L;              ///< the items
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class ListSet )

/// the union: C starts as B and gets the items of A not in B, inserted
/// without looking for them in C, O( n m ) time (exercise 8.2)
template< class T >
ListSet< T > setUnion( const ListSet< T > & A , const ListSet< T > & B );

/// the intersection, O( n m ) time (exercise 8.1)
template< class T >
ListSet< T > setIntersection( const ListSet< T > & A ,
                              const ListSet< T > & B );

/// the difference, O( n m ) time (the procedure of section 8.2)
template< class T >
ListSet< T > setDifference( const ListSet< T > & A , const ListSet< T > & B );

/// the symmetric difference, O( n m ) time (exercise 8.5)
template< class T >
ListSet< T > setSymmetricDifference( const ListSet< T > & A ,
                                     const ListSet< T > & B );

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/ListSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListSet.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ListSet.h ---------------------------*/
/*--------------------------------------------------------------------------*/
