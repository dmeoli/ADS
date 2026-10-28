/*--------------------------------------------------------------------------*/
/*---------------------------- File LAVector.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The LA-vector of Boffa, Ferragina and Vinciguerra, beyond the book: a
 * non-decreasing sequence \f$ x_0 \le \cdots \le x_{n-1} \f$ of integers of
 * 64 bits, i.e., the places of the 1s of a vector of bits, compressed by the
 * regularities that a model of lines finds in it, with `access()`, the
 * select of the vector of bits, and `nextGEQ()`, its rank (see \ref
 * theory_succinct_learned).
 *
 * The points \f$ ( i , x_i ) \f$ are covered by the fewest segments whose
 * lines predict each item within \f$ \epsilon \f$, the optimal piecewise
 * linear approximation of the PGM-index [see `PGMIndex.h`], and each item
 * is stored as its <em>correction</em>, its distance from the prediction of
 * its segment, in \f$ c \f$ bits for \f$ \epsilon = 2^{c-1} - 1 \f$: the
 * segment \f$ j \f$, from the place \f$ p_j \f$, predicts
 * \f$ f_j( i ) = \beta_j + \lfloor \alpha_j ( i - p_j ) \rfloor \f$, and
 * \f$ x_i = f_j( i ) + C[ i ] \f$ with \f$ 0 \le C[ i ] < 2^{w_j} \f$, the
 * base \f$ \beta_j \f$ taking the least correction of the segment, so that
 * \f$ w_j \f$ is the width of the range of its corrections (\f$ c \f$, one
 * bit more where the rounding of the line in `double` spreads them); the
 * sequence takes
 * \f[
 *   \sum_j w_j ( p_{j+1} - p_j ) + m \, ( 192 + \lceil \log_2 n \rceil
 *   + 2 \lceil \log_2 m \rceil )
 * \f]
 * bits about for \f$ m \f$ segments: 24 bytes for the base, the slope and
 * the place and the width of the corrections, the first place in a field
 * of \f$ \lceil \log_2 n \rceil \f$ bits, and the two tables of the
 * searches, about a segment number each, in fields of
 * \f$ \lceil \log_2 m \rceil \f$ bits; the first item of a segment is not
 * kept, but read as its base plus its first correction. On
 * a sequence with long runs of nearly constant gaps (the places of a
 * regular pattern, the keys of a data set produced by a process) \f$ m \f$
 * is small and \f$ c \f$ bits are well below the
 * \f$ 2 + \log_2( u / n ) \f$ of Elias-Fano [see `EliasFano.h`]; on random
 * gaps the corrections cost about what the low bits of Elias-Fano cost and
 * the segments are pure overhead. `tuned()` picks the \f$ c \f$ of the
 * fewest bits, by building the sequence for each \f$ c \f$ in turn. The
 * LA-vector of the paper chooses \f$ c \f$ segment by segment, by a
 * shortest path on the segments of all the \f$ c \f$; here the width
 * \f$ w_j \f$ of each segment adapts to its corrections anyway, and on the
 * sequences of the benchmark learned the shortest path changed the bits by
 * a few hundredths of `tuned()`, at the price of a slower construction,
 * which is why it is not kept.
 *
 * The operations:
 *
 * - `access( i )` finds the segment of \f$ i \f$ through a table of the
 *   segment of every \f$ 2^t \f$-th place (\f$ 2^t \approx n / m \f$, so
 *   that a block of places meets one segment or two on the average), and
 *   returns \f$ f_j( i ) + C[ i ] \f$, in \f$ O( 1 ) \f$ expected time;
 * - `nextGEQ( x )`, the place of the first item \f$ \ge x \f$, finds the
 *   last segment whose first item is smaller than \f$ x \f$ through a table
 *   on the high bits of the items (\f$ m \f$ buckets, the first segment of
 *   each), then inverts the line: since
 *   \f$ f_j( i ) \le x_i < f_j( i ) + 2^{w_j} \f$, the answer lies between
 *   the places where \f$ f_j \f$ reaches \f$ x - 2^{w_j} + 1 \f$ and
 *   \f$ x \f$, \f$ 2^{w_j} / \alpha_j + 2 \f$ places, searched by a binary
 *   search on the corrections, in \f$ O( c - \log_2 \alpha_j ) \f$ time
 *   once the segment is found; the whole segment is searched when the
 *   floating point misplaces the window, which is checked.
 *
 * The contracts [see `Contract.h`]: the sequence is non-decreasing (an
 * audit) with fewer than \f$ 2^{32} \f$ items, \f$ 0 \le c \le 63 \f$,
 * `access()` needs \f$ i < n \f$; the audit of the construction is that
 * `access()` gives back each item, and the postcondition of `nextGEQ()` is
 * that the items before the place are \f$ < x \f$ and the one at it is not.
 * The functions are defined in `LAVector.tpp`.
 *
 * \see A. Boffa, P. Ferragina, G. Vinciguerra, A "learned" approach to
 *      quicken and compress rank/select dictionaries, Proceedings of the
 *      23rd ALENEX, 46-59, 2021, doi:10.1137/1.9781611976472.4
 * \see A. Boffa, P. Ferragina, G. Vinciguerra, A learned approach to design
 *      compressed rank/select data structures, ACM Transactions on
 *      Algorithms 18(3), 1-28, 2022, doi:10.1145/3524060
 * \see P. Ferragina, G. Vinciguerra, The PGM-index: a fully-dynamic
 *      compressed learned index with provable worst-case bounds, Proceedings
 *      of the VLDB Endowment 13(8), 1162-1175, 2020,
 *      doi:10.14778/3389133.3389135
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

#ifndef __ADS_LAVector
 #define __ADS_LAVector
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/succinct/BitVector.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS LAVector ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a non-decreasing sequence of integers of 64 bits as the segments of an
/// optimal piecewise linear approximation and the corrections of the items
class LAVector {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty sequence
 LAVector( void ) = default;

 /// the sequence \f$ x \f$ with corrections of \f$ c \f$ bits, i.e., the
 /// error \f$ \epsilon = 2^{c-1} - 1 \f$ (0 for \f$ c \le 1 \f$)
 ///
 /// In pseudocode (Boffa, Ferragina and Vinciguerra, beyond the book),
 /// \f$ T( v ) \f$ the integer part of \f$ v \ge 0 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( x , c ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{segment } j \text{ of }
 ///     \mathrm{approximate}( ( i , x_i ) , \epsilon )\ \textbf{do}\quad
 ///     \triangleright\ \text{see } \mathrm{PGMIndex} \cr
 /// 3 & \qquad \textbf{for}\ i \gets p_j\ \textbf{to}\ p_{j+1} - 1
 ///     \ \textbf{do}\ d_i \gets x_i - x_{p_j} - T( \alpha_j ( i - p_j ) )
 ///     \cr
 /// 4 & \qquad \beta_j \gets x_{p_j} + \min_i d_i;\ w_j \gets
 ///     \lceil \log_2( \max_i d_i - \min_i d_i + 1 ) \rceil \cr
 /// 5 & \qquad \textbf{for}\ i \gets p_j\ \textbf{to}\ p_{j+1} - 1
 ///     \ \textbf{do}\ \mathrm{append}( C , d_i - \min_i d_i , w_j ) \cr
 /// 6 & \quad \text{fill the table of the segment of every } 2^t
 ///     \text{-th place and the table on the high bits of the first items}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is non-decreasing (an audit), \f$ n < 2^{32} \f$ and
 ///      \f$ 0 \le c \le 63 \f$
 explicit LAVector( const std::vector< std::uint64_t > & x , int c = 7 );

 /// the sequence \f$ x \f$ with the \f$ c \f$ of the fewest bits, among
 /// the odd ones up to \f$ c_{\mathrm{max}} \f$ of `widest()` and the two
 /// around the best of them
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tuned}( x ) \cr
 /// 2 & \quad b \gets \mathrm{build}( x , 1 );\ t \gets c_{\mathrm{max}}
 ///     \cr
 /// 3 & \quad \textbf{for}\ c \gets 3\ \textbf{to}\ t\ \textbf{step}\ 2
 ///     \ \textbf{do} \cr
 /// 4 & \qquad v \gets \mathrm{build}( x , c );\ \textbf{if}\ v \text{ takes}
 ///     \text{ fewer bits than } b\ \textbf{then}\ b \gets v \cr
 /// 5 & \quad \textbf{foreach}\ c \in \{ c_b - 1 , c_b + 1 \}\ \textbf{do}
 ///     \ \text{the same} \cr
 /// 6 & \quad \textbf{return}\ b
 /// \end{array}}
 /// \f]
 /// \pre as for the constructor
 [[nodiscard]] static LAVector tuned( const std::vector< std::uint64_t > & x );

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const;

 /// the bits \f$ c \f$ of the corrections asked for
 [[nodiscard]] int correctionBits( void ) const;

 /// the number of segments, \f$ m \f$
 [[nodiscard]] std::size_t segments( void ) const;

 /// the item \f$ i \f$
 ///
 /// In pseudocode (Boffa, Ferragina and Vinciguerra, beyond the book),
 /// \f$ J \f$ the table of the segment of every \f$ 2^t \f$-th place:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad j \gets J[ \lfloor i / 2^t \rfloor ] \cr
 /// 3 & \quad \textbf{while}\ p_{j+1} \le i\ \textbf{do}\ j \gets j + 1 \cr
 /// 4 & \quad \textbf{return}\ \beta_j + T( \alpha_j ( i - p_j ) ) + C[ i ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 /// the item \f$ i \f$, as `access()`
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const;

 /// the \f$ k \f$-th smallest item, i.e., `access( k - 1 )`, the select of
 /// the vector of bits whose 1s are the items
 /// \pre \f$ 1 \le k \le \f$ `size()`
 [[nodiscard]] std::uint64_t select( std::size_t k ) const;

 /// the place of the first item \f$ \ge x \f$, `size()` if there is none,
 /// i.e., the number of items \f$ < x \f$, the rank of the vector of bits
 /// whose 1s are the items
 ///
 /// In pseudocode (Boffa, Ferragina and Vinciguerra, beyond the book),
 /// \f$ V \f$ the table of the first segment of each bucket of the high
 /// bits and \f$ \ell \f$ the low bits left out:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{nextGEQ}( x ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{or}\ x \le x_0\ \textbf{then return}
 ///     \ 0 \cr
 /// 3 & \quad \textbf{if}\ x > x_{n-1}\ \textbf{then return}\ n \cr
 /// 4 & \quad h \gets \lfloor ( x - x_0 ) / 2^\ell \rfloor \cr
 /// 5 & \quad j \gets \text{the last segment in } [ V[ h ] - 1 , V[ h + 1 ] ]
 ///     \text{ with } x_{p_j} < x\text{, by a binary search} \cr
 /// 6 & \quad a \gets p_j + \lfloor ( x - \beta_j - 2^{w_j} ) / \alpha_j
 ///     \rfloor - 1;\ b \gets p_j + \lceil ( x - \beta_j + 1 ) / \alpha_j
 ///     \rceil + 1 \cr
 /// 7 & \quad \textbf{if}\ \text{not}\ x_{a-1} < x \le x_b\ \textbf{then}
 ///     \ ( a , b ) \gets ( p_j , p_{j+1} ) \cr
 /// 8 & \quad \textbf{return}\ \text{the first } i \in [ a , b ] \text{ with }
 ///     x_i \ge x \text{, by a binary search}
 /// \end{array}}
 /// \f]
 /// \post the items before it are \f$ < x \f$, the one at it is
 ///       \f$ \ge x \f$
 [[nodiscard]] std::size_t nextGEQ( std::uint64_t x ) const;

 /// the number of items \f$ < x \f$, as `nextGEQ()`
 [[nodiscard]] std::size_t rank( std::uint64_t x ) const;

 /// whether \f$ x \f$ is an item, by `nextGEQ()`
 [[nodiscard]] bool contains( std::uint64_t x ) const;

 /// the bits of the corrections
 [[nodiscard]] std::size_t correctionsInBits( void ) const;

 /// all the bits: the corrections, the segments and the tables
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
 using const_iterator = SubscriptIterator< LAVector , std::uint64_t >;
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

 /// a segment: the base, the slope, and where its corrections begin
 /// times 128 plus their width
 struct Segment {
  std::uint64_t base;        ///< \f$ \beta_j \f$
  double slope;              ///< \f$ \alpha_j \f$, not negative
  std::uint64_t ow;          ///< its first bit in C times 128, plus w_j
  };

 /// integers of a fixed number of bits, the least that holds them all,
 /// one after the other in words with one more at the end, so that an
 /// integer is read by one load of 8 bytes from its first byte
 struct Packed {
  std::vector< std::uint64_t > words;   ///< the bits, from the lowest
  int width = 0;                        ///< the bits of an integer
  std::uint64_t mask = 0;               ///< their mask
  std::size_t bits = 0;                 ///< the bits of the integers

  /// the integers of \f$ v \f$
  void pack( const std::vector< std::uint64_t > & v ) {
   std::uint64_t top = 0;
   for( const auto x : v ) top = std::max( top , x );
   width = top == 0 ? 0 : lambda( top ) + 1;
   ADS_PRE( width <= 56 );
   mask = ( std::uint64_t( 1 ) << width ) - 1;
   bits = v.size() * std::size_t( width );
   words.assign( bits / 64 + 2 , 0 );
   for( std::size_t i = 0 , b = 0 ; i < v.size() ; ++i , b += width ) {
    words[ b / 64 ] |= v[ i ] << ( b % 64 );
    if( b % 64 + std::size_t( width ) > 64 )
     words[ b / 64 + 1 ] |= v[ i ] >> ( 64 - b % 64 );
    }
   }
  /// the integers \f$ i \f$ and \f$ i + 1 \f$, by one load when they
  /// fit in it
  [[nodiscard]] std::pair< std::size_t , std::size_t > pair(
                                                   std::size_t i ) const {
   if constexpr( std::endian::native == std::endian::little )
    if( width <= 28 ) {
     const std::size_t b = i * std::size_t( width );
     std::uint64_t x;
     std::memcpy( &x , reinterpret_cast< const unsigned char * >(
                         words.data() ) + b / 8 , 8 );
     x >>= b % 8;
     return( std::make_pair( std::size_t( x & mask ) ,
                             std::size_t( ( x >> width ) & mask ) ) );
     }
   return( std::make_pair( ( *this )[ i ] , ( *this )[ i + 1 ] ) );
   }
  /// the integer \f$ i \f$
  [[nodiscard]] std::size_t operator[]( std::size_t i ) const {
   const std::size_t b = i * std::size_t( width );
   if constexpr( std::endian::native == std::endian::little ) {
    std::uint64_t x;
    std::memcpy( &x , reinterpret_cast< const unsigned char * >(
                        words.data() ) + b / 8 , 8 );
    return( std::size_t( ( x >> ( b % 8 ) ) & mask ) );
    }
   else {
    const std::size_t o = b % 64;
    return( std::size_t( ( ( words[ b / 64 ] >> o ) |
                           ( ( words[ b / 64 + 1 ] << 1 ) << ( 63 - o ) ) )
                         & mask ) );
    }
   }
  };

 /// the widest useful correction of the items \f$ x \f$, \f$ c_{\mathrm{max}}
 /// = \min\{ 63 , \max\{ \lfloor \log_2 g \rfloor + 6 , \lceil ( \lfloor
 /// \log_2 x_{n-1} \rfloor + 1 ) / 2 \rceil \} \} \f$, \f$ g \f$ the
 /// average gap: wider corrections than six bits above those of the
 /// average gap, or than half the bits of the items, never pay
 [[nodiscard]] static int widest( const std::vector< std::uint64_t > & x );

 /// the error of the corrections of \f$ c \f$ bits
 [[nodiscard]] static std::int64_t error( int c );

 /// appends the segment of the places \f$ [ f , l ) \f$ of \f$ x \f$ with
 /// the slope \f$ a \f$
 void append( const std::vector< std::uint64_t > & x , std::size_t f ,
              std::size_t l , double a );

 /// the tables of the searches, once the segments are appended
 void finish( const std::vector< std::uint64_t > & x );

 /// the integer part of \f$ v \f$, 0 below 0 and \f$ 2^{64} - 1 \f$ from
 /// \f$ 2^{64} \f$ on
 [[nodiscard]] static std::uint64_t whole( double v );

 /// the segment of the place \f$ i \f$, and in \f$ p \f$ its first place
 [[nodiscard]] std::size_t segmentOf( std::size_t i , std::size_t & p )
  const;

 /// the item \f$ d \f$ places after the first of the segment \f$ j \f$
 [[nodiscard]] std::uint64_t item( std::size_t j , std::size_t d ) const;

 std::size_t f_n = 0;                    ///< the items
 int f_c = 0;                            ///< the bits asked for
 BitVector f_C;                          ///< the corrections
 std::vector< Segment > f_seg;           ///< the segments
 Packed f_p;                             ///< their first places, and n
 std::vector< std::uint64_t > f_first;   ///< the same, while building
 Packed f_J;                             ///< the segment of each block
 int f_t = 0;                            ///< the places of a block, log
 Packed f_V;                             ///< the segments before a bucket
 int f_l = 0;                            ///< the low bits of a bucket
 std::uint64_t f_x0 = 0;                 ///< the first item
 std::uint64_t f_last = 0;               ///< the last item

 };  // end( class LAVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/LAVector.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LAVector.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File LAVector.h ---------------------------*/
/*--------------------------------------------------------------------------*/
