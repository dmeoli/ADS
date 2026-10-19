/*--------------------------------------------------------------------------*/
/*-------------------------- File StringSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact search of a pattern P of m bytes in a text T of n bytes,
 * beyond the book: the engineered search of ADS, the one that the
 * benchmark of the searches compares with memmem() of the C library,
 * std::search() with its three searchers and the searchers of
 * Boost.Algorithm, while kmp() [see StringMatching.h] stays the algorithm
 * of the textbook.
 *
 * StringSearcher prepares P once, as the searchers of the standard
 * library do, and then finds its occurrences in any text:
 *
 * - P of one byte is found by memchr();
 * - otherwise k probes, two places of P with their bytes (four if P has
 *   fewer than 8 distinct bytes, as on DNA), filter W places of T at once:
 *   the bytes of T at the offsets of the probes from W consecutive starts
 *   are compared with the bytes of the probes by W-byte vector
 *   comparisons, AVX-512 (W = 64), AVX2 (32) or SSE2 (16) as the target
 *   allows, and only the starts that pass every probe are compared with P
 *   by memcmp(); elsewhere memchr() finds the first probe and the others
 *   are checked one by one. The probes are the rarest bytes of P under a
 *   fixed ranking of the bytes by their frequency in text (the letters of
 *   English by frequency, the blank and the newline the most common, the
 *   control and the high bytes the rarest), and on a tie the later
 *   places, so that a probe of a frequent byte (e.g., the blank) passes
 *   seldom;
 * - P of kSkip bytes or more is searched first by the shifts of its
 *   q-grams (of Wu and Manber, and of the Hash3 of Lecroq): q is the least
 *   length with \f$ d^q \ge 16 m \f$ for the d distinct bytes of P, at
 *   most 8, the q-grams are hashed into 4096 shifts, the distance from the
 *   last place of each q-gram in P to the end of P, and the q-gram at the
 *   end of the window of T moves it by its shift, up to m - q + 1 places,
 *   reading one load of 8 bytes per window; when 64 windows have moved by
 *   less than m / 4 each on average (a text of few distinct q-grams), the
 *   filter of the probes takes over;
 * - every search keeps the comparisons that fail: when their cost, m per
 *   failure, exceeds 16 times the length of the text scanned, the search
 *   goes on with the Two-Way algorithm of Crochemore and Perrin, so that
 *   no text costs more than O( n + m ) comparisons in all (e.g., a pattern
 *   that agrees with a periodic text on its probes and nowhere else). The
 *   Two-Way algorithm cuts P at a critical factorization u v, found by two
 *   computations of maximal suffixes, compares v from left to right and u
 *   from right to left, and shifts by the period of P (with a memory of
 *   the prefix that is known to match) or by max( |u| , |v| ) + 1, in
 *   constant extra space, its factorization made only when the search
 *   comes to it; it is the algorithm of memmem() in the GNU C library,
 *   which adds a table of shifts for the long patterns.
 *
 * find() gives the first occurrence at or after from (npos if none),
 * findAll() all of them, overlapping ones included, in increasing order,
 * count() their number, and operator() the range of the first occurrence
 * in [ first , last ), the interface of the searchers of std::search().
 * The empty pattern occurs at every place 0 , ... , n. The functions
 * findFirst() and findAll() of the namespace prepare P and search once.
 *
 * The contracts [see Contract.h]: an occurrence found is one, and
 * findAll() gives those of naiveMatching() [see StringMatching.h] (an
 * audit, O( n m ) time). The functions are defined in StringSearch.tpp.
 *
 * \see M. Crochemore, D. Perrin, Two-way string-matching, Journal of the
 *      ACM 38(3), 651-675, 1991, doi:10.1145/116825.116845
 * \see S. Faro, T. Lecroq, The exact online string matching problem: a
 *      review of the most recent results, ACM Computing Surveys 45(2), 13,
 *      2013, doi:10.1145/2431211.2431212
 * \see S. Faro, M. O. Kulekci, Fast packed string matching for short
 *      patterns, Proceedings of the 15th Meeting on Algorithm Engineering
 *      and Experiments (ALENEX), 113-121, 2013,
 *      doi:10.1137/1.9781611972931.10
 * \see W. Mula, SIMD-friendly algorithms for substring searching, 2016,
 *      http://0x80.pl/notesen/2016-11-28-simd-strfind.html
 * \see S. Wu, U. Manber, A fast algorithm for multi-pattern searching,
 *      Technical Report TR-94-17, University of Arizona, 1994
 * \see T. Lecroq, Fast exact string matching algorithms, Information
 *      Processing Letters 102(6), 229-235, 2007,
 *      doi:10.1016/j.ipl.2007.01.002
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
 static constexpr std::size_t kSkip = 128;

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
 std::vector< std::uint16_t > f_shift;   ///< the shift of each hash
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
