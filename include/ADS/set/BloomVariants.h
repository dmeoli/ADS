/*--------------------------------------------------------------------------*/
/*-------------------------- File BloomVariants.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The variants and the analysis of the filter of Bloom [see BloomFilter.h
 * for the standard one], beyond the book:
 *
 * - SpectralBloomFilter, the filter of Cohen and Matias for multisets: m
 *   counters instead of bits, an insertion of x with multiplicity f adds f
 *   to its r counters and a removal subtracts it, and the multiplicity of x
 *   is estimated by the least of its r counters (the minimum selection),
 *   never below the true one and above it only when all r counters of x
 *   are shared, with the probability of a false positive of the filter of
 *   Bloom with the same m, n and r. With counters of a few bits it is the
 *   counting filter of Fan, Cao, Almeida and Broder, which removes keys;
 *   a counter that reaches its largest value sticks there, as they do, so
 *   that a removal never makes a false negative;
 * - PartitionedBloomFilter, whose m bits are cut into r parts of m / r
 *   bits, each hash setting one bit of its own part, so that the r bits of
 *   a key are distinct: the expected bits set and the false positives are
 *   about those of the standard filter, \f$ ( 1 - e^{-r n / m} )^r \f$,
 *   with a slightly smaller variance (Grandi);
 * - the analysis: bloomFalsePositive(), the probability \f$ ( 1 - ( 1 - 1 /
 *   m )^{r n} )^r \approx ( 1 - e^{-r n / m} )^r \f$, least for \f$ r = ( m
 *   / n ) \ln 2 \f$, where a bit is 0 with probability 1 / 2 and the error
 *   is \f$ 0.6185^{m/n} \f$ [see bloomBestHashes()]; bloomLowerBound(), the
 *   \f$ n \log_2( 1 / \epsilon ) \f$ bits that any structure with false
 *   positives \f$ \epsilon \f$ and no false negative needs, of which a
 *   filter of Bloom uses \f$ \log_2 e \approx 1.44 \f$ times [see
 *   bloomBits()]; compressedBloom(), the trade-off of Mitzenmacher between
 *   the size of a filter and its entropy, by which a larger and sparser
 *   filter, compressed by an arithmetic coder, sends fewer bits for the
 *   same false positives;
 * - bloomIntersection(), the intersection of two sets on two machines by
 *   sending the filter of A and receiving the keys Q of B that it accepts,
 *   \f$ A \cap B \subseteq Q \f$, then \f$ A \cap Q \f$: about \f$ | A
 *   \cap B | + 0.6185^{m_A / | A |} | B | \f$ keys travel back, instead of
 *   the whole A.
 *
 * The r hashes of a key are \f$ h_1 + i h_2 \bmod m \f$ of two hashes, as
 * in BloomFilter. Other filters of today, closer to the lower bound for
 * small false positives, are the quotient filters (Bender et al.), the
 * cuckoo filters (Fan et al.) and the xor filters (Graf and Lemire), the
 * latter with about \f$ 1.23 \log_2( 1 / \epsilon ) \f$ bits per key.
 *
 * The contracts [see Contract.h]: m >= 1 and r >= 1 (m >= r for the
 * partitioned filter); a removal of x with multiplicity f only if the
 * estimate of x is at least f; an item inserted is always maybe there and
 * its estimate is at least its multiplicity, postconditions; for the
 * analysis m >= 1, n >= 0, r >= 1, 0 < epsilon < 1. The functions are
 * defined in BloomVariants.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8 (section 8.7)
 * \see S. Cohen, Y. Matias, Spectral Bloom filters, Proceedings of SIGMOD
 *      2003, 241-252, 2003, doi:10.1145/872757.872787
 * \see L. Fan, P. Cao, J. Almeida, A. Z. Broder, Summary cache: a scalable
 *      wide-area web cache sharing protocol, IEEE/ACM Transactions on
 *      Networking 8(3), 281-293, 2000, doi:10.1109/90.851975
 * \see M. Mitzenmacher, Compressed Bloom filters, IEEE/ACM Transactions on
 *      Networking 10(5), 604-612, 2002, doi:10.1109/TNET.2002.803864
 * \see A. Z. Broder, M. Mitzenmacher, Network applications of Bloom
 *      filters: a survey, Internet Mathematics 1(4), 485-509, 2004,
 *      doi:10.1080/15427951.2004.10129096
 * \see F. Grandi, On the analysis of Bloom filters, Information Processing
 *      Letters 129, 35-39, 2018, doi:10.1016/j.ipl.2017.09.004
 * \see T. M. Graf, D. Lemire, Xor filters: faster and smaller than Bloom
 *      and cuckoo filters, ACM Journal of Experimental Algorithmics 25,
 *      article 1.5, 2020, doi:10.1145/3376122
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

#ifndef __ADS_BloomVariants
 #define __ADS_BloomVariants
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/set/BloomFilter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <unordered_set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the probability ( 1 - ( 1 - 1 / m )^( r n ) )^r of a false positive of a
/// filter of m bits and r hashes after n insertions
/// \pre m >= 1 and r >= 1
[[nodiscard]] inline double bloomFalsePositive( double m , double n ,
                                                int r );

/// the number of hashes, floor or ceiling of ( m / n ) ln 2, of the least
/// false positives
/// \pre m >= 1 and n >= 1
[[nodiscard]] inline int bloomBestHashes( double m , double n );

/// n log2( 1 / epsilon ), the bits that any filter of n keys with false
/// positives epsilon needs
/// \pre n >= 0 and 0 < epsilon < 1
[[nodiscard]] inline double bloomLowerBound( double n , double epsilon );

/// n log2( 1 / epsilon ) / ln 2, the bits of a filter of Bloom with the
/// best number of hashes and false positives epsilon
/// \pre n >= 0 and 0 < epsilon < 1
[[nodiscard]] inline double bloomBits( double n , double epsilon );

/// the false positives and the bits per key after an arithmetic coding,
/// m / n times the entropy of a bit, of a filter of m / n = bitsPerKey
/// bits per key and r hashes
/// \pre bitsPerKey > 0 and r >= 1
[[nodiscard]] inline std::pair< double , double > compressedBloom(
                                                   double bitsPerKey , int r );

/// the intersection of A and B by a filter of A of mA bits: the keys Q of
/// B that the filter accepts, and the keys of A in Q, in the order of B
/// \pre mA >= 1 and A not empty
/// \post the second is the exact intersection, within the first
template< class T , class H = std::hash< T > >
[[nodiscard]] std::pair< std::vector< T > , std::vector< T > >
 bloomIntersection( const std::vector< T > & A , const std::vector< T > & B ,
                    std::size_t mA );

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS SpectralBloomFilter ------------------------*/
/*--------------------------------------------------------------------------*/

