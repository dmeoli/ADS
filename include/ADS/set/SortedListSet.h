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
/*-------------------------- CLASS SortedListSet ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set as a list sorted in increasing order
template< std::totally_ordered T >
class SortedListSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 SortedListSet( void ) = default;

 SortedListSet( std::initializer_list< T > items ) {
  for( const auto & x : items )
   insert( x );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 [[nodiscard]] bool contains( const T & x ) const {
  const auto p = lower( x );
  return( ! f_L.finished( p ) && ( f_L.read( p ) == x ) );
  }

 /// O( n ) time, to keep the order
 void insert( const T & x ) {
  const auto p = lower( x );
  if( f_L.finished( p ) || ! ( f_L.read( p ) == x ) ) {
   f_L.insert( p , x );
   ++f_size;
   }
  }

 /// appends x, larger than all the items, O( 1 ) time; throws
 /// std::logic_error if it is not
 void insertLast( const T & x ) {
  if( f_size && ! ( f_L.read( f_L.tail() ) < x ) )
   throw( std::logic_error(
                    "ADS::SortedListSet::insertLast: x is not the largest" ) );
  f_L.insert( f_L.next( f_L.tail() ) , x );
  ++f_size;
  }

 void remove( const T & x ) {
  const auto p = lower( x );
  if( ! f_L.finished( p ) && ( f_L.read( p ) == x ) ) {
   f_L.remove( p );
   --f_size;
   }
  }

 /// the smallest item, O( 1 ) time
 [[nodiscard]] std::optional< T > min( void ) const {
  if( ! f_size )
   return( std::nullopt );
  return( f_L.read( f_L.head() ) );
  }

 /// the largest item, O( 1 ) time
 [[nodiscard]] std::optional< T > max( void ) const {
  if( ! f_size )
   return( std::nullopt );
  return( f_L.read( f_L.tail() ) );
  }

 /// f( x ) on the items in increasing order, O( n ) time
 template< class F >
 void forEach( F && f ) const {
  for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) )
   f( f_L.read( p ) );
  }

 /// the sorted list of the items
 [[nodiscard]] const List< T > & list( void ) const { return( f_L ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the position of the first item not smaller than x
 typename List< T >::Pos lower( const T & x ) const {
  auto p = f_L.head();
  while( ! f_L.finished( p ) && ( f_L.read( p ) < x ) )
   p = f_L.next( p );
  return( p );
  }

 List< T > f_L;              ///< the items, in increasing order
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class SortedListSet )

/// the merge of the sorted lists of A and B: an item of A only, of B only
/// or of both goes into the result if a, b or both say so; O( n + m ) time
template< class T >
SortedListSet< T > merge( const SortedListSet< T > & A ,
                          const SortedListSet< T > & B ,
                          bool a , bool b , bool both )
{
 SortedListSet< T > C;
 const auto & LA = A.list();
 const auto & LB = B.list();
 auto p = LA.head();
 auto q = LB.head();
 while( ! LA.finished( p ) && ! LB.finished( q ) ) {
  const T & x = LA.read( p );
  const T & y = LB.read( q );
  if( x == y ) {
   if( both ) C.insertLast( x );
   p = LA.next( p );
   q = LB.next( q );
   }
  else if( x < y ) {
   if( a ) C.insertLast( x );
   p = LA.next( p );
   }
  else {
   if( b ) C.insertLast( y );
   q = LB.next( q );
   }
  }
 for( ; a && ! LA.finished( p ) ; p = LA.next( p ) )
  C.insertLast( LA.read( p ) );
 for( ; b && ! LB.finished( q ) ; q = LB.next( q ) )
  C.insertLast( LB.read( q ) );
 return( C );
 }

/// the union, O( n + m ) time (exercise 8.3)
template< class T >
SortedListSet< T > setUnion( const SortedListSet< T > & A ,
                             const SortedListSet< T > & B )
{
 return( merge( A , B , true , true , true ) );
 }

/// the intersection, O( n + m ) time (the procedure of section 8.3)
template< class T >
SortedListSet< T > setIntersection( const SortedListSet< T > & A ,
                                    const SortedListSet< T > & B )
{
 return( merge( A , B , false , false , true ) );
 }

/// the difference, O( n + m ) time (exercise 8.4)
template< class T >
SortedListSet< T > setDifference( const SortedListSet< T > & A ,
                                  const SortedListSet< T > & B )
{
 return( merge( A , B , true , false , false ) );
 }

/// the symmetric difference, O( n + m ) time (exercise 8.5)
template< class T >
SortedListSet< T > setSymmetricDifference( const SortedListSet< T > & A ,
                                           const SortedListSet< T > & B )
{
 return( merge( A , B , true , true , false ) );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* SortedListSet.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SortedListSet.h ------------------------*/
/*--------------------------------------------------------------------------*/
