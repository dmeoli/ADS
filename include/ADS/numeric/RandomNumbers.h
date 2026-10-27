/*--------------------------------------------------------------------------*/
/*-------------------------- File RandomNumbers.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Random numbers as Knuth treats them (chapter 3 of The Art of Computer
 * Programming), beyond the book:
 *
 * - `LinearCongruential`, \f$ X_{n+1} = ( a X_n + c ) \bmod m \f$ (section
 *   3.2.1); with the theorem of Hull and Dobell its period is \f$ m \f$
 *   exactly when \f$ c \f$ and \f$ m \f$ are coprime, \f$ a - 1 \f$ is a
 *   multiple of every prime factor of \f$ m \f$, and of 4 if \f$ m \f$ is;
 *   `minstd()` gives the multiplicative generator of Park and Miller, and
 *   `mmix()` that of Knuth for MMIX, \f$ m = 2^{64} \f$; its parameters are
 *   values at run time, and so are `min()` and `max()`, which a
 *   UniformRandomBitGenerator has constant: `LinearCongruentialT` is the
 *   same generator with \f$ a , c , m \f$ fixed at compile time, which the
 *   distributions of the standard library take, `MinStd` and `Mmix` those
 *   of Park and Miller and of Knuth;
 * - `chiSquare()` and `kolmogorovSmirnov()` are the two general tests of
 *   section 3.3.1: the statistic
 *   \f$ V = \sum ( Y_s - n p_s )^2 / ( n p_s ) \f$ of the counts in
 *   \f$ k \f$ categories, to compare with the \f$ \chi^2 \f$ distribution
 *   with \f$ k - 1 \f$ degrees of freedom, and the statistics
 *   \f$ K^+_n , K^-_n \f$ of the largest deviations of the empirical
 *   distribution from the continuous one; `serialTest()` applies the first
 *   to the pairs of consecutive numbers (section 3.3.2B);
 * - `spectralTest2()` is the spectral test in two dimensions (section
 *   3.3.4): the points \f$ ( X_n , X_{n+1} ) \f$ of a linear congruential
 *   generator lie on parallel lines at the distance \f$ 1 / \nu_2 \f$, where
 *   \f$ \nu_2 \f$ is the length of the shortest nonzero vector
 *   \f$ ( u , v ) \f$ with \f$ u + a v \equiv 0 \pmod m \f$, found by the
 *   reduction of Gauss and Lagrange of the basis
 *   \f$ ( m , 0 ) , ( -a , 1 ) \f$; a good generator has \f$ \nu_2 \f$ near
 *   \f$ \sqrt{m} \f$;
 * - `shuffle()` is Algorithm 3.4.2P (Fisher and Yates, Durstenfeld);
 *   `selectionSampling()` is Algorithm 3.4.2S, \f$ n \f$ of \f$ N \f$
 *   records in order with one pass, and `reservoirSampling()` is Algorithm
 *   3.4.2R, \f$ n \f$ records from a stream of unknown length.
 *
 * The contracts [see `Contract.h`]: \f$ m \ge 1 \f$ and \f$ a , c < m \f$
 * for the generator, positive expected counts for the tests,
 * \f$ n \le N \f$ for the samplings. The functions are defined in
 * `RandomNumbers.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, chapter 3
 * \see T. E. Hull, A. R. Dobell, Random number generators, SIAM Review
 *      4(3), 230-254, 1962, doi:10.1137/1004061
 * \see S. K. Park, K. W. Miller, Random number generators: good ones are
 *      hard to find, Communications of the ACM 31(10), 1192-1201, 1988,
 *      doi:10.1145/63039.63042
 * \see R. A. Fisher, F. Yates, Statistical Tables for Biological,
 *      Agricultural and Medical Research, Oliver and Boyd, 1938, and R.
 *      Durstenfeld, Algorithm 235: random permutation, Communications of
 *      the ACM 7(7), 420, 1964, doi:10.1145/364520.364540
 * \see J. S. Vitter, Random sampling with a reservoir, ACM Transactions on
 *      Mathematical Software 11(1), 37-57, 1985, doi:10.1145/3147.3165
 * \see R. P. Brent, An improved Monte Carlo factorization algorithm, BIT
 *      Numerical Mathematics 20(2), 176-184, 1980, doi:10.1007/BF01933190
 *      (the detection of the cycle of period())
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

#ifndef __ADS_RandomNumbers
 #define __ADS_RandomNumbers
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/NumberTheory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS LinearCongruential ------------------------*/
/*--------------------------------------------------------------------------*/

