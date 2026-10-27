/*--------------------------------------------------------------------------*/
/*---------------------------- File BitVector.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector of bits on which the succinct structures and the integer codes
 * of ADS are built: the bits are packed in words of 64 bits, the bit
 * \f$ i \f$ being the bit \f$ i \bmod 64 \f$ (from the least significant
 * one) of the word \f$ \lfloor i / 64 \rfloor \f$, so that a field of up to
 * 64 bits is read or written with two shifts at most. Besides the random
 * access to a bit, in \f$ O( 1 ) \f$ time, `BitVector` appends a bit or a
 * field of \f$ \mathit{len} \f$ bits, `push_back()` and `append()`, in
 * \f$ O( 1 ) \f$ amortized time, and reads a field of
 * \f$ \mathit{len} \f$ bits, `get()`, in \f$ O( 1 ) \f$ time; the space is
 * \f$ n + O( 512 ) \f$ bits for \f$ n \f$ bits, the vector of words
 * growing as `std::vector` does. The words are whole lines of the cache of
 * 64 bytes, with one word at least past the last bit, and they start on a
 * line (on a huge page from 2 MB on, which spares the misses of the TLB of
 * a random access); the bits past the size are 0 (the invariant). Hence
 * `count()` and the rank of a word [see `RankSelect.h`] need no mask at the
 * end, a field is read with no test of the end, and a directory may read
 * the words of a line of its own. Unlike `std::vector< bool >`, whose
 * layout is left to the implementation, `BitVector` exposes its words,
 * which is what a rank directory or a code reading a word at a time needs.
 *
 * `selectInWord()` gives the place of the \f$ k \f$-th 1 of a word by the
 * broadword method of Vigna: the sideways sums of the bytes [see
 * `Bitwise.h`] are accumulated by one multiplication, the byte holding the
 * \f$ k \f$-th 1 is found by a comparison of the 8 bytes in parallel and
 * the trailing zeros, and the 1 inside it by clearing the lower 1s, in
 * \f$ O( 1 ) \f$ time and with no loop over the 64 bits (with the
 * instructions BMI2, a single deposit of the bit \f$ 2^k \f$ on the 1s of
 * the word). It is the last step of every select in ADS, in place of the
 * tables of the small blocks of the classical analysis (Jacobson, Clark).
 * `SubscriptIterator` is the random-access constant iterator of a structure
 * read by its `operator[]`, the bits of a `BitVector` or the items of a
 * compressed sequence [see `EliasFano.h`].
 *
 * The contracts [see `Contract.h`]: the places and the fields are in the
 * vector, a field has at most 64 bits and its value fits in them, and
 * `selectInWord()` needs \f$ k \f$ smaller than the 1s of the word; the
 * invariant is that the bits past the size are 0. The functions are
 * defined in `BitVector.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see S. Vigna, Broadword implementation of rank/select queries, Proc.
 *      WEA 2008, LNCS 5038, 154-168, 2008,
 *      doi:10.1007/978-3-540-68552-4_12
 * \see S. Gog, M. Petri, Optimized succinct data structures for massive
 *      data, Software: Practice and Experience 44(11), 1287-1314, 2014,
 *      doi:10.1002/spe.2198
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

#ifndef __ADS_BitVector
 #define __ADS_BitVector
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/memory/HugePageAllocator.h"

#include <algorithm>
#include <bit>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef __BMI2__
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the allocator of the words of a BitVector and of the directories built
/// on them: a block of 2 MB or more on huge pages [see
/// HugePageAllocator.h], a smaller one aligned to a line of the cache
template< class T >
class LineAllocator {
 public:
 /// the type of an item
 using value_type = T;
 static constexpr std::size_t LINE = 64;   ///< the bytes of a line

 LineAllocator( void ) = default;
 template< class U >
 LineAllocator( const LineAllocator< U > & ) noexcept {}

 [[nodiscard]] T * allocate( std::size_t n ) {
  if( n * sizeof( T ) >= HugePageAllocator< T >::PAGE )
   return( HugePageAllocator< T >().allocate( n ) );
  return( static_cast< T * >( ::operator new( n * sizeof( T ) ,
                                              std::align_val_t( LINE ) ) ) );
  }

 /// a value constructed with no argument is left uninitialized (the
 /// words that are about to be written), any other one as asked
 template< class U >
 void construct( U * p ) noexcept {
  ::new( static_cast< void * >( p ) ) U;
  }
 template< class U , class... A >
 void construct( U * p , A &&... a ) {
  ::new( static_cast< void * >( p ) ) U( std::forward< A >( a )... );
  }

 void deallocate( T * p , std::size_t n ) noexcept {
  if( n * sizeof( T ) >= HugePageAllocator< T >::PAGE )
   HugePageAllocator< T >().deallocate( p , n );
  else ::operator delete( p , std::align_val_t( LINE ) );
  }

 template< class U >
 bool operator==( const LineAllocator< U > & ) const noexcept {
  return( true );
  }
 };

}  // end( namespace detail )

/// the words of a `BitVector`, on lines of the cache
using Words = std::vector< Word , detail::LineAllocator< Word > >;

/// the place of the \f$ k \f$-th 1 of the word \f$ x \f$, counting \f$ k \f$
/// from 0 and the places from the least significant bit
///
/// In pseudocode (the broadword select of Vigna, beyond the book), with
/// \f$ L_8 = \sum_{j < 8} 2^{8 j} \f$, \f$ H_8 = 2^7 L_8 \f$ and
/// \f$ \mathrm{ctz} \f$ the trailing zeros of a word:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{selectInWord}( x , k ) \cr
/// 2 & \quad s \gets \text{the sideways sums of the bytes of } x \cr
/// 3 & \quad s \gets s \cdot L_8\quad \triangleright\ \text{the byte } j
///     \text{ of } s \text{ counts the 1s of the bytes } 0 , \ldots , j \cr
/// 4 & \quad b \gets \lfloor \mathrm{ctz}( ( s + ( 127 - k ) L_8 ) \wedge H_8
///     ) / 8 \rfloor\quad \triangleright\ \text{the first byte past } k \cr
/// 5 & \quad r \gets k - \text{the byte } b - 1 \text{ of } s
///     \ \text{(0 if } b = 0 \text{)} \cr
/// 6 & \quad v \gets \text{the byte } b \text{ of } x \cr
/// 7 & \quad \textbf{for}\ t \gets 1\ \textbf{to}\ r\ \textbf{do}\ v \gets v
///     \wedge ( v - 1 )\quad \triangleright\ \text{its lowest 1 cleared} \cr
/// 8 & \quad \textbf{return}\ 8 b + \mathrm{ctz}( v )
/// \end{array}}
/// \f]
/// \pre \f$ k < \f$ `sidewaysSum( x )`
[[nodiscard]] constexpr int selectInWord( Word x , int k );

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS SubscriptIterator -------------------------*/
/*--------------------------------------------------------------------------*/

