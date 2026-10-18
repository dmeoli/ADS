/*--------------------------------------------------------------------------*/
/*----------------------- File InterpolationSearch.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search by interpolation of section 12.1 in a sorted vector of
 * numbers: in the part A[ i .. j ] the key v is looked for in the place
 * \f[
 *   m = i + \left\lfloor ( j - i ) \frac{v - A[ i ]}{A[ j ] - A[ i ]}
 *   \right\rfloor
 * \f]
 * instead of the middle one, as one opens a phone book near its start to
 * look for a name in C. With keys uniformly distributed it takes
 * \f$ O( \log \log n ) \f$ expected time, but O( n ) in the worst case, so
 * that with few keys or with keys not uniform the binary search is better.
 * The search stops when v is out of [ A[ i ] , A[ j ] ], and probes A[ i ]
 * when A[ i ] = A[ j ]. The fraction is computed in double, and when it is
 * undefined, i.e., 0 / 0 for distinct keys equal as doubles (integers past
 * 2^53) or inf / inf for infinite keys, the probe is the middle place.
 *
 * The contracts [see Contract.h]: the vector must be sorted, an audit checked
 * at the level 3, and a miss is checked against a linear scan. The functions
 * are defined in InterpolationSearch.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see Y. Perl, A. Itai, H. Avni, Interpolation search: a log log N
 *      search, Communications of the ACM 21(7), 550-553, 1978,
 *      doi:10.1145/359545.359557
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

/// ricercaInterpolata(): the place of v in the sorted A, or -1;
/// trace( i , j , m ) on each probe
/// \pre A is sorted
/// \post the result is -1 if and only if v is not in A
template< class T , class F >
long interpolationSearch( const std::vector< T > & A , const T & v ,
                          F && trace );

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
