/*--------------------------------------------------------------------------*/
/*----------------------------- File ListSet.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET as an unsorted LIST of section 8.2: `contains()`, `insert()` and
 * `remove()` in \f$ O( n ) \f$, `insertNew()` in \f$ O( 1 ) \f$ when the
 * item is known to be absent, the operations of two sets in
 * \f$ O( n m ) \f$, which is optimal when the items can only be compared
 * for equality (exercises 8.1 and 8.2).
 *
 * The contracts [see `Contract.h`]: `insertNew()` needs an item that is not
 * there, after `insert()` the item is there and after `remove()` it is
 * not, and each operation of two sets gives what its name says; the
 * invariant is that no item is there twice. The operations are defined in
 * `ListSet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 8.2,
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

 /// the empty set
 ListSet( void ) = default;

 /// the set of the given items, a repeated one taken once
 ListSet( std::initializer_list< T > items );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the set has no item
 [[nodiscard]] bool empty( void ) const;

 /// whether \f$ x \f$ is in the set, by a scan of the list,
 /// \f$ O( n ) \f$ time
 [[nodiscard]] bool contains( const T & x ) const;

 /// \f$ x \f$ into the set, \f$ O( n ) \f$ time, for the check that
 /// \f$ x \f$ is not there
 ///
 /// In pseudocode (section 8.2):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad \textbf{if not}\ \mathrm{contains}( x )\ \textbf{then}
 ///     \ \mathrm{insertNew}( x )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is in the set
 void insert( const T & x );

 /// \f$ x \f$, which must not be there, at the head of the list,
 /// \f$ O( 1 ) \f$ time
 /// \pre \f$ x \f$ is not in the set (an audit, \f$ O( n ) \f$)
 void insertNew( const T & x );

 /// \f$ x \f$ out of the set, \f$ O( n ) \f$ time
 ///
 /// In pseudocode (section 8.2), \f$ L \f$ the list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad p \gets \text{the position of } x \text{ in } L,
 ///     \ \text{found by a scan} \cr
 /// 3 & \quad \textbf{if}\ p \ne \textbf{nil}\ \textbf{then}
 ///     \ L.\mathrm{remove}( p )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not in the set
 void remove( const T & x );

 /// the smallest item, none if the set is empty, by a scan, \f$ O( n ) \f$
 /// time
 [[nodiscard]] std::optional< T > min( void ) const
  requires std::totally_ordered< T >;

 /// `f( x )` on the items, from the last inserted, \f$ O( n ) \f$ time
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of the list, bidirectional

 using value_type = T;                    ///< the items
 using size_type = std::size_t;           ///< the sizes
 using difference_type = std::ptrdiff_t;  ///< the distances
 using reference = const T &;             ///< a constant reference
 using const_reference = const T &;       ///< a constant reference
 /// the iterator of the list, constant
 using const_iterator = typename List< T >::const_iterator;
 using iterator = const_iterator;         ///< the same, constant

 /// the iterator on the item at the head of the list
 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 typename List< T >::Pos find( const T & x ) const;

 /// no item twice, and the size is the length of the list, \f$ O( n^2 ) \f$
 [[nodiscard]] bool distinct( void ) const;

 List< T > f_L;              ///< the items
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class ListSet )

/// the union: \f$ C \f$ starts as \f$ B \f$ and gets the items of
/// \f$ A \f$ not in \f$ B \f$, inserted without looking for them in
/// \f$ C \f$, \f$ O( n m ) \f$ time (exercise 8.2)
///
/// In pseudocode (exercise 8.2), each search in \f$ B \f$ a scan of
/// \f$ O( m ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setUnion}( A , B ) \cr
/// 2 & \quad C \gets \text{a copy of } B \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{with not}
///     \ B.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{insertNew}( x ) \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< class T >
ListSet< T > setUnion( const ListSet< T > & A , const ListSet< T > & B );

/// the intersection, \f$ O( n m ) \f$ time (exercise 8.1)
///
/// In pseudocode (exercise 8.1), each search a scan of the other list:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setIntersection}( A , B ) \cr
/// 2 & \quad C \gets \text{an empty set} \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{with}
///     \ B.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{insertNew}( x ) \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< class T >
ListSet< T > setIntersection( const ListSet< T > & A ,
                              const ListSet< T > & B );

/// the difference, \f$ O( n m ) \f$ time (the procedure of section 8.2)
///
/// In pseudocode (section 8.2), each search a scan of the other list:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setDifference}( A , B ) \cr
/// 2 & \quad C \gets \text{an empty set} \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{with not}
///     \ B.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{insertNew}( x ) \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< class T >
ListSet< T > setDifference( const ListSet< T > & A , const ListSet< T > & B );

/// the symmetric difference, \f$ O( n m ) \f$ time (exercise 8.5)
///
/// In pseudocode (exercise 8.5), each search a scan of the other list:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setSymmetricDifference}( A , B ) \cr
/// 2 & \quad C \gets \mathrm{setDifference}( A , B ) \cr
/// 3 & \quad \textbf{foreach}\ x \in B\ \textbf{with not}
///     \ A.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{insertNew}( x ) \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
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
