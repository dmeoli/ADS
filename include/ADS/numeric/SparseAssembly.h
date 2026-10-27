/*--------------------------------------------------------------------------*/
/*-------------------------- File SparseAssembly.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The assembly of a sparse matrix from its triplets, beyond the book: the
 * entries \f$ ( i , j , v ) \f$ in any order, with repetitions, become the
 * arrays of the compressed form that the solvers of linear programs take
 * (\f$ \mathit{beg} \f$, \f$ \mathit{ind} \f$ and \f$ \mathit{val} \f$, by
 * columns or by rows), written in the arrays of the caller, with no matrix
 * object in between and no other allocation than a workspace that a caller
 * may keep from one assembly to the next.
 *
 * `compressTriplets()` first distributes the entries by their major index
 * (the column of the compressed sparse columns, the row of the compressed
 * sparse rows), by the counting sort (section 5.2 of Knuth): one read of the
 * major indices counts them, the prefix sums give \f$ \mathit{beg} \f$, and
 * a second read of the triplets writes each entry at the cursor of its major
 * index in \f$ \mathit{ind} \f$ and \f$ \mathit{val} \f$, straight into the
 * arrays of the caller, so that each major index has its entries in the
 * order of the input. Then each major index sums its repetitions and sorts
 * its distinct entries: a mark per minor index, the place written last for
 * it plus 1, tells whether a minor index already has a place in the current
 * major index (the places of the earlier ones all come before it), and if so
 * the value is added there, in the order of the input, otherwise it takes
 * the next place (chapter 2 of Davis); the distinct entries are then sorted
 * by their minor indices, by insertion up to 32 of them, i.e., at most
 * \f$ k ( k - 1 ) / 2 \f$ moves for \f$ k \f$ of them and none if they are
 * already in order, and by the sort of the library beyond. Only one pass
 * scatters the entries over memory, where the sort by the least significant
 * index first, or the transposition of a matrix built by the other index
 * (as in `setFromTriplets()` of Eigen), scatters them twice; the time is
 * \f$ O( z + m + n + \sum_i \min( k_i^2 , k_i \log k_i ) ) \f$ for
 * \f$ z \f$ triplets of an \f$ m \times n \f$ matrix whose major index
 * \f$ i \f$ has \f$ k_i \f$ distinct entries, i.e., \f$ O( z + m + n ) \f$
 * for rows or columns of bounded length, and the workspace holds a cursor
 * per major index, a mark per minor one and the pairs of the longest major
 * index past 32 entries.
 *
 * `assembleCsc()` and `assembleCsr()` name the two orientations:
 * \f$ \mathit{beg} \f$ has one entry more than the major indices,
 * \f$ \mathit{beg}[ k ] \f$ being the first entry of the major index
 * \f$ k \f$ and its last entry the number of the entries,
 * \f$ \mathit{ind} \f$ the minor index of each entry and \f$ \mathit{val} \f$
 * its value. The entries summed to 0 stay, as the solvers accept them.
 *
 * The contracts [see `Contract.h`]: the indices of the triplets are below
 * the dimensions, \f$ \mathit{beg} \f$ has room for the major indices plus
 * 1, \f$ \mathit{ind} \f$ and \f$ \mathit{val} \f$ for the triplets, and the
 * number of the triplets fits the index type `I`. The functions are defined
 * in `SparseAssembly.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 5.2 (the
 *      distribution counting, and the sort by the least significant digit
 *      first)
 * \see F. G. Gustavson, Two fast algorithms for sparse matrices:
 *      multiplication and permuted transposition, ACM Transactions on
 *      Mathematical Software 4(3), 250-269, 1978, doi:10.1145/355791.355796
 * \see T. A. Davis, Direct Methods for Sparse Linear Systems, SIAM, 2006,
 *      doi:10.1137/1.9780898718881, chapter 2 (the compression of the
 *      triplets and the sum of the duplicates)
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

#ifndef __ADS_SparseAssembly
 #define __ADS_SparseAssembly
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/DenseMatrix.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS AssemblyWorkspace ------------------------*/
/*--------------------------------------------------------------------------*/

/// the scratch of `compressTriplets()`, which keeps its memory from one
/// call to the next
template< class I , class V >
struct AssemblyWorkspace {
 template< class U >                  // not zeroed when they grow
 using Vector = std::vector< U , detail::DefaultInitAllocator< U > >;

