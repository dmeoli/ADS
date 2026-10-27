/*--------------------------------------------------------------------------*/
/*---------------------------- File SellMatrix.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A sparse matrix in the format SELL-\f$ C \f$-\f$ \sigma \f$ of Kreutzer,
 * Hager, Wellein, Fehske and Bishop, beyond the book: the rows, sorted by
 * their number of nonzeros within windows of \f$ \sigma \f$ consecutive
 * rows, go in chunks of \f$ C \f$ rows, and each chunk keeps its nonzeros by
 * columns of the chunk, the \f$ k \f$-th nonzero of its \f$ C \f$ rows side
 * by side, padded with zeros to the longest of its rows. The product
 * \f$ y = A x \f$ then runs the \f$ C \f$ rows of a chunk in the \f$ C \f$
 * lanes of the vector registers (\f$ C = 8 \f$ doubles for AVX-512, 4 for
 * AVX2), the values and the columns of a step loaded in one stream and
 * \f$ x \f$ gathered, instead of one row at a time with a reduction at its
 * end, as in CSR [see `CsrMatrix.h`]. The sort within the windows makes the
 * rows of a chunk nearly as long, so that the padding,
 * \f$ \sum_c C \max_{i \in c} r_i - \mathit{nnz} \f$ for the chunks
 * \f$ c \f$ and the row lengths \f$ r_i \f$, is small (none on the rows of
 * equal length of a stencil), while a window of \f$ \sigma \ll m \f$ rows
 * keeps the rows near their place and so the reads of \f$ x \f$ and the
 * writes of \f$ y \f$ near each other.
 *
 * - The matrix is built from a `CsrMatrix` in
 *   \f$ O( \mathit{nnz} + m \log \sigma ) \f$ time: the sort of each window
 *   by row length (stable, so that rows of equal length keep their order),
 *   the widths of the chunks, and the placement of the nonzeros; it takes
 *   \f$ 12 ( \mathit{nnz} + \mathit{pad} ) + 8 m / C + 4 m \f$ bytes: the
 *   values and the columns, the offset and the width of each chunk, and the
 *   permutation of the rows.
 * - `multiply()` is \f$ y = A x \f$: for each chunk, \f$ C \f$ sums in the
 *   lanes over the width of the chunk, then each sum written to its row;
 *   \f$ 2 ( \mathit{nnz} + \mathit{pad} ) \f$ operations, all of them in
 *   full vectors.
 *
 * The contracts [see `Contract.h`]: \f$ \sigma \ge 1 \f$ and
 * \f$ m < 2^{32} \f$; \f$ x \f$ has \f$ n \f$ entries and \f$ y \f$ \f$ m \f$
 * entries. The functions are defined in `SellMatrix.tpp`.
 *
 * \see M. Kreutzer, G. Hager, G. Wellein, H. Fehske, A. R. Bishop, A
 *      unified sparse matrix data format for efficient general sparse
 *      matrix-vector multiplication on modern processors with wide SIMD
 *      units, SIAM Journal on Scientific Computing 36(5), C401-C423, 2014,
 *      doi:10.1137/130930352
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

#ifndef __ADS_SellMatrix
 #define __ADS_SellMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/CsrMatrix.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS SellMatrix ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sparse matrix of doubles in SELL-\f$ C \f$-\f$ \sigma \f$, chunks of
/// \f$ C \f$ rows (4 by default, the fastest on a Xeon Gold 5120 with
/// AVX-512: the gathers of 8 lanes cost more than they save)
template< std::size_t C = 4 >
class SellMatrix {
 static_assert( ( C >= 1 ) && ( ( C & ( C - 1 ) ) == 0 ) ,
                "the rows of a chunk are a power of 2" );
 public:

 /// the matrix \f$ A \f$, its rows sorted by length within windows of
 /// \f$ \sigma \f$ rows
 ///
 /// In pseudocode (Kreutzer and others, beyond the book), the rows padded
 /// to whole chunks by empty ones:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{SellMatrix}( A , \sigma ) \cr
 /// 2 & \quad \pi \gets ( 0 , 1 , \ldots , m - 1 ) \cr
 /// 3 & \quad \textbf{foreach}\ \text{window of } \sigma \text{ slots of }
 ///     \pi\ \textbf{do}\ \text{sort it stably by decreasing row length}
 ///     \cr
 /// 4 & \quad \textbf{foreach}\ \text{chunk } c \text{ of the slots }
 ///     c C , \ldots , c C + C - 1\ \textbf{do} \cr
 /// 5 & \qquad w_c \gets \max_l r_{\pi( c C + l )};
 ///     \ \mathit{start}[ c + 1 ] \gets \mathit{start}[ c ] + C w_c \cr
 /// 6 & \qquad \textbf{foreach}\ l < C , k < r_{\pi( c C + l )}\ \textbf{do}
 ///     \cr
 /// 7 & \qquad\quad \text{place the } k \text{-th nonzero of the row }
 ///     \pi( c C + l ) \text{ at } \mathit{start}[ c ] + k C + l \cr
 /// 8 & \qquad \text{the places left get the value 0}
 /// \end{array}}
 /// \f]
 /// \pre \f$ \sigma \ge 1 \f$ and \f$ m < 2^{32} \f$
 explicit SellMatrix( const CsrMatrix & A , std::size_t sigma = 256 );

 /// the number \f$ m \f$ of rows
 [[nodiscard]] std::size_t rows( void ) const { return( f_m ); }
 /// the number \f$ n \f$ of columns
 [[nodiscard]] std::size_t cols( void ) const { return( f_n ); }

 /// the number \f$ \mathit{nnz} \f$ of the nonzeros of \f$ A \f$
 [[nodiscard]] std::size_t nonzeros( void ) const { return( f_nnz ); }
 /// the places kept, \f$ \mathit{nnz} \f$ and the padding
 [[nodiscard]] std::size_t places( void ) const { return( f_val.size() ); }

 /// the product \f$ y = A x \f$ into \f$ y \f$, which must have \f$ m \f$
 /// entries, as the chunks from 0 to `chunks()`
 /// \pre \f$ x \f$ has \f$ n \f$ entries and \f$ y \f$ \f$ m \f$ entries,
 ///      apart from \f$ x \f$
 void multiply( const double * x , double * y ) const;

 /// the entries of \f$ y = A x \f$ of the rows of the chunks
 /// \f$ [ \mathit{first} , \mathit{last} ) \f$ of \f$ C \f$ rows (by the
 /// permutation), so that the chunks may be split among threads
 ///
 /// In pseudocode (Kreutzer and others, beyond the book), the \f$ C \f$
 /// lanes in the registers:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{multiply}( x , y , \mathit{first} ,
 ///     \mathit{last} ) \cr
 /// 2 & \quad \textbf{for}\ c \gets \mathit{first}\ \textbf{to}
 ///     \ \mathit{last} - 1\ \textbf{do} \cr
 /// 3 & \qquad s_0 , \ldots , s_{C-1} \gets 0 \cr
 /// 4 & \qquad \textbf{for}\ p \gets \mathit{start}[ c ] , \mathit{start}[ c
 ///     ] + C , \ldots < \mathit{start}[ c + 1 ]\ \textbf{do} \cr
 /// 5 & \qquad\quad \textbf{foreach}\ l < C\ \textbf{do}\ s_l \gets s_l +
 ///     \mathit{val}[ p + l ] \, x_{\mathit{col}[ p + l ]} \cr
 /// 6 & \qquad \textbf{foreach}\ l < C \text{ of a real row}\ \textbf{do}
 ///     \ y_{\pi( c C + l )} \gets s_l
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ has \f$ n \f$ entries and \f$ y \f$ \f$ m \f$ entries,
 ///      apart from \f$ x \f$,
 ///      \f$ \mathit{first} \le \mathit{last} \le \mathrm{chunks}() \f$
 void multiply( const double * x , double * y , std::size_t first ,
                std::size_t last ) const;

 /// the number of chunks
 [[nodiscard]] std::size_t chunks( void ) const {
  return( f_start.size() - 1 );
  }

 /// the place where the chunk \f$ c \f$ starts, `chunks()` for the end
 [[nodiscard]] std::size_t chunkStart( std::size_t c ) const {
  return( f_start[ c ] );
  }

 /// the product \f$ y = A x \f$, a new vector
 /// \pre \f$ x \f$ has \f$ n \f$ entries
 [[nodiscard]] std::vector< double > multiply(
                                     const std::vector< double > & x ) const;

 private:
 std::size_t f_m ,                         ///< the rows
             f_n ,                         ///< the columns
             f_nnz;                        ///< the nonzeros
 /// the `chunks() + 1` offsets of the chunks in the places
 std::vector< std::size_t > f_start;
 std::vector< std::uint32_t > f_perm;      ///< the row of each slot
 std::vector< std::uint32_t > f_col;       ///< the column of each place
 std::vector< double > f_val;              ///< its value, 0 for padding
 };  // end( class SellMatrix )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/SellMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SellMatrix.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SellMatrix.h --------------------------*/
/*--------------------------------------------------------------------------*/