/// the generator \f$ X_{n+1} = ( a X_n + c ) \bmod m \f$, \f$ m = 0 \f$
/// meaning \f$ 2^{64} \f$
class LinearCongruential {
 public:
 using result_type = std::uint64_t;   ///< the type of the numbers

 /// the generator of multiplier \f$ a \f$, increment \f$ c \f$ and modulus
 /// \f$ m \f$, from \f$ X_0 = \mathit{seed} \f$
 /// \pre \f$ m = 0 \f$, or \f$ a < m \f$, \f$ c < m \f$ and
 ///      \f$ \mathit{seed} < m \f$
 LinearCongruential( std::uint64_t a , std::uint64_t c , std::uint64_t m ,
                     std::uint64_t seed );

 /// the generator of Park and Miller, \f$ a = 16807 \f$, \f$ c = 0 \f$,
 /// \f$ m = 2^{31} - 1 \f$
 static LinearCongruential minstd( std::uint64_t seed = 1 );

 /// the generator of Knuth for MMIX, \f$ m = 2^{64} \f$
 static LinearCongruential mmix( std::uint64_t seed = 1 );

 /// the next number \f$ X_{n+1} = ( a X_n + c ) \bmod m \f$, by
 /// `mulMod()` and an addition that does not overflow (section 3.2.1 of
 /// Knuth)
 std::uint64_t operator()( void );

 /// the least number, 1 if \f$ c = 0 \f$ as in
 /// `std::linear_congruential_engine` (0 is then a fixed point, which the
 /// multiplicative generators, as `minstd()`, never reach), 0 otherwise
 [[nodiscard]] std::uint64_t min( void ) const { return( f_c == 0 ); }
 /// the largest number, \f$ m - 1 \f$
 [[nodiscard]] std::uint64_t max( void ) const {
  return( f_m ? f_m - 1 : std::numeric_limits< std::uint64_t >::max() );
  }

 /// the period, by walking the sequence, when it is at most
 /// \f$ \mathit{limit} \f$; 0 otherwise
 ///
 /// In pseudocode (the cycle detection of Brent, beyond the book), the
 /// tortoise moved to the hare at each power of 2:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{period}( \mathit{limit} ) \cr
 /// 2 & \quad t \gets X_1;\ h \gets X_2;\ \mathit{power} \gets 1;\ \lambda
 ///     \gets 1 \cr
 /// 3 & \quad \textbf{while}\ t \ne h\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \mathit{power} = \lambda\ \textbf{then}\ t \gets
 ///     h;\ \mathit{power} \gets 2 \mathit{power};\ \lambda \gets 0 \cr
 /// 5 & \qquad h \gets \text{the next number};\ \lambda \gets \lambda + 1 \cr
 /// 6 & \qquad \textbf{if}\ \lambda > \mathit{limit}\ \textbf{then return}
 ///     \ 0 \cr
 /// 7 & \quad \textbf{return}\ \lambda
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::uint64_t period( std::uint64_t limit ) const;

 private:
 std::uint64_t f_a ,   ///< the multiplier
               f_c ,   ///< the increment
               f_m ,   ///< the modulus, 0 for \f$ 2^{64} \f$
               f_x;    ///< the last number
 };

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS LinearCongruentialT ------------------------*/
/*--------------------------------------------------------------------------*/

/// the generator \f$ X_{n+1} = ( a X_n + c ) \bmod m \f$ of
/// `LinearCongruential` with \f$ a , c , m \f$ fixed at compile time,
/// \f$ m = 0 \f$ meaning \f$ 2^{64} \f$: a UniformRandomBitGenerator, as
/// `min()` and `max()` are constant
template< std::uint64_t a , std::uint64_t c , std::uint64_t m >
class LinearCongruentialT {
 static_assert( ( m == 0 ) || ( ( a < m ) && ( c < m ) ) , "a , c < m" );
 public:
 using result_type = std::uint64_t;   ///< the type of the numbers

