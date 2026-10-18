/*--------------------------------------------------------------------------*/
/*----------------------------- File Entropy.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The empirical entropies against which the compressors of this folder are
 * measured. The entropy of order 0 of a distribution is
 * \f$ H_0 = \sum_\sigma P[ \sigma ] \log_2 ( 1 / P[ \sigma ] ) \f$ bits per
 * symbol, and that of a string s takes \f$ P[ \sigma ] = n_\sigma / |s|
 * \f$, the frequency of \f$ \sigma \f$ in s; it bounds from below the
 * average length of a prefix-free code (Shannon), and the code of Huffman
 * stays within one bit of it, \f$ H_0 \le L_H < H_0 + 1 \f$ [see
 * CanonicalHuffman.h], while the arithmetic code of s takes at most
 * \f$ 2 + |s| H_0 \f$ bits [see ArithmeticCoding.h]. The entropy of order
 * k of s is
 * \f$ H_k( s ) = \frac{1}{|s|} \sum_{w \in \Sigma^k} |w_s| H_0( w_s ) \f$,
 * where \f$ w_s \f$ is the string of the symbols that follow the
 * occurrences of w in s, in their order; \f$ H_k \le H_0 \f$, and it is the
 * measure reached by the models of context [see PPM.h] and by the
 * compressors built on the Burrows-Wheeler transform [see
 * BurrowsWheeler.h]. Each function takes \f$ O( |s| \log |s| ) \f$ time.
 *
 * The contracts [see Contract.h]: the frequencies are not negative and not
 * all 0. The functions are defined in Entropy.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 12 and 14
 * \see C. E. Shannon, A mathematical theory of communication, Bell System
 *      Technical Journal 27(3), 379-423, 1948,
 *      doi:10.1002/j.1538-7305.1948.tb01338.x
 * \see G. Manzini, An analysis of the Burrows-Wheeler transform, Journal
 *      of the ACM 48(3), 407-430, 2001, doi:10.1145/382780.382782
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

#ifndef __ADS_Entropy
 #define __ADS_Entropy
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the entropy of order 0, in bits per symbol, of the distribution
/// proportional to the frequencies freq
/// \pre no frequency negative, not all 0
template< class C , class F >
[[nodiscard]] double entropy( const std::map< C , F > & freq );

/// the empirical entropy of order 0 of s, in bits per symbol; 0 if s is
/// empty
[[nodiscard]] inline double entropy( std::string_view s );

/// the empirical entropy of order k of s, in bits per symbol; 0 if s is
/// empty
/// \post entropy( s , k ) <= entropy( s ) (up to the rounding)
[[nodiscard]] inline double entropy( std::string_view s , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/Entropy.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Entropy.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Entropy.h ---------------------------*/
/*--------------------------------------------------------------------------*/
