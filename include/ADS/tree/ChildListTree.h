/*--------------------------------------------------------------------------*/
/*-------------------------- File ChildListTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree realized with lists of children, beyond the book: the
 * nodes in a vector, each with its value, the place of its parent and the
 * `List` [see `List.h`] of the places of its children, in their order. A
 * node also keeps the position of its own cell in the list of its parent, so
 * that `rightSibling()` is the next cell and takes \f$ O( 1 ) \f$ time, as
 * `leftmostChild()`, `insertChild()` and `insertSibling()`; the children of
 * a node are scanned in \f$ O( 1 ) \f$ time each, which is what this
 * realization is for, and a removed subtree returns its places to a free
 * list. The operations and the visits are those of `CursorTree` [see
 * `CursorTree.h`].
 *
 * The contracts [see `Contract.h`]: a node is a used place, and a sibling is
 * inserted next to a node that is not the root; the invariant is that the
 * nodes reached from the root are used, are in the list of their parent at
 * their own position, and are all the nodes. The operations are defined in
 * `ChildListTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1-5.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 * \see A. V. Aho, J. E. Hopcroft, J. D. Ullman, Data Structures and
 *      Algorithms, Addison-Wesley, 1983, section 3.3 (the lists of
 *      children)
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
 static constexpr int NIL = -1;   ///< no node

 /// the empty tree
 ChildListTree( void ) = default;

 /// a tree of one node of value \f$ v \f$
 explicit ChildListTree( const T & v );

 /// true if the tree has no node
 [[nodiscard]] bool empty( void ) const;

 /// the place of the root, `NIL` if the tree is empty
 [[nodiscard]] int root( void ) const;

 /// the parent of \f$ n \f$, `NIL` for the root
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int parent( int n ) const;

 /// the first child of \f$ n \f$, the head of its list, `NIL` for a leaf
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int leftmostChild( int n ) const;

 /// the next sibling of \f$ n \f$, the next cell of the list of its parent,
 /// `NIL` if there is none
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int rightSibling( int n ) const;

 /// true if \f$ n \f$ has no child
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] bool isLeaf( int n ) const;

 /// whether \f$ n \f$ is the last of its siblings
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] bool isLastSibling( int n ) const;

 /// the list of the children of \f$ n \f$, in their order
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] const List< int > & children( int n ) const;

 /// the value of \f$ n \f$
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] const T & read( int n ) const;

 /// writes \f$ v \f$ in \f$ n \f$
 /// \pre \f$ n \f$ is a node
 void write( int n , const T & v );

 /// the number of nodes
 [[nodiscard]] int size( void ) const;

 /// a new first child of \f$ n \f$ of value \f$ v \f$, in \f$ O( 1 ) \f$
 ///
 /// In pseudocode (Aho, Hopcroft and Ullman, section 3.3), \f$ N \f$ the
 /// vector of the nodes and \f$ C_n \f$ the list of the children of
 /// \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertChild}( n , v ) \cr
 /// 2 & \quad c \gets \text{a free place, or a new one, of value } v
 ///     \text{ and parent } n \cr
 /// 3 & \quad N[ c ].\mathit{where} \gets C_n.\mathrm{insert}(
 ///     C_n.\mathrm{head}() , c ) \cr
 /// 4 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is a node
 int insertChild( int n , const T & v );

 /// a new next sibling of \f$ n \f$, not the root, of value \f$ v \f$, in
 /// \f$ O( 1 ) \f$: as `insertChild()` on the list of the parent of
 /// \f$ n \f$, after the cell of \f$ n \f$
 /// \pre \f$ n \f$ is a node other than the root
 int insertSibling( int n , const T & v );

 /// removes the subtree of \f$ n \f$, detached from its parent in
 /// \f$ O( 1 ) \f$ by the position of its cell, its places back to the free
 /// list
 /// \pre \f$ n \f$ is a node
 void remove( int n );

 /// the nodes in previsit
 [[nodiscard]] std::vector< int > previsit( void ) const;

 /// the nodes in postvisit
 [[nodiscard]] std::vector< int > postvisit( void ) const;

 /// the nodes in invisit with index \f$ i \f$, as
 /// `CursorTree::invisit()` on the lists of the children
 [[nodiscard]] std::vector< int > invisit( int i ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node: its value, its parent, its children and its own cell
 struct Node {
  T value;                            ///< the value
  int parent;                         ///< the parent, or the next free place
  List< int > children;               ///< the places of the children
  typename List< int >::Pos where;    ///< the cell of the node in its parent
  bool used;                          ///< false for a free place
  };

 const Node & node( int n ) const;
 Node & node( int n );

 /// a new node of value \f$ v \f$ under \f$ p \f$, not yet in a list of
 /// children
 int make( const T & v , int p );

 /// frees \f$ n \f$ and its subtree
 void release( int n );

 /// the visit of `kind` 0 previsit, 1 postvisit, 2 invisit with index
 /// \f$ i \f$
 void visit( int n , int i , std::vector< int > & P , int kind ) const;

 /// the nodes reached from the root are used, are at their position in
 /// the list of their parent, and are all the nodes, \f$ O( n ) \f$
 [[nodiscard]] bool linked( void ) const;

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;          ///< the root
 int f_free = NIL;          ///< the first free place, linked by parent
 int f_size = 0;            ///< the number of nodes

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
