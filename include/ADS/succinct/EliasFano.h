/*--------------------------------------------------------------------------*/
/*---------------------------- File EliasFano.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Elias-Fano representation of a non-decreasing sequence
 * \f$ x_0 \le \cdots \le x_{n-1} \f$ of integers in \f$ [ 0 , u ) \f$
 * (chapters 11, Integer Coding, and 15, Compressed Data Structures, of
 * Ferragina's book): each item is split into its
 * \f$ \ell = \lceil \log_2( u / n ) \rceil \f$ low bits, written one after
 * the other in \f$ L \f$, and its high bits, the bucket
 * \f$ x_i \gg \ell \f$, written in \f$ H \f$ as the unary counts of the
 * buckets (\f$ 1^c 0 \f$ for a bucket of \f$ c \f$ items), so that the
 * \f$ i \f$-th 1 of \f$ H \f$ is at the place \f$ ( x_i \gg \ell ) + i \f$.
 * \f$ H \f$ has \f$ n \f$ 1s and at most \f$ n \f$ 0s, since
 * \f$ 2^\ell \ge u / n \f$, and the sequence takes at most
 * \f$ 2 n + n \lceil \log_2( u / n ) \rceil \f$ bits whatever its
 * distribution, within 2 bits per item of the
 * \f$ \log_2 \binom{u}{n} \f$ bits needed to tell \f$ n \f$ items of
 * \f$ [ 0 , u ) \f$ apart.
 *
 * With rank and select on \f$ H \f$ [see `RankSelect.h`] the sequence is
 * not only decoded but searched in place:
 *
 * - `access( i )`, \f$ x_i \f$ in \f$ O( 1 ) \f$ time, as
 *   \f$ \mathrm{select}_1( i + 1 ) - i \f$ followed by the \f$ i \f$-th
 *   field of \f$ L \f$;
 * - `nextGEQ( x )`, the place of the first item \f$ \ge x \f$, which is the
 *   rank of \f$ x \f$ (the items \f$ < x \f$) and gives the predecessor and
 *   the successor: \f$ \mathrm{select}_0 \f$ of the bucket of \f$ x \f$
 *   gives the first item of that bucket, and the scan stops at its end, in
 *   \f$ O( 1 + c ) \f$ time for a bucket of \f$ c \f$ items, expected
 *   \f$ O( 1 ) \f$ since \f$ 2^\ell \approx u / n \f$; past 8 items (the
 *   clusters of a sequence fill some buckets) the end of the bucket is found
 *   by \f$ \mathrm{select}_0 \f$ too and its low bits, sorted, by a binary
 *   search, \f$ O( \log c ) \f$ time;
 * - viewed as the places of the 1s of a vector of bits \f$ B \f$ of
 *   \f$ u \f$ bits with \f$ n \f$ 1s (as in chapter 15, Compressed Data
 *   Structures, of Ferragina's book), `access( i )` is
 *   \f$ \mathrm{select}_1( i + 1 ) \f$ of \f$ B \f$ and
 *   `nextGEQ( i )` is \f$ \mathrm{rank}_1( i ) \f$ of \f$ B \f$, in
 *   \f$ n ( 2 + \log( u / n ) ) + o( n ) \f$ bits rather than
 *   \f$ u + o( u ) \f$.
 *
 * Ferragina's book computes the rank of \f$ B \f$ by a binary search on
 * the select, in \f$ O( \log n ) \f$ time; the bucket found by
 * \f$ \mathrm{select}_0 \f$ makes it \f$ O( 1 ) \f$ expected, the design of
 * the quasi-succinct indices of Vigna and of the sdarray of
 * Okanohara and Sadakane. \f$ H \f$ is written with the 1s for the items
 * and the 0s for the buckets, as in its chapter 11 (its chapter 15 swaps
 * them, which only swaps \f$ \mathrm{select}_0 \f$ and
 * \f$ \mathrm{select}_1 \f$). The representation ignores the regularities
 * of the sequence: the partitioned Elias-Fano of Ottaviano and Venturini
 * [see `PartitionedEliasFano.h`] splits it into chunks, each one coded at
 * its own best, and comes within a few percent of the interpolative code
 * [see `IntegerCodes.h`] keeping the random access.
 *
 * The contracts [see `Contract.h`]: the sequence is non-decreasing (an
 * audit) and below \f$ u \f$, and `access()` needs \f$ i < n \f$; the audit
 * of the construction is that `access()` gives back each item, and the
 * postcondition of `nextGEQ()` is that the items before the place are
 * \f$ < x \f$ and the one at it is not. The functions are defined in
 * `EliasFano.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 11, Integer Coding, and 15, Compressed Data
 *      Structures, doi:10.1017/9781009128933
 * \see P. Elias, Efficient storage and retrieval by content and address of
 *      static files, Journal of the ACM 21(2), 246-260, 1974,
 *      doi:10.1145/321812.321820
 * \see R. M. Fano, On the number of bits required to implement an
 *      associative memory, Memorandum 61, Computation Structures Group,
 *      MIT, 1971
 * \see D. Okanohara, K. Sadakane, Practical entropy-compressed rank/select
 *      dictionary, Proc. 9th ALENEX, 60-70, 2007,
 *      doi:10.1137/1.9781611972870.6
 * \see S. Vigna, Quasi-succinct indices, Proc. 6th WSDM, 83-92, 2013,
 *      doi:10.1145/2433396.2433409
 * \see G. Ottaviano, R. Venturini, Partitioned Elias-Fano indexes, Proc.
 *      37th SIGIR, 273-282, 2014, doi:10.1145/2600428.2609615
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

#ifndef __ADS_EliasFano
 #define __ADS_EliasFano
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BasicEliasFano --------------------------*/
/*--------------------------------------------------------------------------*/

