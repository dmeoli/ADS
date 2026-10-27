/*--------------------------------------------------------------------------*/
/*------------------------ File RollbackUnionFind.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET of section 10.2 with the undo of its merges, beyond the book
 * [see `MFSet.h`], and on it the connectivity of a graph whose edges come
 * and go, when all the changes and the questions are known in advance (see
 * \ref theory_mfset_rollback).
 *
 * `RollbackUnionFind` is the forest of the parents with the union by rank
 * and without any compression of the paths, so that `find()` costs
 * \f$ O( \log n ) \f$ in the worst case (Lemma 2 of \ref theory_mfset_union);
 * each `merge()` changes at most two words, which a record on a stack keeps,
 * `undo()` puts them back in \f$ O( 1 ) \f$, and `rollback( t )` undoes the
 * merges back to the checkpoint \f$ t \f$, the number of merges done when
 * `checkpoint()` gave it (a merge of two elements already together leaves a
 * record too, so that the checkpoints count the calls). The space is
 * \f$ 12 n \f$ bytes and 12 bytes per record.
 *
 * `offlineConnectivity()` answers, for a sequence of \f$ k \f$ operations on
 * \f$ n \f$ nodes, each the insertion of an edge, its deletion or a question
 * (whether two nodes are connected, or how many components there are), the
 * questions in their order, by the segment tree over the times of Lemma 4 of
 * \ref theory_mfset_rollback, in \f$ O( k \log k \log n ) \f$ time and
 * \f$ O( k \log k ) \f$ space.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 0 \f$, the elements in
 * \f$ [ 0 , n ) \f$; a rollback goes back to a checkpoint not beyond the
 * merges done, and an `undo()` needs one; a deletion in
 * `offlineConnectivity()` needs the edge alive (of parallel edges, the last
 * inserted dies first), and a loop \f$ [ u , u ] \f$ is allowed and joins
 * nothing. The invariant is that the parents make a forest of `count()`
 * trees, the rank of each root bounding its height, and that the records
 * undo the merges. The operations are defined in `RollbackUnionFind.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see H. Mannila, E. Ukkonen, The set union problem with backtracking,
 *      Proceedings of ICALP, LNCS 226, 236-243, 1986,
 *      doi:10.1007/3-540-16761-7_73
 * \see J. Westbrook, R. E. Tarjan, Amortized analysis of algorithms for set
 *      union with backtracking, SIAM Journal on Computing 18(1), 1-11,
 *      1989, doi:10.1137/0218001
 * \see A. Apostolico, G. F. Italiano, G. Gambosi, M. Talamo, The set union
 *      problem with unlimited backtracking, SIAM Journal on Computing
 *      23(1), 50-70, 1994, doi:10.1137/S0097539789173597
 * \see D. Eppstein, Offline algorithms for dynamic minimum spanning tree
 *      problems, Journal of Algorithms 17(2), 237-250, 1994,
 *      doi:10.1006/jagm.1994.1033
 * \see R. Peng, B. Sandlund, D. D. Sleator, Optimal offline dynamic
 *      2,3-edge/vertex connectivity, Proceedings of WADS, LNCS 11646,
 *      553-565, 2019, doi:10.1007/978-3-030-24766-9_40
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

#ifndef __ADS_RollbackUnionFind
 #define __ADS_RollbackUnionFind
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS RollbackUnionFind -------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets of \f$ [ 0 , n ) \f$ by rank, with no
/// compression, whose merges can be undone
class RollbackUnionFind {

/*--------------------------------------------------------------------------*/
 public:

 /// the \f$ n \f$ components \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$, no
 /// merge done
 /// \pre \f$ n \ge 0 \f$
 explicit RollbackUnionFind( int n = 0 );

 /// back to the \f$ n \f$ components, no merge done
 /// \pre \f$ n \ge 0 \f$
 void reset( int n );

 /// the number of elements
 [[nodiscard]] int n( void ) const { return( int( f_p.size() ) ); }

 /// the number of components
 [[nodiscard]] int count( void ) const { return( f_count ); }

 /// the representative of the component of \f$ x \f$, in
 /// \f$ O( \log n ) \f$
 ///
 /// In pseudocode (section 10.2.2, with no compression):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( x ) \cr
 /// 2 & \quad \textbf{while}\ p[ x ] \ne x\ \textbf{do}\ x \gets p[ x ] \cr
 /// 3 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] int find( int x ) const;

 /// joins the components of \f$ x \f$ and \f$ y \f$, the root of lower
 /// rank under the other, and records it; true if they were distinct
 ///
 /// In pseudocode (section 10.2.3, with the record of the undo, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( x , y ) \cr
 /// 2 & \quad x \gets \mathrm{find}( x );\ y \gets \mathrm{find}( y ) \cr
 /// 3 & \quad \textbf{if}\ x = y\ \textbf{then}\ \mathrm{push}( R ,
 ///     ( \textbf{nil} , \textbf{nil} , \textbf{false} ) );
 ///     \ \textbf{return}\ \textbf{false} \cr
 /// 4 & \quad \textbf{if}\ \mathit{rank}[ x ] > \mathit{rank}[ y ]
 ///     \ \textbf{then}\ \text{swap } x \text{ and } y \cr
 /// 5 & \quad p[ x ] \gets y;\ g \gets \mathit{rank}[ x ] =
 ///     \mathit{rank}[ y ];\ \textbf{if}\ g\ \textbf{then}
 ///     \ \mathit{rank}[ y ] \gets \mathit{rank}[ y ] + 1 \cr
 /// 6 & \quad \mathrm{push}( R , ( x , y , g ) );\ \textbf{return}
 ///     \ \textbf{true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 bool merge( int x , int y );

 /// whether \f$ x \f$ and \f$ y \f$ are in the same component
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 [[nodiscard]] bool same( int x , int y ) const {
  return( find( x ) == find( y ) );
  }

 /// the number of elements of the component of \f$ x \f$
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] int size( int x ) const { return( f_size[ find( x ) ] ); }

 /// the merges done and not undone, the time to roll back to
 [[nodiscard]] int checkpoint( void ) const {
  return( int( f_stack.size() ) );
  }

 /// undoes the last \f$ k \f$ merges
 ///
 /// In pseudocode (beyond the book; each step restores the forest before
 /// the merge that it undoes):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{undo}( k ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ k\ \textbf{do} \cr
 /// 3 & \qquad ( x , y , g ) \gets \mathrm{pop}( R ) \cr
 /// 4 & \qquad \textbf{if}\ x \ne \textbf{nil}\ \textbf{then}
 ///     \ p[ x ] \gets x;\ \textbf{if}\ g\ \textbf{then}
 ///     \ \mathit{rank}[ y ] \gets \mathit{rank}[ y ] - 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le k \le \f$ `checkpoint()`
 void undo( int k = 1 );

 /// undoes the merges back to the checkpoint \f$ t \f$, i.e.,
 /// `undo( checkpoint() - t )`
 /// \pre \f$ 0 \le t \le \f$ `checkpoint()`
 void rollback( int t );