/// the random-access constant iterator on the items `T` of the owner `O`,
/// read by `( *o )[ i ]`
template< class O , class T >
class SubscriptIterator {
 public:
 /// the type of an item
 using value_type = T;
 /// an item, returned by value
 using reference = T;
 /// the type of the distance of two places
 using difference_type = std::ptrdiff_t;
 /// a random-access iterator, read in place
 using iterator_concept = std::random_access_iterator_tag;
 /// an input iterator for the algorithms of before C++20, since its items
 /// are returned by value
 using iterator_category = std::input_iterator_tag;

 /// an iterator on no owner
 SubscriptIterator( void ) = default;
 /// the iterator on the item \f$ i \f$ of `*o`
 SubscriptIterator( const O * o , std::size_t i )
  : f_o( o ) , f_i( difference_type( i ) ) {}

 /// the item
 reference operator*( void ) const { return( ( *this )[ 0 ] ); }
 /// the item \f$ k \f$ places ahead
 reference operator[]( difference_type k ) const {
  return( ( *f_o )[ std::size_t( f_i + k ) ] );
  }

 /// the next item
 SubscriptIterator & operator++( void ) { ++f_i; return( *this ); }
 /// the previous item
 SubscriptIterator & operator--( void ) { --f_i; return( *this ); }
 /// the next item, returning the iterator before it
 SubscriptIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 /// the previous item, returning the iterator before it
 SubscriptIterator operator--( int ) { auto t = *this; --f_i; return( t ); }
 /// \f$ k \f$ places ahead
 SubscriptIterator & operator+=( difference_type k ) {
  f_i += k;
  return( *this );
  }
 /// \f$ k \f$ places back
 SubscriptIterator & operator-=( difference_type k ) {
  f_i -= k;
  return( *this );
  }

