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
 * prime and \f$ ( g( h( k ) ) \bmod m ) | 1 \f$ for m a power of two
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
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.2-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
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

#include "ADS/dictionary/TableSize.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
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

 enum class Kind { LINEAR , QUADRATIC , DOUBLE , TRIANGULAR };

 /// the second function of the double hashing, of the hash h( k )
 using Step = std::function< std::size_t( std::size_t ) >;

 /// H( k , i ) = ( H0( k ) + delta i ) mod m
 static Probing linear( long delta = 1 ) {
  return( Probing( Kind::LINEAR , delta , 0 ) );
  }

 /// H( k , i ) = ( H0( k ) + c1 i + c2 i^2 ) mod m, m prime
 static Probing quadratic( long c1 , long c2 ) {
  return( Probing( Kind::QUADRATIC , c1 , c2 ) );
  }

 /// H( k , i ) = ( H0( k ) + Hdelta( k ) i ) mod m
 static Probing double_hashing( void ) {
  return( Probing( Kind::DOUBLE , 0 , 0 ) );
  }

 /// H( k , i ) = ( H0( k ) + Hdelta( k ) i ) mod m, the step from g( h( k ) )
 static Probing double_hashing( Step g ) {
  Probing p( Kind::DOUBLE , 0 , 0 );
  p.f_g = std::move( g );
  return( p );
  }

 /// H( k , i ) = ( H0( k ) + i ( i + 1 ) / 2 ) mod m, m a power of two
 static Probing triangular( void ) {
  return( Probing( Kind::TRIANGULAR , 0 , 0 ) );
  }

 [[nodiscard]] Kind kind( void ) const { return( f_kind ); }

 /// the place H( k , i ) in a table of size m, from h = h( k )
 [[nodiscard]] std::size_t at( std::size_t h , std::size_t i ,
                               std::size_t m ) const {
  const std::size_t h0 = h % m;
  switch( f_kind ) {
   case( Kind::LINEAR ):
    return( ( h0 + mod( f_c1 , m ) * ( i % m ) ) % m );
   case( Kind::QUADRATIC ): {
    const std::size_t j = i % m;
    return( ( h0 + mod( f_c1 , m ) * j + mod( f_c2 , m ) * ( j * j % m ) )
            % m );
    }
   case( Kind::TRIANGULAR ): {
    const std::size_t j = i % ( 2 * m );
    return( ( h0 + j * ( j + 1 ) / 2 % m ) % m );
    }
   default:
    return( ( h0 + step( f_g ? f_g( h ) : h , m ) * ( i % m ) ) % m );
   }
  }

 /// the step Hdelta( k ) of the double hashing, coprime with m when m is
 /// prime or a power of two
 [[nodiscard]] static std::size_t step( std::size_t h , std::size_t m ) {
  if( m == 1 )
   return( 1 );
  if( is_pow2( m ) )
   return( ( ( h / m ) % m ) | 1 );
  return( 1 + h % ( m - 1 ) );
  }

 /// throws std::invalid_argument if the sequence cannot work with m
 void check( std::size_t m ) const {
  if( ( f_kind == Kind::LINEAR ) &&
      ( std::gcd( mod( f_c1 , m ) , m ) != 1 ) )
   throw( std::invalid_argument( "ADS::Probing: gcd( delta , m ) != 1" ) );
  if( ( f_kind == Kind::QUADRATIC ) && ! is_prime( m ) )
   throw( std::invalid_argument( "ADS::Probing: quadratic, m not prime" ) );
  if( ( f_kind == Kind::TRIANGULAR ) && ! is_pow2( m ) )
   throw( std::invalid_argument(
                        "ADS::Probing: triangular, m not a power of 2" ) );
  if( ( f_kind == Kind::DOUBLE ) && ( m > 1 ) && ! is_prime( m ) &&
      ! is_pow2( m ) )
   throw( std::invalid_argument(
                     "ADS::Probing: double, m not prime nor a power of 2" ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 Probing( Kind k , long c1 , long c2 ) : f_kind( k ) , f_c1( c1 ) ,
                                         f_c2( c2 ) {}

 static std::size_t mod( long c , std::size_t m ) {
  const long r = c % long( m );
  return( r < 0 ? r + m : r );
  }

 Kind f_kind;
 long f_c1;
 long f_c2;
 Step f_g;   ///< the second function of the double hashing, if any

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

 /// the empty table of size m > 0 with the given inspection, which
 /// restructures itself or not
 OpenHashTable( std::size_t m , Probing probe , bool restructure = false ,
                H h = H() )
  : f_T( m ) , f_probe( probe ) , f_m0( m ) ,
    f_restructure( restructure ) , f_h( h ) {
  if( m == 0 )
   throw( std::invalid_argument( "ADS::OpenHashTable: m = 0" ) );
  f_probe.check( m );
  }

/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const {
  const auto i = scan( k , false );
  if( is( i , k ) )
   return( f_T[ i ].value );
  return( std::nullopt );
  }

 /// associates the value v to the key k; throws std::overflow_error if the
 /// inspection finds no place
 void insert( const K & k , const V & v ) {
  const auto i = scan( k , true );
  auto & c = f_T[ i ];
  if( c.slot == Slot::FULL && ! ( c.key == k ) )
   throw( std::overflow_error( "ADS::OpenHashTable::insert: no place" ) );
  if( c.slot != Slot::FULL ) {
   if( c.slot == Slot::DELETED )
    --f_deleted;
   ++f_size;
   }
  c = { Slot::FULL , k , v };
  if( f_restructure && ( 2 * ( f_size + f_deleted ) > f_T.size() ) )
   rebuild( resized( f_T.size() , true , f_m0 ) );
  }

 /// removes the key k, if it is there, leaving a deleted place
 void remove( const K & k ) {
  const auto i = scan( k , false );
  if( ! is( i , k ) )
   return;
  f_T[ i ].slot = Slot::DELETED;
  --f_size;
  ++f_deleted;
  if( f_restructure && ( f_T.size() > f_m0 ) &&
      ( 8 * f_size < f_T.size() ) )
   rebuild( resized( f_T.size() , false , f_m0 ) );
  }

 [[nodiscard]] bool contains( const K & k ) const {
  return( is( scan( k , false ) , k ) );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// the size m of the vector
 [[nodiscard]] std::size_t capacity( void ) const { return( f_T.size() ); }

 /// the load factor n / m
 [[nodiscard]] double load( void ) const {
  return( double( f_size ) / f_T.size() );
  }

 /// the foreach of the book: f( k , v ) on the pairs, place after place,
 /// O( m ) time
 template< class F >
 void forEach( F && f ) const {
  for( const auto & c : f_T )
   if( c.slot == Slot::FULL )
    f( c.key , c.value );
  }

 /// the state of the place i
 [[nodiscard]] Slot slot( std::size_t i ) const { return( f_T.at( i ).slot ); }

 /// the key in the place i, which is full
 [[nodiscard]] const K & keyAt( std::size_t i ) const {
  if( f_T.at( i ).slot != Slot::FULL )
   throw( std::logic_error( "ADS::OpenHashTable::keyAt: not full" ) );
  return( f_T[ i ].key );
  }

 /// the places examined by the last operation
 [[nodiscard]] std::size_t probes( void ) const { return( f_probes ); }

 /// the number of restructurings so far
 [[nodiscard]] std::size_t restructures( void ) const { return( f_rebuilds ); }

 /// the number of pairs moved by the restructurings so far
 [[nodiscard]] std::size_t moved( void ) const { return( f_moved ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  Slot slot = Slot::NIL;
  K key{};
  V value{};
  };

 bool is( std::size_t i , const K & k ) const {
  return( ( f_T[ i ].slot == Slot::FULL ) && ( f_T[ i ].key == k ) );
  }

 /// scan( k , insert ) of the book: the place of k, or the one where the
 /// inspection stopped (nil, or the last after m inspections); for an
 /// insertion of a key that is not there, the first deleted place met
 std::size_t scan( const K & k , bool insert ) const {
  const std::size_t m = f_T.size();
  const std::size_t h = f_h( k );
  long firstDeleted = -1;
  std::size_t i = 0;
  std::size_t j = f_probe.at( h , 0 , m );
  while( ! is( j , k ) && ( f_T[ j ].slot != Slot::NIL ) && ( i < m ) ) {
   if( ( f_T[ j ].slot == Slot::DELETED ) && ( firstDeleted == -1 ) )
    firstDeleted = j;
   j = f_probe.at( h , ++i , m );
   }
  f_probes = std::min( i + 1 , m );
  if( insert && ! is( j , k ) && ( firstDeleted != -1 ) )
   return( firstDeleted );
  return( j );
  }

 void rebuild( std::size_t m ) {
  f_probe.check( m );
  std::vector< Cell > T( m );
  std::swap( f_T , T );
  f_size = f_deleted = 0;
  for( auto & c : T )
   if( c.slot == Slot::FULL ) {
    f_T[ scan( c.key , true ) ] = c;
    ++f_size;
    }
  ++f_rebuilds;
  f_moved += f_size;
  }

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

#endif  /* OpenHashTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File OpenHashTable.h ------------------------*/
/*--------------------------------------------------------------------------*/
