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
 * The contracts [see Contract.h]: insert() ensures that the key is there
 * and remove() that it is not; the invariant is the order of the keys, the
 * heights stored and the balance of every node, an O( n ) check at the
 * level 3. The operations are defined in AVLTree.tpp.
 *
 * \see G. M. Adelson-Velsky, E. M. Landis, An algorithm for the
 *      organization of information, Soviet Mathematics Doklady 3,
 *      1259-1263, 1962
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.3
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

#ifndef __ADS_AVLTree
 #define __ADS_AVLTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

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

/// a DICTIONARY on an AVL tree
template< std::totally_ordered K , class V >
class AVLTree {

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

 [[nodiscard]] bool isEmpty( void ) const;

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
  std::unique_ptr< Node > left , right;
  };
 using Ptr = std::unique_ptr< Node >;

 static int h( const Ptr & t ) { return( t ? t->h : -1 ); }
 static void update( Ptr & t );
 static void rotateRight( Ptr & t );
 static void rotateLeft( Ptr & t );
 static void balance( Ptr & t );
 bool insert( Ptr & t , const K & k , const V & v );
 bool remove( Ptr & t , const K & k );
 template< class F >
 static void visit( const Ptr & t , F & f );

 /// the order, the heights and the balance of the subtree, O( n )
 static bool valid( const Ptr & t , const K * lo , const K * hi );

 Ptr f_root;              ///< the root
 std::size_t f_size = 0;  ///< the number of keys

 };  // end( class AVLTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/AVLTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AVLTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File AVLTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
