/*--------------------------------------------------------------------------*/
/*---------------------------- File ShellSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Shell Sort (section 15.1), a local search that lowers the number of
 * inversions: an insertion sort of the items h apart, for decreasing gaps
 * h down to 1, so that the far items move first and the last pass, the
 * insertion sort, finds the vector almost sorted. In place, iterative, not
 * stable; its time depends on the gaps:
 *
 * - knuthGaps(), the h = ( 3^i - 1 ) / 2 = 1 , 4 , 13 , 40 , 121 , ... of
 *   the book, \f$ O( n^{1.5} ) \f$ in the worst case;
 * - shellGaps(), the n / 2 , n / 4 , ... , 1 of Shell, \f$ \Theta( n^2 ) \f$
 *   in the worst case;
 * - hibbardGaps(), the \f$ 2^k - 1 \f$ = 1 , 3 , 7 , 15 , ... of Hibbard,
 *   which unlike those of Shell are coprime, \f$ O( n^{1.5} ) \f$ in the
 *   worst case;
 * - prattGaps(), all the 3^i 2^j up to n, \f$ O( n \log^2 n ) \f$ in the
 *   worst case;
 * - ciuraGaps(), 1 , 4 , 10 , 23 , 57 , 132 , 301 , 701 and then
 *   \f$ \times 2.25 \f$, found by experiment and among the fastest in
 *   practice.
 *
 * The contracts [see Contract.h]: the four generators give gaps that start
 * from 1 and increase; shellSort() needs positive increasing gaps, and the
 * vector ends with the same items, sorted when the first gap is 1 (with a
 * single gap h it is only h-sorted, as in Figura 15.4), an audit. The
 * functions are defined in ShellSort.tpp.
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

/// the gaps 1 , 4 , 13 , 40 , ... below n, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > knuthGaps( std::size_t n );

/// the gaps 1 , ... , n / 4 , n / 2 of Shell, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > shellGaps( std::size_t n );

/// the gaps 1 , 3 , 7 , 15 , ... below n, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > hibbardGaps( std::size_t n );

/// the gaps 3^i 2^j up to n, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > prattGaps( std::size_t n );

/// the gaps of Ciura below n, increasing
/// \post the gaps start from 1 and increase
inline std::vector< std::size_t > ciuraGaps( std::size_t n );

/// shellSort() with the given gaps, increasing and starting from 1
/// \pre the gaps are positive and increase
/// \post A holds the same items, sorted if the first gap is 1
template< class T , class C = std::less<> >
void shellSort( std::vector< T > & A ,
                const std::vector< std::size_t > & gaps , C less = C() );

/// shellSort() of the book, the gaps 1 , 4 , 13 , ...
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