 /// the generator from \f$ X_0 = \mathit{seed} \f$
 /// \pre \f$ m = 0 \f$ or \f$ \mathit{seed} < m \f$
 explicit constexpr LinearCongruentialT( std::uint64_t seed = 1 );

 /// the next number, \f$ X_{n+1} = ( a X_n + c ) \bmod m \f$
 constexpr std::uint64_t operator()( void );

 /// the least number, 1 if \f$ c = 0 \f$, 0 otherwise
 static constexpr std::uint64_t min( void ) { return( c == 0 ); }
 /// the largest number, \f$ m - 1 \f$
 static constexpr std::uint64_t max( void ) {
  return( m ? m - 1 : std::numeric_limits< std::uint64_t >::max() );
  }

 /// whether the two generators are in the same state
 bool operator==( const LinearCongruentialT & ) const = default;

 private:
 std::uint64_t f_x;   ///< the last number, \f$ X_n \f$
 };

/// the generator of Park and Miller, \f$ a = 16807 \f$, \f$ c = 0 \f$,
/// \f$ m = 2^{31} - 1 \f$
using MinStd = LinearCongruentialT< 16807 , 0 , 2147483647 >;

/// the generator of Knuth for MMIX, \f$ m = 2^{64} \f$
using Mmix = LinearCongruentialT< 6364136223846793005ULL ,
                                  1442695040888963407ULL , 0 >;

/// the chi-square statistic \f$ V \f$ of the counts \f$ Y \f$ against the
/// probabilities \f$ p \f$ over \f$ n = \sum_s Y_s \f$ samples
///
/// In pseudocode (section 3.3.1 of Knuth, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{chiSquare}( Y , p ) \cr
/// 2 & \quad n \gets \sum_s Y_s \cr
/// 3 & \quad \textbf{return}\ \sum_s ( Y_s - n p_s )^2 / ( n p_s )
/// \end{array}}
/// \f]
/// \pre \f$ Y \f$ and \f$ p \f$ of the same size, each \f$ n p_s > 0 \f$
inline double chiSquare( const std::vector< long > & Y ,
                         const std::vector< double > & p );

/// the statistics \f$ ( K^+_n , K^-_n ) \f$ of Kolmogorov and Smirnov of
/// the numbers \f$ U \f$ in \f$ [ 0 , 1 ) \f$ against the uniform
/// distribution, scaled by \f$ \sqrt{n} \f$
///
/// In pseudocode (section 3.3.1 of Knuth, beyond the book), on the numbers
/// sorted:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kolmogorovSmirnov}( U ) \cr
/// 2 & \quad \text{sort } U \text{ into } x_1 \le \cdots \le x_n \cr
/// 3 & \quad K^+ \gets \sqrt{n} \max_j ( j / n - x_j );\ K^- \gets \sqrt{n}
///     \max_j ( x_j - ( j - 1 ) / n ) \cr
/// 4 & \quad \textbf{return}\ ( K^+ , K^- )
/// \end{array}}
/// \f]
/// \pre \f$ U \f$ not empty
inline std::pair< double , double > kolmogorovSmirnov(
                                             std::vector< double > U );

/// the chi-square statistic of the pairs \f$ ( U_{2j} , U_{2j+1} ) \f$ of
/// numbers in \f$ [ 0 , 1 ) \f$ in \f$ d \times d \f$ cells, \f$ d^2 - 1 \f$
/// degrees of freedom
///
/// In pseudocode (section 3.3.2B of Knuth, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{serialTest}( U , d ) \cr
/// 2 & \quad Y \gets d^2 \text{ zero counts} \cr
/// 3 & \quad \textbf{foreach}\ j\ \textbf{do}\ Y[ \lfloor d U_{2j} \rfloor ,
///     \lfloor d U_{2j+1} \rfloor ] \gets Y[ \ldots ] + 1 \cr
/// 4 & \quad \textbf{return}\ \mathrm{chiSquare}( Y , \text{each } 1 / d^2 )
/// \end{array}}
/// \f]
/// \pre \f$ d \ge 2 \f$ and at least 2 numbers
inline double serialTest( const std::vector< double > & U , int d );

