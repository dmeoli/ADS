/*--------------------------------------------------------------------------*/
/*---------------------------- File ShellSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Shell Sort (section 15.1), a local search that lowers the number of
 * inversions: an insertion sort of the items h apart, for decreasing gaps
 * h down to 1, so that the far items move first and the last pass, the
 * insertion sort, finds the vector almost sorted. In place, iterative, not
 * stable; its time depends on the gaps:
 *
 * - knuthGaps(), the h = ( 3^i - 1 ) / 2 = 1 , 4 , 13 , 40 , 121 , ... of
 *   the book, \f$ O( n^{1.5} ) \f$ in the worst case;
 * - shellGaps(), the n / 2 , n / 4 , ... , 1 of Shell, \f$ \Theta( n^2 ) \f$
 *   in the worst case;
 * - prattGaps(), all the 3^i 2^j up to n, \f$ O( n \log^2 n ) \f$ in the
 *   worst case;
 * - ciuraGaps(), 1 , 4 , 10 , 23 , 57 , 132 , 301 , 701 and then
 *   \f$ \times 2.25 \f$, found by experiment and among the fastest in
 *   practice.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see D. L. Shell, A high-speed sorting procedure, Communications of the
 *      ACM 2(7), 30-32, 1959, doi:10.1145/368370.368387
 * \see V. R. Pratt, Shellsort and sorting networks, PhD thesis, Stanford
 *      University, 1971
 * \see M. Ciura, Best increments for the average case of Shellsort, FCT
 *      2001, LNCS 2138, 106-117, doi:10.1007/3-540-44669-9_12
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

#ifndef __ADS_ShellSort
 #define __ADS_ShellSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the gaps 1 , 4 , 13 , 40 , ... below n, increasing
inline std::vector< std::size_t > knuthGaps( std::size_t n )
{
 std::vector< std::size_t > h = { 1 };
 while( 3 * h.back() + 1 < n )
  h.push_back( 3 * h.back() + 1 );
 return( h );
 }

/// the gaps 1 , ... , n / 4 , n / 2 of Shell, increasing
inline std::vector< std::size_t > shellGaps( std::size_t n )
{
 std::vector< std::size_t > h;
 for( std::size_t g = n / 2 ; g > 0 ; g /= 2 )
  h.insert( h.begin() , g );
 if( h.empty() )
  h.push_back( 1 );
 return( h );
 }

/// the gaps 3^i 2^j up to n, increasing
inline std::vector< std::size_t > prattGaps( std::size_t n )
{
 std::vector< std::size_t > h;
 for( std::size_t p = 1 ; p <= std::max< std::size_t >( n , 1 ) ; p *= 3 )
  for( std::size_t q = p ; q <= std::max< std::size_t >( n , 1 ) ; q *= 2 )
   h.push_back( q );
 std::sort( h.begin() , h.end() );
 return( h );
 }

/// the gaps of Ciura below n, increasing
inline std::vector< std::size_t > ciuraGaps( std::size_t n )
{
 std::vector< std::size_t > h = { 1 , 4 , 10 , 23 , 57 , 132 , 301 , 701 };
 while( h.back() < n )
  h.push_back( std::size_t( double( h.back() ) * 2.25 ) );
 while( ( h.size() > 1 ) && ( h.back() >= n ) )
  h.pop_back();
 return( h );
 }

/// shellSort() with the given gaps, increasing and starting from 1
template< class T , class C = std::less<> >
void shellSort( std::vector< T > & A ,
                const std::vector< std::size_t > & gaps , C less = C() )
{
 const std::size_t n = A.size();
 for( std::size_t g = gaps.size() ; g-- > 0 ; ) {
  const std::size_t h = gaps[ g ];
  for( std::size_t i = h ; i < n ; ++i ) {
   T temp = std::move( A[ i ] );
   std::size_t j = i;
   while( ( j >= h ) && less( temp , A[ j - h ] ) ) {
    A[ j ] = std::move( A[ j - h ] );
    j -= h;
    }
   A[ j ] = std::move( temp );
   }
  }
 }

/// shellSort() of the book, the gaps 1 , 4 , 13 , ...
template< class T , class C = std::less<> >
void shellSort( std::vector< T > & A , C less = C() )
{
 shellSort( A , knuthGaps( A.size() ) , less );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ShellSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ShellSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
