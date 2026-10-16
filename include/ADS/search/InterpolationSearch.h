/*--------------------------------------------------------------------------*/
/*----------------------- File InterpolationSearch.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search by interpolation of section 12.1 in a sorted vector of
 * numbers: in the part A[ i .. j ] the key v is looked for in the place
 * \f[
 *   m = i + \left\lfloor ( j - i ) \frac{v - A[ i ]}{A[ j ] - A[ i ]}
 *   \right\rfloor
 * \f]
 * instead of the middle one, as one opens a phone book near its start to
 * look for a name in C. With keys uniformly distributed it takes
 * \f$ O( \log \log n ) \f$ expected time, but O( n ) in the worst case, so
 * that with few keys or with keys not uniform the binary search is better.
 * The search stops when v is out of [ A[ i ] , A[ j ] ], and probes A[ i ]
 * when A[ i ] = A[ j ].
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see Y. Perl, A. Itai, H. Avni, Interpolation search: a log log N
 *      search, Communications of the ACM 21(7), 550-553, 1978,
 *      doi:10.1145/359545.359557
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

#ifndef __ADS_InterpolationSearch
 #define __ADS_InterpolationSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// ricercaInterpolata(): the place of v in the sorted A, or -1;
/// trace( i , j , m ) on each probe
template< class T , class F >
long interpolationSearch( const std::vector< T > & A , const T & v ,
                          F && trace )
{
 long i = 0 , j = long( A.size() ) - 1;
 while( ( i <= j ) && ! ( v < A[ i ] ) && ! ( A[ j ] < v ) ) {
  const long m = ( A[ j ] == A[ i ] ) ? i :
   i + long( ( double( j - i ) * double( v - A[ i ] ) ) /
             double( A[ j ] - A[ i ] ) );
  trace( i , j , m );
  if( A[ m ] == v )
   return( m );
  if( A[ m ] < v )
   i = m + 1;
  else
   j = m - 1;
  }
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

#endif  /* InterpolationSearch.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File InterpolationSearch.h ---------------------*/
/*--------------------------------------------------------------------------*/
