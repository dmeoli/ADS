/*--------------------------------------------------------------------------*/
/*--------------------------- File CursorTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree with cursors, beyond the book: the nodes in a vector, each
 * with its value and the places of its parent, of its first child and of its
 * next sibling, `NIL` if missing, as the `Tree` with pointers [see `Tree.h`]
 * but with indices. A removed subtree returns its places to a free list.
 * `insertChild()` and `insertSibling()` hang a new node, `insertChildTree()`
 * and `insertSiblingTree()` a copy of a subtree of another tree, as first
 * child or as next sibling; the visits are those of the book.
 *
 * The contracts [see `Contract.h`]: a node is a used place, a sibling is
 * inserted next to a node that is not the root, and the subtree copied is a
 * node of the other tree; the invariant is that the nodes reached from the
 * root link back to their parent and are all the nodes. The operations are
 * defined in `CursorTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.1-5.3,
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

#ifndef __ADS_CursorTree
 #define __ADS_CursorTree
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
/*---------------------------- CLASS CursorTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree on a vector of nodes linked by their places
template< class T >
class CursorTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;   ///< no node

 /// the empty tree
 CursorTree( void ) = default;

 /// a tree of one node of value \f$ v \f$
 explicit CursorTree( const T & v );

 /// true if the tree has no node
 [[nodiscard]] bool empty( void ) const;

 /// the place of the root, `NIL` if the tree is empty
 [[nodiscard]] int root( void ) const;

 /// the parent of \f$ n \f$, `NIL` for the root
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int parent( int n ) const;

 /// the first child of \f$ n \f$, `NIL` for a leaf
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int leftmostChild( int n ) const;

 /// the next sibling of \f$ n \f$, `NIL` if there is none
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] int rightSibling( int n ) const;

 /// true if \f$ n \f$ has no child
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] bool isLeaf( int n ) const;

 /// whether \f$ n \f$ is the last of its siblings
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] bool isLastSibling( int n ) const;

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
 /// In pseudocode (section 5.2, with cursors), \f$ N \f$ the vector of the
 /// nodes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertChild}( n , v ) \cr
 /// 2 & \quad c \gets \text{a free place, or a new one};\ N[ c ] \gets ( v
 ///     , n , \mathrm{NIL} , \mathrm{NIL} ) \cr
 /// 3 & \quad N[ c ].\mathit{sibling} \gets N[ n ].\mathit{child};\ N[ n
 ///     ].\mathit{child} \gets c \cr
 /// 4 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is a node
 int insertChild( int n , const T & v );

 /// a new next sibling of \f$ n \f$, not the root, of value \f$ v \f$, in
 /// \f$ O( 1 ) \f$, as `insertChild()` with the sibling of \f$ n \f$ in place
 /// of its child
 /// \pre \f$ n \f$ is a node other than the root
 int insertSibling( int n , const T & v );

 /// a copy of the subtree of \f$ t \f$ in \f$ s \f$ as the first child of
 /// \f$ n \f$; \f$ t \f$ may be this tree, copied as it was before the call
 ///
 /// In pseudocode (section 5.3), the copy a previsit, then linked as in
 /// `insertChild()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{copy}( t , s , p ) \cr
 /// 2 & \quad c \gets \text{a new node of value } t.\mathrm{read}( s )
 ///     \text{ under } p;\ \ell \gets \mathrm{NIL} \cr
 /// 3 & \quad \textbf{foreach}\ \text{child } u \text{ of } s \text{ in }
 ///     t \text{, in order}\ \textbf{do} \cr
 /// 4 & \qquad x \gets \mathrm{copy}( t , u , c ) \cr
 /// 5 & \qquad \textbf{if}\ \ell = \mathrm{NIL}\ \textbf{then}\ N[ c
 ///     ].\mathit{child} \gets x\ \textbf{else}\ N[ \ell ].\mathit{sibling}
 ///     \gets x \cr
 /// 6 & \qquad \ell \gets x \cr
 /// 7 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is a node, \f$ s \f$ a node of \f$ t \f$
 int insertChildTree( int n , const CursorTree & t , int s );

 /// a copy of the subtree of \f$ t \f$ in \f$ s \f$ as the next sibling of
 /// \f$ n \f$; \f$ t \f$ may be this tree, copied as it was before the call
 /// \pre \f$ n \f$ is a node other than the root, \f$ s \f$ a node of
 ///      \f$ t \f$
 int insertSiblingTree( int n , const CursorTree & t , int s );

 /// removes the subtree of \f$ n \f$, detached from its parent, its places
 /// back to the free list
 ///
 /// In pseudocode (section 5.2, with cursors), \f$ O( d + m ) \f$ for
 /// \f$ d \f$ the siblings before \f$ n \f$ and \f$ m \f$ the nodes of its
 /// subtree:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( n ) \cr
 /// 2 & \quad p \gets N[ n ].\mathit{parent} \cr
 /// 3 & \quad \textbf{if}\ p = \mathrm{NIL}\ \textbf{then}
 ///     \ \mathit{root} \gets \mathrm{NIL} \cr
 /// 4 & \quad \textbf{else}\ \text{unlink } n \text{ from the list of the }
 ///     \text{children of } p \text{, by a scan} \cr
 /// 5 & \quad \text{give the places of the subtree of } n \text{ back to the }
 ///     \text{free list, in postvisit}
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is a node
 void remove( int n );

 /// the nodes in previsit
 [[nodiscard]] std::vector< int > previsit( void ) const;

 /// the nodes in postvisit
 [[nodiscard]] std::vector< int > postvisit( void ) const;

 /// the nodes in invisit with index \f$ i \f$, each node after its first
 /// \f$ i \f$ subtrees
 ///
 /// In pseudocode (section 5.3), the visit of `Tree.h` on the places, the
 /// previsit and the postvisit being the same with the node first or last:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{invisit}( n , i ) \cr
 /// 2 & \quad k \gets 0 \cr
 /// 3 & \quad \textbf{foreach}\ \text{child } u \text{ of } n \text{, in }
 ///     \text{order}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ k = i\ \textbf{then}\ \text{append } n \cr
 /// 5 & \qquad \mathrm{invisit}( u , i );\ k \gets k + 1 \cr
 /// 6 & \quad \textbf{if}\ k \le i\ \textbf{then}\ \text{append } n
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::vector< int > invisit( int i ) const;

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
  using It = SlotIterator< CursorTree , Used >;
  return( std::ranges::subrange( It( this , 0 ) ,
                                 It( this , f_N.size() ) ) );
  }

 /// the walk of the children of a node
 struct Siblings {
  static constexpr int none = NIL;   ///< the end
  /// the next sibling of \f$ n \f$
  static int next( const CursorTree * o , int n , int ) {
   return( o->rightSibling( n ) );
   }
  };

 /// the children of \f$ n \f$, from the leftmost, forward
 /// \pre \f$ n \f$ is a node
 [[nodiscard]] auto children( int n ) const {
  using It = WalkIterator< CursorTree , int , Siblings >;
  return( std::ranges::subrange( It( this , leftmostChild( n ) , n ) ,
                                 It() ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a node: its value, its links and whether the place is used
 struct Node {
  T value;                        ///< the value
  int parent ,                    ///< the parent, `NIL` for the root
      child ,                     ///< the first child, `NIL` if missing
      sibling;                    ///< the next sibling, `NIL` if missing
  bool used;                      ///< false for a free place
  };

 const Node & node( int n ) const;

 Node & node( int n );

 /// the nodes reached from the root are used, link back to their parent
 /// and are all the nodes, \f$ O( n ) \f$
 [[nodiscard]] bool linked( void ) const;

 int make( const T & v , int p );

 void free_node( int n );

 void release_children( int n );

 /// a copy of the node \f$ s \f$ of \f$ t \f$, with its children, under
 /// \f$ p \f$
 int copy( const CursorTree & t , int s , int p );

 /// the visit of `kind` 0 previsit, 1 postvisit, 2 invisit with index
 /// \f$ i \f$
 void visit( int n , int i , std::vector< int > & P , int kind ) const;

 std::vector< Node > f_N;   ///< the nodes, used or free
 int f_root = NIL;          ///< the root
 int f_free = NIL;          ///< the first free place
 int f_size = 0;            ///< the number of nodes

 };  // end( class CursorTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/CursorTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CursorTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CursorTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
