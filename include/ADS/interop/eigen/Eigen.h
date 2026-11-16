/*--------------------------------------------------------------------------*/
/*------------------------------ File Eigen.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The matrices of ADS as matrices of Eigen (https://eigen.tuxfamily.org),
 * so that the solvers, the decompositions and the products of Eigen run on
 * them, beyond the book:
 *
 * - `map( A )` of a `DenseMatrix< T >` is an `Eigen::Map` of a row-major
 *   dense matrix over its entries, with no copy, writable if \f$ A \f$ is;
 * - `map( A )` of a `BasicCsrMatrix< T , I , I >` whose columns and offsets
 *   have the same signed type I (e.g., `BasicCsrMatrix< double , int >`)
 *   is an `Eigen::Map` of a row-major `Eigen::SparseMatrix< T ,
 *   Eigen::RowMajor , I >` over its three arrays, with no copy, since these
 *   are the compressed arrays of Eigen (which requires one signed type for
 *   both, its `StorageIndex`);
 * - `toEigen( A )` copies a `BasicCsrMatrix` of any types into an
 *   `Eigen::SparseMatrix< T , Eigen::RowMajor , S >`, the arrays copied
 *   one by one in \f$ O( m + \mathit{nnz} ) \f$ time, which the default
 *   `CsrMatrix` (32-bit unsigned columns and `std::size_t` offsets) needs,
 *   as its layout is not one of Eigen;
 * - `fromEigen( S )` copies a sparse expression of Eigen into a
 *   `BasicCsrMatrix`, through its triplets.
 *
 * Eigen has its own sparse and dense matrices, and ADS does not replace
 * them: a program that keeps its data in the structures of ADS calls Eigen
 * on them through these views, and the copies are only for the layouts
 * that Eigen cannot read.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs the headers of Eigen (3.4 or later), found by CMake when
 * `ADS_WITH_EIGEN` is AUTO or ON (the target `ADS::Eigen`). The contracts
 * [see `Contract.h`]: a map does not outlive its matrix, nor sees a matrix
 * that changes its size; the indices of a copy fit its type.
 *
 * \see G. Guennebaud, B. Jacob, et al., Eigen v3, 2010,
 *      https://eigen.tuxfamily.org
 * \see Y. Saad, Iterative Methods for Sparse Linear Systems, 2nd ed., SIAM,
 *      2003, section 3.4
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

#ifndef __ADS_interop_eigen_Eigen
 #define __ADS_interop_eigen_Eigen
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <Eigen/SparseCore> )
 #error "the interoperation with Eigen needs the headers of Eigen"
#endif

#include "ADS/Contract.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::eigen
{

/*--------------------------------------------------------------------------*/
/*--------------------------------- TYPES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a dense matrix of Eigen by rows, the layout of `DenseMatrix`
template< class T >
using RowMatrix =
  Eigen::Matrix< T , Eigen::Dynamic , Eigen::Dynamic , Eigen::RowMajor >;

/// a sparse matrix of Eigen by rows, of the index type I
template< class T , class I = int >
using RowSparse = Eigen::SparseMatrix< T , Eigen::RowMajor , I >;

/*--------------------------------------------------------------------------*/
/*---------------------------------- VIEWS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the entries of \f$ A \f$ as a dense matrix of Eigen, with no copy
template< class T >
[[nodiscard]] Eigen::Map< RowMatrix< T > > map( DenseMatrix< T > & A )
{
 return( Eigen::Map< RowMatrix< T > >( A.data() , Eigen::Index( A.rows() ) ,
					Eigen::Index( A.cols() ) ) );
 }

/// the entries of a constant \f$ A \f$ as a dense matrix of Eigen, with no
/// copy
template< class T >
[[nodiscard]] Eigen::Map< const RowMatrix< T > >
map( const DenseMatrix< T > & A )
{
 return( Eigen::Map< const RowMatrix< T > >(
   A.data() , Eigen::Index( A.rows() ) , Eigen::Index( A.cols() ) ) );
 }

/// \f$ A \f$ as a sparse matrix of Eigen by rows, its offsets, columns and
/// values read in place: the columns and the offsets of one signed type,
/// the `StorageIndex` of Eigen
template< class T , std::signed_integral I >
[[nodiscard]] Eigen::Map< const RowSparse< T , I > >
map( const BasicCsrMatrix< T , I , I > & A )
{
 return( Eigen::Map< const RowSparse< T , I > >(
   Eigen::Index( A.rows() ) , Eigen::Index( A.cols() ) ,
   Eigen::Index( A.nonzeros() ) , A.rowStart() , A.columns() , A.values() ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- COPIES --------------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ A \f$ copied into a sparse matrix of Eigen by rows of the index
/// type S, in \f$ O( m + \mathit{nnz} ) \f$ time, for the layouts that
/// `map()` does not take
/// \pre the sizes and the nonzeros fit S
template< class S = int , class T , class I , class P >
[[nodiscard]] RowSparse< T , S >
toEigen( const BasicCsrMatrix< T , I , P > & A )
{
 ADS_PRE( ( A.rows() <= std::size_t( std::numeric_limits< S >::max() ) ) &&
	  ( A.cols() <= std::size_t( std::numeric_limits< S >::max() ) ) &&
	  ( A.nonzeros() <= std::size_t( std::numeric_limits< S >::max() ) ) );
 RowSparse< T , S > E( Eigen::Index( A.rows() ) , Eigen::Index( A.cols() ) );
 E.resizeNonZeros( Eigen::Index( A.nonzeros() ) );
 std::transform( A.rowStart() , A.rowStart() + A.rows() + 1 ,
		 E.outerIndexPtr() ,
		 []( P p ) { return( S( p ) ); } );
 std::transform( A.columns() , A.columns() + A.nonzeros() , E.innerIndexPtr() ,
		 []( I j ) { return( S( j ) ); } );
 std::copy( A.values() , A.values() + A.nonzeros() , E.valuePtr() );
 return( E );
 }

/// the sparse expression \f$ S \f$ of Eigen copied into a `BasicCsrMatrix`
/// of the columns I and the offsets P, by its triplets
/// \pre the columns fit I and the nonzeros P
template< class I = std::uint32_t , class P = detail::CsrOffset< I > ,
	  class D >
[[nodiscard]] BasicCsrMatrix< typename D::Scalar , I , P >
fromEigen( const Eigen::SparseMatrixBase< D > & S )
{
 using T = typename D::Scalar;
 const RowSparse< T , typename D::StorageIndex > R( S.derived() );
 std::vector< typename BasicCsrMatrix< T , I , P >::Triplet > E;
 E.reserve( std::size_t( R.nonZeros() ) );
 for( Eigen::Index i = 0 ; i < R.outerSize() ; ++i )
  for( typename RowSparse< T , typename D::StorageIndex >::InnerIterator it(
	 R , i ) ;
       it ; ++it )
   E.emplace_back( std::size_t( it.row() ) , std::size_t( it.col() ) ,
		   it.value() );
 return( BasicCsrMatrix< T , I , P >(
   std::size_t( R.rows() ) , std::size_t( R.cols() ) , std::move( E ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::eigen )

/*--------------------------------------------------------------------------*/

#endif /* Eigen.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Eigen.h ----------------------------*/
/*--------------------------------------------------------------------------*/
