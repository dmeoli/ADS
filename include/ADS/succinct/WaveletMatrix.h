/*--------------------------------------------------------------------------*/
/*-------------------------- File WaveletMatrix.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The wavelet matrix of Claude, Navarro and Ordóñez: a static sequence
 * \f$ S[ 0 .. n - 1 ] \f$ of integers of \f$ w \f$ bits, i.e., on the
 * alphabet \f$ [ 0 , 2^w ) \f$, in \f$ n w \f$ bits plus the directories of
 * rank and select [see `RankSelect.h`], with access, rank and select of any
 * symbol in \f$ O( w ) \f$ time, and the queries on a range of places that a
 * sequence of integers needs (the \f$ k \f$-th smallest item of a range, the
 * items below a value), also in \f$ O( w ) \f$ time.
 *
 * The level \f$ \ell = 0 , \ldots , w - 1 \f$ is a vector of bits
 * \f$ B_\ell \f$ that holds the bit \f$ w - 1 - \ell \f$ (from the most
 * significant one) of each item, in the order that the levels above leave:
 * \f$ B_0 \f$ is in the order of \f$ S \f$, and the items go to the next
 * level stably sorted by the bit, the \f$ Z_\ell \f$ ones with a 0 first and
 * those with a 1 after them. Hence the item at the place \f$ i \f$ of a
 * level is at the place \f$ \mathrm{rank}_0( i ) \f$ of the next one if its
 * bit is 0, and at \f$ Z_\ell + \mathrm{rank}_1( i ) \f$ if it is 1, and the
 * items equal to \f$ c \f$, in the order of \f$ S \f$, are consecutive at
 * the last level, at a place that the bits of \f$ c \f$ alone determine:
 *
 * - `access( i )` follows the item down, a bit and a rank per level;
 * - `rank( c , i )`, the number of \f$ c \f$ in \f$ S[ 0 .. i - 1 ] \f$,
 *   follows the places 0 and \f$ i \f$ down along the bits of \f$ c \f$, and
 *   the difference at the last level is the answer;
 * - `select( c , k )`, the place of the \f$ k \f$-th \f$ c \f$, goes down to
 *   the place of the \f$ k \f$-th \f$ c \f$ at the last level and back up by
 *   select, \f$ \mathrm{select}_0 \f$ or \f$ \mathrm{select}_1 \f$ as the
 *   bit of \f$ c \f$ at each level;
 * - `quantile( i , j , k )`, the \f$ k \f$-th smallest item of
 *   \f$ S[ i .. j - 1 ] \f$, goes down the levels with the range, to the 0s
 *   if the range has more than \f$ k \f$ of them and to the 1s otherwise,
 *   and `countLess( i , j , c )` sums the 0s that the range leaves at the
 *   levels where \f$ c \f$ has a 1.
 *
 * Unlike the wavelet tree of Grossi, Gupta and Vitter, whose nodes split
 * the alphabet in halves and need a pointer or an offset each, the matrix
 * keeps one vector of \f$ n \f$ bits per level and no tree, at the same
 * costs: it is the form of choice for the large alphabets (Claude, Navarro
 * and Ordóñez), and the base of the compressed indices of the integers and
 * of the r-index. A wavelet tree shaped by the code of Huffman (Mäkinen and
 * Navarro) takes \f$ n ( H_0 + 1 ) \f$ bits instead of \f$ n w \f$ and is
 * faster on a skewed alphabet [see `HuffmanWaveletTree.h`]; the matrix here
 * is the form for the alphabets of integers with no known distribution.
 * Each level carries the directory of `RankSelect` [see `RankSelect.h`], so
 * that the matrix takes about \f$ 1.5 \, n w \f$ bits, and a query of rank
 * \f$ 2 w \f$ ranks of one load and one sideways sum each.
 *
 * The contracts [see `Contract.h`]: the items fit in \f$ w \f$ bits,
 * \f$ 1 \le w \le 64 \f$, the places are in the sequence,
 * \f$ i \le j \f$ for a range, and \f$ k \f$ counts from 1 for select and
 * from 0 for the quantile, below the number of the items; the audit of the
 * construction is that `access()` gives back each item. The functions are
 * defined in `WaveletMatrix.tpp`.
 *
 * \see F. Claude, G. Navarro, A. Ordóñez, The wavelet matrix: an efficient
 *      wavelet tree for large alphabets, Information Systems 47, 15-32,
 *      2015, doi:10.1016/j.is.2014.06.002
 * \see R. Grossi, A. Gupta, J. S. Vitter, High-order entropy-compressed
 *      text indexes, Proc. 14th SODA, 841-850, 2003
 * \see V. Mäkinen, G. Navarro, Succinct suffix arrays based on run-length
 *      encoding, Proc. 16th CPM, LNCS 3537, 45-56, 2005,
 *      doi:10.1007/11496656_5
 * \see G. Navarro, Wavelet trees for all, Journal of Discrete Algorithms
 *      25, 2-20, 2014, doi:10.1016/j.jda.2013.07.004
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

#ifndef __ADS_WaveletMatrix
 #define __ADS_WaveletMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS WaveletMatrix --------------------------*/
