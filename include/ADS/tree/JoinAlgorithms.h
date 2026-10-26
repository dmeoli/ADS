/*--------------------------------------------------------------------------*/
/*------------------------- File JoinAlgorithms.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The balanced binary search trees whose only balancing operation is join,
 * beyond the book, and the operations on ordered sets and maps built on
 * it, sequential and parallel. join( L , k , R ), for two trees L and R
 * whose keys are smaller and larger than k, returns a balanced tree of the
 * keys of L, k and those of R; it is the one operation that knows the
 * balancing scheme, and split, the removals, the insertions, the union, the
 * intersection, the difference, the batches and the filters are written
 * once, on join alone, for every scheme. Adams wrote the union of the trees
 * balanced by weight in this way; Blelloch, Ferizovic and Sun showed that
 * the joins of four schemes (the AVL trees, the Red-Black trees, the trees
 * balanced by weight and the treaps) are enough for the set operations to
 * take the optimal work, and that the two recursive calls of each of them
 * can run in parallel.
 *
 * \b Ranks. The cost of join is bounded through a rank \f$ r( T ) \f$ of
 * each tree, which is \f$ \Theta( \log n ) \f$ for a tree of n keys: the
 * height h for the AVL trees; the black height \f$ \hat{h} \f$ (the black
 * nodes on a path from the root to a leaf, the root excluded if it is
 * black) for the Red-Black trees; \f$ \log_2 w( T ) \f$, w the weight (the
 * keys plus 1), for the trees balanced by weight and, in expectation, for
 * the treaps. Each join below goes down the right spine of the higher
 * tree (the left one of the lower, symmetrically) until it meets a subtree
 * c of the rank of the other tree, hangs k there with c and the other
 * tree as its children, and fixes the balance on the way back by at most
 * a double rotation per level; hence it costs
 * \f$ O( | r( L ) - r( R ) | + 1 ) \f$, and the rank of its result is
 * between \f$ \max( r( L ) , r( R ) ) \f$ and that plus 1. For the AVL
 * trees the subtree met has height \f$ h( R ) \f$ or \f$ h( R ) + 1 \f$,
 * the new node has height at most \f$ h( R ) + 2 \f$, and a rotation at
 * the parent brings the difference back to 1 (Lemma 1 of the paper). For
 * the Red-Black trees the subtree met is black, of black height
 * \f$ \hat{h}( R ) \f$, the new node is red, and the only fault, a red
 * node with a red child, is pushed up by one rotation per level and, at
 * the root, solved by painting it black. For the trees balanced by weight
 * of parameter \f$ \alpha \le 1 - 1 / \sqrt{2} \f$ (here 0.29), each
 * level needs a single or a double rotation, as for the insertions of
 * Nievergelt and Reingold, and the rank is that of the weight. For the
 * treaps, join is the merge of the two spines by the priorities, and k
 * goes down until its priority beats those below: its expected cost is
 * that of an insertion, \f$ O( \log n ) \f$.
 *
 * \b Split. split( T , k ) exposes the root ( L , m , R ) of T, recurs on
 * the side of k and joins the other side with m and the part of the
 * recursion on its side. The joins at the levels of the path have ranks
 * that differ by the ranks of the subtrees left behind, so that their
 * costs telescope to \f$ O( r( T ) ) = O( \log n ) \f$; join2( L , R ),
 * the join with no key in between, takes the last key of L off it by
 * splitLast() and joins it with R, again in \f$ O( \log n ) \f$.
 *
 * \b Work. union( A , B ) exposes the root ( L , k , R ) of the smaller
 * tree, splits the larger one at k, takes the unions of the two left and
 * of the two right sides, and joins them with k; the intersection and
 * the difference differ only in whether k survives, and use join2 when it
 * does not. For sizes \f$ m \le n \f$, the splits at the level i of the
 * recursion are \f$ 2^i \f$ splits of trees whose sizes sum to n, which
 * cost \f$ O( 2^i \log( n / 2^i + 1 ) ) \f$ by the concavity of the
 * logarithm; summed over the \f$ \log m \f$ levels, and with the joins
 * costing no more than the splits (their rank differences telescope), the
 * work is
 * \f[ W( m , n ) = \Theta\Bigl( m \log\Bigl( \frac{n}{m} + 1 \Bigr) \Bigr)
 * \f]
 * the lower bound of the comparisons for merging two sorted sequences
 * (Hwang and Lin; Brown and Tarjan), which is \f$ \Theta( m + n ) \f$ for
 * \f$ m \approx n \f$ and \f$ \Theta( \log n ) \f$ for \f$ m = 1 \f$. The
 * two recursive calls touch disjoint trees, so that they run in parallel:
 * the span is \f$ O( \log m \log n ) \f$, as each level of the recursion
 * waits for a split and a join. A batch of m sorted pairs, multiInsert(),
 * is the union with the tree built from the batch, and multiRemove() the
 * difference, both done without building it: the batch is split by a
 * binary search on the key of the root, in the same work; filter() and
 * mapReduce() visit every node, \f$ O( n ) \f$ work and
 * \f$ O( \log^2 n ) \f$ span.
 *
 * \b Augmentation. A node may keep a value of its subtree, computed from
 * those of its two children and of its own pair (its size, its height,
 * the sum of the values of an augmented map [see JoinTree.h]); since every
 * node that changes is passed to connect(), which recomputes that value,
 * and the rotations are two exposes and two connects, join keeps it, and
 * so does every algorithm above, at the cost of a combination per node
 * that join touches.
 *
 * \b Layout. A tree gives its nodes to the algorithms through an adaptor
 * [see JoinAdaptor]: the joins of the four schemes (AVLJoin, RedBlackJoin,
 * WeightBalancedJoin and TreapJoin) are written on it, JoinAlgorithms
 * writes on it split(), join2() and the bulk operations, and Joinable
 * gives them to the class of the tree as methods: AVLTree, Treap and the
 * Red-Black tree of SearchTree, whose nodes keep the sizes of their
 * subtrees for it, and JoinTree [see JoinTree.h], whose four schemes are
 * augmented by a template parameter. All the operations are destructive:
 * they consume their arguments and reuse their nodes, and a tree is not
 * shared between versions (the persistent trees of PAM copy the path that
 * changes, and count the references to the nodes).
 *
 * \b Parallelism. An operation given an executor [see Executor.h] runs
 * its two recursive calls by parallelInvoke(), until the two trees have
 * fewer than kJoinGrain keys in all, or the executor one thread, where it
 * goes on sequentially; the shape of the result does not depend on the
 * executor, nor on the number of its threads, since join is
 * deterministic and the priorities of a batch of a treap are drawn by the
 * place of each pair in the batch. A key is compared, and a node read or
 * written, by one thread at a time.
 *
 * \b Pseudocode. The convention: a tree is nil or a node ( L , k , R ) of
 * children L and R and key k; expose( T ) gives the three, node( L , k ,
 * R ) makes a node and recomputes what it keeps (connect()), r( T ) is the
 * rank, ← the assignment, and the lines are numbered as in the paper.
 * Each join is given for the case of the left tree higher, the other
 * being its mirror; the parallel calls are marked "in parallel".
 *
 * \b State of the art. PAM is the library of the paper on augmented maps,
 * persistent and on ParlayLib; PaC-trees (the library CPAM) keep blocks of
 * up to B keys, possibly compressed, in the leaves of the same trees, and
 * reach the same bounds with less space and fewer cache misses. De Man and
 * Dhulipala give batch-parallel treaps (and link-cut and Euler tour trees)
 * on a join of the same kind. oneTBB (concurrent_map) and folly
 * (ConcurrentSkipList) keep concurrent skip lists, without bulk set
 * operations or split; Boost.Intrusive has AVL, Red-Black, scapegoat,
 * splay and treap trees with no split nor join of trees.
 *
 * The contracts [see Contract.h]: the preconditions of join() and join2()
 * on the order of the keys, the batches sorted with no repeated key (an
 * audit), and after every operation the invariant of the tree, which the
 * class checks at the level 3: the order of the keys, the sizes, the
 * augmented values and the balance of its scheme (the heights of AVL, the
 * colors and the black heights of Red-Black, the weights, the heap order
 * of the priorities). The operations are defined in JoinAlgorithms.tpp.
 *
 * \see S. Adams, Efficient sets: a balancing act, Journal of Functional
 *      Programming 3(4), 553-561, 1993, doi:10.1017/S0956796800000885
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Just join for parallel
 *      ordered sets, Proceedings of the 28th ACM Symposium on Parallelism
 *      in Algorithms and Architectures (SPAA), 253-264, 2016,
 *      doi:10.1145/2935764.2935768
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Joinable parallel balanced
 *      binary trees, ACM Transactions on Parallel Computing 9(2), 1-41,
 *      2022, doi:10.1145/3512769
 * \see G. E. Blelloch, M. Reid-Miller, Fast set operations using treaps,
 *      Proceedings of the 10th ACM Symposium on Parallel Algorithms and
 *      Architectures (SPAA), 16-26, 1998, doi:10.1145/277651.277660
 * \see M. R. Brown, R. E. Tarjan, A fast merging algorithm, Journal of the
 *      ACM 26(2), 211-226, 1979, doi:10.1145/322123.322127
 * \see J. Nievergelt, E. M. Reingold, Binary search trees of bounded
 *      balance, SIAM Journal on Computing 2(1), 33-43, 1973,
 *      doi:10.1137/0202005
 * \see F. K. Hwang, S. Lin, A simple algorithm for merging two disjoint
 *      linearly ordered sets, SIAM Journal on Computing 1(1), 31-39,
 *      1972, doi:10.1137/0201004
 * \see Y. Hirai, K. Yamamoto, Balancing weight-balanced trees, Journal of
 *      Functional Programming 21(3), 287-307, 2011,
 *      doi:10.1017/S0956796811000104
 * \see R. Seidel, C. R. Aragon, Randomized search trees, Algorithmica
 *      16(4-5), 464-497, 1996, doi:10.1007/BF01940876
 * \see R. E. Tarjan, Data Structures and Network Algorithms, SIAM, 1983,
 *      chapter 4 (the join of the Red-Black trees),
 *      doi:10.1137/1.9781611970265
 * \see Y. Sun, D. Ferizovic, G. E. Blelloch, PAM: parallel augmented maps,
 *      Proceedings of the 23rd ACM SIGPLAN Symposium on Principles and
 *      Practice of Parallel Programming (PPoPP), 290-304, 2018,
 *      doi:10.1145/3178487.3178509
 * \see L. Dhulipala, G. E. Blelloch, Y. Gu, Y. Sun, PaC-trees: supporting
 *      parallel and compressed purely-functional collections, Proceedings
 *      of the 43rd ACM SIGPLAN Conference on Programming Language Design
 *      and Implementation (PLDI), 108-121, 2022,
 *      doi:10.1145/3519939.3523733
 * \see Q. De Man, L. Dhulipala, Fast and theoretically efficient
 *      batch-parallel link-cut trees, Euler tour trees, and treaps,
 *      Proceedings of the 38th ACM Symposium on Parallelism in Algorithms
 *      and Architectures (SPAA), 234-246, 2026,
 *      doi:10.1145/3816782.3819216
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

#ifndef __ADS_JoinAlgorithms
 #define __ADS_JoinAlgorithms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CONCEPT JoinAdaptor --------------------------*/
