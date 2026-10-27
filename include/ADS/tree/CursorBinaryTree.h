/*--------------------------------------------------------------------------*/
/*------------------------ File CursorBinaryTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A binary tree with cursors, beyond the book: the nodes in a vector, each
 * with its value and the places of its parent and of its two children,
 * `NIL` if missing, so that the links are indices instead of pointers and
 * the tree can be copied, saved or moved as a whole. A removed subtree
 * returns its places to a free list, which the next insertions reuse. Moving
 * along the tree, reading, writing and inserting a node take
 * \f$ O( 1 ) \f$ amortized time; removing or copying a subtree takes time
 * linear in its size.
 *
 * The contracts [see `Contract.h`]: a node is a used place, `setRoot()`
 * needs an empty tree, a child is inserted where none is, and the subtree
 * copied is a node of the other tree; the invariant is that the nodes
 * reached from the root link back to their parent and are all the nodes. The
 * operations are defined in `CursorBinaryTree.tpp`.
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
 static constexpr int NIL = -1;   ///< no node

 /// the empty tree
 CursorBinaryTree( void ) = default;

 /// a tree of one node of value \f$ v \f$
 explicit CursorBinaryTree( const T & v );

 /// true if the tree has no node
 [[nodiscard]] bool empty( void ) const;

 /// the place of the root, `NIL` if the tree is empty
 [[nodiscard]] int root( void ) const;

 /// the parent of \f$ n \f$, `NIL` for the root
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int parent( int n ) const;

 /// the left child of \f$ n \f$, `NIL` if missing
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int left( int n ) const;

 /// the right child of \f$ n \f$, `NIL` if missing
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int right( int n ) const;

 /// true if \f$ n \f$ has no child
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] bool isLeaf( int n ) const;

 /// the value of \f$ n \f$
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] const T & read( int n ) const;

 /// writes \f$ v \f$ in \f$ n \f$
 /// \pre \f$ n \f$ is a node
 void write( int n , const T & v );

 /// the number of nodes
 [[nodiscard]] int size( void ) const;

 /// the root of value \f$ v \f$ of the empty tree
 /// \pre the tree is empty
 int setRoot( const T & v );

 /// a new left child of \f$ n \f$ of value \f$ v \f$, which must be
 /// missing
 ///
 /// In pseudocode (section 5.5, with cursors), \f$ N \f$ the vector of the
 /// nodes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertLeft}( n , v ) \cr
 /// 2 & \quad c \gets \text{a free place, or a new one};\ N[ c ] \gets ( v
 ///     , n , \mathrm{NIL} , \mathrm{NIL} ) \cr
 /// 3 & \quad N[ n ].\mathit{left} \gets c;\ \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is a node with no left child
 int insertLeft( int n , const T & v );

 /// a new right child of \f$ n \f$ of value \f$ v \f$, which must be
 /// missing, as `insertLeft()` on the right
 /// \pre \f$ n \f$ is a node with no right child
 int insertRight( int n , const T & v );

 /// a copy of the subtree of \f$ t \f$ in \f$ s \f$ as the left subtree of
 /// \f$ n \f$; \f$ t \f$ may be this tree, copied as it was before the call
 ///
 /// In pseudocode (section 5.6), a previsit of \f$ t \f$ from \f$ s \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{copy}( t , s , p ) \cr
 /// 2 & \quad \textbf{if}\ s = \mathrm{NIL}\ \textbf{then return}
 ///     \ \mathrm{NIL} \cr
 /// 3 & \quad c \gets \text{a new node of value } t.\mathrm{read}( s )
 ///     \text{ under } p \cr
 /// 4 & \quad N[ c ].\mathit{left} \gets \mathrm{copy}( t , t.\mathrm{left}(
 ///     s ) , c );\ N[ c ].\mathit{right} \gets \mathrm{copy}( t ,
 ///     t.\mathrm{right}( s ) , c ) \cr
 /// 5 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is a node with no left child, \f$ s \f$ a node of
 ///      \f$ t \f$
 int insertLeft( int n , const CursorBinaryTree & t , int s );

 /// a copy of the subtree of \f$ t \f$ in \f$ s \f$ as the right subtree of
 /// \f$ n \f$, as the `insertLeft()` above on the right
 /// \pre \f$ n \f$ is a node with no right child, \f$ s \f$ a node of
 ///      \f$ t \f$
 int insertRight( int n , const CursorBinaryTree & t , int s );

 /// removes the subtree of \f$ n \f$, detached from its parent, its places
 /// back to the free list, in time linear in its size
 /// \pre \f$ n \f$ is a node
 void remove( int n );

 /// the nodes in preorder
 [[nodiscard]] std::vector< int > preorder( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the places in use, for `SlotIterator`
 struct Used {
  /// the places
  static std::size_t size( const auto & o ) { return( o.f_N.size() ); }
  /// whether the place \f$ i \f$ holds a node
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_N[ i ].used );
   }
  /// the node of the place \f$ i \f$, the place itself
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

 /// a node: its value, its links and whether the place is used
 struct Node {
  T value;                      ///< the value
  int parent ,                  ///< the parent, `NIL` for the root
      left ,                    ///< the left child, `NIL` if missing
      right;                    ///< the right child, `NIL` if missing
  bool used;                    ///< false for a free place
  };

 const Node & node( int n ) const;

 Node & node( int n );

 /// the nodes reached from the root are used, link back to their parent
 /// and are all the nodes, \f$ O( n ) \f$
 [[nodiscard]] bool linked( void ) const;

 int make( const T & v , int p );

 void release( int n );

 int copy( const CursorBinaryTree & t , int s , int p );

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;          ///< the root
 int f_free = NIL;          ///< the first free place
 int f_size = 0;            ///< the number of nodes

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
