/*--------------------------------------------------------------------------*/
/*-------------------------- File SortedListSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET as a sorted LIST of section 8.3: insert() in O( n ), min() and
 * max() in O( 1 ), the operations by merging the two lists in O( n + m ),
 * optimal since merging needs \f$ \Omega( n + m ) \f$ (exercises 8.3 and
 * 8.4).
 *
 * The merges append to the end of the result, the place \f$ pos_n \f$ =
 * next( tail() ) of a LIST: the C.insert( C.tail() , x ) of the book, with
 * the insert() of section 4.1 that puts x before the item in the given
 * position, would put x before the last item.
 *
 * The contracts [see Contract.h]: insertLast() needs an item larger than
 * the others, after insert() the item is there and after remove() it is
 * not, and each operation of two sets gives what its name says; the
 * invariant is that the list increases strictly. The operations are
 * defined in SortedListSet.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 8.3,
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

#ifndef __ADS_SortedListSet
 #define __ADS_SortedListSet
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
/*-------------------------- CLASS SortedListSet ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set as a list sorted in increasing order
template< std::totally_ordered T >
class SortedListSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 SortedListSet( void ) = default;

 SortedListSet( std::initializer_list< T > items );

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] bool contains( const T & x ) const;

 /// O( n ) time, to keep the order
 /// \post x is in the set
 void insert( const T & x );

 /// appends x, larger than all the items, O( 1 ) time
 /// \pre the set is empty or its largest item is smaller than x
 void insertLast( const T & x );

 /// \post x is not in the set
 void remove( const T & x );

 /// the smallest item, O( 1 ) time
 [[nodiscard]] std::optional< T > min( void ) const;

 /// the largest item, O( 1 ) time
 [[nodiscard]] std::optional< T > max( void ) const;

 /// f( x ) on the items in increasing order, O( n ) time
 template< class F >
 void forEach( F && f ) const;

 /// the sorted list of the items
 [[nodiscard]] const List< T > & list( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in increasing order, bidirectional

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

 /// the position of the first item not smaller than x
 typename List< T >::Pos lower( const T & x ) const;

 /// the list increases strictly, and the size is its length, O( n )
 [[nodiscard]] bool sorted( void ) const;

 List< T > f_L;              ///< the items, in increasing order
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class SortedListSet )

/// the merge of the sorted lists of A and B: an item of A only, of B only
/// or of both goes into the result if a, b or both say so; O( n + m ) time
template< class T >
SortedListSet< T > merge( const SortedListSet< T > & A ,
                          const SortedListSet< T > & B ,
                          bool a , bool b , bool both );

/// the union, O( n + m ) time (exercise 8.3)
template< class T >
SortedListSet< T > setUnion( const SortedListSet< T > & A ,
                             const SortedListSet< T > & B );

/// the intersection, O( n + m ) time (the procedure of section 8.3)
template< class T >
SortedListSet< T > setIntersection( const SortedListSet< T > & A ,
                                    const SortedListSet< T > & B );

/// the difference, O( n + m ) time (exercise 8.4)
template< class T >
SortedListSet< T > setDifference( const SortedListSet< T > & A ,
                                  const SortedListSet< T > & B );

/// the symmetric difference, O( n + m ) time (exercise 8.5)
template< class T >
SortedListSet< T > setSymmetricDifference( const SortedListSet< T > & A ,
                                           const SortedListSet< T > & B );

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/SortedListSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SortedListSet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SortedListSet.h ------------------------*/
/*--------------------------------------------------------------------------*/
