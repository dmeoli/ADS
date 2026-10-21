/*--------------------------------------------------------------------------*/
/*--------------------------- File ParallelFFT.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel fast Fourier transform, beyond the book: fft( ex , a ,
 * inverse ) is the transform of fft( a , inverse ) [see FFT.h] on the
 * threads of an executor [see Executor.h], by the four steps of Bailey on
 * top of the sequential fft(). The \f$ n = n_1 n_2 \f$ points, \f$ n_1 =
 * 2^{\lfloor k / 2 \rfloor} \f$ for \f$ n = 2^k \f$, are the matrix
 * \f$ a_{j_1 j_2} = a[ j_1 n_2 + j_2 ] \f$ of \f$ n_1 \f$ rows, and with
 * \f$ \omega_N = e^{-2 \pi i / N} \f$
 * \f[
 *   \hat a[ k_1 + n_1 k_2 ] = \sum_{j_2} \omega_{n_2}^{j_2 k_2}
 *   \Bigl( \omega_n^{j_2 k_1} \sum_{j_1} \omega_{n_1}^{j_1 k_1}
 *   a_{j_1 j_2} \Bigr) ,
 * \f]
 * i.e., the \f$ n_2 \f$ transforms of the columns, the multiplication by
 * the twiddle factors \f$ \omega_n^{j_2 k_1} \f$, the \f$ n_1 \f$
 * transforms of the rows, and the transposition of the result. Each step
 * is a parallel loop: the transforms of length \f$ n_1 \f$ or \f$ n_2 \f$
 * (about \f$ \sqrt n \f$, in the cache of each core) are independent and
 * each is the sequential fft() of a row, and the columns become rows by
 * three transpositions of the whole matrix by blocks of 32 x 32 points,
 * also in parallel. The twiddle \f$ \omega_n^t \f$, \f$ t = j_2 k_1 \bmod
 * n \f$, is the product of two entries of tables of \f$ 2^{\lceil k / 2
 * \rceil} \f$ powers, \f$ \omega_n^{t_h 2^h} \omega_n^{t_l} \f$, each
 * computed by the library within an ulp, so that a twiddle has an error
 * of a few ulps instead of growing with a recurrence of products. The
 * inverse runs the same steps with \f$ \omega_n^{-1} \f$, and its row
 * transforms divide by \f$ n_1 \f$ and by \f$ n_2 \f$, by n in all.
 * The work is that of the sequential transform plus 3 passes of
 * transposition and one of twiddles over the n points, \f$ O( n \log n /
 * p + n / p ) \f$ time on p threads; the transform of a row is not split.
 * The result equals the sequential one up to the rounding, the error of
 * both being \f$ O( \epsilon \log n ) \f$ relative to the norm.
 *
 * The contracts [see Contract.h]: the size of a is a power of 2 (or 0).
 * \see D. H. Bailey, FFTs in external or hierarchical memory, The Journal
 *      of Supercomputing 4(1), 23-35, 1990, doi:10.1007/BF00162341
 * \see C. Van Loan, Computational Frameworks for the Fast Fourier
 *      Transform, SIAM, 1992, doi:10.1137/1.9781611970999
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ParallelFFT
 #define __ADS_ParallelFFT
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/FFT.h"
#include "ADS/parallel/Executor.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the points from which the parallel transform splits its work
inline constexpr std::size_t kParallelFFT = std::size_t( 1 ) << 15;

/// the discrete Fourier transform of a in place on the threads of ex; the
/// inverse one, divided by n, if inverse
/// \pre the size of a is a power of 2 (or 0)
template< Executor E >
void fft( E & ex , std::vector< Complex > & a , bool inverse );

/*--------------------------------------------------------------------------*/

