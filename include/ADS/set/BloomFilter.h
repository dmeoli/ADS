/*--------------------------------------------------------------------------*/
/*--------------------------- File BloomFilter.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The filter of Bloom, beyond the book: a set that may answer yes for an
 * item it does not hold, but never no for one it holds, in m bits whatever
 * the items. An item sets the k bits \f$ h_1 + i h_2 \bmod m \f$, i = 0 ,
 * ... , k - 1, of two hashes, as Kirsch and Mitzenmacher showed to be as
 * good as k independent hashes, and is maybe there if all its bits are
 * set; after n insertions the probability of a false positive is about
 * \f$ ( 1 - e^{-k n / m} )^k \f$, the least for \f$ k = ( m / n ) \ln 2
 * \f$, i.e., about \f$ 0.6185^{m/n} \f$ (1 % with about 9.6 bits per
 * item). The two hashes come from the hash of the item mixed by splitmix64()
 * [see RandomHashing.h].
 *
 * The contracts [see Contract.h]: m >= 1 and k >= 1; an item inserted is
 * always maybe there. The operations are defined in BloomFilter.tpp.
 *
 * \see B. H. Bloom, Space/time trade-offs in hash coding with allowable
 *      errors, Communications of the ACM 13(7), 422-426, 1970,
 *      doi:10.1145/362686.362692
 * \see A. Kirsch, M. Mitzenmacher, Less hashing, same performance: building
 *      a better Bloom filter, Proceedings of ESA 2006, LNCS 4168, 456-467,
 *      2006
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

/// a filter of Bloom of m bits and k hashes
template< class T , class H = std::hash< T > >
class BloomFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter of m bits and k hashes
 /// \pre m >= 1 and k >= 1
 BloomFilter( std::size_t m , int k , H h = H() );

 /// the filter of the best k for n items and the false positives p
 /// \pre n >= 1 and 0 < p < 1
 static BloomFilter forItems( std::size_t n , double p );

 /// \post maybe( x )
 void insert( const T & x );

 /// false only if x was never inserted
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the expected false positives after the insertions made
 [[nodiscard]] double falsePositiveRate( void ) const;

 [[nodiscard]] std::size_t bits( void ) const;

 [[nodiscard]] int hashes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::vector< std::uint64_t > f_B;   ///< the bits
 std::size_t f_m;
 int f_k;
 std::size_t f_n = 0;                ///< the insertions
 H f_h;

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