/// a non-decreasing sequence of integers in the Elias-Fano representation,
/// the inventories of select of \f$ H \f$ built in the constructor if not
/// `Lazy` and at the first select of each value if `Lazy` [see
/// `RankSelect.h`]
template< bool Lazy >
class BasicEliasFano {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty sequence
 BasicEliasFano( void ) = default;

 /// the sequence \f$ x \f$ in the universe \f$ [ 0 , x_{n-1} + 1 ) \f$
 /// \pre \f$ x \f$ is non-decreasing (an audit) and
 ///      \f$ x_{n-1} < 2^{64} - 1 \f$, so that the universe fits in 64 bits
 explicit BasicEliasFano( const std::vector< std::uint64_t > & x );

 /// the sequence \f$ x \f$ in the universe \f$ [ 0 , u ) \f$
 ///
 /// In pseudocode (chapter 11, Integer Coding, of Ferragina's book, beyond
 /// the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( x , u ) \cr
 /// 2 & \quad \ell \gets \lceil \log_2( u / n ) \rceil\ \text{(0 if }
 ///     u \le n \text{)} \cr
 /// 3 & \quad H \gets n + \lfloor ( u - 1 ) / 2^\ell \rfloor + 1 \text{ bits }
 ///     \text{equal to 0};\ L \gets \text{an empty vector of bits} \cr
 /// 4 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 5 & \qquad \mathrm{append}( L , x_i \bmod 2^\ell , \ell ) \cr
 /// 6 & \qquad H[ \lfloor x_i / 2^\ell \rfloor + i ] \gets 1 \cr
 /// 7 & \quad \text{build the directory of rank and select of } H
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is non-decreasing (an audit) and its items are below
 ///      \f$ u \f$
 BasicEliasFano( const std::vector< std::uint64_t > & x , std::uint64_t u );

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const;

 /// the bound \f$ u \f$ of the items
 [[nodiscard]] std::uint64_t universe( void ) const;

