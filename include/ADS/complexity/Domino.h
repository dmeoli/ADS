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
 * The contracts [see Contract.h]: a covering returned by squareTiling() is
 * checked by legalTiling(); the certificate and the linear tiling accept any
 * input. The operations are defined in Domino.tpp.
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

#include "ADS/Contract.h"

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

/// the certificate: T is a legal covering of the n x n square by D, with
/// d = D[ 0 ] in ( 0 , 0 ); false if n < 1, if T is not n x n or if a tile
/// of T is not one of D
inline bool legalTiling( const std::vector< Tile > & D , int n ,
                         const Tiling & T );

/// a legal covering of the n x n square, by backtrack, or nothing (also if
/// n < 1 or D is empty)
/// \post a covering returned passes legalTiling()
inline std::optional< Tiling > squareTiling( const std::vector< Tile > & D ,
                                             int n );

/// the linear tiling of a strip of n tiles from d = D[ 0 ], O( m^2 ); false
/// if n < 1 or D is empty, true for any n if a cycle of tiles that fit is
/// reachable from d
inline bool linearTiling( const std::vector< Tile > & D , long n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/impl/Domino.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Domino.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Domino.h ----------------------------*/
/*--------------------------------------------------------------------------*/
