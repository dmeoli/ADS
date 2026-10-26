/*--------------------------------------------------------------------------*/
/*---------------------------- File MultiSearch.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered search of many patterns at once, beyond the book:
 * MultiSearcher finds all the occurrences in a text T of n bytes of k
 * patterns \f$ P_0 , \ldots , P_{k-1} \f$: one pattern by StringSearcher
 * [see StringSearch.h], up to 64 by a filter of shuffles of bytes, more
 * by a filter of shift-or on super characters, and by the automaton of
 * Aho and Corasick [see AhoCorasick.h] where a filter lets too much
 * through; the benchmark of the multiple search compares it with
 * AhoCorasick and with Vectorscan (the portable Hyperscan).
 *
 * The filter of shuffles puts the patterns, sorted, into 8 buckets of
 * consecutive ones, so that the patterns of a bucket tend to share their
 * first bytes (the patterns go into buckets, and the buckets are told apart
 * by the 8 bits of a byte, as in the FDR matcher of Hyperscan). The place i
 * of T is a candidate for the bucket b if, for every \f$ r < q \f$, some
 * pattern of b has the low half (4 bits) of \f$ T[ i + r ] \f$ as the low
 * half of its byte r and some pattern of b (maybe another) the high half as
 * the high half:
 * \f[
 *   C_i = \bigwedge_{ r < q } \mathit{lo}_r[ T[ i + r ] \bmod 16 ] \wedge
 *         \mathit{hi}_r[ \lfloor T[ i + r ] / 16 \rfloor ] ,
 * \f]
 * the bit b of the tables \f$ \mathit{lo}_r \f$ and \f$ \mathit{hi}_r \f$
 * (16 bytes each) set for the halves of the bytes r of the patterns of b.
 * q is the least number of bytes, at most 8 and at most the shortest
 * pattern, that brings the probability of a place to pass below
 * \f$ 2^{-10} \f$, the bytes of the text drawn with the frequencies of the
 * bytes of the patterns: \f$ \prod_{ r < q } \sum_x f( x ) [ C( x , r )
 * \neq 0 ] < 2^{-10} \f$, so that a large alphabet needs one or two bytes
 * and a small one more. A table of 16 bytes is one register, and one
 * shuffle of bytes (vpshufb) looks up 32 places of T at once, so that a
 * block of 32 places costs q loads, 2 q shuffles and a few logical
 * operations, with no branch but the one on a candidate. A candidate is
 * checked against the patterns of its buckets, by their first 8 bytes in
 * one comparison of words and then by the rest. Every occurrence makes
 * its place a candidate for its bucket, so that nothing is missed; when
 * the checks exceed 2 a place on average (patterns everywhere, or a text
 * whose bytes the filter cannot tell apart), the automaton goes on from
 * the place reached, its occurrences that start before it left out (and
 * count() takes the count of the automaton from a little before that
 * place, less the occurrences before it in a window of the length of the
 * longest pattern).
 *
 * Above 64 patterns the halves of the bytes of a bucket cover them all,
 * and the filter of shift-or of FDR takes over, on the first
 * \f$ L = \min( 8 , \ell ) \f$ bytes of the patterns, \f$ \ell \f$ the
 * shortest. The super character of the place p is \f$ T[ p ] \f$ with the
 * low 3 bits of \f$ T[ p + 1 ] \f$, 11 bits, and its mask of 64 bits has
 * the bit b of its byte \f$ L - 1 - r \f$ off if a pattern of the bucket b
 * has that super character at its place r < L (its byte r and the low
 * bits of the byte r + 1, any of them past its end), the bytes from L on
 * all off. A pattern that starts at s can then end its window at
 * \f$ e = s + L - 1 \f$ only if the bit b of the byte \f$ e - p \f$ of the
 * mask of p is off for every \f$ e - L < p \le e \f$:
 * \f[
 *   \mathit{St}_e = \bigvee_{ t < L } \mathit{sh}[ x( e - t ) ]_t
 * \f]
 * has a bit off. A block of 8 places adds each mask moved by d bytes into
 * a register of 16 bytes, the states of its 8 ends and of the next 8,
 * one load, one shift of bytes and one or a place, the 11 bits read from
 * two words of the text with no other load; the table of the masks is
 * 16 KB. A state with a bit off sends its start to a table of the
 * prefixes of L bytes of the patterns (chained, by a multiplicative hash),
 * which checks the patterns of that prefix. When the patterns have 8
 * bytes or more, the filter takes the masks of the even places alone (a
 * stride of 2, as FDR does on long patterns), half the loads: a window
 * of \f$ W = \min( 8 , \lfloor \ell / 2 \rfloor ) \f$ even places, and
 * each pattern twice, at its even offsets for the occurrences that start
 * at an even place (in 4 buckets, the bits 0 to 3) and at its odd ones
 * for the others (the bits 4 to 7), kept only if a window passes with
 * probability below 2^-8 on a text of the super characters of the
 * patterns, since its buckets hold twice the patterns.
 *
 * findAll() gives the pairs ( start , number ) of the occurrences, by
 * increasing start and number, count() their number; the patterns are
 * numbered in their order, and the empty pattern is not allowed.
 *
 * The contracts [see Contract.h]: a pattern is not empty; findAll() gives,
 * for each pattern, the occurrences of naiveMatching() [see
 * StringMatching.h] (an audit). The functions are defined in
 * MultiSearch.tpp.
 *
 * \see X. Wang, Y. Hong, H. Chang, K. Park, G. Langdale, J. Hu, H. Zhu,
 *      Hyperscan: a fast multi-pattern regex matcher for modern CPUs,
 *      Proceedings of the 16th USENIX Symposium on Networked Systems Design
 *      and Implementation (NSDI), 631-648, 2019,
 *      https://www.usenix.org/conference/nsdi19/presentation/wang-xiang
 * \see A. V. Aho, M. J. Corasick, Efficient string matching: an aid to
 *      bibliographic search, Communications of the ACM 18(6), 333-340,
 *      1975, doi:10.1145/360825.360855
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

#ifndef __ADS_MultiSearch
 #define __ADS_MultiSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/AhoCorasick.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS MultiSearcher ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the search of many patterns at once, by a SIMD filter or by the
/// automaton of Aho and Corasick
class MultiSearcher {

/*--------------------------------------------------------------------------*/
 public:

 /// the patterns P, numbered 0 , 1 , ... in their order
 /// \pre no pattern is empty
 explicit MultiSearcher( const std::vector< std::string > & P );

 /// the occurrences ( start , number ) of the patterns in T, by increasing
 /// start and number
 /// \post the occurrences of naiveMatching() for each pattern (an audit)
 [[nodiscard]] std::vector< std::pair< std::size_t , std::size_t > >
  findAll( std::string_view T ) const;

 /// the number of the occurrences of the patterns in T
 [[nodiscard]] std::size_t count( std::string_view T ) const;

 /// the number of patterns
 [[nodiscard]] std::size_t size( void ) const { return( f_P.size() ); }

 /// whether the SIMD filter searches (else the shift-or filter)
 [[nodiscard]] bool filtered( void ) const { return( f_q > 0 ); }

 /// the patterns of more than kFilter go to the shift-or filter
 static constexpr std::size_t kFilter = 64;

