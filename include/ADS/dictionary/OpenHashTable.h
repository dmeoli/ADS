/*--------------------------------------------------------------------------*/
/*-------------------------- File OpenHashTable.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash table with open addressing of section 7.2.2, a realization of
 * the DICTIONARY [see `ADT.h`].
 *
 * The table keeps the pairs in the vector itself (open addressing): the
 * key \f$ k \f$ goes into the first free place of its sequence of inspection
 * \f$ H( k , 0 ) , H( k , 1 ) , \ldots , H( k , m - 1 ) \f$, which is
 *
 * - linear, \f$ H( k , i ) = ( H_0( k ) + \delta i ) \bmod m \f$, a
 *   permutation of the places when \f$ \gcd( \delta , m ) = 1 \f$;
 * - quadratic, \f$ H( k , i ) = ( H_0( k ) + c_1 i + c_2 i^2 ) \bmod m \f$
 *   with \f$ m \f$ prime, which in general is not a permutation, so an
 *   insertion may fail while some place is still free;
 * - double hashing,
 *   \f$ H( k , i ) = ( H_0( k ) + H_\delta( k ) i ) \bmod m \f$, with
 *   \f$ H_\delta( k ) = 1 + ( h( k ) \bmod ( m - 1 ) ) \f$ for \f$ m \f$
 *   prime and
 *   \f$ H_\delta( k ) = ( \lfloor h( k ) / m \rfloor \bmod m ) | 1 \f$, odd,
 *   for \f$ m \f$ a power of two, so that the step is coprime with \f$ m \f$
 *
 * or, beyond the book, the triangular inspection
 * \f$ H( k , i ) = ( H_0( k ) + i ( i + 1 ) / 2 ) \bmod m \f$, a permutation
 * when \f$ m \f$ is a power of two, and the double hashing whose step comes
 * from a second function \f$ g \f$ of the hash,
 * \f$ H_\delta( k ) = 1 + ( g( h( k ) ) \bmod ( m - 1 ) ) \f$ for \f$ m \f$
 * prime and \f$ ( g( h( k ) ) \bmod m ) | 1 \f$ for \f$ m \f$ a power of two,
 * and the pseudo-random inspection
 * \f$ H( k , i ) = ( H_0( k ) + p_i ) \bmod m \f$, where
 * \f$ p_0 = 0 , p_1 , \ldots , p_{m-1} \f$ is a fixed random permutation of
 * \f$ 0 , \ldots , m - 1 \f$, drawn from a seed for each size \f$ m \f$, so
 * that it works with every \f$ m \f$ and two keys with different
 * \f$ H_0 \f$ do not share the rest of their inspection as in the linear
 * one (no primary clustering)
 *
 * [see `Probing`], with \f$ H_0( k ) = h( k ) \bmod m \f$ and \f$ h \f$ the
 * hash of the keys. A place is nil (never used), full or deleted: the
 * deleted places, left by `remove()`, do not stop an inspection but are
 * reused by `insert()`. The scan of the book stops at the key, at a nil
 * place or after \f$ m \f$ inspections, and remembers the first deleted place
 * met [see `scan()`].
 *
 * With the load factor \f$ \alpha = n / m \f$, the expected number of
 * places accessed by an unsuccessful search \f$ I( \alpha ) \f$ and by a
 * successful one \f$ S( \alpha ) \f$ are, under uniform hashing,
 * \f[
 *   \begin{array}{lll}
 *     \mbox{linear} & \frac{1}{2} \left( 1 + \frac{1}{( 1 - \alpha )^2}
 *     \right) & \frac{1}{2} \left( 1 + \frac{1}{1 - \alpha} \right) \cr
 *     \mbox{double} & \frac{1}{1 - \alpha} & - \frac{1}{\alpha}
 *     \ln( 1 - \alpha )
 *   \end{array}
 * \f]
 * and `probes()` gives the number of places that the last operation
 * examined, the nil that ends an unsuccessful search included.
 *
 * The table may restructure itself (section 7.4): it doubles \f$ m \f$ when
 * the full and deleted places are more than \f$ m / 2 \f$, so that
 * \f$ \alpha \leq 1 / 2 \f$ always and the deleted places go away, and
 * halves it, down to the initial size, when \f$ \alpha < 1 / 8 \f$. The
 * two thresholds are 4 times apart, not twice as in the example of the
 * book (0.5 and 0.25): with the latter the table just doubled has
 * \f$ \alpha \approx 1 / 4 \f$, a couple of removals halve it back to
 * \f$ \alpha \approx 1 / 2 \f$, a single insertion doubles it again, and
 * a sequence alternating them costs \f$ O( m ) \f$ per operation [see
 * `TableSize.h`]. A new size that the inspection does not work with, a
 * prime that divides the step of a linear one, is replaced by the next
 * prime that it works with. Since only a removal halves \f$ m \f$, a table
 * emptied may stay above its initial size.
 *
 * The contracts [see `Contract.h`]: \f$ m > 0 \f$ and an inspection that
 * works with \f$ m \f$ (a linear step coprime with \f$ m \f$, a prime
 * \f$ m \f$ for the quadratic one, a power of two for the triangular one, a
 * prime or a power of two for the double hashing, any \f$ m \f$ for the
 * pseudo-random one), a full place for `keyAt()`; after `insert()` the key
 * is in the table and after `remove()` it is not; the invariant is that the
 * first place of the inspection of each key that holds it is its own place,
 * with no nil before it, and that `size()` and the deleted places are
 * counted right. On a table of more than 1024 places the invariant is
 * checked on the inspection of the key of the operation only, up to its
 * first nil, so that a long sequence of operations at the level 3 stays
 * linear; the checks do not change `probes()`. An insertion whose
 * inspection finds no place, which a quadratic inspection allows while some
 * place is free, is not a fault of the caller and throws
 * `std::overflow_error`. The operations are defined in `OpenHashTable.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.2-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (the pseudo-random and the
 *      quadratic inspections)
 * \see W. W. Peterson, Addressing for random-access storage, IBM Journal
 *      of Research and Development 1(2), 130-146, 1957,
 *      doi:10.1147/rd.12.0130
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

#ifndef __ADS_OpenHashTable
 #define __ADS_OpenHashTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/TableSize.h"
#include "ADS/numeric/RandomNumbers.h"

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Probing ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a sequence of inspection \f$ H( k , i ) \f$ of an `OpenHashTable`
class Probing {

/*--------------------------------------------------------------------------*/
 public:

 /// the kind of the inspection
 enum class Kind {
  LINEAR ,       ///< \f$ ( H_0( k ) + \delta i ) \bmod m \f$
  QUADRATIC ,    ///< \f$ ( H_0( k ) + c_1 i + c_2 i^2 ) \bmod m \f$
  DOUBLE ,       ///< \f$ ( H_0( k ) + H_\delta( k ) i ) \bmod m \f$
  TRIANGULAR ,   ///< \f$ ( H_0( k ) + i ( i + 1 ) / 2 ) \bmod m \f$
  RANDOM         ///< \f$ ( H_0( k ) + p_i ) \bmod m \f$
 };

 /// the second function of the double hashing, of the hash \f$ h( k ) \f$
 using Step = std::function< std::size_t( std::size_t ) >;

 /// \f$ H( k , i ) = ( H_0( k ) + \delta i ) \bmod m \f$
 static Probing linear( long delta = 1 );

 /// \f$ H( k , i ) = ( H_0( k ) + c_1 i + c_2 i^2 ) \bmod m \f$,
