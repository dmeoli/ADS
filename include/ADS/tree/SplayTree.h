/*--------------------------------------------------------------------------*/
/*---------------------------- File SplayTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The splay tree of Sleator and Tarjan, beyond the book: a binary search
 * tree with no balance information, where each access moves the node
 * accessed to the root by rotations in pairs (zig-zig and zig-zag), which
 * roughly halve the depth of the nodes on its path; any sequence of m
 * operations on n keys takes \f$ O( ( m + n ) \log n ) \f$ time, i.e.,
 * \f$ O( \log n ) \f$ amortized each, and the keys accessed often stay
 * near the root (the static optimality and the working set theorems). The
 * splay is top-down, in one pass from the root that splits the tree into a
 * left and a right part and joins them at the end. It is a DICTIONARY [see
 * ADT.h]; lookup() also splays, hence it changes the shape of the tree
 * (but not its keys) and is not const.
 *
 * The contracts [see Contract.h]: insert() ensures that the key is there,
 * at the root, and remove() that it is not; the invariant is the order of
 * the keys, an O( n ) check at the level 3. The operations are defined in
 * SplayTree.tpp.
 *
 * \see D. D. Sleator, R. E. Tarjan, Self-adjusting binary search trees,
 *      Journal of the ACM 32(3), 652-686, 1985, doi:10.1145/3828.3835
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

 SplayTree( void ) = default;
 SplayTree( const SplayTree & ) = delete;
 SplayTree & operator=( const SplayTree & ) = delete;

 /// the nodes of other, which is left empty
 SplayTree( SplayTree && other ) noexcept;
 ~SplayTree();

 /// the value of k, or nothing; k, or the last node on its path, goes to
 /// the root
 [[nodiscard]] std::optional< V > lookup( const K & k );

 /// \post k has the value v, at the root
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] std::size_t size( void ) const;

 /// the key at the root, if any
 [[nodiscard]] std::optional< K > root( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  K key;
  V value;
  Node * left = nullptr , * right = nullptr;
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, along the path from the root; a lookup splays the tree
 // and invalidates it

 /// the shape of the tree, for PathIterator
 struct Shape {
  static auto root( const auto & o ) { return( o.f_root ); }
  static auto left( const auto * n ) { return( n->left ); }
  static auto right( const auto * n ) { return( n->right ); }
  static auto read( const auto * n ) {
   return( std::pair< const K & , const V & >( n->key , n->value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using const_iterator = PathIterator< SplayTree , Shape >;
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

 /// the top-down splay of k in t: the new root
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

#include "ADS/tree/SplayTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SplayTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SplayTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
