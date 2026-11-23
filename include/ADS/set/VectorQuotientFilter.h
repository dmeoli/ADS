/*--------------------------------------------------------------------------*/
/*----------------------- File VectorQuotientFilter.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector quotient filter of Pandey, Conway, Durie, Bender,
 * Farach-Colton and Johnson, beyond the book: a dynamic filter of a set
 * \f$ S \f$ of keys, which takes insertions and removals one at a time,
 * whose blocks of one line of the cache are small quotient filters with
 * the quotients in a word or two of bits, and the place of a key chosen
 * between two blocks, so that no key moves once placed and an operation
 * reads two lines, in \f$ O( 1 ) \f$ time at any load.
 *
 * A block has 64 bytes: \f$ s \f$ slots of fingerprints of \f$ f \f$ bits
 * (the type `F`) for \f$ c \f$ buckets, \f$ s = 48 \f$ and \f$ c = 80 \f$
 * for \f$ f = 8 \f$, \f$ s = 28 \f$ and \f$ c = 36 \f$ for \f$ f = 16 \f$,
 * and \f$ s + c \f$ bits of metadata (128 and 64) in front of them. The
 * metadata writes, from the low bit, for each bucket \f$ j = 0 , \ldots ,
 * c - 1 \f$, a 0 for each fingerprint of the bucket and then a 1, and the
 * free slots as 1s at the top: the fingerprints of the bucket \f$ j \f$
 * are those of the slots \f$ [ \sigma_{j-1} - ( j - 1 ) , \sigma_j - j ) \f$
 * (\f$ [ 0 , \sigma_0 ) \f$ for \f$ j = 0 \f$), \f$ \sigma_j \f$ the place
 * of the 1 of rank \f$ j \f$, and the slots in use are \f$ s + c \f$ less
 * the 1s. The slots are kept in the order of their buckets: an insertion
 * into the bucket \f$ j \f$ puts a 0 at \f$ \sigma_j \f$, shifting up the
 * bits above (the top one, a free slot, is lost) and the slots from
 * \f$ \sigma_j - j \f$ on, and a removal does the opposite, a 1 entering
 * at the top.
 *
 * A key with the hash \f$ h \f$ has the fingerprint \f$ t \f$, the high
 * \f$ f \f$ bits of \f$ h a \bmod 2^{64} \f$, \f$ a \f$ an odd constant,
 * and the two buckets of the \f$ R = c B \f$ of the \f$ B \f$ blocks
 * \f[ b_1 = \lfloor h_1( h ) R / 2^{32} \rfloor \;, \qquad
 *     b_2 = ( g( t ) - b_1 ) \bmod R \;, \f]
 * \f$ h_1( h ) \f$ the high half of \f$ h \f$ and \f$ g( t ) \f$ a hash of
 * \f$ t \f$ onto \f$ [ 0 , R ) \f$, so that \f$ b \mapsto ( g( t ) - b )
 * \bmod R \f$ is an involution and each bucket of a fingerprint gives the
 * other. The bucket \f$ j \f$ is the bucket \f$ j \bmod c \f$ of the block
 * \f$ \lfloor j / c \rfloor \f$. An insertion puts \f$ t \f$ in \f$ b_1 \f$
 * if the block of \f$ b_1 \f$ has at most \f$ 3 s / 4 \f$ slots in use,
 * and otherwise in the bucket of the block with fewer (the power of two
 * choices of Azar, Broder, Karlin and Upfal, shortcut as in the paper);
 * it fails, changing nothing, when both blocks are full. A query of
 * \f$ t \f$ compares it with all the slots of each of its two blocks at
 * once, the bytes or the words of 16 bits of the line with the vectors of
 * Highway with `ADS_WITH_HIGHWAY` [see `SimdInline.h`], and only when a
 * slot equals \f$ t \f$ (for an absent key, with probability at most
 * \f$ s \alpha 2^{-f} \f$ per block) finds the slots of the bucket, by one
 * `select` of the metadata and the 1 just below it, the second block
 * prefetched while the first is read; without the vectors, it finds the
 * slots of the bucket and compares them one by one. The places of the 1s
 * are found with no branch on the word of the metadata that holds them,
 * which the buckets would mispredict half of the times.
 *
 * Analysis. With \f$ n \f$ keys in, a bucket holds \f$ n / R \f$
 * fingerprints on the average, and a key out of \f$ S \f$, whose buckets
 * and fingerprint are independent of the keys in, is compared with
 * \f$ 2 n / R \f$ of them on the average, each equal to \f$ t \f$ with
 * probability \f$ 2^{-f} \f$:
 * \f[ \varepsilon \le \frac{2 n}{R \, 2^f} = 2 \alpha \frac{s}{c}
 *     2^{-f} \;, \f]
 * \f$ \alpha \f$ the fraction of the slots in use, i.e., at most
 * \f$ 1.2 \alpha / 256 \f$ for \f$ f = 8 \f$ and
 * \f$ 1.56 \alpha / 65536 \f$ for \f$ f = 16 \f$, in
 * \f$ 512 / ( s \alpha ) \f$ bits per key (11.9 and 20.3 at
 * \f$ \alpha = 0.9 \f$). By the power of two choices the most loaded block
 * stays near the mean, so that the blocks fill to a load of about 0.95 for
 * \f$ f = 8 \f$ and 0.9 for \f$ f = 16 \f$, whose blocks are smaller,
 * before an insertion fails (in the tests of ADS); the constructor for
 * \f$ n \f$ keys takes the blocks that hold them at the load 0.9 for
 * \f$ f = 8 \f$ and 0.85 for \f$ f = 16 \f$. A key inserted \f$ j \f$
 * times takes \f$ j \f$ slots, and is removed once by each removal.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`], and mixed by `hashWord()` otherwise.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{31} \f$ keys; a key
 * inserted is maybe there, a postcondition. A key removed must have been
 * inserted, which no contract can check: the removal of another key may
 * clear the fingerprint of a key of \f$ S \f$, a false negative. The
 * operations are defined in `VectorQuotientFilter.tpp`.
 *
 * \see P. Pandey, A. Conway, J. Durie, M. A. Bender, M. Farach-Colton,
 *      R. Johnson, Vector quotient filters: overcoming the time/space
 *      trade-off in filter design, Proceedings of the 2021 International
 *      Conference on Management of Data (SIGMOD 2021), 1386-1399, 2021,
 *      doi:10.1145/3448016.3452841
 * \see Y. Azar, A. Z. Broder, A. R. Karlin, E. Upfal, Balanced
 *      allocations, SIAM Journal on Computing 29(1), 180-200, 1999,
 *      doi:10.1137/S0097539795288490
 * \see P. Pandey, M. A. Bender, R. Johnson, R. Patro, A general-purpose
 *      counting filter: making every bit count, Proceedings of SIGMOD 2017,
 *      775-787, 2017, doi:10.1145/3035918.3035963 (the metadata of the
 *      quotients in unary)
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

#ifndef __ADS_VectorQuotientFilter
 #define __ADS_VectorQuotientFilter
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/SimdInline.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/external/MappedFile.h"
#include "ADS/memory/Prefetch.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

/// a function inlined even where the compiler would not
#ifndef ADS_FILTER_INLINE
 #define ADS_FILTER_INLINE ADS_ALWAYS_INLINE
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS VectorQuotientFilter -----------------------*/
/*--------------------------------------------------------------------------*/

