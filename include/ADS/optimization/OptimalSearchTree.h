/*--------------------------------------------------------------------------*/
/*------------------------ File OptimalSearchTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The optimal binary search tree: the keys \f$ 0 , \ldots , n - 1 \f$ are
 * searched with the weights \f$ p_i \f$ (frequencies or probabilities), and
 * the gaps between them, the unsuccessful searches, with the weights
 * \f$ q_0 , \ldots , q_n \f$; the tree of least weighted path length
 * \f$ \sum_i p_i ( d_i + 1 ) + \sum_j q_j d'_j \f$, \f$ d_i \f$ the depth of
 * the key \f$ i \f$ and \f$ d'_j \f$ that of the external node \f$ j \f$, the
 * root at depth 0. With
 * \f$ W( i , j ) = \sum_{i \le l < j} p_l + \sum_{i \le l \le j} q_l \f$ the
 * cost of the keys \f$ i , \ldots , j - 1 \f$ is \f$ C[ i ][ i ] = 0 \f$ and
 * \f[
 *   C[ i ][ j ] = W( i , j ) + \min_{i \le r < j} C[ i ][ r ]
 *                 + C[ r + 1 ][ j ] ,
 * \f]
 * \f$ O( n^3 ) \f$ time as written. Knuth proved that a best root
 * \f$ R[ i ][ j ] \f$ can be looked for between \f$ R[ i ][ j - 1 ] \f$ and
 * \f$ R[ i + 1 ][ j ] \f$, which makes the total time \f$ O( n^2 ) \f$ by a
 * telescoping sum, with \f$ O( n^2 ) \f$ space; Yao derived it from the
 * quadrangle inequality of \f$ W \f$, the largest best root being monotone.
 * `optimalSearchTree()` does so.
 *
 * In the literature: Hu and Tucker, and Garsia and Wachs, build in
 * \f$ O( n \log n ) \f$ the optimal alphabetic tree, where only the
 * \f$ q_j \f$ count (the keys in the leaves); a tree within a constant of the
 * optimum comes in \f$ O( n ) \f$ from the weight-balancing rule of Mehlhorn;
 * the splay trees are within a constant of the optimal static tree without
 * knowing the weights [see `SplayTree.h`].
 *
 * The contracts [see `Contract.h`]: the weights are not negative and
 * \f$ |q| = |p| + 1 \f$ when \f$ q \f$ is given; the tree has
 * \f$ 0 , \ldots , n - 1 \f$ as its symmetric visit and the cost found
 * (checked at level 3). The functions are defined in
 * `OptimalSearchTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13 and the
 *      Esercizi Programmazione dinamica, exercise B4 of 11/01/2012,
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

/// an optimal binary search tree on the keys \f$ 0 , \ldots , n - 1 \f$: its
/// cost, its root and the children of each key, \f$ -1 \f$ for none (and for
/// the root of the empty tree)
template< class T >
struct OptimalSearchTree {
 T cost;                       ///< the weighted path length
 int root;                     ///< the key at the root
 std::vector< int > left;      ///< the left child of each key
 std::vector< int > right;     ///< the right child of each key
 };

/// the optimal binary search tree of the keys of weights \f$ p \f$ and of the
/// gaps of weights \f$ q \f$
///
/// In pseudocode (exercise B4 of 11/01/2012, Chiavi frequenti, of the
/// Esercizi Programmazione dinamica of the book, with the gaps and the bound
/// of Knuth on the roots beyond the book), by
/// increasing length \f$ h \f$ of the range of keys:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{optimalSearchTree}( p , q ) \cr
/// 2 & \quad C[ i ][ i ] \gets 0 \text{ for each } i \cr
/// 3 & \quad \textbf{for}\ h \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - h\ \textbf{do} \cr
/// 5 & \qquad\quad j \gets i + h \cr
/// 6 & \qquad\quad \textbf{if}\ h = 1\ \textbf{then}\ \mathit{lo} ,
///     \mathit{hi} \gets i\ \textbf{else}\ \mathit{lo} \gets R[ i ][ j - 1
///     ];\ \mathit{hi} \gets R[ i + 1 ][ j ] \cr
/// 7 & \qquad\quad R[ i ][ j ] \gets \text{the largest } r \in [ \mathit{lo} ,
///     \mathit{hi} ] \text{ of least } C[ i ][ r ] + C[ r + 1 ][ j ] \cr
/// 8 & \qquad\quad C[ i ][ j ] \gets W( i , j ) + C[ i ][ R[ i ][ j ] ] + C[
///     R[ i ][ j ] + 1 ][ j ] \cr
/// 9 & \quad \textbf{return}\ C[ 0 ][ n ] \text{ and the tree built from } R[
///     0 ][ n ] \text{ down}
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ and \f$ q \f$ not negative, \f$ |q| = |p| + 1 \f$
/// \post a tree whose symmetric visit is \f$ 0 , \ldots , n - 1 \f$, of the
///       cost found (audit)
template< class T >
OptimalSearchTree< T > optimalSearchTree( const std::vector< T > & p ,
                                          const std::vector< T > & q );

/// the tree of `optimalSearchTree()` above, with the gaps of weight 0
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{optimalSearchTree}( p ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{optimalSearchTree}( p , ( 0 , \ldots , 0
///     ) )
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ not negative
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
