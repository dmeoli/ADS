/*--------------------------------------------------------------------------*/
/*---------------------------- File Paradigms.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The general schemes of the techniques of design of algorithms, beyond the
 * book, as functions of the parts that make a technique concrete.
 *
 * A problem is a quintuple < I , S , R , O , Q >: the instances I, the
 * solutions S, the relation R between them, the answers O and the
 * function Q that gives the answer of an instance. In a problem of search
 * the answer is a solution s with < i , s > in R, or none (written
 * \f$ \bot \f$); in one of decision it is whether such an s exists, and
 * it may be found with the search (the constructive method) or not; in one
 * of optimization it is a solution that is best for a measure m with
 * values in an ordered set, or none. A search space Z of an instance is a
 * set of objects with a function of admissibility a and one of answer o,
 * such that a( z ) holds if and only if o( z ) is a solution; for a
 * problem of optimization Z must also contain an optimal solution.
 *
 * - enumerateSearch() examines Z in a total order, first(), next() and
 *   last(), and returns the answer of the first admissible object, or
 *   nothing: the enumerative method;
 * - enumerateOptimize() examines all of Z and returns the answer of an
 *   admissible object of best measure, or nothing;
 * - backtrack() visits the tree of the partial solutions in depth: the
 *   root is the empty one and a node at level j has a child for each way
 *   of adding the component j + 1; a node that prune() rejects is not
 *   visited, its subtree with it, and each leaf is handed to visit(), which
 *   returns true to stop;
 * - greedy() builds a solution by stages, choosing at each stage the best
 *   of the values admissible there and never going back;
 * - divideEtImpera() solves an instance directly if it is small, and
 *   otherwise divides it into instances of the same problem, solves them
 *   recursively and combines their solutions;
 * - maximal() returns the elements of a set that are maximal for a partial
 *   order, i.e., those with no element strictly better, in
 *   \f$ O( n^2 ) \f$: the answers of a problem of optimization whose
 *   measure is only partially ordered.
 *
 * The functions are defined in Paradigms.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 13-16,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// the enumerative search: z = first, the answer o( z ) of the first z
/// with a( z ), next( z ) after z until last( z ) holds; nothing if none
template< class Z , class Next , class Last , class A , class O >
auto enumerateSearch( Z z , Next && next , Last && last , A && a , O && o )
 -> std::optional< decltype( o( z ) ) >;

/// the enumerative optimization: the answer of an admissible object of
/// Z with no admissible one better( m( y ) , m( x ) ), or nothing
template< class Z , class Next , class Last , class A , class O , class M ,
          class B >
auto enumerateOptimize( Z z , Next && next , Last && last , A && a ,
                        O && o , M && m , B && better )
 -> std::optional< decltype( o( z ) ) >;

/// the backtracking from the node N: prune( N ) rejects N and its subtree,
/// a leaf goes to visit( N ), which returns true to stop, children( N )
/// gives the children of a node that is not a leaf; true if stopped
template< class N , class C , class P , class L , class V >
bool backtrack( const N & node , C && children , P && prune , L && leaf ,
                V && visit );

/// the greedy scheme: for each of k stages, the best( candidates ) of the
/// values candidates( i , x ) admissible after the partial solution x is
/// appended to x, the stage skipped if there is none
template< class X , class Cand , class Best >
std::vector< X > greedy( int k , Cand && candidates , Best && best );

/// divide et impera: direct( x ) if small( x ), else the combination of
/// the solutions of the instances that divide( x ) gives
template< class X , class S , class D , class V , class C >
auto divideEtImpera( const X & x , S && small , D && direct , V && divide ,
                     C && combine ) -> decltype( direct( x ) );

/// the elements of X that are maximal for the partial order better( a ,
/// b ), i.e., a strictly better than b
template< class T , class B >
std::vector< T > maximal( const std::vector< T > & X , B && better );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/Paradigms.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Paradigms.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Paradigms.h --------------------------*/
/*--------------------------------------------------------------------------*/
