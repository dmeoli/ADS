/*--------------------------------------------------------------------------*/
/*------------------------- File StringMatching.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact string matching, beyond the book: the occurrences of a
 * pattern P of m characters in a text T of n characters, the places
 * \f$ s \in [ 0 , n - m ] \f$ with \f$ T[ s .. s + m - 1 ] = P \f$. These
 * are the algorithms of the textbooks, which count their comparisons of
 * characters where asked; the engineered search of ADS is StringSearcher
 * [see StringSearch.h].
 *
 * - naiveMatching() tries each shift of P along T, \f$ O( n m ) \f$;
 * - kmp(), the algorithm of Knuth, Morris and Pratt, first computes the
 *   failure function of P, \f$ F[ q ] \f$ the length of the longest proper
 *   border of \f$ P[ 0 .. q - 1 ] \f$ (a border is both a prefix and a
 *   suffix), and then scans T once, shifting P by its failure on a
 *   mismatch, so that no character of T is compared again after a match:
 *   at most \f$ 2 n \f$ comparisons, \f$ \Theta( n + m ) \f$ time.
 *   strongFailure() is the failure function improved by Knuth, \f$ G[ q ] =
 *   G[ F[ q ] ] \f$ if \f$ P[ F[ q ] ] = P[ q ] \f$ and \f$ F[ q ] \f$
 *   otherwise: a mismatch on P[ q ] would repeat on the character after the
 *   border; kmpStrong() is kmp() with it, which never compares a character
 *   of T twice against the same character of P;
 * - boyerMoore() compares P from right to left and shifts by the larger of
 *   the bad character rule (the last place of the character of T in P) and
 *   the strong good suffix rule (the next place of the suffix matched,
 *   preceded by another character, or the longest prefix of P that is its
 *   suffix), both prepared in \f$ O( m + \sigma ) \f$; \f$ O( n / m ) \f$
 *   comparisons at best, \f$ O( n m ) \f$ in the worst case for all the
 *   occurrences (O( n ) for the first one);
 * - horspool() shifts by the bad character of the last character of the
 *   window alone, \f$ \mathit{shift}[ c ] = m - 1 - \max\{ i < m - 1 :
 *   P[ i ] = c \} \f$ (m if none), the simplest and often the fastest of
 *   the family on large alphabets, \f$ O( n m ) \f$ in the worst case;
 * - shiftOr() (m <= 64) keeps in a word D the prefixes of P that end at the
 *   current character, a clear bit i when \f$ P[ 0 .. i ] \f$ does, \f$ D
 *   \leftarrow ( D \ll 1 ) \vee B[ T[ j ] ] \f$ with the bit i of \f$ B[ c ]
 *   \f$ clear if \f$ P[ i ] = c \f$, \f$ \Theta( n ) \f$ word operations;
 * - zFunction() gives \f$ Z[ i ] = \mathrm{lcp}( S , S[ i .. ] ) \f$ for every
 *   i in \f$ \Theta( n ) \f$, from the Z-box \f$ [ l , r ) \f$ that reaches
 *   farthest: inside it \f$ Z[ i ] \ge \min( Z[ i - l ] , r - i ) \f$, and
 *   only the characters past r are compared; zMatching() extends the boxes
 *   of T against P in the same way, \f$ \Theta( n + m ) \f$;
 * - twoWay() is the Two-Way algorithm of Crochemore and Perrin [see
 *   StringSearch.h]: the critical factorization P = u v by two maximal
 *   suffixes, v compared from left to right and u from right to left, at
 *   most \f$ 2 n \f$ comparisons in O( 1 ) extra space.
 *
 * The empty pattern occurs at every place 0 , ... , n.
 *
 * The contracts [see Contract.h]: the failure function gives proper
 * borders, the Z-function values that fit the string, and every search the
 * occurrences of naiveMatching() (checked at level 3). The operations are
 * defined in StringMatching.tpp.
 *
 * \see D. E. Knuth, J. H. Morris, V. R. Pratt, Fast pattern matching in
 *      strings, SIAM Journal on Computing 6(2), 323-350, 1977,
 *      doi:10.1137/0206024
 * \see R. S. Boyer, J. S. Moore, A fast string searching algorithm,
 *      Communications of the ACM 20(10), 762-772, 1977,
 *      doi:10.1145/359842.359859
 * \see R. N. Horspool, Practical fast searching in strings, Software:
 *      Practice and Experience 10(6), 501-506, 1980,
 *      doi:10.1002/spe.4380100608
 * \see R. A. Baeza-Yates, G. H. Gonnet, A new approach to text searching,
 *      Communications of the ACM 35(10), 74-82, 1992,
 *      doi:10.1145/135239.135243
 * \see D. Gusfield, Algorithms on Strings, Trees and Sequences, Cambridge
 *      University Press, 1997, section 1.4, doi:10.1017/CBO9780511574931
 * \see M. Crochemore, D. Perrin, Two-way string-matching, Journal of the
 *      ACM 38(3), 650-674, 1991, doi:10.1145/116825.116845
 * \see C. Charras, T. Lecroq, Handbook of Exact String Matching
 *      Algorithms, King's College Publications, 2004, ISBN 0-9543006-4-5
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

#ifndef __ADS_StringMatching
 #define __ADS_StringMatching
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <cstdint>
#include "ADS/Contract.h"

#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the places of all the occurrences of P in T, by trying every shift
inline std::vector< std::size_t > naiveMatching( std::string_view T ,
                                                 std::string_view P );

/// the failure function: F[ q ] is the length of the longest proper
/// border of P[ 0 .. q - 1 ], F[ 0 ] = -1
/// \post F[ q ] < q for q > 0
inline std::vector< long > failure( std::string_view P );

/// the places of all the occurrences of P in T, by Knuth-Morris-Pratt;
/// the comparisons of characters go to *comparisons, if given
/// \post the occurrences of naiveMatching( T , P )
inline std::vector< std::size_t > kmp( std::string_view T ,
                                       std::string_view P ,
                                       long * comparisons = nullptr );

/// the strong failure function: G[ q ] = F[ q ] unless P[ F[ q ] ] =
/// P[ q ], in which case G[ q ] = G[ F[ q ] ]; G[ m ] = F[ m ]
/// \post G[ q ] <= F[ q ]
inline std::vector< long > strongFailure( std::string_view P );

/// kmp() with the strong failure function on a mismatch
/// \post the occurrences of naiveMatching( T , P )
inline std::vector< std::size_t > kmpStrong( std::string_view T ,
                                             std::string_view P ,
                                             long * comparisons = nullptr );

/// the places of all the occurrences of P in T, by Boyer-Moore with the
/// bad character and the strong good suffix rules
/// \post the occurrences of naiveMatching( T , P ) (an audit)
inline std::vector< std::size_t > boyerMoore( std::string_view T ,
                                              std::string_view P ,
                                              long * comparisons = nullptr );

/// the places of all the occurrences of P in T, by Horspool
/// \post the occurrences of naiveMatching( T , P ) (an audit)
inline std::vector< std::size_t > horspool( std::string_view T ,
                                            std::string_view P ,
                                            long * comparisons = nullptr );

/// the places of all the occurrences of P in T, by Shift-Or
/// \pre P.size() <= 64
/// \post the occurrences of naiveMatching( T , P ) (an audit)
inline std::vector< std::size_t > shiftOr( std::string_view T ,
                                           std::string_view P );

/// the Z-function: Z[ i ] is the length of the longest common prefix of S
/// and S[ i .. ], Z[ 0 ] = |S|
/// \post Z[ i ] <= |S| - i
inline std::vector< std::size_t > zFunction( std::string_view S );

/// the places of all the occurrences of P in T, by the Z-function of P
/// and the Z-boxes of T
/// \post the occurrences of naiveMatching( T , P ) (an audit)
inline std::vector< std::size_t > zMatching( std::string_view T ,
                                             std::string_view P );

/// the places of all the occurrences of P in T, by the Two-Way algorithm
/// of Crochemore and Perrin
/// \post the occurrences of naiveMatching( T , P ) (an audit)
inline std::vector< std::size_t > twoWay( std::string_view T ,
                                          std::string_view P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/StringMatching.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StringMatching.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StringMatching.h ------------------------*/
/*--------------------------------------------------------------------------*/
