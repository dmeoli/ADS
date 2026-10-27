/*--------------------------------------------------------------------------*/
/*-------------------------- File SortedListSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET as a sorted LIST of section 8.3: `insert()` in \f$ O( n ) \f$,
 * `min()` and `max()` in \f$ O( 1 ) \f$, the operations of two sets by
 * merging the two lists in \f$ O( n + m ) \f$, optimal since merging needs
 * \f$ \Omega( n + m ) \f$ (exercises 8.3 and 8.4).
 *
 * The merges append to the end of the result, the place
 * \f$ \mathit{pos}_n = \mathrm{next}( \mathrm{tail}() ) \f$ of a LIST: the
 * `C.insert( C.tail() , x )` of the book, with the `insert()` of section 4.1
 * that puts \f$ x \f$ before the item in the given position, would put
 * \f$ x \f$ before the last item.
 *
 * The contracts [see `Contract.h`]: `insertLast()` needs an item larger
 * than the others, after `insert()` the item is there and after `remove()`
 * it is not, and each operation of two sets gives what its name says; the
 * invariant is that the list increases strictly. The operations are defined
 * in `SortedListSet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 8.3,
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

 /// the empty set
 SortedListSet( void ) = default;

 /// the set of the given items, in any order, a repeated one taken once
 SortedListSet( std::initializer_list< T > items );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the set has no item
 [[nodiscard]] bool empty( void ) const;

 /// whether \f$ x \f$ is in the set, by a scan that stops at the first
 /// item not smaller than \f$ x \f$, \f$ O( n ) \f$ time
 [[nodiscard]] bool contains( const T & x ) const;

 /// \f$ x \f$ into the set, \f$ O( n ) \f$ time, to keep the order
 ///
 /// In pseudocode (section 8.3), \f$ L \f$ the list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad p \gets L.\mathrm{head}() \cr
 /// 3 & \quad \textbf{while not}\ L.\mathrm{finished}( p )\ \textbf{and}
 ///     \ L.\mathrm{read}( p ) < x\ \textbf{do}\ p \gets L.\mathrm{next}( p )
 ///     \cr
 /// 4 & \quad \textbf{if}\ L.\mathrm{finished}( p )\ \textbf{or}
 ///     \ L.\mathrm{read}( p ) \ne x\ \textbf{then}\ L.\mathrm{insert}( p , x
 ///     )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is in the set
 void insert( const T & x );

 /// \f$ x \f$, larger than all the items, at the end of the list,
 /// \f$ O( 1 ) \f$ time
 /// \pre the set is empty or its largest item is smaller than \f$ x \f$
 void insertLast( const T & x );

 /// \f$ x \f$ out of the set, \f$ O( n ) \f$ time
 ///
 /// In pseudocode (section 8.3), the scan of `insert()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad p \gets \text{the first position of } L \text{ not smaller }
 ///     \text{than } x \cr
 /// 3 & \quad \textbf{if not}\ L.\mathrm{finished}( p )\ \textbf{and}
 ///     \ L.\mathrm{read}( p ) = x\ \textbf{then}\ L.\mathrm{remove}( p )
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not in the set
 void remove( const T & x );

 /// the smallest item, at the head, none if the set is empty,
 /// \f$ O( 1 ) \f$ time
 [[nodiscard]] std::optional< T > min( void ) const;

 /// the largest item, at the tail, none if the set is empty,
 /// \f$ O( 1 ) \f$ time
 [[nodiscard]] std::optional< T > max( void ) const;

 /// `f( x )` on the items in increasing order, \f$ O( n ) \f$ time
 template< class F >
 void forEach( F && f ) const;

 /// the sorted list of the items
 [[nodiscard]] const List< T > & list( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in increasing order, bidirectional

 using value_type = T;                    ///< the items
 using size_type = std::size_t;           ///< the sizes
 using difference_type = std::ptrdiff_t;  ///< the distances
 using reference = const T &;             ///< a constant reference
 using const_reference = const T &;       ///< a constant reference
 /// the iterator of the list, constant
 using const_iterator = typename List< T >::const_iterator;
 using iterator = const_iterator;         ///< the same, constant

 /// the iterator on the smallest item
 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 /// the iterator past the largest item
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the position of the first item not smaller than \f$ x \f$
 typename List< T >::Pos lower( const T & x ) const;

 /// the list increases strictly, and the size is its length, \f$ O( n ) \f$
 [[nodiscard]] bool sorted( void ) const;

 List< T > f_L;              ///< the items, in increasing order
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class SortedListSet )

/// the merge of the sorted lists of \f$ A \f$ and \f$ B \f$: an item of
/// \f$ A \f$ only, of \f$ B \f$ only or of both goes into the result if
/// `a`, `b` or `both` say so; \f$ O( n + m ) \f$ time
///
/// In pseudocode (section 8.3), each comparison passing at least one item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{merge}( A , B , a , b , \mathit{both} ) \cr
/// 2 & \quad C \gets \text{an empty set};\ p \gets A.\mathrm{head}();\ q
///     \gets B.\mathrm{head}() \cr
/// 3 & \quad \textbf{while not}\ A.\mathrm{finished}( p )\ \textbf{and not}
///     \ B.\mathrm{finished}( q )\ \textbf{do} \cr
/// 4 & \qquad x \gets A.\mathrm{read}( p );\ y \gets B.\mathrm{read}( q ) \cr
/// 5 & \qquad \textbf{if}\ x = y\ \textbf{then} \cr
/// 6 & \qquad\quad \textbf{if}\ \mathit{both}\ \textbf{then}
///     \ C.\mathrm{insertLast}( x ) \cr
/// 7 & \qquad\quad p \gets A.\mathrm{next}( p );\ q \gets B.\mathrm{next}( q )
///     \cr
/// 8 & \qquad \textbf{else if}\ x < y\ \textbf{then} \cr
/// 9 & \qquad\quad \textbf{if}\ a\ \textbf{then}\ C.\mathrm{insertLast}( x )
///     \cr
/// 10 & \qquad\quad p \gets A.\mathrm{next}( p ) \cr
/// 11 & \qquad \textbf{else} \cr
/// 12 & \qquad\quad \textbf{if}\ b\ \textbf{then}\ C.\mathrm{insertLast}( y )
///     \cr
/// 13 & \qquad\quad q \gets B.\mathrm{next}( q ) \cr
/// 14 & \quad \textbf{if}\ a\ \textbf{then}\ \text{append to } C \text{ the }
///     \text{items of } A \text{ from } p \cr
/// 15 & \quad \textbf{if}\ b\ \textbf{then}\ \text{append to } C \text{ the }
///     \text{items of } B \text{ from } q \cr
/// 16 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< class T >
SortedListSet< T > merge( const SortedListSet< T > & A ,
                          const SortedListSet< T > & B ,
                          bool a , bool b , bool both );

/// the union, \f$ O( n + m ) \f$ time (exercise 8.3)
///
/// It is the merge above with the items it keeps:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setUnion}( A , B ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{merge}( A , B ,
///     \textbf{true} , \textbf{true} , \textbf{true} )
/// \end{array}}
/// \f]
template< class T >
SortedListSet< T > setUnion( const SortedListSet< T > & A ,
                             const SortedListSet< T > & B );

/// the intersection, \f$ O( n + m ) \f$ time (the procedure of section 8.3)
///
/// It is the merge above with the items it keeps:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setIntersection}( A , B ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{merge}( A , B ,
///     \textbf{false} , \textbf{false} , \textbf{true} )
/// \end{array}}
/// \f]
template< class T >
SortedListSet< T > setIntersection( const SortedListSet< T > & A ,
                                    const SortedListSet< T > & B );

/// the difference, \f$ O( n + m ) \f$ time (exercise 8.4)
///
/// It is the merge above with the items it keeps:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setDifference}( A , B ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{merge}( A , B ,
///     \textbf{true} , \textbf{false} , \textbf{false} )
/// \end{array}}
/// \f]
template< class T >
SortedListSet< T > setDifference( const SortedListSet< T > & A ,
                                  const SortedListSet< T > & B );

/// the symmetric difference, \f$ O( n + m ) \f$ time (exercise 8.5)
///
/// It is the merge above with the items it keeps:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setSymmetricDifference}( A , B ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{merge}( A , B ,
///     \textbf{true} , \textbf{true} , \textbf{false} )
/// \end{array}}
/// \f]
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
