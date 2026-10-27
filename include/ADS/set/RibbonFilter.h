/*--------------------------------------------------------------------------*/
/*--------------------------- File RibbonFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The standard Ribbon filter of Dillinger and Walzer, beyond the book: a
 * static filter of a set \f$ S \f$ of \f$ n \f$ keys, built once, which
 * answers maybe for every key of \f$ S \f$ and for a key out of \f$ S \f$
 * with probability about \f$ 2^{-r} \f$, for results of \f$ r \f$ bits (8 or
 * 16, the type `F`), in \f$ r ( 1 + \varepsilon ) \f$ bits per key,
 * \f$ \varepsilon \f$ a small overhead [see below], against the
 * \f$ 1.44 r \f$ of a filter of Bloom with the same false positives [see
 * `BloomFilter.h`] and the \f$ 1.125 r \f$ of the binary fuse filter at
 * large \f$ n \f$ [see `BinaryFuseFilter.h`].
 *
 * The filter is the solution \f$ Z \f$, a matrix of \f$ m \f$ rows of
 * \f$ r \f$ bits over \f$ \mathbb{F}_2 \f$, of the linear system of the
 * \f$ n \f$ keys: a key with the hash \f$ h \f$ has the start
 * \f$ s( h ) \in \{ 0 , \dots , m - 64 \} \f$, the coefficients
 * \f$ c( h ) \f$, a word of \f$ w = 64 \f$ bits whose bit 0 is 1, and the
 * fingerprint \f$ f( h ) \f$ of \f$ r \f$ bits, and its equation is
 * \f[ \bigoplus_{j = 0}^{63} c_j( h ) \, Z_{s( h ) + j} = f( h ) \;, \f]
 * the XOR of the rows of \f$ Z \f$ selected by the bits of \f$ c \f$ from
 * the row \f$ s( h ) \f$. A key is maybe in \f$ S \f$ if its equation
 * holds; a key out of \f$ S \f$ meets it with probability \f$ 2^{-r} \f$,
 * the left side being a random word independent of \f$ f( h ) \f$, so that
 * the false positives are \f$ \varepsilon_{fp} = 2^{-r} \f$, about
 * \f$ 0.39 \% \f$ for \f$ r = 8 \f$ and \f$ 0.0015 \% \f$ for \f$ r = 16 \f$.
 * The start is \f$ s = \lfloor h ( m - 63 ) / 2^{64} \rfloor \f$ (Lemire),
 * with no remainder.
 *
 * The coefficients are \f$ c( h ) = ( h a \bmod 2^{64} ) \vee 1 \f$,
 * \f$ a \f$ an odd constant, the start depending on the high bits of
 * \f$ h \f$ and the fingerprint \f$ f( h ) \f$ on its bits 0 to
 * \f$ r - 1 \f$ and 32 to \f$ 31 + r \f$.
 *
 * The build solves the system by Gauss elimination on the fly (Rapid
 * Incremental Boolean Banding ON the fly, Dillinger and Walzer): the
 * equations are kept in echelon form, the row \f$ i \f$ holding either no
 * equation or one whose first coefficient is in the column \f$ i \f$; a new
 * equation \f$ ( s , c , f ) \f$ goes to the row \f$ s \f$ if it is free,
 * and otherwise is XORed with the equation of the row \f$ s \f$ and shifted
 * to the first column of the result that is 1, until it finds a free row;
 * an equation that becomes 0 is redundant if its right side is 0 too (a
 * repeated key), and makes the system unsolvable otherwise. The rows are
 * then solved from the last,
 * \f$ Z_i = f_i \oplus \bigoplus_{j \ge 1} c_{i,j} Z_{i+j} \f$, by keeping
 * for each of the \f$ r \f$ columns the word of the 64 rows below, so that a
 * row costs \f$ r \f$ parities of a word; a row with no equation takes a
 * pseudorandom value. The banding prefetches the rows of the key 16 places
 * ahead (Chen et al.), whose start is known from its hash, so that the
 * misses of the cache of several keys overlap. With AVX-512 the back
 * substitution takes 4 rows at a time, 8 columns per register: with
 * \f$ W \f$ the words of the rows from \f$ i \f$, the row \f$ i - d \f$,
 * \f$ d = 1 , \ldots , 4 \f$, is
 * \f[ Z_{i-d} = f_{i-d} \oplus \mathrm{parity}\bigl( ( c_{i-d} \gg d )
 *     \wedge W \bigr) \oplus \bigoplus_{k=1}^{d-1} c_{i-d,k} \,
 *     Z_{i-d+k} \;, \f]
 * the 4 parities taken from the same \f$ W \f$ at once and the bits of the
 * new rows among them added after, by masks, so that the chain of
 * dependences is one parity per 4 rows instead of one per row; the solution
 * is the same as row by row. The solution is stored interleaved: for each
 * block of 64 rows, \f$ r \f$ words, the \f$ j \f$-th of which holds the
 * column \f$ j \f$ of the block, so that a query reads the \f$ 2 r \f$
 * words of the blocks of its 64 rows, which are consecutive, and makes each
 * bit of its result by the parity of
 * \f$ ( c \ll o ) \wedge u \oplus ( c \gg ( 64 - o ) ) \wedge v \f$,
 * \f$ o \f$ the offset of \f$ s \f$ in its block and \f$ u \f$, \f$ v \f$
 * the words of the column in the two blocks. The blocks are aligned to the
 * lines of cache of 64 bytes, each line holding 8 columns of a block, and a
 * query takes the 8 parities of a line at once: with AVX-512 the 8 words of
 * each block in one register, the parity of each byte by a table of the 16
 * nibbles (`vpshufb`) after folding its two halves, and the 8 bytes of a
 * word summed by `vpsadbw`, whose bit 0 is the parity (the population count
 * of Muła, Kurz and Lemire reduced modulo 2); with AVX2 the same on 4 words
 * at a time; on any machine by one popcount per word. A query compares the 8
 * bits of a line with those of \f$ f( h ) \f$ before reading the next line,
 * so that a key out of \f$ S \f$ reads one line of each block with
 * probability \f$ 1 - 2^{-8} \f$: 2 lines for \f$ r = 8 \f$ and
 * \f$ r = 16 \f$ alike, against the 4 lines of a key of \f$ S \f$ for
 * \f$ r = 16 \f$. The answers are the same on every machine.
 *
 * Space and success. For \f$ m \f$ rows the filter takes \f$ r m \f$ bits
 * (and \f$ r \f$ words of padding). Dietzfelbinger and Walzer show that a
 * system of \f$ n = ( 1 - \varepsilon ) m \f$ equations with random blocks
 * of length \f$ L = O( \log n / \varepsilon ) \f$ has full rank with high
 * probability, its elimination taking \f$ O( m / \varepsilon^2 ) \f$
 * expected time; with \f$ L = w = 64 \f$ fixed, the overhead needed grows as
 * \f$ \log n \f$, and the build takes
 * \f[ m = n ( 1 + \varepsilon ) \;, \qquad \varepsilon =
 *     \max\bigl( 0.01 \,,\, 0.0083 \log_2 n - 0.025 \bigr) \;, \f]
 * rounded up to a multiple of 64 and at least 64: about \f$ 9 \% \f$ at
 * \f$ 10^5 \f$ keys and \f$ 14 \% \f$ at \f$ 10^6 \f$, i.e., 8.9 and 9.1
 * bits per key for \f$ r = 8 \f$. If the system has no solution the build
 * draws a new seed, and after every 4 failures adds \f$ 2 \% \f$ of
 * \f$ n \f$ rows.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise;
 * for each seed it is mixed once more by a folded product with the seed.
 * The keys may repeat: a repeated key gives a redundant equation, and
 * counts in `size()` once per occurrence.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{32} \f$ keys; after
 * the build every key is maybe there, an audit. A build that draws 1000
 * seeds with no success throws `std::runtime_error`. The operations are
 * defined in `RibbonFilter.tpp`.
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
 * \see W. Muła, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046 (the counts of the bytes by vpshufb and
 *      vpsadbw, here their parities)
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

/// a static filter of the keys of type `T` with results of type `F`, one
/// equation of 64 coefficients per key
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class RibbonFilter {

 static_assert( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) ,
                "the results have at most 32 bits" );

/*--------------------------------------------------------------------------*/
 public:

 /// the filter of the keys, which may repeat
 ///
 /// In pseudocode (Dillinger and Walzer), \f$ C[ i ] \f$ and \f$ R[ i ] \f$
 /// the coefficients and the right side of the equation of the row
 /// \f$ i \f$, \f$ C[ i ] = 0 \f$ if none, \f$ \mathrm{ctz} \f$ the
 /// number of the trailing zeros:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( K ) \cr
 /// 2 & \quad \textbf{repeat}\ \text{draw a seed};\ C \gets 0 ;\ R \gets 0
 ///     \cr
 /// 3 & \qquad \textbf{foreach}\ k \in K\ \textbf{do} \cr
 /// 4 & \qquad\quad ( s , c , f ) \gets ( s( h ) , c( h ) , f( h ) )
 ///     \text{ of the hash } h \text{ of } k \cr
 /// 5 & \qquad\quad \textbf{while}\ C[ s ] \ne 0\ \textbf{do} \cr
 /// 6 & \qquad\qquad c \gets c \oplus C[ s ];\ f \gets f \oplus R[ s ] \cr
 /// 7 & \qquad\qquad \textbf{if}\ c = 0\ \textbf{then break}\quad
 ///     \triangleright\ \text{redundant if } f = 0 \text{, unsolvable}
 ///     \text{ otherwise} \cr
 /// 8 & \qquad\qquad z \gets \mathrm{ctz}( c );\ s \gets s + z;\ c \gets
 ///     c / 2^z \cr
 /// 9 & \qquad\quad \textbf{if}\ c \ne 0\ \textbf{then}\ C[ s ] \gets c;
 ///     \ R[ s ] \gets f \cr
 /// 10 & \quad \textbf{until}\ \text{no equation was unsolvable} \cr
 /// 11 & \quad \textbf{for}\ i \gets m - 1\ \textbf{downto}\ 0
 ///     \ \textbf{do} \cr
 /// 12 & \qquad Z_i \gets R[ i ] \oplus \bigoplus_{j \ge 1} C[ i ]_j Z_{i+j}
 ///     \text{, or a pseudorandom value if } C[ i ] = 0
 /// \end{array}}
 /// \f]
 /// \pre fewer than \f$ 2^{32} \f$ keys
 /// \post every key is maybe there (an audit)
 explicit RibbonFilter( const std::vector< T > & keys ,
                        std::uint64_t seed = 1 , H h = H() );

 /// whether \f$ x \f$ may be in the set: true for every key of the set,
 /// and for any other with probability about \f$ 2^{-r} \f$, \f$ r \f$
 /// the bits of `F`
 ///
 /// In pseudocode (Dillinger and Walzer), the 64 rows from \f$ s \f$ read
 /// as the words of two blocks, a line of cache of 8 columns at a time:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybe}( x ) \cr
 /// 2 & \quad ( s , c , f ) \gets ( s( h ) , c( h ) , f( h ) ) \text{ of }
 ///     \text{the hash } h \text{ of } x \cr
 /// 3 & \quad \textbf{foreach}\ \text{line of 8 columns } j\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \bigoplus_{t = 0}^{63} c_t Z_{s+t , j} \ne f_j
 ///     \ \text{for one of its } j\ \textbf{then return false} \cr
 /// 5 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the keys of the build, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the filter has no key
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
 static constexpr std::size_t kMaxTrials = 1000;   ///< seeds at most
 static constexpr std::size_t kAhead = 16;  ///< keys prefetched ahead

 /// a line of cache: the words of 8 columns of a block of 64 rows
 struct alignas( 64 ) Line {
  std::uint64_t w[ 8 ];   ///< the words of the 8 columns
  };

 /// the hash of \f$ x \f$ before the seed, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t base( const T & x ) const;

 /// the hash of the base hash \f$ b \f$ under the seed
 [[nodiscard]] std::uint64_t mixed( std::uint64_t b ) const;

 /// the start of the hash \f$ h \f$
 [[nodiscard]] std::size_t start( std::uint64_t h ) const;

 /// the coefficients of the hash \f$ h \f$, bit 0 set
 [[nodiscard]] static std::uint64_t coefficients( std::uint64_t h );

 /// the fingerprint of the hash \f$ h \f$
 [[nodiscard]] static F fingerprint( std::uint64_t h );

 /// the parities of the 8 words
 /// \f$ ( u_j \wedge \mathit{lo} ) \oplus ( u_{r+j} \wedge \mathit{hi} ) \f$,
 /// the bit \f$ j \f$ of the result for the word \f$ j \f$, `u` aligned
 /// to 64 bytes
 [[nodiscard]] static std::uint32_t parities( const std::uint64_t * u ,
                                              std::uint64_t lo ,
                                              std::uint64_t hi );

 /// the solution of the equations of the keys, false if there is none
 bool build( const std::vector< T > & keys );

 /// the solution \f$ Z \f$ of the banded rows \f$ C \f$ and their right
 /// sides \f$ R \f$
 void solve( const std::vector< std::uint64_t > & C ,
             const std::vector< F > & R );

 std::vector< Line > f_Z;            ///< kR / 8 lines per block of 64 rows
 std::size_t f_m = 0;                ///< the rows
 std::uint64_t f_seed;               ///< the seed of the last trial
 std::size_t f_n = 0;                ///< the keys, with their repetitions
 std::size_t f_trials = 0;           ///< the seeds drawn
 H f_h;                              ///< the hash of the keys

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
