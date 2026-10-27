/*--------------------------------------------------------------------------*/
/*------------------------------- File FFT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fast Fourier transforms, beyond the book (sections 4.3.3 and 4.6.4 of
 * Knuth): the discrete transform
 * \f$ \hat a_k = \sum_{j<n} a_j \omega_n^{j k} \f$,
 * \f$ \omega_n = e^{-2 \pi i / n} \f$, of \f$ n \f$ complex numbers in
 * double or single precision, for every \f$ n \f$, in
 * \f$ O( n \log n ) \f$ operations instead of the \f$ n^2 \f$ products of
 * the definition, `dft()`; its inverse
 * \f$ a_j = n^{-1} \sum_k \hat a_k \omega_n^{-j k} \f$; the transforms of
 * real data, of the cosines and of the sines, many transforms at once and
 * those in more dimensions, and the product of polynomials.
 *
 * <b>The codelets.</b> A transform of \f$ N \f$ points, \f$ N \f$ a power
 * of 2 up to 64 or \f$ 2^a 3^b 5^c \f$ up to 25 (only 3 and 5 under
 * `ADS_FFT_LEAN`), is one codelet, unrolled when compiled with its items in
 * registers and its powers of \f$ \omega_N \f$ the constants of a table: by
 * decimation in time in radix 4 for the powers of 2,
 * \f[ \hat a_{k + u N/4} = \sum_{t<4} ( -i )^{t u} \omega_N^{t k}
 *     \hat a^{(t)}_k , \qquad k < N / 4 , \f]
 * \f$ \hat a^{(t)} \f$ the transform of the items
 * \f$ t , t + 4 , \ldots \f$; a size \f$ N = A B \f$ with \f$ A \f$ and
 * \f$ B \f$ coprime (\f$ A \f$ the power of the smallest prime of
 * \f$ N \f$) by the prime-factor algorithm of Good and Thomas, with no
 * power of \f$ \omega \f$ between its two stages: the input
 * \f$ ( a B + b A ) \bmod N \f$ and the output
 * \f$ ( u B ( B^{-1} \bmod A ) + v A ( A^{-1} \bmod B ) ) \bmod N \f$ of
 * indices \f$ a , u < A \f$ and \f$ b , v < B \f$ make the transform one in
 * 2 dimensions,
 * \f[ \hat a_{k( u , v )} = \sum_{b<B} \omega_B^{b v} \sum_{a<A}
 *     \omega_A^{a u} \, a_{( a B + b A ) \bmod N} , \f]
 * the transforms of length \f$ A \f$ of \f$ B \f$ columns and of length
 * \f$ B \f$ of \f$ A \f$ rows; the other sizes in the smallest prime radix
 * \f$ p \f$ of \f$ N \f$. A prime \f$ p \f$ is done by pairs, with
 * \f$ s_j = a_j + a_{p-j} \f$, \f$ d_j = a_j - a_{p-j} \f$ and
 * \f$ h = ( p - 1 ) / 2 \f$:
 * \f[ \hat a_{k} , \hat a_{p-k} = a_0 + \sum_{j \le h} s_j \cos \frac{2 \pi
 *     j k}{p} \mp i \sum_{j \le h} d_j \sin \frac{2 \pi j k}{p} , \f]
 * \f$ 2 h^2 \f$ real products by component instead of \f$ ( p - 1 )^2 \f$;
 * the primes 7, 11, 13 have such codelets, and every other prime up to 101
 * the same sums in loops.
 *
 * <b>The four steps.</b> Above the codelets, \f$ n = r m \f$ points are the
 * matrix of \f$ m \f$ rows and \f$ r \f$ columns, \f$ a_{r i + j} \f$, and,
 * by the factorization of Cooley and Tukey,
 * \f[ \hat a_{k + m q} = \sum_{j<r} \omega_r^{j q} \, \omega_n^{j k}
 *     \sum_{i<m} a_{r i + j} \omega_m^{i k} , \qquad k < m , \; q < r :
 *     \f]
 * the transforms of length \f$ m \f$ of the columns, the products by
 * \f$ \omega_n^{j k} \f$, and those of length \f$ r \f$ of the rows, whose
 * outputs are those of the transform in their order, with no reversal of
 * the digits (the four steps of Bailey, the last one, the transposition,
 * done by the order in which the rows are written). Each step works on
 * \f$ W \f$ columns, or rows, at a time, one in each lane of a vector of
 * \f$ W \f$ numbers (4 doubles or 8 floats on AVX, half on SSE2 and NEON),
 * the real and the imaginary parts in separate vectors: a codelet on such
 * items does \f$ W \f$ transforms with the instructions of one, and the
 * blocks of \f$ W \times W \f$ between the two steps are transposed in
 * registers, as in the short vector codes of Franchetti and Pueschel; the
 * numbers alternated in memory and apart in the registers, as the mixed
 * layout of Popovici, Franchetti and Low. The vectors are those of the
 * extensions of GCC and Clang, the registers of SSE2 and AVX by the
 * intrinsics of x86 with the compilers that have no such extensions
 * (MSVC), or arrays of \f$ W \f$ numbers elsewhere or with `ADS_FFT_SIMD`
 * defined as 0. The columns and the rows longer than the codelets go by the
 * rounds of Stockham, each a codelet of radix \f$ R_t \f$ with its powers of
 * \f$ \omega \f$,
 * \f[ y_{q + s ( R p + j )} = \omega_{m R}^{p j} \sum_{i<R} x_{q + s ( p +
 *     i m )} \, \omega_R^{i j} , \f]
 * as few rounds as the radices of the codelets up to 32 allow, the radices
 * as equal as can be; 4 of them at a time, gathered from lines of the cache
 * brought in some rows ahead. The rows have about 32 points up to
 * \f$ 2^{15} \f$ points, 64 at \f$ 2^{12} \f$ and \f$ 2^{16} \f$ and about
 * \f$ \sqrt n \f$ above: the divisors \f$ r \f$ of \f$ n \f$ by their
 * distance from those lengths, those that make \f$ r \f$ and \f$ m \f$
 * multiples of \f$ W \f$ (otherwise the last block of the lanes is shorter)
 * and whose rows and columns have a codelet first (the sizes
 * \f$ 2^a 3^b 5^c \f$ up to 60 there); up to \f$ 2^{17} \f$ points the first
 * 4 of them are timed when the plan is made and the fastest kept, a choice
 * among a few, as `FFTW_MEASURE` makes among many (`ADS_FFT_TUNE` sets the
 * limit, 0 none). Below \f$ 2 W^2 \f$ points the lanes are half as wide.
 * Up to 1024 points a power of 2 (2048 in single precision), and at 48 and
 * 96, the four steps are unrolled into one function whose columns stay in
 * an array of \f$ n / W \f$ items, the transform of 16 points in 2 codelets
 * of 4; so are the transforms in 2 dimensions of the powers of 2 up to
 * \f$ 32 \times 32 \f$. In double precision on the vectors of AVX the many
 * transforms of 4 to 32 points (those of `fftMany()`, the rows and the
 * columns of `fft2()` and `fftn()`) go two at a time, one in each half of a
 * vector of the numbers alternated, with no transposition (a product by
 * \f$ i \f$ costs one swap of the parts), and a power of 2 \f$ n \f$ of 8 or
 * 16 points by the two halves \f$ E \f$ and \f$ O \f$ of the even and the
 * odd points so made, then \f$ \hat a_k = E_k + \omega_n^k O_k \f$,
 * \f$ \hat a_{k + n/2} = E_k - \omega_n^k O_k \f$. The powers of
 * \f$ \omega \f$ are each computed alone, the angle taken to
 * \f$ [ 0 , \pi / 4 ] \f$ by the symmetries of the circle, in integers, so
 * that each has the error of a sine or a cosine of a small angle; from
 * \f$ 2^{16} \f$ points on, those between the steps are the products of two
 * tables of about \f$ n / W \f$ and \f$ r \f$ items, rather than one of
 * \f$ n \f$.
 *
 * <b>The large primes.</b> A prime \f$ n \f$ above 13 goes by the algorithm
 * of Rader, the transform of the nonzero indices as a cyclic convolution of
 * \f$ n - 1 \f$ points: for a generator \f$ g \f$ of the integers modulo
 * \f$ n \f$,
 * \f[ \hat a_{g^{-q}} = a_0 + \sum_{p < n-1} a_{g^p} \, \omega_n^{g^{p -
 *     q}} , \qquad \hat a_0 = \sum_j a_j , \f]
 * by two transforms of \f$ n - 1 \f$ points (the transform of
 * \f$ \omega_n^{g^{-q}} \f$ made once); a size whose prime above 101 is not
 * \f$ n \f$, or a prime whose \f$ n - 1 \f$ costs more, by that of
 * Bluestein, from \f$ j k = ( j^2 + k^2 - ( k - j )^2 ) / 2 \f$,
 * \f[ \hat a_k = w_k \sum_j ( a_j w_j ) \overline{w_{k-j}} , \qquad w_j =
 *     \omega_{2n}^{j^2} , \f]
 * a convolution of \f$ M \ge 2 n - 1 \f$ points of the form
 * \f$ 2^a 3^b 5^c \f$, by two transforms of \f$ M \f$ (that of
 * \f$ \overline w \f$ made once). The choice among the rounds, Rader and
 * Bluestein is by a cost of about \f$ n \sum_t \log_2 R_t \f$ for the rounds
 * with codelets and \f$ n p / 2 \f$ for a prime radix \f$ p \f$ in loops.
 * The inverse transforms by Rader and Bluestein are the conjugates of the
 * forward ones of the conjugates.
 *
 * <b>Real data, cosines and sines.</b> The transform of \f$ n \f$ real
 * numbers, \f$ n \f$ even, is one complex transform of \f$ h = n / 2 \f$
 * points, \f$ z_j = x_{2j} + i x_{2j+1} \f$, and a pass: with
 * \f$ E_k = ( Z_k + \overline{Z_{h-k}} ) / 2 \f$ and
 * \f$ O_k = ( Z_k - \overline{Z_{h-k}} ) / 2 i \f$ (the transforms of the
 * even and of the odd items), \f$ \hat x_k = E_k + \omega_n^k O_k \f$ and
 * \f$ \hat x_{h-k} = \overline{E_k - \omega_n^k O_k} \f$; the inverse runs
 * the same steps backwards. Up to 16 points the complex transform is one
 * codelet whose outputs stay in registers for the pass; the inverse of 4098
 * to 131072 points on the four steps does its pass in their first step, the
 * points \f$ Z_K \f$ made from \f$ \hat x_K \f$ and \f$ \hat x_{h-K} \f$ as
 * the columns read them (and `convolve()` there the products of the two
 * transforms); elsewhere the pass runs on its own on the numbers
 * alternated, \f$ \mathit{VL} \f$ pairs to a vector with 3 shuffles. The
 * transforms of the cosines and of the sines are those of FFTW (`REDFT00`,
 * `REDFT10`, `REDFT01`, `REDFT11` and `RODFT00` to `RODFT11`), with no
 * normalization; the type 2 by the algorithm of Makhoul, the real transform
 * \f$ V \f$ of \f$ v_j = x_{2j} , v_{n-1-j} = x_{2j+1} \f$ and
 * \f$ y_k = 2 \Re( \omega_{4n}^k V_k ) \f$,
 * \f$ y_{n-k} = -2 \Im( \omega_{4n}^k V_k ) \f$; the type 3 by the same
 * steps backwards; the type 4, for \f$ n \f$ even, by the complex transform
 * of \f$ n / 2 \f$ points of
 * \f$ ( x_{2j} + i x_{n-1-2j} ) \omega_{8n}^{4j+1} \f$ times
 * \f$ \omega_{2n}^k \f$, the outputs \f$ 2 k \f$ and \f$ n - 1 - 2 k \f$ its
 * real part and minus its imaginary part, times 2; the type 1 of both by the
 * real transform of the even, or odd, extension of \f$ 2 ( n - 1 ) \f$, or
 * \f$ 2 ( n + 1 ) \f$, points; the sines of types 2 to 4 by the cosines, the
 * items with the signs alternated or in the reverse order. The type 3 is the
 * inverse of the type 2, and the type 4 of itself, times \f$ 2 n \f$.
 *
 * <b>The cost.</b> A transform of \f$ n = 2^K \f$ points does about
 * \f$ 4.25 n K \f$ real operations, the radix 4 saving 15% of those of
 * radix 2; its data cross the memory in 2 passes (the columns, the rows)
 * plus one per round of Stockham of the columns or rows longer than a
 * codelet, the four steps keeping a buffer of \f$ n + O( \sqrt n ) \f$ items
 * per thread. The plans, the tables and the buffers are kept per thread for
 * each size met, made at the first transform of a size, in \f$ O( n ) \f$
 * evaluations of sines and cosines (\f$ O( n \log n ) \f$ for Rader and
 * Bluestein, whose plans also hold a transform) and, up to \f$ 2^{17} \f$
 * points, the 12 transforms that time the factorizations.
 *
 * <b>The rounding.</b> With powers of \f$ \omega \f$ computed to a relative
 * error \f$ \mu \f$, a transform by \f$ L \f$ rounds of radix 2 has
 * \f$ \| \tilde y - y \|_2 / \| y \|_2 \le L \eta / ( 1 - L \eta ) \f$,
 * \f$ \eta = \mu + \gamma_4 ( \sqrt 2 + \mu ) \f$,
 * \f$ \gamma_4 = 4 u / ( 1 - 4 u ) \f$ and \f$ u \f$ the unit roundoff
 * (Higham, Theorem 24.2); with \f$ \mu \f$ near \f$ u \f$ the bound is about
 * \f$ 6.7 u \log_2 n \f$, while the error of the rounding observed on random
 * data grows as \f$ u \sqrt{ \log n } \f$ (Schatzman). Bluestein adds the
 * error of a convolution of \f$ M \f$ points, a few times larger. The tests
 * check every size up to 300 and the sizes of each plan against the
 * definition in `long double`.
 *
 * The transform turns a convolution into a product item by item;
 * `convolve()` multiplies two polynomials so, by the real transforms of the
 * smallest even size \f$ 2^a 3^b 5^c \f$ that holds their product, or of the
 * power of 2 if that size is not below \f$ 3 / 4 \f$ of it, and the product
 * of integers of many digits [see `BigInteger.h`] is a convolution of their
 * digits followed by the carries.
 *
 * The contracts [see `Contract.h`]: the sizes of the arguments of
 * `irfft()`, the types of `dct()` and `dst()`; the inverse transform of the
 * transform gives the vector back, up to the rounding, which the tests
 * check. The functions are defined in `FFT.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.3 and 4.6.4
 * \see J. W. Cooley, J. W. Tukey, An algorithm for the machine calculation
 *      of complex Fourier series, Mathematics of Computation 19(90),
 *      297-301, 1965, doi:10.1090/S0025-5718-1965-0178586-1
 * \see I. J. Good, The interaction algorithm and practical Fourier
 *      analysis, Journal of the Royal Statistical Society, Series B 20(2),
 *      361-372, 1958, doi:10.1111/j.2517-6161.1958.tb00300.x
 * \see L. H. Thomas, Using computers to solve problems in physics, in W.
 *      F. Freiberger, W. Prager (eds.), Applications of Digital Computers,
 *      Ginn, Boston, 1963 (no doi)
 * \see R. C. Singleton, An algorithm for computing the mixed radix fast
 *      Fourier transform, IEEE Transactions on Audio and Electroacoustics
 *      17(2), 93-103, 1969, doi:10.1109/TAU.1969.1162042
 * \see M. Frigo, S. G. Johnson, The design and implementation of FFTW3,
 *      Proceedings of the IEEE 93(2), 216-231, 2005,
 *      doi:10.1109/JPROC.2004.840301
 * \see D. H. Bailey, FFTs in external or hierarchical memory, The Journal
 *      of Supercomputing 4(1), 23-35, 1990, doi:10.1007/BF00162341
 * \see F. Franchetti, M. Pueschel, Short vector code generation for the
 *      discrete Fourier transform, Proceedings of the International
 *      Parallel and Distributed Processing Symposium (IPDPS), 2003,
 *      doi:10.1109/IPDPS.2003.1213153
 * \see D. T. Popovici, F. Franchetti, T. M. Low, Mixed data layout kernels
 *      for vectorized complex arithmetic, IEEE High Performance Extreme
 *      Computing Conference (HPEC), 1-7, 2017,
 *      doi:10.1109/HPEC.2017.8091024
 * \see T. G. Stockham, High-speed convolution and correlation, AFIPS
 *      Spring Joint Computer Conference, 229-233, 1966,
 *      doi:10.1145/1464182.1464209
 * \see C. Van Loan, Computational Frameworks for the Fast Fourier
 *      Transform, SIAM, 1992, doi:10.1137/1.9781611970999
 * \see C. M. Rader, Discrete Fourier transforms when the number of data
 *      samples is prime, Proceedings of the IEEE 56(6), 1107-1108, 1968,
 *      doi:10.1109/PROC.1968.6477
 * \see L. I. Bluestein, A linear filtering approach to the computation of
 *      discrete Fourier transform, IEEE Transactions on Audio and
 *      Electroacoustics 18(4), 451-455, 1970, doi:10.1109/TAU.1970.1162132
 * \see J. W. Cooley, P. A. W. Lewis, P. D. Welch, The fast Fourier
 *      transform algorithm: programming considerations in the calculation
 *      of sine, cosine and Laplace transforms, Journal of Sound and
 *      Vibration 12(3), 315-337, 1970, doi:10.1016/0022-460X(70)90075-1
 * \see H. V. Sorensen, D. L. Jones, M. T. Heideman, C. S. Burrus,
 *      Real-valued fast Fourier transform algorithms, IEEE Transactions on
 *      Acoustics, Speech, and Signal Processing 35(6), 849-863, 1987,
 *      doi:10.1109/TASSP.1987.1165220
 * \see J. Makhoul, A fast cosine transform in one and two dimensions, IEEE
 *      Transactions on Acoustics, Speech, and Signal Processing 28(1),
 *      27-34, 1980, doi:10.1109/TASSP.1980.1163351
 * \see Z. Wang, Fast algorithms for the discrete W transform and for the
 *      discrete Fourier transform, IEEE Transactions on Acoustics, Speech,
 *      and Signal Processing 32(4), 803-816, 1984,
 *      doi:10.1109/TASSP.1984.1164399
 * \see N. J. Higham, Accuracy and Stability of Numerical Algorithms, 2nd
 *      ed., SIAM, 2002, chapter 24, doi:10.1137/1.9780898718027
 * \see J. C. Schatzman, Accuracy of the discrete Fourier transform and the
 *      fast Fourier transform, SIAM Journal on Scientific Computing 17(5),
 *      1150-1166, 1996, doi:10.1137/S1064827593247023
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

#ifndef __ADS_FFT
 #define __ADS_FFT
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <numbers>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

/// the width of the vectors of fft(), in doubles: 4 on AVX (also with
/// AVX-512, which on Skylake lowers the clock more than it gains; 8 on
/// request), 2 on SSE2 and NEON; 0, the default elsewhere and selectable
/// on any machine, works on arrays of 4 doubles with no vector instruction
#ifndef ADS_FFT_SIMD
 #if defined( __GNUC__ ) || defined( __clang__ )
  #if defined( __AVX__ )
   #define ADS_FFT_SIMD 4
  #elif defined( __SSE2__ ) || defined( __ARM_NEON )
   #define ADS_FFT_SIMD 2
  #else
   #define ADS_FFT_SIMD 0
  #endif
 #elif defined( _MSC_VER ) && ( defined( _M_X64 ) || defined( _M_IX86 ) )
  #if defined( __AVX__ )
   #define ADS_FFT_SIMD 4
  #elif defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
   #define ADS_FFT_SIMD 2
  #else
   #define ADS_FFT_SIMD 0
  #endif
 #else
  #define ADS_FFT_SIMD 0
 #endif
#endif

/// 1 if the vectors are the registers of SSE2 and AVX by the intrinsics of
/// x86, the default of the compilers with no vector extensions (MSVC) and
/// selectable with GCC and Clang on x86; 0 for the vector extensions
#ifndef ADS_FFT_INTRINSICS
 #if ( ADS_FFT_SIMD > 1 ) && ! ( defined( __GNUC__ ) || defined( __clang__ ) )
  #define ADS_FFT_INTRINSICS 1
 #else
  #define ADS_FFT_INTRINSICS 0
 #endif
#endif

/// 1 to compile fewer codelets (the sizes 3 and 5 alone beside the powers
/// of 2, the unrolled four steps up to 256 points), the default of the
/// compilers other than GCC and Clang (MSVC), whose time on the unrolled
/// codelets grows faster; 0 for all of them
#ifndef ADS_FFT_LEAN
 #if defined( __GNUC__ ) || defined( __clang__ )
  #define ADS_FFT_LEAN 0
 #else
  #define ADS_FFT_LEAN 1
 #endif
#endif

#if ADS_FFT_INTRINSICS
 #if ADS_FFT_SIMD > 4
  #error "ADS_FFT_INTRINSICS has the registers of SSE2 and AVX only"
 #endif
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Complex = std::complex< double >;   ///< a complex number in double

/// the discrete Fourier transform by its definition, \f$ O( n^2 ) \f$; the
/// inverse one, divided by \f$ n \f$, if `inverse`
///
/// In pseudocode (section 4.6.4 of Knuth, beyond the book),
/// \f$ \sigma = +1 \f$ for the inverse and \f$ -1 \f$ otherwise:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dft}( a , \mathit{inverse} ) \cr
/// 2 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad \hat a_k \gets \sum_{j<n} a_j e^{\sigma 2 \pi i ( j k \bmod n )
///     / n} \cr
/// 4 & \qquad \textbf{if}\ \mathit{inverse}\ \textbf{then}\ \hat a_k \gets
///     \hat a_k / n \cr
/// 5 & \quad \textbf{return}\ \hat a
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< Complex > dft(
                         const std::vector< Complex > & a , bool inverse );

/// the discrete Fourier transform of the \f$ n \f$ points of \f$ a \f$ in
/// place, in double or single precision, \f$ O( n \log n ) \f$ for every
/// \f$ n \f$; the inverse one, divided by \f$ n \f$, if `inverse`
///
/// In pseudocode (Cooley and Tukey, Bailey, Rader and Bluestein, beyond the
/// book), the choice of the plan made once per size [see the file]:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{fft}( a , n , \mathit{inverse} ) \cr
/// 2 & \quad \textbf{if}\ n \le 1\ \textbf{then return} \cr
/// 3 & \quad \textbf{if}\ n \text{ has a codelet}\ \textbf{then}\ \text{run }
///     \text{it};\ \textbf{return} \cr
/// 4 & \quad \textbf{if}\ \text{the plan of } n \text{ is the four steps, }
///     n = r m\ \textbf{then} \cr
/// 5 & \qquad \text{transform the } r \text{ columns of length } m \cr
/// 6 & \qquad \text{multiply } a_{r i + j} \text{ by } \omega_n^{j i}
///     \text{, and transform the } m \text{ rows of length } r \cr
/// 7 & \quad \textbf{else if}\ \text{it is Rader}\ \textbf{then}\ \text{a }
///     \text{cyclic convolution of } n - 1 \text{ points} \cr
/// 8 & \quad \textbf{else if}\ \text{it is Bluestein}\ \textbf{then}
///     \ \text{a convolution of } M \ge 2 n - 1 \text{ points} \cr
/// 9 & \quad \textbf{else}\ \text{the rounds of radix } p \text{ in loops}
/// \end{array}}
/// \f]
/// where the inverse of Rader and of Bluestein conjugates the input and the
/// output and divides by \f$ n \f$.
template< class T >
inline void fft( std::complex< T > * a , std::size_t n , bool inverse );

/// the transform of `fft()` above on the whole vector \f$ a \f$
///
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{fft}( a , \mathit{inverse} ) \cr
/// 2 & \quad \mathrm{fft}( a , |a| , \mathit{inverse} )
/// \end{array}}
/// \f]
template< class T >
inline void fft( std::vector< std::complex< T > > & a , bool inverse );

/// the transforms of \f$ \mathit{howmany} \f$ vectors of \f$ n \f$ points
/// each, the vector \f$ t \f$ from \f$ a + t n \f$ on, in place
///
/// In pseudocode (beyond the book), \f$ W \f$ vectors at a time in the lanes
/// of the vectors of the processor:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{fftMany}( a , n , \mathit{howmany} ,
///     \mathit{inverse} ) \cr
/// 2 & \quad \textbf{for}\ t \gets 0\ \textbf{to}\ \mathit{howmany} - 1
///     \ \textbf{do}\ \mathrm{fft}( a + t n , n , \mathit{inverse} )
/// \end{array}}
/// \f]
template< class T >
inline void fftMany( std::complex< T > * a , std::size_t n ,
                     std::size_t howmany , bool inverse );

/// the transform in \f$ d = |\mathit{dims}| \f$ dimensions of the array
/// \f$ a \f$ of extents \f$ \mathit{dims} \f$, in the order of the rows (the
/// last index the fastest), in place; the inverse one divided by the number
/// of points
///
/// In pseudocode (the transform by dimensions, beyond the book), since the
/// transform in \f$ d \f$ dimensions is the product of those along each:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{fftn}( a , \mathit{dims} ,
///     \mathit{inverse} ) \cr
/// 2 & \quad \textbf{for}\ k \gets d - 1\ \textbf{downto}\ 0\ \textbf{do}
///     \cr
/// 3 & \qquad \text{transform each line of } a \text{ along the index }
///     k \text{, of } \mathit{dims}_k \text{ points}
/// \end{array}}
/// \f]
template< class T >
inline void fftn( std::complex< T > * a ,
                  const std::vector< std::size_t > & dims , bool inverse );

/// the transform in 2 dimensions of the matrix \f$ a \f$ of
/// \f$ \mathit{rows} \times \mathit{cols} \f$ points, by rows, in place
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{fft2}( a , \mathit{rows} , \mathit{cols}
///     , \mathit{inverse} ) \cr
/// 2 & \quad \textbf{if}\ \text{the size has an unrolled transform}
///     \ \textbf{then}\ \text{run it};\ \textbf{return} \cr
/// 3 & \quad \mathrm{fftn}( a , ( \mathit{rows} , \mathit{cols} ) ,
///     \mathit{inverse} )
/// \end{array}}
/// \f]
template< class T >
inline void fft2( std::complex< T > * a , std::size_t rows ,
                  std::size_t cols , bool inverse );

/// the transform of the \f$ n \f$ real numbers \f$ x \f$, its
/// \f$ n / 2 + 1 \f$ outputs \f$ X_k \f$, \f$ k \le n / 2 \f$ (the others
/// are \f$ \overline{X_{n-k}} \f$); \f$ X \f$ may be where \f$ x \f$ is, if it
/// has room for \f$ n + 2 \f$ numbers
///
/// In pseudocode (Cooley, Lewis and Welch, beyond the book), for \f$ n \f$
/// even, \f$ h = n / 2 \f$ and \f$ \overline{Z_h} = Z_0 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{rfft}( x , n , X ) \cr
/// 2 & \quad \textbf{if}\ n \text{ is odd}\ \textbf{then}\ X \gets \text{the }
///     \text{first } \lfloor n / 2 \rfloor + 1 \text{ outputs of }
///     \mathrm{fft}( x ) ;\ \textbf{return} \cr
/// 3 & \quad Z \gets \mathrm{fft}( z ) \text{, } z_j = x_{2j} + i x_{2j+1}
///     \text{, } j < h \cr
/// 4 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ h\ \textbf{do} \cr
/// 5 & \qquad E_k \gets ( Z_k + \overline{Z_{h-k}} ) / 2;\ O_k \gets ( Z_k -
///     \overline{Z_{h-k}} ) / 2 i \cr
/// 6 & \qquad X_k \gets E_k + \omega_n^k O_k
/// \end{array}}
/// \f]
template< class T >
inline void rfft( const T * x , std::size_t n , std::complex< T > * X );

/// the outputs of `rfft()` above as a new vector of
/// \f$ \lfloor n / 2 \rfloor + 1 \f$ numbers, empty if \f$ x \f$ is
template< class T >
[[nodiscard]] inline std::vector< std::complex< T > > rfft(
                                                const std::vector< T > & x );

/// the \f$ n \f$ real numbers \f$ x \f$ whose transform has the outputs
/// \f$ X_0 , \ldots , X_{n/2} \f$, i.e., the inverse transform divided by
/// \f$ n \f$; \f$ x \f$ may be where \f$ X \f$ is
///
/// In pseudocode (beyond the book), the steps of `rfft()` backwards, for
/// \f$ n \f$ even and \f$ h = n / 2 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{irfft}( X , n , x ) \cr
/// 2 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ h - 1\ \textbf{do} \cr
/// 3 & \qquad E_k \gets ( X_k + \overline{X_{h-k}} ) / 2;\ O_k \gets ( X_k -
///     \overline{X_{h-k}} ) \omega_n^{-k} / 2 \cr
/// 4 & \qquad Z_k \gets E_k + i O_k \cr
/// 5 & \quad z \gets \text{the inverse } \mathrm{fft}( Z ) \cr
/// 6 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ h - 1\ \textbf{do}
///     \ x_{2j} \gets \Re z_j;\ x_{2j+1} \gets \Im z_j
/// \end{array}}
/// \f]
/// \pre \f$ X \f$ has \f$ n / 2 + 1 \f$ items (0 if \f$ n \f$ is 0)
template< class T >
inline void irfft( const std::complex< T > * X , std::size_t n , T * x );

/// the numbers of `irfft()` above as a new vector of \f$ n \f$ numbers
/// \pre \f$ X \f$ has \f$ \lfloor n / 2 \rfloor + 1 \f$ items (0 if \f$ n \f$
///      is 0)
template< class T >
[[nodiscard]] inline std::vector< T > irfft(
                                  const std::vector< std::complex< T > > & X ,
                                  std::size_t n );

/// the transform of the cosines of type 1 to 4 of the \f$ n \f$ real numbers
/// of \f$ x \f$ in place, as those of FFTW (`REDFT00`, `REDFT10`, `REDFT01`,
/// `REDFT11`), with no normalization: the type 3 is the inverse of the type
/// 2, and the type 4 of itself, times \f$ 2 n \f$ (\f$ 2 ( n - 1 ) \f$ for
/// the type 1)
///
/// In pseudocode (Makhoul, beyond the book), the type 2 by a real transform:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dct}( x , n , 2 ) \cr
/// 2 & \quad v_j \gets x_{2j};\ v_{n-1-j} \gets x_{2j+1}\quad
///     \triangleright\ j < n / 2 \cr
/// 3 & \quad V \gets \mathrm{rfft}( v ) \cr
/// 4 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ \lfloor n / 2 \rfloor
///     \ \textbf{do} \cr
/// 5 & \qquad x_k \gets 2 \Re( \omega_{4n}^k V_k );\ x_{n-k} \gets -2 \Im(
///     \omega_{4n}^k V_k )\quad \triangleright\ \text{the latter for } k > 0
/// \end{array}}
/// \f]
/// and the type 3 runs the same steps backwards, the type 4 is a complex
/// transform of \f$ n / 2 \f$ points and the type 1 a real transform of the
/// even extension of \f$ 2 ( n - 1 ) \f$ points [see the file].
/// \pre \f$ 1 \le \mathit{type} \le 4 \f$, and \f$ n \ne 1 \f$ for the type 1
template< class T >
inline void dct( T * x , std::size_t n , unsigned type );

/// the transform of `dct()` above on the whole vector \f$ x \f$
/// \pre \f$ 1 \le \mathit{type} \le 4 \f$, and \f$ |x| \ne 1 \f$ for the
///      type 1
template< class T >
inline void dct( std::vector< T > & x , unsigned type );

/// the transform of the sines of type 1 to 4 of the \f$ n \f$ real numbers
/// of \f$ x \f$ in place, as those of FFTW (`RODFT00`, `RODFT10`, `RODFT01`,
/// `RODFT11`)
///
/// In pseudocode (beyond the book), the types 2 to 4 by the cosines and the
/// type 1 by the odd extension of \f$ 2 ( n + 1 ) \f$ points:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dst}( x , n , \mathit{type} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{type} = 2\ \textbf{then} \cr
/// 3 & \qquad x_j \gets ( -1 )^j x_j;\ \mathrm{dct}( x , n , 2 );
///     \ \text{reverse } x \cr
/// 4 & \quad \textbf{else if}\ \mathit{type} \in \{ 3 , 4 \}\ \textbf{then}
///     \cr
/// 5 & \qquad \text{reverse } x;\ \mathrm{dct}( x , n , \mathit{type} );\ x_k
///     \gets ( -1 )^k x_k \cr
/// 6 & \quad \textbf{else}\ \text{the real transform of the odd extension}
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le \mathit{type} \le 4 \f$
template< class T >
inline void dst( T * x , std::size_t n , unsigned type );

/// the transform of `dst()` above on the whole vector \f$ x \f$
/// \pre \f$ 1 \le \mathit{type} \le 4 \f$
template< class T >
inline void dst( std::vector< T > & x , unsigned type );

/// the coefficients of the product of the polynomials \f$ p \f$ and
/// \f$ q \f$, by the real transforms
///
/// In pseudocode (the convolution theorem, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{convolve}( p , q ) \cr
/// 2 & \quad m \gets |p| + |q| - 1;\ n \gets \text{the smallest even }
///     2^a 3^b 5^c \ge m \text{, or a power of 2} \cr
/// 3 & \quad P \gets \mathrm{rfft}( p \text{ padded to } n );\ Q \gets
///     \mathrm{rfft}( q \text{ padded to } n ) \cr
/// 4 & \quad r \gets \mathrm{irfft}( ( P_k Q_k )_{k \le n/2} , n ) \cr
/// 5 & \quad \textbf{return}\ r_0 , \ldots , r_{m-1}
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< double > convolve(
                                          const std::vector< double > & p ,
                                          const std::vector< double > & q );

/// the product of `convolve()` above in single precision, or double for
/// vectors of doubles
template< class T >
[[nodiscard]] inline std::vector< T > convolve( const std::vector< T > & p ,
                                                const std::vector< T > & q );

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
