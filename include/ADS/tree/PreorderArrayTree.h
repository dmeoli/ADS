/*--------------------------------------------------------------------------*/
/*------------------------ File PreorderArrayTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree in a vector in preorder, beyond the book: the place i
 * holds the value of the i-th node of the previsit and the place of its
 * parent, NIL for the root in the place 0. The subtree of a node is the
 * stretch of the vector that starts with it and ends before the first
 * place whose parent precedes it, so that the first child of i, if any, is
 * i + 1, and the next sibling is the place after its subtree. Reading and
 * writing take O( 1 ) time, moving along the tree and inserting or removing
 * a subtree O( n ), since the places after it shift and their parents are
 * renumbered. It extends the vector of parents of section 5.4 [see
 * ParentTree.h] with the values and the order of the children.
 *
 * The contracts [see Contract.h]: a node is a place of the vector, and a
 * sibling is inserted next to a node that is not the root; the invariant
 * is that the root comes first and each other node after its parent. The
 * operations are defined in PreorderArrayTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.4,
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

#ifndef __ADS_PreorderArrayTree
 #define __ADS_PreorderArrayTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <ranges>
#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS PreorderArrayTree -------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree in a vector of ( parent , value ) in preorder
template< class T >
class PreorderArrayTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 PreorderArrayTree( void ) = default;

 /// a tree of one node of value v
 explicit PreorderArrayTree( const T & v );

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] int size( void ) const;

 [[nodiscard]] int root( void ) const;

 [[nodiscard]] int parent( int i ) const;

 [[nodiscard]] const T & read( int i ) const;

 void write( int i , const T & v );

 /// the first child, i + 1 if its parent is i
 [[nodiscard]] int leftmostChild( int i ) const;

 /// the next sibling, the place after the subtree of i if it has the same
 /// parent
 [[nodiscard]] int rightSibling( int i ) const;

 [[nodiscard]] bool isLeaf( int i ) const;

 /// the place after the subtree of i
 [[nodiscard]] int subtreeEnd( int i ) const;

 /// a copy of t as the first child of i; t may be this tree, copied as
 /// it was before the call
 void insertChild( int i , const PreorderArrayTree & t );

 /// a copy of t as the next sibling of i, not the root; t may be this
 /// tree, copied as it was before the call
 void insertSibling( int i , const PreorderArrayTree & t );

 /// removes the subtree of i
 void remove( int i );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the values of the nodes in preorder, which is the order of the array,
 /// random access
 [[nodiscard]] auto values( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  int parent;
  T value;
  };

 const Cell & cell( int i ) const;

 /// the root first, and each other node after its parent, O( n )
 [[nodiscard]] bool preordered( void ) const;

 /// the cells of t to the place at, its root a child of p: the places
 /// from at on shift right by the size of t
 void splice( int at , int p , const PreorderArrayTree & t );

 std::vector< Cell > f_A;   ///< the nodes in preorder

 };  // end( class PreorderArrayTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/PreorderArrayTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PreorderArrayTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File PreorderArrayTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
