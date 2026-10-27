/*--------------------------------------------------------------------------*/
/*-------------------------- File SparseMatrix.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A sparse matrix on orthogonal lists, beyond the book (section 2.2.6 of
 * Knuth): each nonzero entry is a node on the circular list of its row,
 * sorted by column, and on that of its column, sorted by row, each list with
 * a head node; the space is \f$ O( m + n + \mathit{nnz} ) \f$ and an entry is
 * found by walking its row. `pivot()` is the pivot step of Algorithm 2.2.6S,
 * the core of the elimination by exchange: with the pivot \f$ a \f$ in the
 * row \f$ r \f$ and the column \f$ c \f$, the pivot becomes \f$ 1 / a \f$,
 * the rest of its row \f$ b / a \f$, the rest of its column \f$ - c / a \f$,
 * and every other entry \f$ d \f$ with a nonzero \f$ b \f$ in its column of
 * the pivot row and \f$ c \f$ in its row of the pivot column becomes
 * \f$ d - b c / a \f$, the entries that become zero leaving the lists and
 * those that appear (the fill-in) entering them. Knuth's step walks the
 * rows of the pivot column side by side with the pivot row, in time
 * proportional to the nonzeros it touches; this one reads and writes each
 * of the \f$ |R| \cdot |C| \f$ entries it touches by `get()` and `set()`,
 * each a walk of a row and of a column. A pivot step on each diagonal
 * place, in turn, inverts a matrix whose pivots are not zero.
 *
 * The contracts [see `Contract.h`]: the places are in the matrix, and the
 * pivot is not zero; the invariant is that the lists are sorted and that a
 * node is on the lists of its row and of its column, an audit. The
 * operations are defined in `SparseMatrix.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.2.6
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

#ifndef __ADS_SparseMatrix
 #define __ADS_SparseMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <iterator>
#include <map>
#include <ranges>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// a forward iterator along an orthogonal list of the nodes of a sparse
/// matrix [see `SparseMatrix`], by row or by column, the head of the list
/// being the end; `*it` gives the column (or the row) and the value
template< class Node >
class OrthogonalIterator {
 public:
 using reference = std::pair< std::size_t , double >;
 using value_type = reference;
 using difference_type = std::ptrdiff_t;
 using iterator_concept = std::forward_iterator_tag;
 using iterator_category = std::input_iterator_tag;

 OrthogonalIterator( void ) = default;
 OrthogonalIterator( const std::vector< Node > * N , long q , bool byRow )
  : f_N( N ) , f_q( q ) , f_row( byRow ) {}

 /// the column (or row) of the entry and its value
 reference operator*( void ) const {
  const Node & x = ( *f_N )[ f_q ];
  return( reference( f_row ? x.col : x.row , x.val ) );
  }
 OrthogonalIterator & operator++( void ) {
  f_q = f_row ? ( *f_N )[ f_q ].right : ( *f_N )[ f_q ].down;
  return( *this );
  }
 OrthogonalIterator operator++( int ) {
  auto t = *this;
  ++*this;
  return( t );
  }
 bool operator==( const OrthogonalIterator & o ) const {
  return( f_q == o.f_q );
  }

 private:
 const std::vector< Node > * f_N = nullptr;
 long f_q = -1;            ///< the node, the head of the list at the end
 bool f_row = true;

 };  // end( class OrthogonalIterator )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SparseMatrix ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sparse matrix on orthogonal circular lists
class SparseMatrix {

/*--------------------------------------------------------------------------*/
 public:

 /// the zero matrix of \f$ m \f$ rows and \f$ n \f$ columns, the
 /// \f$ m + n \f$ head nodes alone
 SparseMatrix( std::size_t m , std::size_t n );

 /// the number \f$ m \f$ of rows
 [[nodiscard]] std::size_t rows( void ) const;
 /// the number \f$ n \f$ of columns
 [[nodiscard]] std::size_t cols( void ) const;

 /// the number \f$ \mathit{nnz} \f$ of nonzero entries
 [[nodiscard]] std::size_t nonzeros( void ) const;

 /// the entry \f$ ( i , j ) \f$, 0 if it has no node, by a walk of the row
 /// \f$ i \f$
 /// \pre \f$ i < m \f$ and \f$ j < n \f$
 [[nodiscard]] double get( std::size_t i , std::size_t j ) const;

 /// sets the entry \f$ ( i , j ) \f$ to \f$ v \f$, removing its node if
 /// \f$ v \f$ is 0
 ///
 /// In pseudocode (section 2.2.6 of Knuth, beyond the book), \f$ p \f$ and
 /// \f$ u \f$ the nodes before the place on its row and on its column:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{set}( i , j , v ) \cr
 /// 2 & \quad q \gets \text{the node of } ( i , j ) \text{ on the row } i
 ///     \text{, or } \textbf{nil} \cr
 /// 3 & \quad \textbf{if}\ q \ne \textbf{nil}\ \textbf{and}\ v \ne 0
 ///     \ \textbf{then}\ q.\mathit{val} \gets v \cr
 /// 4 & \quad \textbf{else if}\ q \ne \textbf{nil}\ \textbf{then}
 ///     \ p.\mathit{right} \gets q.\mathit{right};\ u.\mathit{down} \gets
 ///     q.\mathit{down} \cr
 /// 5 & \quad \textbf{else if}\ v \ne 0\ \textbf{then} \cr
 /// 6 & \qquad q \gets \text{a new node } ( i , j , v ) \text{ with }
 ///     q.\mathit{right} \gets p.\mathit{right} \text{, } q.\mathit{down}
 ///     \gets u.\mathit{down} \cr
 /// 7 & \qquad p.\mathit{right} \gets q;\ u.\mathit{down} \gets q
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < m \f$ and \f$ j < n \f$
 void set( std::size_t i , std::size_t j , double v );

 /// the nonzeros of the row \f$ i \f$, by column
 /// \pre \f$ i < m \f$
 [[nodiscard]] std::vector< std::pair< std::size_t , double > > row(
                                                       std::size_t i ) const;

 /// the nonzeros of the column \f$ j \f$, by row
 /// \pre \f$ j < n \f$
 [[nodiscard]] std::vector< std::pair< std::size_t , double > > col(
                                                       std::size_t j ) const;

 /// the pivot step on the entry \f$ ( r , c ) \f$; the entries of absolute
 /// value at most \f$ \varepsilon \f$ vanish
 ///
 /// In pseudocode (Algorithm 2.2.6S of Knuth, beyond the book), \f$ R \f$ and
 /// \f$ C \f$ the nonzeros of the pivot row and column before the step:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{pivot}( r , c , \varepsilon ) \cr
 /// 2 & \quad a \gets \mathrm{get}( r , c ) \cr
 /// 3 & \quad \textbf{foreach}\ ( i , c_i ) \in C \text{, } i \ne r
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{foreach}\ ( j , b_j ) \in R \text{, } j \ne c
 ///     \ \textbf{do} \cr
 /// 5 & \qquad\quad d \gets \mathrm{get}( i , j ) - b_j c_i / a \cr
 /// 6 & \qquad\quad \textbf{if}\ |d| \le \varepsilon\ \textbf{then}\ d \gets
 ///     0 \cr
 /// 7 & \qquad\quad \mathrm{set}( i , j , d ) \cr
 /// 8 & \quad \textbf{foreach}\ ( j , b_j ) \in R \text{, } j \ne c
 ///     \ \textbf{do}\ \mathrm{set}( r , j , b_j / a ) \cr
 /// 9 & \quad \textbf{foreach}\ ( i , c_i ) \in C \text{, } i \ne r
 ///     \ \textbf{do}\ \mathrm{set}( i , c , - c_i / a ) \cr
 /// 10 & \quad \mathrm{set}( r , c , 1 / a )
 /// \end{array}}
 /// \f]
 /// \pre \f$ r < m \f$, \f$ c < n \f$ and the entry \f$ ( r , c ) \f$ is
 ///      not 0
 void pivot( std::size_t r , std::size_t c , double eps = 0 );