/*--------------------------------------------------------------------------*/

/// the sum of the sizes of two trees below which the algorithms run on the
/// calling thread alone
inline constexpr std::size_t kJoinGrain = 2048;

/// the nodes of a binary search tree as the algorithms based on join see
/// them: nil(), the empty tree; left(), right(), key(), value() and
/// size(), the reading of a node (size() of nil is 0); expose( t , l , r ),
/// which takes off the children of t into l and r, so that t is a node
/// alone; connect( m , l , r ), which hangs l and r under the node alone m
/// and updates what m keeps of its subtree (its size, its height, its
/// augmented value, the parents of l and r); dispose( t ), which frees
/// the subtree t (nothing for nil); rank( t ), the rank of a tree, and
/// childRank( t , r , c ), that of the child c of t of rank r, in
/// \f$ O( 1 ) \f$; join( l , rl , m , r , rr ), the join of the balancing
/// scheme, which returns the root and its rank
template< class A >
concept JoinAdaptor = requires( typename A::Node * t ,
                                typename A::Node * & l ,
                                const typename A::Key & k ,
                                typename A::Rank r ) {
 { A::nil() } -> std::same_as< typename A::Node * >;
 { A::left( t ) } -> std::same_as< typename A::Node * >;
 { A::right( t ) } -> std::same_as< typename A::Node * >;
 { A::key( t ) } -> std::same_as< const typename A::Key & >;
 { A::value( t ) } -> std::same_as< typename A::Value & >;
 { A::size( t ) } -> std::convertible_to< std::size_t >;
 A::expose( t , l , l );
 { A::connect( t , t , t ) } -> std::same_as< typename A::Node * >;
 A::dispose( t );
 { A::rank( t ) } -> std::same_as< typename A::Rank >;
 { A::childRank( t , r , t ) } -> std::same_as< typename A::Rank >;
 { A::join( t , r , t , t , r ) }
  -> std::same_as< std::pair< typename A::Node * , typename A::Rank > >;
 { k < k } -> std::convertible_to< bool >;
 };