/// \f$ m \f$ prime
 static Probing quadratic( long c1 , long c2 );

 /// \f$ H( k , i ) = ( H_0( k ) + H_\delta( k ) i ) \bmod m \f$
 static Probing double_hashing( void );

 /// \f$ H( k , i ) = ( H_0( k ) + H_\delta( k ) i ) \bmod m \f$, the step
/// from \f$ g( h( k ) ) \f$
 static Probing double_hashing( Step g );

 /// \f$ H( k , i ) = ( H_0( k ) + i ( i + 1 ) / 2 ) \bmod m \f$, \f$ m \f$ a
/// power of two
 static Probing triangular( void );

 /// \f$ H( k , i ) = ( H_0( k ) + p_i ) \bmod m \f$, \f$ p \f$ a random
 /// permutation of \f$ 0 , \ldots , m - 1 \f$ with \f$ p_0 = 0 \f$, drawn
 /// from the seed for each \f$ m \f$
 static Probing pseudo_random( unsigned long seed = 1 );

 /// the kind of the inspection
 [[nodiscard]] Kind kind( void ) const;

 /// the place \f$ H( k , i ) \f$ in a table of size \f$ m \f$, from
 /// \f$ h = h( k ) \f$
 /// \pre \f$ m > 0 \f$
 /// \post the result is below \f$ m \f$
 [[nodiscard]] std::size_t at( std::size_t h , std::size_t i ,
                               std::size_t m ) const;

 /// the step \f$ H_\delta( k ) \f$ of the double hashing, coprime with
 /// \f$ m \f$ when \f$ m \f$ is prime or a power of two
 /// \pre \f$ m > 0 \f$
 [[nodiscard]] static std::size_t step( std::size_t h , std::size_t m );

 /// the constant difference \f$ H( k , i + 1 ) - H( k , i ) \bmod m \f$ of
 /// the linear inspection and of the double hashing, so that a scan adds it
 /// instead of computing \f$ H( k , i ) \f$; 0 for the other inspections
 /// \pre \f$ m > 0 \f$
 [[nodiscard]] std::size_t stride( std::size_t h , std::size_t m ) const;

 /// whether the sequence can work with a table of size \f$ m \f$
 [[nodiscard]] bool works( std::size_t m ) const;

 /// checks that the sequence works with \f$ m \f$
 /// \pre `works( m )`
 void check( std::size_t m ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the inspection of the kind \f$ k \f$ with the constants
 /// \f$ c_1 \f$ and \f$ c_2 \f$
 Probing( Kind k , long c1 , long c2 );

 /// \f$ c \bmod m \f$, in \f$ [ 0 , m ) \f$ also for \f$ c < 0 \f$
 static std::size_t mod( long c , std::size_t m );

 /// the permutation \f$ p \f$ of the pseudo-random inspection for the size
/// \f$ m \f$
 const std::vector< std::size_t > & permutation( std::size_t m ) const;

 Kind f_kind;                ///< the kind
 long f_c1;                  ///< \f$ \delta \f$ or \f$ c_1 \f$
 long f_c2;                  ///< \f$ c_2 \f$
 Step f_g;   ///< the second function of the double hashing, if any
 unsigned long f_seed = 0;   ///< the seed of the pseudo-random inspection
 mutable std::vector< std::size_t > f_p;   ///< its permutation, for `f_m`
 mutable std::size_t f_m = 0;              ///< the size of `f_p`

 };  // end( class Probing )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OpenHashTable ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with open addressing
