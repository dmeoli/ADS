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

namespace ADS
{

/// the simulation of `choice()`, success and failure
class Chooser
{

/*--------------------------------------------------------------------------*/
 public:
 /// the choice among \f$ \{ 0 , \ldots , k - 1 \} \f$ that the current
 /// sequence makes at this depth, 0 the first time the depth is reached
 ///
 /// In pseudocode (section 18.3), \f$ P \f$ the sequence of the pairs
 /// (choice, alternatives) and \f$ t \f$ the choices made in this run:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{choice}( k )\quad \triangleright\ P
 ///     \text{ the pairs (choice, arity) of the run, } t \text{ the next} \cr
 /// 2 & \quad \textbf{if}\ t = |P|\ \textbf{then}\ \text{append } ( 0 , k )
 ///     \text{ to } P\quad \triangleright\ \text{a new choice starts from }
 ///     0 \cr
 /// 3 & \quad t \gets t + 1;\ \textbf{return}\ \text{the choice of } P[ t ]
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
 /// 1 & \textbf{function}\ \mathrm{run}( \mathit{alg} )\quad \triangleright
 ///     \ \text{the runs in the lexicographic order of their choices} \cr
 /// 2 & \quad P \gets () \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad t \gets 0;\ \textbf{if}\ \mathit{alg}()
 ///     \ \textbf{then return true} \cr
 /// 5 & \qquad \text{keep the first } t \text{ pairs of } P
 ///     \text{, then drop from its end the pairs } ( k - 1 , k )
 ///     \text{ already at their last choice} \cr
 /// 6 & \qquad \textbf{if}\ P = ()\ \textbf{then return false}\quad
 ///     \triangleright\ \text{every run failed} \cr
 /// 7 & \qquad \text{raise by one the choice of the last pair of } P
 /// \end{array}}
 /// \f]
 template< class A >
 bool run( A && alg , long * runs = nullptr );

 /// the choices of the last run, the successful one after a YES
 [[nodiscard]] std::vector< int > choices( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 std::vector< std::pair< int , int > > f_path;  ///< ( choice , of how many )
 std::size_t f_depth = 0;                      ///< the choices made so far

 };  // end( class Chooser )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/impl/Nondeterminism.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Nondeterminism.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Nondeterminism.h ------------------------*/
/*--------------------------------------------------------------------------*/
