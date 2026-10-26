/*--------------------------------------------------------------------------*/
/*--------------------------- File DenseHashMap.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash map whose pairs lie in a dense vector, beyond the book: the
 * pairs ( key , value ) are the elements \f$ 0 , \ldots , n - 1 \f$ of a
 * vector, in the order of their insertions up to the removals, and a
 * separate index of open addressing maps a key to the position of its pair
 * in the vector. A visit of the pairs is then a scan of n contiguous
 * pairs, with no empty place in between, and the index is rebuilt from the
 * vector without moving a pair; the price is one more access per search
 * (the index, then the pair) and iterators that an insertion or a removal
 * may invalidate, as for a vector [see FlatHashTable.h for the table whose
 * pairs never move].
 *
 * The index is made of groups of one line of cache, 64 bytes: 16 control
 * bytes, of which the first 12 are those of the 12 places of the group,
 * empty (0) or full, in which case the byte is a fingerprint of 8 bits of
 * the hash of the key, never 0, and the 16th is a byte of overflow, and 12
 * positions of 32 bits in the vector. The control bytes are compared with
 * the fingerprint of a key all at once, by SSE2, NEON or broadword
 * operations as in FlatHashTable, and the positions of the places that
 * match are in the same line, so that a search costs one miss in the index
 * and one in the vector. The hash is mixed as in FlatHashTable: the top
 * bits give the first group, the low 8 bits the fingerprint and the bits 8
 * to 10 the bit of the key in the bytes of overflow; the G groups, a power
 * of 2, are inspected in the triangular order \f$ g_i = ( g_0 + i ( i + 1 )
 * / 2 ) \bmod G \f$, an insertion that goes past a full group sets in its
 * byte of overflow the bit of its key, and a search stops at the first
 * group whose overflow lacks the bit of its key.
 *
 * A removal empties the place of the key in the index and moves the last
 * pair of the vector into the position left free (a swap with the last
 * and a pop), so that the vector stays dense: the place of the last pair in
 * the index is found by the inspection of its key, comparing the positions
 * and not the keys, and takes the new position. The place emptied is
 * written as a whole group, at an address that depends only on the hash,
 * so that the loads of the next operations need not wait for the control
 * bytes [see FlatHashTable.h]. A place emptied in a group with some
 * overflow still counts as full until the next rebuild; the places counted
 * are bounded by one more per removal and counted exactly only when the
 * full places and the bound reach \f$ 7 m / 8 \f$ of the m places, the
 * index being rebuilt if fewer than \f$ m / 16 \f$ places are left, twice
 * as large if the pairs exceed half of \f$ 7 m / 8 \f$. A rebuild reads the
 * keys in the vector in order, hashes them again and fills the new groups,
 * each in the order of its places; so each operation costs \f$ O( 1 ) \f$
 * expected amortized time, with the expected number of groups inspected
 * by a search bounded by a constant for a load at most 7 / 8 and a hash
 * that behaves as random.
 *
 * Space: \f$ 16 n \f$ bytes for pairs of 16 bytes, plus 64 bytes per 12
 * places of the index, about \f$ 5.3 \f$ bytes per place, at a load
 * between 7 / 16 and 7 / 8 after the first growth.
 *
 * The iterators are those of the vector of the pairs, constant, in the
 * order of the positions; an insertion of a new key may move the vector
 * and invalidates them, a removal moves the last pair into the position
 * of the removed one, and an insertion of a key already there replaces its
 * value in place. find() gives a value to change in place. The keys and
 * the values must be movable; there are fewer than \f$ 2^{32} \f$ pairs.
 *
 * The contracts [see Contract.h]: after insert() the key is in the map and
 * after remove() it is not; the invariant is that each position of the
 * vector is held by exactly one full place of the index, reached by the
 * inspection of its key, and that the full places and the bound of those
 * counted stay at most 7 / 8 of the places; on an index of more than 1024
 * places, only the inspection of the key of the operation is checked. The
 * operations are defined in DenseHashMap.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 6.4 (open
 *      addressing, the multiplicative hashing)
 * \see M. Kulukundis, Designing a fast, efficient, cache-friendly hash
 *      table, step by step, CppCon 2017 (the control bytes and the groups)
 * \see J. M. Lopez Munoz, Inside boost::unordered_flat_map, 2022 (the
 *      groups with a byte of overflow, the mixing of the hash),
 *      https://bannalia.blogspot.com/2022/11/inside-boostunorderedflatmap.html
 * \see R. Hettinger, More compact dictionaries with faster iteration,
 *      python-dev, 2012, https://mail.python.org/pipermail/python-dev/
 *      2012-December/123028.html (the pairs in a dense vector apart from
 *      an index of positions, the layout of the dictionaries of CPython
 *      3.6)
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

#ifndef __ADS_DenseHashMap
 #define __ADS_DenseHashMap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DenseHashMap ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash map with its pairs in a dense vector and an index of positions
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > >
class DenseHashMap {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty map, with room for n pairs before it grows
 explicit DenseHashMap( std::size_t n = 0 , H h = H() , Eq eq = Eq() );

 DenseHashMap( const DenseHashMap & o );

 /// leaves o empty, with no places
 DenseHashMap( DenseHashMap && o ) noexcept;

 DenseHashMap & operator=( DenseHashMap o ) noexcept;

 ~DenseHashMap() = default;

/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of the key k, nil if it is not there
 [[nodiscard]] const V * find( const K & k ) const;

 /// the value of the key k, to change in place, nil if it is not there
 [[nodiscard]] V * find( const K & k );

 /// associates the value v to the key k; true if k was not there
 /// \post k is in the map
 bool insert( const K & k , const V & v );

 /// as insert( k , v ), moving the key and the value
 bool insert( K && k , V && v );

 /// removes the key k, if it is there, moving the last pair into its
 /// position; true if it was
 /// \post k is not in the map
 bool remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the number of places of the index, 12 times 0 or a power of 2 at
 /// least 2
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor n / m of the index, 0 with no places
 [[nodiscard]] double load( void ) const;

 /// the places of the index emptied by remove() in groups that some
 /// inspection went past, which count as full until the index is
 /// rebuilt; \f$ O( m ) \f$ time, by a scan of the groups
 [[nodiscard]] std::size_t deleted( void ) const;

 /// room for n pairs before the map grows
 void reserve( std::size_t n );

 /// removes all the pairs, keeping the places
 void clear( void );

 /// f( k , v ) on the pairs, in the order of the vector
 template< class F >
 void forEach( F && f ) const;

 /// the pairs, in the order of the vector
 [[nodiscard]] const std::pair< K , V > * data( void ) const;

 void swap( DenseHashMap & o ) noexcept;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs, in the order of the vector, random
 // access; an insertion of a new key or a removal invalidates them

 using value_type = std::pair< K , V >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const std::pair< K , V > &;
 using const_reference = const std::pair< K , V > &;
 using const_iterator =
  typename std::vector< std::pair< K , V > >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( f_kv.cbegin() );
  }
 [[nodiscard]] const_iterator end( void ) const { return( f_kv.cend() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::uint8_t EMPTY = 0;        ///< a place never used
 static constexpr std::size_t SLOTS = 12;        ///< the places of a group
 static constexpr std::size_t OVER = 15;         ///< the byte of overflow
 static constexpr std::uint32_t PLACES = 0xFFF;  ///< the mask of the places

 /// a group, one line of cache: the control bytes of the 12 places, 3
 /// bytes never matched, the byte of overflow and the positions
 struct alignas( 64 ) Group {
  std::uint8_t c[ 16 ];
  std::uint32_t at[ SLOTS ];
  };

 /// the places of a group whose control bytes satisfy a test, the bit j
 /// for the place j
 using Mask = std::uint32_t;

 /// the hash taken as it is, if it declares is_avalanching on 64 bits
 static constexpr bool AVALANCHING = ( sizeof( std::size_t ) >= 8 ) &&
  requires { typename H::is_avalanching; };

 /// the groups of an index with no places, all empty, their overflow 0;
 /// never written
 inline static Group s_none[ 2 ] = {};

 /// the mixed hash of k
 [[nodiscard]] std::uint64_t mix( const K & k ) const;

 /// the bit of a mixed hash in the bytes of overflow, one of 8
 [[nodiscard]] static std::uint8_t bit( std::uint64_t x );

 /// the first group of a mixed hash
 [[nodiscard]] std::size_t home( std::uint64_t x ) const;

 /// the places of the group c whose control byte is the fingerprint t
 /// [see detail::flatTags], exactly
 [[nodiscard]] static Mask match( const Group & c , std::uint32_t t );

 /// the empty places of the group c
 [[nodiscard]] static Mask empties( const Group & c );

 /// the group c with the control byte of its place j EMPTY, written whole
 static void vacate( Group & c , std::size_t j );

 /// the position of k, whose mixed hash is x, in the vector, or -1
 [[nodiscard]] std::size_t locate( std::uint64_t x , const K & k ) const;

 /// a key passed out of line: by value if it is a small trivial type
 using KeyArg = std::conditional_t< std::is_trivially_copyable_v< K > &&
                                    ( sizeof( K ) <= 2 * sizeof( void * ) ) ,
                                    K , const K & >;

 /// the group and the place of k past the first group g of x, whose
 /// overflow has the bit of x; the group -1 if k is not there
 ADS_FLAT_NOINLINE std::pair< std::size_t , std::size_t > further(
                    std::uint64_t x , std::size_t g , KeyArg k ) const;

 /// the first empty place of the inspection of x, the bit of x set in the
 /// overflow of the full groups before it, as group * 16 + place
 std::size_t claim( std::uint64_t x );

 /// the place of the position p, whose key has the mixed hash x
 std::uint32_t & holder( std::uint64_t x , std::size_t p );

 /// the insertion of a pair, the key and the value forwarded
 template< class KK , class VV >
 bool put( KK && k , VV && v );

 /// the place of the position p emptied in the group g, the last pair
 /// moved into p
 void unlink( Group & g , std::size_t j , std::size_t p );

 /// the index rebuilt with G groups, a power of 2 at least 2, from the
 /// vector
 void rebuild( std::size_t G );

 /// f_deleted set to deleted(); true if the index has room for m / 16 more
 /// pairs or places counted before the next recount
 ADS_FLAT_NOINLINE bool recount( void );

 /// the index rebuilt for one more pair, larger if needed
 ADS_FLAT_NOINLINE void grow( void );

 /// the groups for n pairs, 0 or a power of 2 at least 2
 [[nodiscard]] static std::size_t groups( std::size_t n );

 /// at most 7 / 8 of m places, the most that can be full or counted
 [[nodiscard]] static constexpr std::size_t bound( std::size_t m );

 /// each position held by one full place reached by the inspection of its
 /// key, the bound respected; past 1024 places only the inspection of x
 [[nodiscard]] bool consistent( std::uint64_t x ) const;

 /// frees the groups
 struct Free {
  void operator()( Group * g ) const noexcept;
  };

 std::vector< std::pair< K , V > > f_kv;   ///< the pairs, dense
 std::unique_ptr< Group[] , Free > f_g;    ///< the groups, nil if none
 Group * f_ctrl = s_none;          ///< the groups, or s_none
 std::size_t f_mask = 0;           ///< the groups - 1, 0 with none
 std::size_t f_cap = 0;            ///< the places
 int f_shift = 63;                 ///< 64 - log2 of the groups
 std::size_t f_max = 0;            ///< bound( f_cap )
 std::size_t f_deleted = 0;        ///< a bound of deleted()
 H f_h;                            ///< the hash of the keys
 Eq f_eq;                          ///< the equality of the keys

 };  // end( class DenseHashMap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/DenseHashMap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DenseHashMap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DenseHashMap.h -------------------------*/
/*--------------------------------------------------------------------------*/