 /// the iterator \f$ k \f$ places after \f$ a \f$
 friend SubscriptIterator operator+( SubscriptIterator a ,
                                     difference_type k ) {
  return( a += k );
  }
 /// the iterator \f$ k \f$ places after \f$ a \f$
 friend SubscriptIterator operator+( difference_type k ,
                                     SubscriptIterator a ) {
  return( a += k );
  }
 /// the iterator \f$ k \f$ places before \f$ a \f$
 friend SubscriptIterator operator-( SubscriptIterator a ,
                                     difference_type k ) {
  return( a -= k );
  }
 /// the places from \f$ b \f$ to \f$ a \f$
 friend difference_type operator-( const SubscriptIterator & a ,
                                   const SubscriptIterator & b ) {
  return( a.f_i - b.f_i );
  }

 /// whether the two iterators are on the same place
 bool operator==( const SubscriptIterator & o ) const {
  return( f_i == o.f_i );
  }
 /// the order of the places of the two iterators
 std::strong_ordering operator<=>( const SubscriptIterator & o ) const {
  return( f_i <=> o.f_i );
  }

 /// the place of the item
 [[nodiscard]] std::size_t index( void ) const {
  return( std::size_t( f_i ) );
  }

 private:
 const O * f_o = nullptr;
 difference_type f_i = 0;

 };  // end( class SubscriptIterator )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BitVector -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a vector of bits packed in words of 64 bits
class BitVector {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty vector
 BitVector( void ) = default;

 /// \f$ n \f$ bits equal to \f$ b \f$
 explicit BitVector( std::size_t n , bool b = false );

 /// the bits written in \f$ s \f$, the bit 0 first
 /// \pre \f$ s \f$ has only the characters `'0'` and `'1'`
 explicit BitVector( std::string_view s );

 /// the \f$ n \f$ bits of the words \f$ w \f$, the bit \f$ i \f$ in the bit
 /// \f$ i \bmod 64 \f$ of the word \f$ \lfloor i / 64 \rfloor \f$, as in
 /// `words()`
 /// \pre \f$ w \f$ holds \f$ \lceil n / 64 \rceil \f$ words at least
 BitVector( std::span< const Word > w , std::size_t n );

 /// the number of bits
 [[nodiscard]] std::size_t size( void ) const;

 /// whether there is no bit
 [[nodiscard]] bool empty( void ) const;

 /// the bit \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// writes \f$ b \f$ in the bit \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 /// \post the bit \f$ i \f$ is \f$ b \f$
 void set( std::size_t i , bool b = true );

 /// appends the bit \f$ b \f$
 /// \post the size grows by 1 and the last bit is \f$ b \f$
 void push_back( bool b );

