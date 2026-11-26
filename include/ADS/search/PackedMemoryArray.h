/*--------------------------------------------------------------------------*/
/*------------------------ File PackedMemoryArray.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The packed memory array of Itai, Konheim and Rodeh and of Bender, Demaine
 * and Farach-Colton, beyond the book: a sorted set of keys kept in order in
 * one array with empty cells between them, so that an insertion moves few
 * keys and a visit reads the memory in order, with the layout of the
 * searches of Wheatman, Burns, Buluç and Xu, the compressed leaves and the
 * insertions and removals of sorted batches in parallel of their CPMA (see
 * \ref theory_search_pma).
 *
 * The array is cut into \f$ L = 2^H \f$ leaves of \f$ C \f$ units each,
 * the leaves of an implicit complete binary tree whose node of height
 * \f$ h \f$ is a run of \f$ 2^h \f$ consecutive leaves (its region). A unit
 * is a key in `PackedMemoryArray< K >` and a byte in
 * `CompressedPackedMemoryArray< K >`; the units of a region are those that
 * its keys take, and its density their ratio to the \f$ 2^h C \f$ units of
 * the region. The thresholds of Bender, Demaine and Farach-Colton bound the
 * density of a region of height \f$ h \f$ by
 * \f[
 *   \rho_h = \frac{1}{8} + \frac{1}{8} \frac{h}{H} \quad \text{and}
 *   \quad \tau_h = 1 - \frac{1}{4} \frac{h}{H} ,
 * \f]
 * i.e., from \f$ [ 1/8 , 1 ] \f$ at a leaf to \f$ [ 1/4 , 3/4 ] \f$ at the
 * root, so that a region is the more strictly bounded the larger it is.
 * An insertion that brings a leaf past \f$ C \f$ units climbs the tree to
 * the first region whose density, the key included, is at most
 * \f$ \tau_h \f$, and spreads its keys evenly over its leaves (the
 * redistribution); a removal that brings a leaf below \f$ \rho_0 C \f$
 * units climbs to the first region at least \f$ \rho_h \f$ dense; when the
 * root itself is out of its bounds, the array is built again with the
 * fewest leaves that make it at most half full, which doubles or halves
 * \f$ L \f$ in the common case. Hence, with \f$ C = \Theta( \log N ) \f$,
 * an update moves \f$ O( \log^2 N ) \f$ units amortized, i.e.,
 * \f$ O( \log^2 N / B ) \f$ lines of cache of \f$ B \f$ units, besides its
 * search (Bender, Demaine and Farach-Colton).
 *
 * The layout, which decides the constants:
 *
 * - the keys of a leaf are packed to its left, the first of them (its
 *   head) apart, as Wheatman and Xu do: a leaf is its head, its count of
 *   units and the cells of the other keys, with no empty cell before a key
 *   and no mark of an empty cell, and every leaf has a key when the set has
 *   one;
 * - the heads lie in an array of their own in the order of Eytzinger (the
 *   heap order of the implicit tree of the \f$ L \f$ leaves), the SPMA of
 *   Wheatman, Burns, Buluç and Xu: a search walks the heads from the root
 *   with no branch, \f$ \lfloor \log_2 L \rfloor + 1 \f$ comparisons that
 *   read the first lines of the array again at each search, and then scans
 *   the one leaf whose head precedes the key;
 * - in the compressed array a leaf keeps its head whole and the others as
 *   the differences from the previous key, each in the variable-byte code
 *   [see `encodeVByte()` in `IntegerCodes.h`], as in the CPMA, the delta
 *   encoding of the sorted keys: a key of 64 bits close to the previous one
 *   takes one byte instead of eight. A leaf of the compressed array is
 *   decoded from its head on, and a key that does not fit the leaf is a
 *   case of the redistribution like any other;
 * - the leaf has \f$ C = \max( 16 , 2^{\lceil \log_2 ( 2 b ) \rceil} )
 *   \f$ keys, or \f$ \max( 256 , 2^{\lceil \log_2 ( 16 b ) \rceil} ) \f$
 *   bytes when compressed, \f$ b \f$ the bits of \f$ n \f$, fixed at each
 *   building of the array, and a count of its units (4 bytes).
 *
 * The batches, `insertBatch()` and `eraseBatch()` on an executor [see
 * `Executor.h`], follow the three phases of the CPMA, each one parallel
 * within itself: (i) the merge, which sorts the batch, cuts it into blocks
 * whose borders are moved to the borders of the leaves, so that each leaf
 * belongs to one block, and merges into each leaf, by one search per leaf,
 * the keys of the batch that fall into it, writing in place the leaves
 * that still fit and with the same head and keeping the others apart (the
 * leaves that overflow); (ii) the counting, which climbs the tree a level
 * at a time from the leaves touched, the nodes of a level in parallel,
 * each region counted once from the counts of its children, up to the
 * regions that respect their thresholds; (iii) the redistribution of these
 * regions, the largest that contain each other, in parallel. A batch of at
 * least \f$ n / 8 \f$ keys merges the whole set with it and builds the
 * array again, and one of fewer than 64 keys goes a key at a time, as the
 * CPMA does at the two ends of the sizes of the batches.
 *
 * The contracts [see `Contract.h`]: the keys of
 * `CompressedPackedMemoryArray` are unsigned integers, those of
 * `PackedMemoryArray` are trivially copyable and totally ordered by `<`;
 * the order of the array (every leaf sorted, every key of a leaf below the
 * head of the next one, the counts of the units true, every leaf with a
 * key) is an invariant checked after each batch and each building. The
 * functions are defined in `PackedMemoryArray.tpp`.
 *
 * \see A. Itai, A. G. Konheim, M. Rodeh, A sparse table implementation of
 *      priority queues, Proceedings of ICALP 1981, LNCS 115, 417-431,
 *      1981, doi:10.1007/3-540-10843-2_34
 * \see M. A. Bender, E. D. Demaine, M. Farach-Colton, Cache-oblivious
 *      B-trees, SIAM Journal on Computing 35(2), 341-358, 2005,
 *      doi:10.1137/S0097539701389956 (the thresholds of the density)
 * \see B. Wheatman, H. Xu, A parallel packed memory array to store dynamic
 *      graphs, Proceedings of ALENEX 2021, 31-45, 2021,
 *      doi:10.1137/1.9781611976472.3 (the keys packed to the left)
 * \see B. Wheatman, R. Burns, A. Buluç, H. Xu, Optimizing search layouts
 *      in packed memory arrays, Proceedings of ALENEX 2023, 148-161, 2023,
 *      doi:10.1137/1.9781611977561.ch13 (the heads in the order of
 *      Eytzinger)
 * \see B. Wheatman, R. Burns, A. Buluç, H. Xu, CPMA: an efficient
 *      batch-parallel compressed set without pointers, Proceedings of
 *      PPoPP 2024, 348-363, 2024, doi:10.1145/3627535.3638492
 * \see P.-V. Khuong, P. Morin, Array layouts for comparison-based
 *      searching, ACM Journal of Experimental Algorithmics 22,
 *      1.3:1-1.3:39, 2017, doi:10.1145/3053370 (the search of the order of
 *      Eytzinger)
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

#ifndef __ADS_PackedMemoryArray
 #define __ADS_PackedMemoryArray
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/IntegerCodes.h"
#include "ADS/memory/Prefetch.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelSort.h"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// the keys of a packed memory array: unsigned integers if compressed
/// (`Z`), else trivially copyable and totally ordered
template< class K , bool Z >
concept PMAKey =
  ( Z && std::unsigned_integral< K > ) ||
  ( ! Z && std::totally_ordered< K > && std::is_trivially_copyable_v< K > &&
    std::is_default_constructible_v< K > );

/// the most bytes of the variable-byte code of a difference of two keys
/// `K`, or 1 if `K` is not an unsigned integer
template< class K >
constexpr std::size_t pmaWide( void )
{
 if constexpr( std::unsigned_integral< K > )
  return( std::max( sizeof( K ) , std::size_t( vbyteLength( std::uint64_t(
				    std::numeric_limits< K >::max() ) ) ) ) );
 else
  return( 1 );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS PackedMemoryArray -------------------------*/
