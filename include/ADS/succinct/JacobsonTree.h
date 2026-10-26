/*--------------------------------------------------------------------------*/
/*--------------------------- File JacobsonTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary tree of Jacobson in 2n + 1 bits (section 15.2.1 of the notes):
 * the tree of n nodes is completed with a leaf in place of each missing
 * child, its 2n + 1 nodes are visited by levels, and B gets a 1 for a node
 * of the tree and a 0 for a leaf added. With the places counted from 1, the
 * children of the x-th node by levels are at the places 2x and 2x + 1, as in
 * a heap, since each node adds its two children to the visit; the node at
 * the place p is the \f$ Rank_1( p ) \f$-th one, and the x-th node is at
 * the place \f$ Select_1( x ) \f$, a child of the \f$ \lfloor Select_1( x
 * ) / 2 \rfloor \f$-th one. Hence, with rank and select in O( 1 ) time
 * [see RankSelect.h], left(), right() and parent() take O( 1 ) time, in
 * 2n + 1 + o( n ) bits (Theorem 15.6), against the \f$ \Theta( n \log n )
 * \f$ bits of the pointers. The \f$ 2n - \Theta( \log n ) \f$ bits needed
 * to tell apart the
 * \f$ C_n \approx 4^n / ( n^{3/2} \sqrt{\pi} ) \f$ binary trees of n nodes
 * show that the representation is within O( log n ) bits of the optimum.
 *
 * The nodes are numbered 0 , ... , n - 1 by levels, from the left; node v
 * is at the place \f$ select_1( v + 1 ) \f$ of B, counted from 0, and the
 * data of the nodes, if any, go in a vector indexed by that number. The
 * representation answers the navigation but not the size of a subtree,
 * which needs the order of a visit in depth [see BPTree.h].
 *
 * The contracts [see Contract.h]: the children lists describe a binary tree
 * rooted at 0, and B is the code of a binary tree (2n + 1 bits, the x-th
 * 1 before the places 2x and 2x + 1, an audit); the nodes are in
 * [ 0 , n ). The functions
 * are defined in JacobsonTree.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
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

/// a binary tree in 2n + 1 bits, its nodes by levels
class JacobsonTree {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no node

 /// the empty tree
 JacobsonTree( void );

 /// the tree whose node u has the children c[ u ].first (left) and
 /// c[ u ].second (right), nil if missing, rooted at 0
 /// \pre c describes a binary tree rooted at 0
 explicit JacobsonTree( const std::vector< std::pair< std::size_t ,
                                                      std::size_t > > & c );

 /// the tree whose code by levels is b
 /// \pre b is the code of a binary tree (an audit)
 explicit JacobsonTree( BitVector b );

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const;

 /// the root, nil if the tree is empty
 [[nodiscard]] std::size_t root( void ) const;

 /// \pre v < size()
 [[nodiscard]] std::size_t left( std::size_t v ) const;

 /// \pre v < size()
 [[nodiscard]] std::size_t right( std::size_t v ) const;

 /// nil for the root
 /// \pre v < size()
 [[nodiscard]] std::size_t parent( std::size_t v ) const;

 /// \pre v < size()
 [[nodiscard]] bool isLeaf( std::size_t v ) const;

 /// B, the code by levels
 [[nodiscard]] const BitVector & bits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the node at the place q of B, nil if a leaf added
 [[nodiscard]] std::size_t nodeAt( std::size_t q ) const;

 /// 2n + 1 bits, each node before the places of its children
 [[nodiscard]] bool valid( void ) const;

 RankSelect f_b;

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
