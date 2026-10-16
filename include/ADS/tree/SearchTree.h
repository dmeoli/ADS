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
 SearchTree( void ) : f_nil( new Link ) { clear_nil(); }

 /// a copy, node by node with the same shape, in O( n ) time
 SearchTree( const SearchTree & o ) : SearchTree() {
  f_root = copy( o , o.f_root , f_nil.get() );
  f_size = o.f_size;
  }

 SearchTree( SearchTree && o ) noexcept : SearchTree() { swap( o ); }

 SearchTree & operator=( SearchTree o ) noexcept {
  swap( o );
  return( *this );
  }

 ~SearchTree() { clear(); }

 void swap( SearchTree & o ) noexcept {
  std::swap( f_nil , o.f_nil );
  std::swap( f_root , o.f_root );
  std::swap( f_size , o.f_size );
  }

/*--------------------------------------------------------------------------*/
/*------------------------- DICTIONARY OPERATIONS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const {
  Link * u = lookupNode( f_root , k );
  if( u == nil() )
   return( std::nullopt );
  return( value( u ) );
  }

 /// associates the value v to the key k
 void insert( const K & k , const V & v ) { f_root = insertNode( f_root , k ,
                                                                 v ); }

 /// removes the key k, if present
 void remove( const K & k ) { f_root = removeNode( f_root , k ); }

 /// true if the key k is present
 [[nodiscard]] bool contains( const K & k ) const {
  return( lookupNode( f_root , k ) != nil() );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// removes all the nodes
 void clear( void ) {
  std::vector< Link * > S;   // iterative, as the height may be n
  if( f_root != nil() )
   S.push_back( f_root );
  while( ! S.empty() ) {
   Link * u = S.back();
   S.pop_back();
   if( u->left != nil() ) S.push_back( u->left );
   if( u->right != nil() ) S.push_back( u->right );
   delete static_cast< Node * >( u );
   }
  f_root = nil();
  f_size = 0;
  clear_nil();
  }

/*--------------------------------------------------------------------------*/
/*---------------------------- NAVIGATION ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** The nodes can be walked as in the book: from root(), along the fields
 * parent, left and right of a Link, until the leaf, isNil(). They must not
 * be changed. */

 [[nodiscard]] Link * root( void ) const { return( f_root ); }

 /// true if u is the sentinel leaf (or the parent of the root)
 [[nodiscard]] bool isNil( const Link * u ) const { return( u == nil() ); }

 [[nodiscard]] static const K & key( const Link * u ) {
  return( static_cast< const Node * >( u )->key );
  }

 [[nodiscard]] static const V & value( const Link * u ) {
  return( static_cast< const Node * >( u )->value );
  }

 /// the node of the key x, nil if absent
 [[nodiscard]] Link * lookupNode( const K & x ) const {
  return( lookupNode( f_root , x ) );
  }

 /// the node of the smallest key, nil if the tree is empty
 [[nodiscard]] Link * min( void ) const {
  return( f_root == nil() ? nil() : min( f_root ) );
  }

 /// the node of the largest key, nil if the tree is empty
 [[nodiscard]] Link * max( void ) const {
  return( f_root == nil() ? nil() : max( f_root ) );
  }

 /// successorNode(): the node of the next key, nil after the largest one
 [[nodiscard]] Link * successorNode( Link * t ) const {
  if( t == nil() )
   return( t );
  if( t->right != nil() )
   return( min( t->right ) );
  Link * p = t->parent;
  while( ( p != nil() ) && ( t == p->right ) ) {
   t = p;
   p = p->parent;
   }
  return( p );
  }

 /// the foreach of the book: f( k , v ) on the pairs in increasing order of
 /// the keys, O( n ) time
 template< class F >
 void forEach( F && f ) const {
  for( Link * t = min() ; t != nil() ; t = successorNode( t ) )
   f( key( t ) , value( t ) );
  }

 /// predecessorNode(): the node of the previous key, nil before the first
 [[nodiscard]] Link * predecessorNode( Link * t ) const {
  if( t == nil() )
   return( t );
  if( t->left != nil() )
   return( max( t->left ) );
  Link * p = t->parent;
  while( ( p != nil() ) && ( t == p->left ) ) {
   t = p;
   p = p->parent;
   }
  return( p );
  }

