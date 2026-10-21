/*--------------------------------------------------------------------------*/
/*------------------------------- File FFT.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fast Fourier transforms, beyond the book (4.3.3 and 4.6.4 of Knuth):
 * the discrete transform \f$ \hat a_k = \sum_{j<n} a_j \omega_n^{j k} \f$,
 * \f$ \omega_n = e^{-2 \pi i / n} \f$, of n complex numbers in double or
 * single precision, for every n, in \f$ O( n \log n ) \f$ operations
 * instead of the \f$ n^2 \f$ products of the definition, dft(); its inverse
 * \f$ a_j = n^{-1} \sum_k \hat a_k \omega_n^{-j k} \f$; the transforms of
 * real data, of the cosines and of the sines, many transforms at once and
 * those in more dimensions, and the product of polynomials.
 *
 * <b>The codelets.</b> A transform of N points, N a power of 2 up to 64 or
 * \f$ 2^a 3^b 5^c \f$ up to 25 (only 3 and 5 under ADS_FFT_LEAN), is one
 * codelet, unrolled when compiled with its items in registers and its
 * powers of \f$ \omega_N \f$ the constants of a table: by decimation in
 * time in radix 4 for the powers of 2,
 * \f[ \hat a_{k + u N/4} = \sum_{t<4} ( -i )^{t u} \omega_N^{t k}
 *     \hat a^{(t)}_k , \qquad k < N / 4 , \f]
 * \f$ \hat a^{(t)} \f$ the transform of the items \f$ t , t + 4 , \ldots
 * \f$; a size N = A B with A and B coprime (A the power of the smallest
 * prime of N) by the prime-factor algorithm of Good and Thomas, with no
 * power of \f$ \omega \f$ between its two stages: the input \f$ ( a B +
 * b A ) \bmod N \f$ and the output \f$ ( u B ( B^{-1} \bmod A ) + v A (
 * A^{-1} \bmod B ) ) \bmod N \f$ of indices \f$ a , u < A \f$ and \f$ b ,
 * v < B \f$ make the transform one in 2 dimensions,
 * \f[ \hat a_{k( u , v )} = \sum_{b<B} \omega_B^{b v} \sum_{a<A}
 *     \omega_A^{a u} \, a_{( a B + b A ) \bmod N} , \f]
 * the transforms of length A of B columns and of length B of A rows; the
 * other sizes in the smallest prime radix p of N. A prime p is done
 * by pairs, with \f$ s_j = a_j + a_{p-j} \f$, \f$ d_j = a_j - a_{p-j} \f$
 * and \f$ h = ( p - 1 ) / 2 \f$:
 * \f[ \hat a_{k} , \hat a_{p-k} = a_0 + \sum_{j \le h} s_j \cos \frac{2 \pi
 *     j k}{p} \mp i \sum_{j \le h} d_j \sin \frac{2 \pi j k}{p} , \f]
 * \f$ 2 h^2 \f$ real products by component instead of \f$ ( p - 1 )^2 \f$;
 * the primes 7 , 11 , 13 have such codelets, and every other prime up to
 * 101 the same sums in loops.
 *
 * <b>The four steps.</b> Above the codelets, n = r m points are the matrix
 * of m rows and r columns, \f$ a_{r i + j} \f$, and, by the factorization
 * of Cooley and Tukey,
 * \f[ \hat a_{k + m q} = \sum_{j<r} \omega_r^{j q} \, \omega_n^{j k}
 *     \sum_{i<m} a_{r i + j} \omega_m^{i k} , \qquad k < m , \; q < r :
 *     \f]
 * the transforms of length m of the columns, the products by \f$
 * \omega_n^{j k} \f$, and those of length r of the rows, whose outputs are
 * those of the transform in their order, with no reversal of the digits
 * (the four steps of Bailey, the last one, the transposition, done by the
 * order in which the rows are written). Each step works on W columns, or
 * rows, at a time, one in each lane of a vector of W numbers (4 doubles
 * or 8 floats on AVX, half on SSE2 and NEON), the real and the imaginary
 * parts in separate vectors: a codelet on such items does W transforms
 * with the instructions of one, and the blocks of W by W between the two
 * steps are transposed in registers, as in the short vector codes of
 * Franchetti and Pueschel; the numbers alternated in memory and apart in
 * the registers, as the mixed layout of Popovici, Franchetti and Low. The
 * vectors are those of the extensions of GCC and Clang, the registers of
 * SSE2 and AVX by the intrinsics of x86 with the compilers that have no
 * such extensions (MSVC), or arrays of W numbers elsewhere or with
 * ADS_FFT_SIMD defined as 0. The columns and
 * the rows longer than the codelets go by the rounds of Stockham, each a
 * codelet of radix \f$ R_t \f$ with its powers of \f$ \omega \f$,
 * \f[ y_{q + s ( R p + j )} = \omega_{m R}^{p j} \sum_{i<R} x_{q + s ( p +
 *     i m )} \, \omega_R^{i j} , \f]
 * as few rounds as the radices of the codelets up to 32 allow, the
 * radices as equal as can be; 4 of them at a time, gathered from lines of
 * the cache brought in some rows ahead. The rows have about 32 points up
 * to \f$ 2^{15} \f$ points, 64 at \f$ 2^{12} \f$ and \f$ 2^{16} \f$ and
 * about \f$ \sqrt n \f$ above: the divisors r of n by their distance from
 * those lengths, those that make r and m multiples of W (otherwise the
 * last block of the lanes is shorter) and whose rows and columns have a
 * codelet first (the sizes \f$ 2^a 3^b 5^c \f$ up to 60 there); up to
 * \f$ 2^{17} \f$ points the first 4 of them are timed when the plan is
 * made and the fastest kept, a choice among a few, as FFTW_MEASURE makes
 * among many (ADS_FFT_TUNE sets the limit, 0 none). Below \f$ 2 W^2 \f$
 * points the lanes are half as wide. Up to 1024 points a power of 2 (2048
 * in single precision), and at 48 and 96, the four steps are unrolled
 * into one function whose columns stay in an array of n / W items, the
 * transform of 16 points in 2 codelets of 4; so are the transforms in 2
 * dimensions of the powers of 2 up to 32 x 32. In double precision on the
 * vectors of AVX the many transforms of 4 to 32 points (those of
 * fftMany(), the rows and the columns of fft2() and fftn()) go two at a
 * time, one in each half of a vector of the numbers alternated, with no
 * transposition (a product by i costs one swap of the parts), and a power
 * of 2 n of 8 or 16 points by the two halves \f$ E \f$ and \f$ O \f$ of the
 * even and the odd points so made, then \f$ \hat a_k = E_k + \omega_n^k
 * O_k \f$, \f$ \hat a_{k + n/2} = E_k - \omega_n^k O_k \f$. The powers
 * of \f$ \omega \f$ are each computed alone, the angle taken to \f$ [ 0 ,
 * \pi / 4 ] \f$ by the symmetries of the circle, in integers, so that
 * each has the error of a sine or a cosine of a small angle; from \f$
 * 2^{16} \f$ points on, those between the steps are the products of two
 * tables of about \f$ n / W \f$ and r items, rather than one of n.
 *
 * <b>The large primes.</b> A prime n above 13 goes by the algorithm of
 * Rader, the transform of the nonzero indices as a cyclic convolution of
 * n - 1 points: for a generator g of the integers mod n,
 * \f[ \hat a_{g^{-q}} = a_0 + \sum_{p < n-1} a_{g^p} \, \omega_n^{g^{p -
 *     q}} , \qquad \hat a_0 = \sum_j a_j , \f]
 * by two transforms of n - 1 points (the transform of \f$ \omega_n^{g^{-q}}
 * \f$ made once); a size whose prime above 101 is not n, or a prime whose
 * n - 1 costs more, by that of Bluestein, from \f$ j k = ( j^2 + k^2 - ( k
 * - j )^2 ) / 2 \f$,
 * \f[ \hat a_k = w_k \sum_j ( a_j w_j ) \overline{w_{k-j}} , \qquad w_j =
 *     \omega_{2n}^{j^2} , \f]
 * a convolution of \f$ M \ge 2 n - 1 \f$ points of the form \f$ 2^a 3^b
 * 5^c \f$, by two transforms of M (that of \f$ \overline w \f$ made
 * once). The choice among the rounds, Rader and Bluestein is by a cost of
 * about \f$ n \sum_t \log_2 R_t \f$ for the rounds with codelets and \f$ n
 * p / 2 \f$ for a prime radix p in loops. The inverse transforms by Rader
 * and Bluestein are the conjugates of the forward ones of the conjugates.
 *
 * <b>Real data, cosines and sines.</b> The transform of n real numbers, n
 * even, is one complex transform of h = n / 2 points, \f$ z_j = x_{2j} + i
 * x_{2j+1} \f$, and a pass: with \f$ E_k = ( Z_k + \overline{Z_{h-k}} ) / 2
 * \f$ and \f$ O_k = ( Z_k - \overline{Z_{h-k}} ) / 2 i \f$ (the transforms
 * of the even and of the odd items), \f$ \hat x_k = E_k + \omega_n^k O_k
 * \f$ and \f$ \hat x_{h-k} = \overline{E_k - \omega_n^k O_k} \f$; the
 * inverse runs the same steps backwards. Up to 16 points the complex
 * transform is one codelet whose outputs stay in registers for the pass;
 * above 2048 points, on the four steps, the pass is done by their last
 * step: the block of the rows k and that of their mirrors m - k are
 * transformed together, so that \f$ Z_K \f$ and \f$ Z_{h-K} \f$, \f$ K =
 * k + m q \f$, \f$ h - K = ( m - k ) + m ( r - 1 - q ) \f$, are in the same
 * lane of the two and the outputs leave the registers once, as the
 * inverse does its pass in the first step, on the points as the columns
 * read them; elsewhere the pass runs on the numbers alternated, VL pairs
 * to a vector with 3 shuffles. The transforms of the cosines and of the
 * sines are those of FFTW (REDFT00, REDFT10,
 * REDFT01, REDFT11 and RODFT00 to RODFT11), with no normalization; the
 * type 2 by the algorithm of Makhoul, the real transform V of \f$ v_j =
 * x_{2j} , v_{n-1-j} = x_{2j+1} \f$ and \f$ y_k = 2 \Re( \omega_{4n}^k V_k
 * ) , y_{n-k} = -2 \Im( \omega_{4n}^k V_k ) \f$; the type 3 by the same
 * steps backwards; the type 4, for n even, by the complex transform of n
 * / 2 points of \f$ ( x_{2j} + i x_{n-1-2j} ) \omega_{8n}^{4j+1} \f$ times
 * \f$ \omega_{2n}^k \f$, the outputs 2 k and n - 1 - 2 k its real part
 * and minus its imaginary part, times 2; the type 1 of both by the real
 * transform of the even, or odd, extension of 2 ( n - 1 ), or 2 ( n + 1 ),
 * points; the sines of types 2 to 4 by the cosines, the items with the
 * signs alternated or in the reverse order. The type 3 is the inverse of
 * the type 2, and the type 4 of itself, times 2 n.
 *
 * <b>The cost.</b> A transform of \f$ n = 2^K \f$ points does about \f$
 * 4.25 n K \f$ real operations, the radix 4 saving 15% of those of
 * radix 2; its data cross the memory in 2 passes (the columns, the rows)
 * plus one per round of Stockham of the columns or rows longer than a
 * codelet, the four steps keeping a buffer of n + O( \sqrt n ) items per
 * thread. The plans, the tables and the buffers are kept per thread for
 * each size met, made at the first transform of a size, in \f$ O( n ) \f$
 * evaluations of sines and cosines (\f$ O( n \log n ) \f$ for Rader and
 * Bluestein, whose plans also hold a transform) and, up to \f$ 2^{17} \f$
 * points, the 12 transforms that time the factorizations.
 *
 * <b>The rounding.</b> With powers of \f$ \omega \f$ computed to a
 * relative error \f$ \mu \f$, a transform by L rounds of radix 2 has
 * \f$ \| \tilde y - y \|_2 / \| y \|_2 \le L \eta / ( 1 - L \eta ) \f$,
 * \f$ \eta = \mu + \gamma_4 ( \sqrt 2 + \mu ) \f$, \f$ \gamma_4 = 4 u / ( 1
 * - 4 u ) \f$ and u the unit roundoff (Higham, Theorem 24.2); with \f$ \mu
 * \f$ near u the bound is about \f$ 6.7 u \log_2 n \f$, while the error of
 * the rounding observed on random data grows as \f$ u \sqrt{ \log n } \f$
 * (Schatzman). Bluestein adds the error of a convolution of M points, a
 * few times larger. The tests check every size up to 300 and the sizes
 * of each plan against the definition in long double.
 *
 * The transform turns a convolution into a product item by item;
 * convolve() multiplies two polynomials so, by the real transforms of the
 * smallest even size \f$ 2^a 3^b 5^c \f$ that holds their product, or of
 * the power of 2 if that size is not below 3 / 4 of it, and
 * the product of integers of many digits [see BigInteger.h] is a
 * convolution of their digits followed by the carries.
 *
 * The contracts [see Contract.h]: the sizes of the arguments of irfft(),
 * the types of dct() and dst(); the inverse transform of the transform
 * gives the vector back, up to the rounding, which the tests check. The
 * functions are defined in FFT.tpp.
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

using Complex = std::complex< double >;

/// the discrete Fourier transform by its definition, O( n^2 ); the inverse
/// one, divided by n, if inverse
[[nodiscard]] inline std::vector< Complex > dft(
                         const std::vector< Complex > & a , bool inverse );

/// the discrete Fourier transform of the n points of a in place, in
/// double or single precision, O( n log n ) for every n; the inverse one,
/// divided by n, if inverse
template< class T >
inline void fft( std::complex< T > * a , std::size_t n , bool inverse );

template< class T >
inline void fft( std::vector< std::complex< T > > & a , bool inverse );

/// the transforms of howmany vectors of n points each, the vector t from a
/// + t n on, in place
template< class T >
inline void fftMany( std::complex< T > * a , std::size_t n ,
                     std::size_t howmany , bool inverse );

/// the transform in d = | dims | dimensions of the array a of extents dims,
/// in the order of the rows (the last index the fastest), in place; the
/// inverse one divided by the number of points
template< class T >
inline void fftn( std::complex< T > * a ,
                  const std::vector< std::size_t > & dims , bool inverse );

/// the transform in 2 dimensions of the matrix a of rows x cols points, by
/// rows, in place
template< class T >
inline void fft2( std::complex< T > * a , std::size_t rows ,
                  std::size_t cols , bool inverse );

/// the transform of the n real numbers x, its n / 2 + 1 outputs X_k , k <=
/// n / 2 (the others are conj( X_{n-k} )); X may be where x is, if it has
/// room for n + 2 numbers
template< class T >
inline void rfft( const T * x , std::size_t n , std::complex< T > * X );

template< class T >
[[nodiscard]] inline std::vector< std::complex< T > > rfft(
                                                const std::vector< T > & x );

/// the n real numbers x whose transform has the outputs X_0 , ... ,
/// X_{n/2}, i.e., the inverse transform divided by n; x may be where X is
/// \pre X has n / 2 + 1 items (0 if n is 0)
template< class T >
inline void irfft( const std::complex< T > * X , std::size_t n , T * x );

template< class T >
[[nodiscard]] inline std::vector< T > irfft(
                                  const std::vector< std::complex< T > > & X ,
                                  std::size_t n );

/// the transform of the cosines of type 1 to 4 of the n real numbers of x
/// in place, as those of FFTW (REDFT00, REDFT10, REDFT01, REDFT11), with
/// no normalization: the type 3 is the inverse of the type 2, and the type
/// 4 of itself, times 2 n (2 ( n - 1 ) for the type 1)
/// \pre 1 <= type <= 4, and n != 1 for the type 1
template< class T >
inline void dct( T * x , std::size_t n , unsigned type );

template< class T >
inline void dct( std::vector< T > & x , unsigned type );

/// the transform of the sines of type 1 to 4 of the n real numbers of x in
/// place, as those of FFTW (RODFT00, RODFT10, RODFT01, RODFT11)
/// \pre 1 <= type <= 4
template< class T >
inline void dst( T * x , std::size_t n , unsigned type );

template< class T >
inline void dst( std::vector< T > & x , unsigned type );

/// the coefficients of the product of the polynomials p and q
[[nodiscard]] inline std::vector< double > convolve(
                                          const std::vector< double > & p ,
                                          const std::vector< double > & q );

/// the same in single precision, or double for vectors of doubles
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
