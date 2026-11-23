/*--------------------------------------------------------------------------*/
/*-------------------------- File MementoFilter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range filter of Eslami and Dayan, beyond the book: `MementoFilter`, a
 * dynamic filter of a set \f$ S \f$ of keys of \f$ w \f$ bits that tells
 * whether a range \f$ [ a , b ] \f$ of at most \f$ 2^r \f$ keys may hold a
 * key of \f$ S \f$, with a false positive probability bounded whatever the
 * keys and the queries (the correlated ones included), and takes the
 * insertions and the removals of keys one at a time and the growth of its
 * table, which the static `GrafiteFilter` [see `GrafiteFilter.h`] does not.
 *
 * The universe is cut into partitions of \f$ 2^r \f$ consecutive keys: a
 * key \f$ x \f$ has the prefix \f$ \pi( x ) = \lfloor x / 2^r \rfloor \f$
 * and the memento \f$ \mu( x ) = x \bmod 2^r \f$, its place in its
 * partition. The prefix is hashed into a quotient filter, the table of the
 * rank-and-select quotient filter [see `QuotientTable` in
 * `CountingQuotientFilter.h`] of \f$ m = 2^\ell \f$ quotients whose slots
 * (of a type `S`) hold \f$ f + r \f$ bits, a fingerprint of \f$ f \f$ bits
 * and a memento: the high \f$ \ell + f \f$ bits \f$ y \f$ of the hash of
 * \f$ \pi( x ) \f$ give the quotient \f$ \lfloor y / 2^f \rfloor \f$ and
 * the fingerprint \f$ \phi = y \bmod 2^f \f$, 1 if it is 0, so that no
 * fingerprint is 0. A run holds its keepsake boxes, one per fingerprint, in
 * increasing order of their fingerprints, and the box of \f$ \phi \f$ the
 * mementos \f$ \mu_1 \le \cdots \le \mu_k \f$ of the keys in with that
 * quotient and that fingerprint, as
 * - \f$ ( \phi , \mu_1 ) \f$ for \f$ k = 1 \f$;
 * - \f$ ( \phi , \mu_1 ) ( \phi , \mu_2 ) \f$ for \f$ k = 2 \f$;
 * - \f$ ( \phi , \mu_1 ) ( 0 , \mu_k ) \f$ and then, packed in the
 *   \f$ f + r \f$ bits of the slots that follow, the length \f$ k - 2 \f$
 *   in chunks of \f$ r \f$ bits and \f$ \mu_2 , \ldots , \mu_{k-1} \f$ in
 *   \f$ r \f$ bits each, for \f$ k \ge 3 \f$: the fingerprint 0, below that
 *   of the box, tells this case, and the length, in base
 *   \f$ 2^r - 1 \f$ after as many chunks of \f$ 2^r - 1 \f$ as its digits
 *   but one when it is not below \f$ 2^r - 1 \f$, tells where the box ends.
 * The smallest and the largest memento of a box are thus in its first two
 * slots, and a box of \f$ k \le 2^r \f$ keys takes at most \f$ k \f$
 * slots when \f$ r \le f \f$.
 *
 * A range \f$ [ a , b ] \f$ with \f$ b - a < 2^r \f$ meets the partition
 * of \f$ \pi( a ) \f$ and, if it differs, that of \f$ \pi( b ) =
 * \pi( a ) + 1 \f$: in the first case it may hold a key if the box of
 * \f$ \pi( a ) \f$ has a memento in \f$ [ \mu( a ) , \mu( b ) ] \f$, which
 * its two first slots decide but when \f$ \mu_1 < \mu( a ) \f$ and
 * \f$ \mu_k > \mu( b ) \f$, and in the second if the box of \f$ \pi( a ) \f$
 * has \f$ \mu_k \ge \mu( a ) \f$ or that of \f$ \pi( b ) \f$ has
 * \f$ \mu_1 \le \mu( b ) \f$; a longer range also asks whether each
 * partition between them has a box, up to \f$ m \f$ of them (beyond, the
 * answer is true if the filter is not empty). A query reads one run, or
 * two, of the table, in \f$ O( 1 ) \f$ expected time for the keys of a
 * random hash and a constant load.
 *
 * Analysis. Let \f$ P \f$ be the partitions that hold a key of \f$ S \f$
 * and \f$ \rho = ( 2^f + 2 ) / 4^f \f$ the probability that two
 * fingerprints are equal (the value 0 is replaced by 1). A box answers for
 * a partition \f$ \pi \f$ only if it is that of \f$ \pi \f$ or that of
 * another partition with the same quotient and fingerprint, which for a
 * random hash has the probability \f$ \rho / m \f$; by the union bound an
 * empty range that meets at most two partitions is a false positive with
 * probability at most
 * \f[ \varepsilon \le \frac{2 P \rho}{m} \le \frac{2 n \rho}{m}
 *     \approx \frac{n}{m} \, 2^{1-f} \;, \f]
 * whatever the keys and the range, since the only randomness is that of
 * the hash, \f$ n \f$ the keys in, and \f$ P \rho / m \f$ for a point;
 * the mementos only lower it. The table takes \f$ 8 s + 2.125 \f$ bits per
 * slot, \f$ s \f$ the bytes of `S`, i.e., \f$ f + r + 2.125 \f$ when
 * \f$ f + r \f$ fills them.
 *
 * `resize()` doubles the quotients \f$ k \f$ times by taking the
 * \f$ k \f$ high bits of the fingerprints into the quotients, as the
 * counting quotient filter does, so that the fingerprints lose \f$ k \f$
 * bits and the bound doubles each time at the same load; the new keys get
 * the same fingerprints (the paper keeps instead fingerprints of
 * \f$ f \f$ bits for them, by an age counter in each slot, the fluid
 * fingerprints of InfiniFilter, which this implementation does not have).
 *
 * The keys are of a type `T` that `radixKey()` maps in order onto unsigned
 * words [see `RadixSort.h`], of \f$ w = 32 \f$ or 64 bits, the prefix of a
 * key hashed by `hashWord()` with a seed [see `FastHash.h`].
 *
 * The contracts [see `Contract.h`]: \f$ r \ge 2 \f$, \f$ f \ge 2 \f$,
 * \f$ f + r \le \f$ the bits of `S`, \f$ r < w \f$, fewer than
 * \f$ 2^{40} \f$ slots; a range has \f$ a \le b \f$; a key inserted may be
 * there, a postcondition. A removal of a key whose memento is not in its
 * box is rejected (false, nothing changed); a key removed must have been
 * inserted, which no contract can check. The operations are defined in
 * `MementoFilter.tpp`.
 *
 * \see N. Eslami, N. Dayan, Memento filter: a fast, dynamic, and robust
 *      range filter, Proceedings of the ACM on Management of Data 2(6),
 *      244, 1-27, 2024, doi:10.1145/3698820
 * \see P. Pandey, M. A. Bender, R. Johnson, R. Patro, A general-purpose
 *      counting filter: making every bit count, Proceedings of SIGMOD 2017,
 *      775-787, 2017, doi:10.1145/3035918.3035963 (the rank-and-select
 *      quotient filter)
 * \see N. Dayan, I. Bercea, P. Reviriego, R. Pagh, InfiniFilter: expanding
 *      filters to infinity and beyond, Proceedings of the ACM on Management
 *      of Data 1(2), 140, 1-27, 2023, doi:10.1145/3589285 (the fluid
 *      fingerprints)
 * \see M. Costa, P. Ferragina, G. Vinciguerra, Grafite: taming adversarial
 *      queries with optimal range filters, Proceedings of the ACM on
 *      Management of Data 2(1), 1-23, 2024, doi:10.1145/3639258
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

#ifndef __ADS_MementoFilter
 #define __ADS_MementoFilter
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/set/CountingQuotientFilter.h"
#include "ADS/sort/RadixSort.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MementoFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a dynamic range filter of the keys of type `T`: the prefixes of
/// \f$ w - r \f$ bits hashed into a quotient filter of slots of type `S`,
/// \f$ f \f$ bits of fingerprint and \f$ r \f$ of memento, the false
/// positives of a range of at most \f$ 2^r \f$ keys at most about
/// \f$ ( n / m ) \, 2^{1-f} \f$ whatever the keys and the range
template< RadixKey T = std::uint64_t , class S = std::uint16_t >
class MementoFilter
{

 static_assert( std::is_unsigned_v< S > && ( sizeof( S ) <= 8 ) ,
		"the slots are unsigned words" );

/*--------------------------------------------------------------------------*/
 public:
 /// a filter of one block, empty
 MementoFilter( void ) : MementoFilter( 1 ) { }

 /// the empty filter for \f$ n \f$ keys, at the load 0.95 when each takes
 /// a slot: \f$ m = 2^\ell \f$ quotients, the least power of 2 with
 /// \f$ 0.95 \, m \ge n \f$, at least 64; mementos of \f$ r \f$ bits and
 /// fingerprints of \f$ f \f$ (the bits of `S` left by \f$ r \f$, if 0)
 /// \pre \f$ r \ge 2 \f$, \f$ 2 \le f \f$, \f$ f + r \le \f$ the bits of
 /// `S`, \f$ r < w \f$ and \f$ m < 2^{40} \f$
 explicit MementoFilter( std::size_t n , int r = 8 , int f = 0 ,
			 std::uint64_t seed = 1 );

 /// \f$ x \f$ into the filter: its memento into the box of its prefix,
 /// false (and nothing changed) if the table is full
 ///
 /// In pseudocode (Eslami and Dayan), \f$ ( q , \phi ) \f$ the quotient
 /// and the fingerprint of \f$ \pi( x ) \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad i \gets \mathrm{runStart}( q ) \cr
 /// 3 & \quad \textbf{if}\ \mathrm{occ}( q )\ \textbf{then} \cr
 /// 4 & \qquad \textbf{while}\ i \le \mathrm{runEnd}( q ) \text{ and the box}
 ///     \text{ at } i \text{ has a fingerprint } \phi' < \phi\ \textbf{do}
 ///     \ i \gets \text{the slot after the box} \cr
 /// 5 & \qquad \textbf{if}\ \phi' = \phi\ \textbf{then return}
 ///     \ \mathrm{replace}( q , i , \text{its slots} , \text{the box with }
 ///     \mu( x ) \text{ added} ) \cr
 /// 6 & \quad \textbf{return}\ \mathrm{replace}( q , i , 0 , ( \phi ,
 ///     \mu( x ) ) )
 /// \end{array}}
 /// \f]
 /// \post `maybe( x )` if true
 bool insert( const T & x );

 /// one insertion of \f$ x \f$ removed, its memento from the box of its
 /// prefix, false (and nothing changed) if the box has no such memento
 /// \pre \f$ x \f$ was inserted (otherwise another key may be lost)
 bool remove( const T & x );

 /// whether \f$ [ a , b ] \f$ may hold a key: true if it does, and for an
 /// empty range of at most \f$ 2^r \f$ keys with probability at most
 /// `falsePositiveBound()`
 ///
 /// In pseudocode (Eslami and Dayan), \f$ \mu_1 \f$ and \f$ \mu_k \f$ the
 /// smallest and the largest memento of a box, both in its first two
 /// slots:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{mayContain}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ \pi( a ) = \pi( b )\ \textbf{then} \cr
 /// 3 & \qquad B \gets \text{the box of } \pi( a );\ \textbf{if}\ B =
 ///     \textbf{nil}\ \textbf{then return false} \cr
 /// 4 & \qquad \textbf{if}\ \mu_1 > \mu( b ) \text{ or } \mu_k < \mu( a )
 ///     \ \textbf{then return false} \cr
 /// 5 & \qquad \textbf{if}\ \mu_1 \ge \mu( a ) \text{ or } \mu_k \le \mu( b )
 ///     \ \textbf{then return true} \cr
 /// 6 & \qquad \textbf{return}\ \text{a memento of the list of } B
 ///     \text{ is in } [ \mu( a ) , \mu( b ) ] \cr
 /// 7 & \quad \textbf{if}\ \text{the box of } \pi( a ) \text{ has } \mu_k \ge
 ///     \mu( a )\ \textbf{then return true} \cr
 /// 8 & \quad \textbf{if}\ \text{the box of } \pi( b ) \text{ has } \mu_1 \le
 ///     \mu( b )\ \textbf{then return true} \cr
 /// 9 & \quad \textbf{return}\ \text{a partition strictly between } \pi( a )
 ///     \text{ and } \pi( b ) \text{ has a box}
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \le b \f$
 [[nodiscard]] bool mayContain( const T & a , const T & b ) const;

 /// `mayContain( x , x )`: true for a key, and with probability at most
 /// `falsePositiveBound() / 2` otherwise
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the table taken to \f$ m' \ge m \f$ quotients, a power of 2, the high
 /// bits of the fingerprints into the quotients, by the insertion of every
 /// box in order; false (and nothing changed) if they do not fit
 /// \pre \f$ m' \ge m \f$ a power of 2 with \f$ f - \log_2( m' / m ) \ge 2 \f$
 bool resize( std::size_t m );

 /// the bound \f$ 2 n \rho / m \f$ of the false positive probability of an
 /// empty range of at most \f$ 2^r \f$ keys, \f$ n \f$ the keys in
 [[nodiscard]] double falsePositiveBound( void ) const;

 /// removes all the keys
 void clear( void );

 /// the keys in, with their repetitions
 [[nodiscard]] std::size_t size( void ) const;

 /// whether no key is in
 [[nodiscard]] bool empty( void ) const;

 /// the boxes, the distinct pairs of a quotient and a fingerprint
 [[nodiscard]] std::size_t boxes( void ) const;

 /// the quotients \f$ m \f$ (without the spare slots)
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the slots in use over \f$ m \f$, the load \f$ \alpha \f$
 [[nodiscard]] double load( void ) const;

 /// \f$ r \f$, the bits of a memento
 [[nodiscard]] int mementoBits( void ) const;

 /// \f$ f \f$, the bits of a fingerprint
 [[nodiscard]] int fingerprintBits( void ) const;

 /// the bits of the table
 [[nodiscard]] std::size_t bits( void ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "MementoFilter";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix( { sizeof( T ) , sizeof( S ) } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 /// a box read from its first slot: the fingerprint, the smallest and the
 /// largest memento, the mementos, the first slot of the list, the bit of
 /// the list where its mementos start and the last slot of the box
 struct Box
 {
  std::uint64_t fp = 0 , lo = 0 , hi = 0;
  std::size_t k = 0 , list = 0 , at = 0 , last = 0;
  };

 /// the word of a key, in the order of the keys
 [[nodiscard]] static std::uint64_t word( const T & x );

 /// the quotient and the fingerprint of the prefix \f$ p \f$
 void place( std::uint64_t p , std::size_t & q , std::uint64_t & fp ) const;

 /// the box from the slot \f$ i \f$ of a run ending at \f$ e \f$
 [[nodiscard]] Box box( std::size_t i , std::size_t e ) const;

 /// the box of the fingerprint \f$ fp \f$ in the run of \f$ q \f$: false if
 /// none, and then \f$ i \f$ the slot where it would go
 bool find( std::size_t q , std::uint64_t fp , Box & B ,
	    std::size_t & i ) const;

 /// the \f$ r \f$ bits at the bit \f$ t \f$ of the slots from \f$ i \f$
 [[nodiscard]] std::uint64_t chunk( std::size_t i , std::size_t t ) const;

 /// the mementos of a box, in order
 [[nodiscard]] std::vector< std::uint64_t > mementos( const Box & B ) const;

 /// the slots of the box of \f$ fp \f$ with the mementos \f$ M \f$
 [[nodiscard]] std::vector< S >
 encode( std::uint64_t fp , const std::vector< std::uint64_t > & M ) const;

 /// whether the box \f$ B \f$ has a memento in \f$ [ a , b ] \f$
 [[nodiscard]] bool meets( const Box & B , std::uint64_t a ,
			   std::uint64_t b ) const;

 /// the box of the partition \f$ p \f$, false if none
 bool boxOf( std::uint64_t p , Box & B ) const;

 /// the memento \f$ u \f$ into the box of \f$ ( q , fp ) \f$
 bool add( std::size_t q , std::uint64_t fp , std::uint64_t u );

 detail::QuotientTable< S > f_Q;  ///< the table
 std::size_t f_n = 0;             ///< the keys in
 std::size_t f_boxes = 0;         ///< the boxes
 std::uint64_t f_seed = 1;        ///< the seed of the hash
 int f_r = 8;                     ///< \f$ r \f$
 int f_f = 8;                     ///< \f$ f \f$
 int f_w = 16;                    ///< the bits of a slot, \f$ f + r \f$

 };  // end( class MementoFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/MementoFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MementoFilter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MementoFilter.h ------------------------*/
/*--------------------------------------------------------------------------*/
