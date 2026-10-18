/*--------------------------------------------------------------------------*/
/*------------------------------- File FFT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fast Fourier transform, beyond the book (4.3.3 and 4.6.4 of Knuth): the
 * discrete transform \f$ \hat a_k = \sum_j a_j \omega^{j k} \f$, \f$ \omega =
 * e^{-2 \pi i / n} \f$, of a vector of n complex numbers, in \f$ O( n \log n )
 * \f$ operations instead of the \f$ O( n^2 ) \f$ of the definition, dft().
 * fft() is the iterative form of Cooley and Tukey: the items in the order of
 * the reversed bits of their index, then \f$ \log_2 n \f$ rounds of
 * butterflies. The powers of \f$ \omega \f$ come from a table where each one
 * is computed alone, which keeps the rounding error near that of one product;
 * the table is laid out round by round, so that a round reads it in a row, and
 * the table of the last size is kept per thread, since it costs n / 2 sines
 * and cosines. The products are written by their formula, since the operator *
 * of std::complex also handles the infinities and the NaNs at the price of a
 * call. The transform turns a convolution into a product item by item;
 * convolve() multiplies two polynomials so, and the product of integers of
 * many digits [see BigInteger.h] is a convolution of their digits followed by
 * the carries.
 *
 * The contracts [see Contract.h]: n is a power of 2 for fft(); the inverse
 * transform of the transform gives the vector back, a postcondition up to
 * the rounding checked as an audit on dft(). The functions are defined in
 * FFT.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.3 and 4.6.4
 * \see J. W. Cooley, J. W. Tukey, An algorithm for the machine calculation
 *      of complex Fourier series, Mathematics of Computation 19(90),
 *      297-301, 1965, doi:10.1090/S0025-5718-1965-0178586-1
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

#include <complex>
#include <cstddef>
#include <numbers>
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
