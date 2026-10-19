/*--------------------------------------------------------------------------*/
/*---------------------------- File RecSplit.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimal perfect hash function of ADS by recursive splitting, the
 * RecSplit of Esposito, Mueller Graf and Vigna, beyond the book [see
 * MinimalPerfectHash.h for the one by pilots, faster but about 3 bits per
 * key]: a function that maps the n distinct keys of a static set S onto
 * \f$ \{ 0 , \ldots , n - 1 \} \f$ with no collision in about 1.8 bits per
 * key for the leaves of 8 keys and the buckets of 100, against the lower
 * bound of \f$ \log_2 e \approx 1.4427 \f$ bits per key of any minimal
 * perfect hash function.
 *
 * A key goes to the bucket \f$ \lfloor h_1 \lceil n / b \rceil / 2^{64}
 * \rfloor \f$ of \f$ \lceil n / b \rceil \f$ buckets, b keys each on
 * average, by its first hash \f$ h_1 \f$; the m keys of a bucket are then
 * split recursively by their second hash \f$ h_2 \f$, so that each leaf of
 * the splitting tree holds at most \f$ \ell \f$ keys (the template
 * parameter L). A node of m keys with the seed x sends a key to the place
 * \f$ \rho_x = \lfloor g_{x , d}( h_2 ) \, m / 2^{64} \rfloor \in [ 0 , m )
 * \f$, where \f$ g_{x , d} \f$ is a folded product salted by x and by the
 * depth d of the node, and its children by the place:
 *
 * - a leaf (\f$ m \le \ell \f$) needs a bijection, the seed x for which the
 *   m places are distinct, with probability \f$ m! / m^m \f$, so that its
 *   expected trials are \f$ m^m / m! \approx e^m / \sqrt{ 2 \pi m } \f$;
 * - a node of \f$ \ell < m \le s \ell \f$ keys, \f$ s = \max\{ 2 , \lceil
 *   0.35 \ell + 0.5 \rceil \} \f$ (the lower aggregation), goes into parts
 *   of \f$ \ell \f$ keys, the part \f$ \lfloor \rho_x / \ell \rfloor \f$,
 *   the last one with the rest;
 * - a node of \f$ s \ell < m \le t s \ell \f$ keys, \f$ t = \lceil 0.21
 *   \ell + 0.9 \rceil \f$ for \f$ \ell \ge 7 \f$ and 2 below (the upper
 *   aggregation), into parts of \f$ s \ell \f$ keys in the same way;
 * - a larger node into two parts, of \f$ u = \lceil \lfloor m / 2 \rfloor
 *   / ( t s \ell ) \rceil \, t s \ell \f$ and \f$ m - u \f$ keys, by
 *   \f$ \rho_x < u \f$.
 *
 * The seed of a node is the first x that gives each part its size, with
 * probability
 * \f[
 *   p = \frac{ m! }{ k_0! \cdots k_{s-1}! } \prod_{i} \Bigl( \frac{ k_i }{ m }
 *   \Bigr)^{ k_i }
 * \f]
 * for the parts of \f$ k_0 , \ldots , k_{s-1} \f$ keys, so that the seed is
 * geometric of parameter p and the product of the probabilities of all
 * the nodes of a tree of m keys is \f$ m! / m^m \f$ whatever the shape (the
 * invariance of RecSplit). The seeds are written in preorder by a code of
 * Golomb and Rice, whose parameter for a geometric source of parameter p
 * is the optimal one of Kiely,
 * \f[
 *   r( p ) = \max\Bigl\{ 0 , \Bigl\lceil \log_2 \frac{ -\log_2 \varphi }{
 *   \log_2 ( 1 - p ) } \Bigr\rceil \Bigr\} , \qquad
 *   \varphi = \frac{ 1 + \sqrt{ 5 } }{ 2 } ,
 * \f]
 * a function of m alone, so that the code of a seed takes \f$ c + \log_2 (
 * 1 / p ) \f$ bits in expectation with \f$ 1.56 \le c < 2.56 \f$, and a
 * tree of m keys with t nodes \f$ c t + m \log_2 e + O( \log m ) \f$ bits.
 * The r( p ) low bits of all the seeds of a bucket come first, then all
 * their unary parts; a table of the sizes \f$ m < 4096 \f$, computed once
 * for each \f$ \ell \f$, gives r, the low bits of a subtree and its number
 * of nodes, so that a subtree is skipped by moving a pointer on the low
 * bits and by a select on the unary parts. The numbers of keys before each
 * bucket and the place of its code are two Elias-Fano sequences [see
 * EliasFano.h], the second one less \f$ \beta \f$ times the first, for the
 * \f$ \beta \f$ bits per key of the codes, and both less i times their
 * least gap, which leaves to their low bits only the deviations from the
 * average.
 *
 * The search of a leaf uses the rotation fitting of Bez, Kurpicz, Lehmann
 * and Sanders: the keys of the leaf go to two sets A and B by the highest
 * bit of \f$ h_2 \f$; for each x, the places of A and of B are two masks
 * of m bits (x is discarded if a set has two keys in one place), and the
 * leaf takes the seed \f$ x m + r \f$ for the first rotation r of the mask
 * of B that fills the holes of the mask of A. A key of B goes to the place
 * \f$ ( \rho_x + r ) \bmod m \f$; each x tries m rotations, so that the
 * leaf evaluates about m times fewer hashes for the same expected seed.
 * The leaves of \f$ \ell \le 24 \f$ keys fit in a word, and the search
 * grows as \f$ e^\ell \f$: 1 <= L <= 24 (a build at \f$ \ell = 16 \f$ is
 * already slow).
 *
 * An evaluation computes the two hashes and the bucket, reads the three
 * values of the Elias-Fano sequences (three select) and descends the
 * \f$ O( \log ( b / \ell ) ) \f$ levels of the tree, each one a read of
 * the low bits, a scan of the unary part and, to skip the earlier
 * siblings, a select in a word or a few; the build takes expected
 * \f$ O( n \, e^\ell / ( \ell \sqrt{ \ell } ) ) \f$ time. For a key out
 * of S the result is some place below n.
 *
 * The keys are hashed by h( k ), taken as it is if H declares
 * is_avalanching [see FastHash.h] and mixed by hashWord() otherwise; the
 * two hashes of 64 bits are two folded products of that hash XOR the seed,
 * 128 bits per key. Two keys of a bucket with the same \f$ h_2 \f$, or a
 * bucket of 4096 keys or more, make the build draw a new seed; two keys
 * with the same hash of 64 bits cannot be separated.
 *
 * Consensus-RecSplit of Lehmann, Sanders, Walzer and Ziegler is the space
 * frontier of this family: it searches the seeds of consecutive nodes
 * together, each one a continuation of the bits of the previous ones, and
 * takes \f$ ( 1 + \varepsilon ) \, n \log_2 e \f$ bits in \f$ O( n /
 * \varepsilon ) \f$ expected time.
 *
 * The contracts [see Contract.h]: the keys distinct (an audit), fewer than
 * \f$ 2^{32} \f$, 1 <= b <= 2000; after the build the keys are mapped onto
 * { 0 , ... , n - 1 }, an audit. A build that draws 100 seeds with no
 * success throws std::runtime_error. The operations are defined in
 * RecSplit.tpp.
 *
 * \see E. Esposito, T. Mueller Graf, S. Vigna, RecSplit: minimal perfect
 *      hashing via recursive splitting, Proceedings of the 22nd Workshop on
 *      Algorithm Engineering and Experiments (ALENEX 2020), 175-185, 2020,
 *      doi:10.1137/1.9781611976007.14
 * \see S. W. Golomb, Run-length encodings, IEEE Transactions on Information
 *      Theory 12(3), 399-401, 1966, doi:10.1109/TIT.1966.1053907
 * \see A. Kiely, Selecting the Golomb parameter in Rice coding, IPN
 *      Progress Report 42-159, Jet Propulsion Laboratory, 1-18, 2004,
 *      https://ipnpr.jpl.nasa.gov/progress_report/42-159/159E.pdf
 * \see P. Elias, Efficient storage and retrieval by content and address of
 *      static files, Journal of the ACM 21(2), 246-260, 1974,
 *      doi:10.1145/321812.321820
 * \see D. Bez, F. Kurpicz, H.-P. Lehmann, P. Sanders, High performance
 *      construction of RecSplit based minimal perfect hash functions,
 *      Proceedings of the 31st European Symposium on Algorithms (ESA 2023),
 *      LIPIcs 274, 19:1-19:16, 2023, doi:10.4230/LIPIcs.ESA.2023.19
 * \see H.-P. Lehmann, P. Sanders, S. Walzer, J. Ziegler, Combined search
 *      and encoding for seeds, with an application to minimal perfect
 *      hashing, Proceedings of the 33rd European Symposium on Algorithms
 *      (ESA 2025), LIPIcs 351, 109:1-109:18, 2025,
 *      doi:10.4230/LIPIcs.ESA.2025.109
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

#ifndef __ADS_RecSplit
 #define __ADS_RecSplit
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the tables of the splitting trees of RecSplit for the sizes m of a
/// subtree: the parameter of Golomb and Rice of its root, the low bits of
/// all its codes and its number of codes
struct RecSplitTables {
 std::vector< std::uint8_t > rice;
 std::vector< std::uint32_t > low;
 std::vector< std::uint16_t > nodes;
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS RecSplit -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a minimal perfect hash function of a static set of keys by recursive
/// splitting, with leaves of at most L keys
template< class T , unsigned L = 8 , class H = DefaultHash< T > >
class RecSplit {

 static_assert( ( L >= 1 ) && ( L <= 24 ) , "RecSplit: 1 <= L <= 24" );

/*--------------------------------------------------------------------------*/
 public:

 using Key = T;   ///< the type of the keys

 /// the function of the distinct keys, with b keys per bucket on average
 /// \pre the keys are distinct (an audit) and fewer than 2^32, and
 ///      1 <= b <= 2000
 /// \post the keys are mapped onto { 0 , ... , n - 1 } (an audit)
 explicit RecSplit( const std::vector< T > & keys , std::size_t b = 100 ,
                    std::uint64_t seed = 1 , H h = H() );

 /// the value of k, in [ 0 , n ); some value in it for a key out of S
 /// \pre size() >= 1
 [[nodiscard]] std::size_t operator()( const T & k ) const;

 /// the number n of keys
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bits of the function: the codes, the two Elias-Fano sequences
 /// with their directories and the object
 [[nodiscard]] std::size_t bits( void ) const;

 /// the number of buckets
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the seeds drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxTrials = 100;
 static constexpr std::size_t kMaxBucket = 4096;  ///< the bound of a size
 /// the parts of the lower and of the upper aggregation
 static constexpr std::size_t kLower =
  std::max< std::size_t >( 2 , ( 35 * L + 50 + 99 ) / 100 ) * L;
 static constexpr std::size_t kUpper =
  ( L >= 7 ? ( 21 * L + 90 + 99 ) / 100 : 2 ) * kLower;

 /// the tables of L, computed at the first use
 [[nodiscard]] static const detail::RecSplitTables & tables( void );

 /// the size of the left part of a node of m > kUpper keys
 [[nodiscard]] static constexpr std::size_t left( std::size_t m );

 /// the hash of a key of second hash h at the depth d with the seed x
 [[nodiscard]] static std::uint64_t node( std::uint64_t h ,
                                          std::uint64_t x , unsigned d );

 /// the place in [ 0 , m ) of the hash g
 [[nodiscard]] static std::size_t remap( std::uint64_t g , std::size_t m );

 /// the hash of k before the seed, mixed unless H avalanches
 [[nodiscard]] std::uint64_t base( const T & k ) const;

 /// the 64 bits of the codes from the bit at
 [[nodiscard]] std::uint64_t window( std::uint64_t at ) const;

 /// the place after the c-th 1 of the codes from the bit at
 [[nodiscard]] std::uint64_t skipOnes( std::uint64_t at ,
                                       std::size_t c ) const;

 /// the seeds of the m keys K of a node at the depth d, in preorder, as
 /// the pairs ( seed , bits of its low part ); S is as long as K
 static void split( std::uint64_t * K , std::uint64_t * S , std::size_t m ,
                    unsigned d , std::vector< std::uint64_t > & seeds ,
                    std::vector< std::uint8_t > & rice );

 /// the codes of the base hashes B; false if two hashes of a bucket
 /// cannot be separated or a bucket is too large
 bool build( const std::vector< std::uint64_t > & B , std::size_t b );

 std::uint64_t f_seed;
 std::size_t f_n = 0;
 std::size_t f_nb = 0;                 ///< the buckets
 std::vector< std::uint64_t > f_code;  ///< the codes, two words more
 EliasFano f_keys;          ///< the keys before bucket i, less i f_dk
 EliasFano f_place;         ///< the place of bucket i, less its estimate
 std::uint64_t f_dk = 0;    ///< the least gap of the keys before
 std::int64_t f_dp = 0;     ///< the least gap of the places, less beta
 std::uint64_t f_beta = 0;  ///< the bits per key of the codes, by 2^16
 const detail::RecSplitTables * f_t = nullptr;
 std::size_t f_trials = 0;
 H f_h;

 };  // end( class RecSplit )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/RecSplit.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RecSplit.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RecSplit.h ---------------------------*/
/*--------------------------------------------------------------------------*/
