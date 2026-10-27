/*--------------------------------------------------------------------------*/
/*---------------------------- File SkipList.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The skip list of Pugh, beyond the book: a sorted list of the keys with
 * express lanes, each node being in the levels 0 to \f$ L - 1 \f$,
 * \f$ L \f$ drawn with probability \f$ 2^{-L} \f$, and each level a sorted
 * list of its nodes; a search goes along the highest level and drops a
 * level before passing the key, in \f$ O( \log n ) \f$ expected time, and an
 * insertion or a removal links or unlinks the node on its levels. It is a
 * DICTIONARY [see `ADT.h`] as simple as a list and, with the right levels,
 * as fast as a balanced tree, whose balance is in the random levels rather
 * than in rotations; the generator is `SplitMix64` [see `RandomHashing.h`],
 * seeded, so that a run is reproducible.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is there
 * and `remove()` that it is not; the invariant is that each level is sorted
 * and a sublist of the level below, an \f$ O( n \log n ) \f$ check at the
 * level 3. The operations are defined in `SkipList.tpp`.
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

 /// the empty list, its levels drawn by a generator from the seed
 explicit SkipList( std::uint64_t seed = 1 );

 /// no copy, since the nodes are linked by pointers
 SkipList( const SkipList & ) = delete;

 /// no copy, since the nodes are linked by pointers
 SkipList & operator=( const SkipList & ) = delete;

 /// the nodes of `other`, which is left empty
 SkipList( SkipList && other ) noexcept;

 /// frees the memory of the structure
 ~SkipList();

 /// the value of the key \f$ k \f$, or nothing, in \f$ O( \log n ) \f$
 /// expected time
 ///
 /// In pseudocode (Pugh, beyond the book), \f$ x.\mathit{next}[ l ] \f$ the
 /// successor of \f$ x \f$ on the level \f$ l \f$, the head being a node with
 /// no key; the search drops a level just before passing \f$ k \f$, and the
 /// last node met on each level is kept in \f$ U \f$ for `insert()` and
 /// `remove()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{before}( k ) \cr
 /// 2 & \quad x \gets \mathit{head} \cr
 /// 3 & \quad \textbf{for}\ l \gets L - 1\ \textbf{downto}\ 0\ \textbf{do}
 ///     \cr
 /// 4 & \qquad \textbf{while}\ x.\mathit{next}[ l ] \ne \textbf{nil}
 ///     \ \textbf{and}\ x.\mathit{next}[ l ].\mathit{key} < k\ \textbf{do}\ x
 ///     \gets x.\mathit{next}[ l ] \cr
 /// 5 & \qquad U[ l ] \gets x \cr
 /// 6 & \quad \textbf{return}\ ( x.\mathit{next}[ 0 ] , U ) \cr
 /// 7 & \textbf{function}\ \mathrm{lookup}( k ) \cr
 /// 8 & \quad ( y , U ) \gets \mathrm{before}( k ) \cr
 /// 9 & \quad \textbf{if}\ y \ne \textbf{nil}\ \textbf{and}\ y.\mathit{key}
 ///     = k\ \textbf{then return}\ y.\mathit{value}\ \textbf{else return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$, in
 /// \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Pugh, beyond the book), the level of a new node drawn as
 /// the number of trailing 1s of a random word plus one, i.e., \f$ \ell \f$
 /// with probability \f$ 2^{-\ell} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad ( y , U ) \gets \mathrm{before}( k ) \cr
 /// 3 & \quad \textbf{if}\ y \ne \textbf{nil}\ \textbf{and}\ y.\mathit{key}
 ///     = k\ \textbf{then}\ y.\mathit{value} \gets v;\ \textbf{return} \cr
 /// 4 & \quad \ell \gets \text{a random level};\ \textbf{for}\ l \gets L
 ///     \ \textbf{to}\ \ell - 1\ \textbf{do}\ U[ l ] \gets \mathit{head} \cr
 /// 5 & \quad L \gets \max\{ L , \ell \};\ x \gets \text{a node } ( k , v )
 ///     \text{ of } \ell \text{ levels} \cr
 /// 6 & \quad \textbf{for}\ l \gets 0\ \textbf{to}\ \ell - 1\ \textbf{do}
 ///     \ x.\mathit{next}[ l ] \gets U[ l ].\mathit{next}[ l ];\ U[ l
 ///     ].\mathit{next}[ l ] \gets x
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, unlinking its node from each
 /// of its levels after `before( k )` and dropping the levels left empty, in
 /// \f$ O( \log n ) \f$ expected time
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of levels in use
 [[nodiscard]] int levels( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kMax = 32;   ///< the largest number of levels

 struct Node {
  K key;                           ///< the key
  V value;                         ///< its value
  std::vector< Node * > next;      ///< the successor on each level
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key, along
 // the bottom level, forward

 /// the iterator on the pairs, along the bottom level
 class const_iterator {
  public:
  /// a reference to an item
  using reference = std::pair< const K & , const V & >;
  /// the type of the items, for the standard library
  using value_type = reference;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// the concept of the iterator, for C++20
  using iterator_concept = std::forward_iterator_tag;
  /// the category of the iterator
  using iterator_category = std::input_iterator_tag;

  /// the end
  const_iterator( void ) = default;

  /// on the pair of the node \f$ x \f$
  explicit const_iterator( const Node * x ) : f_x( x ) {}

  /// the pair of the node
reference operator*( void ) const {
   return( reference( f_x->key , f_x->value ) );
   }
  /// to the next node of the bottom level
  const_iterator & operator++( void ) {
   f_x = f_x->next[ 0 ];
   return( *this );
   }
  /// to the next node, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same node
  bool operator==( const const_iterator & o ) const = default;

  private:
  const Node * f_x = nullptr;
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< const K & , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = value_type;
 /// a constant reference to an item
 using const_reference = value_type;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_head.empty() ? nullptr : f_head[ 0 ] ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the last node before \f$ k \f$ on each level, in `update`
 Node * before( const K & k , Node * update[ kMax ] ) const;

 /// each level sorted and a sublist of the one below, \f$ O( n \log n ) \f$
 [[nodiscard]] bool sorted( void ) const;

 std::vector< Node * > f_head;     ///< the first node on each level
 int f_levels = 0;                 ///< the levels in use
 std::size_t f_size = 0;           ///< the number of keys
 SplitMix64 f_g;                   ///< the generator of the levels

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
