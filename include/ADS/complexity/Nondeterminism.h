/*--------------------------------------------------------------------------*/
/*------------------------- File Nondeterminism.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The nondeterministic algorithms of section 18.2 simulated by enumeration
 * (section 18.3). A nondeterministic algorithm is a function that receives
 * a Chooser and calls choice( k ) to pick one of k alternatives, returning
 * true for success and false for failure; it answers YES if some sequence
 * of choices leads to success. run() replays the algorithm on every
 * sequence of choices, in increasing order, a depth first visit of the
 * tree of the choices that backtracks on the last choice that still has an
 * untried alternative, and stops at the first success; its time is the
 * number of leaves of the tree times the time of a run, exponential in the
 * height of the tree, while the nondeterministic algorithm takes as much as
 * one path.
 *
 * The contracts [see Contract.h]: choice() needs k >= 1 and returns a choice
 * of [ 0 , k ). The operations are defined in Nondeterminism.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 18.2 and 18.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
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

#ifndef __ADS_Nondeterminism
 #define __ADS_Nondeterminism
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the simulation of choice(), success and failure
class Chooser {

/*--------------------------------------------------------------------------*/
 public:

 /// choice( { 0 , ... , k - 1 } )
 /// \pre k >= 1
 /// \post the result is in [ 0 , k )
 int choice( int k );

 /// whether some sequence of choices makes alg( *this ) return true; the
 /// runs made go to *runs, if given
 template< class A >
 bool run( A && alg , long * runs = nullptr );

 /// the choices of the last run, the successful one after a YES
 [[nodiscard]] std::vector< int > choices( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::vector< std::pair< int , int > > f_path;   ///< ( choice , of how many )
 std::size_t f_depth = 0;                        ///< the choices made so far

 };  // end( class Chooser )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Nondeterminism.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Nondeterminism.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Nondeterminism.h ------------------------*/
/*--------------------------------------------------------------------------*/