namespace detail {

/// D = the transpose of the r x c matrix S by rows, by blocks of 32 x 32
template< Executor E >
void parallelTranspose( E & ex , const Complex * S , Complex * D ,
                        std::size_t r , std::size_t c )
{
 constexpr std::size_t b = 32;
 const std::size_t rb = ( r + b - 1 ) / b , cb = ( c + b - 1 ) / b;
 parallelFor( ex , 0 , rb * cb , [ & ]( std::size_t t ) {
  const std::size_t i0 = ( t / cb ) * b , j0 = ( t % cb ) * b ,
                    i1 = std::min( r , i0 + b ) , j1 = std::min( c , j0 + b );
  for( std::size_t i = i0 ; i < i1 ; ++i )
   for( std::size_t j = j0 ; j < j1 ; ++j )
    D[ j * r + i ] = S[ i * c + j ];
  } , 16 );
 }

}  // end( namespace detail )

template< Executor E >
void fft( E & ex , std::vector< Complex > & a , bool inverse )
{
 const std::size_t n = a.size();
 ADS_PRE( ( n == 0 ) || std::has_single_bit( n ) );
 if( ( ex.concurrency() == 1 ) || ( n < kParallelFFT ) ) {
  fft( a , inverse );
  return;
  }
 const int k = std::countr_zero( n );
 const std::size_t n1 = std::size_t( 1 ) << ( k / 2 ) , n2 = n / n1;
 // the twiddles w^t = Hi[ t >> h ] Lo[ t & ( 2^h - 1 ) ]
 const int h = ( k + 1 ) / 2;
 const std::size_t lo = std::size_t( 1 ) << h , hiN = n >> h;
 const double sgn = inverse ? 1.0 : -1.0;
 std::vector< Complex > Lo( lo ) , Hi( hiN );
 parallelFor( ex , 0 , lo , [ & ]( std::size_t t ) {
  Lo[ t ] = std::polar( 1.0 , sgn * 2 * std::numbers::pi * double( t ) /
                              double( n ) ); } );
 parallelFor( ex , 0 , hiN , [ & ]( std::size_t t ) {
  Hi[ t ] = std::polar( 1.0 , sgn * 2 * std::numbers::pi *
                              double( t << h ) / double( n ) ); } );
 std::vector< Complex > b( n );
 // the columns as rows: b is n2 x n1
 detail::parallelTranspose( ex , a.data() , b.data() , n1 , n2 );
 parallelForBlocks( ex , 0 , n2 , [ & ]( std::size_t r0 , std::size_t r1 ) {
  std::vector< Complex > v( n1 );
  for( std::size_t j2 = r0 ; j2 < r1 ; ++j2 ) {
   Complex * row = b.data() + j2 * n1;
   std::copy( row , row + n1 , v.begin() );
   fft( v , inverse );
   for( std::size_t k1 = 0 ; k1 < n1 ; ++k1 ) {
    const std::size_t t = ( j2 * k1 ) & ( n - 1 );
    row[ k1 ] = v[ k1 ] * ( Hi[ t >> h ] * Lo[ t & ( lo - 1 ) ] );
    }
   }
  } , 1 );
 // back to rows of n2: a is n1 x n2, each row transformed
 detail::parallelTranspose( ex , b.data() , a.data() , n2 , n1 );
 parallelForBlocks( ex , 0 , n1 , [ & ]( std::size_t r0 , std::size_t r1 ) {
  std::vector< Complex > v( n2 );
  for( std::size_t k1 = r0 ; k1 < r1 ; ++k1 ) {
   Complex * row = a.data() + k1 * n2;
   std::copy( row , row + n2 , v.begin() );
   fft( v , inverse );
   std::copy( v.begin() , v.end() , row );
   }
  } , 1 );
 // the output in its order: \hat a[ k1 + n1 k2 ] is a[ k1 ][ k2 ]
 detail::parallelTranspose( ex , a.data() , b.data() , n1 , n2 );
 a.swap( b );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ParallelFFT.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ParallelFFT.h -------------------------*/
/*--------------------------------------------------------------------------*/
