/*--------------------------------------------------------------------------*/
/*----------------------- File InterpolationSearch.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search by interpolation of section 12.1 in a sorted vector of
 * numbers: in the part \f$ A[ i .. j ] \f$ the key \f$ v \f$ is looked for in
 * the place
 * \f[
 *   m = i + \left\lfloor ( j - i ) \frac{v - A[ i ]}{A[ j ] - A[ i ]}
 *   \right\rfloor
 * \f]
 * instead of the middle one, as one opens a phone book near its start to look
 * for a name in C. With keys uniformly distributed it takes
 * \f$ O( \log \log n ) \f$ expected time, but \f$ O( n ) \f$ in the worst
 * case, so that with few keys or with keys not uniform the binary search is
 * better. The search stops when \f$ v \f$ is out of
 * \f$ [ A[ i ] , A[ j ] ] \f$, and probes \f$ A[ i ] \f$ when
 * \f$ A[ i ] = A[ j ] \f$. The fraction is computed in `double`, and when it
 * is undefined, i.e., \f$ 0 / 0 \f$ for distinct keys equal as doubles
 * (integers past \f$ 2^{53} \f$) or \f$ \infty / \infty \f$ for infinite
 * keys, the probe is the middle place.
 *
 * The contracts [see `Contract.h`]: the vector must be sorted, an audit
 * checked at the level 3, and a miss is checked against a linear scan. The
 * functions are defined in `InterpolationSearch.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see Y. Perl, A. Itai, H. Avni, Interpolation search: a log log N
 *      search, Communications of the ACM 21(7), 550-553, 1978,
 *      doi:10.1145/359545.359557
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

#ifndef __ADS_InterpolationSearch
 #define __ADS_InterpolationSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place of \f$ v \f$ in the sorted \f$ A \f$, or -1; `trace( i , j , m )`
/// on each probe (the ricercaInterpolata() of the book)
///
/// In pseudocode (section 12.1), the offset kept in \f$ [ 0 , j - i ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{interpolationSearch}( A , v ) \cr
/// 2 & \quad i \gets 0;\ j \gets n - 1 \cr
/// 3 & \quad \textbf{while}\ i \le j\ \textbf{and}\ A[ i ] \le v \le A[ j ]
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ i ] = A[ j ]\ \textbf{then}\ m \gets i \cr
/// 5 & \qquad \textbf{else}\ m \gets i + \lfloor ( j - i ) ( v - A[ i ] ) / (
///     A[ j ] - A[ i ] ) \rfloor \cr
/// 6 & \qquad \mathrm{trace}( i , j , m ) \cr
/// 7 & \qquad \textbf{if}\ A[ m ] = v\ \textbf{then return}\ m \cr
/// 8 & \qquad \textbf{else if}\ A[ m ] < v\ \textbf{then}\ i \gets m + 1 \cr
/// 9 & \qquad \textbf{else}\ j \gets m - 1 \cr
/// 10 & \quad \textbf{return}\ -1
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted
/// \post the result is -1 if and only if \f$ v \f$ is not in \f$ A \f$
template< class T , class F >
long interpolationSearch( const std::vector< T > & A , const T & v ,
                          F && trace );

/// the search of `interpolationSearch()` above, with no trace
template< class T >
long interpolationSearch( const std::vector< T > & A , const T & v );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/InterpolationSearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* InterpolationSearch.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File InterpolationSearch.h ---------------------*/
/*--------------------------------------------------------------------------*/
