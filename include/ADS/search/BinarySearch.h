/*--------------------------------------------------------------------------*/
/*-------------------------- File BinarySearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search of section 1.2 in a sorted vector A[ 0 .. n - 1 ]: v
 * is compared with the middle item A[ m ] of the part A[ i .. j ] still to
 * examine, and the search goes on in A[ i .. m - 1 ] if v < A[ m ] and in
 * A[ m + 1 .. j ] if v > A[ m ]; the part halves at each call, so that at
 * most \f$ \lfloor \log_2 n \rfloor + 1 \f$ calls are made, 2 comparisons
 * each. The middle is computed as \f$ m = i + \lfloor ( j - i ) / 2 \rfloor
 * \f$, which unlike \f$ \lfloor ( i + j ) / 2 \rfloor \f$ cannot overflow
 * (the bug of Arrays.binarySearch of Java until 2003). The search may
 * report each call (i, j, m) to a function, as the table of the example.
 *
 * lowerBound() and upperBound() give the first place of an item not
 * smaller, and larger, than v, with \f$ \lceil \log_2 ( n + 1 ) \rceil \f$
 * comparisons: their difference is the number of occurrences of v
 * (exercise 12.13). gallopSearch() looks for the first place of an item
 * not smaller than v by doubling a bound before the binary search, in
 * \f$ O( \log h ) \f$ time if that place is h (exercise 12.9).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 1.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
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

#ifndef __ADS_BinarySearch
 #define __ADS_BinarySearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// binarySearch(): the place of v in the sorted A, or -1; trace( i , j ,
/// m ) on each call that examines A[ m ]
template< class T , class F >
long binarySearch( const std::vector< T > & A , const T & v , F && trace )
{
 long i = 0 , j = long( A.size() ) - 1;
 while( i <= j ) {                          // the tail recursion of bsRec()
  const long m = i + ( j - i ) / 2;
  trace( i , j , m );
  if( A[ m ] == v )
   return( m );
  if( A[ m ] < v )
   i = m + 1;                               // the right part
  else
   j = m - 1;                               // the left part
  }
 return( -1 );                              // an empty part
 }

/// binarySearch() with no trace
template< class T >
long binarySearch( const std::vector< T > & A , const T & v )
{
 return( binarySearch( A , v , []( long , long , long ) {} ) );
 }

/// the first place in [ lo , hi ) of an item not smaller than v, hi if none
template< class T , class C = std::less<> >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        std::size_t lo , std::size_t hi , C less = C() )
{
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( less( A[ m ] , v ) )
   lo = m + 1;
  else
   hi = m;
  }
 return( lo );
 }

template< class T , class C = std::less<> >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        C less = C() )
{
 return( lowerBound( A , v , 0 , A.size() , less ) );
 }

/// the first place of an item larger than v, n if none
template< class T , class C = std::less<> >
std::size_t upperBound( const std::vector< T > & A , const T & v ,
                        C less = C() )
{
 std::size_t lo = 0 , hi = A.size();
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( less( v , A[ m ] ) )
   hi = m;
  else
   lo = m + 1;
  }
 return( lo );
 }

/// the first place h of an item not smaller than v, found by looking at
/// the places 0 , 1 , 3 , 7 , ... and then by a binary search, O( log h )
template< class T , class C = std::less<> >
std::size_t gallopSearch( const std::vector< T > & A , const T & v ,
                          C less = C() )
{
 std::size_t lo = 0 , step = 1 , hi = 0;
 while( ( hi < A.size() ) && less( A[ hi ] , v ) ) {
  lo = hi + 1;
  hi += step;
  step *= 2;
  }
 return( lowerBound( A , v , lo , std::min( hi + 1 , A.size() ) , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* BinarySearch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinarySearch.h -------------------------*/
/*--------------------------------------------------------------------------*/