/// a filter of Bloom of m counters C and r hashes, for multisets
template< class T , class C = std::uint32_t , class H = std::hash< T > >
class SpectralBloomFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter of m counters and r hashes
 /// \pre m >= 1 and r >= 1
 SpectralBloomFilter( std::size_t m , int r , H h = H() );

 /// adds f to the multiplicity of x
 /// \post count( x ) >= f, and maybe( x ) if f > 0
 void insert( const T & x , C f = 1 );

 /// subtracts f from the multiplicity of x; a stuck counter stays
 /// \pre count( x ) >= f
 void remove( const T & x , C f = 1 );

 /// the least counter of x: never below its multiplicity
 [[nodiscard]] C count( const T & x ) const;

 /// false only if x is not there
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the counters at their largest value, which no longer move
 [[nodiscard]] std::size_t stuck( void ) const;

 [[nodiscard]] std::size_t counters( void ) const;

 [[nodiscard]] int hashes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the places of the r counters of x
 [[nodiscard]] std::vector< std::size_t > places( const T & x ) const;

 std::vector< C > f_C;
 int f_r;
 H f_h;

 };  // end( class SpectralBloomFilter )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS PartitionedBloomFilter ----------------------*/
/*--------------------------------------------------------------------------*/

/// a filter of Bloom of r parts of m / r bits, a hash for each part
template< class T , class H = std::hash< T > >
class PartitionedBloomFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter of about m bits in r parts
 /// \pre r >= 1 and m >= r
 PartitionedBloomFilter( std::size_t m , int r , H h = H() );

 /// \post maybe( x )
 void insert( const T & x );

 /// false only if x was never inserted
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the bits set
 [[nodiscard]] std::size_t ones( void ) const;

 /// the bits, r times those of a part
 [[nodiscard]] std::size_t bits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the bit of x in the part i
 [[nodiscard]] std::size_t bit( const T & x , int i ) const;

 std::vector< bool > f_B;
 std::size_t f_w;                  ///< the bits of a part
 int f_r;
 H f_h;

 };  // end( class PartitionedBloomFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/BloomVariants.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BloomVariants.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BloomVariants.h ------------------------*/
/*--------------------------------------------------------------------------*/
