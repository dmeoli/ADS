/*--------------------------------------------------------------------------*/
/*-------------------------- File DancingLinks.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact cover by dancing links of Knuth (Algorithm 7.2.2.1X), beyond
 * the book [see Enumeration.h for the backtrack of chapter 16]: given the
 * items and the options, each a set of items, find the sets of options that
 * contain every primary item exactly once and every secondary item at most
 * once. The items and the options are on circular doubly linked lists, a
 * node per item of an option; covering an item takes it off the list of the
 * items and the options that contain it off the lists of their other items,
 * and since a removed node keeps its own links, the uncovering puts it back
 * by the two assignments \f$ L[ R[ x ] ] = x , R[ L[ x ] ] = x \f$, in the
 * reverse order. The search branches on the item with the fewest options
 * left (the MRV heuristic), which detects the dead ends early.
 *
 * Many puzzles are exact covers: the n queens (the rows and columns
 * primary, the diagonals secondary), the Sudoku (a cell, a row, a column and
 * a box with each digit, 324 primary items), the pentominoes. The tests
 * count the queens and solve the Sudoku of the backtrack chapter.
 *
 * The contracts [see Contract.h]: the items of an option are distinct and in [
 * 0 , n ), and one of them is primary (an option of secondary items alone
 * would never be chosen); every solution visited is an exact cover, an audit;
 * the links after the search are as before it, an audit. The operations are
 * defined in DancingLinks.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4B, Addison-Wesley,
 *      2022, section 7.2.2.1
 * \see D. E. Knuth, Dancing links, in Millennial Perspectives in Computer
 *      Science, Palgrave, 187-214, 2000, arXiv:cs/0011047
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

/// the exact cover problem on dancing links
class ExactCover {

/*--------------------------------------------------------------------------*/
 public:

 /// the items 0 , ... , primary - 1 primary, the next secondary ones
 /// \pre primary >= 0 and secondary >= 0
 ExactCover( int primary , int secondary = 0 );

 /// a new option, its index
 /// \pre items distinct, in [ 0 , primary + secondary ), one primary
 int addOption( const std::vector< int > & items );

 /// calls visit( options ) on each solution, the indices of its options,
 /// until it returns true; the number of solutions visited
 /// \post each solution is an exact cover (an audit)
 template< class F >
 long solve( F && visit );

 /// the nodes updated by the last solve(), Knuth's measure of the work
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

#include "ADS/backtrack/DancingLinks.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DancingLinks.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DancingLinks.h -------------------------*/
/*--------------------------------------------------------------------------*/
