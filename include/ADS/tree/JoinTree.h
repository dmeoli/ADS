/*--------------------------------------------------------------------------*/
/*---------------------------- File JoinTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search trees balanced by join alone, and the augmented maps,
 * beyond the book: `JoinTree< K , V , B , G >` is a DICTIONARY [see
 * `ADT.h`] of keys `K` and values `V` whose only balancing operation is the
 * join of the scheme `B` [see `JoinAlgorithms.h`], `AVLBalance`,
 * `RedBlackBalance`, `WeightBalance` or `TreapBalance`, so that the
 * insertions and the removals are a split or a descent followed by joins,
 * and the bulk operations of `Joinable` (union, intersection, difference,
 * batches of insertions and of removals, filter, map and reduce, extraction
 * of a range) are those of the four schemes alike. A node keeps the size of
 * its subtree and one word for its scheme: the height plus 1 for AVL, the
 * black height and the color for Red-Black, the priority for the treap
 * (the weight is the size); `JoinAVLTree`, `JoinRedBlackTree`,
 * `WeightBalancedTree` and `JoinTreap` name the four.
 *
 * The augmentation `G` is a monoid on a type `G::Value`, of identity
 * `G::identity()` and associative operation `G::combine()`, and a map
 * `G::from( k , v )` of a pair into it [see `Augmentation`]. A node
 * \f$ u \f$ keeps the augmented value of its subtree \f$ T_u \f$,
 * \f[
 *   \mathit{aug}( u ) = \mathit{aug}( u.\mathit{left} ) \otimes
 *   \mathit{from}( u.k , u.v ) \otimes \mathit{aug}( u.\mathit{right} ) ,
 * \f]
 * the combination of the values of its pairs by increasing keys, which
 * need not be commutative; `connect()` recomputes it, so that every join,
 * and every operation built on join, keeps it at the cost of one
 * combination per node touched. `augValue()` is that of the root, in
 * \f$ O( 1 ) \f$; `augRange( lo , hi )` combines the values of the
 * \f$ O( \log n ) \f$ pieces that cover the keys of
 * \f$ [ \mathit{lo} , \mathit{hi} ] \f$: from the node where the paths to
 * \f$ \mathit{lo} \f$ and to \f$ \mathit{hi} \f$ part, the pairs met on the
 * path to \f$ \mathit{lo} \f$ with the right subtrees it leaves behind, the
 * pair of the node, and the left subtrees and the pairs of the path to
 * \f$ \mathit{hi} \f$; `augPrefix( k )` does the same on the path to
 * \f$ k \f$ alone, and `augVisit()` gives the pieces themselves, in order,
 * to a function (the range tree counts on them [see `RangeTree.h`]).
 * `augSelect( p )` descends to the first pair whose prefix satisfies a
 * monotone predicate (the \f$ i \f$-th key on the counts, the first prefix
 * whose sum reaches a threshold), and `augFilter( q , f )` gives the pairs
 * whose value satisfies a predicate that distributes over the combination,
 * entering only the subtrees whose augmented value satisfies it, in time
 * proportional to the pairs given (see \ref theory_tree_observed).
 * `SumAugmentation` and `MaxAugmentation` are the sums and the maxima of
 * the values, and `AugmentedMap` the map augmented by `G` and balanced by
 * weight, as in PAM; the interval tree [see `IntervalTree.h`] and the range
 * tree are augmented maps.
 *
 * An augmentation may declare its algebra by static members, which refine
 * `Augmentation` by the concepts of `Algebra.h`: `commutative`,
 * `inverse( a )` (a group), `selective` (the combination of two values is
 * one of them), as `SumAugmentation` (an abelian group when its values
 * have opposites) and `MaxAugmentation` (commutative and selective) do.
 * `assign( k , v )` changes the value of a key that is there and keeps the
 * shape: when `G` is an abelian group of exact values, it combines each
 * augmented value of the path to \f$ k \f$ with the difference of the new
 * and the old value, otherwise it
 * recomputes them from the children, from \f$ k \f$ up.
 *
 * The changes go to the observers of the subject policy `Policy` [see
 * `Modification.h`], `NoObservers` by default, which adds neither a member
 * nor an instruction: `Inserted`, `Erased` (told before the removal) and
 * `Assigned` of the key, whose handle is the key itself, by `insert()`,
 * `remove()` and `assign()` (an insertion of a key that is there is an
 * assignment), `Cleared` by `clear()` and on a tree whose nodes a bulk
 * operation takes (the second tree of a union, a tree moved from), and
 * `Rebuilt` on a tree whose nodes it puts back (the first tree of a union,
 * the trees of a split, both trees of a swap). With someone listening, an
 * insertion and a removal make one more descent, to read the old value;
 * an observer that throws from `on()` of `Inserted` or `Assigned` gets the
 * change undone (the strong guarantee). A tree of static observers is not
 * copied nor moved, and has no split; a copy or a move of the others has
 * no observer.
 *
 * The nodes come from the allocator `A`, `std::allocator` by default (by
 * `new` and `delete`), which must be stateless, its allocators all equal
 * and default-constructible (e.g., `HugePageAllocator` or
 * `mi_stl_allocator` of mimalloc): the algorithms of `JoinAlgorithms.h`
 * build and free the nodes with no tree at hand, and in parallel by the set
 * operations, so that a stateful allocator, as one of `std::pmr`, has
 * nowhere to be kept.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is
 * there, `remove()` that it is not and `assign()` that the key has its new
 * value; the predicate of `augSelect()` is monotone on the prefixes, that
 * of `augFilter()` false on the identity and distributive (not checked);
 * the operations of a group are total on the values; the invariant is the
 * order of the keys, the sizes, the augmented values (when `G::Value` is
 * comparable) and the balance of the scheme (the heights of AVL; no red
 * node with a red child and the same black height on every path, the root
 * black, for Red-Black; \f$ \alpha ( w_l + w_r ) \le \min( w_l , w_r ) \f$
 * for the weights of the two children of every node; the heap order of the
 * priorities of the treap), an \f$ O( n ) \f$ check at the level 3. The
 * operations are defined in `JoinTree.tpp`.
 *
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Joinable parallel balanced
 *      binary trees, ACM Transactions on Parallel Computing 9(2), 1-41,
 *      2022, doi:10.1145/3512769
 * \see Y. Sun, D. Ferizovic, G. E. Blelloch, PAM: parallel augmented maps,
 *      Proceedings of the 23rd ACM SIGPLAN Symposium on Principles and
 *      Practice of Parallel Programming (PPoPP), 290-304, 2018,
 *      doi:10.1145/3178487.3178509
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, chapter 14 (augmenting the
 *      data structures)
 * \see J. Nievergelt, E. M. Reingold, Binary search trees of bounded
 *      balance, SIAM Journal on Computing 2(1), 33-43, 1973,
 *      doi:10.1137/0202005
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

#ifndef __ADS_JoinTree
 #define __ADS_JoinTree
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Algebra.h"
#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/Modification.h"
#include "ADS/Portable.h"
#include "ADS/memory/AllocatorAware.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/JoinAlgorithms.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ SCHEMES AND AUGMENTATIONS -----------------------*/
/*--------------------------------------------------------------------------*/

