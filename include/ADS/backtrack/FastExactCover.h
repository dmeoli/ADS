/*--------------------------------------------------------------------------*/
/*------------------------- File FastExactCover.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered exact cover with colors, beyond the book [see
 * DancingLinks.h for Algorithm 7.2.2.1X written as the text describes it]:
 * the exact covering with colors (XCC, Algorithm 7.2.2.1C of Knuth) on
 * sparse sets instead of linked lists, the "dancing cells" of Knuth's
 * program SSXCC after the design of Solnon.
 *
 * The problem: items, primary or secondary, and options, each a set of
 * items where a secondary item may carry a color; a solution is a set of
 * options that contains each primary item exactly once and gives each
 * secondary item at most one color (several options may share a secondary
 * item if they give it the same color, and an uncolored secondary item is
 * at most once). Without colors this is the exact cover of DancingLinks.h,
 * with the same solutions, visited in another order.
 *
 * The structure: the nodes of the options are one array of records
 * { item , loc , color } of 16 bytes, the nodes of an option consecutive
 * and followed by a spacer whose item is minus the size of the option, so
 * that the other nodes of an option are walked by the index alone. Each
 * item x has a segment of an array set, where its active options are the
 * first size( x ) entries, each node knowing its place loc in the segment
 * of its item: a sparse set of Briggs and Torczon, where an option leaves
 * the set of x by a swap with the last active one and a decrement of
 * size( x ), in O( 1 ). The active items are a sparse set too, the array
 * of the items with the place pos( x ) of each.
 *
 * The search, at each level: the active primary item x with the least
 * size( x ) (the MRV rule, ties to the least index) is taken out, its
 * options are hidden from the other items, and the sizes of all the
 * active items are pushed on a stack; then each option of x in turn takes
 * its other items out and hides their incompatible options (all of them
 * for a primary or uncolored item, those of another color for a colored
 * one), and the search goes one level deeper. Going back costs no undo of
 * the swaps: the sizes on the stack are written back, and the sets are
 * again those of the level, in another order, so that a level costs
 * O( active ) to save and restore instead of an update per link. Hiding
 * an option checks the size of each primary item it leaves: a size 0
 * abandons the choice at once, before going deeper, and a size 1 makes
 * the item a forced move, taken at the next level without saving the
 * sizes (its only option needs no return). The time is O( 1 ) per update
 * of a set; the worst case stays exponential, as the problem is
 * NP-complete.
 *
 * The contracts [see Contract.h]: the items of an option are distinct, in
 * [ 0 , primary + secondary ), one at least primary, and a color, if given,
 * is not negative and only on a secondary item; every solution visited is a
 * solution, an audit. The operations are defined in FastExactCover.tpp.
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

/// the exact cover with colors on sparse sets (dancing cells)
class FastExactCover {

/*--------------------------------------------------------------------------*/
 public:

 /// the items 0 , ... , primary - 1 primary, the next secondary ones
 /// \pre primary >= 0 and secondary >= 0
 FastExactCover( int primary , int secondary = 0 );

 /// a new option, its index; colors[ k ] > 0 is the color of items[ k ],
 /// 0 or no colors none
 /// \pre items distinct, in [ 0 , primary + secondary ), one primary;
 ///      colors empty or of the size of items, not negative, 0 on the
 ///      primary items
 int addOption( const std::vector< int > & items ,
                const std::vector< int > & colors = {} );

 /// calls visit( options ) on each solution, the indices of its options
 /// in the order chosen, until it returns true (if it returns bool); the
 /// number visited
 /// \post each solution is a solution (an audit)
 template< class F >
 long solve( F && visit );

 /// the options taken out of a set by the last solve()
 [[nodiscard]] long updates( void ) const { return( f_updates ); }

 /// the nodes of the tree of the search of the last solve()
 [[nodiscard]] long nodes( void ) const { return( f_nodes ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  int item;                                 ///< the item, -size a spacer
  int loc;                                  ///< its place in set, the
                                            ///< option of a spacer
  int color;
  int pad;
  };

 struct Item {
  int start , size , pos , pad;
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
