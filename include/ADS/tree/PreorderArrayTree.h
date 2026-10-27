/*--------------------------------------------------------------------------*/
/*------------------------ File PreorderArrayTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree in a vector in preorder, beyond the book: the place
 * \f$ i \f$ holds the value of the \f$ i \f$-th node of the previsit and the
 * place of its parent, `NIL` for the root in the place 0. The subtree of a
 * node is the stretch of the vector that starts with it and ends before the
 * first place whose parent precedes it, so that the first child of \f$ i \f$,
 * if any, is \f$ i + 1 \f$, and the next sibling is the place after its
 * subtree. Reading and writing take \f$ O( 1 ) \f$ time, moving along the
 * tree and inserting or removing a subtree \f$ O( n ) \f$, since the places
 * after it shift and their parents are renumbered. It extends the vector of
 * parents of section 5.4 [see `ParentTree.h`] with the values and the order
 * of the children.
 *
 * The contracts [see `Contract.h`]: a node is a place of the vector, and a
 * sibling is inserted next to a node that is not the root; the invariant is
 * that the root comes first and each other node after its parent. The
 * operations are defined in `PreorderArrayTree.tpp`.
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
 static constexpr int NIL = -1;   ///< no node

 /// the empty tree
 PreorderArrayTree( void ) = default;

 /// a tree of one node of value \f$ v \f$
 explicit PreorderArrayTree( const T & v );

 /// true if the tree has no node
 [[nodiscard]] bool empty( void ) const;

 /// the number of nodes, \f$ n \f$
 [[nodiscard]] int size( void ) const;

 /// the root, 0, or `NIL` if the tree is empty
 [[nodiscard]] int root( void ) const;

 /// the parent of \f$ i \f$, `NIL` for the root
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] int parent( int i ) const;

 /// the value of \f$ i \f$
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] const T & read( int i ) const;

 /// writes \f$ v \f$ in \f$ i \f$
 /// \pre \f$ i \f$ is a node
 void write( int i , const T & v );

 /// the first child, \f$ i + 1 \f$ if its parent is \f$ i \f$, `NIL`
 /// otherwise
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] int leftmostChild( int i ) const;

 /// the next sibling, the place after the subtree of \f$ i \f$ if it has the
 /// same parent, `NIL` otherwise
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] int rightSibling( int i ) const;

 /// true if \f$ i \f$ has no child
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] bool isLeaf( int i ) const;

 /// the place after the subtree of \f$ i \f$
 ///
 /// In pseudocode (beyond the book), \f$ p \f$ the parents: the subtree
 /// goes on while the parents are in it, i.e., not before \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{subtreeEnd}( i ) \cr
 /// 2 & \quad e \gets i + 1 \cr
 /// 3 & \quad \textbf{while}\ e < n\ \textbf{and}\ p[ e ] \ge i
 ///     \ \textbf{do}\ e \gets e + 1 \cr
 /// 4 & \quad \textbf{return}\ e
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] int subtreeEnd( int i ) const;

 /// a copy of \f$ t \f$ as the first child of \f$ i \f$; \f$ t \f$ may be
 /// this tree, copied as it was before the call
 ///
 /// In pseudocode (beyond the book), the cells of \f$ t \f$, of \f$ k \f$
 /// nodes, spliced in at the place \f$ i + 1 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertChild}( i , t ) \cr
 /// 2 & \quad a \gets i + 1;\ C \gets \text{a copy of the cells of } t \cr
 /// 3 & \quad \textbf{for}\ j \gets a\ \textbf{to}\ n - 1\ \textbf{do}
 ///     \ \textbf{if}\ p[ j ] \ge a\ \textbf{then}\ p[ j ] \gets p[ j ] + k
 ///     \cr
 /// 4 & \quad C[ 0 ].\mathit{parent} \gets i;\ \textbf{for}\ j \gets 1
 ///     \ \textbf{to}\ k - 1\ \textbf{do}\ C[ j ].\mathit{parent} \gets C[ j
 ///     ].\mathit{parent} + a \cr
 /// 5 & \quad \text{insert } C \text{ at the place } a
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ is a node
 void insertChild( int i , const PreorderArrayTree & t );

 /// a copy of \f$ t \f$ as the next sibling of \f$ i \f$, not the root;
 /// \f$ t \f$ may be this tree, copied as it was before the call: the splice
 /// of `insertChild()` at the end of the subtree of \f$ i \f$, under the
 /// parent of \f$ i \f$
 /// \pre \f$ i \f$ is a node other than the root
 void insertSibling( int i , const PreorderArrayTree & t );

 /// removes the subtree of \f$ i \f$, the places after it shifted back and
 /// their parents renumbered
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( i ) \cr
 /// 2 & \quad e \gets \mathrm{subtreeEnd}( i );\ k \gets e - i \cr
 /// 3 & \quad \text{erase the places } [ i , e ) \cr
 /// 4 & \quad \textbf{for}\ j \gets i\ \textbf{to}\ n - 1\ \textbf{do}
 ///     \ \textbf{if}\ p[ j ] \ge e\ \textbf{then}\ p[ j ] \gets p[ j ] - k
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ is a node
 void remove( int i );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the values of the nodes in preorder, which is the order of the array,
 /// random access
 [[nodiscard]] auto values( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node: the place of its parent and its value
 struct Cell {
  int parent;   ///< the place of the parent, `NIL` for the root
  T value;      ///< the value
  };

 const Cell & cell( int i ) const;

 /// the root first, and each other node after its parent, \f$ O( n ) \f$
 [[nodiscard]] bool preordered( void ) const;

 /// the cells of \f$ t \f$ to the place `at`, its root a child of
 /// \f$ p \f$: the places from `at` on shift right by the size of
 /// \f$ t \f$
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
