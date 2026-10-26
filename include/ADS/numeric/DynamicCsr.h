/*--------------------------------------------------------------------------*/
/*---------------------------- File DynamicCsr.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A compressed sparse matrix that grows and shrinks by whole rows (or
 * whole columns), beyond the book: the bundle of the cuts of a
 * decomposition, whose rows come in batches and go away in batches, kept
 * at all times in the arrays beg, ind and val that the solvers of linear
 * programs take, so that handing it over costs nothing.
 *
 * The matrix is three vectors: beg, with one entry more than the rows,
 * beg[ i ] the first entry of the row i and beg[ last ] the number of the
 * entries, and ind and val, the minor index and the value of each entry,
 * the rows one after the other. append() adds a row at the end in time
 * linear in its entries, amortized over the doubling of the vectors;
 * remove() takes a strictly increasing set of rows and closes the gaps in
 * one pass, moving each entry after the first row removed once, in
 * \f$ O( r + z' ) \f$ time for r rows and \f$ z' \f$ entries after the
 * first row removed, with no scratch memory. The same type is the
 * compressed sparse columns, DynamicCsc, when the major index is a
 * column.
 *
 * The contracts [see Contract.h]: a row has as many indices as values,
 * the number of the entries fits the index type I, the rows removed are
 * strictly increasing and below the number of the rows; the invariant is
 * that beg starts at 0, never decreases and ends at the number of the
 * entries. The operations are defined in DynamicCsr.tpp.
 *
 * \see F. G. Gustavson, Two fast algorithms for sparse matrices:
 *      multiplication and permuted transposition, ACM Transactions on
 *      Mathematical Software 4(3), 250-269, 1978, doi:10.1145/355791.355796
 *      (the compressed sparse rows)
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

#ifndef __ADS_DynamicCsr
 #define __ADS_DynamicCsr
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/DenseMatrix.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DynamicCsr ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sparse matrix by rows, with indices of type I and values of type V,
/// whose rows are appended and removed
template< class I = int , class V = double >
class DynamicCsr {

 static_assert( std::is_integral_v< I > );

/*--------------------------------------------------------------------------*/
 public:

 using Index = I;   ///< the type of the indices
 using Value = V;   ///< the type of the values

 /// the matrix of no rows
 DynamicCsr( void ) : f_beg( 1 , I( 0 ) ) {}

 /// the number of the rows
 [[nodiscard]] std::size_t rows( void ) const {
  return( f_beg.size() - 1 );
  }

 /// the number of the entries
 [[nodiscard]] std::size_t nonzeros( void ) const { return( f_ind.size() ); }

 [[nodiscard]] bool empty( void ) const { return( rows() == 0 ); }

 /// room for r rows and z entries
 void reserve( std::size_t r , std::size_t z );

 /// removes all the rows, keeping the memory
 void clear( void );

 /// appends the row of the entries ( ind[ k ] , val[ k ] ); its index
 /// \pre ind and val have the same length
 std::size_t append( std::span< const I > ind , std::span< const V > val );

 /// appends the r rows of a compressed form: the row k has the entries
 /// beg[ k ] - beg[ 0 ] to beg[ k + 1 ] - beg[ 0 ] of ind and val; the
 /// index of the first one
 /// \pre beg is not decreasing
 std::size_t append( std::size_t r , const I * beg , const I * ind ,
                     const V * val );

 /// removes the rows of E (a contiguous range of integers, e.g., a
 /// std::vector), closing the gaps in one pass
 /// \pre E is strictly increasing and below rows()
 template< std::ranges::contiguous_range R >
 void remove( const R & E );

 /// removes the rows from first to last, last excluded
 /// \pre first <= last <= rows()
 void remove( std::size_t first , std::size_t last );

 /// the minor indices of the row i
 /// \pre i < rows()
 [[nodiscard]] std::span< const I > indices( std::size_t i ) const;

 /// the values of the row i
 /// \pre i < rows()
 [[nodiscard]] std::span< const V > values( std::size_t i ) const;

 /// the values of the row i, to change in place
 /// \pre i < rows()
 [[nodiscard]] std::span< V > values( std::size_t i );

 /// the rows() + 1 starts of the rows, the last one the number of the
 /// entries
 [[nodiscard]] const I * beg( void ) const { return( f_beg.data() ); }

 /// the minor index of each entry
 [[nodiscard]] const I * ind( void ) const { return( f_ind.data() ); }

 /// the value of each entry
 [[nodiscard]] const V * val( void ) const { return( f_val.data() ); }

/*--------------------------------------------------------------------------*/
 private:

 template< class U >                  // not zeroed when they grow
 using Vector = std::vector< U , detail::DefaultInitAllocator< U > >;

 /// the invariant
 [[nodiscard]] bool consistent( void ) const;

 Vector< I > f_beg;   ///< the starts of the rows, and the end
 Vector< I > f_ind;   ///< the minor indices
 Vector< V > f_val;   ///< the values

 };  // end( class DynamicCsr )

/// the same by columns
template< class I = int , class V = double >
using DynamicCsc = DynamicCsr< I , V >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/DynamicCsr.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DynamicCsr.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File DynamicCsr.h -------------------------*/
/*--------------------------------------------------------------------------*/
