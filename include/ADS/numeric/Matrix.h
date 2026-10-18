/*--------------------------------------------------------------------------*/
/*----------------------------- File Matrix.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The product of two square matrices n x n (section 12.4):
 *
 * - multiplyNaive(), \f$ c_{ij} = \sum_k a_{ik} b_{kj} \f$,
 *   \f$ \Theta( n^3 ) \f$;
 * - strassen(), with A, B and C split in four blocks n / 2 x n / 2 and the
 *   seven products
 * \f[
 *   \begin{array}{ll}
 *     X_1 = ( A_{11} + A_{22} ) ( B_{11} + B_{22} ) &
 *     X_2 = ( A_{21} + A_{22} ) B_{11} \\
 *     X_3 = A_{11} ( B_{12} - B_{22} ) & X_4 = A_{22} ( B_{21} - B_{11} ) \\
 *     X_5 = ( A_{11} + A_{12} ) B_{22} &
 *     X_6 = ( A_{21} - A_{11} ) ( B_{11} + B_{12} ) \\
 *     X_7 = ( A_{12} - A_{22} ) ( B_{21} + B_{22} ) &
 *   \end{array}
 * \f]
 *   and \f$ C_{11} = X_1 + X_4 - X_5 + X_7 \f$, \f$ C_{12} = X_3 + X_5 \f$,
 *   \f$ C_{21} = X_2 + X_4 \f$, \f$ C_{22} = X_1 + X_3 - X_2 + X_6 \f$:
 *   \f$ T( n ) = 7 T( n / 2 ) + \Theta( n^2 ) = O( n^{\log_2 7} ) =
 *   O( n^{2.81} ) \f$. The matrices are bordered with zeros up to a power
 *   of two, and the blocks of at most cutoff rows are multiplied by the
 *   naive method, which is faster on small ones.
 *
 * Both count in *mults, if given, the products of two entries. multiply(),
 * transpose() and scale() handle rectangular matrices.
 *
 * The contracts [see Contract.h]: the products need matrices of compatible
 * sizes, every row as long as the first, n x n for multiplyNaive() and
 * strassen(), p x q and q x r for multiply(), and transpose() a rectangular
 * matrix; the result of strassen() equals the one of multiplyNaive(), an
 * audit. The functions are defined in Matrix.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see V. Strassen, Gaussian elimination is not optimal, Numerische
 *      Mathematik 13, 354-356, 1969, doi:10.1007/BF02165411
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

/// moltiplicaMatrici(): Theta( n^3 )
/// \pre A and B are n x n
template< class T >
Matrix< T > multiplyNaive( const Matrix< T > & A , const Matrix< T > & B ,
                           long * mults = nullptr );

/// the product of A, p x q, and B, q x r, Theta( p q r ); with q = 0 the
/// vector B has no row to give r, and the product is p x 0
/// \pre A is p x q and B is q x r
template< class T >
Matrix< T > multiply( const Matrix< T > & A , const Matrix< T > & B );

/// the transpose of A
/// \pre A is rectangular
template< class T >
Matrix< T > transpose( const Matrix< T > & A );

/// s A
template< class T >
Matrix< T > scale( Matrix< T > A , const T & s );

namespace detail {

/// A has r rows of c columns each
template< class T >
bool hasShape( const Matrix< T > & A , std::size_t r , std::size_t c );

template< class T >
Matrix< T > block( const Matrix< T > & A , std::size_t r , std::size_t c ,
                   std::size_t h );

template< class T >
Matrix< T > plus( const Matrix< T > & A , const Matrix< T > & B , T s = 1 );

template< class T >
Matrix< T > strassenRec( const Matrix< T > & A , const Matrix< T > & B ,
                         std::size_t cutoff , long * mults );

}  // end( namespace detail )

/// strassen(): O( n^2.81 ), the blocks of at most cutoff rows by the naive
/// method
/// \pre A and B are n x n
/// \post the result is A B
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