template< class K , class V , class H = std::hash< K > >
class OpenHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the state of a place
 enum class Slot {
 NIL ,      ///< never used: it stops an inspection
 FULL ,     ///< it holds a pair
 DELETED    ///< emptied by `remove()`: an inspection goes past it
 };

 /// the empty table of size m with the given inspection, which
 /// restructures itself or not
 /// \pre \f$ m > 0 \f$ and `probe.works( m )`
 OpenHashTable( std::size_t m , Probing probe , bool restructure = false ,
                H h = H() );

/*--------------------------------------------------------------------------*/

 /// the value of the key \f$ k \f$, or nothing
 ///
 /// In pseudocode (section 7.2.2), \f$ T[ 0 .. m - 1 ] \f$ the places; the
 /// scan of the book stops at the key, at a nil place or after \f$ m \f$
 /// inspections, and for an insertion remembers the first deleted place:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{scan}( k , \mathit{insert} ) \cr
 /// 2 & \quad d \gets \textbf{nil};\ i \gets 0;\ j \gets H( k , 0 ) \cr
 /// 3 & \quad \textbf{while}\ i < m\ \textbf{and}\ T[ j ] \ne
 ///     \text{nil}\ \textbf{and not}\ ( T[ j ] \text{ full with key } k )
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ T[ j ] \text{ is deleted }\ \textbf{and}\ d =
 ///     \textbf{nil}\ \textbf{then}\ d \gets j \cr
 /// 5 & \qquad i \gets i + 1;\ j \gets H( k , i ) \cr
 /// 6 & \quad \textbf{if}\ \mathit{insert} \ \textbf{and not}\ ( T[ j ]
 ///     \text{ } \text{full with key } k )\ \textbf{and}\ d \ne \textbf{nil}
 ///     \ \textbf{then return}\ d \cr
 /// 7 & \quad \textbf{return}\ j \cr
 /// 8 & \textbf{function}\ \mathrm{lookup}( k ) \cr
 /// 9 & \quad j \gets \mathrm{scan}( k , \textbf{false} ) \cr
 /// 10 & \quad \textbf{if}\ T[ j ] \text{ is full with key } k
 ///     \ \textbf{then return}\ T[ j ].\mathit{value}\ \textbf{else return
 ///     nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$; throws
 /// `std::overflow_error` if the inspection finds no place
 ///
 /// In pseudocode (sections 7.2.2 and 7.4), the table doubling when its
 /// full and deleted places pass \f$ m / 2 \f$, if it restructures itself:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad j \gets \mathrm{scan}( k , \textbf{true} ) \cr
 /// 3 & \quad \textbf{if}\ T[ j ] \text{ is full with key } k\ \textbf{then}
 ///     \ T[ j ].\mathit{value} \gets v;\ \textbf{return} \cr
 /// 4 & \quad \textbf{if}\ T[ j ] \text{ is full}\ \textbf{then}
 ///     \ \text{throw an overflow} \cr
 /// 5 & \quad T[ j ] \gets ( k , v ) \text{, full};\ n \gets n + 1 \cr
 /// 6 & \quad \textbf{if}\ \text{the full and deleted places exceed } m / 2
 ///     \ \textbf{then}\ \text{rebuild on } \mathrm{resized}( m )
 ///     \text{ places}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is in the table
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, leaving a deleted place, by
 /// `scan( k , false )` of `lookup()`; the table, if it restructures itself,
 /// halves below the load \f$ 1 / 8 \f$
 /// \post \f$ k \f$ is not in the table
 void remove( const K & k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the size \f$ m \f$ of the vector
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor \f$ n / m \f$
 [[nodiscard]] double load( void ) const;

 /// the foreach of the book: `f( k , v )` on the pairs, place after place,
 /// \f$ O( m ) \f$ time
 template< class F >
 void forEach( F && f ) const;

 /// the state of the place \f$ i \f$
 /// \pre \f$ i \f$ is less than `capacity()`
 [[nodiscard]] Slot slot( std::size_t i ) const;

 /// the key in the place \f$ i \f$
 /// \pre \f$ i \f$ is less than `capacity()` and the place is full
 [[nodiscard]] const K & keyAt( std::size_t i ) const;

 /// the places examined by the last operation
 [[nodiscard]] std::size_t probes( void ) const;

 /// the number of restructurings so far
 [[nodiscard]] std::size_t restructures( void ) const;

 /// the number of pairs moved by the restructurings so far
 [[nodiscard]] std::size_t moved( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places, bidirectional; it skips the empty and the deleted places, and a
 // rebuild after an insertion or a removal invalidates it

 /// the places of the table, for `SlotIterator`
 struct Places {
  /// the number of places
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  /// whether the place \f$ i \f$ is full
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ].slot == Slot::FULL );
   }
  /// the pair of the place \f$ i \f$
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< const K & , const V & >( o.f_T[ i ].key ,
                                              o.f_T[ i ].value ) );
   }
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< const K & , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = std::pair< const K & , const V & >;
 /// a constant reference to an item
 using const_reference = std::pair< const K & , const V & >;
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< OpenHashTable , Places >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  Slot slot = Slot::NIL;          ///< the state
  K key{};                        ///< the key
  V value{};                      ///< its value
  };

 /// whether the place \f$ i \f$ is full with the key \f$ k \f$
 [[nodiscard]] bool is( std::size_t i , const K & k ) const;

 /// `scan( k , insert )` of the book: the place of \f$ k \f$, or the one
