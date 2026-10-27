/*--------------------------------------------------------------------------*/
/*--------------------------- File DenseMatrix.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered dense matrices, beyond the book: `DenseMatrix` keeps the
 * \f$ m \times n \f$ entries by rows in one contiguous vector, instead of
 * the vector of rows of `Matrix.h` [see `Matrix.h`], and the product and the
 * transposition run on it at the speed of the processor.
 *
 * - `gemm()` is \f$ C \leftarrow \alpha A B + \beta C \f$ on matrices by
 *   rows with leading dimensions, organized as the GotoBLAS and BLIS
 *   libraries do: the loop on the panels of \f$ N_C \f$ columns of
 *   \f$ B \f$, the one on the slices of \f$ K_C \f$ of the common dimension,
 *   whose panel of \f$ B \f$ is packed in micro-panels of \f$ N_R \f$
 *   columns (\f$ K_C \times N_C \f$ in the last level cache), the one on the
 *   blocks of \f$ M_C \f$ rows of \f$ A \f$, packed in micro-panels of
 *   \f$ M_R \f$ rows (\f$ M_C \times K_C \f$ in the second level), and two
 *   loops of micro-tiles \f$ M_R \times N_R \f$ of \f$ C \f$, each the sum of
 *   \f$ K_C \f$ products of a column of \f$ M_R \f$ entries of \f$ A \f$ by a
 *   row of \f$ N_R \f$ entries of \f$ B \f$, held in registers:
 *   \f$ M_R N_R / w \f$ vector accumulators, \f$ w \f$ the entries of a
 *   vector, each step loading \f$ N_R / w \f$ vectors of \f$ B \f$ and
 *   broadcasting \f$ M_R \f$ entries of \f$ A \f$ into fused products and
 *   sums. The micro-kernel is written with the intrinsics of AVX2 for
 *   `double` (\f$ 6 \times 16 \f$, 24 accumulators of 4) and `float`, and in
 *   portable C++ otherwise, where the compiler vectorizes what it can;
 *   `ADS_GEMM_AVX512` selects a kernel of AVX-512 (\f$ 12 \times 16 \f$, 24
 *   accumulators of 8) for the processors with two units of fused products
 *   of 512 bits, since on those with one (e.g., the Xeon Gold 5120) both
 *   widths make 16 operations a cycle and the vectors of 256 bits run at a
 *   higher clock. \f$ K_C = 384 \f$, \f$ M_C = 480 \f$ and
 *   \f$ N_C = 4096 \f$ came from a search on the same processor. The product
 *   makes \f$ 2 m n k \f$ operations in \f$ \Theta( m n k ) \f$ time; a step
 *   of the micro-kernel makes \f$ M_R N_R \f$ fused operations on
 *   \f$ M_R + N_R \f$ entries loaded from the first level (\f$ 96 / 22 \f$
 *   for \f$ 6 \times 16 \f$), the packing moves
 *   \f$ m k \lceil n / N_C \rceil + k n \f$ entries, a fraction
 *   \f$ O( 1 / M_C + 1 / N_C ) \f$ of the work, and the buffers take
 *   \f$ K_C ( M_C + N_C ) \f$ entries, kept by the thread between the calls.
 *   The partial tiles at the borders are packed with zeros, and only their
 *   entries inside \f$ C \f$ are written. A small product
 *   (\f$ m n k \le 2^{21} \f$) packs \f$ B \f$ alone and reads \f$ A \f$
 *   where it is, the tiles of its last rows and columns by kernels of their
 *   shape.
 * - `transpose()` moves blocks of \f$ 32 \times 32 \f$ entries, which stay
 *   in the cache of first level both read by columns and written by rows,
 *   instead of running through whole columns: \f$ \Theta( m n ) \f$ time and
 *   \f$ O( m n / L ) \f$ misses of lines of \f$ L \f$ entries when a block of
 *   \f$ 32 L \f$ entries fits the cache (the tall cache), the bound of the
 *   cache-oblivious transposition.
 * - `multiply()` is the product \f$ A B \f$, a new matrix, by `gemm()`.
 * - `multiplyStrassen()` is the product by the variant of Winograd of the
 *   algorithm of Strassen [see `strassen()` of `Matrix.h`], on the views of
 *   the quadrants: with \f$ S_1 = A_{21} + A_{22} \f$,
 *   \f$ S_2 = S_1 - A_{11} \f$, \f$ S_3 = A_{11} - A_{21} \f$,
 *   \f$ S_4 = A_{12} - S_2 \f$, \f$ T_1 = B_{12} - B_{11} \f$,
 *   \f$ T_2 = B_{22} - T_1 \f$, \f$ T_3 = B_{22} - B_{12} \f$,
 *   \f$ T_4 = T_2 - B_{21} \f$ and the products \f$ P_1 = A_{11} B_{11} \f$,
 *   \f$ P_2 = A_{12} B_{21} \f$, \f$ P_3 = S_4 B_{22} \f$,
 *   \f$ P_4 = A_{22} T_4 \f$, \f$ P_5 = S_1 T_1 \f$, \f$ P_6 = S_2 T_2 \f$,
 *   \f$ P_7 = S_3 T_3 \f$, the quadrants of \f$ C \f$ are
 *   \f$ C_{11} = P_1 + P_2 \f$, \f$ C_{12} = U + P_5 + P_3 \f$,
 *   \f$ C_{21} = U + P_7 - P_4 \f$, \f$ C_{22} = U + P_7 + P_5 \f$ with
 *   \f$ U = P_1 + P_6 \f$: 7 products and 15 sums of halves,
 *   \f$ T( n ) = 7 T( n / 2 ) + O( n^2 ) \f$, i.e.,
 *   \f$ T( n ) = O( n^{\log_2 7} ) = O( n^{2.81} ) \f$, until a dimension
 *   falls below the cutoff, where `gemm()` takes over; an odd dimension
 *   leaves its last row or column to `gemm()` (dynamic peeling). The bound
 *   of the error grows as
 *   \f$ ( n / n_0 )^{\log_2 18} n_0^2 u \| A \| \| B \| \f$ for the cutoff
 *   \f$ n_0 \f$ and the unit roundoff \f$ u \f$, against the
 *   \f$ n u | A | | B | \f$ of the classical product (Higham), which is the
 *   price of the fewer products: with the cutoff 3072, found on the
 *   benchmark, one level spares \f$ 1 / 8 \f$ of the operations of a product
 *   of 4096; the schedule of the sums needs three temporaries of a quadrant.
 *
 * The contracts [see `Contract.h`]: the sizes of the factors agree, the
 * entries read are inside the matrices; `gemm()` needs leading dimensions at
 * least the columns. The functions are defined in `DenseMatrix.tpp`.
 *
 * \see K. Goto, R. A. van de Geijn, Anatomy of high-performance matrix
 *      multiplication, ACM Transactions on Mathematical Software 34(3),
 *      12:1-25, 2008, doi:10.1145/1356052.1356053
 * \see F. G. Van Zee, R. A. van de Geijn, BLIS: a framework for rapidly
 *      instantiating BLAS functionality, ACM Transactions on Mathematical
 *      Software 41(3), 14:1-33, 2015, doi:10.1145/2764454
 * \see T. M. Low, F. D. Igual, T. M. Smith, E. S. Quintana-Orti, Analytical
 *      modeling is enough for high-performance BLIS, ACM Transactions on
 *      Mathematical Software 43(2), 12:1-18, 2016, doi:10.1145/2925987
 * \see V. Strassen, Gaussian elimination is not optimal, Numerische
 *      Mathematik 13(4), 354-356, 1969, doi:10.1007/BF02165411
 * \see S. Winograd, On multiplication of 2 x 2 matrices, Linear Algebra
 *      and its Applications 4(4), 381-388, 1971,
 *      doi:10.1016/0024-3795(71)90009-7
 * \see N. J. Higham, Exploiting fast matrix multiplication within the
 *      level 3 BLAS, ACM Transactions on Mathematical Software 16(4),
 *      352-368, 1990, doi:10.1145/98267.98290
 * \see J. Huang, T. M. Smith, G. M. Henry, R. A. van de Geijn, Strassen's
 *      algorithm reloaded, SC16: International Conference for High
 *      Performance Computing, Networking, Storage and Analysis, 690-701,
 *      2016, doi:10.1109/SC.2016.58
 * \see M. Frigo, C. E. Leiserson, H. Prokop, S. Ramachandran,
 *      Cache-oblivious algorithms, ACM Transactions on Algorithms 8(1),
 *      4:1-22, 2012, doi:10.1145/2071379.2071383
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

#ifndef __ADS_DenseMatrix
 #define __ADS_DenseMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/Matrix.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#if defined( __AVX512F__ ) || ( defined( __AVX2__ ) && defined( __FMA__ ) )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the allocator that builds the items by default initialization: a number
/// is not zeroed when it is written next
template< class U >
struct DefaultInitAllocator : std::allocator< U > {
 using value_type = U;
 template< class V >
 struct rebind { using other = DefaultInitAllocator< V >; };
 DefaultInitAllocator( void ) = default;
 template< class V >
 DefaultInitAllocator( const DefaultInitAllocator< V > & ) noexcept {}
 template< class V >
 void construct( V * p ) noexcept( std::is_nothrow_default_constructible_v<
                                    V > ) {
  ::new( static_cast< void * >( p ) ) V;
  }
 template< class V , class ... Args >
 void construct( V * p , Args && ... args ) {
  ::new( static_cast< void * >( p ) ) V( std::forward< Args >( args )... );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DenseMatrix ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an \f$ m \times n \f$ matrix by rows in one contiguous vector
template< class T >
class DenseMatrix {
 public:
 using value_type = T;   ///< the type of the entries

 /// the empty \f$ 0 \times 0 \f$ matrix
 DenseMatrix( void ) = default;

 /// the \f$ m \times n \f$ matrix of entries \f$ v \f$
 DenseMatrix( std::size_t m , std::size_t n , const T & v = T( 0 ) )
  : f_m( m ) , f_n( n ) , f_A( m * n ) {
  std::fill( f_A.begin() , f_A.end() , v );
  }

 /// the \f$ m \times n \f$ matrix whose entries are not set, which a
 /// function then writes, if `T` is a number (they are `T()` otherwise)
 static DenseMatrix uninitialized( std::size_t m , std::size_t n ) {
  DenseMatrix M;
  M.f_m = m;
  M.f_n = n;
  M.f_A.resize( m * n );
  return( M );
  }

 /// the matrix of the rows of the `Matrix` of the book
 /// \pre the rows have the same length
 explicit DenseMatrix( const Matrix< T > & A );

 /// the number \f$ m \f$ of rows
 [[nodiscard]] std::size_t rows( void ) const { return( f_m ); }
 /// the number \f$ n \f$ of columns
 [[nodiscard]] std::size_t cols( void ) const { return( f_n ); }

 /// the entry \f$ ( i , j ) \f$, at the place \f$ i n + j \f$
 /// \pre \f$ i < \mathrm{rows}() \f$ and \f$ j < \mathrm{cols}() \f$
 [[nodiscard]] T & operator()( std::size_t i , std::size_t j ) {
  ADS_PRE( ( i < f_m ) && ( j < f_n ) );
  return( f_A[ i * f_n + j ] );
  }
 /// the entry \f$ ( i , j ) \f$ of a constant matrix
 /// \pre \f$ i < \mathrm{rows}() \f$ and \f$ j < \mathrm{cols}() \f$
 [[nodiscard]] const T & operator()( std::size_t i , std::size_t j ) const {
  ADS_PRE( ( i < f_m ) && ( j < f_n ) );
  return( f_A[ i * f_n + j ] );
  }

 /// the entries, by rows
 [[nodiscard]] T * data( void ) { return( f_A.data() ); }
 /// the entries of a constant matrix, by rows
 [[nodiscard]] const T * data( void ) const { return( f_A.data() ); }

 /// the `Matrix` of the book with the same entries
 [[nodiscard]] Matrix< T > toMatrix( void ) const;

 /// true if the two matrices have the same sizes and entries
 bool operator==( const DenseMatrix & ) const = default;

 private:
 std::size_t f_m = 0 ,   ///< the rows
             f_n = 0;    ///< the columns
 std::vector< T , detail::DefaultInitAllocator< T > > f_A;   ///< by rows
 };  // end( class DenseMatrix )

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the product \f$ C \gets \alpha A B + \beta C \f$, \f$ A \f$
/// \f$ m \times k \f$, \f$ B \f$ \f$ k \times n \f$ and \f$ C \f$
/// \f$ m \times n \f$ by rows, with the leading dimensions `lda`, `ldb`,
/// `ldc` (the distance of two rows); \f$ C \f$ is not read when
/// \f$ \beta = 0 \f$
///
/// In pseudocode (Goto and van de Geijn, beyond the book), the blocks fitted
/// to the levels of the cache:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{gemm}( m , n , k , \alpha , A , B ,
///     \beta , C ) \cr
/// 2 & \quad \textbf{if}\ k = 0\ \textbf{or}\ \alpha = 0\ \textbf{then}\ C
///     \gets \beta C;\ \textbf{return} \cr
/// 3 & \quad \textbf{for}\ j_c \gets 0 , N_C , 2 N_C , \ldots < n
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ p_c \gets 0 , K_C , 2 K_C , \ldots < k
///     \ \textbf{do} \cr
/// 5 & \qquad\quad \tilde B \gets \text{the block } K_C \times N_C \text{ of }
///     B \text{, packed by micro-panels of } N_R \cr
/// 6 & \qquad\quad \textbf{for}\ i_c \gets 0 , M_C , 2 M_C , \ldots < m
///     \ \textbf{do} \cr
/// 7 & \qquad\qquad \tilde A \gets \text{the block } M_C \times K_C
///     \text{ of } A \text{, packed by micro-panels of } M_R \cr
/// 8 & \qquad\qquad \textbf{foreach}\ \text{micro-tile } M_R \times N_R
///     \text{ of the block of } C\ \textbf{do} \cr
/// 9 & \qquad\qquad\quad \text{add } \alpha \text{ times its } K_C
///     \text{ rank-one products, in registers} \cr
/// 10 & \qquad\qquad\quad \text{(times } \beta \text{ the old tile on the }
///     \text{first slice, } p_c = 0 \text{)}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{lda} \ge k \f$, \f$ \mathit{ldb} \ge n \f$,
///      \f$ \mathit{ldc} \ge n \f$
template< class T >
void gemm( std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc );

/// the product \f$ A B \f$, a new matrix, by `gemm()` with
/// \f$ \alpha = 1 \f$ and \f$ \beta = 0 \f$
/// \pre `A.cols() == B.rows()`
template< class T >
DenseMatrix< T > multiply( const DenseMatrix< T > & A ,
                           const DenseMatrix< T > & B );

/// the transpose \f$ A^T \f$, by blocks of \f$ 32 \times 32 \f$
///
/// In pseudocode (beyond the book), each block read and written in the first
/// level of the cache:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{transpose}( A ) \cr
/// 2 & \quad \textbf{for}\ j_b \gets 0 , 32 , \ldots < n\ \textbf{do} \cr
/// 3 & \qquad \textbf{for}\ i_b \gets 0 , 32 , \ldots < m\ \textbf{do} \cr
/// 4 & \qquad\quad \textbf{foreach}\ j \in [ j_b , j_b + 32 ) , i \in [ i_b ,
///     i_b + 32 )\ \textbf{do}\ b_{ji} \gets a_{ij} \cr
/// 5 & \quad \textbf{return}\ B
/// \end{array}}
/// \f]
template< class T >
DenseMatrix< T > transpose( const DenseMatrix< T > & A );

/// the product \f$ A B \f$ by the variant of Winograd of the algorithm of
/// Strassen, 7 products of halves and 15 sums a level, down to blocks of
/// fewer than \f$ \mathit{cutoff} \f$ rows, columns or inner terms,
/// multiplied by `gemm()`
///
/// In pseudocode (Strassen and Winograd, beyond the book), \f$ S_i \f$,
/// \f$ T_i \f$, \f$ P_i \f$ and \f$ U \f$ those of the file:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{strassenRec}( A , B ) \cr
/// 2 & \quad \textbf{if}\ \min( m , n , k ) < \mathit{cutoff}
///     \ \textbf{then return}\ \mathrm{gemm}( A , B ) \cr
/// 3 & \quad \text{split the even parts of } A \text{ and } B \text{ in }
///     \text{their quadrants} \cr
/// 4 & \quad P_1 , \ldots , P_7 \gets \text{the products of the sums } S_i
///     \text{ and } T_i \text{, by } \mathrm{strassenRec} \cr
/// 5 & \quad U \gets P_1 + P_6;\ C_{11} \gets P_1 + P_2;\ C_{12} \gets U +
///     P_5 + P_3 \cr
/// 6 & \quad C_{21} \gets U + P_7 - P_4;\ C_{22} \gets U + P_7 + P_5 \cr
/// 7 & \quad \text{add the odd inner term, the odd column and row by }
///     \mathrm{gemm} \cr
/// 8 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre `A.cols() == B.rows()` and \f$ \mathit{cutoff} \ge 2 \f$
template< class T >
DenseMatrix< T > multiplyStrassen( const DenseMatrix< T > & A ,
                                   const DenseMatrix< T > & B ,
                                   std::size_t cutoff = 3072 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/DenseMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DenseMatrix.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File DenseMatrix.h -------------------------*/
/*--------------------------------------------------------------------------*/
