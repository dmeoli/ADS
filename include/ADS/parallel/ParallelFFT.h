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
 * transforms of the rows, and the transposition of the result. The
 * transforms of length \f$ n_1 \f$ or \f$ n_2 \f$ (about \f$ \sqrt n \f$,
 * in the cache of each core) are independent, each the sequential fft()
 * of a row copied into a vector of its thread, and they run in parallel
 * loops; the columns become rows, and the rows columns, by three parallel
 * transpositions through a buffer of n points [see Scratch.h], each in
 * tasks of 16 rows by 256 columns and tiles of 16 x 16. A transposition
 * that walks whole columns instead, e.g., gathering 8 columns at a time
 * from the rows, crosses a page at each row (a row of \f$ 2^{11} \f$
 * points is 32 KB) and misses the TLB at each one: on the benchmark,
 * gathering and putting back the columns of \f$ 2^{22} \f$ points takes
 * longer than the whole sequential transform.
 * The work is that of the sequential transform plus the twiddles and 4
 * passes of moves (3 transpositions and the copy of the result back),
 * \f$ O( n \log n / p ) \f$ time on p threads, the transform of a row
 * not split.
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
 *         Università di Pisa \n
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
#include "ADS/parallel/Scratch.h"

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

/// D = the transpose of the r x c matrix S by rows, in tasks of 16 rows of
/// S by 256 of its columns, each by tiles of 16 x 16: a task reads 16 rows
/// and writes 256, so that its pages fit the TLB whatever the distance of
/// two rows (a column of a large matrix crosses a page at each row)
template< Executor E >
void tiledTranspose( E & ex , const Complex * S , Complex * D ,
                     std::size_t r , std::size_t c )
{
 constexpr std::size_t R = 16 , Cn = 256;
 const std::size_t rt = ( r + R - 1 ) / R , ct = ( c + Cn - 1 ) / Cn;
 parallelFor( ex , 0 , rt * ct , [ & ]( std::size_t task ) {
  const std::size_t i0 = ( task / ct ) * R , i1 = std::min( r , i0 + R ) ,
                    c0 = ( task % ct ) * Cn , c1 = std::min( c , c0 + Cn );
  for( std::size_t j0 = c0 ; j0 < c1 ; j0 += R ) {
   const std::size_t j1 = std::min( c1 , j0 + R );
   for( std::size_t i = i0 ; i < i1 ; ++i )
    for( std::size_t j = j0 ; j < j1 ; ++j )
     D[ j * r + i ] = S[ i * c + j ];
   }
  } , 1 );
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
 detail::ScratchBuffer< Complex > room( n );
 Complex * const b = room.data();
 // the columns as rows (b is n2 x n1), transformed and multiplied by
 // their twiddles; back to rows of n2 (in a), transformed; the result
 // transposed into b and copied to a
 detail::tiledTranspose( ex , a.data() , b , n1 , n2 );
 parallelForBlocks( ex , 0 , n2 , [ & ]( std::size_t r0 , std::size_t r1 ) {
  std::vector< Complex > v( n1 );
  for( std::size_t j2 = r0 ; j2 < r1 ; ++j2 ) {
   Complex * row = b + j2 * n1;
   std::copy( row , row + n1 , v.begin() );
   fft( v , inverse );
   for( std::size_t k1 = 0 ; k1 < n1 ; ++k1 ) {
    const std::size_t t = ( j2 * k1 ) & ( n - 1 );
    row[ k1 ] = v[ k1 ] * ( Hi[ t >> h ] * Lo[ t & ( lo - 1 ) ] );
    }
   }
  } , 1 );
 detail::tiledTranspose( ex , b , a.data() , n2 , n1 );
 parallelForBlocks( ex , 0 , n1 , [ & ]( std::size_t r0 , std::size_t r1 ) {
  std::vector< Complex > v( n2 );
  for( std::size_t k1 = r0 ; k1 < r1 ; ++k1 ) {
   Complex * row = a.data() + k1 * n2;
   std::copy( row , row + n2 , v.begin() );
   fft( v , inverse );
   std::copy( v.begin() , v.end() , row );
   }
  } , 1 );
 // \hat a[ k1 + n1 k2 ] is the entry k2 of the row k1
 detail::tiledTranspose( ex , a.data() , b , n1 , n2 );
 parallelForBlocks( ex , 0 , n , [ & ]( std::size_t lo2 , std::size_t hi2 ) {
  std::copy( b + lo2 , b + hi2 , a.data() + lo2 ); } , 16384 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ParallelFFT.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ParallelFFT.h -------------------------*/
/*--------------------------------------------------------------------------*/