/*--------------------------------------------------------------------------*/
 private:

 [[nodiscard]] Link * nil( void ) const { return( f_nil.get() ); }

 void clear_nil( void ) {
  f_nil->parent = f_nil->left = f_nil->right = f_nil.get();
  f_nil->color = Color::BLACK;
  }

 static K & key_ref( Link * u ) {
  return( static_cast< Node * >( u )->key );
  }

 static V & value_ref( Link * u ) {
  return( static_cast< Node * >( u )->value );
  }

 Link * new_node( const K & x , const V & v ) {
  ++f_size;
  return( new Node{ { nil() , nil() , nil() , Color::BLACK } , x , v } );
  }

 /// the copy of the subtree u of o, hung under p; iterative
 Link * copy( const SearchTree & o , Link * u , Link * p ) {
  if( u == o.nil() )
   return( nil() );
  struct Item { Link * from; Link * parent; Link ** slot; };
  Link * r = nil();
  std::vector< Item > S = { { u , p , &r } };
  while( ! S.empty() ) {
   const Item it = S.back();
   S.pop_back();
   Link * c = new Node{ { it.parent , nil() , nil() , it.from->color } ,
                        key( it.from ) , value( it.from ) };
   *it.slot = c;
   if( it.from->left != o.nil() ) S.push_back( { it.from->left , c ,
                                                 &c->left } );
   if( it.from->right != o.nil() ) S.push_back( { it.from->right , c ,
                                                  &c->right } );
   }
  return( r );
  }

/*--------------------------------------------------------------------------*/
/*------------------- THE PROCEDURES OF SECTION 6.1 ------------------------*/
/*--------------------------------------------------------------------------*/

 Link * lookupNode( Link * T , const K & x ) const {
  while( ( T != nil() ) && ( key( T ) != x ) )
   T = x < key( T ) ? T->left : T->right;
  return( T );
  }

 Link * min( Link * T ) const {
  while( T->left != nil() )
   T = T->left;
  return( T );
  }

 Link * max( Link * T ) const {
  while( T->right != nil() )
   T = T->right;
  return( T );
  }

 /// link(): u becomes the child of p on the side of x; the sentinel takes
 /// the parent too, as balanceDelete() reads it
 void link( Link * p , Link * u , const K & x ) {
  u->parent = p;
  if( p != nil() ) {
   if( x < key( p ) ) p->left = u;
   else p->right = u;
   }
  }

 Link * insertNode( Link * T , const K & x , const V & v ) {
  Link * p = nil();                              // parent
  Link * u = T;
  while( ( u != nil() ) && ( key( u ) != x ) ) {  // finds where to insert
   p = u;
   u = x < key( u ) ? u->left : u->right;
   }
  if( u != nil() ) {                             // the key is there
   value_ref( u ) = v;
   return( T );                                  // mended: also if RB
   }
  Link * n = new_node( x , v );
  link( p , n , x );
  if constexpr( RB ) {
   balanceInsert( n );
   while( n->parent != nil() )
    n = n->parent;
   return( n );                                  // the current root
   }
  else
   return( p == nil() ? n : T );                 // the first node
  }

 Link * removeNode( Link * T , const K & x0 ) {
  Link * u = lookupNode( T , x0 );
  if( u == nil() )
   return( T );
  const K * x = &x0;
  if( ( u->left != nil() ) && ( u->right != nil() ) ) {   // case (3)
   Link * s = u->right;
   while( s->left != nil() )
    s = s->left;
   key_ref( u ) = key( s );
   value_ref( u ) = value( s );
   x = &key( s );
   u = s;
   }
  Link * t;
  if( ( u->left != nil() ) && ( u->right == nil() ) )     // case (2), left
   t = u->left;
  else                                                    // (2) right, (1)
   t = u->right;
  link( u->parent , t , *x );
  if constexpr( RB )
   if( u->color == Color::BLACK )
    balanceDelete( t );
  if( u->parent == nil() )
   T = t;
  delete static_cast< Node * >( u );
  --f_size;
  if constexpr( RB )
   if( T != nil() )
    while( T->parent != nil() )
     T = T->parent;
  if( T == nil() )
   clear_nil();
  return( T );
  }

