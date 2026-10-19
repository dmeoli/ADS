/*--------------------------------------------------------------------------*/
/*-------------------------- File FlatHashTable.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table with open addressing and control bytes, beyond the book: the
 * engineered DICTIONARY of ADS [see ADT.h], the one that the benchmark of
 * the dictionaries compares with boost::unordered_flat_map, while
 * OpenHashTable [see OpenHashTable.h] stays the table of section 7.2.2.
 *
 * Beside the vector of the pairs, the table keeps one control byte per
 * place: empty (0x80) or full, in which case the byte is a fingerprint of
 * 7 bits of the hash of its key. The places come in groups
 * of 8, whose 8 control bytes are read as one word of 64 bits and compared
 * with the fingerprint of a key all at once by broadword operations (SWAR,
 * SIMD within a register), with no instructions beyond the arithmetic of
 * the words; a key is compared only in the places whose fingerprint
 * matches, a false match costing a comparison and nothing else. The hash
 * h( k ) is multiplied by \f$ \lfloor 2^{64} / \phi \rfloor \f$ (the
 * multiplicative hashing of Knuth): the top bits of the product give the
 * first group of the key and the low 7 bits of the product XOR its bits
 * 32 to 38 give the fingerprint, so that even the identity hash of the
 * integers of std::hash spreads over the table. The number of places is a
 * power of 2, at least 8, and the groups are inspected in the triangular
 * order \f$ g_i = ( g_0 + i ( i + 1 ) / 2 ) \bmod G \f$, a permutation of
 * the G groups.
 *
 * Each group has also a byte of overflow, after the Boost table, kept right
 * after its 8 control bytes, so that a search reads the 9 bytes from one line
 * of cache in most cases: an insertion that goes past a full group sets in it
 * the bit of its key, one of 8 chosen by the bits 39 to 41 of the product. A
 * search stops at the first group whose overflow lacks the bit of its key,
 * after the places of that group whose fingerprint matches: a search for an
 * absent key reads one word of control bytes and one byte of overflow in the
 * common case and never touches the pairs, which is why it costs a fraction of
 * the search in a table that keeps its states with the pairs. remove() empties
 * the place, with no deleted marks; but a place emptied in a group whose
 * overflow is not 0 still counts as full, since the bits stay set until the
 * next rebuild and the searches keep going past the group. An insertion takes
 * the first empty place of the inspection of its key. The full places and
 * those still counted stay at most 7 / 8 of the places: past it the table is
 * rebuilt, twice as large if the pairs exceed 7 / 16 of the places, of the
 * same size otherwise (only the bits of overflow and the places counted go
 * away), which costs O( 1 ) amortized per insertion; so some place is always
 * empty, and each inspection ends, since it visits each group at most once.
 *
 * The iterators visit the pairs in the order of the places, as constant
 * pairs of references ( key , value ); find() gives a value to change in
 * place. A removal moves no other pair and keeps the iterators valid but
 * the one on the removed pair (erase( it ) returns the next one), an
 * insertion of a new key that rebuilds the table invalidates all of them,
 * and an insertion of a key already there replaces its value in place.
 *
 * The contracts [see Contract.h]: after insert() the key is in the table
 * and after remove() it is not; the invariant is that each full place is
 * reached by the inspection of its key, the bit of the key set in the
 * overflow of each group before it, that the control bytes match the
 * count of the pairs, and that the full places and those still counted
 * stay at most 7 / 8 of the places. On a table of more than 1024 places
 * the invariant is checked on the inspection of the key of the operation
 * only, up to its first group whose overflow lacks the bit of the key.
 * The operations are defined in FlatHashTable.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 6.4 (open
 *      addressing, the multiplicative hashing)
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A,
 *      Combinatorial Algorithms, Part 1, Addison-Wesley, 2011, section
 *      7.1.3 (the broadword computing)
 * \see M. Kulukundis, Designing a fast, efficient, cache-friendly hash
 *      table, step by step, CppCon 2017 (the Swiss tables of Abseil, the
 *      control bytes and the groups)
 * \see J. M. Lopez Munoz, Inside boost::unordered_flat_map, 2022 (the
 *      bytes of overflow),
 *      https://bannalia.blogspot.com/2022/11/inside-boostunorderedflatmap.html
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

#ifndef __ADS_FlatHashTable
 #define __ADS_FlatHashTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FlatHashTable ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with open addressing and control bytes in groups of 8
template< class K , class V , class H = std::hash< K > ,
          class Eq = std::equal_to< K > >
class FlatHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty table, with room for n pairs before it grows
 explicit FlatHashTable( std::size_t n = 0 , H h = H() , Eq eq = Eq() );

 FlatHashTable( const FlatHashTable & o );

 /// leaves o empty, with no places
 FlatHashTable( FlatHashTable && o ) noexcept;

 FlatHashTable & operator=( FlatHashTable o ) noexcept;

 ~FlatHashTable();

/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of the key k, nil if it is not there
 [[nodiscard]] const V * find( const K & k ) const;

 /// the value of the key k, to change in place, nil if it is not there
 [[nodiscard]] V * find( const K & k );

 /// associates the value v to the key k; true if k was not there
 /// \post k is in the table
 bool insert( const K & k , const V & v );

 /// as insert( k , v ), moving the key and the value
 bool insert( K && k , V && v );

 /// removes the key k, if it is there, emptying its place; true if it was
 /// \post k is not in the table
 bool remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 /// the number of places, 0 or a power of 2 at least 8
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor n / m, 0 with no places
 [[nodiscard]] double load( void ) const;

 /// the places emptied by remove() in groups that some inspection went
 /// past, which count as full until the table is rebuilt
 [[nodiscard]] std::size_t deleted( void ) const;

 /// room for n pairs before the table grows
 void reserve( std::size_t n );

 /// removes all the pairs, keeping the places
 void clear( void );

 /// f( k , v ) on the pairs, place after place
 template< class F >
 void forEach( F && f ) const;

 void swap( FlatHashTable & o ) noexcept;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places, bidirectional; a removal invalidates only the iterators on the
 // removed pair, an insertion that rebuilds the table all of them

 /// the places of the table, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_cap ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_ctrl[ cb( i ) ] < EMPTY );
   }
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< const K & , const V & >( o.f_slot[ i ].kv.first ,
                                              o.f_slot[ i ].kv.second ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::pair< const K & , const V & >;
 using const_reference = std::pair< const K & , const V & >;
 using const_iterator = SlotIterator< FlatHashTable , Places >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_cap ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// removes the pair of it, and returns the iterator on the next one
 /// \pre it is on a pair of the table
 const_iterator erase( const_iterator it );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::uint8_t EMPTY = 0x80;     ///< a place never used
 static constexpr std::size_t GROUP = 8;         ///< the places of a group

 /// a place, whose pair lives only while its control byte is full
 union Slot {
  Slot( void ) {}
  ~Slot() {}
  std::pair< K , V > kv;
  };

 /// the group of a place-less table, its control bytes empty and its byte
 /// of overflow 0, never written
 alignas( 8 ) inline static std::uint8_t s_none[ GROUP + 1 ] = {
  EMPTY , EMPTY , EMPTY , EMPTY , EMPTY , EMPTY , EMPTY , EMPTY , 0 };

 /// the hash of k times floor( 2^64 / phi )
 [[nodiscard]] std::uint64_t mix( const K & k ) const;

 /// the fingerprint of a mixed hash, below 0x80
 [[nodiscard]] static std::uint8_t tag( std::uint64_t x );

 /// the bit of a mixed hash in the bytes of overflow, one of 8
 [[nodiscard]] static std::uint8_t bit( std::uint64_t x );

 /// the first group of a mixed hash
 [[nodiscard]] std::size_t home( std::uint64_t x ) const;

 /// the control bytes of the group g [see word()]
 [[nodiscard]] std::uint64_t group( std::size_t g ) const;

 /// the 8 control bytes from c, the byte c[ j ] in the bits 8 j to
 /// 8 j + 7 on any machine
 [[nodiscard]] static std::uint64_t word( const std::uint8_t * c );

 /// the index in f_ctrl of the control byte of the place i
 [[nodiscard]] static constexpr std::size_t cb( std::size_t i );

 /// the index in f_ctrl of the byte of overflow of the group g
 [[nodiscard]] static constexpr std::size_t ob( std::size_t g );

 /// the bit 8 j + 7 for each place j of the word w whose byte is t, and
 /// maybe for a place right after one of those (a false match)
 [[nodiscard]] static std::uint64_t matches( std::uint64_t w ,
                                             std::uint8_t t );

 /// the bit 8 j + 7 for each empty place j of the word w (exact)
 [[nodiscard]] static std::uint64_t empties( std::uint64_t w );

 /// the place j of the lowest bit of a mask of matches
 [[nodiscard]] static std::size_t lowest( std::uint64_t m );

 /// the place of k, whose mixed hash is x, nil if k is not there
 [[nodiscard]] Slot * locate( std::uint64_t x , const K & k ) const;

 /// the first empty place of the inspection of x, the bit of x set in the
 /// overflow of the full groups before it
 std::size_t claim( std::uint64_t x );

 /// the insertion of a pair, the key and the value forwarded
 template< class KK , class VV >
 bool put( KK && k , VV && v );

 /// the table rebuilt for one more pair, and the place where the pair kv
 /// of mixed hash x goes, built there; kv is a copy, since the key or the
 /// value inserted may live in the table that moves
 std::size_t grow( std::uint64_t x , std::pair< K , V > && kv );

 /// empties the place i, whose pair is destroyed
 void drop( std::size_t i );

 /// the table rebuilt with m places, a power of 2 at least 8
 void rebuild( std::size_t m );

 /// the places for n pairs, 0 or a power of 2 at least 8
 [[nodiscard]] static std::size_t places( std::size_t n );

 /// destroys the pairs and frees the places
 void release( void ) noexcept;

 /// each pair found by the inspection of its key, the count right and at
 /// most 7 / 8 of the places full or deleted; on more than 1024 places
 /// only the pairs on the inspection of the mixed hash x, up to its first
 /// group whose overflow lacks the bit of x
 [[nodiscard]] bool consistent( std::uint64_t x ) const;

 std::uint8_t * f_ctrl = s_none;   ///< the groups of 8 control bytes and
                                   ///< 1 byte of overflow
 Slot * f_slot = nullptr;          ///< the places
 std::size_t f_cap = 0;            ///< the number of places
 std::size_t f_mask = 0;           ///< the groups - 1
 int f_shift = 63;                 ///< 64 - log2 of the groups, at most 63
 std::size_t f_size = 0;           ///< the full places
 std::size_t f_deleted = 0;        ///< see deleted()
 H f_h;                            ///< the hash of the keys
 Eq f_eq;                          ///< the equality of the keys

 };  // end( class FlatHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/FlatHashTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FlatHashTable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FlatHashTable.h ------------------------*/
/*--------------------------------------------------------------------------*/
