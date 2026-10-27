/*--------------------------------------------------------------------------*/
/*--------------------------- File PrefixFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The prefix filter of Even, Even and Morrison, beyond the book: an
 * incremental filter, which takes insertions one at a time and no
 * removals, of a set \f$ S \f$ of at most \f$ n \f$ keys, \f$ n \f$ known
 * at its construction, answering maybe for every key of \f$ S \f$ and for a
 * key out of \f$ S \f$ with a small probability. Its first level is a table
 * of \f$ m \f$ bins, each a pocket dictionary of 32 bytes (half a line of
 * cache) that holds at most \f$ k = 25 \f$ fingerprints; its second level,
 * the spare, is a blocked filter of Bloom [see `BlockedBloomFilter.h`] that
 * takes the fingerprints for which the bins have no room. A query reads one
 * bin, and the spare only in the rare case described below, so that it
 * costs one miss of the cache in almost every case, against two of the
 * cuckoo filter [see `CuckooFilter.h`]; an insertion never moves a
 * fingerprint from a bin to another, so that its time does not grow with
 * the load.
 *
 * A key with the hash \f$ h \f$ has the fingerprint \f$ ( b , q , r ) \f$:
 * its bin \f$ b = \lfloor h_1 m / 2^{32} \rfloor \f$, \f$ h_1 \f$ the high
 * 32 bits of \f$ h \f$, and its mini-fingerprint
 * \f$ x = \lfloor h_0 s / 2^{32} \rfloor \f$ in \f$ [ 0 , s ) \f$,
 * \f$ h_0 \f$ the low 32 bits of \f$ h \f$ and
 * \f$ s = Q 2^R = 25 \cdot 2^8 = 6400 \f$, cut into the quotient
 * \f$ q = \lfloor x / 2^8 \rfloor < Q = 25 \f$ and the remainder
 * \f$ r = x \bmod 2^8 \f$ (both reductions by a product, as Lemire). A bin
 * is the pocket dictionary \f$ \mathrm{PD}( Q , R , k ) =
 * \mathrm{PD}( 25 , 8 , 25 ) \f$ of Bercea and Even: a header of
 * \f$ Q + k = 50 \f$ bits that writes the number \f$ c_q \f$ of the
 * remainders of each quotient \f$ q \f$ in unary,
 * \f$ 1^{c_0} 0 \, 1^{c_1} 0 \cdots 1^{c_{24}} 0 \f$ from the low bit, 6
 * bits for the quotient of the largest fingerprint of the bin once it has
 * overflowed (63 before), and a body of 25 bytes holding the remainders in
 * the order of their quotients, those of one quotient (its run) in
 * increasing order. The run of \f$ q \f$ is the slots
 * \f$ [ z_{q-1} + 1 - q , z_q - q ) \f$ of the body, \f$ z_q \f$ being the
 * place of the zero of rank \f$ q \f$ of the header (\f$ z_{-1} = -1 \f$):
 * with a zero put below the header, its zeros of ranks \f$ q \f$ and
 * \f$ q + 1 \f$ are \f$ z_{q-1} + 1 \f$ and \f$ z_q + 1 \f$, both found by
 * one `pdep` of \f$ 3 \cdot 2^q \f$ into the complement (BMI2), or by
 * clearing the lowest bit \f$ q \f$ times on any machine; the `pdep` gives
 * the two bits \f$ d_0 = 2^{z_{q-1}+1} \f$ and \f$ d_1 = 2^{z_q+1} \f$, and
 * the slots of the run are the bits of \f$ ( d_1 / 2 - d_0 ) / 2^q \f$, with
 * no count of zeros. A query compares the 32 bytes of the bin with
 * \f$ r \f$ at once (AVX-512 or AVX2, a byte at a time on any machine),
 * stops if no byte of the body is equal, as it is for about \f$ 91 \% \f$
 * of the keys out of \f$ S \f$ (\f$ 1 - ( 1 - 2^{-8} )^{23.75} \f$ of them
 * have one), and otherwise tells whether one of the equal bytes is in the
 * run of \f$ q \f$. A key not found in the bin is compared with the bound
 * of \f$ x \f$ in the bin, its largest fingerprint if it overflowed and
 * above every \f$ x \f$ if not, which is the 6 bits of the quotient and the
 * last byte of the body; the bound is out of the path of a key found, and
 * the only branch on the bin besides those on the equal bytes is the rare
 * one to the spare, out of line, a mispredicted branch on a load costing
 * the loads of the queries in flight. An insertion finds the place of
 * \f$ r \f$ in its run by counting the bytes of the run below \f$ r \f$ (one
 * comparison of the 32 bytes), and moves the bytes above it by one within a
 * register of 256 bits (AVX2), or within 4 words on any machine, while one
 * more one goes into the header at the end of the run. The answers are the
 * same on every machine.
 *
 * The bins keep the prefix of their fingerprints. Each bin holds the
 * smallest, in the order of \f$ x = 2^8 q + r \f$, of the fingerprints that
 * came to it, at most 25: an insertion into a full bin with \f$ x \f$ not
 * below the maximum \f$ x_{\max} \f$ of the bin sends \f$ ( b , x ) \f$ to
 * the spare, and one with \f$ x < x_{\max} \f$ evicts \f$ x_{\max} \f$ to the
 * spare and takes its place; either way the bin is marked overflowed, by
 * writing in the header the quotient of its maximum. Its maximum is the last
 * byte of the body, with the quotient \f$ \lfloor \log_2 w \rfloor - 24 \f$
 * of the last run, \f$ w \f$ the header (written again after an eviction),
 * since the maximum of a bin never grows once it is full. Every fingerprint
 * sent to the spare is then at least the maximum of its bin, and a query of
 * \f$ x \f$ looks in the bin and, if \f$ x \f$ is not there, in the spare
 * only if the bin has overflowed and \f$ x > x_{\max} \f$ (a key of the bin
 * is at most \f$ x_{\max} \f$): a key of \f$ S \f$ is always found, the
 * fingerprint equal to the maximum being in the bin. The spare holds the
 * pairs \f$ s b + x \f$ hashed again.
 *
 * Analysis. Let \f$ n \f$ keys fall into
 * \f$ m = \lceil n / ( \alpha k ) \rceil \f$ bins, \f$ \alpha = 0.95 \f$, so
 * that the load of a bin is the binomial
 * \f$ B \sim \mathrm{Bin}( n , 1 / m ) \f$ of mean \f$ \alpha k \f$. The
 * fingerprints sent to the spare are \f$ X = \sum_b ( B_b - k )^+ \f$, of
 * mean
 * \f[ \mathrm{E}[ X ] = n - m \, \mathrm{E}[ \min( B , k ) ] \;, \qquad
 *     \mathrm{E}[ X ] = n \Bigl( 1 - \frac{1}{m} \Bigr)
 *     \Pr[ B = k ] \le \frac{n}{\sqrt{2 \pi k}} \quad
 *     \text{for } m = n / k \f]
 * (Even et al., Theorem 5, by the bound of Stirling), i.e., \f$ 8.0 \% \f$
 * of the keys at \f$ \alpha = 1 \f$, and \f$ 5.9 \% \f$ at
 * \f$ \alpha = 0.95 \f$ [see `expectedSpareSize()`]; \f$ X \f$ is
 * concentrated around its mean, and the spare is built for
 * \f$ n' = 1.1 \, \mathrm{E}[ X ] \f$ fingerprints, as the paper suggests,
 * at 12 bits each (\f$ \varepsilon' \approx 0.35 \% \f$). A blocked filter
 * of Bloom never fails: a spare that gets more than \f$ n' \f$ fingerprints
 * only answers yes more often, so that every insertion succeeds until
 * \f$ n \f$ keys are in, after which an insertion returns false and changes
 * nothing.
 *
 * A key out of \f$ S \f$ whose bin got \f$ j \f$ fingerprints is a false
 * positive if its \f$ x \f$ is one of them, whether the bin kept it or sent
 * it to the spare (which answers yes to it, holding it exactly), or if the
 * bin overflowed, \f$ x \f$ is above the \f$ k \f$-th smallest of the
 * \f$ j \f$ fingerprints (with the probability
 * \f$ ( j + 1 - k ) / ( j + 1 ) \f$ by the rank of \f$ x \f$ among them) and
 * the spare answers yes to a pair it does not hold, with its probability
 * \f$ \varepsilon' \f$:
 * \f[ \varepsilon \approx \sum_{j \ge 0} \Pr[ B = j ] \Bigl( 1 -
 *     \Bigl( 1 - \frac{1}{s} \Bigr)^j + [ j > k ] \,
 *     \frac{j + 1 - k}{j + 1} \, \varepsilon' \Bigr)
 *     \le \frac{\alpha k}{s} + \frac{\varepsilon'}{\sqrt{2 \pi k}} \;, \f]
 * the bound being Theorem 2 of the paper, \f$ k / s = 1 / 256 \f$ [see
 * `falsePositiveRate()`, the sum at the current load]: \f$ 0.39 \% \f$ when
 * full, \f$ 0.37 \% \f$ from the fingerprints and \f$ 0.02 \% \f$ from the
 * spare, about the \f$ 1 / 256 \f$ of a filter of fingerprints of 8 bits; a
 * spare of 8 bits per fingerprint would save 0.26 bits per key and add
 * \f$ 0.1 \% \f$ of false positives. The bins take
 * \f$ 256 / ( \alpha k ) = 10.78 \f$ bits per key and the spare
 * \f$ 12 \cdot 1.1 \, \mathrm{E}[ X ] / n \approx 0.77 \f$, about 11.55 in
 * all for large \f$ n \f$ (a block of 256 bits for the spare at the least).
 *
 * A key inserted \f$ j \f$ times takes \f$ j \f$ places of its bin, a
 * duplicate past a full bin going to the spare. The adversarial keys whose
 * hashes share their bin all go past the first 25 to the spare, which then
 * answers yes to most of the queries of that bin, never no to a key of
 * \f$ S \f$.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`], and mixed by `hashWord()` otherwise.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{32} \f$ keys; a key
 * inserted is maybe there, a postcondition; the count of a bin, an
 * invariant. The operations are defined in `PrefixFilter.tpp`.
 *
 * \see T. Even, G. Even, A. Morrison, Prefix filter: practically and
 *      theoretically better than Bloom, Proceedings of the VLDB Endowment
 *      15(7), 1311-1323, 2022, doi:10.14778/3523210.3523211
 * \see I. O. Bercea, G. Even, A dynamic space-efficient filter with
 *      constant time operations, 17th Scandinavian Symposium and Workshops
 *      on Algorithm Theory (SWAT 2020), LIPIcs 162, 11:1-11:17, 2020,
 *      doi:10.4230/LIPIcs.SWAT.2020.11 (the pocket dictionaries)
 * \see D. Lemire, Fast random integer generation in an interval, ACM
 *      Transactions on Modeling and Computer Simulation 29(1), 3, 2019,
 *      doi:10.1145/3230636 (the bin and the mini-fingerprint by a product)
 * \see F. Putze, P. Sanders, J. Singler, Cache-, hash-, and space-efficient
 *      Bloom filters, Journal of Experimental Algorithmics 14, 4.4, 2009,
 *      doi:10.1145/1498698.1594230 (the spare)
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

#ifndef __ADS_PrefixFilter
 #define __ADS_PrefixFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/set/BlockedBloomFilter.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#if defined( __AVX2__ ) || defined( __BMI2__ )
 #include <immintrin.h>
#endif

/// a function inlined even where the compiler would not
#ifndef ADS_FILTER_INLINE
 #if defined( __GNUC__ ) || defined( __clang__ )
  #define ADS_FILTER_INLINE inline __attribute__(( always_inline ))
 #elif defined( _MSC_VER )
  #define ADS_FILTER_INLINE __forceinline
 #else
  #define ADS_FILTER_INLINE inline
 #endif
#endif

/// a function never inlined, the rare path out of a query
#ifndef ADS_FILTER_NOINLINE
 #if defined( __GNUC__ ) || defined( __clang__ )
  #define ADS_FILTER_NOINLINE __attribute__(( noinline ))
 #elif defined( _MSC_VER )
  #define ADS_FILTER_NOINLINE __declspec( noinline )
 #else
  #define ADS_FILTER_NOINLINE
 #endif
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PrefixFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an incremental filter of at most \f$ n \f$ keys of type `T`: bins of 25
/// fingerprints of 8 bits over 25 quotients, the overflow in a spare
template< class T , class H = DefaultHash< T > >
class PrefixFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter for \f$ n \f$ keys, its bins at the load 0.95 when
 /// they are in, at least one bin
 /// \pre \f$ n < 2^{32} \f$
 explicit PrefixFilter( std::size_t n , H h = H() );

 /// \f$ x \f$ into the filter by `insertHash()` of its hash, false (and
 /// nothing changed) if \f$ n \f$ keys are in
 /// \post `maybe( x )` if true
 bool insert( const T & x );

 /// whether \f$ x \f$ may be in the set, by `maybeHash()` of its hash:
 /// false only if \f$ x \f$ is not in the set
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the hash \f$ h \f$ of 64 bits, which must be random, into the filter,
 /// false (and nothing changed) if \f$ n \f$ keys are in
 ///
 /// In pseudocode (Even, Even and Morrison), \f$ x_{\max} \f$ the largest
 /// mini-fingerprint of a full bin:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertHash}( h ) \cr
 /// 2 & \quad \textbf{if}\ n \text{ keys are in }\ \textbf{then return
 ///     false} \cr
 /// 3 & \quad b \gets \lfloor h_1 m / 2^{32} \rfloor;\ x \gets \lfloor h_0
 ///     s / 2^{32} \rfloor \cr
 /// 4 & \quad \textbf{if}\ \text{the bin } b \text{ holds fewer than 25 }
 ///     \ \textbf{then}\ \text{put } ( \lfloor x / 2^8 \rfloor , x \bmod 2^8 )
 ///     \text{ in its run} \cr
 /// 5 & \quad \textbf{else if}\ x \ge x_{\max}\ \textbf{then}
 ///     \ \text{insert } s b + x \text{ into the spare} \cr
 /// 6 & \quad \textbf{else}\ \text{insert } s b + x_{\max} \text{ into the }
 ///     \text{spare, and put } x \text{ in the place of } x_{\max} \cr
 /// 7 & \quad \textbf{if}\ \text{the bin was full }\ \textbf{then}
 ///     \ \text{write the quotient of its maximum in the header} \cr
 /// 8 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 bool insertHash( std::uint64_t h );

 /// whether the hash \f$ h \f$ may be in the set: false only if it is not
 ///
 /// In pseudocode (Even, Even and Morrison), the bound of a bin being its
 /// largest mini-fingerprint once it overflowed, and above every
 /// mini-fingerprint before:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybeHash}( h ) \cr
 /// 2 & \quad b \gets \lfloor h_1 m / 2^{32} \rfloor;\ x \gets \lfloor h_0
 ///     s / 2^{32} \rfloor \cr
 /// 3 & \quad \textbf{if}\ x \bmod 2^8 \text{ is in the run of }
 ///     \lfloor x / 2^8 \rfloor \text{ of the bin } b\ \textbf{then return
 ///     true} \cr
 /// 4 & \quad \textbf{if}\ x > \text{the bound of the bin } b
 ///     \ \textbf{then return}\ \text{the answer of the spare to } s b + x \cr
 /// 5 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// removes all the keys
 void clear( void );

 /// the keys in, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 /// whether no key is in
 [[nodiscard]] bool empty( void ) const;

 /// the keys that fit, the \f$ n \f$ of the constructor
 [[nodiscard]] std::size_t capacity( void ) const;

 /// `size() / ( 25 bins() )`, at most about 0.95
 [[nodiscard]] double load( void ) const;

 /// the bits of the bins and of the spare
 [[nodiscard]] std::size_t bits( void ) const;

 /// the bins, of 256 bits each
 [[nodiscard]] std::size_t bins( void ) const;

 /// the fingerprints sent to the spare
 [[nodiscard]] std::size_t spareSize( void ) const;

 /// the expected fingerprints sent to the spare when `capacity()` keys are
 /// in, \f$ \mathrm{E}[ X ] = n - m \, \mathrm{E}[ \min( B , 25 ) ] \f$
 [[nodiscard]] double expectedSpareSize( void ) const;

 /// the expected false positives at the current load, the sum of the
 /// analysis of `PrefixFilter.h` over the loads \f$ j \f$ of a bin
 [[nodiscard]] double falsePositiveRate( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a pocket dictionary: the header in the low 50 bits, the quotient of
 /// the maximum (63 before an overflow) in the next 6, the 25 remainders
 /// in the other bytes
 struct alignas( 32 ) Bin {
  /// the bytes, the 6 bits of the quotient at 63 in an empty bin
  unsigned char b[ 32 ] = { 0 , 0 , 0 , 0 , 0 , 0 , 0xFC };
  };

 static constexpr unsigned kK = 25;        ///< remainders in a bin
 static constexpr unsigned kQ = 25;        ///< quotients
 static constexpr std::uint32_t kS = kQ << 8;   ///< mini-fingerprints
 static constexpr unsigned kBody = 7;      ///< the first byte of the body
 static constexpr std::uint64_t kHead = ( std::uint64_t( 1 ) << 50 ) - 1;
 static constexpr std::uint64_t kTop = std::uint64_t( 63 ) << 50;
 static constexpr double kAlpha = 0.95;    ///< the load of the bins
 static constexpr double kSpareBits = 12;  ///< bits per spare item

 /// the header and the quotient of the maximum of a bin
 [[nodiscard]] static std::uint64_t header( const Bin & B );

 /// the header and the quotient of the maximum \f$ w \f$ written into
 /// \f$ B \f$
 static void setHeader( Bin & B , std::uint64_t w );

 /// the place of the zero of rank \f$ j \f$ of \f$ w \f$,
 /// \f$ j < 64 - \mathrm{popcount}( w ) \f$, by clearing the lowest zero
 /// \f$ j \f$ times
 [[nodiscard]] static unsigned select0( std::uint64_t w , unsigned j );

 /// the slots \f$ [ s , e ) \f$ of the body holding the run of \f$ q \f$
 static void run( std::uint64_t w , unsigned q , unsigned & s ,
                  unsigned & e );

 /// the bits \f$ i \f$ of the body bytes \f$ i \f$ of \f$ B \f$ equal to
 /// \f$ r \f$
 [[nodiscard]] static std::uint32_t equal( const Bin & B , unsigned r );

 /// the bits \f$ i \f$ of the body bytes \f$ i \f$ of \f$ B \f$ below
 /// \f$ r \f$
 [[nodiscard]] static std::uint32_t below( const Bin & B , unsigned r );

 /// the bits of the slots of the run of \f$ q \f$ of the header \f$ w \f$
 [[nodiscard]] static std::uint32_t runMask( std::uint64_t w , unsigned q );

 /// true if \f$ r \f$ is in the run of \f$ q \f$ of \f$ B \f$, whose
 /// header is \f$ w \f$
 [[nodiscard]] static bool find( const Bin & B , std::uint64_t w ,
                                 unsigned q , unsigned r );

 /// the largest mini-fingerprint of the full bin \f$ B \f$, whose header
 /// is \f$ w \f$
 [[nodiscard]] static std::uint32_t maximum( const Bin & B ,
                                             std::uint64_t w );

 /// \f$ ( q , r ) \f$ put into \f$ B \f$, which holds \f$ c < 25 \f$
 /// remainders, \f$ w \f$ its header
 static void put( Bin & B , std::uint64_t w , unsigned q , unsigned r ,
                  unsigned c );

 /// the bins for \f$ n \f$ keys, at least one
 [[nodiscard]] static std::size_t binsFor( std::size_t n );

 /// the expected \f$ \mathrm{E}[ X ] \f$ of \f$ n \f$ keys in \f$ m \f$
 /// bins
 [[nodiscard]] static double spareMean( std::size_t n , std::size_t m );

 /// the hash of \f$ x \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the bin of the hash \f$ h \f$
 [[nodiscard]] std::size_t bin( std::uint64_t h ) const;

 /// the mini-fingerprint of the hash \f$ h \f$, in \f$ [ 0 , 6400 ) \f$
 [[nodiscard]] static std::uint32_t mini( std::uint64_t h );

 /// the answer of the spare to the mini-fingerprint \f$ x \f$ of the bin
 /// \f$ i \f$, the rare path of a query, out of line
 [[nodiscard]] bool spare( std::size_t i , std::uint32_t x ) const;

 /// the hash in the spare of the mini-fingerprint \f$ x \f$ of the bin
 /// \f$ i \f$
 [[nodiscard]] static std::uint64_t spareHash( std::size_t i ,
                                               std::uint32_t x );

 std::vector< Bin > f_B;                   ///< the bins
 std::size_t f_cap;                        ///< n
 std::size_t f_n = 0;                      ///< the keys in
 std::size_t f_spareN = 0;                 ///< the fingerprints spared
 BlockedBloomFilter< std::uint64_t > f_spare;   ///< the spare
 H f_h;                                    ///< the hash of the keys

 };  // end( class PrefixFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/PrefixFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PrefixFilter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File PrefixFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