/*--------------------------------------------------------------------------*/
/*------------------- THE PROCEDURES OF SECTION 6.2 ------------------------*/
/*--------------------------------------------------------------------------*/

 /// rotateLeft(): the right child y of x goes up, x down (Figura 6.4)
 Link * rotateLeft( Link * x ) {
  Link * y = x->right;
  Link * p = x->parent;
  x->right = y->left;                            // (1) B right of x
  if( y->left != nil() ) y->left->parent = x;
  y->left = x;                                   // (2) x left of y
  x->parent = y;
  y->parent = p;                                 // (3) y child of p
  if( p != nil() ) {
   if( p->left == x ) p->left = y;
   else p->right = y;
   }
  return( y );
  }

 /// rotateRight(): rotateLeft() with left and right swapped
 Link * rotateRight( Link * x ) {
  Link * y = x->left;
  Link * p = x->parent;
  x->left = y->right;
  if( y->right != nil() ) y->right->parent = x;
  y->right = x;
  x->parent = y;
  y->parent = p;
  if( p != nil() ) {
   if( p->right == x ) p->right = y;
   else p->left = y;
   }
  return( y );
  }

 static constexpr Color RED = Color::RED;
 static constexpr Color BLACK = Color::BLACK;

 void balanceInsert( Link * t ) {
  t->color = RED;
  while( t != nil() ) {
   Link * p = t->parent;                                  // parent
   Link * n = p != nil() ? p->parent : nil();             // grandparent
   Link * z = n == nil() ? nil() : ( n->left == p ? n->right : n->left );
   if( p == nil() ) {                                     // case (1)
    t->color = BLACK;
    t = nil();
    }
   else if( p->color == BLACK )                           // case (2)
    t = nil();
   else if( z->color == RED ) {                           // case (3)
    p->color = z->color = BLACK;
    n->color = RED;
    t = n;
    }
   else if( ( t == p->right ) && ( p == n->left ) ) {     // case (4a)
    rotateLeft( p );
    t = p;
    }
   else if( ( t == p->left ) && ( p == n->right ) ) {     // case (4b)
    rotateRight( p );
    t = p;
    }
   else {
    if( ( t == p->left ) && ( p == n->left ) )           // case (5a)
     rotateRight( n );
    else                                                 // case (5b)
     rotateLeft( n );
    p->color = BLACK;
    n->color = RED;
    t = nil();
    }
   }
  }

 /// balanceDelete(): the cases (1) to (4) with t a left child, and their
 /// mirrors (5) to (8), the same code with left and right swapped
 void balanceDelete( Link * t ) {
  while( ( t->parent != nil() ) && ( t->color == BLACK ) ) {  // mended
   Link * p = t->parent;                                       // parent
   const bool L = ( t == p->left );
   Link * Link::* near = L ? &Link::left : &Link::right;
   Link * Link::* far = L ? &Link::right : &Link::left;
   auto rot_near = [ & ]( Link * x ) {    // rotateLeft() if t is left
    return( L ? rotateLeft( x ) : rotateRight( x ) ); };
   auto rot_far = [ & ]( Link * x ) {
    return( L ? rotateRight( x ) : rotateLeft( x ) ); };
   Link * f = p->*far;                                         // sibling
   Link * ns = f->*near;                                       // nephews
   Link * nd = f->*far;
   if( f->color == RED ) {                                     // (1)
    p->color = RED;
    f->color = BLACK;
    rot_near( p );         // t unchanged, then (2), (3) or (4)
    }
   else if( ( ns->color == BLACK ) && ( nd->color == BLACK ) ) {  // (2)
    f->color = RED;
    t = p;
    }
   else if( ( ns->color == RED ) && ( nd->color == BLACK ) ) {    // (3)
    ns->color = BLACK;
    f->color = RED;
    rot_far( f );          // t unchanged, then (4)
    }
   else {                                                      // (4)
    f->color = p->color;
    p->color = BLACK;
    nd->color = BLACK;
    rot_near( p );
    t = f;                 // mended: t = T, the current root
    while( t->parent != nil() )
     t = t->parent;
    }
   }
  t->color = BLACK;        // on the sentinel too, which stays black
  }

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

#endif  /* SearchTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SearchTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
