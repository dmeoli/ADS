/*--------------------------------------------------------------------------*/
/*-------------------------- File PointerTable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table of pointers with open addressing, beyond the book: the set of
 * the objects of a search (its closed list), each found from its key, e.g.,
 * the state of a node. A place holds the pointer and the whole hash of 64
 * bits of the key of its object, so that a probe compares two keys only
 * when their hashes are equal and a growth moves the places without
 * touching the objects; the inspection is linear [see `OpenHashTable.h`],
 * i.e., along consecutive places and lines of cache, which beats the lists
 * of overflow when the table does not fit in the cache, and the first
 * place of a hash is its top bits after the product by
 * \f$ \lfloor 2^{64} / \phi \rfloor \f$ [see `fibonacciPlace()`], so that
 * hashes whose low bits are regular (e.g., the owner of a state in a
 * parallel search, \f$ h \bmod p \f$) still spread over the table. The load
 * factor stays below \f$ 7 / 10 \f$, the table doubling past it. There is no
 * removal, which a search does not need and which would cost the
 * simplicity of the probes. `find_or_insert( key , hash , make )` inserts
 * the object that `make()` returns only if the key is not there, with one
 * inspection; `prefetch( hash )` asks for the first place of a hash, so
 * that the misses of the lookups of all the successors of a node overlap.
 * The places come from the allocator `A`, which may give, e.g., huge pages
 * [see `HugePageAllocator.h`], and `slotsFor( n )` tells how many places
 * \f$ n \f$ objects take, so that a table built with them, or reserved by
 * `reserve( n )`, never grows on the way.
 *
 * Two keys are equal by `eq( p , key )`, with `p` a pointer of the table;
 * by default `p->state == key`, the canonical node of a state of a search.
 *
 * With `ADS_WITH_BOOST` defined (by the option of CMake of the same name,
 * which defines it whenever the headers of Boost 1.83 or later are found, by
 * the makefile, which does the same, or by hand), the places are those of
 * `boost::unordered_flat_set`, which on the benchmark of the dictionaries
 * inserts and finds 2 to 3 times faster than the linear inspection above,
 * and `ADS_POINTERTABLE_BOOST` defined to 0 keeps the code of ADS. The
 * interface stays the same, the members and their signatures, the
 * equality `Eq`, the allocator `A` (rebound to the places, e.g., huge
 * pages for Boost too) and the constructor from the number of places, with
 * four differences. A place holds the hash and the pointer as above, but
 * the first place of a hash and the inspection are those of Boost (groups
 * of 15 places with a byte of control each, the hash mixed again by a
 * product, since it does not declare `is_avalanching`). The load factor
 * stays below \f$ 7 / 8 \f$, the maximum load of Boost (all the places
 * up to 29 of them), and a table built with \f$ m \f$ places has room for
 * \f$ 7 m / 10 \f$ objects with no growth, as many as the places of ADS
 * hold, so that a table of `slotsFor( n )` places still takes \f$ n \f$
 * objects with no growth, in about as many bytes; `capacity()` is the
 * number of places of Boost, \f$ 15 \f$ times a power of 2, less 1, and
 * `reserve( n )` leaves room for \f$ n \f$ objects as Boost computes it.
 * Finally, `prefetch()` does nothing, since Boost has no way to ask for the
 * group of a hash, and the iterator is forward, the one of Boost.
 *
 * The contracts [see `Contract.h`]: `make()` returns a pointer that is not
 * nil; the invariant is that the load factor stays below \f$ 7 / 10 \f$ and
 * that each object is reached from the first place of its hash with no
 * empty place in between (checked on the key of the operation), and with
 * Boost that the object of the key is found again. The operations are
 * defined in `PointerTable.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (linear probing, the
 *      multiplicative hashing)
 * \see E. Burns, M. Hatem, M. J. Leighton, W. Ruml, Implementing fast
 *      heuristic search code, Proceedings of the 5th Symposium on
 *      Combinatorial Search (SoCS), 25-32, 2012,
 *      doi:10.1609/socs.v3i1.18245
 * \see J. M. Lopez Munoz, Inside boost::unordered_flat_map, 2022 (the
 *      groups of Boost),
 *      https://bannalia.blogspot.com/2022/11/inside-boostunorderedflatmap.html
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

#ifndef __ADS_PointerTable
 #define __ADS_PointerTable
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

// the places of boost::unordered_flat_set, with ADS_WITH_BOOST
#if defined( ADS_WITH_BOOST ) && ! defined( ADS_POINTERTABLE_BOOST )
 #define ADS_POINTERTABLE_BOOST 1
#endif
#ifndef ADS_POINTERTABLE_BOOST
 #define ADS_POINTERTABLE_BOOST 0
#endif
#if ADS_POINTERTABLE_BOOST
 #include <iterator>

 #include <boost/unordered/unordered_flat_set.hpp>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the default equality of `PointerTable`: `p->state == key`
struct StateEqual
{
 /// whether the state of the object \f$ p \f$ is `key`
 template< class P , class K >
 bool operator()( const P * p , const K & key ) const {
  return( p->state == key );
  }
 };

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PointerTable ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table of pointers to objects found by their keys
template< class T , class Eq = StateEqual ,
	  class A = std::allocator< std::byte > >
class PointerTable
{

/*--------------------------------------------------------------------------*/
 public:
 /// a table of at least `capacity` places, a power of 2, at least 16
 explicit PointerTable( std::size_t capacity = 16 , Eq eq = Eq() ,
			const A & alloc = A() );

 /// a copy of the places, the objects shared
 PointerTable( const PointerTable & ) = default;

 /// a copy of the places, the objects shared
 PointerTable & operator=( const PointerTable & ) = default;

 /// the table of `o`, which is left empty, with no places until an
 /// insertion
 PointerTable( PointerTable && o ) noexcept;

 /// the table of `o`, which is left empty, as the constructor above
 PointerTable & operator=( PointerTable && o ) noexcept;

 /// no object, the places kept; the objects are not touched
 void clear( void );

 /// room for \f$ n \f$ objects with no growth, i.e., at least
 /// `slotsFor( n )` places
 void reserve( std::size_t n );

 /// the places that \f$ n \f$ objects need with no growth: the least
 /// power of 2, at least 16, above \f$ 10 n / 7 \f$, so that a table
 /// built with it, or reserved for \f$ n \f$, takes them all
 [[nodiscard]] static constexpr std::size_t slotsFor( std::size_t n ) {
  std::size_t m = 16;
  while( n * 10 > m * 7 )
   m *= 2;
  return( m );
  }

 /// the object of the key, whose hash is `hash`, and false; if there is
 /// none, the object that `make()` returns, now in the table, and true
 ///
 /// In pseudocode (the linear inspection of section 6.4 of TAOCP, beyond the
 /// book), \f$ S[ 0 .. m - 1 ] \f$ the places, \f$ m \f$ a power of 2, each
 /// with a hash and a pointer:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findOrInsert}( \mathit{key} , h ,
 ///     \mathit{make} ) \cr
 /// 2 & \quad i \gets \mathrm{fibonacciPlace}( h , \log_2 m ) \cr
 /// 3 & \quad \textbf{while}\ S[ i ].p \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ S[ i ].h = h\ \textbf{and}\ \mathrm{eq}( S[ i ].p
 ///     , \mathit{key} )\ \textbf{then return}\ ( S[ i ].p , \textbf{false} )
 ///     \cr
 /// 5 & \qquad i \gets ( i + 1 ) \bmod m \cr
 /// 6 & \quad S[ i ] \gets ( h , \mathit{make}() );\ n \gets n + 1 \cr
 /// 7 & \quad \textbf{if}\ 10 n > 7 m\ \textbf{then}\ \text{double the }
 ///     \text{places, each reinserted from the first place of its hash} \cr
 /// 8 & \quad \textbf{return}\ ( S[ i ].p , \textbf{true} )
 /// \end{array}}
 /// \f]
 /// \post the result is not nil
 template< class K , class Make >
 std::pair< T * , bool > find_or_insert( const K & key , std::uint64_t hash ,
					Make && make );

 /// the object of the key, nil if there is none: the inspection of
 /// `find_or_insert()` up to the first empty place
 template< class K >
 [[nodiscard]] T * find( const K & key , std::uint64_t hash ) const;

 /// brings in the cache the first place of `hash`
 void prefetch( std::uint64_t hash ) const;

 /// the number of objects
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of places
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the memory of the places, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

 /// `f( p )` on each object, in the order of the places
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a view of the pointers to the objects, in the order of the places,
 // bidirectional (forward with Boost); the key of an object must not change
 // through it, and an insertion that grows the table invalidates it

