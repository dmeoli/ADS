/*--------------------------------------------------------------------------*/
/*------------------------- File StringMatching.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact string matching, beyond the book: the occurrences of a
 * pattern P of m characters in a text T of n characters.
 *
 * - naiveMatching() tries each shift of P along T, O( n m );
 * - kmp(), the algorithm of Knuth, Morris and Pratt, first computes the
 *   failure function of P, the length of the longest proper border of each
 *   prefix (a border is both a prefix and a suffix), and then scans T once,
 *   shifting P by its failure on a mismatch, so that no character of T is
 *   compared again after a match, \f$ \Theta( n + m ) \f$.
 *
 * The empty pattern occurs at every place 0 , ... , n.
 *
 * \see D. E. Knuth, J. H. Morris, V. R. Pratt, Fast pattern matching in
 *      strings, SIAM Journal on Computing 6(2), 323-350, 1977,
 *      doi:10.1137/0206024
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

#ifndef __ADS_StringMatching
 #define __ADS_StringMatching
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the places of all the occurrences of P in T, by trying every shift
inline std::vector< std::size_t > naiveMatching( std::string_view T ,
                                                 std::string_view P )
{
 std::vector< std::size_t > R;
 for( std::size_t s = 0 ; s + P.size() <= T.size() ; ++s )
  if( T.compare( s , P.size() , P ) == 0 ) R.push_back( s );
 return( R );
 }

/// the failure function: F[ q ] is the length of the longest proper
/// border of P[ 0 .. q - 1 ], F[ 0 ] = -1
inline std::vector< long > failure( std::string_view P )
{
 std::vector< long > F( P.size() + 1 );
 F[ 0 ] = -1;
 long k = -1;
 for( std::size_t q = 0 ; q < P.size() ; ++q ) {
  while( ( k >= 0 ) && ( P[ k ] != P[ q ] ) ) k = F[ k ];
  F[ q + 1 ] = ++k;
  }
 return( F );
 }

/// the places of all the occurrences of P in T, by Knuth-Morris-Pratt;
/// the comparisons of characters go to *comparisons, if given
inline std::vector< std::size_t > kmp( std::string_view T ,
                                       std::string_view P ,
                                       long * comparisons = nullptr )
{
 std::vector< std::size_t > R;
 const auto F = failure( P );
 const long m = long( P.size() );
 long q = 0;                            // the characters of P matched
 if( m == 0 ) {
  for( std::size_t i = 0 ; i <= T.size() ; ++i ) R.push_back( i );
  return( R );
  }
 for( std::size_t i = 0 ; i < T.size() ; ++i ) {
  for( ; ; ) {
   if( comparisons ) ++*comparisons;
   if( P[ q ] == T[ i ] ) break;
   q = F[ q ];
   if( q < 0 ) break;
   }
  if( ++q == m ) {
   R.push_back( i + 1 - m );
   q = F[ m ];
   }
  }
 return( R );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* StringMatching.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StringMatching.h ------------------------*/
/*--------------------------------------------------------------------------*/
