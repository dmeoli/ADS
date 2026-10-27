/*--------------------------------------------------------------------------*/
/*-------------------------- File RoaringBitmap.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressed bitmap of Chambi, Lemire, Kaser and Godin (Roaring),
 * beyond the book: a SET of integers of 32 bits [see `ADT.h`], the
 * engineered counterpart of the boolean vector of section 8.1 [see
 * `BoolSet.h`] for universes too large for a bit per integer and sets too
 * dense for a sorted list [see `VectorIntersection.h`].
 *
 * The universe is cut into chunks of \f$ 2^{16} \f$ integers by their top
 * 16 bits, and each chunk that holds some item is a container of the low
 * 16 bits of its items, kept in a vector sorted by the top bits:
 *
 * - an array container, the sorted array of the low bits, when the chunk
 *   holds at most 4096 items (2 bytes per item, at most 8 KB);
 * - a bitmap container, the \f$ 2^{16} \f$ bits of the chunk in 1024 words
 *   (8 KB), when it holds more.
 *
 * So no container takes more than 8 KB, and a set of \f$ n \f$ items takes
 * at most about \f$ 2 n \f$ bytes and at most the bits of its universe,
 * whichever is less. A search, an insertion and a removal find the
 * container by a binary search with no branch on the chunks, kept apart in
 * an array of their own, \f$ O( \log c ) \f$ for \f$ c \f$ containers, and
 * then search an array in the same way (or test a bit), and shift it (at
 * most 4096 items, a few lines of cache) or set a bit; an array that passes
 * 4096 items becomes a bitmap and a bitmap that falls to 4096 an array. The
 * operations of two sets merge the vectors of the containers by their
 * chunks and combine the containers of the same chunk:
 *
 * - two bitmaps word by word, the bits of the result counted in the same
 *   pass (4 words at a time with AVX2 [see `BoolSet.h`]), a result of at
 *   most 4096 items then written as an array;
 * - a bitmap and an array by testing, setting, clearing or flipping the
 *   bits of the array in a copy of the bitmap;
 * - two arrays, with SSE4.2, by blocks of 8 items: for the intersection
 *   and the difference, a block of each array compared all against all by
 *   one comparison of strings (the method of Schlegel, Willhalm and
 *   Lehner), the items of the block of the first array found, or those not
 *   found in any block of the second one, packed by a shuffle and stored,
 *   and the block that ends first moving on; for the union, the two blocks
 *   merged by the network of minima and maxima of Inoue and Taura, the 8
 *   smallest written but those equal to the one before them, and the next
 *   block taken from the array with the smaller head; for the symmetric
 *   difference, the union of the two differences (by a merge with no branch
 *   on short arrays); a union or a symmetric difference of more than 4096
 *   items becoming a bitmap.
 *
 * Compared with the literature: Roaring adds containers of runs (the
 * intervals of consecutive integers, for sets with long runs) and the lazy
 * unions of many bitmaps whose counts are computed at the end; the bitmaps
 * of the other families (WAH, Concise, EWAH) compress runs of words and are
 * slower to combine.
 *
 * `rank( x )`, the items smaller than \f$ x \f$, sums the counts of the
 * containers before that of \f$ x \f$ and adds the place of \f$ x \f$ in it,
 * \f$ O( c + 4096 ) \f$; `select( i )` finds the container of the rank
 * \f$ i \f$ by the same counts and the item in it; `lowerBound( x )` starts
 * an iterator at the first item not smaller than \f$ x \f$.
 *
 * The iterators visit the items in increasing order; an insertion or a
 * removal invalidates them. The contracts [see `Contract.h`]: after
 * `insert()` the item is there and after `remove()` it is not; the
 * constructor from an array needs strictly increasing items, an audit; the
 * invariant is that the containers are sorted by their chunks, not empty,
 * arrays of at most 4096 sorted items and bitmaps of more, with their counts
 * right, an \f$ O( n ) \f$ check at the level 3 on the sets of up to 4096
 * items. The operations are defined in `RoaringBitmap.tpp`.
 *
 * \see S. Chambi, D. Lemire, O. Kaser, R. Godin, Better bitmap performance
 *      with Roaring bitmaps, Software: Practice and Experience 46(5),
 *      709-719, 2016, doi:10.1002/spe.2325
 * \see D. Lemire, O. Kaser, N. Kurz, L. Deri, C. O'Hara, F. Saint-Jacques,
 *      G. Ssi-Yan-Kai, Roaring bitmaps: implementation of an optimized
 *      software library, Software: Practice and Experience 48(4), 867-895,
 *      2018, doi:10.1002/spe.2560
 * \see B. Schlegel, T. Willhalm, W. Lehner, Fast sorted-set intersection
 *      using SIMD instructions, Proceedings of ADMS 2011, 1-8, 2011
 * \see H. Inoue, K. Taura, SIMD- and cache-friendly algorithm for sorting
 *      an array of structures, Proceedings of the VLDB Endowment 8(11),
 *      1274-1285, 2015, doi:10.14778/2809974.2809988
 * \see W. Muła, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046
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

#ifndef __ADS_RoaringBitmap
 #define __ADS_RoaringBitmap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/set/BoolSet.h"

#include <cstddef>
#include <bit>
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS RoaringBitmap ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of integers of 32 bits as a Roaring bitmap
class RoaringBitmap {

/*--------------------------------------------------------------------------*/
 public:

 using Item = std::uint32_t;   ///< the type of the items

 /// the items of an array container at most
 static constexpr std::uint32_t ARRAY = 4096;

 /// the empty set
 RoaringBitmap( void ) = default;

 /// a copy, with containers of its own
 RoaringBitmap( const RoaringBitmap & o );

 /// the containers of `o`, which is left empty
 RoaringBitmap( RoaringBitmap && o ) noexcept = default;

 /// the containers of `o`, a copy or a move (the copy and swap idiom)
 RoaringBitmap & operator=( RoaringBitmap o ) noexcept;

 /// the set of the \f$ n \f$ strictly increasing items of
 /// \f$ a[ 0 .. n ) \f$, a container per chunk in one scan
 /// \pre \f$ a \f$ strictly increasing (an audit)
 RoaringBitmap( const std::uint32_t * a , std::size_t n );

 /// whether \f$ x \f$ is in the set
 ///
 /// In pseudocode (Chambi et al.), \f$ K \f$ the sorted chunks of the
 /// containers:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{contains}( x ) \cr
 /// 2 & \quad i \gets \text{the first place of } K \text{ not smaller than }
 ///     \lfloor x / 2^{16} \rfloor \text{, by a binary search} \cr
 /// 3 & \quad \textbf{if}\ \text{there is none, or } K[ i ] \ne \lfloor x /
 ///     2^{16} \rfloor\ \textbf{then return false} \cr
 /// 4 & \quad v \gets x \bmod 2^{16} \cr
 /// 5 & \quad \textbf{if}\ \text{the container } i \text{ is a bitmap }
 ///     \ \textbf{then return}\ \text{its bit } v \cr
 /// 6 & \quad \textbf{return}\ v \text{ is in its array, by a binary search}
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool contains( std::uint32_t x ) const;

 /// \f$ x \f$ into the set; true if it was not there
 ///
 /// In pseudocode (Chambi et al.):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad i \gets \text{the container of } \lfloor x / 2^{16} \rfloor
 ///     \text{, as in contains} \cr
 /// 3 & \quad \textbf{if}\ \text{there is none }\ \textbf{then}
 ///     \ \text{insert at } i \text{ the array } ( x \bmod 2^{16} );
 ///     \ \textbf{return true} \cr
 /// 4 & \quad \text{set the bit, or insert into the array, } x \bmod 2^{16}
 ///     \cr
 /// 5 & \quad \textbf{if}\ \text{the array passed 4096 items }
 ///     \ \textbf{then}\ \text{turn it into a bitmap} \cr
 /// 6 & \quad \textbf{return}\ \text{whether } x \text{ was added}
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is in the set
 bool insert( std::uint32_t x );

 /// \f$ x \f$ out of the set; true if it was there
 ///
 /// In pseudocode (Chambi et al.), the converse of `insert()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad i \gets \text{the container of } \lfloor x / 2^{16} \rfloor
 ///     ;\ \textbf{if}\ \text{there is none }\ \textbf{then return false} \cr
 /// 3 & \quad \text{clear the bit, or remove from the array, } x \bmod 2^{16}
 ///     \cr
 /// 4 & \quad \textbf{if}\ \text{the container is empty }\ \textbf{then}
 ///     \ \text{remove it} \cr
 /// 5 & \quad \textbf{else if}\ \text{a bitmap fell to 4096 items }
 ///     \ \textbf{then}\ \text{turn it into an array} \cr
 /// 6 & \quad \textbf{return}\ \text{whether } x \text{ was there}
 /// \end{array}}
 /// \f]
 /// \post \f$ x \f$ is not in the set
 bool remove( std::uint32_t x );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the set has no item
 [[nodiscard]] bool empty( void ) const;

 /// all the items removed
 void clear( void );

 /// the items of this set and of `o` exchanged, \f$ O( 1 ) \f$
 void swap( RoaringBitmap & o ) noexcept;

 /// the containers \f$ c \f$
 [[nodiscard]] std::size_t containers( void ) const;

 /// the containers that are bitmaps
 [[nodiscard]] std::size_t bitmaps( void ) const;

 /// the bytes of the containers
 [[nodiscard]] std::size_t bytes( void ) const;

 /// `f( x )` on the items in increasing order
 template< class F >
 void forEach( F && f ) const;

 /// the items in increasing order
 [[nodiscard]] std::vector< std::uint32_t > items( void ) const;

 /// the items smaller than \f$ x \f$: the counts of the containers before
 /// that of \f$ x \f$, \f$ O( c ) \f$, and the place of \f$ x \f$ in it
 ///
 /// In pseudocode (Chambi et al.), \f$ | X_j | \f$ the items of the
 /// container \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank}( x ) \cr
 /// 2 & \quad i \gets \text{the first container whose chunk is not smaller }
 ///     \text{than } \lfloor x / 2^{16} \rfloor \cr
 /// 3 & \quad \rho \gets \sum_{j < i} | X_j | \cr
 /// 4 & \quad \textbf{if}\ \text{the chunk of } i \text{ is }
 ///     \lfloor x / 2^{16} \rfloor\ \textbf{then}\ \rho \gets \rho +
 ///     \text{the items of } X_i \text{ below } x \bmod 2^{16} \cr
 /// 5 & \quad \textbf{return}\ \rho
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t rank( std::uint32_t x ) const;

 friend RoaringBitmap setUnion( const RoaringBitmap & A ,
                                const RoaringBitmap & B );
 friend RoaringBitmap setIntersection( const RoaringBitmap & A ,
                                       const RoaringBitmap & B );
 friend RoaringBitmap setDifference( const RoaringBitmap & A ,
                                     const RoaringBitmap & B );
 friend RoaringBitmap setSymmetricDifference( const RoaringBitmap & A ,
                                              const RoaringBitmap & B );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t WORDS = 1024;   ///< the words of a bitmap

 /// a container: the low bits of its items, sorted, or its bitmap
 struct Block {
  std::uint16_t key = 0;                 ///< the top bits of the chunk
  std::uint32_t card = 0;                ///< the items
  std::vector< std::uint16_t , detail::UninitAllocator< std::uint16_t > > a;
                                         ///< the array, if not a bitmap
  std::unique_ptr< std::uint64_t[] > w;  ///< the bitmap, if one

  /// whether the container is a bitmap
  [[nodiscard]] bool bits( void ) const { return( w != nullptr ); }
  };

 /// the operation of two sets
 enum class Op {
  AND ,      ///< the intersection
  OR ,       ///< the union
  ANDNOT ,   ///< the difference
  XOR        ///< the symmetric difference
  };

 /// the operation `O` of two sets, by the merge of their containers
 template< Op O >
 static RoaringBitmap combine( const RoaringBitmap & A ,
                               const RoaringBitmap & B );

 /// the operation `O` of two containers of the same chunk
 template< Op O >
 static Block combine( const Block & a , const Block & b );

 /// a copy of the container \f$ b \f$
 static Block copy( const Block & b );

 /// the array of \f$ b \f$ turned into a bitmap
 static void toBitmap( Block & b );

 /// the bitmap of \f$ b \f$ turned into an array
 static void toArray( Block & b );

 /// the container of the chunk `key` with the words \f$ w \f$ of `card`
 /// items, an array if they are at most 4096
 static Block fromWords( std::uint16_t key ,
                         std::unique_ptr< std::uint64_t[] > w ,
                         std::uint32_t card );

 /// the first container whose chunk is not smaller than `key`, by a
 /// binary search with no branch
 [[nodiscard]] std::size_t find( std::uint16_t key ) const;

 /// the invariant of the containers, \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;

 /// the chunks of the containers, which a search reads
 void rekey( void );

 std::vector< Block > f_B;              ///< the containers by their chunks
 std::vector< std::uint16_t > f_K;      ///< their chunks, apart
 std::size_t f_size = 0;                ///< the items

