/*--------------------------------------------------------------------------*/
/*-------------------------- File Segmentations.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The number of ways of writing a sequence X of n items as a concatenation
 * of words, the counting version of the word break problem [see
 * WordBreak.h, which finds one of them].
 *
 * - countSegmentations(): D[ i ], the segmentations of X[ i .. n ), is the sum
 *   of D[ i + |w| ] over the words w of the list that occur in X at i, D[ n ]
 *   = 1, filled from the right in \f$ O( n \sum_w |w| ) \f$ time and
 *   \f$ O( n ) \f$ space. With the automaton of Aho and Corasick on the words,
 *   the occurrences ending at each place come in one scan, and the time drops
 *   to \f$ O( n + \sum_w |w| + occ ) \f$ for occ occurrences.
 * - countDecodings(): the readings of a string of digits as letters, A = 1,
 *   ..., Z = 26, the segmentations into the words 1, ..., 26: D[ i ] = 0 if
 *   d_i = 0, else D[ i + 1 ] plus D[ i + 2 ] if d_i d_{i+1} is in 10 .. 26;
 *   \f$ \Theta( n ) \f$ time and \f$ O( 1 ) \f$ space, the two last entries of
 *   D. On n digits 1 the count is the Fibonacci number \f$ F_{n+1} \f$.
 *
 * The counts grow exponentially with n, and they are given modulo
 * \f$ 2^{64} \f$. countSegmentations() takes any sequence S with size(),
 * operator[] and the comparison of its items; the overload on std::string
 * also takes string literals.
 *
 * The contracts [see Contract.h]: no word is empty, and the digits are in
 * 0 .. 9. The functions are defined in Segmentations.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see A. V. Aho, M. J. Corasick, Efficient string matching: an aid to
 *      bibliographic search, Communications of the ACM 18(6), 333-340, 1975
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

#ifndef __ADS_Segmentations
 #define __ADS_Segmentations
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the number of ways of writing X as a sequence of the words, modulo
/// 2^64; a word repeated in the list counts once for each copy; 1 for the
/// empty X
/// \pre no word is empty
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::uint64_t countSegmentations(
                             const S & X , const std::vector< S > & words );

/// countSegmentations() on std::string (or literals)
[[nodiscard]] inline std::uint64_t countSegmentations(
            const std::string & X , const std::vector< std::string > & words );

/// the number of readings of the digits as letters A = 1, ..., Z = 26,
/// modulo 2^64; 1 for no digits
/// \pre every digit in 0 .. 9
[[nodiscard]] inline std::uint64_t countDecodings(
                                           const std::vector< int > & digits );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Segmentations.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Segmentations.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Segmentations.h ------------------------*/
/*--------------------------------------------------------------------------*/
