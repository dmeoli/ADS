/*--------------------------------------------------------------------------*/
/*--------------------------- File RibbonFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The standard Ribbon filter of Dillinger and Walzer, beyond the book: a
 * static filter of a set S of n keys, built once, which answers maybe for
 * every key of S and for a key out of S with probability about
 * \f$ 2^{-r} \f$, for results of r bits (8 or 16, the type F), in
 * \f$ r ( 1 + \varepsilon ) \f$ bits per key, \f$ \varepsilon \f$ a small
 * overhead [see below], against the \f$ 1.44 r \f$ of a filter of Bloom
 * with the same false positives [see BloomFilter.h] and the
 * \f$ 1.125 r \f$ of the binary fuse filter at large n [see
 * BinaryFuseFilter.h].
 *
 * The filter is the solution Z, a matrix of m rows of r bits over
 * \f$ \mathbb{F}_2 \f$, of the linear system of the n keys: a key k with
 * the hash h has the start \f$ s( h ) \in \{ 0 , \dots , m - 64 \} \f$,
 * the coefficients \f$ c( h ) \f$, a word of w = 64 bits whose bit 0 is 1,
 * and the fingerprint \f$ f( h ) \f$ of r bits, and its equation is
 * \f[ \bigoplus_{j = 0}^{63} c_j( h ) \, Z_{s( h ) + j} = f( h ) \;, \f]
 * the XOR of the rows of Z selected by the bits of c from the row
 * \f$ s( h ) \f$. A key is maybe in S if its equation holds; a key out of
 * S meets it with probability \f$ 2^{-r} \f$, the left side being a
 * random word independent of \f$ f( h ) \f$, so that the false positives
 * are \f$ \varepsilon_{fp} = 2^{-r} \f$, about 0.39 % for r = 8 and
 * 0.0015 % for r = 16. The start is
 * \f$ s = \lfloor h ( m - 63 ) / 2^{64} \rfloor \f$ (Lemire), with no
 * remainder.
 *
 * The coefficients are \f$ c( h ) = ( h a \bmod 2^{64} ) \vee 1 \f$, a an
 * odd constant, the start depending on the high bits of h and the
 * fingerprint \f$ f( h ) \f$ on its bits 0 to r - 1 and 32 to 31 + r.
 *
 * The build solves the system by Gauss elimination on the fly (Rapid
 * Incremental Boolean Banding ON the fly, Dillinger and Walzer): the
 * equations are kept in echelon form, row i holding either no equation or
 * one whose first coefficient is in column i; a new equation
 * \f$ ( s , c , f ) \f$ goes to row s if it is free, and otherwise is
 * XORed with the equation of row s and shifted to the first column of the
 * result that is 1, until it finds a free row; an equation that becomes 0
 * is redundant if its right side is 0 too (a repeated key), and makes the
 * system unsolvable otherwise. The rows are then solved from the last,
 * \f$ Z_i = f_i \oplus \bigoplus_{j \ge 1} c_{i,j} Z_{i+j} \f$, by keeping
 * for each of the r columns the word of the 64 rows below, so that a row
 * costs r parities of a word; a row with no equation takes a pseudorandom
 * value. The banding prefetches the rows of the key 16 places ahead
 * (Chen et al.), whose start is known from its hash, so that the misses
 * of the cache of several keys overlap. With AVX-512 the back
 * substitution takes 4 rows at a time, 8 columns per register: with W
 * the words of the rows from i, the row i - d, d = 1, ..., 4, is
 * \f[ Z_{i-d} = f_{i-d} \oplus \mathrm{parity}\bigl( ( c_{i-d} \gg d )
 *     \wedge W \bigr) \oplus \bigoplus_{k=1}^{d-1} c_{i-d,k} \,
 *     Z_{i-d+k} \;, \f]
 * the 4 parities taken from the same W at once and the bits of the new
 * rows among them added after, by masks, so that the chain of
 * dependences is one parity per 4 rows instead of one per row; the
 * solution is the same as row by row. The solution is stored
 * interleaved: for each block of 64 rows, r words, the j-th of which
 * holds the column j of the block, so that a query reads the 2 r words
 * of the blocks of its 64 rows, which are consecutive, and makes each
 * bit of its result by the parity of
 * \f$ ( c \ll o ) \wedge u \oplus ( c \gg ( 64 - o ) ) \wedge v \f$, o the
 * offset of s in its block and u, v the words of the column in the two
 * blocks. The blocks are aligned to the lines of cache of 64 bytes, each
 * line holding 8 columns of a block, and a query takes the 8 parities of
 * a line at once: with AVX-512 the 8 words of each block in one register,
 * the parity of each byte by a table of the 16 nibbles (vpshufb) after
 * folding its two halves, and the 8 bytes of a word summed by vpsadbw,
 * whose bit 0 is the parity (the population count of Mula, Kurz and
 * Lemire reduced modulo 2); with AVX2 the same on 4 words at a time; on
 * any machine by one popcount per word. A query compares the 8 bits of a
 * line with those of \f$ f( h ) \f$ before reading the next line, so that
 * a key out of S reads one line of each block with probability
 * \f$ 1 - 2^{-8} \f$: 2 lines for r = 8 and r = 16 alike, against the
 * 4 lines of a key of S for r = 16. The answers are the same on every
 * machine.
 *
 * Space and success. For m rows the filter takes \f$ r m \f$ bits (and r
 * words of padding). Dietzfelbinger and Walzer show that a system of
 * \f$ n = ( 1 - \varepsilon ) m \f$ equations with random blocks of
 * length \f$ L = O( \log n / \varepsilon ) \f$ has full rank with high
 * probability, its elimination taking \f$ O( m / \varepsilon^2 ) \f$
 * expected time; with L = w = 64 fixed, the overhead needed grows as
 * \f$ \log n \f$, and the build takes
 * \f[ m = n ( 1 + \varepsilon ) \;, \qquad \varepsilon =
 *     \max\bigl( 0.01 \,,\, 0.0083 \log_2 n - 0.025 \bigr) \;, \f]
 * rounded up to a multiple of 64 and at least 64: about 9 % at
 * \f$ 10^5 \f$ keys and 14 % at \f$ 10^6 \f$, i.e., 8.9 and 9.1 bits per
 * key for r = 8. If the system has no solution the build draws a new seed,
 * and after every 4 failures adds 2 % of n rows.
 *
 * The hash of a key is h( k ), taken as it is if H declares
 * is_avalanching [see FastHash.h] and mixed by hashWord() otherwise; for
 * each seed it is mixed once more by a folded product with the seed. The
 * keys may repeat: a repeated key gives a redundant equation, and counts
 * in size() once per occurrence.
 *
 * The contracts [see Contract.h]: fewer than \f$ 2^{32} \f$ keys; after
 * the build every key is maybe there, an audit. A build that draws 1000
 * seeds with no success throws std::runtime_error. The operations are
 * defined in RibbonFilter.tpp.
 *
 * \see P. C. Dillinger, S. Walzer, Ribbon filter: practically smaller than
 *      Bloom and Xor, arXiv:2103.02515, 2021,
 *      https://arxiv.org/abs/2103.02515
 * \see D. Lemire, Fast random integer generation in an interval, ACM
 *      Transactions on Modeling and Computer Simulation 29(1), 3:1-3:12,
 *      2019, doi:10.1145/3230636 (the start by a product)
 * \see M. Dietzfelbinger, S. Walzer, Efficient Gauss elimination for
 *      near-quadratic matrices with one short random block per row, with
 *      applications, Proceedings of ESA 2019, LIPIcs 144, 39:1-39:18,
 *      2019, doi:10.4230/LIPIcs.ESA.2019.39
 * \see S. Chen, A. Ailamaki, P. B. Gibbons, T. C. Mowry, Improving hash
 *      join performance through prefetching, Proceedings of ICDE 2004,
 *      116-127, 2004, doi:10.1109/ICDE.2004.1319989 (the prefetch of the
 *      rows of the keys ahead, in the banding)
 * \see W. Mula, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046 (the counts of the bytes by vpshufb and
 *      vpsadbw, here their parities)
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

#ifndef __ADS_RibbonFilter
 #define __ADS_RibbonFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#if defined( __AVX2__ ) || defined( __AVX512BW__ )
 #include <immintrin.h>
#endif

/// a function inlined even where the compiler would not
#ifndef ADS_FILTER_INLINE
 #if defined( __GNUC__ ) || defined( __clang__ )
  #define ADS_FILTER_INLINE inline __attribute__(( always_inline ))
 #elif defined( _MSC_VER )
  #define ADS_FILTER_INLINE __forceinline
 #else
  #define ADS_FILTER_INLINE inline
 #endif
#endif

/// a hint to bring the line of p into the cache, or nothing
#ifndef ADS_FILTER_PREFETCH
 #if defined( __GNUC__ ) || defined( __clang__ )
  #define ADS_FILTER_PREFETCH( p ) \
   __builtin_prefetch( ( const void * )( p ) )
 #else
  #define ADS_FILTER_PREFETCH( p ) ( ( void ) 0 )
 #endif
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RibbonFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a static filter of the keys of type T with results of type F, one
/// equation of 64 coefficients per key
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class RibbonFilter {

 static_assert( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) ,
                "the results have at most 32 bits" );

/*--------------------------------------------------------------------------*/
 public:

 /// the filter of the keys, which may repeat
 /// \pre fewer than 2^32 keys
 /// \post every key is maybe there (an audit)
 explicit RibbonFilter( const std::vector< T > & keys ,
                        std::uint64_t seed = 1 , H h = H() );

 /// true for every key of the set, and for any other with probability
 /// about 2^-r, r the bits of F
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the keys of the build, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bits of the solution, padding included
 [[nodiscard]] std::size_t bits( void ) const;

 /// the rows m of the solution
 [[nodiscard]] std::size_t rows( void ) const;

 /// the seeds drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kR = 8 * sizeof( F );       ///< the bits of a result
 static constexpr std::size_t kMaxTrials = 1000;
 static constexpr std::size_t kAhead = 16;  ///< keys prefetched ahead

 /// a line of cache: the words of 8 columns of a block of 64 rows
 struct alignas( 64 ) Line {
  std::uint64_t w[ 8 ];
  };

 /// the hash of x before the seed, mixed unless H avalanches
 [[nodiscard]] std::uint64_t base( const T & x ) const;

 /// the hash of the base hash b under the seed
 [[nodiscard]] std::uint64_t mixed( std::uint64_t b ) const;

 /// the start of the hash h
 [[nodiscard]] std::size_t start( std::uint64_t h ) const;

 /// the coefficients of the hash h, bit 0 set
 [[nodiscard]] static std::uint64_t coefficients( std::uint64_t h );

 /// the fingerprint of the hash h
 [[nodiscard]] static F fingerprint( std::uint64_t h );

 /// the parities of the 8 words ( u_j & lo ) ^ ( u_{r+j} & hi ), bit j
 /// of the result for the word j, u aligned to 64 bytes
 [[nodiscard]] static std::uint32_t parities( const std::uint64_t * u ,
                                              std::uint64_t lo ,
                                              std::uint64_t hi );

 /// the solution of the equations of the keys, false if there is none
 bool build( const std::vector< T > & keys );

 /// the solution Z of the banded rows C and their right sides R
 void solve( const std::vector< std::uint64_t > & C ,
             const std::vector< F > & R );

 std::vector< Line > f_Z;            ///< kR / 8 lines per block of 64 rows
 std::size_t f_m = 0;                ///< the rows
 std::uint64_t f_seed;
 std::size_t f_n = 0;
 std::size_t f_trials = 0;
 H f_h;

 };  // end( class RibbonFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/RibbonFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RibbonFilter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File RibbonFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
