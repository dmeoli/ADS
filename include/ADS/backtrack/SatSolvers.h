/*--------------------------------------------------------------------------*/
/*--------------------------- File SatSolvers.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two solvers of the satisfiability of a formula in conjunctive normal form
 * [see Problems.h for the formula and the backtrack of the reductions],
 * after Knuth (7.2.2.2), beyond the book.
 *
 * - dpll() is the procedure of Davis, Putnam, Logemann and Loveland: unit
 *   propagation (a clause with all its literals false but one forces it),
 *   then a branch on a literal of a shortest open clause, and back on a
 *   contradiction; the propagation scans the clauses, simple and slow.
 * - Cdcl is conflict-driven clause learning (Algorithm 7.2.2.2C): each
 *   clause watches two of its literals, which need attention only when one
 *   becomes false, so that the propagation touches few clauses; a conflict
 *   is analyzed back to its first unique implication point, which gives a
 *   learned clause implied by the formula, and the search jumps back to the
 *   level where the clause forces its literal; the variables in the
 *   conflicts gain activity (VSIDS), and the decisions take the most
 *   active one, with the value it had last (phase saving); restarts follow
 *   the sequence of Luby, Sinclair and Zuckerman. The learned clauses are
 *   kept (no deletion), which suits formulae of moderate size.
 *
 * The contracts [see Contract.h]: the literals are \f$ \pm ( v + 1 ) \f$
 * with v in [ 0 , n ); a model returned passes satCertificate(), a
 * postcondition. A formula declared unsatisfiable has no certificate here
 * (a proof of the learned clauses, as DRAT, would be one). The functions
 * are defined in SatSolvers.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4B, Addison-Wesley,
 *      2022, section 7.2.2.2
 * \see M. Davis, G. Logemann, D. Loveland, A machine program for
 *      theorem-proving, Communications of the ACM 5(7), 394-397, 1962,
 *      doi:10.1145/368273.368557
 * \see J. P. Marques-Silva, K. A. Sakallah, GRASP: a search algorithm for
 *      propositional satisfiability, IEEE Transactions on Computers 48(5),
 *      506-521, 1999, doi:10.1109/12.769433
 * \see M. W. Moskewicz, C. F. Madigan, Y. Zhao, L. Zhang, S. Malik,
 *      Chaff: engineering an efficient SAT solver, DAC 2001, 530-535,
 *      doi:10.1145/378239.379017
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

#ifndef __ADS_SatSolvers
 #define __ADS_SatSolvers
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a model of F by DPLL, or nothing if F is unsatisfiable
/// \pre n >= 0 and the literals of F are variables of [ 0 , n )
/// \post a model returned passes satCertificate()
inline std::optional< std::vector< bool > > dpll( const Cnf & F , int n );

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Cdcl --------------------------------*/
/*--------------------------------------------------------------------------*/

/// conflict-driven clause learning with two watched literals
class Cdcl {

/*--------------------------------------------------------------------------*/
 public:

 enum class Result { SAT , UNSAT , UNKNOWN };

 /// \pre n >= 0 and the literals of F are variables of [ 0 , n )
 Cdcl( const Cnf & F , int n );

 /// the search, UNKNOWN after maxConflicts conflicts if that is >= 0
 /// \post SAT means that model() passes satCertificate()
 Result solve( long maxConflicts = -1 );

 /// the model found by solve()
 [[nodiscard]] const std::vector< bool > & model( void ) const;

 [[nodiscard]] long conflicts( void ) const { return( f_conflicts ); }
 [[nodiscard]] long decisions( void ) const { return( f_decisions ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the literal ±( v + 1 ) as 2 v + ( negative )
 static int code( int lit ) {
  return( 2 * ( std::abs( lit ) - 1 ) + ( lit < 0 ) );
  }

 /// -1 false, 0 free, 1 true
 [[nodiscard]] int value( int c ) const;

 void enqueue( int c , int reason );
 [[nodiscard]] int propagate( void );      ///< a conflict clause, or -1
 void analyze( int confl , std::vector< int > & learnt , int & back );
 void cancelUntil( int level );
 void bump( int v );
 [[nodiscard]] int pickBranch( void );     ///< a free variable, or -1
 [[nodiscard]] int level( void ) const { return( int( f_trailLim.size() ) ); }
 int addClause( std::vector< int > c );

 Cnf f_F;
 int f_n;
 bool f_empty = false;                      ///< an empty clause or a
                                            ///< conflict at level 0
 std::vector< std::vector< int > > f_C;     ///< the clauses, as codes
 std::vector< std::vector< int > > f_watch; ///< the clauses by literal
 std::vector< int > f_assign , f_level , f_reason;
 std::vector< bool > f_phase , f_seen , f_model;
 std::vector< int > f_trail , f_trailLim;
 std::size_t f_qhead = 0;
 std::vector< double > f_act;
 double f_inc = 1;
 std::priority_queue< std::pair< double , int > > f_heap;
 long f_conflicts = 0 , f_decisions = 0;

 };  // end( class Cdcl )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/SatSolvers.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SatSolvers.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SatSolvers.h --------------------------*/
/*--------------------------------------------------------------------------*/