#if ! ADS_POINTERTABLE_BOOST
 /// the places of the table, for `SlotIterator`
 struct Places
 {
  /// the number of places
  static std::size_t size( const auto & o ) { return( o.f_S.size() ); }
  /// whether the place \f$ i \f$ holds an object
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_S[ i ].p != nullptr );
   }
  /// the pointer of the place \f$ i \f$
  static auto read( const auto & o , std::size_t i ) {
   return( o.f_S[ i ].p );
   }
  };
#endif

 /// the type of the items, for the standard library
 using value_type = T *;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = T *;
 /// a constant reference to an item
 using const_reference = T *;
#if ! ADS_POINTERTABLE_BOOST
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< PointerTable , Places >;
#else
 /// the iterator on the items, constant: the one of Boost on the places
 class const_iterator;
#endif
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const;
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const;
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:
 /// a place: the hash and the object, nil if the place is empty
 struct Slot
 {
  std::uint64_t hash = 0;  ///< the hash of the key of the object
  T * p = nullptr;         ///< the object, nil if empty
  };

#if ! ADS_POINTERTABLE_BOOST
 using SA = typename std::allocator_traits< A >::template rebind_alloc< Slot >;

 /// the first place of a hash
 [[nodiscard]] std::size_t place( std::uint64_t hash ) const;

 /// \f$ m \f$ empty places, a power of 2
 void reset( std::size_t m );

 /// twice the places
 void grow( void );

 /// the object of the key is reached from the first place of `hash` with no
 /// empty place before it
 template< class K >
 [[nodiscard]] bool reached( const K & key , std::uint64_t hash ) const;

 std::vector< Slot , SA > f_S;  ///< the places
 std::size_t f_mask = 0;       ///< places - 1
 int f_bits = 0;               ///< log2 of the places
 std::size_t f_size = 0;       ///< the number of objects
 Eq f_eq;                      ///< the equality of a key and an object