struct AVLBalance
{ };  ///< the AVL trees [see `AVLJoin`]
struct RedBlackBalance
{ };  ///< the Red-Black trees [see `RedBlackJoin`]
struct WeightBalance
{ };  ///< the weight-balanced trees
struct TreapBalance
{ };  ///< the treaps [see `TreapJoin`]

/// an augmentation of a map from `K` to `V`: a monoid on `G::Value`, of
/// identity `G::identity()` and associative operation
/// `G::combine( a , b )`, and the value `G::from( k , v )` of a pair; the
/// augmented value of a subtree is the combination of the values of its
/// pairs by increasing keys
template< class G , class K , class V >
concept Augmentation =
  requires( const K & k , const V & v , const typename G::Value & a )
{
   { G::identity() } -> std::convertible_to< typename G::Value >;
   { G::from( k , v ) } -> std::convertible_to< typename G::Value >;
   { G::combine( a , a ) } -> std::convertible_to< typename G::Value >;
   };

/// \cond
namespace detail
{

/// whether a tree of the subject policy `P` can be made with no observer
/// (a copy, a move, the second tree of a split)
template< class P >
inline constexpr bool unbound_v = std::is_same_v< P , NoObservers >;

template< class L >
inline constexpr bool unbound_v< Dynamic< L > > = true;

}  // end( namespace detail )
/// \endcond

