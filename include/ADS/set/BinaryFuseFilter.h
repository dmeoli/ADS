/*--------------------------------------------------------------------------*/
/*------------------------- File BinaryFuseFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary fuse filter of Graf and Lemire, beyond the book: a static
 * filter of a set \f$ S \f$ of \f$ n \f$ keys, built once, which answers
 * maybe for every key of \f$ S \f$ and for a key out of \f$ S \f$ with
 * probability about \f$ 2^{-b} \f$, in about \f$ 1.13 b \f$ bits per key for
 * fingerprints of \f$ b \f$ bits (8 or 16), against the \f$ 1.44 b \f$ of a
 * filter of Bloom with the same false positives [see `BloomFilter.h` and
 * `BlockedBloomFilter.h`] and the \f$ 1.23 b \f$ of the xor filter, of which
 * it is the successor.
 *
 * A key with the hash \f$ h \f$ has 3 places \f$ h_0 , h_1 , h_2 \f$ of an
 * array \f$ F \f$ of fingerprints, and is maybe in \f$ S \f$ if
 * \f$ F[ h_0 ] \oplus F[ h_1 ] \oplus F[ h_2 ] = f( h ) \f$, its
 * fingerprint. The array is cut into segments of \f$ L \f$ places (a power
 * of 2): \f$ h_0 \f$ is a place of the first \f$ C L \f$ ones, by a product
 * of \f$ h \f$, and \f$ h_1 \f$ and \f$ h_2 \f$ are in the two segments that
 * follow, at places given by other bits of \f$ h \f$, so that the 3 places
 * of a key are within 3 consecutive segments (the spatial coupling that
 * lets the array have only about \f$ 1.125 n \f$ places for large \f$ n \f$,
 * against \f$ 1.23 n \f$ for 3 places anywhere).
 *
 * The build solves the system of the \f$ n \f$ equations by peeling: a place
 * reached by exactly one key is solved last for that key, which is then
 * removed from its other two places, and so on; each place keeps the number
 * of its keys, the XOR of their hashes and the XOR of the indices (0, 1 or
 * 2) by which they reach it, so that a place left with one key tells the
 * hash of that key and which of its places it is, and the peeling touches
 * nothing but these arrays. If every key is peeled, the fingerprints are
 * assigned in the reverse order of the peeling,
 * \f$ F[ h_i ] = f( h ) \oplus F[ h_{i+1} ] \oplus F[ h_{i+2} ] \f$, the
 * place \f$ h_i \f$ being still free when its key is reached; otherwise
 * (with probability well below \f$ 1 \% \f$ for the sizes of the paper) the
 * build draws a new seed. Before the counts, the hashes are sorted by their
 * segment by a counting sort, so that the counts and the peeling walk the
 * arrays in order and stay in the cache. The keys may repeat: a key that
 * occurs twice has twice the same hash, which cannot be peeled, so a build
 * that fails removes the equal hashes before drawing a new seed.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise;
 * for each seed it is mixed once more by a folded product with the seed.
 * The array has at most \f$ 2^{32} \f$ places.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{31} \f$ keys; after
 * the build every key is maybe there, an audit. A build that draws 1000
 * seeds with no success throws `std::runtime_error`. The operations are
 * defined in `BinaryFuseFilter.tpp`.
 *
 * \see T. M. Graf, D. Lemire, Binary fuse filters: fast and smaller than
 *      xor filters, ACM Journal of Experimental Algorithmics 27, 1.5, 2022,
 *      doi:10.1145/3510449
 * \see T. M. Graf, D. Lemire, Xor filters: faster and smaller than Bloom
 *      and cuckoo filters, ACM Journal of Experimental Algorithmics 25,
 *      1.5, 2020, doi:10.1145/3376122
 * \see M. Dietzfelbinger, S. Walzer, Dense peelable random uniform
 *      hypergraphs, Proceedings of ESA 2019, LIPIcs 144, 38:1-38:16, 2019,
 *      doi:10.4230/LIPIcs.ESA.2019.38 (the spatial coupling)
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

/// a static filter of the keys of type `T` with fingerprints of type `F`,
/// 3 places per key in 3 consecutive segments
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
class BinaryFuseFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the filter of the keys, which may repeat
 ///
 /// In pseudocode (Graf and Lemire), \f$ c[ p ] \f$ the keys of the place
 /// \f$ p \f$, \f$ x[ p ] \f$ the XOR of their hashes and \f$ s[ p ] \f$
 /// the XOR of the indices by which they reach it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( K ) \cr
 /// 2 & \quad \textbf{repeat} \cr
 /// 3 & \qquad \text{draw a seed};\ H \gets \text{the hashes of } K
 ///     \text{, sorted by the segment of } h_0 \cr
 /// 4 & \qquad \textbf{foreach}\ h \in H ,\ i \in \{ 0 , 1 , 2 \}
 ///     \ \textbf{do}\ c[ h_i ] \gets c[ h_i ] + 1;\ x[ h_i ] \gets x[ h_i ]
 ///     \oplus h;\ s[ h_i ] \gets s[ h_i ] \oplus i \cr
 /// 5 & \qquad Q \gets \text{the places } p \text{ with } c[ p ] = 1;\ P
 ///     \gets \text{an empty stack} \cr
 /// 6 & \qquad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 7 & \qquad\quad p \gets \text{a place of } Q;\ \textbf{if}\ c[ p ]
 ///     \ne 1\ \textbf{then continue} \cr
 /// 8 & \qquad\quad h \gets x[ p ];\ i \gets s[ p ];
 ///     \ \mathrm{push}( P , ( h , i ) ) \cr
 /// 9 & \qquad\quad \textbf{foreach}\ j \ne i\ \textbf{do}\ \text{remove }
 ///     h \text{ from } h_j \text{, adding } h_j \text{ to } Q \text{ if it }
 ///     \text{has one key left} \cr
 /// 10 & \qquad \textbf{if}\ \text{not all the keys are in } P
 ///     \ \textbf{then}\ \text{remove the equal hashes of } H \cr
 /// 11 & \quad \textbf{until}\ \text{all the keys are in } P \cr
 /// 12 & \quad \textbf{while}\ P \ne \emptyset\ \textbf{do} \cr
 /// 13 & \qquad ( h , i ) \gets \mathrm{pop}( P );\ F[ h_i ] \gets f( h )
 ///     \oplus F[ h_0 ] \oplus F[ h_1 ] \oplus F[ h_2 ] \text{ with }
 ///     F[ h_i ] = 0
 /// \end{array}}
 /// \f]
 /// \pre fewer than \f$ 2^{31} \f$ keys
 /// \post every key is maybe there (an audit)
 explicit BinaryFuseFilter( const std::vector< T > & keys ,
                            std::uint64_t seed = 1 , H h = H() );

 /// whether \f$ x \f$ may be in the set: true for every key of the set,
 /// and for any other with probability about \f$ 2^{-b} \f$, \f$ b \f$
 /// the bits of `F`
 ///
 /// In pseudocode (Graf and Lemire):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybe}( x ) \cr
 /// 2 & \quad h \gets \text{the hash of } x \text{ under the seed} \cr
 /// 3 & \quad \textbf{return}\ F[ h_0 ] \oplus F[ h_1 ] \oplus F[ h_2 ] = f( h
 ///     )
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the keys of the build, the repeated ones once each
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the filter has no key
 [[nodiscard]] bool empty( void ) const;

 /// the bits of the fingerprints
 [[nodiscard]] std::size_t bits( void ) const;

 /// the seeds drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxTrials = 1000;   ///< seeds at most

 /// the hash of \f$ x \f$ before the seed, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t base( const T & x ) const;

 /// the hash of the base hash \f$ b \f$ under the seed
 [[nodiscard]] std::uint64_t mixed( std::uint64_t b ) const;

 /// the fingerprint of the hash \f$ h \f$
 [[nodiscard]] static F fingerprint( std::uint64_t h );

 /// the place \f$ h_i \f$, \f$ i \f$ 0, 1 or 2, of the hash \f$ h \f$
 [[nodiscard]] std::uint32_t place( std::uint64_t h , int i ) const;

 /// the sizes of the array for \f$ n \f$ keys
 void layout( std::size_t n );

 /// the fingerprints of the base hashes \f$ B \f$, false if the peeling
 /// fails
 bool build( const std::vector< std::uint64_t > & B );

 std::vector< F > f_F;             ///< the fingerprints
 std::uint64_t f_seed;             ///< the seed of the last trial
 std::uint32_t f_L = 0;            ///< the places of a segment
 std::uint32_t f_CL = 0;           ///< the places of h_0, C L
 std::size_t f_n = 0;              ///< the keys, the repeated ones once
 std::size_t f_trials = 0;         ///< the seeds drawn
 H f_h;                            ///< the hash of the keys

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
