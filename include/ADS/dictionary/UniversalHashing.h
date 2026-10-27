/*--------------------------------------------------------------------------*/
/*------------------------ File UniversalHashing.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Universal hashing, beyond the book [see HashFunctions.h for the fixed
 * functions of section 7.1]: a class H of functions \f$ U \rightarrow \{ 0
 * , \ldots , m - 1 \} \f$ is universal if two distinct keys collide under
 * at most \f$ | H | / m \f$ of its functions, and c-universal if under at
 * most \f$ c | H | / m \f$; a function drawn at random from it gives every
 * set of keys the expected lists of length \f$ 1 + \alpha \f$ of the simple
 * uniform hashing, \f$ \alpha = n / m \f$, and the longest of them is
 * \f$ O( \log n / \log \log n ) \f$ in expectation when \f$ m = \Theta( n )
 * \f$, whatever the keys, while for every fixed function there are n keys
 * that fall in one list. The classes are
 *
 * - CarterWegmanHash, \f$ h_{a,b}( k ) = ( ( a k + b ) \bmod p ) \bmod m
 *   \f$ with p a prime larger than the keys, \f$ 1 \le a < p \f$ and \f$ 0
 *   \le b < p \f$, universal for every m; with \f$ p = 2^{61} - 1 \f$ the
 *   product mod p is made of four products of 32 bits and a few shifts,
 *   with no integer of 128 bits [see mulMod61() and mulMod() in
 *   NumberTheory.h];
 * - DotProductHash, \f$ h_a( k ) = \sum_i a_i k_i \bmod m \f$ with m prime,
 *   the key cut into its r digits \f$ k_i \f$ in base m and \f$ a \f$ a
 *   vector of r digits: universal, since for two keys that differ in a
 *   digit the collision fixes that digit of a once the others are chosen;
 * - MultiplyShiftHash, the class of Dietzfelbinger, Hagerup, Katajainen and
 *   Penttonen, \f$ h_a( k ) = ( a k \bmod 2^h ) \operatorname{div}
 *   2^{h-l} \f$ with a odd, for keys of h bits and \f$ m = 2^l \f$:
 *   2-universal (two keys collide with probability at most \f$ 2 / m \f$),
 *   and only a product and a shift, the fastest of the three;
 * - StringHash, the polynomial \f$ \sum_i s_i x^i \bmod p \f$ of the
 *   characters of a string at a random point x, \f$ p = 2^{61} - 1 \f$: two
 *   distinct strings of length at most L collide with probability at most
 *   \f$ L / p \f$, since their difference is a polynomial of degree at most
 *   L with at most L roots; it turns a string into a key for the classes
 *   above.
 *
 * DLeftHashTable is the d-left hashing of Vöcking on top of them: d tables
 * of m / d lists, each with a function of its own, a key going to the
 * shortest of its d lists and to the leftmost one on a tie; a search looks
 * at the d lists. The longest list is then \f$ \ln \ln n / ( d \ln \phi_d )
 * + O( 1 ) \f$ with high probability for n = m keys, \f$ \phi_d \f$ being
 * the ratio of the generalized Fibonacci numbers, against \f$ \Theta( \log
 * n / \log \log n ) \f$ with one function: the power of two choices of
 * Azar, Broder, Karlin and Upfal, an exponential gain for one more probe,
 * which lets each list be a bucket of fixed size \f$ O( \log \log n ) \f$
 * instead of a linked list. Its functions are simple tabulation [see
 * TabulationHash in RandomHashing.h], for which Dahlgaard, Knudsen,
 * Rotenberg and Thorup proved a longest list of \f$ O( \log \log n ) \f$
 * with high probability.
 *
 * The contracts [see Contract.h]: a key below the universe of its class (p
 * for CarterWegmanHash, \f$ m^r \f$ for DotProductHash, \f$ 2^h \f$ for
 * MultiplyShiftHash), the parameters of a class in its range (a prime p or
 * m is the caller's duty, not checked), each hash below its m; insert() of
 * DLeftHashTable ensures that the key is there and remove() that it is not,
 * and its invariant is that each key is in one of its d lists and is counted
 * once, checked on the key of the operation only past 1024 keys, so that a
 * long sequence of operations at the level 3 stays linear. The functions
 * are defined in UniversalHashing.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8 (sections 8.2 and 8.3)
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
 *      1631-1642, 2016
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

/// h( k ) = ( ( a k + b ) mod p ) mod m, of the class H_{p,m}
class CarterWegmanHash {
 public:
 /// a function drawn from H_{p,m} with p = 2^61 - 1, from the seed
 /// \pre m >= 1
 explicit CarterWegmanHash( std::uint64_t m , std::uint64_t seed = 1 );

 /// the function of the given parameters
 /// \pre 2 <= p < 2^63, 1 <= a < p, b < p and m >= 1
 CarterWegmanHash( std::uint64_t p , std::uint64_t a , std::uint64_t b ,
                   std::uint64_t m );

 /// \pre k < p
 /// \post the result is below m
 [[nodiscard]] std::uint64_t operator()( std::uint64_t k ) const;

 [[nodiscard]] std::uint64_t p( void ) const { return( f_p ); }
 [[nodiscard]] std::uint64_t a( void ) const { return( f_a ); }
 [[nodiscard]] std::uint64_t b( void ) const { return( f_b ); }
 [[nodiscard]] std::uint64_t m( void ) const { return( f_m ); }

 private:
 std::uint64_t f_p , f_a , f_b , f_m;
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DotProductHash ---------------------------*/
/*--------------------------------------------------------------------------*/