/*------------------------- THE STANDARD INTERFACE -------------------------*/
 public:

 // a constant view of the items in increasing order, forward

 /// the iterator on the items in increasing order, forward
 class const_iterator {
  public:
  using value_type = std::uint32_t;                  ///< the items
  using reference = std::uint32_t;                   ///< an item, by value
  using difference_type = std::ptrdiff_t;            ///< the distances
  /// forward for the concepts of C++20
  using iterator_concept = std::forward_iterator_tag;
  /// input for the older algorithms, since `*it` is a value
  using iterator_category = std::input_iterator_tag;

  /// an iterator on no set
  const_iterator( void ) = default;

  /// the item
  reference operator*( void ) const { return( f_x ); }

  /// to the next item: the next bit of the word of a bitmap, or the next
  /// place, word or container
  const_iterator & operator++( void ) {
   if( f_word ) {                           // the next bit of the word
    f_word &= f_word - 1;
    if( f_word ) {
     f_x = f_base + std::uint32_t( std::countr_zero( f_word ) );
     return( *this );
     }
    }
   ++f_i;
   settle();
   return( *this );
   }
  /// to the next item, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same item of the same container
  bool operator==( const const_iterator & o ) const {
   return( ( f_b == o.f_b ) && ( f_i == o.f_i ) && ( f_x == o.f_x ) );
   }

  private:
  friend class RoaringBitmap;

  /// on the first item of the container \f$ b \f$ of `s`, at the end if
  /// there is none
  const_iterator( const RoaringBitmap * s , std::size_t b );

  /// on the first item from the place, the word or the container reached,
  /// moving to the next container while there is none
  void settle( void );

  const RoaringBitmap * f_s = nullptr;  ///< the set
  std::size_t f_b = 0;                  ///< the container
  std::size_t f_i = 0;                  ///< the place, or the word
  std::uint64_t f_word = 0;             ///< the bits left of the word
  std::uint32_t f_base = 0;             ///< the item of its bit 0
  std::uint32_t f_x = 0;                ///< the item

 };  // end( class const_iterator )

 using value_type = std::uint32_t;          ///< the items
 using size_type = std::size_t;             ///< the sizes
 using difference_type = std::ptrdiff_t;    ///< the distances
 using reference = std::uint32_t;           ///< an item, by value
 using const_reference = std::uint32_t;     ///< an item, by value
 using iterator = const_iterator;           ///< the same, constant

 /// the iterator on the smallest item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the largest item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_B.size() ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the first item not smaller than \f$ x \f$, `end()` if none
 ///
 /// In pseudocode (Chambi et al.):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( x ) \cr
 /// 2 & \quad i \gets \text{the first container whose chunk is not smaller }
 ///     \text{than } \lfloor x / 2^{16} \rfloor \cr
 /// 3 & \quad \textbf{if}\ \text{the chunk of } i \text{ is } \lfloor x /
 ///     2^{16} \rfloor\ \textbf{then}\ \text{start at the first item of } i
 ///     \text{ not smaller than } x \cr
 /// 4 & \quad \textbf{else}\ \text{start at the first item of } i \cr
 /// 5 & \quad \textbf{return}\ \text{the iterator there, moved to the next }
 ///     \text{container while there is no item}
 /// \end{array}}
 /// \f]
 [[nodiscard]] const_iterator lowerBound( std::uint32_t x ) const;

 /// the item of rank \f$ i \f$ (from 0), `end()` if
 /// \f$ i \ge \f$ `size()`
 ///
 /// In pseudocode (Chambi et al.), \f$ | X_j | \f$ the items of the
 /// container \f$ j \f$ and \f$ W_k \f$ the words of a bitmap:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( i ) \cr
 /// 2 & \quad j \gets 0;\ \textbf{while}\ i \ge | X_j |\ \textbf{do}\ i \gets
 ///     i - | X_j |;\ j \gets j + 1 \cr
 /// 3 & \quad \textbf{if}\ X_j \text{ is an array }\ \textbf{then}\ v \gets
 ///     X_j[ i ] \cr
 /// 4 & \quad \textbf{else} \cr
 /// 5 & \qquad k \gets 0;\ \textbf{while}\ i \ge \mathrm{popcount}( W_k )
 ///     \ \textbf{do}\ i \gets i - \mathrm{popcount}( W_k );\ k \gets k + 1
 ///     \cr
 /// 6 & \qquad v \gets 64 k + \text{the place of the bit set of rank } i
 ///     \text{ of } W_k \cr
 /// 7 & \quad \textbf{return}\ \mathrm{lowerBound}( 2^{16} \cdot
 ///     \text{the chunk of } X_j + v )
 /// \end{array}}
 /// \f]
 [[nodiscard]] const_iterator select( std::size_t i ) const;

 };  // end( class RoaringBitmap )

