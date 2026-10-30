/*--------------------------------------------------------------------------*/
/*------------------------ File MultikeyQuickSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multikey quicksort of Bentley and Sedgewick, as chapter 7, Sorting
 * Strings, of Ferragina's book presents it: the quicksort of strings by one
 * character at a time, a partition into three parts, the strings whose
 * character \f$ d \f$ is smaller, equal and larger than that of the pivot,
 * the equal part going on with the character \f$ d + 1 \f$ and the others
 * with \f$ d \f$. It is the sort that corresponds
 * to the ternary search tree of the same paper, and makes
 * \f$ O( D + n \log n ) \f$ comparisons of characters for \f$ D \f$ the total
 * length of the distinguishing prefixes, against the \f$ O( n \log n ) \f$
 * comparisons of whole strings of a quicksort, each as long as their common
 * prefix. It sorts in place, not stable.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same strings
 * sorted, an audit. The function is defined in `MultikeyQuickSort.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 7, Sorting Strings, doi:10.1017/9781009128933
 * \see J. L. Bentley, R. Sedgewick, Fast algorithms for sorting and
 *      searching strings, Proceedings of the 8th ACM-SIAM Symposium on
 *      Discrete Algorithms (SODA), 360-369, 1997
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

#ifndef __ADS_MultikeyQuickSort
 #define __ADS_MultikeyQuickSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the multikey quicksort of strings
///
/// In pseudocode (Algorithm 7.1 of chapter 7, Sorting Strings, of Ferragina's
/// book, with the middle string as the pivot), \f$ s[ d ] \f$ the character
/// \f$ d \f$ of \f$ s \f$, \f$ -1 \f$ past its end:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{mkqs}( A , \mathit{lo} , \mathit{hi} , d )
///     \cr
/// 2 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} \le 10\ \textbf{then}
///     \ \text{insertion sort by the suffixes from } d;\ \textbf{return} \cr
/// 3 & \quad v \gets A[ \lfloor ( \mathit{lo} + \mathit{hi} ) / 2 \rfloor ][ d
///     ] \cr
/// 4 & \quad \text{split } A[ \mathit{lo} .. \mathit{hi} - 1 ] \text{ into }
///     \text{the parts of character } d \text{ below, at and above } v \cr
/// 5 & \quad \mathit{lt} , \mathit{gt} \gets \text{the first places of the }
///     \text{equal and of the larger part} \cr
/// 6 & \quad \mathrm{mkqs}( A , \mathit{lo} , \mathit{lt} , d ) \cr
/// 7 & \quad \textbf{if}\ v \ge 0\ \textbf{then}\ \mathrm{mkqs}( A ,
///     \mathit{lt} , \mathit{gt} , d + 1 ) \cr
/// 8 & \quad \mathrm{mkqs}( A , \mathit{gt} , \mathit{hi} , d ) \cr
/// 9 & \textbf{procedure}\ \mathrm{multikeyQuickSort}( A ) \cr
/// 10 & \quad \mathrm{mkqs}( A , 0 , n , 0 )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same strings (audit)
inline void multikeyQuickSort( std::vector< std::string > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/MultikeyQuickSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MultikeyQuickSort.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File MultikeyQuickSort.h ----------------------*/
/*--------------------------------------------------------------------------*/
