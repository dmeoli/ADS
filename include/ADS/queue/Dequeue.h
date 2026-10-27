/*--------------------------------------------------------------------------*/
/*----------------------------- File Dequeue.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The DEQUEUE (double ended queue) of exercise 4.9, a sequence changed at
 * both ends, which is a STACK when used at one end and a QUEUE when used at
 * both [see `ADT.h`]: realized with a LIST with pointers, every operation
 * takes \f$ O( 1 ) \f$ time. It is the structure of the algorithm of
 * Pape-D'Esopo (section 11.8).
 *
 * The contracts [see `Contract.h`]: `removeHead()`, `removeTail()`,
 * `head()` and `tail()` need a dequeue that is not empty, and the insertions
 * ensure one that is not, with the item at the end where it was put. The
 * operations are defined in `Dequeue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see U. Pape, Implementation and efficiency of Moore-algorithms for the
 *      shortest route problem, Mathematical Programming 7(1), 212-222,
 *      1974, doi:10.1007/BF01585517
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

#ifndef __ADS_Dequeue
 #define __ADS_Dequeue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"

#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Dequeue ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DEQUEUE on a bidirectional list with pointers
template< class T >
class Dequeue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const;

 /// puts \f$ v \f$ at the head, `insert( head() , v )` of the list
 ///
 /// In pseudocode (exercise 4.9), \f$ L \f$ the list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertHead}( v ) \cr
 /// 2 & \quad L.\mathrm{insert}( L.\mathrm{head}() , v )
 /// \end{array}}
 /// \f]
 /// \post the dequeue is not empty
 void insertHead( const T & v );

 /// puts \f$ v \f$ at the tail, before the sentinel of the list
 ///
 /// In pseudocode (exercise 4.9):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertTail}( v ) \cr
 /// 2 & \quad L.\mathrm{insert}( L.\mathrm{next}( L.\mathrm{tail}() ) , v )
 /// \end{array}}
 /// \f]
 /// \post the dequeue is not empty
 void insertTail( const T & v );

 /// removes the item at the head and returns it
 ///
 /// In pseudocode (exercise 4.9):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{removeHead}() \cr
 /// 2 & \quad v \gets L.\mathrm{read}( L.\mathrm{head}() );
 ///     \ L.\mathrm{remove}( L.\mathrm{head}() );\ \textbf{return}\ v
 /// \end{array}}
 /// \f]
 /// \pre the dequeue is not empty
 T removeHead( void );

 /// removes the item at the tail and returns it
 ///
 /// In pseudocode (exercise 4.9):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{removeTail}() \cr
 /// 2 & \quad v \gets L.\mathrm{read}( L.\mathrm{tail}() );
 ///     \ L.\mathrm{remove}( L.\mathrm{tail}() );\ \textbf{return}\ v
 /// \end{array}}
 /// \f]
 /// \pre the dequeue is not empty
 T removeTail( void );

 /// the item at the head
 /// \pre the dequeue is not empty
 [[nodiscard]] const T & head( void ) const;

 /// the item at the tail
 /// \pre the dequeue is not empty
 [[nodiscard]] const T & tail( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the bidirectional iterators of the list, from the head to the tail

 /// the type of the items
 using value_type = T;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = T &;
 /// a constant reference to an item
 using const_reference = const T &;
 /// the iterator on the items
 using iterator = typename List< T >::iterator;
 /// the iterator on the constant items
 using const_iterator = typename List< T >::const_iterator;
 /// the iterator on the items, backwards
 using reverse_iterator = typename List< T >::reverse_iterator;
 /// the iterator on the constant items, backwards
 using const_reverse_iterator = typename List< T >::const_reverse_iterator;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( f_L.begin() ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( f_L.end() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }
 /// the iterator on the last item, backwards
 [[nodiscard]] reverse_iterator rbegin( void ) { return( f_L.rbegin() ); }
 /// the iterator before the first item, backwards
 [[nodiscard]] reverse_iterator rend( void ) { return( f_L.rend() ); }
 /// the constant iterator on the last item, backwards
 [[nodiscard]] const_reverse_iterator rbegin( void ) const {
  return( f_L.rbegin() );
  }
 /// the constant iterator before the first item, backwards
 [[nodiscard]] const_reverse_iterator rend( void ) const {
  return( f_L.rend() );
  }

 /// `insertTail()`, for `std::back_inserter`
 void push_back( const T & v ) { insertTail( v ); }

 /// `insertHead()`, for `std::front_inserter`
 void push_front( const T & v ) { insertHead( v ); }

/*--------------------------------------------------------------------------*/
 private:

 List< T > f_L;   ///< the items, from the head

 };  // end( class Dequeue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/Dequeue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Dequeue.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Dequeue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
