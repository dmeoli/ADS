/*--------------------------------------------------------------------------*/
/*------------------------------- File Bag.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The bag of exercise 1.5 of Morin's book, beyond the book: a `USet` [see
 * `SetInterfaces.h`] that keeps the items equal to one another, all of
 * them: `insert( x )` always adds \f$ x \f$, `find( x )` gives one of the
 * items equal to \f$ x \f$, `findAll( x )` all of them, `count( x )` how
 * many, and `remove( x )` removes the last one added, each in
 * \f$ O( 1 ) \f$ expected time (amortized for `insert()` and `remove()`,
 * which may rebuild the table) but `findAll()`, linear in what it gives.
 * The bag is a USet of groups, as the exercise asks: a hash table [see
 * `FlatHashTable.h`] from an item to the vector of the items equal to it,
 * in the order of their insertions; equal items need not be identical
 * (records equal by their key, e.g.), which is what sets it apart from a
 * multiset of counters [see `HashMultiset.h`].
 *
 * The contracts [see `Contract.h`]: after `insert( x )` there is one more
 * item equal to \f$ x \f$, after `remove( x )` one less if there was one;
 * the invariant is that no group is empty and that the groups count
 * `size()` items. The operations are defined in `Bag.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercise 1.5
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

#ifndef __ADS_Bag
 #define __ADS_Bag
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FlatHashTable.h"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASS Bag --------------------------------*/
/*--------------------------------------------------------------------------*/

/// a USet that keeps the items equal to one another, on a hash table of
/// groups
template< class T , class H = DefaultHash< T > ,
	  class Eq = std::equal_to< T > >
class Bag
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the items

 /// the empty bag
 explicit Bag( H h = H() , Eq eq = Eq() ) : f_T( 0 , h , eq ) { }

 /// the number \f$ n \f$ of items, the equal ones counted each
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of groups of equal items
 [[nodiscard]] std::size_t distinct( void ) const { return( f_T.size() ); }

 /// adds \f$ x \f$, \f$ O( 1 ) \f$ expected amortized; always true
 ///
 /// In pseudocode (Morin, exercise 1.5), \f$ G \f$ the table of the
 /// groups:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad \textbf{if}\ G.\mathrm{find}( x ) = \textbf{nil}\ \textbf{then}
 ///     \ G.\mathrm{insert}( x , \langle x \rangle ) \cr
 /// 3 & \quad \textbf{else}\ \text{append } x \text{ to } G.\mathrm{find}( x
 ///     ) \cr
 /// 4 & \quad n \gets n + 1;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post one item equal to \f$ x \f$ more
 bool insert( const T & x );

 /// removes the last item added equal to \f$ x \f$, \f$ O( 1 ) \f$
 /// expected amortized; true if there was one
 ///
 /// In pseudocode (Morin, exercise 1.5), \f$ G \f$ the table of the
 /// groups:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad g \gets G.\mathrm{find}( x );\ \textbf{if}\ g =
 ///     \textbf{nil}\ \textbf{then return false} \cr
 /// 3 & \quad \textbf{if}\ | g | > 1\ \textbf{then}\ \text{remove the
 ///     last item of } g\ \textbf{else}\ G.\mathrm{remove}( x ) \cr
 /// 4 & \quad n \gets n - 1;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post one item equal to \f$ x \f$ less, if there was one
 bool remove( const T & x );

 /// an item equal to \f$ x \f$, the first one added, nil if none,
 /// \f$ O( 1 ) \f$ expected
 [[nodiscard]] const T * find( const T & x ) const;

 /// all the items equal to \f$ x \f$, in the order of their insertions
 [[nodiscard]] std::vector< T > findAll( const T & x ) const;

 /// the number of items equal to \f$ x \f$
 [[nodiscard]] std::size_t count( const T & x ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// no group is empty and the groups count \f$ n \f$ items
 [[nodiscard]] bool counted( void ) const;

 FlatHashTable< T , std::vector< T > , H , Eq > f_T;  ///< the groups
 std::size_t f_n = 0;                              ///< the items

 };  // end( class Bag )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/Bag.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Bag.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File Bag.h -----------------------------*/
/*--------------------------------------------------------------------------*/
