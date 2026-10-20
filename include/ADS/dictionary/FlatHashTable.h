/*--------------------------------------------------------------------------*/
/*-------------------------- File FlatHashTable.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A hash table with open addressing and control bytes, beyond the book: the
 * engineered DICTIONARY of ADS [see ADT.h], the one that the benchmark of
 * the dictionaries compares with boost::unordered_flat_map, while
 * OpenHashTable [see OpenHashTable.h] stays the table of section 7.2.2.
 *
 * The places come in groups of 15, after the Boost table: beside the
 * vector of the pairs, each group has 16 bytes aligned to 16, one control
 * byte per place, empty (0) or full, in which case the byte is a
 * fingerprint of 8 bits of the hash of its key, never 0, and a 16th byte
 * of overflow. The 15 control bytes are compared with the fingerprint of a
 * key all at once: by one comparison of SSE2 on x86-64, by one of NEON on
 * ARM64, and elsewhere by broadword operations (SWAR, SIMD within a
 * register) on two words of 64 bits; ADS_FLAT_SIMD, defined as 0 before
 * the header, selects the broadword operations on any machine. A key is
 * compared only in the places whose fingerprint matches, a false match
 * costing a comparison and nothing else. When some fingerprint matches,
 * the first line of cache of the pairs of the group is prefetched, as in
 * Boost: the address depends only on the group, so that the processor
 * issues the prefetch on the predicted branch while the control bytes are
 * still on their way, and the two misses of a search overlap in part.
 *
 * The hash h( k ) is mixed as in Boost: the product of h( k ) by
 * \f$ \lfloor 2^{64} / \phi \rfloor \f$ on 128 bits, its two halves
 * XORed, so that every bit of the result depends on every bit of h( k )
 * and even the identity hash of the integers of std::hash spreads over the
 * table; a hash that declares the member type is_avalanching (the
 * convention of Boost) is taken as it is, on 64 bits, as the default one,
 * DefaultHash< K > [see FastHash.h], is for the integers and the strings,
 * whose hash by FastHash costs one product per 16 bytes. The top bits of the
 * mixed hash give the first group of the key, its low 8 bits the
 * fingerprint (1 in place of 0) and its bits 8 to 10 the bit of the key in
 * the bytes of overflow. The number of groups is a power of 2 and the
 * groups are inspected in the triangular order
 * \f$ g_i = ( g_0 + i ( i + 1 ) / 2 ) \bmod G \f$, a permutation of the G
 * groups.
 *
 * An insertion that goes past a full group sets in its byte of overflow
 * the bit of its key, one of 8. A search stops at the first group whose
 * overflow lacks the bit of its key, after the places of that group whose
 * fingerprint matches: a search for an absent key reads one group of 16
 * bytes, one line of cache, in the common case and never touches the
 * pairs. remove() empties the place, with no deleted marks; but a place
 * emptied in a group whose overflow is not 0 still counts as full, since
 * the bits stay set until the next rebuild and the searches keep going past
 * the group. A removal in the first group of its key writes the 16 control
 * bytes of the group whole, at an address that depends only on the hash:
 * the store of the single byte would have an address known only once the
 * control bytes arrive from memory, and the loads of the next operations
 * could not pass it before then, so that the removals of random keys would
 * pay their misses one after the other instead of overlapping them (on a
 * Xeon Gold 5120, 55 ns against 33 ns per removal from 10^6 keys). For the
 * same reason a removal counts the places it may leave counted by a bound
 * d, one more per removal, with no load: when $ n + d $ reaches the
 * bound of the full places, the places counted are counted exactly, in
 * $ O( m / 15 ) $ time, and the table is rebuilt only if fewer than
 * $ m / 16 $ places are left, so that the next count comes at least
 * $ m / 16 $ operations later and costs O( 1 ) amortized. An insertion
 * takes the first empty place of the inspection of
 * its key. The full places and those still counted stay at most 7 / 8 of
 * the places: past it the table is rebuilt, twice as large if the pairs
 * exceed half of that bound, of the same size otherwise (only the bits of
 * overflow and the places counted go away), which costs O( 1 ) amortized
 * per insertion; the new pair is built in the new places before the old
 * pairs move, so that a key or a value that lives in the table is still
 * there to copy. The rebuild only fills the new places, each group in the
 * order of its places, so that it keeps the count of the full places of
 * each group in a byte apart and never reads the control bytes it has just
 * written (a load of a group right after the store of one of its bytes
 * waits for the store). So some place is always empty, and each inspection
 * ends, since it visits each group at most once. The search of an
 * insertion is the one of find(), whose first group gives the place of a
 * new key when it has an empty one, the common case; the groups after the
 * first, and the rebuild, are out of line.
 *
 * The iterators visit the pairs in the order of the places, as constant
 * pairs of references ( key , value ), a group at a time by the mask of
 * its full places [see const_iterator]; find() gives a value to change in
 * place. A removal moves no other pair and keeps the iterators valid but
 * the one on the removed pair (erase( it ) returns the next one), an
 * insertion of a new key that rebuilds the table invalidates all of them
 * (one that does not rebuild it keeps them, the new pair visited or not),
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
 *      groups of 15 places with a byte of overflow, the mixing of the
 *      hash, the prefetch of the pairs),
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
#include "ADS/dictionary/FastHash.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/// the matching of the 15 control bytes of a group: 1 by SSE2, 2 by NEON,
/// 0 by broadword operations, on any machine; by default the first that
/// the target has, and the same in all the translation units of a program
#ifndef ADS_FLAT_SIMD
 #if defined( __SSE2__ ) || defined( _M_X64 ) || \
     ( defined( _M_IX86_FP ) && ( _M_IX86_FP >= 2 ) )
  #define ADS_FLAT_SIMD 1
 #elif ( defined( __ARM_NEON ) && defined( __AARCH64EL__ ) ) || \
       defined( _M_ARM64 )
  #define ADS_FLAT_SIMD 2
 #else
  #define ADS_FLAT_SIMD 0
 #endif
#endif

#if ADS_FLAT_SIMD == 1
 #include <emmintrin.h>
#elif ADS_FLAT_SIMD == 2
 #include <arm_neon.h>
#endif

#if defined( _MSC_VER ) && ( defined( _M_X64 ) || defined( _M_ARM64 ) )
 #include <intrin.h>
#endif

/// a hint to bring the line of p into the cache, or nothing
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FLAT_PREFETCH( p ) __builtin_prefetch( ( const void * )( p ) )
#elif ( ADS_FLAT_SIMD == 1 ) && defined( _MSC_VER )
 #define ADS_FLAT_PREFETCH( p ) \
  _mm_prefetch( ( const char * )( p ) , _MM_HINT_T0 )
#else
 #define ADS_FLAT_PREFETCH( p ) ( ( void ) 0 )
#endif

/// a function inlined even where the compiler would not
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FLAT_INLINE inline __attribute__(( always_inline ))
#elif defined( _MSC_VER )
 #define ADS_FLAT_INLINE __forceinline
#else
 #define ADS_FLAT_INLINE inline
#endif

/// a function never inlined, the rare path out of a loop
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FLAT_NOINLINE __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_FLAT_NOINLINE __declspec( noinline )
#else
 #define ADS_FLAT_NOINLINE
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the high 64 bits of the product a b on 128 bits XOR its low 64 bits
[[nodiscard]] inline std::uint64_t mulx( std::uint64_t a , std::uint64_t b );

/// mulx( a , b ) by products of 32 bits, on any machine
[[nodiscard]] constexpr std::uint64_t mulxPortable( std::uint64_t a ,
                                                    std::uint64_t b );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FlatHashTable ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with open addressing and control bytes in groups of 15
template< class K , class V , class H = DefaultHash< K > ,
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

 [[nodiscard]] bool empty( void ) const;

 /// the number of places, 15 times 0 or a power of 2 at least 2
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor n / m, 0 with no places
 [[nodiscard]] double load( void ) const;

 /// the places emptied by remove() in groups that some inspection went
 /// past, which count as full until the table is rebuilt; O( m ) time, by
 /// a scan of the groups
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

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::pair< const K & , const V & >;
 using const_reference = std::pair< const K & , const V & >;

 /// the iterator on the full places [see below]
 class const_iterator;

 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const;
 [[nodiscard]] const_iterator end( void ) const;
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// removes the pair of it, and returns the iterator on the next one
 /// \pre it is on a pair of the table
 const_iterator erase( const_iterator it );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::uint8_t EMPTY = 0;        ///< a place never used
 static constexpr std::size_t GROUP = 16;        ///< the bytes of a group
 static constexpr std::size_t SLOTS = 15;        ///< the places of a group
 static constexpr std::size_t OVER = 15;         ///< the byte of overflow

 /// the control bytes of the 15 places of a group and its byte of overflow
 struct alignas( 16 ) Group {
  std::uint8_t c[ GROUP ];
  };

 /// the places of a group whose control bytes satisfy a test: on SSE2 and
 /// by SWAR the bit j for the place j, on NEON the bit 4 j + 3
#if ADS_FLAT_SIMD == 2
 using Mask = std::uint64_t;
 static constexpr int STRIDE = 2;                ///< log2 of the bits of j
#else
 using Mask = std::uint32_t;
 static constexpr int STRIDE = 0;
#endif

 /// the mask of the place 0 alone
 static constexpr Mask FIRST = Mask( 1 ) << ( ( 1 << STRIDE ) - 1 );

 /// a place, whose pair lives only while its control byte is full
 union Slot {
  Slot( void ) {}
  ~Slot() {}
  std::pair< K , V > kv;
  };

 /// the alignment of the slots, at least a line of cache, so that no
 /// slot of 16 , 32 or 64 bytes straddles two lines
 static constexpr std::size_t ALIGN = alignof( Slot ) > 64 ? alignof( Slot )
                                                           : 64;

 /// the G + 1 groups of a table of G groups, aligned to ALIGN, all empty
 /// with their overflow 0 but the last, the sentinel [see const_iterator]
 [[nodiscard]] static Group * control( std::size_t G );

 /// the 15 G slots of G groups, aligned to ALIGN, no pair built
 [[nodiscard]] static Slot * allocate( std::size_t G );

 /// frees the groups of control() and the slots of allocate()
 struct Free {
  void operator()( Group * c ) const noexcept;
  void operator()( Slot * s ) const noexcept;
  };

 /// the hash taken as it is, if it declares is_avalanching on 64 bits
 static constexpr bool AVALANCHING = ( sizeof( std::size_t ) >= 8 ) &&
  requires { typename H::is_avalanching; };

 /// the 2 groups of a place-less table, all empty, their overflow 0, and
 /// its sentinel; never written
 inline static Group s_none[ 3 ] = { {} , {} , { { 1 } } };

 /// the mixed hash of k [see the file]
 [[nodiscard]] std::uint64_t mix( const K & k ) const;

 /// the fingerprint of a mixed hash, never EMPTY, in each of 4 bytes
 [[nodiscard]] static std::uint32_t tag( std::uint64_t x );

 /// the bit of a mixed hash in the bytes of overflow, one of 8
 [[nodiscard]] static std::uint8_t bit( std::uint64_t x );

 /// the first group of a mixed hash
 [[nodiscard]] std::size_t home( std::uint64_t x ) const;

 /// the places of the group c whose control byte is the fingerprint t
 /// [see tag()], exactly
 [[nodiscard]] static Mask match( const Group & c , std::uint32_t t );

 /// the empty places of the group c
 [[nodiscard]] static Mask empties( const Group & c );

 /// the full places of the group c
 [[nodiscard]] static Mask fulls( const Group & c );

 /// the place j of the lowest bit of a mask
 [[nodiscard]] static std::size_t lowest( Mask m );

#if ADS_FLAT_SIMD == 0
 /// the 8 bytes from c, the byte c[ j ] in the bits 8 j to 8 j + 7
 [[nodiscard]] static std::uint64_t word( const std::uint8_t * c );

 /// the bit 8 j + 7 for each byte j of w that is 0, exactly
 [[nodiscard]] static std::uint64_t zeros( std::uint64_t w );

 /// the bits 8 j + 7 of z gathered in the bits j
 [[nodiscard]] static Mask gather( std::uint64_t z );
#endif

 /// the control byte of the place i, the place j of the group i / 16
 [[nodiscard]] std::uint8_t & ctrl( std::size_t i ) const;

 /// the slot of the place i
 [[nodiscard]] Slot * slot( std::size_t i ) const;

 /// the place of k, whose mixed hash is x, nil if k is not there
 [[nodiscard]] Slot * locate( std::uint64_t x , const K & k ) const;

 /// the place of k among the places m of the group g, those whose
 /// fingerprint matches, nil if k is not there
 [[nodiscard]] Slot * scan( std::size_t g , Mask m , const K & k ) const;

 /// a key passed out of line: by value if it is a small trivial type, so
 /// that the common path never stores it to memory to take its address
 using KeyArg = std::conditional_t< std::is_trivially_copyable_v< K > &&
                                    ( sizeof( K ) <= 2 * sizeof( void * ) ) ,
                                    K , const K & >;

 /// locate( x , k ) past the first group g of x, whose overflow has the
 /// bit of x
 ADS_FLAT_NOINLINE Slot * further( std::uint64_t x , std::size_t g ,
                                   KeyArg k ) const;

 /// the first empty place of the inspection of x, the bit of x set in the
 /// overflow of the full groups before it
 std::size_t claim( std::uint64_t x );

 /// the insertion of a pair, the key and the value forwarded
 template< class KK , class VV >
 bool put( KK && k , VV && v );

 /// the table rebuilt for one more pair, the pair ( k , v ) of mixed hash
 /// x built in the new places before the old pairs move; its place
 template< class KK , class VV >
 ADS_FLAT_NOINLINE std::size_t grow( std::uint64_t x , KK && k , VV && v );

 /// empties the place i, whose pair is destroyed
 void drop( std::size_t i );

 /// f_deleted set to deleted(); true if the table has room for f_cap / 16
 /// more pairs or places counted before the next recount, false if it is
 /// to be rebuilt
 ADS_FLAT_NOINLINE bool recount( void );

 /// the group c with the control byte of its place j EMPTY, written whole
 /// at the address of c [see remove()]
 static void vacate( Group & c , std::size_t j );

 /// the table rebuilt with G groups, a power of 2 at least 2, first( o )
 /// called on
 /// the new table o before the pairs move there
 template< class F >
 void rebuild( std::size_t G , F && first );

 /// the groups for n pairs, 0 or a power of 2 at least 2
 [[nodiscard]] static std::size_t groups( std::size_t n );

 /// at most 7 / 8 of m places, the most that can be full or counted
 [[nodiscard]] static constexpr std::size_t bound( std::size_t m );

 /// f( i ) on each full place i, group after group
 template< class F >
 void each( F && f ) const;

 /// destroys the pairs and frees the places
 void release( void ) noexcept;

 /// each pair found by the inspection of its key, the count right and at
 /// most 7 / 8 of the places full or deleted; on more than 1024 places
 /// only the pairs on the inspection of the mixed hash x, up to its first
 /// group whose overflow lacks the bit of x
 [[nodiscard]] bool consistent( std::uint64_t x ) const;

 Group * f_ctrl = s_none;          ///< the groups of control bytes
 Slot * f_slot = nullptr;          ///< the places, 15 per group
 std::size_t f_cap = 0;            ///< the number of places
 std::size_t f_mask = 0;           ///< the groups - 1
 int f_shift = 63;                 ///< 64 - log2 of the groups
 std::size_t f_max = 0;            ///< bound( f_cap )
 std::size_t f_size = 0;           ///< the full places
 std::size_t f_deleted = 0;        ///< a bound of deleted() [see recount()]
 H f_h;                            ///< the hash of the keys
 Eq f_eq;                          ///< the equality of the keys

/*--------------------------------------------------------------------------*/
 public:

 /// the iterator on the full places, by its group, the slot of the first
 /// place of the group and the mask of the full places of the group from
 /// its own on, the lowest bit its place: a step clears that bit and
 /// keeps the places still full in the control bytes, so that the next
 /// place depends on the previous one by two operations on the mask and
 /// a place emptied meanwhile is skipped; past the group it skips the
 /// empty groups whole, up to a sentinel group after the last, whose byte 0
 /// is full and whose first place is the end, so that a step tests no
 /// bound
 class const_iterator {
  public:
  using value_type = std::pair< const K & , const V & >;
  using reference = value_type;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;

  reference operator*( void ) const {
   const auto & kv = f_b[ lowest( f_m ) ].kv;
   return( reference( kv.first , kv.second ) );
   }

  const_iterator & operator++( void ) {
   f_m &= ( f_m - 1 ) & fulls( *f_g );
   if( ! f_m ) [[unlikely]]
    forward();
   return( *this );
   }

  const_iterator & operator--( void );

  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( f_b + lowest( f_m ) == o.f_b + lowest( o.f_m ) );
   }

  private:
  friend class FlatHashTable;

  /// on the place of the lowest bit of m in the group g, whose first
  /// slot is b
  const_iterator( const Group * g , const Slot * b , Mask m )
   : f_g( g ) , f_b( b ) , f_m( m ) {}

  /// to the first full place of the groups after f_g, maybe the sentinel
  void forward( void );

  const Group * f_g = nullptr;      ///< the group of the place
  const Slot * f_b = nullptr;       ///< the slot of the place 0 of f_g
  Mask f_m = FIRST;                 ///< the full places from this one on

 };  // end( class const_iterator )

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
