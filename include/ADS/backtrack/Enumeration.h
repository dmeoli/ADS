/*--------------------------------------------------------------------------*/
/*--------------------------- File Enumeration.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The enumeration of the feasible solutions by backtrack (section 16.2):
 * a solution is a vector S[ 0 .. n - 1 ], each S[ i ] taken from the
 * choices allowed by S[ 0 .. i - 1 ]; each choice is tried, the solutions
 * are processed, and the search goes on recursively from S[ 0 .. i ], a
 * depth first visit of the tree of the choices. The processing returns
 * true to stop at the first solution.
 *
 * - subsets(), the subsets of k of the n items, by the choices in or out,
 *   pruned when k items are in or when the items left are too few;
 * - permutations() and derangements(), each item not chosen yet, the
 *   latter not in its own place (exercises 16.2 and 16.3);
 * - subsetSum(), the subsets of positive items of sum k, pruned when the
 *   sum exceeds k (exercise 16.4).
 *
 * Each calls f( S ) on the solutions, in increasing order of the choices,
 * and returns how many there are, or stops when f returns true. The
 * subsets of k items are \f$ \binom{n}{k} = n! / ( k! ( n - k )! ) \f$ (the
 * text writes n! / ( n - k )!, the number of their orderings).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.2 and
 *      exercises 16.2-16.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#ifndef __ADS_Enumeration
 #define __ADS_Enumeration
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the subsets of k of the items 0 , ... , n - 1, as vectors of n booleans;
/// f( S ) returns true to stop
template< class F >
long subsets( int n , int k , F && f )
{
 std::vector< bool > S( n , false );
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self , int i , int c ) -> void {
  if( stop ) return;
  if( c == k ) {                                // a feasible solution
   ++count;
   stop = f( S );
   return;
   }
  if( ( i == n ) || ( c + ( n - i ) < k ) )     // pruned
   return;
  S[ i ] = true;
  self( self , i + 1 , c + 1 );
  S[ i ] = false;
  self( self , i + 1 , c );
  };
 rec( rec , 0 , 0 );
 return( count );
 }

namespace detail {

template< class T , class F >
long permute( const std::vector< T > & A , bool derange , F && f )
{
 const std::size_t n = A.size();
 std::vector< T > S;
 std::vector< bool > used( n , false );
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self ) -> void {
  if( stop ) return;
  if( S.size() == n ) {
   ++count;
   stop = f( S );
   return;
   }
  for( std::size_t c = 0 ; c < n && ! stop ; ++c )
   if( ! used[ c ] && ! ( derange && ( c == S.size() ) ) ) {
    used[ c ] = true;                         // A.remove( c )
    S.push_back( A[ c ] );
    self( self );
    S.pop_back();
    used[ c ] = false;                        // A.insert( c )
    }
  };
 rec( rec );
 return( count );
 }

}  // end( namespace detail )

/// the permutations of the items, f( S ) on each
template< class T , class F >
long permutations( const std::vector< T > & A , F && f )
{
 return( detail::permute( A , false , f ) );
 }

/// the derangements, no item in its own place, f( S ) on each
template< class T , class F >
long derangements( const std::vector< T > & A , F && f )
{
 return( detail::permute( A , true , f ) );
 }

/// the subsets of the positive items of sum k, as the places taken
template< class T , class F >
long subsetSum( const std::vector< T > & A , T k , F && f )
{
 std::vector< std::size_t > S;
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self , std::size_t i , T sum ) -> void {
  if( stop ) return;
  if( sum == k ) {
   ++count;
   stop = f( S );
   return;
   }
  for( std::size_t j = i ; j < A.size() && ! stop ; ++j )
   if( sum + A[ j ] <= k ) {                 // pruned when it exceeds k
    S.push_back( j );
    self( self , j + 1 , sum + A[ j ] );
    S.pop_back();
    }
  };
 rec( rec , 0 , T( 0 ) );
 return( count );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Enumeration.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Enumeration.h -------------------------*/
/*--------------------------------------------------------------------------*/