/*--------------------------------------------------------------------------*/
/*----------------------- THE JOINS OF THE SCHEMES -------------------------*/
/*--------------------------------------------------------------------------*/

/// the join of the AVL trees; A gives also height( t ), -1 for nil, which
/// connect() keeps
template< class A >
struct AVLJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// the join of the AVL trees l and r with m between them, in
 /// \f$ O( | h( l ) - h( r ) | + 1 ) \f$ time; joinLeft() is the mirror
 /// of joinRight():
 ///
 ///     JOIN-AVL( L , k , R )
 ///      1  if h( L ) > h( R ) + 1 then return JOIN-RIGHT-AVL( L , k , R )
 ///      2  if h( R ) > h( L ) + 1 then return JOIN-LEFT-AVL( L , k , R )
 ///      3  return node( L , k , R )
 ///
 ///     JOIN-RIGHT-AVL( L , k , R )
 ///      1  ( l , k' , c ) ← expose( L )
 ///      2  if h( c ) ≤ h( R ) + 1 then
 ///      3     T' ← node( c , k , R )
 ///      4     if h( T' ) ≤ h( l ) + 1 then return node( l , k' , T' )
 ///      5     return rotateLeft( node( l , k' , rotateRight( T' ) ) )
 ///      6  T' ← JOIN-RIGHT-AVL( c , k , R )
 ///      7  T'' ← node( l , k' , T' )
 ///      8  if h( T' ) ≤ h( l ) + 1 then return T''
 ///      9  return rotateLeft( T'' )
 static Node * join( Node * l , Node * m , Node * r );

 /// the join when l is higher than r by more than 1
 static Node * joinRight( Node * l , Node * m , Node * r );

 /// the join when r is higher than l by more than 1
 static Node * joinLeft( Node * l , Node * m , Node * r );
 };

