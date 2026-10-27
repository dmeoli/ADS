/*--------------------------------------------------------------------------*/
/*----------------------------- File Minimum.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum of a vector \f$ A[ 0 .. n - 1 ] \f$ (section 1.2): `min()`
 * keeps the minimum so far and compares it with each of the other items,
 * \f$ n - 1 \f$ comparisons, which are needed, since each comparison can
 * exclude at most one of the two items from being the minimum (section
 * 2.6). `argmin()` gives the place of the minimum of
 * \f$ A[ \mathit{start} .. \mathit{end} ] \f$, the first one if it occurs
 * more than once, as the selection sort of section 2.7.1 asks; `minMax()`
 * and `minMaxDivide()`, beyond the book, give the minimum and the maximum
 * together with \f$ \lceil 3 n / 2 \rceil - 2 \f$ comparisons, by pairs and
 * by halves, which Pohl proved optimal.
 *
 * The contracts [see `Contract.h`]: `argmin()` needs
 * \f$ \mathit{start} \le \mathit{end} < n \f$ and `min()` a vector that is
 * not empty; that no item of the range is smaller than the one found is an
 * audit, as costly as the search. The functions are defined in
 * `Minimum.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 1.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf, and
 *      sections 2.6 and 2.7.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see I. Pohl, A sorting problem and its complexity, Communications of the
 *      ACM 15(6), 462-464, 1972, doi:10.1145/361405.361423
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

#ifndef __ADS_Minimum
 #define __ADS_Minimum
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place of the first minimum of the range
/// \f$ A[ \mathit{start} .. \mathit{end} ] \f$
///
/// In pseudocode (section 2.7.1, where the selection sort calls it):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{argmin}( A , \mathit{start} , \mathit{end}
///     ) \cr
/// 2 & \quad \mathit{pos} \gets \mathit{start} \cr
/// 3 & \quad \textbf{for}\ j \gets \mathit{start} + 1\ \textbf{to}
///     \ \mathit{end}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ j ] < A[ \mathit{pos} ]\ \textbf{then}
///     \ \mathit{pos} \gets j \cr
/// 5 & \quad \textbf{return}\ \mathit{pos}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{start} \le \mathit{end} < n \f$
/// \post no item of \f$ A[ \mathit{start} .. \mathit{end} ] \f$ is smaller
///       than \f$ A[ \mathit{pos} ] \f$ (audit)
template< class T , class C = std::less<> >
std::size_t argmin( const std::vector< T > & A , std::size_t start ,
                    std::size_t end , C less = C() );

/// the minimum of a vector that is not empty, \f$ n - 1 \f$ comparisons
///
/// In pseudocode (section 1.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{min}( A ) \cr
/// 2 & \quad \textbf{return}\ A[ \mathrm{argmin}( A , 0 , n - 1 ) ]
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty
template< class T , class C = std::less<> >
const T & min( const std::vector< T > & A , C less = C() );

/// the minimum and the maximum together, by pairs: the two items of a pair
/// are compared, the smaller with the minimum and the larger with the
/// maximum, \f$ \lceil 3 n / 2 \rceil - 2 \f$ comparisons instead of the
/// \f$ 2 ( n - 1 ) \f$ of two searches
///
/// In pseudocode (Pohl, beyond the book), 3 comparisons for each pair:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minMax}( A ) \cr
/// 2 & \quad \mathit{lo} \gets A[ 0 ];\ \mathit{hi} \gets A[ 0 ];\ i \gets 1
///     \cr
/// 3 & \quad \textbf{if}\ n \text{ is even}\ \textbf{then} \cr
/// 4 & \qquad \textbf{if}\ A[ 1 ] < A[ 0 ]\ \textbf{then}\ \mathit{lo} \gets
///     A[ 1 ]\ \textbf{else}\ \mathit{hi} \gets A[ 1 ] \cr
/// 5 & \qquad i \gets 2 \cr
/// 6 & \quad \textbf{while}\ i + 1 < n\ \textbf{do} \cr
/// 7 & \qquad ( a , b ) \gets A[ i ] , A[ i + 1 ] \text{, the smaller first}
///     \cr
/// 8 & \qquad \textbf{if}\ a < \mathit{lo}\ \textbf{then}\ \mathit{lo} \gets
///     a \cr
/// 9 & \qquad \textbf{if}\ \mathit{hi} < b\ \textbf{then}\ \mathit{hi} \gets
///     b \cr
/// 10 & \qquad i \gets i + 2 \cr
/// 11 & \quad \textbf{return}\ ( \mathit{lo} , \mathit{hi} )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty
/// \post the minimum and the maximum of \f$ A \f$ (audit)
template< class T , class C = std::less<> >
std::pair< T , T > minMax( const std::vector< T > & A , C less = C() );

/// the pair of `minMax()` by divide et impera: the halves solved recursively
/// and their minima and maxima compared, \f$ T( 2 ) = 1 \f$,
/// \f$ T( n ) = 2 T( n / 2 ) + 2 \f$, i.e., \f$ 3 n / 2 - 2 \f$ comparisons
/// for \f$ n \f$ a power of 2
///
/// In pseudocode (Pohl, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minMaxRec}( A , i , j ) \cr
/// 2 & \quad \textbf{if}\ i = j\ \textbf{then return}\ ( A[ i ] , A[ i ] ) \cr
/// 3 & \quad \textbf{if}\ j = i + 1\ \textbf{then return}\ ( A[ i ] , A[ j ]
///     ) \text{, the smaller first} \cr
/// 4 & \quad m \gets i + \lfloor ( j - i ) / 2 \rfloor \cr
/// 5 & \quad ( a , b ) \gets \mathrm{minMaxRec}( A , i , m );\ ( c , d )
///     \gets \mathrm{minMaxRec}( A , m + 1 , j ) \cr
/// 6 & \quad \textbf{return}\ ( \min\{ a , c \} , \max\{ b , d \} ) \cr
/// 7 & \textbf{function}\ \mathrm{minMaxDivide}( A ) \cr
/// 8 & \quad \textbf{return}\ \mathrm{minMaxRec}( A , 0 , n - 1 )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty
/// \post the minimum and the maximum of \f$ A \f$ (audit)
template< class T , class C = std::less<> >
std::pair< T , T > minMaxDivide( const std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/Minimum.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Minimum.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Minimum.h ---------------------------*/
/*--------------------------------------------------------------------------*/
