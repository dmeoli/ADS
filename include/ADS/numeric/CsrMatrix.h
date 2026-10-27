/*--------------------------------------------------------------------------*/
/*---------------------------- File CsrMatrix.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A sparse matrix in compressed sparse rows (CSR), beyond the book: the
 * nonzeros of each row, by column, in one vector of values and one of
 * columns, and the place where each row begins in a vector of \f$ m + 1 \f$
 * offsets, \f$ 12 \, \mathit{nnz} + 8 m \f$ bytes for doubles, read in order
 * by the product with a vector instead of the walk of the orthogonal lists
 * of `SparseMatrix` [see `SparseMatrix.h`], whose nodes are scattered.
 *
 * - The matrix is built from triplets \f$ ( i , j , v ) \f$ in any order by
 *   a counting sort on the rows, a sort of the columns of each row and the
 *   sum of the equal places, \f$ O( \mathit{nnz} + m + \mathrm{sort} ) \f$,
 *   or from a `SparseMatrix`.
 * - `multiply()` is the product \f$ y = A x \f$, by rows: the nonzeros
 *   stream from memory in one pass, the entries of \f$ x \f$ are read by
 *   column: \f$ 2 \, \mathit{nnz} \f$ operations on
 *   \f$ 12 \, \mathit{nnz} + 8 ( m + 1 ) \f$ bytes of the matrix, \f$ 8 m \f$
 *   written and up to \f$ 8 \, \mathit{nnz} \f$ read of \f$ x \f$, so that
 *   the product is bound by the memory, and on the benchmark neither more
 *   accumulators per row nor the gathers of AVX-512 made it faster.
 * - `transpose()` is the CSR of the transpose, i.e., the matrix by columns,
 *   by a counting sort on the columns that keeps the rows sorted,
 *   \f$ O( \mathit{nnz} + m + n ) \f$; a large matrix whose rows span
 *   columns far apart (a sample of blocks of 64 rows decides) goes first
 *   into 256 buckets of consecutive columns and then each bucket into its
 *   slice of the transpose, two passes whose writes stay in few streams and
 *   in the cache instead of one pass of writes scattered over all the
 *   transpose (the partitioned radix sort of the databases):
 *   \f$ 2 \, \mathit{nnz} \f$ moves instead of \f$ \mathit{nnz} \f$, but
 *   \f$ O( \mathit{nnz} / L ) \f$ misses instead of up to
 *   \f$ \mathit{nnz} \f$, for lines of \f$ L \f$ items, when a slice of
 *   \f$ \mathit{nnz} / 256 \f$ items fits the cache.
 *
 * The contracts [see `Contract.h`]: the places of the triplets are in the
 * matrix, and the columns fit 32 bits (the rows too, for `transpose()`);
 * \f$ x \f$ has \f$ n \f$ entries; the invariant is that the offsets do not
 * decrease and the columns of a row increase, an audit. The functions are
 * defined in `CsrMatrix.tpp`.
 *
 * \see Y. Saad, Iterative Methods for Sparse Linear Systems, 2nd ed., SIAM,
 *      2003, section 3.4
 * \see S. Williams, L. Oliker, R. Vuduc, J. Shalf, K. Yelick, J. Demmel,
 *      Optimization of sparse matrix-vector multiplication on emerging
 *      multicore platforms, Parallel Computing 35(3), 178-194, 2009,
 *      doi:10.1016/j.parco.2008.12.006
 * \see O. Polychroniou, K. A. Ross, A comprehensive study of main-memory
 *      partitioning and its application to large-scale comparison- and
 *      radix-sort, Proceedings of the 2014 ACM SIGMOD International
 *      Conference on Management of Data, 755-766, 2014,
 *      doi:10.1145/2588555.2610522
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

#ifndef __ADS_CsrMatrix
 #define __ADS_CsrMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/DenseMatrix.h"
#include "ADS/numeric/SparseMatrix.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS CsrMatrix -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sparse \f$ m \times n \f$ matrix of doubles in compressed sparse rows
class CsrMatrix {
 public:
 /// an entry \f$ ( i , j , v ) \f$
 using Triplet = std::tuple< std::size_t , std::size_t , double >;

 /// the \f$ m \times n \f$ zero matrix
 CsrMatrix( std::size_t m = 0 , std::size_t n = 0 );

 /// the \f$ m \times n \f$ matrix of the triplets \f$ ( i , j , v ) \f$, the
 /// values of equal places summed
 ///
 /// In pseudocode (Saad, section 3.4, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{CsrMatrix}( m , n , T ) \cr
 /// 2 & \quad \mathit{row}[ i + 1 ] \gets \text{the triplets of the row } i
 ///     \text{, for each } i \cr
 /// 3 & \quad \mathit{row} \gets \text{its prefix sums}\quad \triangleright
 ///     \ \text{the counting sort} \cr
 /// 4 & \quad E \gets \text{the pairs } ( j , v ) \text{ of } T \text{ put }
 ///     \text{by row at the offsets} \cr
 /// 5 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
 /// 6 & \qquad \text{sort the pairs of the row } i \text{ by column} \cr
 /// 7 & \qquad \text{append them to } \mathit{col} \text{ and }
 ///     \mathit{val} \text{, a repeated column added to the last} \cr
 /// 8 & \qquad \mathit{row}[ i ] \gets \text{where the row } i \text{ began}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < m \f$ and \f$ j < n \f$ for each triplet, and
 ///      \f$ n \le 2^{32} \f$
 CsrMatrix( std::size_t m , std::size_t n , std::vector< Triplet > T );

 /// the matrix of the orthogonal lists of the book, row by row
 explicit CsrMatrix( const SparseMatrix & S );

 /// the number \f$ m \f$ of rows
 [[nodiscard]] std::size_t rows( void ) const { return( f_m ); }
 /// the number \f$ n \f$ of columns
 [[nodiscard]] std::size_t cols( void ) const { return( f_n ); }
 /// the number \f$ \mathit{nnz} \f$ of the entries kept
 [[nodiscard]] std::size_t nonzeros( void ) const { return( f_val.size() ); }

 /// the entry \f$ ( i , j ) \f$, by a binary search in the row
 /// \pre \f$ i < m \f$ and \f$ j < n \f$
 [[nodiscard]] double get( std::size_t i , std::size_t j ) const;

 /// the product \f$ y = A x \f$, a new vector
 /// \pre \f$ x \f$ has \f$ n \f$ entries
 [[nodiscard]] std::vector< double > multiply(
                                     const std::vector< double > & x ) const;

 /// the product \f$ y = A x \f$ into \f$ y \f$, which must have \f$ m \f$
 /// entries
 ///
 /// In pseudocode (Saad, section 3.4, beyond the book), one pass on the
 /// nonzeros:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{multiply}( x , y ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
 /// 3 & \qquad y_i \gets \sum_{p = \mathit{row}[ i ]}^{\mathit{row}[ i + 1 ]
 ///     - 1} \mathit{val}[ p ] \, x_{\mathit{col}[ p ]}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ has \f$ n \f$ entries and \f$ y \f$ \f$ m \f$ entries,
 ///      apart from \f$ x \f$
 void multiply( const double * x , double * y ) const;

 /// the transpose, \f$ n \times m \f$
 ///
 /// In pseudocode (beyond the book), by a counting sort on the columns,
 /// stable on the rows:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{transpose}() \cr
 /// 2 & \quad \mathit{trow} \gets \text{the prefix sums of the counts of }
 ///     \text{the columns} \cr
 /// 3 & \quad \mathit{at} \gets \mathit{trow} \cr
 /// 4 & \quad \textbf{if}\ \text{the rows span columns far apart}
 ///     \ \textbf{then} \cr
 /// 5 & \qquad \text{scatter the entries, in the order of the rows, into }
 ///     \text{256 buckets of columns} \cr
 /// 6 & \quad \textbf{foreach}\ \text{entry } ( i , j , v ) \text{ by row, }
 ///     \text{or by bucket}\ \textbf{do} \cr
 /// 7 & \qquad q \gets \mathit{at}[ j ];\ \mathit{at}[ j ] \gets q + 1;
 ///     \ \mathit{tcol}[ q ] \gets i;\ \mathit{tval}[ q ] \gets v \cr
 /// 8 & \quad \textbf{return}\ ( \mathit{trow} , \mathit{tcol} ,
 ///     \mathit{tval} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ m \le 2^{32} \f$
 [[nodiscard]] CsrMatrix transpose( void ) const;

 /// the \f$ m + 1 \f$ offsets of the rows
 [[nodiscard]] const std::size_t * rowStart( void ) const {
  return( f_row.data() );
  }
 /// the column of each nonzero, row by row
 [[nodiscard]] const std::uint32_t * columns( void ) const {
  return( f_col.data() );
  }
 /// the value of each nonzero, row by row
 [[nodiscard]] const double * values( void ) const { return( f_val.data() ); }

 /// true if the two matrices have the same sizes and the same entries kept
 bool operator==( const CsrMatrix & ) const = default;

 private:
 /// the offsets not decreasing, the columns of each row increasing
 [[nodiscard]] bool sorted( void ) const;

 template< class U >                  // not zeroed when they grow
 using Vector = std::vector< U , detail::DefaultInitAllocator< U > >;

 std::size_t f_m ,                     ///< the rows
             f_n;                      ///< the columns
 Vector< std::size_t > f_row;          ///< the \f$ m + 1 \f$ offsets
 Vector< std::uint32_t > f_col;        ///< the column of each nonzero
 Vector< double > f_val;               ///< its value
 };  // end( class CsrMatrix )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/CsrMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CsrMatrix.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CsrMatrix.h --------------------------*/
/*--------------------------------------------------------------------------*/
