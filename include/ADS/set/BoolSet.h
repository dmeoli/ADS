/*--------------------------------------------------------------------------*/
/*----------------------------- File BoolSet.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The SET of the integers in \f$ [ 0 , N ) \f$ as a boolean vector of
 * section 8.1: `contains()`, `insert()` and `remove()` in \f$ O( 1 ) \f$,
 * `min()`, `max()` and `forEach()` in \f$ O( N ) \f$, union, intersection,
 * difference and symmetric difference in \f$ \Theta( N ) \f$ (the first is
 * called `setUnion()`, `union` being a keyword of C++, and so the others are
 * `setIntersection()`, `setDifference()` and `setSymmetricDifference()`).
 *
 * The vector is kept as words of 64 bits, and the operations of two sets
 * and the scans of `min()`, `max()` and `forEach()` go a word at a time:
 * the same loops on the \f$ N \f$ booleans, 64 of them in one operation of
 * the processor, which leaves the bounds unchanged and divides the time by
 * up to 64 (\f$ \lceil N / 64 \rceil \f$ operations on words for a set of
 * dimension \f$ N \f$); the size of a result is the count of its bits, in
 * the same pass as the operation, with AVX2 by the method of Muła, Kurz and
 * Lemire (a shuffle of bytes looks up the bits of each half byte, and a sum
 * of absolute differences adds the bytes), 4 words at a time, and the words
 * of a result are written once, not filled with zeros before.
 *
 * The contracts [see `Contract.h`]: \f$ N \ge 0 \f$, an item inserted is in
 * \f$ [ 0 , N ) \f$, and each operation of two sets gives, item by item,
 * what its name says; the invariant is that the size counts the bits set.
 * The operations are defined in `BoolSet.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 8.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
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

#ifndef __ADS_BoolSet
 #define __ADS_BoolSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#if defined( __AVX2__ )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// an allocator whose construction with no value leaves the item as it is
/// (a default-initialization), so that a vector grows with no fill
template< class T >
struct UninitAllocator : std::allocator< T > {
 template< class U > struct rebind { using other = UninitAllocator< U >; };
 UninitAllocator( void ) = default;
 template< class U >
 UninitAllocator( const UninitAllocator< U > & ) noexcept {}
 template< class U >
 void construct( U * p ) noexcept( std::is_nothrow_default_constructible_v<
                                    U > ) {
  ::new( static_cast< void * >( p ) ) U;
  }
 template< class U , class... Args >
 void construct( U * p , Args &&... args ) {
  ::new( static_cast< void * >( p ) ) U( std::forward< Args >( args )... );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BoolSet ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of integers in \f$ [ 0 , N ) \f$ as a boolean vector
class BoolSet {

/*--------------------------------------------------------------------------*/
 public:

 using Item = long;   ///< the type of the items

 /// the empty set of the integers in \f$ [ 0 , N ) \f$, the `Set( N )` of
 /// the book
 /// \pre \f$ N \ge 0 \f$
 explicit BoolSet( long N );

 /// the number of items, kept by the operations, \f$ O( 1 ) \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the set has no item
 [[nodiscard]] bool empty( void ) const;

 /// the size \f$ N \f$ of the vector
 [[nodiscard]] long dim( void ) const;

 /// whether \f$ x \f$ is in the set, false also for an \f$ x \f$ out of
 /// \f$ [ 0 , N ) \f$, by reading its bit
 [[nodiscard]] bool contains( long x ) const;

 /// \f$ x \f$ into the set, by setting its bit
 /// \pre \f$ x \in [ 0 , N ) \f$
 /// \post \f$ x \f$ is in the set
 void insert( long x );

 /// \f$ x \f$ out of the set, by clearing its bit
 /// \post \f$ x \f$ is not in the set
 void remove( long x );

 /// the smallest item, none if the set is empty, \f$ O( N ) \f$ time
 ///
 /// In pseudocode (section 8.1), a word \f$ W[ k ] \f$ of 64 booleans at a
 /// time, \f$ \mathrm{ctz} \f$ the number of its trailing zeros:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{min}() \cr
 /// 2 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ \lceil N / 64 \rceil - 1
 ///     \ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ W[ k ] \ne 0\ \textbf{then return}\ 64 k +
 ///     \mathrm{ctz}( W[ k ] ) \cr
 /// 4 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< long > min( void ) const;

 /// the largest item, none if the set is empty, \f$ O( N ) \f$ time
 ///
 /// In pseudocode (section 8.1), the scan of `min()` from the last word,
 /// \f$ \mathrm{clz} \f$ the number of the leading zeros:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{max}() \cr
 /// 2 & \quad \textbf{for}\ k \gets \lceil N / 64 \rceil - 1\ \textbf{downto}
 ///     0\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ W[ k ] \ne 0\ \textbf{then return}\ 64 k + 63 -
 ///     \mathrm{clz}( W[ k ] ) \cr
 /// 4 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< long > max( void ) const;

 /// `f( x )` on the items in increasing order, \f$ O( N ) \f$ time
 ///
 /// In pseudocode (section 8.1), the bits set of each word taken from the
 /// lowest, each step clearing the lowest bit set:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{forEach}( f ) \cr
 /// 2 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ \lceil N / 64 \rceil - 1
 ///     \ \textbf{do} \cr
 /// 3 & \qquad w \gets W[ k ] \cr
 /// 4 & \qquad \textbf{while}\ w \ne 0\ \textbf{do}\ f( 64 k +
 ///     \mathrm{ctz}( w ) );\ w \gets w \wedge ( w - 1 )
 /// \end{array}}
 /// \f]
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in increasing order, bidirectional; an
 // item is computed from its bit

 /// the places of the table, for `SlotIterator`
 struct Places {
  /// the places, \f$ N \f$
  static std::size_t size( const auto & o ) { return( o.f_N ); }
  /// whether the bit \f$ i \f$ is set
  static bool occupied( const auto & o , std::size_t i ) {
   return( ( o.f_W[ i / 64 ] >> ( i % 64 ) ) & 1 );
   }
  /// the item of the place \f$ i \f$, \f$ i \f$ itself
  static auto read( const auto & , std::size_t i ) {
   return( long( i ) );
   }
  };

 using value_type = long;                 ///< the items
 using size_type = std::size_t;           ///< the sizes
 using difference_type = std::ptrdiff_t;  ///< the distances
 using reference = long;                  ///< an item, by value
 using const_reference = long;            ///< an item, by value
 /// the iterator, on the bits set in increasing order
 using const_iterator = SlotIterator< BoolSet , Places >;
 using iterator = const_iterator;         ///< the same, constant

 /// the iterator on the smallest item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the largest item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the size is the number of bits set, \f$ O( N ) \f$
 [[nodiscard]] bool counted( void ) const;

 /// the bits of `C` from those of `A` and `B` by `op( a , b )` on the words, a
 /// missing word being 0, the bits past the dimension of C cleared and
 /// counted
 template< class Op >
 static void combine( BoolSet & C , const BoolSet & A , const BoolSet & B ,
                      Op op );

 friend BoolSet setUnion( const BoolSet & A , const BoolSet & B );
 friend BoolSet setIntersection( const BoolSet & A , const BoolSet & B );
 friend BoolSet setDifference( const BoolSet & A , const BoolSet & B );
 friend BoolSet setSymmetricDifference( const BoolSet & A ,
                                        const BoolSet & B );

 /// a set of dimension \f$ N \f$ whose words are not filled
 struct Raw {};
 BoolSet( long N , Raw );

 /// the bits, 64 per word
 std::vector< std::uint64_t , detail::UninitAllocator< std::uint64_t > > f_W;
 std::size_t f_N = 0;                ///< the dimension
 std::size_t f_size = 0;             ///< the number of items

 };  // end( class BoolSet )

