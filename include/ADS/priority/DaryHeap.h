/*--------------------------------------------------------------------------*/
/*---------------------------- File DaryHeap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A d-ary heap of plain items, beyond the book: the heap of section 10.1
 * with D children per node in a vector, the children of the node i in the
 * places \f$ D i + 1 , \ldots , D i + D \f$ [see dary_child()], ordered by
 * a predicate better( a , b ), true if a must come out before b. The items
 * are meant to be small and to carry their keys (e.g., f, g and a pointer
 * to the node of a search), so that a comparison never follows a pointer;
 * with D = 4 the heap is half as deep as a binary one, the four children
 * of a node lie in one line of cache, and a push moves up half as many
 * levels, at the price of D - 1 more comparisons per level on the way down.
 * There are no handles and no decrease-key: an item whose key improves is
 * pushed again and the stale copy skipped (lazy deletion), which is what
 * makes the items plain and the heap fast. HeapPriorityQueue [see
 * HeapPriorityQueue.h] is the realization with handles of the book.
 *
 * The contracts [see Contract.h]: D >= 2, top() and pop() need a heap that
 * is not empty; the invariant is the order of the heap, no item better
 * than its parent. The operations are defined in DaryHeap.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see D. B. Johnson, Priority queues with update and finding minimum
 *      spanning trees, Information Processing Letters 4(3), 53-57, 1975,
 *      doi:10.1016/0020-0190(75)90001-0
 * \see A. LaMarca, R. E. Ladner, The influence of caches on the
 *      performance of heaps, ACM Journal of Experimental Algorithmics 1,
 *      article 4, 1996, doi:10.1145/235141.235145
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

#ifndef __ADS_DaryHeap
 #define __ADS_DaryHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DaryHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a D-ary heap of plain items, the best one by better() on top
template< class T , class Better = std::less< T > , unsigned D = 4 >
class DaryHeap {

 static_assert( D >= 2 , "a heap needs at least two children per node" );

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 explicit DaryHeap( Better better = Better() );

 /// pushes x, moving it up while better than its parent
 void push( const T & x );

 /// the best item
 /// \pre the heap is not empty
 [[nodiscard]] const T & top( void ) const;

 /// removes the best item and returns it, the last item moved down from
 /// the root
 /// \pre the heap is not empty
 T pop( void );

 [[nodiscard]] bool isEmpty( void ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// room for n items, so that the pushes up to n do not reallocate
 void reserve( std::size_t n );

 void clear( void );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of the heap (the root first,
 // the rest in no useful order), contiguous; a change through it would
 // break the heap

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using iterator = const T *;
 using const_iterator = const T *;

 [[nodiscard]] const_iterator begin( void ) const { return( f_A.data() ); }
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A.data() + f_A.size() );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 // the hole at i moves up (down) until x fits in it; x comes as a value,
 // never stored and read back, which would stall the forwarding of the
 // store to the load

 void up( std::size_t i , T x );

 void down( std::size_t i , T x );

 /// no item better than its parent, O( n )
 [[nodiscard]] bool ordered( void ) const;

 std::vector< T > f_A;   ///< the heap, the root in f_A[ 0 ]
 Better f_better;        ///< the order

 };  // end( class DaryHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/DaryHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DaryHeap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File DaryHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
