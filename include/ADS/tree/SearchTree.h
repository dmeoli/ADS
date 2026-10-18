/*--------------------------------------------------------------------------*/
/*--------------------------- File SearchTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search tree (ABR) and the Red-Black tree of the book, as a
 * DICTIONARY [see ADT.h] from keys to values.
 *
 * In a binary search tree the keys in the left subtree of a node u are
 * smaller than the key of u, and those in the right subtree larger, so that
 * the invisit examines the keys in increasing order; lookupNode(),
 * insertNode() and removeNode() go down one path and take O( h ) time, h
 * being the height of the tree, which is O( n ) in the worst case.
 *
 * A Red-Black tree is a binary search tree whose nodes are red or black,
 * with the constraints (V1) the root is black, (V2) the leaves are black,
 * (V3) both children of a red node are black, (V4) all the paths from a node
 * to the leaves below it have the same number of black nodes, its black
 * height b( u ). The subtree of u has at least \f$ 2^{b(u)} - 1 \f$
 * internal nodes (Teorema 6.1), no path from the root to a leaf is longer
 * than twice another one (Teorema 6.2), and the height is
 * \f[
 *   h \leq 2 \log_2( n + 1 )
 * \f]
 * (Teorema 6.3). The procedures are those of the binary search tree, plus
 * balanceInsert() after an insertion and balanceDelete() after the removal
 * of a black node, which restore the constraints with O( log n ) changes of
 * color and at most 3 rotations, in O( log n ) time.
 *
 * The two trees are one class, SearchTree< K , V , RB >, the procedures of
 * the book written once and the two balancing calls under if constexpr( RB )
 * where the book "highlights them in black"; BinarySearchTree< K , V > and
 * RedBlackTree< K , V > are its two instances. The leaves are one sentinel
 * node of the tree, black, which the book allows and which balanceDelete()
 * needs: when the removed node had no children, the leaf t that replaces it
 * must know its parent, which link() writes in the sentinel. Three points of
 * the pseudocode are mended, each marked in the code: balanceDelete() stops
 * when t is the root rather than when t is the old root T, which a rotation
 * of the case (1) may have moved down; the case (4) climbs to the root
 * instead of reading the old T; and the Red-Black insertNode() returns the
 * root also when the key was there already.
 *
 * The contracts [see Contract.h]: after insert() the key is there and
 * after remove() it is not; key(), value(), successorNode() and
 * predecessorNode() need a node; the invariant is the order of the keys,
 * the parents and, for a Red-Black tree, the constraints (V1)-(V4) [see
 * valid()]. The operations are defined in SearchTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 6.1 and 6.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
 * \see R. Bayer, Symmetric binary B-trees: data structure and maintenance
 *      algorithms, Acta Informatica 1(4), 290-306, 1972,
 *      doi:10.1007/BF00289509
 * \see L. J. Guibas, R. Sedgewick, A dichromatic framework for balanced
 *      trees, Proceedings of the 19th FOCS, 8-21, 1978,
 *      doi:10.1109/SFCS.1978.3
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_SearchTree
 #define __ADS_SearchTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SearchTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the color of a node of a Red-Black tree
enum class Color : unsigned char { RED , BLACK };

/// a binary search tree, balanced as a Red-Black tree if RB
template< std::totally_ordered K , class V , bool RB >
class SearchTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the links of a node; the sentinel leaf is a Link and nothing more
 struct Link {
  Link * parent;                ///< the parent
  Link * left;                  ///< the left child
  Link * right;                 ///< the right child
  Color color = Color::BLACK;   ///< the color, read only if RB
  };

 /// a node of the tree, with its key and its value
 struct Node : Link {
  K key;     ///< the key
  V value;   ///< the value
  };

 /// an empty tree
 SearchTree( void );

 /// a copy, node by node with the same shape, in O( n ) time
 SearchTree( const SearchTree & o );

 SearchTree( SearchTree && o ) noexcept;

 SearchTree & operator=( SearchTree o ) noexcept;

 ~SearchTree();

 void swap( SearchTree & o ) noexcept;

/*--------------------------------------------------------------------------*/
/*------------------------- DICTIONARY OPERATIONS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value v to the key k
 void insert( const K & k , const V & v );

 /// removes the key k, if present
 void remove( const K & k );

 /// true if the key k is present
 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 /// removes all the nodes
 void clear( void );

/*--------------------------------------------------------------------------*/
/*---------------------------- NAVIGATION ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** The nodes can be walked as in the book: from root(), along the fields
 * parent, left and right of a Link, until the leaf, isNil(). They must not
 * be changed. */

 [[nodiscard]] Link * root( void ) const;

 /// true if u is the sentinel leaf (or the parent of the root)
 [[nodiscard]] bool isNil( const Link * u ) const;

 /// the key of the node u
 /// \pre u is a node, not the sentinel leaf, which has no key (only nil
 ///      is checked, as a static function cannot know the sentinel)
 [[nodiscard]] static const K & key( const Link * u );

 /// the value of the node u
 /// \pre u is a node, not the sentinel leaf
 [[nodiscard]] static const V & value( const Link * u );

 /// the node of the key x, nil if absent
 [[nodiscard]] Link * lookupNode( const K & x ) const;

 /// the node of the smallest key, nil if the tree is empty
 [[nodiscard]] Link * min( void ) const;

 /// the node of the largest key, nil if the tree is empty
 [[nodiscard]] Link * max( void ) const;

 /// successorNode(): the node of the next key, nil after the largest one
 /// and from nil
 /// \pre t is not a null pointer
 [[nodiscard]] Link * successorNode( Link * t ) const;

 /// the foreach of the book: f( k , v ) on the pairs in increasing order of
 /// the keys, O( n ) time
 template< class F >
 void forEach( F && f ) const;

 /// predecessorNode(): the node of the previous key, nil before the first
 /// one and from nil
 /// \pre t is not a null pointer
 [[nodiscard]] Link * predecessorNode( Link * t ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional by the links to the parents, the sentinel leaf being the
 // end

 class const_iterator {
  public:
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;
  const_iterator( const SearchTree * t , Link * x ) : f_t( t ) , f_x( x ) {}

  reference operator*( void ) const {
   return( reference( f_t->key( f_x ) , f_t->value( f_x ) ) );
   }
  const_iterator & operator++( void ) {
   f_x = f_t->successorNode( f_x );
   return( *this );
   }
  const_iterator & operator--( void ) {
   f_x = f_t->isNil( f_x ) ? f_t->max() : f_t->predecessorNode( f_x );
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }
  bool operator==( const const_iterator & o ) const {
   return( f_x == o.f_x );
   }

  /// the node, the sentinel at the end
  [[nodiscard]] Link * node( void ) const { return( f_x ); }

  private:
  const SearchTree * f_t = nullptr;
  Link * f_x = nullptr;
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , min() ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_nil.get() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the order of the keys, the parents, the number of nodes and, if RB,
 /// the constraints (V1)-(V4), O( n )
 [[nodiscard]] bool valid( void ) const;

 [[nodiscard]] Link * nil( void ) const;

 void clear_nil( void );

 static K & key_ref( Link * u );

 static V & value_ref( Link * u );

 Link * new_node( const K & x , const V & v );

 /// the copy of the subtree u of o, hung under p; iterative
 Link * copy( const SearchTree & o , Link * u , Link * p );

/*--------------------------------------------------------------------------*/
/*------------------- THE PROCEDURES OF SECTION 6.1 ------------------------*/
/*--------------------------------------------------------------------------*/

 Link * lookupNode( Link * T , const K & x ) const;

 Link * min( Link * T ) const;

 Link * max( Link * T ) const;

 /// link(): u becomes the child of p on the side of x; the sentinel takes
 /// the parent too, as balanceDelete() reads it
 void link( Link * p , Link * u , const K & x );

 Link * insertNode( Link * T , const K & x , const V & v );

 Link * removeNode( Link * T , const K & x0 );

/*--------------------------------------------------------------------------*/
/*------------------- THE PROCEDURES OF SECTION 6.2 ------------------------*/
/*--------------------------------------------------------------------------*/

 /// rotateLeft(): the right child y of x goes up, x down (Figura 6.4)
 Link * rotateLeft( Link * x );

 /// rotateRight(): rotateLeft() with left and right swapped
 Link * rotateRight( Link * x );

 static constexpr Color RED = Color::RED;
 static constexpr Color BLACK = Color::BLACK;

 void balanceInsert( Link * t );

 /// balanceDelete(): the cases (1) to (4) with t a left child, and their
 /// mirrors (5) to (8), the same code with left and right swapped
 void balanceDelete( Link * t );

/*--------------------------------------------------------------------------*/

 std::unique_ptr< Link > f_nil;   ///< the sentinel leaf
 Link * f_root = f_nil.get();     ///< the root, the sentinel if empty
 std::size_t f_size = 0;          ///< the number of keys

 };  // end( class SearchTree )

/*--------------------------------------------------------------------------*/

/// the binary search tree (ABR) of section 6.1
template< std::totally_ordered K , class V >
using BinarySearchTree = SearchTree< K , V , false >;

/// the Red-Black tree of section 6.2
template< std::totally_ordered K , class V >
using RedBlackTree = SearchTree< K , V , true >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/SearchTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SearchTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SearchTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
