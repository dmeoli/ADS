/*--------------------------------------------------------------------------*/
/*--------------------------- File SinglyList.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see ADT.h] realized as a singly linked list, not circular and
 * with no sentinel (Esercizio 4.6 of the book). The position
 * \f$ pos_i \f$ is the address of the cell of \f$ a_{i-1} \f$ for i > 0
 * and nil for i = 0, so that the cell before an item, which insert() and
 * remove() change, is always at hand: every operation takes O( 1 ) time but
 * tail() and prev(), which scan the list and take O( n ). \f$ pos_n \f$ is
 * the address of the last cell (nil if the list is empty), and
 * \f$ pos_{-1} \f$ a distinguished address of the list itself. This is the
 * convention of the realizations of a list with pointers or with cursors in
 * which a position is the cell that precedes the item, e.g., in the book by
 * Aho, Hopcroft and Ullman.
 *
 * The contracts [see Contract.h]: read(), write() and remove() need a
 * position that holds an item, i.e., one that is not finished();
 * insert() and write() ensure that v is read back in its position. A list
 * is copied cell by cell and moved in O( 1 ) time, the one moved from left
 * empty. The operations are defined in SinglyList.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizio 4.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see A. V. Aho, J. E. Hopcroft, J. D. Ullman, Data Structures and
 *      Algorithms, Addison-Wesley, 1983, section 2.2
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

#ifndef __ADS_SinglyList
 #define __ADS_SinglyList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <initializer_list>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SinglyList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized as a singly linked list with no sentinel
template< class T >
class SinglyList {

/*--------------------------------------------------------------------------*/
 public:

 /// a cell of the list
 struct Cell {
  T value;      ///< the item
  Cell * next;  ///< the next cell, nil after the last one
  };

 using Item = T;       ///< the type of the items
 using Pos = Cell *;   ///< the cell before the item, nil for the first one

 SinglyList( void ) = default;

 /// the list of the given items, in their order
 SinglyList( std::initializer_list< T > items );

 SinglyList( const SinglyList & other );

 /// the cells of other, which is left empty
 SinglyList( SinglyList && other ) noexcept;

 SinglyList & operator=( SinglyList other ) noexcept;

 ~SinglyList();

 void swap( SinglyList & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool isEmpty( void ) const;

 /// true on \f$ pos_{-1} \f$ and on \f$ pos_n \f$, the address of the
 /// last cell (nil if the list is empty)
 [[nodiscard]] bool finished( Pos p ) const;

 /// \f$ pos_0 \f$, i.e., nil
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item, O( n )
 [[nodiscard]] Pos tail( void ) const;

 /// the position that follows p
 /// \pre p is not \f$ pos_n \f$
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position that precedes p, O( n )
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts v in position p, i.e., before the item in p; the new item takes
 /// the position p
 /// \pre p is not \f$ pos_{-1} \f$
 /// \post read( p ) is v
 Pos insert( Pos p , const T & v );

 /// removes the item in position p, whose successor takes the position p
 /// \pre p holds an item
 Pos remove( Pos p );

 /// \pre p holds an item
 [[nodiscard]] const T & read( Pos p ) const;

 /// \pre p holds an item
 /// \post read( p ) is v
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the same items in the same order
 [[nodiscard]] bool operator==( const SinglyList & o ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the cell of the item in position p
 [[nodiscard]] Cell * cell( Pos p ) const;

 Cell * f_first = nullptr;               ///< the first cell
 mutable Cell f_before{ T() , nullptr }; ///< its address is pos_-1

 };  // end( class SinglyList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/SinglyList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SinglyList.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SinglyList.h --------------------------*/
/*--------------------------------------------------------------------------*/
