/*--------------------------------------------------------------------------*/
/*--------------------------- File DenseMatrix.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered dense matrices, beyond the book: DenseMatrix keeps the
 * m x n entries by rows in one contiguous vector, instead of the vector of
 * rows of Matrix.h [see Matrix.h], and the product, the transposition and
 * the solution of linear systems run on it at the speed of the processor.
 *
 * - gemm() is \f$ C \leftarrow \alpha A B + \beta C \f$ on matrices by rows
 *   with leading dimensions, organized as the GotoBLAS and BLIS libraries
 *   do: the loop on the panels of NC columns of B, the one on the slices of
 *   KC of the common dimension, whose panel of B is packed in
 *   micro-panels of NR columns (KC x NC in the last level cache), the one
 *   on the blocks of MC rows of A, packed in micro-panels of MR rows
 *   (MC x KC in the second level), and two loops of micro-tiles MR x NR
 *   of C, each the sum of KC products of a column of MR entries of A by a
 *   row of NR entries of B, held in registers: MR NR / w vector
 *   accumulators, w the entries of a vector, each step loading NR / w
 *   vectors of B and broadcasting MR entries of A into fused products and
 *   sums. The micro-kernel is written with the intrinsics of AVX2 for
 *   double (6 x 16, 24 accumulators of 4) and float, and in portable C++
 *   otherwise, where the compiler vectorizes what it can; ADS_GEMM_AVX512
 *   selects a kernel of AVX-512 (12 x 16, 24 accumulators of 8) for the
 *   processors with two units of fused products of 512 bits, since on
 *   those with one (e.g., the Xeon Gold 5120) both widths make 16
 *   operations a cycle and the vectors of 256 bits run at a higher clock.
 *   KC = 384, MC = 480 and NC = 4096 came from a search on the same
 *   processor. The product makes \f$ 2 m n k \f$ operations in
 *   \f$ \Theta( m n k ) \f$ time; a step of the micro-kernel makes
 *   \f$ M_R N_R \f$ fused operations on \f$ M_R + N_R \f$ entries loaded
 *   from the first level (\f$ 96 / 22 \f$ for 6 x 16), the packing moves
 *   \f$ m k \lceil n / N_C \rceil + k n \f$ entries, a fraction
 *   \f$ O( 1 / M_C + 1 / N_C ) \f$ of the work, and the buffers take
 *   \f$ K_C ( M_C + N_C ) \f$ entries, kept by the thread between the
 *   calls. The partial tiles at the borders are packed with zeros, and
 *   only their entries inside C are written. A small product (\f$ m n k
 *   \le 2^{21} \f$) packs B alone and reads A where it is, the tiles of
 *   its last rows and columns by kernels of their shape.
 * - transpose() moves blocks of 32 x 32 entries, which stay in the cache of
 *   first level both read by columns and written by rows, instead of
 *   running through whole columns: \f$ \Theta( m n ) \f$ time and
 *   \f$ O( m n / L ) \f$ misses of lines of L entries when a block of
 *   \f$ 32 L \f$ entries fits the cache (the tall cache), the bound of the
 *   cache-oblivious transposition.
 * - multiply() is the product A B, a new matrix, by gemm().
 * - multiplyStrassen() is the product by the variant of Winograd of the
 *   algorithm of Strassen [see strassen() of Matrix.h], on the views of
 *   the quadrants: with \f$ S_1 = A_{21} + A_{22} \f$, \f$ S_2 = S_1 -
 *   A_{11} \f$, \f$ S_3 = A_{11} - A_{21} \f$, \f$ S_4 = A_{12} - S_2 \f$,
 *   \f$ T_1 = B_{12} - B_{11} \f$, \f$ T_2 = B_{22} - T_1 \f$, \f$ T_3 =
 *   B_{22} - B_{12} \f$, \f$ T_4 = T_2 - B_{21} \f$ and the products \f$ P_1
 *   = A_{11} B_{11} \f$, \f$ P_2 = A_{12} B_{21} \f$, \f$ P_3 = S_4 B_{22}
 *   \f$, \f$ P_4 = A_{22} T_4 \f$, \f$ P_5 = S_1 T_1 \f$, \f$ P_6 = S_2 T_2
 *   \f$, \f$ P_7 = S_3 T_3 \f$, the quadrants of C are \f$ C_{11} = P_1 +
 *   P_2 \f$, \f$ C_{12} = U + P_5 + P_3 \f$, \f$ C_{21} = U + P_7 - P_4 \f$,
 *   \f$ C_{22} = U + P_7 + P_5 \f$ with \f$ U = P_1 + P_6 \f$: 7 products
 *   and 15 sums of halves, \f$ T( n ) = 7 T( n / 2 ) + O( n^2 ) =
 *   O( n^{\log_2 7} ) = O( n^{2.81} ) \f$, until a dimension falls below
 *   the cutoff, where gemm() takes over; an odd dimension leaves its last
 *   row or column to gemm() (dynamic peeling). The bound of the error
 *   grows as \f$ ( n / n_0 )^{\log_2 18} n_0^2 u \| A \| \| B \| \f$ for
 *   the cutoff \f$ n_0 \f$ and the unit roundoff u, against the \f$ n u
 *   | A | | B | \f$ of the classical product (Higham), which is the price
 *   of the fewer products: with the cutoff 3072, found on the benchmark,
 *   one level spares 1/8 of the operations of a product of 4096; the
 *   schedule of the sums needs three temporaries of a quadrant.
 *
 * The contracts [see Contract.h]: the sizes of the factors agree, the
 * entries read are inside the matrices; gemm() needs leading dimensions at
 * least the columns. The functions are defined in DenseMatrix.tpp.
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

/// an m x n matrix by rows in one contiguous vector
template< class T >
class DenseMatrix {
 public:
 using value_type = T;

 DenseMatrix( void ) = default;

 /// the m x n matrix of entries v
 DenseMatrix( std::size_t m , std::size_t n , const T & v = T( 0 ) )
  : f_m( m ) , f_n( n ) , f_A( m * n ) {
  std::fill( f_A.begin() , f_A.end() , v );
  }

 /// the m x n matrix whose entries are not set, which a function then
 /// writes, if T is a number (they are T() otherwise)
 static DenseMatrix uninitialized( std::size_t m , std::size_t n ) {
  DenseMatrix M;
  M.f_m = m;
  M.f_n = n;
  M.f_A.resize( m * n );
  return( M );
  }

 /// the matrix of the rows of the book's Matrix
 /// \pre the rows have the same length
 explicit DenseMatrix( const Matrix< T > & A );

 [[nodiscard]] std::size_t rows( void ) const { return( f_m ); }
 [[nodiscard]] std::size_t cols( void ) const { return( f_n ); }

 /// the entry ( i , j )
 /// \pre i < rows() and j < cols()
 [[nodiscard]] T & operator()( std::size_t i , std::size_t j ) {
  ADS_PRE( ( i < f_m ) && ( j < f_n ) );
  return( f_A[ i * f_n + j ] );
  }
 [[nodiscard]] const T & operator()( std::size_t i , std::size_t j ) const {
  ADS_PRE( ( i < f_m ) && ( j < f_n ) );
  return( f_A[ i * f_n + j ] );
  }

 /// the entries, by rows
 [[nodiscard]] T * data( void ) { return( f_A.data() ); }
 [[nodiscard]] const T * data( void ) const { return( f_A.data() ); }

 /// the book's Matrix of the same entries
 [[nodiscard]] Matrix< T > toMatrix( void ) const;

 bool operator==( const DenseMatrix & ) const = default;

 private:
 std::size_t f_m = 0 , f_n = 0;
 std::vector< T , detail::DefaultInitAllocator< T > > f_A;   ///< by rows
 };  // end( class DenseMatrix )

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// C <- alpha A B + beta C, A m x k, B k x n and C m x n by rows, with the
/// leading dimensions lda , ldb , ldc (the distance of two rows); C is not
/// read when beta = 0
/// \pre lda >= k , ldb >= n , ldc >= n
template< class T >
void gemm( std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc );

/// the product A B
/// \pre A.cols() == B.rows()
template< class T >
DenseMatrix< T > multiply( const DenseMatrix< T > & A ,
                           const DenseMatrix< T > & B );

/// the transpose of A
template< class T >
DenseMatrix< T > transpose( const DenseMatrix< T > & A );

/// the product A B by the variant of Winograd of the algorithm of
/// Strassen, 7 products of halves and 15 sums a level, down to blocks of
/// fewer than cutoff rows, columns or inner terms, multiplied by gemm()
/// \pre A.cols() == B.rows() and cutoff >= 2
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
