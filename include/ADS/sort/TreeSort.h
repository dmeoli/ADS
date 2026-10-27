/*--------------------------------------------------------------------------*/
/*---------------------------- File TreeSort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree sort (exercise 6.1 and its solution): the items are inserted in a
 * search tree [see `SearchTree.h`] and read back by the symmetric visit, which
 * gives the keys in increasing order; each key keeps the number of its copies,
 * so that the repeated items are not lost. On a Red-Black tree the insertions
 * take \f$ O( n \log n ) \f$ time in the worst case, and on a binary search
 * tree with no balance \f$ \Theta( n^2 ) \f$ on a sorted input, which is why
 * the sort is the example of the cost of the balance; it is also the natural
 * sort of a functional language, which builds the tree by recursion and
 * flattens it.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same items
 * sorted, an audit. The function is defined in `TreeSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 6.1 and
 *      exercise 6.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
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

#ifndef __ADS_TreeSort
 #define __ADS_TreeSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"
#include "ADS/tree/SearchTree.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the tree sort on a Red-Black tree, or on a search tree with no balance if
/// `balanced` is false
///
/// In pseudocode (the search trees of section 6.1 and exercise 6.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{treeSort}( A ) \cr
/// 2 & \quad S \gets \text{an empty search tree of pairs } ( \mathit{key} ,
///     \mathit{count} ) \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{do} \cr
/// 4 & \qquad c \gets S.\mathrm{lookup}( x ) \cr
/// 5 & \qquad \textbf{if}\ c = \textbf{nil}\ \textbf{then}\ S.\mathrm{insert}(
///     x , 1 )\ \textbf{else}\ S.\mathrm{insert}( x , c + 1 ) \cr
/// 6 & \quad i \gets 0 \cr
/// 7 & \quad \textbf{foreach}\ ( k , c ) \in S \text{ in symmetric order}
///     \ \textbf{do} \cr
/// 8 & \qquad \text{write } k \text{ in } A[ i .. i + c - 1 ];\ i \gets i + c
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< std::totally_ordered T >
void treeSort( std::vector< T > & A , bool balanced = true );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/TreeSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TreeSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File TreeSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
