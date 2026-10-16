/*--------------------------------------------------------------------------*/
/*-------------------- File LongestCommonSubsequence.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The longest common subsequence (LCS) of P and T, the reality check of
 * section 13.7: DP[ i ][ j ], its length for the prefixes P( i ) and
 * T( j ), is 0 if i = 0 or j = 0, DP[ i - 1 ][ j - 1 ] + 1 if
 * p_{i-1} = t_{j-1}, and \f$ \max\{ DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] \}
 * \f$ otherwise, Theta( m n ). lcsMemo() is the memoized version of the
 * book, which solves only the subproblems that it reaches. diff() reads the
 * differences from the table in O( m + n ), as the Unix command does on
 * the lines of two files (exercise 13.10): the items of P only marked
 * '-', the items of T only '+', the common ones ' '. The longest common
 * substring, contiguous, has DP[ i ][ j ] = DP[ i - 1 ][ j - 1 ] + 1 if
 * p_{i-1} = t_{j-1} and 0 otherwise, its length the largest entry
 * (exercise 13.2).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see J. W. Hunt, M. D. McIlroy, An algorithm for differential file
 *      comparison, Computing Science Technical Report 41, Bell
 *      Laboratories, 1976
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

#ifndef __ADS_LongestCommonSubsequence
 #define __ADS_LongestCommonSubsequence
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the table of the lengths of the LCS of the prefixes
template< class S >
std::vector< std::vector< int > > lcsTable( const S & P , const S & T )
{
 const std::size_t m = P.size() , n = T.size();
 std::vector< std::vector< int > > DP( m + 1 , std::vector< int >( n + 1 ) );
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ i ][ j ] = P[ i - 1 ] == T[ j - 1 ] ? DP[ i - 1 ][ j - 1 ] + 1 :
                  std::max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] );
 return( DP );
 }

/// lcs() of the book, by memoization: -1 marks the subproblems not solved
/// yet; the number of the ones solved goes to *solved, if given
template< class S >
int lcsMemo( const S & P , const S & T , long * solved = nullptr )
{
 std::vector< std::vector< int > > DP(
                   P.size() + 1 , std::vector< int >( T.size() + 1 , -1 ) );
 auto rec = [ & ]( auto & self , std::size_t i , std::size_t j ) -> int {
  if( ( i == 0 ) || ( j == 0 ) )
   return( 0 );
  if( DP[ i ][ j ] < 0 ) {
   if( solved ) ++*solved;
   DP[ i ][ j ] = P[ i - 1 ] == T[ j - 1 ] ?
                  self( self , i - 1 , j - 1 ) + 1 :
                  std::max( self( self , i - 1 , j ) ,
                            self( self , i , j - 1 ) );
   }
  return( DP[ i ][ j ] );
  };
 return( rec( rec , P.size() , T.size() ) );
 }

/// a longest common subsequence, read back from the table
template< class S >
S lcs( const S & P , const S & T )
{
 const auto DP = lcsTable( P , T );
 S out;
 for( std::size_t i = P.size() , j = T.size() ; ( i > 0 ) && ( j > 0 ) ; )
  if( P[ i - 1 ] == T[ j - 1 ] ) {
   out.push_back( P[ i - 1 ] );
   --i; --j;
   }
  else if( DP[ i - 1 ][ j ] >= DP[ i ][ j - 1 ] )
   --i;
  else
   --j;
 std::reverse( out.begin() , out.end() );
 return( out );
 }

/// the differences from P to T: ( '-' , p ) for an item of P only,
/// ( '+' , t ) for an item of T only, ( ' ' , x ) for a common one
template< class S >
std::vector< std::pair< char , typename S::value_type > > diff(
                                                 const S & P , const S & T )
{
 const auto DP = lcsTable( P , T );
 std::vector< std::pair< char , typename S::value_type > > out;
 std::size_t i = P.size() , j = T.size();
 while( ( i > 0 ) || ( j > 0 ) )
  if( ( i > 0 ) && ( j > 0 ) && ( P[ i - 1 ] == T[ j - 1 ] ) ) {
   out.emplace_back( ' ' , P[ i - 1 ] );
   --i; --j;
   }
  else if( ( j > 0 ) &&
           ( ( i == 0 ) || ( DP[ i ][ j - 1 ] >= DP[ i - 1 ][ j ] ) ) )
   out.emplace_back( '+' , T[ --j ] );
  else
   out.emplace_back( '-' , P[ --i ] );
 std::reverse( out.begin() , out.end() );
 return( out );
 }

/// the longest common substring: its length and where it ends in P and T
template< class S >
std::tuple< std::size_t , std::size_t , std::size_t >
longestCommonSubstring( const S & P , const S & T )
{
 std::vector< std::size_t > prev( T.size() + 1 , 0 ) , DP( T.size() + 1 , 0 );
 std::size_t best = 0 , ei = 0 , ej = 0;
 for( std::size_t i = 1 ; i <= P.size() ; ++i ) {
  std::swap( prev , DP );
  for( std::size_t j = 1 ; j <= T.size() ; ++j ) {
   DP[ j ] = P[ i - 1 ] == T[ j - 1 ] ? prev[ j - 1 ] + 1 : 0;
   if( DP[ j ] > best ) { best = DP[ j ]; ei = i; ej = j; }
   }
  }
 return( std::tuple( best , ei , ej ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* LongestCommonSubsequence.h included */

/*--------------------------------------------------------------------------*/
/*------------------ End File LongestCommonSubsequence.h -------------------*/
/*--------------------------------------------------------------------------*/
