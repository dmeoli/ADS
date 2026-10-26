/*--------------------------------------------------------------------------*/
/*---------------------------- File ListMFSet.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET on lists of section 10.2.1 [see MFSet.h], each element knowing
 * the head of its list: find() in O( 1 ), merge() relabels the shorter
 * list, so that an element is relabeled at most \f$ \log_2 n \f$ times.
 *
 * The contracts [see Contract.h]: the elements are in [ 0 , n ), and after
 * merge() x and y have the same head; the invariant is that each element
 * is once in the list of its head. The operations are defined in
 * ListMFSet.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. E. Hopcroft, J. D. Ullman, Set merging algorithms, SIAM Journal
 *      on Computing 2(4), 294-303, 1973, doi:10.1137/0202024
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

#ifndef __ADS_ListMFSet
 #define __ADS_ListMFSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ListMFSet -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets on lists, each element with the head of its
/// list
class ListMFSet {

/*--------------------------------------------------------------------------*/
 public:

 /// \pre n >= 0
 explicit ListMFSet( int n );

 [[nodiscard]] int n( void ) const;

 /// O( 1 ) time
 /// \pre x in [ 0 , n )
 [[nodiscard]] int find( int x ) const;

 /// the shorter list goes after the longer one, its elements relabeled
 /// \pre x , y in [ 0 , n )
 /// \post x and y have the same head
 bool merge( int x , int y );

 /// the elements relabeled so far
 [[nodiscard]] long relabels( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the members of the set of x
 /// \pre x is in [ 0 , n )
 [[nodiscard]] const std::vector< int > & members( int x ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// each element is once in the list of its head, O( n )
 [[nodiscard]] bool listed( void ) const;

 std::vector< int > f_head;                 ///< the head of each list
 std::vector< std::vector< int > > f_list;  ///< the list of each head
 long f_relabels = 0;                       ///< see relabels()

 };  // end( class ListMFSet )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/ListMFSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListMFSet.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ListMFSet.h --------------------------*/
/*--------------------------------------------------------------------------*/
