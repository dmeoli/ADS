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
 * seeded, so that a run is reproducible. It is also the `SkiplistSSet` of
 * Morin (section 4.2): `lowerBound()` is his `find(x)`, the smallest key
 * not smaller than \f$ x \f$.
 *
 * A node is one block of memory, its key, its value and the tower of its
 * successors one after the other, and the head is a tower with no node, so
 * that a step of a search reads one line and tests no special case. A
 * search compares \f$ x \f$ once with each node: when it drops a level and
 * meets the node that stopped it on the level above, it drops again with no
 * comparison (exercise 4.8 of Morin), which saves a quarter or more of the
 * comparisons of the plain search (see \ref theory_dictionary_tries).
 * `fingerLookup()` is the finger search of exercise 4.10: the list keeps
 * the last node before the previous key on each level, climbs them from
 * the bottom to the first one from which the new key is reachable and goes
 * down from there, in \f$ O( 1 + \log r ) \f$ expected time, \f$ r \f$
 * the keys between the two.
 *
 * Where it is meant to be faster than the `SkiplistSSet` of Morin, for the
 * benchmarks to check: the levels come from `SplitMix64` instead of
 * `rand()`, a few cycles against a call into the C library; a node has
 * exactly \f$ \ell \f$ links where his has \f$ \ell + 1 \f$; the search
 * skips the comparisons repeated across the levels (16.2 comparisons per
 * search against 22.6 on 3000 keys, in the test of exercise 4.8), which
 * counts when comparing a key costs more than a word, e.g., strings. The
 * layout of a node, one block with its tower, and the removal in one
 * search are as in his code.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is there
 * and `remove()` that it is not; the invariant is that each level is sorted
 * and a sublist of the level below, an \f$ O( n \log n ) \f$ check at the
 * level 3. The operations are defined in `SkipList.tpp`.
 *
 * \see W. Pugh, Skip lists: a probabilistic alternative to balanced trees,
 *      Communications of the ACM 33(6), 668-676, 1990,
 *      doi:10.1145/78973.78977
 * \see W. Pugh, A skip list cookbook, Technical report CS-TR-2286.1,
 *      University of Maryland, 1990 (the finger search)
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 4.2 and exercises 4.8 and 4.10,
 *      doi:10.15215/aupress/9781927356388.01
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
#include <memory>
#include <new>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS SkipList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a skip list, an SSet of Morin by `lowerBound()`
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
 /// In pseudocode (Pugh, beyond the book, and exercise 4.8 of Morin),
 /// \f$ x.\mathit{next}[ l ] \f$ the successor of \f$ x \f$ on the level
 /// \f$ l \f$, the head being a node with no key; the search drops a level
 /// just before passing \f$ k \f$, the last node met on each level is kept in
 /// \f$ U \f$ for `insert()` and `remove()`, and \f$ s \f$, the last node
 /// found not smaller than \f$ k \f$, is never compared again:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{before}( k ) \cr
 /// 2 & \quad x \gets \mathit{head};\ s \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{for}\ l \gets L - 1\ \textbf{downto}\ 0\ \textbf{do}
 ///     \cr
 /// 4 & \qquad \textbf{while}\ x.\mathit{next}[ l ] \ne s\ \textbf{and}
 ///     \ x.\mathit{next}[ l ].\mathit{key} < k\ \textbf{do}\ x \gets
 ///     x.\mathit{next}[ l ] \cr
 /// 5 & \qquad s \gets x.\mathit{next}[ l ];\ U[ l ] \gets x \cr
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

 /// true if there is no key
 [[nodiscard]] bool empty( void ) const { return( f_size == 0 ); }

 /// the number of levels in use
 [[nodiscard]] int levels( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kMax = 32;   ///< the largest number of levels

 /// a node, followed in its block by the tower of its \f$ \ell \f$
 /// successors [see `tower()`]
 struct Node {
  K key;                           ///< the key
  V value;                         ///< its value
  int height;                      ///< the number \f$ \ell \f$ of levels
  };

 /// the offset of the tower from the start of a node
 static constexpr std::size_t kTower =
  ( sizeof( Node ) + alignof( Node * ) - 1 ) / alignof( Node * ) *
  alignof( Node * );

 /// the tower of the successors of \f$ x \f$
 static Node ** tower( Node * x ) {
  return( std::launder( reinterpret_cast< Node ** >(
           reinterpret_cast< std::byte * >( x ) + kTower ) ) );
  }

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
  explicit const_iterator( Node * x ) : f_x( x ) {}

  /// the pair of the node
  reference operator*( void ) const {
   return( reference( f_x->key , f_x->value ) );
   }
  /// to the next node of the bottom level
  const_iterator & operator++( void ) {
   f_x = tower( f_x )[ 0 ];
   return( *this );
   }
  /// to the next node, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same node
  bool operator==( const const_iterator & o ) const = default;

  private:
  Node * f_x = nullptr;
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
  return( const_iterator( f_head[ 0 ] ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*----------------------------- THE SSET OF MORIN --------------------------*/

 /// the iterator on the smallest key not smaller than \f$ k \f$, `end()` if
 /// there is none (the `find(x)` of an SSet of Morin), by `before( k )`, in
 /// \f$ O( \log n ) \f$ expected time
 [[nodiscard]] const_iterator lowerBound( const K & k ) const;

 /// the address of the key \f$ k \f$, nil if absent, in \f$ O( \log n ) \f$
 /// expected time: the `find( x )` of a USet of Morin [see `DictionarySet`];
 /// valid until the key is removed
 [[nodiscard]] const K * findKey( const K & k ) const;

 /// the address of the smallest key not smaller than \f$ k \f$, nil if none,
 /// in \f$ O( \log n ) \f$ expected time: the `find( x )` of an SSet of Morin
 /// [see `DictionarySet`]; valid until the key is removed
 [[nodiscard]] const K * lowerBoundKey( const K & k ) const;

 /// the value of \f$ k \f$, or nothing, by a finger search from the key of
 /// the previous `fingerLookup()`, in \f$ O( 1 + \log r ) \f$ expected
 /// time, \f$ r \f$ the keys between the two (exercise 4.10 of Morin); an
 /// insertion or a removal brings the finger back to the head
 ///
 /// In pseudocode (Pugh, beyond the book), \f$ F[ l ] \f$ the last node
 /// before the previous key on the level \f$ l \f$, the head counting as
 /// \f$ -\infty \f$; the finger climbs while its node is not before
 /// \f$ k \f$ or its successor is, and the search goes down from there,
 /// updating \f$ F \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{fingerLookup}( k ) \cr
 /// 2 & \quad l \gets 0 \cr
 /// 3 & \quad \textbf{while}\ l < L - 1\ \textbf{and}\ ( F[ l ].\mathit{key}
 ///     \ge k\ \textbf{or}\ F[ l ].\mathit{next}[ l ].\mathit{key} < k )
 ///     \ \textbf{do}\ l \gets l + 1 \cr
 /// 4 & \quad x \gets F[ l ];\ \textbf{if}\ x.\mathit{key} \ge k
 ///     \ \textbf{then}\ x \gets \mathit{head} \cr
 /// 5 & \quad \textbf{for}\ j \gets l\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 6 & \qquad \textbf{while}\ x.\mathit{next}[ j ] \ne \textbf{nil}
 ///     \ \textbf{and}\ x.\mathit{next}[ j ].\mathit{key} < k\ \textbf{do}
 ///     \ x \gets x.\mathit{next}[ j ] \cr
 /// 7 & \qquad F[ j ] \gets x \cr
 /// 8 & \quad y \gets x.\mathit{next}[ 0 ] \cr
 /// 9 & \quad \textbf{if}\ y \ne \textbf{nil}\ \textbf{and}\ y.\mathit{key}
 ///     = k\ \textbf{then return}\ y.\mathit{value}\ \textbf{else return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > fingerLookup( const K & k );

 private:

 /// the node of \f$ k \f$ or nil, the last node before \f$ k \f$ on each
 /// level in `update` (nil for the head), by `before()` above
 Node * before( const K & k , Node * update[ kMax ] ) const;

 /// the tower of \f$ x \f$, that of the head if \f$ x \f$ is nil
 Node ** towerOf( Node * x ) const {
  return( x ? tower( x ) : const_cast< Node ** >( f_head ) );
  }

 /// a new node of \f$ L \f$ levels, its successors nil
 static Node * make( const K & k , const V & v , int L );

 /// frees the node \f$ x \f$
 static void dispose( Node * x ) noexcept;

 /// each level sorted and a sublist of the one below, \f$ O( n \log n ) \f$
 [[nodiscard]] bool sorted( void ) const;

 Node * f_head[ kMax ] = {};       ///< the first node on each level
 Node * f_finger[ kMax ] = {};     ///< the finger, nil for the head
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