/// no augmentation: nothing is kept, nothing computed
struct NoAugmentation
{
 /// the empty value
 struct Value
 {
  /// all equal
  bool operator==( const Value & ) const = default;
  };
 /// the empty value
 static Value identity( void ) { return( Value() ); }
 /// the empty value
 template< class K , class V >
 static Value from( const K & , const V & ) {
  return( Value() );
  }
 /// the empty value
 static Value combine( const Value & , const Value & ) { return( Value() ); }
 };

/// the sum of the values, an abelian group when `V` has the opposites
template< class V >
struct SumAugmentation
{
 using Value = V;  ///< the sums, of the type of the values
 static constexpr bool commutative = true;  ///< \f$ a + b = b + a \f$
 /// 0, the value made by `V()`
 static Value identity( void ) { return( V() ); }
 /// the value \f$ v \f$ of the pair
 template< class K >
 static Value from( const K & , const V & v ) {
  return( v );
  }
 /// \f$ a + b \f$
 static Value combine( const Value & a , const Value & b ) { return( a + b ); }
 /// \f$ -a \f$
 static Value inverse( const Value & a )
  requires requires { -a; }
 {
  return( -a );
  }
 };

/// the largest of the values, whose identity is \f$ -\infty \f$ if `V`
/// has it, its smallest value otherwise: a commutative and selective
/// monoid
template< class V >
struct MaxAugmentation
{
 using Value = V;  ///< the maxima, of the type of the values
 static constexpr bool commutative = true;  ///< the order does not count
 static constexpr bool selective = true;    ///< the maximum is one of two
 /// the smallest value of the type
 static Value identity( void ) {
  if constexpr( std::numeric_limits< V >::has_infinity )
   return( -std::numeric_limits< V >::infinity() );
  else
   return( std::numeric_limits< V >::lowest() );
  }
 /// the value \f$ v \f$ of the pair
 template< class K >
 static Value from( const K & , const V & v ) {
  return( v );
  }
 /// \f$ \max( a , b ) \f$
 static Value combine( const Value & a , const Value & b ) {
  return( a < b ? b : a );
  }
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS JoinTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a binary search tree balanced by the join of `B` alone,
/// augmented by `G`, whose changes go to the observers of the subject
/// policy `Policy` [see `Modification.h`]
template< std::totally_ordered K , class V , class B = AVLBalance ,
	  class G = NoAugmentation , class Policy = NoObservers ,
	  class A = std::allocator< std::pair< const K , V > > >
 requires Augmentation< G , K , V >
class JoinTree;

/// the adaptor of the nodes of a `JoinTree` [see `JoinAdaptor`]
template< std::totally_ordered K , class V , class B , class G , class Policy ,
	  class A >
struct JoinTreeNodes;

template< std::totally_ordered K , class V , class B , class G , class Policy ,
	  class A >
 requires Augmentation< G , K , V >
class JoinTree
    : public Joinable< JoinTree< K , V , B , G , Policy , A > ,
		       JoinTreeNodes< K , V , B , G , Policy , A > , K , V >
{

 friend struct JoinTreeNodes< K , V , B , G , Policy , A >;
 using Nodes = JoinTreeNodes< K , V , B , G , Policy , A >;

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;                       ///< the type of the keys
 using Value = V;                     ///< the type of the values
 using AugValue = typename G::Value;  ///< the type of the augmentation
 using Augmented = G;                 ///< the augmentation
 using allocator_type = A;            ///< the allocator of the nodes

 /// the allocator of the nodes, stateless
 [[nodiscard]] allocator_type get_allocator( void ) const {
  return( allocator_type() );
  }

 /// the insertion of a key, whose handle is the key itself
 using Inserted = mod::Inserted< K , K , V >;
 /// the removal of a key, told before it is made outside a batch
 using Erased = mod::Erased< K , K , V >;
 /// the value of a key that is there changed
 using Assigned = mod::Assigned< K , V >;
 /// the events: `Cleared` by `clear()` and on the trees that a bulk
 /// operation consumes, `Rebuilt` on the tree that it leaves
 using modifications =
   mod::list< Inserted , Erased , Assigned , mod::Cleared , mod::Rebuilt >;
 /// the subject [see `Modification.h`]
 using Subject = typename Policy::template bind< modifications , JoinTree >;

 /// a node: what the navigation below reads
 struct Node
 {
  Node * left = nullptr;   ///< the left child
  Node * right = nullptr;  ///< the right child
  std::size_t size = 1;    ///< the nodes of the subtree
  /// the height plus 1 (AVL), the black height times 2 plus 1 if red
  /// (Red-Black), the priority (treap)
  std::uint64_t meta = 0;
  ADS_NO_UNIQUE_ADDRESS AugValue aug{};  ///< the augmented value
  K key;                                 ///< the key
  V value;                               ///< the value
  };

 /// an empty tree; the seed of the priorities, for a treap
 explicit JoinTree( std::uint64_t seed = 1 )
  requires detail::unbound_v< Policy >;

 /// an empty tree whose changes go to the static observers `o...` of the
 /// policy; the seed of the priorities, for a treap
 template< class... O >
  requires( sizeof...( O ) > 0 )
 JoinTree( std::uint64_t seed , O &... o );

 /// a copy, node by node with the same shape, in \f$ O( n ) \f$ time,
 /// with no observer (none of a policy with static observers, by a static
 /// assertion)
 JoinTree( const JoinTree & o );

 /// the nodes of `o`, which is left empty (and tells `Cleared`), in
 /// \f$ O( 1 ) \f$ time, with no observer
 JoinTree( JoinTree && o ) noexcept
  requires detail::unbound_v< Policy >;

 /// the nodes of `o`, by copy and swap
 JoinTree & operator=( JoinTree o ) noexcept
  requires detail::unbound_v< Policy >;

 /// frees the nodes
 ~JoinTree();

 /// exchanges the nodes with `o`; the observers of both get `Rebuilt`
 void swap( JoinTree & o ) noexcept;

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }

 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

/*--------------------------------------------------------------------------*/
/*------------------------- DICTIONARY OPERATIONS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the value of \f$ k \f$, or nothing, by the descent of a binary search
 /// tree, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ time; tells
 /// `Inserted`, or `Assigned` if \f$ k \f$ was there (by one more descent,
 /// only when someone listens)
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), the
 /// insertion by joins of `JoinAlgorithms::insert()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad T \gets \mathrm{insert}( T , \text{a node alone of } ( k , v )
 ///     )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ time; tells
 /// `Erased` (by one more descent, only when someone listens)
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), the
 /// removal by joins of `JoinAlgorithms::remove()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( k ) \cr
 /// 2 & \quad T \gets \mathrm{remove}( T , k )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// the value of \f$ k \f$ set to \f$ v \f$, and true, if \f$ k \f$ is
 /// there, nothing and false otherwise, in \f$ O( \log n ) \f$ time; the
 /// shape stays, and the augmented values of the path to \f$ k \f$ change
 /// by one combination each with the difference of the old and the new
 /// value when `G` is an abelian group of exact values (`commutative` and
 /// `inverse()`), by the recomputation of the value of each node from its
 /// children otherwise; tells `Assigned`
 ///
 /// In pseudocode (beyond the book), \f$ \otimes \f$ the operation of
 /// `G` and \f$ T \f$ the root:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{assign}( k , v ) \cr
 /// 2 & \quad u \gets \text{the node of } k \text{, by the descent from }
 ///     T;\ \textbf{if}\ u = \textbf{nil}\ \textbf{then return}
 ///     \ \mathit{false} \cr
 /// 3 & \quad \textbf{if}\ G \text{ is an abelian group of exact values}
 ///     \ \textbf{then} \cr
 /// 4 & \qquad d \gets \mathrm{from}( k , v ) \otimes \mathrm{from}( k ,
 ///     u.v )^{-1} \cr
 /// 5 & \qquad \textbf{foreach}\ x \text{ on the path from } T \text{ to }
 ///     u\ \textbf{do}\ \mathit{aug}( x ) \gets \mathit{aug}( x ) \otimes
 ///     d \cr
 /// 6 & \qquad u.v \gets v \cr
 /// 7 & \quad \textbf{else} \cr
 /// 8 & \qquad u.v \gets v \cr
 /// 9 & \qquad \textbf{foreach}\ x \text{ on the path from } u \text{ to }
 ///     T\ \textbf{do}\ \mathit{aug}( x ) \gets \mathit{aug}(
 ///     x.\mathit{left} ) \otimes \mathrm{from}( x ) \otimes \mathit{aug}(
 ///     x.\mathit{right} ) \cr
 /// 10 & \quad \textbf{return}\ \mathit{true}
 /// \end{array}}
 /// \f]
 /// \post if true, \f$ k \f$ has the value \f$ v \f$
 bool assign( const K & k , const V & v );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find( x )` of an SSet of Morin), in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< std::pair< K , V > >
 lowerBound( const K & k ) const;

 /// the address of the key \f$ k \f$, nil if absent, in \f$ O( \log n ) \f$
 /// time: the `find( x )` of a USet of Morin [see `DictionarySet`]; valid
 /// until the key is removed
 [[nodiscard]] const K * findKey( const K & k ) const;

 /// the address of the smallest key not smaller than \f$ k \f$, nil if none,
 /// in \f$ O( \log n ) \f$ time: the `find( x )` of an SSet of Morin [see
 /// `DictionarySet`]; valid until the key is removed
 [[nodiscard]] const K * lowerBoundKey( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there is no key
 [[nodiscard]] bool empty( void ) const;

 /// removes all the pairs; tells `Cleared` if there was one
 void clear( void );

 /// the height of the tree, -1 if empty, in \f$ O( n ) \f$ time
 [[nodiscard]] int height( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
/*------------------------- AUGMENTED QUERIES ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the augmented value of the whole map, in \f$ O( 1 ) \f$ time
 [[nodiscard]] AugValue augValue( void ) const;

 /// the augmented value of the pairs with keys in
 /// \f$ [ \mathit{lo} , \mathit{hi} ] \f$, in \f$ O( \log n ) \f$ calls of
 /// `combine()`, the combination of the pieces of `augVisit()`
 [[nodiscard]] AugValue augRange( const K & lo , const K & hi ) const;

 /// the augmented value of the pairs with keys \f$ \le k \f$, in
 /// \f$ O( \log n ) \f$ calls of `combine()`
 ///
 /// In pseudocode (Sun, Ferizovic and Blelloch, beyond the book), on the
 /// path to \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{augPrefix}( k ) \cr
 /// 2 & \quad a \gets \mathrm{identity}();\ u \gets T \cr
 /// 3 & \quad \textbf{while}\ u \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ k < u.\mathit{key}\ \textbf{then}\ u \gets
 ///     u.\mathit{left} \cr
 /// 5 & \qquad \textbf{else}\ a \gets a \otimes \mathit{aug}( u.\mathit{left}
 ///     ) \otimes \mathrm{from}( u );\ u \gets u.\mathit{right} \cr
 /// 6 & \quad \textbf{return}\ a
 /// \end{array}}
 /// \f]
 [[nodiscard]] AugValue augPrefix( const K & k ) const;

 /// `f( a )` on the augmented values \f$ a \f$ of the \f$ O( \log n ) \f$
 /// pieces (whole subtrees and single pairs) whose union is the set of the
 /// keys in \f$ [ \mathit{lo} , \mathit{hi} ] \f$, by increasing keys
 ///
 /// In pseudocode (Sun, Ferizovic and Blelloch, beyond the book), \f$ t \f$
 /// the node where the paths to \f$ \mathit{lo} \f$ and to
 /// \f$ \mathit{hi} \f$ part:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{augVisit}( \mathit{lo} , \mathit{hi} , f
 ///     ) \cr
 /// 2 & \quad t \gets \text{the first node of the descent from the root }
 ///     \text{with } \mathit{lo} \le t.\mathit{key} \le \mathit{hi} \cr
 /// 3 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
 /// 4 & \quad P \gets \text{the nodes } u \text{ with } u.\mathit{key} \ge
 ///     \mathit{lo} \text{ on the path to } \mathit{lo} \text{ in }
 ///     t.\mathit{left} \cr
 /// 5 & \quad \textbf{foreach}\ u \in P \text{, by increasing keys}
 ///     \ \textbf{do}\ f( \mathrm{from}( u ) );\ f( \mathit{aug}(
 ///     u.\mathit{right} ) ) \cr
 /// 6 & \quad f( \mathrm{from}( t ) ) \cr
 /// 7 & \quad \textbf{foreach}\ u \text{ with } u.\mathit{key} \le
 ///     \mathit{hi} \text{ on the path to } \mathit{hi} \text{ in }
 ///     t.\mathit{right} \text{, in order}\ \textbf{do} \cr
 /// 8 & \qquad f( \mathit{aug}( u.\mathit{left} ) );\ f( \mathrm{from}( u )
 ///     )
 /// \end{array}}
 /// \f]
 template< class F >
 void augVisit( const K & lo , const K & hi , F && f ) const;

 /// the first pair, by increasing keys, whose prefix (the augmented value
 /// of the pairs up to it) satisfies \f$ p \f$, or nothing, in
 /// \f$ O( \log n ) \f$ calls of `combine()` and of \f$ p \f$: e.g., the
 /// \f$ i \f$-th key, \f$ p( a ) = a \ge i \f$ on the sums of values 1,
 /// and the first prefix whose sum reaches a threshold
 ///
 /// In pseudocode (beyond the book), on one descent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{augSelect}( p ) \cr
 /// 2 & \quad a \gets \mathrm{identity}();\ u \gets T \cr
 /// 3 & \quad \textbf{while}\ u \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad b \gets a \otimes \mathit{aug}( u.\mathit{left} ) \cr
 /// 5 & \qquad \textbf{if}\ u.\mathit{left} \ne \textbf{nil}\ \textbf{and}
 ///     \ p( b )\ \textbf{then}\ u \gets u.\mathit{left};
 ///     \ \textbf{continue} \cr
 /// 6 & \qquad b \gets b \otimes \mathrm{from}( u ) \cr
 /// 7 & \qquad \textbf{if}\ p( b )\ \textbf{then return}\ ( u.\mathit{key}
 ///     , u.v ) \cr
 /// 8 & \qquad a \gets b;\ u \gets u.\mathit{right} \cr
 /// 9 & \quad \textbf{return}\ \textbf{nil}
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$ is monotone on the prefixes: if it holds on the prefix
 ///      up to a key, it holds on the prefix up to any larger key
 template< class P >
 [[nodiscard]] std::optional< std::pair< K , V > > augSelect( P && p ) const;

 /// `f( k , v )` on the pairs \f$ ( k , v ) \f$ with
 /// \f$ q( \mathrm{from}( k , v ) ) \f$, by increasing keys, entering only
 /// the subtrees whose augmented value satisfies \f$ q \f$: for \f$ m \f$
 /// such pairs on a tree of height \f$ h \f$, at most
 /// \f$ 3 m ( h + 1 ) + 1 \f$ calls of \f$ q \f$, i.e.,
 /// \f$ O( 1 + m \log n ) \f$
 ///
 /// In pseudocode (Sun, Ferizovic and Blelloch, beyond the book), from
 /// \f$ u = T \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{augFilter}( u , q , f ) \cr
 /// 2 & \quad \textbf{if}\ u = \textbf{nil}\ \textbf{or not}\ q(
 ///     \mathit{aug}( u ) )\ \textbf{then return} \cr
 /// 3 & \quad \mathrm{augFilter}( u.\mathit{left} , q , f ) \cr
 /// 4 & \quad \textbf{if}\ q( \mathrm{from}( u ) )\ \textbf{then}\ f(
 ///     u.\mathit{key} , u.v ) \cr
 /// 5 & \quad \mathrm{augFilter}( u.\mathit{right} , q , f )
 /// \end{array}}
 /// \f]
 /// \pre \f$ q \f$ is false on the identity and
 ///      \f$ q( a \otimes b ) = q( a ) \lor q( b ) \f$ (e.g.,
 ///      \f$ q( a ) = a \ge \theta \f$ on the maxima)
 template< class Q , class F >
 void augFilter( Q && q , F && f ) const;

/*--------------------------------------------------------------------------*/
/*---------------------------- NAVIGATION ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the root, `nullptr` if empty; the nodes must not be changed
 [[nodiscard]] const Node * root( void ) const { return( f_root ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the shape of the tree, for `PathIterator`
 struct Shape
 {
  /// the root of the tree `o`
  static auto root( const auto & o ) { return( o.f_root ); }
  /// the left child of the node `n`
  static auto left( const auto * n ) { return( n->left ); }
  /// the right child of the node `n`
  static auto right( const auto * n ) { return( n->right ); }
  /// the pair of the node `n`
  static auto read( const auto * n ) {
   return( PairRef< const K & , const V & >( n->key , n->value ) );
   }
  };

 /// the pairs ( key , value ) by value, which own their copies
 using value_type = std::pair< K , V >;
 /// the type of the keys
 using key_type = K;
 /// the type of the values
 using mapped_type = V;
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the type of the distances
 using reference = PairRef< const K & , const V & >;  ///< references, by value
 using const_reference = reference;                  ///< the same
 /// the constant iterator by increasing key, along the path from the root
 using const_iterator = PathIterator< JoinTree , Shape >;
 using iterator = const_iterator;  ///< the constant iterator

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

/*--------------------------------------------------------------------------*/
 private:
 /// the order, the sizes, the augmented values and the balance of the
 /// scheme, \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;


