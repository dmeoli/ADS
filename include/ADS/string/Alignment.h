/*--------------------------------------------------------------------------*/
/*---------------------------- File Alignment.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The alignments of two strings P and T of exercises 13.7-13.9: spaces are
 * inserted in P or T, never one in front of the other, and each column
 * scores +1 if the characters are equal, -1 if they differ and -2 if one
 * is a space. DP[ i ][ j ], the best score of P( i ) and T( j ), is
 * \f[
 *   \max\{ DP[ i ][ j - 1 ] - 2 , DP[ i - 1 ][ j ] - 2 ,
 *          DP[ i - 1 ][ j - 1 ] + \delta_{ij} \}
 * \f]
 * with \f$ \delta_{ij} = \pm 1 \f$, Theta( m n ) time:
 *
 * - global, DP[ i ][ 0 ] = -2 i and DP[ 0 ][ j ] = -2 j, the score in
 *   DP[ m ][ n ];
 * - semiglobal, the spaces before the start and after the end of either
 *   string free: the first row and column are 0, the score the largest of
 *   the last row and column;
 * - local, the best alignment of two substrings: 0 is a fourth term of the
 *   maximum, and the score the largest entry.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercises 13.7-13.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see S. B. Needleman, C. D. Wunsch, A general method applicable to the
 *      search for similarities in the amino acid sequence of two proteins,
 *      Journal of Molecular Biology 48(3), 443-453, 1970,
 *      doi:10.1016/0022-2836(70)90057-4
 * \see T. F. Smith, M. S. Waterman, Identification of common molecular
 *      subsequences, Journal of Molecular Biology 147(1), 195-197, 1981,
 *      doi:10.1016/0022-2836(81)90087-5
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

#ifndef __ADS_Alignment
 #define __ADS_Alignment
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the kind of alignment
enum class Alignment { GLOBAL , SEMIGLOBAL , LOCAL };

/// the best score of an alignment of P and T
template< class S >
int alignmentScore( const S & P , const S & T , Alignment kind )
{
 const std::size_t m = P.size() , n = T.size();
 const bool global = kind == Alignment::GLOBAL;
 std::vector< std::vector< int > > DP( m + 1 , std::vector< int >( n + 1 ) );
 for( std::size_t i = 0 ; i <= m ; ++i )
  DP[ i ][ 0 ] = global ? -2 * int( i ) : 0;
 for( std::size_t j = 0 ; j <= n ; ++j )
  DP[ 0 ][ j ] = global ? -2 * int( j ) : 0;
 int best = 0;
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j ) {
   const int d = P[ i - 1 ] == T[ j - 1 ] ? 1 : -1;
   int v = std::max( { DP[ i ][ j - 1 ] - 2 , DP[ i - 1 ][ j ] - 2 ,
                       DP[ i - 1 ][ j - 1 ] + d } );
   if( kind == Alignment::LOCAL )
    v = std::max( v , 0 );
   DP[ i ][ j ] = v;
   best = std::max( best , v );
   }
 if( global )
  return( DP[ m ][ n ] );
 if( kind == Alignment::LOCAL )
  return( best );
 int s = DP[ m ][ n ];                      // the last row and column
 for( std::size_t j = 0 ; j <= n ; ++j ) s = std::max( s , DP[ m ][ j ] );
 for( std::size_t i = 0 ; i <= m ; ++i ) s = std::max( s , DP[ i ][ n ] );
 return( s );
 }

/// a best global alignment, the two strings with the spaces as gap
template< class S >
std::pair< S , S > globalAlignment( const S & P , const S & T ,
                                    typename S::value_type gap = ' ' )
{
 const std::size_t m = P.size() , n = T.size();
 std::vector< std::vector< int > > DP( m + 1 , std::vector< int >( n + 1 ) );
 for( std::size_t i = 0 ; i <= m ; ++i ) DP[ i ][ 0 ] = -2 * int( i );
 for( std::size_t j = 0 ; j <= n ; ++j ) DP[ 0 ][ j ] = -2 * int( j );
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ i ][ j ] = std::max( { DP[ i ][ j - 1 ] - 2 , DP[ i - 1 ][ j ] - 2 ,
                              DP[ i - 1 ][ j - 1 ] +
                              ( P[ i - 1 ] == T[ j - 1 ] ? 1 : -1 ) } );
 S a , b;
 for( std::size_t i = m , j = n ; ( i > 0 ) || ( j > 0 ) ; )
  if( ( i > 0 ) && ( j > 0 ) && ( DP[ i ][ j ] == DP[ i - 1 ][ j - 1 ] +
                                  ( P[ i - 1 ] == T[ j - 1 ] ? 1 : -1 ) ) ) {
   a.push_back( P[ --i ] );
   b.push_back( T[ --j ] );
   }
  else if( ( i > 0 ) && ( DP[ i ][ j ] == DP[ i - 1 ][ j ] - 2 ) ) {
   a.push_back( P[ --i ] );
   b.push_back( gap );
   }
  else {
   a.push_back( gap );
   b.push_back( T[ --j ] );
   }
 std::reverse( a.begin() , a.end() );
 std::reverse( b.begin() , b.end() );
 return( std::pair( a , b ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Alignment.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Alignment.h --------------------------*/
/*--------------------------------------------------------------------------*/
