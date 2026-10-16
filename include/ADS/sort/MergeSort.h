/*--------------------------------------------------------------------------*/
/*---------------------------- File MergeSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Merge Sort (section 2.7.3): the two halves of the vector are sorted
 * recursively and merged by Merge() through a vector B allocated once,
 * \f$ T( n ) = 2 T( n / 2 ) + \Theta( n ) = \Theta( n \log n ) \f$ time,
 * optimal among the sorts by comparisons, \f$ \Theta( n ) \f$ extra space,
 * stable (an item of the left part goes first when equal). When the right
 * part runs out first, the rest of the left one is copied; when the left
 * one does, the rest of the right one is already in place.
 *
 * - mergeSort( A , cutoff ) sorts the parts of at most cutoff items with
 *   insertionSort(), which is faster on few items (section 2.8);
 * - mergeSortBottomUp() merges the runs of length 1 , 2 , 4 , ... with no
 *   recursion and no stack (exercise 12.5);
 * - mergeSort( L ) sorts a LIST with pointers in O( n log n ) by splitting
 *   it in two halves, sorting them and merging their cells, with no copy of
 *   the items (exercise 12.1);
 * - countInversions() counts the pairs i < j with A[ i ] > A[ j ]: when
 *   the merge takes A[ j ] from the right part, it is smaller than all the
 *   mid - i + 1 items left in the left part (exercise 12.3).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 5.2.4
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

#ifndef __ADS_MergeSort
 #define __ADS_MergeSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.h"
#include "ADS/sort/InsertionSort.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// Merge() of the book: A[ start .. mid ] and A[ mid + 1 .. end ], sorted,
/// merged through B; the inversions met are added to inv
template< class T , class C = std::less<> >
void mergeParts( std::vector< T > & A , std::vector< T > & B ,
                 std::size_t start , std::size_t mid , std::size_t end ,
                 C less = C() , long * inv = nullptr )
{
 std::size_t i = start , j = mid + 1 , k = start;
 while( ( i <= mid ) && ( j <= end ) )
  if( ! less( A[ j ] , A[ i ] ) )              // A[ i ] <= A[ j ]
   B[ k++ ] = std::move( A[ i++ ] );
  else {
   if( inv )
    *inv += long( mid - i + 1 );
   B[ k++ ] = std::move( A[ j++ ] );
   }
 while( i <= mid )                              // the rest of the left part
  B[ k++ ] = std::move( A[ i++ ] );
 for( std::size_t t = start ; t < k ; ++t )
  A[ t ] = std::move( B[ t ] );
 }

/// MergeSortRec() of A[ start .. end ], insertionSort() below cutoff items
template< class T , class C = std::less<> >
void mergeSortRec( std::vector< T > & A , std::vector< T > & B ,
                   std::size_t start , std::size_t end , std::size_t cutoff ,
                   C less = C() , long * inv = nullptr )
{
 if( end - start + 1 <= cutoff ) {
  insertionSort( A , start , end + 1 , less );
  return;
  }
 if( start < end ) {
  const std::size_t mid = start + ( end - start ) / 2;
  mergeSortRec( A , B , start , mid , cutoff , less , inv );
  mergeSortRec( A , B , mid + 1 , end , cutoff , less , inv );
  mergeParts( A , B , start , mid , end , less , inv );
  }
 }

/// MergeSort(): A in increasing order, stable; parts of at most cutoff
/// items by insertionSort() (1 is the book)
template< class T , class C = std::less<> >
void mergeSort( std::vector< T > & A , std::size_t cutoff = 1 ,
                C less = C() )
{
 if( A.size() < 2 )
  return;
 std::vector< T > B( A.size() );
 mergeSortRec( A , B , 0 , A.size() - 1 ,
               std::max< std::size_t >( cutoff , 1 ) , less );
 }

/// the iterative Merge Sort: the runs of length w are merged in pairs, for
/// w = 1 , 2 , 4 , ...
template< class T , class C = std::less<> >
void mergeSortBottomUp( std::vector< T > & A , C less = C() )
{
 const std::size_t n = A.size();
 std::vector< T > B( n );
 for( std::size_t w = 1 ; w < n ; w *= 2 )
  for( std::size_t start = 0 ; start + w < n ; start += 2 * w )
   mergeParts( A , B , start , start + w - 1 ,
               std::min( start + 2 * w , n ) - 1 , less );
 }

/// the number of inversions of A, which ends sorted, O( n log n )
template< class T , class C = std::less<> >
long countInversions( std::vector< T > & A , C less = C() )
{
 long inv = 0;
 if( A.size() > 1 ) {
  std::vector< T > B( A.size() );
  mergeSortRec( A , B , 0 , A.size() - 1 , 1 , less , &inv );
  }
 return( inv );
 }

/// Merge Sort of a list with pointers: the cells are moved, not the items
template< class T , class C = std::less<> >
void mergeSort( List< T > & L , C less = C() )
{
 // split: the second half moves to R
 std::size_t n = 0;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  ++n;
 if( n < 2 )
  return;
 List< T > R;
 auto p = L.head();
 for( std::size_t i = 0 ; i < n / 2 ; ++i )
  p = L.next( p );
 while( ! L.finished( p ) ) {
  R.insert( R.next( R.tail() ) , L.read( p ) );
  p = L.remove( p );
  }
 mergeSort( L , less );
 mergeSort( R , less );
 // merge: each item of R goes before the first larger item of L
 auto q = L.head();
 for( auto r = R.head() ; ! R.finished( r ) ; r = R.next( r ) ) {
  while( ! L.finished( q ) && ! less( R.read( r ) , L.read( q ) ) )
   q = L.next( q );
  L.insert( q , R.read( r ) );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MergeSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MergeSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
