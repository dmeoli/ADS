/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelNumeric.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel products of matrices, beyond the book: the products by a
 * vector of the sparse matrices `CsrMatrix` and `SellMatrix` [see
 * `CsrMatrix.h` and `SellMatrix.h`], and the product of dense matrices of
 * `gemm()` [see `DenseMatrix.h`], on the threads of an executor [see
 * `Executor.h`], with the same results as the sequential ones, bit for bit:
 * each entry of the output is computed by the same sequential code, with the
 * same order of the sums, by one thread.
 *
 * - `multiply( ex , A , x , y )` for a `CsrMatrix` of \f$ m \f$ rows and
 *   \f$ \mathit{nnz} \f$ nonzeros cuts the rows into \f$ B = 4 p \f$ parts
 *   of equal \f$ m + \mathit{nnz} \f$, the cost of a row being its nonzeros
 *   plus the store of its sum (the diagonal of the merge path of Merrill and
 *   Garland: the part \f$ q \f$ ends at the first row \f$ i \f$ with
 *   \f$ i + \mathit{start}( i ) \ge ( q + 1 ) ( m + \mathit{nnz} ) / B \f$,
 *   found by a binary search, since \f$ i + \mathit{start}( i ) \f$ grows
 *   with \f$ i \f$), and runs the product of each part on one thread. A row
 *   is never split among threads, which would change the order of its sums;
 *   a row longer than a part is therefore a part of its own, and only a
 *   matrix with a few rows holding most of the nonzeros is unbalanced.
 * - `multiply( ex , A , x , y )` for a `SellMatrix` cuts its chunks of
 *   \f$ C \f$ rows in the same way by their places, padding included.
 * - `gemm( ex , ... )` with the backend of a CBLAS [see `DenseMatrix.h`]
 *   is one call of the CBLAS from the calling thread, as the sequential
 *   `gemm()`: the CBLAS runs on its own threads (e.g., those of
 *   `OPENBLAS_NUM_THREADS` for OpenBLAS), and the executor runs nothing,
 *   since the threads of both would multiply each other; the result is that
 *   of the sequential `gemm()`, bit for bit, as it is the same call. Without
 *   it, each of \f$ t = \min\{ p , m \} \f$ threads takes a range of
 *   the rows of \f$ C \f$, as evenly as they go, and computes them by the
 *   loops of the sequential `gemm()`, whose sums of each entry run in the
 *   order of the common index on every split of the rows.
 *
 * The products by a vector move \f$ 12 \mathit{nnz} + 8 ( m + n ) \f$ bytes
 * and are bound by the bandwidth of the memory, so that \f$ p \f$ threads
 * gain at most the ratio of the bandwidth of the socket to that of one core.
 *
 * The contracts [see `Contract.h`]: those of the sequential products. The
 * functions are defined in `ParallelNumeric.tpp`.
 *
 * \see D. Merrill, M. Garland, Merge-based parallel sparse matrix-vector
 *      multiplication, International Conference for High Performance
 *      Computing, Networking, Storage and Analysis (SC), 678-689, 2016,
 *      doi:10.1109/SC.2016.57
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

#ifndef __ADS_ParallelNumeric
 #define __ADS_ParallelNumeric
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"
#include "ADS/numeric/SellMatrix.h"
#include "ADS/parallel/Executor.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the product \f$ y = A x \f$ of the sparse matrix \f$ A \f$ by rows on
/// the threads of `ex`
///
/// In pseudocode (the partition of the merge path of Merrill and Garland,
/// beyond the book), \f$ \mathit{start}[ 0 .. m ] \f$ the first nonzero of
/// each row, \f$ \mathit{nnz} = \mathit{start}[ m ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{multiply}( \mathit{ex} , A , x , y ) \cr
/// 2 & \quad \textbf{if}\ p = 1\ \textbf{or}\ m + \mathit{nnz} < 2^{15}
///     \ \textbf{then}\ \text{the sequential product};\ \textbf{return} \cr
/// 3 & \quad B \gets \min\{ 4 p , m \} \cr
/// 4 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ B - 1
///     \ \textbf{do in parallel} \cr
/// 5 & \qquad r_0 \gets \text{the first } i \text{ with } i +
///     \mathit{start}[ i ] \ge \lfloor q ( m + \mathit{nnz} ) / B \rfloor
///     \quad \triangleright\ \text{by a binary search} \cr
/// 6 & \qquad r_1 \gets \text{the first } i \text{ with } i +
///     \mathit{start}[ i ] \ge \lfloor ( q + 1 ) ( m + \mathit{nnz} ) / B
///     \rfloor \cr
/// 7 & \qquad \textbf{for}\ i \gets r_0\ \textbf{to}\ r_1 - 1\ \textbf{do}
///     \ y[ i ] \gets \sum_{k = \mathit{start}[ i ]}^{\mathit{start}[ i + 1 ]
///     - 1} \mathit{val}[ k ] \, x[ \mathit{col}[ k ] ]
/// \end{array}}
/// \f]
/// \pre \f$ x \f$ has `A.cols()` entries and \f$ y \f$ `A.rows()`, apart
///      from \f$ x \f$
template< Executor E >
void multiply( E & ex , const CsrMatrix & A , const double * x , double * y );

