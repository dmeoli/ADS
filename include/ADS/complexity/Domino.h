/*--------------------------------------------------------------------------*/
/*----------------------------- File Domino.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The square tiling (BOUNDED TILING, the DOMINO LIMITATO of the book) of
 * chapter 18: \f$ n \times n \f$ squares to cover with copies of \f$ m \f$
 * oriented tiles, each split by its diagonals into four colored quarters,
 * never rotated, the tile \f$ d = D[ 0 ] \f$ in the corner \f$ ( 0 , 0 ) \f$,
 * and touching sides of the same color. The square \f$ ( i , j ) \f$ is the
 * column \f$ i \f$ and the row \f$ j \f$ from the bottom left; a tile is left
 * of the one in \f$ ( i + 1 , j ) \f$ and below the one in
 * \f$ ( i , j + 1 ) \f$.
 *
 * `legalTiling()` is the certificate, \f$ O( n^2 ) \f$; `squareTiling()` the
 * enumeration by backtrack of the \f$ m^{n^2 - 1} \f$ coverings, pruned at
 * the first wrong side. `linearTiling()` solves the linear tiling, a strip
 * \f$ 1 \times n \f$, in polynomial time: by the reduction of the text, the
 * tiles are the nodes of a graph with an edge \f$ ( d_u , d_v ) \f$ when the
 * right side of \f$ d_u \f$ has the color of the left side of \f$ d_v \f$,
 * and a strip is a walk of \f$ n - 1 \f$ edges from \f$ d \f$, which exists
 * if and only if a cycle is reachable from \f$ d \f$ or the longest path
 * from \f$ d \f$, in the acyclic part, has \f$ n - 1 \f$ edges;
 * \f$ O( m^2 ) \f$ time.
 *
 * The contracts [see `Contract.h`]: a covering returned by `squareTiling()`
 * is checked by `legalTiling()`; the certificate and the linear tiling accept
 * any input. The operations are defined in `Domino.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 18,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
 * \see H. Wang, Proving theorems by pattern recognition II, Bell System
 *      Technical Journal 40(1), 1-41, 1961,
 *      doi:10.1002/j.1538-7305.1961.tb03975.x
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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
 int up ,      ///< the color of the upper side
     right ,   ///< the color of the right side
     down ,    ///< the color of the lower side
     left;     ///< the color of the left side
 };

using Tiling = std::vector< std::vector< int > >;   ///< the tile of ( i , j )

/// whether \f$ T \f$ is a legal covering of the \f$ n \times n \f$ square by
/// \f$ D \f$, with \f$ d = D[ 0 ] \f$ in \f$ ( 0 , 0 ) \f$; false if
/// \f$ n < 1 \f$, if \f$ T \f$ is not \f$ n \times n \f$ or if a tile of
/// \f$ T \f$ is not one of \f$ D \f$
///
/// In pseudocode (chapter 18), the certificate in \f$ O( n^2 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{legalTiling}( D , n , T ) \cr
/// 2 & \quad \textbf{if}\ n < 1 \text{, } T \text{ is not } n \times n
///     \text{ or not of tiles of } D\ \textbf{then return false} \cr
/// 3 & \quad \textbf{if}\ T[ 0 ][ 0 ] \ne 0\ \textbf{then return false} \cr
/// 4 & \quad \textbf{foreach}\ \text{square } ( i , j )\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ i + 1 < n \text{ and } D[ T[ i ][ j ]
///     ].\mathit{right} \ne D[ T[ i + 1 ][ j ] ].\mathit{left}
///     \ \textbf{then return false} \cr
/// 6 & \qquad \textbf{if}\ j + 1 < n \text{ and } D[ T[ i ][ j ] ].\mathit{up}
///     \ne D[ T[ i ][ j + 1 ] ].\mathit{down}\ \textbf{then return false} \cr
/// 7 & \quad \textbf{return true}
/// \end{array}}
/// \f]
inline bool legalTiling( const std::vector< Tile > & D , int n ,
                         const Tiling & T );

/// a legal covering of the \f$ n \times n \f$ square, by backtrack, or
/// nothing (also if \f$ n < 1 \f$ or \f$ D \f$ is empty)
///
/// In pseudocode (chapter 18), the squares filled by rows from the bottom,
/// \f$ s = i + j n \f$, a tile placed only if it fits its left and lower
/// neighbors:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{place}( s )\quad \triangleright
///     \ \text{started with } s = 0 \cr
/// 2 & \quad \textbf{if}\ s = n^2\ \textbf{then return true} \cr
/// 3 & \quad i \gets s \bmod n;\ j \gets \lfloor s / n \rfloor \cr
/// 4 & \quad \textbf{foreach}\ t \in ( \{ 0 \} \text{ if } s = 0
///     \text{, else } 0 , \ldots , m - 1 )\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ i > 0 \text{ and } D[ T[ i - 1 ][ j ]
///     ].\mathit{right} \ne D[ t ].\mathit{left}\ \textbf{then continue} \cr
/// 6 & \qquad \textbf{if}\ j > 0 \text{ and } D[ T[ i ][ j - 1 ] ].\mathit{up}
///     \ne D[ t ].\mathit{down}\ \textbf{then continue} \cr
/// 7 & \qquad T[ i ][ j ] \gets t \cr
/// 8 & \qquad \textbf{if}\ \mathrm{place}( s + 1 )
///     \ \textbf{then return true} \cr
/// 9 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \post a covering returned passes `legalTiling()`
inline std::optional< Tiling > squareTiling( const std::vector< Tile > & D ,
                                             int n );

/// whether a strip of \f$ n \f$ tiles from \f$ d = D[ 0 ] \f$ exists,
/// \f$ O( m^2 ) \f$; false if \f$ n < 1 \f$ or \f$ D \f$ is empty, true for
/// any \f$ n \f$ if a cycle of tiles that fit is reachable from \f$ d \f$
///
/// In pseudocode (section 18.5), a visit in depth of the graph of the tiles
/// from \f$ d \f$, \f$ L[ u ] \f$ the edges of the longest path from
/// \f$ u \f$ in the acyclic part:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{visit}( u )\quad \triangleright
///     \ \mathit{state}[ u ] \text{ new, open or done} \cr
/// 2 & \quad \mathit{state}[ u ] \gets \text{open} \cr
/// 3 & \quad \textbf{foreach}\ v \text{ with } D[ u ].\mathit{right} = D[ v
///     ].\mathit{left}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{state}[ v ] = \text{open}\ \textbf{then}
///     \ \mathit{cycle} \gets \textbf{true} \cr
/// 5 & \qquad \textbf{else if}\ \mathit{state}[ v ] = \text{new}
///     \ \textbf{then}\ \mathrm{visit}( v ) \cr
/// 6 & \qquad L[ u ] \gets \max\{ L[ u ] , 1 + L[ v ] \} \cr
/// 7 & \quad \mathit{state}[ u ] \gets \text{done} \cr
/// 8 & \textbf{function}\ \mathrm{linearTiling}( D , n ) \cr
/// 9 & \quad \textbf{if}\ n < 1 \text{ or } m = 0
///     \ \textbf{then return false} \cr
/// 10 & \quad \mathrm{visit}( 0 ) \cr
/// 11 & \quad \textbf{return}\ \mathit{cycle}\ \textbf{or}\ L[ 0 ] \ge n - 1
/// \end{array}}
/// \f]
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