/*--------------------------------------------------------------------------*/

/// a static sequence of integers of \f$ w \f$ bits with access, rank, select
/// and the range quantiles in \f$ O( w ) \f$ time
class WaveletMatrix {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty sequence
 WaveletMatrix( void ) = default;

 /// the sequence \f$ x \f$, its items of \f$ w \f$ bits (0: the bits of the
 /// largest one), built by the passes of up to 8 levels of the constructor
 /// below on the calling thread, the levels those of a stable partition by
 /// each bit, from the most significant one (see Lemma 7 of
 /// \ref theory_succinct_parallel)
 ///
 /// In pseudocode (Claude, Navarro and Ordóñez, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( x , w ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{build}( \text{the calling thread}
 ///     , x , w )
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le w \le 64 \f$ and the items are below \f$ 2^w \f$
 explicit WaveletMatrix( std::span< const std::uint64_t > x , int w = 0 );

 /// the matrix of `WaveletMatrix( x , w )` above, the same levels and
 /// directories, built on the threads of the executor `ex` by up to 8
 /// levels at a time (see \ref theory_succinct_parallel)
 ///
 /// In pseudocode (the stable sorts by the prefixes of the bits, beyond the
 /// book), \f$ x \f$ cut into slices \f$ X_0 , \ldots , X_{q-1} \f$, the
 /// levels \f$ [ \ell , \ell + k ) \f$ of a pass, \f$ b_j( c ) \f$ the bit
 /// \f$ w - 1 - \ell - j \f$ of \f$ c \f$ and
 /// \f$ \kappa_j( c ) = \sum_{i < j} 2^i b_i( c ) \f$, the key by which
 /// the level \f$ \ell + j \f$ is the stable sort of the level
 /// \f$ \ell \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( \mathit{ex} , x , w ) \cr
 /// 2 & \quad C \gets x;\ \ell \gets 0 \cr
 /// 3 & \quad \textbf{while}\ \ell < w\ \textbf{do} \cr
 /// 4 & \qquad k \gets \text{the levels of the pass, at most 8} \cr
 /// 5 & \qquad \textbf{foreach}\ \text{slice } X_s \text{ of } C
 ///     \ \textbf{do in parallel}\ h_s[ a ] \gets | \{ c \in X_s :
 ///     \kappa_k( c ) = a \} | \cr
 /// 6 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ k\ \textbf{do}
 ///     \ P_{j , s}[ a ] \gets \text{the items of } C \text{ with }
 ///     \kappa_j < a \cr
 /// & \qquad\quad \text{or with } \kappa_j = a \text{ in the slices }
 ///     \text{before } X_s \text{, by the sums of the } h \cr
 /// 7 & \qquad Z_{\ell + j} \gets \text{the items with } b_j = 0
 ///     \text{, for } j < k \cr
 /// 8 & \qquad \textbf{foreach}\ \text{slice } X_s\ \textbf{do in}
 ///     \ \textbf{parallel} \cr
 /// 9 & \qquad\quad Q_{j , s}[ a ] \gets \text{the items of } X_s
 ///     \text{ with } \kappa_j < a\quad \triangleright\ \text{places in}
 ///     \text{ the slice} \cr
 /// 10 & \qquad\quad \textbf{foreach}\ c \in X_s \text{ in order}
 ///     \ \textbf{do} \cr
 /// 11 & \qquad\qquad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1
 ///     \ \textbf{do}\ L_{s , j}[ Q_{j , s}[ \kappa_j( c ) ] ] \gets
 ///     b_j( c );\ Q_{j , s}[ \kappa_j( c ) ] \gets Q_{j , s}[ \kappa_j( c )
 ///     ] + 1 \cr
 /// 12 & \qquad\qquad C'[ P_{k , s}[ \kappa_k( c ) ] ] \gets c;
 ///     \ P_{k , s}[ \kappa_k( c ) ] \gets P_{k , s}[ \kappa_k( c ) ] + 1
 ///     \cr
 /// 13 & \qquad \textbf{foreach}\ \text{block of words of each }
 ///     B_{\ell + j}\ \textbf{do in parallel} \cr
 /// 14 & \qquad\quad \text{copy into it the runs of the } L_{s , j}
 ///     \text{ that fall in it, each at its place } P_{j , s} \cr
 /// 15 & \qquad C \gets C';\ \ell \gets \ell + k \cr
 /// 16 & \quad \textbf{return}\ \text{the levels } B_\ell \text{ with }
 ///     \text{their directories, built on } \mathit{ex} \text{, and the }
 ///     Z_\ell
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le w \le 64 \f$ and the items are below \f$ 2^w \f$
 template< Executor E >
 WaveletMatrix( E & ex , std::span< const std::uint64_t > x , int w = 0 );