/// the \f$ \nu_2 \f$ of the spectral test of the generator of multiplier
/// \f$ a \f$ and modulus \f$ m \f$
///
/// In pseudocode (the reduction of Gauss and Lagrange, section 3.3.4 of
/// Knuth, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{spectralTest2}( a , m ) \cr
/// 2 & \quad b_1 \gets ( m , 0 );\ b_2 \gets ( -a , 1 ) \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \| b_1 \| < \| b_2 \|\ \textbf{then}
///     \ \mathrm{swap}( b_1 , b_2 ) \cr
/// 5 & \qquad b_1 \gets b_1 - \lfloor b_1 \cdot b_2 / \| b_2 \|^2 \rceil
///     b_2 \cr
/// 6 & \qquad \textbf{if}\ \| b_1 \| \ge \| b_2 \|\ \textbf{then return}
///     \ \| b_2 \|
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 2 \f$ and \f$ 0 < a < m \f$
inline double spectralTest2( std::uint64_t a , std::uint64_t m );

/// the vector \f$ A \f$ shuffled by \f$ g \f$, every permutation equally
/// likely up to the bias of \f$ g() \bmod j \f$
///
/// In pseudocode (Algorithm 3.4.2P of Knuth, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{shuffle}( A , g ) \cr
/// 2 & \quad \textbf{for}\ j \gets |A|\ \textbf{downto}\ 2\ \textbf{do} \cr
/// 3 & \qquad k \gets g() \bmod j;\ \mathrm{swap}( A[ j - 1 ] , A[ k ] )
/// \end{array}}
/// \f]
template< class T , class R >
void shuffle( std::vector< T > & A , R & g );

/// \f$ n \f$ of the \f$ N \f$ items \f$ 0 , \ldots , N - 1 \f$, in order, by
/// one pass
///
/// In pseudocode (Algorithm 3.4.2S of Knuth, beyond the book), the item
/// \f$ t \f$ taken with probability \f$ ( n - m ) / ( N - t ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{selectionSampling}( n , N , g ) \cr
/// 2 & \quad O \gets \text{the empty list};\ t \gets 0 \cr
/// 3 & \quad \textbf{while}\ |O| < n\ \textbf{do} \cr
/// 4 & \qquad U \gets \text{a uniform of } [ 0 , 1 ) \cr
/// 5 & \qquad \textbf{if}\ ( N - t ) U < n - |O|\ \textbf{then}
///     \ \text{append } t \text{ to } O \cr
/// 6 & \qquad t \gets t + 1 \cr
/// 7 & \quad \textbf{return}\ O
/// \end{array}}
/// \f]
/// \pre \f$ n \le N \f$
template< class R >
std::vector< std::size_t > selectionSampling( std::size_t n , std::size_t N ,
                                              R & g );

/// \f$ n \f$ items from the sequence \f$ S \f$, as if it were a stream of
/// unknown length; all of \f$ S \f$, in its order, when \f$ S \f$ has at
/// most \f$ n \f$ items (a stream shorter than the sample)
///
/// In pseudocode (Algorithm 3.4.2R of Knuth, beyond the book), the item
/// \f$ t \f$ entering with probability \f$ n / ( t + 1 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reservoirSampling}( S , n , g ) \cr
/// 2 & \quad \textbf{for}\ t \gets 0\ \textbf{to}\ |S| - 1\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ t < n\ \textbf{then}\ \text{append } S[ t ]
///     \text{ to } R \cr
/// 4 & \qquad \textbf{else}\ M \gets g() \bmod ( t + 1 );\ \textbf{if}\ M < n
///     \ \textbf{then}\ R[ M ] \gets S[ t ] \cr
/// 5 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
template< class T , class R >
std::vector< T > reservoirSampling( const std::vector< T > & S ,
                                    std::size_t n , R & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/RandomNumbers.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RandomNumbers.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RandomNumbers.h ------------------------*/
/*--------------------------------------------------------------------------*/
