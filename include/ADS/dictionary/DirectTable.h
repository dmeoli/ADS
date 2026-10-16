/*--------------------------------------------------------------------------*/
/*--------------------------- File DirectTable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The table with direct access of chapter 7: when the keys are the
 * integers in an interval [ a , b ] of size m, the function H( k ) = k - a
 * is injective, a perfect hash, and the DICTIONARY [see ADT.h] is a vector
 * of size m, every operation in O( 1 ) time; a key out of [ a , b ] throws
 * std::out_of_range.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
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

#ifndef __ADS_DirectTable
 #define __ADS_DirectTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
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
 DirectTable( long a , long b ) : f_a( a ) {
  if( b < a )
   throw( std::invalid_argument( "ADS::DirectTable: b < a" ) );
  f_A.resize( b - a + 1 );
  }

 [[nodiscard]] std::optional< V > lookup( long k ) const {
  return( f_A[ H( k ) ] );
  }

 void insert( long k , const V & v ) {
  auto & c = f_A[ H( k ) ];
  if( ! c )
   ++f_size;
  c = v;
  }

 void remove( long k ) {
  auto & c = f_A[ H( k ) ];
  if( c )
   --f_size;
  c.reset();
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the size m = b - a + 1 of the vector
 [[nodiscard]] std::size_t capacity( void ) const { return( f_A.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 std::size_t H( long k ) const {
  if( ( k < f_a ) || ( k - f_a >= long( f_A.size() ) ) )
   throw( std::out_of_range( "ADS::DirectTable: key out of [a, b]" ) );
  return( k - f_a );
  }

 long f_a;                              ///< the smallest key
 std::vector< std::optional< V > > f_A; ///< the vector, nil if empty
 std::size_t f_size = 0;                ///< the number of keys

 };  // end( class DirectTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* DirectTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File DirectTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
