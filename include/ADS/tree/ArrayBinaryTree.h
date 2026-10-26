/*--------------------------------------------------------------------------*/
/*------------------------- File ArrayBinaryTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A binary tree in a positional vector, beyond the book: the root in
 * the place 0, the children of the place i in 2 i + 1 and 2 i + 2, its
 * parent in \f$ \lfloor ( i - 1 ) / 2 \rfloor \f$ [see dary_child()], an
 * empty place where no node is. Moving along the tree, reading and writing
 * take O( 1 ) time; the vector grows to the deepest place used, so that a
 * tree of height h takes \f$ O( 2^h ) \f$ space, which suits the trees
 * nearly complete. insertLeft() and insertRight() copy a whole tree under
 * a node and remove() empties a subtree, in time linear in the places of
 * the subtree. A node is a place, NIL if missing.
 *
 * The contracts [see Contract.h]: read() and write() need a node,
 * setRoot() an empty tree, and a subtree is inserted under a node where
 * no child is; the invariant is that each node but the root has its
 * parent. The operations are defined in ArrayBinaryTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_ArrayBinaryTree
 #define __ADS_ArrayBinaryTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/tree/BinaryTree.h"

#include <cstddef>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ArrayBinaryTree --------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary tree in a positional vector
template< class T >
class ArrayBinaryTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr long NIL = -1;

 /// the empty tree
 ArrayBinaryTree( void ) = default;

 /// a tree of one node of value v
 explicit ArrayBinaryTree( const T & v );

 /// a new root of value v with copies of l and r as its subtrees
 ArrayBinaryTree( const T & v , const ArrayBinaryTree & l ,
                  const ArrayBinaryTree & r );

 [[nodiscard]] bool empty( void ) const;

 /// the root, NIL if the tree is empty
 [[nodiscard]] long root( void ) const;

 /// the parent of i, NIL for the root
 /// \pre i is a node
 [[nodiscard]] long parent( long i ) const;

 /// the left child of i, NIL if there is none
 /// \pre i is a node
 [[nodiscard]] long left( long i ) const;

 /// the right child of i, NIL if there is none
 /// \pre i is a node
 [[nodiscard]] long right( long i ) const;

 /// \pre i is a node
 [[nodiscard]] bool isLeaf( long i ) const;

 [[nodiscard]] const T & read( long i ) const;

 void write( long i , const T & v );

 /// the root of value v of the empty tree
 void setRoot( const T & v );

 /// a copy of t as the left subtree of i, which must be missing; t may be
 /// this tree, copied as it was before the call
 void insertLeft( long i , const ArrayBinaryTree & t );

 /// a copy of t as the right subtree of i, which must be missing; t may
 /// be this tree, copied as it was before the call
 void insertRight( long i , const ArrayBinaryTree & t );

 /// empties the subtree of i
 void remove( long i );

 /// the places of the nodes in preorder
 [[nodiscard]] std::vector< long > preorder( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the places in use, for SlotIterator
 struct Used {
  static std::size_t size( const auto & o ) { return( o.f_A.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_A[ i ].has_value() );
   }
  static long read( const auto & , std::size_t i ) { return( long( i ) ); }
  };

 /// the nodes, in the order of their places, bidirectional
 [[nodiscard]] auto nodes( void ) const {
  using It = SlotIterator< ArrayBinaryTree , Used >;
  return( std::ranges::subrange( It( this , 0 ) ,
                                 It( this , f_A.size() ) ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 [[nodiscard]] bool has( long i ) const;

 std::optional< T > & place( long i );

 const std::optional< T > & at( long i ) const;

 std::optional< T > & at( long i );

 /// empties the places of the subtree of i
 void clear( long i );

 /// each node but the root has its parent, O( size of the vector )
 [[nodiscard]] bool rooted( void ) const;

 /// the place j of t to the place k of this tree, and so on down
 void copy( long k , const ArrayBinaryTree & t , long j );

 void graft( long i , long c , const ArrayBinaryTree & t );

 std::vector< std::optional< T > > f_A;   ///< the places

 };  // end( class ArrayBinaryTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ArrayBinaryTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArrayBinaryTree.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ArrayBinaryTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
