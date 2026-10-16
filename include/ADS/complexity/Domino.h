/*--------------------------------------------------------------------------*/
/*----------------------------- File Domino.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The square tiling (DOMINO LIMITATO) of chapter 18: n x n squares to cover
 * with copies of m oriented tiles, each split by its diagonals into four
 * colored quarters, never rotated, the tile d = D[ 0 ] in the corner ( 0 ,
 * 0 ), and touching sides of the same color. The square ( i , j ) is the
 * column i and the row j from the bottom left; a tile is left of the one
 * in ( i + 1 , j ) and below the one in ( i , j + 1 ).
 *
 * legalTiling() is the certificate, O( n^2 ); squareTiling() the
 * enumeration by backtrack of the m^{n^2 - 1} coverings, pruned at the first
 * wrong side. linearTiling() solves the linear tiling, a strip 1 x n, in
 * polynomial time: by the reduction of the text, the tiles are the nodes
 * of a graph with ( d_u , d_v ) when the right side of d_u has the color of
 * the left side of d_v, and a strip is a walk of n - 1 edges from d, which
 * exists if and only if a cycle is reachable from d or the longest path
 * from d, in the acyclic part, has n - 1 edges; O( m^2 ) time.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 18,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
 * \see H. Wang, Proving theorems by pattern recognition II, Bell System
 *      Technical Journal 40(1), 1-41, 1961,
 *      doi:10.1002/j.1538-7305.1961.tb03975.x
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

#ifndef __ADS_Domino
 #define __ADS_Domino
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a tile: the colors of its four sides
struct Tile {
 int up , right , down , left;
 };

using Tiling = std::vector< std::vector< int > >;   ///< T[ i ][ j ], a tile

/// the certificate: T is a legal covering of the n x n square by D
inline bool legalTiling( const std::vector< Tile > & D , int n ,
                         const Tiling & T )
{
 if( ( int( T.size() ) != n ) || ( n < 1 ) || ( T[ 0 ][ 0 ] != 0 ) )
  return( false );
 for( int i = 0 ; i < n ; ++i ) {
  if( int( T[ i ].size() ) != n ) return( false );
  for( int j = 0 ; j < n ; ++j ) {
   const int t = T[ i ][ j ];
   if( ( t < 0 ) || ( t >= int( D.size() ) ) ) return( false );
   if( ( i + 1 < n ) && ( D[ t ].right != D[ T[ i + 1 ][ j ] ].left ) )
    return( false );
   if( ( j + 1 < n ) && ( D[ t ].up != D[ T[ i ][ j + 1 ] ].down ) )
    return( false );
   }
  }
 return( true );
 }

/// a legal covering of the n x n square, by backtrack, or nothing
inline std::optional< Tiling > squareTiling( const std::vector< Tile > & D ,
                                             int n )
{
 if( ( n < 1 ) || D.empty() )
  return( std::nullopt );
 Tiling T( n , std::vector< int >( n , -1 ) );
 auto rec = [ & ]( auto & self , int s ) -> bool {
  if( s == n * n )
   return( true );
  const int i = s % n , j = s / n;                 // by rows from the bottom
  for( int t = 0 ; t < int( D.size() ) ; ++t ) {
   if( ( s == 0 ) && ( t != 0 ) ) break;
   if( ( i > 0 ) && ( D[ T[ i - 1 ][ j ] ].right != D[ t ].left ) ) continue;
   if( ( j > 0 ) && ( D[ T[ i ][ j - 1 ] ].up != D[ t ].down ) ) continue;
   T[ i ][ j ] = t;
   if( self( self , s + 1 ) )
    return( true );
   }
  T[ i ][ j ] = -1;
  return( false );
  };
 if( ! rec( rec , 0 ) )
  return( std::nullopt );
 return( T );
 }

/// the linear tiling of a strip of n tiles from d = D[ 0 ], O( m^2 )
inline bool linearTiling( const std::vector< Tile > & D , long n )
{
 const int m = int( D.size() );
 if( ( n < 1 ) || ( m == 0 ) )
  return( false );
 // the nodes reachable from d, and a cycle among them, by a dfs
 std::vector< int > state( m , 0 );               // 0 new, 1 open, 2 done
 std::vector< long > longest( m , 0 );            // in the acyclic part
 bool cycle = false;
 auto dfs = [ & ]( auto & self , int u ) -> void {
  state[ u ] = 1;
  for( int v = 0 ; v < m ; ++v )
   if( D[ u ].right == D[ v ].left ) {
    if( state[ v ] == 1 ) cycle = true;
    else if( state[ v ] == 0 ) self( self , v );
    longest[ u ] = std::max( longest[ u ] , 1 + longest[ v ] );
    }
  state[ u ] = 2;
  };
 dfs( dfs , 0 );
 return( cycle || ( longest[ 0 ] >= n - 1 ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Domino.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Domino.h ----------------------------*/
/*--------------------------------------------------------------------------*/