/// h( k ) = sum_i a_i k_i mod m, the k_i being the digits of k in base m
class DotProductHash {
 public:
 /// a function with r random digits a_i, from the seed
 /// \pre 2 <= m <= 2^32 and r >= 1
 DotProductHash( std::uint64_t m , int r , std::uint64_t seed = 1 );

 /// the function of the digits a
 /// \pre 2 <= m <= 2^32, a not empty, each digit below m
 DotProductHash( std::uint64_t m , std::vector< std::uint64_t > a );

 /// \pre k < m^r
 /// \post the result is below m
 [[nodiscard]] std::uint64_t operator()( std::uint64_t k ) const;

 /// the size m^r of the universe, or 0 if it exceeds 2^64 - 1
 [[nodiscard]] std::uint64_t universe( void ) const;

 private:
 std::uint64_t f_m;
 std::vector< std::uint64_t > f_a;   ///< the digits, the least first
 };

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MultiplyShiftHash -------------------------*/
/*--------------------------------------------------------------------------*/

/// h( k ) = ( a k mod 2^h ) div 2^( h - l ), with a odd
class MultiplyShiftHash {
 public:
 /// a function with a random odd a, from the seed
 /// \pre 1 <= l < h <= 64
 MultiplyShiftHash( int h , int l , std::uint64_t seed = 1 );

 /// the function of the multiplier a
 /// \pre 1 <= l < h <= 64, a odd and below 2^h
 [[nodiscard]] static MultiplyShiftHash ofMultiplier( int h , int l ,
                                                      std::uint64_t a );

 /// \pre k < 2^h
 /// \post the result is below 2^l
 [[nodiscard]] std::uint64_t operator()( std::uint64_t k ) const;

 private:
 int f_h , f_l;
 std::uint64_t f_a;
 std::uint64_t f_mask;   ///< 2^h - 1
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS StringHash ----------------------------*/
/*--------------------------------------------------------------------------*/

/// h( s ) = sum_i s_i x^i mod 2^61 - 1, at a random point x
class StringHash {
 public:
 explicit StringHash( std::uint64_t seed = 1 );

 /// \post the result is below 2^61 - 1
 [[nodiscard]] std::uint64_t operator()( std::string_view s ) const;

 private:
 std::uint64_t f_x;   ///< the point, in [ 1 , p )
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DLeftHashTable ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on d tables of lists, a key in the shortest of its d lists
template< class K , class V , class H = std::hash< K > >
class DLeftHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// d tables of w lists each, w = max( 1 , m / d )
 /// \pre m >= 1 and d >= 1
 DLeftHashTable( std::size_t m , int d , std::uint64_t seed = 1 ,
                 H h = H() );

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

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

 class const_iterator {
  public:
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;
  const_iterator( const std::vector< Bucket > * T , std::size_t b )
   : f_T( T ) , f_b( b ) { forward(); }

  reference operator*( void ) const {
   const auto & e = ( *f_T )[ f_b ][ f_i ];
   return( reference( e.first , e.second ) );
   }
  const_iterator & operator++( void ) {
   ++f_i;
   forward();
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  bool operator==( const const_iterator & o ) const {
   return( ( f_b == o.f_b ) && ( f_i == o.f_i ) );
   }

  private:
  /// to the first pair from ( f_b , f_i ) on, or to the end
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

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_T , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( &f_T , f_T.size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the list of k in the table i
 [[nodiscard]] std::size_t list( const K & k , int i ) const;

 /// the list of the table that holds k and its place there, or none
 [[nodiscard]] std::optional< std::pair< std::size_t , std::size_t > >
  find( const K & k ) const;

 /// k at most once in its lists and, up to 1024 keys, each key in one of
 /// its lists and counted once, O( n d )
 [[nodiscard]] bool placed( const K & k ) const;

 std::vector< Bucket > f_T;            ///< table i at [ i w , ( i + 1 ) w )
 std::size_t f_w;                      ///< the lists of a table
 std::vector< TabulationHash > f_g;    ///< the function of each table
 std::size_t f_n = 0;
 H f_h;

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
