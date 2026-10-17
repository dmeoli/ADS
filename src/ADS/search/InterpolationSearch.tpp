/*--------------------------------------------------------------------------*/
/*---------------------- File InterpolationSearch.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of InterpolationSearch.h [see
 * InterpolationSearch.h], with their contracts.
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

template< class T , class F >
long interpolationSearch( const std::vector< T > & A , const T & v ,
                          F && trace )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 long i = 0 , j = long( A.size() ) - 1;
 while( ( i <= j ) && ! ( v < A[ i ] ) && ! ( A[ j ] < v ) ) {
  // the differences in double, since v - A[ i ] may overflow T, and m
  // clamped to [ i , j ] against the rounding
  const long m = ( A[ j ] == A[ i ] ) ? i : std::clamp( i + long(
   ( double( j - i ) * ( double( v ) - double( A[ i ] ) ) ) /
   ( double( A[ j ] ) - double( A[ i ] ) ) ) , i , j );
  trace( i , j , m );
  if( A[ m ] == v )
   return( m );
  if( A[ m ] < v )
   i = m + 1;
  else
   j = m - 1;
  }
 ADS_AUDIT( std::find( A.begin() , A.end() , v ) == A.end() );
 return( -1 );
 }

template< class T >
long interpolationSearch( const std::vector< T > & A , const T & v )
{
 return( interpolationSearch( A , v , []( long , long , long ) {} ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File InterpolationSearch.tpp --------------------*/
/*--------------------------------------------------------------------------*/
