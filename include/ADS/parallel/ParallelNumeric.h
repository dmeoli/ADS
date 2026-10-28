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
 * - `gemm( ex , ... )` runs the loops of the sequential `gemm()` on the
 *   panels of \f$ N_C \f$ columns of \f$ B \f$ and on the slices of
 *   \f$ K_C \f$ of the common dimension, and parallelizes within them as
 *   BLIS does: the panel \f$ K_C \times N_C \f$ of \f$ B \f$ is packed once,
 *   its micro-panels shared among the threads, and each thread takes a range
 *   of the rows of \f$ C \f$ (whole micro-panels of \f$ M_R \f$ rows, as
 *   many to each thread as they go), packs its blocks of \f$ M_C \f$ rows of
 *   \f$ A \f$ in its own buffer and runs the micro-kernels on them and on the
 *   shared panel of \f$ B \f$ (the loop of the blocks of \f$ A \f$ in
 *   parallel, which Smith et al. found the one that scales on the processors
 *   whose cores have private second levels). Each entry of \f$ C \f$ sees the
 *   same slices and the same micro-kernel as in `gemm()`; the small
 *   products, which `gemm()` computes by another path, go to it on one
 *   thread. With the backend of a CBLAS [see `DenseMatrix.h`], the thin
 *   products that go to it give each thread a range of the rows of
 *   \f$ C \f$ instead, as evenly as they go, which it
 *   multiplies by one call of the CBLAS, the panel of \f$ B \f$ packed by
 *   each call; the CBLAS is then expected to run on one thread (e.g.,
 *   OpenBLAS with `OPENBLAS_NUM_THREADS=1`), or its threads multiply those
 *   of the executor.
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
 * \see T. M. Smith, R. A. van de Geijn, M. Smelyanskiy, J. R. Hammond,
 *      F. G. Van Zee, Anatomy of high-performance many-threaded matrix
 *      multiplication, IEEE International Parallel and Distributed
 *      Processing Symposium (IPDPS), 1049-1059, 2014,
 *      doi:10.1109/IPDPS.2014.110
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
/// In pseudocode (the loops of BLIS of Smith et al., beyond the book), with
/// the blocks \f$ K_C = 384 \f$, \f$ M_C \f$ and \f$ N_C \f$ and the
/// micro-panels of \f$ M_R \f$ rows and \f$ N_R \f$ columns of the
/// sequential `gemm()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{gemm}( \mathit{ex} , m , n , k , \alpha ,
///     A , B , \beta , C ) \cr
/// 2 & \quad \textbf{if}\ p = 1\ \textbf{or}\ \text{the product is small or }
///     \text{degenerate, or } m < 2 M_R\ \textbf{then} \cr
/// 3 & \qquad \mathrm{gemm}( m , n , k , \alpha , A , B , \beta , C );
///     \ \textbf{return} \cr
/// 4 & \quad \textbf{if}\ \text{a CBLAS is the backend and the product is}
///     \text{ thin}\ \textbf{then} \cr
/// 5 & \qquad \textbf{for}\ u \gets 0\ \textbf{to}\ \min\{ p , m \} - 1
///     \ \textbf{do in parallel}\ \text{the } u\text{-th range of rows of }
///     C \text{ by the CBLAS} \cr
/// 6 & \qquad \textbf{return} \cr
/// 7 & \quad t \gets \min\{ p , \lceil m / M_R \rceil \}\quad
///     \triangleright\ \text{the threads, a range of micro-panels each} \cr
/// 8 & \quad \textbf{for}\ j_c \gets 0\ \textbf{to}\ n - 1\ \textbf{step}
///     \ N_C\ \textbf{do} \cr
/// 9 & \qquad \textbf{for}\ p_c \gets 0\ \textbf{to}\ k - 1\ \textbf{step}
///     \ K_C\ \textbf{do} \cr
/// 10 & \qquad\quad b \gets \beta\ \textbf{if}\ p_c = 0\ \textbf{else}\ 1 \cr
/// 11 & \qquad\quad \text{pack the panel } B[ p_c .. , j_c .. ] \text{ in }
///     \text{parallel, a range of micro-panels per thread} \cr
/// 12 & \qquad\quad \textbf{for}\ u \gets 0\ \textbf{to}\ t - 1
///     \ \textbf{do in parallel} \cr
/// 13 & \qquad\qquad \textbf{foreach}\ \text{block of } M_C \text{ rows }
///     i_c \text{ in the range of } u\ \textbf{do} \cr
/// 14 & \qquad\qquad\quad \text{pack } A[ i_c .. , p_c .. ] \text{ in the }
///     \text{buffer of the thread} \cr
/// 15 & \qquad\qquad\quad \textbf{foreach}\ \text{micro-panel } j_r
///     \text{ of } B\ \textbf{and}\ i_r \text{ of } A\ \textbf{do} \cr
/// 16 & \qquad\qquad\qquad C[ i_c + i_r .. , j_c + j_r .. ] \gets \alpha \,
///     A_{i_r} B_{j_r} + b \, C[ i_c + i_r .. , j_c + j_r .. ]
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
