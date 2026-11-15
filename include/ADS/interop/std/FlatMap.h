/*--------------------------------------------------------------------------*/
/*----------------------------- File FlatMap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sequences of ADS as the containers of `std::flat_set` and
 * `std::flat_map` of C++23, beyond the book. These keep their keys (and
 * their values) sorted in any sequence container with random-access
 * iterators, which they edit through the members by iterator of a
 * SequenceContainer (`insert( p , x )`, `emplace( p , ... )`, `erase( p )`,
 * `erase( first , last )`); `TieredVector`, `RootishArrayStack`, `Treque`,
 * `CubishArrayStack` and `DualArrayDeque` have them, each one call of the
 * operation by rank at the rank of the iterator, and `StdSequence< S >`
 * adds them to any other sequence \f$ S \f$ of ADS that inserts and erases
 * by rank (`ContiguousDeque`, whose iterators are pointers, so that its
 * own `insert( 0 , x )` would be ambiguous between a rank and a null
 * pointer). `TieredFlatSet` and `TieredFlatMap` are the flat containers on
 * `TieredVector`: a binary search on blocks of \f$ O( \sqrt{n} ) \f$ items
 * as on a vector, and an insertion or an erasure in \f$ O( \sqrt{n} ) \f$
 * time instead of the \f$ O( n ) \f$ of `std::vector`, so that changing
 * one template argument of a `std::flat_map` with many insertions brings
 * it close to a search tree while it keeps the scans of an array.
 *
 * \see M. T. Goodrich, J. G. Kloss II, Tiered vectors: efficient dynamic
 *      arrays for rank-based sequences, WADS 1999, LNCS 1663, 205-216,
 *      doi:10.1007/3-540-48447-7_21
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

#ifndef __ADS_interop_std_FlatMap
 #define __ADS_interop_std_FlatMap
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/TieredVector.h"

#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <version>
#if defined( __cpp_lib_flat_map )
 #include <flat_map>
#endif
#if defined( __cpp_lib_flat_set )
 #include <flat_set>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS StdSequence ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the sequence \f$ S \f$ of ADS, which inserts and erases by rank, with the
/// members by iterator of a SequenceContainer, each one call of `insert( i ,
/// x )` or `erase( i )` of \f$ S \f$ at the rank \f$ i \f$ of the iterator
template< class S >
 requires( std::random_access_iterator< typename S::iterator > &&
	   requires( S & s , std::size_t i , const typename S::value_type & x )
{
	    s.insert( i , x );
	    s.erase( i );
	    } )
class StdSequence : public S
{

 public:
 using value_type = typename S::value_type;   ///< the items
 using reference = value_type &;              ///< a reference to an item
 using const_reference = const value_type &;  ///< to a constant item
 using size_type = std::size_t;               ///< the sizes
 using difference_type = std::ptrdiff_t;      ///< the distances
 using iterator = typename S::iterator;       ///< the iterator
 using const_iterator = typename S::const_iterator;  ///< the constant one

 using S::S;
 using S::begin;
 using S::end;

 /// the empty sequence
 StdSequence( void ) = default;

 /// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$, in their order
 template< std::input_iterator I , std::sentinel_for< I > E >
 StdSequence( I first , E last ) {
  insert( cend() , first , last );
  }

 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// \f$ x \f$ before the item of \f$ p \f$; an iterator on \f$ x \f$
 iterator insert( const_iterator p , const value_type & x ) {
  const std::size_t i = rankOf( p );
  S::insert( i , x );
  return( begin() + difference_type( i ) );
  }

 /// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$ before the item
 /// of \f$ p \f$, in their order; an iterator on the first of them
 template< std::input_iterator I , std::sentinel_for< I > E >
 iterator insert( const_iterator p , I first , E last ) {
  const std::size_t i0 = rankOf( p );
  for( std::size_t i = i0 ; first != last ; ++first )
   S::insert( i++ , value_type( *first ) );
  return( begin() + difference_type( i0 ) );
  }

 /// an item built from \f$ a \f$ before the item of \f$ p \f$
 template< class... A >
 iterator emplace( const_iterator p , A &&... a ) {
  return( insert( p , value_type( std::forward< A >( a )... ) ) );
  }

 /// an item built from \f$ a \f$ at the end
 template< class... A >
 reference emplace_back( A &&... a ) {
  return( *insert( cend() , value_type( std::forward< A >( a )... ) ) );
  }

 /// the item of \f$ p \f$ removed; an iterator on the item after it
 iterator erase( const_iterator p ) {
  const std::size_t i = rankOf( p );
  S::erase( i );
  return( begin() + difference_type( i ) );
  }

 /// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$ removed; an
 /// iterator on the item after them
 iterator erase( const_iterator first , const_iterator last ) {
  const std::size_t i = rankOf( first );
  for( std::size_t k = rankOf( last ) - i ; k > 0 ; --k )
   S::erase( i );
  return( begin() + difference_type( i ) );
  }

 private:
 /// the rank of the item of \f$ p \f$
 std::size_t rankOf( const_iterator p ) const {
  return( std::size_t( p - cbegin() ) );
  }

 };  // end( class StdSequence )

/*--------------------------------------------------------------------------*/
/*------------------------ THE FLAT CONTAINERS -----------------------------*/
/*--------------------------------------------------------------------------*/

#if defined( __cpp_lib_flat_set )
/// `std::flat_set` with its keys in a `TieredVector`
template< class K , class C = std::less< K > >
using TieredFlatSet = std::flat_set< K , C , TieredVector< K > >;
#endif

#if defined( __cpp_lib_flat_map )
/// `std::flat_map` with its keys and its values in two `TieredVector`s
template< class K , class V , class C = std::less< K > >
using TieredFlatMap =
  std::flat_map< K , V , C , TieredVector< K > , TieredVector< V > >;
#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* FlatMap.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File FlatMap.h --------------------------*/
/*--------------------------------------------------------------------------*/
