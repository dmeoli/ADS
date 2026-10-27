/*--------------------------------------------------------------------------*/
/*---------------------------- File ListMFSet.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET on lists of section 10.2.1 [see `MFSet.h`], each element knowing
 * the head of its list: `find()` in \f$ O( 1 ) \f$, `merge()` relabels the
 * shorter list, so that an element is relabeled at most \f$ \log_2 n \f$
 * times.
 *
 * The contracts [see `Contract.h`]: the elements are in \f$ [ 0 , n ) \f$,
 * and after `merge()` \f$ x \f$ and \f$ y \f$ have the same head; the
 * invariant is that each element is once in the list of its head. The
 * operations are defined in `ListMFSet.tpp`.
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

 /// the \f$ n \f$ singletons \f$ \{ 0 \} , \ldots , \{ n - 1 \} \f$
 /// \pre \f$ n \ge 0 \f$
 explicit ListMFSet( int n );

 /// the number \f$ n \f$ of elements
 [[nodiscard]] int n( void ) const;

 /// the head of the list of \f$ x \f$, in \f$ O( 1 ) \f$ time
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] int find( int x ) const;

 /// joins the lists of \f$ x \f$ and \f$ y \f$, the shorter after the
 /// longer, its elements relabeled; true if they were distinct
 ///
 /// In pseudocode (section 10.2.1), \f$ \mathit{head}[ x ] \f$ the head of
 /// the list of \f$ x \f$ and \f$ L[ a ] \f$ the list of the head \f$ a \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{merge}( x , y ) \cr
 /// 2 & \quad a \gets \mathit{head}[ x ];\ b \gets \mathit{head}[ y ] \cr
 /// 3 & \quad \textbf{if}\ a = b\ \textbf{then return false} \cr
 /// 4 & \quad \textbf{if}\ |L[ a ]| < |L[ b ]|\ \textbf{then}\ \text{swap }
 ///     a \text{ and } b \cr
 /// 5 & \quad \textbf{foreach}\ z \in L[ b ]\ \textbf{do}\ \mathit{head}[ z ]
 ///     \gets a;\ \text{append } z \text{ to } L[ a ] \cr
 /// 6 & \quad L[ b ] \gets \emptyset;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x , y \in [ 0 , n ) \f$
 /// \post \f$ x \f$ and \f$ y \f$ have the same head
 bool merge( int x , int y );

 /// the elements relabeled so far
 [[nodiscard]] long relabels( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the members of the set of \f$ x \f$
 /// \pre \f$ x \in [ 0 , n ) \f$
 [[nodiscard]] const std::vector< int > & members( int x ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// each element is once in the list of its head, \f$ O( n ) \f$
 [[nodiscard]] bool listed( void ) const;

 std::vector< int > f_head;                 ///< the head of each list
 std::vector< std::vector< int > > f_list;  ///< the list of each head
 long f_relabels = 0;                       ///< see `relabels()`

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
