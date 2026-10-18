/*--------------------------------------------------------------------------*/
/*-------------------------- File OpenHashTable.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash table with open addressing of section 7.2.2, a realization of
 * the DICTIONARY [see ADT.h].
 *
 * The table keeps the pairs in the vector itself (open addressing): the
 * key k goes into the first free place of its sequence of inspection
 * \f$ H( k , 0 ) , H( k , 1 ) , \ldots , H( k , m - 1 ) \f$, which is
 *
 * - linear, \f$ H( k , i ) = ( H_0( k ) + \delta i ) \bmod m \f$, a
 *   permutation of the places when \f$ \gcd( \delta , m ) = 1 \f$;
 * - quadratic, \f$ H( k , i ) = ( H_0( k ) + c_1 i + c_2 i^2 ) \bmod m \f$
 *   with m prime, which in general is not a permutation, so an insertion
 *   may fail while some place is still free;
 * - double hashing, \f$ H( k , i ) = ( H_0( k ) + H_\delta( k ) i ) \bmod m
 *   \f$, with \f$ H_\delta( k ) = 1 + ( h( k ) \bmod ( m - 1 ) ) \f$ for m
 *   prime and \f$ H_\delta( k ) = ( \lfloor h( k ) / m \rfloor \bmod m ) |
 *   1 \f$, odd, for m a power of two, so that the step is coprime with m
 *
 * or, beyond the book, the triangular inspection \f$ H( k , i ) = ( H_0( k )
 * + i ( i + 1 ) / 2 ) \bmod m \f$, a permutation when m is a power of two,
 * and the double hashing whose step comes from a second function g of the
 * hash, \f$ H_\delta( k ) = 1 + ( g( h( k ) ) \bmod ( m - 1 ) ) \f$ for m
 * prime and \f$ ( g( h( k ) ) \bmod m ) | 1 \f$ for m a power of two, and
 * the pseudo-random inspection \f$ H( k , i ) = ( H_0( k ) + p_i ) \bmod m
 * \f$, where \f$ p_0 = 0 , p_1 , \ldots , p_{m-1} \f$ is a fixed random
 * permutation of \f$ 0 , \ldots , m - 1 \f$, drawn from a seed for each
 * size m, so that it works with every m and two keys with different
 * \f$ H_0 \f$ do not share the rest of their inspection as in the linear
 * one (no primary clustering)
 *
 * [see Probing], with \f$ H_0( k ) = h( k ) \bmod m \f$ and h the hash of the
 * keys. A place is nil (never used), full or deleted: the deleted places,
 * left by remove(), do not stop an inspection but are reused by insert().
 * The scan of the book stops at the key, at a nil place or after m
 * inspections, and remembers the first deleted place met [see scan()].
 *
 * With the load factor \f$ \alpha = n / m \f$, the expected number of
 * places accessed by an unsuccessful search I( \alpha ) and by a
 * successful one S( \alpha ) are, under uniform hashing,
 * \f[
 *   \begin{array}{lll}
 *     \mbox{linear} & \frac{1}{2} \left( 1 + \frac{1}{( 1 - \alpha )^2}
 *     \right) & \frac{1}{2} \left( 1 + \frac{1}{1 - \alpha} \right) \\
 *     \mbox{double} & \frac{1}{1 - \alpha} & - \frac{1}{\alpha}
 *     \ln( 1 - \alpha )
 *   \end{array}
 * \f]
 * and probes() gives the number of places that the last operation
 * examined, the nil that ends an unsuccessful search included.
 *
 * The table may restructure itself (section 7.4): it doubles m when the
 * full and deleted places are more than m / 2, so that
 * \f$ \alpha \leq 1 / 2 \f$ always and the deleted places go away, and
 * halves it, down to the initial size, when \f$ \alpha < 1 / 8 \f$. The
 * two thresholds are 4 times apart, not twice as in the example of the
 * book (0.5 and 0.25): with the latter the table just doubled has
 * \f$ \alpha \approx 1 / 4 \f$, a couple of removals halve it back to
 * \f$ \alpha \approx 1 / 2 \f$, a single insertion doubles it again, and
 * a sequence alternating them costs O( m ) per operation [see
 * TableSize.h].
 *
 * The contracts [see Contract.h]: m > 0 and an inspection that works with m (a
 * linear step coprime with m, a prime m for the quadratic one, a power of two
 * for the triangular one, a prime or a power of two for the double hashing,
 * any m for the pseudo-random one), a full place for keyAt(); after insert()
 * the key is in the table and after remove() it is not; the invariant is that
 * the first place of the inspection of each key that holds it is its own
 * place, with no nil before it, and that size() and the deleted places are
 * counted right. On a table of more than 1024 places the invariant is checked
 * on the inspection of the key of the operation only, up to its first nil, so
 * that a long sequence of operations at the level 3 stays linear; the checks
 * do not change probes(). An insertion whose inspection finds no place, which
 * a quadratic inspection allows while some place is free, is not a fault of
 * the caller and throws std::overflow_error. The operations are defined in
 * OpenHashTable.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.2-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (the pseudo-random and the
 *      quadratic inspections)
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 6.4
 * \see W. W. Peterson, Addressing for random-access storage, IBM Journal
 *      of Research and Development 1(2), 130-146, 1957,
 *      doi:10.1147/rd.12.0130
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

#ifndef __ADS_OpenHashTable
 #define __ADS_OpenHashTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/TableSize.h"

#include <algorithm>
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

/// a sequence of inspection H( k , i ) of an OpenHashTable
class Probing {

/*--------------------------------------------------------------------------*/
 public:

 enum class Kind { LINEAR , QUADRATIC , DOUBLE , TRIANGULAR , RANDOM };

 /// the second function of the double hashing, of the hash h( k )
 using Step = std::function< std::size_t( std::size_t ) >;

 /// H( k , i ) = ( H0( k ) + delta i ) mod m
 static Probing linear( long delta = 1 );

 /// H( k , i ) = ( H0( k ) + c1 i + c2 i^2 ) mod m, m prime
 static Probing quadratic( long c1 , long c2 );

 /// H( k , i ) = ( H0( k ) + Hdelta( k ) i ) mod m
 static Probing double_hashing( void );

 /// H( k , i ) = ( H0( k ) + Hdelta( k ) i ) mod m, the step from g( h( k ) )
 static Probing double_hashing( Step g );

 /// H( k , i ) = ( H0( k ) + i ( i + 1 ) / 2 ) mod m, m a power of two
 static Probing triangular( void );

 /// H( k , i ) = ( H0( k ) + p_i ) mod m, p a random permutation of
 /// 0 , ... , m - 1 with p_0 = 0, drawn from the seed for each m
 static Probing pseudo_random( unsigned long seed = 1 );

 [[nodiscard]] Kind kind( void ) const;

 /// the place H( k , i ) in a table of size m, from h = h( k )
 /// \pre m > 0
 /// \post the result is below m
 [[nodiscard]] std::size_t at( std::size_t h , std::size_t i ,
                               std::size_t m ) const;

 /// the step Hdelta( k ) of the double hashing, coprime with m when m is
 /// prime or a power of two
 /// \pre m > 0
 [[nodiscard]] static std::size_t step( std::size_t h , std::size_t m );

 /// whether the sequence can work with a table of size m
 [[nodiscard]] bool works( std::size_t m ) const;

 /// \pre works( m )
 void check( std::size_t m ) const;