/// the product \f$ A x \f$ of the sparse matrix \f$ A \f$ by rows on the
/// threads of `ex`, as a new vector
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiply}( \mathit{ex} , A , x ) \cr
/// 2 & \quad y \gets \text{a vector of } m \text{ entries};
///     \ \mathrm{multiply}( \mathit{ex} , A , x , y );\ \textbf{return}\ y
/// \end{array}}
/// \f]
/// \pre \f$ x \f$ has `A.cols()` entries
template< Executor E >
[[nodiscard]] std::vector< double > multiply( E & ex , const CsrMatrix & A ,
                                            const std::vector< double > & x );

/// the product \f$ y = A x \f$ of the sparse matrix \f$ A \f$ by chunks of
/// \f$ C \f$ rows on the threads of `ex`
///
/// In pseudocode (beyond the book), the chunks cut as the rows of the
/// product of a `CsrMatrix` above, \f$ \mathit{start}[ c ] \f$ the first
/// place of the chunk \f$ c \f$ and \f$ \mathit{pl} \f$ the places of the
/// \f$ \mathit{ch} \f$ chunks:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{multiply}( \mathit{ex} , A , x , y ) \cr
/// 2 & \quad \textbf{if}\ p = 1\ \textbf{or}\ \mathit{pl} + \mathit{ch} <
///     2^{15}\ \textbf{then}\ \text{the sequential product};
///     \ \textbf{return} \cr
/// 3 & \quad B \gets \min\{ 4 p , \mathit{ch} \} \cr
/// 4 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ B - 1
///     \ \textbf{do in parallel} \cr
/// 5 & \qquad c_0 \gets \text{the first } c \text{ with } c +
///     \mathit{start}[ c ] \ge \lfloor q ( \mathit{pl} + \mathit{ch} ) / B
///     \rfloor \cr
/// 6 & \qquad c_1 \gets \text{the first } c \text{ with } c +
///     \mathit{start}[ c ] \ge \lfloor ( q + 1 ) ( \mathit{pl} + \mathit{ch} )
///     / B \rfloor \cr
/// 7 & \qquad \text{the sequential product of the chunks } c_0 , \ldots ,
///     c_1 - 1
/// \end{array}}
/// \f]
/// \pre \f$ x \f$ has `A.cols()` entries and \f$ y \f$ `A.rows()`, apart
///      from \f$ x \f$
template< Executor E , std::size_t C >
void multiply( E & ex , const SellMatrix< C > & A , const double * x ,
               double * y );

/// the product \f$ A x \f$ of the sparse matrix \f$ A \f$ by chunks on the
/// threads of `ex`, as a new vector
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiply}( \mathit{ex} , A , x ) \cr
/// 2 & \quad y \gets \text{a vector of } m \text{ entries};
///     \ \mathrm{multiply}( \mathit{ex} , A , x , y );\ \textbf{return}\ y
/// \end{array}}
/// \f]
/// \pre \f$ x \f$ has `A.cols()` entries
template< Executor E , std::size_t C >
[[nodiscard]] std::vector< double > multiply( E & ex ,
                                            const SellMatrix< C > & A ,
                                            const std::vector< double > & x );

/// \f$ C \gets \alpha A B + \beta C \f$ on the threads of `ex`, for
/// \f$ A \f$ of \f$ m \times k \f$, \f$ B \f$ of \f$ k \times n \f$ and
/// \f$ C \f$ of \f$ m \times n \f$ by rows, as `gemm()`
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{gemm}( \mathit{ex} , m , n , k , \alpha ,
///     A , B , \beta , C ) \cr
/// 2 & \quad \textbf{if}\ \text{a CBLAS is the backend}\ \textbf{then}
///     \ C \gets \alpha A B + \beta C \text{ by one call};\ \textbf{return}
///     \cr
/// 3 & \quad t \gets \min\{ p , m \} \cr
/// 4 & \quad \textbf{for}\ u \gets 0\ \textbf{to}\ t - 1\ \textbf{do in}
///     \ \textbf{parallel} \cr
/// 5 & \qquad \mathrm{gemm}( \text{the rows } [ m u / t , m ( u + 1 ) / t )
///     \text{ of } A \text{ and } C )
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{lda} \ge k \f$, \f$ \mathit{ldb} \ge n \f$,
///      \f$ \mathit{ldc} \ge n \f$
template< Executor E , class T >
void gemm( E & ex , std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc );

/// the product \f$ A B \f$ of two dense matrices on the threads of `ex`
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiply}( \mathit{ex} , A , B ) \cr
/// 2 & \quad C \gets \text{a matrix of } m \times n;\ \mathrm{gemm}(
///     \mathit{ex} , m , n , k , 1 , A , B , 0 , C );\ \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre `A.cols() == B.rows()`
template< Executor E , class T >
[[nodiscard]] DenseMatrix< T > multiply( E & ex , const DenseMatrix< T > & A ,
                                         const DenseMatrix< T > & B );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelNumeric.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelNumeric.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ParallelNumeric.h -----------------------*/
/*--------------------------------------------------------------------------*/
