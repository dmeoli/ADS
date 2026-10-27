/*--------------------------------------------------------------------------*/
/*------------------------- File ArrayBinaryTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A binary tree in a positional vector, beyond the book: the root in the
 * place 0, the children of the place \f$ i \f$ in \f$ 2 i + 1 \f$ and
 * \f$ 2 i + 2 \f$, its parent in \f$ \lfloor ( i - 1 ) / 2 \rfloor \f$ [see
 * `dary_child()`], an empty place where no node is. Moving along the tree,
 * reading and writing take \f$ O( 1 ) \f$ time; the vector grows to the
 * deepest place used, so that a tree of height \f$ h \f$ takes
 * \f$ O( 2^h ) \f$ space, which suits the trees nearly complete.
 * `insertLeft()` and `insertRight()` copy a whole tree under a node and
 * `remove()` empties a subtree, in time linear in the places of the subtree.
 * A node is a place, `NIL` if missing.
 *
 * The contracts [see `Contract.h`]: `read()` and `write()` need a node,
 * `setRoot()` an empty tree, and a subtree is inserted under a node where no
 * child is; the invariant is that each node but the root has its parent. The
 * operations are defined in `ArrayBinaryTree.tpp`.
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
 static constexpr long NIL = -1;   ///< no node

 /// the empty tree
 ArrayBinaryTree( void ) = default;

 /// a tree of one node of value \f$ v \f$
 explicit ArrayBinaryTree( const T & v );

 /// a new root of value \f$ v \f$ with copies of \f$ l \f$ and \f$ r \f$
 /// as its subtrees
 ArrayBinaryTree( const T & v , const ArrayBinaryTree & l ,
                  const ArrayBinaryTree & r );

 /// true if the tree has no node
 [[nodiscard]] bool empty( void ) const;

 /// the root, 0, or `NIL` if the tree is empty
 [[nodiscard]] long root( void ) const;

 /// the parent \f$ \lfloor ( i - 1 ) / 2 \rfloor \f$ of \f$ i \f$, `NIL` for
 /// the root
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] long parent( long i ) const;

 /// the left child \f$ 2 i + 1 \f$ of \f$ i \f$, `NIL` if there is none
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] long left( long i ) const;

 /// the right child \f$ 2 i + 2 \f$ of \f$ i \f$, `NIL` if there is none
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] long right( long i ) const;

 /// true if \f$ i \f$ has no child
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] bool isLeaf( long i ) const;

 /// the value of \f$ i \f$
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] const T & read( long i ) const;

 /// writes \f$ v \f$ in \f$ i \f$
 /// \pre \f$ i \f$ is a node
 void write( long i , const T & v );

 /// the root of value \f$ v \f$ of the empty tree
 /// \pre the tree is empty
 void setRoot( const T & v );

 /// a copy of \f$ t \f$ as the left subtree of \f$ i \f$, which must be
 /// missing; \f$ t \f$ may be this tree, copied as it was before the call
 ///
 /// In pseudocode (beyond the book), the place \f$ j \f$ of \f$ t \f$ copied
 /// to the place \f$ k \f$, from \f$ j = 0 \f$ and \f$ k = 2 i + 1 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{copy}( k , t , j ) \cr
 /// 2 & \quad \textbf{if}\ t \text{ has no node in } j\ \textbf{then
 ///     return} \cr
 /// 3 & \quad A[ k ] \gets t.A[ j ]\quad \triangleright\ \text{the vector }
 ///     \text{grown to } k \text{ if needed} \cr
 /// 4 & \quad \mathrm{copy}( 2 k + 1 , t , 2 j + 1 );\ \mathrm{copy}( 2 k + 2
 ///     , t , 2 j + 2 )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ is a node with no left child
 void insertLeft( long i , const ArrayBinaryTree & t );

 /// a copy of \f$ t \f$ as the right subtree of \f$ i \f$, which must be
 /// missing; \f$ t \f$ may be this tree, copied as it was before the call:
 /// the copy of `insertLeft()` from \f$ k = 2 i + 2 \f$
 /// \pre \f$ i \f$ is a node with no right child
 void insertRight( long i , const ArrayBinaryTree & t );

 /// empties the subtree of \f$ i \f$, a previsit of its places
 /// \pre \f$ i \f$ is a node
 void remove( long i );

 /// the places of the nodes in preorder
 [[nodiscard]] std::vector< long > preorder( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the places in use, for `SlotIterator`
 struct Used {
  /// the places
  static std::size_t size( const auto & o ) { return( o.f_A.size() ); }
  /// whether the place \f$ i \f$ holds a node
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_A[ i ].has_value() );
   }
  /// the node of the place \f$ i \f$, the place itself
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

 /// empties the places of the subtree of \f$ i \f$
 void clear( long i );

 /// each node but the root has its parent, in time linear in the size of
 /// the vector
 [[nodiscard]] bool rooted( void ) const;

 /// the place \f$ j \f$ of \f$ t \f$ to the place \f$ k \f$ of this tree,
 /// and so on down
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
