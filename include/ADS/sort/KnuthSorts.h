/*--------------------------------------------------------------------------*/
/*--------------------------- File KnuthSorts.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The internal sorts of Knuth that the book leaves out, beyond the book:
 *
 * - `binaryInsertionSort()` (section 5.2.1 of Knuth) finds the place of each
 *   item by a binary search in the sorted prefix,
 *   \f$ \sum_k \lceil \log_2 k \rceil \f$ comparisons, near the least
 *   possible, but still \f$ \Theta( n^2 ) \f$ moves; stable;
 * - `listInsertionSort()` (Algorithm 5.2.1L) inserts each item in a list of
 *   links, so that no item moves until the end, when the list is followed to
 *   rearrange the vector: \f$ \Theta( n^2 ) \f$ comparisons and only
 *   \f$ O( n ) \f$ moves of items; stable;
 * - `addressCalculationSort()` (section 5.2.1, of Isaac and Singleton) inserts
 *   each number in one of \f$ M \f$ lists by its value, the list \f$ i \f$
 *   taking the numbers in the \f$ i \f$-th of \f$ M \f$ equal ranges between
 *   the least and the largest, as one would file cards under the letters:
 *   \f$ O( n ) \f$ expected time on numbers spread evenly, with \f$ M \f$
 *   about \f$ n / 4 \f$; stable;
 * - `radixExchangeSort()` (Algorithm 5.2.2R) is the quicksort on the bits of
 *   the keys from the most significant: the part is split into the keys with
 *   the bit 0 and those with the bit 1, by exchanges as in a partition, and
 *   each half is sorted on the next bit; \f$ O( n w ) \f$ for keys of
 *   \f$ w \f$ bits, not stable;
 * - `tournamentSort()` (section 5.2.3, tree selection) plays a knockout
 *   tournament on a complete binary tree of the items, the winner (the least)
 *   goes out and only the matches on its path are played again,
 *   \f$ \lceil \log_2 n \rceil \f$ comparisons each: \f$ O( n \log n ) \f$
 *   with \f$ O( n ) \f$ extra places, the idea from which heapsort grew;
 *   stable, since equal winners are broken by their places;
 * - `mergeInsertionSort()` of Ford and Johnson (section 5.3.1): the items are
 *   paired and compared, the larger ones sorted recursively, and the smaller
 *   ones inserted by binary search into the chain in the order of the
 *   Jacobsthal numbers \f$ 1 , 3 , 5 , 11 , 21 , \ldots \f$, so that each
 *   insertion searches a prefix of \f$ 2^k - 1 \f$ items; it makes
 *   \f$ \sum_{k=1}^n \lceil \log_2 ( 3 k / 4 ) \rceil \f$ comparisons in the
 *   worst case, the fewest of any known method for most \f$ n \f$ (and the
 *   least possible for \f$ n \le 11 \f$ and \f$ n = 20 , 21 \f$), at the price
 *   of a bookkeeping that makes it slow in time; not stable.
 *
 * The contracts [see `Contract.h`]: each sort ends with the same items sorted,
 * an audit. The functions are defined in `KnuthSorts.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.2.1, 5.2.2, 5.2.3 and 5.3.1
 * \see E. J. Isaac, R. C. Singleton, Sorting by address calculation,
 *      Journal of the ACM 3(3), 169-174, 1956, doi:10.1145/320831.320834
 * \see P. Hildebrandt, H. Isbitz, Radix exchange: an internal sorting
 *      method for digital computers, Journal of the ACM 6(2), 156-163,
 *      1959, doi:10.1145/320964.320972
 * \see L. R. Ford, S. M. Johnson, A tournament problem, American
 *      Mathematical Monthly 66(5), 387-389, 1959, doi:10.2307/2308750
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

#ifndef __ADS_KnuthSorts
 #define __ADS_KnuthSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the binary insertion sort
///
/// In pseudocode (Knuth, section 5.2.1, beyond the book), each item placed
/// after its equals:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{binaryInsertionSort}( A , n ) \cr
/// 2 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad \mathit{lo} \gets 0;\ \mathit{hi} \gets i \cr
/// 4 & \qquad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 5 & \qquad\quad m \gets \lfloor ( \mathit{lo} + \mathit{hi} ) / 2 \rfloor
///     \cr
/// 6 & \qquad\quad \textbf{if}\ A[ i ] < A[ m ]\ \textbf{then}\ \mathit{hi}
///     \gets m\ \textbf{else}\ \mathit{lo} \gets m + 1 \cr
/// 7 & \qquad \text{rotate } A[ \mathit{lo} .. i ] \text{ one place to the }
///     \text{right}
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void binaryInsertionSort( std::vector< T > & A , C less = C() );

/// the list insertion sort; the moves of the items go to `*moves`, if given
///
/// In pseudocode (Algorithm 5.2.1L of Knuth, beyond the book), \f$ L \f$ the
/// links of a list whose head is \f$ L[ 0 ] \f$, the item \f$ A[ j ] \f$ its
/// node \f$ j + 1 \f$ and 0 its end:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{listInsertionSort}( A , n ) \cr
/// 2 & \quad L[ 0 ] \gets n;\ L[ n ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ j \gets n - 2\ \textbf{downto}\ 0\ \textbf{do} \cr
/// 4 & \qquad p \gets 0;\ q \gets L[ 0 ] \cr
/// 5 & \qquad \textbf{while}\ q \ne 0\ \textbf{and}\ A[ q - 1 ] < A[ j ]
///     \ \textbf{do}\ p \gets q;\ q \gets L[ q ] \cr
/// 6 & \qquad L[ p ] \gets j + 1;\ L[ j + 1 ] \gets q \cr
/// 7 & \quad \text{copy the items in the order of the list, from } L[ 0 ]
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void listInsertionSort( std::vector< T > & A , C less = C() ,
                        std::size_t * moves = nullptr );

/// the address calculation sort of numbers into \f$ M \f$ lists,
/// \f$ M = \lfloor n / 4 \rfloor + 1 \f$ if \f$ M \f$ is 0; with an infinite
/// range (an infinity, or a span beyond a `double`) all the items go to one
/// list, an insertion sort
///
/// In pseudocode (Isaac and Singleton, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{addressCalculationSort}( A , n , M ) \cr
/// 2 & \quad \mathit{lo} \gets \min A;\ w \gets \max A - \mathit{lo} \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{do} \cr
/// 4 & \qquad i \gets \min\{ M - 1 , \lfloor ( x - \mathit{lo} ) M / w
///     \rfloor \} \cr
/// 5 & \qquad \text{insert } x \text{ in the sorted list } B[ i ] \text{, }
///     \text{after its equals} \cr
/// 6 & \quad A \gets B[ 0 ] \cdot B[ 1 ] \cdots B[ M - 1 ]
/// \end{array}}
/// \f]
/// where \f$ i = 0 \f$ when \f$ w \f$ is 0 or infinite.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T >
void addressCalculationSort( std::vector< T > & A , std::size_t M = 0 );

/// the radix exchange sort
///
/// In pseudocode (Algorithm 5.2.2R of Knuth, beyond the book), \f$ b \f$ the
/// bit examined, from the most significant one \f$ w - 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{radixExchange}( A , \mathit{lo} ,
///     \mathit{hi} , b ) \cr
/// 2 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} \le 1\ \textbf{or}\ b < 0
///     \ \textbf{then return} \cr
/// 3 & \quad i \gets \mathit{lo};\ j \gets \mathit{hi} \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \textbf{while}\ i < j \text{ and the bit } b \text{ of } A[ i ]
///     \text{ is 0}\ \textbf{do}\ i \gets i + 1 \cr
/// 6 & \qquad \textbf{while}\ i < j \text{ and the bit } b \text{ of } A[ j -
///     1 ] \text{ is 1}\ \textbf{do}\ j \gets j - 1 \cr
/// 7 & \qquad \textbf{if}\ i \ge j\ \textbf{then}\ \textbf{break} \cr
/// 8 & \qquad A[ i ] \leftrightarrow A[ j - 1 ];\ i \gets i + 1;\ j \gets j -
///     1 \cr
/// 9 & \quad \mathrm{radixExchange}( A , \mathit{lo} , i , b - 1 ) \cr
/// 10 & \quad \mathrm{radixExchange}( A , i , \mathit{hi} , b - 1 ) \cr
/// 11 & \textbf{procedure}\ \mathrm{radixExchangeSort}( A , n ) \cr
/// 12 & \quad \mathrm{radixExchange}( A , 0 , n , w - 1 )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< RadixKey T >
void radixExchangeSort( std::vector< T > & A );

/// the tournament sort
///
/// In pseudocode (the tree selection of Knuth, section 5.2.3, beyond the
/// book), \f$ W[ v ] \f$ the winner of the match at the node \f$ v \f$ of a
/// complete tree with \f$ L \ge n \f$ leaves, \f$ \bot \f$ for no item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{tournamentSort}( A , n ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ L - 1\ \textbf{do}\ W[ L +
///     i ] \gets i \text{ if } i < n \text{, else } \bot \cr
/// 3 & \quad \textbf{for}\ v \gets L - 1\ \textbf{downto}\ 1\ \textbf{do}\ W[
///     v ] \gets \mathrm{play}( W[ 2 v ] , W[ 2 v + 1 ] ) \cr
/// 4 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad B[ k ] \gets A[ W[ 1 ] ];\ v \gets L + W[ 1 ];\ W[ v ] \gets
///     \bot \cr
/// 6 & \qquad \textbf{while}\ v > 1\ \textbf{do}\ v \gets \lfloor v / 2
///     \rfloor;\ W[ v ] \gets \mathrm{play}( W[ 2 v ] , W[ 2 v + 1 ] ) \cr
/// 7 & \quad A \gets B
/// \end{array}}
/// \f]
/// where \f$ \mathrm{play}( a , b ) \f$ returns the smaller item, \f$ a \f$ on
/// a tie, and the one that is not \f$ \bot \f$ against \f$ \bot \f$.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void tournamentSort( std::vector< T > & A , C less = C() );

/// the merge insertion sort of Ford and Johnson
///
/// In pseudocode (Ford and Johnson, section 5.3.1 of Knuth, beyond the book),
/// \f$ t_k = ( 2^{k+1} + ( -1 )^k ) / 3 \f$ the Jacobsthal numbers
/// \f$ 1 , 3 , 5 , 11 , 21 , \ldots \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fordJohnson}( v ) \cr
/// 2 & \quad \textbf{if}\ |v| \le 1\ \textbf{then return}\ v \cr
/// 3 & \quad \text{pair } v[ 0 ] , v[ 1 ] \text{, then } v[ 2 ] , v[ 3 ]
///     \text{, and so on, by one comparison each} \cr
/// 4 & \quad ( l_1 , \ldots , l_k ) \gets \mathrm{fordJohnson}( \text{the }
///     \text{larger item of each pair} ) \cr
/// 5 & \quad s_i \gets \text{the partner of } l_i;\ s_{k+1} \gets \text{the }
///     \text{item left alone, if } |v| \text{ is odd} \cr
/// 6 & \quad C \gets ( s_1 , l_1 , \ldots , l_k ) \cr
/// 7 & \quad \textbf{for}\ j = 2 , 3 , \ldots\ \textbf{do} \cr
/// 8 & \qquad \textbf{for}\ i \gets \min\{ t_j , \text{the last } s \}
///     \ \textbf{downto}\ t_{j-1} + 1\ \textbf{do} \cr
/// 9 & \qquad\quad \text{insert } s_i \text{ in } C \text{ by binary search }
///     \text{before } l_i \cr
/// 10 & \quad \textbf{return}\ C \cr
/// 11 & \textbf{procedure}\ \mathrm{mergeInsertionSort}( A ) \cr
/// 12 & \quad A \gets \text{the items of } A \text{ in the order of }
///     \mathrm{fordJohnson}( 0 , \ldots , n - 1 )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void mergeInsertionSort( std::vector< T > & A , C less = C() );

/// the worst case of the comparisons of `mergeInsertionSort()` on \f$ n \f$
/// items, \f$ \sum_{k=1}^{n} \lceil \log_2 ( 3 k / 4 ) \rceil \f$
inline long fordJohnsonBound( std::size_t n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/KnuthSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnuthSorts.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File KnuthSorts.h --------------------------*/
/*--------------------------------------------------------------------------*/