/// the join of the Red-Black trees, whose ranks are the black heights; A
/// gives also red( t ), false for nil, and paint( t , red )
template< class A >
struct RedBlackJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// the join of the Red-Black trees l and r, of black heights bl and br,
 /// with m between them: the root and its black height, in
 /// \f$ O( | b_l - b_r | + 1 ) \f$ time; joinLeft() is the mirror of
 /// joinRight():
 ///
 ///     JOIN-RB( L , k , R )
 ///      1  if bh( L ) > bh( R ) then
 ///      2     T' ← JOIN-RIGHT-RB( L , k , R )
 ///      3     if T' and its right child are red then paint T' black
 ///      4     return T'
 ///      5  if bh( R ) > bh( L ) then the mirror of the lines 2 to 4
 ///      6  if L and R are black then return node( L , k red , R )
 ///      7  return node( L , k black , R )
 ///
 ///     JOIN-RIGHT-RB( L , k , R )
 ///      1  if L is black and bh( L ) = bh( R ) then
 ///      2     return node( L , k red , R )
 ///      3  ( l , k' , c ) ← expose( L )
 ///      4  T' ← JOIN-RIGHT-RB( c , k , R )
 ///      5  T'' ← node( l , k' , T' )
 ///      6  if k' is black and T' and its right child are red then
 ///      7     paint the right child of T' black
 ///      8     return rotateLeft( T'' )
 ///      9  return T
 static std::pair< Node * , int > join( Node * l , int bl , Node * m ,
                                        Node * r , int br );

 /// the join when the black height bl of l is larger than br
 static Node * joinRight( Node * l , int bl , Node * m , Node * r ,
                          int br );

 /// the join when the black height br of r is larger than bl
 static Node * joinLeft( Node * l , int bl , Node * m , Node * r , int br );
 };

