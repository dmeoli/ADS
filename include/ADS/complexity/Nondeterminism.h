/*--------------------------------------------------------------------------*/
/*------------------------- File Nondeterminism.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The nondeterministic algorithms of section 18.2 simulated by enumeration
 * (section 18.3). A nondeterministic algorithm is a function that receives
 * a `Chooser` and calls `choice( k )` to pick one of \f$ k \f$ alternatives,
 * returning true for success and false for failure; it answers YES if some
 * sequence of choices leads to success. `run()` replays the algorithm on
 * every sequence of choices, in increasing order, a visit in depth of the
 * tree of the choices that backtracks on the last choice that still has an
 * untried alternative, and stops at the first success; its time is the
 * number of leaves of the tree times the time of a run, exponential in the
 * height of the tree, while the nondeterministic algorithm takes as much as
 * one path.
 *
 * The contracts [see `Contract.h`]: `choice()` needs \f$ k \ge 1 \f$ and
 * returns a choice of \f$ [ 0 , k ) \f$. The operations are defined in
 * `Nondeterminism.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 18.2 and 18.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
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

/// the simulation of `choice()`, success and failure
class Chooser {

/*--------------------------------------------------------------------------*/
 public:

 /// the choice among \f$ \{ 0 , \ldots , k - 1 \} \f$ that the current
 /// sequence makes at this depth, 0 the first time the depth is reached
 ///
 /// In pseudocode (section 18.3), \f$ P \f$ the sequence of the pairs
 /// (choice, alternatives) and \f$ t \f$ the choices made in this run:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{choice}( k ) \cr
 /// 2 & \quad \textbf{if}\ t = |P|\ \textbf{then}\ \text{append } ( 0 , k )
 ///     \text{ to } P \cr
 /// 3 & \quad t \gets t + 1 \cr
 /// 4 & \quad \textbf{return}\ \text{the choice of } P[ t ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \ge 1 \f$
 /// \post the result is in \f$ [ 0 , k ) \f$
 int choice( int k );

 /// whether some sequence of choices makes `alg( *this )` return true; the
 /// runs made go to `*runs`, if given
 ///
 /// In pseudocode (section 18.3), the sequences in lexicographic order, the
 /// next one obtained by dropping the choices with no alternative left and
 /// advancing the last one:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{run}( \mathit{alg} ) \cr
 /// 2 & \quad P \gets \text{the empty sequence} \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad t \gets 0 \cr
 /// 5 & \qquad \textbf{if}\ \mathit{alg}()\ \textbf{then return true}\quad
 ///     \triangleright\ \text{success} \cr
 /// 6 & \qquad \text{truncate } P \text{ to its first } t \text{ pairs} \cr
 /// 7 & \qquad \textbf{while}\ P \ne \emptyset \text{ and its last pair is }
 ///     ( k - 1 , k )\ \textbf{do}\ \text{drop it} \cr
 /// 8 & \qquad \textbf{if}\ P = \emptyset\ \textbf{then return false} \cr
 /// 9 & \qquad \text{add 1 to the choice of the last pair of } P
 /// \end{array}}
 /// \f]
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

#include "ADS/complexity/impl/Nondeterminism.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Nondeterminism.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Nondeterminism.h ------------------------*/
/*--------------------------------------------------------------------------*/
