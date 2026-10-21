/*--------------------------------------------------------------------------*/
/*---------------------------- File SellMatrix.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A sparse matrix in the format SELL-C-sigma of Kreutzer, Hager, Wellein,
 * Fehske and Bishop, beyond the book: the rows, sorted by their number of
 * nonzeros within windows of sigma consecutive rows, go in chunks of C
 * rows, and each chunk keeps its nonzeros by columns of the chunk, the
 * k-th nonzero of its C rows side by side, padded with zeros to the
 * longest of its rows. The product y = A x then runs the C rows of a chunk
 * in the C lanes of the vector registers (C = 8 doubles for AVX-512, 4 for
 * AVX2), the values and the columns of a step loaded in one stream and x
 * gathered, instead of one row at a time with a reduction at its end, as
 * in CSR [see CsrMatrix.h]. The sort within the windows makes the rows of
 * a chunk nearly as long, so that the padding, \f$ \sum_c C \max_{i \in
 * c} r_i - nnz \f$ for the chunks c and the row lengths \f$ r_i \f$, is
 * small (none on the rows of equal length of a stencil), while a window
 * of \f$ \sigma \ll m \f$ rows keeps the rows near their place and so the
 * reads of x and the writes of y near each other.
 *
 * - The matrix is built from a CsrMatrix in O( nnz + m \log \sigma )
 *   time: the sort of each window by row length (stable, so that rows of
 *   equal length keep their order), the widths of the chunks, and the
 *   placement of the nonzeros; it takes \f$ 12 ( nnz + pad ) + 8 m / C
 *   + 4 m \f$ bytes: the values and the columns, the offset and the width
 *   of each chunk, and the permutation of the rows.
 * - multiply() is y = A x: for each chunk, C sums in the lanes over the
 *   width of the chunk, then each sum written to its row; \f$ 2 ( nnz +
 *   pad ) \f$ operations, all of them in full vectors.
 *
 * The contracts [see Contract.h]: sigma >= 1 and m < 2^32; x has n
 * entries and y m entries. The
 * functions are defined in SellMatrix.tpp.
 *
 * \see M. Kreutzer, G. Hager, G. Wellein, H. Fehske, A. R. Bishop, A
 *      unified sparse matrix data format for efficient general sparse
 *      matrix-vector multiplication on modern processors with wide SIMD
 *      units, SIAM Journal on Scientific Computing 36(5), C401-C423, 2014,
 *      doi:10.1137/130930352
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

/// a sparse matrix of doubles in SELL-C-sigma, chunks of C rows (4 by
/// default, the fastest on a Xeon Gold 5120 with AVX-512: the gathers of 8
/// lanes cost more than they save)
template< std::size_t C = 4 >
class SellMatrix {
 static_assert( ( C >= 1 ) && ( ( C & ( C - 1 ) ) == 0 ) ,
                "the rows of a chunk are a power of 2" );
 public:

 /// the matrix A, its rows sorted by length within windows of sigma rows
 /// \pre sigma >= 1 and m < 2^32
 explicit SellMatrix( const CsrMatrix & A , std::size_t sigma = 256 );

 [[nodiscard]] std::size_t rows( void ) const { return( f_m ); }
 [[nodiscard]] std::size_t cols( void ) const { return( f_n ); }

 /// the nonzeros of A, and the places with the padding
 [[nodiscard]] std::size_t nonzeros( void ) const { return( f_nnz ); }
 [[nodiscard]] std::size_t places( void ) const { return( f_val.size() ); }

 /// y = A x into y, which must have m entries
 /// \pre x has n entries and y m entries, apart from x
 void multiply( const double * x , double * y ) const;

 /// y = A x
 /// \pre x has n entries
 [[nodiscard]] std::vector< double > multiply(
                                     const std::vector< double > & x ) const;

 private:
 std::size_t f_m , f_n , f_nnz;
 std::vector< std::size_t > f_start;       ///< chunks + 1 offsets of places
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
