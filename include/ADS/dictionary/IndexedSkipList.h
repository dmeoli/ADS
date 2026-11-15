/*--------------------------------------------------------------------------*/
/*------------------------- File IndexedSkipList.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The indexed skip list, beyond the book: the skip list of Pugh [see
 * `SkipList.h`] whose links carry their width, the number of keys they jump
 * over, so that besides the operations of a DICTIONARY [see `ADT.h`] and the
 * `find(x)` of an SSet of Morin (`lowerBound()`) it reaches a key by its
 * rank, the number of keys smaller than it, and gives the rank of a key,
 * both in \f$ O( \log n ) \f$ expected time (exercise 4.9 of Morin). A
 * search by key sums the widths of the links it follows, which is the rank
 * of the last node before the key; a search by rank goes along a level while
 * the widths summed stay below the rank, as `SkipListSequence` does by
 * index. An insertion splits the link that it crosses on each of its levels
 * and adds 1 to the width of those that pass over it, a removal the reverse,
 * so that the widths cost a constant factor on the plain skip list. A node is
 * one block of memory, its key, its value and the tower of its links; the
 * generator is `SplitMix64` [see `RandomHashing.h`], seeded.
 *
 * Morin gives no code for it: the benchmarks compare it with `SkipList`,
 * whose operations it shares, to measure the price of the widths, one
 * word per link and one addition per step.
 *
 * The contracts [see `Contract.h`]: `select()` needs a rank below `size()`;
 * `insert()` ensures that the key is there and `remove()` that it is not; the
 * invariant, an \f$ O( n \log n ) \f$ check at the level 3, is the order of
 * the keys and that the widths of each level sum to \f$ n + 1 \f$. The
 * operations are defined in `IndexedSkipList.tpp`.
 *
 * \see W. Pugh, A skip list cookbook, Technical report CS-TR-2286.1,
 *      University of Maryland, 1990 (the widths of the links)
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 4.3 and exercise
 *      4.9
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

#ifndef __ADS_IndexedSkipList
 #define __ADS_IndexedSkipList
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

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS IndexedSkipList --------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a skip list with the widths of the links, which reaches
/// a key by its rank
template< std::totally_ordered K , class V >
class IndexedSkipList
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;    ///< the type of the keys
 using Value = V;  ///< the type of the values

 /// the empty list, its levels drawn by a generator from the seed
 explicit IndexedSkipList( std::uint64_t seed = 1 );

 /// no copy, since the nodes are linked by pointers
 IndexedSkipList( const IndexedSkipList & ) = delete;

 /// no copy, since the nodes are linked by pointers
 IndexedSkipList & operator=( const IndexedSkipList & ) = delete;

 /// the nodes of `other`, which is left empty
 IndexedSkipList( IndexedSkipList && other ) noexcept;

 /// frees the memory of the structure
 ~IndexedSkipList();

 /// the value of \f$ k \f$, or nothing, by the search of `SkipList`, in
 /// \f$ O( \log n ) \f$ expected time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ expected
 /// time
 ///
 /// In pseudocode (Pugh, beyond the book), \f$ U[ l ] \f$ the last node
 /// before \f$ k \f$ on the level \f$ l \f$ and \f$ r[ l ] \f$ its rank, the
 /// head of rank \f$ -1 \f$, \f$ y \f$ the new node of \f$ \ell \f$ levels:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad ( U , r ) \gets \text{the search of } k \text{, summing the}
 ///     \text{ widths} \cr
 /// 3 & \quad \textbf{if}\ U[ 0 ].\mathit{next}[ 0 ].\mathit{key} = k
 ///     \ \textbf{then}\ \text{its value} \gets v;\ \textbf{return} \cr
 /// 4 & \quad i \gets r[ 0 ] + 1 \quad \triangleright\ \text{the rank of }
 ///     k \cr
 /// 5 & \quad \textbf{for}\ l \gets 0\ \textbf{to}\ L - 1\ \textbf{do} \cr
 /// 6 & \qquad x \gets U[ l ] \cr
 /// 7 & \qquad \textbf{if}\ l \ge \ell\ \textbf{then}\ x.w[ l ] \gets x.w[ l
 ///     ] + 1 \cr
 /// 8 & \qquad \textbf{else}\ y.\mathit{next}[ l ] \gets x.\mathit{next}[ l
 ///     ];\ y.w[ l ] \gets r[ l ] + x.w[ l ] + 1 - i;\ x.\mathit{next}[ l ]
 ///     \gets y;\ x.w[ l ] \gets i - r[ l ]
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ expected
 /// time: on each level of its node the link that reaches it is replaced by
 /// the one that leaves it, their widths summed less 1, and the links that
 /// pass over it lose 1
 ///
 /// In pseudocode (Pugh, beyond the book), with \f$ U \f$ as in `insert()`
 /// and \f$ y \f$ the node of \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( k ) \cr
 /// 2 & \quad U \gets \text{the search of } k;\ y \gets U[ 0
 ///     ].\mathit{next}[ 0 ] \cr
 /// 3 & \quad \textbf{if}\ y = \textbf{nil}\ \textbf{or}\ y.\mathit{key}
 ///     \ne k\ \textbf{then return} \cr
 /// 4 & \quad \textbf{for}\ l \gets 0\ \textbf{to}\ L - 1\ \textbf{do} \cr
 /// 5 & \qquad x \gets U[ l ] \cr
 /// 6 & \qquad \textbf{if}\ x.\mathit{next}[ l ] = y\ \textbf{then}
 ///     \ x.w[ l ] \gets x.w[ l ] + y.w[ l ] - 1;\ x.\mathit{next}[ l ]
 ///     \gets y.\mathit{next}[ l ] \cr
 /// 7 & \qquad \textbf{else}\ x.w[ l ] \gets x.w[ l ] - 1 \cr
 /// 8 & \quad \textbf{for}\ l \ge L\ \textbf{do}\ \mathit{head}.w[ l ]
 ///     \gets \mathit{head}.w[ l ] - 1 \cr
 /// 9 & \quad \text{free } y
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// true if there is no key
 [[nodiscard]] bool empty( void ) const { return( f_size == 0 ); }

 /// the number of keys smaller than \f$ k \f$, the sum of the widths along
 /// the search of \f$ k \f$, in \f$ O( \log n ) \f$ expected time
 [[nodiscard]] std::size_t rank( const K & k ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:
 static constexpr int kMax = 32;  ///< the largest number of levels

 struct Node;

 /// a link of a level: the successor and the width
 struct Link
 {
  Node * next;    ///< the successor, nil at the end
  std::size_t w;  ///< the ranks it advances by
  };

 /// a node, followed in its block by the tower of its links
 struct Node
 {
  K key;       ///< the key
  V value;     ///< its value
  int height;  ///< the number of levels
  };

 /// the offset of the tower from the start of a node
 static constexpr std::size_t kTower =
   ( sizeof( Node ) + alignof( Link ) - 1 ) / alignof( Link ) *
   alignof( Link );

 /// the tower of the links of \f$ x \f$
 static Link * tower( Node * x ) {
  return( std::launder( reinterpret_cast< Link * >(
    reinterpret_cast< std::byte * >( x ) + kTower ) ) );
  }

 public:
/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the iterator on the pairs, along the bottom level
 class const_iterator
 {
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
  explicit const_iterator( Node * x ) : f_x( x ) { }

  /// the pair of the node
  reference operator*( void ) const {
   return( reference( f_x->key , f_x->value ) );
   }
  /// to the next node of the bottom level
  const_iterator & operator++( void ) {
   f_x = tower( f_x )[ 0 ].next;
   return( *this );
   }
  /// to the next node, returning the iterator before the step
  const_iterator operator++( int ) {
   auto t = *this;
   ++*this;
   return( t );
   }

  /// equal on the same node
  bool operator==( const const_iterator & o ) const = default;

  private:
  Node * f_x = nullptr;
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the distances
 using reference = value_type;            ///< a pair, by value
 using const_reference = value_type;      ///< a pair, by value
 using iterator = const_iterator;         ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_head[ 0 ].next ) );
  }
 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator() );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the iterator on the smallest key not smaller than \f$ k \f$ (the
 /// `find(x)` of an SSet of Morin), `end()` if there is none
 [[nodiscard]] const_iterator lowerBound( const K & k ) const;

 /// the address of the key \f$ k \f$, nil if absent, in \f$ O( \log n ) \f$
 /// expected time: the `find( x )` of a USet of Morin [see `DictionarySet`];
 /// valid until the key is removed
 [[nodiscard]] const K * findKey( const K & k ) const;

 /// the address of the smallest key not smaller than \f$ k \f$, nil if none,
 /// in \f$ O( \log n ) \f$ expected time: the `find( x )` of an SSet of Morin
 /// [see `DictionarySet`]; valid until the key is removed
 [[nodiscard]] const K * lowerBoundKey( const K & k ) const;

 /// the iterator on the key of rank \f$ i \f$, i.e., with \f$ i \f$ keys
 /// smaller than it, in \f$ O( \log n ) \f$ expected time (the `get(i)` of
 /// exercise 4.9 of Morin)
 ///
 /// In pseudocode (Pugh, beyond the book), \f$ j \f$ the rank of \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( i ) \cr
 /// 2 & \quad x \gets \mathit{head};\ j \gets -1 \cr
 /// 3 & \quad \textbf{for}\ l \gets L - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ j + x.w[ l ] \le i\ \textbf{do}\ j \gets j +
 ///     x.w[ l ];\ x \gets x.\mathit{next}[ l ] \cr
 /// 5 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 [[nodiscard]] const_iterator select( std::size_t i ) const;

 private:
 /// the tower of \f$ x \f$, that of the head if \f$ x \f$ is nil
 Link * towerOf( Node * x ) const {
  return( x ? tower( x ) : const_cast< Link * >( f_head ) );
  }

 /// the last node before \f$ k \f$ on each level in `update` and its rank
 /// plus one in `rank`; the node of \f$ k \f$, or nil
 Node * before( const K & k , Node * update[ kMax ] ,
		std::size_t rank[ kMax ] ) const;

 /// a new node of \f$ L \f$ levels
 static Node * make( const K & k , const V & v , int L );

 /// frees the node \f$ x \f$
 static void dispose( Node * x ) noexcept;

 /// the keys sorted and the widths consistent, \f$ O( n \log n ) \f$
 [[nodiscard]] bool valid( void ) const;

 Link f_head[ kMax ];     ///< the tower of the head
 int f_levels = 0;        ///< the levels in use
 std::size_t f_size = 0;  ///< the number of keys
 SplitMix64 f_g;          ///< the generator of the levels

 };  // end( class IndexedSkipList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/IndexedSkipList.tpp"

/*--------------------------------------------------------------------------*/

#endif /* IndexedSkipList.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File IndexedSkipList.h -----------------------*/
/*--------------------------------------------------------------------------*/
