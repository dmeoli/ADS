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
 * The places come from the allocator `A`, which may give, e.g., huge pages.
 *
 * Two keys are equal by `eq( p , key )`, with `p` a pointer of the table;
 * by default `p->state == key`, the canonical node of a state of a search.
 *
 * The contracts [see `Contract.h`]: `make()` returns a pointer that is not
 * nil; the invariant is that the load factor stays below \f$ 7 / 10 \f$ and
 * that each object is reached from the first place of its hash with no
 * empty place in between (checked on the key of the operation). The
 * operations are defined in `PointerTable.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (linear probing, the
 *      multiplicative hashing)
 * \see E. Burns, M. Hatem, M. J. Leighton, W. Ruml, Implementing fast
 *      heuristic search code, Proceedings of the 5th Symposium on
 *      Combinatorial Search (SoCS), 25-32, 2012
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

#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the default equality of `PointerTable`: `p->state == key`
struct StateEqual {
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
class PointerTable {

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

 /// room for \f$ n \f$ objects with no growth
 void reserve( std::size_t n );

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
 std::pair< T * , bool > find_or_insert( const K & key ,
                                         std::uint64_t hash , Make && make );

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
 // bidirectional; the key of an object must not change through it, and an
 // insertion that grows the table invalidates it

 /// the places of the table, for `SlotIterator`
 struct Places {
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
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< PointerTable , Places >;
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

 /// a place: the hash and the object, nil if the place is empty
 struct Slot {
  std::uint64_t hash = 0;         ///< the hash of the key of the object
  T * p = nullptr;                ///< the object, nil if empty
  };

 using SA = typename std::allocator_traits< A >::template
            rebind_alloc< Slot >;

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

 std::vector< Slot , SA > f_S;   ///< the places
 std::size_t f_mask = 0;         ///< places - 1
 int f_bits = 0;                 ///< log2 of the places
 std::size_t f_size = 0;         ///< the number of objects
 Eq f_eq;                        ///< the equality of a key and an object

 };  // end( class PointerTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PointerTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PointerTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PointerTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
