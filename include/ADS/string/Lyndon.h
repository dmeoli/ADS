/*--------------------------------------------------------------------------*/
/*----------------------------- File Lyndon.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Lyndon factorization and the least rotation of a string, beyond the
 * book. A Lyndon word is a nonempty string strictly smaller than all its
 * proper rotations (equivalently, than all its proper suffixes), and every
 * string S has one factorization
 * \f[
 *   S = w_1 w_2 \cdots w_k , \qquad w_1 \ge w_2 \ge \cdots \ge w_k ,
 * \f]
 * into Lyndon words (Chen, Fox and Lyndon).
 *
 * - lyndonFactorization() is the algorithm of Duval: it keeps S = s_1 s_2 s_3
 *   with s_1 factored, \f$ s_2 = w^h w' \f$ for a Lyndon word w and a proper
 *   prefix w' of w, and compares the next character c of \f$ s_3 \f$ with the
 *   one |w| places before: if equal, w' grows; if larger, \f$ s_2 c \f$ is a
 *   Lyndon word and becomes w; if smaller, the h copies of w are factors and
 *   the scan resumes after them. Each comparison either advances the scan or
 *   is followed by the output of a factor at least as long as the characters
 *   rescanned, so that the time is \f$ O( n ) \f$, in \f$ O( 1 ) \f$ space
 *   besides the factors.
 * - leastRotation() gives the start of the lexicographically least
 *   rotation of S (the first one if several), by the same scan on S S: the
 *   least rotation starts at the start of the factor of S S that covers
 *   the place n, \f$ O( n ) \f$ time.
 *
 * The contracts [see Contract.h]: the factors are Lyndon words in
 * non-increasing order that give S back (an audit), and the rotation is
 * not larger than any other (an audit, \f$ O( n^2 ) \f$). The functions
 * are defined in Lyndon.tpp.
 *
 * \see K. T. Chen, R. H. Fox, R. C. Lyndon, Free differential calculus,
 *      IV. The quotient groups of the lower central series, Annals of
 *      Mathematics 68(1), 81-95, 1958, doi:10.2307/1970044
 * \see J.-P. Duval, Factorizing words over an ordered alphabet, Journal of
 *      Algorithms 4(4), 363-381, 1983, doi:10.1016/0196-6774(83)90017-2
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

#ifndef __ADS_Lyndon
 #define __ADS_Lyndon
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// S is a Lyndon word: nonempty and smaller than all its proper suffixes,
/// \f$ O( n^2 ) \f$ by the definition
[[nodiscard]] inline bool isLyndon( std::string_view S );

/// the starts of the factors of the Lyndon factorization of S, by Duval
/// \post Lyndon words, non-increasing, that give S back (an audit)
[[nodiscard]] inline std::vector< std::size_t > lyndonFactorization(
                                                        std::string_view S );

/// the start of the least rotation of S, 0 if S is empty
/// \post no rotation is smaller (an audit)
[[nodiscard]] inline std::size_t leastRotation( std::string_view S );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Lyndon.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Lyndon.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Lyndon.h ----------------------------*/
/*--------------------------------------------------------------------------*/
