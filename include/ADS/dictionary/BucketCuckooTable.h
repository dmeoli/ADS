/*--------------------------------------------------------------------------*/
/*------------------------ File BucketCuckooTable.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cuckoo hashing with buckets of 4 places and a search in breadth of
 * Li, Andersen, Kaminsky and Freedman (the table of MemC3 and libcuckoo),
 * beyond the book: the engineered cuckoo table of ADS [see
 * `CuckooHashing.h` for the cuckoo hashing of Pagh and Rodler, one key per
 * place]. A key with the hash \f$ h \f$ may be in any of the 4 places of two
 * buckets, so that a search looks at 8 places in the worst case, and the
 * table reaches a load of 95 % and more before an insertion fails, against
 * less than 50 % with one key per place.
 *
 * Each place has a tag of 8 bits, 0 when empty and otherwise 8 bits of the
 * hash of its key (1 in place of 0); the 4 tags of a bucket are a word of
 * 32 bits, compared with the tag of a key all at once by broadword
 * operations, so that a key is compared only in the places whose tag
 * matches. The first bucket of a key is given by the top bits of \f$ h \f$,
 * the second by the first and the tag,
 * \f$ b_2 = b_1 \oplus ( g( t ) \wedge ( B - 1 ) \vee 1 ) \f$ for \f$ B \f$
 * buckets (a power of 2) and a mixing \f$ g \f$ of the tag (the partial-key
 * cuckoo hashing of MemC3), an involution, so that a key moves to its other
 * bucket with no need to hash it again.
 *
 * An insertion puts a new key in a free place of its two buckets if any;
 * otherwise it searches in breadth, from the places of its buckets, a path
 * of at most 5 moves that ends in a bucket with a free place (each key of a
 * bucket of the path moving to its other bucket), and moves the keys along
 * it from the end, so that the new key takes the first place of the path.
 * The search in breadth finds a shorter path than the random walk of the
 * cuckoo hashing, and moves fewer keys. When no path is found within 5
 * moves and 1024 buckets (or past the load of 98 %), the table doubles and
 * every key is placed again, which costs \f$ O( 1 ) \f$ amortized per
 * insertion; when the hash of a key reads memory that the key points to, as
 * for the strings, the doubling hashes all the keys first, prefetching the
 * memory of the key 8 places ahead, and then places them, prefetching the
 * first bucket of the key 8 places ahead, so that the misses of the keys
 * overlap. A removal clears the tag of the place.
 *
 * No doubling separates keys whose hashes are equal, and more than 8 of
 * them find no place at any size of the table. A key that
 * still finds no place after 2 doublings of its insertion, or in a table at
 * most half full, therefore goes to a stash, a vector of pairs searched in
 * order after the two buckets (the cuckoo hashing with a stash of Kirsch,
 * Mitzenmacher and Wieder, whose stash has a constant size, here any); a
 * doubling places the pairs of the stash again with the others, and those
 * that find no place go to the new stash. A table that has held at most
 * \f$ n \f$ keys thus has at most \f$ \max( P_0 , 4 n ) \f$ places,
 * \f$ P_0 \f$ those of the empty table, an insertion makes at most 2
 * doublings, and a search of a key that is not in its buckets costs one
 * test while the stash is empty (as it is with any but a degenerate hash)
 * and a scan of the stash otherwise. A removal from the stash leaves a hole
 * there, which the next key of the stash fills.
 *
 * The iterators visit the pairs in the order of the places, the stash
 * last, as constant
 * pairs of references \f$ ( \mathit{key} , \mathit{value} ) \f$; `find()`
 * gives a value to change in place. An insertion of a new key may move
 * other keys and invalidates the iterators; a removal keeps them valid but
 * the one on the removed pair.
 *
 * The hash of a key is `h( k )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`] and mixed by `hashWord()` otherwise.
 * The keys and the values must be default constructible and movable.
 *
 * The tags and the places come from the allocator `A`, `std::allocator` by
 * default, with the rules of the standard [see `AllocatorAware.h`].
 *
 * The contracts [see `Contract.h`]: after `insert()` the key is in the
 * table and after `remove()` it is not; the invariant is that each key is
 * once in one of its two buckets with its tag or in the stash, and that the
 * tags and the stash count the pairs,
 * checked on the buckets of the key of the operation only past 1024 places.
 * The operations are defined in `BucketCuckooTable.tpp`.
 *
 * \see X. Li, D. G. Andersen, M. Kaminsky, M. J. Freedman, Algorithmic
 *      improvements for fast concurrent cuckoo hashing, Proceedings of
 *      EuroSys 2014, 27, 2014, doi:10.1145/2592798.2592820
 * \see B. Fan, D. G. Andersen, M. Kaminsky, MemC3: compact and concurrent
 *      MemCache with dumber caching and smarter hashing, Proceedings of
 *      NSDI 2013, 371-384, 2013 (the partial-key cuckoo hashing)
 * \see M. Dietzfelbinger, C. Weidling, Balanced allocation and dictionaries
 *      with tightly packed constant size bins, Theoretical Computer Science
 *      380(1-2), 47-68, 2007, doi:10.1016/j.tcs.2007.02.054 (the buckets
 *      of several places)
 * \see A. Kirsch, M. Mitzenmacher, U. Wieder, More robust hashing: cuckoo
 *      hashing with a stash, SIAM Journal on Computing 39(4), 1543-1561,
 *      2009, doi:10.1137/080728743 (the stash)
 * \see S. Chen, A. Ailamaki, P. B. Gibbons, T. C. Mowry, Improving hash
 *      join performance through prefetching, Proceedings of ICDE 2004,
 *      116-127, 2004, doi:10.1109/ICDE.2004.1319989 (the prefetch of the
 *      memory of the keys some keys ahead, in the doubling)
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

#ifndef __ADS_BucketCuckooTable
 #define __ADS_BucketCuckooTable
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/memory/AllocatorAware.h"
#include "ADS/memory/Prefetch.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BucketCuckooTable -------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on the cuckoo hashing with buckets of 4 places
template< class K , class V , class H = DefaultHash< K > ,
	  class Eq = std::equal_to< K > ,
	  class A = std::allocator< std::pair< const K , V > > >
class BucketCuckooTable
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;    ///< the type of the keys
 using Value = V;  ///< the type of the values

 /// the empty table, with room for \f$ n \f$ pairs before it grows, its
 /// places from `alloc`
 explicit BucketCuckooTable( std::size_t n = 0 , H h = H() , Eq eq = Eq() ,
			     const A & alloc = A() );

 /// the value of the key \f$ k \f$, or nothing, from the 8 places of its two
 /// buckets, compared first by their tags
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of the key \f$ k \f$, nil if it is not there
 [[nodiscard]] const V * find( const K & k ) const;

 /// the value of the key \f$ k \f$, to change in place, nil if it is not
 /// there
 [[nodiscard]] V * find( const K & k );

 /// associates the value \f$ v \f$ to the key \f$ k \f$; true if \f$ k \f$
 /// was not there
 ///
 /// In pseudocode (Li, Andersen, Kaminsky and Freedman, with the stash of
 /// Kirsch, Mitzenmacher and Wieder, beyond the book), \f$ b_1 \f$ and
 /// \f$ b_2 \f$ the two buckets of \f$ k \f$, \f$ n \f$ the keys and
 /// \f$ P \f$ the places; the search in breadth goes from a bucket to the
 /// other buckets of its keys, at most 5 moves deep and 1024 buckets in all:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ k \text{ is in } b_1 \text{, in } b_2
 ///     \text{ or in the stash}\ \textbf{then}\ \text{give it the value } v;
 ///     \ \textbf{return false} \cr
 /// 3 & \quad \textbf{for}\ g \gets 0 , 1 , 2\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ b_1 \text{ or } b_2 \text{ has a free place }
 ///     \ \textbf{then}\ \text{put } ( k , v ) \text{ there};\ \textbf{return
 ///     true} \cr
 /// 5 & \qquad \pi \gets \text{a path in breadth from } \{ b_1 , b_2 \}
 ///     \text{ to a bucket with a free place} \cr
 /// 6 & \qquad \textbf{if}\ \pi \ne \textbf{nil}\ \textbf{then}\ \text{move }
 ///     \text{the keys along } \pi \text{ from its end, put } ( k , v )
 ///     \text{ at its start};\ \textbf{return true} \cr
 /// 7 & \qquad \textbf{if}\ g = 2\ \textbf{or}\ 2 ( n + 1 ) \le P
 ///     \ \textbf{then}\ \textbf{break} \cr
 /// 8 & \qquad \text{double the table and place every pair again, those }
 ///     \text{that find no place in the stash} \cr
 /// 9 & \quad \text{put } ( k , v ) \text{ in the stash};\ \textbf{return
 ///     true}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is in the table
 bool insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, clearing the tag of its
 /// place; true if it was
 /// \post \f$ k \f$ is not in the table
 bool remove( const K & k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the places, 4 per bucket
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load, `size()` / `capacity()`
 [[nodiscard]] double load( void ) const;

 /// the doublings of the table
 [[nodiscard]] std::size_t rehashes( void ) const;

 /// the keys moved by the insertions
 [[nodiscard]] std::size_t moves( void ) const;

 /// the keys in the stash, 0 but for degenerate hashes
 [[nodiscard]] std::size_t stashed( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 struct Slot
 {
  K key{};    ///< the key
  V value{};  ///< its value
  };

 static constexpr int kDepth = 5;             ///< the moves of a path
 static constexpr std::size_t kNodes = 1024;  ///< the buckets searched
 static constexpr int kGrowths = 2;  ///< the doublings of an insertion

 public:
/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places, bidirectional

 /// the places, for `SlotIterator`
 struct Places
 {
  /// the number of places, the stash included
  static std::size_t size( const auto & o ) {
   return( o.f_S.size() + o.f_X.size() );
   }
  /// whether the place \f$ i \f$ holds a key, i.e., its tag is not 0, or
  /// its place of the stash is not a hole
  static bool occupied( const auto & o , std::size_t i ) {
   if( i >= o.f_S.size() )
    return( o.f_XT[ i - o.f_S.size() ] != 0 );
   return( ( o.f_T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF );
   }
  /// the pair of the place \f$ i \f$
  static auto read( const auto & o , std::size_t i ) {
   const Slot & s = o.at( i );
   return( PairRef< const K & , const V & >( s.key , s.value ) );
   }
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< K , V >;
 /// the type of the keys
 using key_type = K;
 /// the type of the values
 using mapped_type = V;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = PairRef< const K & , const V & >;
 /// a constant reference to an item
 using const_reference = PairRef< const K & , const V & >;
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< BucketCuckooTable , Places >;
 /// the iterator on the items
 using iterator = const_iterator;
 /// the type of the allocator
 using allocator_type = A;
 /// the allocator of the places
 [[nodiscard]] allocator_type get_allocator( void ) const {
  return( allocator_type( f_S.get_allocator() ) );
  }

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_S.size() + f_X.size() ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:
 /// the hash of \f$ k \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const K & k ) const;

 /// the tag of the hash \f$ h \f$, never 0
 [[nodiscard]] static std::uint32_t tag( std::uint64_t h );

 /// the first bucket of the hash \f$ h \f$
 [[nodiscard]] std::size_t first( std::uint64_t h ) const;

 /// the other bucket of a key with the tag \f$ t \f$ in the bucket
 /// \f$ b \f$, \f$ b \oplus ( g( t ) \wedge ( B - 1 ) \vee 1 ) \f$
 [[nodiscard]] std::size_t other( std::size_t b , std::uint32_t t ) const;

 /// the place of \f$ k \f$ with the hash \f$ h \f$, past the buckets if in
 /// the stash, or `~0`
 [[nodiscard]] std::size_t locate( const K & k , std::uint64_t h ) const;

 /// the pair of the place \f$ i \f$, of the stash past the buckets
 [[nodiscard]] const Slot & at( std::size_t i ) const;

 /// the pair of the place \f$ i \f$, to change
 [[nodiscard]] Slot & at( std::size_t i );

 /// the pair put in the stash, in its first hole if any
 template< class KK , class VV >
 void stash( KK && k , VV && v );

 /// a free place of the bucket \f$ b \f$, or 4
 [[nodiscard]] int freePlace( std::size_t b ) const;

 /// the pair in the free place \f$ i \f$ of the bucket \f$ b \f$ with the tag
 /// \f$ t \f$
 template< class KK , class VV >
 void put( std::size_t b , int i , std::uint32_t t , KK && k , VV && v );

 /// a free place for a new key with the hash \f$ h \f$, by moving keys along
 /// a path found in breadth; `~0` if none within `kDepth` moves
 std::size_t makeRoom( std::uint64_t h );

 /// the table doubled, every pair placed again or put in the stash
 void grow( void );

 /// the pair placed in a table with room for it, false if it found none
 template< class KK , class VV >
 bool place( std::uint64_t h , KK && k , VV && v );

 /// \f$ k \f$ in one of its buckets with its tag or in the stash, at most
 /// once, and, up to 1024 places, every pair so and counted right
 [[nodiscard]] bool placed( const K & k ) const;

 /// the vector of the tags
 using Tags = detail::AllocVector< A , std::uint32_t >;
 /// the vector of the places
 using Slots = detail::AllocVector< A , Slot >;

 Tags f_T;                    ///< the 4 tags of each bucket
 Slots f_S;                   ///< the places, 4 per bucket
 Slots f_X;                   ///< the stash
 Tags f_XT;                   ///< 1 where the stash holds a pair, 0 a hole
 std::size_t f_xn = 0;        ///< the pairs in the stash
 int f_lg = 1;                ///< log2 of the buckets
 std::size_t f_n = 0;         ///< the number of keys
 std::size_t f_rehashes = 0;  ///< the doublings
 std::size_t f_moves = 0;     ///< the keys moved
 H f_h;                       ///< the hash of the keys
 Eq f_eq;                     ///< the equality of the keys

 };  // end( class BucketCuckooTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/BucketCuckooTable.tpp"

/*--------------------------------------------------------------------------*/

#endif /* BucketCuckooTable.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BucketCuckooTable.h ----------------------*/
/*--------------------------------------------------------------------------*/
