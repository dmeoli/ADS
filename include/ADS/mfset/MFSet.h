/*--------------------------------------------------------------------------*/
/*------------------------------ File MFSet.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET of section 10.2, the union of disjoint sets: `MFSet( n )` makes
 * the \f$ n \f$ components \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$,
 * `find( x )` gives the representative of the component of \f$ x \f$, and
 * `merge( x , y )` joins the components of \f$ x \f$ and \f$ y \f$, if they
 * are distinct.
 *
 * `MFSet` keeps the components as a forest, a vector of parents \f$ p \f$,
 * the roots with \f$ p[ x ] = x \f$ being the representatives (section
 * 10.2.2): `find()` goes up to the root, `merge()` makes a root a child of
 * the other. Two heuristics may be switched off, to study them:
 *
 * - the union by rank (section 10.2.3): \f$ \mathit{rank}[ r ] \f$ is the
 *   height of the tree of the root \f$ r \f$, the lower tree goes under the
 *   higher one and the rank grows by one only when they are equal; by
 *   Theorem 10.1 a tree of root \f$ r \f$ has at least
 *   \f$ 2^{\mathit{rank}[ r ]} \f$ nodes, so `find()` and `merge()` take
 *   \f$ O( \log n ) \f$;
 * - the union by size, in place of the rank: \f$ \mathit{size}[ r ] \f$ is
 *   the number of nodes of the tree of the root \f$ r \f$, and the smaller
 *   tree goes under the larger one (on a tie, the root of \f$ y \f$ under
 *   that of \f$ x \f$). A node goes one level down only when its tree at
 *   least doubles, hence a tree of root \f$ r \f$ has height at most
 *   \f$ \log_2 \mathit{size}[ r ] \f$, and the bound \f$ O( \log n ) \f$ is
 *   the same;
 * - the compression of the paths (section 10.2.4): `find()` makes each node
 *   on its way a child of the root. With both, a sequence of \f$ m \f$
 *   operations, \f$ m \geq n \f$, takes \f$ O( n + m \alpha( m , n ) ) \f$,
 *   where \f$ \alpha \f$, the inverse of the function of Ackermann, is at
 *   most 4 for every \f$ n \f$ and \f$ m \f$ that fit in a computer. Then
 *   \f$ \mathit{rank}[ r ] \f$ is an upper bound to the height (exercise
 *   10.7).
 *
 * `find()` is the iterative version in two passes, the root first and then
 * the compression, with no stack (exercise 10.8). `ListMFSet.h` has the
 * realization on lists of section 10.2.1. `components()` gives the MFSET of
 * the connected components of an unoriented graph, which can then follow
 * new edges (section 10.2.5).
 *
 * The contracts [see `Contract.h`]: the elements are in \f$ [ 0 , n ) \f$,
 * `find()` gives a root and after `merge()` \f$ x \f$ and \f$ y \f$ have the
 * same root; the invariant is that the parents form a forest of `count()`
 * trees, the rank growing towards the roots with the union by rank, and
 * \f$ \mathit{size}[ r ] \f$ the number of nodes of the tree of each root
 * \f$ r \f$ with the union by size. `merge()` of two elements already in the
 * same component returns false and changes nothing, a relaxation of the
 * precondition that they are in distinct components which some
 * specifications state. The operations are defined in `MFSet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see B. A. Galler, M. J. Fischer, An improved equivalence algorithm,
 *      Communications of the ACM 7(5), 301-303, 1964,
 *      doi:10.1145/364099.364331
 * \see R. E. Tarjan, Efficiency of a good but not linear set union
 *      algorithm, Journal of the ACM 22(2), 215-225, 1975,
 *      doi:10.1145/321879.321884
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
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

#ifndef __ADS_MFSet
 #define __ADS_MFSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS MFSet -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets on a forest
class MFSet {

/*--------------------------------------------------------------------------*/
 public:

 /// how `merge()` chooses the root that goes under the other one
 enum class Union {
  NONE ,   ///< the root of \f$ y \f$ under that of \f$ x \f$
  RANK ,   ///< the lower tree under the higher one
  SIZE     ///< the smaller tree under the larger one
  };

 /// the \f$ n \f$ components \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$ (the
 /// `Mfset( n )` of the book), with the given union and with the compression
 /// of the paths or not
 /// \pre \f$ n \ge 0 \f$
 explicit MFSet( int n , Union u = Union::RANK , bool compress = true );

 /// the number \f$ n \f$ of elements
 [[nodiscard]] int n( void ) const;

 /// the number of components
 [[nodiscard]] int count( void ) const;

 /// the representative of the component of \f$ x \f$, the root of its tree
 ///
 /// In pseudocode (sections 10.2.2 and 10.2.4, the iterative version of
 /// exercise 10.8), the root first and then the compression:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( x ) \cr
 /// 2 & \quad r \gets x \cr
 /// 3 & \quad \textbf{while}\ p[ r ] \ne r\ \textbf{do}\ r \gets p[ r ] \cr
 /// 4 & \quad \textbf{if}\ \mathit{compress}\ \textbf{then} \cr
 /// 5 & \qquad \textbf{while}\ p[ x ] \ne r\ \textbf{do}\ t \gets p[ x ];
 ///     \ p[ x ] \gets r;\ x \gets t \cr
 /// 6 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \in [ 0 , n ) \f$
 /// \post the result is a root
 int find( int x );

 /// joins the components of \f$ x \f$ and \f$ y \f$; true if they were
 /// distinct
 ///
 /// In pseudocode (sections 10.2.2 and 10.2.3), with the union by rank:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( x , y ) \cr
 /// 2 & \quad r_x \gets \mathrm{find}( x );\ r_y \gets \mathrm{find}( y ) \cr
 /// 3 & \quad \textbf{if}\ r_x = r_y\ \textbf{then return false} \cr
 /// 4 & \quad \textbf{if}\ \mathit{rank}[ r_x ] > \mathit{rank}[ r_y ]
 ///     \ \textbf{then}\ p[ r_y ] \gets r_x \cr
 /// 5 & \quad \textbf{else if}\ \mathit{rank}[ r_y ] > \mathit{rank}[ r_x
 ///     ]\ \textbf{then}\ p[ r_x ] \gets r_y \cr
 /// 6 & \quad \textbf{else}\ p[ r_x ] \gets r_y;\ \mathit{rank}[ r_y ]
 ///     \gets \mathit{rank}[ r_y ] + 1 \cr
 /// 7 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// and with the union by size the root of the smaller tree goes under the
 /// other one, whose size grows by that of the smaller tree (the root of
 /// \f$ y \f$ under that of \f$ x \f$ on a tie).
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 /// \post \f$ x \f$ and \f$ y \f$ are in the same component
 bool merge( int x , int y );

 /// true if \f$ x \f$ and \f$ y \f$ are in the same component,
 /// `find( x ) == find( y )`
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 bool same( int x , int y );

 /// the parent of \f$ x \f$, \f$ x \f$ itself if a root
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] int parent( int x ) const;

 /// the rank of \f$ x \f$
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] int rank( int x ) const;

 /// the number of nodes of the tree of \f$ x \f$, if \f$ x \f$ is a root
 /// and the union is by size
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] int size( int x ) const;

 /// the steps up the trees made by `find()` so far
 [[nodiscard]] long steps( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the root of \f$ x \f$, without compressing the path
 [[nodiscard]] int root( int x ) const;

 /// the parents form a forest of `count()` trees, with the rank growing
 /// upwards if the heuristic is on, \f$ O( n ) \f$
 [[nodiscard]] bool forest( void ) const;

 std::vector< int > f_p;      ///< the parents
 std::vector< int > f_rank;   ///< the ranks
 std::vector< int > f_size;   ///< the sizes of the trees, at the roots
 Union f_union;               ///< the heuristic of the union
 bool f_compress;             ///< the compression of the paths
 int f_count;                 ///< the number of components
 long f_steps = 0;            ///< see `steps()`

 };  // end( class MFSet )

/*--------------------------------------------------------------------------*/

/// the MFSET of the connected components of an unoriented graph, built by
/// merging the ends of each edge (the `cc()` of section 10.2.5); its
/// elements are the names \f$ [ 0 , \mathrm{bound}() ) \f$ of \f$ g \f$, so
/// that a name that is not a node of \f$ g \f$, a deleted one, is a
/// component of its own, counted by `count()`
///
/// In pseudocode (section 10.2.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{components}( G ) \cr
/// 2 & \quad M \gets \mathrm{Mfset}( G.\mathrm{bound}() ) \cr
/// 3 & \quad \textbf{foreach}\ u \in V ,\ v \in G.\mathrm{adj}( u )
///     \ \textbf{do}\ M.\mathrm{merge}( u , v ) \cr
/// 4 & \quad \textbf{return}\ M
/// \end{array}}
/// \f]
template< Graph G >
MFSet components( const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/MFSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MFSet.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File MFSet.h ----------------------------*/
/*--------------------------------------------------------------------------*/
