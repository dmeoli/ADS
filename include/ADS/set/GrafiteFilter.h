/*--------------------------------------------------------------------------*/
/*-------------------------- File GrafiteFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range filters of Costa, Ferragina and Vinciguerra, beyond the book:
 * GrafiteFilter, a static filter of a set S of n keys that tells whether a
 * range \f$ [ a , b ] \f$ of the universe may hold a key of S with a false
 * positive probability that depends only on the length of the range,
 * whatever the keys and the queries (adversarial ones included), and
 * BucketingFilter, the heuristic of the same paper, faster and with no
 * such guarantee. Both answer the queries of a point too, as the range
 * \f$ [ x , x ] \f$.
 *
 * A range filter answers "maybe" for every range that holds a key of S
 * (no false negatives) and for an empty range of length \f$ \ell = b - a
 * + 1 \f$ with probability \f$ \varepsilon( \ell ) \f$, the false positive
 * probability. The filters of points (Bloom, binary fuse, Ribbon, cuckoo
 * [see BloomFilter.h, BinaryFuseFilter.h, RibbonFilter.h, CuckooFilter.h])
 * answer a range only by asking each of its \f$ \ell \f$ points, which
 * costs \f$ \ell \f$ queries and \f$ \ell \f$ times their false positive
 * probability; the tries of SuRF and the learned models of SNARF and
 * Proteus answer a range in one step, but a query close to a key, which
 * the workloads of a key-value store often ask (a correlated query), finds
 * them unable to tell the range from the key, and their false positives
 * go to 1 (Oasis, a learned filter of segments, shares the weakness).
 * Grafite is the filter whose guarantee holds for every query, within 2
 * bits per key of the lower bound; the robust filters after it, Memento
 * and Diva, keep a similar guarantee while they take insertions and keys
 * of any length, at the price of more bits and slower queries, so that
 * for a static set of keys of 64 bits Grafite remains the choice.
 *
 * Theory. With B bits per key, Grafite reduces the universe
 * \f$ [ 0 , 2^{64} ) \f$ to \f$ [ 0 , r ) \f$, \f$ r = n \, 2^{B-2} \f$,
 * by the hash
 * \f[
 *   h( x ) = \bigl( q( \lfloor x / r \rfloor ) + x \bigr) \bmod r ,
 * \f]
 * with q a hash of the blocks \f$ \lfloor x / r \rfloor \f$ of r
 * consecutive keys into \f$ [ 0 , r ) \f$, drawn from a pairwise
 * independent family: inside a block, h is the rotation of the keys by
 * q of the block, so that the keys of a range in one block go to a
 * range of \f$ [ 0 , r ) \f$ of the same length, cyclically (the
 * locality), while the blocks are rotated independently of each other.
 * The codes h( S ), sorted and without repetitions, are kept by
 * Elias-Fano [see EliasFano.h] in the universe r, in
 * \f$ n ( 2 + \lceil \log_2( r / n ) \rceil ) = n B \f$ bits and the
 * directory of select, and a range \f$ [ a , b ] \f$ with \f$ b - a < r \f$
 * meets at most two blocks, whose images are at most two cyclic ranges
 * of \f$ [ 0 , r ) \f$ of \f$ \ell \f$ codes in all: the answer is
 * whether a code lies in them, one nextGEQ() and one access() each, in
 * \f$ O( 1 ) \f$ time for a constant B. A key of S in \f$ [ a , b ] \f$
 * has its
 * code in them, so that there is no false negative. A key x of S out of
 * \f$ [ a , b ] \f$ in a block of the range has its code out of the
 * image of the part of the range in its own block, since h is one to one
 * on a block of r keys; with respect to the other part of the range, and
 * to all of it for a key in another block, the code
 * \f$ ( q( j ) + x ) \bmod r \f$ of x in the block j is uniform on
 * \f$ [ 0 , r ) \f$ given q of the other block, q being pairwise
 * independent, so that x is a false positive with probability at most
 * \f$ \ell / r \f$. By the union bound over the n keys,
 * \f[
 *   \varepsilon( \ell ) \le \min\Bigl( 1 , \frac{ n \ell }{ r } \Bigr) =
 *   \min\Bigl( 1 , \frac{ \ell }{ 2^{B-2} } \Bigr) ,
 * \f]
 * whatever S and the range, since the only randomness is that of q
 * (Theorem 3.4 and Corollary 3.5 of the paper, which generalize the
 * construction of Goswami, Grønlund, Larsen and Pagh); the lower bound
 * of the same paper needs \f$ n \log_2( \ell / \varepsilon ) - O( n ) \f$
 * bits, within 2 bits per key of Grafite. A range with \f$ b - a \ge r \f$
 * has \f$ n \ell / r \ge 1 \f$, and the filter answers maybe if S is not
 * empty. The paper gives r as \f$ n L / \varepsilon \f$ for the ranges
 * of length at most L and the probability \f$ \varepsilon \f$: with
 * B bits per key this is \f$ \varepsilon = L / 2^{B-2} \f$, so that L
 * need not be fixed in advance.
 *
 * The hash q here is the multiply-add-shift of Dietzfelbinger, on 128
 * bits, \f$ q'( j ) = \lfloor ( \alpha j + \beta ) \bmod 2^{128} /
 * 2^{64} \rfloor \f$ with \f$ \alpha , \beta \f$ random words of 128 bits
 * from the seed, pairwise independent from the words of 64 bits to those
 * of 64 bits (the Carter-Wegman hash modulo a prime of the paper needs a
 * prime above the \f$ 2^{64} / r \f$ blocks), mapped onto
 * \f$ [ 0 , r ) \f$ by \f$ q( j ) = \lfloor q'( j ) \, r / 2^{64}
 * \rfloor \f$, whose values have probabilities within
 * \f$ 1 / r \pm 2^{-64} \f$, which adds at most \f$ n \ell 2^{-64} \f$ to
 * \f$ \varepsilon( \ell ) \f$.
 *
 * In pseudocode (the query of Grafite, beyond the book), with the m
 * codes sorted in C[ 0 .. m - 1 ] and \f$ a \le b \f$:
 * \f[
 * \begin{array}{r@{\quad}l}
 * 1 & \textbf{function}\ \mathrm{mayContain}( a , b ) \\
 * 2 & \quad \textbf{if}\ m = 0\ \textbf{then}\ \textbf{return}\
 *     \textbf{false} \\
 * 3 & \quad \textbf{if}\ b - a \ge r\ \textbf{then}\ \textbf{return}\
 *     \textbf{true}\quad \triangleright\ \text{the bound is } 1 \\
 * 4 & \quad j_a \gets \lfloor a / r \rfloor;\ j_b \gets
 *     \lfloor b / r \rfloor \\
 * 5 & \quad \textbf{if}\ j_a = j_b\ \textbf{then}\ \textbf{return}\
 *     \mathrm{meets}( h( a ) , h( b ) ) \\
 * 6 & \quad c \gets j_b \, r\quad \triangleright\ \text{the first key of
 *     the block of } b \\
 * 7 & \quad \textbf{return}\ \mathrm{meets}( h( a ) , h( c - 1 ) )\
 *     \textbf{or}\ \mathrm{meets}( h( c ) , h( b ) ) \\
 * 8 & \textbf{function}\ \mathrm{meets}( x , y )\quad \triangleright\
 *     \text{a code in the cyclic range from } x \text{ to } y \\
 * 9 & \quad p \gets \mathrm{nextGEQ}( x ) \\
 * 10 & \quad \textbf{if}\ x \le y\ \textbf{then}\ \textbf{return}\
 *     p < m\ \textbf{and}\ C[ p ] \le y \\
 * 11 & \quad \textbf{return}\ p < m\ \textbf{or}\ C[ 0 ] \le y\quad
 *     \triangleright\ \text{the range wraps around } r
 * \end{array}
 * \f]
 * and the build: the n distinct keys, \f$ r \gets n \, 2^{B-2} \f$, the
 * codes \f$ h( x ) \f$ of the keys sorted and once each, and their
 * Elias-Fano in the universe r.
 *
 * BucketingFilter cuts the universe into buckets of s keys,
 * \f$ s = \lceil 2^{64} / ( n \, 2^{B-2} ) \rceil \f$, and keeps by
 * Elias-Fano the buckets \f$ \lfloor x / s \rfloor \f$ that hold a key,
 * in at most B bits per key; \f$ [ a , b ] \f$ may hold a key if a kept
 * bucket is in \f$ [ \lfloor a / s \rfloor , \lfloor b / s \rfloor ]
 * \f$, one nextGEQ() and one access(). A query whose range is out of the
 * buckets of S is answered right, which makes the filter exact on keys
 * and queries drawn independently at random from the universe, and
 * faster to build and to query than Grafite (no hash); but a query that
 * falls in the bucket of a key without holding it (a correlated query,
 * at distance below s from the key) is always a false positive: there is
 * no bound that holds for every query.
 *
 * The keys are of a type T that radixKey() maps in order onto unsigned
 * words [see RadixSort.h]: integers and floating point numbers, whose
 * ranges are those of their order (-0 and +0 are two keys, -0 the
 * smaller, as the total order of IEEE 754 has them).
 *
 * The contracts [see Contract.h]: \f$ 2 \le B \le 40 \f$ and
 * \f$ n \, 2^{B-2} < 2^{63} \f$; a range has \f$ a \le b \f$; after the
 * build every key is maybe there, an audit. The operations are defined in
 * GrafiteFilter.tpp.
 *
 * \see M. Costa, P. Ferragina, G. Vinciguerra, Grafite: taming adversarial
 *      queries with optimal range filters, Proceedings of the ACM on
 *      Management of Data 2(1), 1-23, 2024, doi:10.1145/3639258
 * \see M. Goswami, A. Grønlund, K. G. Larsen, R. Pagh, Approximate range
 *      emptiness in constant time and optimal space, Proceedings of the
 *      26th ACM-SIAM Symposium on Discrete Algorithms (SODA), 769-775,
 *      2015, doi:10.1137/1.9781611973730.52
 * \see M. Dietzfelbinger, Universal hashing and k-wise independent random
 *      variables via integer arithmetic without primes, Proceedings of
 *      STACS 96, LNCS 1046, 567-580, 1996, doi:10.1007/3-540-60922-9_46
 * \see M. N. Wegman, J. L. Carter, New hash functions and their use in
 *      authentication and set equality, Journal of Computer and System
 *      Sciences 22(3), 265-279, 1981, doi:10.1016/0022-0000(81)90033-7
 * \see H. Zhang, H. Lim, V. Leis, D. G. Andersen, M. Kaminsky, K. Keeton,
 *      A. Pavlo, SuRF: practical range query filtering with fast succinct
 *      tries, Proceedings of SIGMOD 2018, 323-336, 2018,
 *      doi:10.1145/3183713.3196931
 * \see S. Luo, S. Chatterjee, R. Ketsetsidis, N. Dayan, W. Qin,
 *      S. Idreos, Rosetta: a robust space-time optimized range filter for
 *      key-value stores, Proceedings of SIGMOD 2020, 2071-2086, 2020,
 *      doi:10.1145/3318464.3389731
 * \see K. Vaidya, S. Chatterjee, E. Knorr, M. Mitzenmacher, S. Idreos,
 *      T. Kraska, SNARF: a learning-enhanced range filter, Proceedings of
 *      the VLDB Endowment 15(8), 1632-1644, 2022,
 *      doi:10.14778/3529337.3529347
 * \see E. R. Knorr, B. Lemaire, A. Lim, S. Luo, H. Zhang, S. Idreos,
 *      M. Mitzenmacher, Proteus: a self-designing range filter,
 *      Proceedings of SIGMOD 2022, 1670-1684, 2022,
 *      doi:10.1145/3514221.3526167
 * \see N. Eslami, N. Dayan, Memento filter: a fast, dynamic, and robust
 *      range filter, Proceedings of the ACM on Management of Data 2(6),
 *      244, 1-27, 2024, doi:10.1145/3698820
 * \see G. Chen, Z. He, M. Li, S. Luo, Oasis: an optimal disjoint segmented
 *      learned range filter, Proceedings of the VLDB Endowment 17(8),
 *      1911-1924, 2024, doi:10.14778/3659437.3659447
 * \see N. Eslami, I. O. Bercea, N. Dayan, Diva: dynamic range filter for
 *      var-length keys and queries, Proceedings of the VLDB Endowment
 *      18(11), 3923-3936, 2025, doi:10.14778/3749646.3749664
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

#ifndef __ADS_GrafiteFilter
 #define __ADS_GrafiteFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS GrafiteFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a static range filter of the keys of type T, B bits per key, with
/// false positive probability at most \f$ \ell / 2^{B-2} \f$ for a range
/// of \f$ \ell \f$ keys, whatever the keys and the range
template< RadixKey T = std::uint64_t >
class GrafiteFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the filter of the keys, which may repeat, in B bits per key
 /// \pre \f$ 2 \le B \le 40 \f$ and \f$ n \, 2^{B-2} < 2^{63} \f$
 /// \post every key is maybe there (an audit)
 explicit GrafiteFilter( const std::vector< T > & keys , int B = 16 ,
                         std::uint64_t seed = 1 );

 /// true if \f$ [ a , b ] \f$ holds a key, and with probability at most
 /// \f$ ( b - a + 1 ) / 2^{B-2} \f$ otherwise
 /// \pre \f$ a \le b \f$
 [[nodiscard]] bool mayContain( const T & a , const T & b ) const;

 /// mayContain( x , x ): true for a key, and with probability at most
 /// \f$ 2^{-(B-2)} \f$ otherwise
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the bound \f$ \min( 1 , \ell / 2^{B-2} ) \f$ of the false positive
 /// probability of a range of \f$ \ell \f$ keys
 [[nodiscard]] double falsePositiveBound( double l ) const;

 /// the keys of the build, the repeated ones once each
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the distinct codes, n less the keys of S that share a code
 [[nodiscard]] std::size_t codes( void ) const;

 /// the reduced universe \f$ r = n \, 2^{B-2} \f$
 [[nodiscard]] std::uint64_t reduced( void ) const;

 /// the bits of the codes and of the directory of select
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the code \f$ h( x ) \f$ of the word x
 [[nodiscard]] std::uint64_t code( std::uint64_t x ) const;

 /// whether a code is in the cyclic range from x to y of \f$ [ 0 , r ) \f$
 [[nodiscard]] bool meets( std::uint64_t x , std::uint64_t y ) const;

 EliasFano f_codes;
 std::uint64_t f_r = 1;                   ///< the reduced universe
 std::uint64_t f_alo = 0 , f_ahi = 0;     ///< alpha
 std::uint64_t f_blo = 0 , f_bhi = 0;     ///< beta
 std::uint64_t f_first = 0;               ///< the smallest code
 std::size_t f_n = 0;
 int f_B;

 };  // end( class GrafiteFilter )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BucketingFilter --------------------------*/
