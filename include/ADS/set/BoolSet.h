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
 explicit BoolSet( long N ) {
  if( N < 0 )
   throw( std::invalid_argument( "ADS::BoolSet: N < 0" ) );
  f_V.assign( N , false );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// the size N of the vector
 [[nodiscard]] long dim( void ) const { return( long( f_V.size() ) ); }

 /// false also for an x out of [ 0 , N )
 [[nodiscard]] bool contains( long x ) const {
  return( ( x >= 0 ) && ( x < dim() ) && f_V[ x ] );
  }

 /// throws std::out_of_range if x is not in [ 0 , N )
 void insert( long x ) {
  if( ( x < 0 ) || ( x >= dim() ) )
   throw( std::out_of_range( "ADS::BoolSet::insert: x out of [0, N)" ) );
  if( ! f_V[ x ] )
   ++f_size;
  f_V[ x ] = true;
  }

 void remove( long x ) {
  if( contains( x ) ) {
   --f_size;
   f_V[ x ] = false;
   }
  }

 /// the smallest item, O( N ) time
 [[nodiscard]] std::optional< long > min( void ) const {
  for( long i = 0 ; i < dim() ; ++i )
   if( f_V[ i ] )
    return( i );
  return( std::nullopt );
  }

 /// the largest item, O( N ) time
 [[nodiscard]] std::optional< long > max( void ) const {
  for( long i = dim() - 1 ; i >= 0 ; --i )
   if( f_V[ i ] )
    return( i );
  return( std::nullopt );
  }

 /// f( x ) on the items in increasing order, O( N ) time
 template< class F >
 void forEach( F && f ) const {
  for( long i = 0 ; i < dim() ; ++i )
   if( f_V[ i ] )
    f( i );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< bool > f_V;    ///< the bits
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class BoolSet )

/// the union, a set of dimension max( A.dim() , B.dim() )
inline BoolSet setUnion( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) );
 for( long i = 0 ; i < A.dim() ; ++i )
  if( A.contains( i ) ) C.insert( i );
 for( long i = 0 ; i < B.dim() ; ++i )
  if( B.contains( i ) ) C.insert( i );
 return( C );
 }

/// the intersection, a set of dimension min( A.dim() , B.dim() )
inline BoolSet setIntersection( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::min( A.dim() , B.dim() ) );
 for( long i = 0 ; i < C.dim() ; ++i )
  if( A.contains( i ) && B.contains( i ) ) C.insert( i );
 return( C );
 }

/// the difference, a set of dimension A.dim()
inline BoolSet setDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( A.dim() );
 for( long i = 0 ; i < A.dim() ; ++i )
  if( A.contains( i ) && ! B.contains( i ) ) C.insert( i );
 return( C );
 }

/// the symmetric difference, the items in exactly one of A and B
inline BoolSet setSymmetricDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) );
 for( long i = 0 ; i < C.dim() ; ++i )
  if( A.contains( i ) != B.contains( i ) ) C.insert( i );
 return( C );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* BoolSet.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BoolSet.h ---------------------------*/
/*--------------------------------------------------------------------------*/
