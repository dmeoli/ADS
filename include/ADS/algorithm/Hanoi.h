/*--------------------------------------------------------------------------*/
/*------------------------------ File Hanoi.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The towers of Hanoi: moving \f$ n \f$ disks from the peg `source` to the
 * peg `dest`, one at a time and never on a smaller one, with the peg `middle`
 * as a spare. The recursive procedure moves \f$ n - 1 \f$ disks on `middle`,
 * the last one on `dest` and the \f$ n - 1 \f$ disks on `dest`, hence it
 * makes
 * \f[
 *   T( n ) = 2 T( n - 1 ) + 1 = 2^n - 1
 * \f]
 * moves. The two iterative procedures of the book simulate its calls with a
 * STACK of their parameters: the first one by hand, the second one by the
 * systematic rules that also save the point where a call resumes (`START`,
 * `AFTER1`, `END`, `STOP`). All of them hand each move to a function
 * `f( from , to )`, so that the sequence of moves can be checked.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 1 \f$ disks and three distinct
 * pegs. The functions are defined in `Hanoi.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_Hanoi
 #define __ADS_Hanoi
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/ArrayStack.h"

#include <tuple>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the moves of \f$ n \ge 1 \f$ disks from `source` to `dest`, by the
/// recursion of the book (its `hanoi-recursive()`)
///
/// In pseudocode (section 4.4):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{hanoi}( n , \mathit{src} , \mathit{dest} ,
///     \mathit{mid} ) \cr
/// 2 & \quad \textbf{if}\ n = 1\ \textbf{then}\ \mathrm{f}( \mathit{src} ,
///     \mathit{dest} ) \cr
/// 3 & \quad \textbf{else} \cr
/// 4 & \qquad \mathrm{hanoi}( n - 1 , \mathit{src} , \mathit{mid} ,
///     \mathit{dest} )\quad \triangleright\ \text{the call (1)} \cr
/// 5 & \qquad \mathrm{f}( \mathit{src} , \mathit{dest} ) \cr
/// 6 & \qquad \mathrm{hanoi}( n - 1 , \mathit{mid} , \mathit{dest} ,
///     \mathit{src} )\quad \triangleright\ \text{the call (2)}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$, and `source`, `dest` and `middle` distinct
template< class F >
void hanoi_recursive( int n , int source , int dest , int middle , F && f );

/*--------------------------------------------------------------------------*/

/// the moves of `hanoi_recursive()` with the calls on a stack (the first
/// `hanoi-iterative()` of the book): the calls (1) are pushed, and each pop
/// resumes the call left open, moving a disk and simulating the call (2)
///
/// In pseudocode (section 4.4), the call (2) being the last one, a tail
/// call turned into a change of the parameters:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{hanoiIterative}( n , \mathit{src} ,
///     \mathit{dest} , \mathit{mid} ) \cr
/// 2 & \quad S \gets \text{an empty stack} \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{while}\ n \ne 1\ \textbf{do} \cr
/// 5 & \qquad\quad \mathrm{push}( S , ( n , \mathit{src} , \mathit{dest} ,
///     \mathit{mid} ) );\ n \gets n - 1;\ \mathit{dest} \leftrightarrow
///     \mathit{mid} \cr
/// 6 & \qquad \mathrm{f}( \mathit{src} , \mathit{dest} ) \cr
/// 7 & \qquad \textbf{if}\ S = \emptyset\ \textbf{then return} \cr
/// 8 & \qquad ( n , \mathit{src} , \mathit{dest} , \mathit{mid} ) \gets
///     \mathrm{pop}( S );\ \mathrm{f}( \mathit{src} , \mathit{dest} ) \cr
/// 9 & \qquad n \gets n - 1;\ \mathit{src} \leftrightarrow \mathit{mid}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$, and `source`, `dest` and `middle` distinct
template< class F >
void hanoi_iterative( int n , int source , int dest , int middle , F && f );

/*--------------------------------------------------------------------------*/

/// the moves of `hanoi_recursive()` by the systematic rules (the second
/// `hanoi-iterative()` of the book), saving on the stack the parameters and
/// the label from which the call resumes
///
/// In pseudocode (section 4.4), a call pushing the label after it and a
/// return popping it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{hanoiLabels}( n , \mathit{src} ,
///     \mathit{dest} , \mathit{mid} ) \cr
/// 2 & \quad S \gets \text{an empty stack};\ \mathit{pos} \gets \text{START}
///     \cr
/// 3 & \quad \textbf{while}\ \mathit{pos} \ne \text{STOP}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{pos} = \text{START}\ \textbf{and}\ n = 1
///     \ \textbf{then}\ \mathrm{f}( \mathit{src} , \mathit{dest} );
///     \ \mathit{pos} \gets \text{END} \cr
/// 5 & \qquad \textbf{else if}\ \mathit{pos} = \text{START}\ \textbf{then} \cr
/// 6 & \qquad\quad \mathrm{push}( S , ( n , \mathit{src} , \mathit{dest} ,
///     \mathit{mid} , \text{AFTER1} ) );\ n \gets n - 1;\ \mathit{dest}
///     \leftrightarrow \mathit{mid} \cr
/// 7 & \qquad \textbf{else if}\ \mathit{pos} = \text{AFTER1}\ \textbf{then}
///     \cr
/// 8 & \qquad\quad \mathrm{f}( \mathit{src} , \mathit{dest} );\ \mathrm{push}(
///     S , ( n , \mathit{src} , \mathit{dest} , \mathit{mid} , \text{END} ) )
///     \cr
/// 9 & \qquad\quad n \gets n - 1;\ \mathit{src} \leftrightarrow \mathit{mid};
///     \ \mathit{pos} \gets \text{START} \cr
/// 10 & \qquad \textbf{else if}\ S = \emptyset\ \textbf{then}\ \mathit{pos}
///     \gets \text{STOP} \cr
/// 11 & \qquad \textbf{else}\ ( n , \mathit{src} , \mathit{dest} ,
///     \mathit{mid} , \mathit{pos} ) \gets \mathrm{pop}( S )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$, and `source`, `dest` and `middle` distinct
template< class F >
void hanoi_iterative_labels( int n , int source , int dest , int middle ,
                             F && f );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/Hanoi.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Hanoi.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Hanoi.h ----------------------------*/
/*--------------------------------------------------------------------------*/
