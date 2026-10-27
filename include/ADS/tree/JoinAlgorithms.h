/*--------------------------------------------------------------------------*/
/*------------------------- File JoinAlgorithms.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The balanced binary search trees whose only balancing operation is join,
 * beyond the book, and the operations on ordered sets and maps built on it,
 * sequential and parallel. `join( L , k , R )`, for two trees \f$ L \f$ and
 * \f$ R \f$ whose keys are smaller and larger than \f$ k \f$, returns a
 * balanced tree of the keys of \f$ L \f$, \f$ k \f$ and those of \f$ R \f$;
 * it is the one operation that knows the balancing scheme, and split, the
 * removals, the insertions, the union, the intersection, the difference,
 * the batches and the filters are written once, on join alone, for every
 * scheme. Adams wrote the union of the trees balanced by weight in this
 * way; Blelloch, Ferizovic and Sun showed that the joins of four schemes
 * (the AVL trees, the Red-Black trees, the trees balanced by weight and the
 * treaps) are enough for the set operations to take the optimal work, and
 * that the two recursive calls of each of them can run in parallel.
 *
 * The cost of join is bounded through a rank \f$ r( T ) \f$ of each tree,
 * which is \f$ \Theta( \log n ) \f$ for a tree of \f$ n \f$ keys: the height
 * \f$ h \f$ for the AVL trees; the black height \f$ \hat{h} \f$ (the black
 * nodes on a path from the root to a leaf, the root excluded if it is
 * black) for the Red-Black trees; \f$ \log_2 w( T ) \f$, \f$ w \f$ the
 * weight (the keys plus 1), for the trees balanced by weight and, in
 * expectation, for the treaps. Each join below goes down the right spine
 * of the higher tree (the left one of the lower, symmetrically) until it
 * meets a subtree \f$ c \f$ of the rank of the other tree, hangs \f$ k \f$
 * there with \f$ c \f$ and the other tree as its children, and fixes the
 * balance on the way back by at most a double rotation per level; hence it
 * costs \f$ O( | r( L ) - r( R ) | + 1 ) \f$, and the rank of its result is
 * between \f$ \max( r( L ) , r( R ) ) \f$ and that plus 1. The trees
 * balanced by weight take the parameter
 * \f$ \alpha \le 1 - 1 / \sqrt{2} \f$, here 0.29; for the treaps, join is
 * the merge of the two spines by the priorities, and its expected cost is
 * that of an insertion, \f$ O( \log n ) \f$.
 *
 * `split( T , k )` exposes the root \f$ ( L , m , R ) \f$ of \f$ T \f$,
 * recurs on the side of \f$ k \f$ and joins the other side with \f$ m \f$
 * and the part of the recursion on its side, in \f$ O( \log n ) \f$, since
 * the costs of its joins telescope; `join2( L , R )`, the join with no key
 * in between, takes the last key of \f$ L \f$ off it by `splitLast()` and
 * joins it with \f$ R \f$, again in \f$ O( \log n ) \f$. The union of
 * \f$ A \f$ and \f$ B \f$ exposes the root of the smaller tree, splits the
 * larger one at its key, takes the unions of the two left and of the two
 * right sides, in parallel, and joins them with the key; the intersection
 * and the difference differ only in whether the key survives, and use
 * `join2` when it does not. For sizes \f$ m \le n \f$ the work is
 * \f$ \Theta( m \log( n / m + 1 ) ) \f$, the lower bound of the comparisons
 * for merging two sorted sequences, and the span \f$ O( \log m \log n ) \f$.
 * A batch of \f$ m \f$ sorted pairs, `multiInsert()`, is the union with the
 * tree built from the batch, and `multiRemove()` the difference, both done
 * without building it: the batch is split by a binary search on the key of
 * the root, in the same work; `filter()` and `mapReduce()` visit every
 * node, \f$ O( n ) \f$ work and \f$ O( \log^2 n ) \f$ span. The arguments
 * are on the theory page of the area.
 *
 * A node may keep a value of its subtree, computed from those of its two
 * children and of its own pair (its size, its height, the sum of the values
 * of an augmented map [see `JoinTree.h`]); since every node that changes
 * is passed to `connect()`, which recomputes that value, and the rotations
 * are two exposes and two connects, join keeps it, and so does every
 * algorithm above, at the cost of a combination per node that join
 * touches.
 *
 * A tree gives its nodes to the algorithms through an adaptor [see
 * `JoinAdaptor`]: the joins of the four schemes (`AVLJoin`, `RedBlackJoin`,
 * `WeightBalancedJoin` and `TreapJoin`) are written on it, `JoinAlgorithms`
 * writes on it `split()`, `join2()` and the bulk operations, and `Joinable`
 * gives them to the class of the tree as methods: `AVLTree`, `Treap` and
 * the Red-Black tree of `SearchTree`, whose nodes keep the sizes of their
 * subtrees for it, and `JoinTree` [see `JoinTree.h`], whose four schemes
 * are augmented by a template parameter. All the operations are
 * destructive: they consume their arguments and reuse their nodes, and a
 * tree is not shared between versions (the persistent trees of PAM copy
 * the path that changes, and count the references to the nodes).
 *
 * An operation given an executor [see `Executor.h`] runs its two recursive
 * calls by `parallelInvoke()`, until the two trees have fewer than
 * `kJoinGrain` keys in all, or the executor one thread, where it goes on
 * sequentially; the shape of the result does not depend on the executor,
 * nor on the number of its threads, since join is deterministic and the
 * priorities of a batch of a treap are drawn by the place of each pair in
 * the batch. A key is compared, and a node read or written, by one thread
 * at a time.
 *
 * In the pseudocode of the functions below, a tree is nil or a node
 * \f$ ( L , k , R ) \f$ of children \f$ L \f$ and \f$ R \f$ and key \f$ k \f$;
 * \f$ \mathrm{expose}( T ) \f$ gives the three, \f$ \mathrm{node}( L , k ,
 * R ) \f$ makes a node and recomputes what it keeps (`connect()`), and
 * \f$ r( T ) \f$ is the rank. Each join is given for the case of the left
 * tree higher, the other being its mirror.
 *
 * Among the libraries, PAM is that of the paper on augmented maps,
 * persistent and on ParlayLib; PaC-trees (the library CPAM) keep blocks of
 * up to \f$ B \f$ keys, possibly compressed, in the leaves of the same
 * trees, and reach the same bounds with less space and fewer cache misses.
 * De Man and Dhulipala give batch-parallel treaps (and link-cut and Euler
 * tour trees) on a join of the same kind. oneTBB (`concurrent_map`) and
 * folly (`ConcurrentSkipList`) keep concurrent skip lists, without bulk set
 * operations or split; Boost.Intrusive has AVL, Red-Black, scapegoat, splay
 * and treap trees with no split nor join of trees.
 *
 * The contracts [see `Contract.h`]: the preconditions of `join()` and
 * `join2()` on the order of the keys, the batches sorted with no repeated
 * key (an audit), and after every operation the invariant of the tree,
 * which the class checks at the level 3: the order of the keys, the sizes,
 * the augmented values and the balance of its scheme (the heights of AVL,
 * the colors and the black heights of Red-Black, the weights, the heap
 * order of the priorities). The operations are defined in
 * `JoinAlgorithms.tpp`.
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
/// them: `nil()`, the empty tree; `left()`, `right()`, `key()`, `value()`
/// and `size()`, the reading of a node (`size()` of nil is 0);
/// `expose( t , l , r )`, which takes off the children of \f$ t \f$ into
/// \f$ l \f$ and \f$ r \f$, so that \f$ t \f$ is a node alone;
/// `connect( m , l , r )`, which hangs \f$ l \f$ and \f$ r \f$ under the node
/// alone \f$ m \f$ and updates what \f$ m \f$ keeps of its subtree (its
/// size, its height, its augmented value, the parents of \f$ l \f$ and
/// \f$ r \f$); `dispose( t )`, which frees the subtree \f$ t \f$ (nothing
/// for nil); `rank( t )`, the rank of a tree, and `childRank( t , r , c )`,
/// that of the child \f$ c \f$ of \f$ t \f$ of rank \f$ r \f$, in
/// \f$ O( 1 ) \f$; `join( l , rl , m , r , rr )`, the join of the balancing
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

/// the join of the AVL trees; `A` gives also `height( t )`, -1 for nil,
/// which `connect()` keeps
template< class A >
struct AVLJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// the join of the AVL trees \f$ l \f$ and \f$ r \f$ with \f$ m \f$ between
 /// them, in \f$ O( | h( l ) - h( r ) | + 1 ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), \f$ h \f$
 /// the height, and the mirror of `joinRight()` when \f$ R \f$ is higher:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join}( L , k , R ) \cr
 /// 2 & \quad \textbf{if}\ h( L ) > h( R ) + 1\ \textbf{then return}
 ///     \ \mathrm{joinRight}( L , k , R ) \cr
 /// 3 & \quad \textbf{if}\ h( R ) > h( L ) + 1\ \textbf{then return}
 ///     \ \mathrm{joinLeft}( L , k , R ) \cr
 /// 4 & \quad \textbf{return}\ \mathrm{node}( L , k , R ) \cr
 /// 5 & \textbf{function}\ \mathrm{joinRight}( L , k , R ) \cr
 /// 6 & \quad ( l , k' , c ) \gets \mathrm{expose}( L ) \cr
 /// 7 & \quad \textbf{if}\ h( c ) \le h( R ) + 1\ \textbf{then} \cr
 /// 8 & \qquad T' \gets \mathrm{node}( c , k , R ) \cr
 /// 9 & \qquad \textbf{if}\ h( T' ) \le h( l ) + 1\ \textbf{then return}
 ///     \ \mathrm{node}( l , k' , T' ) \cr
 /// 10 & \qquad \textbf{return}\ \mathrm{rotateLeft}( \mathrm{node}( l , k' ,
 ///     \mathrm{rotateRight}( T' ) ) ) \cr
 /// 11 & \quad T' \gets \mathrm{joinRight}( c , k , R );\ T'' \gets
 ///     \mathrm{node}( l , k' , T' ) \cr
 /// 12 & \quad \textbf{if}\ h( T' ) \le h( l ) + 1\ \textbf{then return}\ T''
 ///     \cr
 /// 13 & \quad \textbf{return}\ \mathrm{rotateLeft}( T'' )
 /// \end{array}}
 /// \f]
 static Node * join( Node * l , Node * m , Node * r );

 /// the join when \f$ l \f$ is higher than \f$ r \f$ by more than 1, by the
 /// right spine of \f$ l \f$
 static Node * joinRight( Node * l , Node * m , Node * r );

 /// the join when \f$ r \f$ is higher than \f$ l \f$ by more than 1, the
 /// mirror of `joinRight()`
 static Node * joinLeft( Node * l , Node * m , Node * r );
 };

/// the join of the Red-Black trees, whose ranks are the black heights; `A`
/// gives also `red( t )`, false for nil, and `paint( t , red )`
template< class A >
struct RedBlackJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// the join of the Red-Black trees \f$ l \f$ and \f$ r \f$, of black heights
 /// \f$ b_l \f$ and \f$ b_r \f$, with \f$ m \f$ between them: the root and its
 /// black height, in \f$ O( | b_l - b_r | + 1 ) \f$ time
 ///
 /// In pseudocode (Tarjan, chapter 4, and Blelloch, Ferizovic and Sun, beyond
 /// the book), \f$ \hat{h} \f$ the black height, and the mirror of
 /// `joinRight()` when \f$ R \f$ is higher:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join}( L , k , R ) \cr
 /// 2 & \quad \textbf{if}\ \hat{h}( L ) > \hat{h}( R )\ \textbf{then} \cr
 /// 3 & \qquad T' \gets \mathrm{joinRight}( L , k , R ) \cr
 /// 4 & \qquad \textbf{if}\ T' \text{ and its right child are red}
 ///     \ \textbf{then}\ \text{paint } T' \text{ black} \cr
 /// 5 & \qquad \textbf{return}\ T' \cr
 /// 6 & \quad \textbf{if}\ \hat{h}( R ) > \hat{h}( L )\ \textbf{then}
 ///     \ \text{the mirror of the lines 3 to 5} \cr
 /// 7 & \quad \textbf{if}\ L \text{ and } R \text{ are black}\ \textbf{then}
 ///     \ \textbf{return}\ \mathrm{node}( L , k , R ) \text{, } k \text{ red}
 ///     \cr
 /// 8 & \quad \textbf{return}\ \mathrm{node}( L , k , R ) \text{, } k
 ///     \text{ black} \cr
 /// 9 & \textbf{function}\ \mathrm{joinRight}( L , k , R ) \cr
 /// 10 & \quad \textbf{if}\ L \text{ is black}\ \textbf{and}\ \hat{h}( L ) =
 ///     \hat{h}( R )\ \textbf{then return}\ \mathrm{node}( L , k , R )
 ///     \text{, }
 ///     k \text{ red} \cr
 /// 11 & \quad ( l , k' , c ) \gets \mathrm{expose}( L ) \cr
 /// 12 & \quad T' \gets \mathrm{joinRight}( c , k , R );\ T'' \gets
 ///     \mathrm{node}( l , k' , T' ) \cr
 /// 13 & \quad \textbf{if}\ k' \text{ is black}\ \textbf{and}\ T' \text{ and }
 ///     \text{its right child are red}\ \textbf{then} \cr
 /// 14 & \qquad \text{paint the right child of } T' \text{ black};
 ///     \ \textbf{return}\ \mathrm{rotateLeft}( T'' ) \cr
 /// 15 & \quad \textbf{return}\ T''
 /// \end{array}}
 /// \f]
 static std::pair< Node * , int > join( Node * l , int bl , Node * m ,
                                        Node * r , int br );

 /// the join when the black height \f$ b_l \f$ of \f$ l \f$ is larger than
 /// \f$ b_r \f$, by the right spine of \f$ l \f$
 static Node * joinRight( Node * l , int bl , Node * m , Node * r ,
                          int br );

 /// the join when the black height \f$ b_r \f$ of \f$ r \f$ is larger than
 /// \f$ b_l \f$, the mirror of `joinRight()`
 static Node * joinLeft( Node * l , int bl , Node * m , Node * r , int br );
 };

/// the join of the trees balanced by weight, with \f$ \alpha = 0.29 \f$
template< class A >
struct WeightBalancedJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 static constexpr std::size_t kAlphaNum = 29;    ///< \f$ \alpha \f$, up
 static constexpr std::size_t kAlphaDen = 100;   ///< and down

 /// the join of the trees \f$ l \f$ and \f$ r \f$ balanced by weight with
 /// \f$ m \f$ between them, in \f$ O( | \log w( l ) - \log w( r ) | + 1 ) \f$
 /// time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, after Nievergelt and Reingold,
 /// beyond the book), \f$ w \f$ the weight, \f$ \mathrm{like}( a , b ) \f$
 /// true if \f$ \alpha ( a + b ) \le \min( a , b ) \f$, and the mirror of
 /// `joinRight()` when \f$ R \f$ is heavier:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join}( L , k , R ) \cr
 /// 2 & \quad \textbf{if}\ \mathrm{like}( w( L ) , w( R ) )\ \textbf{then
 ///     return}
 ///     \ \mathrm{node}( L , k , R ) \cr
 /// 3 & \quad \textbf{if}\ w( L ) > w( R )\ \textbf{then return}
 ///     \ \mathrm{joinRight}( L , k , R ) \cr
 /// 4 & \quad \textbf{return}\ \mathrm{joinLeft}( L , k , R ) \cr
 /// 5 & \textbf{function}\ \mathrm{joinRight}( L , k , R ) \cr
 /// 6 & \quad \textbf{if}\ \mathrm{like}( w( L ) , w( R ) )\ \textbf{then
 ///     return}
 ///     \ \mathrm{node}( L , k , R ) \cr
 /// 7 & \quad ( l , k' , c ) \gets \mathrm{expose}( L );\ T' \gets
 ///     \mathrm{joinRight}( c , k , R ) \cr
 /// 8 & \quad \textbf{if}\ \mathrm{like}( w( l ) , w( T' ) )\ \textbf{then
 ///     return}
 ///     \ \mathrm{node}( l , k' , T' ) \cr
 /// 9 & \quad ( l_1 , r_1 ) \gets \text{the children of } T' \cr
 /// 10 & \quad \textbf{if}\ \mathrm{like}( w( l ) , w( l_1 ) )\ \textbf{and}
 ///     \ \mathrm{like}( w( l ) + w( l_1 ) , w( r_1 ) )\ \textbf{then} \cr
 /// 11 & \qquad \textbf{return}\ \mathrm{rotateLeft}( \mathrm{node}( l , k' ,
 ///     T' )
 ///     ) \cr
 /// 12 & \quad \textbf{return}\ \mathrm{rotateLeft}( \mathrm{node}( l , k' ,
 ///     \mathrm{rotateRight}( T' ) ) )
 /// \end{array}}
 /// \f]
 static Node * join( Node * l , Node * m , Node * r );

 /// the join when \f$ l \f$ is heavier than \f$ r \f$ beyond the balance, by
 /// the right spine of \f$ l \f$
 static Node * joinRight( Node * l , Node * m , Node * r );

 /// the join when \f$ r \f$ is heavier than \f$ l \f$ beyond the balance, the
 /// mirror of `joinRight()`
 static Node * joinLeft( Node * l , Node * m , Node * r );

 /// whether the weights \f$ a \f$ and \f$ b \f$ may be the two sides of a
 /// node: \f$ \alpha ( a + b ) \le \min( a , b ) \f$
 static bool like( std::size_t a , std::size_t b );

 /// the weight of \f$ t \f$, its size plus 1
 static std::size_t weight( const Node * t );
 };

/// the join of the treaps; `A` gives also `priority( t )`, of a node
template< class A >
struct TreapJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// the join of the treaps \f$ l \f$ and \f$ r \f$ with \f$ m \f$ between
 /// them, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Blelloch and Reid-Miller, beyond the book), \f$ p \f$ the
 /// priority, \f$ p( \textbf{nil} ) = -\infty \f$, the root of the result the
 /// one of highest priority among those of \f$ L \f$ and \f$ R \f$ and
 /// \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join}( L , k , R ) \cr
 /// 2 & \quad \textbf{if}\ p( L ) > p( k )\ \textbf{and}\ p( L ) \ge p( R )
 ///     \ \textbf{then} \cr
 /// 3 & \qquad ( l , k' , r ) \gets \mathrm{expose}( L );\ \textbf{return}
 ///     \ \mathrm{node}( l , k' , \mathrm{join}( r , k , R ) ) \cr
 /// 4 & \quad \textbf{if}\ p( R ) > p( k )\ \textbf{then} \cr
 /// 5 & \qquad ( l , k' , r ) \gets \mathrm{expose}( R );\ \textbf{return}
 ///     \ \mathrm{node}( \mathrm{join}( L , k , l ) , k' , r ) \cr
 /// 6 & \quad \textbf{return}\ \mathrm{node}( L , k , R )
 /// \end{array}}
 /// \f]
 static Node * join( Node * l , Node * m , Node * r );
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS JoinAlgorithms ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the algorithms on the trees whose only balancing is the join of `A`;
/// each of them consumes the trees it is given and returns the trees it
/// makes
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

 /// the tree of root \f$ t \f$, its rank computed
 [[nodiscard]] static Tree whole( Node * t );

 /// the subtrees and the root of \f$ x \f$
 /// \pre \f$ x \f$ is not empty
 [[nodiscard]] static Exposed expose( Tree x );

 /// the join of the scheme, with the ranks
 [[nodiscard]] static Tree join( Tree l , Node * m , Tree r );

 /// the keys of \f$ x \f$ smaller than \f$ k \f$ in \f$ l \f$, those larger in
 /// \f$ r \f$, and the node of \f$ k \f$ returned, alone (nil if \f$ k \f$ is
 /// not in \f$ x \f$), in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), the costs
 /// of the joins telescoping along the path:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{split}( T , k ) \cr
 /// 2 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{then return}\ (
 ///     \textbf{nil} , \textbf{nil} , \textbf{nil} ) \cr
 /// 3 & \quad ( L , m , R ) \gets \mathrm{expose}( T ) \cr
 /// 4 & \quad \textbf{if}\ k < m\ \textbf{then} \cr
 /// 5 & \qquad ( L' , b , R' ) \gets \mathrm{split}( L , k );\ \textbf{return}
 ///     \ ( L' , b , \mathrm{join}( R' , m , R ) ) \cr
 /// 6 & \quad \textbf{if}\ m < k\ \textbf{then} \cr
 /// 7 & \qquad ( L' , b , R' ) \gets \mathrm{split}( R , k );\ \textbf{return}
 ///     \ ( \mathrm{join}( L , m , L' ) , b , R' ) \cr
 /// 8 & \quad \textbf{return}\ ( L , m , R )
 /// \end{array}}
 /// \f]
 static Node * split( Tree x , const Key & k , Tree & l , Tree & r );

 /// the node of the largest key of \f$ x \f$ returned, alone, and the others
 /// in `rest`, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{splitLast}( T ) \cr
 /// 2 & \quad ( L , k , R ) \gets \mathrm{expose}( T ) \cr
 /// 3 & \quad \textbf{if}\ R = \textbf{nil}\ \textbf{then return}\ ( L , k )
 ///     \cr
 /// 4 & \quad ( T' , k' ) \gets \mathrm{splitLast}( R );\ \textbf{return}\ (
 ///     \mathrm{join}( L , k , T' ) , k' )
 /// \end{array}}
 /// \f]
 /// \pre \f$ x \f$ is not empty
 static Node * splitLast( Tree x , Tree & rest );

 /// the tree of the keys of \f$ l \f$ and of \f$ r \f$, with no key in
 /// between, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join2}( L , R ) \cr
 /// 2 & \quad \textbf{if}\ L = \textbf{nil}\ \textbf{then return}\ R \cr
 /// 3 & \quad ( L' , k ) \gets \mathrm{splitLast}( L );\ \textbf{return}
 ///     \ \mathrm{join}( L' , k , R )
 /// \end{array}}
 /// \f]
 /// \pre the keys of \f$ l \f$ are smaller than those of \f$ r \f$
 [[nodiscard]] static Tree join2( Tree l , Tree r );

 /// the node alone \f$ n \f$ inserted in \f$ x \f$, `fresh` whether its key
 /// was not there (if it was, its value replaces the old one and \f$ n \f$ is
 /// freed), by joins on the way back, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( T , n ) \cr
 /// 2 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{then return}
 ///     \ \mathrm{join}( \textbf{nil} , n , \textbf{nil} ) \cr
 /// 3 & \quad ( L , m , R ) \gets \mathrm{expose}( T ) \cr
 /// 4 & \quad \textbf{if}\ n < m\ \textbf{then return}\ \mathrm{join}(
 ///     \mathrm{insert}( L , n ) , m , R ) \cr
 /// 5 & \quad \textbf{if}\ m < n\ \textbf{then return}\ \mathrm{join}( L , m ,
 ///     \mathrm{insert}( R , n ) ) \cr
 /// 6 & \quad \text{the value of } m \gets \text{that of } n;\ \text{free } n;
 ///     \ \textbf{return}\ \mathrm{join}( L , m , R )
 /// \end{array}}
 /// \f]
 [[nodiscard]] static Tree insert( Tree x , Node * n , bool & fresh );

 /// the key \f$ k \f$ taken out of \f$ x \f$ and freed, `gone` whether it was
 /// there, by joins on the way back and `join2` where it was, in
 /// \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( T , k ) \cr
 /// 2 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{then return}\ T \cr
 /// 3 & \quad ( L , m , R ) \gets \mathrm{expose}( T ) \cr
 /// 4 & \quad \textbf{if}\ k < m\ \textbf{then return}\ \mathrm{join}(
 ///     \mathrm{remove}( L , k ) , m , R ) \cr
 /// 5 & \quad \textbf{if}\ m < k\ \textbf{then return}\ \mathrm{join}( L , m ,
 ///     \mathrm{remove}( R , k ) ) \cr
 /// 6 & \quad \text{free } m;\ \textbf{return}\ \mathrm{join2}( L , R )
 /// \end{array}}
 /// \f]
 [[nodiscard]] static Tree remove( Tree x , const Key & k , bool & gone );

 /// the union of \f$ a \f$ and \f$ b \f$, on a key of both the value
 /// \f$ f( \text{of } a , \text{of } b ) \f$, with
 /// \f$ O( m \log( n / m + 1 ) ) \f$ work and \f$ O( \log m \log n ) \f$ span
 /// for sizes \f$ m \le n \f$
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{union}( A , B ) \cr
 /// 2 & \quad \textbf{if}\ A = \textbf{nil}\ \textbf{then return}\ B \cr
 /// 3 & \quad \textbf{if}\ B = \textbf{nil}\ \textbf{then return}\ A \cr
 /// 4 & \quad ( L , k , R ) \gets \mathrm{expose}( \text{the smaller of } A
 ///     \text{ and } B ) \cr
 /// 5 & \quad ( L' , b , R' ) \gets \mathrm{split}( \text{the larger of } A
 ///     \text{ and } B , k ) \cr
 /// 6 & \quad T_1 \gets \mathrm{union}( L , L' ) \text{ and } T_2 \gets
 ///     \mathrm{union}( R , R' ) \text{, in parallel} \cr
 /// 7 & \quad \textbf{if}\ b \ne
 ///     \textbf{nil}\ \textbf{then}\ \text{the value }
 ///     \text{of } k \gets f( \text{of } A , \text{of } B ) \cr
 /// 8 & \quad \textbf{return}\ \mathrm{join}( T_1 , k , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E , class F >
 [[nodiscard]] static Tree unite( E & ex , Tree a , Tree b , const F & f );

 /// the intersection of \f$ a \f$ and \f$ b \f$, the value
 /// \f$ f( \text{of } a , \text{of } b ) \f$, with the work and the span of
 /// `unite()`; the nodes that leave are freed
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{intersect}( A , B ) \cr
 /// 2 & \quad \textbf{if}\ A = \textbf{nil}\ \textbf{or}\ B = \textbf{nil}
 ///     \ \textbf{then return}\ \textbf{nil} \cr
 /// 3 & \quad ( L , k , R ) \gets \mathrm{expose}( \text{the smaller of } A
 ///     \text{ and } B ) \cr
 /// 4 & \quad ( L' , b , R' ) \gets \mathrm{split}( \text{the larger of } A
 ///     \text{ and } B , k ) \cr
 /// 5 & \quad T_1 \gets \mathrm{intersect}( L , L' ) \text{ and } T_2 \gets
 ///     \mathrm{intersect}( R , R' ) \text{, in parallel} \cr
 /// 6 & \quad \textbf{if}\ b = \textbf{nil}\ \textbf{then return}
 ///     \ \mathrm{join2}( T_1 , T_2 ) \cr
 /// 7 & \quad \text{the value of } k \gets f( \text{of } A , \text{of } B );
 ///     \ \textbf{return}\ \mathrm{join}( T_1 , k , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E , class F >
 [[nodiscard]] static Tree intersect( E & ex , Tree a , Tree b ,
                                      const F & f );

 /// the keys of \f$ a \f$ not in \f$ b \f$, with the work and the span of
 /// `unite()`; the nodes of \f$ b \f$ and those of \f$ a \f$ that leave are
 /// freed
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), the split
 /// always of the larger tree:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{difference}( A , B ) \cr
 /// 2 & \quad \textbf{if}\ A = \textbf{nil}\ \textbf{or}\ B = \textbf{nil}
 ///     \ \textbf{then return}\ A \cr
 /// 3 & \quad \textbf{if}\ |A| < |B|\ \textbf{then} \cr
 /// 4 & \qquad ( L , k , R ) \gets \mathrm{expose}( A );\ ( L' , b , R' )
 ///     \gets
 ///     \mathrm{split}( B , k ) \cr
 /// 5 & \qquad T_1 \gets \mathrm{difference}( L , L' ) \text{ and } T_2 \gets
 ///     \mathrm{difference}( R , R' ) \text{, in parallel} \cr
 /// 6 & \qquad \textbf{if}\ b = \textbf{nil}\ \textbf{then return}
 ///     \ \mathrm{join}( T_1 , k , T_2 )\ \textbf{else
 ///     return}\ \mathrm{join2}(
 ///     T_1 , T_2 ) \cr
 /// 7 & \quad ( L , k , R ) \gets \mathrm{expose}( B );\ ( L' , b , R' ) \gets
 ///     \mathrm{split}( A , k ) \cr
 /// 8 & \quad T_1 \gets \mathrm{difference}( L' , L ) \text{ and } T_2 \gets
 ///     \mathrm{difference}( R' , R ) \text{, in parallel} \cr
 /// 9 & \quad \textbf{return}\ \mathrm{join2}( T_1 , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E >
 [[nodiscard]] static Tree subtract( E & ex , Tree a , Tree b );

 /// the tree of the \f$ n \f$ nodes alone of \f$ N \f$, whose keys increase:
 /// the middle one joined with the trees of the two halves, built in parallel,
 /// with \f$ O( n ) \f$ work and \f$ O( \log^2 n ) \f$ span
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( N[ 0 .. n - 1 ] ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ \textbf{nil} \cr
 /// 3 & \quad h \gets \lfloor n / 2 \rfloor \cr
 /// 4 & \quad T_1 \gets \mathrm{build}( N[ 0 .. h - 1 ] ) \text{ and } T_2
 ///     \gets \mathrm{build}( N[ h + 1 .. n - 1 ] ) \text{, in parallel} \cr
 /// 5 & \quad \textbf{return}\ \mathrm{join}( T_1 , N[ h ] , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E >
 [[nodiscard]] static Tree build( E & ex , Node * const * N ,
                                  std::size_t n );

 /// the \f$ n \f$ nodes alone of \f$ N \f$, whose keys increase, inserted in
 /// \f$ x \f$; on a key already there the value
 /// \f$ f( \text{old} , \text{new} ) \f$, and the node of \f$ N \f$ freed. It
 /// is the union of \f$ x \f$ and of the tree of \f$ N \f$, with the split of
 /// the batch by a binary search in place of `split()`, so that the tree of
 /// \f$ N \f$ is never built: \f$ O( m \log( n / m + 1 ) ) \f$ work for
 /// \f$ m = \min( n , |x| ) \f$
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiInsert}( T , N[ 1 .. n ] ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ T \cr
 /// 3 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{then return}
 ///     \ \mathrm{build}( N ) \cr
 /// 4 & \quad ( L , k , R ) \gets \mathrm{expose}( T ) \cr
 /// 5 & \quad j \gets \text{the first index with } N[ j ] \ge k \text{, by a }
 ///     \text{binary search} \cr
 /// 6 & \quad j' \gets j + 1 \text{ if } N[ j ] = k \text{, } j
 ///     \text{ otherwise}
 ///     \cr
 /// 7 & \quad T_1 \gets \mathrm{multiInsert}( L , N[ 1 .. j - 1 ] )
 ///     \text{ and }
 ///     T_2 \gets \mathrm{multiInsert}( R , N[ j' .. n ] ) \text{, in } \cr
 /// & \qquad \text{parallel} \cr
 /// 8 & \quad \textbf{return}\ \mathrm{join}( T_1 , k , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E , class F >
 [[nodiscard]] static Tree multiInsert( E & ex , Tree x , Node * const * N ,
                                        std::size_t n , const F & f );

 /// the \f$ n \f$ keys of \f$ K \f$, increasing, taken out of \f$ x \f$ and
 /// freed, `gone` how many were there: the difference of \f$ x \f$ and
 /// \f$ K \f$ with the split of \f$ K \f$ by a binary search,
 /// \f$ O( m \log( n / m + 1 ) ) \f$ work
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiRemove}( T , K[ 1 .. n ] ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{or}\ T = \textbf{nil}\ \textbf{then
 ///     return}\ T \cr
 /// 3 & \quad ( L , k , R ) \gets \mathrm{expose}( T );\ j \gets
 ///     \text{the first }
 ///     \text{index with } K[ j ] \ge k \cr
 /// 4 & \quad j' \gets j + 1 \text{ if } K[ j ] = k \text{, } j
 ///     \text{ otherwise}
 ///     \cr
 /// 5 & \quad T_1 \gets \mathrm{multiRemove}( L , K[ 1 .. j - 1 ] )
 ///     \text{ and }
 ///     T_2 \gets \mathrm{multiRemove}( R , K[ j' .. n ] ) \text{, in } \cr
 /// & \qquad \text{parallel} \cr
 /// 6 & \quad \textbf{if}\ K[ j ] = k\ \textbf{then}\ \text{free } k;
 ///     \ \textbf{return}\ \mathrm{join2}( T_1 , T_2 ) \cr
 /// 7 & \quad \textbf{return}\ \mathrm{join}( T_1 , k , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E >
 [[nodiscard]] static Tree multiRemove( E & ex , Tree x , const Key * K ,
                                        std::size_t n , std::size_t & gone );

 /// the nodes of \f$ x \f$ with \f$ p( k , v ) \f$ kept, the others freed:
 /// \f$ O( n ) \f$ work and \f$ O( \log^2 n ) \f$ span
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{filter}( T , p ) \cr
 /// 2 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{then return}\ T \cr
 /// 3 & \quad ( L , k , R ) \gets \mathrm{expose}( T ) \cr
 /// 4 & \quad T_1 \gets \mathrm{filter}( L , p ) \text{ and } T_2 \gets
 ///     \mathrm{filter}( R , p ) \text{, in parallel} \cr
 /// 5 & \quad \textbf{if}\ p( k )\ \textbf{then return}\ \mathrm{join}( T_1 ,
 ///     k ,
 ///     T_2 ) \cr
 /// 6 & \quad \text{free } k;\ \textbf{return}\ \mathrm{join2}( T_1 , T_2 )
 /// \end{array}}
 /// \f]
 template< Executor E , class P >
 [[nodiscard]] static Tree filter( E & ex , Tree x , const P & p );

 /// the reduction of the values \f$ \mathrm{map}( k , v ) \f$ of the pairs of
 /// the tree of root \f$ t \f$, in the order of the keys, so that
 /// \f$ \mathrm{reduce} \f$ needs to be associative only, of identity
 /// \f$ \mathit{id} \f$; \f$ O( n ) \f$ work and \f$ O( \log n ) \f$ span
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{mapReduce}( T ) \cr
 /// 2 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{then return}\ \mathit{id}
 ///     \cr
 /// 3 & \quad ( L , k , R ) \gets \text{the children and the pair of } T \cr
 /// 4 & \quad a \gets \mathrm{mapReduce}( L ) \text{ and } b \gets
 ///     \mathrm{mapReduce}( R ) \text{, in parallel} \cr
 /// 5 & \quad \textbf{return}\ \mathrm{reduce}( \mathrm{reduce}( a ,
 ///     \mathrm{map}( k ) ) , b )
 /// \end{array}}
 /// \f]
 template< Executor E , class M , class R , class U >
 [[nodiscard]] static U mapReduce( E & ex , Node * t , const M & map ,
                                   const R & reduce , const U & id );

 /// the keys of \f$ x \f$ in \f$ [ \mathit{lo} , \mathit{hi} ] \f$ taken out
 /// into the tree returned, by two splits, two joins and a `join2`, in
 /// \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{extract}( T , \mathit{lo} , \mathit{hi} )
 ///     \cr
 /// 2 & \quad ( A , u , B ) \gets \mathrm{split}( T , \mathit{lo} );\ B \gets
 ///     \mathrm{join}( \textbf{nil} , u , B ) \text{ if } u \ne \textbf{nil}
 ///     \cr
 /// 3 & \quad ( C , v , D ) \gets \mathrm{split}( B , \mathit{hi} );\ C \gets
 ///     \mathrm{join}( C , v , \textbf{nil} ) \text{ if } v \ne \textbf{nil}
 ///     \cr
 /// 4 & \quad T \gets \mathrm{join2}( A , D );\ \textbf{return}\ C
 /// \end{array}}
 /// \f]
 [[nodiscard]] static Tree extract( Tree & x , const Key & lo ,
                                    const Key & hi );

 /// the smallest key of \f$ t \f$
 /// \pre \f$ t \f$ is not nil
 [[nodiscard]] static const Key & firstKey( Node * t );

 /// the largest key of \f$ t \f$
 /// \pre \f$ t \f$ is not nil
 [[nodiscard]] static const Key & lastKey( Node * t );

 private:

 /// `g()` and `h()`, in parallel on `ex`
 template< Executor E , class G , class H >
 static void fork( E & ex , G && g , H && h );

 /// `g()` and `h()`, one after the other
 template< class G , class H >
 static void fork( Sequential & ex , G && g , H && h );

 /// whether \f$ n \f$ items are too few for more than the calling thread
 template< Executor E >
 [[nodiscard]] static bool small( E & ex , std::size_t n );

 };  // end( struct JoinAlgorithms )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Joinable ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the value of the second tree, or of the batch, on a key in both
struct TakeSecond {
 /// \f$ b \f$
 template< class V >
 V operator()( const V & , const V & b ) const { return( b ); }
 };

/// the value of the first tree on a key in both
struct TakeFirst {
 /// \f$ a \f$
 template< class V >
 V operator()( const V & a , const V & ) const { return( a ); }
 };

/// the operations based on join of the tree `T` of keys `Key` and values
/// `Value`, whose nodes `A` adapts [see `JoinAdaptor`], each the function
/// of `JoinAlgorithms` of the same name: a base of `T` (which makes `A` its
/// friend); `A` is read only in the bodies, as `T` is incomplete where its
/// bases are named, and gives `take( t )`, the root of \f$ t \f$ taken out
/// of it, which is left empty; `put( t , r )`, \f$ r \f$ the new root of the
/// empty \f$ t \f$, which updates its size; `peek( t )`, its root;
/// `fresh( t )`, an empty tree like \f$ t \f$; `valid( t )`, the invariant
/// of \f$ t \f$; `salt( t )` and `make( k , v , s , i )`, a node alone of the
/// pair \f$ ( k , v ) \f$, the \f$ i \f$-th of a batch of salt \f$ s \f$,
/// which may be called in parallel
template< class T , class A , class Key , class Value >
class Joinable {

/*--------------------------------------------------------------------------*/
 public:

