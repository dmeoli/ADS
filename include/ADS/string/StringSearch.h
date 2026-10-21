/*--------------------------------------------------------------------------*/
/*-------------------------- File StringSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact search of a pattern P of m bytes in a text T of n bytes,
 * beyond the book: the engineered search of ADS, the one that the
 * benchmark of the searches compares with memmem() of the C library,
 * std::search() with its three searchers and the searchers of
 * Boost.Algorithm, while kmp() [see StringMatching.h] stays the algorithm
 * of the textbook. The occurrences are the places
 * \f[
 *   \mathrm{Occ}( P , T ) = \{ s \in [ 0 , n - m ] : T[ s .. s + m - 1 ]
 *                            = P \} .
 * \f]
 *
 * StringSearcher prepares P once, as the searchers of the standard
 * library do, and then finds its occurrences in any text:
 *
 * - P of one byte is found by memchr();
 * - P of fewer than kSkip bytes goes through the filter of k probes (the
 *   packed filters of Faro and Kulekci and of Mula): the places \f$ o_1 ,
 *   \ldots , o_k \f$ of the rarest bytes \f$ b_i = P[ o_i ] \f$ of P, k = 2,
 *   or 4 if P hints at a small alphabet (fewer than 8 distinct bytes in 8
 *   or more, or a repeated byte in fewer, as on DNA), and the start s is
 *   compared with P by memcmp() only if \f$ T[ s + o_i ] = b_i \f$ for
 *   every i. The test is made for W consecutive starts at once by k vector
 *   comparisons of W bytes and their AND, AVX-512 (W = 64), AVX2 (32) or
 *   SSE2 (16) as the target allows, four blocks a step with the
 *   comparisons with P out of the way of the probes (elsewhere memchr()
 *   finds the first probe and the others are checked one by one), so that
 *   on a text of
 *   independent bytes of probabilities \f$ p_c \f$ the expected number of
 *   comparisons with P is \f$ ( n - m + 1 ) \prod_i p_{b_i} \f$, e.g.,
 *   \f$ n / 256 \f$ on DNA with four probes. The rarest bytes are those of
 *   a fixed ranking of the bytes by their frequency in text (the letters of
 *   English by frequency, the blank and the newline the most common, the
 *   control and the high bytes the rarest), the later place on a tie;
 * - P of kSkip bytes or more is searched by the shifts of its q-grams, in
 *   the spirit of Horspool and of Wu and Manber, as the hashed q-grams of
 *   Lecroq: q is the least length with \f$ d^q \ge 16 m \f$ for the d
 *   distinct bytes of P, \f$ 2 \le q \le 8 \f$, a q-gram g is hashed to
 *   \f$ h( g ) \in [ 0 , 4096 ) \f$ (the 12 top bits of its product by
 *   \f$ \lfloor 2^{64} / \phi \rfloor \f$), and
 *   \f[
 *     \mathit{shift}[ x ] = \min\bigl\{ 255 , m - q + 1 , \min\{ m - q - i :
 *       0 \le i < m - q , \; h( P[ i .. i + q - 1 ] ) = x \} \bigr\} ,
 *   \f]
 *   0 for the hash of the last q-gram of P; the window at s moves to \f$ s
 *   + \mathit{shift}[ h( T[ s + m - q .. s + m - 1 ] ) ] \f$, and a window
 *   of shift 0 is compared with P and moves by the shift that the last
 *   q-gram of P has among the others. A shift never passes an occurrence,
 *   since it is at most the distance to the last place where the q-gram at
 *   the end of the window occurs in P. On random texts a window moves by
 *   about m - q + 1, \f$ O( n / m ) \f$ windows of one load of 8 bytes; the
 *   text 1024 bytes ahead is prefetched, and the four quarters of a text
 *   of 16 KB or more are searched at once, their windows in registers, so
 *   that the chains of loads of the four overlap (the occurrences of a
 *   quarter wait for the end of the ones before it). When 64 steps have
 *   moved the first window by less than 16 m (a text of few distinct
 *   q-grams), the filter of the probes takes over;
 * - every search counts its comparisons with P, m bytes each, those that find
 *   an occurrence too: once their bytes exceed \f$ 16 ( s - s_0 ) + 4096 \f$
 *   at the window s of a search begun at \f$ s_0 \f$, the search goes on with
 *   the Two-Way algorithm of Crochemore and Perrin, so that no text costs more
 *   than \f$ 16 n + 4096 + m \f$ bytes compared before it, and \f$ 2 n \f$
 *   comparisons after it, O( n + m ) in all (e.g., a pattern that agrees with
 *   a periodic text on its probes and nowhere else, or one that occurs at
 *   every place, \f$ a^m \f$ in \f$ a^n \f$, which cost \f$ \Theta( n m ) \f$
 *   to the comparisons with P alone). The Two-Way algorithm cuts P at a
 *   critical factorization P = u v, whose local period at |u| is the period p
 *   of P, found by the two maximal suffixes of P for an order of the bytes and
 *   its reverse (the longer one gives |u|); it compares v from left to right
 *   and then u from right to left, and moves by p, with a memory of the prefix
 *   of P that is known to match, if u is a suffix of \f$ P[ 0 .. p + |u| - 1 ]
 *   \f$, or else by \f$ \max( |u| , |v| ) + 1 \f$, in O( 1 ) extra space and
 *   at most \f$ 2 n \f$ comparisons, made here 8 bytes at a time in both
 *   directions; its factorization, O( m ), is made only when a search
 *   comes to it.
 *
 * Preparing P takes O( 256 + m ) time (the last place of each byte gives
 * the probes) and 4 KB for the shifts; find() gives the first occurrence at
 * or after from (npos if none), findAll() all of them, overlapping ones
 * included, in increasing order, count() their number, and operator() the
 * range of the first occurrence in [ first , last ), the interface of the
 * searchers of std::search(). The empty pattern occurs at every place 0 ,
 * ... , n. The functions findFirst() and findAll() of the namespace prepare
 * P and search once.
 *
 * The contracts [see Contract.h]: an occurrence found is one, and
 * findAll() gives those of naiveMatching() [see StringMatching.h] (an
 * audit, O( n m ) time). The functions are defined in StringSearch.tpp.
 *
 * \see S. Faro, M. O. Kulekci, Fast packed string matching for short
 *      patterns, Proceedings of the 15th Workshop on Algorithm Engineering
 *      and Experiments (ALENEX), 113-121, 2013,
 *      doi:10.1137/1.9781611972931.10
 * \see W. Mula, SIMD-friendly algorithms for substring searching, 2016,
 *      http://0x80.pl/notesen/2016-11-28-simd-strfind.html
 * \see R. N. Horspool, Practical fast searching in strings, Software:
 *      Practice and Experience 10(6), 501-506, 1980,
 *      doi:10.1002/spe.4380100608
 * \see S. Wu, U. Manber, A fast algorithm for multi-pattern searching,
 *      Technical Report TR-94-17, Department of Computer Science,
 *      University of Arizona, 1994,
 *      https://www.cs.arizona.edu/sites/default/files/TR94-17.pdf
 * \see T. Lecroq, Fast exact string matching algorithms, Information
 *      Processing Letters 102(6), 229-235, 2007,
 *      doi:10.1016/j.ipl.2007.01.002
 * \see M. Crochemore, D. Perrin, Two-way string-matching, Journal of the
 *      ACM 38(3), 650-674, 1991, doi:10.1145/116825.116845
 * \see S. Faro, T. Lecroq, The exact online string matching problem: a
 *      review of the most recent results, ACM Computing Surveys 45(2),
 *      13:1-13:42, 2013, doi:10.1145/2431211.2431212
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

#ifndef __ADS_StringSearch
 #define __ADS_StringSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/StringMatching.h"

#include <cstddef>
#include <array>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// the width of the vector comparisons of the filter: 3 AVX-512 (64
/// bytes), 2 AVX2 (32), 1 SSE2 (16), 0 none (memchr() and bytes); by
/// default the widest that the target has, and the same in all the
/// translation units of a program
#ifndef ADS_SEARCH_SIMD
 #if defined( __AVX512BW__ )
  #define ADS_SEARCH_SIMD 3
 #elif defined( __AVX2__ )
  #define ADS_SEARCH_SIMD 2
 #elif defined( __SSE2__ ) || defined( _M_X64 )
  #define ADS_SEARCH_SIMD 1
 #else
  #define ADS_SEARCH_SIMD 0
 #endif
#endif

#if ADS_SEARCH_SIMD > 0
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS StringSearcher ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a pattern prepared for the search of its occurrences in any text
class StringSearcher {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t npos = std::string_view::npos;

 /// the patterns of kSkip bytes or more are searched by the shifts of the
 /// q-grams
 static constexpr std::size_t kSkip = 64;

 /// the empty pattern
 StringSearcher( void ) : StringSearcher( std::string_view() ) {}

 /// P prepared: its probes and the shifts of its q-grams, O( m )
 explicit StringSearcher( std::string_view P );

 /// the first occurrence of P at or after from in T, npos if none
 /// \post npos, or an occurrence at or after from
 [[nodiscard]] std::size_t find( std::string_view T ,
                                 std::size_t from = 0 ) const;

 /// all the occurrences of P in T, in increasing order
 /// \post the occurrences of naiveMatching( T , P ) (an audit)
 [[nodiscard]] std::vector< std::size_t > findAll( std::string_view T ) const;

 /// the number of occurrences of P in T
 [[nodiscard]] std::size_t count( std::string_view T ) const;

 /// the range of the first occurrence of P in [ first , last ), or
 /// ( last , last ): the searcher of std::search()
 template< std::contiguous_iterator I >
  requires( sizeof( std::iter_value_t< I > ) == 1 )
 [[nodiscard]] std::pair< I , I > operator()( I first , I last ) const {
  const std::string_view T( reinterpret_cast< const char * >(
                             std::to_address( first ) ) ,
                            std::size_t( last - first ) );
  const std::size_t s = find( T );
  if( s == npos ) return( std::pair( last , last ) );
  return( std::pair( first + std::ptrdiff_t( s ) ,
                     first + std::ptrdiff_t( s + f_P.size() ) ) );
  }

 [[nodiscard]] std::string_view pattern( void ) const { return( f_P ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the occurrences of P in T from from on, each given to report(), which
 /// returns true to stop; the place where it stopped, or npos
 template< class F >
 std::size_t scan( std::string_view T , std::size_t from ,
                   F && report ) const;

 /// scan() by the filter of the probes, k of them, with vectors of W bytes
 template< int k , int W , class F >
 std::size_t filter( const unsigned char * T , std::size_t n ,
                     std::size_t from , F && report ) const;

 /// scan() by memchr() on the first probe and the bytes of the others
 template< class F >
 std::size_t filterBytes( const unsigned char * T , std::size_t n ,
                          std::size_t from , F && report ) const;

 /// scan() by the shifts of the q-grams at the end of the window, for the
 /// long patterns
 template< class F >
 std::size_t skip( const unsigned char * T , std::size_t n ,
                   std::size_t from , F && report ) const;

 /// skip() on four segments of T at once, whose chains of loads overlap,
 /// the occurrences of each segment reported after those of the ones
 /// before it
 template< class F >
 std::size_t skipPair( const unsigned char * T , std::size_t n ,
                       std::size_t from , F && report ) const;

 /// the hash of the q-gram at p, in [ 0 , 4096 )
 [[nodiscard]] std::size_t gram( const unsigned char * p ) const;

 /// scan() by the Two-Way algorithm, from from on
 template< class F >
 std::size_t twoWay( const unsigned char * T , std::size_t n ,
                     std::size_t from , F && report ) const;

 std::string f_P;
 std::size_t f_off[ 4 ] = { 0 , 0 , 0 , 0 };  ///< the offsets of the probes
 unsigned char f_byte[ 4 ] = { 0 , 0 , 0 , 0 };   ///< and their bytes
 int f_probes = 0;                            ///< 0 , 2 or 4
 std::size_t f_q = 0;      ///< the length of the q-grams, 0 if no skip
 std::uint64_t f_qmask = 0;              ///< the q bytes of a load of 8
 std::array< std::uint8_t , 4096 > f_shift; ///< the shift of each hash
 std::size_t f_shift0 = 1;               ///< after a window that ends in
                                         ///< the last q-gram of P

 };  // end( class StringSearcher )

/*--------------------------------------------------------------------------*/

/// the first occurrence of P in T at or after from, npos if none
[[nodiscard]] inline std::size_t findFirst( std::string_view T ,
                                            std::string_view P ,
                                            std::size_t from = 0 );

/// all the occurrences of P in T, overlapping ones included
[[nodiscard]] inline std::vector< std::size_t > findAll( std::string_view T ,
                                                    std::string_view P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/StringSearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StringSearch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File StringSearch.h -------------------------*/
/*--------------------------------------------------------------------------*/
