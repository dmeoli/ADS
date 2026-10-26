/*--------------------------------------------------------------------------*/
/*------------------------ File BucketCuckooTable.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cuckoo hashing with buckets of 4 places and a search in breadth of
 * Li, Andersen, Kaminsky and Freedman (the table of MemC3 and libcuckoo),
 * beyond the book: the engineered cuckoo table of ADS [see CuckooHashing.h
 * for the cuckoo hashing of Pagh and Rodler, one key per place]. A key with
 * the hash h may be in any of the 4 places of two buckets, so that a search
 * looks at 8 places in the worst case, and the table reaches a load of 95 %
 * and more before an insertion fails, against less than 50 % with one key
 * per place.
 *
 * Each place has a tag of 8 bits, 0 when empty and otherwise 8 bits of the
 * hash of its key (1 in place of 0); the 4 tags of a bucket are a word of
 * 32 bits, compared with the tag of a key all at once by broadword
 * operations, so that a key is compared only in the places whose tag
 * matches. The first bucket of a key is given by the top bits of h, the
 * second by the first and the tag, \f$ b_2 = b_1 \oplus ( g( t ) \wedge
 * ( B - 1 ) \vee 1 ) \f$ for B buckets (a power of 2) and a mixing g of
 * the tag (the partial-key cuckoo hashing of MemC3), an involution, so that
 * a key moves to its other bucket with no need to hash it again.
 *
 * An insertion puts a new key in a free place of its two buckets if any;
 * otherwise it searches in breadth, from the places of its buckets, a path
 * of at most 5 moves that ends in a bucket with a free place (each key of a
 * bucket of the path moving to its other bucket), and moves the keys along
 * it from the end, so that the new key takes the first place of the path.
 * The search in breadth finds a shorter path than the random walk of the
 * cuckoo hashing, and moves fewer keys. When no path is found within 5
 * moves and 1024 buckets (or past the load of 98 %), the table doubles and
 * every key is placed again, which costs O( 1 ) amortized per insertion;
 * when the hash of a key reads memory that the key points to, as for the
 * strings, the doubling hashes all the keys first, prefetching the memory
 * of the key 8 places ahead, and then places them, prefetching the first
 * bucket of the key 8 places ahead, so that the misses of the keys
 * overlap. A removal clears the tag of the place.
 *
 * The iterators visit the pairs in the order of the places, as constant
 * pairs of references ( key , value ); find() gives a value to change in
 * place. An insertion of a new key may move other keys and invalidates the
 * iterators; a removal keeps them valid but the one on the removed pair.
 *
 * The hash of a key is h( k ), taken as it is if H declares is_avalanching
 * [see FastHash.h] and mixed by hashWord() otherwise. The keys and the
 * values must be default constructible and movable.
 *
 * The contracts [see Contract.h]: after insert() the key is in the table
 * and after remove() it is not; the invariant is that each key is in one of
 * its two buckets with its tag, and that the tags count the pairs, checked
 * on the buckets of the key of the operation only past 1024 places. The
 * operations are defined in BucketCuckooTable.tpp.
 *
 * \see X. Li, D. G. Andersen, M. Kaminsky, M. J. Freedman, Algorithmic
 *      improvements for fast concurrent cuckoo hashing, Proceedings of
 *      EuroSys 2014, 27, 2014, doi:10.1145/2592798.2592820
 * \see B. Fan, D. G. Andersen, M. Kaminsky, MemC3: compact and concurrent
 *      MemCache with dumber caching and smarter hashing, Proceedings of
 *      NSDI 2013, 371-384, 2013 (the partial-key cuckoo hashing)
 * \see M. Dietzfelbinger, C. Weidling, Balanced allocation and dictionaries
 *      with tightly packed constant size bins, Theoretical Computer Science
 *      380(1-2), 47-68, 2007 (the buckets of several places)
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BucketCuckooTable -------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on the cuckoo hashing with buckets of 4 places
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > >
class BucketCuckooTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty table, with room for n pairs before it grows
 explicit BucketCuckooTable( std::size_t n = 0 , H h = H() , Eq eq = Eq() );

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of the key k, nil if it is not there
 [[nodiscard]] const V * find( const K & k ) const;

 /// the value of the key k, to change in place, nil if it is not there
 [[nodiscard]] V * find( const K & k );

 /// associates the value v to the key k; true if k was not there
 /// \post k is in the table
 bool insert( const K & k , const V & v );

 /// removes the key k, if it is there; true if it was
 /// \post k is not in the table
 bool remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the places, 4 per bucket
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load, size() / capacity()
 [[nodiscard]] double load( void ) const;

 /// the doublings of the table
 [[nodiscard]] std::size_t rehashes( void ) const;

 /// the keys moved by the insertions
 [[nodiscard]] std::size_t moves( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Slot {
  K key{};
  V value{};
  };

 static constexpr int kDepth = 5;          ///< the moves of a path
 static constexpr std::size_t kNodes = 1024;   ///< the buckets searched

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places, bidirectional

 /// the places, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_S.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( ( o.f_T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF );
   }
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< const K & , const V & >( o.f_S[ i ].key ,
                                              o.f_S[ i ].value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::pair< const K & , const V & >;
 using const_reference = std::pair< const K & , const V & >;
 using const_iterator = SlotIterator< BucketCuckooTable , Places >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_S.size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the hash of k, mixed unless H avalanches
 [[nodiscard]] std::uint64_t hash( const K & k ) const;

 /// the tag of the hash h, never 0
 [[nodiscard]] static std::uint32_t tag( std::uint64_t h );

 /// the first bucket of the hash h
 [[nodiscard]] std::size_t first( std::uint64_t h ) const;

 /// the other bucket of a key with the tag t in the bucket b
 [[nodiscard]] std::size_t other( std::size_t b , std::uint32_t t ) const;

 /// the place of k with the hash h, or ~0
 [[nodiscard]] std::size_t locate( const K & k , std::uint64_t h ) const;

 /// a free place of the bucket b, or 4
 [[nodiscard]] int freePlace( std::size_t b ) const;

 /// the pair in the free place i of the bucket b with the tag t
 template< class KK , class VV >
 void put( std::size_t b , int i , std::uint32_t t , KK && k , VV && v );

 /// a free place for a new key with the hash h, by moving keys along a
 /// path found in breadth; ~0 if none within kDepth moves
 std::size_t makeRoom( std::uint64_t h );

 /// the table doubled, every pair placed again
 void grow( void );

 /// the pair placed in a table with room for it, false if it found none
 template< class KK , class VV >
 bool place( std::uint64_t h , KK && k , VV && v );

 /// k in one of its buckets with its tag and at most once, and, up to 1024
 /// places, every pair so and counted right
 [[nodiscard]] bool placed( const K & k ) const;

 std::vector< std::uint32_t > f_T; ///< the 4 tags of each bucket
 std::vector< Slot > f_S;          ///< the places, 4 per bucket
 int f_lg = 1;                     ///< log2 of the buckets
 std::size_t f_n = 0;
 std::size_t f_rehashes = 0;
 std::size_t f_moves = 0;
 H f_h;
 Eq f_eq;

 };  // end( class BucketCuckooTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/BucketCuckooTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BucketCuckooTable.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BucketCuckooTable.h ----------------------*/
/*--------------------------------------------------------------------------*/
