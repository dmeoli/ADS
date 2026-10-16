/*--------------------------------------------------------------------------*/
/*---------------------------- File ListMFSet.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET on lists of section 10.2.1 [see MFSet.h], each element knowing
 * the head of its list: find() in O( 1 ), merge() relabels the shorter
 * list, so that an element is relabeled at most \f$ \log_2 n \f$ times.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. E. Hopcroft, J. D. Ullman, Set merging algorithms, SIAM Journal
 *      on Computing 2(4), 294-303, 1973, doi:10.1137/0202024
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

#ifndef __ADS_ListMFSet
 #define __ADS_ListMFSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ListMFSet -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets on lists, each element with the head of its
/// list
class ListMFSet {

/*--------------------------------------------------------------------------*/
 public:

 explicit ListMFSet( int n ) : f_head( n ) , f_list( n ) {
  if( n < 0 )
   throw( std::invalid_argument( "ADS::ListMFSet: n < 0" ) );
  for( int i = 0 ; i < n ; ++i ) {
   f_head[ i ] = i;
   f_list[ i ] = { i };
   }
  }

 [[nodiscard]] int n( void ) const { return( int( f_head.size() ) ); }

 /// O( 1 ) time
 [[nodiscard]] int find( int x ) const { return( f_head.at( x ) ); }

 /// the shorter list goes after the longer one, its elements relabeled
 bool merge( int x , int y ) {
  int a = find( x ) , b = find( y );
  if( a == b )
   return( false );
  if( f_list[ a ].size() < f_list[ b ].size() )
   std::swap( a , b );
  for( int z : f_list[ b ] ) {
   f_head[ z ] = a;
   f_list[ a ].push_back( z );
   ++f_relabels;
   }
  f_list[ b ].clear();
  return( true );
  }

 /// the elements relabeled so far
 [[nodiscard]] long relabels( void ) const { return( f_relabels ); }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< int > f_head;                 ///< the head of each list
 std::vector< std::vector< int > > f_list;  ///< the list of each head
 long f_relabels = 0;                       ///< see relabels()

 };  // end( class ListMFSet )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ListMFSet.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ListMFSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
