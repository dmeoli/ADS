/*--------------------------------------------------------------------------*/
/*---------------------------- File SkipList.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The skip list of Pugh, beyond the book: a sorted list of the keys with
 * express lanes, each node being in the levels 0 to L - 1, L drawn with
 * probability \f$ 2^{-L} \f$, and each level a sorted list of its nodes;
 * a search goes along the highest level and drops a level before passing
 * the key, in \f$ O( \log n ) \f$ expected time, and an insertion or a
 * removal links or unlinks the node on its levels. It is a DICTIONARY [see
 * ADT.h] as simple as a list and, with the right levels, as fast as a
 * balanced tree, whose balance is in the random levels rather than in
 * rotations; the generator is SplitMix64 [see RandomHashing.h], seeded, so
 * that a run is reproducible.
 *
 * The contracts [see Contract.h]: insert() ensures that the key is there and
 * remove() that it is not; the invariant is that each level is sorted and a
 * sublist of the level below, an \f$ O( n \log n ) \f$ check at the level 3.
 * The operations are defined in SkipList.tpp.
 *
 * \see W. Pugh, Skip lists: a probabilistic alternative to balanced trees,
 *      Communications of the ACM 33(6), 668-676, 1990,
 *      doi:10.1145/78973.78977
 * \see G. L. Steele, D. Lea, C. H. Flood, Fast splittable pseudorandom
 *      number generators, Proceedings of OOPSLA 2014, 453-472, 2014,
 *      doi:10.1145/2660193.2660195 (SplitMix64)
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

#ifndef __ADS_SkipList
 #define __ADS_SkipList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS SkipList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a skip list
template< std::totally_ordered K , class V >
class SkipList {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 explicit SkipList( std::uint64_t seed = 1 );

 SkipList( const SkipList & ) = delete;
 SkipList & operator=( const SkipList & ) = delete;

 /// the nodes of other, which is left empty
 SkipList( SkipList && other ) noexcept;

 ~SkipList();

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the number of levels in use
 [[nodiscard]] int levels( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kMax = 32;   ///< the largest number of levels

 struct Node {
  K key;
  V value;
  std::vector< Node * > next;      ///< the successor on each level
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key, along
 // the bottom level, forward

 class const_iterator {
  public:
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;
  explicit const_iterator( const Node * x ) : f_x( x ) {}

  reference operator*( void ) const {
   return( reference( f_x->key , f_x->value ) );
   }
  const_iterator & operator++( void ) {
   f_x = f_x->next[ 0 ];
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  bool operator==( const const_iterator & o ) const = default;

  private:
  const Node * f_x = nullptr;
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_head.empty() ? nullptr : f_head[ 0 ] ) );
  }
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the last node before k on each level, in update
 Node * before( const K & k , Node * update[ kMax ] ) const;

 /// each level sorted and a sublist of the one below, \f$ O( n \log n ) \f$
 [[nodiscard]] bool sorted( void ) const;

 std::vector< Node * > f_head;     ///< the first node on each level
 int f_levels = 0;
 std::size_t f_size = 0;
 SplitMix64 f_g;

 };  // end( class SkipList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/SkipList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SkipList.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SkipList.h ---------------------------*/
/*--------------------------------------------------------------------------*/
