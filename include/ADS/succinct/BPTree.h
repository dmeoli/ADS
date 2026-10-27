/*--------------------------------------------------------------------------*/
/*----------------------------- File BPTree.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered tree of any degree in \f$ 2 n \f$ bits by its balanced
 * parentheses: a visit in depth writes an open parenthesis when it enters a
 * node and a closed one when it leaves it, so that a node is the pair of its
 * parentheses and its subtree is what they enclose (the code with which
 * chapter 15, Compressed Data Structures, of Ferragina's book counts the trees
 * of \f$ n \f$ nodes). The nodes are numbered in preorder, the node \f$ v \f$
 * being the \f$ ( v + 1 ) \f$-th open parenthesis, at the place
 * \f$ \mathrm{select}_1( v + 1 ) \f$, and with the matching of the parentheses
 * [see `BalancedParentheses.h`]:
 *
 * - `parent( v )` is `enclose()` of its open parenthesis,
 *   `leftmostChild( v )` is \f$ v + 1 \f$ if the next parenthesis is an
 *   open one, and `rightSibling( v )` the open parenthesis after its closed
 *   one, if any;
 * - `subtreeSize( v )` is \f$ ( j - i + 1 ) / 2 \f$ for its parentheses at
 *   \f$ i \f$ and \f$ j \f$, `depth( v )` is the excess at its open
 *   parenthesis less 1, and \f$ u \f$ is an ancestor of \f$ v \f$ if the
 *   open parenthesis of \f$ v \f$ is between the parentheses of \f$ u \f$;
 *
 * in the time of a search in the range min-max tree,
 * \f$ O( \log ( n / 8192 ) ) \f$ plus the scan of a group and of a leaf, and
 * in the time of a select and of a rank for `leftmostChild()`, `depth()` and
 * `isLeaf()`. Against LOUDS [see `LoudsTree.h`], the parentheses also give
 * the size of a subtree, the depth and the ancestors, and number the nodes
 * of a subtree consecutively (from \f$ v \f$ to
 * \f$ v + \mathrm{subtreeSize}( v ) - 1 \f$), at the price of the searches;
 * DFUDS of Benoit, Demaine, Munro, Raman, Raman and Rao, the degrees in
 * unary in preorder, also gives the \f$ i \f$-th child in \f$ O( 1 ) \f$
 * time with the same matching, which the same chapter does not treat.
 *
 * The contracts [see `Contract.h`]: the lists of the children describe a
 * tree rooted at 0, the parentheses are balanced and enclose one tree, and
 * the nodes are in \f$ [ 0 , n ) \f$. The functions are defined in
 * `BPTree.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see J. I. Munro, V. Raman, Succinct representation of balanced
 *      parentheses and static trees, SIAM Journal on Computing 31(3),
 *      762-776, 2001, doi:10.1137/S0097539799364092
 * \see D. Benoit, E. D. Demaine, J. I. Munro, R. Raman, V. Raman, S. S. Rao,
 *      Representing trees of higher degree, Algorithmica 43(4), 275-292,
 *      2005, doi:10.1007/s00453-004-1146-6
 * \see G. Navarro, K. Sadakane, Fully functional static and dynamic
 *      succinct trees, ACM Transactions on Algorithms 10(3), 16, 2014,
 *      doi:10.1145/2601073
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

#ifndef __ADS_BPTree
 #define __ADS_BPTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/BitVector.h"

#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS BPTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree in \f$ 2 n \f$ bits by balanced parentheses, its nodes in
/// preorder
class BPTree {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no node

 /// the empty tree
 BPTree( void ) = default;

 /// the tree whose node \f$ u \f$ has the children \f$ c[ u ] \f$, in order,
 /// rooted at 0
 ///
 /// In pseudocode (the parentheses of Munro and Raman, beyond the book), by
 /// a visit in depth with an explicit stack of pairs (node, next child):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{parentheses}( c ) \cr
 /// 2 & \quad B \gets ( 1 );\ S \gets \text{the stack } ( ( 0 , 0 ) ) \cr
 /// 3 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
 /// 4 & \qquad ( u , k ) \gets \mathrm{top}( S ) \cr
 /// 5 & \qquad \textbf{if}\ k < | c[ u ] |\ \textbf{then} \cr
 /// 6 & \qquad\quad w \gets c[ u ][ k ];\ \mathrm{top}( S ) \gets ( u , k + 1
 ///     ) \cr
 /// 7 & \qquad\quad B \gets B \cdot 1;\ \mathrm{push}( S , ( w , 0 ) )\quad
 ///     \triangleright\ \text{enter } w \cr
 /// 8 & \qquad \textbf{else} \cr
 /// 9 & \qquad\quad B \gets B \cdot 0;\ \mathrm{pop}( S )\quad
 ///     \triangleright\ \text{leave } u \cr
 /// 10 & \quad \textbf{return}\ B
 /// \end{array}}
 /// \f]
 /// \pre \f$ c \f$ describes a tree rooted at 0
 explicit BPTree( const std::vector< std::vector< std::size_t > > & c );

 /// the tree whose parentheses are \f$ b \f$
 /// \pre \f$ b \f$ is balanced and encloses one tree (an audit)
 explicit BPTree( BitVector b );

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const;

 /// the root, `nil` if the tree is empty
 [[nodiscard]] std::size_t root( void ) const;

 /// the parent of \f$ v \f$, `nil` for the root
 ///
 /// In pseudocode (Navarro and Sadakane, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{parent}( v ) \cr
 /// 2 & \quad q \gets \mathrm{enclose}( \mathrm{select}_1( v + 1 ) ) \cr
 /// 3 & \quad \textbf{if}\ q = \textbf{nil}\ \textbf{then return nil} \cr
 /// 4 & \quad \textbf{return}\ \mathrm{rank}_1( q )
 /// \end{array}}
 /// \f]
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t parent( std::size_t v ) const;

 /// the first child of \f$ v \f$, `nil` for a leaf, which is \f$ v + 1 \f$
 /// when an open parenthesis follows that of \f$ v \f$
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t leftmostChild( std::size_t v ) const;

 /// the next sibling of \f$ v \f$, `nil` for the last one
 ///
 /// In pseudocode (Navarro and Sadakane, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rightSibling}( v ) \cr
 /// 2 & \quad q \gets \mathrm{findClose}( \mathrm{select}_1( v + 1 ) ) + 1
 ///     \cr
 /// 3 & \quad \textbf{if}\ q < | B |\ \textbf{and}\ B[ q ] = 1
 ///     \ \textbf{then return}\ \mathrm{rank}_1( q ) \cr
 /// 4 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t rightSibling( std::size_t v ) const;

 /// the number of children of \f$ v \f$, by a walk along them with
 /// `leftmostChild()` and `rightSibling()`
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t degree( std::size_t v ) const;

 /// the nodes of the subtree of \f$ v \f$, \f$ v \f$ included, as
 /// \f$ ( \mathrm{findClose}( p ) - p + 1 ) / 2 \f$ for the open parenthesis
 /// \f$ p \f$ of \f$ v \f$
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t subtreeSize( std::size_t v ) const;

 /// the edges from the root to \f$ v \f$, the excess at its open parenthesis
 /// less 1
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] std::size_t depth( std::size_t v ) const;

 /// whether \f$ u \f$ is \f$ v \f$ or one of its ancestors, i.e., whether
 /// \f$ u \le v < u + \mathrm{subtreeSize}( u ) \f$
 /// \pre \f$ u , v < \mathrm{size}() \f$
 [[nodiscard]] bool isAncestor( std::size_t u , std::size_t v ) const;

 /// whether \f$ v \f$ has no children
 /// \pre \f$ v < \mathrm{size}() \f$
 [[nodiscard]] bool isLeaf( std::size_t v ) const;

 /// the parentheses, with their matching
 [[nodiscard]] const BalancedParentheses & parentheses( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the place of the open parenthesis of \f$ v \f$
 [[nodiscard]] std::size_t open( std::size_t v ) const;

 BalancedParentheses f_p;   ///< the parentheses of the visit in depth

 };  // end( class BPTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/BPTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BPTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File BPTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
