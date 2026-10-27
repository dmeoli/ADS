/*--------------------------------------------------------------------------*/
/*------------------------- File MaximumSegments.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The segments (subarrays) of largest sum and their variants, of chapter 2 (A
 * Warm-up) of the Pearls of Ferragina: the gains of a stock as the differences
 * of its quotations, and the regions of a DNA sequence rich in C and G. The
 * array is \f$ D[ 0 .. n - 1 ] \f$, and
 * \f$ P[ i ] = D[ 0 ] + \cdots + D[ i - 1 ] \f$ are its prefix sums, so that
 * the segment \f$ D[ b .. s ] \f$ sums to \f$ P[ s + 1 ] - P[ b ] \f$.
 *
 * - `maxSumSegmentCubic()` and `maxSumSegmentQuadratic()` are the cubic-time
 *   and the quadratic-time algorithms of that chapter, all the pairs
 *   \f$ ( b , s ) \f$, the sum recomputed or extended, \f$ \Theta( n^3 ) \f$
 *   and \f$ \Theta( n^2 ) \f$;
 * - `maxSumSegment()` is its linear-time algorithm, the scan of Kadane with
 *   the positions: a prefix of negative sum is dropped, since no optimum
 *   starts after a segment of positive sum nor has a prefix of negative sum
 *   (two properties of that chapter), \f$ O( n ) \f$ time, one pass,
 *   \f$ \lceil n / B \rceil \f$ I/Os whatever \f$ B \f$ (cache oblivious).
 *   `maxSubarraySum()` of `Subsequences.h` is the same scan for the value
 *   only, the empty segment allowed; here the segment is not empty, so that an
 *   array of negative numbers gives its largest one;
 * - `maxSumSegmentPrefix()` is its other linear-time algorithm: the best
 *   segment ending in \f$ s \f$ is \f$ P[ s + 1 ] - \min_{b \le s} P[ b ] \f$,
 *   the minimum carried along the scan;
 * - `maxSumSegmentLength()` bounds the length to \f$ [ L , U ] \f$: the
 *   minimum of \f$ P \f$ over the window of the admissible starts, by a deque
 *   of increasing prefix sums (the sliding minimum), \f$ O( n ) \f$, the
 *   algorithm of Lin, Jiang and Chao;
 * - `longestSegmentAtLeast()` is the algorithm of its variants for the longest
 *   segment of sum at least \f$ t \f$: at step \f$ i \f$ only the starts left
 *   of \f$ L_i = i - \ell \f$ can beat the current length \f$ \ell \f$, and
 *   among them only the chain of leftmost minima
 *   \f$ C_1 = \mathit{LMin}[ L_i ] \f$, \f$ C_2 = \mathit{LMin}[ C_1 ] \f$,
 *   \f$ \ldots \f$, whose prefix sums increase; the chain is walked while the
 *   sum stays at least \f$ t \f$, and every step past the first lengthens the
 *   solution, hence \f$ O( n ) \f$ amortized. `longestSegmentAverageAtLeast()`
 *   reduces the longest segment of average at least \f$ p / q \f$ to it, by
 *   the items \f$ q D[ k ] - p \f$;
 * - `maxDensitySegment()` is the segment of largest average among those of
 *   length at least \f$ L \f$: the average of \f$ D[ x .. i - 1 ] \f$ is the
 *   slope from \f$ ( x , P[ x ] ) \f$ to \f$ ( i , P[ i ] ) \f$, and the best
 *   \f$ x \f$ for \f$ i \f$ lies on the lower convex hull of the points
 *   \f$ x \le i - L \f$, kept by the monotone chain as \f$ i \f$ grows, the
 *   tangent found by a binary search, \f$ O( n \log n ) \f$. Goldwasser, Kao
 *   and Lu, and Chung and Lu, reach \f$ O( n ) \f$ by walking the hull with a
 *   pointer that moves only forward under a further argument (and also handle
 *   an upper bound on the length); the binary search keeps the code short and
 *   exact on integers;
 * - `gcIndicator()` maps a DNA sequence to 1 for C and G and 0 otherwise,
 *   the input of the two biological measures of the Pearls:
 *   \f$ x - p \ell \f$ (a segment sum of the scores \f$ 1 - p \f$ and
 *   \f$ -p \f$, e.g., \f$ q D[ k ] - p \f$ for \f$ p / q \f$) and the
 *   density \f$ x / \ell \f$, \f$ x \f$ the count of C and G of a segment of
 *   length \f$ \ell \f$.
 *
 * A `Segment` holds the sum of
 * \f$ D[ \mathit{first} .. \mathit{first} + \mathit{length} - 1 ] \f$,
 * length 0 for none. The contracts [see
 * `Contract.h`]: \f$ D \f$ not empty where a segment must exist,
 * \f$ 1 \le L \le U \f$, \f$ L \le n \f$, \f$ q > 0 \f$; the sum of a segment
 * is that of its items, and `maxSumSegment()` agrees with `maxSubarraySum()`
 * when its sum is not negative, audits. The sums of the segments and of the
 * prefixes of \f$ D \f$ must fit in `T`, which is not checked (an item of
 * `T` alone may be its largest or its smallest value). The functions are
 * defined in `MaximumSegments.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 2 (A Warm-up), doi:10.1017/9781009128933
 * \see J. Bentley, Programming pearls: algorithm design techniques,
 *      Communications of the ACM 27(9), 865-873, 1984,
 *      doi:10.1145/358234.381162
 * \see Y.-L. Lin, T. Jiang, K.-M. Chao, Efficient algorithms for locating
 *      the length-constrained heaviest segments with applications to
 *      biomolecular sequence analysis, Journal of Computer and System
 *      Sciences 65(3), 570-586, 2002, doi:10.1016/S0022-0000(02)00010-7
 * \see M. H. Goldwasser, M.-Y. Kao, H.-I. Lu, Linear-time algorithms for
 *      computing maximum-density sequence segments with bioinformatics
 *      applications, Journal of Computer and System Sciences 70(2),
 *      128-144, 2005, doi:10.1016/j.jcss.2004.08.001
 * \see K.-M. Chung, H.-I. Lu, An optimal algorithm for the maximum-density
 *      segment problem, SIAM Journal on Computing 34(2), 373-387, 2005,
 *      doi:10.1137/S0097539704440430
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

#ifndef __ADS_MaximumSegments
 #define __ADS_MaximumSegments
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/algorithm/Subsequences.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a segment \f$ D[ f .. f + \ell - 1 ] \f$ of the array and its sum, with
/// \f$ f \f$ the place `first` and \f$ \ell \f$ the `length`
template< class T >
struct Segment {
 T sum{};                                   ///< the sum of its items
 std::size_t first = 0;                     ///< the place of its first item
 std::size_t length = 0;                    ///< 0 for no segment
 };

/// a nonempty segment of largest sum, by all the pairs, \f$ O( n^3 ) \f$
///
/// In pseudocode (the cubic-time algorithm of chapter 2 of the
/// Pearls):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSumSegmentCubic}( D[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \mathit{best} \gets ( D[ 0 ] , 0 , 1 ) \cr
/// 3 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ s \gets b\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad\quad x \gets D[ b ] + \cdots + D[ s ]\quad \triangleright
///     \ \text{a loop of } s - b + 1 \text{ steps} \cr
/// 6 & \qquad\quad \textbf{if}\ x > \mathit{best}.\mathit{sum}\ \textbf{then}
///     \ \mathit{best} \gets ( x , b , s - b + 1 ) \cr
/// 7 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is not empty
template< class T >
Segment< T > maxSumSegmentCubic( const std::vector< T > & D );

/// a nonempty segment of largest sum, by all the pairs, \f$ O( n^2 ) \f$
///
/// In pseudocode (the quadratic-time algorithm of chapter 2 of the
/// Pearls), the sum of
/// \f$ D[ b .. s ] \f$ extended from that of \f$ D[ b .. s - 1 ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSumSegmentQuadratic}( D[ 0 .. n - 1 ] )
///     \cr
/// 2 & \quad \mathit{best} \gets ( D[ 0 ] , 0 , 1 ) \cr
/// 3 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad x \gets 0 \cr
/// 5 & \qquad \textbf{for}\ s \gets b\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 6 & \qquad\quad x \gets x + D[ s ] \cr
/// 7 & \qquad\quad \textbf{if}\ x > \mathit{best}.\mathit{sum}\ \textbf{then}
///     \ \mathit{best} \gets ( x , b , s - b + 1 ) \cr
/// 8 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is not empty
template< class T >
Segment< T > maxSumSegmentQuadratic( const std::vector< T > & D );

/// a nonempty segment of largest sum, by one scan, \f$ O( n ) \f$
///
/// In pseudocode (the linear-time algorithm of chapter 2 of the
/// Pearls, the scan of Kadane), a new
/// start after each prefix of negative sum:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSumSegment}( D[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \mathit{best} \gets ( D[ 0 ] , 0 , 1 );\ x \gets 0;\ b \gets 0
///     \cr
/// 3 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad x \gets x + D[ s ] \cr
/// 5 & \qquad \textbf{if}\ x > \mathit{best}.\mathit{sum}\ \textbf{then}
///     \ \mathit{best} \gets ( x , b , s - b + 1 ) \cr
/// 6 & \qquad \textbf{if}\ x < 0\ \textbf{then}\ x \gets 0;\ b \gets s + 1 \cr
/// 7 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is not empty
/// \post its sum is `maxSubarraySum( D )` if not negative (audit)
template< class T >
Segment< T > maxSumSegment( const std::vector< T > & D );

/// a nonempty segment of largest sum, by the minimum prefix sum,
/// \f$ O( n ) \f$
///
/// In pseudocode (the other linear-time algorithm of chapter 2 of
/// the Pearls), \f$ x = P[ s + 1 ] \f$ and
/// \f$ m \f$ the least \f$ P[ b ] \f$, \f$ b \le s \f$, at the place
/// \f$ b_m \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSumSegmentPrefix}( D[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \mathit{best} \gets ( D[ 0 ] , 0 , 1 );\ x \gets 0;\ m \gets 0;
///     \ b_m \gets 0 \cr
/// 3 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad x \gets x + D[ s ] \cr
/// 5 & \qquad \textbf{if}\ x - m > \mathit{best}.\mathit{sum}\ \textbf{then}
///     \ \mathit{best} \gets ( x - m , b_m , s - b_m + 1 ) \cr
/// 6 & \qquad \textbf{if}\ x < m\ \textbf{then}\ m \gets x;\ b_m \gets s + 1
///     \cr
/// 7 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is not empty
template< class T >
Segment< T > maxSumSegmentPrefix( const std::vector< T > & D );

/// a segment of largest sum among those of length in \f$ [ L , U ] \f$,
/// \f$ O( n ) \f$
///
/// In pseudocode (the sliding minimum of Lin, Jiang and Chao), the deque
/// \f$ W \f$ holding the admissible starts by increasing \f$ P \f$, so that
/// its head is the best start for the end \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSumSegmentLength}( D , L , U ) \cr
/// 2 & \quad W \gets \text{an empty deque};\ \mathit{best} \gets \text{none}
///     \cr
/// 3 & \quad \textbf{for}\ i \gets L\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad x \gets i - L\quad \triangleright\ \text{the new admissible }
///     \text{start} \cr
/// 5 & \qquad \textbf{while}\ W \ne \emptyset\ \textbf{and}\ P[ \mathrm{tail}(
///     W ) ] \ge P[ x ]\ \textbf{do}\ \text{remove the tail of } W \cr
/// 6 & \qquad \text{append } x \text{ to } W \cr
/// 7 & \qquad \textbf{while}\ \mathrm{head}( W ) < i - U\ \textbf{do}
///     \ \text{remove the head of } W \cr
/// 8 & \qquad b \gets \mathrm{head}( W ) \cr
/// 9 & \qquad \textbf{if}\ P[ i ] - P[ b ] > \mathit{best}.\mathit{sum}
///     \ \textbf{then}\ \mathit{best} \gets ( P[ i ] - P[ b ] , b , i - b )
///     \cr
/// 10 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le L \le U \f$ and \f$ L \le n \f$
template< class T >
Segment< T > maxSumSegmentLength( const std::vector< T > & D ,
                                  std::size_t L , std::size_t U );

/// the longest segment of sum at least \f$ t \f$, length 0 if none,
/// \f$ O( n ) \f$
///
/// In pseudocode (the variants of chapter 2 of the Pearls),
/// \f$ \mathit{LMin}[ x ] \f$ the leftmost place of the minimum of
/// \f$ P[ 0 .. x - 1 ] \f$ and \f$ \ell \f$ the length of the best segment so
/// far:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestSegmentAtLeast}( D , t ) \cr
/// 2 & \quad \ell \gets 0;\ \mathit{best} \gets \text{none} \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \ell \ge i\ \textbf{then continue} \cr
/// 5 & \qquad c \gets \mathit{LMin}[ i - \ell ];\ f \gets \textbf{nil} \cr
/// 6 & \qquad \textbf{while}\ P[ i ] - P[ c ] \ge t\ \textbf{do}\quad
///     \triangleright\ \text{the chain of the leftmost minima} \cr
/// 7 & \qquad\quad f \gets c \cr
/// 8 & \qquad\quad \textbf{if}\ c = 0\ \textbf{then}\ \textbf{break} \cr
/// 9 & \qquad\quad c \gets \mathit{LMin}[ c ] \cr
/// 10 & \qquad \textbf{if}\ f \ne \textbf{nil}\ \textbf{then}\ \mathit{best}
///     \gets ( P[ i ] - P[ f ] , f , i - f );\ \ell \gets i - f \cr
/// 11 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \post its sum is that of its items and at least \f$ t \f$ (audit)
template< class T >
Segment< T > longestSegmentAtLeast( const std::vector< T > & D , T t );

/// the longest segment of average at least \f$ p / q \f$, length 0 if none
///
/// It is `longestSegmentAtLeast()` on the items \f$ q D[ k ] - p \f$, whose
/// segments have a sum at least 0 exactly when the average of the same
/// segment of \f$ D \f$ is at least \f$ p / q \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestSegmentAverageAtLeast}( D , p , q
///     ) \cr
/// 2 & \quad S \gets \mathrm{longestSegmentAtLeast}( ( q D[ k ] - p )_{k=0}^{n
///     - 1} , 0 ) \cr
/// 3 & \quad \textbf{return}\ S \text{ with the sum of its items in } D
/// \end{array}}
/// \f]
/// \pre \f$ q > 0 \f$
template< class T >
Segment< T > longestSegmentAverageAtLeast( const std::vector< T > & D ,
                                           T p , T q );

/// a segment of largest average among those of length at least \f$ L \f$,
/// its sum in the result, \f$ O( n \log n ) \f$
///
/// In pseudocode (beyond the Pearls, after Goldwasser, Kao and Lu), the
/// average of \f$ D[ x .. i - 1 ] \f$ being the slope from
/// \f$ ( x , P[ x ] ) \f$ to \f$ ( i , P[ i ] ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxDensitySegment}( D , L ) \cr
/// 2 & \quad H \gets \text{an empty stack};\ \mathit{best} \gets
///     \text{none} \cr
/// 3 & \quad \textbf{for}\ i \gets L\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad x \gets i - L \cr
/// 5 & \qquad \textbf{while}\ |H| \ge 2 \text{ and the last two of } H
///     \text{ and } x \text{ turn not left}\ \textbf{do}\ \mathrm{pop}( H )
///     \cr
/// 6 & \qquad \mathrm{push}( H , x )\quad \triangleright\ \text{the lower }
///     \text{hull of the starts} \cr
/// 7 & \qquad b \gets \text{the vertex of } H \text{ where the tangent from }
///     ( i , P[ i ] ) \text{ touches} \cr
/// 8 & \qquad\quad \triangleright\ \text{by a binary search on the turns} \cr
/// 9 & \qquad \textbf{if}\ ( P[ i ] - P[ b ] ) / ( i - b ) \text{ beats }
///     \mathit{best}\ \textbf{then}\ \mathit{best} \gets ( P[ i ] - P[ b ] ,
///     b , i - b ) \cr
/// 10 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le L \le n \f$
template< class T >
Segment< T > maxDensitySegment( const std::vector< T > & D ,
                                std::size_t L );

/// 1 for the nucleotides C and G, in either case, and 0 for the others
[[nodiscard]] inline std::vector< int > gcIndicator(
                                                 const std::string & dna );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/MaximumSegments.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MaximumSegments.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File MaximumSegments.h -----------------------*/
/*--------------------------------------------------------------------------*/
