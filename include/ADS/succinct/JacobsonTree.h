/*--------------------------------------------------------------------------*/
/*--------------------------- File JacobsonTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary tree of Jacobson in \f$ 2 n + 1 \f$ bits (chapter 15, Compressed
 * Data Structures, of Ferragina's book): the tree of \f$ n \f$ nodes is
 * completed with a leaf in place of each missing child, its \f$ 2 n + 1 \f$
 * nodes are visited by levels, and the code \f$ B \f$ gets a 1 for a node of
 * the tree and a 0 for a leaf added. With the places counted from 1, the
 * children of the \f$ x \f$-th node by levels are at the places \f$ 2 x \f$
 * and \f$ 2 x + 1 \f$, as in a heap, since each node adds its two children to
 * the visit; the node at the place \f$ p \f$ is the
 * \f$ \mathrm{rank}_1( p ) \f$-th one, and the \f$ x \f$-th node is at the
 * place \f$ \mathrm{select}_1( x ) \f$, a child of the
 * \f$ \lfloor \mathrm{select}_1( x ) / 2 \rfloor \f$-th one. Hence, with rank
 * and select in \f$ O( 1 ) \f$ time [see `RankSelect.h`], `left()`, `right()`
 * and `parent()` take \f$ O( 1 ) \f$ time, in \f$ 2 n + 1 + o( n ) \f$ bits,
 * as Jacobson proves, against the \f$ \Theta( n \log n ) \f$ bits of the
 * pointers. The \f$ 2 n - \Theta( \log n ) \f$ bits needed to tell apart the
 * \f$ C_n \approx 4^n / ( n^{3/2} \sqrt{\pi} ) \f$ binary trees of \f$ n \f$
 * nodes show that the representation is within \f$ O( \log n ) \f$ bits of the
 * optimum.
 *
 * The nodes are numbered \f$ 0 , \ldots , n - 1 \f$ by levels, from the
 * left; the node \f$ v \f$ is at the place \f$ \mathrm{select}_1( v + 1 ) \f$
 * of \f$ B \f$, counted from 0, and the data of the nodes, if any, go in a
 * vector indexed by that number. The representation answers the navigation
 * but not the size of a subtree, which needs the order of a visit in depth
 * [see `BPTree.h`].
 *
 * The contracts [see `Contract.h`]: the lists of the children describe a
 * binary tree rooted at 0, and \f$ B \f$ is the code of a binary tree
 * (\f$ 2 n + 1 \f$ bits, the \f$ x \f$-th 1 before the places \f$ 2 x \f$
 * and \f$ 2 x + 1 \f$, an audit); the nodes are in \f$ [ 0 , n ) \f$. The
 * functions are defined in `JacobsonTree.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see G. Navarro, Compact Data Structures: A Practical Approach,
 *      Cambridge University Press, 2016, chapter 8,
 *      doi:10.1017/CBO9781316588284
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

#ifndef __ADS_JacobsonTree
 #define __ADS_JacobsonTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS JacobsonTree ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary tree in \f$ 2 n + 1 \f$ bits, its nodes by levels
class JacobsonTree {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no node

 /// the empty tree
 JacobsonTree( void );

 /// the tree whose node \f$ u \f$ has the children `c[ u ].first` (left)
 /// and `c[ u ].second` (right), `nil` if missing, rooted at 0
 ///
 /// In pseudocode (the binary trees of Jacobson, beyond the book), a bit
 /// per child, present or not, in a visit in breadth:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{levelCode}( c ) \cr
 /// 2 & \quad B \gets ( 1 )\quad \triangleright\ \text{the root} \cr
 /// 3 & \quad Q \gets \text{an empty queue};\ \mathrm{enqueue}( Q , 0 ) \cr
 /// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 5 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
 /// 6 & \qquad \textbf{foreach}\ w \in ( c[ u ].\mathit{first} , c[ u
 ///     ].\mathit{second} )\ \textbf{do} \cr
 /// 7 & \qquad\quad \textbf{if}\ w = \textbf{nil}\ \textbf{then}\ B \gets B
 ///     \cdot 0\ \textbf{else}\ B \gets B \cdot 1;\ \mathrm{enqueue}( Q , w )
 ///     \cr
 /// 8 & \quad \textbf{return}\ B
 /// \end{array}}
 /// \f]
 /// \pre \f$ c \f$ describes a binary tree rooted at 0
 explicit JacobsonTree( const std::vector< std::pair< std::size_t ,
                                                      std::size_t > > & c );

 /// the tree whose code by levels is \f$ b \f$
 /// \pre \f$ b \f$ is the code of a binary tree (an audit)
 explicit JacobsonTree( BitVector b );

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const;

 /// the root, `nil` if the tree is empty
 [[nodiscard]] std::size_t root( void ) const;

 /// the left child of \f$ v \f$, `nil` if missing: the node at the place
 /// \f$ 2 v + 1 \f$ of \f$ B \f$, counted from 0
 ///
 /// In pseudocode (Jacobson, beyond the book), the places counted from 0
 /// and \f$ \mathrm{rank}_1( q ) \f$ the 1s before \f$ q \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{left}( v ) \cr
 /// 2 & \quad \textbf{if}\ B[ 2 v + 1 ] = 0\ \textbf{then return nil} \cr
 /// 3 & \quad \textbf{return}\ \mathrm{rank}_1( 2 v + 1 )
 /// \end{array}}
 /// \f]
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t left( std::size_t v ) const;

 /// the right child of \f$ v \f$, `nil` if missing: `left()` with the place
 /// \f$ 2 v + 2 \f$
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t right( std::size_t v ) const;

 /// the parent of \f$ v \f$, `nil` for the root
 ///
 /// In pseudocode (Jacobson, beyond the book), with the places counted
 /// from 0 and the ranks of select from 1:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{parent}( v ) \cr
 /// 2 & \quad \textbf{if}\ v = 0\ \textbf{then return nil} \cr
 /// 3 & \quad \textbf{return}\ \lfloor ( \mathrm{select}_1( v + 1 ) + 1 ) /
 ///     2 \rfloor - 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ v < \mathrm{size}() \f$
 /// \post \f$ v \f$ is the left or the right child of the node returned
 [[nodiscard]] std::size_t parent( std::size_t v ) const;

 /// whether \f$ v \f$ has no children
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] bool isLeaf( std::size_t v ) const;

 /// \f$ B \f$, the code by levels
 [[nodiscard]] const BitVector & bits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the node at the place \f$ q \f$ of \f$ B \f$, `nil` if a leaf added
 [[nodiscard]] std::size_t nodeAt( std::size_t q ) const;

 /// whether there are \f$ 2 n + 1 \f$ bits, each node before the places of
 /// its children
 [[nodiscard]] bool valid( void ) const;

 RankSelect f_b;   ///< the code by levels, with its rank and select

 };  // end( class JacobsonTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/JacobsonTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* JacobsonTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File JacobsonTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
