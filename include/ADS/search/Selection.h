/*--------------------------------------------------------------------------*/
/*---------------------------- File Selection.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The selection of section 17.1: the item of rank k, the k-th smallest,
 * 1 <= k <= n, the median for k = ceil( n / 2 ), without sorting.
 *
 * - selection(), of Hoare: perno() puts its pivot in its place j, the
 *   q = j - start + 1 smallest items before it, and the search goes on in
 *   the part that holds rank k; with the pivot chosen at random, swapped
 *   to A[ start ] before perno(), the expected number of comparisons
 *   satisfies \f$ T( n ) \leq n + \frac{2}{n} \sum_{q = \lfloor n / 2
 *   \rfloor}^{n-1} T( q ) \leq c n \f$, linear for every input;
 * - medianOfMedians(), of Blum, Floyd, Pratt, Rivest and Tarjan, linear in
 *   the worst case: the pivot is the median of the medians of the groups of
 *   5, which has at least 3 n / 10 items on each side, so that
 *   \f$ T( n ) \leq T( n / 5 ) + T( 7 n / 10 ) + O( n ) = O( n ) \f$.
 *
 * Both rearrange A.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see C. A. R. Hoare, Algorithm 65: Find, Communications of the ACM 4(7),
 *      321-322, 1961, doi:10.1145/366622.366647
 * \see M. Blum, R. W. Floyd, V. Pratt, R. L. Rivest, R. E. Tarjan, Time
 *      bounds for selection, Journal of Computer and System Sciences 7(4),
 *      448-461, 1973, doi:10.1016/S0022-0000(73)80033-9
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

#ifndef __ADS_Selection
 #define __ADS_Selection
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/QuickSort.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// selezione(): the item of rank k, 1 <= k <= n, of A; with a generator
/// the pivots are random, otherwise the first item of the part
template< class T , class R = std::mt19937 >
T selection( std::vector< T > & A , std::size_t k , R * rng = nullptr )
{
 if( ( k < 1 ) || ( k > A.size() ) )
  throw( std::invalid_argument( "ADS::selection: k not in [1, n]" ) );
 std::size_t start = 0 , end = A.size() - 1;
 for( ; ; ) {                                   // the tail recursion
  if( start == end )
   return( A[ start ] );
  if( rng ) {
   std::uniform_int_distribution< std::size_t > pick( start , end );
   std::swap( A[ pick( *rng ) ] , A[ start ] );
   }
  const std::size_t j = partition( A , start , end );
  const std::size_t q = j - start + 1;
  if( k == q )
   return( A[ j ] );
  if( k < q )
   end = j - 1;
  else {
   start = j + 1;
   k -= q;
   }
  }
 }

/// the item of rank k, 1 <= k <= n, in linear time in the worst case
template< class T >
T medianOfMedians( std::vector< T > & A , std::size_t k )
{
 if( ( k < 1 ) || ( k > A.size() ) )
  throw( std::invalid_argument( "ADS::medianOfMedians: k not in [1, n]" ) );
 auto rec = [ & ]( auto & self , std::size_t lo , std::size_t hi ,
                   std::size_t r ) -> T {     // rank r in A[ lo .. hi ]
  for( ; ; ) {
   const std::size_t n = hi - lo + 1;
   if( n <= 10 ) {
    insertionSort( A , lo , hi + 1 );
    return( A[ lo + r - 1 ] );
    }
   // the medians of the groups of 5 to the front
   std::size_t m = lo;
   for( std::size_t g = lo ; g + 4 <= hi ; g += 5 ) {
    insertionSort( A , g , g + 5 );
    std::swap( A[ m++ ] , A[ g + 2 ] );
    }
   const T pivot = self( self , lo , m - 1 , ( m - lo + 1 ) / 2 );
   // three ways: smaller, equal, larger
   std::size_t lt = lo , i = lo , gt = hi;
   while( i <= gt )
    if( A[ i ] < pivot ) std::swap( A[ lt++ ] , A[ i++ ] );
    else if( pivot < A[ i ] ) std::swap( A[ i ] , A[ gt-- ] );
    else ++i;
   if( r <= lt - lo )
    hi = lt - 1;
   else if( r <= i - lo )
    return( pivot );
   else {
    r -= i - lo;
    lo = i;
    }
   }
  };
 return( rec( rec , 0 , A.size() - 1 , k ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Selection.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Selection.h --------------------------*/
/*--------------------------------------------------------------------------*/
