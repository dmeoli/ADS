/*--------------------------------------------------------------------------*/
/*------------------------------ File BTree.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The B-tree of Bayer and McCreight, beyond the book [see BPlusTree.h for
 * the B+-tree of chapter 6, which has no removal]: a search tree whose
 * nodes hold from t - 1 to 2 t - 1 keys (the root from 1), a node of j keys
 * having j + 1 children, and all the leaves at the same depth, so that the
 * height is at most \f$ \log_t ( ( n + 1 ) / 2 ) \f$ and a node fills a
 * block of the disk. insert() goes down from the root splitting each full
 * node it meets, so that the leaf reached has room; remove() goes down
 * making sure that each node it enters has at least t keys, by borrowing a
 * key from a sibling or by merging with it, so that a key can always be
 * taken out of a leaf (a key of an internal node is replaced by its
 * predecessor or successor, or the two children around it are merged);
 * both in one pass, \f$ O( t \log_t n ) \f$ time. It is a DICTIONARY [see
 * ADT.h], and forEach() visits the pairs by increasing keys.
 *
 * The contracts [see Contract.h]: t >= 2; insert() ensures that the key is
 * there and remove() that it is not; the invariant is the order of the
 * keys, their number in each node and the depth of the leaves, an O( n )
 * check at the level 3. The operations are defined in BTree.tpp.
 *
 * \see R. Bayer, E. M. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, chapter 18
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.4
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

#ifndef __ADS_BTree
 #define __ADS_BTree
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
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BTree --------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a B-tree of minimum degree t
template< std::totally_ordered K , class V >
class BTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty B-tree of minimum degree t
 /// \pre t >= 2
 explicit BTree( int t = 3 );

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool isEmpty( void ) const;

 /// the height, 0 for a single leaf, -1 if empty
 [[nodiscard]] int height( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::vector< K > k;
  std::vector< V > v;
  std::vector< std::unique_ptr< Node > > c;   ///< empty in a leaf
  [[nodiscard]] bool leaf( void ) const { return( c.empty() ); }
  };

 /// the first place in x of a key not smaller than key
 static std::size_t place( const Node * x , const K & key );

 /// splits the full child i of x
 void split( Node * x , std::size_t i );

 /// merges the children j and j + 1 of x with the key j between them
 static void merge( Node * x , std::size_t j );

 /// makes the child i of x have at least t keys; the child to go down
 std::size_t fill( Node * x , std::size_t i );

 bool remove( Node * x , const K & key );

 template< class F >
 static void visit( const Node * x , F & f );

 /// the order, the sizes and the depth of the leaves, O( n )
 bool valid( const Node * x , const K * lo , const K * hi , int d ,
             int & leafDepth , bool root ) const;

 int f_t;                          ///< the minimum degree
 std::unique_ptr< Node > f_root;   ///< the root
 std::size_t f_size = 0;           ///< the number of keys

 };  // end( class BTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/BTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File BTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
