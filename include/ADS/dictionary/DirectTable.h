/*--------------------------------------------------------------------------*/
/*--------------------------- File DirectTable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The table with direct access of chapter 7: when the keys are the
 * integers in an interval \f$ [ a , b ] \f$ of size \f$ m \f$, the function
 * \f$ H( k ) = k - a \f$ is injective, a perfect hash, and the DICTIONARY
 * [see `ADT.h`] is a vector of size \f$ m \f$, every operation in
 * \f$ O( 1 ) \f$ time.
 *
 * The contracts [see `Contract.h`]: \f$ a \le b \f$ with \f$ b - a + 1 \f$
 * places that fit in a `std::size_t`, and every key in \f$ [ a , b ] \f$,
 * checked without overflow for any `long`; the invariant is that `size()`
 * counts the places that are not empty. The operations are defined in
 * `DirectTable.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
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

#ifndef __ADS_DirectTable
 #define __ADS_DirectTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <cstddef>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DirectTable ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the table with direct access of the keys in \f$ [ a , b ] \f$,
/// \f$ H( k ) = k - a \f$: every operation in \f$ O( 1 ) \f$ time,
/// \f$ b - a + 1 \f$ places of memory
template< class V >
class DirectTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = long;   ///< the type of the keys
 using Value = V;    ///< the type of the values

 /// the empty table of the keys in \f$ [ a , b ] \f$
 /// \pre \f$ a \le b \f$, and \f$ b - a + 1 \f$ places fit in a `std::size_t`
 ///      (not the whole range of `long`)
 DirectTable( long a , long b );

 /// the value of \f$ k \f$, the place \f$ A[ k - a ] \f$, or nothing
 /// \pre \f$ k \in [ a , b ] \f$
 [[nodiscard]] std::optional< V > lookup( long k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, writing it in \f$ A[ k - a ] \f$
 /// \pre \f$ k \in [ a , b ] \f$
 /// \post `lookup( k )` is \f$ v \f$
 void insert( long k , const V & v );

 /// removes \f$ k \f$, if it is there, emptying \f$ A[ k - a ] \f$
 /// \pre \f$ k \in [ a , b ] \f$
 /// \post `lookup( k )` is nothing
 void remove( long k );

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the size \f$ m = b - a + 1 \f$ of the vector
 [[nodiscard]] std::size_t capacity( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, the key computed from the place

 /// the places of the table, for `SlotIterator`
 struct Places {
  /// the number of places
  static std::size_t size( const auto & o ) { return( o.f_A.size() ); }
  /// whether the place \f$ i \f$ holds a key
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_A[ i ].has_value() );
   }
  /// the pair of the place \f$ i \f$, its key computed as \f$ a + i \f$
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< long , const V & >( o.f_a + long( i ) , *o.f_A[ i ] ) );
   }
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< long , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = std::pair< long , const V & >;
 /// a constant reference to an item
 using const_reference = std::pair< long , const V & >;
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< DirectTable , Places >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// \f$ H( k ) = k - a \f$
 [[nodiscard]] std::size_t H( long k ) const;

 /// whether \f$ k \f$ is in \f$ [ a , b ] \f$
 [[nodiscard]] bool inRange( long k ) const;

 /// size() counts the places that are not empty, \f$ O( m ) \f$
 [[nodiscard]] bool counted( void ) const;

 long f_a;                              ///< the smallest key
 std::vector< std::optional< V > > f_A; ///< the vector, nil if empty
 std::size_t f_size = 0;                ///< the number of keys

 };  // end( class DirectTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/DirectTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DirectTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File DirectTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
