/*--------------------------------------------------------------------------*/
/*------------------------ File TreePriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a tree.
 *
 * TreePriorityQueue is a Red-Black tree of the pairs ( priority , number
 * of insertion ) (section 10.1.2): every operation in O( log n ).
 *
 * The contracts [see Contract.h]: min() and deleteMin() need a queue that
 * is not empty, decrease() the handle of an item still in the queue and a
 * priority not larger than its own; the invariant is that each item of the
 * tree has the priority recorded for its handle. The operations are
 * defined in TreePriorityQueue.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#ifndef __ADS_TreePriorityQueue
 #define __ADS_TreePriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/tree/SearchTree.h"

#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS TreePriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a Red-Black tree
template< class T , std::totally_ordered P = int >
class TreePriorityQueue {

 using Key = std::pair< P , long >;   ///< priority, number of insertion

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;        ///< the type of the items
 using Priority = P;    ///< the type of the priorities
 using Handle = long;   ///< the number of insertion of an item

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] int size( void ) const;

 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// \pre the queue is not empty
 /// \post one item less
 T deleteMin( void );

 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// \pre y is an item still in the queue and p is not larger than its
 /// priority
 void decrease( Handle y , const P & p );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( ( priority , number of insertion ) ,
 // item ) in increasing order of priority, bidirectional, those of the tree

 using value_type = typename RedBlackTree< Key , T >::value_type;
 using const_iterator = typename RedBlackTree< Key , T >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_t.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_t.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// each item of the tree with the priority of its handle, O( n )
 [[nodiscard]] bool consistent( void ) const;

 RedBlackTree< Key , T > f_t;   ///< the items by priority
 std::vector< P > f_p;          ///< the priority of each insertion

 };  // end( class TreePriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/TreePriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TreePriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File TreePriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