/*--------------------------------------------------------------------------*/
 private:

 /// the occurrences in T, each ( start , number ) given to report(), by
 /// increasing start; npos, or the place from which the automaton must
 /// go on, if the filter lets too much through
 template< class F >
 std::size_t scan( std::string_view T , F && report ) const;

 /// npos, as std::string_view::npos
 static constexpr std::size_t npos = std::string_view::npos;

 /// the occurrences that start at from or later, by the automaton, given
 /// to report() by increasing end
 template< class F >
 void automaton( std::string_view T , std::size_t from ,
                 F && report ) const;

 /// scan() by the shift-or filter on super characters
 template< class F >
 std::size_t shiftOr( std::string_view T , F && report ) const;

 /// shiftOr() of stride 2, on the masks of the even places
 template< class F >
 std::size_t shiftOr2( std::string_view T , F && report ) const;

 /// the occurrences that start at s, by the table of the prefixes of the
 /// shift-or filter; the number of the checks
 template< class F >
 std::size_t verify( const unsigned char * T , std::size_t n ,
                     std::size_t s , F && report ) const;

 /// the places s .. s + 63 whose bits are on in mask, candidates for the
 /// buckets in C[ 0 .. 63 ], checked against their patterns; the number
 /// of the checks
 template< class F >
 std::size_t check( const unsigned char * T , std::size_t n , std::size_t s ,
             std::uint64_t mask , const unsigned char * C ,
             F && report ) const;

 std::vector< std::string > f_P;     ///< the patterns
 std::size_t f_q = 0;                ///< the bytes of the filter, 0 if none
 std::size_t f_least = 0;            ///< the length of the shortest one
 std::size_t f_longest = 0;          ///< and of the longest one
 /// the halves of the bytes r < q: lo[ r ][ x ] and hi[ r ][ x ]
 std::array< std::array< std::uint8_t , 16 > , 8 > f_lo{} , f_hi{};
 std::array< std::vector< std::uint32_t > , 8 > f_bucket;  ///< patterns
 std::vector< std::uint64_t > f_head;  ///< the first 8 bytes of each
 std::vector< std::uint64_t > f_mask;  ///< those of them in the pattern
 std::size_t f_L = 0;                ///< the window of the shift-or filter
 std::vector< std::uint64_t > f_sh;  ///< its masks, by super character
 std::size_t f_W = 0;                ///< the window of stride 2, 0 if none
 std::vector< std::uint64_t > f_sh2;  ///< its masks
 std::vector< std::uint32_t > f_first;  ///< the table of the prefixes
 std::vector< std::uint32_t > f_next;   ///< its chains
 std::vector< std::uint64_t > f_key;    ///< the prefix of each pattern
 int f_bits = 0;                     ///< the bits of the table
 AhoCorasick f_ac;                   ///< on many checks

 };  // end( class MultiSearcher )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/MultiSearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MultiSearch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MultiSearch.h -------------------------*/
/*--------------------------------------------------------------------------*/
