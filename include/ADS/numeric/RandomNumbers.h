/*--------------------------------------------------------------------------*/
/*-------------------------- File RandomNumbers.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Random numbers as Knuth treats them (chapter 3), beyond the book:
 *
 * - LinearCongruential, \f$ X_{n+1} = ( a X_n + c ) \bmod m \f$ (3.2.1),
 *   a UniformRandomBitGenerator; with the theorem of Hull and Dobell its
 *   period is m exactly when c and m are coprime, a - 1 is a multiple of
 *   every prime factor of m, and of 4 if m is; minstd() gives the
 *   multiplicative generator of Park and Miller, and mmix() that of Knuth for
 *   MMIX, \f$ m = 2^{64} \f$;
 * - chiSquare() and kolmogorovSmirnov() are the two general tests of 3.3.1:
 *   the statistic \f$ V = \sum ( Y_s - n p_s )^2 / ( n p_s ) \f$ of the
 *   counts in k categories, to compare with the \f$ \chi^2 \f$ distribution
 *   with k - 1 degrees of freedom, and the statistics \f$ K^+_n , K^-_n \f$ of
 *   the largest deviations of the empirical distribution from the
 *   continuous one; serialTest() applies the first to the pairs of
 *   consecutive numbers (3.3.2B);
 * - spectralTest2() is the spectral test in two dimensions (3.3.4): the
 *   points \f$ ( X_n , X_{n+1} ) \f$ of a linear congruential generator lie
 *   on parallel lines at the distance \f$ 1 / \nu_2 \f$, where
 *   \f$ \nu_2 \f$ is the length of the shortest nonzero vector
 *   \f$ ( u , v ) \f$ with \f$ u + a v \equiv 0 \pmod m \f$, found by the
 *   reduction of Gauss and Lagrange of the basis \f$ ( m , 0 ) , ( -a , 1 )
 *   \f$; a good generator has \f$ \nu_2 \f$ near \f$ \sqrt{m} \f$;
 * - shuffle() is Algorithm 3.4.2P (Fisher and Yates, Durstenfeld), every
 *   permutation equally likely; selectionSampling() is Algorithm 3.4.2S,
 *   n of N records in order with one pass, and reservoirSampling() is
 *   Algorithm 3.4.2R, n records from a stream of unknown length.
 *
 * The contracts [see Contract.h]: m >= 1 and a , c < m for the generator,
 * positive expected counts for the tests, n <= N for the samplings. The
 * functions are defined in RandomNumbers.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, chapter 3
 * \see T. E. Hull, A. R. Dobell, Random number generators, SIAM Review
 *      4(3), 230-254, 1962, doi:10.1137/1004061
 * \see S. K. Park, K. W. Miller, Random number generators: good ones are
 *      hard to find, Communications of the ACM 31(10), 1192-1201, 1988,
 *      doi:10.1145/63039.63042
 * \see J. S. Vitter, Random sampling with a reservoir, ACM Transactions on
 *      Mathematical Software 11(1), 37-57, 1985, doi:10.1145/3147.3165
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

/// X_{n+1} = ( a X_n + c ) mod m, m = 0 meaning 2^64
class LinearCongruential {
 public:
 using result_type = std::uint64_t;

 /// \pre m == 0 or ( a < m and c < m and seed < m )
 LinearCongruential( std::uint64_t a , std::uint64_t c , std::uint64_t m ,
                     std::uint64_t seed );

 /// the generator of Park and Miller, a = 16807, c = 0, m = 2^31 - 1
 static LinearCongruential minstd( std::uint64_t seed = 1 );

 /// the generator of Knuth for MMIX, m = 2^64
 static LinearCongruential mmix( std::uint64_t seed = 1 );

 std::uint64_t operator()( void );

 [[nodiscard]] std::uint64_t min( void ) const { return( 0 ); }
 [[nodiscard]] std::uint64_t max( void ) const {
  return( f_m ? f_m - 1 : std::numeric_limits< std::uint64_t >::max() );
  }

 /// the period, by walking the sequence, when it is at most limit; 0
 /// otherwise
 [[nodiscard]] std::uint64_t period( std::uint64_t limit ) const;

 private:
 std::uint64_t f_a , f_c , f_m , f_x;
 };

/// the chi-square statistic V of the counts Y against the probabilities p
/// over n samples
/// \pre Y and p of the same size, each n p_s > 0
inline double chiSquare( const std::vector< long > & Y ,
                         const std::vector< double > & p );

/// the statistics ( K+ , K- ) of Kolmogorov and Smirnov of the numbers U
/// in [ 0 , 1 ) against the uniform distribution, scaled by sqrt( n )
/// \pre U not empty
inline std::pair< double , double > kolmogorovSmirnov(
                                             std::vector< double > U );

/// the chi-square statistic of the pairs ( U_{2j} , U_{2j+1} ) of numbers
/// in [ 0 , 1 ) in d x d cells, d^2 - 1 degrees of freedom
/// \pre d >= 2 and at least 2 numbers
inline double serialTest( const std::vector< double > & U , int d );

/// nu_2 of the spectral test of the generator of multiplier a and modulus m
/// \pre m >= 2 and 0 < a < m
inline double spectralTest2( std::uint64_t a , std::uint64_t m );

/// the vector A shuffled by g, every permutation equally likely
template< class T , class R >
void shuffle( std::vector< T > & A , R & g );

/// n of the N items 0 , ... , N - 1, in order, by one pass
/// \pre n <= N
template< class R >
std::vector< std::size_t > selectionSampling( std::size_t n , std::size_t N ,
                                              R & g );

/// n items from the sequence S, as if it were a stream of unknown length
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
