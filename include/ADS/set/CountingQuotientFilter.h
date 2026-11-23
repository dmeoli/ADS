/*--------------------------------------------------------------------------*/
/*---------------------- File CountingQuotientFilter.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The counting quotient filter of Pandey, Bender, Johnson and Patro, beyond
 * the book: a dynamic filter of a multiset \f$ S \f$ of keys, which takes
 * insertions and removals of any number of copies of a key and answers, for
 * a key \f$ x \f$, a count that is exact for a key of \f$ S \f$ unless the
 * fingerprint of \f$ x \f$ is shared with another key, and 0 for a key out
 * of \f$ S \f$ but with a small probability. It is built on the table of
 * the rank-and-select quotient filter of the same paper, `QuotientTable`,
 * which the range filter of `MementoFilter.h` shares.
 *
 * The table. A key has the fingerprint \f$ y \f$ of \f$ p = \ell + r \f$
 * bits, the high \f$ p \f$ bits of its hash, split into the quotient
 * \f$ q = \lfloor y / 2^r \rfloor < m = 2^\ell \f$ and the remainder
 * \f$ y \bmod 2^r \f$. The table has \f$ m \f$ slots of a type `S` and,
 * past them, a few blocks of spare slots for the overflow of the last ones
 * (the slots of \f$ \lceil 10 \sqrt m / 64 \rceil \f$ blocks); the slots
 * hold the remainders, those of one quotient \f$ q \f$ (its run) in
 * consecutive slots, the runs in the order of their quotients, a run
 * starting at the slot \f$ q \f$ or, if the runs before it reach that slot,
 * just after the last of them (the hashing of Robin Hood, cf. the linear
 * probing of `OpenHashTable.h`). Two bits per slot describe them: the bit
 * \f$ \mathrm{occ}( q ) \f$ is set when the quotient \f$ q \f$ has a run,
 * and \f$ \mathrm{end}( i ) \f$ when the slot \f$ i \f$ is the last of a
 * run, so that the run of the quotient of rank \f$ j \f$ among those with
 * \f$ \mathrm{occ} \f$ set ends at the slot of the end bit of rank
 * \f$ j \f$. The slots are cut into blocks of 64, and a block keeps the two
 * words of its bits and a byte, its offset \f$ o_b \f$, the number of its
 * slots, from the first, that the runs of the quotients of the blocks
 * before it take (saturated at 255, and then computed): the end of the run
 * of \f$ q \f$, in the block \f$ b = \lfloor q / 64 \rfloor \f$, is the end
 * bit of rank \f$ \mathrm{rank}_{\mathrm{occ}}( q ) - 1 \f$ of the block
 * \f$ b \f$ counted from its slot \f$ o_b \f$ on, one `select` of a word
 * (and more if the run leaves the block), and a query reads one block, or
 * two, in \f$ O( 1 ) \f$ time if the clusters (the maximal sequences of
 * slots in use) are short. The layout of a block is that of the paper, the
 * offset, the two words and the 64 slots in \f$ 17 + 64 s \f$ bytes,
 * \f$ s \f$ the bytes of `S`: \f$ 8 s + 2.125 \f$ bits per slot.
 *
 * An insertion of \f$ k \f$ slots into a run shifts by one slot, \f$ k \f$
 * times, the slots from the place up to the first slot not in use, and adds
 * 1 to the offsets of the blocks crossed; a removal moves back the slots of
 * the runs after the place, each run no further than its quotient, as the
 * removal of the linear probing does, and computes again the offsets of the
 * blocks after it until one does not change. `replace()` changes the
 * \f$ o \f$ slots of a run from a place into \f$ k \f$ others, by these
 * two, so that the filters write their encodings of a key (a remainder and
 * a counter, a fingerprint and its mementos) through it alone.
 *
 * The counters. A run holds its remainders in increasing order, and a
 * remainder \f$ x \f$ inserted \f$ c \f$ times takes \f$ c \f$ slots for
 * \f$ c \le 2 \f$ (\f$ x \f$, \f$ x x \f$) and otherwise the slots
 * \f$ x \, d_{k-1} \cdots d_0 \, x \f$, the digits of \f$ c - 3 \f$ in base
 * \f$ 2^r - 2 \f$, each shifted to avoid 0 and \f$ x \f$, with a leading 0
 * if the first is not below \f$ x \f$ (for \f$ x = 0 \f$: \f$ 000 \f$ for 3,
 * and \f$ 0 \, d \cdots d \, 00 \f$ for more, in base \f$ 2^r - 1 \f$ with
 * no digit 0): a slot below the one before it, in an increasing run, opens
 * a counter, and the counter of a key takes
 * \f$ O( 1 + \log c / r ) \f$ slots. A key in \f$ S \f$ with \f$ c \f$
 * copies is counted exactly unless another key of \f$ S \f$ has the same
 * fingerprint, whose copies add to its count; a key out of \f$ S \f$ is
 * counted 0 unless its fingerprint is that of a key of \f$ S \f$, which for
 * a random hash has the probability at most \f$ D / 2^p \f$, \f$ D \f$ the
 * distinct fingerprints in, i.e., \f$ \alpha \, 2^{-r} \f$ at most at the
 * load \f$ \alpha \f$ of the slots, in \f$ ( r + 2.125 ) / \alpha \f$ bits
 * per key for distinct keys.
 *
 * `resize()` takes the table to \f$ m' = 2^{\ell'} \f$ slots with the same
 * fingerprints, i.e., \f$ r' = p - \ell' \f$ (so that doubling the slots
 * takes one bit from the remainders, and doubles the false positives at the
 * same load), by inserting each fingerprint with its count, in their
 * order, into the new table; `merge()` adds to a filter the counts of
 * another of the same fingerprints and the same hash, by the merge of the
 * two sequences of fingerprints, inserted in order into a new table,
 * doubled while the slots of both pass 0.95 of it. Inserted in order, each
 * fingerprint goes after all the others, and the cost of both is linear in
 * the slots in the expected case.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`], and mixed by `hashWord()` otherwise.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{40} \f$ slots;
 * \f$ p \le 64 \f$; an insertion of \f$ c \ge 1 \f$ copies, true, leaves
 * the key maybe there, and its count at least \f$ c \f$, a postcondition;
 * `QuotientTable::consistent()` checks the offsets and the bits of the
 * table against its runs, in time linear in the slots, for the tests. A
 * removal of \f$ c \f$ copies of a key whose count is below \f$ c \f$ is
 * rejected (false, nothing changed). The operations are defined in
 * `CountingQuotientFilter.tpp`.
 *
 * \see P. Pandey, M. A. Bender, R. Johnson, R. Patro, A general-purpose
 *      counting filter: making every bit count, Proceedings of the 2017 ACM
 *      International Conference on Management of Data (SIGMOD 2017),
 *      775-787, 2017, doi:10.1145/3035918.3035963
 * \see M. A. Bender, M. Farach-Colton, R. Johnson, R. Kraner,
 *      B. C. Kuszmaul, D. Medjedovic, P. Montes, P. Shetty, R. P. Spillane,
 *      E. Zadok, Don't thrash: how to cache your hash on flash, Proceedings
 *      of the VLDB Endowment 5(11), 1627-1637, 2012,
 *      doi:10.14778/2350229.2350275 (the quotient filter)
 * \see J. G. Cleary, Compact hash tables using bidirectional linear
 *      probing, IEEE Transactions on Computers C-33(9), 828-834, 1984,
 *      doi:10.1109/TC.1984.1676499 (the quotients by the place)
 * \see P. Celis, P.-Å. Larson, J. I. Munro, Robin Hood hashing,
 *      Proceedings of the 26th Annual Symposium on Foundations of Computer
 *      Science (FOCS), 281-288, 1985, doi:10.1109/SFCS.1985.48
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

#ifndef __ADS_CountingQuotientFilter
 #define __ADS_CountingQuotientFilter
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/external/MappedFile.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <array>
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

namespace detail
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS QuotientTable ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the table of a rank-and-select quotient filter: \f$ 2^\ell \f$
/// quotients, their runs of slots of type `S` (an unsigned integer of 8,
/// 16, 32 or 64 bits) in blocks of 64 with the bits occupied and end and
/// an offset
template< class S >
class QuotientTable
{

 static_assert( std::is_unsigned_v< S > && ( sizeof( S ) <= 8 ) ,
		"the slots are unsigned words" );

/*--------------------------------------------------------------------------*/
 public:
 /// a table of one block, empty
 QuotientTable( void ) : QuotientTable( 6 ) { }

 /// the empty table of \f$ m = 2^\ell \f$ quotients, \f$ \ell \ge 6 \f$,
 /// and the spare blocks past them
 /// \pre \f$ 6 \le \ell < 40 \f$
 explicit QuotientTable( int l );

 /// \f$ \ell \f$, the bits of a quotient
 [[nodiscard]] int quotientBits( void ) const { return( f_l ); }

 /// the quotients, \f$ m = 2^\ell \f$
 [[nodiscard]] std::size_t quotients( void ) const {
  return( std::size_t( 1 ) << f_l );
  }

 /// all the slots, the spare ones included
 [[nodiscard]] std::size_t slots( void ) const { return( 64 * f_nb ); }

 /// the slots in use
 [[nodiscard]] std::size_t used( void ) const { return( f_used ); }

 /// whether the quotient \f$ q \f$ has a run
 [[nodiscard]] bool occupied( std::size_t q ) const {
  return( ( occ( q / 64 ) >> ( q % 64 ) ) & 1 );
  }

 /// whether the slot \f$ i \f$ ends a run
 [[nodiscard]] bool isEnd( std::size_t i ) const {
  return( ( end( i / 64 ) >> ( i % 64 ) ) & 1 );
  }

 /// the slot \f$ i \f$
 [[nodiscard]] S get( std::size_t i ) const {
  S v;
  std::memcpy( &v , slotAt( i ) , sizeof( S ) );
  return( v );
  }

 /// the slot \f$ i \f$ set to \f$ v \f$
 void set( std::size_t i , S v ) {
  std::memcpy( slotAt( i ) , &v , sizeof( S ) );
  }

 /// the end of the run of the last quotient up to \f$ q \f$ that has one,
 /// or \f$ q \f$ if that run ends before \f$ q \f$ (or there is none)
 ///
 /// In pseudocode (Pandey, Bender, Johnson and Patro), \f$ b \f$ the block
 /// of \f$ q \f$ and \f$ o_b \f$ its offset:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{runEnd}( q ) \cr
 /// 2 & \quad b \gets \lfloor q / 64 \rfloor;\ j \gets
 ///     \mathrm{rank}_{\mathrm{occ}}( b , q \bmod 64 )\quad \triangleright
 ///     \ \text{the bits up to } q \text{, itself included} \cr
 /// 3 & \quad \textbf{if}\ j = 0\ \textbf{then return}\ \max( q , 64 b +
 ///     o_b - 1 ) \cr
 /// 4 & \quad e \gets \text{the end bit of rank } j - 1 \text{ of the slots }
 ///     64 b + o_b , 64 b + o_b + 1 , \ldots\quad \triangleright
 ///     \ \mathrm{select} \text{ on the words} \cr
 /// 5 & \quad \textbf{return}\ \max( q , e )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t runEnd( std::size_t q ) const {
  return( runEnd( q , offset( q / 64 ) ) );
  }

 /// the first slot of the run of \f$ q \f$, or the slot where a run of
 /// \f$ q \f$ would start
 [[nodiscard]] std::size_t runStart( std::size_t q ) const {
  return( q == 0 ? 0 : runEnd( q - 1 ) + 1 );
  }

 /// the \f$ o \f$ slots from \f$ p \f$ of the run of \f$ q \f$ replaced by
 /// the \f$ k \f$ values of \f$ v \f$, false (and nothing changed) if the
 /// slots of the table cannot hold them or the slots in use would pass
 /// \f$ 0.95 \, m \f$ (beyond which the clusters, of mean length near
 /// \f$ ( 1 - \alpha )^{-2} \f$, grow too long); a run is
 /// made if \f$ q \f$ has none (then \f$ o = 0 \f$ and
 /// \f$ p = \f$ `runStart( q )`), and removed if it is left empty
 ///
 /// In pseudocode (Pandey, Bender, Johnson and Patro), \f$ e \f$ the end of
 /// the run of \f$ q \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{replace}( q , p , o , v_1 \cdots v_k ) \cr
 /// 2 & \quad \textbf{if}\ k > o \text{ and fewer than } k - o
 ///     \text{ slots from } p + o \text{ are free, or the slots in use}
 ///     \text{ would pass } 0.95\, m\ \textbf{then return false} \cr
 /// 3 & \quad \textbf{repeat}\ k - o\ \textbf{times}\ \text{open a slot at }
 ///     p + o\text{: the slots up to the first free one shift by one,}
 ///     \text{ the offsets of the blocks crossed grow by one} \cr
 /// 4 & \quad \text{write } v_1 , \ldots , v_k \text{ from } p
 ///     \text{; move the end bit of } q \text{ to the last slot of its run}
 ///     \cr
 /// 5 & \quad \textbf{if}\ k < o\ \textbf{then}\ \text{move the slots}
 ///     \text{ after } p + k \text{ back by } o - k \text{, each run no}
 ///     \text{ further than its quotient, and compute the offsets again} \cr
 /// 6 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ q < m \f$, \f$ p + o \le e + 1 \f$ and the slots in the run
 bool replace( std::size_t q , std::size_t p , std::size_t o , const S * v ,
	       std::size_t k );

 /// `fn( q , s , e )` for each quotient \f$ q \f$ with a run, from the
 /// smallest, the run in the slots \f$ s , \ldots , e \f$
 template< class Fn >
 void forEachRun( Fn && fn ) const;

 /// all the runs removed
 void clear( void );

 /// the bits of the blocks
 [[nodiscard]] std::size_t bits( void ) const { return( 8 * f_T.size() ); }

 /// true if the offsets and the bits agree with the runs, in time linear in
 /// the slots
 [[nodiscard]] bool consistent( void ) const;

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 static constexpr std::size_t kBlock = 17 + 64 * sizeof( S );  ///< bytes
 static constexpr unsigned kSat = 255;  ///< the offset to compute

 /// the bits \f$ 0 , \ldots , k - 1 \f$ of a word, \f$ k \le 64 \f$
 [[nodiscard]] static constexpr std::uint64_t low( std::size_t k ) {
  return( k >= 64 ? ~std::uint64_t( 0 ) : ( std::uint64_t( 1 ) << k ) - 1 );
  }

 /// the place of the \f$ k \f$-th 1 of \f$ w \f$, 64 if there are fewer
 [[nodiscard]] static int select( std::uint64_t w , std::size_t k ) {
  return( k < std::size_t( std::popcount( w ) ) ? selectInWord( w , int( k ) )
						 : 64 );
  }

 [[nodiscard]] const std::uint8_t * block( std::size_t b ) const {
  return( f_T.data() + b * kBlock );
  }
 [[nodiscard]] std::uint8_t * block( std::size_t b ) {
  return( f_T.data() + b * kBlock );
  }
 [[nodiscard]] const std::uint8_t * slotAt( std::size_t i ) const {
  return( block( i / 64 ) + 17 + ( i % 64 ) * sizeof( S ) );
  }
 [[nodiscard]] std::uint8_t * slotAt( std::size_t i ) {
  return( block( i / 64 ) + 17 + ( i % 64 ) * sizeof( S ) );
  }
 [[nodiscard]] std::uint64_t word( std::size_t b , int w ) const {
  std::uint64_t x;
  std::memcpy( &x , block( b ) + 1 + 8 * w , 8 );
  return( x );
  }
 void setWord( std::size_t b , int w , std::uint64_t x ) {
  std::memcpy( block( b ) + 1 + 8 * w , &x , 8 );
  }
 [[nodiscard]] std::uint64_t occ( std::size_t b ) const {
  return( word( b , 0 ) );
  }
 [[nodiscard]] std::uint64_t end( std::size_t b ) const {
  return( word( b , 1 ) );
  }
 void setEnd( std::size_t i , bool v );
 void setOcc( std::size_t q , bool v );

 /// the offset of the block \f$ b \f$, computed if saturated
 [[nodiscard]] std::size_t offset( std::size_t b ) const;

 /// `runEnd( q )` with the offset \f$ o_b \f$ of the block of \f$ q \f$
 [[nodiscard]] std::size_t runEnd( std::size_t q , std::size_t ob ) const;

 /// a lower bound on the slots in use from \f$ i \f$ on, 0 if \f$ i \f$
 /// is free
 [[nodiscard]] std::size_t busy( std::size_t i ) const;

 /// the first free slot from \f$ i \f$ on, `slots()` if none
 [[nodiscard]] std::size_t firstFree( std::size_t i ) const;

 /// one slot opened at \f$ p \f$ for the run of \f$ q \f$: the slots from
 /// \f$ p \f$ to the first free one shift by one with their end bits
 void open( std::size_t q , std::size_t p );

 /// the \f$ d \f$ slots from \f$ p \f$ of the run of \f$ q \f$ removed, the
 /// runs after them moved back
 void close( std::size_t q , std::size_t s , std::size_t p , std::size_t d );

 std::vector< std::uint8_t , MappableAllocator< std::uint8_t > > f_T;
 std::size_t f_nb = 0;    ///< the blocks
 std::size_t f_used = 0;  ///< the slots in use
 int f_l = 6;             ///< \f$ \ell \f$

 };  // end( class QuotientTable )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS CountingQuotientFilter ----------------------*/
