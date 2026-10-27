/*--------------------------------------------------------------------------*/
/*---------------------------- File ShellSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Shell Sort (section 15.1), a local search that lowers the number of
 * inversions: an insertion sort of the items \f$ h \f$ apart, for decreasing
 * gaps \f$ h \f$ down to 1, so that the far items move first and the last
 * pass, the insertion sort, finds the vector almost sorted. In place,
 * iterative, not stable; its time depends on the gaps:
 *
 * - `knuthGaps()`, the \f$ h = ( 3^i - 1 ) / 2 \f$, i.e.,
 *   \f$ 1 , 4 , 13 , 40 , 121 , \ldots \f$ of the book, \f$ O( n^{1.5} ) \f$
 *   in the worst case;
 * - `shellGaps()`, the \f$ n / 2 , n / 4 , \ldots , 1 \f$ of Shell,
 *   \f$ \Theta( n^2 ) \f$ in the worst case;
 * - `hibbardGaps()`, the \f$ 2^k - 1 = 1 , 3 , 7 , 15 , \ldots \f$ of
 *   Hibbard, which unlike those of Shell are coprime, \f$ O( n^{1.5} ) \f$ in
 *   the worst case;
 * - `prattGaps()`, all the \f$ 3^i 2^j \f$ up to \f$ n \f$,
 *   \f$ O( n \log^2 n ) \f$ in the worst case;
 * - `ciuraGaps()`, \f$ 1 , 4 , 10 , 23 , 57 , 132 , 301 , 701 \f$ and then
 *   \f$ \times 2.25 \f$, found by experiment and among the fastest in
 *   practice.
 *
 * The contracts [see `Contract.h`]: the five generators give gaps that start
 * from 1 and increase; `shellSort()` needs positive increasing gaps, and the
 * vector ends with the same items, sorted when the first gap is 1 (with a
 * single gap \f$ h \f$ it is only \f$ h \f$-sorted, as in figure 15.4), an
 * audit. The functions are defined in `ShellSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see D. L. Shell, A high-speed sorting procedure, Communications of the
 *      ACM 2(7), 30-32, 1959, doi:10.1145/368370.368387
 * \see T. N. Hibbard, An empirical study of minimal storage sorting,
 *      Communications of the ACM 6(5), 206-213, 1963,
 *      doi:10.1145/366552.366557
 * \see V. R. Pratt, Shellsort and sorting networks, PhD thesis, Stanford
 *      University, 1971
 * \see M. Ciura, Best increments for the average case of Shellsort, FCT
 *      2001, LNCS 2138, 106-117, doi:10.1007/3-540-44669-9_12
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

#ifndef __ADS_ShellSort
 #define __ADS_ShellSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the gaps \f$ h_1 = 1 \f$, \f$ h_{i+1} = 3 h_i + 1 \f$ below \f$ n \f$
/// (\f$ 1 , 4 , 13 , 40 , \ldots \f$), increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > knuthGaps( std::size_t n );

/// the gaps \f$ 1 , \ldots , \lfloor n / 4 \rfloor , \lfloor n / 2 \rfloor \f$
/// of Shell, each half the next one, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > shellGaps( std::size_t n );

/// the gaps \f$ 2^k - 1 \f$ below \f$ n \f$ (\f$ 1 , 3 , 7 , 15 , \ldots \f$),
/// increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > hibbardGaps( std::size_t n );

/// the gaps \f$ 3^i 2^j \f$ up to \f$ n \f$, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > prattGaps( std::size_t n );

/// the gaps of Ciura below \f$ n \f$, \f$ 1 , 4 , 10 , \ldots , 701 \f$ and
/// then each \f$ 2.25 \f$ times the one before, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > ciuraGaps( std::size_t n );

/// the Shell Sort with the given gaps, increasing and starting from 1
///
/// In pseudocode (section 15.1), an insertion sort of the items \f$ h \f$
/// apart for each gap \f$ h \f$, the largest first:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{shellSort}( A , n , H ) \cr
/// 2 & \quad \textbf{foreach}\ h \in H \text{ in decreasing order}
///     \ \textbf{do} \cr
/// 3 & \qquad \textbf{for}\ i \gets h\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad\quad \mathit{temp} \gets A[ i ];\ j \gets i \cr
/// 5 & \qquad\quad \textbf{while}\ j \ge h\ \textbf{and}\ \mathit{temp} < A[ j
///     - h ]\ \textbf{do} \cr
/// 6 & \qquad\qquad A[ j ] \gets A[ j - h ];\ j \gets j - h \cr
/// 7 & \qquad\quad A[ j ] \gets \mathit{temp}
/// \end{array}}
/// \f]
/// \pre the gaps are positive and increase
/// \post \f$ A \f$ holds the same items, sorted if the first gap is 1 (audit)
template< class T , class C = std::less<> >
void shellSort( std::vector< T > & A ,
                const std::vector< std::size_t > & gaps , C less = C() );

/// the Shell Sort of the book, with the gaps \f$ 1 , 4 , 13 , \ldots \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{shellSort}( A , n ) \cr
/// 2 & \quad \mathrm{shellSort}( A , n , \mathrm{knuthGaps}( n ) )
/// \end{array}}
/// \f]
template< class T , class C = std::less<> >
void shellSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/ShellSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ShellSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ShellSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
