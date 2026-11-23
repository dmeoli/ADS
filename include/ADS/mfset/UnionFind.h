/*--------------------------------------------------------------------------*/
/*---------------------------- File UnionFind.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET of section 10.2 engineered, beyond the book [see `MFSet.h`]: the
 * forest of the parents in one vector of integers, where a root keeps minus
 * the size of its tree instead of a parent, so that the union by size needs no
 * second vector and a `find()` touches one array; `find()` halves the path
 * (every other node on the way is made a child of its grandparent), which
 * needs a single pass, no recursion and no stack, and keeps the bound of the
 * compression with the union by size (Tarjan and van Leeuwen): a sequence of
 * \f$ m \geq n \f$ operations on \f$ n \f$ elements takes
 * \f[
 *   O( n + m \, \alpha( m , n ) ) ,
 * \f]
 * \f$ \alpha \f$ the inverse of the function of Ackermann, at most 4 for any
 * \f$ n \f$ that fits a computer; a tree of root \f$ r \f$ has at least
 * \f$ 2^h \f$ nodes, \f$ h \f$ its height, so that a single `find()` is
 * \f$ O( \log n ) \f$ in the worst case. The space is \f$ 4 n \f$ bytes.
 * Patwary, Blair and Manne found the union by size or rank with halving
 * among the fastest variants, close to the splicing of Rem, which the
 * benchmark of this library finds slower up to \f$ 10^6 \f$ elements. It is
 * the union-find of the observer of the spanning forests [see
 * `Reoptimization.h`], while `kruskal()` takes any MFSET with `merge()`.
 *
 * The forest tells its changes through the subject policy `Policy` [see
 * `Modification.h`], `NoObservers` by default, with which every operation
 * compiles to the code of the plain forest: `merge()` of two components
 * sends `Changed< Parent , int , int >` with the handle \f$ x \f$ of the
 * root made a child, from \f$ -1 \f$ to the root \f$ y \f$ of the other
 * tree (after the link, undone if an observer throws), and `reset()`
 * sends `Rebuilt`. Only the links of two roots are told: the shortening of
 * the paths by `find()` is internal maintenance, which leaves every
 * component as it was, so that an observer that keeps a parent for each
 * element from these events alone holds a forest with the components of
 * the structure, although not its shape.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 0 \f$, the elements are in
 * \f$ [ 0 , n ) \f$; the invariant is that the parents make a forest of
 * `count()` trees whose roots hold minus their sizes. The operations are
 * defined in `UnionFind.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
 * \see M. M. A. Patwary, J. Blair, F. Manne, Experiments on union-find
 *      algorithms for the disjoint-set data structure, Proceedings of SEA,
 *      LNCS 6049, 411-423, 2010, doi:10.1007/978-3-642-13193-6_35
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

#ifndef __ADS_UnionFind
 #define __ADS_UnionFind
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/Portable.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DisjointSets ---------------------------*/
/*--------------------------------------------------------------------------*/

/// how the roots of two trees are joined: the smaller tree under the root
/// of the larger, or the lower rank under the higher
enum class Union {
 SIZE ,  ///< the smaller tree under the root of the larger
 RANK   ///< the lower rank under the higher
 };

/// how `find()` shortens the path: halving it (every other node on the way
/// made a child of its grandparent, one pass), or compressing it (all the
/// nodes on the way made children of the root, two passes)
enum class Compression {
 HALVING ,  ///< half the path under grandparents
 FULL      ///< each node under the root
 };

/// the union of disjoint sets of \f$ [ 0 , n ) \f$, the roots joined by `L`,
/// the paths shortened by `C`, the links of the roots told through `Policy`
template< Union L = Union::SIZE , Compression C = Compression::HALVING ,
	  class Policy = NoObservers >
class DisjointSets
{

/*--------------------------------------------------------------------------*/
 public:
 /// the link of a root under another root: the parent of the handle from
 /// \f$ -1 \f$ to the other root
 using Linked = mod::Changed< mod::Parent , int , int >;
 /// the events: a root linked under another, the forest made anew
 using modifications = mod::list< Linked , mod::Rebuilt >;
 /// the subject
 using Subject = typename Policy::template bind< modifications ,
						 DisjointSets >;