/*--------------------------------------------------------------------------*/

/// the sorted set of the keys `K` of the packed memory array, with the
/// leaves compressed by the differences of the keys if `Z`
template< class K , bool Z = false >
 requires detail::PMAKey< K , Z >
class PackedMemoryArray
{

/*--------------------------------------------------------------------------*/
 public:
 using key_type = K;  ///< the type of the keys

 /// whether the leaves are compressed
 static constexpr bool kCompressed = Z;

 /// the units of a head: a key, or its bytes when compressed
 static constexpr std::size_t kHead = Z ? sizeof( K ) : 1;

 /// the most units of a key: its bytes or those of the longest difference
 /// when compressed, else 1
 static constexpr std::size_t kWide = Z ? detail::pmaWide< K >() : 1;

 /// the most units of a leaf, \f$ C \f$ for the largest \f$ n \f$
 static constexpr std::size_t kMaxUnits = Z ? 1024 : 128;

 /// the batches with fewer keys go a key at a time
 static constexpr std::size_t kPointBatch = 64;

 /// an empty set
 PackedMemoryArray( void ) = default;

 /// the set of the keys of \f$ S \f$, in any order, a key more than once
 /// counted once
 explicit PackedMemoryArray( std::vector< K > S );

 /// inserts \f$ x \f$ if absent; whether it was
 ///
 /// In pseudocode (Bender, Demaine and Farach-Colton, beyond the book),
 /// \f$ u( \cdot ) \f$ the units of a leaf or of a region,
 /// \f$ \mathrm{leaf}( x ) \f$ the last leaf whose head is at most
 /// \f$ x \f$ (the first if none) and \f$ R_h \f$ the region of height
 /// \f$ h \f$ above it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{then}\ \mathrm{build}( \{ x \} );
 ///     \ \textbf{return true} \cr
 /// 3 & \quad \ell \gets \mathrm{leaf}( x );\ \textbf{if}\ x \in \ell
 ///     \ \textbf{then return false} \cr
 /// 4 & \quad \textbf{if}\ u( \ell \cup \{ x \} ) \le C\ \textbf{then}
 ///     \ \text{write } \ell \cup \{ x \} \text{ in } \ell;\ \textbf{return
 ///     true} \cr
 /// 5 & \quad \textbf{for}\ h \gets 1\ \textbf{to}\ H\ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ u( R_h ) + u( x ) \le \tau_h 2^h C
 ///     \ \textbf{and}\ \mathrm{spread}( R_h , \text{the keys of } R_h
 ///     \cup \{ x \} )\ \textbf{then return true} \cr
 /// 7 & \quad \mathrm{build}( \text{the keys} \cup \{ x \} )\quad
 ///     \triangleright\ \text{at most half full} \cr
 /// 8 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 bool insert( const K & x );

