/*--------------------------------------------------------------------------*/
/*----------------------------- File ListSet.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET as an unsorted LIST of section 8.2: contains(), insert() and
 * remove() in O( n ), insertNew() in O( 1 ) when the item is known to be
 * absent, the operations in O( n m ), which is optimal when the items can
 * only be compared for equality (exercises 8.1 and 8.2).
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

#include "ADS/sequence/List.h"

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

 ListSet( std::initializer_list< T > items ) {
  for( const auto & x : items )
   insert( x );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 [[nodiscard]] bool contains( const T & x ) const {
  return( ! f_L.finished( find( x ) ) );
  }

 /// O( n ) time, for the check that x is not there
 void insert( const T & x ) {
  if( ! contains( x ) )
   insertNew( x );
  }

 /// inserts x, which must not be there, at the head, O( 1 ) time
 void insertNew( const T & x ) {
  f_L.insert( f_L.head() , x );
  ++f_size;
  }

 void remove( const T & x ) {
  const auto p = find( x );
  if( ! f_L.finished( p ) ) {
   f_L.remove( p );
   --f_size;
   }
  }

 /// the smallest item, O( n ) time
 [[nodiscard]] std::optional< T > min( void ) const
  requires std::totally_ordered< T > {
  std::optional< T > m;
  forEach( [ & ]( const T & x ) { if( ! m || ( x < *m ) ) m = x; } );
  return( m );
  }

 /// f( x ) on the items, from the last inserted, O( n ) time
 template< class F >
 void forEach( F && f ) const {
  for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) )
   f( f_L.read( p ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 typename List< T >::Pos find( const T & x ) const {
  auto p = f_L.head();
  while( ! f_L.finished( p ) && ! ( f_L.read( p ) == x ) )
   p = f_L.next( p );
  return( p );
  }

 List< T > f_L;              ///< the items
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class ListSet )

/// the union: C starts as B and gets the items of A not in B, inserted
/// without looking for them in C, O( n m ) time (exercise 8.2)
template< class T >
ListSet< T > setUnion( const ListSet< T > & A , const ListSet< T > & B )
{
 ListSet< T > C = B;
 A.forEach( [ & ]( const T & x ) {
  if( ! B.contains( x ) )
   C.insertNew( x );
  } );
 return( C );
 }

/// the intersection, O( n m ) time (exercise 8.1)
template< class T >
ListSet< T > setIntersection( const ListSet< T > & A ,
                              const ListSet< T > & B )
{
 ListSet< T > C;
 A.forEach( [ & ]( const T & x ) {
  if( B.contains( x ) )
   C.insertNew( x );
  } );
 return( C );
 }

/// the difference, O( n m ) time (the procedure of section 8.2)
template< class T >
ListSet< T > setDifference( const ListSet< T > & A , const ListSet< T > & B )
{
 ListSet< T > C;
 A.forEach( [ & ]( const T & x ) {
  if( ! B.contains( x ) )
   C.insertNew( x );
  } );
 return( C );
 }

/// the symmetric difference, O( n m ) time (exercise 8.5)
template< class T >
ListSet< T > setSymmetricDifference( const ListSet< T > & A ,
                                     const ListSet< T > & B )
{
 ListSet< T > C = setDifference( A , B );
 B.forEach( [ & ]( const T & x ) {
  if( ! A.contains( x ) )
   C.insertNew( x );
  } );
 return( C );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ListSet.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ListSet.h ---------------------------*/
/*--------------------------------------------------------------------------*/
