/*--------------------------------------------------------------------------*/
/*-------------------------- File PointerTable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table of pointers with open addressing, beyond the book: the set of
 * the objects of a search (its closed list), each found from its key, e.g.,
 * the state of a node. A place holds the pointer and the whole hash of 64
 * bits of the key of its object, so that a probe compares two keys only
 * when their hashes are equal and a growth moves the places without
 * touching the objects; the inspection is linear [see OpenHashTable.h],
 * i.e., along consecutive places and lines of cache, which beats the lists
 * of overflow when the table does not fit in the cache, and the first
 * place of a hash is its top bits after the product by
 * \f$ \lfloor 2^{64} / \phi \rfloor \f$ [see fibonacciPlace()], so that
 * hashes whose low bits are regular (e.g., the owner of a state in a
 * parallel search, h mod p) still spread over the table. The load factor
 * stays below 7 / 10, the table doubling past it. There is no removal,
 * which a search does not need and which would cost the simplicity of the
 * probes. find_or_insert( key , hash , make ) inserts the object that
 * make() returns only if the key is not there, with one inspection;
 * prefetch( hash ) asks for the first place of a hash, so that the misses
 * of the lookups of all the successors of a node overlap. The places come
 * from the allocator A, which may give, e.g., huge pages.
 *
 * Two keys are equal by eq( p , key ), with p a pointer of the table; by
 * default p->state == key, the canonical node of a state of a search.
 *
 * The contracts [see Contract.h]: make() returns a pointer that is not nil;
 * the invariant is that the load factor stays below 7 / 10 and that each
 * object is reached from the first place of its hash with no empty place
 * in between (checked on the key of the operation). The operations are
 * defined in PointerTable.tpp.
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
 *         Universita' di Pisa \n
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

/// the default equality of PointerTable: p->state == key
struct StateEqual {
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

 /// a table of at least capacity places, a power of 2, at least 16
 explicit PointerTable( std::size_t capacity = 16 , Eq eq = Eq() ,
                        const A & alloc = A() );

 PointerTable( const PointerTable & ) = default;
 PointerTable & operator=( const PointerTable & ) = default;

 /// the table of o, which is left empty, with no places until an insertion
 PointerTable( PointerTable && o ) noexcept;
 PointerTable & operator=( PointerTable && o ) noexcept;

 /// no object, the places kept; the objects are not touched
 void clear( void );

 /// room for n objects with no growth
 void reserve( std::size_t n );

 /// the object of the key, whose hash is hash, and false; if there is
 /// none, the object that make() returns, now in the table, and true
 /// \post the result is not nil
 template< class K , class Make >
 std::pair< T * , bool > find_or_insert( const K & key ,
                                         std::uint64_t hash , Make && make );

 /// the object of the key, nil if there is none
 template< class K >
 [[nodiscard]] T * find( const K & key , std::uint64_t hash ) const;

 /// brings in the cache the first place of hash
 void prefetch( std::uint64_t hash ) const;

 /// the number of objects
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of places
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the memory of the places, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

 /// f( p ) on each object, in the order of the places
 template< class F >
 void forEach( F && f ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a view of the pointers to the objects, in the order of the places,
 // bidirectional; the key of an object must not change through it, and an
 // insertion that grows the table invalidates it

 /// the places of the table, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_S.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_S[ i ].p != nullptr );
   }
  static auto read( const auto & o , std::size_t i ) {
   return( o.f_S[ i ].p );
   }
  };

 using value_type = T *;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T *;
 using const_reference = T *;
 using const_iterator = SlotIterator< PointerTable , Places >;
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

 /// a place: the hash and the object, nil if the place is empty
 struct Slot {
  std::uint64_t hash = 0;
  T * p = nullptr;
  };

 using SA = typename std::allocator_traits< A >::template
            rebind_alloc< Slot >;

 /// the first place of a hash
 [[nodiscard]] std::size_t place( std::uint64_t hash ) const;

 /// m empty places, a power of 2
 void reset( std::size_t m );

 /// twice the places
 void grow( void );

 /// the object of the key is reached from the first place of hash with no
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
