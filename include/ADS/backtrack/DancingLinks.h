/*--------------------------------------------------------------------------*/
/*-------------------------- File DancingLinks.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact cover by the dancing links of Knuth (his Algorithm X), beyond the
 * book [see `Enumeration.h` for the backtrack of chapter 16]: given the items
 * and the options, each a set of items, find the sets of options that contain
 * every primary item exactly once and every secondary item at most once. The
 * items and the options are on circular doubly linked lists, a node per item
 * of an option; covering an item takes it off the list of the items and the
 * options that contain it off the lists of their other items, and since a
 * removed node \f$ x \f$ keeps its own links, the uncovering puts it back by
 * the two assignments \f$ L[ R[ x ] ] \gets x \f$,
 * \f$ R[ L[ x ] ] \gets x \f$, in the reverse order. The search branches on
 * the item with the fewest options left (the MRV heuristic), which detects the
 * dead ends early.
 *
 * Many puzzles are exact covers: the \f$ n \f$ queens (the rows and the
 * columns primary, the diagonals secondary), the Sudoku (a cell, a row, a
 * column and a box with each digit, 324 primary items), the pentominoes. The
 * tests count the queens and solve the Sudoku of the backtrack chapter.
 *
 * The contracts [see `Contract.h`]: the items of an option are distinct and in
 * \f$ [ 0 , n ) \f$, and one of them is primary (an option of secondary items
 * alone would never be chosen); every solution visited is an exact cover, an
 * audit; the links after the search are as before it, an audit. The operations
 * are defined in `DancingLinks.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4B, Addison-Wesley,
 *      2022, section 7.2.2.1
 * \see D. E. Knuth, Dancing links, in Millennial Perspectives in Computer
 *      Science, Palgrave, 187-214, 2000, arXiv:cs/0011047
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

#ifndef __ADS_DancingLinks
 #define __ADS_DancingLinks
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS ExactCover -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the exact cover of the primary items, with the secondary ones at most once,
/// on dancing links
class ExactCover {

/*--------------------------------------------------------------------------*/
 public:

 /// the items \f$ 0 , \ldots , \mathit{primary} - 1 \f$ primary, the next
 /// \f$ \mathit{secondary} \f$ ones secondary, and no option
 /// \pre \f$ \mathit{primary} \ge 0 \f$ and \f$ \mathit{secondary} \ge 0 \f$
 ExactCover( int primary , int secondary = 0 );

 /// a new option, made of `items`, and its index; a node at the bottom of the
 /// list of each of its items
 /// \pre the items are distinct, in
 ///      \f$ [ 0 , \mathit{primary} + \mathit{secondary} ) \f$, one at least
 ///      primary
 int addOption( const std::vector< int > & items );

 /// calls `visit( options )` on each solution, the indices of its options,
 /// until it returns true; the number of solutions visited
 ///
 /// In pseudocode (Knuth's Algorithm X on dancing links, beyond the book),
 /// \f$ \mathit{len}( i ) \f$ the options of the item \f$ i \f$ left and
 /// \f$ \mathit{top}( p ) \f$ the item of the node \f$ p \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{search}() \cr
 /// 2 & \quad \textbf{if}\ \text{no primary item is left }\ \textbf{then}
 ///     \ \mathrm{visit}( \mathit{chosen} );\ \textbf{return} \cr
 /// 3 & \quad i \gets \text{a primary item of least } \mathit{len}( i )\quad
 ///     \triangleright\ \text{the MRV heuristic} \cr
 /// 4 & \quad \textbf{if}\ \mathit{len}( i ) = 0\ \textbf{then return} \cr
 /// 5 & \quad \mathrm{cover}( i ) \cr
 /// 6 & \quad \textbf{foreach}\ x \text{ in the list of } i \text{, top}
 ///     \text{ down}\ \textbf{do} \cr
 /// 7 & \qquad \text{append the option of } x \text{ to } \mathit{chosen} \cr
 /// 8 & \qquad \textbf{foreach}\ p \ne x \text{ in the option of } x
 ///     \ \textbf{do}\ \mathrm{cover}( \mathit{top}( p ) ) \cr
 /// 9 & \qquad \mathrm{search}() \cr
 /// 10 & \qquad \textbf{foreach}\ p \ne x \text{ in the option of } x
 ///      \text{, in the reverse order}\ \textbf{do}\ \mathrm{uncover}(
 ///      \mathit{top}( p ) ) \cr
 /// 11 & \qquad \text{remove the last option of } \mathit{chosen} \cr
 /// 12 & \quad \mathrm{uncover}( i ) \cr
 /// 13 & \textbf{procedure}\ \mathrm{cover}( i ) \cr
 /// 14 & \quad \textbf{foreach}\ p \text{ in the list of } i \text{, top}
 ///      \text{ down}\ \textbf{do} \cr
 /// 15 & \qquad \textbf{foreach}\ q \ne p \text{ in the option of } p
 ///      \ \textbf{do}\quad \triangleright\ \text{the option hidden} \cr
 /// 16 & \qquad\quad U[ D[ q ] ] \gets U[ q ];\ D[ U[ q ] ] \gets D[ q ];
 ///      \ \mathit{len}( \mathit{top}( q ) ) \gets \mathit{len}(
 ///      \mathit{top}( q ) ) - 1 \cr
 /// 17 & \quad L[ R[ i ] ] \gets L[ i ];\ R[ L[ i ] ] \gets R[ i ]\quad
 ///      \triangleright\ i \text{ off the list of the items} \cr
 /// 18 & \textbf{procedure}\ \mathrm{uncover}( i ) \cr
 /// 19 & \quad L[ R[ i ] ] \gets i;\ R[ L[ i ] ] \gets i \cr
 /// 20 & \quad \textbf{foreach}\ p \text{ in the list of } i \text{, bottom}
 ///      \text{ up}\ \textbf{do} \cr
 /// 21 & \qquad \textbf{foreach}\ q \ne p \text{ in the option of } p
 ///      \text{, in the reverse order}\ \textbf{do} \cr
 /// 22 & \qquad\quad U[ D[ q ] ] \gets q;\ D[ U[ q ] ] \gets q;
 ///      \ \mathit{len}( \mathit{top}( q ) ) \gets \mathit{len}(
 ///      \mathit{top}( q ) ) + 1
 /// \end{array}}
 /// \f]
 /// The search stops as soon as `visit` returns true.
 /// \post each solution is an exact cover (audit)
 template< class F >
 long solve( F && visit );

 /// the nodes updated by the last `solve()`, Knuth's measure of the work
 [[nodiscard]] long updates( void ) const { return( f_updates ); }

/*--------------------------------------------------------------------------*/
 private:

 void cover( int i );
 void uncover( int i );
 void hide( int p );
 void unhide( int p );

 [[nodiscard]] bool exact( const std::vector< int > & options ) const;

 int f_primary , f_items;
 std::vector< int > f_L , f_R;              ///< the list of the items
 std::vector< int > f_U , f_D , f_top;      ///< the nodes, by item
 std::vector< int > f_len;                  ///< the options of an item
 std::vector< int > f_option;               ///< the option of a node
 std::vector< std::vector< int > > f_options;
 long f_updates = 0;

 };  // end( class ExactCover )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/DancingLinks.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DancingLinks.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DancingLinks.h -------------------------*/
/*--------------------------------------------------------------------------*/