/*--------------------------------------------------------------------------*/
 private:

 /// a merge: the root hung (\f$ -1 \f$ if nothing was joined) and whether
 /// the rank of the other grew
 struct Record {
  int child;                                     ///< the root hung
  int parent;                                    ///< the other root
  bool grew;                                     ///< whether its rank grew
  };

 /// a forest of `count()` trees, each of height at most the rank of its
 /// root and of the size it holds
 [[nodiscard]] bool forest( void ) const;

 std::vector< int > f_p;          ///< the parents, \f$ x \f$ itself at a root
 std::vector< int > f_rank;       ///< the ranks of the roots
 std::vector< int > f_size;       ///< the sizes of the roots
 std::vector< Record > f_stack;   ///< the merges, to undo
 int f_count = 0;                 ///< the number of components

 };  // end( class RollbackUnionFind )

/*--------------------------------------------------------------------------*/
/*----------------------- OFFLINE DYNAMIC CONNECTIVITY ---------------------*/
/*--------------------------------------------------------------------------*/

/// an operation on a graph whose edges come and go
struct ConnectivityOp {
 /// the kinds of operation
 enum Kind {
  INSERT ,       ///< the insertion of the edge \f$ [ u , v ] \f$
  DELETE ,       ///< the deletion of the edge \f$ [ u , v ] \f$
  CONNECTED ,    ///< whether \f$ u \f$ and \f$ v \f$ are connected
  COMPONENTS     ///< how many components there are
  };
 Kind kind;                                      ///< the kind
 int u = 0;                                      ///< the first node
 int v = 0;                                      ///< the second node
 };

