/*--------------------------------------------------------------------------*/
/*-------------------------- File ChildListTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree realized with lists of children, beyond the book: the
 * nodes in a vector, each with its value, the place of its parent and the
 * List [see List.h] of the places of its children, in their order. A node
 * also keeps the position of its own cell in the list of its parent, so
 * that rightSibling() is the next cell and takes O( 1 ) time, as
 * leftmostChild(), insertChild() and insertSibling(); the children of a
 * node are scanned in O( 1 ) time each, which is what this realization is
 * for, and a removed subtree returns its places to a free list. The
 * operations and the visits are those of CursorTree [see CursorTree.h].
 *
 * The contracts [see Contract.h]: a node is a used place, and a sibling is
 * inserted next to a node that is not the root; the invariant is that the
 * nodes reached from the root are used, are in the list of their parent at
 * their own position, and are all the nodes. The operations are defined in
 * ChildListTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1-5.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 * \see A. V. Aho, J. E. Hopcroft, J. D. Ullman, Data Structures and
 *      Algorithms, Addison-Wesley, 1983, section 3.3 (the lists of
 *      children)
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

#ifndef __ADS_ChildListTree
 #define __ADS_ChildListTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ChildListTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree whose nodes keep the list of their children
template< class T >
class ChildListTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 ChildListTree( void ) = default;

 /// a tree of one node of value v
 explicit ChildListTree( const T & v );

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] int root( void ) const;

 /// \pre n is a node
 [[nodiscard]] int parent( int n ) const;

 /// \pre n is a node
 [[nodiscard]] int leftmostChild( int n ) const;

 /// \pre n is a node
 [[nodiscard]] int rightSibling( int n ) const;

 /// \pre n is a node
 [[nodiscard]] bool isLeaf( int n ) const;

 /// whether n is the last of its siblings
 /// \pre n is a node
 [[nodiscard]] bool isLastSibling( int n ) const;

 /// the list of the children of n, in their order
 /// \pre n is a node
 [[nodiscard]] const List< int > & children( int n ) const;

 /// \pre n is a node
 [[nodiscard]] const T & read( int n ) const;

 /// \pre n is a node
 void write( int n , const T & v );

 [[nodiscard]] int size( void ) const;

 /// a new first child of n of value v
 /// \pre n is a node
 int insertChild( int n , const T & v );

 /// a new next sibling of n, not the root, of value v
 /// \pre n is a node other than the root
 int insertSibling( int n , const T & v );

 /// removes the subtree of n, detached from its parent
 /// \pre n is a node
 void remove( int n );

 /// the nodes in previsit, postvisit, and invisit with index i
 [[nodiscard]] std::vector< int > previsit( void ) const;
 [[nodiscard]] std::vector< int > postvisit( void ) const;
 [[nodiscard]] std::vector< int > invisit( int i ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  T value;
  int parent;
  List< int > children;               ///< the places of the children
  typename List< int >::Pos where;    ///< the cell of n in its parent
  bool used;
  };

 const Node & node( int n ) const;
 Node & node( int n );

 /// a new node of value v under p, not yet in a list of children
 int make( const T & v , int p );

 /// frees n and its subtree
 void release( int n );

 /// kind 0 previsit, 1 postvisit, 2 invisit with index i
 void visit( int n , int i , std::vector< int > & P , int kind ) const;

 /// the nodes reached from the root are used, are at their position in
 /// the list of their parent, and are all the nodes, O( n )
 [[nodiscard]] bool linked( void ) const;

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;
 int f_free = NIL;          ///< the first free place, linked by parent
 int f_size = 0;

 };  // end( class ChildListTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ChildListTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ChildListTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ChildListTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
