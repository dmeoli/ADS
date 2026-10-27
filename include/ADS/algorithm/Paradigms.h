/*--------------------------------------------------------------------------*/
/*---------------------------- File Paradigms.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The general schemes of the techniques of design of algorithms, beyond the
 * book, as functions of the parts that make a technique concrete.
 *
 * A problem is a quintuple \f$ \langle I , S , R , O , Q \rangle \f$: the
 * instances \f$ I \f$, the solutions \f$ S \f$, the relation \f$ R \f$
 * between them, the answers \f$ O \f$ and the function \f$ Q \f$ that gives
 * the answer of an instance. In a problem of search the answer is a solution
 * \f$ s \f$ with \f$ \langle i , s \rangle \in R \f$, or none (written
 * \f$ \bot \f$); in one of decision it is whether such an \f$ s \f$ exists,
 * and it may be found with the search (the constructive method) or not; in
 * one of optimization it is a solution that is best for a measure \f$ m \f$
 * with values in an ordered set, or none. A search space \f$ Z \f$ of an
 * instance is a set of objects with a function of admissibility \f$ a \f$
 * and one of answer \f$ o \f$, such that \f$ a( z ) \f$ holds if and only if
 * \f$ o( z ) \f$ is a solution; for a problem of optimization \f$ Z \f$ must
 * also contain an optimal solution.
 *
 * - `enumerateSearch()` examines \f$ Z \f$ in a total order, `first()`,
 *   `next()` and `last()`, and returns the answer of the first admissible
 *   object, or nothing: the enumerative method;
 * - `enumerateOptimize()` examines all of \f$ Z \f$ and returns the answer
 *   of an admissible object of best measure, or nothing;
 * - `backtrack()` visits the tree of the partial solutions in depth: the
 *   root is the empty one and a node at level \f$ j \f$ has a child for each
 *   way of adding the component \f$ j + 1 \f$; a node that `prune()`
 *   rejects is not visited, its subtree with it, and each leaf is handed to
 *   `visit()`, which returns true to stop;
 * - `greedy()` builds a solution by stages, choosing at each stage the best
 *   of the values admissible there and never going back;
 * - `divideEtImpera()` solves an instance directly if it is small, and
 *   otherwise divides it into instances of the same problem, solves them
 *   recursively and combines their solutions;
 * - `maximal()` returns the elements of a set that are maximal for a partial
 *   order, i.e., those with no element strictly better, in
 *   \f$ O( n^2 ) \f$: the answers of a problem of optimization whose measure
 *   is only partially ordered.
 *
 * The functions are defined in `Paradigms.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 12 to 16,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
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

#ifndef __ADS_Paradigms
 #define __ADS_Paradigms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the enumerative search from the first object \f$ z \f$: the answer
/// \f$ o( z ) \f$ of the first \f$ z \f$ with \f$ a( z ) \f$, `next( z )`
/// after \f$ z \f$ until `last( z )` holds; nothing if none
///
/// In pseudocode (the enumerative method):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{enumerateSearch}( z ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \textbf{if}\ a( z )\ \textbf{then return}\ o( z ) \cr
/// 4 & \qquad \textbf{if}\ \mathrm{last}( z )\ \textbf{then return}\ \bot \cr
/// 5 & \qquad z \gets \mathrm{next}( z )
/// \end{array}}
/// \f]
template< class Z , class Next , class Last , class A , class O >
auto enumerateSearch( Z z , Next && next , Last && last , A && a , O && o )
 -> std::optional< decltype( o( z ) ) >;

/// the enumerative optimization: the answer of an admissible object of
/// \f$ Z \f$ with no admissible one better, i.e., with no \f$ y \f$ such that
/// `better( m( y ) , m( x ) )`, or nothing
///
/// In pseudocode (the enumerative method), the first best object kept:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{enumerateOptimize}( z ) \cr
/// 2 & \quad b \gets \textbf{nil} \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ a( z )\ \textbf{and}\ ( b = \textbf{nil}
///     \ \textbf{or}\ \mathrm{better}( m( z ) , m( b ) ) )\ \textbf{then}\ b
///     \gets z \cr
/// 5 & \qquad \textbf{if}\ \mathrm{last}( z )\ \textbf{then}\ \textbf{break}
///     \cr
/// 6 & \qquad z \gets \mathrm{next}( z ) \cr
/// 7 & \quad \textbf{if}\ b = \textbf{nil}\ \textbf{then return}\ \bot
///     \ \textbf{else return}\ o( b )
/// \end{array}}
/// \f]
template< class Z , class Next , class Last , class A , class O , class M ,
          class B >
auto enumerateOptimize( Z z , Next && next , Last && last , A && a ,
                        O && o , M && m , B && better )
 -> std::optional< decltype( o( z ) ) >;

/// the backtracking from the node \f$ N \f$: `prune( N )` rejects \f$ N \f$
/// and its subtree, a leaf goes to `visit( N )`, which returns true to stop,
/// `children( N )` gives the children of a node that is not a leaf; true if
/// stopped
///
/// In pseudocode (the scheme of chapter 16):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{backtrack}( N ) \cr
/// 2 & \quad \textbf{if}\ \mathrm{prune}( N )\ \textbf{then return false} \cr
/// 3 & \quad \textbf{if}\ \mathrm{leaf}( N )\ \textbf{then return}
///     \ \mathrm{visit}( N ) \cr
/// 4 & \quad \textbf{foreach}\ C \in \mathrm{children}( N )\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{backtrack}( C )\ \textbf{then return true}
///     \cr
/// 6 & \quad \textbf{return false}
/// \end{array}}
/// \f]
template< class N , class C , class P , class L , class V >
bool backtrack( const N & node , C && children , P && prune , L && leaf ,
                V && visit );

/// the greedy scheme: for each of \f$ k \f$ stages, the best of the values
/// `candidates( i , x )` admissible after the partial solution \f$ x \f$ is
/// appended to \f$ x \f$, the stage skipped if there is none
///
/// In pseudocode (the scheme of chapter 14):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{greedy}( k ) \cr
/// 2 & \quad x \gets \text{the empty sequence} \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ k - 1\ \textbf{do} \cr
/// 4 & \qquad C \gets \mathrm{candidates}( i , x ) \cr
/// 5 & \qquad \textbf{if}\ C \ne \emptyset\ \textbf{then}\ x \gets x \cdot (
///     \mathrm{best}( C ) ) \cr
/// 6 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
template< class X , class Cand , class Best >
std::vector< X > greedy( int k , Cand && candidates , Best && best );

/// divide et impera: `direct( x )` if `small( x )`, else the combination of
/// the solutions of the instances that `divide( x )` gives
///
/// In pseudocode (the scheme of chapter 12):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{divideEtImpera}( x ) \cr
/// 2 & \quad \textbf{if}\ \mathrm{small}( x )\ \textbf{then return}
///     \ \mathrm{direct}( x ) \cr
/// 3 & \quad S \gets \text{the empty sequence} \cr
/// 4 & \quad \textbf{foreach}\ y \in \mathrm{divide}( x )\ \textbf{do}\ S
///     \gets S \cdot ( \mathrm{divideEtImpera}( y ) ) \cr
/// 5 & \quad \textbf{return}\ \mathrm{combine}( S )
/// \end{array}}
/// \f]
template< class X , class S , class D , class V , class C >
auto divideEtImpera( const X & x , S && small , D && direct , V && divide ,
                     C && combine ) -> decltype( direct( x ) );

/// the elements of \f$ X \f$ that are maximal for the partial order
/// `better( a , b )`, i.e., \f$ a \f$ strictly better than \f$ b \f$
///
/// In pseudocode, by all the pairs, \f$ O( n^2 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maximal}( X ) \cr
/// 2 & \quad M \gets \text{the empty sequence} \cr
/// 3 & \quad \textbf{foreach}\ x \in X\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{no } y \in X \text{ has }
///     \mathrm{better}( y , x )\ \textbf{then}\ M \gets M \cdot ( x ) \cr
/// 5 & \quad \textbf{return}\ M
/// \end{array}}
/// \f]
template< class T , class B >
std::vector< T > maximal( const std::vector< T > & X , B && better );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/Paradigms.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Paradigms.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Paradigms.h --------------------------*/
/*--------------------------------------------------------------------------*/
