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

 /// choice( { 0 , ... , k - 1 } ), k >= 1
 int choice( int k ) {
  if( f_depth == f_path.size() )
   f_path.push_back( { 0 , k } );
  return( f_path[ f_depth++ ].first );
  }

 /// whether some sequence of choices makes alg( *this ) return true; the
 /// runs made go to *runs, if given
 template< class A >
 bool run( A && alg , long * runs = nullptr ) {
  f_path.clear();
  for( ; ; ) {
   f_depth = 0;
   if( runs ) ++*runs;
   if( alg( *this ) )
    return( true );                            // success
   f_path.resize( f_depth );                   // failure: the next sequence
   while( ! f_path.empty() && ( f_path.back().first + 1 ==
                                f_path.back().second ) )
    f_path.pop_back();
   if( f_path.empty() )
    return( false );
   ++f_path.back().first;
   }
  }

 /// the choices of the last run, the successful one after a YES
 [[nodiscard]] std::vector< int > choices( void ) const {
  std::vector< int > c;
  for( auto & [ x , k ] : f_path ) c.push_back( x );
  return( c );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< std::pair< int , int > > f_path;   ///< ( choice , of how many )
 std::size_t f_depth = 0;                        ///< the choices made so far

 };  // end( class Chooser )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Nondeterminism.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Nondeterminism.h ------------------------*/
/*--------------------------------------------------------------------------*/
