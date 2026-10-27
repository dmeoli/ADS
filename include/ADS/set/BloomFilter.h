/*--------------------------------------------------------------------------*/
/*--------------------------- File BloomFilter.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The filter of Bloom, beyond the book: a set that may answer yes for an
 * item it does not hold, but never no for one it holds, in \f$ m \f$ bits
 * whatever the items. An item sets the \f$ k \f$ bits
 * \f$ h_1 + i h_2 \bmod m \f$, \f$ i = 0 , \ldots , k - 1 \f$, of two
 * hashes, as Kirsch and Mitzenmacher showed to be as good as \f$ k \f$
 * independent hashes, and is maybe there if all its bits are set; after
 * \f$ n \f$ insertions the probability of a false positive is about
 * \f$ ( 1 - e^{-k n / m} )^k \f$, the least for
 * \f$ k = ( m / n ) \ln 2 \f$, i.e., about \f$ 0.6185^{m/n} \f$
 * (\f$ 1 \% \f$ with about 9.6 bits per item). The two hashes come from the
 * hash of the item mixed by `splitmix64()` [see `RandomHashing.h`].
 *
 * The contracts [see `Contract.h`]: \f$ m \ge 1 \f$ and \f$ k \ge 1 \f$; an
 * item inserted is always maybe there. The operations are defined in
 * `BloomFilter.tpp`.
 *
 * \see B. H. Bloom, Space/time trade-offs in hash coding with allowable
 *      errors, Communications of the ACM 13(7), 422-426, 1970,
 *      doi:10.1145/362686.362692
 * \see A. Kirsch, M. Mitzenmacher, Less hashing, same performance: building
 *      a better Bloom filter, Proceedings of ESA 2006, LNCS 4168, 456-467,
 *      2006, doi:10.1007/11841036_42
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

#ifndef __ADS_BloomFilter
 #define __ADS_BloomFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BloomFilter ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a filter of Bloom of \f$ m \f$ bits and \f$ k \f$ hashes
template< class T , class H = std::hash< T > >
class BloomFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter of \f$ m \f$ bits and \f$ k \f$ hashes
 /// \pre \f$ m \ge 1 \f$ and \f$ k \ge 1 \f$
 BloomFilter( std::size_t m , int k , H h = H() );

 /// the filter of the best \f$ k \f$ for \f$ n \f$ items and the false
 /// positives \f$ p \f$:
 /// \f$ m = \lceil n \ln( 1 / p ) / ( \ln 2 )^2 \rceil \f$ bits and
 /// \f$ k = ( m / n ) \ln 2 \f$ rounded, at least 1
 /// \pre \f$ n \ge 1 \f$ and \f$ 0 < p < 1 \f$
 static BloomFilter forItems( std::size_t n , double p );

 /// \f$ x \f$ into the filter, by setting its \f$ k \f$ bits
 ///
 /// In pseudocode (Bloom, with the two hashes of Kirsch and Mitzenmacher),
 /// \f$ B \f$ the bits and \f$ h_2 \f$ odd:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad h_1 \gets \mathrm{splitmix64}( h( x ) );\ h_2 \gets
 ///     \mathrm{splitmix64}( h_1 ) \vee 1 \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
 ///     \ B[ ( h_1 + i h_2 ) \bmod m ] \gets 1
 /// \end{array}}
 /// \f]
 /// \post `maybe( x )`
 void insert( const T & x );

 /// whether \f$ x \f$ may be in the set: false only if \f$ x \f$ was
 /// never inserted
 ///
 /// In pseudocode (Bloom), the bits of `insert()` read instead of set:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybe}( x ) \cr
 /// 2 & \quad h_1 \gets \mathrm{splitmix64}( h( x ) );\ h_2 \gets
 ///     \mathrm{splitmix64}( h_1 ) \vee 1 \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ k - 1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ B[ ( h_1 + i h_2 ) \bmod m ] = 0\ \textbf{then
 ///     return false} \cr
 /// 5 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the expected false positives after the \f$ n \f$ insertions made,
 /// \f$ ( 1 - e^{-k n / m} )^k \f$
 [[nodiscard]] double falsePositiveRate( void ) const;

 /// the bits \f$ m \f$
 [[nodiscard]] std::size_t bits( void ) const;

 /// the hashes \f$ k \f$
 [[nodiscard]] int hashes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::vector< std::uint64_t > f_B;   ///< the bits
 std::size_t f_m;                    ///< the bits
 int f_k;                            ///< the hashes
 std::size_t f_n = 0;                ///< the insertions
 H f_h;                              ///< the hash of the items

 };  // end( class BloomFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/BloomFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BloomFilter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BloomFilter.h -------------------------*/
/*--------------------------------------------------------------------------*/
