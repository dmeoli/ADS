/*--------------------------------------------------------------------------*/
/*------------------------- File BinaryCounter.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of BinaryCounter.h [see BinaryCounter.h].
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

inline std::size_t add( std::vector< int > & A )
{
 ADS_AUDIT( std::all_of( A.begin() , A.end() , []( int b ) {
  return( ( b == 0 ) || ( b == 1 ) ); } ) );
 const std::size_t k = A.size();
 std::size_t i = 0;
 while( ( i < k ) && ( A[ i ] == 1 ) ) {
  A[ i ] = 0;
  ++i;
  }
 if( i < k ) {
  A[ i ] = 1;
  ADS_POST( i + 1 <= k );
  return( i + 1 );
  }
 return( k );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BinaryCounter.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