 /// removes \f$ x \f$ if there; whether it was
 bool erase( const K & x );

 /// whether \f$ x \f$ is there
 [[nodiscard]] bool contains( const K & x ) const;

 /// the least key \f$ \ge x \f$, none if there is none
 [[nodiscard]] std::optional< K > lowerBound( const K & x ) const;

 /// calls \f$ f( k ) \f$ on the keys, by increasing key
 template< class F >
 void forEach( F && f ) const;

 /// calls \f$ f( k ) \f$ on the keys with \f$ lo \le k < hi \f$, by
 /// increasing key
 template< class F >
 void forEach( const K & lo , const K & hi , F && f ) const;

 /// calls \f$ f( k ) \f$, while it returns true, on the keys from the least
 /// one \f$ \ge lo \f$ on, by increasing key
 template< class F >
 void scan( const K & lo , F && f ) const;

 /// inserts the keys of the batch \f$ B \f$ (in any order, a key more than
 /// once counted once) on the executor \f$ \mathit{ex} \f$; how many were
 /// absent
 ///
 /// In pseudocode (the batch insertion of the CPMA of Wheatman, Burns,
 /// Buluç and Xu, beyond the book), \f$ T \f$ the leaves touched and
 /// \f$ \mathit{Out} \f$ the leaves that overflow:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertBatch}( \mathit{ex} , B ) \cr
 /// 2 & \quad \text{sort } B \text{ on } \mathit{ex} \text{ and drop the}
 ///     \text{ keys twice};\ k \gets | B | \cr
 /// 3 & \quad \textbf{if}\ k < 64\ \textbf{then}\ \textbf{foreach}\ x \in B
 ///     \ \textbf{do}\ \mathrm{insert}( x ) \cr
 /// 4 & \quad \textbf{else if}\ k \ge n / 8\ \textbf{then}\ \mathrm{build}(
 ///     \text{the keys} \cup B ) \cr
 /// 5 & \quad \textbf{else} \cr
 /// 6 & \qquad \text{cut } B \text{ into blocks, each border moved to the }
 ///     \text{first key of its leaf} \cr
 /// 7 & \qquad \textbf{parallel foreach}\ \text{block}\ \textbf{do}
 ///     \ \textbf{foreach}\ \text{leaf } \ell \text{ of a key of the}
 ///     \text{ block}\ \textbf{do} \cr
 /// 8 & \qquad\quad M \gets \ell \cup ( \text{the keys of } B \text{ in }
 ///     \ell );\ T \gets T \cup \{ \ell \} \cr
 /// 9 & \qquad\quad \textbf{if}\ u( M ) \le C \text{ and } M \text{ has}
 ///     \text{ the head of } \ell\ \textbf{then}\ \text{write } M \text{ in }
 ///     \ell\ \textbf{else}\ \mathit{Out}[ \ell ] \gets M \cr
 /// 10 & \qquad S \gets \mathrm{count}( T )\quad \triangleright\ \text{the}
 ///     \text{ regions to spread, or the root} \cr
 /// 11 & \qquad \textbf{if}\ S \text{ is the root}\ \textbf{then}
 ///     \ \mathrm{build}( \text{the keys} \cup \mathit{Out} ) \cr
 /// 12 & \qquad \textbf{else parallel foreach}\ R \in S\ \textbf{do}
 ///     \ \mathrm{spread}( R , \text{the keys of } R \text{ and of}
 ///     \ \mathit{Out} \text{ in } R )
 /// \end{array}}
 /// \f]
 template< Executor E >
 std::size_t insertBatch( E & ex , std::vector< K > B );