/*--------------------------------------------------------------------------*/
 private:

 Probing( Kind k , long c1 , long c2 );

 static std::size_t mod( long c , std::size_t m );

 /// the permutation p of the pseudo-random inspection for the size m
 const std::vector< std::size_t > & permutation( std::size_t m ) const;

 Kind f_kind;
 long f_c1;
 long f_c2;
 Step f_g;   ///< the second function of the double hashing, if any
 unsigned long f_seed = 0;   ///< the seed of the pseudo-random inspection
 mutable std::vector< std::size_t > f_p;   ///< its permutation, for f_m
 mutable std::size_t f_m = 0;              ///< the size of f_p

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
 enum class Slot { NIL , FULL , DELETED };

 /// the empty table of size m with the given inspection, which
 /// restructures itself or not
 /// \pre m > 0 and probe.works( m )
 OpenHashTable( std::size_t m , Probing probe , bool restructure = false ,
                H h = H() );

/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value v to the key k; throws std::overflow_error if the
 /// inspection finds no place
 /// \post k is in the table
 void insert( const K & k , const V & v );

 /// removes the key k, if it is there, leaving a deleted place
 /// \post k is not in the table
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 /// the size m of the vector
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor n / m
 [[nodiscard]] double load( void ) const;

 /// the foreach of the book: f( k , v ) on the pairs, place after place,
 /// O( m ) time
 template< class F >
 void forEach( F && f ) const;

 /// the state of the place i
 /// \pre i < capacity()
 [[nodiscard]] Slot slot( std::size_t i ) const;

 /// the key in the place i
 /// \pre i < capacity() and the place is full
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

 /// the places of the table, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ].slot == Slot::FULL );
   }
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< const K & , const V & >( o.f_T[ i ].key ,
                                              o.f_T[ i ].value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::pair< const K & , const V & >;
 using const_reference = std::pair< const K & , const V & >;
 using const_iterator = SlotIterator< OpenHashTable , Places >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  Slot slot = Slot::NIL;
  K key{};
  V value{};
  };

 [[nodiscard]] bool is( std::size_t i , const K & k ) const;

 /// scan( k , insert ) of the book: the place of k, or the one where the
 /// inspection stopped (nil, or the last after m inspections); for an
 /// insertion of a key that is not there, the first deleted place met
 std::size_t scan( const K & k , bool insert ) const;

 /// the first place of the inspection of k that holds it, with no nil
 /// before it, or capacity(); probes() does not change
 [[nodiscard]] std::size_t where( const K & k ) const;

 /// each key found by its inspection in its own place, and the full and
 /// deleted places counted right, O( m ) inspections; on more than 1024
 /// places, only the keys on the inspection of k, up to its first nil
 [[nodiscard]] bool consistent( const K & k ) const;

 void rebuild( std::size_t m );

 std::vector< Cell > f_T;          ///< the places
 Probing f_probe;                  ///< the inspection
 std::size_t f_m0;                 ///< the initial size
 bool f_restructure;               ///< whether it restructures
 H f_h;                            ///< the hash of the keys
 std::size_t f_size = 0;           ///< the full places
 std::size_t f_deleted = 0;        ///< the deleted places
 mutable std::size_t f_probes = 0; ///< see probes()
 std::size_t f_rebuilds = 0;       ///< see restructures()
 std::size_t f_moved = 0;          ///< see moved()

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
