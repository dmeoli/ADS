/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelNumeric.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel products of matrices, beyond the book: the products by a
 * vector of the sparse matrices CsrMatrix and SellMatrix [see CsrMatrix.h
 * and SellMatrix.h], and the product of dense matrices of gemm() [see
 * DenseMatrix.h], on the threads of an executor [see Executor.h], with
 * the same results as the sequential ones, bit for bit: each entry of the
 * output is computed by the same sequential code, with the same order of
 * the sums, by one thread.
 * - multiply( ex , A , x , y ) for a CsrMatrix cuts the rows into 4 p
 *   parts of equal \f$ rows + nonzeros \f$, the cost of a row being its
 *   nonzeros plus the store of its sum (the diagonal of the merge path of
 *   Merrill and Garland: the part q ends at the first row i with
 *   \f$ i + \mathit{start}( i ) \geq q ( m + nnz ) / B \f$, found by a
 *   binary search, since \f$ i + \mathit{start}( i ) \f$ grows with i),
 *   and runs the product of each part on one thread. A row is never split
 *   among threads, which would change the order of its sums; a row longer
 *   than a part is therefore a part of its own, and only a matrix with a
 *   few rows holding most of the nonzeros is unbalanced.
 * - multiply( ex , A , x , y ) for a SellMatrix cuts its chunks of C rows
 *   in the same way by their places, padding included.
 * - gemm( ex , ... ) runs the loops of the sequential gemm() on the
 *   panels of NC columns of B and on the slices of KC of the common
 *   dimension, and parallelizes within them as BLIS does: the panel
 *   \f$ K_C \times N_C \f$ of B is packed once, its micro-panels shared
 *   among the threads, and each thread takes a range of the rows of C
 *   (whole micro-panels of MR rows, as many to each thread as they go),
 *   packs its blocks of MC rows of A in its own buffer and runs the
 *   micro-kernels on them and on the shared panel of B (the loop of the
 *   blocks of A in parallel, which Smith et al. found the one that scales
 *   on the processors whose cores have private second levels). Each entry
 *   of C sees the same slices and the same micro-kernel as in gemm(); the
 *   small products, which gemm() computes by another path, go to it on one
 *   thread.
 * The products by a vector move \f$ 12 nnz + 8 ( m + n ) \f$ bytes and are
 * bound by the bandwidth of the memory, so that p threads gain at most the
 * ratio of the bandwidth of the socket to that of one core.
 *
 * The contracts [see Contract.h]: those of the sequential products.
 * \see D. Merrill, M. Garland, Merge-based parallel sparse matrix-vector
 *      multiplication, International Conference for High Performance
 *      Computing, Networking, Storage and Analysis (SC), 678-689, 2016,
 *      doi:10.1109/SC.2016.57
 * \see T. M. Smith, R. A. van de Geijn, M. Smelyanskiy, J. R. Hammond,
 *      F. G. Van Zee, Anatomy of high-performance many-threaded matrix
 *      multiplication, IEEE International Parallel and Distributed
 *      Processing Symposium (IPDPS), 1049-1059, 2014,
 *      doi:10.1109/IPDPS.2014.110
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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

/// y = A x on the threads of ex
/// \pre x has A.cols() entries and y A.rows(), apart from x
template< Executor E >
void multiply( E & ex , const CsrMatrix & A , const double * x , double * y );

/// A x on the threads of ex
/// \pre x has A.cols() entries
template< Executor E >
[[nodiscard]] std::vector< double > multiply( E & ex , const CsrMatrix & A ,
                                            const std::vector< double > & x );

/// y = A x on the threads of ex
/// \pre x has A.cols() entries and y A.rows(), apart from x
template< Executor E , std::size_t C >
void multiply( E & ex , const SellMatrix< C > & A , const double * x ,
               double * y );

/// A x on the threads of ex
/// \pre x has A.cols() entries
template< Executor E , std::size_t C >
[[nodiscard]] std::vector< double > multiply( E & ex ,
                                            const SellMatrix< C > & A ,
                                            const std::vector< double > & x );

/// C <- alpha A B + beta C on the threads of ex, as gemm()
/// \pre lda >= k , ldb >= n , ldc >= n
template< Executor E , class T >
void gemm( E & ex , std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc );

/// the product A B on the threads of ex
/// \pre A.cols() == B.rows()
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