/// the join of the trees balanced by weight, with \f$ \alpha = 0.29 \f$
template< class A >
struct WeightBalancedJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 static constexpr std::size_t kAlphaNum = 29;    ///< \f$ \alpha \f$, up
 static constexpr std::size_t kAlphaDen = 100;   ///< and down

 /// the join of the trees l and r balanced by weight with m between
 /// them, in \f$ O( | \log w( l ) - \log w( r ) | + 1 ) \f$ time, where
 /// like( a , b ) is \f$ \alpha ( a + b ) \le \min( a , b ) \f$ and
 /// joinLeft() is the mirror of joinRight():
 ///
 ///     JOIN-WB( L , k , R )
 ///      1  if like( w( L ) , w( R ) ) then return node( L , k , R )
 ///      2  if w( L ) > w( R ) then return JOIN-RIGHT-WB( L , k , R )
 ///      3  return JOIN-LEFT-WB( L , k , R )
 ///
 ///     JOIN-RIGHT-WB( L , k , R )
 ///      1  if like( w( L ) , w( R ) ) then return node( L , k , R )
 ///      2  ( l , k' , c ) ← expose( L )
 ///      3  T' ← JOIN-RIGHT-WB( c , k , R )
 ///      4  ( l1 , k1 , r1 ) ← expose( T' ), read only
 ///      5  if like( w( l ) , w( T' ) ) then return node( l , k' , T' )
 ///      6  if like( w( l ) , w( l1 ) ) and
 ///            like( w( l ) + w( l1 ) , w( r1 ) ) then
 ///      7     return rotateLeft( node( l , k' , T' ) )
 ///      8  return rotateLeft( node( l , k' , rotateRight( T' ) ) )
 static Node * join( Node * l , Node * m , Node * r );

 /// the join when l is heavier than r beyond the balance
 static Node * joinRight( Node * l , Node * m , Node * r );

 /// the join when r is heavier than l beyond the balance
 static Node * joinLeft( Node * l , Node * m , Node * r );

 /// whether the weights a and b may be the two sides of a node:
 /// \f$ \alpha ( a + b ) \le \min( a , b ) \f$
 static bool like( std::size_t a , std::size_t b );

 /// the weight of t, its size plus 1
 static std::size_t weight( const Node * t );
 };

/// the join of the treaps; A gives also priority( t ), of a node
template< class A >
struct TreapJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// the join of the treaps l and r with m between them, in
 /// \f$ O( \log n ) \f$ expected time, where p( nil ) is \f$ -\infty \f$:
 ///
 ///     JOIN-TREAP( L , k , R )
 ///      1  if p( L ) > p( k ) and p( L ) ≥ p( R ) then
 ///      2     ( l , k' , r ) ← expose( L )
 ///      3     return node( l , k' , JOIN-TREAP( r , k , R ) )
 ///      4  if p( R ) > p( k ) then
 ///      5     ( l , k' , r ) ← expose( R )
 ///      6     return node( JOIN-TREAP( L , k , l ) , k' , r )
 ///      7  return node( L , k , R )
 static Node * join( Node * l , Node * m , Node * r );
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS JoinAlgorithms ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the algorithms on the trees whose only balancing is the join of A; each
/// of them consumes the trees it is given and returns the trees it makes
template< JoinAdaptor A >
struct JoinAlgorithms {

 using Node = typename A::Node;    ///< the type of the nodes
 using Key = typename A::Key;      ///< the type of the keys
 using Value = typename A::Value;  ///< the type of the values
 using Rank = typename A::Rank;    ///< the type of the ranks

 /// a tree: its root and its rank
 struct Tree {
  Node * t = nullptr;   ///< the root, nil if empty
  Rank r{};             ///< the rank
  };

 /// a tree taken apart at its root
 struct Exposed {
  Tree l;               ///< the left subtree
  Node * m = nullptr;   ///< the root, alone
  Tree r;               ///< the right subtree
  };

 /// the empty tree
 [[nodiscard]] static Tree empty( void );

 /// the tree of root t, its rank computed
 [[nodiscard]] static Tree whole( Node * t );

 /// the subtrees and the root of x
 /// \pre x is not empty
 [[nodiscard]] static Exposed expose( Tree x );

 /// the join of the scheme
 [[nodiscard]] static Tree join( Tree l , Node * m , Tree r );

 /// the keys of x smaller than k in l, those larger in r, and the node of
 /// k returned, alone (nil if k is not in x), in \f$ O( \log n ) \f$ time:
 ///
 ///     SPLIT( T , k )
 ///      1  if T = nil then return ( nil , nil , nil )
 ///      2  ( L , m , R ) ← expose( T )
 ///      3  if k < m then
 ///      4     ( L' , b , R' ) ← SPLIT( L , k )
 ///      5     return ( L' , b , JOIN( R' , m , R ) )
 ///      6  if m < k then
 ///      7     ( L' , b , R' ) ← SPLIT( R , k )
 ///      8     return ( JOIN( L , m , L' ) , b , R' )
 ///      9  return ( L , m , R )
 static Node * split( Tree x , const Key & k , Tree & l , Tree & r );

