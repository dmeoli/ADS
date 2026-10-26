/*--------------------------------------------------------------------------*/
/*----------------------------- File AVLTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The AVL tree of Adelson-Velsky and Landis, beyond the book [see
 * SearchTree.h for the Red-Black tree of chapter 6]: a binary search tree
 * whose every node has subtrees of heights differing by at most 1, so
 * that its height is below \f$ 1.4405 \log_2 ( n + 2 ) \f$, a little more
 * balanced than a Red-Black tree (whose bound is \f$ 2 \log_2 ( n + 1 )
 * \f$), at the price of more rotations on the updates. Each node keeps its
 * height; an insertion or a removal goes down recursively and, on the way
 * back, restores the balance of each node by a single or a double
 * rotation, in \f$ O( \log n ) \f$ time. It is a DICTIONARY [see ADT.h]
 * with lookup(), insert() and remove(), and forEach() visits the pairs by
 * increasing keys.
 *
 * Each node keeps also the size of its subtree, so that the AVL tree has
 * the operations based on join [see JoinAlgorithms.h]: split(), join(),
 * join2(), extractRange(), the union, the intersection and the difference
 * with another tree, the batches of insertions and of removals, filter()
 * and mapReduce(), sequential or on the threads of an executor; its join
 * goes down the spine of the higher tree to a subtree of the height of the
 * other, in \f$ O( | h_l - h_r | + 1 ) \f$ time.
 *
 * The contracts [see Contract.h]: insert() ensures that the key is there
 * and remove() that it is not; the invariant is the order of the keys, the
 * heights and the sizes stored and the balance of every node, an
 * \f$ O( n ) \f$ check at the level 3. The operations are defined in
 * AVLTree.tpp.
 *
 * \see G. M. Adelson-Velsky, E. M. Landis, An algorithm for the
 *      organization of information, Soviet Mathematics Doklady 3,
 *      1259-1263, 1962
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.3
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Just join for parallel
 *      ordered sets, Proceedings of the 28th ACM Symposium on Parallelism
 *      in Algorithms and Architectures (SPAA), 253-264, 2016,
 *      doi:10.1145/2935764.2935768
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

#ifndef __ADS_AVLTree
 #define __ADS_AVLTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/tree/JoinAlgorithms.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS AVLTree -------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V >
class AVLTree;

/// the adaptor of the nodes of an AVLTree [see JoinAdaptor]
template< std::totally_ordered K , class V >
struct AVLTreeNodes;

/// a DICTIONARY on an AVL tree, with the operations based on join [see
/// Joinable]
template< std::totally_ordered K , class V >
class AVLTree
 : public Joinable< AVLTree< K , V > , AVLTreeNodes< K , V > , K , V > {

 friend struct AVLTreeNodes< K , V >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the value of k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates v to k
 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// removes k, if it is there
 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the height of the tree, -1 if it is empty
 [[nodiscard]] int height( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  K key;
  V value;
  int h = 0;                     ///< the height of the subtree
  std::size_t s = 1;             ///< the nodes of the subtree
  std::unique_ptr< Node > left , right;
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, along the path from the root (the nodes have no parent)

 /// the shape of the tree, for PathIterator
 struct Shape {
  static auto root( const auto & o ) { return( o.f_root.get() ); }
  static auto left( const auto * n ) { return( n->left.get() ); }
  static auto right( const auto * n ) { return( n->right.get() ); }
  static auto read( const auto * n ) {
   return( std::pair< const K & , const V & >( n->key , n->value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using const_iterator = PathIterator< AVLTree , Shape >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:
 using Ptr = std::unique_ptr< Node >;

 static int h( const Ptr & t ) { return( t ? t->h : -1 ); }
 static std::size_t sz( const Ptr & t ) { return( t ? t->s : 0 ); }
 static void update( Ptr & t );
 static void rotateRight( Ptr & t );
 static void rotateLeft( Ptr & t );
 static void balance( Ptr & t );
 bool insert( Ptr & t , const K & k , const V & v );
 bool remove( Ptr & t , const K & k );
 template< class F >
 static void visit( const Ptr & t , F & f );

 /// the order, the heights, the sizes and the balance of the subtree,
 /// \f$ O( n ) \f$
 static bool valid( const Ptr & t , const K * lo , const K * hi );

 Ptr f_root;              ///< the root
 std::size_t f_size = 0;  ///< the number of keys

 };  // end( class AVLTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/AVLTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AVLTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File AVLTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
