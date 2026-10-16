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

#include <cstddef>
#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

template< class T >
using Matrix = std::vector< std::vector< T > >;   ///< by rows

/// moltiplicaMatrici(): Theta( n^3 )
template< class T >
Matrix< T > multiplyNaive( const Matrix< T > & A , const Matrix< T > & B ,
                           long * mults = nullptr )
{
 const std::size_t n = A.size();
 Matrix< T > C( n , std::vector< T >( n , T( 0 ) ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t k = 0 ; k < n ; ++k )
   for( std::size_t j = 0 ; j < n ; ++j )
    C[ i ][ j ] += A[ i ][ k ] * B[ k ][ j ];
 if( mults )
  *mults += long( n * n * n );
 return( C );
 }

/// the product of A, p x q, and B, q x r, Theta( p q r )
template< class T >
Matrix< T > multiply( const Matrix< T > & A , const Matrix< T > & B )
{
 const std::size_t p = A.size() , q = B.size() ,
                   r = B.empty() ? 0 : B[ 0 ].size();
 Matrix< T > C( p , std::vector< T >( r , T( 0 ) ) );
 for( std::size_t i = 0 ; i < p ; ++i )
  for( std::size_t k = 0 ; k < q ; ++k )
   for( std::size_t j = 0 ; j < r ; ++j )
    C[ i ][ j ] += A[ i ][ k ] * B[ k ][ j ];
 return( C );
 }

/// the transpose of A
template< class T >
Matrix< T > transpose( const Matrix< T > & A )
{
 const std::size_t p = A.size() , q = A.empty() ? 0 : A[ 0 ].size();
 Matrix< T > B( q , std::vector< T >( p ) );
 for( std::size_t i = 0 ; i < p ; ++i )
  for( std::size_t j = 0 ; j < q ; ++j ) B[ j ][ i ] = A[ i ][ j ];
 return( B );
 }

/// s A
template< class T >
Matrix< T > scale( Matrix< T > A , const T & s )
{
 for( auto & row : A )
  for( auto & x : row ) x *= s;
 return( A );
 }

namespace detail {

template< class T >
Matrix< T > block( const Matrix< T > & A , std::size_t r , std::size_t c ,
                   std::size_t h )
{
 Matrix< T > X( h , std::vector< T >( h ) );
 for( std::size_t i = 0 ; i < h ; ++i )
  for( std::size_t j = 0 ; j < h ; ++j )
   X[ i ][ j ] = A[ r + i ][ c + j ];
 return( X );
 }

template< class T >
Matrix< T > plus( const Matrix< T > & A , const Matrix< T > & B , T s = 1 )
{
 Matrix< T > C = A;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = 0 ; j < A.size() ; ++j )
   C[ i ][ j ] += s * B[ i ][ j ];
 return( C );
 }

template< class T >
Matrix< T > strassenRec( const Matrix< T > & A , const Matrix< T > & B ,
                         std::size_t cutoff , long * mults )
{
 const std::size_t n = A.size();
 if( n <= cutoff )
  return( multiplyNaive( A , B , mults ) );
 const std::size_t h = n / 2;
 const auto A11 = block( A , 0 , 0 , h ) , A12 = block( A , 0 , h , h ) ,
            A21 = block( A , h , 0 , h ) , A22 = block( A , h , h , h ) ,
            B11 = block( B , 0 , 0 , h ) , B12 = block( B , 0 , h , h ) ,
            B21 = block( B , h , 0 , h ) , B22 = block( B , h , h , h );
 const T m = T( -1 );
 auto S = [ & ]( const Matrix< T > & X , const Matrix< T > & Y ) {
  return( strassenRec( X , Y , cutoff , mults ) ); };
 const auto X1 = S( plus( A11 , A22 ) , plus( B11 , B22 ) );
 const auto X2 = S( plus( A21 , A22 ) , B11 );
 const auto X3 = S( A11 , plus( B12 , B22 , m ) );
 const auto X4 = S( A22 , plus( B21 , B11 , m ) );
 const auto X5 = S( plus( A11 , A12 ) , B22 );
 const auto X6 = S( plus( A21 , A11 , m ) , plus( B11 , B12 ) );
 const auto X7 = S( plus( A12 , A22 , m ) , plus( B21 , B22 ) );
 const auto C11 = plus( plus( plus( X1 , X4 ) , X5 , m ) , X7 );
 const auto C12 = plus( X3 , X5 );
 const auto C21 = plus( X2 , X4 );
 const auto C22 = plus( plus( plus( X1 , X3 ) , X2 , m ) , X6 );
 Matrix< T > C( n , std::vector< T >( n ) );
 for( std::size_t i = 0 ; i < h ; ++i )
  for( std::size_t j = 0 ; j < h ; ++j ) {
   C[ i ][ j ] = C11[ i ][ j ];
   C[ i ][ j + h ] = C12[ i ][ j ];
   C[ i + h ][ j ] = C21[ i ][ j ];
   C[ i + h ][ j + h ] = C22[ i ][ j ];
   }
 return( C );
 }

}  // end( namespace detail )

/// strassen(): O( n^2.81 ), the blocks of at most cutoff rows by the naive
/// method
template< class T >
Matrix< T > strassen( const Matrix< T > & A , const Matrix< T > & B ,
                      std::size_t cutoff = 1 , long * mults = nullptr )
{
 const std::size_t n = A.size();
 if( ( B.size() != n ) || ( n && ( A[ 0 ].size() != n ) ) )
  throw( std::invalid_argument( "ADS::strassen: not two n x n matrices" ) );
 std::size_t N = 1;
 while( N < n )
  N *= 2;
 Matrix< T > P( N , std::vector< T >( N , T( 0 ) ) ) , Q = P;
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j ) {
   P[ i ][ j ] = A[ i ][ j ];
   Q[ i ][ j ] = B[ i ][ j ];
   }
 auto R = detail::strassenRec( P , Q , cutoff < 1 ? 1 : cutoff , mults );
 R.resize( n );
 for( auto & r : R )
  r.resize( n );
 return( R );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Matrix.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Matrix.h ----------------------------*/
/*--------------------------------------------------------------------------*/
