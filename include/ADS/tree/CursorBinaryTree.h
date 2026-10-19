/*--------------------------------------------------------------------------*/
/*------------------------ File CursorBinaryTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A binary tree with cursors, beyond the book: the nodes in a vector,
 * each with its value and the places of its parent and of its two
 * children, NIL if missing, so that the links are indices instead of
 * pointers and the tree can be copied, saved or moved as a whole. A
 * removed subtree returns its places to a free list, which the next
 * insertions reuse. Moving along the tree, reading, writing and inserting a
 * node take O( 1 ) amortized time; removing or copying a subtree takes time
 * linear in its size.
 *
 * The contracts [see Contract.h]: a node is a used place, setRoot() needs
 * an empty tree, a child is inserted where none is, and the subtree copied
 * is a node of the other tree; the invariant is that the nodes reached from
 * the root link back to their parent and are all the nodes. The operations
 * are defined in CursorBinaryTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_CursorBinaryTree
 #define __ADS_CursorBinaryTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <cstddef>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CursorBinaryTree -------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary tree on a vector of nodes linked by their places
template< class T >
class CursorBinaryTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 CursorBinaryTree( void ) = default;

 /// a tree of one node of value v
 explicit CursorBinaryTree( const T & v );

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] int root( void ) const;

 [[nodiscard]] int parent( int n ) const;

 [[nodiscard]] int left( int n ) const;

 [[nodiscard]] int right( int n ) const;

 [[nodiscard]] bool isLeaf( int n ) const;

 [[nodiscard]] const T & read( int n ) const;

 void write( int n , const T & v );

 /// the number of nodes
 [[nodiscard]] int size( void ) const;

 /// the root of value v of the empty tree
 int setRoot( const T & v );

 /// a new left child of n of value v, which must be missing
 int insertLeft( int n , const T & v );

 /// a new right child of n of value v, which must be missing
 int insertRight( int n , const T & v );

 /// a copy of the subtree of t in s as the left subtree of n; t may be
 /// this tree, copied as it was before the call
 int insertLeft( int n , const CursorBinaryTree & t , int s );

 /// a copy of the subtree of t in s as the right subtree of n; t may be
 /// this tree, copied as it was before the call
 int insertRight( int n , const CursorBinaryTree & t , int s );

 /// removes the subtree of n, detached from its parent
 void remove( int n );

 /// the nodes in preorder
 [[nodiscard]] std::vector< int > preorder( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the places in use, for SlotIterator
 struct Used {
  static std::size_t size( const auto & o ) { return( o.f_N.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_N[ i ].used );
   }
  static int read( const auto & , std::size_t i ) { return( int( i ) ); }
  };

 /// the nodes, in the order of their places, bidirectional
 [[nodiscard]] auto nodes( void ) const {
  using It = SlotIterator< CursorBinaryTree , Used >;
  return( std::ranges::subrange( It( this , 0 ) ,
                                 It( this , f_N.size() ) ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  T value;
  int parent , left , right;
  bool used;
  };

 const Node & node( int n ) const;

 Node & node( int n );

 /// the nodes reached from the root are used, link back to their parent
 /// and are all the nodes, O( n )
 [[nodiscard]] bool linked( void ) const;

 int make( const T & v , int p );

 void release( int n );

 int copy( const CursorBinaryTree & t , int s , int p );

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;
 int f_free = NIL;          ///< the first free place
 int f_size = 0;

 };  // end( class CursorBinaryTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/CursorBinaryTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CursorBinaryTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File CursorBinaryTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
