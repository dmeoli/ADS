/*--------------------------------------------------------------------------*/
/*------------------------ File ListPriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a list.
 *
 * ListPriorityQueue is the lists of section 10.1.1: sorted, min() and
 * deleteMin() in O( 1 ), insert() and decrease() in O( n ); unsorted, the
 * other way round. The handle is the position of the cell.
 *
 * The contracts [see Contract.h]: min() and deleteMin() need a queue that
 * is not empty, decrease() a handle of an item of this queue (checked in
 * O( n ) with the invariants) and a priority not larger than its own;
 * after deleteMin() no item has a smaller priority than the one removed;
 * the invariant is that the list is sorted when it must be and that size()
 * counts its cells. The operations are defined in ListPriorityQueue.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#ifndef __ADS_ListPriorityQueue
 #define __ADS_ListPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ListPriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a list, sorted by priority or not
template< class T , std::totally_ordered P = int , bool Sorted = true >
class ListPriorityQueue {

 using Pair = std::pair< P , T >;

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;                                ///< the type of the items
 using Priority = P;                            ///< the type of priorities
 using Handle = typename List< Pair >::Pos;     ///< the cell of an item

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] int size( void ) const;

 /// O( 1 ) if sorted, O( n ) otherwise
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// O( 1 ) if sorted, O( n ) otherwise
 /// \pre the queue is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

 /// O( n ) if sorted, O( 1 ) otherwise
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// O( n ) if sorted, O( 1 ) otherwise; the handle stays valid
 /// \pre y is an item of this queue and p is not larger than its priority
 void decrease( Handle & y , const P & p );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( priority , item ), by priority when the
 // list is sorted, bidirectional; a change through it would break the order

 using value_type = Pair;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const Pair &;
 using const_reference = const Pair &;
 using iterator = typename List< Pair >::const_iterator;
 using const_iterator = typename List< Pair >::const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the cell of smallest priority
 [[nodiscard]] Handle first( void ) const;

 /// the position of the first item of priority larger than p
 [[nodiscard]] Handle after( const P & p ) const;

 /// whether y is a cell of the list, O( n )
 [[nodiscard]] bool owns( Handle y ) const;

 /// sorted if it must be, and size() its length, O( n )
 [[nodiscard]] bool consistent( void ) const;

 List< Pair > f_L;   ///< the items
 int f_size = 0;     ///< their number

 };  // end( class ListPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/ListPriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ListPriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