 /// the node of \f$ k \f$, nil if absent
 [[nodiscard]] Node * findNode( const K & k ) const;

 /// whether `G` updates the path of an assignment by a difference
 static constexpr bool kDelta = AbelianGroup< G > && exact_v< G >;

/*------------------------ THE CHANGES, UNOBSERVED -------------------------*/

 /// insert() with no event
 void insertPlain( const K & k , const V & v );
 /// remove() with no event
 void removePlain( const K & k );
 /// assign() with no event
 bool assignPlain( const K & k , const V & v );

/*------------------------- THE CHANGES, OBSERVED --------------------------*/

 /// insert() when someone listens
 void insertTold( const K & k , const V & v );
 /// insert() when someone listens, out of the hot code
 ADS_COLD_PATH void insertCold( const K & k , const V & v ) {
  insertTold( k , v );
  }
 /// remove() when someone listens
 void removeTold( const K & k );
 /// remove() when someone listens, out of the hot code
 ADS_COLD_PATH void removeCold( const K & k ) { removeTold( k ); }
 /// assign() when someone listens
 bool assignTold( const K & k , const V & v );
 /// assign() when someone listens, out of the hot code
 ADS_COLD_PATH bool assignCold( const K & k , const V & v ) {
  return( assignTold( k , v ) );
  }
 /// clear() when someone listens
 ADS_COLD_PATH void clearTold( void );
 /// `Cleared`, if someone listens: the nodes were taken out of the tree
 void tellCleared( void ) noexcept;
 /// `Rebuilt`, if someone listens: the nodes were put in the tree
 void tellRebuilt( void ) noexcept;