 /// the node of the largest key of x returned, alone, and the others in
 /// rest, in \f$ O( \log n ) \f$ time
 /// \pre x is not empty
 ///
 ///     SPLIT-LAST( T )
 ///      1  ( L , k , R ) ← expose( T )
 ///      2  if R = nil then return ( L , k )
 ///      3  ( T' , k' ) ← SPLIT-LAST( R )
 ///      4  return ( JOIN( L , k , T' ) , k' )
 static Node * splitLast( Tree x , Tree & rest );

 /// the tree of the keys of l and of r, with no key in between, in
 /// \f$ O( \log n ) \f$ time
 /// \pre the keys of l are smaller than those of r
 ///
 ///     JOIN2( L , R )
 ///      1  if L = nil then return R
 ///      2  ( L' , k ) ← SPLIT-LAST( L )
 ///      3  return JOIN( L' , k , R )
 [[nodiscard]] static Tree join2( Tree l , Tree r );

 /// the node alone n inserted in x, fresh whether its key was not there
 /// (if it was, its value replaces the old one and n is freed), by joins
 /// on the way back, in \f$ O( \log n ) \f$ time
 [[nodiscard]] static Tree insert( Tree x , Node * n , bool & fresh );

 /// the key k taken out of x and freed, gone whether it was there, by
 /// joins on the way back and join2 where it was, in \f$ O( \log n ) \f$
 /// time
 [[nodiscard]] static Tree remove( Tree x , const Key & k , bool & gone );

 /// the union of a and b, on a key of both the value f( of a , of b ),
 /// with \f$ O( m \log( n / m + 1 ) ) \f$ work and
 /// \f$ O( \log m \log n ) \f$ span for sizes \f$ m \le n \f$:
 ///
 ///     UNION( A , B )
 ///      1  if A = nil then return B
 ///      2  if B = nil then return A
 ///      3  ( L , k , R ) ← expose( the smaller of A and B )
 ///      4  ( L' , b , R' ) ← SPLIT( the larger of A and B , k )
 ///      5  in parallel: T1 ← UNION( L , L' ) and T2 ← UNION( R , R' )
 ///      6  if b ≠ nil then the value of k ← f( of A , of B )
 ///      7  return JOIN( T1 , k , T2 )
 template< Executor E , class F >
 [[nodiscard]] static Tree unite( E & ex , Tree a , Tree b , const F & f );

 /// the intersection of a and b, the value f( of a , of b ), with the
 /// work and the span of unite(); the nodes that leave are freed:
 ///
 ///     INTERSECT( A , B )
 ///      1  if A = nil or B = nil then return nil
 ///      2  ( L , k , R ) ← expose( the smaller of A and B )
 ///      3  ( L' , b , R' ) ← SPLIT( the larger of A and B , k )
 ///      4  in parallel: T1 ← INTERSECT( L , L' ) and
 ///                      T2 ← INTERSECT( R , R' )
 ///      5  if b = nil then return JOIN2( T1 , T2 )
 ///      6  the value of k ← f( of A , of B )
 ///      7  return JOIN( T1 , k , T2 )
 template< Executor E , class F >
 [[nodiscard]] static Tree intersect( E & ex , Tree a , Tree b ,
                                      const F & f );

 /// the keys of a not in b, with the work and the span of unite(); the
 /// nodes of b and those of a that leave are freed:
 ///
 ///     DIFFERENCE( A , B )
 ///      1  if A = nil or B = nil then return A
 ///      2  if | A | < | B | then
 ///      3     ( L , k , R ) ← expose( A ) ; ( L' , b , R' ) ← SPLIT( B , k )
 ///      4     in parallel: T1 ← DIFFERENCE( L , L' ) and
 ///                         T2 ← DIFFERENCE( R , R' )
 ///      5     if b = nil then return JOIN( T1 , k , T2 )
 ///      6     return JOIN2( T1 , T2 )
 ///      7  ( L , k , R ) ← expose( B ) ; ( L' , b , R' ) ← SPLIT( A , k )
 ///      8  in parallel: T1 ← DIFFERENCE( L' , L ) and
 ///                      T2 ← DIFFERENCE( R' , R )
 ///      9  return JOIN2( T1 , T2 )
 template< Executor E >
 [[nodiscard]] static Tree subtract( E & ex , Tree a , Tree b );

 /// the tree of the n nodes alone of N, whose keys increase: the middle
 /// one joined with the trees of the two halves, built in parallel, with
 /// \f$ O( n ) \f$ work and \f$ O( \log^2 n ) \f$ span
 template< Executor E >
 [[nodiscard]] static Tree build( E & ex , Node * const * N ,
                                  std::size_t n );