/*--------------------------------------------------------------------------*/
/*------------------------- SPLITTING AND JOINING --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the keys before \f$ k \f$, the value of \f$ k \f$ (if there), the keys
 /// after \f$ k \f$, by `JoinAlgorithms::split()`; the tree is consumed, in
 /// \f$ O( \log n ) \f$ time
 [[nodiscard]] std::tuple< T , std::optional< Value > , T > split(
                                                        const Key & k ) &&;

 /// the tree of the keys of \f$ l \f$, the pair \f$ ( k , v ) \f$ and the
 /// keys of \f$ r \f$, by the join of the scheme; \f$ l \f$ and \f$ r \f$ are
 /// consumed, in \f$ O( | \log n_l - \log n_r | + 1 ) \f$ time
 /// \pre the keys of \f$ l \f$ are smaller than \f$ k \f$, those of \f$ r \f$
 ///      larger
 [[nodiscard]] static T join( T && l , const Key & k , const Value & v ,
                              T && r );

 /// the tree of the keys of \f$ l \f$ and of \f$ r \f$, which are consumed,
 /// by `JoinAlgorithms::join2()`, in \f$ O( \log n ) \f$ time
 /// \pre the keys of \f$ l \f$ are smaller than those of \f$ r \f$
 [[nodiscard]] static T join2( T && l , T && r );

 /// the pairs of the keys in \f$ [ \mathit{lo} , \mathit{hi} ] \f$, taken
 /// out of the tree by `JoinAlgorithms::extract()`, in \f$ O( \log n ) \f$
 /// time
 [[nodiscard]] T extractRange( const Key & lo , const Key & hi );

/*--------------------------------------------------------------------------*/
/*--------------------------- BULK OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the union with `o`, which is consumed, by `JoinAlgorithms::unite()`;
 /// on a key of both, the value `f( mine , its )`, by default its;
 /// \f$ O( m \log( n / m + 1 ) ) \f$ work for sizes \f$ m \le n \f$
 template< class F = TakeSecond >
 void unionWith( T && o , F f = F() ) {
  Sequential seq;
  unionWith( seq , std::move( o ) , f );
  }

 /// `unionWith( o , f )` on the threads of `ex`
 template< Executor E , class F = TakeSecond >
 void unionWith( E & ex , T && o , F f = F() );

 /// the intersection with `o`, which is consumed, by
 /// `JoinAlgorithms::intersect()`; the value `f( mine , its )`, by default
 /// mine; the work of `unionWith()`, plus the freeing of the nodes that
 /// leave
 template< class F = TakeFirst >
 void intersectWith( T && o , F f = F() ) {
  Sequential seq;
  intersectWith( seq , std::move( o ) , f );
  }

 /// `intersectWith( o , f )` on the threads of `ex`
 template< Executor E , class F = TakeFirst >
 void intersectWith( E & ex , T && o , F f = F() );

 /// the keys not in `o`, which is consumed, by
 /// `JoinAlgorithms::subtract()`; the work of `unionWith()`, plus the
 /// freeing of the nodes of `o`
 void differenceWith( T && o ) {
  Sequential seq;
  differenceWith( seq , std::move( o ) );
  }

 /// `differenceWith( o )` on the threads of `ex`
 template< Executor E >
 void differenceWith( E & ex , T && o );

 /// the pairs of \f$ B \f$ inserted, by `JoinAlgorithms::multiInsert()`; on
 /// a key already there, the value `f( mine , new )`, by default new;
 /// \f$ O( m \log( n / m + 1 ) ) \f$ work for \f$ m \f$ pairs
 /// \pre the keys of \f$ B \f$ increasing (an audit)
 template< class F = TakeSecond >
 void multiInsert( std::span< const std::pair< Key , Value > > B ,
                   F f = F() ) {
  Sequential seq;
  multiInsert( seq , B , f );
  }

 /// `multiInsert( B , f )` on the threads of `ex`
 template< Executor E , class F = TakeSecond >
 void multiInsert( E & ex , std::span< const std::pair< Key , Value > > B ,
                   F f = F() );

 /// the keys of \f$ B \f$ removed, by `JoinAlgorithms::multiRemove()`,
 /// \f$ O( m \log( n / m + 1 ) ) \f$ work
 /// \pre the keys of \f$ B \f$ increasing (an audit)
 void multiRemove( std::span< const Key > B ) {
  Sequential seq;
  multiRemove( seq , B );
  }

 /// `multiRemove( B )` on the threads of `ex`
 template< Executor E >
 void multiRemove( E & ex , std::span< const Key > B );

 /// only the pairs with `p( k , v )` kept, by `JoinAlgorithms::filter()`,
 /// \f$ O( n ) \f$ work
 template< class P >
 void filter( P p ) {
  Sequential seq;
  filter( seq , p );
  }

 /// `filter( p )` on the threads of `ex`
 template< Executor E , class P >
 void filter( E & ex , P p );

 /// \f$ \mathrm{reduce}( \ldots \mathrm{reduce}( \mathit{id} , \mathrm{map}(
 /// k_1 , v_1 ) ) \ldots , \mathrm{map}( k_n , v_n ) ) \f$ by an associative
 /// `reduce` of identity `id`, by `JoinAlgorithms::mapReduce()`,
 /// \f$ O( n ) \f$ work
 template< class M , class R , class U >
 [[nodiscard]] U mapReduce( M map , R reduce , U id ) const {
  Sequential seq;
  return( mapReduce( seq , map , reduce , std::move( id ) ) );
  }

 /// `mapReduce( map , reduce , id )` on the threads of `ex`
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
