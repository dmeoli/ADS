/*--------------------------------------------------------------------------*/
/*--------------------------- File CursorTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree with cursors, beyond the book: the nodes in a vector,
 * each with its value and the places of its parent, of its first child and
 * of its next sibling, NIL if missing, as the Tree with pointers [see
 * Tree.h] but with indices. A removed subtree returns its places to a free
 * list. insertChild() and insertSibling() hang a new node, insertChildTree()
 * and insertSiblingTree() a copy of a subtree of another tree, as first
 * child or as next sibling; the visits are those of the book.
 *
 * The contracts [see Contract.h]: a node is a used place, a sibling is
 * inserted next to a node that is not the root, and the subtree copied is
 * a node of the other tree; the invariant is that the nodes reached from
 * the root link back to their parent and are all the nodes. The operations
 * are defined in CursorTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1-5.3,
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

#ifndef __ADS_CursorTree
 #define __ADS_CursorTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS CursorTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree on a vector of nodes linked by their places
template< class T >
class CursorTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 CursorTree( void ) = default;

 /// a tree of one node of value v
 explicit CursorTree( const T & v );

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] int root( void ) const;

 [[nodiscard]] int parent( int n ) const;

 [[nodiscard]] int leftmostChild( int n ) const;

 [[nodiscard]] int rightSibling( int n ) const;

 [[nodiscard]] bool isLeaf( int n ) const;

 /// whether n is the last of its siblings
 [[nodiscard]] bool isLastSibling( int n ) const;

 [[nodiscard]] const T & read( int n ) const;

 void write( int n , const T & v );

 [[nodiscard]] int size( void ) const;

 /// a new first child of n of value v
 int insertChild( int n , const T & v );

 /// a new next sibling of n, not the root, of value v
 int insertSibling( int n , const T & v );

 /// a copy of the subtree of t in s as the first child of n
 int insertChildTree( int n , const CursorTree & t , int s );

 /// a copy of the subtree of t in s as the next sibling of n
 int insertSiblingTree( int n , const CursorTree & t , int s );

 /// removes the subtree of n, detached from its parent
 void remove( int n );

 /// the nodes in previsit, postvisit, and invisit with index i
 [[nodiscard]] std::vector< int > previsit( void ) const;

 [[nodiscard]] std::vector< int > postvisit( void ) const;

 [[nodiscard]] std::vector< int > invisit( int i ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  T value;
  int parent , child , sibling;
  bool used;
  };

 const Node & node( int n ) const;

 Node & node( int n );

 /// the nodes reached from the root are used, link back to their parent
 /// and are all the nodes, O( n )
 [[nodiscard]] bool linked( void ) const;

 int make( const T & v , int p );

 void free_node( int n );

 void release_children( int n );

 /// a copy of the node s of t, with its children, under p
 int copy( const CursorTree & t , int s , int p );

 /// kind 0 previsit, 1 postvisit, 2 invisit with index i
 void visit( int n , int i , std::vector< int > & P , int kind ) const;

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;
 int f_free = NIL;          ///< the first free place
 int f_size = 0;

 };  // end( class CursorTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/CursorTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CursorTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CursorTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
