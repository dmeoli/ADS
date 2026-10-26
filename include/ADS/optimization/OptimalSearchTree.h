/*--------------------------------------------------------------------------*/
/*------------------------ File OptimalSearchTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The optimal binary search tree: the keys 0 .. n - 1 are searched with
 * the weights p_i (frequencies or probabilities), and the gaps between
 * them, the unsuccessful searches, with the weights q_0 .. q_n; the tree
 * of least weighted path length
 * \f$ \sum_i p_i ( d_i + 1 ) + \sum_j q_j d'_j \f$, d_i the depth of the
 * key i and d'_j that of the external node j, the root at depth 0.
 *
 * With \f$ W( i , j ) = \sum_{i \leq l < j} p_l + \sum_{i \leq l \leq j}
 * q_l \f$ the cost of the keys i .. j - 1 is C[ i ][ i ] = 0 and
 * \f[
 *   C[ i ][ j ] = W( i , j ) + \min_{i \leq r < j} C[ i ][ r ]
 *                 + C[ r + 1 ][ j ] ,
 * \f]
 * O( n^3 ) time as written. Knuth proved that a best root R[ i ][ j ] can
 * be looked for between R[ i ][ j - 1 ] and R[ i + 1 ][ j ], which makes
 * the total time \f$ O( n^2 ) \f$ by a telescoping sum, with O( n^2 )
 * space; Yao derived it from the quadrangle inequality of W, the largest
 * best root being monotone. optimalSearchTree() does so.
 *
 * In the literature: Hu and Tucker, and Garsia and Wachs, build in
 * O( n log n ) the optimal alphabetic tree, where only the q_j count
 * (the keys in the leaves); a tree within a constant of the optimum comes
 * in O( n ) from the weight-balancing rule of Mehlhorn; the splay trees
 * are within a constant of the optimal static tree without knowing the
 * weights [see SplayTree.h].
 *
 * The contracts [see Contract.h]: the weights are not negative and
 * |q| = |p| + 1 when q is given; the tree has 0 .. n - 1 as its
 * symmetric visit and the cost found (checked at level 3). The functions
 * are defined in OptimalSearchTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see D. E. Knuth, Optimum binary search trees, Acta Informatica 1(1),
 *      14-25, 1971, doi:10.1007/BF00264289
 * \see F. F. Yao, Efficient dynamic programming using quadrangle
 *      inequalities, Proc. 12th ACM Symposium on Theory of Computing,
 *      429-435, 1980, doi:10.1145/800141.804691
 * \see T. C. Hu, A. C. Tucker, Optimal computer search trees and
 *      variable-length alphabetical codes, SIAM Journal on Applied
 *      Mathematics 21(4), 514-532, 1971, doi:10.1137/0121057
 * \see K. Mehlhorn, Nearly optimal binary search trees, Acta Informatica
 *      5(4), 287-295, 1975, doi:10.1007/BF00264563
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

#ifndef __ADS_OptimalSearchTree
 #define __ADS_OptimalSearchTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// an optimal binary search tree on the keys 0 .. n - 1: its cost, its
/// root and the children of each key, -1 for none (and for the root of
/// the empty tree)
template< class T >
struct OptimalSearchTree {
 T cost;                       ///< the weighted path length
 int root;                     ///< the key at the root
 std::vector< int > left;      ///< the left child of each key
 std::vector< int > right;     ///< the right child of each key
 };

/// the optimal binary search tree of the keys of weights p and of the gaps
/// of weights q
/// \pre p and q not negative, |q| = |p| + 1
/// \post a tree whose symmetric visit is 0 .. n - 1, of the cost found
template< class T >
OptimalSearchTree< T > optimalSearchTree( const std::vector< T > & p ,
                                          const std::vector< T > & q );

/// the same with the gaps of weight 0
/// \pre p not negative
template< class T >
OptimalSearchTree< T > optimalSearchTree( const std::vector< T > & p );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/OptimalSearchTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* OptimalSearchTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File OptimalSearchTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
