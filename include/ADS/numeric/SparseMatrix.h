/*--------------------------------------------------------------------------*/
/*-------------------------- File SparseMatrix.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A sparse matrix on orthogonal lists, beyond the book (2.2.6 of Knuth):
 * each nonzero entry is a node on the circular list of its row, sorted by
 * column, and on that of its column, sorted by row, each list with a head
 * node; the space is O( m + n + nnz ) and an entry is found by walking its
 * row. pivot() is the pivot step of Algorithm 2.2.6S, the core of the
 * elimination by exchange: with the pivot \f$ a \f$ in the row r and the
 * column c, the pivot becomes \f$ 1 / a \f$, the rest of its row
 * \f$ b / a \f$, the rest of its column \f$ - c / a \f$, and every other
 * entry d with a nonzero b in its column of the pivot row and c in its row
 * of the pivot column becomes \f$ d - b c / a \f$, the entries that become
 * zero leaving the lists and those that appear (the fill-in) entering
 * them; it costs time proportional to the nonzeros it touches. A pivot
 * step on each diagonal place, in turn, inverts a matrix whose pivots are
 * not zero.
 *
 * The contracts [see Contract.h]: the places are in the matrix, and the
 * pivot is not zero; the invariant is that the lists are sorted and that a
 * node is on the lists of its row and of its column, an audit. The
 * operations are defined in SparseMatrix.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.2.6
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

#ifndef __ADS_SparseMatrix
 #define __ADS_SparseMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cmath>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SparseMatrix ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sparse matrix on orthogonal circular lists
class SparseMatrix {

/*--------------------------------------------------------------------------*/
 public:

 /// the zero matrix of m rows and n columns
 SparseMatrix( std::size_t m , std::size_t n );

 [[nodiscard]] std::size_t rows( void ) const;
 [[nodiscard]] std::size_t cols( void ) const;

 /// the number of nonzero entries
 [[nodiscard]] std::size_t nonzeros( void ) const;

 /// \pre i < m and j < n
 [[nodiscard]] double get( std::size_t i , std::size_t j ) const;

 /// sets the entry ( i , j ) to v, removing its node if v is 0
 /// \pre i < m and j < n
 void set( std::size_t i , std::size_t j , double v );

 /// the nonzeros of the row i, by column
 /// \pre i < m
 [[nodiscard]] std::vector< std::pair< std::size_t , double > > row(
                                                       std::size_t i ) const;

 /// the nonzeros of the column j, by row
 /// \pre j < n
 [[nodiscard]] std::vector< std::pair< std::size_t , double > > col(
                                                       std::size_t j ) const;

 /// the pivot step on the entry ( r , c ); entries below eps vanish
 /// \pre r < m , c < n and the entry ( r , c ) is not 0
 void pivot( std::size_t r , std::size_t c , double eps = 0 );

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::size_t row , col;
  double val;
  long right , down;       ///< the next on the row and on the column
  };

 /// the node before column j on the row i, and that of ( i , j ), or -1
 [[nodiscard]] std::pair< long , long > findInRow( std::size_t i ,
                                                   std::size_t j ) const;
 [[nodiscard]] long beforeInCol( std::size_t i , std::size_t j ) const;

 /// the lists sorted, each node on the lists of its row and column
 [[nodiscard]] bool linked( void ) const;

 std::size_t f_m , f_n;
 std::vector< Node > f_N;  ///< the heads of the rows, of the columns, then
 std::vector< long > f_freeNodes;   ///< the entries; the free ones
 std::size_t f_nnz = 0;

 };  // end( class SparseMatrix )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/SparseMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SparseMatrix.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SparseMatrix.h -------------------------*/
/*--------------------------------------------------------------------------*/
