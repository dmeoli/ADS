/*--------------------------------------------------------------------------*/
/*----------------------------- File BoolSet.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET of the integers in [ 0 , N ) as a boolean vector of section 8.1:
 * contains(), insert() and remove() in O( 1 ), min(), max() and forEach()
 * in O( N ), union, intersection, difference and symmetric difference in
 * \f$ \Theta( N ) \f$ (the first is called setUnion(), union being a
 * keyword of C++, and so the others are setIntersection(),
 * setDifference() and setSymmetricDifference()).
 *
 * The contracts [see Contract.h]: N >= 0, an item inserted is in
 * [ 0 , N ), and each operation of two sets gives, item by item, what its
 * name says; the invariant is that the size counts the bits set. The
 * operations are defined in BoolSet.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 8.1,
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

#ifndef __ADS_BoolSet
 #define __ADS_BoolSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

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
/*----------------------------- CLASS BoolSet ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of integers in [ 0 , N ) as a boolean vector
class BoolSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = long;   ///< the type of the items

 /// Set( N ): the empty set of the integers in [ 0 , N )
 /// \pre N >= 0
 explicit BoolSet( long N );

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the size N of the vector
 [[nodiscard]] long dim( void ) const;

 /// false also for an x out of [ 0 , N )
 [[nodiscard]] bool contains( long x ) const;

 /// \pre x in [ 0 , N )
 /// \post x is in the set
 void insert( long x );

 /// \post x is not in the set
 void remove( long x );

 /// the smallest item, O( N ) time
 [[nodiscard]] std::optional< long > min( void ) const;

 /// the largest item, O( N ) time
 [[nodiscard]] std::optional< long > max( void ) const;

 /// f( x ) on the items in increasing order, O( N ) time
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in increasing order, bidirectional; an
 // item is computed from its bit

 /// the places of the table, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_V.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_V[ i ] );
   }
  static auto read( const auto & , std::size_t i ) {
   return( long( i ) );
   }
  };

 using value_type = long;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = long;
 using const_reference = long;
 using const_iterator = SlotIterator< BoolSet , Places >;
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

 /// the size is the number of bits set, O( N )
 [[nodiscard]] bool counted( void ) const;

 std::vector< bool > f_V;    ///< the bits
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class BoolSet )

/// the union, a set of dimension max( A.dim() , B.dim() )
inline BoolSet setUnion( const BoolSet & A , const BoolSet & B );

/// the intersection, a set of dimension min( A.dim() , B.dim() )
inline BoolSet setIntersection( const BoolSet & A , const BoolSet & B );

/// the difference, a set of dimension A.dim()
inline BoolSet setDifference( const BoolSet & A , const BoolSet & B );

/// the symmetric difference, the items in exactly one of A and B
inline BoolSet setSymmetricDifference( const BoolSet & A ,
                                       const BoolSet & B );

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/BoolSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BoolSet.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BoolSet.h ---------------------------*/
/*--------------------------------------------------------------------------*/
