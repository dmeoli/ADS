/*--------------------------------------------------------------------------*/
/*------------------------- File SkipListSequence.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The `SkiplistList` of Morin, beyond the book: a sequence with access,
 * insertion and removal at any index in \f$ O( \log n ) \f$ expected time,
 * on a skip list [see `SkipList.h`] whose links carry their <em>width</em>,
 * the number of items they jump over. The items are in the order of the
 * sequence on the bottom level, each node is in the levels 0 to
 * \f$ \ell - 1 \f$ with probability \f$ 2^{-\ell} \f$, and a link from the
 * node of index \f$ i \f$ (the head has index \f$ -1 \f$) to the node of
 * index \f$ j \f$ has width \f$ j - i \f$, a link to nil reaching the index
 * \f$ n \f$; a search of the index \f$ i \f$ goes along a level while the
 * widths summed stay below \f$ i \f$ and drops a level otherwise, which is
 * the search by key of a skip list with the positions as keys. An insertion
 * at \f$ i \f$ splits the link that it crosses on each of its levels and
 * adds 1 to the width of the links that pass over it on the levels above, a
 * removal the reverse.
 *
 * `truncate( i )` (exercise 4.11 of Morin) cuts the list after the first
 * \f$ i \f$ items and returns the rest as a list of its own, and
 * `absorb( l )` (exercise 4.12) appends the items of another list, which it
 * leaves empty: both relink one node per level, at the end of the search of
 * the index \f$ i \f$ or of the last node of each level, in
 * \f$ O( \log n ) \f$ expected time, while a vector would move
 * \f$ \Theta( n ) \f$ items. A node is one block of memory, its item and the
 * tower of its links, as in `SkipList`; the generator is `SplitMix64` [see
 * `RandomHashing.h`], seeded.
 *
 * Where it is meant to be faster than the `SkiplistList` of Morin, for the
 * benchmarks to check: his node is three blocks of memory (the node, the
 * array of the successors and that of the widths), so that a step of a
 * search touches three lines, while here a link and its width are
 * adjacent in the one block of the node, one line per step; the widths are
 * words, not `int`, so that a list may pass \f$ 2^{31} \f$ items; and the
 * levels come from `SplitMix64` instead of `rand()`.
 *
 * The contracts [see `Contract.h`]: an index is below `size()` (at most
 * `size()` for an insertion and a truncation); the invariant, an
 * \f$ O( n \log n ) \f$ check at the level 3, is that the widths of each
 * level sum to \f$ n + 1 \f$ and that each level is a sublist of the level
 * below. The operations are defined in `SkipListSequence.tpp`.
 *
 * \see W. Pugh, A skip list cookbook, Technical report CS-TR-2286.1,
 *      University of Maryland, 1990 (the widths of the links)
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 4.3 and exercises
 *      4.11 and 4.12
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

#ifndef __ADS_SkipListSequence
 #define __ADS_SkipListSequence
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <new>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS SkipListSequence -------------------------*/
/*--------------------------------------------------------------------------*/

/// a sequence of items of type `T` on a skip list with the widths of the
/// links, the `SkiplistList` of Morin
template< class T >
class SkipListSequence {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;    ///< the type of the items

 /// the empty sequence, its levels drawn by a generator from the seed
 explicit SkipListSequence( std::uint64_t seed = 1 );

 /// no copy, since the nodes are linked by pointers
 SkipListSequence( const SkipListSequence & ) = delete;

 /// no copy, since the nodes are linked by pointers
 SkipListSequence & operator=( const SkipListSequence & ) = delete;

 /// the nodes of `other`, which is left empty
 SkipListSequence( SkipListSequence && other ) noexcept;

