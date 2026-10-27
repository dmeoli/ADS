/*--------------------------------------------------------------------------*/
/*----------------------------- File Matrix.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The product of two square matrices \f$ n \times n \f$ (section 12.4):
 *
 * - `multiplyNaive()`, \f$ c_{ij} = \sum_k a_{ik} b_{kj} \f$,
 *   \f$ \Theta( n^3 ) \f$;
 * - `strassen()`, with \f$ A \f$, \f$ B \f$ and \f$ C \f$ split in four
 *   blocks \f$ n / 2 \times n / 2 \f$ and the seven products
 * \f[
 *   \begin{array}{ll}
 *     X_1 = ( A_{11} + A_{22} ) ( B_{11} + B_{22} ) &
 *     X_2 = ( A_{21} + A_{22} ) B_{11} \cr
 *     X_3 = A_{11} ( B_{12} - B_{22} ) & X_4 = A_{22} ( B_{21} - B_{11} ) \cr
 *     X_5 = ( A_{11} + A_{12} ) B_{22} &
 *     X_6 = ( A_{21} - A_{11} ) ( B_{11} + B_{12} ) \cr
 *     X_7 = ( A_{12} - A_{22} ) ( B_{21} + B_{22} ) &
 *   \end{array}
 * \f]
 *   and \f$ C_{11} = X_1 + X_4 - X_5 + X_7 \f$, \f$ C_{12} = X_3 + X_5 \f$,
 *   \f$ C_{21} = X_2 + X_4 \f$, \f$ C_{22} = X_1 + X_3 - X_2 + X_6 \f$:
 *   \f$ T( n ) = 7 T( n / 2 ) + \Theta( n^2 ) = O( n^{\log_2 7} ) =
 *   O( n^{2.81} ) \f$. The matrices are bordered with zeros up to a power
 *   of two, and the blocks of at most \f$ \mathit{cutoff} \f$ rows are
 *   multiplied by the naive method, which is faster on small ones.
 *
 * Both count in `*mults`, if given, the products of two entries;
 * `multiply()`, `transpose()` and `scale()` handle rectangular matrices.
 *
 * The contracts [see `Contract.h`]: the products need matrices of
 * compatible sizes, every row as long as the first, \f$ n \times n \f$ for
 * `multiplyNaive()` and `strassen()`, \f$ p \times q \f$ and
 * \f$ q \times r \f$ for `multiply()`, and `transpose()` a rectangular
 * matrix; the result of `strassen()` equals the one of `multiplyNaive()`, an
 * audit. The functions are defined in `Matrix.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see V. Strassen, Gaussian elimination is not optimal, Numerische
 *      Mathematik 13, 354-356, 1969, doi:10.1007/BF02165411
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

#ifndef __ADS_Matrix
 #define __ADS_Matrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

template< class T >
using Matrix = std::vector< std::vector< T > >;   ///< by rows

/// the product \f$ A B \f$ of two \f$ n \times n \f$ matrices by the
/// definition, \f$ \Theta( n^3 ) \f$ (the moltiplicaMatrici() of the book);
/// the products of two entries added to `*mults`, if given
///
/// In pseudocode (section 12.4), the loops in the order \f$ i , k , j \f$,
/// which reads \f$ B \f$ by rows:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiplyNaive}( A , B ) \cr
/// 2 & \quad C \gets \text{the } n \times n \text{ zero matrix} \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ k \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do}
///     \ c_{ij} \gets c_{ij} + a_{ik} b_{kj} \cr
/// 6 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ are \f$ n \times n \f$
template< class T >
Matrix< T > multiplyNaive( const Matrix< T > & A , const Matrix< T > & B ,
                           long * mults = nullptr );

/// the product of \f$ A \f$, \f$ p \times q \f$, and \f$ B \f$,
/// \f$ q \times r \f$, \f$ \Theta( p q r ) \f$; with \f$ q = 0 \f$ the vector
/// \f$ B \f$ has no row to give \f$ r \f$, and the product is
/// \f$ p \times 0 \f$
///
/// In pseudocode, the loops of `multiplyNaive()` on three bounds:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiply}( A , B ) \cr
/// 2 & \quad C \gets \text{the } p \times r \text{ zero matrix} \cr
/// 3 & \quad \textbf{foreach}\ i \le p , k \le q , j \le r\ \textbf{do}
///     \ c_{ij} \gets c_{ij} + a_{ik} b_{kj} \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is \f$ p \times q \f$ and \f$ B \f$ is \f$ q \times r \f$
template< class T >
Matrix< T > multiply( const Matrix< T > & A , const Matrix< T > & B );

/// the transpose \f$ A^T \f$, \f$ b_{ji} = a_{ij} \f$
/// \pre \f$ A \f$ is rectangular
template< class T >
Matrix< T > transpose( const Matrix< T > & A );

/// the product \f$ s A \f$ of the matrix by a scalar
template< class T >
Matrix< T > scale( Matrix< T > A , const T & s );

namespace detail {

/// true if \f$ A \f$ has \f$ r \f$ rows of \f$ c \f$ columns each
template< class T >
bool hasShape( const Matrix< T > & A , std::size_t r , std::size_t c );

/// the block \f$ h \times h \f$ of \f$ A \f$ from the entry
/// \f$ ( r , c ) \f$
template< class T >
Matrix< T > block( const Matrix< T > & A , std::size_t r , std::size_t c ,
                   std::size_t h );

/// the matrix \f$ A + s B \f$
template< class T >
Matrix< T > plus( const Matrix< T > & A , const Matrix< T > & B , T s = 1 );

/// the product of two matrices of the same power of 2 by the seven products
template< class T >
Matrix< T > strassenRec( const Matrix< T > & A , const Matrix< T > & B ,
                         std::size_t cutoff , long * mults );

}  // end( namespace detail )

/// the product \f$ A B \f$ by the algorithm of Strassen,
/// \f$ O( n^{2.81} ) \f$, the blocks of at most \f$ \mathit{cutoff} \f$ rows
/// by the naive method; the products of two entries added to `*mults`, if
/// given
///
/// In pseudocode (section 12.4), \f$ A \f$ and \f$ B \f$ bordered with zeros
/// up to a power of 2, and the product read in the first \f$ n \f$ rows and
/// columns:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{strassenRec}( A , B ) \cr
/// 2 & \quad \textbf{if}\ n \le \mathit{cutoff}\ \textbf{then return}
///     \ \mathrm{multiplyNaive}( A , B ) \cr
/// 3 & \quad \text{split } A \text{ and } B \text{ in the blocks }
///     A_{ij} \text{ and } B_{ij} \text{ of } n / 2 \text{ rows} \cr
/// 4 & \quad X_1 , \ldots , X_7 \gets \text{the seven products of the }
///     \text{file, each by } \mathrm{strassenRec} \cr
/// 5 & \quad C_{11} \gets X_1 + X_4 - X_5 + X_7;\ C_{12} \gets X_3 + X_5 \cr
/// 6 & \quad C_{21} \gets X_2 + X_4;\ C_{22} \gets X_1 + X_3 - X_2 + X_6 \cr
/// 7 & \quad \textbf{return}\ \text{the matrix of the blocks } C_{ij}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ and \f$ B \f$ are \f$ n \times n \f$
/// \post the result is \f$ A B \f$ (audit)
template< class T >
Matrix< T > strassen( const Matrix< T > & A , const Matrix< T > & B ,
                      std::size_t cutoff = 1 , long * mults = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/Matrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Matrix.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Matrix.h ----------------------------*/
/*--------------------------------------------------------------------------*/