 Node * f_root = nullptr;              ///< the root
 SplitMix64 f_g;                       ///< the priorities of a treap
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class JoinTree )

/*--------------------------------------------------------------------------*/

/// the AVL tree whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation ,
	  class Policy = NoObservers ,
	  class A = std::allocator< std::pair< const K , V > > >
using JoinAVLTree = JoinTree< K , V , AVLBalance , G , Policy , A >;

/// the Red-Black tree whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation ,
	  class Policy = NoObservers ,
	  class A = std::allocator< std::pair< const K , V > > >
using JoinRedBlackTree = JoinTree< K , V , RedBlackBalance , G , Policy , A >;

/// the tree balanced by weight, in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation ,
	  class Policy = NoObservers ,
	  class A = std::allocator< std::pair< const K , V > > >
using WeightBalancedTree = JoinTree< K , V , WeightBalance , G , Policy , A >;

/// the treap whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation ,
	  class Policy = NoObservers ,
	  class A = std::allocator< std::pair< const K , V > > >
using JoinTreap = JoinTree< K , V , TreapBalance , G , Policy , A >;

/// the map augmented by G, balanced by weight as in PAM
template< std::totally_ordered K , class V , class G ,
	  class Policy = NoObservers ,
	  class A = std::allocator< std::pair< const K , V > > >
using AugmentedMap = JoinTree< K , V , WeightBalance , G , Policy , A >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/JoinTree.tpp"

/*--------------------------------------------------------------------------*/

#endif /* JoinTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File JoinTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