 /// frees the memory of the structure
 ~SkipListSequence();

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_size == 0 ); }

 /// the number of levels in use
 [[nodiscard]] int levels( void ) const { return( f_levels ); }

 /// the item of index \f$ i \f$, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (section 4.3 of Morin), \f$ x.\mathit{next}[ l ] \f$ and
 /// \f$ x.w[ l ] \f$ the successor of \f$ x \f$ on the level \f$ l \f$ and the
 /// width of the link, \f$ j \f$ the index of \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{get}( i ) \cr
 /// 2 & \quad x \gets \mathit{head};\ j \gets -1 \cr
 /// 3 & \quad \textbf{for}\ l \gets L - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ j + x.w[ l ] \le i\ \textbf{do}\ j \gets j +
 ///     x.w[ l ];\ x \gets x.\mathit{next}[ l ] \cr
 /// 5 & \quad \textbf{return}\ x.\mathit{item}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & get( std::size_t i ) const;

 /// the item of index \f$ i \f$ replaced by \f$ v \f$, by the search of
 /// `get()`
 /// \pre \f$ i < n \f$
 void set( std::size_t i , const T & v );

 /// \f$ v \f$ inserted at the index \f$ i \f$, the items from \f$ i \f$ on
 /// moving one place up, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (section 4.3 of Morin), the node \f$ y \f$ of \f$ \ell \f$
 /// levels, \f$ \ell \f$ drawn as in `SkipList`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{add}( i , v ) \cr
 /// 2 & \quad y \gets \text{a node of } v \text{ and } \ell
 ///     \text{ levels};\ x \gets \mathit{head};\ j \gets -1 \cr
 /// 3 & \quad \textbf{for}\ l \gets L - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ j + x.w[ l ] < i\ \textbf{do}\ j \gets j +
 ///     x.w[ l ];\ x \gets x.\mathit{next}[ l ] \cr
 /// 5 & \qquad \textbf{if}\ l \ge \ell\ \textbf{then}\ x.w[ l ] \gets x.w[ l
 ///     ] + 1 \cr
 /// 6 & \qquad \textbf{else}\ y.\mathit{next}[ l ] \gets x.\mathit{next}[ l
 ///     ];\ y.w[ l ] \gets j + x.w[ l ] + 1 - i;\ x.\mathit{next}[ l ]
 ///     \gets y;\ x.w[ l ] \gets i - j
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 void add( std::size_t i , const T & v );

 /// the item of index \f$ i \f$, as `get( i )` (the name of `IndexedList`)
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  return( get( i ) );
  }

 /// the item of index \f$ i \f$, to be read or written, by the search of
 /// `get()`
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < size() );
  return( nodeAt( i )->item );
  }

 /// \f$ v \f$ inserted at \f$ i \f$, `add( i , v )` (the name of
 /// `IndexedList`)
 /// \pre \f$ i \le n \f$
 void insert( std::size_t i , const T & v ) { add( i , v ); }

 /// the item of index \f$ i \f$ removed, `remove( i )` (the name of
 /// `IndexedList`)
 /// \pre \f$ i < n \f$
 void erase( std::size_t i ) { ( void ) remove( i ); }

 /// \f$ v \f$ appended, `add( size() , v )`
 void push_back( const T & v ) { add( f_size , v ); }

 /// the item of index \f$ i \f$ removed and returned, the items after it
 /// moving one place down, in \f$ O( \log n ) \f$ expected time: on each
 /// level the link that reaches the node is replaced by the one that
 /// leaves it, their widths summed less 1, and the links that pass over it
 /// lose 1
 /// \pre \f$ i < n \f$
 T remove( std::size_t i );

 /// the items of index \f$ i \f$ to \f$ n - 1 \f$ taken out and returned as
 /// a sequence, in \f$ O( \log n ) \f$ expected time (exercise 4.11 of
 /// Morin)
 ///
 /// In pseudocode (beyond the book), \f$ r \f$ the new sequence, whose
 /// head takes on each level the link that leaves the last node before the
 /// index \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{truncate}( i ) \cr
 /// 2 & \quad x \gets \mathit{head};\ j \gets -1 \cr
 /// 3 & \quad \textbf{for}\ l \gets L - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ j + x.w[ l ] < i\ \textbf{do}\ j \gets j +
 ///     x.w[ l ];\ x \gets x.\mathit{next}[ l ] \cr
 /// 5 & \qquad r.\mathit{head}.\mathit{next}[ l ] \gets x.\mathit{next}[ l
 ///     ];\ r.\mathit{head}.w[ l ] \gets j + x.w[ l ] - i + 1 \cr
 /// 6 & \qquad x.\mathit{next}[ l ] \gets \textbf{nil};\ x.w[ l ] \gets i -
 ///     j \cr
 /// 7 & \quad \text{drop the levels left empty in both};\ \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 [[nodiscard]] SkipListSequence truncate( std::size_t i );

 /// the items of \f$ s \f$ appended, in order, and \f$ s \f$ left empty, in
 /// \f$ O( \log n ) \f$ expected time (exercise 4.12 of Morin)
 ///
 /// In pseudocode (beyond the book), \f$ m \f$ the items of \f$ s \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{absorb}( s ) \cr
 /// 2 & \quad x \gets \mathit{head};\ j \gets -1 \cr
 /// 3 & \quad \textbf{for}\ l \gets \max\{ L , L_s \} - 1\ \textbf{downto}\ 0
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ x.\mathit{next}[ l ] \ne \textbf{nil}
 ///     \ \textbf{do}\ j \gets j + x.w[ l ];\ x \gets x.\mathit{next}[ l ]
 ///     \cr
 /// 5 & \qquad x.\mathit{next}[ l ] \gets s.\mathit{head}.\mathit{next}[ l
 ///     ];\ x.w[ l ] \gets n + s.\mathit{head}.w[ l ] - 1 - j \cr
 /// 6 & \quad n \gets n + m;\ s \gets \text{the empty sequence}
 /// \end{array}}
 /// \f]
 void absorb( SkipListSequence & s );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kMax = 32;   ///< the largest number of levels

 struct Node;

 /// a link of a level: the successor and the width
 struct Link {
  Node * next;                     ///< the successor, nil at the end
  std::size_t w;                   ///< the indices it advances by
  };

 /// a node, followed in its block by the tower of its links
 struct Node {
  T item;                          ///< the item
  int height;                      ///< the number of levels
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

 /// the forward iterator on the items, along the bottom level
 template< bool Const >
 class basic_iterator {
  public:
  /// the type of the items
  using value_type = T;
  /// a reference to an item
  using reference = std::conditional_t< Const , const T & , T & >;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// the concept and the category of the iterator
  using iterator_category = std::forward_iterator_tag;

  /// the end
  basic_iterator( void ) = default;

  /// on the node \f$ x \f$
  explicit basic_iterator( Node * x ) : f_x( x ) {}

  /// the constant form of a mutable iterator
  operator basic_iterator< true >( void ) const {
   return( basic_iterator< true >( f_x ) );
   }

  /// the item of the node
  reference operator*( void ) const { return( f_x->item ); }
  /// to the next node of the bottom level
  basic_iterator & operator++( void ) {
   f_x = tower( f_x )[ 0 ].next;
   return( *this );
   }
  /// to the next node, returning the iterator before the step
  basic_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same node
  bool operator==( const basic_iterator & o ) const = default;

  private:
  Node * f_x = nullptr;
  };

 using value_type = T;                          ///< the items
 using size_type = std::size_t;                 ///< the type of the sizes
 using difference_type = std::ptrdiff_t;        ///< the distances
 using reference = T &;                         ///< a reference to an item
 using const_reference = const T &;             ///< a constant reference
 using iterator = basic_iterator< false >;      ///< the iterator
 using const_iterator = basic_iterator< true >; ///< the constant iterator

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) {
  return( iterator( f_head[ 0 ].next ) );
  }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_head[ 0 ].next ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the tower of \f$ x \f$, that of the head if \f$ x \f$ is nil
 Link * towerOf( Node * x ) const {
  return( x ? tower( x ) : const_cast< Link * >( f_head ) );
  }

 /// the node of index \f$ i \f$
 Node * nodeAt( std::size_t i ) const;

 /// a new node of \f$ L \f$ levels
 static Node * make( const T & v , int L );

 /// frees the node \f$ x \f$
 static void dispose( Node * x ) noexcept;

 /// the head of the empty sequence on all the levels
 void clearHead( void );

 /// the widths and the levels consistent, \f$ O( n \log n ) \f$
 [[nodiscard]] bool valid( void ) const;

 Link f_head[ kMax ];              ///< the tower of the head
 int f_levels = 0;                 ///< the levels in use
 std::size_t f_size = 0;           ///< the number of items
 SplitMix64 f_g;                   ///< the generator of the levels

 };  // end( class SkipListSequence )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/SkipListSequence.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SkipListSequence.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SkipListSequence.h ----------------------*/
/*--------------------------------------------------------------------------*/
