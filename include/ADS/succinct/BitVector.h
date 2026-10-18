/*--------------------------------------------------------------------------*/
/*---------------------------- File BitVector.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector of bits on which the succinct structures and the integer codes
 * of ADS are built: the bits are packed in words of 64 bits, the bit i
 * being the bit i mod 64 (from the least significant one) of the word
 * \f$ \lfloor i / 64 \rfloor \f$, so that a field of up to 64 bits is read
 * or written with two shifts at most. Besides the random access to a bit,
 * in O( 1 ) time, BitVector appends a bit or a field of len bits, push_back()
 * and append(), in O( 1 ) amortized time, and reads a field of len bits,
 * get(), in O( 1 ) time; the space is \f$ n + O( 64 ) \f$ bits for n bits,
 * the vector of words doubling as std::vector does. The bits past the size
 * in the last word are 0 (the invariant), and hence count() and the rank
 * of a word [see RankSelect.h] need no mask at the end. Unlike
 * std::vector< bool >, whose layout is left to the implementation,
 * BitVector exposes its words, which is what a rank directory or a code
 * reading a word at a time needs.
 *
 * selectInWord() gives the place of the k-th 1 of a word by the broadword
 * method of Vigna: the sideways sums of the bytes [see Bitwise.h] are
 * accumulated by one multiplication, the byte holding the k-th 1 is found
 * by a comparison of the 8 bytes in parallel and the trailing zeros, and the
 * 1 inside it by clearing the lower 1s, in O( 1 ) time and with no loop over
 * the 64 bits. It is the last step of every select in ADS, in place of the
 * tables of the small blocks of the classical analysis (Jacobson, Clark).
 * SubscriptIterator is the random-access constant iterator of a structure
 * read by its operator[], the bits of a BitVector or the items of a
 * compressed sequence [see EliasFano.h].
 *
 * The contracts [see Contract.h]: the places and the fields are in the
 * vector, a field has at most 64 bits and its value fits in them, and
 * selectInWord() needs k smaller than the 1s of the word; the invariant
 * is that the bits past the size are 0. The functions are defined in
 * BitVector.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
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
 *         Universita' di Pisa \n
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

#include <compare>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place of the k-th 1 of x, counting from 0 and from the least
/// significant bit
/// \pre k < sidewaysSum( x )
[[nodiscard]] constexpr int selectInWord( Word x , int k );

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS SubscriptIterator -------------------------*/
/*--------------------------------------------------------------------------*/

/// the random-access constant iterator on the items T of the owner O, read
/// by ( *o )[ i ]
template< class O , class T >
class SubscriptIterator {
 public:
 using value_type = T;
 using reference = T;
 using difference_type = std::ptrdiff_t;
 using iterator_concept = std::random_access_iterator_tag;
 using iterator_category = std::input_iterator_tag;

 SubscriptIterator( void ) = default;
 SubscriptIterator( const O * o , std::size_t i )
  : f_o( o ) , f_i( difference_type( i ) ) {}

 reference operator*( void ) const { return( ( *this )[ 0 ] ); }
 reference operator[]( difference_type k ) const {
  return( ( *f_o )[ std::size_t( f_i + k ) ] );
  }

 SubscriptIterator & operator++( void ) { ++f_i; return( *this ); }
 SubscriptIterator & operator--( void ) { --f_i; return( *this ); }
 SubscriptIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 SubscriptIterator operator--( int ) { auto t = *this; --f_i; return( t ); }
 SubscriptIterator & operator+=( difference_type k ) {
  f_i += k;
  return( *this );
  }
 SubscriptIterator & operator-=( difference_type k ) {
  f_i -= k;
  return( *this );
  }

 friend SubscriptIterator operator+( SubscriptIterator a ,
                                     difference_type k ) {
  return( a += k );
  }
 friend SubscriptIterator operator+( difference_type k ,
                                     SubscriptIterator a ) {
  return( a += k );
  }
 friend SubscriptIterator operator-( SubscriptIterator a ,
                                     difference_type k ) {
  return( a -= k );
  }
 friend difference_type operator-( const SubscriptIterator & a ,
                                   const SubscriptIterator & b ) {
  return( a.f_i - b.f_i );
  }

 bool operator==( const SubscriptIterator & o ) const {
  return( f_i == o.f_i );
  }
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

 BitVector( void ) = default;

 /// n bits equal to b
 explicit BitVector( std::size_t n , bool b = false );

 /// the bits written in s, the bit 0 first
 /// \pre s has only the characters '0' and '1'
 explicit BitVector( std::string_view s );

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bit i
 /// \pre i < size()
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// writes b in the bit i
 /// \pre i < size()
 /// \post ( *this )[ i ] == b
 void set( std::size_t i , bool b = true );

 /// appends the bit b
 /// \post the size grows by 1 and the last bit is b
 void push_back( bool b );

 /// appends the len bits of v, its bit 0 first
 /// \pre len <= 64 and v < 2^len
 /// \post get( size() - len , len ) == v
 void append( Word v , int len );

 /// the len bits from the place i, the bit i in the bit 0 of the result
 /// \pre len <= 64 and i + len <= size()
 [[nodiscard]] Word get( std::size_t i , int len ) const;

 /// writes the len bits of v from the place i, its bit 0 in the bit i
 /// \pre len <= 64, v < 2^len and i + len <= size()
 /// \post get( i , len ) == v
 void put( std::size_t i , Word v , int len );

 /// the number of 1 bits, in O( n / 64 ) time
 [[nodiscard]] std::size_t count( void ) const;

 /// the words, the bits past the size being 0
 [[nodiscard]] const std::vector< Word > & words( void ) const;

 /// the bits as a string of '0' and '1', the bit 0 first
 [[nodiscard]] std::string toString( void ) const;

 /// room for n bits
 void reserve( std::size_t n );

 /// no bit
 void clear( void );

 friend bool operator==( const BitVector & a , const BitVector & b ) =
                                                                     default;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = bool;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = bool;
 using const_reference = bool;
 using const_iterator = SubscriptIterator< BitVector , bool >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the bits past the size are 0
 [[nodiscard]] bool clean( void ) const;

 std::vector< Word > f_w;   ///< the words, the bit 0 in the first one
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