/*--------------------------------------------------------------------------*/
 private:

 /// a node: a nonzero entry, or the head of a row or of a column
 struct Node {
  std::size_t row ,   ///< the row
              col;    ///< the column
  double val;         ///< the value
  long right ,        ///< the next on the row
       down;          ///< the next on the column
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the nonzeros of a row or of a column as ranges, forward along the
 // orthogonal lists, instead of the vectors that row() and col() build

 using const_iterator = detail::OrthogonalIterator< Node >;

 /// the nonzeros of the row \f$ i \f$ as (column, value), by column
 /// \pre \f$ i < m \f$
 [[nodiscard]] std::ranges::subrange< const_iterator > rowEntries(
                                                       std::size_t i ) const;

 /// the nonzeros of the column \f$ j \f$ as (row, value), by row
 /// \pre \f$ j < n \f$
 [[nodiscard]] std::ranges::subrange< const_iterator > colEntries(
                                                       std::size_t j ) const;

 private:

 /// the node before the column \f$ j \f$ on the row \f$ i \f$, and that of
 /// \f$ ( i , j ) \f$, or \f$ -1 \f$
 [[nodiscard]] std::pair< long , long > findInRow( std::size_t i ,
                                                   std::size_t j ) const;
 /// the node before the row \f$ i \f$ on the column \f$ j \f$
 [[nodiscard]] long beforeInCol( std::size_t i , std::size_t j ) const;

 /// the lists sorted, each node on the lists of its row and column
 [[nodiscard]] bool linked( void ) const;

 std::size_t f_m ,   ///< the rows
             f_n;    ///< the columns
 /// the heads of the rows, those of the columns, then the entries
 std::vector< Node > f_N;
 std::vector< long > f_freeNodes;   ///< the free nodes of the entries
 std::size_t f_nnz = 0;             ///< the nonzero entries

 };  // end( class SparseMatrix )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/SparseMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SparseMatrix.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SparseMatrix.h -------------------------*/
/*--------------------------------------------------------------------------*/