 /// the n nodes alone of N, whose keys increase, inserted in x; on a key
 /// already there the value f( old , new ), and the node of N freed. It is
 /// UNION( x , the tree of N ) with the split of the batch by a binary
 /// search in place of SPLIT, so that the tree of N is never built:
 /// \f$ O( m \log( n / m + 1 ) ) \f$ work for \f$ m = \min( n , | x | ) \f$
 ///
 ///     MULTI-INSERT( T , N[ 1 .. n ] )
 ///      1  if n = 0 then return T
 ///      2  if T = nil then return BUILD( N )
 ///      3  ( L , k , R ) ← expose( T )
 ///      4  j ← the first index with N[ j ] ≥ k
 ///      5  in parallel: T1 ← MULTI-INSERT( L , N[ 1 .. j - 1 ] ) and
 ///                      T2 ← MULTI-INSERT( R , N[ j' .. n ] ), where
 ///                      j' is j + 1 if N[ j ] = k, j otherwise
 ///      6  return JOIN( T1 , k , T2 )
 template< Executor E , class F >
 [[nodiscard]] static Tree multiInsert( E & ex , Tree x , Node * const * N ,
                                        std::size_t n , const F & f );

 /// the n keys of K, increasing, taken out of x and freed, gone how
 /// many were there: DIFFERENCE( x , K ) with the split of K by a binary
 /// search, \f$ O( m \log( n / m + 1 ) ) \f$ work
 template< Executor E >
 [[nodiscard]] static Tree multiRemove( E & ex , Tree x , const Key * K ,
                                        std::size_t n , std::size_t & gone );

 /// the nodes of x with p( k , v ) kept, the others freed: the two
 /// subtrees filtered in parallel and joined by the root if it stays, by
 /// join2 if it does not; \f$ O( n ) \f$ work and
 /// \f$ O( \log^2 n ) \f$ span
 template< Executor E , class P >
 [[nodiscard]] static Tree filter( E & ex , Tree x , const P & p );

 /// reduce( reduce( of the left subtree , map( k , v ) ) , of the right
 /// one ) on the tree of root t, the two subtrees in parallel: the keys
 /// in their order, so that reduce needs to be associative only;
 /// \f$ O( n ) \f$ work and \f$ O( \log n ) \f$ span
 template< Executor E , class M , class R , class U >
 [[nodiscard]] static U mapReduce( E & ex , Node * t , const M & map ,
                                   const R & reduce , const U & id );

 /// the keys of x in \f$ [ lo , hi ] \f$ taken out into the tree
 /// returned, by two splits, two joins and a join2, in
 /// \f$ O( \log n ) \f$ time
 [[nodiscard]] static Tree extract( Tree & x , const Key & lo ,
                                    const Key & hi );

 /// the smallest key of t
 /// \pre t is not nil
 [[nodiscard]] static const Key & firstKey( Node * t );

 /// the largest key of t
 /// \pre t is not nil
 [[nodiscard]] static const Key & lastKey( Node * t );

 private:

 /// g() and h(), in parallel on ex
 template< Executor E , class G , class H >
 static void fork( E & ex , G && g , H && h );

 /// g() and h(), one after the other
 template< class G , class H >
 static void fork( Sequential & ex , G && g , H && h );

 /// whether n items are too few for more than the calling thread
 template< Executor E >
 [[nodiscard]] static bool small( E & ex , std::size_t n );

 };  // end( struct JoinAlgorithms )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Joinable ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the value of the second tree, or of the batch, on a key in both
struct TakeSecond {
 template< class V >
 V operator()( const V & , const V & b ) const { return( b ); }
 };

/// the value of the first tree on a key in both
struct TakeFirst {
 template< class V >
 V operator()( const V & a , const V & ) const { return( a ); }
 };

