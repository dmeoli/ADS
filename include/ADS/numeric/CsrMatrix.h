/*--------------------------------------------------------------------------*/
/*---------------------------- File CsrMatrix.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A sparse matrix in compressed sparse rows (CSR), beyond the book: the
 * nonzeros of each row, by column, in one vector of values and one of
 * columns, and the place where each row begins in a vector of m + 1 offsets,
 * \f$ 12\, nnz + 8\, m \f$ bytes for doubles, read in order by the product
 * with a vector instead of the walk of the orthogonal lists of
 * SparseMatrix [see SparseMatrix.h], whose nodes are scattered.
 *
 * - The matrix is built from triplets ( i , j , v ) in any order by a
 *   counting sort on the rows, a sort of the columns of each row and the
 *   sum of the equal places, O( nnz + m + sort ), or from a SparseMatrix.
 * - multiply() is the product y = A x, by rows: the nonzeros stream from
 *   memory in one pass, the entries of x are read by column; the product
 *   is bound by the memory, and on the benchmark neither more
 *   accumulators per row nor the gathers of AVX-512 made it faster.
 * - transpose() is the CSR of the transpose, i.e., the matrix by columns,
 *   by a counting sort on the columns that keeps the rows sorted, O( nnz +
 *   m + n ); a large matrix whose rows span columns far apart (a sample
 *   of blocks of 64 rows decides) goes first into 256 buckets of
 *   consecutive columns and then each bucket into its slice of the
 *   transpose, two passes whose writes stay in few streams and in the
 *   cache instead of one pass of writes scattered over all the transpose
 *   (the partitioned radix sort of the databases).
 *
 * The contracts [see Contract.h]: the places of the triplets are in the
 * matrix, and the columns fit 32 bits (the rows too, for transpose()); x
 * has n entries; the invariant is that the offsets do not decrease and the
 * columns of a row increase, an audit. The functions are defined in
 * CsrMatrix.tpp.
 *
 * \see Y. Saad, Iterative Methods for Sparse Linear Systems, 2nd ed., SIAM,
 *      2003, section 3.4
 * \see S. Williams, L. Oliker, R. Vuduc, J. Shalf, K. Yelick, J. Demmel,
 *      Optimization of sparse matrix-vector multiplication on emerging
 *      multicore platforms, Parallel Computing 35(3), 178-194, 2009,
 *      doi:10.1016/j.parco.2008.12.006
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

/// a sparse m x n matrix of doubles in compressed sparse rows
class CsrMatrix {
 public:
 using Triplet = std::tuple< std::size_t , std::size_t , double >;

 /// the m x n zero matrix
 CsrMatrix( std::size_t m = 0 , std::size_t n = 0 );

 /// the m x n matrix of the triplets ( i , j , v ), the values of equal
 /// places summed
 /// \pre i < m and j < n for each triplet, n < 2^32
 CsrMatrix( std::size_t m , std::size_t n , std::vector< Triplet > T );

 /// the matrix of the book's orthogonal lists
 explicit CsrMatrix( const SparseMatrix & S );

 [[nodiscard]] std::size_t rows( void ) const { return( f_m ); }
 [[nodiscard]] std::size_t cols( void ) const { return( f_n ); }
 [[nodiscard]] std::size_t nonzeros( void ) const { return( f_val.size() ); }

 /// the entry ( i , j ), by a binary search in the row
 /// \pre i < m and j < n
 [[nodiscard]] double get( std::size_t i , std::size_t j ) const;

 /// y = A x
 /// \pre x has n entries
 [[nodiscard]] std::vector< double > multiply(
                                     const std::vector< double > & x ) const;

 /// y = A x into y, which must have m entries
 /// \pre x has n entries and y m entries, apart from x
 void multiply( const double * x , double * y ) const;

 /// the transpose, n x m
 [[nodiscard]] CsrMatrix transpose( void ) const;

 /// the offsets of the rows, the columns and the values
 [[nodiscard]] const std::size_t * rowStart( void ) const {
  return( f_row.data() );
  }
 [[nodiscard]] const std::uint32_t * columns( void ) const {
  return( f_col.data() );
  }
 [[nodiscard]] const double * values( void ) const { return( f_val.data() ); }

 bool operator==( const CsrMatrix & ) const = default;

 private:
 /// the offsets not decreasing, the columns of each row increasing
 [[nodiscard]] bool sorted( void ) const;

 template< class U >                  // not zeroed when they grow
 using Vector = std::vector< U , detail::DefaultInitAllocator< U > >;

 std::size_t f_m , f_n;
 Vector< std::size_t > f_row;          ///< m + 1 offsets
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