/*--------------------------------------------------------------------------*/

/// a static range filter of the keys of type T, B bits per key: the
/// buckets of \f$ 2^{64} / ( n \, 2^{B-2} ) \f$ keys that hold a key of
/// the set, with no bound on the false positives of the queries near a key
template< RadixKey T = std::uint64_t >
class BucketingFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the filter of the keys, which may repeat, in at most B bits per key
 /// \pre \f$ 2 \le B \le 40 \f$ and \f$ n \, 2^{B-2} < 2^{63} \f$
 /// \post every key is maybe there (an audit)
 explicit BucketingFilter( const std::vector< T > & keys , int B = 16 );

 /// true if \f$ [ a , b ] \f$ holds a key, and if a bucket of a key meets
 /// it
 /// \pre \f$ a \le b \f$
 [[nodiscard]] bool mayContain( const T & a , const T & b ) const;

 /// mayContain( x , x )
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the keys of a bucket, s
 [[nodiscard]] std::uint64_t bucketWidth( void ) const;

 /// the buckets that hold a key
 [[nodiscard]] std::size_t buckets( void ) const;

 /// the bits of the buckets and of the directory of select
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 EliasFano f_buckets;
 std::uint64_t f_s = 1;                   ///< the keys of a bucket

 };  // end( class BucketingFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/GrafiteFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* GrafiteFilter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File GrafiteFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
