/*--------------------------------------------------------------------------*/
/*-------------------------- File BinarySearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search of section 1.2 in a sorted vector \f$ A[ 0 .. n - 1 ] \f$:
 * \f$ v \f$ is compared with the middle item \f$ A[ m ] \f$ of the part
 * \f$ A[ i .. j ] \f$ still to examine, and the search goes on in
 * \f$ A[ i .. m - 1 ] \f$ if \f$ v < A[ m ] \f$ and in \f$ A[ m + 1 .. j ] \f$
 * if \f$ v > A[ m ] \f$; the part halves at each call, so that at most
 * \f$ \lfloor \log_2 n \rfloor + 1 \f$ calls are made, 2 comparisons each.
 * The middle is computed as \f$ m = i + \lfloor ( j - i ) / 2 \rfloor \f$,
 * which unlike \f$ \lfloor ( i + j ) / 2 \rfloor \f$ cannot overflow (the
 * bug of `Arrays.binarySearch` of Java until 2003). The search may report
 * each call \f$ ( i , j , m ) \f$ to a function, as the table of the example.
 *
 * `decisionTree()` gives the decision tree of the search, the binary tree
 * whose root is the middle item \f$ A[ m ] \f$ of the vector and whose
 * subtrees are the decision trees of \f$ A[ 0 .. m - 1 ] \f$ and of
 * \f$ A[ m + 1 .. n - 1 ] \f$: a search for \f$ v \f$ is the path from the
 * root down to the node of \f$ v \f$, or to a missing child, so that its
 * comparisons are the depth of that node plus one. Its invisit gives
 * \f$ A \f$, it is a binary search tree [see `SearchTree.h`] and all its
 * levels are full but the last one; hence its height is
 * \f$ \lfloor \log_2 n \rfloor \f$, the worst case of the search.
 *
 * `lowerBound()` and `upperBound()` give the first place of an item not
 * smaller, and larger, than \f$ v \f$, with
 * \f$ \lceil \log_2 ( n + 1 ) \rceil \f$ comparisons: their difference is
 * the number of occurrences of \f$ v \f$ (exercise 12.13). `gallopSearch()`
 * looks for the first place of an item not smaller than \f$ v \f$ by
 * doubling a bound before the binary search, in \f$ O( \log h ) \f$ time if
 * that place is \f$ h \f$ (exercise 12.9).
 *
 * The contracts [see `Contract.h`]: the vector must be sorted, an audit
 * checked at the level 3 since it costs \f$ O( n ) \f$ against the
 * \f$ O( \log n ) \f$ of the search; `lowerBound()` also needs
 * \f$ \mathit{lo} \le \mathit{hi} \le n \f$, and the place it returns is
 * checked against its two neighbours. The functions are defined in
 * `BinarySearch.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 1.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
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

#ifndef __ADS_BinarySearch
 #define __ADS_BinarySearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"
#include "ADS/tree/BinaryTree.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place of \f$ v \f$ in the sorted \f$ A \f$, or -1; `trace( i , j , m )`
/// on each call that examines \f$ A[ m ] \f$
///
/// In pseudocode (section 1.2), the tail recursion of the book as a loop:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binarySearch}( A , v ) \cr
/// 2 & \quad i \gets 0;\ j \gets n - 1 \cr
/// 3 & \quad \textbf{while}\ i \le j\ \textbf{do} \cr
/// 4 & \qquad m \gets i + \lfloor ( j - i ) / 2 \rfloor;\ \mathrm{trace}( i ,
///     j , m ) \cr
/// 5 & \qquad \textbf{if}\ A[ m ] = v\ \textbf{then return}\ m \cr
/// 6 & \qquad \textbf{else if}\ A[ m ] < v\ \textbf{then}\ i \gets m + 1 \cr
/// 7 & \qquad \textbf{else}\ j \gets m - 1 \cr
/// 8 & \quad \textbf{return}\ -1
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted
/// \post the result is -1 if and only if \f$ v \f$ is not in \f$ A \f$
template< class T , class F >
long binarySearch( const std::vector< T > & A , const T & v , F && trace );

/// the search of `binarySearch()` above, with no trace
template< class T >
long binarySearch( const std::vector< T > & A , const T & v );

/// the decision tree of `binarySearch()` on \f$ A \f$, nil if \f$ A \f$ is
/// empty; the caller owns it
///
/// In pseudocode (the decision trees of section 2.7.4 on the search of
/// section 1.2), the calls of the search made nodes:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decision}( A , i , j ) \cr
/// 2 & \quad \textbf{if}\ i > j\ \textbf{then return nil} \cr
/// 3 & \quad m \gets i + \lfloor ( j - i ) / 2 \rfloor \cr
/// 4 & \quad t \gets \text{a node of item } A[ m ] \cr
/// 5 & \quad t.\mathit{left} \gets \mathrm{decision}( A , i , m - 1 );
///     \ t.\mathit{right} \gets \mathrm{decision}( A , m + 1 , j ) \cr
/// 6 & \quad \textbf{return}\ t \cr
/// 7 & \textbf{function}\ \mathrm{decisionTree}( A ) \cr
/// 8 & \quad \textbf{return}\ \mathrm{decision}( A , 0 , n - 1 )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted
/// \post its invisit is \f$ A \f$ (audit)
template< class T >
BinaryTree< T > * decisionTree( const std::vector< T > & A );

/// the first place in \f$ [ \mathit{lo} , \mathit{hi} ) \f$ of an item not
/// smaller than \f$ v \f$, \f$ \mathit{hi} \f$ if none
///
/// In pseudocode (exercise 12.13), the invariant being
/// \f$ A[ \mathit{lo} - 1 ] < v \le A[ \mathit{hi} ] \f$ (outside the
/// range, \f$ -\infty \f$ and \f$ +\infty \f$):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lowerBound}( A , v , \mathit{lo} ,
///     \mathit{hi} ) \cr
/// 2 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 3 & \qquad m \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo} ) / 2
///     \rfloor \cr
/// 4 & \qquad \textbf{if}\ A[ m ] < v\ \textbf{then}\ \mathit{lo} \gets m + 1
///     \ \textbf{else}\ \mathit{hi} \gets m \cr
/// 5 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{lo} \le \mathit{hi} \le n \f$ and
///      \f$ A[ \mathit{lo} .. \mathit{hi} ) \f$ is sorted
/// \post \f$ A[ r - 1 ] < v \le A[ r ] \f$, within
///       \f$ [ \mathit{lo} , \mathit{hi} ] \f$
template< class T , class C = std::less<> >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        std::size_t lo , std::size_t hi , C less = C() );

/// the place of `lowerBound()` above in the whole of \f$ A \f$, i.e.,
/// `lowerBound( A , v , 0 , n )`
template< class T , class C = std::less<> >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        C less = C() );

/// the first place of an item larger than \f$ v \f$, \f$ n \f$ if none
///
/// In pseudocode (exercise 12.13), the search of `lowerBound()` with the
/// test reversed:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{upperBound}( A , v ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad m \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo} ) / 2
///     \rfloor \cr
/// 5 & \qquad \textbf{if}\ v < A[ m ]\ \textbf{then}\ \mathit{hi} \gets m
///     \ \textbf{else}\ \mathit{lo} \gets m + 1 \cr
/// 6 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted
/// \post \f$ A[ r - 1 ] \le v < A[ r ] \f$
template< class T , class C = std::less<> >
std::size_t upperBound( const std::vector< T > & A , const T & v ,
                        C less = C() );

/// the first place \f$ h \f$ of an item not smaller than \f$ v \f$, found by
/// looking at the places \f$ 0 , 1 , 3 , 7 , \ldots \f$ and then by a binary
/// search, \f$ O( \log h ) \f$
///
/// In pseudocode (exercise 12.9, the unbounded search of Bentley and Yao):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gallopSearch}( A , v ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets 0;\ s \gets 1 \cr
/// 3 & \quad \textbf{while}\ \mathit{hi} < n\ \textbf{and}\ A[ \mathit{hi} ]
///     < v\ \textbf{do} \cr
/// 4 & \qquad \mathit{lo} \gets \mathit{hi} + 1;\ \mathit{hi} \gets
///     \mathit{hi} + s;\ s \gets 2 s \cr
/// 5 & \quad \textbf{return}\ \mathrm{lowerBound}( A , v , \mathit{lo} ,
///     \min\{ \mathit{hi} + 1 , n \} )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted
/// \post \f$ A[ r - 1 ] < v \le A[ r ] \f$
template< class T , class C = std::less<> >
std::size_t gallopSearch( const std::vector< T > & A , const T & v ,
                          C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/BinarySearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BinarySearch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinarySearch.h -------------------------*/
/*--------------------------------------------------------------------------*/
