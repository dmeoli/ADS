/*--------------------------------------------------------------------------*/
/*--------------------------- File FingerTreap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The treap with finger search of exercise 7.9 of Morin, beyond the book: a
 * treap [see `Treap.h`] whose nodes keep their parent, the size of their
 * subtree and its smallest and largest node, so that a search may start from
 * any node \f$ u \f$ rather than from the root: it climbs to the first
 * ancestor \f$ w \f$ whose subtree spans the key,
 * \f$ w.\mathit{min} \le k \le w.\mathit{max} \f$, and goes down from it as
 * usual. The climb and the descent take \f$ O( 1 + \log r ) \f$ expected time,
 * \f$ r \f$ the keys between \f$ k \f$ and the key of \f$ u \f$: the lowest
 * common ancestor of the two nodes in a treap is the node of largest priority
 * among the \f$ r + 1 \f$ keys between them, and the expected depth of a node
 * below it, within that range, is \f$ O( \log r ) \f$ (Lemma 7.1 of Morin
 * applied to the range), which is the argument of Seidel and Aragon for their
 * finger searches. `fingerLookup()` keeps the node of the last search as the
 * finger (part 3 of the exercise), so that a sequence of searches of keys
 * near each other costs \f$ O( 1 + \log r ) \f$ each instead of
 * \f$ O( \log n ) \f$; `lookupFrom()` starts from a node given by `finger()`
 * (part 2). `select()` and `rank()` read the sizes (exercise 7.10).
 *
 * It is a DICTIONARY [see `ADT.h`] with the `find(x)` of an SSet
 * (`lowerBound()`); an insertion puts the new node in a leaf and rotates it up
 * while its priority beats its parent's, a removal rotates the node down to a
 * leaf, and both then walk up to the root to correct the sizes and the
 * extremes, \f$ O( \log n ) \f$ expected time, as a rotation corrects those of
 * its two nodes in \f$ O( 1 ) \f$. A removal brings the finger back to the
 * root. The priorities come from `SplitMix64` [see `RandomHashing.h`].
 *
 * Where it is meant to compare with the `Treap` of Morin, for the benchmarks
 * to check: a node is 3 words larger (the size and the two extremes); the
 * plain operations should cost a little more, and the searches of keys near
 * the previous one much less, \f$ O( \log r ) \f$ against
 * \f$ O( \log n ) \f$.
 *
 * The contracts [see `Contract.h`]: a finger is a node of the treap, and
 * `select()` needs a rank below `size()`; `insert()` ensures that the key is
 * there and `remove()` that it is not; the invariant is the order of the keys,
 * the heap order of the priorities, the parents, the sizes and the extremes,
 * an \f$ O( n ) \f$ check at the level 3. The operations are defined in
 * `FingerTreap.tpp`.
 *
 * \see R. Seidel, C. R. Aragon, Randomized search trees, Algorithmica
 *      16(4-5), 464-497, 1996, doi:10.1007/BF01940876
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 7.2 and exercises 7.9 and 7.10,
 *      doi:10.15215/aupress/9781927356388.01
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

#ifndef __ADS_FingerTreap
 #define __ADS_FingerTreap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS FingerTreap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a treap with the parents, the sizes and the extremes of
/// the subtrees, searched from a finger
template< std::totally_ordered K , class V >
class FingerTreap {

 struct Node;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// a node of the treap, from which a search may start; valid until the
 /// node is removed
 class Finger {
  public:
  /// no node: a search from it starts at the root
  Finger( void ) = default;
  private:
  friend class FingerTreap;
  explicit Finger( Node * u ) : f_u( u ) {}
  Node * f_u = nullptr;
  };

 /// the empty treap, its priorities drawn from the seed
 explicit FingerTreap( std::uint64_t seed = 1 ) : f_g( seed ) {}

 /// no copy, since the nodes are linked by pointers
 FingerTreap( const FingerTreap & ) = delete;

 /// no copy, since the nodes are linked by pointers
 FingerTreap & operator=( const FingerTreap & ) = delete;

 /// the nodes of `other`, which is left empty
 FingerTreap( FingerTreap && other ) noexcept
  : f_root( other.f_root ) , f_size( other.f_size ) , f_g( other.f_g ) {
  other.f_root = other.f_finger = nullptr;
  other.f_size = 0;
  }

 /// frees the nodes
 ~FingerTreap();

 /// the value of \f$ k \f$, or nothing, from the root, in
 /// \f$ O( \log n ) \f$ expected time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of \f$ k \f$, or nothing, by a search from the node of the
 /// last `fingerLookup()`, which becomes the node where this one ends, in
 /// \f$ O( 1 + \log r ) \f$ expected time (exercise 7.9 of Morin)
 [[nodiscard]] std::optional< V > fingerLookup( const K & k );

 /// the value of \f$ k \f$, or nothing, by a search from \f$ f \f$, in
 /// \f$ O( 1 + \log r ) \f$ expected time, \f$ r \f$ the keys between
 /// \f$ k \f$ and that of \f$ f \f$ (the `fingerFind(x,u)` of exercise 7.9
 /// of Morin)
 ///
 /// In pseudocode (exercise 7.9 of Morin, and Seidel and Aragon):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{fingerFind}( k , u ) \cr
 /// 2 & \quad w \gets u \cr
 /// 3 & \quad \textbf{while}\ w.\mathit{parent} \ne \textbf{nil}
 ///     \ \textbf{and}\ ( k < w.\mathit{min}.\mathit{key}\ \textbf{or}
 ///     \ k > w.\mathit{max}.\mathit{key} )\ \textbf{do}\ w \gets
 ///     w.\mathit{parent} \cr
 /// 4 & \quad \textbf{while}\ w \ne \textbf{nil}\ \textbf{and}
 ///     \ w.\mathit{key} \ne k\ \textbf{do}\ w \gets w.\mathit{left}
 ///     \text{ if } k < w.\mathit{key} \text{, else } w.\mathit{right} \cr
 /// 5 & \quad \textbf{return}\ w
 /// \end{array}}
 /// \f]
 /// \pre \f$ f \f$ is a node of this treap, or empty
 [[nodiscard]] std::optional< V > lookupFrom( const K & k ,
                                              Finger f ) const;

 /// the node of \f$ k \f$, or the last node of its search from the root,
 /// as a finger
 [[nodiscard]] Finger finger( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ expected
 /// time: a leaf rotated up while its priority beats its parent's, then the
 /// sizes and the extremes corrected up to the root
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ expected
 /// time: the node rotated down to a leaf, the child of larger priority
 /// going up, and spliced, then the sizes and the extremes corrected up to
 /// the root
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the height, \f$ -1 \f$ if empty, in \f$ O( n ) \f$
 [[nodiscard]] int height( void ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet of Morin)
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
  const K & k ) const;

 /// the pair of rank \f$ i \f$, by the sizes, as `Treap::select()`
 /// \pre \f$ i < n \f$
 [[nodiscard]] std::pair< K , V > select( std::size_t i ) const;

 /// the number of keys smaller than \f$ k \f$, by the sizes
 [[nodiscard]] std::size_t rank( const K & k ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, with its parent, its priority, the size of its subtree and its
 /// extremes
 struct Node {
  K key;                       ///< the key
  V value;                     ///< the value
  std::uint64_t priority;      ///< the random priority
  Node * left = nullptr;       ///< the left child
  Node * right = nullptr;      ///< the right child
  Node * parent = nullptr;     ///< the parent
  Node * lo = nullptr;         ///< the smallest node of the subtree
  Node * hi = nullptr;         ///< the largest node of the subtree
  std::size_t size = 1;        ///< the nodes of the subtree
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the shape of the tree, for `PathIterator`
 struct Shape {
  /// the root of the tree `o`
  static auto root( const auto & o ) { return( o.f_root ); }
  /// the left child of the node `n`
  static auto left( const auto * n ) { return( n->left ); }
  /// the right child of the node `n`
  static auto right( const auto * n ) { return( n->right ); }
  /// the pair of the node `n`
  static auto read( const auto * n ) {
   return( std::pair< const K & , const V & >( n->key , n->value ) );
   }
  };

 /// the pairs of references to a key and its value
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = value_type;             ///< a pair, by value
 using const_reference = value_type;       ///< a pair, by value
 /// the constant iterator by increasing key, along the path from the root
 using const_iterator = PathIterator< FingerTreap , Shape >;
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the node of \f$ k \f$ or the last of its search from \f$ u \f$
 Node * search( const K & k , Node * u ) const;

 /// the size and the extremes of \f$ u \f$ from its children
 static void update( Node * u ) {
  u->size = 1 + ( u->left ? u->left->size : 0 ) +
            ( u->right ? u->right->size : 0 );
  u->lo = u->left ? u->left->lo : u;
  u->hi = u->right ? u->right->hi : u;
  }

 /// the rotation that lifts the left child of \f$ u \f$
 void rotateRight( Node * u );

 /// the rotation that lifts the right child of \f$ u \f$
 void rotateLeft( Node * u );

 /// the order, the heap, the parents, the sizes and the extremes
 [[nodiscard]] bool valid( void ) const;

 Node * f_root = nullptr;     ///< the root
 Node * f_finger = nullptr;   ///< the node of the last finger search
 std::size_t f_size = 0;      ///< the number of keys
 SplitMix64 f_g;              ///< the generator of the priorities

 };  // end( class FingerTreap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/FingerTreap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FingerTreap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File FingerTreap.h -------------------------*/
/*--------------------------------------------------------------------------*/
