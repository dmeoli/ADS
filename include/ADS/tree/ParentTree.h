/*--------------------------------------------------------------------------*/
/*--------------------------- File ParentTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree as a vector of parents: the nodes are \f$ 0 , \ldots , n - 1 \f$,
 * and \f$ p[ i ] \f$ is the parent of \f$ i \f$, or -1 if \f$ i \f$ is a
 * root. It answers `parent()` alone, in \f$ O( 1 ) \f$ time and with the
 * least memory, which is all that is needed when a tree is built and walked
 * towards the root, as the trees of the visits of a graph; the order among
 * siblings is lost, unless it is the one of their numbers. Since
 * `setParent()` may hang any node anywhere, the structure is a forest of
 * \f$ n \f$ isolated nodes at the start.
 *
 * The contracts [see `Contract.h`]: the nodes are in \f$ [ 0 , n ) \f$, and
 * a node is not its own parent. The operations are defined in
 * `ParentTree.tpp`.
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

#ifndef __ADS_ParentTree
 #define __ADS_ParentTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ParentTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a tree (a forest) of the nodes \f$ 0 , \ldots , n - 1 \f$ as a vector of
/// parents
class ParentTree {

/*--------------------------------------------------------------------------*/
 public:

 /// a forest of \f$ n \f$ isolated nodes (the `Tree( n )` of the book)
 /// \pre \f$ n \ge 0 \f$
 explicit ParentTree( int n );

 /// the parent of the node \f$ i \f$; -1 if \f$ i \f$ is a root
 /// \pre \f$ i \f$ is a node
 [[nodiscard]] int parent( int i ) const;

 /// makes the node \f$ i \f$ a child of the node \f$ j \f$, or a root if
 /// \f$ j = -1 \f$, in \f$ O( 1 ) \f$ (section 5.4, \f$ p[ i ] \gets j \f$)
 /// \pre \f$ i \f$ and \f$ j \f$ are nodes, or \f$ j = -1 \f$, and
 ///      \f$ i \ne j \f$; \f$ j \f$ is not in the subtree of \f$ i \f$
 ///      (which would make a cycle)
 void setParent( int i , int j );

 /// the number of nodes
 [[nodiscard]] int size( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// true if the node \f$ j \f$ is in the subtree of \f$ i \f$,
 /// \f$ O( \text{the depth of } j ) \f$
 [[nodiscard]] bool below( int j , int i ) const;

 std::vector< int > f_p;   ///< the vector of parents

 };  // end( class ParentTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ParentTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParentTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ParentTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
