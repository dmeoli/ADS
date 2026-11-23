/*--------------------------------------------------------------------------*/
/*--------------------------- File UltraLogLog.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sketch UltraLogLog of Ertl, beyond the book: an estimate of the
 * number of distinct items of a stream in \f$ m = 2^p \f$ registers of 8
 * bits, mergeable and of the same use as HyperLogLog, whose space at the
 * same error it cuts by 24 or 28 per cent (by its two estimators, against
 * the registers of 6 bits of HyperLogLog). The hash of an item, 64 random
 * bits, gives the register \f$ i \f$ by its top \f$ p \f$ bits and a level
 * \f$ \ell \ge 1 \f$ by the number \f$ \ell - 1 \f$ of the zeros that lead
 * the other \f$ 64 - p \f$ bits (so that \f$ \Pr[ \ell ] = 2^{-\ell} \f$,
 * the last level \f$ L = 65 - p \f$ taking what remains); a register keeps
 * the highest level \f$ u \f$ that it has seen and whether it has seen the
 * levels \f$ u - 1 \f$ and \f$ u - 2 \f$, i.e., the top bit and the two bits
 * below it of the set of its levels written as a word, where the level
 * \f$ \ell \f$ is the bit \f$ \ell + p - 2 \f$: the register holds
 * \f$ 4 ( u + p - 2 ) \f$ plus the two bits, and 0 when it has seen
 * nothing. HyperLogLog keeps \f$ u \f$ alone in 6 bits.
 *
 * Two estimators read the registers [see `estimate()` and `estimateML()`]:
 * - the FGRA estimator of Ertl (the further generalized remaining area,
 *   after the GRA of Pettie and Wang), which sums
 *   \f$ \eta_b 2^{-\tau u} \f$ over the registers, \f$ b \f$ their two
 *   bits, with the constants \f$ \tau \approx 0.8195 \f$ and
 *   \f$ \eta_0 , \ldots , \eta_3 \f$ of the paper, and returns
 *   \f$ m ( m / S )^{1 / \tau} \f$ corrected by a factor
 *   \f$ 1 + O( 1 / m ) \f$; the registers that have not seen their lower
 *   levels (empty, or with \f$ u \le 2 \f$) count by their conditional
 *   expectation given the fraction of the empty ones, in the way Ertl
 *   corrects HyperLogLog, so that the estimate holds from 0 items on. Its
 *   relative standard error is \f$ \sqrt{0.6119 / m} \f$, i.e., a memory
 *   variance product (the bits times the relative variance) of
 *   \f$ 4.895 \f$, against \f$ 6 \cdot 1.04^2 \approx 6.49 \f$ of
 *   HyperLogLog with registers of 6 bits;
 * - the estimator of maximum likelihood, whose logarithmic likelihood in
 *   the rate \f$ \lambda = n / m \f$ of the model of Poisson is
 *   \f$ -\alpha \lambda + \sum_\ell \beta_\ell
 *   \log( 1 - e^{-\lambda w_\ell} ) \f$, concave, maximized by the
 *   iteration of Newton from a lower bound of its root; its relative
 *   standard error is \f$ \sqrt{0.5789 / m} \f$ (a memory variance
 *   product of \f$ 4.631 \f$), at the price of a pass over the levels per
 *   iteration.
 *
 * Two sketches of the same precision merge by the register-wise union of
 * their sets of levels (`merge()`), and the result is the sketch of the
 * union of their streams; a sketch of precision \f$ p \f$ is downsized to
 * any \f$ q < p \f$ (`downsize()`), with the same registers as the sketch
 * of precision \f$ q \f$ of the same stream, which lets a sketch merge into
 * one of lower precision.
 *
 * The hash of an item is the word of 64 bits of `hash64()` of `H` [see
 * `FastHash.h`] finished by `splitmix64()` [see `RandomHashing.h`],
 * which turns the hashes of consecutive integers, regular in their top
 * bits, into random words; `insertHash()` takes a hash of 64 bits that is
 * already random.
 *
 * The contracts [see `Contract.h`]: \f$ 3 \le p \le 26 \f$; a sketch
 * merged into another has a precision at least as high; a sketch is
 * downsized to a precision from 3 to its own. The operations are defined
 * in `UltraLogLog.tpp`.
 *
 * \see O. Ertl, UltraLogLog: a practical and more space-efficient
 *      alternative to HyperLogLog for approximate distinct counting,
 *      Proceedings of the VLDB Endowment 17(7), 1655-1668, 2024,
 *      doi:10.14778/3654621.3654632
 * \see O. Ertl, New cardinality estimation algorithms for HyperLogLog
 *      sketches, 2017, arXiv:1702.01284 (the correction of the registers
 *      that have not seen their lower levels)
 * \see S. Pettie, D. Wang, Information theoretic limits of cardinality
 *      estimation: Fisher meets Shannon, Proceedings of STOC 2021,
 *      556-569, 2021, doi:10.1145/3406325.3451032 (the GRA estimator)
 * \see P. Flajolet, É. Fusy, O. Gandouet, F. Meunier, HyperLogLog: the
 *      analysis of a near-optimal cardinality estimation algorithm,
 *      Proceedings of AofA 2007, DMTCS Proceedings AH, 137-156, 2007,
 *      doi:10.46298/dmtcs.3545
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

#ifndef __ADS_UltraLogLog
 #define __ADS_UltraLogLog
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/external/MappedFile.h"
#include "ADS/set/impl/SketchHash.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS UltraLogLog ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the distinct items of a stream, estimated in \f$ 2^p \f$ registers of 8
/// bits
template< class T , class H = DefaultHash< T > >
class UltraLogLog
{

/*--------------------------------------------------------------------------*/
 public:
 /// the empty sketch of precision \f$ p \f$, \f$ 2^p \f$ registers
 /// \pre \f$ 3 \le p \le 26 \f$
 explicit UltraLogLog( unsigned p = 12 , H h = H() );

 /// \f$ x \f$ into the sketch, by `insertHash()` of its hash
 void insert( const T & x );

 /// the hash \f$ h \f$ of 64 bits, which must be random, into the sketch
 ///
 /// In pseudocode (Ertl), \f$ R \f$ the registers, \f$ \mathrm{nlz} \f$ the
 /// leading zeros of a word of 64 bits, \f$ \mathrm{unpack}( r ) \f$ the
 /// word of the three bits of \f$ r \ne 0 \f$, \f$ ( 4 + r \bmod 4 ) \cdot
 /// 2^{\lfloor r / 4 \rfloor - 2} \f$, and 0 for \f$ r = 0 \f$, and
 /// \f$ \mathrm{pack}( x ) \f$ the register of the top bit \f$ k \f$ of
 /// \f$ x \ne 0 \f$ and of the two bits below it,
 /// \f$ 4 k + \lfloor x / 2^{k - 2} \rfloor \bmod 4 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertHash}( h ) \cr
 /// 2 & \quad i \gets \lfloor h / 2^{64 - p} \rfloor \cr
 /// 3 & \quad z \gets \mathrm{nlz}( ( h \cdot 2^p \bmod 2^{64} ) +
 ///     2^p - 1 )\quad \triangleright\ 0 \le z \le 64 - p \cr
 /// 4 & \quad R[ i ] \gets \mathrm{pack}( \mathrm{unpack}( R[ i ] ) \vee
 ///     2^{z + p - 1} )
 /// \end{array}}
 /// \f]
 void insertHash( std::uint64_t h );

 /// the items of `o` added, as if they had been inserted here: each
 /// register the union of the sets of levels of the two, `o` downsized
 /// first if its precision is higher [see `downsize()`]
 ///
 /// In pseudocode (Ertl), \f$ d = p_o - p \f$ and \f$ \mathrm{block}( i ) \f$
 /// the word of the levels of the \f$ 2^d \f$ registers
 /// \f$ R_o[ 2^d i ] , \ldots , R_o[ 2^d i + 2^d - 1 ] \f$ at the precision
 /// \f$ p \f$ [see `downsize()`]:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{merge}( o ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
 /// 3 & \qquad x \gets \mathrm{unpack}( R[ i ] ) \vee \mathrm{block}( i ) \cr
 /// 4 & \qquad \textbf{if}\ x \ne 0\ \textbf{then}\ R[ i ] \gets
 ///     \mathrm{pack}( x )
 /// \end{array}}
 /// \f]
 /// \pre `o.precision() >= precision()`
 void merge( const UltraLogLog & o );

 /// the sketch of precision \f$ q \f$ of the same items, equal to the one
 /// that their insertions at precision \f$ q \f$ would have made
 ///
 /// In pseudocode (Ertl), \f$ d = p - q \f$: the register \f$ j \f$ of
 /// precision \f$ q \f$ gathers the registers \f$ 2^d j + k \f$, whose
 /// hashes carry \f$ k \f$ in the \f$ d \f$ bits after the top \f$ q \f$;
 /// for \f$ k = 0 \f$ the level is unchanged, while for \f$ k > 0 \f$ it is
 /// fixed by the leading zeros of \f$ k \f$ in \f$ d \f$ bits:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{downsize}( q ) \cr
 /// 2 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ 2^q - 1\ \textbf{do} \cr
 /// 3 & \qquad x \gets \mathrm{unpack}( R[ 2^d j ] ) \cr
 /// 4 & \qquad \textbf{for}\ k \gets 1\ \textbf{to}\ 2^d - 1\ \textbf{do}
 ///     \ \textbf{if}\ R[ 2^d j + k ] \ne 0\ \textbf{then}\ x \gets x \vee
 ///     2^{q + d - 2 - \lfloor \log_2 k \rfloor} \cr
 /// 5 & \qquad R'[ j ] \gets \mathrm{pack}( x )\ \textbf{if}\ x \ne 0
 ///     \ \textbf{else}\ 0 \cr
 /// 6 & \quad \textbf{return}\ R'
 /// \end{array}}
 /// \f]
 /// \pre \f$ 3 \le q \le p \f$
 [[nodiscard]] UltraLogLog downsize( unsigned q ) const;

 /// the estimate of the distinct items by the FGRA estimator, in
 /// \f$ O( 1 ) \f$ time from the tally of the registers, which the
 /// insertions keep
 ///
 /// In pseudocode (Ertl, with the correction of the lower levels after
 /// Ertl 2017), \f$ g( u , b ) = \eta_b 2^{-\tau u} \f$, \f$ C_0 \f$ the
 /// empty registers, \f$ z = C_0 / m \f$, \f$ e_0 \f$, \f$ e_1 \f$ and
 /// \f$ e_2 \f$ the conditional expectations of \f$ g \f$ given an empty
 /// register, one of level 1 and one of level 2 with its bit of the level 1
 /// [see `UltraLogLog.tpp`], \f$ \xi \f$ and \f$ v \f$ the mean and the
 /// relative variance of \f$ g \lambda^\tau \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{estimate}() \cr
 /// 2 & \quad \textbf{if}\ C_0 = m\ \textbf{then return}\ 0 \cr
 /// 3 & \quad S \gets C_0 e_0( z ) + \sum_{u_i = 1} e_1( z ) +
 ///     \sum_{u_i = 2} e_2( b_i , z ) + \sum_{u_i \ge 3} g( u_i , b_i ) \cr
 /// 4 & \quad \textbf{return}\ m ( m \xi / S )^{1 / \tau} / ( 1 + ( 1 /
 ///     \tau ) ( 1 / \tau + 1 ) v / ( 2 m ) )
 /// \end{array}}
 /// \f]
 [[nodiscard]] double estimate( void ) const;

 /// the estimate of the distinct items by maximum likelihood
 ///
 /// In pseudocode (Ertl), \f$ w_\ell = 2^{-\ell} \f$ the rate of the level
 /// \f$ \ell \f$ (and \f$ w_L = 2^{1 - L} \f$), \f$ \beta_\ell \f$ the
 /// registers that have seen the level \f$ \ell \f$, \f$ \alpha \f$ the sum
 /// of the rates of the levels that the registers know they have not seen,
 /// \f$ B = \sum_\ell \beta_\ell \f$ and \f$ W = \sum_\ell \beta_\ell
 /// w_\ell \f$; the root of the decreasing and convex
 /// \f$ f( \lambda ) = \sum_\ell \beta_\ell w_\ell / ( e^{\lambda w_\ell} -
 /// 1 ) - \alpha \f$ lies in \f$ [ B / ( \alpha + W / 2 ) , B / \alpha ] \f$,
 /// and the steps of Newton from the lower end increase to it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{estimateML}() \cr
 /// 2 & \quad \textbf{if}\ B = 0\ \textbf{then return}\ 0 \cr
 /// 3 & \quad \lambda \gets B / ( \alpha + W / 2 ) \cr
 /// 4 & \quad \textbf{repeat}\ \lambda \gets \lambda - f( \lambda ) /
 ///     f'( \lambda )\ \textbf{until the step is below}\ 10^{-12} \lambda \cr
 /// 5 & \quad \textbf{return}\ m \lambda
 /// \end{array}}
 /// \f]
 [[nodiscard]] double estimateML( void ) const;

 /// the relative standard error of `estimate()`, \f$ \sqrt{0.6119 / m} \f$,
 /// or of `estimateML()`, \f$ \sqrt{0.5789 / m} \f$, for \f$ n \gg m \f$
 [[nodiscard]] double relativeStandardError( bool ml = false ) const;

 /// removes all the items
 void clear( void );

 /// whether no item was inserted, i.e., all the registers are 0
 [[nodiscard]] bool empty( void ) const;

 /// the precision \f$ p \f$
 [[nodiscard]] unsigned precision( void ) const;

 /// the registers, \f$ 2^p \f$ bytes
 [[nodiscard]] const std::vector< std::uint8_t > & state( void ) const;

 /// whether the two sketches have the same precision and registers
 [[nodiscard]] bool operator==( const UltraLogLog & o ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "UltraLogLog";

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 /// the hash of \f$ x \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the word of the levels, at the precision \f$ p - d \f$, of the
 /// \f$ 2^d \f$ registers from \f$ R \f$
 [[nodiscard]] static std::uint64_t block( const std::uint8_t * R ,
					   unsigned d ,
					   unsigned q );

 /// the tally of the registers that the FGRA estimator reads, kept by the
 /// changes of the registers with no branch: the number of the registers
 /// of each class (empty, of level 1, of level 2 without and with their
 /// bit of the level 1, and the others), and the sum of
 /// \f$ \eta_b 2^{-\tau k} \f$, \f$ k = \lfloor r / 4 \rfloor \f$, over
 /// the last class, whose terms the tables of the precision give
 struct Tally
 {
  std::size_t c[ 5 ] = {};   ///< the registers of each class
  double s = 0;              ///< the sum over the last class
  const std::uint8_t * cls;  ///< the class of each register
  const double * g;          ///< the term of each register, or 0
  };

 /// the tally made again from the registers
 void retally( void );

 std::vector< std::uint8_t > f_R;  ///< the registers
 unsigned f_p;                     ///< the precision
 H f_h;                            ///< the hash of the items
 Tally f_t;                        ///< the tally of the registers

 };  // end( class UltraLogLog )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/UltraLogLog.tpp"

/*--------------------------------------------------------------------------*/

#endif /* UltraLogLog.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File UltraLogLog.h --------------------------*/
/*--------------------------------------------------------------------------*/
