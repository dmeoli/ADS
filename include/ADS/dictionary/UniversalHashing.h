/*--------------------------------------------------------------------------*/
/*------------------------ File UniversalHashing.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Universal hashing, beyond the book [see `HashFunctions.h` for the fixed
 * functions of section 7.1]: a class \f$ H \f$ of functions
 * \f$ U \rightarrow \{ 0 , \ldots , m - 1 \} \f$ is universal if two
 * distinct keys collide under at most \f$ | H | / m \f$ of its functions,
 * and \f$ c \f$-universal if under at most \f$ c | H | / m \f$; a function
 * drawn at random from it gives every set of keys the expected lists of
 * length \f$ 1 + \alpha \f$ of the simple uniform hashing,
 * \f$ \alpha = n / m \f$, and the longest of them is
 * \f$ O( \log n / \log \log n ) \f$ in expectation when
 * \f$ m = \Theta( n ) \f$, whatever the keys, while for every fixed
 * function there are \f$ n \f$ keys that fall in one list. The classes are
 *
 * - `CarterWegmanHash`,
 *   \f$ h_{a,b}( k ) = ( ( a k + b ) \bmod p ) \bmod m \f$ with \f$ p \f$ a
 *   prime larger than the keys, \f$ 1 \le a < p \f$ and \f$ 0 \le b < p \f$,
 *   universal for every \f$ m \f$; with \f$ p = 2^{61} - 1 \f$ the product
 *   mod \f$ p \f$ is made of four products of 32 bits and a few shifts, with
 *   no integer of 128 bits [see `mulMod61()` and `mulMod()` in
 *   `NumberTheory.h`];
 * - `DotProductHash`, \f$ h_a( k ) = \sum_i a_i k_i \bmod m \f$ with
 *   \f$ m \f$ prime, the key cut into its \f$ r \f$ digits \f$ k_i \f$ in
 *   base \f$ m \f$ and \f$ a \f$ a vector of \f$ r \f$ digits: universal,
 *   since for two keys that differ in a digit the collision fixes that digit
 *   of \f$ a \f$ once the others are chosen;
 * - `MultiplyShiftHash`, the class of Dietzfelbinger, Hagerup, Katajainen
 *   and Penttonen,
 *   \f$ h_a( k ) = ( a k \bmod 2^h ) \operatorname{div} 2^{h-l} \f$ with
 *   \f$ a \f$ odd, for keys of \f$ h \f$ bits and \f$ m = 2^l \f$:
 *   2-universal (two keys collide with probability at most \f$ 2 / m \f$),
 *   and only a product and a shift, the fastest of the three;
 * - `StringHash`, the polynomial \f$ \sum_i ( s_i + 1 ) x^i \bmod p \f$ of
 *   the characters of a string at a random point \f$ x \f$,
 *   \f$ p = 2^{61} - 1 \f$, each character counted one more so that the
 *   character 0 counts too (the strings of zeros of different lengths, or
 *   a string and the same followed by zeros, would collide otherwise): two
 *   distinct strings of length at most \f$ L \f$ collide with probability
 *   at most \f$ L / p \f$, since their difference is a polynomial of degree
 *   at most \f$ L \f$, not zero, with at most \f$ L \f$ roots; it turns a
 *   string into a key for the classes above.
 *
 * `DLeftHashTable` is the d-left hashing of Vöcking on top of them:
 * \f$ d \f$ tables of \f$ m / d \f$ lists, each with a function of its own,
 * a key going to the shortest of its \f$ d \f$ lists and to the leftmost one
 * on a tie; a search looks at the \f$ d \f$ lists. The longest list is then
 * \f$ \ln \ln n / ( d \ln \phi_d ) + O( 1 ) \f$ with high probability for
 * \f$ n = m \f$ keys, \f$ \phi_d \f$ being the ratio of the generalized
 * Fibonacci numbers, against \f$ \Theta( \log n / \log \log n ) \f$ with one
 * function: the power of two choices of Azar, Broder, Karlin and Upfal, an
 * exponential gain for one more probe, which lets each list be a bucket of
 * fixed size \f$ O( \log \log n ) \f$ instead of a linked list. Its
 * functions are simple tabulation [see `TabulationHash` in
 * `RandomHashing.h`], for which Dahlgaard, Knudsen, Rotenberg and Thorup
 * proved a longest list of \f$ O( \log \log n ) \f$ with high probability.
 *
 * The contracts [see `Contract.h`]: a key below the universe of its class
 * (\f$ p \f$ for `CarterWegmanHash`, \f$ m^r \f$ for `DotProductHash`,
 * \f$ 2^h \f$ for `MultiplyShiftHash`), the parameters of a class in its
 * range (a prime \f$ p \f$ or \f$ m \f$ is the caller's duty, not checked),
 * each hash below its \f$ m \f$; `insert()` of `DLeftHashTable` ensures that
 * the key is there and `remove()` that it is not, and its invariant is that
 * each key is in one of its \f$ d \f$ lists and is counted once, checked on
 * the key of the operation only past 1024 keys, so that a long sequence of
 * operations at the level 3 stays linear. The functions are defined in
 * `UniversalHashing.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8, The Dictionary Problem,
 *      doi:10.1017/9781009128933
 * \see J. L. Carter, M. N. Wegman, Universal classes of hash functions,
 *      Journal of Computer and System Sciences 18(2), 143-154, 1979,
 *      doi:10.1016/0022-0000(79)90044-8
 * \see M. Dietzfelbinger, T. Hagerup, J. Katajainen, M. Penttonen, A
 *      reliable randomized algorithm for the closest-pair problem, Journal
 *      of Algorithms 25(1), 19-51, 1997, doi:10.1006/jagm.1997.0873
 * \see Y. Azar, A. Z. Broder, A. R. Karlin, E. Upfal, Balanced
 *      allocations, SIAM Journal on Computing 29(1), 180-200, 1999,
 *      doi:10.1137/S0097539795288490
 * \see B. Vöcking, How asymmetry helps load balancing, Journal of the ACM
 *      50(4), 568-589, 2003, doi:10.1145/792538.792546
 * \see S. Dahlgaard, M. B. T. Knudsen, E. Rotenberg, M. Thorup, The power
 *      of two choices with simple tabulation, Proceedings of SODA 2016,
 *      1631-1642, 2016, doi:10.1137/1.9781611974331.ch111
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

#ifndef __ADS_UniversalHashing
 #define __ADS_UniversalHashing
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/RandomNumbers.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS CarterWegmanHash --------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ h( k ) = ( ( a k + b ) \bmod p ) \bmod m \f$, of the class
/// \f$ H_{p,m} \f$ of Carter and Wegman
class CarterWegmanHash {
 public:
 /// a function drawn from \f$ H_{p,m} \f$ with \f$ p = 2^{61} - 1 \f$, from
 /// the seed
 /// \pre \f$ m \ge 1 \f$
 explicit CarterWegmanHash( std::uint64_t m , std::uint64_t seed = 1 );

 /// the function of the given parameters
 /// \pre \f$ 2 \le p < 2^{63} \f$, \f$ 1 \le a < p \f$, \f$ b < p \f$ and
 ///      \f$ m \ge 1 \f$
 CarterWegmanHash( std::uint64_t p , std::uint64_t a , std::uint64_t b ,
                   std::uint64_t m );

 /// \f$ h( k ) \f$, the product \f$ a k \bmod p \f$ computed with no
 /// overflow [see `mulMod()`]
 /// \pre \f$ k < p \f$
 /// \post the result is below \f$ m \f$
 [[nodiscard]] std::uint64_t operator()( std::uint64_t k ) const;

 /// the prime \f$ p \f$
 [[nodiscard]] std::uint64_t p( void ) const { return( f_p ); }

 /// the multiplier \f$ a \f$
 [[nodiscard]] std::uint64_t a( void ) const { return( f_a ); }

 /// the addend \f$ b \f$
 [[nodiscard]] std::uint64_t b( void ) const { return( f_b ); }

 /// the size \f$ m \f$ of the range
 [[nodiscard]] std::uint64_t m( void ) const { return( f_m ); }

 private:
 /// \f$ p \f$, \f$ a \f$, \f$ b \f$ and \f$ m \f$
 std::uint64_t f_p , f_a , f_b , f_m;
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DotProductHash ---------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ h( k ) = \sum_i a_i k_i \bmod m \f$, the \f$ k_i \f$ being the
/// digits of \f$ k \f$ in base \f$ m \f$
class DotProductHash {
 public:
 /// a function with \f$ r \f$ random digits \f$ a_i \f$, from the seed
 /// \pre \f$ 2 \le m \le 2^{32} \f$ and \f$ r \ge 1 \f$
 DotProductHash( std::uint64_t m , int r , std::uint64_t seed = 1 );

 /// the function of the digits \f$ a \f$
 /// \pre \f$ 2 \le m \le 2^{32} \f$, \f$ a \f$ not empty, each digit below
 ///      \f$ m \f$
 DotProductHash( std::uint64_t m , std::vector< std::uint64_t > a );

 /// \f$ h( k ) \f$
 ///
 /// In pseudocode (chapter 8 of Ferragina):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ h( k ) \cr
 /// 2 & \quad s \gets 0 \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ r - 1\ \textbf{do} \cr
 /// 4 & \qquad s \gets ( s + a_i ( k \bmod m ) ) \bmod m;\ k \gets \lfloor k /
 ///     m \rfloor \cr
 /// 5 & \quad \textbf{return}\ s
 /// \end{array}}
 /// \f]
 /// \pre \f$ k < m^r \f$
 /// \post the result is below \f$ m \f$
 [[nodiscard]] std::uint64_t operator()( std::uint64_t k ) const;

 /// the size \f$ m^r \f$ of the universe, or 0 if it exceeds
/// \f$ 2^{64} - 1 \f$
 [[nodiscard]] std::uint64_t universe( void ) const;

 private:
 std::uint64_t f_m;                  ///< the base \f$ m \f$
 std::vector< std::uint64_t > f_a;   ///< the digits, the least first
 };

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MultiplyShiftHash -------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ h( k ) = ( a k \bmod 2^h ) \operatorname{div} 2^{h - l} \f$, with
/// \f$ a \f$ odd
class MultiplyShiftHash {
 public:
 /// a function with a random odd \f$ a \f$, from the seed
 /// \pre \f$ 1 \le l < h \le 64 \f$
 MultiplyShiftHash( int h , int l , std::uint64_t seed = 1 );

 /// the function of the multiplier \f$ a \f$
 /// \pre \f$ 1 \le l < h \le 64 \f$, \f$ a \f$ odd and below \f$ 2^h \f$
 [[nodiscard]] static MultiplyShiftHash ofMultiplier( int h , int l ,
                                                      std::uint64_t a );

 /// \f$ h( k ) \f$, a product and a shift
 /// \pre \f$ k < 2^h \f$
 /// \post the result is below \f$ 2^l \f$
 [[nodiscard]] std::uint64_t operator()( std::uint64_t k ) const;

 private:
 int f_h , f_l;          ///< \f$ h \f$ and \f$ l \f$
 std::uint64_t f_a;      ///< the multiplier \f$ a \f$
 std::uint64_t f_mask;   ///< \f$ 2^h - 1 \f$
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS StringHash ----------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ h( s ) = \sum_i ( s_i + 1 ) x^i \bmod ( 2^{61} - 1 ) \f$, at a
/// random point \f$ x \f$
class StringHash {
 public:
 /// a function with a random point \f$ x \f$, from the seed
 explicit StringHash( std::uint64_t seed = 1 );

 /// \f$ h( s ) \f$
 ///
 /// In pseudocode (chapter 8 of Ferragina), by the rule of
 /// Horner from the last character, \f$ p = 2^{61} - 1 \f$ and each
 /// character counted one more, so that the character 0 counts:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ h( s ) \cr
 /// 2 & \quad h \gets 0 \cr
 /// 3 & \quad \textbf{for}\ i \gets |s| - 1\ \textbf{downto}\ 0
 ///     \ \textbf{do}\ h \gets ( h x + s_i + 1 ) \bmod p \cr
 /// 4 & \quad \textbf{return}\ h
 /// \end{array}}
 /// \f]
 /// \post the result is below \f$ 2^{61} - 1 \f$
 [[nodiscard]] std::uint64_t operator()( std::string_view s ) const;

 private:
 std::uint64_t f_x;   ///< the point, in \f$ [ 1 , p ) \f$
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DLeftHashTable ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on \f$ d \f$ tables of lists, a key in the shortest of its
/// \f$ d \f$ lists
template< class K , class V , class H = std::hash< K > >
class DLeftHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// \f$ d \f$ tables of \f$ w \f$ lists each,
 /// \f$ w = \max\{ 1 , \lfloor m / d \rfloor \} \f$
 /// \pre \f$ m \ge 1 \f$ and \f$ d \ge 1 \f$
 DLeftHashTable( std::size_t m , int d , std::uint64_t seed = 1 ,
                 H h = H() );

 /// the value of the key \f$ k \f$, or nothing, from its \f$ d \f$ lists
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$
 ///
 /// In pseudocode (the d-left hashing of Vöcking, beyond the book),
 /// \f$ L_i( k ) \f$ the list of \f$ k \f$ in the table \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ k \text{ is in some } L_i( k )\ \textbf{then}
 ///     \ \text{give it the value } v;\ \textbf{return} \cr
 /// 3 & \quad j \gets 0 \cr
 /// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ d - 1\ \textbf{do}
 ///     \ \textbf{if}\ |L_i( k )| < |L_j( k )|\ \textbf{then}\ j \gets i \cr
 /// 5 & \quad \text{append } ( k , v ) \text{ to } L_j( k )\quad
 ///     \triangleright\ \text{the shortest, the leftmost on a tie}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, from its list
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the length of the longest list
 [[nodiscard]] std::size_t maxLoad( void ) const;

 /// the number of tables
 [[nodiscard]] int tables( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Bucket = std::vector< std::pair< K , V > >;

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), table after table and
 // list after list, forward; an insertion or a removal invalidates it

 /// the iterator on the pairs, table after table and list after list
 class const_iterator {
  public:
  /// a reference to an item
  using reference = std::pair< const K & , const V & >;
  /// the type of the items, for the standard library
  using value_type = reference;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// the concept of the iterator, for C++20
  using iterator_concept = std::forward_iterator_tag;
  /// the category of the iterator
  using iterator_category = std::input_iterator_tag;

  /// the end of no table
  const_iterator( void ) = default;

  /// on the first pair from the list \f$ b \f$ of the lists \f$ T \f$
  const_iterator( const std::vector< Bucket > * T , std::size_t b )
   : f_T( T ) , f_b( b ) { forward(); }

  /// the pair
  reference operator*( void ) const {
   const auto & e = ( *f_T )[ f_b ][ f_i ];
   return( reference( e.first , e.second ) );
   }
  /// to the next pair
  const_iterator & operator++( void ) {
   ++f_i;
   forward();
   return( *this );
   }
  /// to the next pair, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same place of the same list
  bool operator==( const const_iterator & o ) const {
   return( ( f_b == o.f_b ) && ( f_i == o.f_i ) );
   }

  private:
  /// to the first pair from `( f_b , f_i )` on, or to the end
  void forward( void ) {
   while( ( f_b < f_T->size() ) && ( f_i >= ( *f_T )[ f_b ].size() ) ) {
    ++f_b;
    f_i = 0;
    }
   }

  const std::vector< Bucket > * f_T = nullptr;
  std::size_t f_b = 0;   ///< the list
  std::size_t f_i = 0;   ///< the place in the list
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< const K & , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = value_type;
 /// a constant reference to an item
 using const_reference = value_type;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_T , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( &f_T , f_T.size() ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the list of \f$ k \f$ in the table \f$ i \f$
 [[nodiscard]] std::size_t list( const K & k , int i ) const;

 /// the list of the table that holds \f$ k \f$ and its place there, or none
 [[nodiscard]] std::optional< std::pair< std::size_t , std::size_t > >
  find( const K & k ) const;

 /// \f$ k \f$ at most once in its lists and, up to 1024 keys, each key in one
 /// of its lists and counted once, \f$ O( n d ) \f$
 [[nodiscard]] bool placed( const K & k ) const;

 /// the table \f$ i \f$ at \f$ [ i w , ( i + 1 ) w ) \f$
std::vector< Bucket > f_T;
 std::size_t f_w;                      ///< the lists of a table
 std::vector< TabulationHash > f_g;    ///< the function of each table
 std::size_t f_n = 0;                  ///< the number of keys
 H f_h;                                ///< the hash of the keys
 };  // end( class DLeftHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/UniversalHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* UniversalHashing.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File UniversalHashing.h -----------------------*/
/*--------------------------------------------------------------------------*/
