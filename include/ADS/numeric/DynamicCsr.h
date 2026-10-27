/*--------------------------------------------------------------------------*/
/*---------------------------- File DynamicCsr.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A compressed sparse matrix that grows and shrinks by whole rows (or whole
 * columns), beyond the book: the bundle of the cuts of a decomposition,
 * whose rows come in batches and go away in batches, kept at all times in
 * the arrays \f$ \mathit{beg} \f$, \f$ \mathit{ind} \f$ and
 * \f$ \mathit{val} \f$ that the solvers of linear programs take, so that
 * handing it over costs nothing.
 *
 * The matrix is three vectors: \f$ \mathit{beg} \f$, with one entry more
 * than the rows, \f$ \mathit{beg}[ i ] \f$ the first entry of the row
 * \f$ i \f$ and its last entry the number of the entries, and
 * \f$ \mathit{ind} \f$ and \f$ \mathit{val} \f$, the minor index and the
 * value of each entry, the rows one after the other. `append()` adds a row
 * at the end in time linear in its entries, amortized over the doubling of
 * the vectors; `remove()` takes a strictly increasing set of rows and closes
 * the gaps in one pass, moving each entry after the first row removed once,
 * in \f$ O( r + z' ) \f$ time for \f$ r \f$ rows and \f$ z' \f$ entries after
 * the first row removed, with no scratch memory. The same type is the
 * compressed sparse columns, `DynamicCsc`, when the major index is a column.
 *
 * The contracts [see `Contract.h`]: a row has as many indices as values,
 * the number of the entries fits the index type `I`, the rows removed are
 * strictly increasing and below the number of the rows; the invariant is
 * that \f$ \mathit{beg} \f$ starts at 0, never decreases and ends at the
 * number of the entries. The operations are defined in `DynamicCsr.tpp`.
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

/// a sparse matrix by rows, with indices of type `I` and values of type
/// `V`, whose rows are appended and removed
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

 /// true if the matrix has no row
 [[nodiscard]] bool empty( void ) const { return( rows() == 0 ); }

 /// room for \f$ r \f$ rows and \f$ z \f$ entries
 void reserve( std::size_t r , std::size_t z );

 /// removes all the rows, keeping the memory
 void clear( void );

 /// appends the row of the entries
 /// \f$ ( \mathit{ind}[ k ] , \mathit{val}[ k ] ) \f$; its index
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{append}( \mathit{ind}' , \mathit{val}' )
 ///     \cr
 /// 2 & \quad \text{append } \mathit{ind}' \text{ to } \mathit{ind}
 ///     \text{ and } \mathit{val}' \text{ to } \mathit{val} \cr
 /// 3 & \quad \text{append } |\mathit{ind}| \text{ to } \mathit{beg} \cr
 /// 4 & \quad \textbf{return}\ \mathrm{rows}() - 1
 /// \end{array}}
 /// \f]
 /// \pre `ind` and `val` have the same length
 std::size_t append( std::span< const I > ind , std::span< const V > val );

 /// appends the \f$ r \f$ rows of a compressed form: the row \f$ k \f$ has
 /// the entries \f$ \mathit{beg}[ k ] - \mathit{beg}[ 0 ] \f$ to
 /// \f$ \mathit{beg}[ k + 1 ] - \mathit{beg}[ 0 ] \f$ of `ind` and `val`;
 /// the index of the first one
 ///
 /// In pseudocode (beyond the book), the starts shifted by
 /// \f$ \delta = |\mathit{ind}| - \mathit{beg}'[ 0 ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{append}( r , \mathit{beg}' ,
 ///     \mathit{ind}' , \mathit{val}' ) \cr
 /// 2 & \quad \text{append the entries from } \mathit{beg}'[ 0 ] \text{ to }
 ///     \mathit{beg}'[ r ] \text{ of } \mathit{ind}' \text{ and }
 ///     \mathit{val}' \cr
 /// 3 & \quad \textbf{for}\ k \gets 1\ \textbf{to}\ r\ \textbf{do}
 ///     \ \text{append } \mathit{beg}'[ k ] + \delta \text{ to } \mathit{beg}
 ///     \cr
 /// 4 & \quad \textbf{return}\ \text{the index of the first row appended}
 /// \end{array}}
 /// \f]
 /// \pre `beg` is not decreasing
 std::size_t append( std::size_t r , const I * beg , const I * ind ,
                     const V * val );

 /// removes the rows of \f$ E \f$ (a contiguous range of integers, e.g., a
 /// `std::vector`), closing the gaps in one pass
 ///
 /// In pseudocode (beyond the book), \f$ o \f$ the next entry written and
 /// \f$ \mathit{out} \f$ the next row:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( E ) \cr
 /// 2 & \quad \mathit{out} \gets E[ 0 ];\ o \gets \mathit{beg}[ \mathit{out}
 ///     ] \cr
 /// 3 & \quad \textbf{for}\ i \gets E[ 0 ]\ \textbf{to}\ \mathrm{rows}() -
 ///     1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ i \in E\ \textbf{then continue} \cr
 /// 5 & \qquad ( s , e ) \gets ( \mathit{beg}[ i ] , \mathit{beg}[ i + 1 ] )
 ///     ;\ \mathit{beg}[ \mathit{out} ] \gets o;\ \mathit{out} \gets
 ///     \mathit{out} + 1 \cr
 /// 6 & \qquad \text{move the entries } s .. e - 1 \text{ to } o .. o + e -
 ///     s - 1;\ o \gets o + e - s \cr
 /// 7 & \quad \mathit{beg}[ \mathit{out} ] \gets o;\ \text{cut the vectors }
 ///     \text{after } \mathit{out} \text{ rows and } o \text{ entries}
 /// \end{array}}
 /// \f]
 /// \pre \f$ E \f$ is strictly increasing and below `rows()`
 template< std::ranges::contiguous_range R >
 void remove( const R & E );

 /// removes the rows from \f$ \mathit{first} \f$ to \f$ \mathit{last} \f$,
 /// \f$ \mathit{last} \f$ excluded, by one erase of their entries and a
 /// shift of the starts after them
 /// \pre \f$ \mathit{first} \le \mathit{last} \le \mathrm{rows}() \f$
 void remove( std::size_t first , std::size_t last );

 /// the minor indices of the row \f$ i \f$
 /// \pre \f$ i < \mathrm{rows}() \f$
 [[nodiscard]] std::span< const I > indices( std::size_t i ) const;

 /// the values of the row \f$ i \f$
 /// \pre \f$ i < \mathrm{rows}() \f$
 [[nodiscard]] std::span< const V > values( std::size_t i ) const;

 /// the values of the row \f$ i \f$, to change in place
 /// \pre \f$ i < \mathrm{rows}() \f$
 [[nodiscard]] std::span< V > values( std::size_t i );

 /// the \f$ \mathrm{rows}() + 1 \f$ starts of the rows, the last one the
 /// number of the entries
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
