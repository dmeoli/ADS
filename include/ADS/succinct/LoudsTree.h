/*--------------------------------------------------------------------------*/
/*---------------------------- File LoudsTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ordered tree of any degree in 2n + 1 bits by LOUDS, the level-order
 * unary degree sequence of Jacobson (section 15.2.2 of the notes): a dummy
 * root of degree 1 is put above the root, the nodes are visited by levels,
 * and the degree d of each one is written in unary as \f$ 0^d 1 \f$, so
 * that B has n 0s, one per node (each one is the child of somebody), and
 * n + 1 1s, one per node and the dummy root (Theorem 15.7). The k-th 0 is
 * the k-th node by levels, and the children of the k-th node are the 0s
 * after the (k + 1)-th 1, the 1 of the dummy root counting first; hence,
 * with rank and select [see RankSelect.h], in O( 1 ) time:
 *
 * - \f$ degree( x ) = Select_1( x + 1 ) - Select_1( x ) - 1 \f$;
 * - \f$ parent( x ) = Rank_1( Select_0( x ) ) = Select_0( x ) - x \f$;
 * - \f$ firstChild( x ) = Select_1( x ) + 1 - x \f$, unless the degree is 0;
 * - \f$ nextSibling( x ) = x + 1 \f$ if the bit after \f$ Select_0( x ) \f$
 *   is 0, none otherwise;
 *
 * with the nodes and the places counted from 1, as in the notes; here they
 * count from 0 (the root is node 0, at the place 0 of B), and the
 * functions are leftmostChild() and rightSibling(), with the names of the
 * TREE of the book [see Tree.h]. The i-th child is the first one plus i,
 * since the children of a node are consecutive in the order by levels,
 * which a visit in depth does not give. LOUDS does not answer the size of
 * a subtree nor the depth of a node, which the parentheses of a visit in
 * depth do [see BPTree.h], at the price of a range min-max tree; it is the
 * simplest of the succinct trees and the fastest for the navigation from
 * the root down (Arroyuelo, Canovas, Navarro and Sadakane), e.g., in the
 * tries of the indices.
 *
 * The contracts [see Contract.h]: the children lists describe a tree rooted
 * at 0, B is a code of LOUDS (n 0s and n + 1 1s, no prefix with more 0s
 * than 1s before the last 1, an audit), and the nodes are in [ 0 , n ).
 * The functions are defined in LoudsTree.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see D. Arroyuelo, R. Canovas, G. Navarro, K. Sadakane, Succinct trees in
 *      practice, Proc. 12th ALENEX, 84-97, 2010,
 *      doi:10.1137/1.9781611972900.9
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

/// an ordered tree in 2n + 1 bits by LOUDS, its nodes by levels
class LoudsTree {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no node

 /// the empty tree
 LoudsTree( void );

 /// the tree whose node u has the children c[ u ], in order, rooted at 0
 /// \pre c describes a tree rooted at 0
 explicit LoudsTree( const std::vector< std::vector< std::size_t > > & c );

 /// the tree whose code is b
 /// \pre b is a code of LOUDS (an audit)
 explicit LoudsTree( BitVector b );

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const;

 /// the root, nil if the tree is empty
 [[nodiscard]] std::size_t root( void ) const;

 /// \pre v < size()
 [[nodiscard]] std::size_t degree( std::size_t v ) const;

 /// nil for the root
 /// \pre v < size()
 [[nodiscard]] std::size_t parent( std::size_t v ) const;

 /// the first child, nil for a leaf
 /// \pre v < size()
 [[nodiscard]] std::size_t leftmostChild( std::size_t v ) const;

 /// the next sibling, nil for the last one
 /// \pre v < size()
 [[nodiscard]] std::size_t rightSibling( std::size_t v ) const;

 /// the child i, from 0
 /// \pre v < size() and i < degree( v )
 [[nodiscard]] std::size_t child( std::size_t v , std::size_t i ) const;

 /// \pre v < size()
 [[nodiscard]] bool isLeaf( std::size_t v ) const;

 /// B, the code
 [[nodiscard]] const BitVector & bits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// n 0s, n + 1 1s, ending in 1, the k-th 0 after the k-th 1
 [[nodiscard]] bool valid( void ) const;

 RankSelect f_b;

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
