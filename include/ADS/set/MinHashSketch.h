/*--------------------------------------------------------------------------*/
/*-------------------------- File MinHashSketch.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minwise hashing of a set by one permutation, with the optimal
 * densification of Shrivastava, beyond the book: a signature of \f$ K \f$
 * words whose fraction of equal places between two sets estimates their
 * similarity of Jaccard \f$ J = | A \cap B | / | A \cup B | \f$ without
 * bias, from one hash per item. The minwise hashing of Broder takes
 * \f$ K \f$ hashes of each item and keeps the least of each; the hashing
 * by one permutation of Li, Owen and Zhang cuts the range of one hash into
 * \f$ K \f$ bins and keeps the least value of each bin, \f$ K \f$ times
 * faster, but leaves empty the bins that no item reaches when the set has
 * few items, and an empty bin has no value to compare. The densification
 * fills each empty bin \f$ i \f$ with the value of the first nonempty bin
 * of the sequence \f$ \pi( i , 1 ) , \pi( i , 2 ) , \ldots \f$ of
 * independent random bins, the same for every set; Shrivastava proves
 * that the probability that two densified signatures agree in a place is
 * then exactly \f$ J \f$, and that the variance of the estimate is below
 * that of the earlier densifications, which borrow from the next nonempty
 * bin in a fixed direction.
 *
 * The bin of the hash \f$ h \f$ of 64 bits is the high half of the
 * product \f$ h K \f$ on 128 bits (the reduction of Lemire), and its value
 * the top 63 bits of the low half, i.e., of the fraction of the range of
 * the bin that \f$ h \f$ reaches; a bin with no item holds
 * \f$ 2^{64} - 1 \f$, which no value equals. The sketch keeps the raw bins,
 * so that two sketches of the same \f$ K \f$ and seed merge by their least
 * values (`merge()`), into the sketch of the union of the two sets, and
 * densifies them only when a signature is asked for (`signature()`), in
 * expected time \f$ O( K^2 / F ) \f$ for \f$ F \f$ nonempty bins.
 *
 * The hash of an item is the word of 64 bits of `hash64()` of `H` [see
 * `FastHash.h`] finished by `splitmix64()` [see `RandomHashing.h`],
 * which turns the hashes of consecutive integers, regular in their top
 * bits, into random words; `insertHash()` takes a hash of 64 bits that is
 * already random.
 *
 * The contracts [see `Contract.h`]: \f$ 1 \le K \le 2^{32} \f$; the
 * sketches merged or compared have the same \f$ K \f$ and seed. The
 * operations are defined in `MinHashSketch.tpp`.
 *
 * \see A. Shrivastava, Optimal densification for fast and accurate minwise
 *      hashing, Proceedings of ICML 2017, PMLR 70, 3154-3163, 2017
 * \see P. Li, A. B. Owen, C.-H. Zhang, One permutation hashing, Advances in
 *      Neural Information Processing Systems 25 (NIPS 2012), 3122-3130,
 *      2012
 * \see A. Z. Broder, On the resemblance and containment of documents,
 *      Proceedings of SEQUENCES 1997, 21-29, 1997,
 *      doi:10.1109/SEQUEN.1997.666900
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

#ifndef __ADS_MinHashSketch
 #define __ADS_MinHashSketch
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/external/MappedFile.h"
#include "ADS/set/impl/SketchHash.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MinHashSketch ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the signature of \f$ K \f$ minima of a set, by one hash per item and the
/// optimal densification
template< class T , class H = DefaultHash< T > >
class MinHashSketch
{

/*--------------------------------------------------------------------------*/
 public:
 /// the value of an empty bin, \f$ 2^{64} - 1 \f$
 static constexpr std::uint64_t kEmpty = ~std::uint64_t( 0 );

 /// the sketch of the empty set, \f$ K \f$ bins, its densification from
 /// the seed
 /// \pre \f$ 1 \le K \le 2^{32} \f$
 explicit MinHashSketch( std::size_t K = 256 , std::uint64_t seed = 0 ,
			 H h = H() );

 /// \f$ x \f$ into the set, by `insertHash()` of its hash
 void insert( const T & x );

 /// the hash \f$ h \f$ of 64 bits, which must be random, into the set
 ///
 /// In pseudocode (Li, Owen and Zhang), \f$ B \f$ the bins:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertHash}( h ) \cr
 /// 2 & \quad i \gets \lfloor h K / 2^{64} \rfloor;\ v \gets \lfloor ( h K
 ///     \bmod 2^{64} ) / 2 \rfloor \cr
 /// 3 & \quad B[ i ] \gets \min( B[ i ] , v )
 /// \end{array}}
 /// \f]
 void insertHash( std::uint64_t h );

 /// the items of `o` added, as if they had been inserted here, by the
 /// least of the two values of each bin
 /// \pre `o` has the bins and the seed of this sketch
 void merge( const MinHashSketch & o );

 /// the densified signature, \f$ K \f$ words (all \f$ 2^{64} - 1 \f$ for
 /// the empty set)
 ///
 /// In pseudocode (Shrivastava), \f$ \pi( i , a ) \f$ the bin of the hash of
 /// the pair \f$ ( i , a ) \f$ under the seed:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{signature}() \cr
 /// 2 & \quad S \gets B;\ \textbf{if}\ \text{all the bins are empty}
 ///     \ \textbf{then return}\ S \cr
 /// 3 & \quad \textbf{for each}\ i\ \text{with}\ B[ i ]\ \text{empty}
 ///     \ \textbf{do} \cr
 /// 4 & \qquad a \gets 1;\ \textbf{while}\ B[ \pi( i , a ) ]\ \text{is}
 ///     \text{ empty}\ \textbf{do}\ a \gets a + 1 \cr
 /// 5 & \qquad S[ i ] \gets B[ \pi( i , a ) ] \cr
 /// 6 & \quad \textbf{return}\ S
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::vector< std::uint64_t > signature( void ) const;

 /// the estimate of the similarity of Jaccard of the two sets, the
 /// fraction of the places where their signatures agree: 1 if both are
 /// empty, 0 if one is
 /// \pre `o` has the bins and the seed of this sketch
 [[nodiscard]] double jaccard( const MinHashSketch & o ) const;

 /// the estimate of the similarity of Jaccard of two signatures of the
 /// same length, the fraction of their equal places
 [[nodiscard]] static double jaccard( const std::vector< std::uint64_t > & a ,
				      const std::vector< std::uint64_t > & b );

 /// the bins, \f$ K \f$
 [[nodiscard]] std::size_t bins( void ) const;

 /// the bins that hold a value
 [[nodiscard]] std::size_t filled( void ) const;

 /// whether no item was inserted
 [[nodiscard]] bool empty( void ) const;

 /// the raw bins, before the densification
 [[nodiscard]] const std::vector< std::uint64_t > & state( void ) const;

 /// removes all the items
 void clear( void );

 /// whether the two sketches have the same seed and bins
 [[nodiscard]] bool operator==( const MinHashSketch & o ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "MinHashSketch";

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 std::vector< std::uint64_t > f_B;  ///< the bins
 std::uint64_t f_seed;              ///< the seed of the densification
 H f_h;                             ///< the hash of the items

 };  // end( class MinHashSketch )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/MinHashSketch.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MinHashSketch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MinHashSketch.h ------------------------*/
/*--------------------------------------------------------------------------*/