/// the union, a set of dimension
/// \f$ \max( \mathrm{dim}( A ) , \mathrm{dim}( B ) ) \f$
///
/// In pseudocode (section 8.1), a word of 64 booleans at a time,
/// a word missing in the shorter vector being 0:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setUnion}( A , B ) \cr
/// 2 & \quad C \gets \text{a set of dimension } \max( \mathrm{dim}( A ) ,
///     \mathrm{dim}( B ) ) \cr
/// 3 & \quad \textbf{foreach}\ \text{word } k \text{ of } C\ \textbf{do}
///     \ C.W[ k ] \gets A.W[ k ] \vee B.W[ k ] \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
inline BoolSet setUnion( const BoolSet & A , const BoolSet & B );

/// the intersection, a set of dimension
/// \f$ \min( \mathrm{dim}( A ) , \mathrm{dim}( B ) ) \f$
///
/// In pseudocode (section 8.1), a word of 64 booleans at a time,
/// a word missing in the shorter vector being 0:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setIntersection}( A , B ) \cr
/// 2 & \quad C \gets \text{a set of dimension } \min( \mathrm{dim}( A ) ,
///     \mathrm{dim}( B ) ) \cr
/// 3 & \quad \textbf{foreach}\ \text{word } k \text{ of } C\ \textbf{do}
///     \ C.W[ k ] \gets A.W[ k ] \wedge B.W[ k ] \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
inline BoolSet setIntersection( const BoolSet & A , const BoolSet & B );

/// the difference, a set of dimension `A.dim()`
///
/// In pseudocode (section 8.1), a word of 64 booleans at a time,
/// a word missing in the shorter vector being 0:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setDifference}( A , B ) \cr
/// 2 & \quad C \gets \text{a set of dimension } \mathrm{dim}( A ) \cr
/// 3 & \quad \textbf{foreach}\ \text{word } k \text{ of } C\ \textbf{do}
///     \ C.W[ k ] \gets A.W[ k ] \wedge \lnot B.W[ k ] \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
inline BoolSet setDifference( const BoolSet & A , const BoolSet & B );

/// the symmetric difference, the items in exactly one of \f$ A \f$ and
/// \f$ B \f$, a set of dimension
/// \f$ \max( \mathrm{dim}( A ) , \mathrm{dim}( B ) ) \f$
///
/// In pseudocode (exercise 8.5), a word of 64 booleans at a time,
/// a word missing in the shorter vector being 0:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setSymmetricDifference}( A , B ) \cr
/// 2 & \quad C \gets \text{a set of dimension } \max( \mathrm{dim}( A ) ,
///     \mathrm{dim}( B ) ) \cr
/// 3 & \quad \textbf{foreach}\ \text{word } k \text{ of } C\ \textbf{do}
///     \ C.W[ k ] \gets A.W[ k ] \oplus B.W[ k ] \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
inline BoolSet setSymmetricDifference( const BoolSet & A ,
                                       const BoolSet & B );

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/BoolSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BoolSet.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BoolSet.h ---------------------------*/
/*--------------------------------------------------------------------------*/