/// the union, by the merge of the containers
///
/// In pseudocode (Chambi et al.), the containers merged by their chunks:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setUnion}( A , B ) \cr
/// 2 & \quad C \gets \text{an empty set} \cr
/// 3 & \quad \textbf{foreach}\ \text{chunk } k \text{ of } A \text{ or } B
///     \text{, in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{both have } k\ \textbf{then}\ X \gets
///     \text{the union of their two containers} \cr
/// 5 & \qquad \textbf{else}\ X \gets \text{a copy of the container of the }
///     \text{one that has it} \cr
/// 6 & \qquad \textbf{if}\ X \text{ is not empty }\ \textbf{then}
///     \ \text{append } X \text{ to } C \cr
/// 7 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
RoaringBitmap setUnion( const RoaringBitmap & A , const RoaringBitmap & B );

/// the intersection, by the merge of the containers
///
/// In pseudocode (Chambi et al.), the containers merged by their chunks:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setIntersection}( A , B ) \cr
/// 2 & \quad C \gets \text{an empty set} \cr
/// 3 & \quad \textbf{foreach}\ \text{chunk } k \text{ of } A \text{ or } B
///     \text{, in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{both have } k\ \textbf{then}\ X \gets
///     \text{the intersection of their two containers} \cr
/// 5 & \qquad \textbf{else}\ X \gets \text{nothing} \cr
/// 6 & \qquad \textbf{if}\ X \text{ is not empty }\ \textbf{then}
///     \ \text{append } X \text{ to } C \cr
/// 7 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
RoaringBitmap setIntersection( const RoaringBitmap & A ,
                               const RoaringBitmap & B );

