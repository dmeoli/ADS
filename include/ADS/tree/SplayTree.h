/*--------------------------------------------------------------------------*/
/*---------------------------- File SplayTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The splay tree of Sleator and Tarjan, beyond the book: a binary search
 * tree with no balance information, where each access moves the node
 * accessed to the root by rotations in pairs (zig-zig and zig-zag), which
 * roughly halve the depth of the nodes on its path; any sequence of
 * \f$ m \f$ operations on \f$ n \f$ keys takes
 * \f$ O( ( m + n ) \log n ) \f$ time, i.e., \f$ O( \log n ) \f$ amortized
 * each, and the keys accessed often stay near the root (the static
 * optimality and the working set theorems). The splay is top-down, in one
 * pass from the root that splits the tree into a left and a right part and
 * joins them at the end. It is a DICTIONARY [see `ADT.h`]; `lookup()` also
 * splays, hence it changes the shape of the tree (but not its keys) and is
 * not const.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is
 * there, at the root, and `remove()` that it is not; the invariant is the
 * order of the keys, an \f$ O( n ) \f$ check at the level 3. The operations
 * are defined in `SplayTree.tpp`.
 *
 * \see D. D. Sleator, R. E. Tarjan, Self-adjusting binary search trees,
 *      Journal of the ACM 32(3), 652-686, 1985, doi:10.1145/3828.3835
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

#ifndef __ADS_SplayTree
 #define __ADS_SplayTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS SplayTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a splay tree
template< std::totally_ordered K , class V >
class SplayTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty tree
 SplayTree( void ) = default;
 /// no copy
 SplayTree( const SplayTree & ) = delete;
 /// no copy
 SplayTree & operator=( const SplayTree & ) = delete;

 /// the nodes of `other`, which is left empty
 SplayTree( SplayTree && other ) noexcept;
 /// frees the nodes
 ~SplayTree();

 /// the value of \f$ k \f$, or nothing; \f$ k \f$, or the last node on its
 /// path, goes to the root, in \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (the top-down splay of Sleator and Tarjan, beyond the
 /// book), \f$ L \f$ and \f$ R \f$ the trees of the keys smaller and larger
 /// than \f$ k \f$ met on the way, \f$ L \f$ growing at its largest key and
 /// \f$ R \f$ at its smallest:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{splay}( t , k ) \cr
 /// 2 & \quad L \gets R \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{if}\ k < t.\mathit{key}\ \textbf{then} \cr
 /// 5 & \qquad\quad \textbf{if}\ t.\mathit{left} = \textbf{nil}
 ///     \ \textbf{then break} \cr
 /// 6 & \qquad\quad \textbf{if}\ k < t.\mathit{left}.\mathit{key}
 ///     \ \textbf{then}\ t \gets \mathrm{rotateRight}( t )\quad
 ///     \triangleright\ \text{zig-zig} \cr
 /// 7 & \qquad\quad \textbf{if}\ t.\mathit{left} = \textbf{nil}
 ///     \ \textbf{then break} \cr
 /// 8 & \qquad\quad \text{hang } t \text{ at the smallest key of } R;\ t
 ///     \gets t.\mathit{left}\quad \triangleright\ \text{link right} \cr
 /// 9 & \qquad \textbf{else if}\ t.\mathit{key} < k\ \textbf{then}
 ///     \ \text{the mirror of the lines 5 to 8, on } L \cr
 /// 10 & \qquad \textbf{else break} \cr
 /// 11 & \quad \text{hang } t.\mathit{left} \text{ at the largest key of }
 ///     L \text{, } t.\mathit{right} \text{ at the smallest of } R \cr
 /// 12 & \quad t.\mathit{left} \gets L;\ t.\mathit{right} \gets R;
 ///     \ \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// and `lookup()` finds \f$ k \f$ at the root after `splay( root , k )`,
 /// or nowhere.
 [[nodiscard]] std::optional< V > lookup( const K & k );

 /// associates \f$ v \f$ to \f$ k \f$, which goes to the root, in
 /// \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book), on the `splay()`
 /// of `lookup()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad T \gets \mathrm{splay}( T , k ) \cr
 /// 3 & \quad \textbf{if}\ T \ne \textbf{nil}\ \textbf{and}
 ///     \ T.\mathit{key} = k\ \textbf{then}\ T.\mathit{value} \gets v;
 ///     \ \textbf{return} \cr
 /// 4 & \quad n \gets \text{a new node of } ( k , v ) \cr
 /// 5 & \quad \textbf{if}\ k < T.\mathit{key}\ \textbf{then}\ (
 ///     n.\mathit{left} , n.\mathit{right} ) \gets ( T.\mathit{left} , T
 ///     );\ T.\mathit{left} \gets \textbf{nil} \cr
 /// 6 & \quad \textbf{else}\ ( n.\mathit{left} , n.\mathit{right} ) \gets (
 ///     T , T.\mathit{right} );\ T.\mathit{right} \gets \textbf{nil} \cr
 /// 7 & \quad T \gets n
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$, at the root
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Sleator and Tarjan, beyond the book), the largest key of
 /// the left part splayed to its root, which then has no right child:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( k ) \cr
 /// 2 & \quad T \gets \mathrm{splay}( T , k ) \cr
 /// 3 & \quad \textbf{if}\ T = \textbf{nil}\ \textbf{or}\ T.\mathit{key}
 ///     \ne k\ \textbf{then return} \cr
 /// 4 & \quad \textbf{if}\ T.\mathit{left} = \textbf{nil}\ \textbf{then}
 ///     \ T' \gets T.\mathit{right} \cr
 /// 5 & \quad \textbf{else}\ T' \gets \mathrm{splay}( T.\mathit{left} , k
 ///     );\ T'.\mathit{right} \gets T.\mathit{right} \cr
 /// 6 & \quad \text{free } T;\ T \gets T'
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the key at the root, if any
 [[nodiscard]] std::optional< K > root( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, with no balance information
 struct Node {
  K key;                         ///< the key
  V value;                       ///< the value
  Node * left = nullptr ,        ///< the left child
       * right = nullptr;        ///< the right child
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, along the path from the root; a lookup splays the tree
 // and invalidates it

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
 using const_iterator = PathIterator< SplayTree , Shape >;
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

 /// the top-down splay of \f$ k \f$ in \f$ t \f$: the new root
 static Node * splay( Node * t , const K & k );

 static void destroy( Node * t );

 template< class F >
 static void visit( const Node * t , F & f );

 static bool ordered( const Node * t , const K * lo , const K * hi );

 Node * f_root = nullptr;
 std::size_t f_size = 0;

 };  // end( class SplayTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/SplayTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SplayTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SplayTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
