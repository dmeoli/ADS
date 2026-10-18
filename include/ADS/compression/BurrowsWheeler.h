/*--------------------------------------------------------------------------*/
/*------------------------- File BurrowsWheeler.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The transform of Burrows and Wheeler and the compressor built on it
 * (chapter 14 of the notes). The rows of the matrix M' are the rotations of
 * s$, $ a sentinel smaller than every symbol, in sorted order; the
 * transform bwt() is its last column L without the $, with the row r of
 * the $. Since the rows that start with a string w are contiguous, the
 * symbols that precede the occurrences of w are a substring of L, and on a
 * text whose symbols depend on their contexts, as those of a language do,
 * L is made of runs of few distinct symbols. The rows of M' are the
 * suffixes of s$ in order, so L[ i ] = s[ SA[ i ] - 1 ] (Theorem 14.6); the
 * suffix array SA is built here by prefix doubling (Manber and Myers),
 * \f$ \lceil \log_2 ( n + 1 ) \rceil \f$ rounds of sorting the suffixes by
 * the ranks of their halves, \f$ O( n \log^2 n ) \f$ time with a comparison
 * sort, a simple sort that any faster construction of the suffix array can
 * replace (the linear Skew of Karkkainen and Sanders, or SA-IS of Nong,
 * Zhang and Chan). The inverse transform inverseBwt() uses the LF-mapping:
 * the k-th occurrence of c in L and the k-th in the first column F are the
 * same symbol of s
 * (Property 14.2), hence \f$ LF[ i ] = C[ L[ i ] ] + rank_{L[ i ]}( L , i )
 * \f$, with C[ c ] the number of the symbols smaller than c; from the row
 * 0, which is $s and ends with s[ n - 1 ], each step of LF moves one symbol
 * to the left in s, n steps in O( n ) time (Theorem 14.3), each a likely
 * miss of the cache.
 *
 * The transforms that follow (section 14.2): the move-to-front of Bentley,
 * Sleator, Tarjan and Wei replaces each symbol with its place in a list of
 * the alphabet and moves it to the front, so the runs of few symbols of L
 * become runs of small integers, O( |\f$ \Sigma \f$| ) time per symbol
 * here; its \f$ \gamma \f$-code takes at most \f$ 2 H_0 + 1 \f$ bits per
 * symbol (Theorem 14.5). The run-length encoding replaces a run of
 * \f$ \ell \f$ symbols c with \f$ \langle \ell , c \rangle \f$. The bzip of
 * section 14.3 chains them: the transform of each block, the move-to-front
 * of L, then the run-length encoding RLE0 of the runs of 0 alone, the
 * other integers grown by 1 and a run of m zeros written in the code of
 * Wheeler, the binary digits of m + 1 after its leading 1 as the symbols 0
 * and 1, and at last an entropy code, here the canonical code of Huffman of
 * the block with lengths up to 20 [see CanonicalHuffman.h], as in bzip2
 * (which writes the digits of the runs from the least significant and
 * splits the block in groups coded by up to 6 tables). Coding L by
 * contexts gives the entropy of order k for every k at once (the boosting
 * of Ferragina, Giancarlo, Manzini and Sciortino, section 14.4), and
 * Manzini bounds the chain of the transform, MTF, RLE and an arithmetic
 * code by \f$ ( 5 + \epsilon ) n H_k^*( s ) \f$ bits plus lower terms,
 * \f$ H_k^* \f$ a variant of the entropy of order k [see Entropy.h]. The
 * transform is also an index [see FMIndex.h].
 *
 * The contracts [see Contract.h]: the row r is that of a transform, each
 * symbol of a move-to-front is in its alphabet of distinct symbols, a run
 * is positive, the integers of RLE0 are at most 256, a block is positive;
 * inverse( transform( x ) ) == x, an audit. The functions are defined in
 * BurrowsWheeler.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 14
 * \see M. Burrows, D. J. Wheeler, A block-sorting lossless data compression
 *      algorithm, Technical Report 124, Digital Systems Research Center,
 *      1994
 * \see U. Manber, G. Myers, Suffix arrays: a new method for on-line string
 *      searches, SIAM Journal on Computing 22(5), 935-948, 1993,
 *      doi:10.1137/0222058
 * \see J. L. Bentley, D. D. Sleator, R. E. Tarjan, V. K. Wei, A locally
 *      adaptive data compression scheme, Communications of the ACM 29(4),
 *      320-330, 1986, doi:10.1145/5684.5688
 * \see G. Manzini, An analysis of the Burrows-Wheeler transform, Journal
 *      of the ACM 48(3), 407-430, 2001, doi:10.1145/382780.382782
 * \see P. Ferragina, R. Giancarlo, G. Manzini, M. Sciortino, Boosting
 *      textual compression in optimal linear time, Journal of the ACM
 *      52(4), 688-713, 2005, doi:10.1145/1082036.1082043
 * \see J. Karkkainen, P. Sanders, S. Burkhardt, Linear work suffix array
 *      construction, Journal of the ACM 53(6), 918-936, 2006,
 *      doi:10.1145/1217856.1217858
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
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

#ifndef __ADS_BurrowsWheeler
 #define __ADS_BurrowsWheeler
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/CanonicalHuffman.h"
#include "ADS/compression/PackedBits.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <map>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the transform of Burrows and Wheeler: the last column of M' without the
/// $, and the row of the $
struct BWT {
 std::string last;
 std::size_t primary;
 friend bool operator==( const BWT & a , const BWT & b ) = default;
 };

/// the transform of s
/// \post inverseBwt( result ) == s (an audit)
[[nodiscard]] inline BWT bwt( std::string_view s );

/// the LF-mapping of the n + 1 rows of the transform b
/// \pre b.primary is 0 if b.last is empty, in [ 1 , n ] otherwise
[[nodiscard]] inline std::vector< std::size_t > lfMapping( const BWT & b );

/// the text of the transform b
/// \pre as lfMapping()
[[nodiscard]] inline std::string inverseBwt( const BWT & b );

/// the places of the symbols of s in the list alphabet, each moved to the
/// front after its use
/// \pre alphabet has distinct symbols, each symbol of s in it
/// \post inverseMoveToFront( result , alphabet ) == s (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > moveToFront(
                              std::string_view s , std::string_view alphabet );

/// moveToFront() with the 256 bytes in their order
[[nodiscard]] inline std::vector< std::uint8_t > moveToFront(
                                                         std::string_view s );

/// the symbols of the places x in the list alphabet
/// \pre alphabet has distinct symbols, each place below its size
[[nodiscard]] inline std::string inverseMoveToFront(
                                       const std::vector< std::uint8_t > & x ,
                                       std::string_view alphabet );

/// inverseMoveToFront() with the 256 bytes in their order
[[nodiscard]] inline std::string inverseMoveToFront(
                                      const std::vector< std::uint8_t > & x );

/// the maximal runs of s as ( length , symbol )
/// \post runLengthDecode( result ) == s (an audit)
[[nodiscard]] inline std::vector< std::pair< std::size_t , char > >
                                        runLengthEncode( std::string_view s );

/// the text of the runs
/// \pre each length positive
[[nodiscard]] inline std::string runLengthDecode(
                   const std::vector< std::pair< std::size_t , char > > & R );

/// RLE0 of bzip: the integers of x but 0 grown by 1, each run of m zeros in
/// the code of Wheeler, the digits of m + 1 after the leading 1
/// \post zeroRunDecode( result ) == x (an audit)
[[nodiscard]] inline std::vector< std::uint32_t > zeroRunEncode(
                                       const std::vector< std::uint8_t > & x );

/// the integers of RLE0
/// \pre each integer at most 256
[[nodiscard]] inline std::vector< std::uint8_t > zeroRunDecode(
                                      const std::vector< std::uint32_t > & y );

/// the bzip of s: blocks of at most block symbols, each transformed, moved
/// to front, coded by RLE0 and by its canonical code of Huffman
/// \pre block > 0
/// \post bzipDecompress( result ) == s (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > bzipCompress(
                            std::string_view s , std::size_t block = 900000 );

/// the text of a bzip
/// \pre code is a bzip
[[nodiscard]] inline std::string bzipDecompress(
                                     std::span< const std::uint8_t > code );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/BurrowsWheeler.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BurrowsWheeler.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BurrowsWheeler.h ------------------------*/
/*--------------------------------------------------------------------------*/
