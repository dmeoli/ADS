/*--------------------------------------------------------------------------*/
/*------------------------- File BinaryFuseFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary fuse filter of Graf and Lemire, beyond the book: a static
 * filter of a set S of n keys, built once, which answers maybe for every
 * key of S and for a key out of S with probability about \f$ 2^{-b} \f$,
 * in about \f$ 1.13 b \f$ bits per key for fingerprints of b bits (8 or
 * 16), against the \f$ 1.44 b \f$ of a filter of Bloom with the same
 * false positives [see BloomFilter.h and BlockedBloomFilter.h] and the
 * \f$ 1.23 b \f$ of the xor filter, of which it is the successor.
 *
 * A key k with the hash h has 3 places \f$ h_0 , h_1 , h_2 \f$ of an
 * array F of fingerprints, and is maybe in S if \f$ F[ h_0 ] \oplus
 * F[ h_1 ] \oplus F[ h_2 ] = f( h ) \f$, its fingerprint. The array is cut
 * into segments of L places (a power of 2): \f$ h_0 \f$ is a place of the
 * first C L ones, by a product of h, and \f$ h_1 \f$ and \f$ h_2 \f$ are in
 * the two segments that follow, at places given by other bits of h, so
 * that the 3 places of a key are within 3 consecutive segments (the
 * spatial coupling that lets the array have only about 1.125 n places for
 * large n, against 1.23 n for 3 places anywhere).
 *
 * The build solves the system of the n equations by peeling: a place
 * reached by exactly one key is solved last for that key, which is then
 * removed from its other two places, and so on; each place keeps the
 * number of its keys, the XOR of their hashes and the XOR of the indices
 * (0, 1 or 2) by which they reach it, so that a place left with one key
 * tells the hash of that key and which of its places it is, and the peeling
 * touches nothing but these arrays. If every key is peeled, the fingerprints
 * are assigned in the reverse order of the peeling,
 * \f$ F[ h_i ] = f( h ) \oplus F[ h_{i+1} ] \oplus F[ h_{i+2} ] \f$, the
 * place \f$ h_i \f$ being still free when its key is reached; otherwise
 * (with probability well below 1 % for the sizes of the paper) the build
 * draws a new seed. Before the counts, the hashes are sorted by their
 * segment by a counting sort, so that the counts and the peeling walk the
 * arrays in order and stay in the cache. The keys may repeat: a key that
 * occurs twice has twice the same hash, which cannot be peeled, so a
 * build that fails removes the equal hashes before drawing a new seed.
 *
 * The hash of a key is h( k ), taken as it is if H declares is_avalanching
 * [see FastHash.h] and mixed by hashWord() otherwise; for each seed it is
 * mixed once more by a folded product with the seed. The array has at most
 * \f$ 2^{32} \f$ places.
 *
 * The contracts [see Contract.h]: fewer than \f$ 2^{31} \f$ keys; after
 * the build every key is maybe there, an audit. A build that draws 1000
 * seeds with no success throws std::runtime_error. The operations are
 * defined in BinaryFuseFilter.tpp.
 *
 * \see T. M. Graf, D. Lemire, Binary fuse filters: fast and smaller than
 *      xor filters, ACM Journal of Experimental Algorithmics 27, 1.5, 2022,
 *      doi:10.1145/3510449
 * \see T. M. Graf, D. Lemire, Xor filters: faster and smaller than Bloom
 *      and cuckoo filters, ACM Journal of Experimental Algorithmics 25,
 *      1.5, 2020, doi:10.1145/3376122
 * \see M. Dietzfelbinger, S. Walzer, Dense peelable random uniform
 *      hypergraphs, Proceedings of ESA 2019, LIPIcs 144, 38, 2019 (the
 *      spatial coupling)
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

#ifndef __ADS_BinaryFuseFilter
 #define __ADS_BinaryFuseFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
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

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BinaryFuseFilter -------------------------*/
/*--------------------------------------------------------------------------*/

/// a static filter of the keys of type T with fingerprints of type F, 3
/// places per key in 3 consecutive segments
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
class BinaryFuseFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the filter of the keys, which may repeat
 /// \pre fewer than 2^31 keys
 /// \post every key is maybe there (an audit)
 explicit BinaryFuseFilter( const std::vector< T > & keys ,
                            std::uint64_t seed = 1 , H h = H() );

 /// true for every key of the set, and for any other with probability
 /// about 2^-b, b the bits of F
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the keys of the build, the repeated ones once each
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bits of the fingerprints
 [[nodiscard]] std::size_t bits( void ) const;

 /// the seeds drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxTrials = 1000;

 /// the hash of x before the seed, mixed unless H avalanches
 [[nodiscard]] std::uint64_t base( const T & x ) const;

 /// the hash of the base hash b under the seed
 [[nodiscard]] std::uint64_t mixed( std::uint64_t b ) const;

 /// the fingerprint of the hash h
 [[nodiscard]] static F fingerprint( std::uint64_t h );

 /// the place i (0, 1 or 2) of the hash h
 [[nodiscard]] std::uint32_t place( std::uint64_t h , int i ) const;

 /// the sizes of the array for n keys
 void layout( std::size_t n );

 /// the fingerprints of the base hashes B, false if the peeling fails
 bool build( const std::vector< std::uint64_t > & B );

 std::vector< F > f_F;             ///< the fingerprints
 std::uint64_t f_seed;
 std::uint32_t f_L = 0;            ///< the places of a segment
 std::uint32_t f_CL = 0;           ///< the places of h_0, C L
 std::size_t f_n = 0;
 std::size_t f_trials = 0;
 H f_h;

 };  // end( class BinaryFuseFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/BinaryFuseFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BinaryFuseFilter.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BinaryFuseFilter.h ----------------------*/
/*--------------------------------------------------------------------------*/
