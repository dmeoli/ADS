/*--------------------------------------------------------------------------*/
/*-------------------------- File ScapegoatTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The scapegoat tree of Galperin and Rivest, the general balanced tree of
 * Andersson (section 8.1 of Morin), beyond the book: a binary search tree
 * with no balance information in its nodes, whose height stays at most
 * \f$ \log_b q \f$, \f$ q \f$ an upper bound on the number of keys that
 * satisfies \f$ q / 2 \le n \le q \f$ and \f$ 1 < b < 2 \f$ a parameter
 * (\f$ b = 3 / 2 \f$ in Morin, \f$ b = 1 / \alpha \f$ in Galperin and
 * Rivest), by rebuilding subtrees into perfectly balanced ones. An insertion
 * that makes a leaf deeper than \f$ \log_b q \f$ climbs from it to the first
 * ancestor \f$ w \f$ below which it is deeper than \f$ \log_b s( w ) \f$, the
 * scapegoat, and rebuilds the subtree of \f$ w \f$, whose height becomes
 * \f$ \lfloor \log_2 s( w ) \rfloor < d - \mathrm{depth}( w ) \f$, so that no
 * node is left deeper than the parent of the new leaf was. The root
 * qualifies, since \f$ \log_b n \le \log_b q < d \f$, and the scapegoat has
 * a child \f$ u \f$ on the path with \f$ s( u ) > s( w ) / b \f$, since
 * otherwise the sizes would shrink by \f$ b \f$ at each level below it.
 * This is the rule of Galperin and Rivest (`HEIGHT`, the default); the
 * rule of Morin (`WEIGHT`), the first ancestor with \f$ s( u ) > s( w ) / b
 * \f$, rebuilds the smallest unbalanced subtree, but the argument of section
 * 8.1 of Morin for it, that the subtree of a scapegoat is not complete and
 * so loses a level, does not cover every case: if \f$ w \f$ has one child,
 * whose two children are leaves, one of them the new one, then
 * \f$ s( w ) = 4 \f$, the child holds \f$ 3 > \frac23 \cdot 4 \f$ of them,
 * and 4 nodes perfectly balanced still have height 2. No sequence of the
 * tests (600000 random ones, on 4 to 32 keys and \f$ b \f$ from 1.3 to 1.8)
 * reached a tree that the rule of Morin left too high, and the class keeps
 * it as an option, its height checked by the invariant. A
 * removal rebuilds the whole tree when \f$ q > 2 n \f$. A search takes
 * \f$ O( \log n ) \f$ in the worst case, and an insertion or a removal
 * \f$ O( \log n ) \f$ amortized (Lemma 8.3 and Theorem 8.1 of Morin, whose
 * argument with \f$ b \f$ in place of \f$ 3 / 2 \f$ charges
 * \f$ 1 / ( 2 / b - 1 ) \f$ credits per node of a path, so that the
 * constant grows as \f$ b \f$ nears 2 and the height grows as it nears 1:
 * exercise 4 of chapter 8).
 *
 * The variants of the exercises of chapter 8 of Morin are options of the
 * class, so that the benchmarks compare them on the same code. The sizes are
 * computed when the climb needs them, with no field in the nodes, as in
 * Morin; `RECOMPUTE` counts the whole subtree of each ancestor, as his code
 * does, `REUSE` (the default, exercise 8.5) adds to the size of the child
 * on the path, known from the step before, the size of the other child
 * alone, which makes the climb cost \f$ O( s( w ) ) \f$ in all instead of a
 * sum of sizes; `ScapegoatTree< K , V , true >` keeps the sizes in the nodes
 * (exercise 8.6), updated along the path of each insertion and removal, so
 * that the climb reads them. The rebuild lays the nodes of the subtree in a
 * buffer and links the middle one at the root of each part (`ARRAY`, as in
 * Morin, but in a buffer kept from one rebuild to the next instead of an
 * array allocated each time), or threads them into a list by their right
 * links and builds the tree from the list by recursion with no buffer
 * (`LIST`, exercise 8.7), the two giving the same tree. The counters
 * `rebuilds()` and `rebuiltNodes()` measure the work of the rebuilds, e.g.,
 * the \f$ \Omega( n \log n ) \f$ of the insertions in increasing order
 * (exercise 8.3).
 *
 * Where it is meant to be faster than the `ScapegoatTree` of Morin, for the
 * benchmarks to check: his nodes have a parent, here the climb follows the
 * path of the descent kept in a vector, so that a node has 2 links instead
 * of 3; his test of the depth computes \f$ \log_{3/2} q \f$ by a logarithm
 * at each insertion, here a comparison with a table of the powers of
 * \f$ b \f$; his climb recomputes \f$ s( w ) \f$ and \f$ s( w.\mathit{parent}
 * ) \f$ from scratch at each step, `REUSE` counts each node at most once; and
 * the buffer of the rebuild is allocated once.
 *
 * The contracts [see `Contract.h`]: \f$ 1 < b < 2 \f$; `insert()` ensures that
 * the key is there and `remove()` that it is not; the invariant, an
 * \f$ O( n ) \f$ check at the level 3, is the order of the keys, the bound
 * \f$ h \le \log_b q \f$, \f$ q / 2 \le n \le q \f$ and the stored sizes. The
 * operations are defined in `ScapegoatTree.tpp`.
 *
 * \see I. Galperin, R. L. Rivest, Scapegoat trees, Proceedings of the 4th
 *      ACM-SIAM Symposium on Discrete Algorithms (SODA), 165-174, 1993
 * \see A. Andersson, Improving partial rebuilding by using simple balance
 *      criteria, Algorithms and Data Structures (WADS 1989), Lecture Notes
 *      in Computer Science 382, 393-402, 1989, doi:10.1007/3-540-51542-9_33
 * \see A. Andersson, General balanced trees, Journal of Algorithms 30(1),
 *      1-18, 1999, doi:10.1006/jagm.1998.0967
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 8.1 and exercises 8.3 to 8.7,
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

#ifndef __ADS_ScapegoatTree
 #define __ADS_ScapegoatTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the rebuilds of the trees of partial rebuilding: a subtree of nodes
/// with the fields `left` and `right` made perfectly balanced, the left
/// part of each node of \f$ s \f$ nodes holding \f$ \lfloor ( s - 1 ) / 2
/// \rfloor \f$ of them, and `set( u , s )` called on each node with the
/// size of its new subtree
struct Rebuild {

 /// the nodes of the subtree of \f$ u \f$ in symmetric order, appended to
 /// \f$ B \f$ by a walk with no stack (the right links threaded as in the
 /// traversal of Morris, and restored)
 template< class N >
 static void flatten( N * u , std::vector< N * > & B ) {
  while( u )
   if( ! u->left ) { B.push_back( u ); u = u->right; }
   else {
    N * p = u->left;
    while( p->right && ( p->right != u ) ) p = p->right;
    if( ! p->right ) { p->right = u; u = u->left; }
    else { p->right = nullptr; B.push_back( u ); u = u->right; }
    }
  }

 /// the balanced tree of \f$ B[ i .. j ) \f$, its root returned
 template< class N , class Set >
 static N * fromArray( N * const * B , std::size_t i , std::size_t j ,
                       Set & set ) {
  if( i >= j ) return( nullptr );
  const std::size_t m = i + ( j - i - 1 ) / 2;
  N * u = B[ m ];
  u->left = fromArray( B , i , m , set );
  u->right = fromArray( B , m + 1 , j , set );
  set( u , j - i );
  return( u );
  }

 /// the subtree of \f$ u \f$ threaded by its right links in symmetric
 /// order before the list \f$ r \f$, its head returned (exercise 8.7 of
 /// Morin)
 template< class N >
 static N * toList( N * u , N * r ) {
  while( u ) {                              // the right spine by a loop,
   r = toList( u->right , r );              // the left subtrees by
   u->right = r;                            // recursion
   r = u;
   N * l = u->left;
   u->left = nullptr;
   u = l;
   }
  return( r );
  }

 /// the balanced tree of the first \f$ s \f$ nodes of the list \f$ h \f$,
 /// which is advanced past them
 template< class N , class Set >
 static N * fromList( std::size_t s , N * & h , Set & set ) {
  if( s == 0 ) return( nullptr );
  N * l = fromList( ( s - 1 ) / 2 , h , set );
  N * u = h;
  h = h->right;
  u->left = l;
  u->right = fromList( s - 1 - ( s - 1 ) / 2 , h , set );
  set( u , s );
  return( u );
  }

 /// the number of nodes of the subtree of \f$ u \f$, by the stack
 /// \f$ S \f$, emptied first
 template< class N >
 static std::size_t count( N * u , std::vector< N * > & S ) {
  std::size_t c = 0;
  S.clear();
  if( u ) S.push_back( u );
  while( ! S.empty() ) {
   N * x = S.back();
   S.pop_back();
   ++c;
   if( x->left ) S.push_back( x->left );
   if( x->right ) S.push_back( x->right );
   }
  return( c );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ScapegoatTree ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a scapegoat tree of height at most \f$ \log_b q \f$,
/// whose nodes keep the size of their subtree if `StoredSizes`
template< std::totally_ordered K , class V , bool StoredSizes = false >
class ScapegoatTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// how the sizes are found when they are not stored
 enum class Sizes {
  RECOMPUTE ,       ///< each one counted, as Morin does
  REUSE             ///< the child on the path known (exercise 8.5)
  };

 /// which ancestor is the scapegoat
 enum class Scapegoats {
  HEIGHT ,          ///< the first too high for its size (Galperin, Rivest)
  WEIGHT            ///< the first with a child too large, as Morin does
  };

 /// how a subtree is rebuilt
 enum class Rebuilds {
  ARRAY ,           ///< by a buffer of its nodes, as Morin does
  LIST              ///< by a list of its nodes, no buffer (exercise 8.7)
  };

 /// the empty tree of height at most \f$ \log_b q \f$
 /// \pre \f$ 1 < b < 2 \f$
 explicit ScapegoatTree( double b = 1.5 , Sizes sizes = Sizes::REUSE ,
                         Rebuilds rebuilds = Rebuilds::ARRAY ,
                         Scapegoats scapegoats = Scapegoats::HEIGHT );

 /// no copy, since the nodes are linked by pointers
 ScapegoatTree( const ScapegoatTree & ) = delete;

 /// no copy, since the nodes are linked by pointers
 ScapegoatTree & operator=( const ScapegoatTree & ) = delete;

 /// the nodes of `other`, which is left empty
 ScapegoatTree( ScapegoatTree && other ) noexcept;

 /// frees the nodes
 ~ScapegoatTree();

 /// the value of \f$ k \f$, or nothing, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (section 8.1 of Morin), \f$ s \f$ the sizes and
 /// \f$ P \f$ the path of the descent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad u \gets \text{a new leaf of } ( k , v ) \text{ at the depth }
 ///     d \text{, after } P;\ n \gets n + 1;\ q \gets q + 1 \cr
 /// 3 & \quad \textbf{if}\ d > \log_b q\ \textbf{then} \cr
 /// 4 & \qquad w \gets u.\mathit{parent} \cr
 /// 5 & \qquad \textbf{while}\ b^{d - \mathrm{depth}( w )} \le s( w )
 ///     \ \textbf{do}\ u \gets w;\ w \gets w.\mathit{parent} \cr
 /// & \qquad\quad \triangleright\ \text{for }\mathtt{WEIGHT}\text{: }
 ///     \textbf{while}\ b \cdot s( u ) \le s( w ) \cr
 /// 6 & \qquad \mathrm{rebuild}( w )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ amortized
 /// time: the removal of a binary search tree, then the whole tree rebuilt
 /// if \f$ q > 2 n \f$, and \f$ q \gets n \f$
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the height, \f$ -1 \f$ if empty, in \f$ O( n ) \f$
 [[nodiscard]] long height( void ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet of Morin)
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
  const K & k ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

 /// the number of rebuilds so far
 [[nodiscard]] std::size_t rebuilds( void ) const { return( f_rebuilds ); }

 /// the nodes of all the rebuilds so far, the work they took
 [[nodiscard]] std::size_t rebuiltNodes( void ) const { return( f_work ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the size of a node, if stored
 struct Size { std::size_t s = 1; };

 /// nothing, if not stored
 struct NoSize {};

 /// a node: its pair, its children and, if stored, its size
 struct Node : std::conditional_t< StoredSizes , Size , NoSize > {
  K key;                       ///< the key
  V value;                     ///< the value
  Node * left = nullptr;       ///< the left child
  Node * right = nullptr;      ///< the right child
  /// the node of a pair
  Node( const K & k , const V & v ) : key( k ) , value( v ) {}
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
 using const_iterator = PathIterator< ScapegoatTree , Shape >;
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

 /// the size of the subtree of \f$ u \f$: read if stored, counted if not
 std::size_t sizeOf( Node * u );

 /// the subtree of \f$ u \f$ rebuilt perfectly balanced, of \f$ s \f$ nodes,
 /// linked in its place below \f$ p \f$ (the root if nil)
 void rebuild( Node * u , std::size_t s , Node * p );

 /// whether \f$ d > \log_b q \f$, by the table of the powers of \f$ b \f$
 bool tooDeep( std::size_t d );

 /// the order, the height, the bounds of \f$ q \f$ and the sizes
 [[nodiscard]] bool valid( void ) const;

 Node * f_root = nullptr;          ///< the root
 std::size_t f_n = 0;              ///< the number \f$ n \f$ of keys
 std::size_t f_q = 0;              ///< the bound \f$ q \f$
 double f_b;                       ///< the base \f$ b \f$
 Sizes f_sizes;                    ///< how the sizes are found
 Rebuilds f_rebuild;               ///< how a subtree is rebuilt
 Scapegoats f_goat;                ///< which ancestor is the scapegoat
 std::vector< double > f_pow;      ///< \f$ b^d \f$ for each depth \f$ d \f$
 std::vector< Node * > f_path;     ///< the path of the last descent
 std::vector< Node * > f_buf;      ///< the buffer of the rebuilds
 std::size_t f_rebuilds = 0;       ///< the rebuilds
 std::size_t f_work = 0;           ///< the nodes rebuilt

 };  // end( class ScapegoatTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ScapegoatTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ScapegoatTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ScapegoatTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
