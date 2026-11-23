/*--------------------------------------------------------------------------*/
/*--------------------- File LeftLeaningRedBlackTree.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The `RedBlackTree` of Morin (chapter 9), a left-leaning red-black tree
 * that simulates a 2-4 tree, beyond the book: a binary search tree whose
 * nodes are red or black with the black-height property (every path from a
 * node down to a nil crosses the same number of black nodes), the no-red-edge
 * property (a red node has no red child) and the left-leaning property (if
 * the left child of a node is black, so is the right one); nil and the root
 * are black. Each black node and its red children are a node of a 2-4 tree,
 * of degree 2 with no red child, 3 with a red left child, 4 with two, and the
 * left-leaning property makes the representation of a 2-4 tree unique; hence
 * the height is at most \f$ 2 \log_2 ( n + 1 ) \f$ (Lemma 9.1 and Lemma 9.2 of
 * Morin). It is a DICTIONARY [see `ADT.h`] with the `find(x)` of an SSet
 * (`lowerBound()`).
 *
 * An insertion adds a red leaf and `addFixup()` restores the properties going
 * up, by `flipLeft()` where the left-leaning property fails, `flipRight()` at
 * a black grandparent whose right child is black (a 2-4 node of degree 3
 * becoming one of degree 4) and `pushBlack()` at a grandparent with two red
 * children (the split of a 2-4 node of degree 5), the last one moving up; a
 * removal splices a node with one child at most, adds its color to the child
 * (possibly making it double black) and `removeFixup()` moves the double
 * black up by the four cases of section 9.2.4 of Morin, `pullBlack()`
 * merging two 2-4 nodes and the flips borrowing from a sibling. Both take
 * \f$ O( \log n ) \f$ time, and Theorem 9.2 of Morin bounds the restructuring,
 * rotations and changes of color, by \f$ O( 1 ) \f$ amortized per operation.
 * `absorb()` joins a tree whose keys are larger (exercises 9.10 and 9.11):
 * the smallest of them becomes a red node over the two trees, linked on the
 * spine of the higher tree where the black heights match, and `addFixup()`
 * restores the properties, \f$ O( h_1 + h_2 ) \f$ time.
 *
 * The links to nil are null pointers: the double black that the removal of a
 * leaf puts on nil is carried by a node of no key placed for the time of
 * `removeFixup()` where the nil was, so that no sentinel is shared or
 * written, and two trees can be joined with no pass on their nodes (where
 * Morin's sentinel, written by the removal, ties every node to its tree;
 * exercise 9.7 asks why the removal sets the parent of the spliced child:
 * the child may be the sentinel, whose parent `removeFixup()` reads). This
 * class is separate from the Red-Black tree of the book [see
 * `SearchTree.h`], which is not left-leaning and repairs by the cases of
 * Cormen et al.
 *
 * Where it is meant to be faster than the `RedBlackTree` of Morin, for the
 * benchmarks to check: the color is a byte, not an `int`; nil is a null
 * pointer read by no load, where his sentinel is a node read at every
 * test of a color; the operations are his, with the same number of
 * rotations, so that the rest of the difference is the allocation and the
 * layout of the nodes.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is there
 * and `remove()` that it is not, `absorb()` needs a tree of larger keys; the
 * invariant, an \f$ O( n ) \f$ check at the level 3, is the order of the keys,
 * the parents and the three properties. The operations are defined in
 * `LeftLeaningRedBlackTree.tpp`.
 *
 * \see L. J. Guibas, R. Sedgewick, A dichromatic framework for balanced
 *      trees, Proceedings of the 19th FOCS, 8-21, 1978,
 *      doi:10.1109/SFCS.1978.3
 * \see A. Andersson, Balanced search trees made simple, Algorithms and Data
 *      Structures (WADS 1993), Lecture Notes in Computer Science 709, 60-71,
 *      1993, doi:10.1007/3-540-57155-8_236
 * \see R. Sedgewick, Left-leaning red-black trees, Department of Computer
 *      Science, Princeton University, 2008
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, chapter 9 and exercises 9.1
 *      to 9.11
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

#ifndef __ADS_LeftLeaningRedBlackTree
 #define __ADS_LeftLeaningRedBlackTree
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS LeftLeaningRedBlackTree ----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on the left-leaning red-black tree of Morin, a simulated
/// 2-4 tree
template< std::totally_ordered K , class V >
class LeftLeaningRedBlackTree
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;    ///< the type of the keys
 using Value = V;  ///< the type of the values

 /// the empty tree
 LeftLeaningRedBlackTree( void ) = default;

 /// no copy, since the nodes are linked by pointers
 LeftLeaningRedBlackTree( const LeftLeaningRedBlackTree & ) = delete;

 /// no copy, since the nodes are linked by pointers
 LeftLeaningRedBlackTree &
 operator=( const LeftLeaningRedBlackTree & ) = delete;

 /// the nodes of `other`, which is left empty
 LeftLeaningRedBlackTree( LeftLeaningRedBlackTree && other ) noexcept
     : f_root( other.f_root ) , f_n( other.f_n ) {
  other.f_root = nullptr;
  other.f_n = 0;
  }

 /// frees the nodes
 ~LeftLeaningRedBlackTree();

 /// the value of \f$ k \f$, or nothing, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (section 9.2.3 of Morin), a red leaf \f$ u \f$ added and
 /// the properties restored going up:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{addFixup}( u ) \cr
 /// 2 & \quad \textbf{while}\ u \text{ is red}\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ u \text{ is the root}\ \textbf{then}\ u
 ///     \text{ black};\ \textbf{return} \cr
 /// 4 & \qquad w \gets u.\mathit{parent} \cr
 /// 5 & \qquad \textbf{if}\ w.\mathit{left} \text{ is black}\ \textbf{then}
 ///     \ \mathrm{flipLeft}( w );\ u \gets w;\ w \gets u.\mathit{parent} \cr
 /// 6 & \qquad \textbf{if}\ w \text{ is black}\ \textbf{then return} \cr
 /// 7 & \qquad g \gets w.\mathit{parent} \cr
 /// 8 & \qquad \textbf{if}\ g.\mathit{right} \text{ is black}\ \textbf{then}
 ///     \ \mathrm{flipRight}( g );\ \textbf{return} \cr
 /// 9 & \qquad \mathrm{pushBlack}( g );\ u \gets g
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (section 9.2.4 of Morin), \f$ u \f$ the child of the
 /// spliced node with the color of the two summed (2 is double black),
 /// \f$ v \f$ its sibling and \f$ w \f$ its parent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{removeFixup}( u ) \cr
 /// 2 & \quad \textbf{while}\ u.\mathit{color} > 1\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ u \text{ is the root}\ \textbf{then}\ u
 ///     \text{ black} \cr
 /// 4 & \qquad \textbf{else if}\ v \text{ is red}\ \textbf{then}
 ///     \ \mathrm{flipRight}( w ) \quad \triangleright\ \text{case 1, }
 ///     \text{then case 3} \cr
 /// 5 & \qquad \textbf{else if}\ u = w.\mathit{left}\ \textbf{then}\ u
 ///     \gets \mathrm{case2}( u ) \cr
 /// 6 & \qquad \textbf{else}\ u \gets \mathrm{case3}( u ) \cr
 /// 7 & \quad \textbf{if}\ u \text{ is not the root and } w.\mathit{right}
 ///     \text{ is red and } w.\mathit{left} \text{ black}\ \textbf{then}
 ///     \ \mathrm{flipLeft}( w ) \cr
 /// 8 & \textbf{function}\ \mathrm{case2}( u ) \cr
 /// 9 & \quad \mathrm{pullBlack}( w );\ \mathrm{flipLeft}( w );\ q \gets
 ///     w.\mathit{right} \cr
 /// 10 & \quad \textbf{if}\ q \text{ is black}\ \textbf{then return}\ v \cr
 /// 11 & \quad \mathrm{rotateLeft}( w );\ \mathrm{flipRight}( v );
 ///     \ \mathrm{pushBlack}( q ) \cr
 /// 12 & \quad \textbf{if}\ v.\mathit{right} \text{ is red}\ \textbf{then}
 ///     \ \mathrm{flipLeft}( v ) \cr
 /// 13 & \quad \textbf{return}\ q \cr
 /// 14 & \textbf{function}\ \mathrm{case3}( u ) \cr
 /// 15 & \quad \mathrm{pullBlack}( w );\ \mathrm{flipRight}( w );\ q \gets
 ///     w.\mathit{left} \cr
 /// 16 & \quad \textbf{if}\ q \text{ is red}\ \textbf{then}
 ///     \ \mathrm{rotateRight}( w );\ \mathrm{flipLeft}( v );
 ///     \ \mathrm{pushBlack}( q );\ \textbf{return}\ q \cr
 /// 17 & \quad \textbf{if}\ v.\mathit{left} \text{ is red}\ \textbf{then}
 ///     \ \mathrm{pushBlack}( v );\ \textbf{return}\ v \cr
 /// 18 & \quad \mathrm{flipLeft}( v );\ \textbf{return}\ w
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the height, \f$ -1 \f$ if empty, in \f$ O( n ) \f$
 [[nodiscard]] long height( void ) const;

 /// the black height, the black nodes on a path from the root to nil
 [[nodiscard]] long blackHeight( void ) const;

 /// the numbers of the nodes of degree 2, 3 and 4 of the 2-4 tree that the
 /// tree simulates (exercises 9.1 and 9.8 of Morin), in \f$ O( n ) \f$
 [[nodiscard]] std::array< std::size_t , 3 > twoFourDegrees( void ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet of Morin)
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

 /// the pairs of \f$ t \f$ appended and \f$ t \f$ left empty, in
 /// \f$ O( h + h_t ) \f$ time (exercises 9.10 and 9.11 of Morin)
 ///
 /// In pseudocode (exercises 9.10 and 9.11 of Morin, beyond the book),
 /// \f$ b \f$ the black heights, for \f$ b( T ) \ge b( t ) \f$ (the other
 /// case along the left spine of \f$ t \f$):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{absorb}( t ) \cr
 /// 2 & \quad m \gets \text{the smallest node of } t \text{, removed from }
 ///     t \cr
 /// 3 & \quad c \gets \text{the first black node of black height } b( t )
 ///     \text{ on the right spine of } T \cr
 /// 4 & \quad m \text{ red};\ m.\mathit{left} \gets c;\ m.\mathit{right}
 ///     \gets t.\mathit{root};\ m \text{ takes the place of } c \cr
 /// 5 & \quad \mathrm{addFixup}( m )
 /// \end{array}}
 /// \f]
 /// \pre the keys of \f$ t \f$ larger than those of this tree
 void absorb( LeftLeaningRedBlackTree & t );

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:
 static constexpr signed char kRed = 0;    ///< the red color
 static constexpr signed char kBlack = 1;  ///< the black color

 /// the links and the color of a node: 0 red, 1 black, 2 double black
 struct Link
 {
  Link * left = nullptr;     ///< the left child
  Link * right = nullptr;    ///< the right child
  Link * parent = nullptr;   ///< the parent
  signed char color = kRed;  ///< the color
  };

 /// a node, a link with a pair
 struct Node : Link
 {
  K key;    ///< the key
  V value;  ///< the value
  /// the red node of a pair
  Node( const K & k , const V & v ) : key( k ) , value( v ) { }
  };

 public:
/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the shape of the tree, for `PathIterator`
 struct Shape
 {
  /// the root of the tree `o`
  static const Node * root( const auto & o ) {
   return( static_cast< const Node * >( o.f_root ) );
   }
  /// the left child of the node `n`
  static const Node * left( const Node * n ) {
   return( static_cast< const Node * >( n->left ) );
   }
  /// the right child of the node `n`
  static const Node * right( const Node * n ) {
   return( static_cast< const Node * >( n->right ) );
   }
  /// the pair of the node `n`
  static auto read( const Node * n ) {
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
 using const_iterator = PathIterator< LeftLeaningRedBlackTree , Shape >;
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

 private:
 static signed char color( const Link * u ) {
  return( u ? u->color : kBlack );
  }
 static const K & key( const Link * u ) {
  return( static_cast< const Node * >( u )->key );
  }
 /// the child pointer of the parent of \f$ u \f$ (or the root) that holds
 /// \f$ u \f$
 Link *& place( Link * u ) {
  return( ! u->parent
	     ? f_root
	     : ( u->parent->left == u ? u->parent->left : u->parent->right ) );
  }
 void rotateLeft( Link * u );
 void rotateRight( Link * u );
 static void pushBlack( Link * u ) {
  --u->color;
  ++u->left->color;
  ++u->right->color;
  }
 static void pullBlack( Link * u ) {
  ++u->color;
  --u->left->color;
  --u->right->color;
  }
 void flipLeft( Link * u ) {
  std::swap( u->color , u->right->color );
  rotateLeft( u );
  }
 void flipRight( Link * u ) {
  std::swap( u->color , u->left->color );
  rotateRight( u );
  }
 void addFixup( Link * u );
 void removeFixup( Link * u );
 Link * removeCase2( Link * u );
 Link * removeCase3( Link * u );
 Link * findLast( const K & k ) const;

 /// the order, the parents and the three properties
 [[nodiscard]] bool valid( void ) const;


 Link * f_root = nullptr;  ///< the root
 std::size_t f_n = 0;      ///< the number of keys

 };  // end( class LeftLeaningRedBlackTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/impl/LeftLeaningRedBlackTree.tpp"

/*--------------------------------------------------------------------------*/

#endif /* LeftLeaningRedBlackTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File LeftLeaningRedBlackTree.h -------------------*/
/*--------------------------------------------------------------------------*/
