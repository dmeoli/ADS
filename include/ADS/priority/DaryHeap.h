/*--------------------------------------------------------------------------*/
/*---------------------------- File DaryHeap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A \f$ D \f$-ary heap of plain items, beyond the book: the heap of section
 * 10.1 with \f$ D \f$ children per node in a vector, the children of the node
 * \f$ i \f$ in the places \f$ D i + 1 , \ldots , D i + D \f$ [see
 * `BinaryTree.h`], ordered by a predicate `better( a , b )`, true if
 * \f$ a \f$ must come out before \f$ b \f$. The items are meant to be small
 * and to carry their keys (e.g., \f$ f \f$, \f$ g \f$ and a pointer to the
 * node of a search), so that a comparison never follows a pointer; with
 * \f$ D = 4 \f$ the heap is half as deep as a binary one, the four children
 * of a node lie in one line of cache, and a push moves up half as many
 * levels, at the price of \f$ D - 1 \f$ more comparisons per level on the way
 * down. With `BottomUp`, `pop()` moves the hole of the root down to a leaf by
 * the best children, \f$ D - 1 \f$ comparisons per level and none with the
 * item that fills it, and then moves the last item up from there (the
 * bottom-up heapsort of Floyd and Wegener): it pays when the last item, as
 * in a heap full of stale copies, would go down to the bottom anyway, and
 * loses when it stops halfway, as in the hold model of the simulations,
 * where the default walk from the top stops as soon as the item fits.
 *
 * There are no handles and no decrease-key: an item whose key improves is
 * pushed again and the stale copy skipped (lazy deletion), which is what
 * makes the items plain and the heap fast. The vector of a large heap is
 * on huge pages [see `HugePageAllocator.h`], so that the walk from the root
 * to a leaf does not miss the TLB at each level and the vector, when it
 * grows, is faulted in 2 MB at a time. `HeapPriorityQueue` [see
 * `HeapPriorityQueue.h`] is the realization with handles of the book.
 *
 * With \f$ D = 2 \f$ it is the `BinaryHeap` of section 10.1 of Morin's
 * book, whose array in the order of Eytzinger is the general one of its
 * exercise 10.4 [see `parent()` and `child()`], and for any \f$ D \f$ it is
 * the `DaryHeap` of its exercise 10.5: a push moves up
 * \f$ \log_D n \f$ levels with one comparison each, a pop moves down as
 * many with \f$ D \f$ comparisons each, so that \f$ D = 4 \f$ halves the
 * levels of both and costs twice the comparisons of a pop, which is the
 * better trade on the machines with caches. Beyond the book, `erase( i )`
 * removes the item in a place (exercise 10.3), `secondTop()` gives the
 * second best item (exercise 10.9) and `smallest( k )` the \f$ k \f$ best
 * (exercise 10.10). Against the code of Morin's book: the hole moves
 * instead of the item being swapped at each level, one write per level
 * instead of the three of a swap, and the walk down compares the children
 * with each other and the best with the item, \f$ D \f$ comparisons per
 * level as in the book (where it is meant to be faster); the vector is
 * not shrunk when it is a third full, as the book does, so that a heap
 * keeps the memory of its largest size and never copies its items for it;
 * and the vector of a large heap is on huge pages.
 *
 * The contracts [see `Contract.h`]: \f$ D \ge 2 \f$, `top()` and `pop()`
 * need a heap that is not empty; the invariant is the order of the heap, no
 * item better than its parent; `erase( i )` needs \f$ i < n \f$. The
 * operations are defined in
 * `DaryHeap.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, section 10.1 and
 *      exercises 10.3 to 10.5, 10.9 and 10.10
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.512284
 * \see D. B. Johnson, Priority queues with update and finding minimum
 *      spanning trees, Information Processing Letters 4(3), 53-57, 1975,
 *      doi:10.1016/0020-0190(75)90001-0
 * \see R. W. Floyd, Algorithm 245: Treesort, Communications of the ACM
 *      7(12), 701, 1964, doi:10.1145/355588.365103
 * \see I. Wegener, BOTTOM-UP-HEAPSORT, a new variant of HEAPSORT beating,
 *      on an average, QUICKSORT (if n is not very small), Theoretical
 *      Computer Science 118(1), 81-98, 1993,
 *      doi:10.1016/0304-3975(93)90364-Y
 * \see A. LaMarca, R. E. Ladner, The influence of caches on the
 *      performance of heaps, ACM Journal of Experimental Algorithmics 1,
 *      article 4, 1996, doi:10.1145/235141.235145
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

#ifndef __ADS_DaryHeap
 #define __ADS_DaryHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DaryHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a \f$ D \f$-ary heap of plain items, the best one by `better()` on top;
/// `pop()` by the walk down from the top, or from the bottom up if
/// `BottomUp`
template< class T , class Better = std::less< T > , unsigned D = 4 ,
          bool BottomUp = false >
class DaryHeap {

 static_assert( D >= 2 , "a heap needs at least two children per node" );

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty heap of the order `better`
 explicit DaryHeap( Better better = Better() );

 /// pushes \f$ x \f$, moving it up while better than its parent, in
 /// \f$ O( \log_D n ) \f$ time
 ///
 /// In pseudocode (the heap of section 10.1.3 with \f$ D \f$ children,
 /// beyond the book), the hole moved up instead of the item swapped:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad i \gets n;\ n \gets n + 1 \cr
 /// 3 & \quad \textbf{while}\ i > 0\ \textbf{and}\ \mathrm{better}( x ,
 ///     A[ \lfloor ( i - 1 ) / D \rfloor ] )\ \textbf{do} \cr
 /// 4 & \qquad A[ i ] \gets A[ \lfloor ( i - 1 ) / D \rfloor ];\ i \gets
 ///     \lfloor ( i - 1 ) / D \rfloor \cr
 /// 5 & \quad A[ i ] \gets x
 /// \end{array}}
 /// \f]
 void push( const T & x );

 /// the best item
 /// \pre the heap is not empty
 [[nodiscard]] const T & top( void ) const;

 /// removes the best item and returns it, the last item moved down from
 /// the root, in \f$ O( D \log_D n ) \f$ time
 ///
 /// In pseudocode (the heap of section 10.1.3 with \f$ D \f$ children;
 /// the walk from the bottom up of Floyd and Wegener if `BottomUp`, beyond
 /// the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad r \gets A[ 0 ];\ n \gets n - 1;\ x \gets A[ n ];\ i \gets
 ///     0 \cr
 /// 3 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ r \cr
 /// 4 & \quad \textbf{while}\ D i + 1 < n\ \textbf{do} \cr
 /// 5 & \qquad b \gets \text{the best of the children } A[ D i + 1 ] ,
 ///     \ldots , A[ \min\{ D i + D , n - 1 \} ] \cr
 /// 6 & \qquad \textbf{if not}\ \mathit{BottomUp}\ \textbf{and not}
 ///     \ \mathrm{better}( A[ b ] , x )\ \textbf{then}\ \textbf{break} \cr
 /// 7 & \qquad A[ i ] \gets A[ b ];\ i \gets b \cr
 /// 8 & \quad \textbf{if}\ \mathit{BottomUp}\ \textbf{then}
 ///     \ \text{move } x \text{ up from } i \text{ as by } \mathrm{push}
 ///     \cr
 /// 9 & \quad \textbf{else}\ A[ i ] \gets x \cr
 /// 10 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre the heap is not empty
 T pop( void );

 /// removes the item in the place \f$ i \f$ of the view and returns it, in
 /// \f$ O( D \log_D n ) \f$ time: the last item takes its place and moves
 /// up or down (exercise 10.3 of Morin's book)
 ///
 /// In pseudocode (exercise 10.3 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad r \gets A[ i ];\ n \gets n - 1;\ x \gets A[ n ] \cr
 /// 3 & \quad \textbf{if}\ i = n\ \textbf{then return}\ r \cr
 /// 4 & \quad \textbf{if}\ i > 0\ \textbf{and}\ \mathrm{better}( x ,
 ///     A[ \mathrm{parent}( i ) ] )\ \textbf{then}\ \text{move } x
 ///     \text{ up from } i \cr
 /// 5 & \quad \textbf{else}\ \text{move } x \text{ down from } i \cr
 /// 6 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// The places of the items change at each operation and the heap gives no
 /// handle to follow them, so that a caller must find \f$ i \f$ first, in
 /// \f$ O( n ) \f$ time: this is why the exercise calls it of little use
 /// [see `IndexedDaryHeap.h` for the heap whose items know their place].
 /// \pre \f$ i < n \f$
 T erase( std::size_t i );

 /// the second best item, the best of the children of the root, or nil if
 /// there are fewer than two items, in \f$ O( D ) \f$ time (exercise 10.9
 /// of Morin's book)
 [[nodiscard]] const T * secondTop( void ) const;

 /// the \f$ k \f$ best items in order, or all of them if fewer, in
 /// \f$ O( k D \log k ) \f$ time (exercise 10.10 of Morin's book)
 ///
 /// In pseudocode (exercise 10.10 of Morin's book), \f$ Q \f$ a heap of
 /// places by their items, each place entering it after its parent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{smallest}( k ) \cr
 /// 2 & \quad Q \gets \{ 0 \};\ S \gets () \cr
 /// 3 & \quad \textbf{while}\ |S| < k\ \textbf{and}\ Q \ne \emptyset
 ///     \ \textbf{do} \cr
 /// 4 & \qquad i \gets \mathrm{deleteMin}( Q );\ S \gets S \cdot
 ///     ( A[ i ] ) \cr
 /// 5 & \qquad \text{put the children of } i \text{ in } Q \cr
 /// 6 & \quad \textbf{return}\ S
 /// \end{array}}
 /// \f]
 /// where \f$ Q \f$ holds at most \f$ k ( D - 1 ) + 1 \f$ places.
 [[nodiscard]] std::vector< T > smallest( std::size_t k ) const;

 /// the parent of the place \f$ i > 0 \f$, \f$ \lfloor ( i - 1 ) / D
 /// \rfloor \f$ (the layout of Eytzinger with \f$ D \f$ children,
 /// exercise 10.4 of Morin's book)
 [[nodiscard]] static constexpr std::size_t parent( std::size_t i ) {
  return( ( i - 1 ) / D );
  }

 /// the child \f$ j \in \{ 0 , \ldots , D - 1 \} \f$ of the place
 /// \f$ i \f$, \f$ D i + j + 1 \f$ (exercise 10.4 of Morin's book)
 [[nodiscard]] static constexpr std::size_t child( std::size_t i ,
                                                   std::size_t j ) {
  return( D * i + j + 1 );
  }

 /// whether the heap has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// room for \f$ n \f$ items, so that the pushes up to \f$ n \f$ do not
 /// reallocate
 void reserve( std::size_t n );

 /// removes all the items
 void clear( void );

 /// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$ in place of
 /// those of the heap, arranged by the construction of Floyd in
 /// \f$ O( n ) \f$ time for \f$ n \f$ items; the heap is unchanged if a
 /// copy throws
 ///
 /// In pseudocode (the `heapBuild()` of section 10.1.4 with \f$ D \f$
 /// children, beyond the book), the leaves being heaps already:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{assign}( \mathit{first} ,
 ///     \mathit{last} ) \cr
 /// 2 & \quad A \gets \text{a copy of the items};\ n \gets |A| \cr
 /// 3 & \quad \textbf{for}\ i \gets \lfloor ( n - 2 ) / D \rfloor
 ///     \ \textbf{downto}\ 0\ \textbf{do}\ \text{move } A[ i ]
 ///     \text{ down as in } \mathrm{pop}
 /// \end{array}}
 /// \f]
 template< std::input_iterator I , std::sentinel_for< I > S >
 void assign( I first , S last );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items in the order of the heap (the root first,
 // the rest in no useful order), contiguous; a change through it would
 // break the heap

 /// the type of the items of the view
 using value_type = T;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between iterators
 using difference_type = std::ptrdiff_t;
 /// the type of a reference to an item of the view
 using reference = const T &;
 /// the type of a constant reference to an item
 using const_reference = const T &;
 /// the same as `const_iterator`
 using iterator = const T *;
 /// the iterator of the view, constant
 using const_iterator = const T *;

 /// the first item of the view
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.data() ); }
 /// the end of the view
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A.data() + f_A.size() );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 // the hole at i moves up (down) until x fits in it; x comes as a value,
 // never stored and read back, which would stall the forwarding of the
 // store to the load

 void up( std::size_t i , T x );

 void down( std::size_t i , T x );

 /// the hole at the root moved down to a leaf by the best children, and x
 /// up from there (Floyd; Wegener)
 void bottomUp( T x );

 /// no item better than its parent, \f$ O( n ) \f$; on more than 1024
 /// items, the root and the last item only
 [[nodiscard]] bool ordered( void ) const;

 /// the heap, the root in f_A[ 0 ]
 std::vector< T , HugePageAllocator< T > > f_A;
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