/// the operations based on join of the tree T of keys Key and values
/// Value, whose nodes A adapts [see JoinAdaptor]: a base of T (which makes
/// A its friend); A is read only in the bodies, as T is incomplete where
/// its bases are named, and gives take( t ), the root of t
/// taken out of it, which is left empty; put( t , r ), r the new root of
/// the empty t, which updates its size; peek( t ), its root; fresh( t ),
/// an empty tree like t; valid( t ), the invariant of t; salt( t ) and
/// make( k , v , s , i ), a node alone of the pair ( k , v ), the i-th of
/// a batch of salt s, which may be called in parallel
template< class T , class A , class Key , class Value >
class Joinable {

/*--------------------------------------------------------------------------*/
 public:

/*--------------------------------------------------------------------------*/
/*------------------------- SPLITTING AND JOINING --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the keys before k, the value of k (if there), the keys after k; the
 /// tree is consumed, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::tuple< T , std::optional< Value > , T > split(
                                                        const Key & k ) &&;

 /// the tree of the keys of l, the pair ( k , v ) and the keys of r; l and
 /// r are consumed, in \f$ O( | \log n_l - \log n_r | + 1 ) \f$ time
 /// \pre the keys of l are smaller than k, those of r larger
 [[nodiscard]] static T join( T && l , const Key & k , const Value & v ,
                              T && r );

 /// the tree of the keys of l and of r, which are consumed, in
 /// \f$ O( \log n ) \f$ time
 /// \pre the keys of l are smaller than those of r
 [[nodiscard]] static T join2( T && l , T && r );

 /// the pairs of the keys in \f$ [ lo , hi ] \f$, taken out of the tree,
 /// in \f$ O( \log n ) \f$ time
 [[nodiscard]] T extractRange( const Key & lo , const Key & hi );

/*--------------------------------------------------------------------------*/
/*--------------------------- BULK OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the union with o, which is consumed; on a key of both, the value
 /// f( mine , its ), by default its; \f$ O( m \log( n / m + 1 ) ) \f$
 /// work for sizes \f$ m \le n \f$
 template< class F = TakeSecond >
 void unionWith( T && o , F f = F() ) {
  Sequential seq;
  unionWith( seq , std::move( o ) , f );
  }

 /// unionWith( o , f ) on the threads of ex
 template< Executor E , class F = TakeSecond >
 void unionWith( E & ex , T && o , F f = F() );

 /// the intersection with o, which is consumed; the value f( mine , its ),
 /// by default mine; the work of unionWith(), plus the freeing of the
 /// nodes that leave
 template< class F = TakeFirst >
 void intersectWith( T && o , F f = F() ) {
  Sequential seq;
  intersectWith( seq , std::move( o ) , f );
  }

 /// intersectWith( o , f ) on the threads of ex
 template< Executor E , class F = TakeFirst >
 void intersectWith( E & ex , T && o , F f = F() );

 /// the keys not in o, which is consumed; the work of unionWith(), plus
 /// the freeing of the nodes of o
 void differenceWith( T && o ) {
  Sequential seq;
  differenceWith( seq , std::move( o ) );
  }

 /// differenceWith( o ) on the threads of ex
 template< Executor E >
 void differenceWith( E & ex , T && o );

 /// the pairs of B inserted; on a key already there, the value f( mine ,
 /// new ), by default new; \f$ O( m \log( n / m + 1 ) ) \f$ work for m
 /// pairs
 /// \pre the keys of B increasing (an audit)
 template< class F = TakeSecond >
 void multiInsert( std::span< const std::pair< Key , Value > > B ,
                   F f = F() ) {
  Sequential seq;
  multiInsert( seq , B , f );
  }

 /// multiInsert( B , f ) on the threads of ex
 template< Executor E , class F = TakeSecond >
 void multiInsert( E & ex , std::span< const std::pair< Key , Value > > B ,
                   F f = F() );

 /// the keys of B removed, \f$ O( m \log( n / m + 1 ) ) \f$ work
 /// \pre the keys of B increasing (an audit)
 void multiRemove( std::span< const Key > B ) {
  Sequential seq;
  multiRemove( seq , B );
  }

 /// multiRemove( B ) on the threads of ex
 template< Executor E >
 void multiRemove( E & ex , std::span< const Key > B );

 /// only the pairs with p( k , v ) kept, \f$ O( n ) \f$ work
 template< class P >
 void filter( P p ) {
  Sequential seq;
  filter( seq , p );
  }

 /// filter( p ) on the threads of ex
 template< Executor E , class P >
 void filter( E & ex , P p );

 /// reduce( ... reduce( id , map( k_1 , v_1 ) ) ... , map( k_n , v_n ) )
 /// by an associative reduce of identity id, \f$ O( n ) \f$ work
 template< class M , class R , class U >
 [[nodiscard]] U mapReduce( M map , R reduce , U id ) const {
  Sequential seq;
  return( mapReduce( seq , map , reduce , std::move( id ) ) );
  }

 /// mapReduce( map , reduce , id ) on the threads of ex
 template< Executor E , class M , class R , class U >
 [[nodiscard]] U mapReduce( E & ex , M map , R reduce , U id ) const;

 };  // end( class Joinable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/JoinAlgorithms.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* JoinAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File JoinAlgorithms.h ------------------------*/
/*--------------------------------------------------------------------------*/
