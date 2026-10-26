/*--------------------------------------------------------------------------*/
/*----------------------- File SubsequenceProblems.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Four classic problems on the subsequences of sequences X, Y, Z of
 * lengths n, m, l, relatives of the longest common subsequence [see
 * LongestCommonSubsequence.h], each by dynamic programming on the
 * prefixes.
 *
 * - isInterleaving(): whether Z is a shuffle (an interleaving) of X and Y,
 *   the items of each in their order: D[ i ][ j ], whether Z[ 0 .. i + j )
 *   interleaves X[ 0 .. i ) and Y[ 0 .. j ), is D[ i - 1 ][ j ] with
 *   x_{i-1} = z_{i+j-1} or D[ i ][ j - 1 ] with y_{j-1} = z_{i+j-1};
 *   \f$ \Theta( n m ) \f$ time and \f$ \Theta( m ) \f$ space, one row of
 *   the table. Mansfield showed the problem NP-complete for the shuffle of
 *   an unbounded number of strings, and Warmuth and Haussler for the
 *   iterated shuffle of one string.
 * - shortestCommonSupersequence(): the shortest sequence that has X and Y
 *   as subsequences, of length n + m - | LCS( X , Y ) |, read back from the
 *   table of the LCS: the common items once, the others in their order;
 *   \f$ \Theta( n m ) \f$ time and space, while scsLength() keeps one row,
 *   \f$ \Theta( m ) \f$ space. For an unbounded number of strings the
 *   problem is NP-hard (Maier).
 * - subsequenceOccurrences(): the number of occurrences of P as a
 *   subsequence of T, the tuples of places j_1 < ... < j_m with
 *   t_{j_k} = p_k: C[ i ][ j ] = C[ i - 1 ][ j ] + [ t_{i-1} = p_{j-1} ]
 *   C[ i - 1 ][ j - 1 ], \f$ \Theta( n m ) \f$ time and one row updated
 *   from the right, \f$ \Theta( m ) \f$ space; the count grows as fast as
 *   the binomial coefficient \f$ \binom{n}{m} \f$, and it is given modulo
 *   \f$ 2^{64} \f$.
 * - lcs3(): a longest common subsequence of three sequences, the table of
 *   lcs() in three dimensions, \f$ \Theta( n m l ) \f$ time and space.
 *   The LCS of two of them and then of the third is not a solution: the
 *   longest pairwise one may leave out the items that the third shares.
 *   For k sequences the table has \f$ \prod n_i \f$ entries, and the
 *   problem is NP-hard for an unbounded k (Maier).
 *
 * Each function takes any sequence S with size(), operator[], push_back()
 * and the comparison of its items; the overloads on std::string also take
 * string literals.
 *
 * The contracts [see Contract.h]: the supersequence has X and Y as
 * subsequences and the length of scsLength(), and lcs3() gives a
 * subsequence of the three (audits). The functions are defined in
 * SubsequenceProblems.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see D. Maier, The complexity of some problems on subsequences and
 *      supersequences, Journal of the ACM 25(2), 322-336, 1978
 * \see A. Mansfield, On the computational complexity of a merge recognition
 *      problem, Discrete Applied Mathematics 5(1), 119-122, 1983
 * \see M. K. Warmuth, D. Haussler, On the complexity of iterated shuffle,
 *      Journal of Computer and System Sciences 28(3), 345-358, 1984
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

#ifndef __ADS_SubsequenceProblems
 #define __ADS_SubsequenceProblems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/LongestCommonSubsequence.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether Z interleaves X and Y keeping the order of the items of each
template< class S > requires std::is_class_v< S >
[[nodiscard]] bool isInterleaving( const S & X , const S & Y , const S & Z );

/// isInterleaving() on std::string (or literals)
[[nodiscard]] inline bool isInterleaving( const std::string & X ,
                                          const std::string & Y ,
                                          const std::string & Z );

/// the length of a shortest common supersequence of X and Y
/// \post n + m - | lcs( X , Y ) | (an audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::size_t scsLength( const S & X , const S & Y );

/// scsLength() on std::string (or literals)
[[nodiscard]] inline std::size_t scsLength( const std::string & X ,
                                            const std::string & Y );

/// a shortest common supersequence of X and Y
/// \post X and Y subsequences of it (an audit), of length scsLength()
template< class S > requires std::is_class_v< S >
[[nodiscard]] S shortestCommonSupersequence( const S & X , const S & Y );

/// shortestCommonSupersequence() on std::string (or literals)
[[nodiscard]] inline std::string shortestCommonSupersequence(
                               const std::string & X , const std::string & Y );

/// the number of occurrences of P as a subsequence of T, modulo 2^64: 1
/// for the empty P, 0 if P is longer than T
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::uint64_t subsequenceOccurrences( const S & P ,
                                                    const S & T );

/// subsequenceOccurrences() on std::string (or literals)
[[nodiscard]] inline std::uint64_t subsequenceOccurrences(
                               const std::string & P , const std::string & T );

/// a longest common subsequence of X, Y and Z
/// \post a subsequence of the three (an audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] S lcs3( const S & X , const S & Y , const S & Z );

/// lcs3() on std::string (or literals)
[[nodiscard]] inline std::string lcs3( const std::string & X ,
                                       const std::string & Y ,
                                       const std::string & Z );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SubsequenceProblems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SubsequenceProblems.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SubsequenceProblems.h ---------------------*/
/*--------------------------------------------------------------------------*/
