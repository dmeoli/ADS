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

 List( void ) : f_sentinel( new Cell{ nullptr , nullptr , T() } ) {
  f_sentinel->pred = f_sentinel;
  f_sentinel->succ = f_sentinel;
  }

 /// the list of the given items, in their order
 List( std::initializer_list< T > items ) : List() {
  for( const auto & v : items )
   insert( next( tail() ) , v );
  }

 List( const List & other ) : List() {
  for( Pos p = other.head() ; ! other.finished( p ) ; p = other.next( p ) )
   insert( next( tail() ) , other.read( p ) );
  }

 List( List && other ) noexcept : List() { swap( other ); }

 List & operator=( List other ) noexcept {
  swap( other );
  return( *this );
  }

 ~List() {
  while( ! isEmpty() )
   remove( head() );
  delete f_sentinel;
  }

 void swap( List & other ) noexcept {
  std::swap( f_sentinel , other.f_sentinel );
  }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if the list is empty, i.e., the sentinel is its own successor
 [[nodiscard]] bool isEmpty( void ) const {
  return( f_sentinel->succ == f_sentinel );
  }

 /// true if p is \f$ pos_{-1} \f$ or \f$ pos_n \f$, i.e., the sentinel
 [[nodiscard]] bool finished( Pos p ) const { return( p == f_sentinel ); }

 /// the position of the first item
 [[nodiscard]] Pos head( void ) const { return( f_sentinel->succ ); }

 /// the position of the last item
 [[nodiscard]] Pos tail( void ) const { return( f_sentinel->pred ); }

 /// the position of the item that follows p
 [[nodiscard]] Pos next( Pos p ) const { return( p->succ ); }

 /// the position of the item that precedes p
 [[nodiscard]] Pos prev( Pos p ) const { return( p->pred ); }

 /// puts v in position p, i.e., before the item in p, and returns its
 /// position
 Pos insert( Pos p , const T & v ) {
  Pos t = new Cell{ p->pred , p , v };
  p->pred->succ = t;
  p->pred = t;
  return( t );
  }

 /// removes the item in position p, returning the position of its
 /// successor, which is now in its place
 Pos remove( Pos p ) {
  p->pred->succ = p->succ;
  p->succ->pred = p->pred;
  Pos t = p->succ;
  delete p;
  return( t );
  }

 /// the item in position p
 [[nodiscard]] const T & read( Pos p ) const { return( p->value ); }

 /// writes v in position p
 void write( Pos p , const T & v ) { p->value = v; }

/*--------------------------------------------------------------------------*/
 private:

 Cell * f_sentinel;   ///< the sentinel, i.e., the list

 };  // end( class List )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* List.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File List.h -----------------------------*/
/*--------------------------------------------------------------------------*/
