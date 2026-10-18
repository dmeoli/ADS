/*--------------------------------------------------------------------------*/
/*------------------------- File StringMatching.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact string matching, beyond the book: the occurrences of a
 * pattern P of m characters in a text T of n characters.
 *
 * - naiveMatching() tries each shift of P along T, O( n m );
 * - kmp(), the algorithm of Knuth, Morris and Pratt, first computes the
 *   failure function of P, the length of the longest proper border of each
 *   prefix (a border is both a prefix and a suffix), and then scans T once,
 *   shifting P by its failure on a mismatch, so that no character of T is
 *   compared again after a match, \f$ \Theta( n + m ) \f$.
 *
 * The empty pattern occurs at every place 0 , ... , n.
 *
 * strongFailure() is the failure function improved by Knuth: if the
 * character that follows the border of P[ 0 .. q - 1 ] is P[ q ] itself, a
 * mismatch on P[ q ] would repeat on it, and the border of that border is
 * taken instead; kmpStrong() is kmp() with it, which never compares a
 * character of T twice against the same character of P.
 *
 * The contracts [see Contract.h]: the failure function gives proper borders,
 * and kmp() the occurrences of naiveMatching() (checked at level 3). The
 * operations are defined in StringMatching.tpp.
 *
 * \see D. E. Knuth, J. H. Morris, V. R. Pratt, Fast pattern matching in
 *      strings, SIAM Journal on Computing 6(2), 323-350, 1977,
 *      doi:10.1137/0206024
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
