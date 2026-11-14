/*--------------------------------------------------------------------------*/
/*-------------------------- File SetInterfaces.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The interfaces of chapter 1 of Morin's book for the sets, beyond the
 * book, as concepts of C++20 [see `ADT.h` for the SET of the book], and
 * their realizations on a vector that exercise 1.6 asks for, the oracles of
 * the tests of the faster ones:
 *
 * - `USet`, the unordered set of section 1.2.3 of Morin: `size()`,
 *   `insert( x )` (Morin's `add( x )`), true if no item equal to \f$ x \f$
 *   was there, `remove( x )`, true if one was, and `find( x )`, the address
 *   of the item equal to \f$ x \f$ or nil, an item that may carry more than
 *   its key;
 * - `SSet`, the sorted set of section 1.2.4: a `USet` whose items are
 *   ordered, with `lowerBound( x )`, the address of the least item not
 *   smaller than \f$ x \f$ or nil (Morin's `find( x )` of an `SSet`, the
 *   successor search);
 * - `KeyedDictionary` and `OrderedDictionary`, the dictionaries that show
 *   their keys as a set: `findKey( k )` the address of the key \f$ k \f$
 *   they hold, and `lowerBoundKey( k )` that of the smallest key not
 *   smaller than \f$ k \f$, both nil if none; `DictionarySet` over the
 *   first is a `USet` and over the second an `SSet` [see
 *   `DictionarySet.h`], the one way in which every hash table, search
 *   tree, skip list and B-tree of the library, whose `lowerBound()` gives
 *   a pair or an iterator, is a set of Morin;
 * - `ArrayUSet`, a `USet` on a vector in no order: `find()` scans it in
 *   \f$ O( n ) \f$, `remove()` moves the last item into the place freed in
 *   \f$ O( 1 ) \f$ after the scan;
 * - `ArraySSet`, an `SSet` on a sorted vector: `find()` and `lowerBound()`
 *   by binary search in \f$ O( \log n ) \f$, `insert()` and `remove()` move
 *   the items after the place by one `memmove()` in \f$ O( n ) \f$ (the
 *   improvements of exercise 1.7: the binary search for the
 *   `find()`, the block move for the changes).
 *
 * The contracts [see `Contract.h`]: after `insert()` the item is there and
 * after `remove()` it is not; the invariant of `ArraySSet` is the strict
 * order of its vector, an \f$ O( n ) \f$ check. The operations are defined
 * in `SetInterfaces.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, sections 1.2.3 and 1.2.4
 *      and exercises 1.6 and 1.7
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

#ifndef __ADS_SetInterfaces
 #define __ADS_SetInterfaces
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the unordered set of Morin, whose `find()` gives the item equal to the
/// one asked
template< class S >
concept USet = requires( S s , const S cs , const typename S::Item & x ) {
 typename S::Item;
 { cs.size() } -> std::convertible_to< std::size_t >;
 { s.insert( x ) } -> std::same_as< bool >;
 { s.remove( x ) } -> std::same_as< bool >;
 { cs.find( x ) } -> std::convertible_to< const typename S::Item * >;
 };

/// the sorted set of Morin, a `USet` with the successor search
template< class S >
concept SSet = USet< S > &&
 requires( const S cs , const typename S::Item & x ) {
 { cs.lowerBound( x ) } -> std::convertible_to< const typename S::Item * >;
 };

/// a dictionary that gives the address of the key equal to the one asked,
/// on which `DictionarySet` is a `USet`; the search may change the
/// dictionary (a splay tree), not the set of its keys
template< class D >
concept KeyedDictionary = requires( D d , const typename D::Key & k ,
                                    const typename D::Value & v ) {
 typename D::Key;
 typename D::Value;
 { d.findKey( k ) } -> std::convertible_to< const typename D::Key * >;
 d.insert( k , v );
 d.remove( k );
 };

/// a `KeyedDictionary` that gives the address of the smallest key not
/// smaller than the one asked, on which `DictionarySet` is an `SSet`
template< class D >
concept OrderedDictionary = KeyedDictionary< D > &&
 requires( D d , const typename D::Key & k ) {
 { d.lowerBoundKey( k ) } -> std::convertible_to< const typename D::Key * >;
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ArrayUSet -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a `USet` on a vector in no order, the items equal by `Eq`
template< class T , class Eq = std::equal_to<> >
class ArrayUSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty set
 explicit ArrayUSet( Eq eq = Eq() ) : f_eq( std::move( eq ) ) {}

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// the item equal to \f$ x \f$, nil if none, \f$ O( n ) \f$
 [[nodiscard]] const T * find( const T & x ) const;

 /// \f$ x \f$ added if no item equal to it is there, \f$ O( n ) \f$; true
 /// if added
 /// \post an item equal to \f$ x \f$ is there
 bool insert( const T & x );

 /// the item equal to \f$ x \f$ removed, the last one moved into its
 /// place, \f$ O( n ) \f$; true if there was one
 /// \post no item equal to \f$ x \f$ is there
 bool remove( const T & x );

 /// the items, in no order
 [[nodiscard]] const std::vector< T > & items( void ) const { return( f_A ); }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< T > f_A;            ///< the items
 [[no_unique_address]] Eq f_eq;   ///< the equality

 };  // end( class ArrayUSet )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ArraySSet -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an `SSet` on a sorted vector, ordered by `C`
template< class T , class C = std::less<> >
class ArraySSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty set
 explicit ArraySSet( C less = C() ) : f_less( std::move( less ) ) {}

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// the item equal to \f$ x \f$, nil if none, \f$ O( \log n ) \f$
 [[nodiscard]] const T * find( const T & x ) const;

 /// the least item not smaller than \f$ x \f$, nil if none, by binary
 /// search in \f$ O( \log n ) \f$ (Morin's `find( x )` of an `SSet`)
 [[nodiscard]] const T * lowerBound( const T & x ) const;

 /// \f$ x \f$ added in its place if no item equal to it is there, the items
 /// after it moved by one place, \f$ O( n ) \f$; true if added
 /// \post an item equal to \f$ x \f$ is there
 bool insert( const T & x );

 /// the item equal to \f$ x \f$ removed, \f$ O( n ) \f$; true if there was
 /// one
 /// \post no item equal to \f$ x \f$ is there
 bool remove( const T & x );

 /// the items, increasing
 [[nodiscard]] const std::vector< T > & items( void ) const { return( f_A ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the items increase strictly
 [[nodiscard]] bool sorted( void ) const;

 std::vector< T > f_A;            ///< the items, increasing
 [[no_unique_address]] C f_less;  ///< the order

 };  // end( class ArraySSet )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/SetInterfaces.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SetInterfaces.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SetInterfaces.h ------------------------*/
/*--------------------------------------------------------------------------*/
