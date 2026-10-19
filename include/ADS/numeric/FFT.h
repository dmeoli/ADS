/*--------------------------------------------------------------------------*/
/*------------------------------- File FFT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fast Fourier transform, beyond the book (4.3.3 and 4.6.4 of Knuth): the
 * discrete transform \f$ \hat a_k = \sum_j a_j \omega^{j k} \f$, \f$ \omega =
 * e^{-2 \pi i / n} \f$, of a vector of n complex numbers, in \f$ O( n \log n )
 * \f$ operations instead of the \f$ O( n^2 ) \f$ of the definition, dft().
 * Up to 64 points fft() is one codelet, the transform unrolled at compile
 * time by decimation in time in radix 4 (2 at the bottom), its items in
 * registers and its powers of \f$ \omega \f$ the constants of a table.
 * Above it fft() is the four steps of Bailey: the n = r m points are the
 * matrix of m rows and r columns, whose columns are transformed, multiplied
 * by \f$ \omega^{j k} \f$ and transformed along the rows, the outputs of
 * which are those of the transform in their order, with no reversal of the
 * bits. Each step works on W columns, or rows, at a time, one in each lane
 * of a vector of W doubles (4 on AVX, 2 on SSE2 and NEON, by the vector
 * extensions of GCC and Clang; arrays of 4 doubles elsewhere, or with
 * ADS_FFT_SIMD defined as 0), the real and the imaginary parts in separate
 * vectors: a codelet on such items does W transforms with the instructions
 * of one, and the blocks of W by W between the two steps are transposed in
 * registers. The transforms longer than 64 points go by the rounds of
 * Stockham, each a codelet of up to 32 points, on 4 columns or rows
 * gathered at a time from lines of the cache that are brought in some rows
 * ahead; the rows have 32 points up to \f$ 2^{15} \f$ points, 64 at \f$
 * 2^{16} \f$ and about \f$ \sqrt n \f$ above, and the blocks of the
 * outputs start where a line of the cache starts. The powers of \f$ \omega
 * \f$ are each computed alone, the angle taken to \f$ [ 0 , \pi / 4 ] \f$,
 * which keeps the rounding error near that of one product; from \f$ 2^{16}
 * \f$ points on, those between the steps are the products of two such
 * tables, of about \f$ n / W \f$ and r items, rather than one of n. The
 * tables and the buffer of the steps are kept per thread and serve every
 * size met and both directions. The transform turns a convolution into a
 * product item by item; convolve() multiplies two polynomials so, the
 * transform of each real vector of n items by one complex transform of n /
 * 2 items, and the real product back by another, and the product of
 * integers of many digits [see BigInteger.h] is a convolution of their
 * digits followed by the carries.
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
 * \see T. G. Stockham, High-speed convolution and correlation, AFIPS
 *      Spring Joint Computer Conference, 229-233, 1966,
 *      doi:10.1145/1464182.1464209
 * \see D. H. Bailey, FFTs in external or hierarchical memory, The Journal
 *      of Supercomputing 4(1), 23-35, 1990, doi:10.1007/BF00162341
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
#include <cstdint>
#include <cstring>
#include <numbers>
#include <utility>
#include <vector>

/// the width of the vectors of fft(), in doubles, by the vector
/// extensions of GCC and Clang: 4 on AVX (also with AVX-512, which on
/// Skylake lowers the clock more than it gains; 8 on request), 2 on SSE2
/// and NEON; 0, the default elsewhere and selectable on any machine, works
/// on arrays of 4 doubles with no vector extension
#ifndef ADS_FFT_SIMD
 #if ! ( defined( __GNUC__ ) || defined( __clang__ ) )
  #define ADS_FFT_SIMD 0
 #elif defined( __AVX__ )
  #define ADS_FFT_SIMD 4
 #elif defined( __SSE2__ ) || defined( __ARM_NEON )
  #define ADS_FFT_SIMD 2
 #else
  #define ADS_FFT_SIMD 0
 #endif
#endif

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
