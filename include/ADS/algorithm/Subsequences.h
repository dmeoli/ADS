/*--------------------------------------------------------------------------*/
/*-------------------------- File Subsequences.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems on a sequence solved by the programming of chapter 13:
 *
 * - the longest increasing subsequence (exercise 13.6): L[ i ], the length
 *   of the longest one that ends in A[ i ], is 1 plus the largest L[ j ],
 *   j < i, with A[ j ] < A[ i ], O( n^2 ); lisFast() keeps, for each
 *   length l, the least item that ends an increasing subsequence of
 *   length l, an increasing vector, and puts each item in place by a
 *   binary search, O( n log n );
 * - the subvector of largest sum (exercises 12.2 and 13.12): t_i, the best
 *   sum of a subvector ending in A[ i ], is \f$ \max\{ t_{i-1} + A[ i ] , 0
 *   \} \f$, the empty subvector of sum 0 allowed, O( n ) (the scan of
 *   Kadane).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercises 13.6 and
 *      13.12, https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see M. L. Fredman, On computing the length of longest increasing
 *      subsequences, Discrete Mathematics 11(1), 29-35, 1975,
 *      doi:10.1016/0012-365X(75)90103-X
 * \see J. Bentley, Programming pearls: algorithm design techniques,
 *      Communications of the ACM 27(9), 865-873, 1984,
 *      doi:10.1145/358234.381162
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

#ifndef __ADS_Subsequences
 #define __ADS_Subsequences
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a longest increasing subsequence, O( n^2 )
template< class T >
std::vector< T > lis( const std::vector< T > & A )
{
 const std::size_t n = A.size();
 std::vector< std::size_t > L( n , 1 ) , prev( n , n );
 std::size_t best = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  for( std::size_t j = 0 ; j < i ; ++j )
   if( ( A[ j ] < A[ i ] ) && ( L[ j ] + 1 > L[ i ] ) ) {
    L[ i ] = L[ j ] + 1;
    prev[ i ] = j;
    }
  if( L[ i ] > L[ best ] )
   best = i;
  }
 std::vector< T > out;
 for( std::size_t i = n ? best : n ; i < n ; i = prev[ i ] )
  out.push_back( A[ i ] );
 std::reverse( out.begin() , out.end() );
 return( out );
 }

/// the length of a longest increasing subsequence, O( n log n )
template< class T >
std::size_t lisLength( const std::vector< T > & A )
{
 std::vector< T > tails;                 // tails[ l ]: the least end of l + 1
 for( const T & a : A ) {
  auto it = std::lower_bound( tails.begin() , tails.end() , a );
  if( it == tails.end() )
   tails.push_back( a );
  else
   *it = a;
  }
 return( tails.size() );
 }

/// the largest sum of a subvector, the empty one included, O( n )
template< class T >
T maxSubarraySum( const std::vector< T > & A )
{
 T best = T( 0 ) , t = T( 0 );
 for( const T & a : A ) {
  t = std::max( t + a , T( 0 ) );
  best = std::max( best , t );
  }
 return( best );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Subsequences.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Subsequences.h -------------------------*/
/*--------------------------------------------------------------------------*/
