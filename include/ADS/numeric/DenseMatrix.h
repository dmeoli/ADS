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
 *   processor. The partial tiles at the borders are packed with zeros,
 *   and only their entries inside C are written.
 * - transpose() moves blocks of 32 x 32 entries, which stay in the cache of
 *   first level both read by rows and written by columns, instead of
 *   running through the whole columns of the result.
 * - multiply() is the product A B, a new matrix, by gemm().
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