 /// `insertBatch()` on the calling thread
 std::size_t insertBatch( std::vector< K > B );

 /// removes the keys of the batch \f$ B \f$ (in any order) on the
 /// executor \f$ \mathit{ex} \f$, by the three phases of
 /// `insertBatch()` with the lower thresholds; how many were there
 template< Executor E >
 std::size_t eraseBatch( E & ex , std::vector< K > B );

 /// `eraseBatch()` on the calling thread
 std::size_t eraseBatch( std::vector< K > B );

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no key
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// removes every key
 void clear( void );

 /// the number of leaves, \f$ L \f$, 0 if empty
 [[nodiscard]] std::size_t leaves( void ) const { return( f_L ); }

 /// the units of a leaf, \f$ C \f$, 0 if empty
 [[nodiscard]] std::size_t leafUnits( void ) const { return( f_C ); }

 /// the bytes of the arrays
 [[nodiscard]] std::size_t bytes( void ) const;

 /// whether the array is in order: every leaf sorted and with a key, every
 /// key below the head of the next leaf, the counts of the units true
 [[nodiscard]] bool wellFormed( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// a cell of the leaves: a key, or a byte when compressed
 using Cell = std::conditional_t< Z , std::uint8_t , K >;

 /// a leaf kept apart by the merge of a batch, until the redistribution:
 /// its keys and their units
 struct Spill
 {
  std::size_t leaf;      ///< the leaf
  std::vector< K > key;  ///< its keys after the merge
  std::size_t units;     ///< their units
  bool used = false;     ///< whether a redistribution has taken them
  };

 /// what the merge of a block of a batch leaves: the leaves touched, those
 /// kept apart, and the keys inserted or removed
 struct Part
 {
  std::vector< std::size_t > touched;  ///< the leaves touched, increasing
  std::vector< Spill > out;            ///< the leaves apart, increasing
  std::size_t changed = 0;             ///< the keys inserted or removed
  };

 /// a node of the counting: its index in its level and its units
 struct Node
 {
  std::size_t index;  ///< the index of the node in its level
  std::size_t units;  ///< the units of its region
  };

 /// the cells of the tail of a leaf, \f$ C \f$ less the head
 [[nodiscard]] std::size_t tailCells( void ) const { return( f_C - kHead ); }

 /// the first cell of the tail of the leaf \f$ \ell \f$
 [[nodiscard]] Cell * tail( std::size_t l ) {
  return( f_tail.data() + l * tailCells() );
  }
 /// the first cell of the tail of the leaf \f$ \ell \f$
 [[nodiscard]] const Cell * tail( std::size_t l ) const {
  return( f_tail.data() + l * tailCells() );
  }

 /// the place in the order of Eytzinger of the head of the leaf \f$ \ell
 /// \f$, the inverse of `leafAt()`: \f$ L \f$ for the leaf 0, else
 /// \f$ 2^{H - 1 - t} + \lfloor \ell / 2^{t + 1} \rfloor \f$, \f$ t \f$
 /// the trailing zeros of \f$ \ell \f$
 [[nodiscard]] std::size_t pos( std::size_t l ) const {
  if( l == 0 )
   return( f_L );
  const int t = std::countr_zero( l );
  return( ( std::size_t( 1 ) << ( f_H - 1 - std::size_t( t ) ) ) +
	   ( l >> ( t + 1 ) ) );
  }

 /// the leaf of the head at the place \f$ j \in [ 1 , L ] \f$ of the order
 /// of Eytzinger, its rank in the visit in order of the implicit tree of
 /// the \f$ L = 2^H \f$ places: 0 for the place \f$ L \f$, the leftmost,
 /// else \f$ ( 2 ( j - 2^d ) + 1 ) 2^{H - 1 - d} \f$, \f$ d \f$ the depth
 /// of \f$ j \f$
 [[nodiscard]] std::size_t leafAt( std::size_t j ) const {
  if( j == f_L )
   return( 0 );
  const std::size_t d = std::size_t( std::bit_width( j ) ) - 1;
  return( ( 2 * ( j - ( std::size_t( 1 ) << d ) ) + 1 ) << ( f_H - 1 - d ) );
  }

 /// the head of the leaf \f$ \ell \f$
 [[nodiscard]] const K & head( std::size_t l ) const {
  return( f_head[ pos( l ) ] );
  }

 /// the units of a run of the keys \f$ x \f$, sorted, as a leaf
 [[nodiscard]] static std::size_t units( const K * x , std::size_t c );

 /// the units that \f$ y \f$ takes after \f$ x < y \f$ in a leaf
 [[nodiscard]] static std::size_t step( const K & x , const K & y );

 /// the last leaf whose head is at most \f$ x \f$, the first if none, by the
 /// heads in the order of Eytzinger
 ///
 /// In pseudocode (the search of the order of Eytzinger of Khuong and
 /// Morin, beyond the book), \f$ E[ 1 .. L ] \f$ the heads in the heap
 /// order and \f$ \mathrm{leaf}( j ) \f$ the leaf of the head \f$ E[ j ]
 /// \f$, computed from \f$ j \f$ by `leafAt()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{leafOf}( x ) \cr
 /// 2 & \quad j \gets 1 \cr
 /// 3 & \quad \textbf{while}\ j \le L\ \textbf{do}\ j \gets 2 j + [ E[ j ]
 ///     \le x ]\quad \triangleright\ \text{a conditional move} \cr
 /// 4 & \quad j \gets \lfloor j / 2^{t + 1} \rfloor \text{, } t \text{ the}
 ///     \text{ trailing ones of } j\quad \triangleright\ \text{the first}
 ///     \text{ head} > x \text{, or } 0 \cr
 /// 5 & \quad \textbf{return}\ \max( 0 , ( j = 0\ ?\ L : \mathrm{leaf}( j )
 ///     ) - 1 )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t leafOf( const K & x ) const;

 /// the keys of the leaf \f$ \ell \f$ written from \f$ \mathit{out} \f$,
 /// and their number
 std::size_t decode( std::size_t l , K * out ) const;

 /// the keys of the leaf \f$ \ell \f$ appended to \f$ \mathit{out} \f$
 void decodeTo( std::size_t l , std::vector< K > & out ) const;

 /// the number of keys of the leaf \f$ \ell \f$
 [[nodiscard]] std::size_t count( std::size_t l ) const;

 /// writes the keys \f$ x \f$ (sorted, at least one, at most \f$ C \f$
 /// units) in the leaf \f$ \ell \f$ and its units, and its head if
 /// \f$ h \f$ (else the head is \f$ x_0 \f$ already)
 void encode( std::size_t l , const K * x , std::size_t c , bool h );

 /// the upper threshold of a region of height \f$ h \f$, in units
 [[nodiscard]] std::size_t upper( std::size_t h ) const;

 /// the lower threshold of a region of height \f$ h \f$, in units
 [[nodiscard]] std::size_t lower( std::size_t h ) const;

 /// the units of the leaves \f$ [ a , a + r ) \f$
 [[nodiscard]] std::size_t unitsOf( std::size_t a , std::size_t r ) const;

 /// spreads the keys \f$ x \f$ (sorted) evenly over the leaves
 /// \f$ [ a , a + r ) \f$ on \f$ \mathit{ex} \f$; false, with nothing
 /// written, if a leaf would not hold its share or would have no key
 ///
 /// In pseudocode (Bender, Demaine and Farach-Colton, beyond the book),
 /// \f$ c_0 = u( \text{a head} ) \f$, \f$ c_i \f$ the units of \f$ x_i \f$
 /// after \f$ x_{i-1} \f$, \f$ P_j = c_0 + \cdots + c_{j-1} \f$ and
 /// \f$ U = P_m \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{spread}( a , r , x_0 \cdots x_{m-1} ) \cr
 /// 2 & \quad \textbf{if}\ m < r\ \textbf{then return false}\quad
 ///     \triangleright\ \text{a leaf would be empty} \cr
 /// 3 & \quad b_0 \gets 0;\ b_r \gets m \cr
 /// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ r - 1\ \textbf{do}
 ///     \ b_i \gets \text{the least } j \text{ with } P_j \ge \lfloor i U / r
 ///     \rfloor\text{, within } [ b_{i-1} + 1 , m - r + i ] \cr
 /// 5 & \quad \textbf{if}\ \text{some } x_{b_i} \cdots x_{b_{i+1}-1}
 ///     \text{ takes more than } C \text{ units}\ \textbf{then return false}
 ///     \cr
 /// 6 & \quad \textbf{parallel for}\ i \gets 0\ \textbf{to}\ r - 1
 ///     \ \textbf{do}\ \text{write } x_{b_i} \cdots x_{b_{i+1}-1} \text{ in
 ///     the leaf } a + i \cr
 /// 7 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 template< Executor E >
 bool spread( E & ex , std::size_t a , std::size_t r ,
	      const std::vector< K > & x );

 /// the leaf \f$ \ell \f$ in \f$ \mathit{Out} \f$ (sorted by leaf), if
 /// there and not yet spread, else `nullptr`
 [[nodiscard]] static const Spill * spillOf( const std::vector< Spill > & Out ,
					     std::size_t l );

 /// the keys of the leaves \f$ [ a , a + r ) \f$, those of a leaf of
 /// \f$ \mathit{Out} \f$ not yet spread taken from it, on \f$ \mathit{ex} \f$
 template< Executor E >
 void gather( E & ex , std::size_t a , std::size_t r ,
	      const std::vector< Spill > & Out , std::vector< K > & x ) const;

 /// builds the array again on the keys \f$ x \f$ (sorted, distinct), with
 /// the fewest leaves that make it at most half full
 template< Executor E >
 void build( E & ex , std::vector< K > && x );

 /// the least region above the leaf \f$ \ell \f$ that respects its
 /// threshold with the key \f$ x \f$ inserted (if \f$ \mathit{in} \f$) or
 /// removed, \f$ \ell \f$ then of \f$ \mathit{lu} \f$ units, spread with
 /// it; false if none does, with nothing written
 bool climb( std::size_t l , const K & x , bool in , std::size_t lu );

 /// the union (if \f$ \mathit{in} \f$) or the difference of the sorted
 /// \f$ A \f$ and \f$ B \f$, by blocks of \f$ A \f$ in parallel
 template< Executor E >
 [[nodiscard]] static std::vector< K >
 setOp( E & ex , const std::vector< K > & A , const std::vector< K > & B ,
	bool in );

 /// the merge of the keys \f$ B[ j .. j_1 - 1 ] \f$ of a batch into their
 /// leaves, which no other block touches: a leaf that fits with the same
 /// head written in place, the others in `part.out`
 void mergeLeaves( const std::vector< K > & B , std::size_t j ,
		   std::size_t j1 ,
		   bool in , Part & part );

 /// the merge of the sorted batch \f$ B \f$ into the leaves, the counting
 /// and the redistribution, the body of `insertBatch()` and of
 /// `eraseBatch()`
 template< Executor E >
 std::size_t batch( E & ex , std::vector< K > & B , bool in );

 std::size_t f_n = 0;  ///< the number of keys
 std::size_t f_L = 0;  ///< the number of leaves, a power of 2
 std::size_t f_H = 0;  ///< the height of the tree, \f$ \log_2 L \f$
 std::size_t f_C = 0;  ///< the units of a leaf

 /// the heads in the order of Eytzinger, from 1, the head of the leaf
 /// \f$ \ell \f$ at the place `pos( l )`
 std::vector< K > f_head;
 /// the units of each leaf, the head included
 std::vector< std::uint32_t > f_units;
 /// the tails of the leaves, \f$ C \f$ less the head cells each
 std::vector< Cell > f_tail;

 };  // end( class PackedMemoryArray )

/// the compressed packed memory array of the unsigned integers `K`, the
/// CPMA of Wheatman, Burns, Buluç and Xu
template< std::unsigned_integral K >
using CompressedPackedMemoryArray = PackedMemoryArray< K , true >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/PackedMemoryArray.tpp"

/*--------------------------------------------------------------------------*/

#endif /* PackedMemoryArray.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File PackedMemoryArray.h ----------------------*/
/*--------------------------------------------------------------------------*/