/*--------------------------------------------------------------------------*/

/// a dynamic counting filter of the keys of type `T`, remainders of up to
/// the bits of `F` (8 or 16) in a rank-and-select quotient table, the
/// counters of the repeated keys in the slots of their runs
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class CountingQuotientFilter
{

 static_assert( std::is_same_v< F , std::uint8_t > ||
		  std::is_same_v< F , std::uint16_t > ,
		"the remainders have 8 or 16 bits" );

/*--------------------------------------------------------------------------*/
 public:
 /// a filter of one block, empty
 CountingQuotientFilter( void ) : CountingQuotientFilter( 1 ) { }

 /// the empty filter for \f$ n \f$ distinct keys, at the load 0.95 when
 /// they are in: \f$ m = 2^\ell \f$ slots, the least power of 2 with
 /// \f$ 0.95 \, m \ge n \f$, at least 64, and remainders of the bits of `F`
 /// \pre \f$ m < 2^{40} \f$
 explicit CountingQuotientFilter( std::size_t n , H h = H() );

 /// \f$ c \f$ copies of \f$ x \f$ into the filter by `insertHash()` of its
 /// hash, false (and nothing changed) if the filter is full
 /// \post `count( x ) >= c` if true
 bool insert( const T & x , std::uint64_t c = 1 );

 /// \f$ c \f$ copies of \f$ x \f$ removed by `removeHash()` of its hash,
 /// false (and nothing changed) if fewer are counted
 bool remove( const T & x , std::uint64_t c = 1 );

 /// the copies of \f$ x \f$, by `countHash()` of its hash
 [[nodiscard]] std::uint64_t count( const T & x ) const;

 /// whether \f$ x \f$ may be in the multiset, `count( x ) > 0`: false only
 /// if \f$ x \f$ is not in it
 [[nodiscard]] bool maybe( const T & x ) const;

 /// \f$ c \f$ copies of the hash \f$ h \f$ of 64 bits, which must be
 /// random, into the filter, false (and nothing changed) if it is full
 ///
 /// In pseudocode (Pandey, Bender, Johnson and Patro), \f$ y \f$ the high
 /// \f$ p \f$ bits of \f$ h \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertHash}( h , c ) \cr
 /// 2 & \quad q \gets \lfloor y / 2^r \rfloor;\ x \gets y \bmod 2^r \cr
 /// 3 & \quad \textbf{if}\ \lnot \mathrm{occ}( q )\ \textbf{then return}
 ///     \ \mathrm{replace}( q , \mathrm{runStart}( q ) , 0 ,
 ///     \mathrm{code}( x , c ) ) \cr
 /// 4 & \quad i \gets \mathrm{runStart}( q );
 ///     \ e \gets \mathrm{runEnd}( q ) \cr
 /// 5 & \quad \textbf{while}\ i \le e\ \textbf{do} \cr
 /// 6 & \qquad ( x' , c' , j ) \gets \text{the counter from } i
 ///     \text{, ending at } j \cr
 /// 7 & \qquad \textbf{if}\ x' = x\ \textbf{then return}\ \mathrm{replace}(
 ///     q , i , j - i + 1 , \mathrm{code}( x , c' + c ) ) \cr
 /// 8 & \qquad \textbf{if}\ x' > x\ \textbf{then break} \cr
 /// 9 & \qquad i \gets j + 1 \cr
 /// 10 & \quad \textbf{return}\ \mathrm{replace}( q , i , 0 ,
 ///     \mathrm{code}( x , c ) )
 /// \end{array}}
 /// \f]
 /// \pre \f$ c \ge 1 \f$
 bool insertHash( std::uint64_t h , std::uint64_t c = 1 );

 /// \f$ c \f$ copies of the hash \f$ h \f$ removed, false (and nothing
 /// changed) if fewer are counted
 ///
 /// In pseudocode (Pandey, Bender, Johnson and Patro):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{removeHash}( h , c ) \cr
 /// 2 & \quad ( i , j , c' ) \gets \text{the counter of } x \text{ in the}
 ///     \text{ run of } q \text{, as in insertHash} \cr
 /// 3 & \quad \textbf{if}\ \text{there is none or } c' < c\ \textbf{then
 ///     return false} \cr
 /// 4 & \quad \textbf{return}\ \mathrm{replace}( q , i , j - i + 1 ,
 ///     \mathrm{code}( x , c' - c ) )\quad \triangleright\ \text{no slot for
 ///     a count of } 0
 /// \end{array}}
 /// \f]
 bool removeHash( std::uint64_t h , std::uint64_t c = 1 );

 /// the copies of the hash \f$ h \f$: the count of its counter, 0 if none
 ///
 /// In pseudocode (Pandey, Bender, Johnson and Patro):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{countHash}( h ) \cr
 /// 2 & \quad \textbf{if}\ \lnot \mathrm{occ}( q )\ \textbf{then return}
 ///     \ 0 \cr
 /// 3 & \quad i \gets \mathrm{runStart}( q );
 ///     \ e \gets \mathrm{runEnd}( q ) \cr
 /// 4 & \quad \textbf{while}\ i \le e\ \textbf{do} \cr
 /// 5 & \qquad ( x' , c' , j ) \gets \text{the counter from } i \cr
 /// 6 & \qquad \textbf{if}\ x' \ge x\ \textbf{then return}\ c' \cdot [ x' =
 ///     x ] \cr
 /// 7 & \qquad i \gets j + 1 \cr
 /// 8 & \quad \textbf{return}\ 0
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::uint64_t countHash( std::uint64_t h ) const;

 /// whether the hash \f$ h \f$ may be in the multiset
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// the counts of \f$ o \f$ added to those of the filter, false (and
 /// nothing changed) if the slots of a table of \f$ p \f$ bits cannot hold
 /// them
 ///
 /// In pseudocode (Pandey, Bender, Johnson and Patro):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( o ) \cr
 /// 2 & \quad m' \gets m;\ \textbf{while}\ \text{the slots of both pass }
 ///     0.95\, m' \text{ and } p - \log_2 m' > 2\ \textbf{do}
 ///     \ m' \gets 2 m' \cr
 /// 3 & \quad A \gets \text{an empty table of } m' \text{ slots and }
 ///     p - \log_2 m' \text{ bits of remainder} \cr
 /// 4 & \quad \textbf{foreach}\ ( y , c ) \text{ in the merge of the}
 ///     \text{ fingerprints of both, in order, the counts of an equal } y
 ///     \text{ added}\ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ \lnot \mathrm{insert}_A( y , c )\ \textbf{then
 ///     return false} \cr
 /// 6 & \quad \text{the table of the filter} \gets A;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre the hashes of both are equal, and their fingerprints have the
 /// same bits
 bool merge( const CountingQuotientFilter & o );

 /// the table taken to \f$ m' \f$ slots, a power of 2, with the same
 /// fingerprints, its remainders of \f$ p - \log_2 m' \f$ bits, by the
 /// insertion of every fingerprint with its count in order; false (and
 /// nothing changed) if they do not fit
 /// \pre \f$ m' \ge 64 \f$ a power of 2 with \f$ 1 \le p - \log_2 m' \le
 /// \f$ the bits of `F`
 bool resize( std::size_t m );

 /// removes all the keys
 void clear( void );

 /// the copies in, the sum of the counts
 [[nodiscard]] std::uint64_t size( void ) const;

 /// the distinct fingerprints in
 [[nodiscard]] std::size_t distinct( void ) const;

 /// whether no key is in
 [[nodiscard]] bool empty( void ) const;

 /// the slots \f$ m \f$ (without the spare ones)
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the slots in use, the counters included
 [[nodiscard]] std::size_t slotsUsed( void ) const;

 /// `slotsUsed() / capacity()`, the load \f$ \alpha \f$
 [[nodiscard]] double load( void ) const;

 /// the bits of a remainder, \f$ r \f$
 [[nodiscard]] int remainderBits( void ) const;

 /// the bits of a fingerprint, \f$ p = \ell + r \f$
 [[nodiscard]] int fingerprintBits( void ) const;

 /// the bound \f$ D / 2^p \f$ of the false positive probability of a key
 /// out of the multiset, \f$ D \f$ the distinct fingerprints in
 [[nodiscard]] double falsePositiveBound( void ) const;

 /// the bits of the table
 [[nodiscard]] std::size_t bits( void ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "CountingQuotientFilter";

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
 /// the slots of a counter, at most 2 + the digits of a count of 64 bits
 using Code = std::array< F , 70 >;

 /// the hash of \f$ x \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the fingerprint of the hash \f$ h \f$, its high \f$ p \f$ bits
 [[nodiscard]] std::uint64_t fingerprint( std::uint64_t h ) const;

 /// the slots of the counter of \f$ c \f$ copies of the remainder
 /// \f$ x \f$ in \f$ C \f$, their number returned (0 for \f$ c = 0 \f$)
 [[nodiscard]] std::size_t code( std::uint64_t x , std::uint64_t c ,
				 Code & C ) const;

 /// the counter from the slot \f$ i \f$ of a run: its remainder, its count
 /// and its last slot
 void decode( std::size_t i , std::uint64_t & x , std::uint64_t & c ,
	      std::size_t & j ) const;

 /// the counter of the fingerprint \f$ y \f$: false if the run of its
 /// quotient has none, and then \f$ i \f$ the slot where it would start;
 /// its first slot \f$ i \f$, its last \f$ j \f$ and its count \f$ c \f$
 bool find( std::uint64_t y , std::size_t & i , std::size_t & j ,
	    std::uint64_t & c ) const;

 /// \f$ c \f$ copies of the fingerprint \f$ y \f$ into the table
 bool add( std::uint64_t y , std::uint64_t c );

 /// `fn( y , c )` for each fingerprint \f$ y \f$ with its count, in order
 template< class Fn >
 void forEach( Fn && fn ) const;

 /// the table of \f$ 2^\ell \f$ slots filled with the pairs of \f$ Y \f$,
 /// in order, into this filter, false (and nothing changed) if they do not
 /// fit
 bool rebuild( int l , const std::vector< std::uint64_t > & Y ,
	       const std::vector< std::uint64_t > & C );

 detail::QuotientTable< F > f_Q;  ///< the table
 std::uint64_t f_n = 0;           ///< the sum of the counts
 std::size_t f_d = 0;             ///< the distinct fingerprints
 int f_r = 8 * sizeof( F );       ///< \f$ r \f$
 H f_h;                           ///< the hash of the keys

 };  // end( class CountingQuotientFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/CountingQuotientFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif /* CountingQuotientFilter.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File CountingQuotientFilter.h -------------------*/
/*--------------------------------------------------------------------------*/