/// the difference, by the merge of the containers
///
/// In pseudocode (Chambi et al.), the containers merged by their chunks:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setDifference}( A , B ) \cr
/// 2 & \quad C \gets \text{an empty set} \cr
/// 3 & \quad \textbf{foreach}\ \text{chunk } k \text{ of } A \text{ or } B
///     \text{, in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{both have } k\ \textbf{then}\ X \gets
///     \text{the difference of their two containers} \cr
/// 5 & \qquad \textbf{else}\ X \gets \text{a copy of the container of } A
///     \text{, if it has it} \cr
/// 6 & \qquad \textbf{if}\ X \text{ is not empty }\ \textbf{then}
///     \ \text{append } X \text{ to } C \cr
/// 7 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
RoaringBitmap setDifference( const RoaringBitmap & A ,
                             const RoaringBitmap & B );

/// the symmetric difference, by the merge of the containers
///
/// In pseudocode (Chambi et al.), the containers merged by their chunks:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setSymmetricDifference}( A , B ) \cr
/// 2 & \quad C \gets \text{an empty set} \cr
/// 3 & \quad \textbf{foreach}\ \text{chunk } k \text{ of } A \text{ or } B
///     \text{, in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{both have } k\ \textbf{then}\ X \gets
///     \text{the symmetric difference of their two containers} \cr
/// 5 & \qquad \textbf{else}\ X \gets \text{a copy of the container of the }
///     \text{one that has it} \cr
/// 6 & \qquad \textbf{if}\ X \text{ is not empty }\ \textbf{then}
///     \ \text{append } X \text{ to } C \cr
/// 7 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
RoaringBitmap setSymmetricDifference( const RoaringBitmap & A ,
                                      const RoaringBitmap & B );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/RoaringBitmap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RoaringBitmap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RoaringBitmap.h ------------------------*/
/*--------------------------------------------------------------------------*/