 /// the sequence \f$ x \f$ of items of 8, 16 or 32 bits, which the
 /// construction reads as they are, as `WaveletMatrix( x , w )` above
 template< class U >
 requires( std::is_same_v< U , std::uint8_t > ||
           std::is_same_v< U , std::uint16_t > ||
           std::is_same_v< U , std::uint32_t > )
 explicit WaveletMatrix( std::span< const U > x , int w = 0 );

 /// the sequence \f$ x \f$ of items of 8, 16 or 32 bits on the threads of
 /// `ex`, as `WaveletMatrix( ex , x , w )` above
 template< Executor E , class U >
 requires( std::is_same_v< U , std::uint8_t > ||
           std::is_same_v< U , std::uint16_t > ||
           std::is_same_v< U , std::uint32_t > )
 WaveletMatrix( E & ex , std::span< const U > x , int w = 0 );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the bits of an item
 [[nodiscard]] int width( void ) const { return( int( f_lev.size() ) ); }

 /// the item \f$ i \f$
 ///
 /// In pseudocode (Claude, Navarro and Ordóñez, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad c \gets 0 \cr
 /// 3 & \quad \textbf{for}\ \ell \gets 0\ \textbf{to}\ w - 1\ \textbf{do} \cr
 /// 4 & \qquad b \gets B_\ell[ i ];\ c \gets 2 c + b \cr
 /// 5 & \qquad \textbf{if}\ b = 1\ \textbf{then}\ i \gets Z_\ell +
 ///     \mathrm{rank}_1( B_\ell , i )\ \textbf{else}\ i \gets
 ///     \mathrm{rank}_0( B_\ell , i ) \cr
 /// 6 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \mathrm{size}() \f$
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 /// the item \f$ i \f$, by `access()`
 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the number of \f$ c \f$ in the places \f$ [ 0 , i ) \f$
 ///
 /// In pseudocode (Claude, Navarro and Ordóñez, beyond the book), with
 /// \f$ c_\ell \f$ the bit \f$ w - 1 - \ell \f$ of \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank}( c , i ) \cr
 /// 2 & \quad \textbf{if}\ c \ge 2^w\ \textbf{then return}\ 0 \cr
 /// 3 & \quad s \gets 0 \cr
 /// 4 & \quad \textbf{for}\ \ell \gets 0\ \textbf{to}\ w - 1\ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ c_\ell = 1\ \textbf{then}\ s \gets Z_\ell +
 ///     \mathrm{rank}_1( B_\ell , s );\ i \gets Z_\ell + \mathrm{rank}_1(
 ///     B_\ell , i ) \cr
 /// 6 & \qquad \textbf{else}\ s \gets \mathrm{rank}_0( B_\ell , s );\ i
 ///     \gets \mathrm{rank}_0( B_\ell , i ) \cr
 /// 7 & \quad \textbf{return}\ i - s
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le \mathrm{size}() \f$
 [[nodiscard]] std::size_t rank( std::uint64_t c , std::size_t i ) const;

 /// the place of the \f$ k \f$-th \f$ c \f$
 ///
 /// In pseudocode (Claude, Navarro and Ordóñez, beyond the book), down to
 /// the start of the items equal to \f$ c \f$ at the last level, as
 /// `rank()` does with \f$ s \f$, and back up, the ranks of select counted
 /// from 1:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( c , k ) \cr
 /// 2 & \quad p \gets \text{the place of } c \text{ at the last level} + k -
 ///     1 \cr
 /// 3 & \quad \textbf{for}\ \ell \gets w - 1\ \textbf{downto}\ 0
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ c_\ell = 1\ \textbf{then}\ p \gets
 ///     \mathrm{select}_1( B_\ell , p - Z_\ell + 1 )\ \textbf{else}\ p
 ///     \gets \mathrm{select}_0( B_\ell , p + 1 ) \cr
 /// 5 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le k \le \mathrm{rank}( c , \mathrm{size}() ) \f$
 /// \post the item at the place is \f$ c \f$, with \f$ k - 1 \f$ of them
 ///       before it
 [[nodiscard]] std::size_t select( std::uint64_t c , std::size_t k ) const;