 /// appends the \f$ \mathit{len} \f$ bits of \f$ v \f$, its bit 0 first
 ///
 /// In pseudocode (beyond the book), \f$ n \f$ the size and \f$ W \f$ the
 /// words, which grow by a line when the last one is reached:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{append}( v , \mathit{len} ) \cr
 /// 2 & \quad w \gets \lfloor n / 64 \rfloor;\ o \gets n \bmod 64 \cr
 /// 3 & \quad W[ w ] \gets W[ w ] \vee ( v \ll o )\quad \triangleright
 ///     \ \text{the bits past } n \text{ are } 0 \cr
 /// 4 & \quad \textbf{if}\ o + \mathit{len} > 64\ \textbf{then}\ W[ w + 1 ]
 ///     \gets v \gg ( 64 - o ) \cr
 /// 5 & \quad n \gets n + \mathit{len}
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{len} \le 64 \f$ and \f$ v < 2^{\mathit{len}} \f$
 /// \post `get( size() - len , len )` is \f$ v \f$
 void append( Word v , int len );

 /// the \f$ \mathit{len} \f$ bits from the place \f$ i \f$, the bit
 /// \f$ i \f$ in the bit 0 of the result
 ///
 /// In pseudocode (beyond the book), with no test of the end, since a word
 /// always follows the last bit:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{get}( i , \mathit{len} ) \cr
 /// 2 & \quad w \gets \lfloor i / 64 \rfloor;\ o \gets i \bmod 64 \cr
 /// 3 & \quad r \gets ( W[ w ] \gg o ) \vee ( ( W[ w + 1 ] \ll 1 ) \ll ( 63
 ///     - o ) ) \cr
 /// 4 & \quad \textbf{return}\ r \bmod 2^{\mathit{len}}
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{len} \le 64 \f$ and
 ///      \f$ i + \mathit{len} \le \f$ `size()`
 [[nodiscard]] Word get( std::size_t i , int len ) const;

 /// writes the \f$ \mathit{len} \f$ bits of \f$ v \f$ from the place
 /// \f$ i \f$, its bit 0 in the bit \f$ i \f$
 /// \pre \f$ \mathit{len} \le 64 \f$, \f$ v < 2^{\mathit{len}} \f$ and
 ///      \f$ i + \mathit{len} \le \f$ `size()`
 /// \post `get( i , len )` is \f$ v \f$
 void put( std::size_t i , Word v , int len );

 /// the number of 1 bits, in \f$ O( n / 64 ) \f$ time
 [[nodiscard]] std::size_t count( void ) const;

 /// the words, the bits past the size being 0; they are at least
 /// \f$ \lfloor n / 64 \rfloor + 2 \f$, a multiple of 8, and start on a
 /// line of the cache
 [[nodiscard]] const Words & words( void ) const;

 /// the bits as a string of `'0'` and `'1'`, the bit 0 first
 [[nodiscard]] std::string toString( void ) const;

 /// room for \f$ n \f$ bits
 void reserve( std::size_t n );

 /// no bit
 void clear( void );

 /// whether the two vectors have the same bits
 friend bool operator==( const BitVector & a , const BitVector & b ) =
                                                                     default;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the type of an item
 using value_type = bool;
 /// the type of a size
 using size_type = std::size_t;
 /// the type of the distance of two places
 using difference_type = std::ptrdiff_t;
 /// an item, returned by value
 using reference = bool;
 /// an item, returned by value
 using const_reference = bool;
 /// the constant iterator, random access
 using const_iterator = SubscriptIterator< BitVector , bool >;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 template< bool > friend class BasicRankSelect;   // which copy the words
 friend class CompactRankSelect;                  // as they count them

 /// the words for n bits: n / 64 + 2, rounded up to a line of 8
 [[nodiscard]] static std::size_t wordsFor( std::size_t n ) {
  return( ( n / 64 + 9 ) & ~std::size_t( 7 ) );
  }

 /// the bits past the size are 0
 [[nodiscard]] bool clean( void ) const;

 Words f_w = Words( 8 , 0 );   ///< the words, the bit 0 in the first one
 std::size_t f_n = 0;       ///< the number of bits

 };  // end( class BitVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/BitVector.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BitVector.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BitVector.h --------------------------*/
/*--------------------------------------------------------------------------*/
