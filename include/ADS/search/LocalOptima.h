/*--------------------------------------------------------------------------*/
/*--------------------------- File LocalOptima.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The local optima of a vector and of a binary tree, found without looking
 * at all the items. A <em>peak</em> of \f$ A[ 0 .. n - 1 ] \f$ is an item not
 * smaller than its neighbours, the items out of \f$ A \f$ counting as smaller
 * than any item, and a <em>local minimum</em> is an item not larger than its
 * neighbours; the maximum is a peak, so that a peak always exists, but
 * finding the maximum needs \f$ n - 1 \f$ comparisons while a peak needs only
 * a logarithmic number of them.
 *
 * - `findPeak()`: a binary search on the invariant that
 *   \f$ A[ \mathit{lo} .. \mathit{hi} ] \f$ holds a peak of \f$ A \f$, true
 *   when \f$ A[ \mathit{lo} - 1 ] < A[ \mathit{lo} ] \f$ (or
 *   \f$ \mathit{lo} = 0 \f$) and \f$ A[ \mathit{hi} ] \f$ is not smaller than
 *   \f$ A[ \mathit{hi} + 1 ] \f$ (or \f$ \mathit{hi} = n - 1 \f$): the middle
 *   item \f$ A[ m ] \f$ is compared with \f$ A[ m + 1 ] \f$, and the search
 *   goes on in \f$ A[ m + 1 .. \mathit{hi} ] \f$ if it is smaller and in
 *   \f$ A[ \mathit{lo} .. m ] \f$ otherwise, both keeping the invariant;
 *   \f$ \lceil \log_2 n \rceil \f$ comparisons, and \f$ \Omega( \log n ) \f$
 *   are needed in the worst case. With distinct items the peak is strict;
 * - `localMinimum()` of a vector: `findPeak()` with the order reversed;
 * - `localMinimum()` of a binary tree: a node not larger than its parent and
 *   its children, found by going down from the root to the first child
 *   smaller than the current node, until no child is smaller; each node
 *   reached is smaller than its parent, the last one is not larger than
 *   its children, \f$ O( h ) \f$ comparisons for a tree of height \f$ h \f$,
 *   i.e., \f$ O( \log n ) \f$ on a complete tree.
 *
 * The same idea works in two dimensions, where a peak of an
 * \f$ n \times n \f$ matrix costs \f$ \Theta( n ) \f$ probes by the search on
 * the middle row and the middle column alternately, but not on general
 * graphs: on the hypercube of dimension \f$ d \f$, Aldous showed that every
 * randomized algorithm needs about \f$ 2^{d / 2} \f$ evaluations to find a
 * local minimum.
 *
 * The contracts [see `Contract.h`]: `findPeak()` and `localMinimum()` of a
 * vector need a vector that is not empty and `localMinimum()` of a tree a
 * tree; the result is checked against its neighbours, a postcondition. The
 * functions are defined in `LocalOptima.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera and Esercizi Alberi, Raccolte da compiti scritti
 * \see D. Aldous, Minimization algorithms and random walk on the d-cube,
 *      The Annals of Probability 11(2), 403-413, 1983,
 *      doi:10.1214/aop/1176993605
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

#ifndef __ADS_LocalOptima
 #define __ADS_LocalOptima
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"
#include "ADS/tree/BinaryTree.h"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place \f$ r \f$ of an item of \f$ A \f$ not smaller than its
/// neighbours, by a binary search; \f$ \lceil \log_2 n \rceil \f$
/// comparisons
///
/// In pseudocode (Esercizi Divide-et-impera of the book), the invariant
/// being that \f$ A[ \mathit{lo} .. \mathit{hi} ] \f$ holds a peak of
/// \f$ A \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{findPeak}( A ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n - 1 \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad m \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo} ) / 2
///     \rfloor \cr
/// 5 & \qquad \textbf{if}\ A[ m ] < A[ m + 1 ]\ \textbf{then}\ \mathit{lo}
///     \gets m + 1\ \textbf{else}\ \mathit{hi} \gets m \cr
/// 6 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty
/// \post \f$ A[ r ] \f$ not smaller than \f$ A[ r - 1 ] \f$ and
///       \f$ A[ r + 1 ] \f$, where they are
template< class T , class C = std::less<> >
std::size_t findPeak( const std::vector< T > & A , C less = C() );

/// the place \f$ r \f$ of an item of \f$ A \f$ not larger than its
/// neighbours, by a binary search; \f$ \lceil \log_2 n \rceil \f$
/// comparisons
///
/// In pseudocode (Esercizi Divide-et-impera of the book), the peak of the
/// reversed order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{localMinimum}( A ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{findPeak}( A ) \text{ with } >
///     \text{ in place of } <
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty
/// \post \f$ A[ r ] \f$ not larger than \f$ A[ r - 1 ] \f$ and
///       \f$ A[ r + 1 ] \f$, where they are
template< class T , class C = std::less<> >
std::size_t localMinimum( const std::vector< T > & A , C less = C() );

/// a node of the tree \f$ t \f$ not larger than its children and, if it is
/// not \f$ t \f$, smaller than its parent: from \f$ t \f$ down to the first
/// child smaller than the current node, the left one first; \f$ O( h ) \f$
/// comparisons
///
/// In pseudocode (Esercizi Alberi of the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{localMinimum}( t ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \textbf{if}\ t.\mathit{left} \ne \textbf{nil}\ \textbf{and}
///     \ t.\mathit{left}.\mathit{key} < t.\mathit{key}\ \textbf{then}\ t \gets
///     t.\mathit{left} \cr
/// 4 & \qquad \textbf{else if}\ t.\mathit{right} \ne \textbf{nil}
///     \ \textbf{and}\ t.\mathit{right}.\mathit{key} < t.\mathit{key}
///     \ \textbf{then}\ t \gets t.\mathit{right} \cr
/// 5 & \qquad \textbf{else return}\ t
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is not nil
/// \post no child of \f$ r \f$ smaller than \f$ r \f$, and \f$ r \f$ smaller
///       than its parent if \f$ r \f$ is not \f$ t \f$
template< class T , class C = std::less<> >
BinaryTree< T > * localMinimum( BinaryTree< T > * t , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/LocalOptima.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LocalOptima.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File LocalOptima.h -------------------------*/
/*--------------------------------------------------------------------------*/