 /// the \f$ k \f$-th smallest item of the places \f$ [ i , j ) \f$, from
 /// \f$ k = 0 \f$
 ///
 /// In pseudocode (Claude, Navarro and Ordóñez, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{quantile}( i , j , k ) \cr
 /// 2 & \quad c \gets 0 \cr
 /// 3 & \quad \textbf{for}\ \ell \gets 0\ \textbf{to}\ w - 1\ \textbf{do} \cr
 /// 4 & \qquad i_1 \gets \mathrm{rank}_1( B_\ell , i );\ j_1 \gets
 ///     \mathrm{rank}_1( B_\ell , j );\ z \gets ( j - j_1 ) - ( i - i_1 ) \cr
 /// 5 & \qquad \textbf{if}\ k < z\ \textbf{then}\ c \gets 2 c;\ i \gets i -
 ///     i_1;\ j \gets j - j_1\quad \triangleright\ \text{to the 0s} \cr
 /// 6 & \qquad \textbf{else}\ c \gets 2 c + 1;\ k \gets k - z;\ i \gets
 ///     Z_\ell + i_1;\ j \gets Z_\ell + j_1 \cr
 /// 7 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j \le \mathrm{size}() \f$ and \f$ k < j - i \f$
 [[nodiscard]] std::uint64_t quantile( std::size_t i , std::size_t j ,
                                       std::size_t k ) const;

 /// the number of the items below \f$ c \f$ in the places \f$ [ i , j ) \f$
 ///
 /// In pseudocode (Claude, Navarro and Ordóñez, beyond the book), the range
 /// following the bits of \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{countLess}( i , j , c ) \cr
 /// 2 & \quad \textbf{if}\ c \ge 2^w\ \textbf{then return}\ j - i \cr
 /// 3 & \quad r \gets 0 \cr
 /// 4 & \quad \textbf{for}\ \ell \gets 0\ \textbf{to}\ w - 1\ \textbf{while}
 ///     \ i < j\ \textbf{do} \cr
 /// 5 & \qquad i_1 \gets \mathrm{rank}_1( B_\ell , i );\ j_1 \gets
 ///     \mathrm{rank}_1( B_\ell , j ) \cr
 /// 6 & \qquad \textbf{if}\ c_\ell = 1\ \textbf{then}\ r \gets r + ( j - j_1
 ///     ) - ( i - i_1 );\ i \gets Z_\ell + i_1;\ j \gets Z_\ell + j_1 \cr
 /// 7 & \qquad \textbf{else}\ i \gets i - i_1;\ j \gets j - j_1 \cr
 /// 8 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j \le \mathrm{size}() \f$
 [[nodiscard]] std::size_t countLess( std::size_t i , std::size_t j ,
                                      std::uint64_t c ) const;

 /// the bits of the levels and of their directories
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = std::uint64_t;            ///< the type of an item
 using size_type = std::size_t;               ///< the type of a size
 using difference_type = std::ptrdiff_t;      ///< the type of a distance
 using reference = std::uint64_t;             ///< an item, by value
 using const_reference = std::uint64_t;       ///< an item, by value
 /// an iterator on the items, by `access()`
 using const_iterator = SubscriptIterator< WaveletMatrix , std::uint64_t >;
 using iterator = const_iterator;             ///< the items are read only

 /// the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// one past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 /// the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// one past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the bit of \f$ c \f$ at the level \f$ l \f$
 [[nodiscard]] bool bit( std::uint64_t c , std::size_t l ) const {
  return( ( c >> ( f_lev.size() - 1 - l ) ) & 1 );
  }

 /// the place of \f$ c \f$ at the last level, where its items start
 [[nodiscard]] std::size_t start( std::uint64_t c ) const;

 /// the fewest items of a slice of the parallel construction
#ifndef ADS_WAVELET_SLICE
 static constexpr std::size_t kSlice = std::size_t( 1 ) << 16;
#else
 static constexpr std::size_t kSlice = ADS_WAVELET_SLICE;   // a test
#endif

 /// the most items of a slice, whose 8 levels take 256 KB
#ifndef ADS_WAVELET_SLICE_MAX
 static constexpr std::size_t kSliceMax = std::size_t( 1 ) << 18;
#else
 static constexpr std::size_t kSliceMax = ADS_WAVELET_SLICE_MAX;   // a test
#endif

 /// the width w (0: the bits of the largest item) and the levels of the
 /// items x, on the threads of ex
 template< class U , Executor E >
 void init( E & ex , std::span< const U > x , int w );

 /// the levels of the items x of w bits on the threads of ex, the items
 /// moved as T between the passes
 template< class T , class U , Executor E >
 void build( E & ex , std::span< const U > x , int w );

 std::size_t f_n = 0;                    ///< the number of items
 std::vector< RankSelect > f_lev;        ///< the levels, the top first
 std::vector< std::size_t > f_zeros;     ///< the 0s of each level

 };  // end( class WaveletMatrix )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/WaveletMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WaveletMatrix.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File WaveletMatrix.h ------------------------*/
/*--------------------------------------------------------------------------*/
