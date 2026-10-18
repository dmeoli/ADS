/*--------------------------------------------------------------------------*/
/*--------------------------- File ParentTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree as a vector of parents: the nodes are 0 , ... , n - 1, and p[ i ]
 * is the parent of i, or -1 if i is a root. It answers parent() alone, in
 * O( 1 ) time and with the least memory, which is all that is needed when a
 * tree is built and walked towards the root, as the trees of the visits of a
 * graph; the order among siblings is lost, unless it is the one of their
 * numbers. Since setParent() may hang any node anywhere, the structure is a
 * forest of n isolated nodes at the start.
 *
 * The contracts [see Contract.h]: the nodes are in [ 0 , n ), and a node
 * is not its own parent. The operations are defined in ParentTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

/// a tree (a forest) of the nodes 0 , ... , n - 1 as a vector of parents
class ParentTree {

/*--------------------------------------------------------------------------*/
 public:

 /// Tree( n ): a forest of n isolated nodes
 /// \pre n >= 0
 explicit ParentTree( int n );

 /// the parent of the node i; -1 if i is a root
 [[nodiscard]] int parent( int i ) const;

 /// makes the node i a child of the node j, or a root if j = -1
 /// \pre i and j are nodes, or j = -1 , and i != j
 void setParent( int i , int j );

 /// the number of nodes
 [[nodiscard]] int size( void ) const;

/*--------------------------------------------------------------------------*/
 private:

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