/// the answers to the questions of `ops` on \f$ n \f$ nodes, in their
/// order: 1 or 0 for `CONNECTED`, the number of components for
/// `COMPONENTS`
///
/// In pseudocode (the divide and conquer of Eppstein on the times, beyond
/// the book; it is correct by Lemma 4 of \ref theory_mfset_rollback), with
/// \f$ q( t ) \f$ the number of the questions before the time \f$ t \f$ and
/// \f$ L( x ) \f$ the edges of the node \f$ x \f$ of the segment tree:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{offlineConnectivity}( n , \mathit{ops} )
///     \cr
/// 2 & \quad \textbf{foreach}\ \text{edge } e \text{ alive on } [ s , t )
///     \ \textbf{do}\ \mathrm{add}( 1 , [ 0 , q ) , [ q( s ) , q( t ) ) ,
///     e ) \cr
/// 3 & \quad \mathrm{visit}( 1 , [ 0 , q ) );\ \textbf{return}
///     \ \text{the answers} \cr
/// 4 & \textbf{procedure}\ \mathrm{add}( x , [ a , b ) , [ s , t ) , e )
///     \cr
/// 5 & \quad \textbf{if}\ [ s , t ) \cap [ a , b ) = \emptyset
///     \ \textbf{then}\ \textbf{return} \cr
/// 6 & \quad \textbf{if}\ [ a , b ) \subseteq [ s , t )\ \textbf{then}
///     \ L( x ) \gets L( x ) \cup \{ e \};\ \textbf{return} \cr
/// 7 & \quad c \gets \lfloor ( a + b ) / 2 \rfloor;\ \mathrm{add}( 2 x ,
///     [ a , c ) , [ s , t ) , e );\ \mathrm{add}( 2 x + 1 , [ c , b ) ,
///     [ s , t ) , e ) \cr
/// 8 & \textbf{procedure}\ \mathrm{visit}( x , [ a , b ) ) \cr
/// 9 & \quad c_0 \gets \mathrm{checkpoint}() \cr
/// 10 & \quad \textbf{foreach}\ [ u , v ] \in L( x )\ \textbf{do}
///     \ \mathrm{merge}( u , v ) \cr
/// 11 & \quad \textbf{if}\ b - a = 1\ \textbf{then}\ \text{answer the}
///     \text{ question } a \text{ by } \mathrm{find}() \cr
/// 12 & \quad \textbf{else}\ c \gets \lfloor ( a + b ) / 2 \rfloor;
///     \ \mathrm{visit}( 2 x , [ a , c ) );\ \mathrm{visit}( 2 x + 1 ,
///     [ c , b ) ) \cr
/// 13 & \quad \mathrm{rollback}( c_0 )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, the nodes in \f$ [ 0 , n ) \f$, each `DELETE` of
///      an edge alive
[[nodiscard]] inline std::vector< int > offlineConnectivity(
                          int n , const std::vector< ConnectivityOp > & ops );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/RollbackUnionFind.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RollbackUnionFind.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RollbackUnionFind.h ----------------------*/
/*--------------------------------------------------------------------------*/
