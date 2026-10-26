/*--------------------------------------------------------------------------*/
/*--------------------------- File DirectTable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The table with direct access of chapter 7: when the keys are the
 * integers in an interval [ a , b ] of size m, the function H( k ) = k - a
 * is injective, a perfect hash, and the DICTIONARY [see ADT.h] is a vector
 * of size m, every operation in O( 1 ) time.
 *
 * The contracts [see Contract.h]: a <= b with b - a + 1 places that fit in
 * a std::size_t, and every key in [ a , b ], checked without overflow for
 * any long; the invariant is that size() counts the places that are not
 * empty. The operations are defined in DirectTable.tpp.
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

/// the table with direct access of the keys in [ a , b ], H( k ) = k - a:
/// every operation in O( 1 ) time, b - a + 1 places of memory
template< class V >
class DirectTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = long;   ///< the type of the keys
 using Value = V;    ///< the type of the values

 /// the empty table of the keys in [ a , b ]
 /// \pre a <= b, and b - a + 1 places fit in a std::size_t (not the whole
 ///      range of long)
 DirectTable( long a , long b );

 /// the value of k, or nothing
 /// \pre k in [ a , b ]
 [[nodiscard]] std::optional< V > lookup( long k ) const;

 /// associates v to k
 /// \pre k in [ a , b ]
 /// \post lookup( k ) is v
 void insert( long k , const V & v );

 /// removes k, if it is there
 /// \pre k in [ a , b ]
 /// \post lookup( k ) is nothing
 void remove( long k );

 [[nodiscard]] std::size_t size( void ) const;

 /// the size m = b - a + 1 of the vector
 [[nodiscard]] std::size_t capacity( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, the key computed from the place

 /// the places of the table, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_A.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_A[ i ].has_value() );
   }
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< long , const V & >( o.f_a + long( i ) , *o.f_A[ i ] ) );
   }
  };

 using value_type = std::pair< long , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::pair< long , const V & >;
 using const_reference = std::pair< long , const V & >;
 using const_iterator = SlotIterator< DirectTable , Places >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// H( k ) = k - a
 [[nodiscard]] std::size_t H( long k ) const;

 /// whether k is in [ a , b ]
 [[nodiscard]] bool inRange( long k ) const;

 /// size() counts the places that are not empty, O( m )
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
