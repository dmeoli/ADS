/*--------------------------------------------------------------------------*/
/*------------------------- File MaximumSegments.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The segments (subarrays) of largest sum and their variants, chapter 2 of
 * the notes: the gains of a stock as the differences of its quotations,
 * and the regions of a DNA sequence rich in C and G.
 *
 * - maxSumSegmentCubic() and maxSumSegmentQuadratic() are Algorithms 2.1
 *   and 2.2, all the pairs ( b , s ), the sum recomputed or extended,
 *   \f$ \Theta( n^3 ) \f$ and \f$ \Theta( n^2 ) \f$.
 * - maxSumSegment() is Algorithm 2.3, the scan of Kadane with the
 *   positions: a prefix of negative sum is dropped, since no optimum
 *   starts after a segment of positive sum nor has a prefix of negative
 *   sum (Properties 1 and 2), O( n ) time, one pass, \f$ \lceil n / B
 *   \rceil \f$ I/Os whatever B (cache oblivious). maxSubarraySum() of
 *   Subsequences.h is the same scan for the value only, the empty segment
 *   allowed; here the segment is not empty, so that an array of negative
 *   numbers gives its largest one.
 * - maxSumSegmentPrefix() is Algorithm 2.4: with the prefix sums P, the
 *   best segment ending in s is \f$ P[ s ] - \min_{b < s} P[ b ] \f$, the
 *   minimum carried along the scan.
 * - maxSumSegmentLength() bounds the length to \f$ [ L , U ] \f$: the
 *   minimum of P over the window of the admissible starts, by a deque of
 *   increasing prefix sums (the sliding minimum), O( n ), the algorithm of
 *   Lin, Jiang and Chao.
 * - longestSegmentAtLeast() is the algorithm of section 2.5 for the
 *   longest segment of sum at least t: at step i only the starts left of
 *   \f$ L_i = i - \ell \f$ can beat the current length \f$ \ell \f$, and
 *   among them only the chain of leftmost minima \f$ C_1 = LMin[ L_i ] ,
 *   C_2 = LMin[ C_1 ] , \ldots \f$, whose prefix sums increase (Fact 2.2);
 *   the chain is walked while the sum stays at least t, and every step
 *   past the first lengthens the solution, hence O( n ) amortized.
 *   longestSegmentAverageAtLeast() reduces the longest segment of average
 *   at least p / q to it, by \f$ q D[ k ] - p \f$.
 * - maxDensitySegment() is the segment of largest average among those of
 *   length at least L: the average of D[ x .. i ) is the slope from
 *   \f$ ( x , P[ x ] ) \f$ to \f$ ( i , P[ i ] ) \f$, and the best x for i
 *   lies on the lower convex hull of the points \f$ x \leq i - L \f$, kept
 *   by the monotone chain as i grows, the tangent found by a binary search,
 *   \f$ O( n \log n ) \f$. Goldwasser, Kao and Lu, and Chung and Lu, reach
 *   O( n ) by walking the hull with a pointer that moves only forward under
 *   a further argument (and also handle an upper bound on the length); the
 *   binary search keeps the code short and exact on integers.
 * - gcIndicator() maps a DNA sequence to 1 for C and G and 0 otherwise,
 *   the input of the two biological measures of the notes: \f$ x - p \ell
 *   \f$ (a segment sum of the scores \f$ 1 - p \f$ and \f$ -p \f$, e.g.,
 *   q D[ k ] - p for p / q) and the density \f$ x / \ell \f$.
 *
 * A Segment holds the sum of D[ first .. first + length ), length 0 for
 * none. The contracts [see Contract.h]: D not empty where a segment must
 * exist, \f$ 1 \leq L \leq U \f$, \f$ L \leq n \f$, q > 0; the sum of a
 * segment is that of its items, and maxSumSegment() agrees with
 * maxSubarraySum() when its sum is not negative, audits. The sums of the
 * segments and of the prefixes of D must fit in T, which is not checked
 * (an item of T alone may be its largest or its smallest value). The
 * functions are defined in MaximumSegments.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 2
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
 *      segment problem, SIAM Journal on Computing 34(2), 373-387, 2004,
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

/// the segment D[ first .. first + length ) and its sum
template< class T >
struct Segment {
 T sum{};
 std::size_t first = 0;
 std::size_t length = 0;                    ///< 0 for no segment
 };

/// a nonempty segment of largest sum, by all the pairs, O( n^3 )
/// \pre D not empty
template< class T >
Segment< T > maxSumSegmentCubic( const std::vector< T > & D );

/// a nonempty segment of largest sum, by all the pairs, O( n^2 )
/// \pre D not empty
template< class T >
Segment< T > maxSumSegmentQuadratic( const std::vector< T > & D );

/// a nonempty segment of largest sum, by one scan, O( n )
/// \pre D not empty
/// \post its sum is maxSubarraySum( D ) if not negative (an audit)
template< class T >
Segment< T > maxSumSegment( const std::vector< T > & D );

/// a nonempty segment of largest sum, by the minimum prefix sum, O( n )
/// \pre D not empty
template< class T >
Segment< T > maxSumSegmentPrefix( const std::vector< T > & D );

/// a segment of largest sum among those of length in [ L , U ], O( n )
/// \pre 1 <= L <= U and L <= the size of D
template< class T >
Segment< T > maxSumSegmentLength( const std::vector< T > & D ,
                                  std::size_t L , std::size_t U );

/// the longest segment of sum at least t, length 0 if none, O( n )
/// \post its sum is that of its items and at least t (an audit)
template< class T >
Segment< T > longestSegmentAtLeast( const std::vector< T > & D , T t );

/// the longest segment of average at least p / q, length 0 if none
/// \pre q > 0
template< class T >
Segment< T > longestSegmentAverageAtLeast( const std::vector< T > & D ,
                                           T p , T q );

/// a segment of largest average among those of length at least L, its
/// sum in the result, O( n log n )
/// \pre 1 <= L <= the size of D
template< class T >
Segment< T > maxDensitySegment( const std::vector< T > & D ,
                                std::size_t L );

/// 1 for the nucleotides C and G, 0 for the others
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
