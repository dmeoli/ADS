/*--------------------------------------------------------------------------*/
/*-------------------------- File ArraySequence.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see ADT.h] realized with a vector: the position of an item
 * is its index, \f$ pos_i = i \f$, with \f$ pos_{-1} = -1 \f$ and
 * \f$ pos_n = n \f$. Moving along the sequence, reading and writing take
 * O( 1 ) time, while insert() and remove() take O( n ) time, since the
 * items after the position move by one place; inserting at the end takes
 * O( 1 ) amortized time, since a full vector is replaced by one of twice
 * its size. If the i-th insertion costs \f$ c_i = i \f$ when it finds the
 * vector full, i.e., when \f$ i = 2^k + 1 \f$, and 1 otherwise, then n
 * insertions cost
 * \f[
 *   C( n ) = \sum_{i=1}^n c_i \leq n + \sum_{k=0}^{\lfloor \log_2 n \rfloor}
 *            2^k \leq n + 2^{\lfloor \log_2 n \rfloor + 1} - 1 \leq 3 n \; ,
 * \f]
 * whereas growing the vector by a constant d makes them cost
 * \f$ \Theta( n^2 ) \f$.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 4.2, 4.2.1
 *      and 4.5, https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_ArraySequence
 #define __ADS_ArraySequence
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <initializer_list>
#include <memory>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ArraySequence ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized with a vector that doubles when it is full
template< class T >
class ArraySequence {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Pos = long; ///< a position is an index, from -1 to n

 ArraySequence( void ) : f_A( new T[ 1 ] ) , f_size( 0 ) , f_capacity( 1 ) {}

 /// the sequence of the given items, in their order
 ArraySequence( std::initializer_list< T > items ) : ArraySequence() {
  for( const auto & v : items )
   insert( f_size , v );
  }

 ArraySequence( const ArraySequence & other )
  : f_A( new T[ other.f_capacity ] ) , f_size( other.f_size ) ,
    f_capacity( other.f_capacity ) {
  for( Pos i = 0 ; i < f_size ; ++i )
   f_A[ i ] = other.f_A[ i ];
  }

 ArraySequence( ArraySequence && other ) noexcept = default;

 ArraySequence & operator=( ArraySequence other ) noexcept {
  f_A.swap( other.f_A );
  std::swap( f_size , other.f_size );
  std::swap( f_capacity , other.f_capacity );
  return( *this );
  }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// true if p is -1 or n
 [[nodiscard]] bool finished( Pos p ) const {
  return( ( p < 0 ) || ( p >= f_size ) );
  }

 [[nodiscard]] Pos head( void ) const { return( 0 ); }

 [[nodiscard]] Pos tail( void ) const { return( f_size - 1 ); }

 [[nodiscard]] Pos next( Pos p ) const { return( p + 1 ); }

 [[nodiscard]] Pos prev( Pos p ) const { return( p - 1 ); }

 /// puts v at index p, the items from index p moving one place on, which
 /// takes O( n - p ) time, O( 1 ) amortized at the end
 Pos insert( Pos p , const T & v ) {
  if( f_size == f_capacity ) {   // full: twice the size
   std::unique_ptr< T[] > B( new T[ 2 * f_capacity ] );
   for( Pos i = 0 ; i < f_size ; ++i )
    B[ i ] = std::move( f_A[ i ] );
   f_A = std::move( B );
   f_capacity *= 2;
   }
  for( Pos i = f_size ; i > p ; --i )
   f_A[ i ] = std::move( f_A[ i - 1 ] );
  f_A[ p ] = v;
  ++f_size;
  return( p );
  }

 /// removes the item at index p, the following ones moving one place back
 Pos remove( Pos p ) {
  for( Pos i = p ; i < f_size - 1 ; ++i )
   f_A[ i ] = std::move( f_A[ i + 1 ] );
  --f_size;
  return( p );
  }

 [[nodiscard]] const T & read( Pos p ) const { return( f_A[ p ] ); }

 void write( Pos p , const T & v ) { f_A[ p ] = v; }

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the number of items
 [[nodiscard]] Pos size( void ) const { return( f_size ); }

 /// the size of the vector, which the analysis of the doubling is about
 [[nodiscard]] Pos capacity( void ) const { return( f_capacity ); }

 /// the same items in the same order
 [[nodiscard]] bool operator==( const ArraySequence & o ) const {
  if( f_size != o.f_size ) return( false );
  for( Pos i = 0 ; i < f_size ; ++i )
   if( !( f_A[ i ] == o.f_A[ i ] ) ) return( false );
  return( true );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::unique_ptr< T[] > f_A;   ///< the vector
 Pos f_size;                   ///< the number of items
 Pos f_capacity;               ///< the size of the vector

 };  // end( class ArraySequence )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ArraySequence.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ArraySequence.h ------------------------*/
/*--------------------------------------------------------------------------*/