/// where the
 /// inspection stopped (nil, or the last after m inspections); for an
 /// insertion of a key that is not there, the first deleted place met
 std::size_t scan( const K & k , bool insert ) const;

 /// the first place of the inspection of \f$ k \f$ that holds it, with no
 /// nil before it, or `capacity()`; `probes()` does not change
 [[nodiscard]] std::size_t where( const K & k ) const;

 /// each key found by its inspection in its own place, and the full and
 /// deleted places counted right, \f$ O( m ) \f$ inspections; on more than
 /// 1024 places, only the keys on the inspection of \f$ k \f$, up to its
/// first nil
 [[nodiscard]] bool consistent( const K & k ) const;

 /// every pair moved to a table of \f$ m \f$ places, with no deleted ones
void rebuild( std::size_t m );

 std::vector< Cell > f_T;          ///< the places
 Probing f_probe;                  ///< the inspection
 std::size_t f_m0;                 ///< the initial size
 std::size_t f_linear = 0;         ///< the stride of a linear inspection
 bool f_restructure;               ///< whether it restructures
 H f_h;                            ///< the hash of the keys
 std::size_t f_size = 0;           ///< the full places
 std::size_t f_deleted = 0;        ///< the deleted places
 /// the places of the last inspection, which the const lookups write too:
 /// atomic, so that lookups from several threads at once do not race
 struct Probes {
  std::atomic< std::size_t > n{ 0 };   ///< the places
  /// none
  Probes( void ) = default;
  /// a copy of the count of `o`
  Probes( const Probes & o ) : n( o.n.load( std::memory_order_relaxed ) ) {}
  /// the count of `o`
  Probes & operator=( const Probes & o ) {
   n.store( o.n.load( std::memory_order_relaxed ) ,
            std::memory_order_relaxed );
   return( *this );
   }
  };
 mutable Probes f_probes;          ///< see `probes()`
 std::size_t f_rebuilds = 0;       ///< see `restructures()`
 std::size_t f_moved = 0;          ///< see `moved()`

 };  // end( class OpenHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/OpenHashTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* OpenHashTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File OpenHashTable.h ------------------------*/
/*--------------------------------------------------------------------------*/
