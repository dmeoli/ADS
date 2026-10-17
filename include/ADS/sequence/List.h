/*--------------------------------------------------------------------------*/
/*------------------------------ File List.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The LIST of the book: a SEQUENCE [see ADT.h] realized as a bidirectional
 * circular list of cells with a sentinel. A list of n items has n + 1
 * cells, the sentinel holding no item; the position \f$ pos_i \f$ is the
 * address of the cell of the item of index i, for \f$ 0 \leq i < n \f$,
 * while \f$ pos_{-1} \f$ and \f$ pos_n \f$ are both the address of the
 * sentinel, which is therefore what finished() looks for. Every operation
 * takes O( 1 ) time, the list \f$ \Theta( n ) \f$ space.
 *
 * The contracts [see Contract.h]: a position is a cell of this list, and
 * read(), write() and remove() need one that holds an item, i.e., not the
 * sentinel; the invariant is that each cell is the predecessor of its
 * successor, which also makes the walk from the sentinel come back to it.
 * The operations are defined in List.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_List
 #define __ADS_List
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
/*---------------------------- CLASS List ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized as a bidirectional circular list with a sentinel
template< class T >
class List {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// a cell of the list: the sentinel is a Cell with no meaningful value
 struct Cell {
  Cell * pred;     ///< the predecessor
  Cell * succ;     ///< the successor
  T value;         ///< the item
  };

 using Pos = Cell *;   ///< a position is the address of a cell

/*--------------------------------------------------------------------------*/

 /// the empty list: the sentinel alone, its own predecessor and successor
 List( void );

 /// the list of the given items, in their order
 List( std::initializer_list< T > items );

 List( const List & other );

 List( List && other ) noexcept;

 List & operator=( List other ) noexcept;

 ~List();

 void swap( List & other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if the list is empty, i.e., the sentinel is its own successor
 [[nodiscard]] bool isEmpty( void ) const;

 /// true if p is \f$ pos_{-1} \f$ or \f$ pos_n \f$, i.e., the sentinel
 [[nodiscard]] bool finished( Pos p ) const;

 /// the position of the first item
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item
 [[nodiscard]] Pos tail( void ) const;

 /// the position of the item that follows p
 /// \pre p is a position of this list
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position of the item that precedes p
 /// \pre p is a position of this list
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts v in position p, i.e., before the item in p, and returns its
 /// position
 /// \pre p is a position of this list
 /// \post the list is not empty, and the result precedes p
 Pos insert( Pos p , const T & v );

 /// removes the item in position p, returning the position of its
 /// successor, which is now in its place
 /// \pre p holds an item
 /// \post the result follows the old predecessor of p
 Pos remove( Pos p );

 /// the item in position p
 /// \pre p holds an item
 [[nodiscard]] const T & read( Pos p ) const;

 /// writes v in position p
 /// \pre p holds an item
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// each cell is the predecessor of its successor, O( n )
 [[nodiscard]] bool linked( void ) const;

 Cell * f_sentinel;   ///< the sentinel, i.e., the list

 };  // end( class List )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* List.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File List.h -----------------------------*/
/*--------------------------------------------------------------------------*/