/// a dynamic filter of the keys of type `T` with fingerprints of type `F`
/// (8 or 16 bits) in blocks of 64 bytes, each a small quotient filter, two
/// blocks per key
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class VectorQuotientFilter
{

 static_assert( std::is_same_v< F , std::uint8_t > ||
		  std::is_same_v< F , std::uint16_t > ,
		"the fingerprints have 8 or 16 bits" );

/*--------------------------------------------------------------------------*/
 public:
 /// a filter of one block, empty
 VectorQuotientFilter( void ) : VectorQuotientFilter( 1 ) { }

 /// the empty filter for \f$ n \f$ keys, at the load 0.9 (0.85 for
 /// \f$ f = 16 \f$) when they are in, at least one block
 /// \pre \f$ n < 2^{31} \f$
 explicit VectorQuotientFilter( std::size_t n , H h = H() );

 /// \f$ x \f$ into the filter by `insertHash()` of its hash, false (and
 /// nothing changed) if both its blocks are full
 /// \post `maybe( x )` if true
 bool insert( const T & x );

 /// one insertion of \f$ x \f$ removed by `removeHash()` of its hash, false
 /// if \f$ x \f$ is not there
 /// \pre \f$ x \f$ was inserted (otherwise another key may be lost)
 bool remove( const T & x );

 /// whether \f$ x \f$ may be in the set, by `maybeHash()` of its hash:
 /// false only if \f$ x \f$ is not in the set
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the hash \f$ h \f$ of 64 bits, which must be random, into the filter,
 /// false (and nothing changed) if both its blocks are full
 ///
 /// In pseudocode (Pandey et al.), \f$ u( b ) \f$ the slots in use of the
 /// block of the bucket \f$ b \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertHash}( h ) \cr
 /// 2 & \quad t \gets t( h );\ b \gets b_1( h ) \cr
 /// 3 & \quad \textbf{if}\ u( b ) > 3 s / 4\ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ u( b_2 ) < u( b )\ \textbf{then}\ b \gets b_2
 ///     \quad \triangleright\ b_2 = ( g( t ) - b_1 ) \bmod R \cr
 /// 5 & \qquad \textbf{if}\ u( b ) = s\ \textbf{then return false} \cr
 /// 6 & \quad j \gets b \bmod c;\ \sigma \gets \text{the place of the 1 of
 ///     rank } j \text{ of the metadata} \cr
 /// 7 & \quad \text{put a 0 at } \sigma \text{, the bits from } \sigma
 ///     \text{ shifting up} \cr
 /// 8 & \quad \text{put } t \text{ in the slot } \sigma - j \text{, the
 ///     slots from it shifting up};\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool insertHash( std::uint64_t h );

 /// one insertion of the hash \f$ h \f$ removed, false if it is not there
 ///
 /// In pseudocode (Pandey et al.):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{removeHash}( h ) \cr
 /// 2 & \quad \textbf{foreach}\ b \in \{ b_1( h ) , b_2( h ) \}
 ///     \ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ \text{a slot } i \text{ of the bucket } b
 ///     \text{ holds } t\ \textbf{then} \cr
 /// 4 & \qquad\quad \text{remove the 0 at } i + ( b \bmod c ) \text{ and the
 ///     slot } i \text{, the bits and the slots above shifting down};
 ///     \ \textbf{return true} \cr
 /// 5 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 bool removeHash( std::uint64_t h );

 /// whether the hash \f$ h \f$ may be in the set: false only if it is not
 ///
 /// In pseudocode (Pandey et al.), the slots of a block compared at once:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybeHash}( h ) \cr
 /// 2 & \quad \textbf{foreach}\ b \in \{ b_1( h ) , b_2( h ) \}
 ///     \ \textbf{do} \cr
 /// 3 & \qquad E \gets \{ i : \text{the slot } i \text{ of the block of } b
 ///     \text{ holds } t \}\quad \triangleright\ \text{one comparison of}
 ///     \text{ vectors} \cr
 /// 4 & \qquad \textbf{if}\ E = \emptyset\ \textbf{then continue} \cr
 /// 5 & \qquad j \gets b \bmod c;\ [ l , u ) \gets [ \sigma_{j-1} - j + 1 ,
 ///     \sigma_j - j )\quad \triangleright\ l = 0 \text{ for } j = 0 \cr
 /// 6 & \qquad \textbf{if}\ E \cap [ l , u ) \ne \emptyset\ \textbf{then
 ///     return true} \cr
 /// 7 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// removes all the keys
 void clear( void );

 /// the keys in, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 /// whether no key is in
 [[nodiscard]] bool empty( void ) const;

 /// the slots, \f$ s \f$ per block
 [[nodiscard]] std::size_t capacity( void ) const;

 /// `size() / capacity()`, the load \f$ \alpha \f$
 [[nodiscard]] double load( void ) const;

 /// the bound \f$ 2 n / ( R \, 2^f ) \f$ of the false positive probability
 /// of a key out of the set
 [[nodiscard]] double falsePositiveBound( void ) const;

 /// the bits of the blocks
 [[nodiscard]] std::size_t bits( void ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "VectorQuotientFilter";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix( { sizeof( T ) , sizeof( F ) } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 static constexpr bool kByte = sizeof( F ) == 1;
 static constexpr unsigned kSlots = kByte ? 48 : 28;          ///< \f$ s \f$
 static constexpr unsigned kBuckets = kByte ? 80 : 36;        ///< \f$ c \f$
 static constexpr unsigned kWords = kByte ? 2 : 1;            ///< of metadata
 static constexpr unsigned kHead = 8 * kWords / sizeof( F );  ///< lanes
 static constexpr unsigned kAlt = 3 * kSlots / 4;     ///< in use, the other
 static constexpr double kLoad = kByte ? 0.9 : 0.85;  ///< of the build

 /// a block: the metadata in the first lanes, the slots in the others
 struct alignas( 64 ) Block
 {
  F v[ 64 / sizeof( F ) ];
  using persist_unit = F;  ///< 64 bytes in a file
  };

 /// the metadata of a block, words of 64 bits
 struct Meta
 {
  std::uint64_t w[ 2 ] = { 0 , 0 };
  };

 /// the hash of \f$ x \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the fingerprint of the hash \f$ h \f$
 [[nodiscard]] static F tag( std::uint64_t h );

 /// the first bucket of the hash \f$ h \f$
 [[nodiscard]] std::size_t bucket( std::uint64_t h ) const;

 /// the other bucket of the fingerprint \f$ t \f$ in the bucket \f$ b \f$
 [[nodiscard]] std::size_t other( std::size_t b , F t ) const;

 [[nodiscard]] static Meta meta( const Block & B );
 static void setMeta( Block & B , const Meta & M );

 /// the slots in use of a block
 [[nodiscard]] static unsigned used( const Meta & M );

 /// the place of the 1 of rank \f$ k \f$ of the metadata
 [[nodiscard]] static unsigned select( const Meta & M , unsigned k );

 /// the slots \f$ [ l , u ) \f$ of the bucket \f$ j \f$ of a block
 static void range( const Meta & M , unsigned j , unsigned & l ,
		    unsigned & u );

 /// the first slot of a block among \f$ [ l , u ) \f$ equal to \f$ t \f$,
 /// -1 if none
 [[nodiscard]] static int find( const Block & B , F t , unsigned l ,
				unsigned u );

 /// \f$ t \f$ into the bucket \f$ b \f$, whose block is not full
 void put( std::size_t b , F t );

 /// \f$ t \f$ removed from the bucket \f$ b \f$, false if it is not there
 bool take( std::size_t b , F t );

 /// whether the bucket \f$ b \f$ holds \f$ t \f$
 [[nodiscard]] bool holds( std::size_t b , F t ) const;

 std::vector< Block , MappableAllocator< Block > > f_B;  ///< the blocks
 std::size_t f_R = kBuckets;                            ///< \f$ R \f$
 std::size_t f_n = 0;                                   ///< the keys in
 H f_h;                                                 ///< the hash

 };  // end( class VectorQuotientFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/VectorQuotientFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif /* VectorQuotientFilter.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File VectorQuotientFilter.h --------------------*/
/*--------------------------------------------------------------------------*/
