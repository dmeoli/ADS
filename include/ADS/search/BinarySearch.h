/*--------------------------------------------------------------------------*/
/*-------------------------- File BinarySearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search of section 1.2 in a sorted vector A[ 0 .. n - 1 ]: v
 * is compared with the middle item A[ m ] of the part A[ i .. j ] still to
 * examine, and the search goes on in A[ i .. m - 1 ] if v < A[ m ] and in
 * A[ m + 1 .. j ] if v > A[ m ]; the part halves at each call, so that at
 * most \f$ \lfloor \log_2 n \rfloor + 1 \f$ calls are made, 2 comparisons
 * each. The middle is computed as \f$ m = i + \lfloor ( j - i ) / 2 \rfloor
 * \f$, which unlike \f$ \lfloor ( i + j ) / 2 \rfloor \f$ cannot overflow
 * (the bug of Arrays.binarySearch of Java until 2003). The search may
 * report each call (i, j, m) to a function, as the table of the example.
 *
 * decisionTree() gives the decision tree of the search, the binary tree
 * whose root is the middle item A[ m ] of the vector and whose subtrees are
 * the decision trees of A[ 0 .. m - 1 ] and of A[ m + 1 .. n - 1 ]: a
 * search for v is the path from the root down to the node of v, or to a
 * missing child, so that its comparisons are the depth of that node plus
 * one. Its invisit gives A, it is a binary search tree [see SearchTree.h]
 * and all its levels are full but the last one, hence its height is
 * \f$ \lfloor \log_2 n \rfloor \f$, the worst case of the search.
 *
 * lowerBound() and upperBound() give the first place of an item not
 * smaller, and larger, than v, with \f$ \lceil \log_2 ( n + 1 ) \rceil \f$
 * comparisons: their difference is the number of occurrences of v
 * (exercise 12.13). gallopSearch() looks for the first place of an item
 * not smaller than v by doubling a bound before the binary search, in
 * \f$ O( \log h ) \f$ time if that place is h (exercise 12.9).
 *
 * The contracts [see Contract.h]: the vector must be sorted, an audit checked
 * at the level 3 since it costs O( n ) against the O( log n ) of the search;
 * lowerBound() also needs lo <= hi <= n, and the place it returns is checked
 * against its two neighbours. The functions are defined in BinarySearch.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 1.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
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

/// binarySearch(): the place of v in the sorted A, or -1; trace( i , j ,
/// m ) on each call that examines A[ m ]
/// \pre A is sorted
/// \post the result is -1 if and only if v is not in A
template< class T , class F >
long binarySearch( const std::vector< T > & A , const T & v , F && trace );

/// binarySearch() with no trace
template< class T >
long binarySearch( const std::vector< T > & A , const T & v );

/// the decision tree of binarySearch() on A, nil if A is empty; the caller
/// owns it
/// \pre A is sorted
/// \post its invisit is A (an audit)
template< class T >
BinaryTree< T > * decisionTree( const std::vector< T > & A );

/// the first place in [ lo , hi ) of an item not smaller than v, hi if none
/// \pre lo <= hi <= n and A[ lo .. hi ) is sorted
/// \post A[ r - 1 ] < v <= A[ r ], within [ lo , hi ]
template< class T , class C = std::less<> >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        std::size_t lo , std::size_t hi , C less = C() );

template< class T , class C = std::less<> >
std::size_t lowerBound( const std::vector< T > & A , const T & v ,
                        C less = C() );

/// the first place of an item larger than v, n if none
/// \pre A is sorted
/// \post A[ r - 1 ] <= v < A[ r ]
template< class T , class C = std::less<> >
std::size_t upperBound( const std::vector< T > & A , const T & v ,
                        C less = C() );

/// the first place h of an item not smaller than v, found by looking at
/// the places 0 , 1 , 3 , 7 , ... and then by a binary search, O( log h )
/// \pre A is sorted
/// \post A[ r - 1 ] < v <= A[ r ]
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