 Vector< I > cursor;                  ///< the cursor of each major index
 Vector< I > mark;                    ///< the last place of each minor one
 std::vector< std::pair< I , V > > pairs;   ///< a long one, to sort
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the compressed form, by major index, of the \f$ z \f$ triplets
/// \f$ ( \mathit{major}_k , \mathit{minor}_k , \mathit{value}_k ) \f$ of a
/// matrix of \f$ \mathit{nmajor} \times \mathit{nminor} \f$, the values of
/// equal places summed; the number of the entries
///
/// The result is \f$ \mathit{beg}[ 0 .. \mathit{nmajor} ] \f$, and the minor
/// indices and the values in `ind` and `val`, increasing in each major
/// index. In pseudocode (the counting sort of section 5.2 of Knuth and the
/// sum of the duplicates of Davis, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{compressTriplets}( \mathit{major} ,
///     \mathit{minor} , \mathit{value} ) \cr
/// 2 & \quad \mathit{beg} \gets \text{the prefix sums of the counts of the }
///     \text{major indices} \cr
/// 3 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ z - 1\ \textbf{do} \cr
/// 4 & \qquad q \gets \mathit{at}[ \mathit{major}[ k ] ]++;
///     \ \mathit{ind}[ q ] \gets \mathit{minor}[ k ];\ \mathit{val}[ q ] \gets
///     \mathit{value}[ k ] \cr
/// 5 & \quad o \gets 0;\ \mathit{mark} \gets \text{all } 0 \cr
/// 6 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ \mathit{nmajor} - 1
///     \ \textbf{do} \cr
/// 7 & \qquad f \gets o \cr
/// 8 & \qquad \textbf{foreach}\ \text{entry } ( j , v ) \text{ of } i
///     \text{, in the order of the input}\ \textbf{do} \cr
/// 9 & \qquad\quad \textbf{if}\ \mathit{mark}[ j ] > f\ \textbf{then}
///     \ \mathit{val}[ \mathit{mark}[ j ] - 1 ] \gets \mathit{val}[
///     \mathit{mark}[ j ] - 1 ] + v \cr
/// 10 & \qquad\quad \textbf{else}\ \mathit{mark}[ j ] \gets o + 1;\ (
///     \mathit{ind}[ o ] , \mathit{val}[ o ] ) \gets ( j , v );\ o \gets
///     o + 1 \cr
/// 11 & \qquad \text{sort the entries } f .. o - 1 \text{ by minor index}
///     \text{, by insertion up to 32} \cr
/// 12 & \qquad \mathit{beg}[ i ] \gets f \cr
/// 13 & \quad \mathit{beg}[ \mathit{nmajor} ] \gets o;\ \textbf{return}\ o
/// \end{array}}
/// \f]
/// \pre the indices are below \f$ \mathit{nmajor} \f$ and
///      \f$ \mathit{nminor} \f$, `beg` has \f$ \mathit{nmajor} + 1 \f$
///      places, `ind` and `val` at least \f$ z \f$, and \f$ z \f$ fits `I`
template< class I , class V , class J >
std::size_t compressTriplets( std::size_t nmajor , std::size_t nminor ,
                              std::size_t z , const J * major ,
                              const J * minor , const V * value , I * beg ,
                              I * ind , V * val ,
                              AssemblyWorkspace< I , V > & work );

/// the form of `compressTriplets()` above, with a workspace of its own
template< class I , class V , class J >
std::size_t compressTriplets( std::size_t nmajor , std::size_t nminor ,
                              std::size_t z , const J * major ,
                              const J * minor , const V * value , I * beg ,
                              I * ind , V * val );

/// the compressed sparse columns of the \f$ m \times n \f$ matrix of the
/// \f$ z \f$ triplets
/// \f$ ( \mathit{row}[ k ] , \mathit{col}[ k ] , \mathit{value}[ k ] ) \f$:
/// `beg` has \f$ n + 1 \f$ places, `ind` holds the rows; the number of the
/// entries
///
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{assembleCsc}( m , n , \mathit{row} ,
///     \mathit{col} , \mathit{value} ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{compressTriplets}( \mathit{col} ,
///     \mathit{row} , \mathit{value} )\quad \triangleright\ n \times m
/// \end{array}}
/// \f]
template< class I , class V , class J >
std::size_t assembleCsc( std::size_t m , std::size_t n , std::size_t z ,
                         const J * row , const J * col , const V * value ,
                         I * beg , I * ind , V * val ,
                         AssemblyWorkspace< I , V > & work );

/// the compressed sparse rows of the same: `beg` has \f$ m + 1 \f$ places,
/// `ind` holds the columns; the number of the entries
///
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{assembleCsr}( m , n , \mathit{row} ,
///     \mathit{col} , \mathit{value} ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{compressTriplets}( \mathit{row} ,
///     \mathit{col} , \mathit{value} )\quad \triangleright\ m \times n
/// \end{array}}
/// \f]
template< class I , class V , class J >
std::size_t assembleCsr( std::size_t m , std::size_t n , std::size_t z ,
                         const J * row , const J * col , const V * value ,
                         I * beg , I * ind , V * val ,
                         AssemblyWorkspace< I , V > & work );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/SparseAssembly.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SparseAssembly.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SparseAssembly.h -----------------------*/
/*--------------------------------------------------------------------------*/
