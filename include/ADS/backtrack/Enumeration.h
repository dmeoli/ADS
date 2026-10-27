/*--------------------------------------------------------------------------*/
/*--------------------------- File Enumeration.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The enumeration of the feasible solutions by backtrack (section 16.2): a
 * solution is a vector \f$ S[ 0 .. n - 1 ] \f$, each \f$ S[ i ] \f$ taken from
 * the choices that \f$ S[ 0 .. i - 1 ] \f$ allows; each choice is tried, the
 * solutions are processed, and the search goes on recursively from
 * \f$ S[ 0 .. i ] \f$, i.e., a visit in depth of the tree of the choices. The
 * processing returns true to stop at the first solution.
 *
 * - `subsets()`, the subsets of \f$ k \f$ of the \f$ n \f$ items, by the
 *   choices in or out, pruned when \f$ k \f$ items are in or when the items
 *   left are too few;
 * - `permutations()` and `derangements()`, each item not chosen yet, the
 *   latter not in its own place (exercises 16.2 and 16.3);
 * - `subsetSum()`, the subsets of positive items of sum \f$ k \f$, pruned when
 *   the sum exceeds \f$ k \f$ (exercise 16.4).
 *
 * Each calls `f( S )` on the solutions, in increasing order of the choices,
 * and returns how many there are, or stops when `f` returns true. The
 * subsets of \f$ k \f$ items are \f$ \binom{n}{k} = n! / ( k! ( n - k )! ) \f$
 * (the text writes \f$ n! / ( n - k )! \f$, the number of their orderings).
 *
 * The contracts [see `Contract.h`]: \f$ n \f$ and \f$ k \f$ are not negative,
 * and the items of `subsetSum()` are positive. The operations are defined in
 * `Enumeration.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.2 and
 *      exercises 16.2-16.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#ifndef __ADS_Enumeration
 #define __ADS_Enumeration
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include "ADS/Contract.h"

#include <algorithm>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the subsets of \f$ k \f$ of the items \f$ 0 , \ldots , n - 1 \f$, as
/// vectors of \f$ n \f$ booleans, `f( S )` on each and true from it to stop;
/// the number visited
///
/// In pseudocode (section 16.2), each item in or out of \f$ S \f$, the vector
/// \f$ S[ 0 .. n - 1 ] \f$ false at the start:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{subsets}( S , i , c )\quad \triangleright
///     \ \text{called as } \mathrm{subsets}( S , 0 , 0 ) \cr
/// 2 & \quad \textbf{if}\ c = k\ \textbf{then return}\ f( S )\quad
///     \triangleright\ \text{a solution, } c \text{ items in} \cr
/// 3 & \quad \textbf{if}\ i = n\ \textbf{or}\ c + ( n - i ) < k
///     \ \textbf{then return false}\quad \triangleright\ \text{pruned} \cr
/// 4 & \quad S[ i ] \gets \textbf{true} \cr
/// 5 & \quad \textbf{if}\ \mathrm{subsets}( S , i + 1 , c + 1 )
///     \ \textbf{then return true} \cr
/// 6 & \quad S[ i ] \gets \textbf{false} \cr
/// 7 & \quad \textbf{return}\ \mathrm{subsets}( S , i + 1 , c )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$ and \f$ k \ge 0 \f$
template< class F >
long subsets( int n , int k , F && f );

namespace detail {

/// the permutations of \f$ A \f$, or its derangements if `derange`
template< class T , class F >
long permute( const std::vector< T > & A , bool derange , F && f );

}  // end( namespace detail )

/// the permutations of the items of \f$ A \f$, `f( S )` on each and true from
/// it to stop; the items are told apart by their places, so that equal items
/// give each permutation of the values more than once, \f$ n! \f$ in all
///
/// In pseudocode (exercise 16.2), \f$ S \f$ empty and no item used at the
/// start:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{permutations}( A , S ) \cr
/// 2 & \quad \textbf{if}\ |S| = n\ \textbf{then return}\ f( S ) \cr
/// 3 & \quad \textbf{for}\ c \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if not}\ \mathit{used}[ c ]\ \textbf{then} \cr
/// 5 & \qquad\quad \mathit{used}[ c ] \gets \textbf{true};\ \text{append }
///     A[ c ] \text{ to } S \cr
/// 6 & \qquad\quad \textbf{if}\ \mathrm{permutations}( A , S )
///     \ \textbf{then return true} \cr
/// 7 & \qquad\quad \text{remove the last item of } S;\ \mathit{used}[ c ]
///     \gets \textbf{false} \cr
/// 8 & \quad \textbf{return false}
/// \end{array}}
/// \f]
template< class T , class F >
long permutations( const std::vector< T > & A , F && f );

/// the derangements of the items of \f$ A \f$, no item in its own place,
/// `f( S )` on each and true from it to stop; the places are deranged, not
/// the values, as in `permutations()`
///
/// In pseudocode (exercise 16.3), the procedure of `permutations()` with the
/// item of place \f$ |S| \f$ excluded from the place \f$ |S| \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{derangements}( A , S ) \cr
/// 2 & \quad \textbf{if}\ |S| = n\ \textbf{then return}\ f( S ) \cr
/// 3 & \quad \textbf{for}\ c \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if not}\ \mathit{used}[ c ]\ \textbf{and}\ c \ne |S|
///     \ \textbf{then} \cr
/// 5 & \qquad\quad \mathit{used}[ c ] \gets \textbf{true};\ \text{append }
///     A[ c ] \text{ to } S \cr
/// 6 & \qquad\quad \textbf{if}\ \mathrm{derangements}( A , S )
///     \ \textbf{then return true} \cr
/// 7 & \qquad\quad \text{remove the last item of } S;\ \mathit{used}[ c ]
///     \gets \textbf{false} \cr
/// 8 & \quad \textbf{return false}
/// \end{array}}
/// \f]
template< class T , class F >
long derangements( const std::vector< T > & A , F && f );

/// the subsets of the positive items of \f$ A \f$ of sum \f$ k \f$, as the
/// places taken, `f( S )` on each and true from it to stop; \f$ k = 0 \f$
/// gives the empty subset only, and equal items in different places give
/// different subsets
///
/// In pseudocode (exercise 16.4): the items being positive, a sum that
/// exceeds \f$ k \f$ cannot come back, and the branch is pruned.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{subsetSum}( A , S , i , s )\quad
///     \triangleright\ \text{called as } \mathrm{subsetSum}( A , () , 0 , 0 )
///     \cr
/// 2 & \quad \textbf{if}\ s = k\ \textbf{then return}\ f( S ) \cr
/// 3 & \quad \textbf{for}\ j \gets i\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ s + A[ j ] \le k\ \textbf{then} \cr
/// 5 & \qquad\quad \text{append } j \text{ to } S \cr
/// 6 & \qquad\quad \textbf{if}\ \mathrm{subsetSum}( A , S , j + 1 , s + A[ j ]
///     )\ \textbf{then return true} \cr
/// 7 & \qquad\quad \text{remove the last item of } S \cr
/// 8 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre the items of \f$ A \f$ are positive
template< class T , class F >
long subsetSum( const std::vector< T > & A , T k , F && f );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/Enumeration.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Enumeration.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Enumeration.h -------------------------*/
/*--------------------------------------------------------------------------*/
