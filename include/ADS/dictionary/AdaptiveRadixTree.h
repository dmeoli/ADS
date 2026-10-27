/*--------------------------------------------------------------------------*/
/*------------------------ File AdaptiveRadixTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The adaptive radix tree of Leis, Kemper and Neumann on strings, beyond
 * the book: the engineered ordered DICTIONARY of ADS [see `ADT.h`] whose
 * keys are strings, the one that the benchmark of the tries compares with
 * the HAT-trie of tsl, with libart and with the B-trees of strings, while
 * `Trie` and `RadixTree` [see `Trie.h`] stay the tries of Fredkin and of
 * Morrison.
 *
 * A node at depth \f$ d \f$ stands for the prefixes of \f$ d \f$ bytes of its
 * keys and branches on their byte \f$ d \f$, and it adapts its layout to the
 * number of its children, so that a node is never much larger than its
 * pointers:
 *
 * - `Node4` and `Node16` keep up to 4 and 16 bytes sorted beside the
 *   pointers to the children of the same places; a `Node16` is searched by
 *   one comparison of vectors of SSE2 (or NEON), the 16 bytes against the
 *   byte of the key at once;
 * - `Node48` keeps an index of 256 bytes, the place of the child of each
 *   byte among its 48 pointers (0 for none);
 * - `Node256` keeps the 256 pointers, the byte of the key being the place.
 *
 * A node grows to the next layout when it is full and shrinks to the
 * previous one when it holds fewer than a quarter of its places (3 in a
 * `Node16`, 12 in a `Node48`, 37 in a `Node256`), with a slack against the
 * repeated changes of layout on an insertion and a removal. A chain of
 * nodes with a single child is compressed into the prefix of the first
 * node that branches (the path compression): the prefix is the length of
 * the chain and its first 8 bytes, a search skipping the bytes past 8
 * (the optimistic compression of the paper, the hybrid of its section
 * III.E) and comparing the whole key at the leaf, where every search ends.
 * A leaf holds the whole key, in one allocation with its value; the key
 * that ends at a node, a prefix of all the keys below it, hangs from the
 * node apart from the children, so that any string, null bytes included,
 * can be a key. A search, an insertion and a removal take \f$ O( k ) \f$
 * time for a key of \f$ k \f$ bytes, whatever the number of the keys, with
 * a branch of \f$ O( 1 ) \f$ per node; since every node but the root has at
 * least two things below it among its children and its own key, the nodes
 * are at most \f$ n - 1 \f$ for \f$ n \f$ keys, and the leaves \f$ n \f$,
 * \f$ \lceil ( 16 + k ) / 16 \rceil \f$ slots of 16 bytes for a key of
 * \f$ k \f$ bytes and a value of 8.
 *
 * A search descends with the byte of the key that follows each node, and
 * prefetches the lines of the next node that it reads for that byte
 * whatever the layout of the node (its header and the keys of a `Node16`,
 * the place of the byte in the index of a `Node48` or among the children of
 * a `Node256`) while the first line is on its way. An insertion first
 * descends as a search; if no prefix on the way was longer than its 8
 * bytes, the way is the one of the key, and the key goes where the search
 * stopped, in one pass; otherwise the search ends at a leaf \f$ l \f$ below
 * the last node that the key reaches, and a second pass goes down again to
 * the node or the leaf whose prefix parts from the key at the first byte
 * \f$ c \f$ where the key and \f$ l \f$ differ, the whole prefixes of the
 * nodes on the way being those of \f$ l \f$, with no search of a leaf below
 * each node.
 *
 * The iterators visit the pairs \f$ ( \mathit{key} , \mathit{value} ) \f$ by
 * increasing keys (a key before the longer ones that begin with it, the
 * children of a node by their bytes), as a `string_view` on the key of the
 * leaf and a constant reference to its value, with a stack of the nodes on
 * the way; `forEach()` and `forPrefix()` visit all the pairs, and those
 * whose key begins with a prefix, by a recursion with no stack, and `find()`
 * gives a value to change in place; the visits prefetch the children of a
 * `Node4` or of a `Node16`, and the iterators those of the next sibling
 * too, so that the leaves of a subtree are on their way while the one before
 * is visited. An insertion or a removal invalidates the iterators. The
 * internal nodes come from pools [see `ObjectPool.h`] on huge pages, the
 * leaves from slabs of 16 to 512 bytes in steps of 16, each size with the
 * list of its free slots, cut from blocks that double up to 2 MB (the
 * larger leaves come from the allocator), so that a leaf costs no call to
 * the allocator.
 *
 * The contracts [see `Contract.h`]: after `insert()` the key is there and
 * after `remove()` it is not; the invariant is that the keys of the leaves
 * extend the paths that lead to them, that every node but the root branches
 * or ends a key, that the bytes of a node are sorted and its layout the one
 * of its count, and that the leaves are `size()`, an \f$ O( n ) \f$ check at
 * the level 3 on the trees of up to 2048 keys. The operations are defined in
 * `AdaptiveRadixTree.tpp`.
 *
 * \see V. Leis, A. Kemper, T. Neumann, The adaptive radix tree: ARTful
 *      indexing for main-memory databases, Proceedings of ICDE 2013,
 *      38-49, 2013, doi:10.1109/ICDE.2013.6544812
 * \see J. Bonwick, The slab allocator: an object-caching kernel memory
 *      allocator, Proceedings of the USENIX Summer 1994 Technical
 *      Conference, 87-98, 1994
 * \see D. R. Morrison, PATRICIA: practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
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

#ifndef __ADS_AdaptiveRadixTree
 #define __ADS_AdaptiveRadixTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/ObjectPool.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS AdaptiveRadixTree -------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered map from strings to values of type `V` on an adaptive radix
/// tree
template< class V >
class AdaptiveRadixTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::string;   ///< the type of the keys
 using Value = V;           ///< the type of the values

 /// the empty tree
AdaptiveRadixTree( void ) = default;

 /// a copy of `other`
 AdaptiveRadixTree( const AdaptiveRadixTree & o );

 /// the items of `other`, which is left empty
 AdaptiveRadixTree( AdaptiveRadixTree && o ) noexcept;

 /// the assignment, by copy and swap
 AdaptiveRadixTree & operator=( AdaptiveRadixTree o ) noexcept;

 /// frees the memory of the structure
 ~AdaptiveRadixTree();

/*--------------------------------------------------------------------------*/

 /// the value of \f$ k \f$, if there
 ///
 /// In pseudocode (Leis, Kemper and Neumann, beyond the book), \f$ d \f$ the
 /// bytes of \f$ k \f$ consumed; a prefix is compared on its first 8 bytes
 /// only (the optimistic compression), the leaf on the whole key:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{search}( k ) \cr
 /// 2 & \quad p \gets \mathit{root};\ d \gets 0 \cr
 /// 3 & \quad \textbf{while}\ p \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ p \text{ is a leaf }\ \textbf{then return}\ p
 ///     \ \text{if its key is } k \text{, else } \textbf{nil} \cr
 /// 5 & \qquad \textbf{if}\ \text{the prefix of } p \text{ does not match } k[
 ///     d .. ]\ \textbf{then return nil} \cr
 /// 6 & \qquad d \gets d + \text{the length of the prefix} \cr
 /// 7 & \qquad \textbf{if}\ d = |k|\ \textbf{then return}
 ///     \ \text{the key that } \text{ends at } p \text{, if it is } k \cr
 /// 8 & \qquad p \gets \text{the child of } p \text{ by } k[ d ];\ d \gets d +
 ///     1\quad \triangleright\ \text{by the layout of } p
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( std::string_view k ) const;

 /// the value of \f$ k \f$, or nil, by the search of `lookup()`
 [[nodiscard]] const V * find( std::string_view k ) const;

 /// the value of \f$ k \f$, to change in place, or nil
 [[nodiscard]] V * find( std::string_view k );

 /// gives \f$ k \f$ the value \f$ v \f$; true if \f$ k \f$ was not there
 ///
 /// In pseudocode (Leis, Kemper and Neumann, beyond the book), the two
 /// passes of the file; a node full grows to the next layout:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \text{descend as } \mathrm{search}( k ) \text{, to where it }
 ///     \text{stops} \cr
 /// 3 & \quad \textbf{if}\ \text{no prefix on the way was longer than 8 }
 ///     \text{bytes}\ \textbf{then} \cr
 /// 4 & \qquad \text{put } k \text{ there: a child, the key of a node, or a }
 ///     \text{new node splitting a leaf or a prefix} \cr
 /// 5 & \quad \textbf{else} \cr
 /// 6 & \qquad l \gets \text{a leaf below the last node reached};\ c \gets
 ///     \text{the bytes that } k \text{ shares with } l \cr
 /// 7 & \qquad \text{descend again to the node or the leaf that parts at } c
 ///     \text{, and put } k \text{ there} \cr
 /// 8 & \quad \textbf{return}\ \text{whether } k \text{ is new}
 /// \end{array}}
 /// \f]
 bool insert( std::string_view k , const V & v );

 /// removes \f$ k \f$, shrinking its node to the previous layout below a
 /// quarter of its places and merging a node left with a single child into
 /// its prefix; true if it was there
 bool remove( std::string_view k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( std::string_view k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// removes all the keys
 void clear( void );

 /// exchanges the contents with `other` in \f$ O( 1 ) \f$
 void swap( AdaptiveRadixTree & o ) noexcept;

 /// `f( key , value )` on each pair, by increasing keys
 template< class F >
 void forEach( F && f ) const;

 /// `f( key , value )` on each pair whose key begins with \f$ p \f$, by
 /// increasing keys
 template< class F >
 void forPrefix( std::string_view p , F && f ) const;

 /// the keys that begin with \f$ p \f$, in increasing order
 [[nodiscard]] std::vector< std::string > keysWithPrefix(
                                                  std::string_view p ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr unsigned MAXP = 8;   ///< the bytes of a prefix kept

 /// the layout of a node
enum Type : std::uint8_t { N4 , N16 , N48 , N256 };

 /// a leaf: the value, and the key after it in the same allocation
 struct Leaf {
  V value;                              ///< the value
  std::uint32_t len;                    ///< the length of the key
  /// the key, after the leaf
  [[nodiscard]] const char * key( void ) const {
   return( reinterpret_cast< const char * >( this + 1 ) );
   }
  /// the key as a view
 [[nodiscard]] std::string_view view( void ) const {
   return( std::string_view( key() , len ) );
   }
  };

 /// what all the internal nodes begin with
 struct Node {
  Type type;                            ///< the layout
  std::uint16_t count = 0;              ///< the children
  std::uint32_t plen = 0;               ///< the length of the prefix
  unsigned char prefix[ MAXP ];         ///< its first `MAXP` bytes
  Leaf * term = nullptr;                ///< the key that ends here
  /// a node of the layout \f$ t \f$
 explicit Node( Type t ) : type( t ) {}
  };

 /// a node of up to 4 children, by sorted bytes
 struct Node4 : Node {
  unsigned char key[ 4 ];               ///< the bytes, sorted
  void * child[ 4 ];                    ///< the children of the bytes
  /// the empty node
  Node4( void ) : Node( N4 ) {}
  };

 /// a node of up to 16 children, by sorted bytes compared at once
 struct Node16 : Node {
  alignas( 16 ) unsigned char key[ 16 ];   ///< the bytes, sorted
  void * child[ 16 ];                   ///< the children of the bytes
  /// the empty node
  Node16( void ) : Node( N16 ) {}
  };

 /// a node of up to 48 children, by an index of the 256 bytes
 struct Node48 : Node {
  unsigned char index[ 256 ];           ///< place + 1 of a byte, or 0
  void * child[ 48 ];                   ///< the children
  /// the empty node
  Node48( void ) : Node( N48 ) { std::memset( index , 0 , 256 ); }
  };

 /// a node of up to 256 children, the byte being the place
 struct Node256 : Node {
  void * child[ 256 ];                  ///< the child of each byte
  /// the empty node
  Node256( void ) : Node( N256 ) {
   for( auto & c : child ) c = nullptr;
   }
  };

 /// a pointer to a child is a leaf if its lowest bit is set
 [[nodiscard]] static bool isLeaf( const void * p ) {
  return( reinterpret_cast< std::uintptr_t >( p ) & 1 );
  }
 /// the leaf of a tagged pointer
[[nodiscard]] static Leaf * asLeaf( const void * p ) {
  return( reinterpret_cast< Leaf * >( reinterpret_cast< std::uintptr_t >(
                                       p ) - 1 ) );
  }
 /// the tagged pointer of a leaf, its lowest bit set
[[nodiscard]] static void * tag( Leaf * l ) {
  return( reinterpret_cast< void * >( reinterpret_cast< std::uintptr_t >(
                                       l ) + 1 ) );
  }

 /// the memory of the leaves: slots of 16 to 512 bytes in steps of 16, a
 /// list of the free slots for each size, cut from blocks that double up
 /// to 2 MB (on huge pages [see HugePageAllocator.h]); larger leaves come
 /// from the allocator
 class Slab {
  public:
  /// no block
  Slab( void ) = default;
  /// no copy, the slots being in use
  Slab( const Slab & ) = delete;
  /// no copy, the slots being in use
  Slab & operator=( const Slab & ) = delete;
  /// frees the blocks
  ~Slab() { release(); }
  /// a slot of at least `bytes` bytes, from the list of its size or from
  /// the current block
  [[nodiscard]] void * take( std::size_t bytes );
  /// gives back the slot \f$ p \f$ of `bytes` bytes to the list of its size
  void give( void * p , std::size_t bytes ) noexcept;
  /// frees all the blocks
  void release( void ) noexcept;
  /// exchanges the contents with `other` in \f$ O( 1 ) \f$
  void swap( Slab & o ) noexcept;
  private:
  /// the step of the sizes and their number
  static constexpr std::size_t STEP = 16 , CLASSES = 32;
  /// a free slot, linked to the next one of its size
  struct Free {
   Free * next;                         ///< the next free slot of its size
   };
  /// the blocks and their sizes
  std::vector< std::pair< char * , std::size_t > > f_blocks;
  char * f_cur = nullptr;               ///< the free part of the block
  char * f_end = nullptr;               ///< the end of the block
  Free * f_free[ CLASSES + 1 ] = {};    ///< the free slots of each size
  };

 [[nodiscard]] Leaf * newLeaf( std::string_view k , const V & v );

 void freeLeaf( Leaf * l ) noexcept;

 template< class N >
 [[nodiscard]] N * make( void );

 void freeNode( Node * n ) noexcept;

 [[nodiscard]] static void * const * child( const Node * n ,
                                            unsigned char c );

 [[nodiscard]] static void * * child( Node * n , unsigned char c );

 [[nodiscard]] static const Leaf * minimum( const void * p );

 [[nodiscard]] static std::uint32_t mismatch( const Node * n ,
                                              std::string_view k ,
                                              std::size_t d );

 void addChild( void * * ref , Node * n , unsigned char c , void * x );

 /// the leaf at `*ref`, on the way of \f$ k \f$ down to depth \f$ d \f$, and
 /// a new leaf of \f$ k \f$ under a new node where their keys part
 void split( void * * ref , std::size_t d , std::string_view k ,
             const V & v );

 void removeChild( void * * ref , Node * n , unsigned char c ,
                   void * * slot );

 void shrink( void * * ref , Node * n );

 [[nodiscard]] const Leaf * search( std::string_view k ) const;

 /// prefetches the lines of \f$ p \f$ that a search of the byte \f$ c \f$
 /// reads, whatever the layout of \f$ p \f$, while its first line is on its
 /// way
 static void ahead( const void * p , unsigned char c );

 /// prefetches the children of a `Node4` or of a `Node16`, which a visit
/// reads
 /// next
 static void children( const Node * n );

 void destroy( void * p ) noexcept;

 template< class F >
 static void visit( const void * p , F & f );

 template< class F >
 static void each( const Node * n , F && f );

 [[nodiscard]] bool valid( void ) const;

 [[nodiscard]] bool valid( const void * p , std::string & path ,
                           std::size_t & leaves , bool root ) const;

 void * f_root = nullptr;               ///< a node or a tagged leaf
 std::size_t f_size = 0;                ///< the keys
 ObjectPool< Node4 > f_n4;              ///< the slots of the nodes
 ObjectPool< Node16 > f_n16;
 ObjectPool< Node48 > f_n48;
 ObjectPool< Node256 > f_n256;
 Slab f_slab;                           ///< the memory of the leaves

/*------------------------- THE STANDARD INTERFACE -------------------------*/
 public:

 // a constant view of the pairs ( key , value ) by increasing keys,
 // forward, a stack of the nodes on the way to a leaf

 class const_iterator;

 /// the type of the items, for the standard library
 using value_type = std::pair< std::string_view , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = value_type;
 /// a constant reference to an item
 using const_reference = value_type;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const;
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const;
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the iterator on the pairs by increasing keys, by a stack of the nodes
 /// on the way to a leaf
 class const_iterator {
  public:
  /// the type of the items, for the standard library
  using value_type = std::pair< std::string_view , const V & >;
  /// a reference to an item
  using reference = value_type;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// the concept of the iterator, for C++20
  using iterator_concept = std::forward_iterator_tag;
  /// the category of the iterator
  using iterator_category = std::input_iterator_tag;

  /// the end
  const_iterator( void ) = default;

  /// the key of the leaf and its value
  reference operator*( void ) const {
   return( reference( f_leaf->view() , f_leaf->value ) );
   }

  /// to the next leaf, by the walk of the stack
  const_iterator & operator++( void ) {
   f_leaf = step( f_S );
   return( *this );
   }
  /// to the next leaf, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same leaf
  bool operator==( const const_iterator & o ) const {
   return( f_leaf == o.f_leaf );
   }

  private:
  friend class AdaptiveRadixTree;

  /// a node on the way and the next place to look at, -1 for its term
  struct Frame {
   const Node * node;                   ///< the node
   int next;                            ///< the next place
   };

  /// on the first leaf below `root`
 explicit const_iterator( const void * root ) {
   if( ! root )
    return;
   if( isLeaf( root ) ) {
    f_leaf = asLeaf( root );
    return;
    }
   f_S.push_back( Frame{ static_cast< const Node * >( root ) , -1 } );
   f_leaf = step( f_S );
   }

  /// the next leaf of the walk of the stack \f$ S \f$, nil at its end
  static const Leaf * step( std::vector< Frame > & S );

  std::vector< Frame > f_S;           ///< the nodes on the way
  const Leaf * f_leaf = nullptr;      ///< the current leaf, or nil

 };  // end( class const_iterator )

 };  // end( class AdaptiveRadixTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/AdaptiveRadixTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AdaptiveRadixTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File AdaptiveRadixTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
