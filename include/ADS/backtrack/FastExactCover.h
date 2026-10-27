/*--------------------------------------------------------------------------*/
/*------------------------- File FastExactCover.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered exact cover with colors, beyond the book [see
 * `DancingLinks.h` for Knuth's Algorithm X written as the text describes it]:
 * the exact covering with colors (XCC, Knuth's Algorithm C) on sparse sets
 * instead of linked lists, the dancing cells of Knuth's program SSXCC after
 * the design of Solnon.
 *
 * The problem: items, primary or secondary, and options, each a set of items
 * where a secondary item may carry a color; a solution is a set of options
 * that contains each primary item exactly once and gives each secondary item
 * at most one color (several options may share a secondary item if they give
 * it the same color, and an uncolored secondary item is at most once). Without
 * colors this is the exact cover of `DancingLinks.h`, with the same solutions,
 * visited in another order.
 *
 * The structure: the nodes of the options are one array of records
 * \f$ \{ \mathit{item} , \mathit{loc} , \mathit{color} \} \f$ of 16 bytes, the
 * nodes of an option consecutive and followed by a spacer whose item is minus
 * the size of the option, so that the other nodes of an option are walked by
 * the index alone. Each item \f$ x \f$ has a segment of an array
 * \f$ \mathit{set} \f$, where its active options are the first
 * \f$ \mathrm{size}( x ) \f$ entries, each node knowing its place
 * \f$ \mathit{loc} \f$ in the segment of its item: a sparse set of Briggs and
 * Torczon, where an option leaves the set of \f$ x \f$ by a swap with the last
 * active one and a decrement of \f$ \mathrm{size}( x ) \f$, in \f$ O( 1 ) \f$.
 * The active items are a sparse set too, the array of the items with the place
 * \f$ \mathrm{pos}( x ) \f$ of each.
 *
 * The search, at each level: the active primary item \f$ x \f$ with the least
 * \f$ \mathrm{size}( x ) \f$ (the MRV rule, ties to the least index) is taken
 * out, its options are hidden from the other items, and the sizes of all the
 * active items are pushed on a stack; then each option of \f$ x \f$ in turn
 * takes its other items out and hides their incompatible options (all of them
 * for a primary or uncolored item, those of another color for a colored one),
 * and the search goes one level deeper. Going back costs no undo of the swaps:
 * the sizes on the stack are written back, and the sets are again those of the
 * level, in another order, so that a level costs \f$ O( \mathit{active} ) \f$
 * to save and restore instead of an update per link. Hiding an option checks
 * the size of each primary item it leaves: a size 0 abandons the choice at
 * once, before going deeper, and a size 1 makes the item a forced move, taken
 * at the next level without saving the sizes (its only option needs no
 * return). The time is \f$ O( 1 ) \f$ per update of a set; the worst case
 * stays exponential, as the problem is NP-complete.
 *
 * The contracts [see `Contract.h`]: the items of an option are distinct, in
 * \f$ [ 0 , \mathit{primary} + \mathit{secondary} ) \f$, one at least primary,
 * and a color, if given, is not negative and only on a secondary item; every
 * solution visited is a solution, an audit. The operations are defined in
 * `FastExactCover.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4B, Addison-Wesley,
 *      2022, section 7.2.2.1 (Algorithms X and C)
 * \see D. E. Knuth, SSXCC, the program of the exact covering with colors
 *      on sparse sets, 2023, https://cs.stanford.edu/~knuth/programs.html
 * \see P. Briggs, L. Torczon, An efficient representation for sparse sets,
 *      ACM Letters on Programming Languages and Systems 2(1-4), 59-69,
 *      1993, doi:10.1145/176454.176484
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

#ifndef __ADS_FastExactCover
 #define __ADS_FastExactCover
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS FastExactCover ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the exact cover with colors on sparse sets, the dancing cells
class FastExactCover {

/*--------------------------------------------------------------------------*/
 public:

 /// the items \f$ 0 , \ldots , \mathit{primary} - 1 \f$ primary, the next
 /// \f$ \mathit{secondary} \f$ ones secondary, and no option
 /// \pre \f$ \mathit{primary} \ge 0 \f$ and \f$ \mathit{secondary} \ge 0 \f$
 FastExactCover( int primary , int secondary = 0 );

 /// a new option, made of `items`, and its index; `colors[ k ]` \f$ > 0 \f$
 /// is the color of `items[ k ]`, and 0, or no `colors`, is no color
 /// \pre the items are distinct, in
 ///      \f$ [ 0 , \mathit{primary} + \mathit{secondary} ) \f$, one at least
 ///      primary; `colors` empty or of the size of `items`, not negative, 0 on
 ///      the primary items
 int addOption( const std::vector< int > & items ,
                const std::vector< int > & colors = {} );

 /// calls `visit( options )` on each solution, the indices of its options in
 /// the order chosen, until it returns true (if it returns `bool`); the
 /// number visited
 ///
 /// In pseudocode (Knuth's Algorithm C on sparse sets, beyond the book),
 /// recursive here while the code keeps the levels on a stack of its own:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{search}() \cr
 /// 2 & \quad \textbf{if}\ \text{a forced item } x \text{ is pending }
 ///     \ \textbf{then}\ \text{branch on } x \text{ without saving the sizes}
 ///     \cr
 /// 3 & \quad \textbf{if}\ \text{no primary item is active }
 ///     \ \textbf{then}\ \mathrm{visit}( \mathit{chosen} );\ \textbf{return}
 ///     \cr
 /// 4 & \quad x \gets \text{the active primary item of least }
 ///     \mathrm{size}( x ) \text{, the least index on ties} \cr
 /// 5 & \quad \textbf{if}\ \mathrm{size}( x ) = 0\ \textbf{then return} \cr
 /// 6 & \quad \text{take } x \text{ out of the active items};
 ///     \ \mathrm{hide}( x , 0 )\quad \triangleright\ \text{without the tests}
 ///     \text{ of lines 17 and 18} \cr
 /// 7 & \quad \text{push the sizes of the active items} \cr
 /// 8 & \quad \textbf{foreach}\ o \text{ among the first } \mathrm{size}(
 ///     x ) \text{ of } \mathit{set}( x )\ \textbf{do} \cr
 /// 9 & \qquad \text{take the other items of } o \text{ out of the active}
 ///     \text{ ones} \cr
 /// 10 & \qquad \textbf{if}\ \mathrm{hide}( c , \text{the color of } c
 ///      \text{ in } o ) \text{ for each other item } c \text{ of } o
 ///      \ \textbf{then} \cr
 /// 11 & \qquad\quad \text{append } o \text{ to } \mathit{chosen};
 ///      \ \mathrm{search}();\ \text{remove } o \text{ from }
 ///      \mathit{chosen} \cr
 /// 12 & \qquad \text{write the sizes pushed back}\quad \triangleright
 ///      \ \text{no swap undone} \cr
 /// 13 & \textbf{function}\ \mathrm{hide}( c , k ) \cr
 /// 14 & \quad \textbf{foreach}\ o \text{ among the first } \mathrm{size}(
 ///      c ) \text{ of } \mathit{set}( c ) \text{, of a color other than }
 ///      k \text{ if } k \ne 0\ \textbf{do} \cr
 /// 15 & \qquad \textbf{foreach}\ \text{active item } u \ne c \text{ of } o
 ///      \ \textbf{do} \cr
 /// 16 & \qquad\quad \text{swap } o \text{ with the last active option of }
 ///      \mathit{set}( u );\ \mathrm{size}( u ) \gets \mathrm{size}( u ) - 1
 ///      \cr
 /// 17 & \qquad\quad \textbf{if}\ u \text{ is primary and } \mathrm{size}(
 ///      u ) = 0\ \textbf{then return false}\quad \triangleright
 ///      \ \text{uncoverable} \cr
 /// 18 & \qquad\quad \textbf{if}\ u \text{ is primary and } \mathrm{size}(
 ///      u ) = 1\ \textbf{then}\ \text{mark } u \text{ as forced} \cr
 /// 19 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post each solution visited is a solution (audit)
 template< class F >
 long solve( F && visit );

 /// the options taken out of a set by the last `solve()`
 [[nodiscard]] long updates( void ) const { return( f_updates ); }

 /// the nodes of the tree of the search of the last `solve()`
 [[nodiscard]] long nodes( void ) const { return( f_nodes ); }

/*--------------------------------------------------------------------------*/
 private:

 /// a node of an option, or the spacer after its nodes
 struct Node {
  int item;                                 ///< the item, -size a spacer
  int loc;                                  ///< its place in set, the
                                            ///< option of a spacer
  int color;                                ///< its color, 0 if none
  int pad;                                  ///< 16 bytes per node
  };

 /// the sparse set of the active options of an item
 struct Item {
  int start ,                               ///< its segment of set
      size ,                                ///< the active options
      pos ,                                 ///< its place in active
      pad;                                  ///< 16 bytes per item
  };

 void build( void );
 bool hide( int c , int color , bool check );

 [[nodiscard]] int optionOf( int x ) const;
 [[nodiscard]] bool solution( const std::vector< int > & options ) const;

 int f_primary , f_items;
 std::vector< std::vector< int > > f_options , f_colors;

 std::vector< Node > f_node;                ///< the search state
 std::vector< Item > f_item;
 std::vector< int > f_set , f_active , f_force;
 int f_nActive = 0 , f_oActive = 0;
 long f_updates = 0 , f_nodes = 0;

 };  // end( class FastExactCover )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/FastExactCover.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FastExactCover.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FastExactCover.h -----------------------*/
/*--------------------------------------------------------------------------*/