 /// the item \f$ i \f$
 ///
 /// In pseudocode (chapter 11, Integer Coding, of Ferragina's book, beyond
 /// the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad h \gets \mathrm{select}_1( H , i + 1 ) - i\quad
 ///     \triangleright\ \text{the bucket of } x_i \cr
 /// 3 & \quad \textbf{return}\ h \cdot 2^\ell + L[ i \ell .. ( i + 1 ) \ell
 ///     - 1 ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 /// the item \f$ i \f$, as `access()`
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const;

 /// the place of the first item \f$ \ge x \f$, `size()` if there is none,
 /// i.e., the number of items \f$ < x \f$
 ///
 /// In pseudocode (Vigna and Okanohara and Sadakane, beyond the book),
 /// \f$ \mathit{low}( i ) \f$ the low bits of the item \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{nextGEQ}( x ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{or}\ x \ge u\ \textbf{then return}
 ///     \ n \cr
 /// 3 & \quad h \gets \lfloor x / 2^\ell \rfloor;\ \mathit{pos} \gets 0
 ///     \ \text{if } h = 0 \text{, else } \mathrm{select}_0( H , h ) + 1 \cr
 /// 4 & \quad p \gets \mathit{pos} - h\quad \triangleright\ \text{the first }
 ///     \text{item of the bucket } h \cr
 /// 5 & \quad \textbf{while}\ H[ \mathit{pos} ] = 1\ \textbf{and}
 ///     \ \mathit{low}( p ) < x \bmod 2^\ell\ \textbf{do} \cr
 /// 6 & \qquad p \gets p + 1;\ \mathit{pos} \gets \mathit{pos} + 1\quad
 ///     \triangleright\ \text{past 8 items, a binary search} \cr
 /// 7 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \post the items before it are \f$ < x \f$, the one at it is
 ///       \f$ \ge x \f$
 [[nodiscard]] std::size_t nextGEQ( std::uint64_t x ) const;

 /// whether \f$ x \f$ is an item, by `nextGEQ()`
 [[nodiscard]] bool contains( std::uint64_t x ) const;

 /// the number of low bits of each item, \f$ \ell \f$
 [[nodiscard]] int lowWidth( void ) const;

 /// the low bits, \f$ L \f$
 [[nodiscard]] const BitVector & low( void ) const;

 /// the high bits, \f$ H \f$, with their directory
 [[nodiscard]] const BasicRankSelect< Lazy > & high( void ) const;

 /// the bits of \f$ L \f$ and \f$ H \f$, without the directory of \f$ H \f$
 [[nodiscard]] std::size_t codeBits( void ) const;

 /// all the bits, with the directory of H
 [[nodiscard]] std::size_t sizeInBits( void ) const;

 /// the items, decoded in \f$ O( n ) \f$ time
 [[nodiscard]] std::vector< std::uint64_t > decode( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the type of an item
 using value_type = std::uint64_t;
 /// the type of a size
 using size_type = std::size_t;
 /// the type of the distance of two places
 using difference_type = std::ptrdiff_t;
 /// an item, returned by value
 using reference = std::uint64_t;
 /// an item, returned by value
 using const_reference = std::uint64_t;
 /// the constant iterator, random access
 using const_iterator = SubscriptIterator< BasicEliasFano , std::uint64_t >;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the bucket of x, x >> ell
 [[nodiscard]] std::uint64_t bucket( std::uint64_t x ) const;

 std::size_t f_n = 0;
 std::uint64_t f_u = 0;
 int f_l = 0;                ///< the low bits of an item
 BitVector f_low;            ///< L
 BasicRankSelect< Lazy > f_high;   ///< H

 };  // end( class BasicEliasFano )

/// Elias-Fano with the inventories of \f$ H \f$ built in the constructor,
/// the fastest `access()` and `nextGEQ()`
using EliasFano = BasicEliasFano< false >;

/// Elias-Fano with the inventories of \f$ H \f$ built at the first query
/// that needs them, the fastest construction
using LazyEliasFano = BasicEliasFano< true >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/EliasFano.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EliasFano.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File EliasFano.h --------------------------*/
/*--------------------------------------------------------------------------*/
