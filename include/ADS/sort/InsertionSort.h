/*--------------------------------------------------------------------------*/
/*-------------------------- File InsertionSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Insertion Sort (section 2.7.2), as one sorts a hand of cards: \f$ A[ i ] \f$
 * is inserted in the sorted prefix \f$ A[ 0 .. i - 1 ] \f$, the larger items
 * moving one place to the right. \f$ \Theta( n ) \f$ time on a sorted vector,
 * \f$ \Theta( n^2 ) \f$ on a reversed one and on average
 * (\f$ n ( n - 1 ) / 4 \f$ moves, one per inversion), \f$ O( 1 ) \f$ space,
 * stable; with its simple inner loop it is the fastest on few items, hence the
 * base case of the hybrid sorts.
 *
 * The contracts [see `Contract.h`]: \f$ \mathit{lo} \le \mathit{hi} \le n \f$,
 * and the range ends sorted with the same items, an audit. The functions are
 * defined in `InsertionSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#ifndef __ADS_InsertionSort
 #define __ADS_InsertionSort
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

/// the insertion sort of \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$
///
/// In pseudocode (section 2.7.2), \f$ A[ \mathit{lo} .. i - 1 ] \f$ sorted
/// before each insertion:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{insertionSort}( A , \mathit{lo} ,
///     \mathit{hi} ) \cr
/// 2 & \quad \textbf{for}\ i \gets \mathit{lo} + 1\ \textbf{to}\ \mathit{hi} -
///     1\ \textbf{do} \cr
/// 3 & \qquad \mathit{temp} \gets A[ i ];\ j \gets i \cr
/// 4 & \qquad \textbf{while}\ j > \mathit{lo}\ \textbf{and}\ \mathit{temp} <
///     A[ j - 1 ]\ \textbf{do} \cr
/// 5 & \qquad\quad A[ j ] \gets A[ j - 1 ];\ j \gets j - 1 \cr
/// 6 & \qquad A[ j ] \gets \mathit{temp}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{lo} \le \mathit{hi} \le n \f$
/// \post \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ is sorted, with the same
///       items (audit)
template< class T , class C = std::less<> >
void insertionSort( std::vector< T > & A , std::size_t lo , std::size_t hi ,
                    C less = C() );

/// the insertion sort with a sentinel: the minimum first goes to
/// \f$ A[ 0 ] \f$, with \f$ n - 1 \f$ comparisons and one swap, so that the
/// inner loop of each insertion needs no test on the index, since it stops at
/// \f$ A[ 0 ] \f$ at the latest
///
/// In pseudocode (section 2.7.2, with a sentinel):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{insertionSortSentinel}( A , n ) \cr
/// 2 & \quad \textbf{if}\ n < 2\ \textbf{then return} \cr
/// 3 & \quad A[ 0 ] \leftrightarrow A[ \mathrm{argmin}( A , 0 , n - 1 ) ] \cr
/// 4 & \quad \textbf{for}\ i \gets 2\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad x \gets A[ i ];\ j \gets i \cr
/// 6 & \qquad \textbf{while}\ x < A[ j - 1 ]\ \textbf{do}\ A[ j ] \gets A[ j
///     - 1 ];\ j \gets j - 1 \cr
/// 7 & \qquad A[ j ] \gets x
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void insertionSortSentinel( std::vector< T > & A , C less = C() );

/// \f$ A \f$ in increasing order by `less`, the insertion sort of the whole
/// vector
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{insertionSort}( A ) \cr
/// 2 & \quad \mathrm{insertionSort}( A , 0 , n )
/// \end{array}}
/// \f]
template< class T , class C = std::less<> >
void insertionSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/InsertionSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* InsertionSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File InsertionSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