#else
 /// a key, its hash and the maker of its object, the argument of the
 /// searches and of the insertions in the set of Boost
 template< class K , class Make >
 struct Probe
 {
  const K & key;       ///< the key
  std::uint64_t hash;  ///< its hash
  Make * make;         ///< the maker of the object, nil for a search
  };

 /// a place of the set of Boost: a `Slot` built from a `Probe` by its
 /// maker, so that the object is made only if the key is not there
 struct Entry : Slot
 {
  /// the place of the hash and of the object that `make()` returns
  template< class K , class Make >
  explicit Entry( const Probe< K , Make > & q );
  };

 /// the hash of a place or of a probe: the one given, which Boost mixes
 struct Hash
 {
  using is_transparent = void;  ///< probes compared to the places
  /// the hash of the place
  std::size_t operator()( const Entry & e ) const {
   return( std::size_t( e.hash ) );
   }
  /// the hash of the probe
  template< class K , class Make >
  std::size_t operator()( const Probe< K , Make > & q ) const {
   return( std::size_t( q.hash ) );
   }
  };

 /// the equality of two places, or of a probe and a place: the hashes
 /// first, then `Eq` on the object and the key
 struct Equal
 {
  using is_transparent = void;  ///< probes compared to the places
  /// on the equality `e` of a key and an object
  explicit Equal( Eq e ) : eq( std::move( e ) ) { }
  Eq eq;  ///< the equality of a key and an object
  /// the same object
  bool operator()( const Entry & a , const Entry & b ) const {
   return( a.p == b.p );
   }
  /// the key of the probe is that of the object of the place
  template< class K , class Make >
  bool operator()( const Probe< K , Make > & q , const Entry & e ) const {
   return( ( e.hash == q.hash ) && eq( e.p , q.key ) );
   }
  /// the same, the operands exchanged
  template< class K , class Make >
  bool operator()( const Entry & e , const Probe< K , Make > & q ) const {
   return( ( *this )( q , e ) );
   }
  };

 using SA =
   typename std::allocator_traits< A >::template rebind_alloc< Entry >;
 /// the set of Boost
 using Set = boost::unordered_flat_set< Entry , Hash , Equal , SA >;

 Set f_set;  ///< the places
#endif

 };  // end( class PointerTable )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS PointerTable::const_iterator ------------------*/
/*--------------------------------------------------------------------------*/

#if ADS_POINTERTABLE_BOOST
/// the iterator of the set of Boost, on the pointers of its places: forward
template< class T , class Eq , class A >
class PointerTable< T , Eq , A >::const_iterator
{
 public:
 using value_type = T *;                  ///< the items
 using reference = T *;                   ///< what `*it` gives
 using difference_type = std::ptrdiff_t;  ///< the distances
 /// forward for the concepts of C++20
 using iterator_concept = std::forward_iterator_tag;
 /// input for the older algorithms, as `*it` is not a reference
 using iterator_category = std::input_iterator_tag;

 /// an iterator on no table
 const_iterator( void ) = default;

 /// the pointer of the place
 reference operator*( void ) const { return( f_i->p ); }

 /// to the next place
 const_iterator & operator++( void ) {
  ++f_i;
  return( *this );
  }

 /// to the next place, returning the iterator before the step
 const_iterator operator++( int ) {
  auto t = *this;
  ++f_i;
  return( t );
  }

 /// equal on the same place
 bool operator==( const const_iterator & o ) const { return( f_i == o.f_i ); }

 private:
 friend class PointerTable;

 /// on the place of `i`
 explicit const_iterator( typename Set::const_iterator i ) : f_i( i ) { }

 typename Set::const_iterator f_i;  ///< the place
 };  // end( class PointerTable::const_iterator )
#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PointerTable.tpp"

/*--------------------------------------------------------------------------*/

#endif /* PointerTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PointerTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
