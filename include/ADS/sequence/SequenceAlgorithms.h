/*--------------------------------------------------------------------------*/
/*----------------------- File SequenceAlgorithms.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms on a SEQUENCE [see ADT.h], beyond the book, through its
 * positions only, so that they work on a List and on an ArraySequence:
 *
 * - search(), the place of an item counted from 0, or -( n + 1 ) after n
 *   comparisons, and searchOrdered() on a sorted sequence, which stops at
 *   the first larger item and gives -( comparisons ) when it misses;
 * - order(), which tells whether a sequence is empty, constant, strictly
 *   or weakly increasing or decreasing, or none of them;
 * - bubbleSort(), \f$ O( n^2 ) \f$ swaps of adjacent items by read() and
 *   write();
 * - naturalMergeSort(), the natural merge sort: the maximal sorted runs
 *   are distributed alternately to two sequences and merged in pairs back,
 *   until a single run is left, in \f$ O( n \log r ) \f$ time for r runs
 *   and O( n ) on a sorted sequence;
 * - mergeSorted(), the merge of two sorted sequences that keeps the
 *   repeated items, and removeDuplicates(), which keeps the first copy of
 *   each item in \f$ O( n^2 ) \f$.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 3.2.1 and
 *      12.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
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

#ifndef __ADS_SequenceAlgorithms
 #define __ADS_SequenceAlgorithms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"

#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------- SEARCH -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the place of v counted from 0, or -( n + 1 )
template< Sequence S >
long search( const S & s , const typename S::Item & v )
{
 long i = 0;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) , ++i )
  if( s.read( p ) == v ) return( i );
 return( -( i + 1 ) );
 }

/// the place of v in the sorted s, or -( the comparisons made )
template< Sequence S >
long searchOrdered( const S & s , const typename S::Item & v )
{
 long i = 0;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) ) {
  ++i;
  if( s.read( p ) == v ) return( i - 1 );
  if( v < s.read( p ) ) break;
  }
 return( -i );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- ORDER ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the order of a sequence, the most specific that holds
enum class Order { EMPTY , CONSTANT , INCREASING , DECREASING ,
                   NONDECREASING , NONINCREASING , NONE };

template< Sequence S >
Order order( const S & s )
{
 if( s.isEmpty() ) return( Order::EMPTY );
 bool up = false , down = false , equal = false;
 for( auto p = s.head() , q = s.next( p ) ; ! s.finished( q ) ;
      p = q , q = s.next( q ) ) {
  if( s.read( p ) < s.read( q ) ) up = true;
  else if( s.read( q ) < s.read( p ) ) down = true;
  else equal = true;
  }
 if( up && down ) return( Order::NONE );
 if( ! up && ! down ) return( Order::CONSTANT );
 if( up ) return( equal ? Order::NONDECREASING : Order::INCREASING );
 return( equal ? Order::NONINCREASING : Order::DECREASING );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- SORTING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// bubble sort: n passes of swaps of adjacent items out of order, stopped
/// at the first pass without swaps
template< Sequence S >
void bubbleSort( S & s )
{
 for( bool swapped = true ; swapped ; ) {
  swapped = false;
  if( s.isEmpty() ) return;
  for( auto p = s.head() , q = s.next( p ) ; ! s.finished( q ) ;
       p = q , q = s.next( q ) )
   if( s.read( q ) < s.read( p ) ) {
    const auto x = s.read( p );
    s.write( p , s.read( q ) );
    s.write( q , x );
    swapped = true;
    }
  }
 }

namespace detail {

template< Sequence S >
void append( S & s , const typename S::Item & v )
{
 s.insert( s.next( s.tail() ) , v );
 }

/// the run of s from p to the end of a sorted stretch appended to d; the
/// position after it
template< Sequence S >
typename S::Pos copyRun( const S & s , typename S::Pos p , S & d )
{
 for( ; ; ) {
  append( d , s.read( p ) );
  const auto q = s.next( p );
  if( s.finished( q ) || ( s.read( q ) < s.read( p ) ) ) return( q );
  p = q;
  }
 }

}  // end( namespace detail )

/// the merge of the sorted a and b, the repeated items kept, those of a
/// first among equals
template< Sequence S >
S mergeSorted( const S & a , const S & b )
{
 S c;
 auto p = a.head();
 auto q = b.head();
 while( ! a.finished( p ) && ! b.finished( q ) )
  if( b.read( q ) < a.read( p ) ) {
   detail::append( c , b.read( q ) );
   q = b.next( q );
   }
  else {
   detail::append( c , a.read( p ) );
   p = a.next( p );
   }
 for( ; ! a.finished( p ) ; p = a.next( p ) )
  detail::append( c , a.read( p ) );
 for( ; ! b.finished( q ) ; q = b.next( q ) )
  detail::append( c , b.read( q ) );
 return( c );
 }

/// the natural merge sort; the number of passes goes to *passes, if given
template< Sequence S >
void naturalMergeSort( S & s , long * passes = nullptr )
{
 if( passes ) *passes = 0;
 for( ; ; ) {
  S a , b;                               // distribute
  long runs = 0;
  for( auto p = s.head() ; ! s.finished( p ) ; ++runs )
   p = detail::copyRun( s , p , runs % 2 ? b : a );
  if( runs <= 1 ) return;
  if( passes ) ++*passes;
  S m;                                   // merge the runs in pairs
  auto p = a.head();
  auto q = b.head();
  while( ! a.finished( p ) || ! b.finished( q ) ) {
   S x , y;
   if( ! a.finished( p ) ) p = detail::copyRun( a , p , x );
   if( ! b.finished( q ) ) q = detail::copyRun( b , q , y );
   const S z = mergeSorted( x , y );
   for( auto r = z.head() ; ! z.finished( r ) ; r = z.next( r ) )
    detail::append( m , z.read( r ) );
   }
  s = std::move( m );
  }
 }

/// keeps the first copy of each item, in O( n^2 ) comparisons
template< Sequence S >
void removeDuplicates( S & s )
{
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  for( auto q = s.next( p ) ; ! s.finished( q ) ; )
   if( s.read( q ) == s.read( p ) ) q = s.remove( q );
   else q = s.next( q );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* SequenceAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SequenceAlgorithms.h ----------------------*/
/*--------------------------------------------------------------------------*/
