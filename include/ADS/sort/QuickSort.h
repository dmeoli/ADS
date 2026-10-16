/*--------------------------------------------------------------------------*/
/*---------------------------- File QuickSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Quicksort (section 12.2): perno() rearranges A[ start .. end ] around
 * the pivot x = A[ start ], so that the items before its final place j are
 * smaller and the ones after it are not, and the two parts are sorted
 * recursively. With P( n ) = n - 1 comparisons per partition,
 * \f[
 *   T( n ) = n - 1 + \frac{2}{n} \sum_{j=1}^{n-1} T( j )
 *   \leq 2 ( n + 1 ) \sum_{k=3}^{n+1} \frac{1}{k} = O( n \log n )
 * \f]
 * on average over the permutations, but \f$ n ( n - 1 ) / 2 \f$ when the
 * pivot is always the smallest item, e.g., on a sorted vector, and also
 * when all the items are equal (exercise 12.4). Not stable.
 *
 * quickSortTuned() combines the improvements of the section: the pivot is
 * the median of A[ start ], A[ ( start + end ) / 2 ] and A[ end ]; the
 * partition of Hoare scans from both ends, swapping the pairs on the wrong
 * sides, so that equal items split evenly and the case of all equal items
 * takes O( n log n ); the parts of at most cutoff items are left to
 * insertionSort(); the recursion is an explicit stack that holds the
 * larger part while the smaller one is sorted, so that it never holds more
 * than \f$ \lfloor \log_2 n \rfloor \f$ parts. quickSort( L ) sorts a LIST
 * with pointers by moving its items into three lists, smaller, equal and
 * larger than the pivot (exercise 12.1).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see C. A. R. Hoare, Quicksort, The Computer Journal 5(1), 10-16, 1962,
 *      doi:10.1093/comjnl/5.1.10
 * \see R. Sedgewick, Implementing Quicksort programs, Communications of the
 *      ACM 21(10), 847-857, 1978, doi:10.1145/359619.359631
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

#ifndef __ADS_QuickSort
 #define __ADS_QuickSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.h"
#include "ADS/sort/InsertionSort.h"

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// perno(): the pivot A[ start ] goes to its place j, the smaller items
/// before it, the others after it; end - start comparisons
template< class T , class C = std::less<> >
std::size_t partition( std::vector< T > & A , std::size_t start ,
                       std::size_t end , C less = C() )
{
 const T x = A[ start ];
 std::size_t j = start;
 for( std::size_t i = start + 1 ; i <= end ; ++i )
  if( less( A[ i ] , x ) )
   std::swap( A[ i ] , A[ ++j ] );
 A[ start ] = A[ j ];
 A[ j ] = x;
 return( j );
 }

/// QuickSort() of A[ start .. end ], the book's
template< class T , class C = std::less<> >
void quickSort( std::vector< T > & A , std::size_t start , std::size_t end ,
                C less = C() )
{
 if( start < end ) {
  const std::size_t j = partition( A , start , end , less );
  if( j > start )
   quickSort( A , start , j - 1 , less );
  quickSort( A , j + 1 , end , less );
  }
 }

template< class T , class C = std::less<> >
void quickSort( std::vector< T > & A , C less = C() )
{
 if( A.size() > 1 )
  quickSort( A , 0 , A.size() - 1 , less );
 }

/// Quicksort with the improvements of section 12.2; the largest number of
/// parts on the stack goes to depth, if given
template< class T , class C = std::less<> >
void quickSortTuned( std::vector< T > & A , std::size_t cutoff = 16 ,
                     C less = C() , std::size_t * depth = nullptr )
{
 if( A.size() < 2 )
  return;
 cutoff = cutoff < 3 ? 3 : cutoff;
 std::vector< std::pair< std::size_t , std::size_t > > S;   // [ lo , hi ]
 std::size_t lo = 0 , hi = A.size() - 1;
 for( ; ; ) {
  if( hi - lo + 1 <= cutoff ) {
   insertionSort( A , lo , hi + 1 , less );
   if( S.empty() )
    return;
   std::tie( lo , hi ) = S.back();
   S.pop_back();
   continue;
   }
  // the three samples sorted, the median, the pivot, in A[ hi - 1 ]: A[ lo ]
  // and A[ hi - 1 ] are then the sentinels of the two scans
  const std::size_t mid = lo + ( hi - lo ) / 2;
  if( less( A[ mid ] , A[ lo ] ) ) std::swap( A[ mid ] , A[ lo ] );
  if( less( A[ hi ] , A[ lo ] ) ) std::swap( A[ hi ] , A[ lo ] );
  if( less( A[ hi ] , A[ mid ] ) ) std::swap( A[ hi ] , A[ mid ] );
  std::swap( A[ mid ] , A[ hi - 1 ] );
  const T x = A[ hi - 1 ];
  std::size_t i = lo , j = hi - 1;
  for( ; ; ) {                                // the partition of Hoare
   while( less( A[ ++i ] , x ) ) {}
   while( less( x , A[ --j ] ) ) {}
   if( i >= j )
    break;
   std::swap( A[ i ] , A[ j ] );
   }
  std::swap( A[ i ] , A[ hi - 1 ] );          // the pivot in its place
  j = i;
  // the larger part on the stack, the smaller one now
  std::pair< std::size_t , std::size_t > left( lo , j ? j - 1 : 0 ) ,
                                         right( j + 1 , hi );
  const bool hasLeft = j > lo , hasRight = j < hi;
  if( hasLeft && hasRight ) {
   if( left.second - left.first < right.second - right.first ) {
    S.push_back( right );
    std::tie( lo , hi ) = left;
    }
   else {
    S.push_back( left );
    std::tie( lo , hi ) = right;
    }
   }
  else if( hasLeft )
   std::tie( lo , hi ) = left;
  else if( hasRight )
   std::tie( lo , hi ) = right;
  else if( S.empty() )
   return;
  else {
   std::tie( lo , hi ) = S.back();
   S.pop_back();
   }
  if( depth && ( S.size() > *depth ) )
   *depth = S.size();
  }
 }

/// Quicksort of a list with pointers: the items go to three lists,
/// smaller, equal and larger than the first one, the first and the last
/// are sorted and the three are joined
template< class T , class C = std::less<> >
void quickSort( List< T > & L , C less = C() )
{
 if( L.isEmpty() || L.finished( L.next( L.head() ) ) )
  return;
 const T x = L.read( L.head() );
 List< T > S , E , G;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.remove( p ) ) {
  const T & y = L.read( p );
  List< T > & D = less( y , x ) ? S : less( x , y ) ? G : E;
  D.insert( D.next( D.tail() ) , y );
  }
 quickSort( S , less );
 quickSort( G , less );
 for( auto * D : { &S , &E , &G } )
  for( auto p = D->head() ; ! D->finished( p ) ; p = D->next( p ) )
   L.insert( L.next( L.tail() ) , D->read( p ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* QuickSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File QuickSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
