/*--------------------------------------------------------------------------*/
/*---------------------------- File LoudsTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered tree of any degree in \f$ 2 n + 1 \f$ bits by LOUDS, the
 * level-order unary degree sequence of Jacobson (chapter 15, Compressed Data
 * Structures, of Ferragina's book): a dummy root of degree 1 is put above the
 * root, the nodes are visited by levels, and the degree \f$ d \f$ of each one
 * is written in unary as \f$ 0^d 1 \f$, so that the code \f$ B \f$ has
 * \f$ n \f$ 0s, one per node (each one is the child of somebody), and
 * \f$ n + 1 \f$ 1s, one per node and the dummy root, i.e., \f$ 2 n + 1 \f$
 * bits. The \f$ k \f$-th 0 is the \f$ k \f$-th node by levels, and the
 * children of the \f$ k \f$-th node are the 0s after the \f$ ( k + 1 ) \f$-th
 * 1, the 1 of the dummy root counting first; hence, with rank and select [see
 * `RankSelect.h`], in \f$ O( 1 ) \f$ time:
 *
 * - \f$ \mathrm{degree}( x ) = \mathrm{select}_1( x + 1 ) -
 *   \mathrm{select}_1( x ) - 1 \f$;
 * - \f$ \mathrm{parent}( x ) = \mathrm{rank}_1( \mathrm{select}_0( x ) ) =
 *   \mathrm{select}_0( x ) - x \f$;
 * - \f$ \mathrm{firstChild}( x ) = \mathrm{select}_1( x ) + 1 - x \f$,
 *   unless the degree is 0;
 * - \f$ \mathrm{nextSibling}( x ) = x + 1 \f$ if the bit after
 *   \f$ \mathrm{select}_0( x ) \f$ is 0, none otherwise;
 *
 * with the nodes and the places counted from 1, as in the same chapter; here
 * they count from 0 (the root is the node 0, at the place 0 of \f$ B \f$),
 * and the functions are `leftmostChild()` and `rightSibling()`, with the
 * names of the TREE of the book [see `Tree.h`]. The \f$ i \f$-th child is
 * the first one plus \f$ i \f$, since the children of a node are consecutive
 * in the order by levels, which a visit in depth does not give. LOUDS does
 * not answer the size of a subtree nor the depth of a node, which the
 * parentheses of a visit in depth do [see `BPTree.h`], at the price of a
 * range min-max tree; it is the simplest of the succinct trees and the
 * fastest for the navigation from the root down (Arroyuelo, Cánovas,
 * Navarro and Sadakane), e.g., in the tries of the indices.
 *
 * The contracts [see `Contract.h`]: the lists of the children describe a
 * tree rooted at 0, \f$ B \f$ is a code of LOUDS (\f$ n \f$ 0s and
 * \f$ n + 1 \f$ 1s, no prefix with more 0s than 1s before the last 1, an
 * audit), and the nodes are in \f$ [ 0 , n ) \f$. The functions are defined
 * in `LoudsTree.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see D. Arroyuelo, R. Cánovas, G. Navarro, K. Sadakane, Succinct trees
 *      in practice, Proc. 12th ALENEX, 84-97, 2010,
 *      doi:10.1137/1.9781611972900.9
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

#ifndef __ADS_LoudsTree
 #define __ADS_LoudsTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS LoudsTree -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree in \f$ 2 n + 1 \f$ bits by LOUDS, its nodes by levels
class LoudsTree {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no node

 /// the empty tree
 LoudsTree( void );

 /// the tree whose node \f$ u \f$ has the children \f$ c[ u ] \f$, in order,
 /// rooted at 0
 ///
 /// In pseudocode (the LOUDS of Jacobson, beyond the book), the degrees in
 /// unary by a visit in breadth:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{louds}( c ) \cr
 /// 2 & \quad B \gets ( 0 , 1 )\quad \triangleright\ \text{the dummy root,}
 ///     \text{ of degree 1} \cr
 /// 3 & \quad Q \gets \text{an empty queue};\ \mathrm{enqueue}( Q , 0 ) \cr
 /// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 5 & \qquad u \gets \mathrm{dequeue}( Q ) \cr
 /// 6 & \qquad \textbf{foreach}\ w \in c[ u ]\ \textbf{do}\ B \gets B \cdot
 ///     0;\ \mathrm{enqueue}( Q , w ) \cr
 /// 7 & \qquad B \gets B \cdot 1 \cr
 /// 8 & \quad \textbf{return}\ B
 /// \end{array}}
 /// \f]
 /// \pre \f$ c \f$ describes a tree rooted at 0
 explicit LoudsTree( const std::vector< std::vector< std::size_t > > & c );

 /// the tree whose code is \f$ b \f$
 /// \pre \f$ b \f$ is a code of LOUDS (an audit)
 explicit LoudsTree( BitVector b );

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const;

 /// the root, `nil` if the tree is empty
 [[nodiscard]] std::size_t root( void ) const;

 /// the number of children of \f$ v \f$, the 0s between the
 /// \f$ ( v + 1 ) \f$-th 1 and the next, i.e.,
 /// \f$ \mathrm{select}_1( v + 2 ) - \mathrm{select}_1( v + 1 ) - 1 \f$
 /// with the ranks counted from 1
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t degree( std::size_t v ) const;

 /// the parent of \f$ v \f$, `nil` for the root, i.e.,
 /// \f$ \mathrm{select}_0( v + 1 ) - v - 1 \f$, the 1s before the 0 of
 /// \f$ v \f$ less the one of the dummy root
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t parent( std::size_t v ) const;

 /// the first child of \f$ v \f$, `nil` for a leaf, i.e.,
 /// \f$ \mathrm{select}_1( v + 1 ) - v \f$ when a 0 follows the
 /// \f$ ( v + 1 ) \f$-th 1
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t leftmostChild( std::size_t v ) const;

 /// the next sibling of \f$ v \f$, `nil` for the last one, i.e.,
 /// \f$ v + 1 \f$ when a 0 follows the 0 of \f$ v \f$
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t rightSibling( std::size_t v ) const;

 /// the child \f$ i \f$ of \f$ v \f$, from 0, i.e.,
 /// \f$ \mathrm{leftmostChild}( v ) + i \f$
 /// \pre \f$ v < \mathrm{size}() \f$ and \f$ i < \mathrm{degree}( v ) \f$
 [[nodiscard]] std::size_t child( std::size_t v , std::size_t i ) const;

 /// whether \f$ v \f$ has no children
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] bool isLeaf( std::size_t v ) const;

 /// \f$ B \f$, the code
 [[nodiscard]] const BitVector & bits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// whether there are \f$ n \f$ 0s and \f$ n + 1 \f$ 1s, ending in 1, the
 /// \f$ k \f$-th 0 after the \f$ k \f$-th 1
 [[nodiscard]] bool valid( void ) const;

 RankSelect f_b;   ///< the code, with its rank and select

 };  // end( class LoudsTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/LoudsTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LoudsTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File LoudsTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
