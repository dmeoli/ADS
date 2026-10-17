/*--------------------------------------------------------------------------*/
/*---------------------------- File CellHeap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A heap of cells of two fields with a garbage collector, beyond the book:
 * the memory of a LISP, where a field holds nil, an atom or the address of
 * a cell, and the cells not reachable from the roots are garbage. collect()
 * marks the reachable cells and sweeps the others onto the free list, and
 * cons() collects when the free list is empty. The marking is that of
 * Deutsch, Schorr and Waite (Algorithm 2.3.5E): the depth-first visit keeps
 * its path in the fields of the cells themselves, reversing each pointer it
 * follows and restoring it on the way back, with one bit per cell telling
 * which field holds the reversed pointer; it needs no stack, which a
 * collector cannot count on when the memory is exhausted, and takes time
 * linear in the cells reached. A recursive marking with a stack is kept for
 * the comparison.
 *
 * The contracts [see Contract.h]: an address is a cell of the heap; cons()
 * needs a free cell after a collection; collect() ensures that it frees
 * exactly the cells that are not reachable and leaves the fields of the
 * others as they were, an audit. The operations are defined in
 * CellHeap.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.3.5
 * \see H. Schorr, W. M. Waite, An efficient machine-independent procedure
 *      for garbage collection in various list structures, Communications of
 *      the ACM 10(8), 501-506, 1967, doi:10.1145/363534.363554
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

#ifndef __ADS_CellHeap
 #define __ADS_CellHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <set>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS CellHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a heap of cells of two fields, with a mark-and-sweep collector
class CellHeap {

/*--------------------------------------------------------------------------*/
 public:

 /// a field: nil, an atom (atom( k ) for k >= 0) or a cell (its address)
 using Value = long;

 static constexpr Value nil = -1;

 /// the atom k
 /// \pre k >= 0
 static Value atom( long k );

 [[nodiscard]] static bool isCell( Value v );

 /// a heap of n cells, all free
 /// \pre n >= 1
 explicit CellHeap( std::size_t n );

 /// a new cell with the fields car and cdr, after a collection if no cell
 /// is free
 /// \pre a cell is free after the collection
 Value cons( Value car , Value cdr );

 /// \pre c is a cell of the heap
 [[nodiscard]] Value car( Value c ) const;
 [[nodiscard]] Value cdr( Value c ) const;
 void setCar( Value c , Value v );
 void setCdr( Value c , Value v );

 /// c is a root
 void addRoot( Value c );
 void removeRoot( Value c );

 /// marks the cells reachable from the roots, by pointer reversal or with
 /// a stack, and frees the others; the cells freed
 /// \post the cells freed are the unreachable ones, the fields of the
 ///       others unchanged (an audit)
 std::size_t collect( bool reversal = true );

 [[nodiscard]] std::size_t freeCells( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  Value car = nil , cdr = nil;
  bool mark = false;
  bool cdrSide = false;    ///< the reversed pointer is in cdr
  bool free = true;
  };

 void markReversal( Value root );
 void markStack( Value root );

 /// the cells reachable from the roots, by a plain visit
 [[nodiscard]] std::vector< bool > reachable( void ) const;

 std::vector< Cell > f_C;
 Value f_free = nil;        ///< the free list, through car
 std::size_t f_nfree = 0;
 std::multiset< Value > f_roots;

 };  // end( class CellHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/CellHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CellHeap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CellHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
