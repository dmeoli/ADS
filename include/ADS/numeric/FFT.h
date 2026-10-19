/*--------------------------------------------------------------------------*/
/*------------------------------- File FFT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fast Fourier transform, beyond the book (4.3.3 and 4.6.4 of Knuth): the
 * discrete transform \f$ \hat a_k = \sum_j a_j \omega^{j k} \f$, \f$ \omega =
 * e^{-2 \pi i / n} \f$, of a vector of n complex numbers, in \f$ O( n \log n )
 * \f$ operations instead of the \f$ O( n^2 ) \f$ of the definition, dft().
 * fft() is the form of Cooley and Tukey by decimation in time, in radix 4:
 * one sweep reads the items in order and writes them, in the order of the
 * reversed bits of their index, to separate arrays of the real and the
 * imaginary parts, doing the first two rounds (three if \f$ \log_2 n \f$ is
 * odd) on the way; then each round of radix 4 does two of radix 2 with 3
 * products instead of 4, on arrays that the compiler vectorizes. Within a
 * block of 2048 items the rounds go one after the other, above it depth
 * first, so that the rounds of a block run while it is in the cache. The
 * powers of \f$ \omega \f$ come from a table where each one is computed
 * alone, its angle taken to \f$ [ 0 , \pi / 4 ] \f$, which keeps the
 * rounding error near that of one product; the table is laid out round by
 * round, so that a round reads it in a row, and serves every size up to
 * the largest one met and both directions, being kept per thread with the
 * arrays of the parts. The transform turns a convolution into a product
 * item by item; convolve() multiplies two polynomials so, the transform of
 * each real vector of n items by one complex transform of n / 2 items, and
 * the real product back by another, and the product of integers of many
 * digits [see BigInteger.h] is a convolution of their digits followed by
 * the carries.
 *
 * The contracts [see Contract.h]: n is a power of 2 for fft(); the inverse
 * transform of the transform gives the vector back, up to the rounding,
 * which the tests check. The functions are defined in FFT.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.3 and 4.6.4
 * \see J. W. Cooley, J. W. Tukey, An algorithm for the machine calculation
 *      of complex Fourier series, Mathematics of Computation 19(90),
 *      297-301, 1965, doi:10.1090/S0025-5718-1965-0178586-1
 * \see C. Van Loan, Computational Frameworks for the Fast Fourier
 *      Transform, SIAM, 1992, doi:10.1137/1.9781611970999
 * \see M. Frigo, S. G. Johnson, The design and implementation of FFTW3,
 *      Proceedings of the IEEE 93(2), 216-231, 2005,
 *      doi:10.1109/JPROC.2004.840301
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

#ifndef __ADS_FFT
 #define __ADS_FFT
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Complex = std::complex< double >;

/// the discrete Fourier transform by its definition, O( n^2 ); the inverse
/// one, divided by n, if inverse
[[nodiscard]] inline std::vector< Complex > dft(
                         const std::vector< Complex > & a , bool inverse );

/// the discrete Fourier transform of a in place, O( n log n ); the inverse
/// one, divided by n, if inverse
/// \pre the size of a is a power of 2 (or 0)
inline void fft( std::vector< Complex > & a , bool inverse );

/// the coefficients of the product of the polynomials p and q
[[nodiscard]] inline std::vector< double > convolve(
                                          const std::vector< double > & p ,
                                          const std::vector< double > & q );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/FFT.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FFT.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File FFT.h -----------------------------*/
/*--------------------------------------------------------------------------*/