 /// the \f$ n \f$ components \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$
 /// \pre \f$ n \ge 0 \f$
 explicit DisjointSets( int n = 0 );

 /// the \f$ n \f$ components \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$, the
 /// static observers `o...`
 /// \pre \f$ n \ge 0 \f$
 template< class... O >
  requires( sizeof...( O ) > 0 )
 DisjointSets( int n , O &... o );

 /// back to the \f$ n \f$ components
 /// \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$
 /// \pre \f$ n \ge 0 \f$
 void reset( int n );

 /// the number \f$ n \f$ of elements
 [[nodiscard]] int n( void ) const { return( int( f_p.size() ) ); }

 /// the number of components
 [[nodiscard]] int count( void ) const { return( f_count ); }

 /// the representative of the component of \f$ x \f$
 ///
 /// In pseudocode (the halving of Tarjan and van Leeuwen, beyond the book),
 /// \f$ p[ x ] < 0 \f$ at a root:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( x ) \cr
 /// 2 & \quad \textbf{while}\ p[ x ] \ge 0\ \textbf{do} \cr
 /// 3 & \qquad y \gets p[ x ];\ \textbf{if}\ p[ y ] < 0\ \textbf{then
 ///     return}\ y \cr
 /// 4 & \qquad p[ x ] \gets p[ y ];\ x \gets p[ y ]\quad \triangleright
 ///     \ x \text{ under its grandparent} \cr
 /// 5 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// and with `FULL` the two passes of `MFSet::find()`.
 /// \pre \f$ x \in [ 0 , n ) \f$
 int find( int x );

 /// joins the components of \f$ x \f$ and \f$ y \f$, the smaller (of lower
 /// rank) under the larger, under the root of \f$ y \f$ if they tie; true if
 /// they were distinct
 ///
 /// In pseudocode (section 10.2.3, the union by size), a root \f$ r \f$
 /// holding \f$ p[ r ] = -\mathit{size}[ r ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( x , y ) \cr
 /// 2 & \quad x \gets \mathrm{find}( x );\ y \gets \mathrm{find}( y );
 ///     \ \textbf{if}\ x = y\ \textbf{then return false} \cr
 /// 3 & \quad \textbf{if}\ p[ y ] > p[ x ]\ \textbf{then}\ x \leftrightarrow
 ///     y\quad \triangleright\ \text{now } y \text{ is the larger tree} \cr
 /// 4 & \quad p[ y ] \gets p[ y ] + p[ x ];\ p[ x ] \gets y;
 ///     \ \textbf{return true}
 /// \end{array}}
 /// \f]
 /// and with `RANK` a root holds \f$ -1 - \mathit{rank}[ r ] \f$, the rank
 /// growing by 1 on a tie.
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 bool merge( int x , int y );

 /// whether \f$ x \f$ and \f$ y \f$ are in the same component
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 bool same( int x , int y );

 /// the number of elements of the component of \f$ x \f$
 /// \pre \f$ x \in [ 0 , n ) \f$, the union by size
 int size( int x )
  requires( L == Union::SIZE );

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class O >
 int attach( O & o )
  requires( Subject::dynamic )
 {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open
 void detach( int id )
  requires( Subject::dynamic )
 {
  f_sub.detach( id );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the root \f$ x \f$ under the root \f$ y \f$, \f$ y \f$ the larger, as
 /// `merge()` does with nobody listening
 void link( int x , int y );

 /// the link of \f$ x \f$ under \f$ y \f$ when someone listens
 void linkTold( int x , int y );

 /// the link when someone listens, out of the hot code
 ADS_COLD_PATH void linkCold( int x , int y ) { linkTold( x , y ); }

 /// a forest of `count()` trees, the roots holding minus their sizes (or
 /// minus one less their ranks)
 [[nodiscard]] bool forest( void ) const;

 std::vector< int > f_p;  ///< the parents, the size or rank at a root
 int f_count = 0;         ///< the number of components
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class DisjointSets )

/// the union by size with path halving
using UnionFind = DisjointSets<>;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/UnionFind.tpp"

/*--------------------------------------------------------------------------*/

#endif /* UnionFind.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File UnionFind.h --------------------------*/
/*--------------------------------------------------------------------------*/
