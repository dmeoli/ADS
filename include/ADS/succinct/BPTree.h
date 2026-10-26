/*--------------------------------------------------------------------------*/
/*----------------------------- File BPTree.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered tree of any degree in 2n bits by its balanced parentheses: a
 * visit in depth writes '(' when it enters a node and ')' when it leaves it,
 * so that a node is the pair of its parentheses and its subtree is what they
 * enclose (the code that the notes use to count the trees of n nodes, the
 * footnote of section 15.2.1). The nodes are numbered in preorder, node v
 * being the (v + 1)-th '(' at the place \f$ select_1( v + 1 ) \f$, and with
 * the matching of the parentheses [see BalancedParentheses.h]:
 *
 * - parent( v ) is enclose() of its '(', leftmostChild( v ) is v + 1 if the
 *   next parenthesis is a '(', and rightSibling( v ) the '(' after its ')',
 *   if any;
 * - subtreeSize( v ) is half the distance from its '(' to its ')', depth( v )
 *   is the excess at its '(' less 1, and u is an ancestor of v if the '(' of
 *   v is between the parentheses of u;
 *
 * in the time of a search in the range min-max tree,
 * \f$ O( \log ( n / 512 ) ) \f$ plus the scan of a block, and \f$ O( 1 ) \f$
 * for leftmostChild(), depth() and isLeaf(). Against LOUDS [see LoudsTree.h],
 * the parentheses also give the size of a subtree, the depth and the
 * ancestors, and number the nodes of a subtree consecutively (from v to v +
 * subtreeSize( v ) - 1), at the price of the searches; DFUDS of Benoit,
 * Demaine, Munro, Raman, Raman and Rao, the degrees in unary in preorder, also
 * gives the i-th child in \f$ O( 1 ) \f$ time with the same matching, which
 * the notes do not treat.
 *
 * The contracts [see Contract.h]: the children lists describe a tree rooted
 * at 0, the parentheses are balanced and enclose one tree, and the nodes
 * are in [ 0 , n ). The functions are defined in BPTree.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
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

/// an ordered tree in 2n bits by balanced parentheses, its nodes in
/// preorder
class BPTree {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no node

 /// the empty tree
 BPTree( void ) = default;

 /// the tree whose node u has the children c[ u ], in order, rooted at 0
 /// \pre c describes a tree rooted at 0
 explicit BPTree( const std::vector< std::vector< std::size_t > > & c );

 /// the tree whose parentheses are b
 /// \pre b is balanced and encloses one tree (an audit)
 explicit BPTree( BitVector b );

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const;

 /// the root, nil if the tree is empty
 [[nodiscard]] std::size_t root( void ) const;

 /// nil for the root
 /// \pre v < size()
 [[nodiscard]] std::size_t parent( std::size_t v ) const;

 /// the first child, nil for a leaf
 /// \pre v < size()
 [[nodiscard]] std::size_t leftmostChild( std::size_t v ) const;

 /// the next sibling, nil for the last one
 /// \pre v < size()
 [[nodiscard]] std::size_t rightSibling( std::size_t v ) const;

 /// the number of children, by a walk along them
 /// \pre v < size()
 [[nodiscard]] std::size_t degree( std::size_t v ) const;

 /// the nodes of the subtree of v, v included
 /// \pre v < size()
 [[nodiscard]] std::size_t subtreeSize( std::size_t v ) const;

 /// the edges from the root
 /// \pre v < size()
 [[nodiscard]] std::size_t depth( std::size_t v ) const;

 /// whether u is v or one of its ancestors
 /// \pre u , v < size()
 [[nodiscard]] bool isAncestor( std::size_t u , std::size_t v ) const;

 /// \pre v < size()
 [[nodiscard]] bool isLeaf( std::size_t v ) const;

 /// the parentheses, with their matching
 [[nodiscard]] const BalancedParentheses & parentheses( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the place of the '(' of v
 [[nodiscard]] std::size_t open( std::size_t v ) const;

 BalancedParentheses f_p;

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
