/*--------------------------------------------------------------------------*/
/*--------------------------- File LocalOptima.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The local optima of a vector and of a binary tree, found without looking
 * at all the items. A peak of A[ 0 .. n - 1 ] is an item not smaller than
 * its neighbours, the items out of A counting as smaller than any item, and
 * a local minimum is an item not larger than its neighbours; the maximum is
 * a peak, so that a peak always exists, but finding the maximum needs n - 1
 * comparisons while a peak needs only a logarithmic number of them.
 *
 * - findPeak(): a binary search on the invariant that A[ lo .. hi ] holds a
 *   peak of A, true when A[ lo - 1 ] < A[ lo ] (or lo = 0) and A[ hi ] is
 *   not smaller than A[ hi + 1 ] (or hi = n - 1): the middle item A[ m ] is
 *   compared with A[ m + 1 ], and the search goes on in A[ m + 1 .. hi ] if
 *   it is smaller and in A[ lo .. m ] otherwise, both keeping the
 *   invariant; \f$ \lceil \log_2 n \rceil \f$ comparisons, and
 *   \f$ \Omega( \log n ) \f$ are needed in the worst case. With distinct
 *   items the peak is strict;
 * - localMinimum() of a vector: findPeak() with the order reversed;
 * - localMinimum() of a binary tree: a node not larger than its parent and
 *   its children, found by going down from the root to the first child
 *   smaller than the current node, until no child is smaller; each node
 *   reached is smaller than its parent, the last one is not larger than
 *   its children, \f$ O( h ) \f$ comparisons for a tree of height h, i.e.,
 *   \f$ O( \log n ) \f$ on a complete tree.
 *
 * The same idea works in two dimensions, where a peak of an n x n matrix
 * costs \f$ \Theta( n ) \f$ probes by the search on the middle row and the
 * middle column alternately, but not on general graphs: on the hypercube
 * of dimension d, Aldous showed that every randomized algorithm needs
 * about \f$ 2^{d / 2} \f$ evaluations to find a local minimum.
 *
 * The contracts [see Contract.h]: findPeak() and localMinimum() of a vector
 * need a vector that is not empty and localMinimum() of a tree a tree; the
 * result is checked against its neighbours, a postcondition. The functions
 * are defined in LocalOptima.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera, Raccolte da compiti scritti
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

/// findPeak(): the place r of an item of A not smaller than its
/// neighbours, by a binary search; \f$ \lceil \log_2 n \rceil \f$
/// comparisons
/// \pre A is not empty
/// \post A[ r ] not smaller than A[ r - 1 ] and A[ r + 1 ], where they are
template< class T , class C = std::less<> >
std::size_t findPeak( const std::vector< T > & A , C less = C() );

/// localMinimum(): the place r of an item of A not larger than its
/// neighbours, by a binary search; \f$ \lceil \log_2 n \rceil \f$
/// comparisons
/// \pre A is not empty
/// \post A[ r ] not larger than A[ r - 1 ] and A[ r + 1 ], where they are
template< class T , class C = std::less<> >
std::size_t localMinimum( const std::vector< T > & A , C less = C() );

/// localMinimum(): a node of the tree t not larger than its children and, if
/// it is not t, smaller than its parent: from t down to the first child
/// smaller than the current node, the left one first; \f$ O( h ) \f$
/// comparisons
/// \pre t is not nil
/// \post no child of r smaller than r, and r smaller than its parent if
///       r is not t
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
